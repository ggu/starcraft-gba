/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_CROWD_PROBE_H
#define SC64_CROWD_PROBE_H

/* Measurement-only crowd scene (never part of a release ROM).
 *
 * SC64_CROWD_PROBE=N boots straight into a custom game on the default map:
 * the human (Zerg) gets N Zerglings in a block by its Hatchery, ordered to
 * attack-move into a block of Marines guarding the Terran bot's Command
 * Center. Everything after the first update is the ordinary simulation, so
 * the same N gives the same game on every build and update costs can be
 * compared directly.
 *
 * SC64_CROWD_PROBE_DEFENDERS sets the Marine count (default 8). */
#if !defined(SC64_RACES) || !defined(SC64_ZP_FULL_STORAGE)
#error "the crowd probe needs a Zerg/Protoss full-storage race profile"
#endif
#ifndef SC64_CROWD_PROBE_DEFENDERS
#define SC64_CROWD_PROBE_DEFENDERS 8
#endif

static int sc64_crowd_probe_spawned;

static int sc64_crowd_probe_actor(int owner, int type, int x, int y,
                                  int order, int tx, int ty)
{
    int n = spawn_unit(owner, type, x, y);
    if (n < 0) return n;
    u[n].raw_type = sc64_race_raw_unit(type);
    u[n].raw_owner = -1;
    u[n].order = order; u[n].tx = tx; u[n].ty = ty; u[n].target = -1;
    return n;
}

static int sc64_crowd_probe_base(int owner, int type, int x, int y)
{
    int n = spawn_build(owner, type, x, y, 0);
    if (n >= 0) { b[n].raw_type = sc64_race_raw_building(type); b[n].raw_owner = -1; }
    return n;
}

static void sc64_crowd_probe_start(void)
{
    race_mode = 1; race_skirmish_map = 0; race_bots = 1;
#ifdef SC64_IMPORTED_SKIRMISH
    race_skirmish_slots = 0; race_skirmish_start_order = 0;
#ifdef SC64_NATIVE_MELEE_STARTS
    race_skirmish_melee = 0;
#endif
#endif
    race_human = R_ZERG;
    race_bot_races[0] = R_TER;
    init_state();
    memset(u, 0, sizeof(u)); memset(b, 0, sizeof(b));
    clear_tracers();
    death_vfx_reset();
    supply_used = 0;
    seed = 0x43524f57u; frame = 0;
    selected_building = -1;
    (void)sc64_crowd_probe_base(SIDE, B_HATCHERY, 160, 160);
    (void)sc64_crowd_probe_base(ENEMY, B_COMMAND, 864, 864);
    for (int i = 0; i < SC64_CROWD_PROBE_DEFENDERS; ++i)
        (void)sc64_crowd_probe_actor(ENEMY, U_MARINE, 760 + (i % 6) * 14,
                                     760 + (i / 6) * 14, ORDER_IDLE, 0, 0);
    /* A block 20 wide, rows 12 pixels apart, laid out away from the base. */
    sc64_crowd_probe_spawned = 0;
    for (int i = 0; i < SC64_CROWD_PROBE && i < MAX_UNITS; ++i) {
        int x = 96 + (i % 20) * 12, y = 240 + (i / 20) * 12;
        if (x >= WORLD_W - 16 || y >= WORLD_H - 16) break;
        if (sc64_crowd_probe_actor(SIDE, U_ZERGLING, x, y,
                                   ORDER_ATTACK_MOVE, 795, 780) >= 0)
            ++sc64_crowd_probe_spawned;
    }
    cx = 300; cy = 300;
    screen = SCREEN_PLAY;
    invalidate_path_grid();
    update_exploration();
}

#endif
