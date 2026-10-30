/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_WEAPON_SOURCE_VFX_H
#define SC64_WEAPON_SOURCE_VFX_H

#include <stdint.h>

/* Source identity fits in the existing Tracer.color int. Bits 8..15 are
 * 64+WeaponType id; bits 16..23 are attacker raw; the high byte is slot+1
 * (zero means no unit slot). The low byte carries logical heading, attack
 * mode and source variant. Generic authored styles use values below 16. */
#define SC64_SOURCE_VFX_STYLE_BASE 64u
#define SC64_SOURCE_VFX_MAX_WEAPON_ID 191u
#define SC64_SOURCE_VFX_STYLE_MIN SC64_SOURCE_VFX_STYLE_BASE
#define SC64_SOURCE_VFX_STYLE_MAX \
    (SC64_SOURCE_VFX_STYLE_BASE + SC64_SOURCE_VFX_MAX_WEAPON_ID)
#define SC64_SOURCE_VFX_VARIANT_MAX 3u
#define SC64_SOURCE_VFX_MAX_SLOT_PLUS1 240u
#define SC64_SOURCE_BULLET_APPEAR_AT_TARGET 2u

typedef struct {
    uint8_t actor_raw;
    uint8_t actor_slot_plus1;
    uint8_t weapon_id;
    uint8_t direction; /* logical 32-way heading, not a GRP frame index */
    uint8_t target_air;
    uint8_t variant;
} Sc64WeaponSourceVfxKey;

static inline int
sc64_weapon_source_vfx_pack(Sc64WeaponSourceVfxKey key, uint32_t *packed)
{
    if (!packed || key.weapon_id > SC64_SOURCE_VFX_MAX_WEAPON_ID ||
        key.direction > 31 || key.target_air > 1 ||
        key.variant > SC64_SOURCE_VFX_VARIANT_MAX ||
        key.actor_slot_plus1 > SC64_SOURCE_VFX_MAX_SLOT_PLUS1) return 0;
    *packed = ((uint32_t)key.actor_slot_plus1 << 24) |
              ((uint32_t)key.actor_raw << 16) |
              ((uint32_t)(SC64_SOURCE_VFX_STYLE_BASE + key.weapon_id) << 8) |
              ((uint32_t)key.variant << 6) |
              ((uint32_t)key.target_air << 5) | key.direction;
    return 1;
}

static inline int
sc64_weapon_source_vfx_is_tagged(uint32_t packed)
{
    uint32_t style = (packed >> 8) & 0xffu;
    return style >= SC64_SOURCE_VFX_STYLE_MIN &&
           style <= SC64_SOURCE_VFX_STYLE_MAX &&
           (packed >> 24) <= SC64_SOURCE_VFX_MAX_SLOT_PLUS1;
}

static inline int
sc64_weapon_source_vfx_unpack(uint32_t packed, Sc64WeaponSourceVfxKey *key)
{
    if (!key || !sc64_weapon_source_vfx_is_tagged(packed)) return 0;
    key->actor_raw = (uint8_t)(packed >> 16);
    key->actor_slot_plus1 = (uint8_t)(packed >> 24);
    key->weapon_id = (uint8_t)(((packed >> 8) & 0xffu) - SC64_SOURCE_VFX_STYLE_BASE);
    key->direction = (uint8_t)(packed & 31u);
    key->target_air = (uint8_t)((packed >> 5) & 1u);
    key->variant = (uint8_t)((packed >> 6) & 3u);
    return 1;
}

/* BW source presentation runs at 24 Hz; the GBA simulation advances at
 * 30 Hz. Age zero is the attack Init frame. This maps a GBA age to the
 * source tick already reached; the inverse gives the first GBA age at which
 * a source-tick event is visible. */
static inline uint32_t
sc64_weapon_source_vfx_source_tick(uint32_t gba_age)
{
    /* floor(4*n/5) = n - ceil(n/5), with no overflowing product. */
    uint32_t fifth = gba_age / 5u;
    return gba_age - fifth - (gba_age - fifth * 5u != 0u);
}

static inline uint32_t
sc64_weapon_source_vfx_first_gba_age(uint32_t source_tick)
{
    /* ceil(5*n/4) = n + ceil(n/4); saturate before the final sum. */
    uint32_t extra = (source_tick >> 2) + ((source_tick & 3u) != 0u);
    return source_tick > UINT32_MAX - extra ? UINT32_MAX : source_tick + extra;
}

/* Convert a source lifetime in 24 Hz updates to the number of 30 Hz GBA
 * updates needed to reach that event. This is duration math, not an extra
 * update: source wait N spans exactly N updates. */
static inline uint32_t
sc64_weapon_source_vfx_gba_duration(uint32_t source_ticks)
{
    return sc64_weapon_source_vfx_first_gba_age(source_ticks);
}

/* Sample a finite frame run expressed in source ticks. `first_frame` can be
 * a directional bank's base (for example pose*17 + heading_frame). Return
 * zero before the event or after the authored frames have ended. */
static inline int
sc64_weapon_source_vfx_sample_frames(uint32_t source_tick,
                                    uint32_t event_tick,
                                    uint32_t ticks_per_frame,
                                    uint8_t first_frame,
                                    uint8_t frame_count,
                                    uint8_t *frame)
{
    uint32_t index;
    if (!frame || !frame_count || !ticks_per_frame || source_tick < event_tick)
        return 0;
    index = (source_tick - event_tick) / ticks_per_frame;
    if (index >= frame_count || (uint32_t)first_frame + index > UINT8_MAX)
        return 0;
    *frame = (uint8_t)(first_frame + index);
    return 1;
}

/* Directional BW GRPs are stored in 17-way banks. Headings 17..31 mirror
 * their 15..1 counterparts; keep the logical heading in the key and fold
 * only when selecting a source frame. */
static inline int
sc64_weapon_source_vfx_direction_frame(uint16_t first_frame,
                                      uint16_t frame_count,
                                      uint8_t direction,
                                      uint16_t *frame,
                                      int *hflip)
{
    uint16_t folded;
    if (!frame || !hflip || direction > 31 || !frame_count) return 0;
    folded = direction <= 16 ? direction : (uint16_t)(32 - direction);
    if (folded >= frame_count || first_frame > UINT16_MAX - folded) return 0;
    *frame = (uint16_t)(first_frame + folded);
    *hflip = direction > 16;
    return 1;
}

/* Bounded linear preview for a flying projectile between the release
 * position and the captured target position. It is a presentation sampler;
 * native Flingy acceleration/turning and damage timing remain separate. */
static inline int
sc64_weapon_source_vfx_linear_sample(int x0, int y0, int x1, int y1,
                                     uint32_t source_tick,
                                     uint32_t flight_ticks,
                                     int *x, int *y)
{
    if (!x || !y || !flight_ticks || flight_ticks > UINT16_MAX ||
        source_tick > flight_ticks) return 0;
    *x = (int)((int64_t)x0 + ((int64_t)x1 - x0) * source_tick / flight_ticks);
    *y = (int)((int64_t)y0 + ((int64_t)y1 - y0) * source_tick / flight_ticks);
    return 1;
}

/* Source bullet behavior 2 appears at its target on creation, so it must not
 * receive a fake travel path. Other behaviors use a bounded linear display
 * preview; source Flingy acceleration/turning remains a separate concern. */
static inline int
sc64_weapon_source_vfx_projectile_position(uint8_t bullet_type,
                                           int x0, int y0, int x1, int y1,
                                           uint32_t source_age,
                                           uint32_t source_lifetime,
                                           int *x, int *y)
{
    if (bullet_type == SC64_SOURCE_BULLET_APPEAR_AT_TARGET) {
        if (!x || !y) return 0;
        *x = x1;
        *y = y1;
        return 1;
    }
    return sc64_weapon_source_vfx_linear_sample(x0, y0, x1, y1,
                                                 source_age, source_lifetime,
                                                 x, y);
}

#endif
