/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Pending-impact replay and two-process SRAM continuation harness.
 *
 * Replaces the normal main loop for one canonical race build, on the host or
 * as an ARM diagnostic ROM. The only thing it writes is the held/pressed keys
 * game_update() receives; world state is read for reports.
 *
 * Tape mode (SC64_TAPE_HEADER) replays a host-controller route recorded with
 * tests/sc64_input_tape_record.h. It reports a world digest every 100 input
 * ticks, the pending-impact load and update cycles per 1,000 play calls, and
 * the terminal result.
 *
 * Save mode (SC64_IMPACT_SAVE_MIN) starts a normal-menu Blood Bath skirmish
 * with three bots, plays neutral input until at least that many impacts are
 * pending, saves from the pause menu and records a neutral continuation keyed
 * by game frame. A second process launched with the same SRAM loads from HOME
 * with L; it must restore the saved world and repeat every continuation
 * digest. The host build runs both phases in one process with host SRAM,
 * which is the oracle for the ARM lines, not a fresh-process result.
 *
 * Every line starts with "IMPACT "; "IMPACT DONE" ends a phase. */
#include "../starcraft/game.c"
#ifdef GBA_HOST
#include <stdio.h>
#endif

#ifndef SC64_IMPACT_EVENTS
#error "Build the impact replay with a pending-impact race profile"
#endif
#if defined(SC64_TAPE_HEADER) == defined(SC64_IMPACT_SAVE_MIN)
#error "Define exactly one of SC64_TAPE_HEADER or SC64_IMPACT_SAVE_MIN"
#endif

volatile u16 demo_keys;

static u32 hash_word(u32 h, u32 v)
{
    for (unsigned i = 0; i < 4; ++i) { h ^= (u8)v; h *= 16777619u; v >>= 8; }
    return h;
}

/* World fields of tests/sc64_skirmish_resume.c plus the pending-impact rows
 * and the result flags. Selection, camera and menus are UI state and are not
 * saved, so they are excluded. */
static u32 world_digest(void)
{
    u32 h = 2166136261u;
#define MIX(v) (h = hash_word(h, (u32)(v)))
    MIX(frame); MIX(seed); MIX(minerals); MIX(gas); MIX(supply_used); MIX(supply_cap);
    MIX(train_type); MIX(weapons_level); MIX(armor_level);
    MIX(weapons_research); MIX(armor_research); MIX(won); MIX(lost);
    MIX(geyser_count);
    for (int i = 0; i < geyser_count; ++i) { MIX(geysers[i].x); MIX(geysers[i].y); }
    for (int cell = 0; cell < MAP_CELLS; ++cell) MIX(SC64_GRID_GET(explored, cell));
    for (int p = 0; p < 5; ++p) {
        MIX(race_player[p]); MIX(race_owner_minerals(p)); MIX(race_owner_gas(p));
    }
    for (int i = 0; i < MAX_MINERALS; ++i) { MIX(m[i].x); MIX(m[i].y); MIX(m[i].amount); }
    for (int i = 0; i < MAX_UNITS; ++i) {
        const Unit *q = &u[i];
        MIX(q->alive);
        if (!q->alive) continue;
        MIX(q->owner); MIX(q->type); MIX(q->x); MIX(q->y);
        MIX(q->hp); MIX(q->maxhp); MIX(q->order); MIX(q->tx); MIX(q->ty); MIX(q->carry);
        MIX(q->build); MIX(q->work_timer); MIX(q->target); MIX(q->path_x); MIX(q->path_y);
        MIX(q->cd); MIX(q->weapon); MIX(q->armor); MIX(q->path_timer); MIX(q->path_retry);
        MIX(q->shield); MIX(q->raw_type); MIX(q->raw_owner); MIX(q->flags);
        MIX(q->shield_remainder);
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i) {
        const Building *q = &b[i];
        MIX(q->alive);
        if (!q->alive) continue;
        MIX(q->owner); MIX(q->type); MIX(q->x); MIX(q->y);
        MIX(q->hp); MIX(q->maxhp); MIX(q->under_construction); MIX(q->build_progress);
        MIX(q->level); MIX(q->queue_count); MIX(q->queue_progress);
        for (int j = 0; j < q->queue_count && j < 5; ++j)
            MIX(q->queue[(q->queue_head + j) % 5]);
        MIX(q->shield); MIX(q->raw_type); MIX(q->raw_owner); MIX(q->flags);
        MIX(q->larva); MIX(q->larva_timer); MIX(q->shield_remainder);
    }
    MIX(race_impact_count);
    for (unsigned i = 0; i < race_impact_count; ++i) {
        const Sc64ImpactEvent *e = &race_impact_rows[i];
        MIX(e->target_meta); MIX(e->damage_hits); MIX(e->steps); MIX(e->source_type);
    }
#undef MIX
    return h;
}

/* ---- Output: host stdout, or the mGBA debug-log registers. ---- */
static char line[200];
static unsigned line_len;
static void put_text(const char *t) { while (*t && line_len < sizeof(line) - 1u) line[line_len++] = *t++; }
static void put_num(const char *key, u32 v)
{
    char t[10]; unsigned k = 0;
    put_text(" "); put_text(key); put_text("=");
    do { t[k++] = (char)('0' + v % 10u); v /= 10u; } while (v);
    while (k && line_len < sizeof(line) - 1u) line[line_len++] = t[--k];
}
static void put_hex(const char *key, u32 v)
{
    static const char d[] = "0123456789abcdef";
    put_text(" "); put_text(key); put_text("=");
    for (int i = 7; i >= 0 && line_len < sizeof(line) - 1u; --i) line[line_len++] = d[(v >> (i * 4)) & 15u];
}
static void begin(const char *tag) { line_len = 0; put_text("IMPACT "); put_text(tag); }
static void finish(void)
{
    line[line_len] = 0;
#ifdef GBA_HOST
    puts(line); fflush(stdout);
#else
    volatile u16 *enable = (volatile u16 *)0x04fff780u;
    volatile u16 *flags = (volatile u16 *)0x04fff700u;
    volatile char *text = (volatile char *)0x04fff600u;
    *enable = 0xc0de;
    /* Warning level, so mGBA's narrow -l 7 mask shows it. */
    if (*enable == 0x1dea) { for (unsigned i = 0; i <= line_len; ++i) text[i] = line[i]; *flags = 0x102; }
#endif
}
static void report_world(const char *tag, u32 tick)
{
    begin(tag); put_num("tick", tick); put_num("frame", (u32)frame);
    put_num("screen", (u32)screen); put_num("won", (u32)won); put_num("lost", (u32)lost);
    put_num("pending", race_impact_count); put_hex("digest", world_digest());
    finish();
}

/* ---- Input driver with pending-impact load and cycle windows. ---- */
static u16 previous;
static u32 calls, max_pending, pending_calls;
static unsigned long long pending_sum;
static unsigned long long window_cycles;
static u32 window_calls, window_peak, windows;

static void input(u16 held)
{
    int prior = screen;
    demo_keys = held;
    u32 start = gba_clock();
    game_update(held, held & (u16)~previous);
    audio_update(); game_render();
    u32 cycles = gba_clock() - start;
    previous = held; ++calls;
    if (race_impact_count > max_pending) max_pending = race_impact_count;
    pending_sum += race_impact_count; pending_calls += race_impact_count != 0;
    if (prior != SCREEN_PLAY || screen != SCREEN_PLAY) return;
    window_cycles += cycles; if (cycles > window_peak) window_peak = cycles;
    if (++window_calls == 1000u) {
        begin("WINDOW"); put_num("index", windows++); put_num("frame", (u32)frame);
        put_num("mean", (u32)(window_cycles / window_calls)); put_num("peak", window_peak);
        put_num("pending", race_impact_count); finish();
        window_cycles = 0; window_calls = window_peak = 0;
    }
}
static void tap(u16 key) { input(key); input(0); }

static void report_load(void)
{
    begin("LOAD"); put_num("calls", calls); put_num("max_pending", max_pending);
    put_num("high_water", race_impact_high_water); put_num("deferred", race_impact_deferred);
    put_num("pending_calls", pending_calls);
    put_num("mean_pending_x100", calls ? (u32)(pending_sum * 100u / calls) : 0);
    finish();
}

#ifdef GBA_HOST
static u32 stack_paint(void) { return 0; }
static void report_stack(u32 top) { (void)top; }
#else
static u32 stack_paint(void)
{
    u32 sp0; __asm__ volatile("mov %0, sp" : "=r"(sp0));
    u32 top = (sp0 - 512u) & ~3u;
    for (volatile u32 *p = (volatile u32 *)0x03005000u; (u32)p < top; ++p) *p = 0xa55aa55au;
    return top;
}
static void report_stack(u32 top)
{
    u32 low = top;
    for (volatile u32 *p = (volatile u32 *)0x03005000u; (u32)p < top; ++p)
        if (*p != 0xa55aa55au) { low = (u32)p; break; }
    begin("STACK"); put_num("margin", low - 0x03005000u); put_num("used", 0x03007e00u - low); finish();
}
#endif

static int fault(void)
{
    return mission_unsupported_count != 0;
}

#ifdef SC64_TAPE_HEADER
#include SC64_TAPE_HEADER /* sc64_tape[] = {tick, held}, SC64_TAPE_COUNT, SC64_TAPE_TICKS */

int main(void)
{
    gba_init(); u32 top = stack_paint(); game_init();
    unsigned next = 0; u16 held = 0;
    for (u32 tick = 1; tick <= SC64_TAPE_TICKS; ++tick) {
        while (next < SC64_TAPE_COUNT && sc64_tape[next].tick == tick) held = sc64_tape[next++].held;
        input(held);
        if (tick % 100u == 0u) report_world("SNAP", tick);
    }
    report_world("TERMINAL", SC64_TAPE_TICKS);
    begin("RESULT"); put_num("inputs", next); put_num("fault", (u32)fault());
    put_num("ok", next == SC64_TAPE_COUNT && won && !lost && !fault()); finish();
    report_load(); report_stack(top);
    begin("DONE"); finish();
    return won && !lost && !fault() ? 0 : 1;
}
#else

#ifndef SC64_IMPACT_SAVE_AFTER
#define SC64_IMPACT_SAVE_AFTER 600
#endif
#ifndef SC64_IMPACT_CONTINUE
#define SC64_IMPACT_CONTINUE 900
#endif
_Static_assert(SC64_IMPACT_SAVE_MIN >= 1 && SC64_IMPACT_SAVE_MIN <= RACE_IMPACT_SAVE_CAPACITY,
               "SC64_IMPACT_SAVE_MIN must fit this profile's saved impact rows");

static void to_row(int row)
{
    for (int step = 0; race_front.row != row && step < 16; ++step)
        tap(race_front.row < row ? KEY_DOWN : KEY_UP);
}
static void set_field(int *field, int value)
{
    for (int i = 0; *field != value && i < 3; ++i) tap(KEY_RIGHT);
}

/* Every frame while impacts from the save may still be pending, then every
 * 50 frames. Keyed by game frame, so pause and load calls do not shift it. */
static int checkpoint(int d) { return d <= 60 || d % 50 == 0; }

static int continue_from(int saved_frame)
{
    int last = frame;
    for (u32 guard = 0; frame < saved_frame + SC64_IMPACT_CONTINUE; ++guard) {
        if (guard > 4u * SC64_IMPACT_CONTINUE || screen != SCREEN_PLAY) return 0;
        input(0);
        if (frame != last && checkpoint(frame - saved_frame))
            report_world("CONT", (u32)(frame - saved_frame));
        last = frame;
    }
    return 1;
}

#ifdef SC64_IMPACT_SAVE_BATTLE
/* Constructed load: after the normal launch, the ARM timing probe's battle is
 * spawned so pending impacts pass this profile's saved-row capacity. */
enum { SKIRMISH_MAP = 0, SKIRMISH_BOTS = 1 };
static void spawn_group(int owner, int type, int count, int x0, int y0, int tx, int ty)
{
    for (int i = 0; i < count; ++i) {
        int slot = spawn_unit(owner, type, x0 + (i % 4) * 14, y0 + (i / 4) * 14);
        if (slot < 0) continue;
        u[slot].raw_owner = owner == SIDE ? 0 : 1;
        u[slot].order = ORDER_ATTACK_MOVE; u[slot].target = -1;
        u[slot].tx = tx; u[slot].ty = ty;
    }
}
static void spawn_battle(void)
{
    spawn_group(SIDE, U_DRAGOON, 16, 220, 260, 420, 290);
    spawn_group(SIDE, U_GOLIATH, 8, 220, 330, 420, 290);
    spawn_group(ENEMY, U_MUTALISK, 14, 400, 250, 220, 290);
    spawn_group(ENEMY, U_HYDRALISK, 10, 400, 320, 220, 290);
}
#else
enum { SKIRMISH_MAP = 1, SKIRMISH_BOTS = 3 };
#endif

static int sram_has_record(void);

static int save_phase(void)
{
    int human = SC64_CAMPAIGN_RACE, refused = 0;
    tap(KEY_DOWN); tap(KEY_A); /* HOME -> Custom Game -> Skirmish */
    if (screen != SCREEN_TITLE || race_front.page != SC_FRONT_SKIRMISH) return 10;
    to_row(0); set_field(&race_front.map, SKIRMISH_MAP);
    to_row(1); set_field(&race_front.race, human);
    to_row(2); set_field(&race_front.bots, SKIRMISH_BOTS);
    for (int i = 0; i < SKIRMISH_BOTS; ++i) {
        to_row(3 + i); set_field(&race_front.bot_races[i], (human + 1 + i) % 3);
    }
    to_row(3 + SKIRMISH_BOTS); tap(KEY_A);
    if (screen != SCREEN_PLAY || !race_mode || race_skirmish_map != SKIRMISH_MAP ||
        race_bots != SKIRMISH_BOTS) return 11;
#ifdef SC64_IMPACT_SAVE_BATTLE
    spawn_battle();
#endif
    report_world("LAUNCH", calls);
    for (u32 guard = 0;; ++guard) {
        if (guard > 60000u || screen != SCREEN_PLAY) return 12;
        input(0);
#ifdef SC64_IMPACT_SAVE_BATTLE
        /* Over capacity, the pause-menu save must refuse before any SRAM write. */
        if (!refused && race_impact_count > RACE_IMPACT_SAVE_CAPACITY) {
            u32 pending = race_impact_count;
            tap(KEY_START);
            if (screen != SCREEN_PAUSE) return 16;
            input(KEY_B);
            if ((pause_notice & 0xc0) != UI_NOTICE_SAVE_FAILED || sram_has_record()) return 17;
            begin("REFUSED"); put_num("frame", (u32)frame); put_num("pending", pending); finish();
            input(KEY_A);
            refused = 1;
            continue;
        }
#endif
#ifdef SC64_IMPACT_SAVE_BATTLE
        if (!refused) continue; /* refuse once over capacity, then save within it */
#endif
        if (frame >= SC64_IMPACT_SAVE_AFTER && race_impact_count >= SC64_IMPACT_SAVE_MIN &&
            race_impact_count <= RACE_IMPACT_SAVE_CAPACITY) break;
    }
    (void)refused;
    tap(KEY_START);
    if (screen != SCREEN_PAUSE) return 13;
    input(KEY_B); /* SRAM write; pause runs no play tick. */
    if ((pause_notice & 0xc0) != UI_NOTICE_SAVED) return 14;
    int saved_frame = frame;
    report_world("SAVED", calls);
    input(KEY_A); /* resume; the release below is the first play tick */
    return continue_from(saved_frame) ? 0 : 15;
}

static int load_phase(void)
{
    input(KEY_L); /* HOME load returns to play without a tick */
    if (screen != SCREEN_PLAY || !race_mode || race_skirmish_map != SKIRMISH_MAP ||
        race_bots != SKIRMISH_BOTS) return 20;
    int saved_frame = frame;
    report_world("RESTORED", calls);
    return continue_from(saved_frame) ? 0 : 21;
}

static int sram_has_record(void)
{
#ifdef GBA_HOST
    return 0;
#else
    volatile const u8 *s = (volatile const u8 *)0x0e000000u;
    for (unsigned slot = 0; slot < 2; ++slot) {
        u32 tag = 0;
        for (unsigned i = 0; i < 4; ++i) tag |= (u32)s[slot * 16384u + i] << (8 * i);
        if (tag == SAVE_VERSION) return 1;
    }
    return 0;
#endif
}

static int run_phase(int load)
{
    u32 top = stack_paint();
    int rc;
    calls = max_pending = pending_calls = 0; pending_sum = 0;
    window_cycles = 0; window_calls = window_peak = windows = 0;
    game_init(); previous = 0;
    begin("PHASE"); put_num("load", (u32)load); put_num("capacity", RACE_IMPACT_SAVE_CAPACITY);
    put_num("save_min", SC64_IMPACT_SAVE_MIN); put_num("tag", SAVE_VERSION); finish();
    rc = load ? load_phase() : save_phase();
    begin("RESULT"); put_num("load", (u32)load); put_num("rc", (u32)rc);
    put_num("fault", (u32)fault()); put_num("ok", !rc && !fault()); finish();
    report_load(); report_stack(top);
    begin("DONE"); finish();
    return rc;
}

int main(void)
{
    gba_init();
#ifdef GBA_HOST
    /* Host oracle: the same two phases share in-process host SRAM. */
    int rc = run_phase(0);
    return rc ? rc : run_phase(1);
#else
    return run_phase(sram_has_record());
#endif
}
#endif
