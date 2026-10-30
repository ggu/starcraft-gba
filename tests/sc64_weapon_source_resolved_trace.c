/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <stdint.h>
/* Hand-authored bank that forces a trace into a NULL active phase after Init.
 * The draw key aliases projectile id 100 to image 200; animation zero selects
 * frame 1, while the NULL overlay frame falls back to frame 0. */
#define GBA_HOST 1
#define MAP_COLS 32
#define MAP_ROWS 32
#define SC64_WEAPON_SOURCE_BANK_VERSION 1
#define SC64_WEAPON_SOURCE_METADATA 1
#define SC64_WEAPON_SOURCE_BANK_HEADER <stdint.h>
#define SC64_WEAPON_SOURCE_PROFILE_COUNT 1
#define SC64_WEAPON_SOURCE_IMAGE_COUNT 2
#define SC64_WEAPON_SOURCE_FRAME_COUNT 2
#define SC64_WEAPON_SOURCE_ANIMATION_COUNT 1
#define SC64_WEAPON_SOURCE_EVENT_COUNT 2
#define SC64_WEAPON_SOURCE_OVERLAY_COUNT 0
#define SC64_WEAPON_SOURCE_LUT_COUNT 1
#define SC64_WEAPON_SOURCE_LO_TABLE_COUNT 0
#define SC64_WEAPON_SOURCE_LO_OFFSET_COUNT 0
#define SC64_SOURCE_BULLET_FLY 0u

typedef struct {
    uint8_t actor_raw, weapon_owner_raw, weapon_id, mode, variant;
    uint8_t attack_anim, projectile_ticks_per_frame, flags;
    uint8_t weapon_bullet_type, weapon_count;
    uint16_t attackwith_tick, attack_image, projectile_image, projectile_flingy;
    uint16_t projectile_lifetime, projectile_first_frame, projectile_frame_count;
    uint16_t impact_source_tick, attack_event_first, attack_event_count;
    uint16_t projectile_event_first, projectile_event_count, overlay_first, overlay_count;
    int32_t speed_raw, halt_raw;
    int16_t accel_raw;
    int8_t forward_offset, upward_offset;
} Sc64WeaponSourceAttackProfile;
typedef struct {
    uint16_t image_id, event_first, event_count, overlay_first, overlay_count;
    uint32_t duration_ticks, loop_pc, loop_source_tick;
    uint8_t animation_id, flags;
} Sc64WeaponSourceAnimation;
typedef struct {
    uint16_t image_id, frame_first, frame_count, canvas_w, canvas_h;
    uint16_t source_frame_count, frame_directory_first;
    uint8_t directional, modifier, color_shift, lut_index, flags;
} Sc64WeaponSourceImage;
typedef struct {
    uint16_t image_id;
    uint32_t source_tick, pc;
    uint16_t source_frame;
    uint8_t opcode, flags;
} Sc64WeaponSourceEvent;
typedef struct {
    uint16_t parent_image, image_id;
    uint32_t source_tick;
    int32_t relative_tick;
    int16_t x, y;
    uint8_t lo_index, aux, opcode, flags;
    uint16_t sprite_id;
} Sc64WeaponSourceOverlay;
typedef struct {
    uint32_t pixel_offset, mirror_pixel_offset;
    int16_t x, y, mirror_x;
    uint16_t w, h, canvas_w, canvas_h, source_frame_id, image_id;
} Sc64WeaponSourceFrame;
typedef struct { uint32_t data_offset; uint16_t rows; uint8_t modifier, color_shift; } Sc64WeaponSourceLut;
typedef struct { uint16_t image_id, frame_count, offset_count; uint8_t lo_kind, flags; uint32_t offset_first; } Sc64WeaponSourceLoTable;
typedef struct { int8_t x, y; } Sc64WeaponSourceLoOffset;

static Sc64WeaponSourceAttackProfile test_profile;
static Sc64WeaponSourceImage test_request_image, test_keyed_image;
static Sc64WeaponSourceAnimation sc64_weapon_source_animations[1];
#define test_init sc64_weapon_source_animations[0]
static Sc64WeaponSourceEvent sc64_weapon_source_events[2];
static Sc64WeaponSourceOverlay sc64_weapon_source_overlays[1];
static const uint8_t test_pixels[4] = {11, 12, 77, 78};
static const Sc64WeaponSourceFrame sc64_weapon_source_frames[2] = {
    {0, UINT32_MAX, 0, 0, 0, 2, 1, 2, 1, 0, 200},
    {2, UINT32_MAX, 0, 0, 0, 2, 1, 2, 1, 1, 200}
};
static const uint16_t sc64_weapon_source_frame_directory[2] = {0, 1};
static const Sc64WeaponSourceLut sc64_weapon_source_luts[1] = {{0, 0, 0, 0}};
static const uint8_t sc64_weapon_source_lut_bytes[1] = {0};
static const Sc64WeaponSourceLoTable sc64_weapon_source_lo_tables[1] = {{0}};
static const Sc64WeaponSourceLoOffset sc64_weapon_source_lo_offsets[1] = {{0, 0}};

static inline const Sc64WeaponSourceAttackProfile *
sc64_weapon_source_profile_lookup(uint8_t actor, uint8_t weapon,
                                  uint8_t mode, uint8_t variant)
{
    return actor == test_profile.actor_raw && weapon == test_profile.weapon_id &&
           mode == test_profile.mode && variant == test_profile.variant ?
           &test_profile : 0;
}
static inline const Sc64WeaponSourceImage *
sc64_weapon_source_image_lookup(uint16_t id)
{
    return id == 100u ? &test_request_image :
           id == 200u ? &test_keyed_image : 0;
}
static inline const Sc64WeaponSourceAnimation *
sc64_weapon_source_animation_lookup(uint16_t id, uint8_t animation)
{
    return (id == 100u || id == 200u) && animation == 0u ? &test_init : 0;
}
static inline __attribute__((unused)) const Sc64WeaponSourceFrame *
sc64_weapon_source_frame_lookup(uint16_t id, uint16_t source_frame)
{
    if (id != 200u || source_frame > 1u) return 0;
    return &sc64_weapon_source_frames[source_frame];
}
static inline const uint8_t *
sc64_weapon_source_frame_pixels(const Sc64WeaponSourceFrame *frame, int mirror)
{
    if (!frame || mirror) return 0;
    return test_pixels + frame->pixel_offset;
}
static inline const Sc64WeaponSourceLut *
sc64_weapon_source_lut_for_image(uint16_t id)
{
    (void)id;
    return 0;
}
static inline int
sc64_weapon_source_lo_lookup(uint16_t image, uint8_t lo, uint16_t frame,
                             uint8_t offset, int16_t *x, int16_t *y)
{
    (void)image; (void)lo; (void)frame; (void)offset; (void)x; (void)y;
    (void)sc64_weapon_source_lo_tables;
    return 0;
}

static int camx, camy;
static int currently_visible(int x, int y) { (void)x; (void)y; return 1; }
#include "../starcraft/sc64_weapon_source_runtime.h"
#include "../starcraft/sc64_weapon_source_vfx_render.h"
#include <assert.h>

static const uint8_t *drawn[16];
static unsigned draw_count;
void gfx_reverse_bytes(u8 *dst, const u8 *src, unsigned n)
{ for (unsigned i = 0; i < n; ++i) dst[i] = src[n - i - 1u]; }
void gfx_blit_sprite(int x, int y, int w, int h, const u8 *src)
{ (void)x; (void)y; (void)w; (void)h; if (draw_count < 16) drawn[draw_count] = src; ++draw_count; }
void gfx_blit_sprite_mapped(int x, int y, int w, int h, const u8 *src,
                            const u8 *lut, unsigned rows)
{ (void)lut; (void)rows; gfx_blit_sprite(x, y, w, h, src); }

int main(void)
{
    test_profile = (Sc64WeaponSourceAttackProfile){
        .actor_raw = 5, .weapon_id = 11, .mode = 0, .variant = 0,
        .projectile_image = 100, .projectile_lifetime = 20,
        .projectile_frame_count = 1, .speed_raw = 5120,
        .weapon_bullet_type = SC64_SOURCE_BULLET_FLY
    };
    test_request_image = (Sc64WeaponSourceImage){
        .image_id = 200, .frame_first = 0, .frame_count = 1,
        .canvas_w = 2, .canvas_h = 1, .source_frame_count = 2,
        .modifier = 0
    };
    test_keyed_image = (Sc64WeaponSourceImage){
        .image_id = 200, .frame_first = 1, .frame_count = 1,
        .canvas_w = 2, .canvas_h = 1, .source_frame_count = 2,
        .modifier = 0
    };
    test_init = (Sc64WeaponSourceAnimation){
        .image_id = 200, .event_first = 0, .event_count = 2,
        .duration_ticks = 3, .loop_pc = UINT32_MAX, .animation_id = 0
    };
    sc64_weapon_source_events[0] = (Sc64WeaponSourceEvent){
        .image_id = 200, .source_tick = 0, .source_frame = 1, .opcode = 0
    };
    sc64_weapon_source_events[1] = (Sc64WeaponSourceEvent){
        .image_id = 200, .source_tick = 2, .source_frame = 1, .opcode = 36
    };
    assert(sc64_weapon_source_trace_startup_ticks(&test_profile) == 2);
    assert(sc64_weapon_source_trace_flight_ticks(&test_profile, 50, 50, 80, 50) == 3);

    uint32_t packed;
    Sc64WeaponSourceVfxKey key = {5, 1, 11, 8, 0, 0};
    assert(sc64_weapon_source_vfx_pack(key, &packed));
    uint32_t gba_age = sc64_weapon_source_vfx_first_gba_age(3);
    sc64_weapon_source_vfx_draw_trace(packed, 50, 50, 80, 50, 1, gba_age);
    assert(draw_count == 1);
    assert(drawn[0] == test_pixels + 2); /* anim0 frame, not NULL fallback */

    /* The requested projectile key 100 resolves to this deliberately invalid
     * alias record. Drawing must re-resolve image->image_id (200), whose
     * record has a valid modifier and distinct frame directory. */
    test_request_image.modifier = 11u;
    draw_count = 0;
    sc64_weapon_source_vfx_draw_trace(packed, 50, 50, 80, 50, 1, gba_age);
    assert(draw_count == 1);
    assert(drawn[0] == test_pixels + 2);
    return 0;
}