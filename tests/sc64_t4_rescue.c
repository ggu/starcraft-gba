/* Synthetic real-engine T4 rescue adapter fixtures. These exercise the
 * ownership policy through race_t4_world_tick(); they are not controller-win
 * proofs and do not modify production runtime state outside normal test setup. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

static void
clear_scene(void)
{
    memset(u, 0, sizeof(u));
    memset(b, 0, sizeof(b));
    memset(invincible_units, 0, sizeof(invincible_units));
    memset(invincible_buildings, 0, sizeof(invincible_buildings));
    race_selected_mission = SC64_T4_GLOBAL_ID;
    race_mode = 0;
    race_campaign.mission = SC64_T4_GLOBAL_ID;
    race_campaign.phase = SC64_RACE_PHASE_ACTIVE;
    race_campaign.elapsed_frames = 0;
    race_campaign.message_ticks = 0;
    race_campaign.objective_flags = 0;
    race_cols = race_rows = 128;
    race_player[SIDE] = R_TER;
    race_player[ENEMY] = R_ZERG;
    race_player[4] = R_ZERG;
    race_player[RESCUABLE] = R_TER;
    race_player[NEUTRAL] = R_TER;
    supply_used = 0;
    supply_cap = 0;
    target_lists_valid = 0;
    sc64_target_index_end();
    memset(path_walkable, 0xff, sizeof(path_walkable));
    path_grid_valid = 0;
    sc64_visibility_reset();
}

static int
put_unit(int owner, int type, int raw_owner, int x, int y)
{
    int slot = spawn_unit(owner, type, x, y);
    assert(slot >= 0);
    u[slot].raw_owner = raw_owner;
    u[slot].raw_type = type == U_MAP_REVEALER ? 101 : 15;
    return slot;
}

static int
put_building(int owner, int type, int raw_owner, int x, int y, int under_construction)
{
    int slot = spawn_build(owner, type, x, y, under_construction);
    assert(slot >= 0);
    b[slot].raw_owner = raw_owner;
    b[slot].raw_type = type == B_COMMAND ? 106 : 131;
    return slot;
}

static int
count_owner_units(int owner)
{
    int i, n = 0;
    for (i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == owner) ++n;
    return n;
}

static void
test_source_start_has_no_automatic_rescue(void)
{
    int i, saw_rescue_source = 0;
    game_init();
    race_selected_mission = SC64_T4_GLOBAL_ID;
    init_state();
    assert(race_campaign.mission == SC64_T4_GLOBAL_ID);
    /* The initial scenario is loaded with Rescue Passive owners intact. */
    for (i = 0; i < MAX_UNITS; ++i) {
        if (u[i].alive && (u[i].raw_owner == 1 || u[i].raw_owner == 2 ||
                           u[i].raw_owner == 7)) {
            saw_rescue_source = 1;
            assert(u[i].owner == RESCUABLE);
        }
    }
    for (i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && (b[i].raw_owner == 1 || b[i].raw_owner == 2 ||
                           b[i].raw_owner == 7))
            assert(b[i].owner == RESCUABLE);
    assert(saw_rescue_source);
}

static void
test_individual_rescue_edges_and_supply(void)
{
    int rescue, human, far_same_slot, attacker;
    clear_scene();
    rescue = put_unit(RESCUABLE, U_CIVILIAN, 1, 100, 100);
    human = put_unit(SIDE, U_SCV, 0, 132, 100); /* inclusive 32px GBA edge */
    far_same_slot = put_unit(RESCUABLE, U_CIVILIAN, 1, 500, 500);
    attacker = put_unit(SIDE, U_MARINE, 0, 120, 100);
    u[attacker].order = ORDER_ATTACK_TARGET;
    u[attacker].target = rescue;
    supply_used = 999; /* The normal state tick must refresh derived supply. */
    race_t4_world_tick();
    assert(u[rescue].owner == SIDE);
    assert(u[rescue].raw_owner == 1);
    assert(sc64_t4_current_source_owner(u[rescue].owner, u[rescue].raw_owner) == 0);
    assert(u[far_same_slot].owner == RESCUABLE); /* non-depot rescue is individual */
    assert(u[attacker].order == ORDER_IDLE && u[attacker].target == -1);
    race_tick_state(); /* The normal game_update cadence ticks state once outside rescue. */
    assert(supply_used == race_owner_supply(SIDE));
    assert(supply_used == 4); /* SCV and Marine; Civilian costs no supply. */
    (void)human;

    clear_scene();
    rescue = put_unit(RESCUABLE, U_CIVILIAN, 2, 100, 100);
    put_unit(SIDE, U_SCV, 0, 133, 100); /* 33px outside */
    race_t4_world_tick();
    assert(u[rescue].owner == RESCUABLE);

    clear_scene();
    rescue = put_unit(RESCUABLE, U_CIVILIAN, 7, 100, 100);
    put_unit(SIDE, U_SCV, 0, 132, 132); /* diagonal corner of the rectangle */
    race_t4_world_tick();
    assert(u[rescue].owner == SIDE);
}

static void
test_rescue_does_not_advance_simulation_state(void)
{
    int rescued;
    int hall, shielded;
    clear_scene();
    race_player[SIDE] = R_ZERG;
    race_player[4] = R_PROTOSS;
    rescued = put_unit(RESCUABLE, U_CIVILIAN, 1, 100, 100);
    put_unit(SIDE, U_SCV, 0, 132, 100);
    hall = put_building(SIDE, B_HATCHERY, 0, 1000, 1000, 0);
    b[hall].larva = 1;
    b[hall].larva_timer = 427;
    shielded = put_unit(4, U_PROBE, 4, 1000, 1000);
    u[shielded].shield = 1;
    u[shielded].shield_remainder = 313;

    race_t4_world_tick();
    assert(u[rescued].owner == SIDE);
    assert(b[hall].larva == 1 && b[hall].larva_timer == 427);
    assert(u[shielded].shield == 1 && u[shielded].shield_remainder == 313);

    /* The enclosing normal update performs exactly one state tick. */
    race_tick_state();
    assert(b[hall].larva == 2 && b[hall].larva_timer == 0);
    assert(u[shielded].shield == 2 && u[shielded].shield_remainder == 0);
}

static void
test_rescued_actor_chain_and_hidden_revealer(void)
{
    int first, second;
    clear_scene();
    first = put_unit(SIDE, U_CIVILIAN, 1, 100, 100);
    second = put_unit(RESCUABLE, U_CIVILIAN, 2, 132, 100);
    race_t4_world_tick();
    assert(u[first].owner == SIDE && u[first].raw_owner == 1);
    assert(u[second].owner == SIDE && u[second].raw_owner == 2);

    clear_scene();
    second = put_unit(RESCUABLE, U_CIVILIAN, 1, 100, 100);
    put_unit(SIDE, U_MAP_REVEALER, 0, 100, 100);
    race_t4_world_tick();
    assert(u[second].owner == RESCUABLE);
}

static void
test_depot_transfers_only_same_slot_live_sprite_group(void)
{
    int hall, same, other, dead, same_building, other_building;
    clear_scene();
    put_unit(SIDE, U_SCV, 0, 132, 100);
    hall = put_building(RESCUABLE, B_HATCHERY, 1, 100, 100, 0);
    same = put_unit(RESCUABLE, U_CIVILIAN, 1, 700, 700);
    other = put_unit(RESCUABLE, U_CIVILIAN, 2, 720, 700);
    dead = put_unit(RESCUABLE, U_CIVILIAN, 1, 740, 700);
    same_building = put_building(RESCUABLE, B_BARRACKS, 1, 760, 700, 0);
    other_building = put_building(RESCUABLE, B_BARRACKS, 2, 860, 700, 0);
    u[dead].alive = 0;
    race_t4_world_tick();
    assert(b[hall].owner == SIDE && b[hall].raw_owner == 1);
    assert(u[same].owner == SIDE && u[same].raw_owner == 1);
    assert(u[other].owner == RESCUABLE && u[other].raw_owner == 2);
    assert(!u[dead].alive && u[dead].owner == RESCUABLE);
    assert(b[same_building].owner == SIDE && b[same_building].raw_owner == 1);
    assert(b[other_building].owner == RESCUABLE && b[other_building].raw_owner == 2);

    clear_scene();
    put_unit(SIDE, U_SCV, 0, 132, 100);
    hall = put_building(RESCUABLE, B_HATCHERY, 1, 100, 100, 1);
    same = put_unit(RESCUABLE, U_CIVILIAN, 1, 700, 700);
    race_t4_world_tick();
    assert(b[hall].owner == SIDE); /* trigger actor still rescues individually */
    assert(u[same].owner == RESCUABLE); /* incomplete depot does not bulk-transfer */
}

int
main(void)
{
    test_source_start_has_no_automatic_rescue();
    test_individual_rescue_edges_and_supply();
    test_rescue_does_not_advance_simulation_state();
    test_rescued_actor_chain_and_hidden_revealer();
    test_depot_transfers_only_same_slot_live_sprite_group();
    puts("T4 rescue adapter fixtures pass (synthetic scenes; not a controller-win proof)");
    return 0;
}
