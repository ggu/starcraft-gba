/* Real-engine worker cargo/input fixture. Compile with SC64_WORKER_CARGO_KIND
 * and a prepared race profile; no imported recipe or game assets are read. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

static void
reset_scene(int human, int enemy)
{
    game_init();
    init_state();
    memset(u, 0, sizeof(u));
    memset(b, 0, sizeof(b));
    memset(m, 0, sizeof(m));
    memset(path_walkable, 0xff, sizeof(path_walkable));
    memset(race_player, 0xff, sizeof(race_player));
    race_mode = 1;
    race_human = human;
    race_bots = 0;
    race_player[SIDE] = human;
    race_player[ENEMY] = enemy;
    race_selected_mission = 0;
    mode = MODE_NORMAL;
    menu_open = box_state = 0;
    selected_building = -1;
    unit_tap = -1;
    minerals = gas = 0;
    frame = 1;
    path_grid_valid = 1;
    target_lists_valid = 0;
    sc64_target_index_active = 0;
    SC64_GRID_CLEAR(visible);
    for (int y = 0; y < MAP_ROWS; ++y)
        for (int x = 0; x < MAP_COLS; ++x)
            SC64_GRID_SET(visible, x + y * MAP_COLS, 1);
}

static void
tap_a(int x, int y)
{
    cx = x; cy = y;
    ++frame;
    interact();
}

static void
one_tick(void)
{
    update_orders();
    ++frame;
}

static void
set_depot(int slot, int owner, int type, int x, int y)
{
    memset(&b[slot], 0, sizeof(b[slot]));
    b[slot].alive = 1; b[slot].owner = owner; b[slot].type = type;
    b[slot].x = x; b[slot].y = y;
    b[slot].hp = b[slot].maxhp = 1000;
}

static void
test_mineral_input(int race, int worker_type)
{
    int worker, worker2, bystander, patch = 4;
    int depot_type = race == R_ZERG ? B_HATCHERY : race == R_PROTOSS ? B_NEXUS : B_COMMAND;
    reset_scene(race, race == R_TER ? R_ZERG : R_TER);
    worker = spawn_unit(SIDE, worker_type, 112, 112);
    worker2 = spawn_unit(SIDE, worker_type, 128, 112);
    bystander = spawn_unit(SIDE, U_MARINE, 144, 112);
    assert(worker >= 0 && worker2 >= 0 && bystander >= 0);
    set_depot(0, SIDE, depot_type, 112, 112);
    u[worker].selected = 1;
    u[worker2].selected = 1;
    u[bystander].selected = 1;
    m[patch].x = 320; m[patch].y = 320; m[patch].amount = 24;
#ifdef SC64_T6_RUNTIME
    u[worker].order = ORDER_REPAIR;
    u[worker].carry = 100; /* Repair Q8 remainder is not resource cargo. */
#endif

    tap_a(m[patch].x, m[patch].y);
    assert(u[worker].order == ORDER_MINE && u[worker].target == patch);
    assert(u[worker].flags & SC64_WORKER_GATHER_PENDING);
    assert(u[worker2].order == ORDER_MINE && u[worker2].target == patch);
    assert(u[bystander].order == ORDER_IDLE); /* workers only */
#ifdef SC64_T6_RUNTIME
    assert(u[worker].carry == 0);
#endif

    /* A repeated order is a patch trip even with same-kind cargo: no early
     * deposit before arrival, then return without a second pickup. */
    u[worker].carry = 5;
    u[worker].flags &= ~SC64_WORKER_CARGO_KIND_MASK; /* minerals */
    minerals = 0;
    one_tick();
    assert(u[worker].carry == 5 && minerals == 0);
    assert(u[worker].flags & SC64_WORKER_GATHER_PENDING);
    u[worker].x = m[patch].x; u[worker].y = m[patch].y;
    one_tick();
    assert(!(u[worker].flags & SC64_WORKER_GATHER_PENDING));
    assert(u[worker].carry == 5 && minerals == 0);
    u[worker].x = b[0].x; u[worker].y = b[0].y;
    one_tick();
    assert(u[worker].carry == 0 && minerals == 5 && gas == 0);
}

static void
test_gas_cargo_retarget_and_credit(int race, int worker_type,
                                  int depot_type, int refinery_type)
{
    int worker, patch = 3;
    reset_scene(race, race == R_TER ? R_ZERG : R_TER);
    worker = spawn_unit(SIDE, worker_type, 100, 100);
    assert(worker >= 0);
    u[worker].selected = 1;
    u[worker].carry = 6;
    u[worker].flags |= SC64_WORKER_CARGO_GAS;
    m[patch].x = 320; m[patch].y = 320; m[patch].amount = 5;
    tap_a(m[patch].x, m[patch].y);
    assert(u[worker].order == ORDER_MINE && u[worker].target == patch);
    assert(u[worker].carry == 6 &&
           (u[worker].flags & SC64_WORKER_CARGO_KIND_MASK) == SC64_WORKER_CARGO_GAS);
    u[worker].x = m[patch].x; u[worker].y = m[patch].y;
    one_tick();
    assert(u[worker].carry == 5 && m[patch].amount == 0);
    assert(!(u[worker].flags & SC64_WORKER_CARGO_KIND_MASK)); /* mineral kind */
    assert(minerals == 0 && gas == 0);

    /* A gas pickup overwrites mineral cargo only on a successful pickup. */
    reset_scene(race, race == R_TER ? R_ZERG : R_TER);
    worker = spawn_unit(SIDE, worker_type, 100, 100);
    assert(worker >= 0);
    u[worker].selected = 1; u[worker].carry = 4;
    set_depot(0, SIDE, refinery_type, 320, 320);
    set_depot(1, SIDE, depot_type, 100, 100);
#ifdef SC64_T6_RUNTIME
    u[worker].order = ORDER_REPAIR;
    u[worker].carry = 100;
#endif
    tap_a(320, 320);
    assert(u[worker].order == ORDER_GAS && u[worker].target == 0);
#ifdef SC64_T6_RUNTIME
    assert(u[worker].carry == 0);
#endif
#ifndef SC64_T6_RUNTIME
    assert(u[worker].carry == 4);
#endif
    u[worker].x = 320; u[worker].y = 320;
    one_tick();
    assert(u[worker].carry == 8);
    assert((u[worker].flags & SC64_WORKER_CARGO_KIND_MASK) == SC64_WORKER_CARGO_GAS);
    assert(minerals == 0 && gas == 0);
    (void)depot_type;
}

static void
test_kind_based_deposit(int race, int worker_type, int depot_type)
{
    int worker;
    reset_scene(race, race == R_TER ? R_ZERG : R_TER);
    worker = spawn_unit(SIDE, worker_type, 120, 120);
    assert(worker >= 0);
    set_depot(0, SIDE, depot_type, 120, 120);
    u[worker].carry = 5;
    u[worker].order = ORDER_MINE;
    u[worker].target = -1; /* Cargo identity controls credit and no target lookup. */
    one_tick();
    assert(u[worker].carry == 0 && minerals == 5 && gas == 0);

    reset_scene(race, race == R_TER ? R_ZERG : R_TER);
    worker = spawn_unit(SIDE, worker_type, 120, 120);
    assert(worker >= 0);
    set_depot(0, SIDE, depot_type, 120, 120);
    u[worker].carry = 8;
    u[worker].order = ORDER_GAS;
    u[worker].target = -1;
    u[worker].flags |= SC64_WORKER_CARGO_GAS;
    one_tick();
    assert(u[worker].carry == 0 && minerals == 0 && gas == 8);
}

static void
test_owned_building_priority(int race, int worker_type, int depot_type)
{
    int worker, patch = 1;
    reset_scene(race, race == R_TER ? R_ZERG : R_TER);
    worker = spawn_unit(SIDE, worker_type, 200, 200);
    assert(worker >= 0);
    u[worker].selected = 1;
    set_depot(0, SIDE, depot_type, 320, 320);
    selected_building = 0;
    minerals = gas = 2000;
    m[patch].x = 320; m[patch].y = 320; m[patch].amount = 20;
    tap_a(320, 320);
    assert(u[worker].order == ORDER_IDLE);
    assert(!(u[worker].flags & SC64_WORKER_GATHER_PENDING));
}

static void
test_build_clears_pending(int race, int worker_type, int building_type)
{
    int worker;
    reset_scene(race, race == R_TER ? R_ZERG : R_TER);
    worker = spawn_unit(SIDE, worker_type, 200, 200);
    assert(worker >= 0);
    u[worker].carry = 8;
    u[worker].order = ORDER_MINE;
    u[worker].target = 0;
    u[worker].flags |= SC64_WORKER_CARGO_GAS | SC64_WORKER_GATHER_PENDING;
    minerals = gas = 2000;
    assert(race_start_build(SIDE, worker, building_type, 480, 480));
    assert(u[worker].order == ORDER_BUILD);
    assert(u[worker].carry == 8);
    assert((u[worker].flags & SC64_WORKER_CARGO_KIND_MASK) == SC64_WORKER_CARGO_GAS);
    assert(!(u[worker].flags & SC64_WORKER_GATHER_PENDING));
    assert(sc64_worker_cargo_badge_kind(&u[worker]) == 2);
}

#ifdef SC64_DIRECT_ATTACK
static void
test_direct_attack_clears_pending(void)
{
    int worker, enemy;
    reset_scene(R_TER, R_ZERG);
    worker = spawn_unit(SIDE, U_SCV, 112, 112);
    enemy = spawn_unit(ENEMY, U_MARINE, 144, 112);
    assert(worker >= 0 && enemy >= 0);
    u[worker].raw_type = 7; u[worker].raw_owner = 0;
    u[enemy].raw_type = 0; u[enemy].raw_owner = 1;
    u[worker].selected = 1;
    u[worker].carry = 6;
    u[worker].order = ORDER_MINE;
    u[worker].target = 0;
    u[worker].flags |= SC64_WORKER_CARGO_GAS | SC64_WORKER_GATHER_PENDING;
    assert(sc64_direct_attack_issue_target(enemy) == 1);
    assert(u[worker].order == ORDER_ATTACK_TARGET);
    assert(u[worker].carry == 6);
    assert((u[worker].flags & SC64_WORKER_CARGO_KIND_MASK) == SC64_WORKER_CARGO_GAS);
    assert(!(u[worker].flags & SC64_WORKER_GATHER_PENDING));
}
#endif

static void
test_interrupt_unknown_depleted_and_priority(int race, int worker_type,
                                              int depot_type, int refinery_type)
{
    int worker, enemy, patch = 2;
    reset_scene(race, race == R_TER ? R_ZERG : R_TER);
    worker = spawn_unit(SIDE, worker_type, 96, 96);
    assert(worker >= 0);
    u[worker].selected = 1; u[worker].carry = 7;
    u[worker].flags |= SC64_WORKER_CARGO_UNKNOWN;
    assert(sc64_worker_cargo_badge_kind(&u[worker]) == 0);
    m[patch].x = 300; m[patch].y = 300; m[patch].amount = 12;
    m[patch + 1].x = 304; m[patch + 1].y = 300; m[patch + 1].amount = 100;
    tap_a(300, 300);
    assert(u[worker].order == ORDER_MINE && (u[worker].flags & SC64_WORKER_GATHER_PENDING));
    m[patch].amount = 0; /* The exact patch depleted before the order arrives. */
    one_tick();
    assert(u[worker].order == ORDER_IDLE && u[worker].carry == 7);
    assert(u[worker].target == patch); /* Never silently substitute a nearby patch. */
    assert((u[worker].flags & SC64_WORKER_CARGO_KIND_MASK) == SC64_WORKER_CARGO_UNKNOWN);
    assert(!(u[worker].flags & SC64_WORKER_GATHER_PENDING));

    /* Explicit new orders clear only pending; STOP and MOVE retain cargo. */
    issue_order(ORDER_MOVE, 160, 160);
    assert(u[worker].order == ORDER_MOVE && u[worker].carry == 7);
    assert((u[worker].flags & SC64_WORKER_CARGO_KIND_MASK) == SC64_WORKER_CARGO_UNKNOWN);
    stop_selected();
    assert(u[worker].order == ORDER_IDLE && u[worker].carry == 7);
    assert((u[worker].flags & SC64_WORKER_CARGO_KIND_MASK) == SC64_WORKER_CARGO_UNKNOWN);
    frame = 30; economy();
    assert(u[worker].order == ORDER_IDLE && u[worker].carry == 7);

    u[worker].flags &= ~SC64_WORKER_CARGO_KIND_MASK;
    u[worker].flags |= SC64_WORKER_CARGO_GAS;
    assert(sc64_worker_cargo_badge_kind(&u[worker]) == 2);
    u[worker].order = ORDER_MOVE;
    assert(sc64_worker_cargo_badge_kind(&u[worker]) == 2); /* order independent */
    u[worker].flags &= ~SC64_WORKER_CARGO_KIND_MASK;
    assert(sc64_worker_cargo_badge_kind(&u[worker]) == 1);
#ifdef SC64_T6_RUNTIME
    u[worker].order = ORDER_REPAIR;
    u[worker].carry = 100;
    assert(sc64_worker_cargo_badge_kind(&u[worker]) == 0);
#endif

    /* Owned refinery beats a patch; enemy combat target beats a patch. */
    u[worker].carry = 0; u[worker].flags &= ~SC64_WORKER_CARGO_KIND_MASK;
    m[patch].amount = 12;
    set_depot(0, SIDE, refinery_type, 300, 300);
    tap_a(300, 300);
    assert(u[worker].order == ORDER_GAS && u[worker].target == 0);
    reset_scene(race, race == R_TER ? R_ZERG : R_TER);
    worker = spawn_unit(SIDE, worker_type, 96, 96);
    enemy = spawn_unit(ENEMY, U_MARINE, 300, 300);
    assert(worker >= 0 && enemy >= 0);
    u[worker].selected = 1;
    m[patch].x = 300; m[patch].y = 300; m[patch].amount = 12;
    tap_a(300, 300);
    assert(u[worker].order == ORDER_ATTACK_MOVE);
    assert(!(u[worker].flags & SC64_WORKER_GATHER_PENDING));
    (void)depot_type;
}

int
main(void)
{
    test_mineral_input(R_TER, U_SCV);
    test_mineral_input(R_ZERG, U_DRONE);
    test_mineral_input(R_PROTOSS, U_PROBE);
    test_gas_cargo_retarget_and_credit(R_TER, U_SCV, B_COMMAND, B_REFINERY);
    test_gas_cargo_retarget_and_credit(R_ZERG, U_DRONE, B_HATCHERY, B_EXTRACTOR);
    test_gas_cargo_retarget_and_credit(R_PROTOSS, U_PROBE, B_NEXUS, B_ASSIMILATOR);
    test_kind_based_deposit(R_TER, U_SCV, B_COMMAND);
    test_kind_based_deposit(R_ZERG, U_DRONE, B_HATCHERY);
    test_kind_based_deposit(R_PROTOSS, U_PROBE, B_NEXUS);
    test_owned_building_priority(R_TER, U_SCV, B_COMMAND);
    test_owned_building_priority(R_ZERG, U_DRONE, B_HATCHERY);
    test_owned_building_priority(R_PROTOSS, U_PROBE, B_NEXUS);
    test_build_clears_pending(R_TER, U_SCV, B_DEPOT);
    test_build_clears_pending(R_ZERG, U_DRONE, B_HATCHERY);
    test_build_clears_pending(R_PROTOSS, U_PROBE, B_PYLON);
#ifdef SC64_DIRECT_ATTACK
    test_direct_attack_clears_pending();
#endif
    test_interrupt_unknown_depleted_and_priority(R_TER, U_SCV, B_COMMAND, B_REFINERY);
    test_interrupt_unknown_depleted_and_priority(R_ZERG, U_DRONE, B_HATCHERY, B_EXTRACTOR);
    test_interrupt_unknown_depleted_and_priority(R_PROTOSS, U_PROBE, B_NEXUS, B_ASSIMILATOR);
    puts("worker gather engine input/cargo checks passed");
    return 0;
}
