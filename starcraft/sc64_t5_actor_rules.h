#ifndef SC64_T5_ACTOR_RULES_H
#define SC64_T5_ACTOR_RULES_H

/* Source-backed facts for actors used by the private T5 scenario. These are
 * data only: they do not claim that the GBA runtime implements StarCraft's
 * cloak or Spider Mine order/arming behavior. Values cite the pinned BWAPI
 * UnitType.cpp and WeaponType.cpp tables (raw IDs follow BWAPI UnitType IDs). */

enum {
    SC64_T5_DAMAGE_NORMAL = 0,
    SC64_T5_DAMAGE_CONCUSSIVE = 1,
    SC64_T5_DAMAGE_EXPLOSIVE = 2
};

enum {
    SC64_T5_SIZE_SMALL = 0,
    SC64_T5_SIZE_MEDIUM = 1
};

enum {
    SC64_T5_POLICY_PERSONNEL_CLOAKING_UNIMPLEMENTED = 1u << 0,
    SC64_T5_POLICY_MINE_DEPLOY_AND_ARMING_UNIMPLEMENTED = 1u << 1,
    SC64_T5_POLICY_SCENARIO_ONLY = 1u << 2
};

enum {
    SC64_T5_NO_RAW_TYPE = 0xffff,
    SC64_T5_TECH_SPIDER_MINES = 3,
    SC64_T5_WEAPON_C10_SARAH_KERRIGAN = 3,
    SC64_T5_WEAPON_FRAGMENTATION_GRENADE_RAYNOR = 5,
    SC64_T5_WEAPON_SPIDER_MINES = 6,
    SC64_T5_WEAPON_NONE = 130
};

typedef struct Sc64T5ActorRule {
    unsigned short raw_type;
    unsigned short hp;
    unsigned char armor;
    unsigned char supply_required; /* BWAPI's doubled supply units. */
    unsigned char size;
    unsigned char normal_ground_attack;
    unsigned char normal_air_attack;
    unsigned char ground_weapon_raw;
    unsigned char air_weapon_raw;
    unsigned char weapon_damage_type;
    unsigned char weapon_damage_factor;
    unsigned char weapon_damage_bonus;
    unsigned short weapon_damage;
    unsigned short weapon_cooldown;
    unsigned short weapon_range_pixels;
    unsigned short sight_tiles;
    unsigned short top_speed_x100; /* BWAPI source topSpeed, scaled by 100. */
    unsigned short producer_raw;   /* whatBuilds(); 0xffff means no producer. */
    unsigned short deployer_raw;   /* Separate from whatBuilds(). */
    unsigned char deploy_tech_raw;
    unsigned char reserved;
    unsigned short policy_flags;
} Sc64T5ActorRule;

/* Weapon raw IDs and stats come from BWAPI WeaponType.cpp; unit values from
 * UnitType.cpp. BWAPI reports no ordinary producer for the two heroes or the
 * mine. OpenBW's vulture order creates raw 13 after Spider Mines tech passes. */
static const Sc64T5ActorRule sc64_t5_actor_rules[] = {
    { 16, 250, 3, 0, SC64_T5_SIZE_SMALL,
      1, 1, SC64_T5_WEAPON_C10_SARAH_KERRIGAN,
      SC64_T5_WEAPON_C10_SARAH_KERRIGAN, SC64_T5_DAMAGE_CONCUSSIVE, 1, 1,
      30, 22, 192, 11, 400, SC64_T5_NO_RAW_TYPE, SC64_T5_NO_RAW_TYPE, 0,
      0, SC64_T5_POLICY_PERSONNEL_CLOAKING_UNIMPLEMENTED |
         SC64_T5_POLICY_SCENARIO_ONLY },
    { 19, 300, 3, 0, SC64_T5_SIZE_MEDIUM,
      1, 0, SC64_T5_WEAPON_FRAGMENTATION_GRENADE_RAYNOR,
      SC64_T5_WEAPON_NONE, SC64_T5_DAMAGE_CONCUSSIVE, 1, 2,
      30, 22, 160, 8, 640, SC64_T5_NO_RAW_TYPE, SC64_T5_NO_RAW_TYPE, 0,
      0, SC64_T5_POLICY_SCENARIO_ONLY },
    { 13, 20, 0, 0, SC64_T5_SIZE_SMALL,
      0, 0, SC64_T5_WEAPON_SPIDER_MINES, SC64_T5_WEAPON_NONE,
      SC64_T5_DAMAGE_EXPLOSIVE, 1, 0,
      125, 22, 10, 3, 1600, SC64_T5_NO_RAW_TYPE, 2,
      SC64_T5_TECH_SPIDER_MINES, 0,
      SC64_T5_POLICY_MINE_DEPLOY_AND_ARMING_UNIMPLEMENTED }
};

#define SC64_T5_ACTOR_RULE_COUNT \
    (sizeof(sc64_t5_actor_rules) / sizeof(sc64_t5_actor_rules[0]))

static const Sc64T5ActorRule *
sc64_t5_actor_rule(unsigned raw_type)
{
    for (unsigned i = 0; i < SC64_T5_ACTOR_RULE_COUNT; ++i)
        if (sc64_t5_actor_rules[i].raw_type == raw_type)
            return &sc64_t5_actor_rules[i];
    return 0;
}

/* A Spider Mine's weapon table entry describes its triggered explosion, not
 * a conventional selectable attack. Its source autonomous arming/targeting
 * state machine is intentionally outside this rules slice. */
static int
sc64_t5_actor_uses_normal_attack(unsigned raw_type)
{
    const Sc64T5ActorRule *rule = sc64_t5_actor_rule(raw_type);
    return rule && (rule->normal_ground_attack || rule->normal_air_attack);
}

#endif
