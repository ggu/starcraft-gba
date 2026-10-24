#include <assert.h>
#include "../starcraft/race_rules.h"

#ifdef SC64_Z5_ACTORS
static void assert_shared_fact(int raw, int type)
{
    const Sc64Z5ActorFact *src = sc64_z5_actor_fact(raw);
    const Sc64ZpActorFact *shared = sc64_zp_actor_fact(type);
    assert(src && shared && src->raw == shared->raw && src->raw == raw);
    assert(src->hp == shared->hp && src->shields == shared->shields);
    assert(src->armor == shared->armor && src->minerals == shared->minerals);
    assert(src->gas == shared->gas && src->build_frames == shared->build_frames);
    assert(src->sight_px == shared->sight_px);
    assert(src->left == shared->left && src->up == shared->up);
    assert(src->right == shared->right && src->down == shared->down);
    assert(src->flying == shared->flying && src->size == shared->size);
    assert(src->ground.damage == shared->ground_damage);
    assert(src->ground.bonus == shared->ground_bonus);
    assert(src->ground.cooldown == shared->ground_cooldown);
    assert(src->ground.range == shared->ground_range && src->ground.hits == shared->ground_hits);
    assert(src->air.damage == shared->air_damage);
    assert(src->air.bonus == shared->air_bonus);
    assert(src->air.cooldown == shared->air_cooldown);
    assert(src->air.range == shared->air_range && src->air.hits == shared->air_hits);
    assert(sc64_race_unit_from_raw(raw) == type && sc64_race_raw_unit(type) == raw);
}
#endif

/* Independently transcribed checks against the pinned source tables:
 * UnitType.cpp entries at raw IDs 1/15/89/90/95/195/218; weapon table
 * C_10_Canister_Rifle; and the named BWAPI predicates for Beacon/Powerup.
 * These assertions intentionally check facts and category boundaries rather
 * than deriving expected answers from the table under test. */
int main(void)
{
#ifndef SC64_Z5_ACTORS
    _Static_assert(U_ZP_INFESTED_KERRIGAN == 28 && SC_RU_COUNT == 33,
                   "ZP-only unit enum IDs changed without the Z5 gate");
    _Static_assert(B_ZP_CONTROL_TOWER == 35 && SC_RB_COUNT == 41,
                   "ZP-only building enum IDs changed without the Z5 gate");
    assert(sc64_race_unit_from_raw(1) == -1);
    assert(sc64_race_unit_from_raw(90) == -1);
    assert(sc64_race_unit_from_raw(218) == -1);
    assert(sc64_race_building_from_raw(195) == -1);
    return 0;
#else
    const Sc64Z5ActorFact *ghost = sc64_z5_actor_fact(1);
    assert(ghost && ghost->raw == 1);
    assert(ghost->hp == 45 && ghost->shields == 0 && ghost->armor == 0);
    assert(ghost->sight_px == 9 * 32 && ghost->flying == 0 && ghost->size == 0);
    assert(ghost->left == 7 && ghost->up == 10 && ghost->right == 7 && ghost->down == 11);
    assert(ghost->ground.damage == 10 && ghost->ground.bonus == 1);
    assert(ghost->ground.cooldown == 22 && ghost->ground.range == 224 && ghost->ground.hits == 1);
    assert(ghost->air.damage == 10 && ghost->air.bonus == 1);
    assert(ghost->air.cooldown == 22 && ghost->air.range == 224 && ghost->air.hits == 1);
    assert((ghost->flags & (SC64_Z5_SOURCE_CLOAKABLE|SC64_Z5_SOURCE_ORGANIC|
                            SC64_Z5_SOURCE_SPELLCASTER|SC64_Z5_SOURCE_AUTO_ATTACK_MOVE)) ==
           (SC64_Z5_SOURCE_CLOAKABLE|SC64_Z5_SOURCE_ORGANIC|
            SC64_Z5_SOURCE_SPELLCASTER|SC64_Z5_SOURCE_AUTO_ATTACK_MOVE));
    assert(sc64_z5_actor_radius_px(ghost) == 6);
    assert(sc64_race_unit_from_raw(1) == U_Z5_GHOST);
    assert(sc64_race_raw_unit(U_Z5_GHOST) == 1);
    assert(sc64_race_unit_rule(U_Z5_GHOST)->hp == 45);
    assert(sc64_race_unit_damage(U_Z5_GHOST) == 10);
    assert(sc64_race_unit_range(U_Z5_GHOST) == 224);
    assert(sc64_race_unit_cooldown(U_Z5_GHOST) == 22);
#ifdef SC64_T4_RUNTIME
    assert(sc64_race_t4_weapon_damage_type(U_Z5_GHOST) ==
           SC64_RACE_DAMAGE_CONCUSSIVE);
    assert(sc64_race_t4_weapon_upgrade_bonus(U_Z5_GHOST) == 1);
#endif
    assert(sc64_race_unit_can_attack_air(U_Z5_GHOST));
    assert(sc64_race_unit_hits_vs(U_Z5_GHOST, 1) == 1);
    assert_shared_fact(1, U_Z5_GHOST);
    assert(sc64_zp_actor_sight_source_pixels(U_Z5_GHOST) == 288);
    assert(sc64_zp_actor_default_hp(U_Z5_GHOST) == 45);
    assert(sc64_zp_actor_default_shields(U_Z5_GHOST) == 0);
    assert(sc64_zp_actor_flags(U_Z5_GHOST) ==
           (SC64_ZP_ACTOR_SELECTABLE|SC64_ZP_ACTOR_MOVABLE|
            SC64_ZP_ACTOR_COLLIDABLE|SC64_ZP_ACTOR_DRAWABLE|
            SC64_ZP_ACTOR_MEN|SC64_ZP_ACTOR_WEAPON));
    { int l=0,u=0,r=0,d=0;
      assert(sc64_zp_actor_bounds_source(U_Z5_GHOST,&l,&u,&r,&d));
      assert(l==7 && u==10 && r==7 && d==11); }

    const Sc64Z5ActorFact *civilian = sc64_z5_actor_fact(15);
    assert(civilian && civilian->hp == 40 && civilian->sight_px == 7 * 32);
    assert(civilian->left == 8 && civilian->up == 9 && civilian->right == 8 && civilian->down == 10);
    assert(civilian->minerals == 0 && civilian->gas == 0);
    assert(civilian->ground.damage == 0 && civilian->air.damage == 0);
    /* The source Attack prototype bit does not imply a weapon: canAttack()
     * resolves to false because both weapon types are None. */
    assert(civilian->flags & SC64_Z5_SOURCE_ATTACK_FLAG);
    assert(!(civilian->flags & SC64_Z5_REF_MOVE_INTERACTION));
    assert(civilian->flags & SC64_Z5_REF_ATTACKABLE);
    assert(sc64_race_unit_from_raw(15) == U_Z5_CIVILIAN);
    assert(sc64_race_raw_unit(U_Z5_CIVILIAN) == 15);
    assert(sc64_race_unit_rule(U_Z5_CIVILIAN)->hp == 40);
    assert(sc64_race_unit_damage(U_Z5_CIVILIAN) == 0);
    assert(sc64_race_unit_can_attack_air(U_Z5_CIVILIAN) == 0);
    assert(sc64_zp_actor_default_hp(U_Z5_CIVILIAN) == 40);
    assert(sc64_zp_actor_default_shields(U_Z5_CIVILIAN) == 0);
    assert(sc64_zp_actor_flags(U_Z5_CIVILIAN) ==
           (SC64_ZP_ACTOR_SELECTABLE|SC64_ZP_ACTOR_MOVABLE|
            SC64_ZP_ACTOR_COLLIDABLE|SC64_ZP_ACTOR_DRAWABLE|SC64_ZP_ACTOR_MEN));
    assert_shared_fact(15, U_Z5_CIVILIAN);

    const int critter_ids[] = {89, 90, 95};
    for (unsigned i = 0; i < sizeof(critter_ids)/sizeof(critter_ids[0]); ++i) {
        const Sc64Z5ActorFact *f = sc64_z5_actor_fact(critter_ids[i]);
        assert(f && f->hp == 60 && f->shields == 0 && f->armor == 0);
        assert(f->size == 0 && f->sight_px == 7 * 32 && !f->flying);
        assert(f->left == 16 && f->up == 16 && f->right == 15 && f->down == 15);
        assert(f->flags & SC64_Z5_SOURCE_CRITTER);
        assert(f->flags & SC64_Z5_SOURCE_AUTO_ATTACK_MOVE);
        assert(f->ground.damage == 0 && f->air.damage == 0);
        assert(sc64_z5_actor_radius_px(f) == 8);
        int type = sc64_race_unit_from_raw(critter_ids[i]);
        assert(sc64_zp_actor_default_hp(type) == 60);
        assert(sc64_zp_actor_default_shields(type) == 0);
        assert(sc64_zp_actor_flags(type) ==
               (SC64_ZP_ACTOR_MOVABLE|SC64_ZP_ACTOR_COLLIDABLE|
                SC64_ZP_ACTOR_DRAWABLE|SC64_ZP_ACTOR_MEN|SC64_ZP_ACTOR_CRITTER));
        assert_shared_fact(critter_ids[i], type);
    }
    /* Critters carry the source Attack bit but, like civilians, have no
     * weapon. Attackability describes being damaged by another unit. */
    assert(sc64_z5_actor_fact(89)->flags & SC64_Z5_SOURCE_ATTACK_FLAG);
    assert(sc64_race_unit_from_raw(89) == U_Z5_RHYNADON);
    assert(sc64_race_unit_from_raw(90) == U_Z5_BENGALAAS);
    assert(sc64_race_unit_from_raw(95) == U_Z5_RAGNASAUR);
    assert(sc64_race_raw_unit(U_Z5_RHYNADON) == 89);
    assert(sc64_race_raw_unit(U_Z5_BENGALAAS) == 90);
    assert(sc64_race_raw_unit(U_Z5_RAGNASAUR) == 95);
    assert(sc64_race_unit_flying(U_Z5_BENGALAAS) == 0);
    assert(sc64_race_unit_damage(U_Z5_RHYNADON) == 0);

    const Sc64Z5BuildingFact *beacon = sc64_z5_building_fact(195);
    assert(beacon && beacon->raw == 195 && beacon->width_tiles == 3 && beacon->height_tiles == 2);
    assert(beacon->hp == 100000 && beacon->shields == 0 && beacon->armor == 0);
    assert(beacon->sight_px == 8 * 32 && beacon->size == 3);
    assert(beacon->left == 48 && beacon->up == 32 && beacon->right == 47 && beacon->down == 31);
    assert((beacon->flags & (SC64_Z5_SOURCE_BUILDING|SC64_Z5_SOURCE_INVINCIBLE|
                             SC64_Z5_SOURCE_BEACON|SC64_Z5_REF_MOVE_INTERACTION)) ==
           (SC64_Z5_SOURCE_BUILDING|SC64_Z5_SOURCE_INVINCIBLE|
            SC64_Z5_SOURCE_BEACON|SC64_Z5_REF_MOVE_INTERACTION));
    assert(!(beacon->flags & SC64_Z5_SOURCE_POWERUP));
    assert(sc64_z5_building_radius_px(beacon) == 24);
    assert(sc64_z5_actor_fact(195) == 0);
    assert(sc64_race_building_from_raw(195) == B_Z5_TERRAN_BEACON);
    assert(sc64_race_raw_building(B_Z5_TERRAN_BEACON) == 195);
    assert(sc64_race_building_rule(B_Z5_TERRAN_BEACON)->hp == 0);
    assert(sc64_zp_building_default_hp(B_Z5_TERRAN_BEACON) == 100000);
#ifdef SC64_T4_RUNTIME
    assert(sc64_race_building_max_hp(B_Z5_TERRAN_BEACON) == 100000);
#endif
    assert(sc64_race_building_armor(B_Z5_TERRAN_BEACON) == 0);
    const Sc64ZpBuildingFact *generic_beacon = sc64_zp_building_fact(B_Z5_TERRAN_BEACON);
    assert(generic_beacon && generic_beacon->hp == beacon->hp);
    assert(generic_beacon->left == beacon->left && generic_beacon->right == beacon->right);
    assert(generic_beacon->flags & SC64_ZP_ACTOR_BEACON);
    assert(generic_beacon->flags ==
           (SC64_ZP_ACTOR_DRAWABLE|SC64_ZP_ACTOR_BUILDING|
            SC64_ZP_ACTOR_INVINCIBLE|SC64_ZP_ACTOR_BEACON));
    assert(!(generic_beacon->flags &
             (SC64_ZP_ACTOR_SELECTABLE|SC64_ZP_ACTOR_COLLIDABLE)));
    assert(sc64_zp_building_sight_source_pixels(B_Z5_TERRAN_BEACON) == 256);
    { int l=0,u=0,r=0,d=0;
      assert(sc64_zp_building_bounds_source(B_Z5_TERRAN_BEACON,&l,&u,&r,&d));
      assert(l==48 && u==32 && r==47 && d==31); }

    const Sc64Z5ActorFact *disk = sc64_z5_actor_fact(218);
    assert(disk && disk->hp == 800 && disk->sight_px == 5 * 32 && disk->size == 3);
    assert(disk->left == 16 && disk->up == 16 && disk->right == 15 && disk->down == 15);
    assert(!disk->flying && disk->ground.damage == 0 && disk->air.damage == 0);
    assert((disk->flags & (SC64_Z5_SOURCE_INVINCIBLE|SC64_Z5_SOURCE_NEUTRAL_ACCESSORIES|
                           SC64_Z5_SOURCE_NPC_ACCESSORIES|SC64_Z5_SOURCE_POWERUP|
                           SC64_Z5_REF_MOVE_INTERACTION)) ==
           (SC64_Z5_SOURCE_INVINCIBLE|SC64_Z5_SOURCE_NEUTRAL_ACCESSORIES|
            SC64_Z5_SOURCE_NPC_ACCESSORIES|SC64_Z5_SOURCE_POWERUP|
            SC64_Z5_REF_MOVE_INTERACTION));
    assert(!(disk->flags & (SC64_Z5_SOURCE_BUILDING|SC64_Z5_REF_ATTACKABLE)));
    assert(sc64_z5_building_fact(218) == 0 && sc64_z5_actor_radius_px(disk) == 8);
    assert(sc64_race_unit_from_raw(218) == U_Z5_DATA_DISK);
    assert(sc64_race_raw_unit(U_Z5_DATA_DISK) == 218);
    assert(sc64_race_unit_rule(U_Z5_DATA_DISK)->hp == 800);
    assert(sc64_race_unit_flying(U_Z5_DATA_DISK) == 0);
    const Sc64ZpActorFact *generic_disk = sc64_zp_actor_fact(U_Z5_DATA_DISK);
    assert(generic_disk && generic_disk->hp == disk->hp);
    assert(generic_disk->left == disk->left && generic_disk->down == disk->down);
    assert(generic_disk->flags ==
           (SC64_ZP_ACTOR_DRAWABLE|SC64_ZP_ACTOR_INVINCIBLE|
            SC64_ZP_ACTOR_POWERUP));
    assert(!(generic_disk->flags &
             (SC64_ZP_ACTOR_SELECTABLE|SC64_ZP_ACTOR_COLLIDABLE|
              SC64_ZP_ACTOR_WEAPON)));
    assert_shared_fact(218, U_Z5_DATA_DISK);

    assert(sc64_z5_actor_fact(0) == 0);
    assert(sc64_z5_actor_fact(196) == 0); /* Protoss beacon is outside this Z5 set. */
    assert(sc64_z5_building_fact(194) == 0);
    assert(sc64_z5_actor_radius_px(0) == 0 && sc64_z5_building_radius_px(0) == 0);
#endif
    return 0;
}
