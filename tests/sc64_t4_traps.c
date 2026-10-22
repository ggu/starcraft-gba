/* Focused engine fixture for the source-verified T4 trap weapon subset. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

static void
reset_world(void)
{
    game_init();
    memset(u, 0, sizeof(u));
    memset(b, 0, sizeof(b));
    memset(invincible_units, 0, sizeof(invincible_units));
    memset(invincible_buildings, 0, sizeof(invincible_buildings));
    memset(&race_t4, 0, sizeof(race_t4));
    memset(tracers, 0, sizeof(tracers));
    race_mode = 0;
    race_selected_mission = SC64_T4_GLOBAL_ID;
    race_campaign.mission = SC64_T4_GLOBAL_ID;
    race_cols = race_rows = 128;
    supply_used = 0;
    supply_cap = 0;
}

static int
put_trap(unsigned slot, unsigned row, int type, int x, int y)
{
    assert(slot < MAX_BUILDINGS && row < 32);
    b[slot].alive = 1;
    b[slot].type = type;
    b[slot].raw_type = sc64_race_raw_building(type);
    b[slot].owner = 4;
    b[slot].raw_owner = 6;
    b[slot].x = x;
    b[slot].y = y;
    b[slot].hp = b[slot].maxhp = sc64_race_building_max_hp(type);
    race_t4.doodad_slot[row] = (u8)slot;
    race_t4.doodad_enabled[row] = 1;
    return (int)slot;
}

static int
put_unit(int owner, int type, int raw, int x, int y)
{
    int slot = spawn_unit(owner, type, x, y);
    assert(slot >= 0);
    u[slot].raw_type = raw;
    u[slot].raw_owner = owner == SIDE ? 0 : 3;
    u[slot].armor = 0;
    u[slot].flags = 0;
    return slot;
}

static void
test_source_weapon_rows(void)
{
    Sc64T4TrapRule r;
    assert(race_t4_trap_rule(B_FLOOR_MISSILE_TRAP, &r));
    assert(r.raw_type == 203 && r.damage == 10 && r.factor == 2 &&
           r.damage_type == SC64_T4_TRAP_EXPLOSIVE && r.range == 80);
    assert(race_t4_trap_rule(B_FLOOR_GUN_TRAP, &r));
    assert(r.raw_type == 209 && r.damage == 10 && r.factor == 1 &&
           r.damage_type == SC64_T4_TRAP_NORMAL && r.range == 80);
    assert(race_t4_trap_rule(B_WALL_FLAME_TRAP, &r));
    assert(r.raw_type == 211 && r.damage == 8 && r.factor == 1 &&
           r.damage_type == SC64_T4_TRAP_CONCUSSIVE && r.range == 32);
    assert(race_t4_trap_rule(B_RIGHT_WALL_MISSILE_TRAP, &r));
    assert(r.raw_type == 212 && r.damage == 10 && r.factor == 2 &&
           r.damage_type == SC64_T4_TRAP_EXPLOSIVE && r.range == 80);
    assert(!race_t4_trap_rule(B_COMMAND, &r));
}

static void
test_damage_types_and_size(void)
{
    assert(race_t4_trap_hp_damage(0, 10, 0, SC64_T4_TRAP_EXPLOSIVE) == 5);
    assert(race_t4_trap_hp_damage(19, 10, 0, SC64_T4_TRAP_EXPLOSIVE) == 7);
    assert(race_t4_trap_hp_damage(3, 10, 0, SC64_T4_TRAP_EXPLOSIVE) == 10);
    assert(race_t4_trap_hp_damage(20, 10, 2, SC64_T4_TRAP_EXPLOSIVE) == 4);
    assert(race_t4_trap_hp_damage(19, 10, 2, SC64_T4_TRAP_EXPLOSIVE) == 6);
    assert(race_t4_trap_hp_damage(3, 10, 2, SC64_T4_TRAP_EXPLOSIVE) == 8);
    assert(race_t4_trap_hp_damage(0, 8, 0, SC64_T4_TRAP_CONCUSSIVE) == 8);
    assert(race_t4_trap_hp_damage(19, 8, 0, SC64_T4_TRAP_CONCUSSIVE) == 4);
    assert(race_t4_trap_hp_damage(3, 8, 0, SC64_T4_TRAP_CONCUSSIVE) == 2);
    assert(race_t4_trap_hp_damage(3, 10, 0, SC64_T4_TRAP_NORMAL) == 10);
    assert(race_t4_trap_hp_damage(3, 1, 100, SC64_T4_TRAP_EXPLOSIVE) == 1);
}

static void
test_each_trap_fires_and_uses_factor(void)
{
    static const struct { int type, raw, expected; } cases[] = {
        {B_FLOOR_MISSILE_TRAP, 203, 90},
        {B_FLOOR_GUN_TRAP, 209, 90},
        {B_WALL_FLAME_TRAP, 211, 92},
        {B_RIGHT_WALL_MISSILE_TRAP, 212, 90}
    };
    for (unsigned c = 0; c < sizeof(cases) / sizeof(cases[0]); ++c) {
        int trap, target;
        reset_world();
        trap = put_trap(8, 0, cases[c].type, 100, 100);
        target = put_unit(SIDE, U_MARINE, 20, 104, 100);
        u[target].hp = u[target].maxhp = 100;
        race_t4_traps_tick();
        assert(u[target].hp == cases[c].expected);
        assert(b[trap].queue_progress == SC64_T4_TRAP_COOLDOWN);
        assert(tracers[0].life == 4);
    }
}

static void
test_factor_hits_shields_and_armor_per_strike(void)
{
    int target;
    reset_world();
    put_trap(8, 0, B_FLOOR_MISSILE_TRAP, 100, 100);
    target = put_unit(SIDE, U_MARINE, 20, 100, 100);
    u[target].hp = 100;
    u[target].shield = 3;
    u[target].armor = 2;
    race_t4_traps_tick();
    /* Each factor hit applies armor, then the Small explosive multiplier. */
    assert(u[target].shield == 0 && u[target].hp == 94);
}

static void
test_target_order_range_and_eligibility(void)
{
    int low, high, flying, trap;
    reset_world();
    trap = put_trap(8, 0, B_FLOOR_GUN_TRAP, 100, 100);
    low = put_unit(SIDE, U_MARINE, 20, 90, 100);
    high = put_unit(SIDE, U_MARINE, 20, 110, 100);
    u[high].hp = u[low].hp = 100;
    race_t4_traps_tick();
    assert(u[low].hp == 90 && u[high].hp == 100);
    assert(b[trap].queue_progress == SC64_T4_TRAP_COOLDOWN);

    reset_world();
    put_trap(8, 0, B_FLOOR_GUN_TRAP, 100, 100);
    flying = put_unit(SIDE, U_WRAITH, 8, 100, 100);
    u[flying].hp = 100;
    race_t4_traps_tick();
    assert(u[flying].hp == 100);

    /* Neutral/rescuable actors are allied and outside the hostile target set. */
    reset_world();
    put_trap(8, 0, B_FLOOR_GUN_TRAP, 100, 100);
    int neutral = put_unit(NEUTRAL, U_MARINE, 20, 100, 100);
    int rescuable = put_unit(RESCUABLE, U_MARINE, 20, 100, 100);
    u[neutral].hp = u[rescuable].hp = 100;
    race_t4_traps_tick();
    assert(u[neutral].hp == 100 && u[rescuable].hp == 100);
    assert(b[8].queue_progress == 0);

    /* Force4 is hostile to the independent enemy force as well as SIDE. */
    int enemy = put_unit(ENEMY, U_MARINE, 20, 100, 100);
    u[enemy].hp = 100;
    race_t4_traps_tick();
    assert(u[enemy].hp == 90);

    reset_world();
    put_trap(8, 0, B_WALL_FLAME_TRAP, 100, 100);
    int far = put_unit(SIDE, U_MARINE, 20, 133, 100);
    u[far].hp = 100;
    race_t4_traps_tick();
    assert(u[far].hp == 100);

    reset_world();
    put_trap(8, 0, B_FLOOR_GUN_TRAP, 100, 100);
    b[8].raw_owner = 4;
    int mismapped = put_unit(SIDE, U_MARINE, 20, 100, 100);
    u[mismapped].hp = 100;
    race_t4_traps_tick();
    assert(u[mismapped].hp == 100 && b[8].queue_progress == 0);
}

static void
test_disabled_intrinsic_and_cooldown(void)
{
    int trap, target;
    reset_world();
    trap = put_trap(8, 0, B_FLOOR_GUN_TRAP, 100, 100);
    target = put_unit(SIDE, U_MARINE, 20, 100, 100);
    u[target].hp = 100;
    race_t4.doodad_enabled[0] = 0;
    race_t4_traps_tick();
    assert(u[target].hp == 100 && b[trap].queue_progress == 0);
    race_t4.doodad_enabled[0] = 1;
    invincible_units[target] = 1;
    race_t4_traps_tick();
    assert(u[target].hp == 100 && b[trap].queue_progress == 0);
    invincible_units[target] = 0;
    u[target].flags = 16;
    race_t4_traps_tick();
    assert(u[target].hp == 100 && b[trap].queue_progress == 0);
    u[target].flags = 0;
    race_t4_traps_tick();
    assert(u[target].hp == 90 && b[trap].queue_progress == 28);
    for (int i = 0; i < 27; ++i) race_t4_traps_tick();
    assert(u[target].hp == 90 && b[trap].queue_progress == 1);
    race_t4_traps_tick();
    assert(u[target].hp == 80 && b[trap].queue_progress == 28);
}

static void
test_building_targets_and_side_supply_death(void)
{
    int trap;
    reset_world();
    trap = put_trap(8, 0, B_FLOOR_GUN_TRAP, 100, 100);
    b[0].alive = 1; b[0].type = B_COMMAND; b[0].owner = SIDE;
    b[0].raw_type = 106; b[0].raw_owner = 0;
    b[0].x = 100; b[0].y = 100; b[0].hp = b[0].maxhp = 5;
    race_t4_traps_tick();
    assert(!b[0].alive && trap < MAX_BUILDINGS);

    reset_world();
    put_trap(8, 0, B_FLOOR_MISSILE_TRAP, 100, 100);
    int target = put_unit(SIDE, U_MARINE, 20, 100, 100);
    u[target].hp = 1;
    supply_used = unit_supply(U_MARINE);
    race_t4_traps_tick();
    assert(!u[target].alive && supply_used == 0);
}

static void
test_invalid_target_indices_are_ignored(void)
{
    int target;
    reset_world();
    target = put_unit(SIDE, U_MARINE, 20, 100, 100);
    u[target].hp = 100;
    race_t4_trap_apply_hit(-1, 10, SC64_T4_TRAP_NORMAL);
    race_t4_trap_apply_hit(MAX_UNITS + MAX_BUILDINGS, 10,
                           SC64_T4_TRAP_NORMAL);
    race_t4_trap_apply_hit(0x7fffffff, 10, SC64_T4_TRAP_NORMAL);
    assert(u[target].hp == 100 && u[target].alive);
}

int
main(void)
{
    test_source_weapon_rows();
    test_damage_types_and_size();
    test_each_trap_fires_and_uses_factor();
    test_factor_hits_shields_and_armor_per_strike();
    test_target_order_range_and_eligibility();
    test_disabled_intrinsic_and_cooldown();
    test_building_targets_and_side_supply_death();
    test_invalid_target_indices_are_ignored();
    puts("T4 trap engine fixtures passed (synthetic isolated-world tests)");
    return 0;
}
