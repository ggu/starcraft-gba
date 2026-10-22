/* Asset-free checks for source actor facts and explicit unsupported policies. */
#include <assert.h>
#include <stddef.h>
#include "../starcraft/sc64_t5_actor_rules.h"

static void check_actor(unsigned raw, unsigned hp, unsigned armor,
                        unsigned supply, unsigned size, unsigned ground,
                        unsigned air, unsigned weapon, unsigned damage,
                        unsigned bonus, unsigned cooldown, unsigned range,
                        unsigned sight, unsigned speed_x100)
{
    const Sc64T5ActorRule *r = sc64_t5_actor_rule(raw);
    assert(r != NULL);
    assert(r->raw_type == raw);
    assert(r->hp == hp && r->armor == armor && r->supply_required == supply);
    assert(r->size == size);
    assert(r->normal_ground_attack == ground && r->normal_air_attack == air);
    assert(r->ground_weapon_raw == weapon);
    assert(r->weapon_damage == damage && r->weapon_damage_bonus == bonus);
    assert(r->weapon_cooldown == cooldown && r->weapon_range_pixels == range);
    assert(r->sight_tiles == sight && r->top_speed_x100 == speed_x100);
}

int main(void)
{
    assert(SC64_T5_ACTOR_RULE_COUNT == 3);

    /* BWAPI UnitType.cpp raw 16: Sarah Kerrigan. WeaponType.cpp raw 3 is
     * C-10 Canister Rifle: 30 concussive, +1, cooldown 22, range 192. */
    check_actor(16, 250, 3, 0, SC64_T5_SIZE_SMALL, 1, 1,
                SC64_T5_WEAPON_C10_SARAH_KERRIGAN, 30, 1, 22, 192, 11, 400);
    const Sc64T5ActorRule *kerrigan = sc64_t5_actor_rule(16);
    assert(kerrigan->air_weapon_raw == SC64_T5_WEAPON_C10_SARAH_KERRIGAN);
    assert(kerrigan->weapon_damage_type == SC64_T5_DAMAGE_CONCUSSIVE);
    assert(kerrigan->weapon_damage_factor == 1);
    assert(kerrigan->producer_raw == SC64_T5_NO_RAW_TYPE);
    assert(kerrigan->policy_flags & SC64_T5_POLICY_PERSONNEL_CLOAKING_UNIMPLEMENTED);
    assert(kerrigan->policy_flags & SC64_T5_POLICY_SCENARIO_ONLY);

    /* Raw 19 is the hero Vulture (not a Marine): it uses the distinct
     * Fragmentation Grenade Jim Raynor weapon, ground only. */
    check_actor(19, 300, 3, 0, SC64_T5_SIZE_MEDIUM, 1, 0,
                SC64_T5_WEAPON_FRAGMENTATION_GRENADE_RAYNOR, 30, 2,
                22, 160, 8, 640);
    const Sc64T5ActorRule *raynor = sc64_t5_actor_rule(19);
    assert(raynor->air_weapon_raw == SC64_T5_WEAPON_NONE);
    assert(raynor->weapon_damage_type == SC64_T5_DAMAGE_CONCUSSIVE);
    assert(raynor->weapon_damage_factor == 1);
    assert(raynor->producer_raw == SC64_T5_NO_RAW_TYPE);
    assert(raynor->policy_flags & SC64_T5_POLICY_SCENARIO_ONLY);

    /* Raw 13's weapon table records a 125 explosive hit, but OpenBW creates
     * mines through the Vulture Spider Mines ability. This slice deliberately
     * does not make them ordinary attack-order actors or model arming. */
    check_actor(13, 20, 0, 0, SC64_T5_SIZE_SMALL, 0, 0,
                SC64_T5_WEAPON_SPIDER_MINES, 125, 0, 22, 10, 3, 1600);
    const Sc64T5ActorRule *mine = sc64_t5_actor_rule(13);
    assert(mine->air_weapon_raw == SC64_T5_WEAPON_NONE);
    assert(mine->weapon_damage_type == SC64_T5_DAMAGE_EXPLOSIVE);
    assert(mine->weapon_damage_factor == 1);
    assert(mine->producer_raw == SC64_T5_NO_RAW_TYPE);
    assert(mine->deployer_raw == 2);
    assert(mine->deploy_tech_raw == SC64_T5_TECH_SPIDER_MINES);
    assert(mine->policy_flags & SC64_T5_POLICY_MINE_DEPLOY_AND_ARMING_UNIMPLEMENTED);

    assert(sc64_t5_actor_uses_normal_attack(16));
    assert(sc64_t5_actor_uses_normal_attack(19));
    assert(!sc64_t5_actor_uses_normal_attack(13));
    assert(sc64_t5_actor_rule(0) == NULL);
    assert(sc64_t5_actor_rule(255) == NULL);
    assert(sc64_t5_actor_rule(65535) == NULL);
    assert(!sc64_t5_actor_uses_normal_attack(255));
    return 0;
}
