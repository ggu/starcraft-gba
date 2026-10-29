/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_BUILDING_DAMAGE_H
#define SC64_BUILDING_DAMAGE_H

/* Pure Terran building burn rules. HP values are in 1/256 HP units.
 * A 30 Hz caller supplies the persistent simulation frame; four source
 * updates run per five caller frames. This phase needs no per-building timer.
 *
 * The caller owns HP storage, repair and death/kill-credit handling. Burning
 * bypasses weapon armor and shields. The upstream damage path also does not
 * check invincibility; this parameter is included to make that explicit.
 */
#define SC64_BUILDING_HP_ONE 256u
#define SC64_BUILDING_BURN_Q8 20u
#define SC64_BUILDING_BURN_OWNER_MAX 4u
#define SC64_BUILDING_BURN_PACKED_MAX 1279u

/* Terran buildings have no shield regeneration; their persisted shield
 * remainder can hold a Q8 HP deficit in the low byte and a GBA-policy
 * damage-credit owner in the high bits. Owner 0 is a real player, not an
 * unset marker. Initialize credit to the victim owner if no attacker exists.
 * Only use this representation for eligible Terran buildings: shielded
 * buildings and special objects can use the field for their own state.
 * Inputs must satisfy hp <= 65535, owner <= 4, and packed <= 1279.
 */
static inline unsigned sc64_building_burn_owner(unsigned packed)
{
    return packed >> 8;
}

static inline unsigned sc64_building_burn_with_owner(unsigned packed, unsigned owner)
{
    return (owner << 8) | (packed & 255u);
}

static inline unsigned sc64_building_burn_hp_q8(unsigned hp, unsigned packed)
{
    return hp ? hp * 256u - (packed & 255u) : 0u;
}

/* Encode a Q8 HP value as ceiling whole HP and its fractional deficit.
 * Preserve credit, including on death; hp_q8 is at most 65535 * 256.
 */
static inline void sc64_building_burn_store_hp(unsigned hp_q8, unsigned *hp,
                                                unsigned *packed)
{
    unsigned whole = (hp_q8 + 255u) / 256u;
    *hp = whole;
    *packed = (*packed & ~255u) | (whole * 256u - hp_q8);
}

/* An integer repair retains a fractional deficit until fully repaired. */
static inline void sc64_building_burn_full_repair(unsigned hp, unsigned max_hp,
                                                   unsigned *packed)
{
    if (hp >= max_hp) *packed &= ~255u;
}

static inline int sc64_building_burn_source_frame(unsigned frame)
{
    return frame % 5u != 0u;
}

/* max_hp is whole HP, as in the source data; valid range 1..65535.
 * Compare without division: floor(visible_hp * 100 / max_hp) <= 33.
 */
static inline int sc64_building_burn_threshold(unsigned hp_q8, unsigned max_hp)
{
    unsigned visible_hp;
    if (!hp_q8 || !max_hp || max_hp > 65535u || hp_q8 > 65535u * 256u)
        return 0;
    visible_hp = (hp_q8 + 255u) / 256u;
    return visible_hp * 100u < max_hp * 34u;
}

/* Input flags describe unit type/race, not ownership. Flying buildings
 * continue to burn. Return remaining Q8 HP, saturating at zero.
 */
static inline unsigned sc64_building_burn_tick(unsigned hp_q8, unsigned max_hp,
                                               int terran, int building,
                                               int completed, int invincible,
                                               unsigned frame)
{
    (void)invincible;
    if (!terran || !building || !completed ||
        !sc64_building_burn_source_frame(frame) ||
        !sc64_building_burn_threshold(hp_q8, max_hp)) return hp_q8;
    return hp_q8 > SC64_BUILDING_BURN_Q8 ? hp_q8 - SC64_BUILDING_BURN_Q8 : 0u;
}

#endif
