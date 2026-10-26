/* SPDX-License-Identifier: GPL-3.0-or-later */
#define GBA_HOST 1
#define MAP_COLS 32
#define MAP_ROWS 32
static int camx, camy;
static int currently_visible(int x, int y) { (void)x; (void)y; return 1; }
#include "../starcraft/sc64_weapon_source_runtime.h"
#include "../starcraft/sc64_weapon_source_vfx_render.h"
#ifdef SC64_UNIT_ATTACK_POSES_ENABLED
#include "../starcraft/sc64_unit_attack_source.h"
#endif

static unsigned draw_calls;
static int last_x, last_y;
static const u8 *draw_sources[4096];
static int draw_xs[4096];
void gfx_blit_sprite(int x, int y, int w, int h, const u8 *src)
{ (void)w; (void)h; if (draw_calls < 4096) { draw_sources[draw_calls] = src; draw_xs[draw_calls] = x; } ++draw_calls; last_x = x; last_y = y; }
void gfx_blit_sprite_mapped(int x, int y, int w, int h, const u8 *src,
                            const u8 *lookup, unsigned rows)
{ (void)w; (void)h; (void)lookup; (void)rows; if (draw_calls < 4096) { draw_sources[draw_calls] = src; draw_xs[draw_calls] = x; } ++draw_calls; last_x = x; last_y = y; }

static int saw_image(uint16_t image_id)
{
    const Sc64WeaponSourceImage *image = sc64_weapon_source_image_lookup(image_id);
    if (!image || image->frame_first + image->frame_count > SC64_WEAPON_SOURCE_FRAME_COUNT)
        return 0;
    for (unsigned c = 0; c < draw_calls && c < 4096; ++c) {
        uintptr_t sample = (uintptr_t)draw_sources[c];
        for (uint16_t i = 0; i < image->frame_count; ++i) {
            const Sc64WeaponSourceFrame *frame =
                &sc64_weapon_source_frames[image->frame_first + i];
            uintptr_t normal = (uintptr_t)sc64_weapon_source_frame_pixels(frame, 0);
            uintptr_t mirror = (uintptr_t)sc64_weapon_source_frame_pixels(frame, 1);
            size_t bytes = (size_t)frame->w * frame->h;
            if ((sample >= normal && sample - normal < bytes) ||
                (sample >= mirror && sample - mirror < bytes)) return 1;
        }
    }
    return 0;
}

static int image_min_x(uint16_t image_id, int *min_x)
{
    const Sc64WeaponSourceImage *image = sc64_weapon_source_image_lookup(image_id);
    int found = 0;
    if (!image || !min_x || image->frame_first + image->frame_count >
                              SC64_WEAPON_SOURCE_FRAME_COUNT) return 0;
    for (unsigned c = 0; c < draw_calls && c < 4096; ++c) {
        uintptr_t sample = (uintptr_t)draw_sources[c];
        for (uint16_t i = 0; i < image->frame_count; ++i) {
            const Sc64WeaponSourceFrame *frame =
                &sc64_weapon_source_frames[image->frame_first + i];
            uintptr_t normal = (uintptr_t)sc64_weapon_source_frame_pixels(frame, 0);
            uintptr_t mirror = (uintptr_t)sc64_weapon_source_frame_pixels(frame, 1);
            size_t bytes = (size_t)frame->w * frame->h;
            if ((sample >= normal && sample - normal < bytes) ||
                (sample >= mirror && sample - mirror < bytes)) {
                if (!found || draw_xs[c] < *min_x) *min_x = draw_xs[c];
                found = 1;
            }
        }
    }
    return found;
}

int main(void)
{
    const Sc64WeaponSourceAttackProfile *p =
        sc64_weapon_source_profile_lookup(5, 11, 0, 0);
    if (!p || !sc64_weapon_source_image_lookup(531u)) return 1;
    uint32_t packed;
    Sc64WeaponSourceVfxKey key = {5, 1, 11, 8, 0, 0};
    if (!sc64_weapon_source_vfx_pack(key, &packed)) return 2;
    uint32_t duration = sc64_weapon_source_trace_duration(p, 50, 50, 80, 50);
    if (!duration) return 3;
    draw_calls = 0;
    sc64_weapon_source_vfx_draw_trace(packed, 50, 50, 80, 50, 1, 0);
    if (saw_image(531) || saw_image(536)) return 23; /* before authored event */
    draw_calls = 0;
    sc64_weapon_source_vfx_draw_trace(packed, 50, 50, 80, 50, 1,
        sc64_weapon_source_vfx_first_gba_age(1));
    if (!saw_image(536) || saw_image(531)) return 24; /* pre-release muzzle */
    draw_calls = 0;
    sc64_weapon_source_vfx_draw_trace(packed, 50, 50, 80, 50, 1,
        sc64_weapon_source_vfx_first_gba_age(
            sc64_weapon_source_trace_release_ticks(p)));
    if (draw_calls < 2 || last_x < 40 || last_x > 100 || last_y < 12 || last_y >= 120)
        return 4;
    /* Tank's image536 muzzle child starts on attack-init tick 1 and remains
     * visible through tick 2, ahead of its attackwith release at tick 3. */
    const Sc64WeaponSourceAnimation *tank_attack =
        sc64_weapon_source_animation_lookup(251, 2);
    int saw_pre_release_muzzle = 0;
    if (tank_attack) for (uint16_t i = 0; i < tank_attack->overlay_count; ++i) {
        const Sc64WeaponSourceOverlay *o =
            &sc64_weapon_source_overlays[tank_attack->overlay_first + i];
        const Sc64WeaponSourceAnimation *child =
            sc64_weapon_source_animation_lookup(o->image_id, 0);
        if (o->image_id == 536 && o->source_tick == 1 && child &&
            child->duration_ticks == 2) saw_pre_release_muzzle = 1;
    }
    if (!saw_pre_release_muzzle || p->attackwith_tick != 3) return 15;
    const Sc64WeaponSourceAnimation *tank_death =
        sc64_weapon_source_animation_lookup(p->projectile_image, 1);
    uint32_t minimum_source_ticks = sc64_weapon_source_trace_release_ticks(p) +
        sc64_weapon_source_trace_startup_ticks(p) +
        sc64_weapon_source_trace_flight_ticks(p, 50, 50, 80, 50) +
        sc64_weapon_source_animation_visual_ticks(tank_death);
    if (duration < sc64_weapon_source_vfx_gba_duration(minimum_source_ticks))
        return 25;

    int16_t lo_x, lo_y;
    if (!sc64_weapon_source_lo_lookup(254, 2, 0, 0, &lo_x, &lo_y)) return 5;
    if (sc64_weapon_source_lo_lookup(254, 2, 102, 0, &lo_x, &lo_y)) return 6;
    draw_calls = 0;
    key = (Sc64WeaponSourceVfxKey){30, 1, 27, 8, 0, 0};
    if (!sc64_weapon_source_vfx_pack(key, &packed)) return 7;
    p = sc64_weapon_source_trace_profile(packed);
    if (!p) return 8;
    const Sc64WeaponSourceImage *siege_parent = sc64_weapon_source_image_lookup(254);
    const Sc64WeaponSourceAnimation *siege_anim =
        sc64_weapon_source_animation_lookup(254, 2);
    int parent_mirror = 0;
    uint16_t parent_frame = sc64_weapon_source_vfx_source_frame(
        siege_parent, siege_anim, 3, 8, &parent_mirror);
    if (parent_frame == UINT16_MAX ||
        !sc64_weapon_source_lo_lookup(254, 2, parent_frame, 0, &lo_x, &lo_y)) return 18;
    if (!sc64_weapon_source_frame_lookup(537, 8)) return 19;
    duration = sc64_weapon_source_trace_duration(p, 50, 50, 80, 50);
    if (!duration) return 9;
    sc64_weapon_source_vfx_draw_trace(packed, 50, 50, 80, 50, 1,
        sc64_weapon_source_vfx_first_gba_age(
            sc64_weapon_source_trace_release_ticks(p)));
    if (draw_calls < 2) return 10; /* projectile + LO-attached image537 */
    if (!saw_image(537)) return 16;

    draw_calls = 0;
    key = (Sc64WeaponSourceVfxKey){70, 1, 74, 8, 1, 0};
    if (!sc64_weapon_source_vfx_pack(key, &packed)) return 11;
    p = sc64_weapon_source_trace_profile(packed);
    if (!p || !p->projectile_image || p->projectile_image != 521) return 12;
    duration = sc64_weapon_source_trace_duration(p, 50, 50, 80, 50);
    if (duration < 4) return 13;
    sc64_weapon_source_vfx_draw_trace(packed, 50, 50, 80, 50, 1,
        sc64_weapon_source_vfx_first_gba_age(
            sc64_weapon_source_trace_release_ticks(p) + 4u));
    if (!saw_image(521) || !saw_image(443)) return 14;
    draw_calls = 0;
    uint32_t arrival_age = sc64_weapon_source_vfx_first_gba_age(
        sc64_weapon_source_trace_release_ticks(p) +
        sc64_weapon_source_trace_startup_ticks(p) +
        sc64_weapon_source_trace_flight_ticks(p, 50, 50, 80, 50));
    if (arrival_age >= duration) return 20;
    sc64_weapon_source_vfx_draw_trace(packed, 50, 50, 80, 50, 1, arrival_age);
    if (!saw_image(442)) return 21; /* authored Death impact child */

    /* Scout's looped flight script emits its trail at projectile-local tick
     * 2. Its birth position must use that local movement age, independent of
     * the actor release and projectile Init offsets. */
    p = sc64_weapon_source_profile_lookup(70, 74, 1, 0);
    if (!p) return 26;
    int launch_x, launch_y, expected_x, expected_y, trail_min_x;
    sc64_weapon_source_trace_launch_origin(p, 8, 50, 50, &launch_x, &launch_y);
    uint32_t init_ticks = sc64_weapon_source_trace_startup_ticks(p);
    uint32_t release_ticks = sc64_weapon_source_trace_release_ticks(p);
    if (!sc64_weapon_source_trace_position(p, launch_x, launch_y, 120, 50,
                                           2, &expected_x, &expected_y)) return 27;
    uint32_t trail_age = sc64_weapon_source_vfx_first_gba_age(
        release_ticks + init_ticks + 2u);
    key = (Sc64WeaponSourceVfxKey){70, 1, 74, 8, 1, 0};
    if (!sc64_weapon_source_vfx_pack(key, &packed)) return 28;
    draw_calls = 0;
    sc64_weapon_source_vfx_draw_trace(packed, 50, 50, 120, 50, 1, trail_age);
    const Sc64WeaponSourceImage *trail_image = sc64_weapon_source_image_lookup(443);
    const Sc64WeaponSourceFrame *trail_frame = sc64_weapon_source_frame_lookup(443, 0);
    if (!trail_image || !trail_frame || !image_min_x(443, &trail_min_x)) return 29;
    int expected_trail_x = expected_x - trail_image->canvas_w / 2 + trail_frame->x;
    if (trail_min_x != expected_trail_x) return 30;

    /* The projectile and its Death child have a finite authored lifetime.
     * A separately longer unit recovery pose may keep the tracer record alive,
     * but must not pin the last projectile/impact frame onscreen. */
    p = sc64_weapon_source_profile_lookup(8, 15, 1, 0);
    if (!p) return 31;
    init_ticks = sc64_weapon_source_trace_startup_ticks(p);
    int wraith_launch_x, wraith_launch_y;
    sc64_weapon_source_trace_launch_origin(p, 8, 50, 50,
                                            &wraith_launch_x, &wraith_launch_y);
    uint32_t flight_ticks = sc64_weapon_source_trace_flight_ticks(
        p, wraith_launch_x, wraith_launch_y, 80, 50);
    const Sc64WeaponSourceAnimation *wraith_death =
        sc64_weapon_source_animation_lookup(p->projectile_image, 1);
    uint32_t death_ticks = sc64_weapon_source_animation_visual_ticks(wraith_death);
    uint32_t source_effect_end = init_ticks + flight_ticks + death_ticks;
    if (!flight_ticks || !death_ticks) return 32;
    uint32_t wraith_effect_end_age = sc64_weapon_source_vfx_first_gba_age(
        sc64_weapon_source_trace_release_ticks(p) + source_effect_end);
    key = (Sc64WeaponSourceVfxKey){8, 1, 15, 8, 1, 0};
    if (!sc64_weapon_source_vfx_pack(key, &packed)) return 33;
    draw_calls = 0;
    sc64_weapon_source_vfx_draw_trace(packed, 50, 50, 80, 50, 1,
        sc64_weapon_source_vfx_first_gba_age(
            sc64_weapon_source_trace_release_ticks(p) + source_effect_end - 1u));
    if (!saw_image(p->projectile_image)) return 34; /* final finite impact span */
    draw_calls = 0;
    sc64_weapon_source_vfx_draw_trace(packed, 50, 50, 80, 50, 1,
        sc64_weapon_source_vfx_first_gba_age(
            sc64_weapon_source_trace_release_ticks(p) + source_effect_end));
    if (saw_image(p->projectile_image)) return 35;
#ifdef SC64_UNIT_ATTACK_POSES_ENABLED
    if (sc64_weapon_source_vfx_gba_duration(
            sc64_unit_attack_source_full_duration(p->actor_raw, p->mode)) <=
        wraith_effect_end_age) return 36;
#endif

    /* Siege image 254 attaches image 537 through a directional LO. Mirroring
     * the parent reverses the authored horizontal offset while preserving the
     * child sprite's own heading. */
    const Sc64WeaponSourceAnimation *siege_overlay_anim =
        sc64_weapon_source_animation_lookup(254, 2);
    int mirrored_lo_checked = 0;
    if (siege_overlay_anim) for (uint16_t parent_frame_index = 0;
                        parent_frame_index < 102 && !mirrored_lo_checked;
                        ++parent_frame_index) {
        int16_t lo_x, lo_y;
        if (!sc64_weapon_source_lo_lookup(254, 2, parent_frame_index, 0,
                                          &lo_x, &lo_y) || lo_x < 4) continue;
        draw_calls = 0;
        sc64_weapon_source_vfx_draw_overlays(siege_overlay_anim, 1, 1, 254,
            parent_frame_index, 100, 50, 8, 0, 0, 0, 0, 0, 0, 0, 0);
        int normal_min_x;
        if (!image_min_x(537, &normal_min_x)) continue;
        draw_calls = 0;
        sc64_weapon_source_vfx_draw_overlays(siege_overlay_anim, 1, 1, 254,
            parent_frame_index, 100, 50, 8, 1, 0, 0, 0, 0, 0, 0, 0);
        int mirror_min_x;
        if (!image_min_x(537, &mirror_min_x)) return 37;
        if (normal_min_x - mirror_min_x != lo_x / 2 - (-lo_x) / 2) return 38;
        mirrored_lo_checked = 1;
    }
    if (!mirrored_lo_checked) return 39;
    return 0;
}
