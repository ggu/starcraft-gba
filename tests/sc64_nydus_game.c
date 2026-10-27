/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * Actual-engine Nydus Canal fixture, hosted by the authenticated Z7 runtime.
 *
 * Z7 itself has no canals. Each scenario launches Z7 through the ordinary
 * menu, then places two linked raw-134 endpoint pairs as a mission adapter
 * would place Z8's preplaced rows, and installs a synthetic source table
 * through the public integration hook. Every passenger then moves only
 * through ordinary input (cursor + A) and game_update(). Damage uses the
 * engine's own hit path. This is host proof, not emulator or hardware proof.
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/sc64_nydus_rules.h"

/* Profile hook, as a Z8 build would supply from its generated source rows. */
static int hook_mission = -1;
static const Sc64NydusSourceRecord *hook_records;
static int test_nydus_source_for(int mission, const Sc64NydusSourceRecord **records,
                                 unsigned *count)
{
    if (mission != hook_mission || !hook_records) return 0;
    *records = hook_records;
    *count = 4;
    return 1;
}
#define SC64_NYDUS_SOURCE_FOR_MISSION(mission, records, count) \
    test_nydus_source_for((mission), (records), (count))
#include "../starcraft/game.c"

#if !defined(SC64_NYDUS_ENABLED) || !defined(SC64_Z7_RUNTIME)
#error "Compile with the Z7 runtime and the optional Nydus feature gate"
#endif

static unsigned held_keys;
static Sc64NydusSourceRecord records[4];
static int ent, ex, ent2, ex2;

static void input(unsigned held)
{
    game_update((u16)held, (u16)(held & ~held_keys));
    held_keys = held;
}

static void press(unsigned key)
{
    input(key);
    input(0);
}

static void step(int frames)
{
    for (int i = 0; i < frames; ++i) input(0);
}

static void launch_z7_normally(void)
{
    game_init();
    held_keys = 0;
    press(KEY_A);
    assert(race_front.page == SC_FRONT_MISSION &&
           race_front.faction == SC_FRONT_ZERG);
    int ordinal = sc64_catalog_playable_ordinal(SC64_FACTION_ZERG,
                                                 SC64_Z7_GLOBAL_ID);
    assert(ordinal >= 0);
    for (int page = 0; page < ordinal / 2; ++page) press(KEY_RIGHT);
    if (ordinal & 1) press(KEY_DOWN);
    press(KEY_A);
    press(KEY_A);
    assert(screen == SCREEN_PLAY && race_z7_active());
    step(4);
}

static int open_site(int near_x, int near_y)
{
    int best = -1, best_d = 0x7fffffff;
    for (int y = near_y - 320; y <= near_y + 320; y += 16)
        for (int x = near_x - 320; x <= near_x + 320; x += 16) {
            int d = dist2(x, y, near_x, near_y), clear = 1;
            if (x < 96 || y < 96 || x >= WORLD_W - 96 || y >= WORLD_H - 96 ||
                d >= best_d || !can_place(B_NYDUS_CANAL, x, y)) continue;
            /* Keep a walkable apron so the exit search has real choices. */
            for (int oy = -48; oy <= 48 && clear; oy += 16)
                for (int ox = -48; ox <= 48 && clear; ox += 16)
                    if ((ox < -16 || ox > 16 || oy < -16 || oy > 16) &&
                        !is_traversable((x + ox) / 16, (y + oy) / 16, -1)) clear = 0;
            for (int i = 0; i < MAX_UNITS && clear; ++i)
                if (u[i].alive && !sc64_race_unit_flying(u[i].type) &&
                    FAST_ABS(u[i].x - x) < 72 && FAST_ABS(u[i].y - y) < 72)
                    clear = 0;
            if (!clear) continue;
            best = x + y * WORLD_W;
            best_d = d;
        }
    assert(best >= 0);
    return best;
}

static int place_endpoint(int near_x, int near_y)
{
    int site = open_site(near_x, near_y);
    int slot = spawn_build(SIDE, B_NYDUS_CANAL, site % WORLD_W, site / WORLD_W, 0);
    assert(slot >= 0 && b[slot].raw_type == SC64_NYDUS_RAW &&
           b[slot].type == B_NYDUS_CANAL && b[slot].hp == SC64_NYDUS_HP);
    invalidate_path_grid();
    return slot;
}

static Sc64NydusSourceRecord record_for(int slot, unsigned index, unsigned linked)
{
    Sc64NydusSourceRecord r;
    memset(&r, 0, sizeof(r));
    r.record_index = (unsigned short)index;
    r.linked_record_index = (unsigned short)linked;
    r.raw_type = SC64_NYDUS_RAW;
    r.relation = SC64_NYDUS_RELATION_LINK;
    r.raw_owner = (unsigned char)b[slot].raw_owner;
    r.x = (short)b[slot].x;
    r.y = (short)b[slot].y;
    return r;
}

static int side_hydra(int n)
{
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].type == U_HYDRALISK && n-- == 0)
            return i;
    assert(!"missing SIDE Hydralisk");
    return -1;
}

static int side_type(int type)
{
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].type == type) return i;
    assert(!"missing SIDE unit type");
    return -1;
}

/* Launch Z7, place two pairs and bind them through the integration hook. */
static void launch_with_canals(void)
{
    int h;
    launch_z7_normally();
    h = side_hydra(0);
    ent = place_endpoint(u[h].x, u[h].y - 96);
    ex = place_endpoint(u[h].x + 360, u[h].y - 260);
    ent2 = place_endpoint(u[h].x - 220, u[h].y - 120);
    ex2 = place_endpoint(u[h].x + 120, u[h].y - 420);
    /* Shuffled rows and record indexes shaped like Z8's two reciprocal pairs. */
    records[0] = record_for(ex2, 197, 195);
    records[1] = record_for(ent, 192, 194);
    records[2] = record_for(ent2, 195, 197);
    records[3] = record_for(ex, 194, 192);
    assert(race_nydus_set_source(SC64_Z7_GLOBAL_ID, records, 4));
    assert(race_nydus_bind_world(1));
    assert(race_nydus.count == 2);
    assert(sc64_nydus_partner(&race_nydus, (unsigned)ent) == ex);
    assert(sc64_nydus_partner(&race_nydus, (unsigned)ex2) == ent2);
}

static void select_unit_normal(int slot)
{
    clear_selection();
    selected_building = -1;
    cx = u[slot].x; cy = u[slot].y;
    press(KEY_A);
    assert(selected_building < 0 && u[slot].selected);
}

static int command_enter(int entrance)
{
    int before = 0, after = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        before += u[i].alive && u[i].order == ORDER_NYDUS_ENTER;
    cx = b[entrance].x; cy = b[entrance].y;
    input(KEY_A);
    input(0);
    for (int i = 0; i < MAX_UNITS; ++i)
        after += u[i].alive && u[i].order == ORDER_NYDUS_ENTER;
    return after - before;
}

static int near_exit(int unit, int exit)
{
    return FAST_ABS(u[unit].x - b[exit].x) <= SC64_NYDUS_EXIT_SEARCH_PX &&
           FAST_ABS(u[unit].y - b[exit].y) <= SC64_NYDUS_EXIT_SEARCH_PX;
}

static int side_alive_units(void)
{
    int n = 0;
    for (int i = 0; i < MAX_UNITS; ++i) n += u[i].alive && u[i].owner == SIDE;
    return n;
}

static int enemy_ground_unit(void)
{
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == ENEMY && !sc64_race_unit_flying(u[i].type) &&
            sc64_game_unit_selectable(u[i].type)) return i;
    assert(!"missing enemy ground unit");
    return -1;
}

static void kill_building(int slot)
{
    for (int guard = 0; guard < 8 && b[slot].alive; ++guard)
        hit_target_impl_ex(MAX_UNITS + slot, 5000, U_HYDRALISK, ENEMY, -1, 0);
    assert(!b[slot].alive);
}

/* One passenger: ordinary command, approach, atomic transfer, retained
 * selection, cleared attack references, unchanged counts, visible exit. */
static void single_passenger_transits(void)
{
    int h, foe, units, supply, jumped_at = -1;
    launch_with_canals();
    h = side_hydra(0);
    foe = enemy_ground_unit();
    units = side_alive_units();
    supply = race_owner_supply(SIDE);
    select_unit_normal(h);
    assert(command_enter(ent) == 1);
    assert(u[h].order == ORDER_NYDUS_ENTER && u[h].target == ent);
    /* An enemy that targets the passenger loses that reference on entry. */
    u[foe].order = ORDER_ATTACK_TARGET; u[foe].target = h;
    for (int f = 0; f < 900 && u[h].order == ORDER_NYDUS_ENTER; ++f) {
        int ox = u[h].x, oy = u[h].y;
        input(0);
        if (u[h].order != ORDER_NYDUS_ENTER) {
            /* The only large jump is entrance contact -> exit search box. */
            assert(sc64_nydus_contact(ox, oy, b[ent].x, b[ent].y));
            assert(near_exit(h, ex));
            jumped_at = f;
        } else {
            assert(FAST_ABS(u[h].x - ox) <= 8 && FAST_ABS(u[h].y - oy) <= 8);
        }
        /* Z7 combat may kill other SIDE actors; the passenger is never lost. */
        assert(u[h].alive);
    }
    assert(jumped_at >= 0 && u[h].alive && u[h].order == ORDER_IDLE && u[h].target == -1);
    assert(u[h].selected && is_traversable(u[h].x / 16, u[h].y / 16, -1));
    assert(u[foe].alive && u[foe].target != h);
    assert(side_alive_units() == units && race_owner_supply(SIDE) == supply);
    step(2);
    assert(currently_visible(u[h].x, u[h].y));
    printf("single transit: frame %d exit offset %+d,%+d\n", jumped_at,
           u[h].x - b[ex].x, u[h].y - b[ex].y);
}

/* Flyers, non-Zerg and enemies are rejected at command time; the command on
 * a canal with no eligible selection keeps ordinary building selection. */
static void eligibility_and_unavailable_destination(void)
{
    int h, ov, foe;
    launch_with_canals();
    h = side_hydra(0);
    ov = side_type(U_OVERLORD);
    foe = enemy_ground_unit();
    clear_selection();
    u[h].selected = u[ov].selected = 1;
    assert(command_enter(ent) == 1);
    assert(u[h].order == ORDER_NYDUS_ENTER && u[ov].order != ORDER_NYDUS_ENTER);
    assert(!race_nydus_can_enter(foe, ent));

    /* Incomplete exit: no order; the press selects the canal as usual. */
    b[ex].under_construction = 1;
    step(1);
    assert(u[h].order == ORDER_IDLE && !near_exit(h, ex)); /* cancelled en route */
    clear_selection();
    u[h].selected = 1;
    assert(command_enter(ent) == 0 && selected_building == ent);
    b[ex].under_construction = 0;
    /* A canal outside the source table is never linked or enterable. */
    {
        int loose = place_endpoint(b[ent].x - 200, b[ent].y + 40);
        assert(sc64_nydus_partner(&race_nydus, (unsigned)loose) < 0 &&
               !race_nydus_can_enter(h, loose));
    }
}

/* Six passengers arriving together are placed in slot order without overlap,
 * and replaying from a save reproduces every exit coordinate. */
static void concurrent_passengers_are_deterministic(void)
{
    int pass[6], first[6][2], units;
    launch_with_canals();
    for (int i = 0; i < 6; ++i) pass[i] = side_hydra(i);
    assert(race_save_game());
    for (int run = 0; run < 2; ++run) {
        if (run) {
            assert(race_load_game());
            assert(sc64_nydus_partner(&race_nydus, (unsigned)ent) == ex);
        }
        units = side_alive_units();
        clear_selection();
        for (int i = 0; i < 6; ++i) u[pass[i]].selected = 1;
        assert(command_enter(ent) == 6);
        for (int f = 0; f < 1200; ++f) {
            int busy = 0;
            input(0);
            for (int i = 0; i < 6; ++i) busy |= u[pass[i]].order == ORDER_NYDUS_ENTER;
            if (!busy) break;
        }
        assert(side_alive_units() == units);
        for (int i = 0; i < 6; ++i) {
            int p = pass[i];
            assert(u[p].alive && u[p].order == ORDER_IDLE && near_exit(p, ex));
            for (int j = 0; j < i; ++j) {
                int q = pass[j], r = sc64_game_unit_collision_radius(u[p].type);
                assert(dist2(u[p].x, u[p].y, u[q].x, u[q].y) >= r * r);
            }
            if (!run) { first[i][0] = u[p].x; first[i][1] = u[p].y; }
            else assert(first[i][0] == u[p].x && first[i][1] == u[p].y);
        }
    }
    printf("concurrent transit: 6 passengers, replay identical\n");
}

/* Placement is not movement: a passenger may separate from a stacked unit
 * during ordinary movement, but an exit point must never retain that overlap. */
static void strict_exit_placement_rejects_overlap_escape(void)
{
    int h, blocker, px = -1, py = -1, radius, found = 0;
    launch_with_canals();
    h = side_hydra(0); blocker = side_hydra(1);
    radius = sc64_game_unit_collision_radius(U_HYDRALISK);
    assert(radius >= 5);
    for (unsigned n = 0; n < SC64_NYDUS_EXIT_CANDIDATES && !found; ++n) {
        int x, y, clear = 1;
        if (!sc64_nydus_exit_candidate(n, b[ex].x, b[ex].y, &x, &y) ||
            x < 96 || y < 96 || x >= WORLD_W - 96 || y >= WORLD_H - 96 ||
            !is_traversable(x / 16, y / 16, -1)) continue;
        for (int i = 0; i < MAX_UNITS && clear; ++i)
            if (i != h && i != blocker && u[i].alive && !SC64_TRANSPORT_LOADED(&u[i]) &&
                !sc64_race_unit_flying(u[i].type) &&
                FAST_ABS(u[i].x - x) < radius + 12 && FAST_ABS(u[i].y - y) < radius + 12)
                clear = 0;
        if (clear) { px = x; py = y; found = 1; }
    }
    assert(found);
    u[h].x = (short)(px + radius - 3); u[h].y = (short)py;
    b[ent].x = (short)(px + 48); b[ent].y = (short)py;
    u[blocker].x = (short)(px + radius - 1); u[blocker].y = (short)py;
    invalidate_path_grid();
    sc64_unit_buckets_begin();
    assert(sc64_unit_bucket_active && is_traversable(px / 16, py / 16, -1));
    assert(unit_step_clear(&u[h], px, py));
    assert(!race_nydus_exit_open(&u[h], px, py));
    sc64_unit_buckets_end();
    printf("strict exit: separating overlap rejected for placement\n");
}

/* Surround the exit with structures: the passenger holds at the entrance
 * with its order intact, then leaves as soon as one blocker is destroyed. */
static void blocked_exit_waits_then_retries(void)
{
    int h, blockers[24], nb = 0, x, y, held = 0;
    launch_with_canals();
    for (int oy = -64; oy <= 64; oy += 32)
        for (int ox = -64; ox <= 64; ox += 32) {
            if (!ox && !oy) continue;
            /* SIDE-owned so Z7's source-owner world validator accepts them. */
            blockers[nb] = spawn_build(SIDE, B_EVOLUTION_CHAMBER,
                                       b[ex].x + ox, b[ex].y + oy, 0);
            assert(blockers[nb] >= 0);
            ++nb;
        }
    invalidate_path_grid();
    h = side_hydra(0);
    assert(!race_nydus_place(h, ex, &x, &y));
    select_unit_normal(h);
    assert(command_enter(ent) == 1);
    for (int f = 0; f < 600; ++f) {
        input(0);
        assert(u[h].order == ORDER_NYDUS_ENTER && !near_exit(h, ex));
        held += sc64_nydus_contact(u[h].x, u[h].y, b[ent].x, b[ent].y);
    }
    assert(held > 100);
    /* Clear the south-west quadrant of blockers through the damage path. */
    for (int i = 0; i < nb; ++i)
        if (b[blockers[i]].x <= b[ex].x && b[blockers[i]].y >= b[ex].y)
            kill_building(blockers[i]);
    for (int f = 0; f < 8 && u[h].order == ORDER_NYDUS_ENTER; ++f) input(0);
    assert(u[h].order == ORDER_IDLE && near_exit(h, ex));
    printf("blocked exit: held %d frames, then exited\n", held);
}

/* Damage to either endpoint kills its partner in the same update; passengers
 * en route stop where they are. A scenario-style removal outside the damage
 * path is reconciled on the next update. Slot reuse never inherits a link. */
static void endpoint_loss_and_slot_reuse(void)
{
    int h, h2, reused;
    launch_with_canals();
    h = side_hydra(0); h2 = side_hydra(1);
    clear_selection(); u[h].selected = u[h2].selected = 1;
    assert(command_enter(ent) == 2);
    step(3);
    kill_building(ex); /* exit destroyed: entrance dies with it */
    assert(!b[ent].alive && !b[ex].alive);
    assert(sc64_nydus_partner(&race_nydus, (unsigned)ent) < 0);
    assert(u[h].order == ORDER_IDLE && u[h2].order == ORDER_IDLE);
    assert(!near_exit(h, ex) && !near_exit(h2, ex));
    assert(race_nydus_issue_selected(b[ent].x, b[ent].y) == 0);

    /* Removal by direct state change (as a scenario Remove would). */
    b[ex2].alive = 0;
    input(0);
    assert(!b[ent2].alive && !race_nydus.link[1].active);

    /* A recycled row is an ordinary building. */
    reused = spawn_build(SIDE, B_NYDUS_CANAL, b[ent].x, b[ent].y, 0);
    assert(reused >= 0 && sc64_nydus_partner(&race_nydus, (unsigned)reused) < 0);
    clear_selection(); u[h].selected = 1;
    assert(command_enter(reused) == 0);
    printf("endpoint loss: partner killed, orders released, reuse unlinked\n");
}

/* A transferred endpoint carries its partner (OpenBW give_unit_to); former
 * owner's passengers are released and cannot issue new orders. */
static void ownership_change_follows_link(void)
{
    int h;
    launch_with_canals();
    h = side_hydra(0);
    select_unit_normal(h);
    assert(command_enter(ent2) == 1);
    step(2);
    {
        int src = -1;
        for (int i = 0; i < 8 && src < 0; ++i)
            if (race_z7_world.live_owner_for_source[i] == ENEMY) src = i;
        assert(src >= 0);
        b[ent2].owner = ENEMY; b[ent2].raw_owner = src;
        input(0);
        assert(b[ex2].owner == ENEMY && b[ex2].raw_owner == src);
        assert(screen == SCREEN_PLAY);
    }
    assert(race_nydus.link[1].owner == ENEMY && u[h].order == ORDER_IDLE);
    assert(!race_nydus_can_enter(h, ent2) && !race_nydus_can_enter(h, ex2));
    clear_selection(); u[h].selected = 1;
    assert(command_enter(ent2) == 0);
    printf("ownership: partner transferred, passengers released\n");
}

static void reject_save_without_live_mutation(const RaceSave *bad)
{
    static Unit before_units[MAX_UNITS];
    static Building before_buildings[MAX_BUILDINGS];
    Sc64NydusState before_links = race_nydus;
    int before_frame = frame;
    memcpy(before_units, u, sizeof(u));
    memcpy(before_buildings, b, sizeof(b));
    /* The rejection must come from the Nydus preflight itself. */
    assert(!race_nydus_save_candidate_valid(bad, race_cols, race_rows));
    save_write(bad, sizeof(*bad), SAVE_VERSION);
    assert(!race_load_game());
    assert(!memcmp(before_units, u, sizeof(u)));
    assert(!memcmp(before_buildings, b, sizeof(b)));
    assert(!memcmp(&before_links, &race_nydus, sizeof(race_nydus)));
    assert(frame == before_frame);
}

static void rewrite_unit(RaceSave *s, int slot, void (*edit)(Unit *))
{
    Unit q;
    assert(race_save_decode_unit(s, (unsigned)slot, (unsigned)race_cols * 16u,
                                 (unsigned)race_rows * 16u, &q));
    edit(&q);
    assert(sc64_zp_unit_save_encode(&s->units[slot], &q, (unsigned)race_cols * 16u,
                                    (unsigned)race_rows * 16u));
}

static int retarget_to;
static void set_target(Unit *q) { q->target = retarget_to; }
static void make_enter(Unit *q) { q->order = ORDER_NYDUS_ENTER; q->target = retarget_to; }

static void rewrite_building(RaceSave *s, int slot, void (*edit)(Building *))
{
    Building q;
    assert(race_save_decode_building(s, (unsigned)slot, &q));
    edit(&q);
    assert(sc64_building_save_encode(&s->buildings[slot], &q, (unsigned)race_cols * 16u,
                                     (unsigned)race_rows * 16u));
}

static void remove_building(Building *q) { q->alive = 0; }
static void move_building(Building *q) { q->x += 16; }

/* Mid-approach save/resume: orders and links survive, nothing is duplicated
 * or lost, and the resumed run matches an uninterrupted run exactly. */
static void save_resume_and_malformed_saves(void)
{
    int pass[4], expect[4][2], units, units_after, flyer;
    RaceSave good, bad;
    launch_with_canals();
    for (int i = 0; i < 4; ++i) pass[i] = side_hydra(i);
    clear_selection();
    for (int i = 0; i < 4; ++i) u[pass[i]].selected = 1;
    assert(command_enter(ent) == 4);
    step(6);
    units = side_alive_units();
    assert(race_save_game());
    good = race_data;
    for (int f = 0; f < 1200; ++f) input(0);
    for (int i = 0; i < 4; ++i) {
        assert(u[pass[i]].order == ORDER_IDLE && near_exit(pass[i], ex));
        expect[i][0] = u[pass[i]].x; expect[i][1] = u[pass[i]].y;
    }
    units_after = side_alive_units(); /* Z7 combat continues meanwhile. */

    assert(race_nydus_save_candidate_valid(&good, race_cols, race_rows));
    save_write(&good, sizeof(good), SAVE_VERSION);
    assert(race_load_game());
    assert(side_alive_units() == units);
    assert(sc64_nydus_partner(&race_nydus, (unsigned)ent) == ex &&
           sc64_nydus_partner(&race_nydus, (unsigned)ent2) == ex2);
    for (int i = 0; i < 4; ++i)
        assert(u[pass[i]].order == ORDER_NYDUS_ENTER && u[pass[i]].target == ent);
    for (int f = 0; f < 1200; ++f) input(0);
    for (int i = 0; i < 4; ++i)
        assert(u[pass[i]].x == expect[i][0] && u[pass[i]].y == expect[i][1]);
    assert(side_alive_units() == units_after);

    /* Malformed candidates are rejected before any live state changes. */
    save_write(&good, sizeof(good), SAVE_VERSION);
    assert(race_load_game());
    /* Passenger aimed at a non-canal building row. */
    bad = good; retarget_to = -1;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].type != B_NYDUS_CANAL) { retarget_to = i; break; }
    assert(retarget_to >= 0);
    rewrite_unit(&bad, pass[0], set_target);
    reject_save_without_live_mutation(&bad);
    /* Passenger aimed at an empty slot. */
    bad = good; retarget_to = MAX_BUILDINGS - 1;
    assert(!b[retarget_to].alive);
    rewrite_unit(&bad, pass[0], set_target);
    reject_save_without_live_mutation(&bad);
    /* A flyer cannot hold the order. */
    bad = good; retarget_to = ent; flyer = side_type(U_OVERLORD);
    rewrite_unit(&bad, flyer, make_enter);
    reject_save_without_live_mutation(&bad);
    /* A lone surviving endpoint is impossible after the partner rule. */
    bad = good;
    rewrite_building(&bad, ex2, remove_building);
    reject_save_without_live_mutation(&bad);
    /* An endpoint moved off its source identity. */
    bad = good;
    rewrite_building(&bad, ex, move_building);
    reject_save_without_live_mutation(&bad);
    /* The unreferenced second pair must not masquerade as destroyed. Moving
     * the first pair would already reject its passengers' orphaned orders. */
    bad = good;
    rewrite_building(&bad, ent2, move_building);
    rewrite_building(&bad, ex2, move_building);
    reject_save_without_live_mutation(&bad);
    /* A whole destroyed pair is a valid save; its rows bind as inactive. */
    bad = good;
    rewrite_building(&bad, ex2, remove_building);
    rewrite_building(&bad, ent2, remove_building);
    save_write(&bad, sizeof(bad), SAVE_VERSION);
    assert(race_load_game());
    assert(!race_nydus.link[1].active && race_nydus.link[0].active);
    printf("save/resume: 4 passengers resumed identically; 6 malformed saves rejected\n");
}

/* A save made while the source belongs to another mission cannot carry Nydus
 * orders, and loading clears stale links. */
static void foreign_source_is_not_trusted(void)
{
    RaceSave good;
    int h;
    launch_with_canals();
    h = side_hydra(0);
    select_unit_normal(h);
    assert(command_enter(ent) == 1);
    assert(race_save_game());
    good = race_data;
    assert(race_nydus_set_source(SC64_Z7_GLOBAL_ID + 1, records, 4));
    reject_save_without_live_mutation(&good);
    assert(race_nydus_set_source(SC64_Z7_GLOBAL_ID, records, 4));
    save_write(&good, sizeof(good), SAVE_VERSION);
    assert(race_load_game() && u[h].order == ORDER_NYDUS_ENTER);
}

/* A fresh boot has no installed source: the save is validated and rebound
 * through the mission hook, keyed by the candidate's own mission. */
static void fresh_boot_load_resolves_through_hook(void)
{
    RaceSave good;
    int h;
    launch_with_canals();
    h = side_hydra(0);
    select_unit_normal(h);
    assert(command_enter(ent) == 1);
    assert(race_save_game());
    good = race_data;
    race_nydus_clear();
    hook_mission = -1; hook_records = 0;
    reject_save_without_live_mutation(&good); /* no source anywhere */
    hook_mission = SC64_Z7_GLOBAL_ID; hook_records = records;
    assert(race_nydus_save_candidate_valid(&good, race_cols, race_rows));
    save_write(&good, sizeof(good), SAVE_VERSION);
    assert(race_load_game());
    assert(race_nydus_source == records && race_nydus_source_mission == SC64_Z7_GLOBAL_ID);
    assert(sc64_nydus_partner(&race_nydus, (unsigned)ent) == ex &&
           u[h].order == ORDER_NYDUS_ENTER && u[h].target == ent);
    for (int f = 0; f < 900 && u[h].order == ORDER_NYDUS_ENTER; ++f) input(0);
    assert(u[h].order == ORDER_IDLE && near_exit(h, ex));
    hook_mission = -1; hook_records = 0;
    printf("fresh boot: hook-resolved preflight, rebind and transit\n");
}

int main(void)
{
    _Static_assert(ORDER_NYDUS_ENTER <= 15, "packed unit order field is four bits");
    _Static_assert(SC_RB_COUNT <= 64, "packed building type field is six bits");
    single_passenger_transits();
    strict_exit_placement_rejects_overlap_escape();
    eligibility_and_unavailable_destination();
    concurrent_passengers_are_deterministic();
    blocked_exit_waits_then_retries();
    endpoint_loss_and_slot_reuse();
    ownership_change_follows_link();
    save_resume_and_malformed_saves();
    foreign_source_is_not_trusted();
    fresh_boot_load_resolves_through_hook();
    puts("sc64 nydus game: ok");
    return 0;
}
