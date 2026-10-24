#ifndef SC64_PSIONIC_STORM_H
#define SC64_PSIONIC_STORM_H

#include "../common/gba.h"

/*
 * Bounded Psionic Storm rules for a future adapter.  This is not wired into
 * the game or a claim of frame-for-frame native Brood War spell behavior.
 *
 * Source-backed facts: BWAPI's pinned WeaponType table has Storm weapon ID
 * 84, 14 damage, Ignore Armor, radial splash and 48-pixel inner/mid/outer
 * radii. The BWAPI TechType is Psionic Storm (ID 19), energy cost 75. In
 * P4's PTEC, player slot 7 uses the global defaults; global research marks
 * Storm researched. OpenBW also allows hero Tassadar to use it without a
 * player research flag. P4's UNIT rows set the slot-7 High Templar and
 * Tassadar energy to 100%; UnitType max energy is 200 (250 for heroes).
 *
 * Adapter policy: each game_update is one 30 Hz simulation tick, and one
 * source simulation frame maps to 30/24 GBA ticks. The local pinned weapon
 * tables do not specify the storm projectile impact schedule. The 16/8 source
 * frame pulse schedule below is therefore an authored policy assumption, not
 * asserted native behavior.
 */
enum {
    SC64_STORM_MAX_ACTIVE = 4,
    SC64_STORM_TECH_ID = 19,
    SC64_STORM_WEAPON_ID = 84,
    SC64_STORM_ENERGY_COST = 75,
    SC64_STORM_DAMAGE_PER_PULSE = 14,
    SC64_STORM_RADIUS_SOURCE_PX = 48,
    SC64_STORM_RADIUS_GBA_PX = 24,
    SC64_STORM_CAST_RANGE_SOURCE_PX = 288,
    SC64_STORM_CAST_RANGE_GBA_PX = 144,
    SC64_STORM_PULSE_COUNT = 8,
    SC64_STORM_FIRST_PULSE_SOURCE_FRAMES = 16,
    SC64_STORM_PULSE_INTERVAL_SOURCE_FRAMES = 8,
    SC64_STORM_FIRST_PULSE_TICKS = 20,
    SC64_STORM_PULSE_INTERVAL_TICKS = 10,
    SC64_STORM_MAX_ENERGY = 200,
    SC64_STORM_HERO_MAX_ENERGY = 250,
    SC64_STORM_ENERGY_ONE = 256,
    SC64_STORM_ENERGY_REGEN_Q8_PER_SOURCE_FRAME = 8,
    SC64_STORM_TARGET_AIR = 1,
    SC64_STORM_TARGET_GROUND = 2,
    SC64_STORM_TARGET_NONBUILDING = 4
};

typedef struct {
    u16 x;
    u16 y;
    u16 age_ticks;
    u8 pulses_left;
    u8 active;
} Sc64PsionicStorm;

typedef struct {
    Sc64PsionicStorm storm[SC64_STORM_MAX_ACTIVE];
} Sc64PsionicStormState;

typedef struct {
    u16 x;
    u16 y;
    u8 effect_index;
    u8 pulse_number;
} Sc64PsionicStormPulse;

static inline void sc64_psionic_storm_init(Sc64PsionicStormState *s)
{
    u8 *p;
    u32 i;
    if (!s) return;
    p = (u8 *)s;
    for (i = 0; i < (u32)sizeof(*s); ++i) p[i] = 0;
}

static inline int sc64_psionic_storm_state_valid(const Sc64PsionicStormState *s)
{
    u32 i;
    if (!s) return 0;
    for (i = 0; i < SC64_STORM_MAX_ACTIVE; ++i) {
        const Sc64PsionicStorm *e = &s->storm[i];
        if (e->active > 1) return 0;
        if (!e->active) {
            if (e->x || e->y || e->age_ticks || e->pulses_left) return 0;
        } else {
            u8 pulses_done = 0;
            if (e->age_ticks >= SC64_STORM_FIRST_PULSE_TICKS)
                pulses_done = (u8)(1u + (e->age_ticks - SC64_STORM_FIRST_PULSE_TICKS) /
                                            SC64_STORM_PULSE_INTERVAL_TICKS);
            if (e->pulses_left == 0 || e->pulses_left > SC64_STORM_PULSE_COUNT ||
                pulses_done >= SC64_STORM_PULSE_COUNT ||
                e->pulses_left != SC64_STORM_PULSE_COUNT - pulses_done ||
                e->age_ticks >= SC64_STORM_FIRST_PULSE_TICKS +
                                   SC64_STORM_PULSE_COUNT * SC64_STORM_PULSE_INTERVAL_TICKS)
                return 0;
        }
    }
    return 1;
}

/* The caller supplies Q8 energy in Unit.carry. Q8 250 energy is 64000, within
 * the compact-save signed-17 range [-65536, 65535]. Do not call for workers
 * whose carry currently has a resource/repair meaning. */
static inline int sc64_psionic_storm_cast(Sc64PsionicStormState *s, u16 x, u16 y,
                                           u16 caster_x, u16 caster_y,
                                           int *energy_q8, int researched)
{
    u32 i;
    int dx, dy;
    if (!s || !energy_q8 || !researched || *energy_q8 < 0 ||
        *energy_q8 > SC64_STORM_HERO_MAX_ENERGY * SC64_STORM_ENERGY_ONE ||
        *energy_q8 < SC64_STORM_ENERGY_COST * SC64_STORM_ENERGY_ONE ||
        !sc64_psionic_storm_state_valid(s)) return 0;
    dx = (int)x - (int)caster_x;
    dy = (int)y - (int)caster_y;
    if (dx < -SC64_STORM_CAST_RANGE_GBA_PX || dx > SC64_STORM_CAST_RANGE_GBA_PX ||
        dy < -SC64_STORM_CAST_RANGE_GBA_PX || dy > SC64_STORM_CAST_RANGE_GBA_PX ||
        dx * dx + dy * dy > SC64_STORM_CAST_RANGE_GBA_PX * SC64_STORM_CAST_RANGE_GBA_PX)
        return 0;
    for (i = 0; i < SC64_STORM_MAX_ACTIVE; ++i) {
        Sc64PsionicStorm *e = &s->storm[i];
        if (!e->active) {
            *energy_q8 -= SC64_STORM_ENERGY_COST * SC64_STORM_ENERGY_ONE;
            e->x = x;
            e->y = y;
            e->age_ticks = 0;
            e->pulses_left = SC64_STORM_PULSE_COUNT;
            e->active = 1;
            return 1;
        }
    }
    return 0;
}

/* Source simulation frames converted to 30 Hz GBA ticks by ceiling at 30/24. */
static inline u32 sc64_psionic_storm_source_frames_to_ticks(u32 frames)
{
    u32 whole = frames / 4u;
    u32 rem = frames % 4u;
    if (whole > 0xffffffffu / 5u) return 0xffffffffu;
    whole *= 5u;
    rem = (rem * 5u + 3u) / 4u;
    if (whole > 0xffffffffu - rem) return 0xffffffffu;
    return whole + rem;
}

/* Convert milliseconds to 30 Hz policy ticks, rounding any partial tick up. */
static inline u32 sc64_psionic_storm_ms_to_ticks(u32 milliseconds)
{
    u32 seconds = milliseconds / 1000u;
    u32 remainder = milliseconds % 1000u;
    if (seconds > 0xffffffffu / 30u) return 0xffffffffu;
    seconds *= 30u;
    remainder = (remainder * 30u + 999u) / 1000u;
    if (seconds > 0xffffffffu - remainder) return 0xffffffffu;
    return seconds + remainder;
}

/* Add 32/5 Q8 energy per 30 Hz adapter tick (8 Q8 per 24 Hz source frame).
 * The caller carries the 0..4 remainder across ticks; the cap is in whole
 * energy points. Returns updated Q8 energy, or -1 on bad input. */
static inline int sc64_psionic_storm_regen_q8(int energy_q8, u16 max_energy,
                                               u8 *remainder_0_to_4)
{
    int cap;
    u8 accum;
    if (energy_q8 < 0 || max_energy > SC64_STORM_HERO_MAX_ENERGY ||
        !remainder_0_to_4 || *remainder_0_to_4 > 4) return -1;
    cap = (int)max_energy * SC64_STORM_ENERGY_ONE;
    /* Source +8 Q8 per 24 Hz frame; 24/30 source frames per GBA tick => 32/5. */
    accum = (u8)(*remainder_0_to_4 + 32u);
    *remainder_0_to_4 = (u8)(accum % 5u);
    if (energy_q8 >= cap) return cap;
    energy_q8 += accum / 5u;
    return energy_q8 > cap ? cap : energy_q8;
}

/* Advance one adapter tick and write each due pulse's center to `pulses`.
 * Returns the number of emitted pulses; effects end after pulse eight.
 * Invalid state returns 0 and leaves state unchanged. */
static inline u8 sc64_psionic_storm_step(Sc64PsionicStormState *s,
                                         Sc64PsionicStormPulse pulses[SC64_STORM_MAX_ACTIVE])
{
    u8 pulse_count = 0;
    u32 i;
    if (!sc64_psionic_storm_state_valid(s) || !pulses) return 0;
    for (i = 0; i < SC64_STORM_MAX_ACTIVE; ++i) {
        Sc64PsionicStorm *e = &s->storm[i];
        if (!e->active) continue;
        ++e->age_ticks;
        if (e->age_ticks >= SC64_STORM_FIRST_PULSE_TICKS &&
            ((e->age_ticks - SC64_STORM_FIRST_PULSE_TICKS) %
             SC64_STORM_PULSE_INTERVAL_TICKS) == 0) {
            pulses[pulse_count].x = e->x;
            pulses[pulse_count].y = e->y;
            pulses[pulse_count].effect_index = (u8)i;
            pulses[pulse_count].pulse_number = (u8)(SC64_STORM_PULSE_COUNT - e->pulses_left + 1u);
            ++pulse_count;
            --e->pulses_left;
            if (!e->pulses_left) {
                e->x = e->y = e->age_ticks = 0;
                e->active = 0;
            }
        }
    }
    return pulse_count;
}

/* Integer center-point radius test in GBA pixels (source radius is 48px). */
static inline int sc64_psionic_storm_contains(const Sc64PsionicStorm *e, u16 x, u16 y)
{
    int dx, dy;
    if (!e || !e->active) return 0;
    dx = (int)e->x - (int)x;
    dy = (int)e->y - (int)y;
    if (dx < -SC64_STORM_RADIUS_GBA_PX || dx > SC64_STORM_RADIUS_GBA_PX ||
        dy < -SC64_STORM_RADIUS_GBA_PX || dy > SC64_STORM_RADIUS_GBA_PX) return 0;
    return dx * dx + dy * dy <= SC64_STORM_RADIUS_GBA_PX * SC64_STORM_RADIUS_GBA_PX;
}

/* OpenBW/BWAPI's pinned weapon target attributes include air, ground and
 * nonbuilding targets. Spell damage ignores armor (including shield armor),
 * while shield points still absorb the damage before HP does. */
static inline int sc64_psionic_storm_target_class(int is_air, int is_building)
{
    if (is_building) return 0;
    return (is_air ? SC64_STORM_TARGET_AIR : SC64_STORM_TARGET_GROUND) |
           SC64_STORM_TARGET_NONBUILDING;
}

/* Apply one Ignore Armor hit in Q8 units. Caller clears `hit_this_tick` at
 * each update start; the first storm hit sets it and overlapping storms in
 * that same update cannot damage this target again. Owner is intentionally
 * absent: native Storm splash can hit friendly units and the caster. */
static inline int sc64_psionic_storm_apply_hit(u8 target_class, int invincible,
                                                u8 *hit_this_tick,
                                                int *hp_q8, int *shields_q8)
{
    int damage = SC64_STORM_DAMAGE_PER_PULSE * SC64_STORM_ENERGY_ONE;
    if (!hit_this_tick || !hp_q8 || !shields_q8 || *hp_q8 <= 0 ||
        invincible || !(target_class & (SC64_STORM_TARGET_AIR | SC64_STORM_TARGET_GROUND)) ||
        (target_class & SC64_STORM_TARGET_NONBUILDING) == 0 || *hit_this_tick) return 0;
    *hit_this_tick = 1;
    if (*shields_q8 > 0) {
        int shield_hit = *shields_q8 < damage ? *shields_q8 : damage;
        *shields_q8 -= shield_hit;
        damage -= shield_hit;
    }
    if (damage > *hp_q8) damage = *hp_q8;
    *hp_q8 -= damage;
    return 1;
}

#endif
