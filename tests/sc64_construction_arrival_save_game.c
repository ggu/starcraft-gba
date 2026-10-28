/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Save/load and malicious-candidate checks for persistent build intents. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

#if !defined(SC64_RACES) || !defined(SC64_UNIFIED) || \
    !defined(SC64_IMPORTED_SKIRMISH) || !defined(SC64_CAMPAIGN_RACE)
#error "Build with one canonical full race host profile"
#endif

static Unit before_units[MAX_UNITS];
static Building before_buildings[MAX_BUILDINGS];

static void candidate_unit_encode(RaceSave *save, int slot, const Unit *unit)
{
#if defined(SC64_LATE_TERRAN_STORAGE) || defined(SC64_ZP_CAMPAIGN_STORAGE)
#ifdef SC64_ZP_FULL_STORAGE
#ifdef SC64_P7_TYPED_COMPACT_ENABLED
    assert(sc64_p7_typed_unit_encode(&save->units[slot], unit,
                                     (unsigned)(race_cols * 16),
                                     (unsigned)(race_rows * 16)));
#elif defined(SC64_Z7_DERIVED_SAVE)
    assert(sc64_derived_unit_encode(&save->units[slot], unit,
                                   (unsigned)(race_cols * 16),
                                   (unsigned)(race_rows * 16)));
#else
    assert(sc64_zp_unit_save_encode(&save->units[slot], unit,
                                    (unsigned)(race_cols * 16),
                                    (unsigned)(race_rows * 16)));
#endif
#else
    assert(sc64_unit_save_encode(&save->units[slot], unit,
                                 (unsigned)(race_cols * 16),
                                 (unsigned)(race_rows * 16)));
#endif
#else
    for (unsigned i = 0; i < RACE_UNIT_WORDS; ++i) {
        int value;
        memcpy(&value, (const u8 *)unit + i * sizeof(int), sizeof(value));
        save->units[slot][i] = (s16)value;
    }
#endif
}

static void set_candidate_worker(RaceSave *save, int worker, int mutation,
                                 int human_race)
{
    Unit unit;
    assert(race_save_decode_unit(save, (unsigned)worker,
                                 (unsigned)(race_cols * 16),
                                 (unsigned)(race_rows * 16), &unit));
    switch (mutation) {
    case 0: /* A valid non-worker unit keeps the pending order. */
        unit.type = human_race == R_TER ? U_MARINE :
                    human_race == R_ZERG ? U_ZERGLING : U_ZEALOT;
        unit.maxhp = sc64_race_unit_rule(unit.type)->hp;
        unit.armor = sc64_race_unit_armor(unit.type);
        if (unit.hp > unit.maxhp) unit.hp = unit.maxhp;
        break;
    case 1: /* The other player's race does not match this building. */
        unit.owner = ENEMY;
        break;
    case 2: /* A valid building enum from another race is not this intent. */
        unit.target = human_race == R_TER ? B_PYLON : B_DEPOT;
        break;
    case 3: /* Destination is outside the saved world geometry. */
        unit.tx = race_cols * 16;
        break;
    default: assert(0); return;
    }
    candidate_unit_encode(save, worker, &unit);
}

static int worker_and_building(int race, int *worker_type, int *building_type)
{
    if (race == R_TER) {
        *worker_type = U_SCV; *building_type = B_DEPOT;
    } else if (race == R_ZERG) {
        *worker_type = U_DRONE; *building_type = B_SPAWNING_POOL;
    } else {
        *worker_type = U_PROBE; *building_type = B_PYLON;
    }
    return 1;
}

static int new_game(int race, int *worker_type, int *building_type,
                    int *worker, int *site_x, int *site_y)
{
    game_init();
    race_mode = 1;
    race_human = race;
    race_bots = 1;
    race_skirmish_map = 0;
    race_bot_races[0] = (race + 1) % 3;
    race_skirmish_init();
    screen = SCREEN_PLAY;
    worker_and_building(race, worker_type, building_type);
    *worker = -1;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].type == *worker_type) {
            *worker = i; break;
        }
    if (*worker < 0 || !race_skirmish_site_for(SIDE, *building_type,
                                               site_x, site_y)) return 0;
    /* Keep the intent pending for a short, deterministic travel interval. */
    u[*worker].x = *site_x - 52;
    u[*worker].y = *site_y;
    u[*worker].order = ORDER_IDLE;
    u[*worker].target = -1;
    minerals = 5000;
    gas = 1000;
    return 1;
}

static void assert_world_unchanged(void)
{
    assert(memcmp(before_units, u, sizeof(u)) == 0);
    assert(memcmp(before_buildings, b, sizeof(b)) == 0);
}

static void run_race(int race)
{
    RaceSave good, bad;
    int worker_type, building_type, worker, x, y, old_minerals, old_gas;
    assert(new_game(race, &worker_type, &building_type, &worker, &x, &y));
    old_minerals = minerals; old_gas = gas;
    assert(race_start_build(SIDE, worker, building_type, x, y));
    assert(u[worker].order == ORDER_BUILD_PENDING &&
           u[worker].target == building_type && u[worker].tx == x &&
           u[worker].ty == y);
    assert(minerals == old_minerals && gas == old_gas);
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        assert(!b[i].alive || b[i].owner != SIDE || b[i].type != building_type ||
               b[i].x != x || b[i].y != y);

    /* Pending intent survives the real profile-selected SRAM codec. */
    assert(race_save_game());
    good = race_data;
    u[worker].order = ORDER_IDLE; u[worker].target = -1;
    assert(race_load_game());
    assert(u[worker].alive && u[worker].order == ORDER_BUILD_PENDING &&
           u[worker].target == building_type && u[worker].tx == x &&
           u[worker].ty == y);
    assert(minerals == old_minerals && gas == old_gas);

    /* Dead slots can retain historical order bits; save/load ignores them. */
    u[worker].alive = 0;
    assert(race_save_game());
    assert(race_load_game());
    assert(!u[worker].alive && u[worker].order == ORDER_BUILD_PENDING);

    /* Build four corrupt but structurally encodable candidates. Each must be
       rejected before it can mutate live actors, buildings, or the wallet. */
    for (int mutation = 0; mutation < 4; ++mutation) {
        bad = good;
        set_candidate_worker(&bad, worker, mutation, race);
        save_write(&bad, sizeof(bad), SAVE_VERSION);
        memcpy(before_units, u, sizeof(u));
        memcpy(before_buildings, b, sizeof(b));
        int minerals_before = minerals, gas_before = gas;
        assert(!race_load_game());
        assert_world_unchanged();
        assert(minerals == minerals_before && gas == gas_before);
    }

    /* Restore the valid pending save, then let the real update loop commit. */
    save_write(&good, sizeof(good), SAVE_VERSION);
    assert(race_load_game());
    old_minerals = minerals; old_gas = gas;
    for (int tick = 0; tick < 256 && u[worker].order == ORDER_BUILD_PENDING &&
         u[worker].alive; ++tick) game_update(0, 0);
    int building = -1;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == SIDE && b[i].type == building_type &&
            b[i].x == x && b[i].y == y) { assert(building < 0); building = i; }
    assert(building >= 0 && b[building].under_construction &&
           b[building].build_progress >= 0);
    assert(minerals < old_minerals || gas < old_gas);
    if (race == R_TER)
        assert(u[worker].alive && u[worker].order == ORDER_BUILD &&
               u[worker].target == building);
    else if (race == R_PROTOSS)
        assert(u[worker].alive && u[worker].order == ORDER_IDLE);
    else
        assert(!u[worker].alive);
    printf("construction intent save/load/validation/commit race=%d building=%d\n",
           race, building);
}

int main(void)
{
    gba_init();
    run_race(R_TER);
    run_race(R_ZERG);
    run_race(R_PROTOSS);
    puts("canonical construction intent save qualification passed");
    return 0;
}
