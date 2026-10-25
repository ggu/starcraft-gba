#include <assert.h>
#include "../starcraft/race_rules.h"

#ifndef SC64_P6_ACTORS
_Static_assert(SC_RU_COUNT == 28, "P6 actor enums stay absent with gate off");
_Static_assert(SC_RB_COUNT == 35, "P6 prop enum stays absent with gate off");
int main(void)
{
    assert(sc64_race_unit_from_raw(50) == -1);
    assert(sc64_race_unit_from_raw(74) == -1);
    assert(sc64_race_unit_from_raw(75) == -1);
    assert(sc64_race_building_from_raw(210) == -1);
    return 0;
}
#else
_Static_assert(SC_RU_COUNT <= 64, "race save unit codec has six-bit type capacity");
_Static_assert(SC_RB_COUNT <= 64, "building type values stay within compact codec bound");
#if defined(SC64_Z5_ACTORS) && defined(SC64_Z5_DOODADS) && \
    defined(SC64_Z6_ACTORS) && defined(SC64_P5_ACTORS) && \
    defined(SC64_T4_RUNTIME)
_Static_assert(SC_RU_COUNT == 52, "P6 appends after mixed T4 and prior actor profile");
_Static_assert(SC_RB_COUNT == 60, "P6 prop appends after mixed T4 profile");
#elif defined(SC64_Z5_ACTORS) && defined(SC64_Z5_DOODADS) && \
      defined(SC64_Z6_ACTORS) && defined(SC64_P5_ACTORS)
_Static_assert(SC_RU_COUNT == 45, "P6 appends after the mixed prior actor profile");
_Static_assert(SC_RB_COUNT == 51, "P6 prop appends after mixed prior doodads");
#elif defined(SC64_T4_RUNTIME)
_Static_assert(SC_RU_COUNT == 43, "P6 appends after T4 and ZP actor sets");
_Static_assert(SC_RB_COUNT == 51, "P6 prop appends after T4 and ZP building sets");
#else
_Static_assert(SC_RU_COUNT == 36, "P6 appends three after the ZP actor set");
_Static_assert(SC_RB_COUNT == 42, "P6 prop appends after the ZP building set");
#endif
_Static_assert(U_P6_HERO_DARK_TEMPLAR == U_P6_INFESTED_TERRAN + 1,
               "P6 actor enum append order is stable");
_Static_assert(U_P6_ZERATUL == U_P6_HERO_DARK_TEMPLAR + 1,
               "P6 actor enum append order is stable");
#ifdef SC64_P5_ACTORS
_Static_assert(U_P6_INFESTED_TERRAN == U_P5_BATTLECRUISER + 1,
               "P6 actors append after P5 without renumbering it");
#else
_Static_assert(U_P6_INFESTED_TERRAN == U_ZP_ULTRALISK + 1,
               "P6 actors append after ZP actors without renumbering them");
#endif

static void check_actor(int type, int raw, int expected_hp, int expected_shields,
                        int expected_minerals, int expected_gas, int frames,
                        int sight_px, int left, int up, int right, int down,
                        int ground_damage, int ground_bonus, int cooldown,
                        int range, int hits)
{
    const Sc64P6AddedActorFact *source = sc64_p6_added_actor_fact(raw);
    const Sc64ZpActorFact *bound = sc64_zp_actor_fact(type);
    const Sc64RaceUnitRule *rule = sc64_race_unit_rule(type);
    int l=0,u=0,r=0,d=0;
    assert(source && bound && rule);
    assert(sc64_race_unit_from_raw(raw) == type);
    assert(sc64_race_raw_unit(type) == raw);
    assert(source->raw == raw && bound->raw == raw);
    assert(rule->producer == 255 && rule->supply ==
           (raw == SC64_P6_RAW_ZERATUL ? 0 : 2));
    assert(rule->hp == expected_hp && rule->shields == expected_shields);
    assert(rule->minerals == expected_minerals && rule->gas == expected_gas);
    assert(rule->build_frames == frames);
    assert(bound->hp == source->hp && bound->shields == source->shields);
    assert(bound->armor == source->armor);
    assert(bound->minerals == source->minerals && bound->gas == source->gas);
    assert(bound->build_frames == source->build_frames);
    assert(bound->sight_px == source->sight_tiles * 32);
    assert(bound->left == source->left && bound->up == source->up);
    assert(bound->right == source->right && bound->down == source->down);
    assert(bound->flying == source->flying);
    assert(bound->ground_damage == source->ground_damage);
    assert(bound->ground_bonus == source->ground_bonus);
    assert(bound->ground_cooldown == source->ground_cooldown);
    assert(bound->ground_range == source->ground_range);
    assert(bound->ground_hits == source->ground_hits);
    assert(bound->air_damage == source->air_damage);
    assert(bound->air_bonus == source->air_bonus);
    assert(bound->air_cooldown == source->air_cooldown);
    assert(bound->air_range == source->air_range);
    assert(bound->air_hits == source->air_hits);
    assert(bound->ground_damage == ground_damage);
    assert(bound->ground_bonus == ground_bonus);
    assert(bound->ground_cooldown == cooldown);
    assert(bound->ground_range == range);
    assert(bound->ground_hits == hits);
    assert(sc64_zp_actor_sight_source_pixels(type) == sight_px);
    assert(sc64_zp_actor_bounds_source(type,&l,&u,&r,&d));
    assert(l==left && u==up && r==right && d==down);
    assert(sc64_race_unit_armor(type) == source->armor);
    assert(!sc64_race_unit_flying(type));
    /* Source attacks remain audit facts only until dedicated P6 combat
       handlers implement suicide/cloak behavior. */
    assert(!(bound->flags & SC64_ZP_ACTOR_WEAPON));
    assert((bound->flags & (SC64_ZP_ACTOR_SELECTABLE|
                            SC64_ZP_ACTOR_MOVABLE|
                            SC64_ZP_ACTOR_COLLIDABLE|
                            SC64_ZP_ACTOR_DRAWABLE|
                            SC64_ZP_ACTOR_MEN)) ==
           (SC64_ZP_ACTOR_SELECTABLE|SC64_ZP_ACTOR_MOVABLE|
            SC64_ZP_ACTOR_COLLIDABLE|SC64_ZP_ACTOR_DRAWABLE|
            SC64_ZP_ACTOR_MEN));
    assert(sc64_race_unit_damage(type) == 0);
    assert(sc64_race_unit_damage_vs(type,0) == 0);
    assert(!sc64_race_unit_can_attack_target_class(type,0));
    assert(!sc64_race_unit_can_attack_air(type));
    assert(sc64_race_unit_name(type)[0] != '\0');
}

int main(void)
{
    const Sc64P6ActorPolicyRequirement *policy;
    const Sc64ZpBuildingFact *trap;
    int l=0,u=0,r=0,d=0;

    check_actor(U_P6_INFESTED_TERRAN,50,60,0,100,50,600,160,
                8,9,8,10,500,0,1,3,1);
    check_actor(U_P6_HERO_DARK_TEMPLAR,74,40,80,150,150,750,224,
                12,6,11,19,45,1,30,15,1);
    check_actor(U_P6_ZERATUL,75,60,400,100,300,1500,224,
                12,6,11,19,100,1,22,15,1);
    assert(sc64_race_unit_from_raw(51) == U_ZP_INFESTED_KERRIGAN);

    policy = sc64_p6_actor_policy_requirement(50);
    assert(policy && policy->required_capabilities != 0 &&
           policy->implemented_capabilities == 0);
    policy = sc64_p6_actor_policy_requirement(74);
    assert(policy && policy->implemented_capabilities == 0);
    policy = sc64_p6_actor_policy_requirement(75);
    assert(policy && policy->implemented_capabilities == 0);

    trap = sc64_p6_added_actor_fact(210) ? sc64_zp_building_fact(B_P6_WALL_MISSILE_TRAP) : 0;
    assert(trap && trap->raw == 210);
    assert(sc64_race_building_from_raw(210) == B_P6_WALL_MISSILE_TRAP);
    assert(sc64_race_raw_building(B_P6_WALL_MISSILE_TRAP) == 210);
    assert(sc64_race_building_rule(B_P6_WALL_MISSILE_TRAP)->worker == 255);
#ifdef SC64_T4_RUNTIME
    assert(sc64_race_building_max_hp(B_P6_WALL_MISSILE_TRAP) == 50);
#endif
    assert(sc64_race_building_armor(B_P6_WALL_MISSILE_TRAP) == 0);
    assert(trap->hp == sc64_p6_added_actor_fact(210)->hp);
    assert(trap->armor == sc64_p6_added_actor_fact(210)->armor);
    assert(trap->sight_px == sc64_p6_added_actor_fact(210)->sight_tiles * 32);
    assert(trap->left == sc64_p6_added_actor_fact(210)->left &&
           trap->up == sc64_p6_added_actor_fact(210)->up &&
           trap->right == sc64_p6_added_actor_fact(210)->right &&
           trap->down == sc64_p6_added_actor_fact(210)->down);
    assert(sc64_zp_building_flags(B_P6_WALL_MISSILE_TRAP) &
           SC64_ZP_ACTOR_BUILDING);
    assert(!(sc64_zp_building_flags(B_P6_WALL_MISSILE_TRAP) &
             SC64_ZP_ACTOR_WEAPON));
    assert(sc64_zp_building_bounds_source(B_P6_WALL_MISSILE_TRAP,
                                          &l,&u,&r,&d));
    assert(l==16 && u==16 && r==15 && d==15);
    assert(!sc64_p6_source_is_building(210));
    assert(sc64_p6_source_is_building(196));
    assert(sc64_race_building_name(B_P6_WALL_MISSILE_TRAP)[0] != '\0');
    assert(sc64_race_unit_from_raw(210) == -1);
    return 0;
}
#endif
