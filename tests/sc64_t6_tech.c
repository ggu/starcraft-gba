/* T6 PUNI selection and Control Tower requirement checks. */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/game.c"

#ifndef SC64_T6_RUNTIME
#error "Compile with the internal T6 runtime profile"
#endif

static void test_source_owner_mapping(void)
{
    assert(race_t6_product_source_slot(SIDE, -1) == 0);
    assert(race_t6_product_source_slot(ENEMY, -1) == 1);
    assert(race_t6_product_source_slot(4, 6) == 4);
    assert(race_t6_product_source_slot(RESCUABLE, 2) == 2);
    assert(race_t6_product_source_slot(RESCUABLE, 6) == 6);
    assert(race_t6_product_source_slot(RESCUABLE, 1) == -1);
    assert(race_t6_product_source_slot(NEUTRAL, 2) == -1);
    assert(!race_t6_candidate_product_available(-1, 0));
    assert(!race_t6_candidate_product_available(12, 0));
    assert(!race_t6_candidate_product_available(0, 228));
}

static void test_puni_bounds_are_closed(void)
{
    assert(!race_t6_source_product_available(-1, 0));
    assert(!race_t6_source_product_available(12, 0));
    assert(!race_t6_source_product_available(0, 228));
    assert(!race_t6_source_product_available(0, -1));
    assert(!race_t6_source_product_available(0, -1));
}

static void test_t6_gate_has_no_effect_on_earlier_missions(void)
{
    game_init();
    race_mode = 0;
    race_selected_mission = 7;
    init_state();
    assert(!race_t6_active());
    assert(race_t6_unit_product_available(SIDE, 0, U_TANK));
    assert(race_t6_building_product_available(SIDE, 0, B_CONTROL_TOWER));
}

static int find_starport_site(int *out_x, int *out_y)
{
    for (int y = 64; y < WORLD_H - 64; y += 16)
        for (int x = 64; x < WORLD_W - 96; x += 16)
            if (can_place(B_STARPORT, x, y) &&
                can_place(B_CONTROL_TOWER, x + SC64_T6_ADDON_X_OFFSET,
                          y + SC64_T6_ADDON_Y_OFFSET)) {
                *out_x = x;
                *out_y = y;
                return 1;
            }
    return 0;
}

static int make_building(int owner, int type, int x, int y, int complete)
{
    int slot = spawn_build(owner, type, x, y, !complete);
    assert(slot >= 0);
    if (complete) {
        b[slot].under_construction = 0;
        b[slot].hp = b[slot].maxhp;
    }
    return slot;
}

static void reset_fixture(void)
{
    game_init();
    race_mode = 0;
    race_selected_mission = SC64_T6_GLOBAL_ID;
    init_state();
    assert(race_t6_active() && screen == SCREEN_PLAY);
    memset(b, 0, sizeof(b));
    memset(u, 0, sizeof(u));
    for (int i = 0; i < MAX_BUILDINGS; ++i) b[i].level = 0;
    minerals = 1000;
    gas = 1000;
    supply_cap = 200;
    supply_used = 0;
    invalidate_path_grid();
}

static void test_disabled_product_rejects_without_payment(void)
{
    int x, y, factory, starport;
    reset_fixture();
    assert(find_starport_site(&x, &y));
    factory = make_building(SIDE, B_FACTORY, x - 96, y + 96, 1);
    (void)factory;
    starport = make_building(SIDE, B_STARPORT, x, y, 1);
    assert(b[starport].raw_owner == 0);
    assert(!race_t6_source_product_available(0, 5)); /* Disabled Tank. */
    assert(race_t6_unit_product_available(SIDE, 0, U_MARINE));
    assert(!race_t6_unit_product_available(SIDE, 0, U_TANK));
    /* A disabled Factory product must be rejected before charging or queueing. */
    int factory_ore = race_owner_minerals(SIDE);
    int factory_gas = race_owner_gas(SIDE);
    assert(!race_queue_unit(SIDE, factory, U_TANK));
    assert(race_owner_minerals(SIDE) == factory_ore &&
           race_owner_gas(SIDE) == factory_gas && !b[factory].queue_count);
}

static void test_paid_tower_completion_gates_dropships(void)
{
    int x, y, factory, command, starport, other_starport, tower;
    reset_fixture();
    assert(find_starport_site(&x, &y));
    factory = make_building(SIDE, B_FACTORY, x - 96, y + 96, 1);
    command = make_building(SIDE, B_COMMAND, x - 192, y - 96, 1);
    (void)command;
    starport = make_building(SIDE, B_STARPORT, x, y, 1);
    other_starport = make_building(SIDE, B_STARPORT, x + 160, y, 1);
    assert(race_t6_building_product_available(SIDE, 0, B_CONTROL_TOWER));

    int ore = race_owner_minerals(SIDE), vespene = race_owner_gas(SIDE);
    assert(!race_queue_unit(SIDE, starport, U_DROPSHIP));
    assert(race_owner_minerals(SIDE) == ore && race_owner_gas(SIDE) == vespene);
    assert(!b[starport].queue_count);
    assert(!race_t6_start_control_tower(factory));
    assert(race_t6_start_control_tower(starport));
    assert(race_owner_minerals(SIDE) == ore - 50 &&
           race_owner_gas(SIDE) == vespene - 50);
    tower = -1;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].type == B_CONTROL_TOWER) tower = i;
    assert(tower >= 0 && b[tower].under_construction && b[tower].hp == 1);
    assert(b[tower].raw_type == 115 && b[tower].raw_owner == 0);
    assert(b[tower].level == starport + 1);
    assert(!race_t6_can_train_dropship(starport));
    assert(!race_queue_unit(SIDE, starport, U_DROPSHIP));
    for (int i = 0; i < 749; ++i) race_t6_tech_tick();
    assert(b[tower].under_construction);
    race_t6_tech_tick();
    assert(!b[tower].under_construction && b[tower].hp == 500);
    assert(race_t6_starport_has_control_tower(starport));
    assert(!race_t6_starport_has_control_tower(other_starport));
    assert(race_t6_can_train_dropship(starport));
    assert(race_queue_unit(SIDE, starport, U_DROPSHIP));
    assert(b[starport].queue_count == 1 && b[starport].queue[0] == U_DROPSHIP);
}

int main(void)
{
    test_source_owner_mapping();
    test_puni_bounds_are_closed();
    test_t6_gate_has_no_effect_on_earlier_missions();
    test_disabled_product_rejects_without_payment();
    test_paid_tower_completion_gates_dropships();
    puts("T6 tech helper tests passed");
    return 0;
}
