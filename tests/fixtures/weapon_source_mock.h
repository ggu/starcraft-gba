/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Hand-authored synthetic records for asset-free runtime tests; no ROM data. */
#ifndef TEST_SC64_WEAPON_SOURCE_GENERATED_H
#define TEST_SC64_WEAPON_SOURCE_GENERATED_H
#include <stdint.h>
#define SC64_WEAPON_SOURCE_BANK_VERSION 1
#define SC64_WEAPON_SOURCE_METADATA 1
#define SC64_WEAPON_SOURCE_EVENT_COUNT 1
#define SC64_WEAPON_SOURCE_OVERLAY_COUNT 1
typedef struct {
    uint8_t actor_raw, weapon_owner_raw, weapon_id, mode, variant;
    uint8_t attack_anim, projectile_ticks_per_frame, flags;
    uint8_t weapon_bullet_type, weapon_count;
    uint16_t attack_image, projectile_image, projectile_flingy;
    uint16_t projectile_lifetime, projectile_first_frame, projectile_frame_count;
    uint16_t attackwith_tick, impact_source_tick, attack_event_first, attack_event_count;
    uint16_t projectile_event_first, projectile_event_count, overlay_first, overlay_count;
    int32_t speed_raw, halt_raw;
    int16_t accel_raw;
    int8_t forward_offset, upward_offset;
} Sc64WeaponSourceAttackProfile;
typedef struct { uint16_t image_id, event_first, event_count,
    overlay_first, overlay_count; uint32_t duration_ticks, loop_pc;
    uint8_t animation_id, flags; } Sc64WeaponSourceAnimation;
typedef struct { uint16_t image_id, frame_first, frame_count, canvas_w, canvas_h;
    uint8_t directional, modifier, color_shift, lut_index, flags; } Sc64WeaponSourceImage;
typedef struct { uint16_t image_id; uint32_t source_tick, pc;
    uint16_t source_frame; uint8_t opcode, flags; } Sc64WeaponSourceEvent;
typedef struct { uint16_t parent_image, image_id; uint32_t source_tick, relative_tick;
    int16_t x, y; uint8_t lo_index, aux, opcode, flags; uint16_t sprite_id;
} Sc64WeaponSourceOverlay;
static Sc64WeaponSourceAttackProfile test_source_profile;
static Sc64WeaponSourceAnimation test_source_init, test_source_death;
static Sc64WeaponSourceImage test_source_image;
static Sc64WeaponSourceEvent sc64_weapon_source_events[1];
static Sc64WeaponSourceOverlay sc64_weapon_source_overlays[1];
static inline const Sc64WeaponSourceAttackProfile *
sc64_weapon_source_profile_lookup(uint8_t actor_raw, uint8_t weapon_id,
                                  uint8_t mode, uint8_t variant)
{
    return actor_raw == test_source_profile.actor_raw &&
           weapon_id == test_source_profile.weapon_id &&
           mode == test_source_profile.mode && variant == test_source_profile.variant
        ? &test_source_profile : 0;
}
static inline const Sc64WeaponSourceImage *
sc64_weapon_source_image_lookup(uint16_t image_id)
{
    return image_id == test_source_image.image_id ? &test_source_image : 0;
}
static inline const Sc64WeaponSourceAnimation *
sc64_weapon_source_animation_lookup(uint16_t image_id, uint8_t animation_id)
{
    if (image_id != test_source_profile.projectile_image) return 0;
    if (!animation_id) return test_source_init.duration_ticks ? &test_source_init : 0;
    if (animation_id == 1) return test_source_death.duration_ticks ? &test_source_death : 0;
    return 0;
}
#endif
