/* Focused adapter fixtures for the T4 trigger effect subset. The scenarios
 * below are synthetic world states; they are not gameplay or fidelity proofs. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"
#include "../starcraft/sc64_t4_runtime.h"
#include "sc64_t4_scenario_generated.h"
static Sc64T4RuntimeState race_t4;

static unsigned create_calls, doodad_calls;
static int create_result = 1, doodad_result = 1;
static const Sc64T4Action *last_create, *last_doodad;
static unsigned last_owner;

static int race_t4_create_action(const Sc64T4Action *a, unsigned owner)
{
    ++create_calls; last_create = a; last_owner = owner; return create_result;
}
static int race_t4_doodad_action(const Sc64T4Action *a, unsigned owner)
{
    ++doodad_calls; last_doodad = a; last_owner = owner; return doodad_result;
}

#include "../starcraft/sc64_t4_effects_adapter.h"

static void reset_world(void)
{
    game_init();
    memset(u, 0, sizeof(u)); memset(b, 0, sizeof(b));
    memset(invincible_units, 0, sizeof(invincible_units));
    memset(invincible_buildings, 0, sizeof(invincible_buildings));
    memset(&race_t4, 0, sizeof(race_t4));
    race_mode = 0; race_selected_mission = SC64_T4_GLOBAL_ID;
    race_campaign.mission = SC64_T4_GLOBAL_ID;
    race_campaign.phase = SC64_RACE_PHASE_ACTIVE;
    race_cols = race_rows = 128;
    race_player[SIDE] = R_TER; race_player[ENEMY] = R_ZERG;
    race_player[4] = R_ZERG; race_player[RESCUABLE] = R_TER;
    supply_used = supply_cap = 0;
    selected_building = -1; target_lists_valid = 0;
    sc64_target_index_end();
    memset(path_walkable, 0xff, sizeof(path_walkable));
    path_grid_valid = 0;
    create_calls = doodad_calls = 0; create_result = doodad_result = 1;
    last_create = last_doodad = 0; last_owner = 0;
}

static int add_unit(int owner, int raw, int sx, int sy)
{
    int type = sc64_race_unit_from_raw((unsigned)raw);
    int slot;
    assert(type >= 0);
    slot = spawn_unit(owner, type, sx / 2, sy / 2);
    assert(slot >= 0);
    u[slot].raw_type = raw;
    u[slot].raw_owner = owner == SIDE ? 0 : owner == ENEMY ? 3 : owner;
    return slot;
}

static int add_building(int owner, int raw, int sx, int sy)
{
    int type = sc64_race_building_from_raw((unsigned)raw);
    int slot;
    assert(type >= 0);
    slot = spawn_build(owner, type, sx / 2, sy / 2, 0);
    assert(slot >= 0);
    b[slot].raw_type = raw;
    b[slot].raw_owner = owner == SIDE ? 0 : owner == ENEMY ? 3 : owner;
    return slot;
}

static void test_queries(void)
{
    Sc64T4Condition c = {0};
    u32 count;
    const Sc64T4Location *loc;
    int slot;
    reset_world();
    loc = &sc64_t4_scenario.locations[0];
    slot = add_unit(SIDE, 0, (int)((loc->left + loc->right) / 2),
                    (int)((loc->top + loc->bottom) / 2));
    add_building(ENEMY, 106, (int)((loc->left + loc->right) / 2),
                 (int)((loc->top + loc->bottom) / 2));
    c.opcode = 2; c.location = 0; c.player = 17; c.unit = 229;
    count = 99;
    assert(race_t4_trigger_count(&race_t4, &c, 0, &count) == 1 && count == 2);
    c.location = 250; count = 98;
    assert(race_t4_trigger_count(&race_t4, &c, 0, &count) == 1 && count == 2);
    c.location = 1;
    c.unit = 230;
    assert(race_t4_trigger_count(&race_t4, &c, 0, &count) == 1 && count == 1);
    c.opcode = 3; c.unit = 229; c.player = 17; count = 42;
    assert(race_t4_trigger_count(&race_t4, &c, 0, &count) == 0 && count == 42);
    c.opcode = 3; c.unit = 0; c.player = 0;
    assert(race_t4_trigger_count(&race_t4, &c, 0, &count) == 1 && count == 1);
    c.location = 0; count = 13;
    assert(race_t4_trigger_count(&race_t4, &c, 0, &count) == 0 && count == 13);
    c.location = 250; count = 77;
    assert(race_t4_trigger_count(&race_t4, &c, 0, &count) == 0 && count == 77);
    c.location = 1; c.unit = 222; count = 55;
    assert(race_t4_trigger_count(&race_t4, &c, 0, &count) == 0 && count == 55);
    assert(u[slot].alive);
    reset_world();
    c.opcode = 2; c.player = 14; c.unit = 229; count = 31;
    assert(race_t4_trigger_count(&race_t4, &c, 0, &count) == 0 && count == 31);
    assert(race_t4_effect_location(&race_t4, 1, &(Sc64T4QueryRect){0}));
    assert(!race_t4_effect_location(&race_t4, 0, &(Sc64T4QueryRect){0}));
    assert(!race_t4_effect_location(&race_t4, 255, &(Sc64T4QueryRect){0}));
    assert(!race_t4_effect_location(&race_t4, 256, &(Sc64T4QueryRect){0}));
    Sc64T4QueryRect anywhere;
    assert(race_t4_effect_location(&race_t4, 64, &anywhere));
    assert(anywhere.left == 0 && anywhere.top == 0 &&
           anywhere.right == race_cols * 32 && anywhere.bottom == race_rows * 32);
}

static void test_presentation_camera_and_wrappers(void)
{
    Sc64T4Action a = {0};
    reset_world();
    a.opcode = 7;
    a.player = 0xffffffffu; /* For these opcodes this is not an owner selector. */
    assert(race_t4_trigger_action(&race_t4, &a, 0) == 1);
    assert(race_t4.message == 7 && race_t4.message_ticks == 90);
    assert(race_t4_trigger_action(&race_t4, &a, 5) == 1);
    assert(race_t4.message_ticks == 90); /* Shared human UI, not AI context. */
    a.opcode = 28; a.location = 1;
    const Sc64T4Location *loc = &sc64_t4_scenario.locations[0];
    assert(race_t4_trigger_action(&race_t4, &a, 0) == 1);
    assert(race_t4.ping_ticks == 90);
    assert(race_t4.ping_x == (loc->left + loc->right) / 4);
    assert(race_t4.ping_y == (loc->top + loc->bottom) / 4);
    a.opcode = 10;
    assert(race_t4_trigger_action(&race_t4, &a, 0) == 1);
    assert(cx == race_t4.ping_x && cy == race_t4.ping_y);
    a.opcode = 11; create_result = 0;
    assert(race_t4_trigger_action(&race_t4, &a, 3) == 0);
    assert(create_calls == 1 && last_create == &a && last_owner == 3);
    a.opcode = 42; doodad_result = -1;
    assert(race_t4_trigger_action(&race_t4, &a, 2) == -1);
    assert(doodad_calls == 1 && last_doodad == &a && last_owner == 2);
    a.flags = 2; a.opcode = 255; a.player = 0xffffffffu;
    assert(race_t4_trigger_action(&race_t4, &a, 0) == 1);
    a.flags = 0;
    assert(race_t4_trigger_action(&race_t4, &a, 0) == -1);
    a.opcode = 22; a.player = 14; a.unit = 229; a.location = 0;
    assert(race_t4_trigger_action(&race_t4, &a, 0) == -1);
}

static void test_remove_and_invincibility(void)
{
    Sc64T4Action a = {0};
    int keep, dead, building;
    reset_world();
    keep = add_unit(SIDE, 0, 200, 200);
    dead = add_unit(ENEMY, 0, 400, 400);
    building = add_building(ENEMY, 106, 500, 500);
    u[keep].selected = 1; u[keep].order = ORDER_ATTACK_TARGET; u[keep].target = dead;
    u[dead].order = ORDER_ATTACK_TARGET; u[dead].target = keep;
    supply_used = 123;
    a.opcode = 22; a.player = 17; a.unit = 229;
    assert(race_t4_trigger_action(&race_t4, &a, 0) == 1);
    assert(!u[keep].alive && !u[dead].alive && !b[building].alive);
    assert(!u[keep].selected && u[keep].target == -1 && u[dead].target == -1);
    assert(supply_used == race_owner_supply(SIDE));

    reset_world();
    keep = add_unit(4, 15, (int)((sc64_t4_scenario.locations[8].left +
                                      sc64_t4_scenario.locations[8].right) / 2),
                    (int)((sc64_t4_scenario.locations[8].top +
                           sc64_t4_scenario.locations[8].bottom) / 2));
    a = (Sc64T4Action){0}; a.opcode = 43; a.location = 9; a.player = 4;
    a.unit = 15; a.modifier = 4;
    assert(race_t4_trigger_action(&race_t4, &a, 0) == 1 && invincible_units[keep]);
    a.modifier = 5;
    assert(race_t4_trigger_action(&race_t4, &a, 0) == 1 && !invincible_units[keep]);
    a.modifier = 6;
    assert(race_t4_trigger_action(&race_t4, &a, 0) == -1 && !invincible_units[keep]);
}

static void test_target_namespaces_on_removal(void)
{
    Sc64T4Action a = {0};
    int revealer, miner, attacker, removed_building, kept_building;
    int builder, gas_worker, other_builder;
    reset_world();
    revealer = add_unit(SIDE, 101, 160, 160);
    miner = add_unit(SIDE, 7, 180, 160);
    attacker = add_unit(SIDE, 0, 200, 160);
    u[miner].order = ORDER_MINE; u[miner].target = 0; /* mineral slot 0 */
    u[attacker].order = ORDER_ATTACK_TARGET; u[attacker].target = revealer;
    a.opcode = 24; a.player = 0; a.unit = 101; a.location = 0;
    assert(race_t4_trigger_action(&race_t4, &a, 0) == 1);
    assert(!u[revealer].alive);
    assert(u[miner].order == ORDER_MINE && u[miner].target == 0);
    assert(u[attacker].order == ORDER_IDLE && u[attacker].target == -1);

    removed_building = add_building(4, 106, 400, 400);
    kept_building = add_building(NEUTRAL, 106, 600, 600);
    builder = add_unit(NEUTRAL, 7, 700, 700);
    gas_worker = add_unit(NEUTRAL, 7, 720, 700);
    other_builder = add_unit(NEUTRAL, 7, 740, 700);
    assert(sc64_t4_query_owner_matches(17, 0, b[kept_building].owner,
                                       b[kept_building].raw_owner) == 0);
    u[builder].order = ORDER_BUILD; u[builder].target = removed_building;
    u[gas_worker].order = ORDER_GAS; u[gas_worker].target = removed_building;
    u[other_builder].order = ORDER_BUILD; u[other_builder].target = kept_building;
    a = (Sc64T4Action){0}; a.opcode = 22; a.player = 17; a.unit = 229; a.location = 0;
    assert(race_t4_trigger_action(&race_t4, &a, 0) == 1);
    assert(!b[removed_building].alive && b[kept_building].alive);
    assert(u[builder].order == ORDER_IDLE && u[builder].target == -1);
    assert(u[gas_worker].order == ORDER_IDLE && u[gas_worker].target == -1);
    assert(u[other_builder].order == ORDER_BUILD &&
           u[other_builder].target == kept_building);
}

static void test_move_location(void)
{
    Sc64T4Action a = {0};
    const Sc64T4Location *from = &sc64_t4_scenario.locations[0];
    const Sc64T4Location *to = &sc64_t4_scenario.locations[29];
    int slot, width = (int)(to->right - to->left), height = (int)(to->bottom - to->top);
    reset_world();
    slot = add_unit(SIDE, 20, (int)((from->left + from->right) / 2),
                    (int)((from->top + from->bottom) / 2));
    a.opcode = 38; a.location = 64; a.argument = 30; a.player = 0; a.unit = 20;
    int move_result = race_t4_trigger_action(&race_t4, &a, 0);
    if (move_result != 1) fprintf(stderr, "move-location rejected loc30 %u,%u..%u,%u source center %d,%d raw=%d owner=%d\n", (unsigned)to->left,(unsigned)to->top,(unsigned)to->right,(unsigned)to->bottom,u[slot].x,u[slot].y,u[slot].raw_type,u[slot].owner);
    assert(move_result == 1);
    assert(race_t4.location30_moved);
    assert(race_t4.location30[2] - race_t4.location30[0] == width);
    assert(race_t4.location30[3] - race_t4.location30[1] == height);
    assert(race_t4.location30[0] <= (u16)(u[slot].x * 2));
    reset_world(); /* no matching actor is a no-op and leaves the dynamic state */
    race_t4.location30_moved = 1;
    race_t4.location30[0] = (u16)to->left; race_t4.location30[1] = (u16)to->top;
    race_t4.location30[2] = (u16)to->right; race_t4.location30[3] = (u16)to->bottom;
    assert(race_t4_trigger_action(&race_t4, &a, 0) == 1);
    assert(race_t4.location30[0] == to->left);
    a.unit = 21;
    assert(race_t4_trigger_action(&race_t4, &a, 0) == -1);
}

static void test_move_units_atomic(void)
{
    Sc64T4Action a = {0};
    const Sc64T4Location *from = &sc64_t4_scenario.locations[13];
    const Sc64T4Location *to = &sc64_t4_scenario.locations[14];
    int sx = (int)((from->left + from->right) / 2);
    int sy = (int)((from->top + from->bottom) / 2);
    int first, second, old_x, old_y, second_x, second_y;
    reset_world();
    path_grid_valid = 1; memset(path_walkable, 0xff, sizeof(path_walkable));
    first = add_unit(SIDE, 0, sx, sy);
    second = add_unit(SIDE, 0, sx + 4, sy + 4);
    a.opcode = 39; a.location = 14; a.argument = 15; a.player = 13;
    a.unit = 230; a.modifier = 0;
    assert(race_t4_trigger_action(&race_t4, &a, 0) == 1);
    assert(u[first].order == ORDER_IDLE && u[second].order == ORDER_IDLE);
    assert(u[first].target == -1 && u[second].target == -1);
    assert((u32)u[first].x * 2 >= to->left && (u32)u[first].x * 2 < to->right);
    assert((u32)u[second].x * 2 >= to->left && (u32)u[second].x * 2 < to->right);
    assert((u32)u[first].y * 2 >= to->top && (u32)u[first].y * 2 < to->bottom);
    assert((u32)u[second].y * 2 >= to->top && (u32)u[second].y * 2 < to->bottom);

    reset_world();
    first = add_unit(SIDE, 0, sx, sy);
    old_x = u[first].x; old_y = u[first].y;
    /* Invalid operands reject without moving or changing orders. */
    a.argument = 16;
    assert(race_t4_trigger_action(&race_t4, &a, 0) == -1);
    assert(u[first].x == old_x && u[first].y == old_y && u[first].order == ORDER_IDLE);
    a.argument = 15; a.player = 0;
    assert(race_t4_trigger_action(&race_t4, &a, 0) == -1);
    assert(u[first].x == old_x && u[first].y == old_y);

    reset_world();
    first = add_unit(SIDE, 0, sx, sy);
    second = add_unit(SIDE, 0, sx + 4, sy + 4);
    old_x = u[first].x; old_y = u[first].y;
    second_x = u[second].x; second_y = u[second].y;
    path_grid_valid = 1; SC64_GRID_CLEAR(path_walkable);
    int only_cell = (((int)((to->top + to->bottom) / 4) / 16) * MAP_COLS) +
                    ((int)((to->left + to->right) / 4) / 16);
    SC64_GRID_SET(path_walkable, only_cell, 1);
    int fit_x = (int)((to->left + to->right) / 4);
    int fit_y = (int)((to->top + to->bottom) / 4);
    u8 planned_slot[1] = {(u8)first};
    u16 planned_x[1] = {(u16)fit_x}, planned_y[1] = {(u16)fit_y};
    assert(race_t4_effect_fit(&u[first], fit_x, fit_y, planned_slot,
                              planned_x, planned_y, 0));
    assert(!race_t4_effect_fit(&u[second], fit_x, fit_y, planned_slot,
                               planned_x, planned_y, 1));
    a.player = 13;
    assert(race_t4_trigger_action(&race_t4, &a, 0) == 0);
    assert(u[first].x == old_x && u[first].y == old_y &&
           u[first].order == ORDER_IDLE && u[first].target == -1);
    assert(u[second].x == second_x && u[second].y == second_y &&
           u[second].order == ORDER_IDLE && u[second].target == -1);
}

int main(void)
{
    test_queries();
    test_presentation_camera_and_wrappers();
    test_remove_and_invincibility();
    test_target_namespaces_on_removal();
    test_move_location();
    test_move_units_atomic();
    puts("T4 trigger effect fixtures pass (synthetic policy tests, not gameplay proof)");
    return 0;
}
