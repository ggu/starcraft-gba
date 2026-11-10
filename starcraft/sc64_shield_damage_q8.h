/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_SHIELD_DAMAGE_Q8_H
#define SC64_SHIELD_DAMAGE_Q8_H

#include <stdint.h>
#include <limits.h>

/* Pure per-hit arithmetic, based on pinned OpenBW weapon_deal_damage.
 * All quantities use 1/256 point units. The caller owns source targeting,
 * visibility, hit multiplicity, deaths, effects and live-state quantization.
 * A caller must not pre-apply armor or reuse a previously resolved packet. */
enum {
    SC64_DAMAGE_NONE, SC64_DAMAGE_NORMAL, SC64_DAMAGE_EXPLOSIVE,
    SC64_DAMAGE_CONCUSSIVE, SC64_DAMAGE_IGNORE_ARMOR
};
enum {
    SC64_DAMAGE_SIZE_INDEPENDENT, SC64_DAMAGE_SIZE_SMALL,
    SC64_DAMAGE_SIZE_MEDIUM, SC64_DAMAGE_SIZE_LARGE
};
typedef struct {
    uint32_t packet_q8, shield_q8, hp_q8, matrix_q8;
    uint32_t shield_armor_q8, hp_armor_q8, acid_bonus_q8;
    unsigned divisor, damage_type, size;
    unsigned has_shields, hallucinated, invincible;
} Sc64ShieldDamageInput;
typedef struct {
    uint32_t shield_damage_q8, hp_damage_q8, matrix_damage_q8;
    uint32_t shield_after_q8, hp_after_q8, matrix_after_q8;
} Sc64ShieldDamageResult;

/* Return zero on invalid/overflowing input and leave *out unchanged. The
 * supported divisor range includes the source direct and 1/2/4 splash paths. */
static inline int sc64_shield_damage_q8(const Sc64ShieldDamageInput *in,
                                         Sc64ShieldDamageResult *out)
{
    Sc64ShieldDamageResult r;
    uint32_t damage;
    if (!in || !out || !in->divisor || in->divisor > 65535u ||
        in->damage_type > SC64_DAMAGE_IGNORE_ARMOR ||
        in->size > SC64_DAMAGE_SIZE_LARGE || in->has_shields > 1u ||
        in->hallucinated > 1u || in->invincible > 1u ||
        (in->hallucinated && in->packet_q8 > UINT32_MAX/2u)) return 0;
    damage = in->hallucinated ? in->packet_q8*2u : in->packet_q8;
    damage /= in->divisor;
    if (damage > UINT32_MAX-in->acid_bonus_q8) return 0;
    damage += in->acid_bonus_q8;
    r.shield_damage_q8=r.hp_damage_q8=r.matrix_damage_q8=0;
    r.shield_after_q8=in->shield_q8;r.hp_after_q8=in->hp_q8;
    r.matrix_after_q8=in->matrix_q8;
    if (!in->hp_q8 || in->invincible) { *out=r; return 1; }
    if (damage < 128u) damage=128u;
    r.matrix_damage_q8=damage<in->matrix_q8?damage:in->matrix_q8;
    damage-=r.matrix_damage_q8;
    r.matrix_after_q8-=r.matrix_damage_q8;
    if (in->has_shields && in->shield_q8>=256u) {
        if (in->damage_type!=SC64_DAMAGE_IGNORE_ARMOR)
            damage=damage>in->shield_armor_q8?damage-in->shield_armor_q8:128u;
        r.shield_damage_q8=damage<in->shield_q8?damage:in->shield_q8;
        damage-=r.shield_damage_q8;
        r.shield_after_q8-=r.shield_damage_q8;
    }
    if (in->damage_type!=SC64_DAMAGE_IGNORE_ARMOR)
        damage=damage>in->hp_armor_q8?damage-in->hp_armor_q8:0u;
    if (in->damage_type==SC64_DAMAGE_NONE) damage=0;
    else if (in->damage_type==SC64_DAMAGE_EXPLOSIVE) {
        if (in->size==SC64_DAMAGE_SIZE_INDEPENDENT) damage=0;
        else if (in->size==SC64_DAMAGE_SIZE_SMALL) damage/=2u;
        else if (in->size==SC64_DAMAGE_SIZE_MEDIUM)
            damage=(damage/4u)*3u+(damage%4u)*3u/4u;
    } else if (in->damage_type==SC64_DAMAGE_CONCUSSIVE) {
        if (in->size==SC64_DAMAGE_SIZE_INDEPENDENT) damage=0;
        else if (in->size==SC64_DAMAGE_SIZE_MEDIUM) damage/=2u;
        else if (in->size==SC64_DAMAGE_SIZE_LARGE) damage/=4u;
    }
    if (!r.shield_damage_q8 && damage<128u) damage=128u;
    r.hp_damage_q8=damage<in->hp_q8?damage:in->hp_q8;
    r.hp_after_q8-=r.hp_damage_q8;
    *out=r;return 1;
}
#endif
