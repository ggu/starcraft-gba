/* Completed imported Lairs and Hives retain the Hatchery's town-hall roles. */
#include <assert.h>
#include <string.h>
#include "../starcraft/game.c"

static void fixture(int townhall, int incomplete)
{
    game_init();
    race_mode = 1;
    race_human = R_ZERG;
    race_bots = 0;
    memset(u, 0, sizeof(u));
    memset(b, 0, sizeof(b));
    memset(race_player, 0xff, sizeof(race_player));
    race_player[SIDE] = R_ZERG;
    minerals = 2000;
    gas = 2000;
    supply_used = supply_cap = 0;
    for (int i = 0; i < 3; ++i) {
        b[i].alive = 1;
        b[i].owner = SIDE;
        b[i].hp = b[i].maxhp = 1000;
        b[i].x = 320 + i * 80;
        b[i].y = 320;
    }
    b[0].type = townhall;
    b[0].under_construction = incomplete;
    b[0].larva = incomplete ? 0 : 3;
    b[1].type = B_SPAWNING_POOL;
    b[2].type = B_HYDRALISK_DEN;
    race_tick_state();
}

static void check_townhall(int type)
{
    fixture(type, 0);
    assert(race_owner_supply_cap(SIDE) == 2);
    assert(race_owner_has_prereq(SIDE, SC_PR_HATCHERY));
    assert(race_site_creep(SIDE, b[0].x + 160, b[0].y));
    assert(!race_site_creep(SIDE, b[0].x + 161, b[0].y));
    assert(b[0].larva == 3);
    assert(owner_command_center(SIDE) == 0);
    /* The fixture includes an existing Overlord so all four separate queues
       fit the game's normal supply limit. */
    assert(spawn_unit(SIDE, U_OVERLORD, 700, 700) >= 0);

    /* A completed imported town hall is a producer for all four requested
       paid larva units; the two prerequisite buildings are completed too. */
    assert(race_queue_unit(SIDE, 0, U_DRONE));
    assert(race_queue_unit(SIDE, 0, U_OVERLORD));
    assert(race_queue_unit(SIDE, 0, U_ZERGLING));
    /* Replenish one fixture larva to cover four distinct product kinds. */
    b[0].larva = 1;
    assert(race_queue_unit(SIDE, 0, U_HYDRALISK));
    assert(b[0].queue_count == 4);
    assert(b[0].larva == 0);
    assert(minerals == 1725 && gas == 1975);

    /* Full larva cap is respected and one 428-frame cycle grows one larva. */
    b[0].larva = 3;
    b[0].larva_timer = 0;
    race_tick_state();
    assert(b[0].larva == 3 && b[0].larva_timer == 0);
    b[0].larva = 2;
    for (int i = 0; i < 427; ++i) race_tick_state();
    assert(b[0].larva == 2);
    race_tick_state();
    assert(b[0].larva == 3 && b[0].larva_timer == 0);

    /* No cross-faction producer substitution, and rejection is atomic. */
    int old_minerals = minerals, old_gas = gas;
    assert(!race_queue_unit(SIDE, 0, U_ZEALOT));
    assert(minerals == old_minerals && gas == old_gas);
    b[3].alive = 1; b[3].owner = SIDE; b[3].type = B_NEXUS;
    b[3].hp = b[3].maxhp = 750;
    b[3].x = 500; b[3].y = 320;
    assert(!race_queue_unit(SIDE, 3, U_DRONE));
    assert(minerals == old_minerals && gas == old_gas);

    /* Mining and gas deliveries use the same home lookup as live gameplay. */
    m[0].x = b[0].x + 96; m[0].y = b[0].y; m[0].amount = 500;
    int worker = spawn_unit(SIDE, U_DRONE, b[0].x + 8, b[0].y);
    assert(worker >= 0);
    u[worker].order = ORDER_MINE; u[worker].target = 0; u[worker].carry = 6;
    old_minerals = minerals;
    update_orders();
    assert(minerals == old_minerals + 6 && u[worker].carry == 0);
    u[worker].order = ORDER_GAS; u[worker].target = 1; u[worker].carry = 8;
    b[1].type = B_EXTRACTOR;
    old_gas = gas;
    update_orders();
    assert(gas == old_gas + 8 && u[worker].carry == 0);

    /* These imported/upgraded structures are recognized, not constructible
       through the ordinary building placement path. */
    assert(!race_can_site(SIDE, B_LAIR, b[0].x + 200, b[0].y));
    assert(!race_can_site(SIDE, B_HIVE, b[0].x + 200, b[0].y));
    assert(!race_can_site(SIDE, B_GREATER_SPIRE, b[0].x + 200, b[0].y));
}

static void check_incomplete_townhall(int type)
{
    fixture(type, 1);
    assert(!race_owner_has_prereq(SIDE, SC_PR_HATCHERY));
    assert(!race_site_creep(SIDE, b[0].x + 100, b[0].y));
    assert(!race_queue_unit(SIDE, 0, U_DRONE));
    assert(b[0].queue_count == 0 && b[0].larva == 0);
    assert(race_owner_supply_cap(SIDE) == 0);
    for (int i = 0; i < 428; ++i) race_tick_state();
    assert(b[0].larva == 0);
    race_complete_building(0);
    assert(building_complete(0) && b[0].larva == 3);
    assert(race_owner_has_prereq(SIDE, SC_PR_HATCHERY));
    assert(race_owner_supply_cap(SIDE) == 2);
    assert(race_queue_unit(SIDE, 0, U_DRONE));
}

int main(void)
{
    gba_init();
    check_townhall(B_HATCHERY);
    check_townhall(B_LAIR);
    check_townhall(B_HIVE);
    check_incomplete_townhall(B_LAIR);
    check_incomplete_townhall(B_HIVE);
    return 0;
}
