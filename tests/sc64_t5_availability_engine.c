/* The fixture writes only test setup and normal queue/save inputs; it does
 * not claim to prove a controller-completed Revolution campaign. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

static void start_t5(void)
{
    game_init();
    race_mode = 0;
    race_selected_mission = SC64_T5_GLOBAL_ID;
    init_state();
    assert(race_t5_active() && screen == SCREEN_PLAY);
}

static int find_building(int owner, int type)
{
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == owner && b[i].type == type &&
            building_complete(i)) return i;
    return -1;
}

static int fixture_building(int owner, int type, int x, int y)
{
    int slot = spawn_build(owner, type, x, y, 0);
    assert(slot >= 0);
    b[slot].raw_owner = owner == SIDE ? 0 : owner == ENEMY ? 6 : -1;
    return slot;
}

static void assert_rejected_without_payment(int owner, int producer, int type)
{
    int old_minerals = race_owner_minerals(owner);
    int old_gas = race_owner_gas(owner);
    int old_count = b[producer].queue_count;
    int old_head = b[producer].queue_head;
    int old_larva = b[producer].larva;
    int old_tail = (old_head + old_count) % 5;
    int old_tail_value = b[producer].queue[old_tail];
    assert(!race_queue_unit(owner, producer, type));
    assert(race_owner_minerals(owner) == old_minerals);
    assert(race_owner_gas(owner) == old_gas);
    assert(b[producer].queue_count == old_count);
    assert(b[producer].queue_head == old_head);
    assert(b[producer].larva == old_larva);
    assert(b[producer].queue[old_tail] == old_tail_value);
}

static void source_specific_production(void)
{
    int barracks, factory, command;
    int old_minerals, old_gas;
    start_t5();
    assert(race_t5_source_product_available(0, 0));
    assert(!race_t5_source_product_available(0, 5));
    assert(race_t5_source_product_available(6, 5));
    assert(!race_t5_source_product_available(12, 5));
    assert(!race_t5_source_product_available(0, 228));
    assert(!race_t5_unit_product_available(RESCUABLE, 4, U_MARINE));
    assert(race_t5_unit_product_available(RESCUABLE, 2, U_MARINE));
    assert(race_t5_unit_product_available(RESCUABLE, 3, U_MARINE));

    barracks = find_building(SIDE, B_BARRACKS);
    factory = find_building(ENEMY, B_FACTORY);
    command = find_building(SIDE, B_COMMAND);
    if (barracks < 0) barracks = fixture_building(SIDE, B_BARRACKS, 1200, 1200);
    if (command < 0) command = fixture_building(SIDE, B_COMMAND, 1350, 1200);
    assert(barracks >= 0 && command >= 0);
    minerals = gas = 500;
    old_minerals = minerals; old_gas = gas;
    assert(race_queue_unit(SIDE, barracks, U_MARINE));
    assert(minerals == old_minerals - sc64_race_unit_rule(U_MARINE)->minerals);
    assert(gas == old_gas);
    assert(b[barracks].queue[(b[barracks].queue_head + b[barracks].queue_count - 1) % 5] == U_MARINE);

    /* Human PUNI overrides deny Ghost/Tank/Goliath before any wallet or queue mutation. */
    assert_rejected_without_payment(SIDE, barracks, U_GHOST);
    assert(race_t5_unit_product_available(SIDE, 0, U_TANK) == 0);
    assert(race_t5_unit_product_available(SIDE, 0, U_GOLIATH) == 0);
    assert(race_t5_unit_product_available(SIDE, 0, U_GHOST) == 0);
    assert(race_t5_unit_product_available(SIDE, 0, U_WRAITH));
    assert(race_t5_unit_product_available(SIDE, 0, U_DROPSHIP));
    assert(race_t5_building_product_available(SIDE, 0, B_CONTROL_TOWER));

    /* Enemy slot 6 has its own override and can queue a Goliath. */
    assert(factory >= 0);
    race_minerals[ENEMY] = 1000;
    race_gas[ENEMY] = 1000;
    assert(race_queue_unit(ENEMY, factory, U_GOLIATH));
    assert(b[factory].queue[(b[factory].queue_head + b[factory].queue_count - 1) % 5] == U_GOLIATH);

    /* A disabled future training product does not invalidate a source-imported actor. */
    {
        int preplaced = -1;
        for (int i = 0; i < MAX_UNITS; ++i)
            if (u[i].alive && u[i].raw_type == 5) { preplaced = i; break; }
        if (preplaced >= 0) assert(u[preplaced].alive && u[preplaced].raw_type == 5);
    }
}

static void building_path_and_non_t5_behavior(void)
{
    int scv = -1, starport, factory;
    start_t5();
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].type == U_SCV) { scv = i; break; }
    if (scv < 0) scv = spawn_unit(SIDE, U_SCV, 1200, 1350);
    assert(scv >= 0);
    assert(race_t5_building_product_available(SIDE, u[scv].raw_owner, B_CONTROL_TOWER));
    /* The ordinary worker builder cannot bypass the dedicated add-on path. */
    {
        int ore = minerals, vespene = gas;
        assert(!race_start_build(SIDE, scv, B_CONTROL_TOWER, u[scv].x, u[scv].y));
        assert(minerals == ore && gas == vespene);
    }
    (void)fixture_building(SIDE, B_COMMAND, 1100, 1000);
    (void)fixture_building(SIDE, B_COMMAND, 1080, 1150);
    factory = fixture_building(SIDE, B_FACTORY, 1200, 1000);
    starport = fixture_building(SIDE, B_STARPORT, 1350, 1000);
    minerals = gas = 1000;
    {
        int ore = minerals, vespene = gas;
        if (race_t5_start_control_tower(starport)) {
            assert(minerals < ore && gas < vespene);
        } else {
            assert(minerals == ore && gas == vespene);
        }
    }
    if (!race_t5_starport_has_control_tower(starport)) {
        int tower = spawn_build(SIDE, B_CONTROL_TOWER,
            b[starport].x + SC64_T5_ADDON_X_OFFSET,
            b[starport].y + SC64_T5_ADDON_Y_OFFSET, 0);
        assert(tower >= 0);
        b[tower].level = starport + 1;
        b[tower].raw_type = 115;
        b[tower].raw_owner = 0;
    }
    assert(race_t5_starport_has_control_tower(starport));
    selected_building = factory;
    train_type = U_VULTURE;
    select_next_train_type();
    assert(train_type == U_VULTURE);
    selected_building = starport;
    train_type = U_WRAITH;
    select_next_train_type();
    assert(train_type == U_DROPSHIP);
    select_next_train_type();
    assert(train_type == U_WRAITH);
    minerals = gas = 1000;
    assert(race_queue_unit(SIDE, starport, U_DROPSHIP));
    assert(race_queue_unit(SIDE, starport, U_WRAITH));
    assert(b[factory].alive);
    race_selected_mission = 7;
    assert(!race_t5_active());
    assert(race_t5_unit_product_available(SIDE, 0, U_GHOST));
    assert(race_t5_unit_product_available(SIDE, 0, U_TANK));
}

static void queued_save_validation_is_atomic(void)
{
    int barracks;
    Unit before_units[MAX_UNITS];
    Building before_buildings[MAX_BUILDINGS];
    Sc64T5RuntimeState before_runtime;
    int before_minerals, before_gas;
    start_t5();
    barracks = find_building(SIDE, B_BARRACKS);
    if (barracks < 0) barracks = fixture_building(SIDE, B_BARRACKS, 1200, 1200);
    (void)fixture_building(SIDE, B_COMMAND, 1350, 1200);
    minerals = 500;
    gas = 100;
    assert(barracks >= 0 && race_queue_unit(SIDE, barracks, U_MARINE));
    assert(race_save_game());
    assert(race_t5_candidate_queues_available(&race_data));

    /* Forge a CRC-valid candidate that queues the source-disabled Tank. */
    race_data.buildings[barracks][offsetof(Building, queue) / sizeof(int) +
                                  b[barracks].queue_head] = U_TANK;
    save_write(&race_data, sizeof(race_data), SAVE_VERSION);
    memcpy(before_units, u, sizeof(u));
    memcpy(before_buildings, b, sizeof(b));
    before_runtime = race_t5;
    before_minerals = minerals; before_gas = gas;
    race_mode = 1; /* The candidate remains a T5 save while the live world is not T5. */
    assert(!race_t5_active());
    assert(!race_t5_candidate_queues_available(&race_data));
    assert(!race_load_game());
    assert(!memcmp(before_units, u, sizeof(u)));
    assert(!memcmp(before_buildings, b, sizeof(b)));
    assert(!memcmp(&before_runtime, &race_t5, sizeof(race_t5)));
    assert(minerals == before_minerals && gas == before_gas);
    assert(race_mode == 1);
}

int main(void)
{
    source_specific_production();
    building_path_and_non_t5_behavior();
    queued_save_validation_is_atomic();
    puts("T5 source availability gates and queued-save validation passed");
    return 0;
}
