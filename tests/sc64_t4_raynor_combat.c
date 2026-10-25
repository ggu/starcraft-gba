/* T4 raw-20 Raynor combat identity and live target-acquisition regression.
 * This is an engine fixture, not a campaign-completion proof. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

static void reset_fixture(void)
{
    init_state();
    memset(u, 0, sizeof(u));
    memset(b, 0, sizeof(b));
    memset(invincible_units, 0, sizeof(invincible_units));
    memset(invincible_buildings, 0, sizeof(invincible_buildings));
    memset(path_walkable, 0xff, sizeof(path_walkable));
    path_grid_valid = 1;
    target_lists_valid = 0;
    sc64_target_index_active = 0;
    race_mode = 0;
    race_selected_mission = SC64_T4_GLOBAL_ID;
    weapons_level = armor_level = 0;
    supply_used = 0;
}

static int add_unit(int owner, int raw, int x, int y)
{
    int type = sc64_race_unit_from_raw(raw);
    int slot;
    assert(type >= 0);
    slot = spawn_unit(owner, type, x, y);
    assert(slot >= 0);
    u[slot].raw_type = raw;
    u[slot].raw_owner = owner == ENEMY ? 1 : 0;
    return slot;
}

static void test_raw_identity_and_rules(void)
{
    reset_fixture();
    int vulture = sc64_race_unit_from_raw(19);
    int marine = sc64_race_unit_from_raw(20);
    assert(vulture == U_RAYNOR);
    assert(marine == U_RAYNOR_MARINE && marine != vulture);
    assert(sc64_race_raw_unit(vulture) == 19);
    assert(sc64_race_raw_unit(marine) == 20);
    assert(sc64_race_unit_rule(vulture)->hp == 200);
    assert(sc64_race_unit_rule(marine)->hp == 200);
    assert(sc64_race_unit_rule(marine)->supply == 0);
    assert(sc64_race_unit_rule(marine)->producer == 255);
    minerals = 1000;
    int barracks = spawn_build(SIDE, B_BARRACKS, 80, 80, 0);
    assert(barracks >= 0);
    assert(!race_queue_unit(SIDE, barracks, marine)); /* Scenario hero is not trainable. */
    assert(sc64_race_unit_armor(marine) == 3);
    assert(sc64_race_unit_damage_vs(marine, 0) == 18);
    assert(sc64_race_unit_damage_vs(marine, 1) == 18);
    assert(sc64_race_unit_range_vs(marine, 0) == 160);
    assert(sc64_race_unit_range_vs(marine, 1) == 160);
    assert(sc64_race_unit_cooldown_vs(marine, 0) == 15);
    assert(sc64_race_unit_cooldown_vs(marine, 1) == 15);
    assert(sc64_race_unit_can_attack_air(marine));
    assert(sc64_game_unit_selectable(marine) && sc64_game_unit_collidable(marine));
    assert(sc64_race_unit_supply_provided(marine) == 0);
    assert(sc64_race_unit_name(marine) &&
           strcmp(sc64_race_unit_name(marine), "RAYNOR MARINE") == 0);
    assert(strcmp(unit_name(marine), "RAYNOR MARINE") == 0);
    assert(mission_unit_sprite(marine) == sc64_race_unit_sprite(20));
    reset_fixture();
    int hero = add_unit(SIDE, 20, 40, 40);
    assert(u[hero].maxhp == 200 && u[hero].hp == 200 && u[hero].armor == 3);
    /* Preserve the old raw-19 actor's T4 behavior: it remains the unarmed,
       faster Raynor Vulture alias rather than inheriting raw-20's weapon. */
    assert(sc64_race_unit_damage(vulture) == 0);
    assert(sc64_race_unit_range(vulture) == 0);
}

static void test_distinct_movement_classes(void)
{
    reset_fixture();
    int old_raynor = add_unit(SIDE, 19, 32, 32);
    int raw20 = add_unit(SIDE, 20, 32, 64);
    move_toward(&u[old_raynor], 80, 32);
    move_toward(&u[raw20], 80, 64);
    assert(u[old_raynor].x == 34); /* Existing Raynor/Vulture speed. */
    assert(u[raw20].x == 33);      /* Marine movement path, one pixel/tick. */
}

static void test_trap_and_normal_enemy_combat(void)
{
    reset_fixture();
    assert(!race_mode && race_selected_mission == SC64_T4_GLOBAL_ID);
    assert(owners_hostile(SIDE, 4));
    int attacker = add_unit(SIDE, 20, 100, 100);
    int trap = spawn_build(4, B_FLOOR_GUN_TRAP, 190, 100, 0);
    assert(trap >= 0);
    b[trap].raw_type = 209;
    b[trap].raw_owner = 6;
    b[trap].flags = 0;
    assert(!sc64_race_t4_building_intrinsically_invincible(B_FLOOR_GUN_TRAP));
    int selected = nearest_enemy_typed(u[attacker].x, u[attacker].y, SIDE,
                                       128, U_RAYNOR_MARINE);
    assert(selected == MAX_UNITS + trap);
    int initial_trap_hp = b[trap].hp;
    tick_combat(&u[attacker], selected);
    assert(b[trap].hp < initial_trap_hp);
    assert(u[attacker].cd == (15 * 5 + 3) / 4);

    reset_fixture();
    attacker = add_unit(SIDE, 20, 100, 100);
    int target = add_unit(ENEMY, 0, 150, 100);
    assert(owners_hostile(SIDE, ENEMY));
    selected = nearest_enemy_typed(u[attacker].x, u[attacker].y, SIDE,
                                   128, U_RAYNOR_MARINE);
    assert(selected == target);
    int initial_hp = u[target].hp;
    tick_combat(&u[attacker], selected);
    assert(u[target].hp == initial_hp - 18);

    reset_fixture();
    attacker = add_unit(SIDE, 20, 100, 100);
    target = add_unit(ENEMY, 0, 150, 100);
    weapons_level = 2;
    selected = nearest_enemy_typed(u[attacker].x, u[attacker].y, SIDE,
                                   128, U_RAYNOR_MARINE);
    tick_combat(&u[attacker], selected);
    assert(u[target].hp == u[target].maxhp - 20); /* +1 base per upgrade level. */

    reset_fixture();
    attacker = add_unit(SIDE, 20, 100, 100);
    target = add_unit(ENEMY, 8, 150, 100); /* Wraith: flying target. */
    assert(sc64_race_unit_flying(u[target].type));
    assert(SC64_CAN_ATTACK_FROM(U_RAYNOR_MARINE, target, SIDE));
    selected = nearest_enemy_typed(u[attacker].x, u[attacker].y, SIDE,
                                   128, U_RAYNOR_MARINE);
    assert(selected == target);
    initial_hp = u[target].hp;
    tick_combat(&u[attacker], selected);
    assert(u[target].hp == initial_hp - 18);
}

static void test_target_guards(void)
{
    reset_fixture();
    int attacker = add_unit(SIDE, 20, 100, 100);
    int friendly = add_unit(SIDE, 0, 110, 100);
    int enemy = add_unit(ENEMY, 0, 120, 100);
    int disk = add_unit(ENEMY, 218, 105, 100);
    assert(!race_can_attack(U_RAYNOR_MARINE, disk));
    assert(nearest_enemy_typed(u[attacker].x, u[attacker].y, SIDE,
                               128, U_RAYNOR_MARINE) == enemy);
    assert(friendly != enemy);
    assert(sc64_race_unit_flying(U_RAYNOR_MARINE) == 0);

    u[enemy].flags |= 16;
    assert(!race_can_attack(U_RAYNOR_MARINE, enemy));
    assert(nearest_enemy_typed(u[attacker].x, u[attacker].y, SIDE,
                               128, U_RAYNOR_MARINE) == -1);
    int beacon = spawn_build(ENEMY, B_TERRAN_BEACON, 120, 100, 0);
    assert(beacon >= 0);
    assert(!race_can_attack(U_RAYNOR_MARINE, MAX_UNITS + beacon));
}

int main(void)
{
    test_raw_identity_and_rules();
    test_distinct_movement_classes();
    test_trap_and_normal_enemy_combat();
    test_target_guards();
    puts("T4 Raynor Marine identity and combat checks passed");
    return 0;
}
