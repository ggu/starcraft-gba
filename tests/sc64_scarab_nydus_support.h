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
#ifndef SC64_Z8_RUNTIME
#define SC64_NYDUS_SOURCE_FOR_MISSION(mission, records, count) \
    test_nydus_source_for((mission), (records), (count))
#endif
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
    press(KEY_START);
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

