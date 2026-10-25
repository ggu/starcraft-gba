#include <assert.h>
#include <string.h>

#include "../starcraft/race_rules.h"

int main(void)
{
#ifdef SC64_P5_ACTORS
    const Sc64ZpActorFact *shuttle = sc64_zp_actor_fact(U_P5_SHUTTLE);
    const Sc64RaceUnitRule *rule = &sc64_race_unit_rules[U_P5_SHUTTLE];
    assert(shuttle != 0);
    assert(shuttle->raw == 69 && shuttle->hp == 80 && shuttle->shields == 60);
    assert(shuttle->armor == 1 && shuttle->minerals == 200 && shuttle->gas == 0);
    assert(shuttle->build_frames == 900 && shuttle->sight_px == 256);
    assert(shuttle->left == 20 && shuttle->up == 16);
    assert(shuttle->right == 19 && shuttle->down == 15);
    assert(shuttle->flying && shuttle->size == 2);
    assert(shuttle->ground_damage == 0 && shuttle->air_damage == 0);
    assert((shuttle->flags & (SC64_ZP_ACTOR_SELECTABLE|
           SC64_ZP_ACTOR_MOVABLE|SC64_ZP_ACTOR_COLLIDABLE|
           SC64_ZP_ACTOR_DRAWABLE|SC64_ZP_ACTOR_MEN)) ==
           (SC64_ZP_ACTOR_SELECTABLE|SC64_ZP_ACTOR_MOVABLE|
            SC64_ZP_ACTOR_COLLIDABLE|SC64_ZP_ACTOR_DRAWABLE|
            SC64_ZP_ACTOR_MEN));
    assert(!(shuttle->flags & SC64_ZP_ACTOR_WEAPON));
    assert(sc64_race_raw_unit(U_P5_SHUTTLE) == 69);
    assert(sc64_race_unit_from_raw(69) == U_P5_SHUTTLE);
    assert(strcmp(sc64_race_unit_name(U_P5_SHUTTLE), "SHUTTLE") == 0);
    assert(rule->race == R_PROTOSS && rule->producer == 255);
    assert(rule->supply == 4 && rule->minerals == 200 && rule->gas == 0);
    assert(rule->build_frames == 900 && rule->hp == 80 && rule->shields == 60);

    /* Trigger-created raw-12 Battlecruisers need their native gun facts even
       though this cinematic actor has no ordinary producer route. */
    const Sc64ZpActorFact *battlecruiser =
        sc64_zp_actor_fact(U_P5_BATTLECRUISER);
    const Sc64RaceUnitRule *bc_rule =
        &sc64_race_unit_rules[U_P5_BATTLECRUISER];
    assert(battlecruiser != 0 && battlecruiser->raw == 12);
    assert(battlecruiser->hp == 500 && battlecruiser->shields == 0);
    assert(battlecruiser->armor == 3 && battlecruiser->minerals == 400);
    assert(battlecruiser->gas == 300 && battlecruiser->build_frames == 2000);
    assert(battlecruiser->sight_px == 352);
    assert(battlecruiser->left == 37 && battlecruiser->up == 29);
    assert(battlecruiser->right == 37 && battlecruiser->down == 29);
    assert(battlecruiser->flying == 1 && battlecruiser->size == 2);
    assert(battlecruiser->ground_damage == 25 && battlecruiser->ground_bonus == 3);
    assert(battlecruiser->ground_cooldown == 30 &&
           battlecruiser->ground_range == 192 && battlecruiser->ground_hits == 1);
    assert(battlecruiser->air_damage == 25 && battlecruiser->air_bonus == 3);
    assert(battlecruiser->air_cooldown == 30 &&
           battlecruiser->air_range == 192 && battlecruiser->air_hits == 1);
    assert((battlecruiser->flags & SC64_ZP_ACTOR_WEAPON) != 0);
    assert(sc64_race_raw_unit(U_P5_BATTLECRUISER) == 12);
    assert(sc64_race_unit_from_raw(12) == U_P5_BATTLECRUISER);
    assert(strcmp(sc64_race_unit_name(U_P5_BATTLECRUISER), "BATTLECRUISER") == 0);
    assert(bc_rule->race == R_TER && bc_rule->producer == 255);
    assert(bc_rule->supply == 12 && bc_rule->hp == 500);
    assert(bc_rule->minerals == 400 && bc_rule->gas == 300);
    assert(bc_rule->build_frames == 2000);
#else
#ifndef SC64_TERRAN_LATE_ACTORS
    assert(SC_RU_COUNT == 28);
    assert(sc64_race_unit_from_raw(69) == -1);
#else
    /* The P5 gate-off build keeps the previously supported late actor IDs. */
    assert(sc64_race_unit_from_raw(69) == U_SHUTTLE);
    assert(sc64_race_unit_from_raw(12) == U_BATTLECRUISER);
#endif
#endif

#ifdef SC64_Z6_ACTORS
    const Sc64ZpActorFact *guardian = sc64_zp_actor_fact(U_Z6_GUARDIAN);
    const Sc64RaceUnitRule *guardian_rule = &sc64_race_unit_rules[U_Z6_GUARDIAN];
    assert(guardian != 0 && guardian->raw == 44);
    assert(guardian->hp == 150 && guardian->shields == 0 && guardian->armor == 2);
    assert(guardian->sight_px == 11 * 32);
    assert(guardian->left == 22 && guardian->up == 22);
    assert(guardian->right == 21 && guardian->down == 21);
    assert(guardian->flying == 1 && guardian->size == 2);
    assert(guardian->ground_damage == 20 && guardian->ground_bonus == 2);
    assert(guardian->ground_cooldown == 30 && guardian->ground_range == 256);
    assert(guardian->air_damage == 0);
    assert((guardian->flags & (SC64_ZP_ACTOR_SELECTABLE|
           SC64_ZP_ACTOR_MOVABLE|SC64_ZP_ACTOR_COLLIDABLE|
           SC64_ZP_ACTOR_DRAWABLE|SC64_ZP_ACTOR_MEN|
           SC64_ZP_ACTOR_WEAPON)) ==
           (SC64_ZP_ACTOR_SELECTABLE|SC64_ZP_ACTOR_MOVABLE|
            SC64_ZP_ACTOR_COLLIDABLE|SC64_ZP_ACTOR_DRAWABLE|
            SC64_ZP_ACTOR_MEN|SC64_ZP_ACTOR_WEAPON));
    assert(sc64_zp_actor_default_hp(U_Z6_GUARDIAN) == 150);
    assert(sc64_zp_actor_default_shields(U_Z6_GUARDIAN) == 0);
    assert(sc64_zp_actor_sight_source_pixels(U_Z6_GUARDIAN) == 352);
    assert(sc64_race_raw_unit(U_Z6_GUARDIAN) == 44);
    assert(sc64_race_unit_from_raw(44) == U_Z6_GUARDIAN);
    assert(strcmp(sc64_race_unit_name(U_Z6_GUARDIAN), "GUARDIAN") == 0);
    assert(guardian_rule->supply == 4 && guardian_rule->hp == 150);
#endif
    return 0;
}
