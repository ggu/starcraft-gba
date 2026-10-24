#ifndef SC64_P7_ARCHON_RULES_H
#define SC64_P7_ARCHON_RULES_H

#if defined(SC64_P7_ARCHON_SPLASH) && \
    (!defined(SC64_P7_ACTORS) || !defined(SC64_RACES) || \
     !defined(SC64_ZP_CAMPAIGN_ACTORS))
#error "P7 Archon splash requires P7 actors, race combat, and Z/P actor facts"
#endif

#ifdef SC64_P7_ARCHON_SPLASH
#include "../common/gba.h"

enum {
    SC64_P7_ARCHON_SOURCE_WEAPON = 70,
    SC64_P7_ARCHON_BASE_DAMAGE = 30,
    SC64_P7_ARCHON_UPGRADE_DAMAGE = 3,
    SC64_P7_ARCHON_RANGE_SOURCE_PX = 64,
    SC64_P7_ARCHON_INNER_SOURCE_PX = 3,
    SC64_P7_ARCHON_MIDDLE_SOURCE_PX = 15,
    SC64_P7_ARCHON_OUTER_SOURCE_PX = 30,
    SC64_P7_ARCHON_COOLDOWN_GBA_TICKS = 25
};

enum {
    SC64_P7_ARCHON_SPLASH_NONE = 0,
    SC64_P7_ARCHON_SPLASH_INNER = 1,
    SC64_P7_ARCHON_SPLASH_MIDDLE = 2,
    SC64_P7_ARCHON_SPLASH_OUTER = 3
};

typedef struct {
    u32 packet_damage_q8;
    u8 tier;
} Sc64P7ArchonPacket;

typedef struct {
    u32 hp_q8, shields_q8, armor_q8;
    u8 has_shields, invincible;
} Sc64P7ArchonDamageTarget;

typedef struct {
    u32 hp_damage_q8, shield_damage_q8;
} Sc64P7ArchonDamageResult;

/* Pinned WeaponType::Psionic_Shockwave (raw 70): Enemy_Splash, Normal
 * damage, and 3/15/30 source-pixel inclusive rings. The source packet is
 * divided before armor; Q8 retains the half-point in the outer ring. The
 * existing GBA hit_target path quantizes to integer HP/shields afterward. */
static inline int
sc64_p7_archon_packet(unsigned upgrade_level,unsigned distance_source_px,
                      Sc64P7ArchonPacket *out)
{
    unsigned divisor,base;
    if (!out || upgrade_level > 10u) return 0;
    out->packet_damage_q8=0;
    out->tier=SC64_P7_ARCHON_SPLASH_NONE;
    if (distance_source_px > SC64_P7_ARCHON_OUTER_SOURCE_PX) return 1;
    if (distance_source_px <= SC64_P7_ARCHON_INNER_SOURCE_PX) {
        divisor=1u; out->tier=SC64_P7_ARCHON_SPLASH_INNER;
    } else if (distance_source_px <= SC64_P7_ARCHON_MIDDLE_SOURCE_PX) {
        divisor=2u; out->tier=SC64_P7_ARCHON_SPLASH_MIDDLE;
    } else {
        divisor=4u; out->tier=SC64_P7_ARCHON_SPLASH_OUTER;
    }
    base=SC64_P7_ARCHON_BASE_DAMAGE+
         SC64_P7_ARCHON_UPGRADE_DAMAGE*upgrade_level;
    out->packet_damage_q8=(base*256u)/divisor;
    return 1;
}

static inline unsigned
sc64_p7_archon_packet_integer_damage(const Sc64P7ArchonPacket *packet)
{
    return packet && packet->tier ? packet->packet_damage_q8/256u : 0u;
}

/* OpenBW Normal-damage ordering for a single Q8 packet. Archon weapon 70 has
 * no size modifier. Shield armor is zero in the compact runtime; HP armor is
 * supplied after shields. Integer GBA HP/shields are adapted by the caller. */
static inline int
sc64_p7_archon_normal_damage(const Sc64P7ArchonDamageTarget *target,
                              u32 packet_q8,Sc64P7ArchonDamageResult *result)
{
    u32 damage,shield_damage=0,hp_damage;
    if (!target || !result || target->has_shields>1 || target->invincible>1 ||
        target->hp_q8>0x7fffffffu || target->shields_q8>0x7fffffffu ||
        target->armor_q8>0x7fffffffu) return 0;
    result->hp_damage_q8=0;
    result->shield_damage_q8=0;
    if (!target->hp_q8 || target->invincible) return 1;
    damage=packet_q8 < 128u ? 128u : packet_q8;
    if (target->has_shields && target->shields_q8>=256u) {
        shield_damage=damage < target->shields_q8 ? damage : target->shields_q8;
        damage-=shield_damage;
    }
    if (damage>target->armor_q8) damage-=target->armor_q8;
    else damage=0;
    if (!shield_damage && damage<128u) damage=128u;
    hp_damage=damage < target->hp_q8 ? damage : target->hp_q8;
    result->shield_damage_q8=shield_damage;
    result->hp_damage_q8=hp_damage;
    return 1;
}
#endif /* SC64_P7_ARCHON_SPLASH */

#endif /* SC64_P7_ARCHON_RULES_H */
