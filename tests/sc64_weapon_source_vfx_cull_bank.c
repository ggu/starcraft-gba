/* SPDX-License-Identifier: GPL-3.0-or-later */
#define GBA_HOST 1
#define MAP_COLS 32
#define MAP_ROWS 32
static int camx, camy;
static int fog_mode;
static unsigned char visible_tiles[MAP_ROWS][MAP_COLS];
static int currently_visible(int x, int y);
#include "../starcraft/sc64_weapon_source_runtime.h"
#include "../starcraft/sc64_weapon_source_vfx_render.h"

#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

static uint8_t pixels[160][240];
static unsigned draw_calls;
static uint32_t baseline_draws;
static uint8_t saw_child_image, saw_lo_child;
static uint8_t saw_mirror_heading;
static const u8 *samples[65536];
static uint32_t sample_count;
static const u8 *current_sources[512];

static int currently_visible(int x, int y)
{
    if (x < 0 || x >= MAP_COLS * 16 || y < 0 || y >= MAP_ROWS * 16)
        return 0;
    return !fog_mode || visible_tiles[y / 16][x / 16];
}

static void paint(int x, int y, int w, int h, const u8 *src)
{
    for (int row = 0; row < h; ++row)
        for (int col = 0; col < w; ++col)
            if (x + col >= 0 && x + col < 240 && y + row >= 0 && y + row < 160)
                pixels[y + row][x + col] = src[row * w + col];
}

static void record_source(const u8 *src)
{
    if (sample_count < sizeof(samples) / sizeof(samples[0]))
        samples[sample_count++] = src;
}

void gfx_reverse_bytes(u8 *dst, const u8 *src, unsigned n)
{
    for (unsigned i = 0; i < n; ++i) dst[i] = src[n - 1u - i];
}
void gfx_blit_sprite(int x, int y, int w, int h, const u8 *src)
{ if (draw_calls < 512u) current_sources[draw_calls] = src; ++draw_calls; record_source(src); paint(x, y, w, h, src); }
void gfx_blit_sprite_mapped(int x, int y, int w, int h, const u8 *src,
                            const u8 *lookup, unsigned rows)
{ (void)lookup; (void)rows; if (draw_calls < 512u) current_sources[draw_calls] = src; ++draw_calls; record_source(src); paint(x, y, w, h, src); }

static int source_uses_frame(const u8 *src, uint16_t image_id)
{
    const Sc64WeaponSourceImage *image = sc64_weapon_source_image_lookup(image_id);
    if (!image) return 0;
    for (uint16_t i = 0; i < image->source_frame_count; ++i) {
        const Sc64WeaponSourceFrame *frame =
            sc64_weapon_source_frame_lookup(image_id, i);
        if (!frame) continue;
        const u8 *normal = sc64_weapon_source_frame_pixels(frame, 0);
        const u8 *mirror = sc64_weapon_source_frame_pixels(frame, 1);
        size_t bytes = (size_t)frame->w * frame->h;
        uintptr_t sample = (uintptr_t)src;
        uintptr_t n = (uintptr_t)normal, m = (uintptr_t)mirror;
        if (normal && sample >= n && sample - n < bytes) return 1;
        if (mirror && sample >= m && sample - m < bytes) return 1;
    }
    return 0;
}

static void classify_observed_sources(void)
{
    for (uint32_t s = 0; s < sample_count; ++s) {
        for (uint32_t i = 0; i < SC64_WEAPON_SOURCE_OVERLAY_COUNT; ++i) {
            const Sc64WeaponSourceOverlay *overlay = &sc64_weapon_source_overlays[i];
            if (!sc64_weapon_source_vfx_overlay_is_image(overlay->opcode)) continue;
            if (source_uses_frame(samples[s], overlay->image_id)) {
                saw_child_image = 1;
                if (overlay->opcode == 13u || overlay->opcode == 14u)
                    saw_lo_child = 1;
                break;
            }
        }
    }
}

static void draw_without_cull(uint32_t packed, int x0, int y0, int x1, int y1,
                              uint32_t age)
{
    sc64_weapon_source_vfx_draw_trace(packed, x0, y0, x1, y1, 1, age);
}

static void compare_cull_to_prepared_bank_pixels(
    uint32_t packed, int x0, int y0, int x1, int y1, uint32_t age)
{
    memset(pixels, 0, sizeof(pixels));
    draw_calls = 0;
    draw_without_cull(packed, x0, y0, x1, y1, age);
    Sc64WeaponSourceVfxKey key;
    uint8_t expected[sizeof(pixels)];
    memcpy(expected, pixels, sizeof(expected));
    unsigned expected_calls = draw_calls;
    baseline_draws += expected_calls != 0;
    if (sc64_weapon_source_vfx_unpack(packed, &key) &&
        key.direction > 16u && expected_calls) {
        const Sc64WeaponSourceAttackProfile *profile =
            sc64_weapon_source_trace_profile(packed);
        const Sc64WeaponSourceAnimation *actor = profile ?
            sc64_weapon_source_animation_lookup(profile->attack_image,
                                                 profile->attack_anim) : 0;
        const Sc64WeaponSourceImage *actor_image = profile ?
            sc64_weapon_source_image_lookup(profile->attack_image) : 0;
        int mirror = 0;
        if (actor && actor_image && actor_image->directional &&
            sc64_weapon_source_vfx_source_frame(actor_image, actor,
                sc64_weapon_source_vfx_source_tick(age), key.direction,
                &mirror) != UINT16_MAX && mirror)
            saw_mirror_heading = 1;
    }
    if (sc64_weapon_source_vfx_unpack(packed, &key) &&
        key.actor_raw == 30 && key.weapon_id == 27)
        for (unsigned i = 0; i < expected_calls && i < 512u; ++i)
            if (source_uses_frame(current_sources[i], 537)) {
                saw_lo_child = 1;
            }

    memset(pixels, 0, sizeof(pixels));
    draw_calls = 0;
    int cull = sc64_weapon_source_vfx_trace_provably_offscreen(
        packed, x0, y0, x1, y1, camx, camy);
    if (!cull) draw_without_cull(packed, x0, y0, x1, y1, age);
    assert(memcmp(expected, pixels, sizeof(expected)) == 0);
    assert(draw_calls == (cull ? 0u : expected_calls));
}

static uint32_t sample_ages(const Sc64WeaponSourceAttackProfile *profile,
                            int x0, int y0, int x1, int y1,
                            uint32_t ages[32])
{
    uint32_t n = 0;
    ages[n++] = 0;
    uint32_t release = sc64_weapon_source_trace_release_ticks(profile);
    uint32_t startup = sc64_weapon_source_trace_startup_ticks(profile);
    uint32_t flight = sc64_weapon_source_bullet_is_stationary(
        profile->weapon_bullet_type) ? 0u :
        sc64_weapon_source_trace_flight_ticks(profile, x0, y0, x1, y1);
    uint32_t death = sc64_weapon_source_animation_visual_ticks(
        sc64_weapon_source_animation_lookup(profile->projectile_image, 1u));
    uint32_t source_ticks[] = {release, release + 1u,
        release + startup, release + startup + 1u,
        release + startup + flight / 2u,
        release + startup + flight,
        release + startup + flight + death / 2u,
        release + startup + flight + death};
    for (unsigned i = 0; i < sizeof(source_ticks) / sizeof(source_ticks[0]); ++i) {
        uint32_t age = sc64_weapon_source_vfx_first_gba_age(source_ticks[i]);
        if (n < 32u) ages[n++] = age;
    }
    const Sc64WeaponSourceAnimation *actor =
        sc64_weapon_source_animation_lookup(profile->attack_image,
                                             profile->attack_anim);
    if (actor && actor->overlay_first <= SC64_WEAPON_SOURCE_OVERLAY_COUNT &&
        actor->overlay_count <= SC64_WEAPON_SOURCE_OVERLAY_COUNT - actor->overlay_first)
        for (uint16_t i = 0; i < actor->overlay_count && n < 32u; ++i) {
            const Sc64WeaponSourceOverlay *overlay =
                &sc64_weapon_source_overlays[actor->overlay_first + i];
            ages[n++] = sc64_weapon_source_vfx_first_gba_age(overlay->source_tick);
        }
    return n;
}

static void prepared_profiles_keep_pixel_output(void)
{
    uint32_t cull_count = 0, cases = 0;
    uint16_t radius_x, radius_y;
    uint32_t bounds = sc64_weapon_source_vfx_cached_bounds();
    assert(sc64_weapon_source_vfx_bounds_unpack(bounds, &radius_x, &radius_y));
    assert(radius_x == 66u && radius_y == 61u);
    static const int screen_endpoints[][4] = {
        {-180, 58, -125, 62}, {365, 58, 420, 62},
        {60, -150, 64, -110}, {60, 260, 64, 300},
        {-170, 60, 330, 70}, {330, 60, -170, 70},
        {-95, 20, 280, 110}, {280, 110, -95, 20},
        {-80, -80, -30, -40}, {300, 180, 350, 230},
        {100, 60, 130, 60}, {220, 60, 270, 60}
    };
    static const int cameras[][2] = {{0, 0}, {128, 64}, {256, 160}};
    for (uint32_t pi = 0; pi < SC64_WEAPON_SOURCE_PROFILE_COUNT; ++pi) {
        const Sc64WeaponSourceAttackProfile *p = &sc64_weapon_source_profiles[pi];
        if (!p->projectile_image) continue;
        for (uint8_t dir = 0; dir < 32; ++dir) {
            Sc64WeaponSourceVfxKey key = {
                p->actor_raw, 1, p->weapon_id, dir, p->mode, p->variant
            };
            uint32_t packed;
            if (!sc64_weapon_source_vfx_pack(key, &packed)) continue;
            for (unsigned c = 0; c < sizeof(cameras) / sizeof(cameras[0]); ++c) {
                camx = cameras[c][0]; camy = cameras[c][1];
                for (int fog = 0; fog < 2; ++fog) {
                    fog_mode = fog;
                    for (int ty = 0; ty < MAP_ROWS; ++ty)
                        for (int tx = 0; tx < MAP_COLS; ++tx)
                            visible_tiles[ty][tx] = (unsigned char)(
                                !fog || ((tx + ty) & 1u) == 0u);
                    for (unsigned e = 0;
                         e < sizeof(screen_endpoints) / sizeof(screen_endpoints[0]); ++e) {
                        int x0 = camx + screen_endpoints[e][0];
                        int y0 = camy - 12 + screen_endpoints[e][1];
                        int x1 = camx + screen_endpoints[e][2];
                        int y1 = camy - 12 + screen_endpoints[e][3];
                        uint32_t ages[32];
                        uint32_t age_count = sample_ages(p, x0, y0, x1, y1, ages);
                        for (uint32_t ai = 0; ai < age_count; ++ai) {
                            int culled = sc64_weapon_source_vfx_trace_provably_offscreen(
                                packed, x0, y0, x1, y1, camx, camy);
                            cull_count += culled != 0;
                            compare_cull_to_prepared_bank_pixels(
                                packed, x0, y0, x1, y1, ages[ai]);
                            ++cases;
                        }
                    }
                }
            }
        }
    }
    assert(cases > 10000);
    assert(cull_count > 0);
    assert(baseline_draws > 0);
    /* A follow trajectory is not proven by the endpoint segment and must stay. */
    for (uint32_t i = 0; i < SC64_WEAPON_SOURCE_PROFILE_COUNT; ++i) {
        const Sc64WeaponSourceAttackProfile *p = &sc64_weapon_source_profiles[i];
        if (p->weapon_bullet_type != SC64_SOURCE_BULLET_FOLLOW) continue;
        Sc64WeaponSourceVfxKey key = {p->actor_raw, 1, p->weapon_id, 0,
                                      p->mode, p->variant};
        uint32_t packed;
        if (!sc64_weapon_source_vfx_pack(key, &packed)) continue;
        assert(!sc64_weapon_source_vfx_trace_provably_offscreen(
            packed, 0, 0, 1000, 1000, 0, 0));
    }
    classify_observed_sources();
    assert(saw_child_image);
    assert(saw_lo_child);
    assert(saw_mirror_heading);
    printf("prepared_bank_cull_pass cases=%lu culls=%lu drawn=%lu sources=%lu radius=%ux%u child=%u lo_child=%u mirrored_heading=%u\n",
        (unsigned long)cases, (unsigned long)cull_count,
        (unsigned long)baseline_draws, (unsigned long)sample_count,
        (unsigned)radius_x, (unsigned)radius_y,
        (unsigned)saw_child_image, (unsigned)saw_lo_child,
        (unsigned)saw_mirror_heading);
}

int main(void)
{
    assert(sc64_weapon_source_vfx_anchor_may_be_visible(0, 0, 0, 0));
    assert(sc64_weapon_source_vfx_anchor_may_be_visible(239, 107, 0, 0));
    assert(!sc64_weapon_source_vfx_anchor_may_be_visible(240, 107, 0, 0));
    assert(!sc64_weapon_source_vfx_anchor_may_be_visible(239, 108, 0, 0));
    assert(sc64_weapon_source_vfx_anchor_may_be_visible(
        INT_MIN, INT_MIN, INT_MAX, INT_MAX)); /* wrap fails open */
    assert(!sc64_weapon_source_vfx_anchor_may_be_visible(
        INT_MAX, INT_MAX, INT_MIN, INT_MIN));
    assert(SC64_WEAPON_SOURCE_BANK_VERSION == 1);
    assert(sc64_weapon_source_vfx_motion_has_proven_bounds(
        SC64_SOURCE_BULLET_FLY));
    assert(sc64_weapon_source_vfx_motion_has_proven_bounds(
        SC64_SOURCE_BULLET_APPEAR_TARGET_UNIT));
    assert(sc64_weapon_source_vfx_motion_has_proven_bounds(
        SC64_SOURCE_BULLET_PERSIST_TARGET_POS));
    assert(sc64_weapon_source_vfx_motion_has_proven_bounds(
        SC64_SOURCE_BULLET_APPEAR_TARGET_POS));
    assert(sc64_weapon_source_vfx_motion_has_proven_bounds(
        SC64_SOURCE_BULLET_APPEAR_SOURCE));
    assert(sc64_weapon_source_vfx_motion_has_proven_bounds(
        SC64_SOURCE_BULLET_SELF_DESTRUCT));
    assert(sc64_weapon_source_vfx_motion_has_proven_bounds(
        SC64_SOURCE_BULLET_ATTACK_TARGET_POS));
    assert(!sc64_weapon_source_vfx_motion_has_proven_bounds(
        SC64_SOURCE_BULLET_FOLLOW));
    assert(!sc64_weapon_source_vfx_motion_has_proven_bounds(7u));
    assert(!sc64_weapon_source_vfx_motion_has_proven_bounds(
        SC64_SOURCE_BULLET_EXTEND_MAX_RANGE));
    assert(!sc64_weapon_source_vfx_motion_has_proven_bounds(255u));
    Sc64WeaponSourceAttackProfile launch_profile = {0};
    launch_profile.forward_offset = -75;
    launch_profile.upward_offset = -20;
    int launch_x, launch_y;
    assert(sc64_weapon_source_vfx_launch_origin_checked(&launch_profile, 8,
        100, 100, &launch_x, &launch_y));
    assert(launch_x == 63 && launch_y == 110);
    assert(sc64_weapon_source_vfx_launch_origin_checked(&launch_profile, 24,
        100, 100, &launch_x, &launch_y));
    assert(launch_x == 137 && launch_y == 110);
    launch_profile.forward_offset = 75;
    launch_profile.upward_offset = 20;
    assert(!sc64_weapon_source_vfx_launch_origin_checked(&launch_profile, 8,
        INT_MAX - 10, 100, &launch_x, &launch_y));
    launch_profile.forward_offset = -75;
    launch_profile.upward_offset = 20;
    assert(!sc64_weapon_source_vfx_launch_origin_checked(&launch_profile, 8,
        INT_MIN + 10, 100, &launch_x, &launch_y));
    launch_profile.forward_offset = 0;
    assert(!sc64_weapon_source_vfx_launch_origin_checked(&launch_profile, 0,
        100, INT_MIN + 5, &launch_x, &launch_y));
    launch_profile.upward_offset = -20;
    assert(!sc64_weapon_source_vfx_launch_origin_checked(&launch_profile, 0,
        100, INT_MAX - 5, &launch_x, &launch_y));
    Sc64WeaponSourceVfxKey missing_key = {255, 1, 191, 0, 0, 0};
    uint32_t missing_packed;
    assert(sc64_weapon_source_vfx_pack(missing_key, &missing_packed));
    assert(!sc64_weapon_source_trace_profile(missing_packed));
    assert(!sc64_weapon_source_vfx_trace_provably_offscreen(
        missing_packed, 0, 0, 4000, 4000, 0, 0));
    Sc64WeaponSourceVfxKey extreme_key = {30, 1, 27, 8, 0, 0};
    uint32_t extreme_packed;
    assert(sc64_weapon_source_vfx_pack(extreme_key, &extreme_packed));
    assert(!sc64_weapon_source_vfx_trace_provably_offscreen(
        extreme_packed, INT_MIN, INT_MIN, INT_MAX, INT_MAX, 0, 0));
    assert(!sc64_weapon_source_vfx_trace_provably_offscreen(
        extreme_packed, 0, 0, (int)UINT16_MAX + 1, 0, 0, 0));
    prepared_profiles_keep_pixel_output();
    return 0;
}
