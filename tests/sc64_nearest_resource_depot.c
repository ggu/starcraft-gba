/* Gated nearest resource-depot behavior over the real engine update path. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

#ifndef SC64_NEAREST_RESOURCE_DEPOT
#error "Compile this fixture with SC64_NEAREST_RESOURCE_DEPOT"
#endif

static void set_depot(int slot, int owner, int type, int x, int y, int incomplete)
{
    memset(&b[slot], 0, sizeof(b[slot]));
    b[slot].alive = 1;
    b[slot].owner = owner;
    b[slot].type = type;
    b[slot].x = x;
    b[slot].y = y;
    b[slot].maxhp = b[slot].hp = 1000;
    b[slot].under_construction = incomplete;
}

static int add_probe(int x, int y)
{
    int slot = spawn_unit(SIDE, U_PROBE, x, y);
    assert(slot >= 0);
    return slot;
}

static int add_worker(int type, int x, int y)
{
    int slot = spawn_unit(SIDE, type, x, y);
    assert(slot >= 0);
    return slot;
}

static void reset_fixture(void)
{
    game_init();
    memset(u, 0, sizeof(u));
    memset(b, 0, sizeof(b));
    memset(m, 0, sizeof(m));
    race_mode = 1;
    race_human = R_PROTOSS;
    race_bots = 0;
    memset(race_player, 0xff, sizeof(race_player));
    race_player[SIDE] = R_PROTOSS;
    race_player[ENEMY] = R_ZERG;
    minerals = gas = 0;
    supply_used = 0;
    supply_cap = 200;
    frame = 1;
    won = lost = 0;
    path_grid_valid = 0;
    invalidate_path_grid();
}

static void check_nearest_mineral_and_gas_returns(void)
{
    reset_fixture();
    set_depot(0, SIDE, B_NEXUS, 320, 320, 0);
    set_depot(1, SIDE, B_NEXUS, 720, 320, 0);
    set_depot(2, SIDE, B_ASSIMILATOR, 760, 400, 0);
    m[0].x = 800; m[0].y = 320; m[0].amount = 500;
    int worker = add_probe(760, 320);
    u[worker].order = ORDER_MINE;
    u[worker].target = 0;
    u[worker].carry = 6;
    u[worker].build = -1;

    /* The nearby second Nexus is chosen and cached for this resource trip. */
    update_orders();
    assert(u[worker].build == 1);
    assert(u[worker].carry == 6 && minerals == 0);
    for (int tick = 0; tick < 64 && u[worker].carry; ++tick) {
        update_orders();
        if (u[worker].carry) assert(minerals == 0);
    }
    assert(u[worker].carry == 0 && minerals == 6);
    assert(u[worker].build == -1);

    /* A gas trip uses the same nearest completed depot and credits only on
     * arrival; the refinery is a resource source, not a return depot. */
    u[worker].x = 760; u[worker].y = 320;
    u[worker].order = ORDER_GAS;
    u[worker].target = 2;
    u[worker].carry = 8;
    u[worker].build = -1;
    int gas_before = gas;
    update_orders();
    assert(u[worker].build == 1);
    assert(u[worker].carry == 8 && gas == gas_before);
    for (int tick = 0; tick < 64 && u[worker].carry; ++tick) {
        update_orders();
        if (u[worker].carry) assert(gas == gas_before);
    }
    assert(u[worker].carry == 0 && gas == gas_before + 8);

    /* The source refinery can disappear after cargo collection; the worker
     * still completes the return trip and credits gas only at the depot. */
    u[worker].x = 760; u[worker].y = 320;
    u[worker].order = ORDER_GAS;
    u[worker].target = 2;
    u[worker].carry = 5;
    u[worker].build = -1;
    b[2].alive = 0;
    gas_before = gas;
    update_orders();
    assert(u[worker].carry == 5 && u[worker].build == 1);
    for (int tick = 0; tick < 64 && u[worker].carry; ++tick) {
        update_orders();
        if (u[worker].carry) assert(gas == gas_before);
    }
    assert(u[worker].carry == 0 && gas == gas_before + 5);
}

static void check_cache_reselection_and_slot_tie(void)
{
    reset_fixture();
    set_depot(0, SIDE, B_NEXUS, 480, 400, 0);
    set_depot(1, SIDE, B_HATCHERY, 560, 400, 0);
    int worker = add_probe(520, 400);
    u[worker].order = ORDER_MINE;
    u[worker].carry = 4;
    assert(race_nearest_resource_depot(&u[worker]) == 0); /* lower slot wins tie */
    assert(u[worker].build == 0);

    /* A valid trip home remains cached even if the worker position changes. */
    u[worker].x = 550;
    assert(race_nearest_resource_depot(&u[worker]) == 0);

    /* Destroying the cached depot invalidates it; the next eligible home is
     * selected without losing the carried resources. */
    /* Reuse its slot for a non-depot building to exercise stale slot data. */
    set_depot(0, SIDE, B_GATEWAY, 480, 400, 0);
    assert(race_nearest_resource_depot(&u[worker]) == 1);
    assert(u[worker].carry == 4);

    /* A decoded saved Unit preserves the cached slot. If that slot is no
     * longer a valid same-owner depot, live selection safely replaces it. */
    Sc64ZpPackedUnitSave encoded;
    Unit decoded;
    assert(sc64_zp_unit_save_encode(&encoded, &u[worker], WORLD_W, WORLD_H));
    assert(sc64_zp_unit_save_decode(&decoded, &encoded, WORLD_W, WORLD_H));
    assert(decoded.build == 1);
    decoded.build = 0;
    assert(sc64_zp_unit_save_encode(&encoded, &decoded, WORLD_W, WORLD_H));
    assert(sc64_zp_unit_save_decode(&decoded, &encoded, WORLD_W, WORLD_H));
    u[worker] = decoded;
    assert(race_nearest_resource_depot(&u[worker]) == 1);
}

static void check_morphing_lair_remains_depot(void)
{
    reset_fixture();
    set_depot(0, SIDE, B_LAIR, 510, 400, 1);
    int worker = add_probe(480, 400);
    u[worker].order = ORDER_MINE;
    u[worker].target = 0;
    u[worker].carry = 6;
    u[worker].build = -1;

    /* A morphing Lair is an active return depot even while its structure
     * flag reports under construction. New incomplete Hatcheries stay out. */
    assert(race_nearest_resource_depot(&u[worker]) == 0);
    assert(race_resource_depot_valid_for(&u[worker], 0));
    set_depot(1, SIDE, B_HATCHERY, 600, 400, 1);
    assert(!race_resource_depot_valid_for(&u[worker], 1));
    update_orders();
    assert(u[worker].carry == 0 && minerals == 6);
}

static void check_sram_roundtrip_cached_home(void)
{
    reset_fixture();
    race_bots = 1;
    race_bot_races[0] = R_TER;
    race_player[ENEMY] = R_TER;
    race_cols = MAP_COLS;
    race_rows = MAP_ROWS;
    race_skirmish_map = 0;
    set_depot(0, SIDE, B_NEXUS, 320, 320, 0);
    set_depot(1, SIDE, B_NEXUS, 720, 320, 0);
    int worker = add_probe(760, 320);
    u[worker].order = ORDER_MINE;
    u[worker].target = 0;
    u[worker].carry = 7;
    assert(race_nearest_resource_depot(&u[worker]) == 1);
    assert(race_save_game());
    u[worker].build = -1;
    u[worker].carry = 0;
    assert(race_load_game());
    assert(u[worker].alive && u[worker].carry == 7 && u[worker].build == -1);
    assert(race_nearest_resource_depot(&u[worker]) == 1);
}

static void check_ineligible_depots_and_topology(void)
{
    reset_fixture();
    set_depot(0, ENEMY, B_NEXUS, 520, 400, 0);
    set_depot(1, SIDE, B_NEXUS, 500, 400, 1);
    set_depot(2, SIDE, B_GATEWAY, 510, 400, 0);
    set_depot(3, SIDE, B_HATCHERY, 640, 400, 0);
    int worker = add_probe(480, 400);
    u[worker].order = ORDER_MINE;
    u[worker].carry = 3;
    assert(race_nearest_resource_depot(&u[worker]) == 3);

    /* Cache no-home rather than rescanning every tick, then wake it when a
     * new path/depot becomes available. */
    const int worker_types[] = {U_SCV, U_DRONE, U_PROBE};
    for (unsigned type_index = 0;
         type_index < sizeof(worker_types) / sizeof(worker_types[0]);
         ++type_index) {
        reset_fixture();
        worker = add_worker(worker_types[type_index], 480, 400);
        m[0].x = 480; m[0].y = 400; m[0].amount = 100;
        u[worker].order = ORDER_MINE;
        u[worker].target = 0;
        u[worker].carry = 3;
        assert(race_nearest_resource_depot(&u[worker]) == -1);
        assert(u[worker].build == -2);
        update_orders();
        assert(u[worker].order == ORDER_MINE && u[worker].carry == 3);
        assert(minerals == 0);

        /* Economy retries cannot erase a carried trip while there is no
         * depot. The -2 sentinel remains asleep until a real wake event. */
        frame = 30;
        economy();
        assert(u[worker].order == ORDER_MINE && u[worker].carry == 3);
        assert(u[worker].build == -2 && minerals == 0);

        /* Put the newly completed base on the worker's 32px approach ring so
         * this test isolates the no-home wake and delivery rules, not route
         * choice around a large building footprint. */
        set_depot(0, SIDE, B_NEXUS, 510, 400, 1);
        invalidate_path_grid();
        assert(u[worker].build == -1);
        assert(race_nearest_resource_depot(&u[worker]) == -1);
        race_complete_building(0);
        assert(u[worker].build == -1);
        assert(race_nearest_resource_depot(&u[worker]) == 0);
        assert(u[worker].carry == 3 && minerals == 0);
        for (int tick = 0; tick < 64 && u[worker].carry; ++tick) {
            update_orders();
            if (u[worker].carry) assert(minerals == 0);
        }
        assert(u[worker].carry == 0 && minerals == 3);
    }
}

static void check_unreachable_closer_depot(void)
{
    reset_fixture();
    int barrier_x = MAP_COLS * 8;
    int worker_x = barrier_x - 24;
    set_depot(0, SIDE, B_NEXUS, barrier_x - 80, WORLD_H / 2, 0);
    set_depot(1, SIDE, B_NEXUS, barrier_x + 24, WORLD_H / 2, 0);
    int worker = add_probe(worker_x, WORLD_H / 2);
    u[worker].order = ORDER_MINE;
    u[worker].carry = 5;
    for (int row = 0; row < MAP_ROWS; ++row)
        set_depot(2 + row, ENEMY, B_GATEWAY, barrier_x, row * 16 + 8, 0);
    invalidate_path_grid();
    assert(!path_next_tile(u[worker].x, u[worker].y,
                           b[1].x, b[1].y, &(int){0}, &(int){0}));
    assert(path_next_tile(u[worker].x, u[worker].y,
                          b[0].x, b[0].y, &(int){0}, &(int){0}));
    assert(race_nearest_resource_depot(&u[worker]) == 0);
}

static void check_topology_reselection_and_reconnect(void)
{
    reset_fixture();
    int barrier_x = MAP_COLS * 8;
    int worker_x = barrier_x - 24;
    set_depot(0, SIDE, B_NEXUS, barrier_x + 24, WORLD_H / 2, 0);
    set_depot(1, SIDE, B_NEXUS, barrier_x - 80, WORLD_H / 2, 0);
    int worker = add_probe(worker_x, WORLD_H / 2);
    u[worker].order = ORDER_MINE;
    u[worker].carry = 5;

    /* Before the wall exists, the closer depot is reachable and cached. */
    assert(race_nearest_resource_depot(&u[worker]) == 0);
    for (int row = 0; row < MAP_ROWS; ++row)
        set_depot(2 + row, ENEMY, B_GATEWAY, barrier_x, row * 16 + 8, 0);
    invalidate_path_grid();
    assert(u[worker].build == -1);
    assert(race_nearest_resource_depot(&u[worker]) == 1);

    /* Removing the wall and invalidating topology lets the nearer cached
     * candidate win again. */
    for (int row = 0; row < MAP_ROWS; ++row) b[2 + row].alive = 0;
    invalidate_path_grid();
    assert(u[worker].build == -1);
    assert(race_nearest_resource_depot(&u[worker]) == 0);
}

#ifdef SC64_TERRAN_LATE_RUNTIME
static void check_airborne_command_center_exclusion(void)
{
    reset_fixture();
    race_mode = 0;
    race_selected_mission = 10;
    set_depot(0, SIDE, B_COMMAND, 500, 400, 0);
    b[0].flags |= 4;
    set_depot(1, SIDE, B_NEXUS, 600, 400, 0);
    int worker = add_probe(520, 400);
    u[worker].order = ORDER_MINE;
    u[worker].carry = 2;
    assert(race_late_active() && race_late_airborne(0));
    assert(race_nearest_resource_depot(&u[worker]) == 1);
}
#endif

int main(void)
{
    gba_init();
    check_nearest_mineral_and_gas_returns();
    check_cache_reselection_and_slot_tie();
    check_morphing_lair_remains_depot();
    check_sram_roundtrip_cached_home();
    check_ineligible_depots_and_topology();
    check_unreachable_closer_depot();
    check_topology_reselection_and_reconnect();
#ifdef SC64_TERRAN_LATE_RUNTIME
    check_airborne_command_center_exclusion();
#endif
    puts("nearest reachable resource depot checks passed");
    return 0;
}
