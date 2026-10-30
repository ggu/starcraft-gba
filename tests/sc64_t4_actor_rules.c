/* Pure, asset-free verification of the T4 raw actor rules. */
#include <assert.h>
#include <string.h>
#include "../starcraft/race_rules.h"

#ifdef SC64_T4_RUNTIME
static void check_bounds(int type, int l, int u, int r, int d)
{
    int got_l=-1, got_u=-1, got_r=-1, got_d=-1;
    assert(sc64_race_t4_bounds_source(type, &got_l, &got_u, &got_r, &got_d));
    assert(got_l==l && got_u==u && got_r==r && got_d==d);
}
static void check_building_bounds(int type, int l, int u, int r, int d)
{
    int got_l=-1, got_u=-1, got_r=-1, got_d=-1;
    assert(sc64_race_t4_building_bounds_source(type, &got_l, &got_u, &got_r, &got_d));
    assert(got_l==l && got_u==u && got_r==r && got_d==d);
}
#endif

int main(void)
{
#ifndef SC64_T4_RUNTIME
    /* The T4 additions must not alter any default-profile compact IDs. */
    assert(SC_RU_COUNT == 28);
    assert(SC_RB_COUNT == 35);
    assert(sc64_race_unit_from_raw(1) == -1);
    assert(sc64_race_building_from_raw(195) == -1);
    assert(sc64_race_unit_damage(U_MARINE) == 6);
    assert(sc64_race_unit_can_attack_air(U_MARINE));
    assert(!sc64_race_unit_can_attack_air(U_VULTURE));
#else
    assert(SC_RU_COUNT == 35);
    assert(U_DATA_DISK == 33 && U_RAYNOR_MARINE == 34);
    assert(sc64_race_unit_from_raw(20) == U_RAYNOR_MARINE);
    assert(sc64_race_raw_unit(U_RAYNOR_MARINE) == 20);
    assert(sc64_race_unit_damage_vs(U_RAYNOR_MARINE, 0) == 18);
    assert(sc64_race_unit_can_attack_air(U_RAYNOR_MARINE));
    assert(sc64_race_unit_rule(U_RAYNOR_MARINE)->supply == 0);
    assert(sc64_race_unit_rule(U_RAYNOR_MARINE)->producer == 255);
    assert(SC_RB_COUNT == 44);
    assert(sc64_race_unit_from_raw(1) == U_GHOST);
    assert(sc64_race_unit_from_raw(15) == U_CIVILIAN);
    assert(sc64_race_unit_from_raw(89) == U_RHYNADON);
    assert(sc64_race_unit_from_raw(95) == U_RAGNASAUR);
    assert(sc64_race_unit_from_raw(101) == U_MAP_REVEALER);
    assert(sc64_race_unit_from_raw(218) == U_DATA_DISK);
    assert(sc64_race_raw_unit(U_GHOST) == 1);
    assert(sc64_race_raw_unit(U_CIVILIAN) == 15);
    assert(sc64_race_raw_unit(U_RHYNADON) == 89);
    assert(sc64_race_raw_unit(U_RAGNASAUR) == 95);
    assert(sc64_race_raw_unit(U_MAP_REVEALER) == 101);
    assert(sc64_race_raw_unit(U_DATA_DISK) == 218);
    assert(sc64_race_unit_from_raw(195) == -1);
    assert(sc64_race_building_from_raw(195) == B_TERRAN_BEACON);
    assert(sc64_race_raw_building(B_TERRAN_BEACON) == 195);
    static const int props[][2] = {
        {203,B_FLOOR_MISSILE_TRAP},{205,B_UPPER_DOOR},
        {206,B_RIGHT_UPPER_DOOR},{207,B_PIT_DOOR},
        {208,B_RIGHT_PIT_DOOR},{209,B_FLOOR_GUN_TRAP},
        {211,B_WALL_FLAME_TRAP},{212,B_RIGHT_WALL_MISSILE_TRAP}
    };
    for (unsigned i=0;i<sizeof(props)/sizeof(props[0]);++i) {
        assert(sc64_race_building_from_raw(props[i][0]) == props[i][1]);
        assert(sc64_race_raw_building(props[i][1]) == props[i][0]);
        assert(sc64_race_building_rule(props[i][1]) != 0);
    }
    assert(sc64_race_t4_is_trap(B_FLOOR_MISSILE_TRAP));
    assert(sc64_race_t4_is_trap(B_FLOOR_GUN_TRAP));
    assert(sc64_race_t4_is_trap(B_WALL_FLAME_TRAP));
    assert(sc64_race_t4_is_trap(B_RIGHT_WALL_MISSILE_TRAP));
    assert(sc64_race_t4_is_door(B_UPPER_DOOR));
    assert(sc64_race_t4_is_door(B_RIGHT_UPPER_DOOR));
    assert(sc64_race_t4_is_door(B_PIT_DOOR));
    assert(sc64_race_t4_is_door(B_RIGHT_PIT_DOOR));
    assert(!sc64_race_t4_is_door(B_TERRAN_BEACON));
    assert(!sc64_race_t4_is_trap(B_TERRAN_BEACON));
    assert(sc64_race_building_max_hp(B_FLOOR_MISSILE_TRAP)==50);
    assert(sc64_race_building_max_hp(B_FLOOR_GUN_TRAP)==50);
    assert(sc64_race_building_max_hp(B_WALL_FLAME_TRAP)==50);
    assert(sc64_race_building_max_hp(B_RIGHT_WALL_MISSILE_TRAP)==50);
    assert(sc64_race_building_max_hp(B_UPPER_DOOR)==100000);
    assert(sc64_race_building_max_hp(B_RIGHT_UPPER_DOOR)==100000);
    assert(sc64_race_building_max_hp(B_PIT_DOOR)==100000);
    assert(sc64_race_building_max_hp(B_RIGHT_PIT_DOOR)==100000);
    assert(sc64_race_t4_building_intrinsically_invincible(B_UPPER_DOOR));
    assert(sc64_race_t4_building_intrinsically_invincible(B_RIGHT_UPPER_DOOR));
    assert(sc64_race_t4_building_intrinsically_invincible(B_PIT_DOOR));
    assert(sc64_race_t4_building_intrinsically_invincible(B_RIGHT_PIT_DOOR));
    assert(!sc64_race_t4_building_intrinsically_invincible(B_FLOOR_MISSILE_TRAP));
    assert(!sc64_race_t4_building_intrinsically_invincible(B_FLOOR_GUN_TRAP));
    assert(!sc64_race_t4_building_intrinsically_invincible(B_WALL_FLAME_TRAP));
    assert(!sc64_race_t4_building_intrinsically_invincible(B_RIGHT_WALL_MISSILE_TRAP));
    assert(strcmp(sc64_race_unit_name(U_GHOST), "GHOST") == 0);
    assert(strcmp(sc64_race_unit_name(U_DATA_DISK), "DATA DISK") == 0);
    assert(strcmp(sc64_race_building_name(B_TERRAN_BEACON), "TERRAN BEACON") == 0);

    /* BWAPI defaults: Ghost HP45, armor0, doubled supply2; the weapon is a
       10-damage C-10 shot at224 source px, cooldown22, +1 per infantry level. */
    assert(sc64_race_unit_rule(U_GHOST)->hp == 45);
    assert(sc64_race_unit_rule(U_GHOST)->supply == 2);
    assert(sc64_race_unit_armor(U_GHOST) == 0);
    assert(sc64_race_unit_damage_vs(U_GHOST, 0) == 10);
    assert(sc64_race_unit_damage_vs(U_GHOST, 1) == 10);
    assert(sc64_race_unit_range_vs(U_GHOST, 0) == 224);
    assert(sc64_race_unit_range_vs(U_GHOST, 1) == 224);
    assert(sc64_race_unit_cooldown_vs(U_GHOST, 0) == 22);
    assert(sc64_race_unit_cooldown_vs(U_GHOST, 1) == 22);
    assert(sc64_race_t4_weapon_damage_type(U_GHOST) == SC64_RACE_DAMAGE_CONCUSSIVE);
    assert(sc64_race_t4_weapon_upgrade_bonus(U_GHOST) == 1);
    /* Source UnitSizeTypes for the T4 roster (pinned BWAPI UnitType.cpp):
       Marine/Ghost/SCV/Civilian/Raynor Marine/Firebat/Zergling and both
       critters are Small; Vulture Medium; Goliath Large; Beacon Independent. */
    assert(sc64_race_t4_source_size(0) == SC64_T4_SIZE_SMALL);
    assert(sc64_race_t4_source_size(1) == SC64_T4_SIZE_SMALL);
    assert(sc64_race_t4_source_size(2) == SC64_T4_SIZE_MEDIUM);
    assert(sc64_race_t4_source_size(3) == SC64_T4_SIZE_LARGE);
    assert(sc64_race_t4_source_size(8) == SC64_T4_SIZE_LARGE); /* Wraith */
    assert(sc64_race_t4_source_size(7) == SC64_T4_SIZE_SMALL);
    assert(sc64_race_t4_source_size(15) == SC64_T4_SIZE_SMALL);
    assert(sc64_race_t4_source_size(20) == SC64_T4_SIZE_SMALL);
    assert(sc64_race_t4_source_size(32) == SC64_T4_SIZE_SMALL);
    assert(sc64_race_t4_source_size(37) == SC64_T4_SIZE_SMALL);
    assert(sc64_race_t4_source_size(89) == SC64_T4_SIZE_SMALL);
    assert(sc64_race_t4_source_size(95) == SC64_T4_SIZE_SMALL);
    assert(sc64_race_t4_source_size(195) == SC64_T4_SIZE_INDEPENDENT);
    assert(sc64_race_t4_source_size(101) == SC64_T4_SIZE_INDEPENDENT);
    assert(sc64_race_t4_source_size(216) == SC64_T4_SIZE_INDEPENDENT);
    assert(sc64_race_t4_source_size(218) == SC64_T4_SIZE_INDEPENDENT);
    /* Canonical building raws currently exposed by sc64_race_raw_building:
       BWAPI classifies every one Large except the Zerg/Terran Beacons. */
    static const int large_building_raws[] = {
        106,111,109,110,112,125,130,131,132,135,139,141,142,143,144,146,
        149,151,154,155,150,133,137,156,157,160,162,164,166,167,113,114,124
    };
    for (unsigned i = 0; i < sizeof(large_building_raws)/sizeof(large_building_raws[0]); ++i)
        assert(sc64_race_t4_source_size(large_building_raws[i]) == SC64_T4_SIZE_LARGE);
    assert(sc64_race_t4_source_size(194) == SC64_T4_SIZE_INDEPENDENT);
    assert(sc64_race_t4_source_size(999) == SC64_T4_SIZE_UNKNOWN);
    /* The helper receives HP-bound damage after shields have already absorbed
       their share. Small is full, Medium halves, Large quarters; it adds the
       Ghost's +1 per level before target armor and uses bounded integer HP. */
    assert(sc64_race_t4_ghost_hp_damage(0, 10, 0) == 10);
    assert(sc64_race_t4_ghost_hp_damage(2, 10, 0) == 5);
    assert(sc64_race_t4_ghost_hp_damage(3, 10, 0) == 2);
    assert(sc64_race_t4_ghost_hp_damage(195, 10, 0) == 10);
    assert(sc64_race_t4_ghost_hp_damage(0, 10, 1) == 9);
    assert(sc64_race_t4_ghost_hp_damage(2, 10, 1) == 4);
    assert(sc64_race_t4_ghost_hp_damage(3, 10, 1) == 2);
    assert(sc64_race_t4_ghost_hp_damage(3, 10, 9) == 1);
    assert(sc64_race_t4_ghost_hp_damage(3, 11, 0) == 2);
    assert(sc64_race_t4_ghost_hp_damage(8, 10, 0) == 2);
    assert(sc64_race_t4_ghost_hp_damage(106, 10, 0) == 2); /* Command Center */
    assert(sc64_race_t4_ghost_hp_damage(2, 13, 0) == 6);
    assert(sc64_race_t4_ghost_hp_damage(0, 13, 0) == 13);
    assert(sc64_race_unit_can_attack_air(U_GHOST));
    assert(sc64_race_unit_can_attack_target_class(U_GHOST, 0));
    assert(sc64_race_unit_can_attack_target_class(U_GHOST, 1));

    /* These actors stay distinct and unarmed; neither unit-name nor generic
       combat rules may turn the props/critter actors into Marines. */
    assert(sc64_race_unit_rule(U_CIVILIAN)->hp == 40);
    assert(sc64_race_unit_rule(U_RHYNADON)->hp == 60);
    assert(sc64_race_unit_rule(U_RAGNASAUR)->hp == 60);
    assert(sc64_race_unit_rule(U_MAP_REVEALER)->hp == 1);
    assert(sc64_race_unit_rule(U_DATA_DISK)->hp == 800);
    assert(sc64_race_unit_damage(U_CIVILIAN) == 0);
    assert(sc64_race_unit_damage(U_RHYNADON) == 0);
    assert(sc64_race_unit_damage(U_RAGNASAUR) == 0);
    assert(sc64_race_unit_damage(U_MAP_REVEALER) == 0);
    assert(sc64_race_unit_damage(U_DATA_DISK) == 0);
    assert(!sc64_race_unit_can_attack_air(U_CIVILIAN));
    assert(!sc64_race_unit_can_attack_target_class(U_CIVILIAN, 0));
    assert(!sc64_race_unit_can_attack_target_class(U_CIVILIAN, 1));
    assert(sc64_race_unit_flying(U_MAP_REVEALER));

    unsigned ghost = sc64_race_t4_actor_flags(U_GHOST);
    assert((ghost & (SC64_T4_ACTOR_SELECTABLE | SC64_T4_ACTOR_MOVABLE |
                     SC64_T4_ACTOR_COLLIDABLE | SC64_T4_ACTOR_DRAWABLE |
                     SC64_T4_ACTOR_MEN | SC64_T4_ACTOR_WEAPON)) ==
           (SC64_T4_ACTOR_SELECTABLE | SC64_T4_ACTOR_MOVABLE |
            SC64_T4_ACTOR_COLLIDABLE | SC64_T4_ACTOR_DRAWABLE |
            SC64_T4_ACTOR_MEN | SC64_T4_ACTOR_WEAPON));
    unsigned civilian = sc64_race_t4_actor_flags(U_CIVILIAN);
    assert((civilian & SC64_T4_ACTOR_MEN) && !(civilian & SC64_T4_ACTOR_WEAPON));
    unsigned critter = sc64_race_t4_actor_flags(U_RHYNADON);
    assert((critter & (SC64_T4_ACTOR_MOVABLE | SC64_T4_ACTOR_COLLIDABLE |
                       SC64_T4_ACTOR_DRAWABLE | SC64_T4_ACTOR_MEN)) ==
           (SC64_T4_ACTOR_MOVABLE | SC64_T4_ACTOR_COLLIDABLE |
            SC64_T4_ACTOR_DRAWABLE | SC64_T4_ACTOR_MEN));
    unsigned revealer = sc64_race_t4_actor_flags(U_MAP_REVEALER);
    assert((revealer & SC64_T4_ACTOR_HIDDEN) &&
           !(revealer & (SC64_T4_ACTOR_COLLIDABLE | SC64_T4_ACTOR_DRAWABLE |
                         SC64_T4_ACTOR_MEN | SC64_T4_ACTOR_WEAPON)));
    unsigned disk = sc64_race_t4_actor_flags(U_DATA_DISK);
    assert((disk & (SC64_T4_ACTOR_DRAWABLE | SC64_T4_ACTOR_INVINCIBLE)) ==
           (SC64_T4_ACTOR_DRAWABLE | SC64_T4_ACTOR_INVINCIBLE));
    assert(!(disk & (SC64_T4_ACTOR_SELECTABLE | SC64_T4_ACTOR_MOVABLE |
                     SC64_T4_ACTOR_COLLIDABLE | SC64_T4_ACTOR_MEN |
                     SC64_T4_ACTOR_WEAPON)));

    assert(sc64_race_t4_unit_intrinsically_invincible(U_DATA_DISK));
    assert(!sc64_race_t4_unit_intrinsically_invincible(U_MAP_REVEALER));
    unsigned beacon = sc64_race_t4_building_flags(B_TERRAN_BEACON);
    assert((beacon & (SC64_T4_ACTOR_DRAWABLE | SC64_T4_ACTOR_BUILDING |
                      SC64_T4_ACTOR_INVINCIBLE)) ==
           (SC64_T4_ACTOR_DRAWABLE | SC64_T4_ACTOR_BUILDING |
            SC64_T4_ACTOR_INVINCIBLE));
    assert(!(beacon & SC64_T4_ACTOR_COLLIDABLE)); /* root's non-solid policy */
    assert(sc64_race_t4_building_intrinsically_invincible(B_TERRAN_BEACON));
    assert(sc64_race_building_rule(B_TERRAN_BEACON)->hp == 0);
    assert(sc64_race_building_max_hp(B_TERRAN_BEACON) == 100000);
    assert(sc64_race_building_max_hp(-1) == 0);
    static const int scenario_only_buildings[] = {
        B_TERRAN_BEACON, B_FLOOR_MISSILE_TRAP, B_UPPER_DOOR,
        B_RIGHT_UPPER_DOOR, B_PIT_DOOR, B_RIGHT_PIT_DOOR,
        B_FLOOR_GUN_TRAP, B_WALL_FLAME_TRAP, B_RIGHT_WALL_MISSILE_TRAP
    };
    for (unsigned i=0;i<sizeof(scenario_only_buildings)/sizeof(scenario_only_buildings[0]);++i) {
        const Sc64RaceBuildingRule *rule =
            sc64_race_building_rule(scenario_only_buildings[i]);
        assert(rule && rule->worker == 255);
        for (int worker=0;worker<SC_RU_COUNT;++worker)
            assert(rule->worker != worker);
    }

    check_bounds(U_GHOST, 7, 10, 7, 11);
    check_bounds(U_CIVILIAN, 8, 9, 8, 10);
    check_bounds(U_RHYNADON, 16, 16, 15, 15);
    check_bounds(U_RAGNASAUR, 16, 16, 15, 15);
    check_bounds(U_MAP_REVEALER, 13, 13, 13, 17);
    check_bounds(U_DATA_DISK, 16, 16, 15, 15);
    int l=-1,u=-1,r=-1,d=-1;
    assert(sc64_race_t4_building_bounds_source(B_TERRAN_BEACON,&l,&u,&r,&d));
    assert(l==48 && u==32 && r==47 && d==31);
    assert(sc64_race_t4_sight_source_pixels(U_GHOST) == 288);
    assert(sc64_race_t4_sight_source_pixels(U_CIVILIAN) == 224);
    assert(sc64_race_t4_sight_source_pixels(U_RHYNADON) == 224);
    assert(sc64_race_t4_sight_source_pixels(U_RAGNASAUR) == 224);
    assert(sc64_race_t4_sight_source_pixels(U_MAP_REVEALER) == 320);
    assert(sc64_race_t4_sight_source_pixels(U_DATA_DISK) == 160);
    assert(sc64_race_t4_building_sight_source_pixels(B_TERRAN_BEACON) == 256);
    assert(sc64_race_t4_building_sight_source_pixels(B_UPPER_DOOR) == 32);
    assert(sc64_race_t4_building_sight_source_pixels(B_RIGHT_UPPER_DOOR) == 32);
    assert(sc64_race_t4_building_sight_source_pixels(B_PIT_DOOR) == 32);
    assert(sc64_race_t4_building_sight_source_pixels(B_RIGHT_PIT_DOOR) == 32);
    assert(sc64_race_t4_building_sight_source_pixels(B_FLOOR_MISSILE_TRAP) == 192);
    assert(sc64_race_t4_building_sight_source_pixels(B_FLOOR_GUN_TRAP) == 192);
    assert(sc64_race_t4_building_sight_source_pixels(B_WALL_FLAME_TRAP) == 96);
    assert(sc64_race_t4_building_sight_source_pixels(B_RIGHT_WALL_MISSILE_TRAP) == 192);
    check_building_bounds(B_UPPER_DOOR,25,17,44,20);
    check_building_bounds(B_RIGHT_UPPER_DOOR,44,17,25,20);
    check_building_bounds(B_PIT_DOOR,41,17,28,20);
    check_building_bounds(B_RIGHT_PIT_DOOR,28,17,41,20);
    check_building_bounds(B_FLOOR_MISSILE_TRAP,32,32,31,31);
    check_building_bounds(B_FLOOR_GUN_TRAP,32,32,31,31);
    check_building_bounds(B_WALL_FLAME_TRAP,16,16,15,15);
    check_building_bounds(B_RIGHT_WALL_MISSILE_TRAP,16,16,15,15);
    /* Generic actors use pinned BWAPI sightRangeTiles * 32. */
    assert(sc64_race_t4_building_sight_source_pixels(B_COMMAND) == 10 * 32);
    assert(sc64_race_t4_building_sight_source_pixels(B_MISSILE_TURRET) == 11 * 32);
    assert(sc64_race_t4_building_sight_source_pixels(B_BARRACKS) == 8 * 32);
    assert(sc64_race_t4_building_sight_source_pixels(B_BUNKER) == 10 * 32);
    assert(sc64_race_t4_sight_source_pixels(U_SCV) == 7 * 32);
    assert(sc64_race_t4_sight_source_pixels(U_MARINE) == 7 * 32);
    assert(sc64_race_t4_sight_source_pixels(U_VULTURE) == 8 * 32);
    assert(sc64_race_t4_sight_source_pixels(U_GOLIATH) == 8 * 32);
    assert(sc64_race_t4_sight_source_pixels(U_WRAITH) == 7 * 32);
    assert(sc64_race_t4_sight_source_pixels(U_TANK) == 10 * 32);
    assert(sc64_race_t4_sight_source_pixels(U_ZERGLING) == 5 * 32);
    assert(sc64_race_t4_sight_source_pixels(U_PROBE) == 8 * 32);
#ifdef SC64_T5_RUNTIME
    assert(sc64_race_t4_sight_source_pixels(U_KERRIGAN) == 11 * 32);
    assert(sc64_race_t4_sight_source_pixels(U_SPIDER_MINE) == 3 * 32);
#endif
    /* Each source sight covers the matching weapon's maximum range. */
    {
        static const int armed[] = { U_MARINE, U_VULTURE, U_GOLIATH, U_WRAITH,
                                     U_TANK, U_FIREBAT, U_GHOST };
        for (unsigned i = 0; i < sizeof(armed) / sizeof(armed[0]); ++i) {
            int sight = sc64_race_t4_sight_source_pixels(armed[i]);
            assert(sight >= sc64_race_unit_range_vs(armed[i], 0));
            assert(sight >= sc64_race_unit_range_vs(armed[i], 1));
        }
    }
#endif
    return 0;
}
