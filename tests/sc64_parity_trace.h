/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * Parity trace emitter: one record set per simulation step, read-only.
 *
 * Include this header AFTER "../starcraft/game.c" and "sc64_route_purity.h",
 * in a host build that defined, BEFORE including game.c, the three hooks
 * SC64_TEST_HIT_OBSERVER, GAME_AUDIO_REQUEST and SC64_TEXT_BOX_POSTED as
 * calls to sc64_pt_hit(), sc64_pt_sound() and sc64_pt_text_box()
 * (tests/sc64_parity_run.c shows how). Host test code only.
 *
 * The emitter never writes engine state. It reads named fields only, so the
 * same schema holds for every profile even though the Unit and Building
 * layouts change with the flag vector. Presentation fields (facing, walk,
 * the death pool and sound requests) advance only while game_render() runs,
 * so a trace is meaningful only from a run that renders after every update,
 * as common/main.c does.
 *
 * Format "sc64-parity-trace/1": UTF-8 text, one record per line, fields
 * separated by one space, integers in decimal unless noted. The first field
 * names the record. Unit and building rows are deltas: a row is written when
 * any field of that slot differs from the last row written for it, and a
 * reader keeps the last row per slot until a removal record. Records between
 * two S lines belong to the step of the S line before them.
 *
 *   #sc64-parity-trace 1                       first line
 *   H <key>=<value> ...                        run header (profile, scenario,
 *        capacities, map size, attack verb mode, time rule)
 *   S step sf host_tick frame screen won lost box_outcome phase countdown boxes
 *        A simulation step was taken. step counts steps from 1; sf is the
 *        source frame, step*4/5 rounded down (5 updates per 4 source frames);
 *        host_tick counts every game_update() call, menus and text boxes
 *        included; frame is the engine counter. box_outcome is the outcome a
 *        text box holds back (0 none, 1 won, 2 lost); phase is the race
 *        campaign phase (-1 outside SC64_RACES); countdown is the mission
 *        clock in updates (-1 when the mission has none); boxes is the number
 *        of open text boxes.
 *   W host_tick frame screen                   an update that took no step but
 *        produced records (text boxes, the result screen)
 *   U slot type raw_type owner raw_owner x y hp maxhp shield energy order tx ty
 *     target cd carry build work_timer flags facing walk anim
 *        Unit row. Missing fields are -1 (raw_type, raw_owner, shield and flags
 *        outside SC64_RACES; energy for a unit the HUD shows none for; facing
 *        without SC64_UNIT_FACING). facing is 0..31 clockwise from north. walk
 *        is the stride phase 0..4 implied by the walk bits after the draw; on
 *        the update a unit stops it can read 0 where the painter drew a last
 *        stride. anim is a derived
 *        kind: 0 stand, 1 move, 2 fired this step (cooldown rose), 3 working
 *        (mine, gas, build, repair), 4 carried (in a transport).
 *   B slot type raw_type owner raw_owner x y hp maxhp shield under_construction
 *     build_progress level queue_count queue_progress q0 q1 q2 q3 q4 larva flags
 *        Building row; q0..q4 are the queue from its head, -1 when empty.
 *   +U slot type owner / -U slot type owner hp     unit birth and removal
 *   +B slot type owner / -B slot type owner hp     building birth and removal
 *   OU slot old new / OB slot old new          owner change of a live actor
 *   TU slot old new / TB slot old new          type change of a live actor
 *        (a morph; a slot freed and refilled inside one step reads the same)
 *   P owner minerals gas supply_used supply_cap    bank row, owners 0..4
 *        (owner 0 only outside SC64_RACES), written when it changes
 *   M index x y amount                         mineral field, when it changes
 *   FV cols rows hex / FE cols rows hex        human visible / explored set
 *        over the active map (cols x rows source tiles), written on the first
 *        step and when the map size changes. Cell c = col + row * cols is bit
 *        c of the bitmap: eight cells per byte, low bit first, two hex digits
 *        per byte.
 *   F cell visible explored                    one fog cell that changed
 *   V row style raw x y birth                  death-effect pool row changed;
 *        "V row 0" when the row is freed. birth is the engine frame.
 *   D target damage source_type source_owner   a hit as the damage path saw
 *        it (SC64_TEST_HIT_OBSERVER), before shields, armour and hallucination
 *        scaling; target is a unit slot or MAX_UNITS + a building slot
 *   A kind raw slot x y on_screen              a sound the observer asked for,
 *        before the on-screen, budget and voice gates; Protoss warp-start
 *        requests (game_audio_foundation) are not recorded (kind is the
 *        GAME_AUDIO_* number; for ALERT, raw is the alert line; for ADVISOR,
 *        the refusal cue; slot is a unit
 *        slot, MAX_UNITS + building, or -1; x y are -1 when it has no place)
 *   X trigger action string_id opened          a mission posted a text-box row
 *        that was not already open or queued; opened is 1 when it joined the
 *        queue, 0 when the queue had no room (a repeat post of an open or
 *        queued row leaves no record)
 *   I verb result detail...                    an injected command (result 0
 *        ok, -1 setup failure); detail is free text to the end of the line
 *   Z key=value ...                            last line: totals and hashes
 *
 * Event buffers are bounded; an overflow is counted in the Z line, never
 * silently dropped. A trace is complete only when its last line is Z with
 * overflow=0 and write_errors=0.
 */
#ifndef SC64_PARITY_TRACE_H
#define SC64_PARITY_TRACE_H
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#ifndef SC64_ROUTE_PURITY_H
#error "include tests/sc64_route_purity.h before sc64_parity_trace.h"
#endif

enum { SC64_PT_UNIT_FIELDS = 23, SC64_PT_BUILDING_FIELDS = 22, SC64_PT_EVENT_CAP = 8192 };

#if defined(SC64_LARGE_MAPS) || defined(SC64_ZP_CAMPAIGN_STORAGE)
#define SC64_PT_CELL_CAP (SC64_GRID_STORAGE_BYTES * 8u)
#else
#define SC64_PT_CELL_CAP ((unsigned)SC64_GRID_STORAGE_BYTES)
#endif

typedef struct { char tag; int v[6]; } Sc64PtEvent;

static struct {
    FILE *out;
    uint64_t fnv;
    unsigned long long lines;
    int started, write_errors;
    unsigned cells, cols, rows;
    int unit_row[MAX_UNITS][SC64_PT_UNIT_FIELDS];
    int unit_alive[MAX_UNITS], unit_cd[MAX_UNITS];
    int building_row[MAX_BUILDINGS][SC64_PT_BUILDING_FIELDS];
    int building_alive[MAX_BUILDINGS];
    int mineral_row[MAX_MINERALS][3];
    int bank_row[5][4];
    u8 vis[SC64_PT_CELL_CAP], exp_[SC64_PT_CELL_CAP];
#ifndef SC64_DEATH_VFX_DISABLED
    Sc64DeathVfx vfx[SC64_DEATH_VFX_POOL];
#endif
    Sc64PtEvent events[SC64_PT_EVENT_CAP];
    char verbs[64][200];
    unsigned event_count, verb_count;
    unsigned long long event_overflow, damage, sounds, boxes, births, removals;
} sc64_pt;

/* The emitter's own route-purity check around each step; a runner measuring
 * throughput may turn it off. */
static int sc64_pt_route_checks = 1;

static void sc64_pt_write(const char *format, ...)
{
    char line[9000];
    va_list args;
    int n;
    if (!sc64_pt.out) return;
    va_start(args, format);
    n = vsnprintf(line, sizeof(line), format, args);
    va_end(args);
    if (n < 0 || (size_t)n >= sizeof(line)) { fputs("parity trace: line overflow\n", stderr); abort(); }
    for (int i = 0; i < n; ++i) { sc64_pt.fnv ^= (unsigned char)line[i]; sc64_pt.fnv *= 1099511628211ull; }
    if (fwrite(line, 1, (size_t)n, sc64_pt.out) != (size_t)n) ++sc64_pt.write_errors;
    sc64_pt.lines += line[n - 1] == '\n';
}

/* ---- Hooks (called from inside the engine) ---------------------------- */

static void sc64_pt_event(char tag, int a, int b0, int c, int d, int e, int f)
{
    Sc64PtEvent *ev;
    if (!sc64_pt.out) return;
    if (sc64_pt.event_count >= SC64_PT_EVENT_CAP) { ++sc64_pt.event_overflow; return; }
    ev = &sc64_pt.events[sc64_pt.event_count++];
    ev->tag = tag; ev->v[0] = a; ev->v[1] = b0; ev->v[2] = c; ev->v[3] = d; ev->v[4] = e; ev->v[5] = f;
}

static void sc64_pt_hit(int target, int damage, int source_type, int source_owner)
{
    ++sc64_pt.damage;
    sc64_pt_event('D', target, damage, source_type, source_owner, 0, 0);
}

static void sc64_pt_sound(int kind, int raw, int slot, int x, int y)
{
    int on = x >= 0 && game_audio_on_screen(x, y);
    ++sc64_pt.sounds;
    sc64_pt_event('A', kind, raw, slot, x, y, on);
}

static int sc64_pt_text_box(unsigned trigger, unsigned action, unsigned long string_id, int opened)
{
    ++sc64_pt.boxes;
    sc64_pt_event('X', (int)trigger, (int)action, (int)string_id, opened != 0, 0, 0);
    return opened;
}

/* Install as sc64_pi_on_verb to log injected commands. */
static void sc64_pt_verb(const char *verb, int result, const char *detail)
{
    if (!sc64_pt.out) return;
    if (sc64_pt.verb_count >= sizeof(sc64_pt.verbs) / sizeof(sc64_pt.verbs[0])) {
        ++sc64_pt.event_overflow;
        return;
    }
    snprintf(sc64_pt.verbs[sc64_pt.verb_count++], sizeof(sc64_pt.verbs[0]), "I %s %d %s\n",
             verb, result, detail);
}

/* ---- Readers ---------------------------------------------------------- */

static int sc64_pt_countdown(void)
{
#ifdef SC64_RACES
    /* The clocks hud_panel_clock() shows, in updates. */
    if (race_mode) return -1;
    if (race_selected_mission == 3)
        return race_campaign.elapsed_frames < 27000u ? (int)(27000u - race_campaign.elapsed_frames) : 0;
#ifdef SC64_T3_ENABLED
    if (race_selected_mission == 6) return (int)race_t3.remaining_ticks;
#endif
#ifdef SC64_Z4_RUNTIME
    if (race_selected_mission == 15) return (int)race_z4.countdown_ticks;
#endif
#endif
    return -1;
}

static int sc64_pt_walk(const Unit *q)
{
    unsigned slot = (unsigned)(q - u);
    unsigned bits = SC64_WALK_BITS[slot];
    unsigned stepped = (bits & SC64_WALK_KEY_MASK) != sc64_walk_ground_key(q);
#ifdef SC64_RACES
    if (sc64_race_unit_flying(q->type)) return sc64_walk_flyer_phase(q);
#endif
    return sc64_walk_ground_phase(q, bits, stepped);
}

static void sc64_pt_unit_row(int i, int *r)
{
    const Unit *q = &u[i];
    int walk = sc64_pt_walk(q), anim = 0;
#ifdef SC64_RACES
    r[0] = q->type; r[1] = q->raw_type; r[2] = q->owner; r[3] = q->raw_owner;
#else
    r[0] = q->type; r[1] = -1; r[2] = q->owner; r[3] = -1;
#endif
    r[4] = q->x; r[5] = q->y; r[6] = q->hp; r[7] = q->maxhp;
#ifdef SC64_RACES
    r[8] = q->shield;
#else
    r[8] = -1;
#endif
#ifdef SC64_HUD_PANEL
    r[9] = hud_unit_energy(q);
#else
    r[9] = -1;
#endif
    r[10] = q->order; r[11] = q->tx; r[12] = q->ty; r[13] = q->target; r[14] = q->cd;
    r[15] = q->carry; r[16] = q->build; r[17] = q->work_timer;
#ifdef SC64_RACES
    r[18] = q->flags;
#else
    r[18] = -1;
#endif
#ifdef SC64_UNIT_FACING
    r[19] = sc64_facing[i] & SC64_FACING_MASK;
#else
    r[19] = -1;
#endif
    r[20] = walk;
    if (SC64_TRANSPORT_LOADED(q)) anim = 4;
    else if (q->cd > sc64_pt.unit_cd[i]) anim = 2;
    else if (walk) anim = 1;
    else if (q->order == ORDER_MINE || q->order == ORDER_GAS || q->order == ORDER_BUILD ||
             q->order == ORDER_BUILD_PENDING
#ifdef SC64_T6_RUNTIME
             || q->order == ORDER_REPAIR
#endif
             ) anim = 3;
    r[21] = anim;
    r[22] = 0; /* reserved, always 0 in schema 1; not written */
}

static void sc64_pt_building_row(int i, int *r)
{
    const Building *q = &b[i];
    int k;
#ifdef SC64_RACES
    r[0] = q->type; r[1] = q->raw_type; r[2] = q->owner; r[3] = q->raw_owner;
#else
    r[0] = q->type; r[1] = -1; r[2] = q->owner; r[3] = -1;
#endif
    r[4] = q->x; r[5] = q->y; r[6] = q->hp; r[7] = q->maxhp;
#ifdef SC64_RACES
    r[8] = q->shield;
#else
    r[8] = -1;
#endif
    r[9] = q->under_construction; r[10] = q->build_progress; r[11] = q->level;
    r[12] = q->queue_count; r[13] = q->queue_progress;
    for (k = 0; k < 5; ++k)
        r[14 + k] = k < q->queue_count ? (int)q->queue[(q->queue_head + k) % 5] : -1;
#ifdef SC64_RACES
    r[19] = q->larva; r[20] = q->flags;
#else
    r[19] = -1; r[20] = -1;
#endif
    r[21] = 0; /* reserved, not written */
}

static void sc64_pt_bank_row(int owner, int *r)
{
#ifdef SC64_RACES
    r[0] = race_owner_minerals(owner); r[1] = race_owner_gas(owner);
    r[2] = race_owner_supply(owner); r[3] = race_owner_supply_cap(owner);
#else
    r[0] = minerals; r[1] = gas; r[2] = supply_used; r[3] = supply_cap;
    (void)owner;
#endif
}

/* ---- Emission --------------------------------------------------------- */

static void sc64_pt_open(FILE *out, const char *profile, const char *scenario, const char *attack_mode)
{
    memset(&sc64_pt, 0, sizeof(sc64_pt));
    sc64_pt.fnv = 1469598103934665603ull;
    sc64_pt.out = out;
    sc64_pt_write("#sc64-parity-trace 1\n");
    sc64_pt_write("H schema=sc64-parity-trace/1 profile=%s scenario=%s attack=%s\n",
                  profile, scenario, attack_mode);
    sc64_pt_write("H max_units=%d max_buildings=%d max_minerals=%d owners=%d cell_cap=%u\n",
                  MAX_UNITS, MAX_BUILDINGS, MAX_MINERALS,
#ifdef SC64_RACES
                  5,
#else
                  1,
#endif
                  (unsigned)SC64_PT_CELL_CAP);
    sc64_pt_write("H time=steps sf=step*4/5 updates_per_source_frame=5/4\n");
}

static void sc64_pt_write_bitmap(const char *tag, const u8 *bits, unsigned cells, unsigned cols, unsigned rows)
{
    static char hex[2 * (SC64_PT_CELL_CAP / 8u + 1u) + 1u];
    static const char digits[] = "0123456789abcdef";
    unsigned bytes = (cells + 7u) / 8u, i;
    for (i = 0; i < bytes; ++i) {
        unsigned v = 0;
        for (unsigned k = 0; k < 8u && i * 8u + k < cells; ++k) v |= (unsigned)(bits[i * 8u + k] & 1u) << k;
        hex[2 * i] = digits[v >> 4]; hex[2 * i + 1] = digits[v & 15u];
    }
    hex[2 * bytes] = 0;
    /* Long lines are split by length only, never by meaning: one write each. */
    if (sc64_pt.out) {
        sc64_pt_write("%s %u %u ", tag, cols, rows);
        for (i = 0; i < 2 * bytes; i += 4096) {
            char chunk[4097];
            unsigned n = 2 * bytes - i < 4096 ? 2 * bytes - i : 4096;
            memcpy(chunk, hex + i, n); chunk[n] = 0;
            sc64_pt_write("%s", chunk);
        }
        sc64_pt_write("\n");
    }
}

static void sc64_pt_fog(void)
{
    /* MAP_CELLS is storage capacity in race profiles; read the active map. */
    unsigned cols = (unsigned)MAP_COLS, rows = (unsigned)MAP_ROWS, cells = cols * rows, c;
    if (cells > SC64_PT_CELL_CAP) { fputs("parity trace: map larger than the fog grid\n", stderr); abort(); }
    if (!sc64_pt.started || cols != sc64_pt.cols || rows != sc64_pt.rows) {
        sc64_pt.cells = cells; sc64_pt.cols = cols; sc64_pt.rows = rows;
        for (c = 0; c < cells; ++c) {
            sc64_pt.vis[c] = (u8)SC64_GRID_GET(visible, c);
            sc64_pt.exp_[c] = (u8)SC64_GRID_GET(explored, c);
        }
        sc64_pt_write_bitmap("FV", sc64_pt.vis, cells, cols, rows);
        sc64_pt_write_bitmap("FE", sc64_pt.exp_, cells, cols, rows);
        return;
    }
    for (c = 0; c < cells; ++c) {
        u8 v = (u8)SC64_GRID_GET(visible, c), e = (u8)SC64_GRID_GET(explored, c);
        if (v != sc64_pt.vis[c] || e != sc64_pt.exp_[c]) {
            sc64_pt.vis[c] = v; sc64_pt.exp_[c] = e;
            sc64_pt_write("F %u %u %u\n", c, v, e);
        }
    }
}

static void sc64_pt_actors(void)
{
    int row[SC64_PT_UNIT_FIELDS > SC64_PT_BUILDING_FIELDS ? SC64_PT_UNIT_FIELDS : SC64_PT_BUILDING_FIELDS];
    for (int i = 0; i < MAX_UNITS; ++i) {
        int alive = u[i].alive != 0, was = sc64_pt.unit_alive[i];
        if (!alive && !was) { sc64_pt.unit_cd[i] = u[i].cd; continue; }
        if (!alive) {
            sc64_pt_write("-U %d %d %d %d\n", i, sc64_pt.unit_row[i][0], sc64_pt.unit_row[i][2], u[i].hp);
            ++sc64_pt.removals;
            sc64_pt.unit_alive[i] = 0;
            sc64_pt.unit_cd[i] = u[i].cd;
            continue;
        }
        sc64_pt_unit_row(i, row);
        if (!was) {
            sc64_pt_write("+U %d %d %d\n", i, row[0], row[2]);
            ++sc64_pt.births;
        } else {
            if (row[2] != sc64_pt.unit_row[i][2]) sc64_pt_write("OU %d %d %d\n", i, sc64_pt.unit_row[i][2], row[2]);
            if (row[0] != sc64_pt.unit_row[i][0]) sc64_pt_write("TU %d %d %d\n", i, sc64_pt.unit_row[i][0], row[0]);
        }
        if (!was || memcmp(row, sc64_pt.unit_row[i], sizeof(sc64_pt.unit_row[i]))) {
            sc64_pt_write("U %d", i);
            for (int k = 0; k < SC64_PT_UNIT_FIELDS - 1; ++k) sc64_pt_write(" %d", row[k]);
            sc64_pt_write("\n");
            memcpy(sc64_pt.unit_row[i], row, sizeof(sc64_pt.unit_row[i]));
        }
        sc64_pt.unit_alive[i] = 1;
        sc64_pt.unit_cd[i] = u[i].cd;
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i) {
        int alive = b[i].alive != 0, was = sc64_pt.building_alive[i];
        if (!alive && !was) continue;
        if (!alive) {
            sc64_pt_write("-B %d %d %d %d\n", i, sc64_pt.building_row[i][0], sc64_pt.building_row[i][2], b[i].hp);
            ++sc64_pt.removals;
            sc64_pt.building_alive[i] = 0;
            continue;
        }
        sc64_pt_building_row(i, row);
        if (!was) {
            sc64_pt_write("+B %d %d %d\n", i, row[0], row[2]);
            ++sc64_pt.births;
        } else {
            if (row[2] != sc64_pt.building_row[i][2]) sc64_pt_write("OB %d %d %d\n", i, sc64_pt.building_row[i][2], row[2]);
            if (row[0] != sc64_pt.building_row[i][0]) sc64_pt_write("TB %d %d %d\n", i, sc64_pt.building_row[i][0], row[0]);
        }
        if (!was || memcmp(row, sc64_pt.building_row[i], sizeof(sc64_pt.building_row[i]))) {
            sc64_pt_write("B %d", i);
            for (int k = 0; k < SC64_PT_BUILDING_FIELDS - 1; ++k) sc64_pt_write(" %d", row[k]);
            sc64_pt_write("\n");
            memcpy(sc64_pt.building_row[i], row, sizeof(sc64_pt.building_row[i]));
        }
        sc64_pt.building_alive[i] = 1;
    }
}

static void sc64_pt_economy(void)
{
#ifdef SC64_RACES
    const int owners = 5;
#else
    const int owners = 1;
#endif
    for (int o = 0; o < owners; ++o) {
        int r[4];
        sc64_pt_bank_row(o, r);
        if (!sc64_pt.started || memcmp(r, sc64_pt.bank_row[o], sizeof(r))) {
            sc64_pt_write("P %d %d %d %d %d\n", o, r[0], r[1], r[2], r[3]);
            memcpy(sc64_pt.bank_row[o], r, sizeof(r));
        }
    }
    for (int i = 0; i < MAX_MINERALS; ++i) {
        int r[3] = { m[i].x, m[i].y, m[i].amount };
        if ((!sc64_pt.started && (r[0] || r[1] || r[2])) || (sc64_pt.started && memcmp(r, sc64_pt.mineral_row[i], sizeof(r))))
            sc64_pt_write("M %d %d %d %d\n", i, r[0], r[1], r[2]);
        memcpy(sc64_pt.mineral_row[i], r, sizeof(r));
    }
}

static void sc64_pt_presentation(void)
{
#ifndef SC64_DEATH_VFX_DISABLED
    for (int i = 0; i < SC64_DEATH_VFX_POOL; ++i) {
        Sc64DeathVfx row;
        death_vfx_test_load_row(i, &row);
        if (row.style != sc64_pt.vfx[i].style || row.raw != sc64_pt.vfx[i].raw ||
            row.x != sc64_pt.vfx[i].x || row.y != sc64_pt.vfx[i].y || row.birth != sc64_pt.vfx[i].birth) {
            if (row.style)
                sc64_pt_write("V %d %u %u %d %d %lu\n", i, (unsigned)row.style, (unsigned)row.raw,
                              (int)row.x, (int)row.y, (unsigned long)row.birth);
            else
                sc64_pt_write("V %d 0\n", i);
            sc64_pt.vfx[i] = row;
        }
    }
#endif
}

static void sc64_pt_flush_events(void)
{
    for (unsigned i = 0; i < sc64_pt.event_count; ++i) {
        const Sc64PtEvent *ev = &sc64_pt.events[i];
        switch (ev->tag) {
        case 'D': sc64_pt_write("D %d %d %d %d\n", ev->v[0], ev->v[1], ev->v[2], ev->v[3]); break;
        case 'A': sc64_pt_write("A %d %d %d %d %d %d\n", ev->v[0], ev->v[1], ev->v[2], ev->v[3], ev->v[4], ev->v[5]); break;
        default: sc64_pt_write("X %d %d %d %d\n", ev->v[0], ev->v[1], ev->v[2], ev->v[3]); break;
        }
    }
    sc64_pt.event_count = 0;
}

static void sc64_pt_flush_verbs(void)
{
    for (unsigned i = 0; i < sc64_pt.verb_count; ++i) sc64_pt_write("%s", sc64_pt.verbs[i]);
    sc64_pt.verb_count = 0;
}

static int sc64_pt_box_outcome(void)
{
#ifdef SC64_TEXT_BOXES
    return sc64_text_box_outcome.result;
#else
    return 0;
#endif
}

static int sc64_pt_boxes_open(void)
{
#ifdef SC64_TEXT_BOXES
    return (int)sc64_text_box_count();
#else
    return 0;
#endif
}

/* Records for one simulation step. Call after game_update() (and
 * game_render()) returned and the step was taken. */
static void sc64_pt_step(unsigned long step, unsigned long host_tick)
{
    uint64_t route = 0;
    if (!sc64_pt.out) return;
    if (sc64_pt_route_checks) route = route_purity_hash();
    sc64_pt_write("S %lu %lu %lu %d %d %d %d %d %d %d %d\n", step, step * 4ul / 5ul, host_tick,
                  (int)frame, screen, won, lost, sc64_pt_box_outcome(),
#ifdef SC64_RACES
                  (int)race_campaign.phase,
#else
                  -1,
#endif
                  sc64_pt_countdown(), sc64_pt_boxes_open());
    sc64_pt_flush_verbs();
    sc64_pt_actors();
    sc64_pt_economy();
    sc64_pt_fog();
    sc64_pt_presentation();
    sc64_pt_flush_events();
    sc64_pt.started = 1;
    if (sc64_pt_route_checks && route_purity_hash() != route) {
        fputs("parity trace: emitter changed route state\n", stderr);
        abort();
    }
}

/* An update that took no step: write what it produced, if anything. */
static void sc64_pt_wait(unsigned long host_tick)
{
    if (!sc64_pt.out || (!sc64_pt.event_count && !sc64_pt.verb_count)) return;
    sc64_pt_write("W %lu %d %d\n", host_tick, (int)frame, screen);
    sc64_pt_flush_verbs();
    sc64_pt_flush_events();
}

/* Returns nonzero when the trace is incomplete: a write failed or a buffer
 * overflowed. */
static int sc64_pt_close(unsigned long steps, unsigned long host_ticks, uint64_t world, uint64_t chain)
{
    uint64_t body;
    if (!sc64_pt.out) return 0;
    sc64_pt_flush_verbs();
    sc64_pt_flush_events();
    body = sc64_pt.fnv;
    sc64_pt_write("Z steps=%lu host_ticks=%lu births=%llu removals=%llu damage=%llu sounds=%llu "
                  "text_boxes=%llu overflow=%llu write_errors=%d world=%016llx world_chain=%016llx "
                  "body_fnv=%016llx\n",
                  steps, host_ticks, sc64_pt.births, sc64_pt.removals, sc64_pt.damage, sc64_pt.sounds,
                  sc64_pt.boxes, sc64_pt.event_overflow, sc64_pt.write_errors, (unsigned long long)world,
                  (unsigned long long)chain, (unsigned long long)body);
    if (fflush(sc64_pt.out)) ++sc64_pt.write_errors;
    sc64_pt.out = 0;
    return sc64_pt.write_errors || sc64_pt.event_overflow;
}

#endif
