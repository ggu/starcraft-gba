/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_WEAPON_SOURCE_VFX_RENDER_H
#define SC64_WEAPON_SOURCE_VFX_RENDER_H

#include <stdint.h>
#include "../common/gba.h"

/* Compact view over one generated GRP frame. Generated source pixels are
 * cropped to this opaque rectangle; x/y place the crop in the full canvas. */
typedef struct {
    const uint8_t *pixels;
    const uint8_t *mirror_pixels;
    const uint8_t *lookup;
    uint16_t image_id;
    int16_t x, y, mirror_x;
    uint16_t width, height, canvas_w, canvas_h;
    uint8_t modifier, lookup_rows;
} Sc64WeaponSourceVfxRaster;

/* Project one already selected source frame. `world_x/y` is the source
 * image center in world pixels. `hflip` mirrors around the image canvas
 * center. Fog is checked per map tile, and the common sprite blitter applies
 * the active GBA scene clip. Modifier 0/1 pixels are already remapped GBA
 * palette indices; modifier 9 pixels are source strengths and modifier 10
 * pixels are opaque-mask selectors for the destination-dependent LUT.
 * Unknown modifiers deliberately draw nothing. */
static void
sc64_weapon_source_vfx_draw_raster(const Sc64WeaponSourceVfxRaster *r,
                                  int world_x, int world_y, int hflip)
{
    if (!r || !r->pixels || !r->width || !r->height ||
        r->width > 256 || r->height > 256 || r->canvas_w > 256 ||
        r->canvas_h > 256 || r->x + (int)r->width > r->canvas_w ||
        r->y + (int)r->height > r->canvas_h) return;
    if (r->modifier != 0 && r->modifier != 1 && r->modifier != 9 &&
        r->modifier != 10) return;
    if ((r->modifier == 9 || r->modifier == 10) && (!r->lookup || !r->lookup_rows)) return;
    if (hflip && r->mirror_x + (int)r->width > r->canvas_w) return;

    int use_mirror_pixels = hflip && r->mirror_pixels;
    int src_x = hflip ? (int)r->mirror_x : r->x;
    const uint8_t *frame_pixels = use_mirror_pixels ? r->mirror_pixels : r->pixels;
    int left = world_x - camx - (int)r->canvas_w / 2 + src_x;
    int top = world_y - camy + 12 - (int)r->canvas_h / 2 + (int)r->y;
    if (left >= 240 || left + r->width <= 0 || top >= 120 ||
        top + r->height <= 12) return;

    /* Fast path: when every map tile under the clipped raster is visible, the
     * tile loop below would draw each row whole, so draw it in one call. The
     * per-pixel reversed branch (no mirror pixels) keeps the tile loop. */
    if (!(hflip && !use_mirror_pixels)) {
        int sy0 = top < 12 ? 12 : top;
        int sy1 = top + (int)r->height > 120 ? 120 : top + (int)r->height;
        int wx0 = left + camx, wx1 = left + (int)r->width - 1 + camx;
        int wy0 = sy0 + camy - 12, wy1 = sy1 - 1 + camy - 12;
        int whole = wx0 >= 0 && wx1 < MAP_COLS * 16 && wy0 >= 0 && wy1 < MAP_ROWS * 16;
        for (int ty = wy0 >> 4; whole && ty <= wy1 >> 4; ++ty)
            for (int tx = wx0 >> 4; whole && tx <= wx1 >> 4; ++tx)
                whole = currently_visible(tx * 16, ty * 16);
        if (whole) {
            for (int sy = sy0; sy < sy1; ++sy) {
                const uint8_t *src = frame_pixels + (sy - top) * r->width;
                if (r->modifier == 9 || r->modifier == 10)
                    gfx_blit_sprite_mapped(left, sy, r->width, 1, src,
                                           r->lookup, r->lookup_rows);
                else
                    gfx_blit_sprite(left, sy, r->width, 1, src);
            }
            return;
        }
    }

    for (int row = 0; row < r->height; ++row) {
        int sy = top + row;
        if (sy < 12 || sy >= 120) continue;
        int col = 0;
        while (col < r->width) {
            int source_col = hflip && !use_mirror_pixels ? (int)r->width - 1 - col : col;
            int wx = left + col + camx;
            int wy = sy + camy - 12;
            int visible = wx >= 0 && wx < MAP_COLS * 16 && wy >= 0 &&
                          wy < MAP_ROWS * 16 && currently_visible(wx, wy);
            if (!visible) { ++col; continue; }

            if (hflip && !use_mirror_pixels) {
                const uint8_t *pixel = frame_pixels + row * r->width + source_col;
                if (r->modifier == 9 || r->modifier == 10)
                    gfx_blit_sprite_mapped(left + col, sy, 1, 1, pixel,
                                           r->lookup, r->lookup_rows);
                else
                    gfx_blit_sprite(left + col, sy, 1, 1, pixel);
                ++col;
                continue;
            }

            /* Bound each blit to one tile so unseen neighboring tiles never
             * leak through a partially visible projectile or impact. */
            int run = 16 - (wx & 15);
            if (run > (int)r->width - col) run = (int)r->width - col;
            /* Visibility is tile-wide and run ends at this tile's edge.
             * The starting coordinate was checked above; do not repeat a
             * visibility lookup for every pixel in the same tile. */
            int pixels = run;
            if (pixels) {
                const uint8_t *src = frame_pixels + row * r->width + col;
                if (r->modifier == 9 || r->modifier == 10)
                    gfx_blit_sprite_mapped(left + col, sy, pixels, 1, src,
                                           r->lookup, r->lookup_rows);
                else
                    gfx_blit_sprite(left + col, sy, pixels, 1, src);
                col += pixels;
            } else {
                ++col;
            }
        }
    }
}

#ifdef SC64_WEAPON_SOURCE_BANK_VERSION
#include "sc64_weapon_source_runtime.h"

static inline uint16_t
sc64_weapon_source_vfx_animation_frame(const Sc64WeaponSourceAnimation *anim,
                                       uint32_t local_tick,
                                       uint16_t fallback)
{
    uint16_t result = fallback;
    if (!anim) return result;
    if (anim->event_first > SC64_WEAPON_SOURCE_EVENT_COUNT ||
        anim->event_count > SC64_WEAPON_SOURCE_EVENT_COUNT - anim->event_first)
        return result;
    uint32_t bounded = local_tick;
    if (anim->loop_pc != UINT32_MAX &&
        anim->loop_source_tick < anim->duration_ticks &&
        bounded >= anim->loop_source_tick) {
        uint32_t loop_length = anim->duration_ticks - anim->loop_source_tick;
        if (loop_length) bounded = anim->loop_source_tick +
                                      (bounded - anim->loop_source_tick) % loop_length;
    }
    if (anim->duration_ticks && bounded >= anim->duration_ticks)
        bounded = anim->duration_ticks - 1u;
    for (uint32_t n = 0; n < anim->event_count; ++n) {
        const Sc64WeaponSourceEvent *event =
            &sc64_weapon_source_events[anim->event_first + n];
        if (event->opcode == 0u && event->source_tick <= bounded)
            result = event->source_frame;
    }
    return result;
}

static inline uint16_t
sc64_weapon_source_vfx_source_frame(const Sc64WeaponSourceImage *image,
                                   const Sc64WeaponSourceAnimation *anim,
                                   uint32_t source_age, uint8_t direction,
                                   int *mirror)
{
    if (!image || !mirror || !image->source_frame_count ||
        image->frame_first >= SC64_WEAPON_SOURCE_FRAME_COUNT) return UINT16_MAX;
    uint16_t fallback = image->frame_count ?
        sc64_weapon_source_frames[image->frame_first].source_frame_id : 0;
    uint16_t frame = sc64_weapon_source_vfx_animation_frame(anim, source_age, fallback);
    *mirror = 0;
    if (image->directional &&
        !sc64_weapon_source_vfx_direction_frame(frame, 17, direction, &frame, mirror))
        return UINT16_MAX;
    return frame;
}

static inline void
sc64_weapon_source_vfx_draw_image(uint16_t image_id, uint8_t animation_id,
                                  uint32_t source_age, uint8_t direction,
                                  int world_x, int world_y)
{
    const Sc64WeaponSourceImage *image = sc64_weapon_source_image_lookup(image_id);
    if (!image || !image->frame_count ||
        image->frame_first >= SC64_WEAPON_SOURCE_FRAME_COUNT ||
        image->modifier > 10u) return;
    const Sc64WeaponSourceAnimation *anim =
        sc64_weapon_source_animation_lookup(image_id, animation_id);
    int mirror = 0;
    uint16_t source_frame = sc64_weapon_source_vfx_source_frame(
        image, anim, source_age, direction, &mirror);
    if (source_frame == UINT16_MAX) return;
    const Sc64WeaponSourceFrame *frame =
        sc64_weapon_source_frame_lookup(image_id, source_frame);
    if (!frame) return;

    Sc64WeaponSourceVfxRaster raster = {0};
    raster.pixels = sc64_weapon_source_frame_pixels(frame, 0);
    raster.mirror_pixels = sc64_weapon_source_frame_pixels(frame, 1);
    raster.image_id = image_id;
    raster.x = frame->x;
    raster.y = frame->y;
    raster.mirror_x = frame->mirror_x;
    raster.width = frame->w;
    raster.height = frame->h;
    raster.canvas_w = frame->canvas_w;
    raster.canvas_h = frame->canvas_h;
    raster.modifier = image->modifier;
    const Sc64WeaponSourceLut *lut = sc64_weapon_source_lut_for_image(image_id);
    if (lut && lut->data_offset < sizeof(sc64_weapon_source_lut_bytes)) {
        raster.lookup = sc64_weapon_source_lut_bytes + lut->data_offset;
        raster.lookup_rows = (uint8_t)(lut->rows > 255 ? 255 : lut->rows);
    }
    sc64_weapon_source_vfx_draw_raster(&raster, world_x, world_y, mirror);
}

static inline int
sc64_weapon_source_vfx_overlay_is_image(uint8_t opcode)
{
    return opcode == 8u || opcode == 9u || opcode == 10u || opcode == 11u ||
           opcode == 13u || opcode == 14u || opcode == 15u || opcode == 16u ||
           opcode == 17u || opcode == 19u || opcode == 20u || opcode == 21u;
}

static inline int
sc64_weapon_source_vfx_overlay_is_sprite(uint8_t opcode)
{
    return opcode == 15u || opcode == 16u || opcode == 17u || opcode == 19u ||
           opcode == 20u || opcode == 21u;
}

/* Draw attached image overlays and independently spawned sprite trails from
 * the generated event stream. A looping SPROL stream keeps at most eight live
 * occurrences; each sprite is placed at its authored birth position. */
static inline void
sc64_weapon_source_vfx_draw_overlays(const Sc64WeaponSourceAnimation *anim,
                                     uint32_t timeline_tick,
                                     uint32_t child_clock,
                                     uint16_t parent_image,
                                     uint16_t parent_frame,
                                     int world_x, int world_y,
                                     uint8_t direction,
                                     int parent_mirror,
                                     int relative_to_release,
                                     const Sc64WeaponSourceAttackProfile *motion,
                                     int x0, int y0, int x1, int y1,
                                     uint32_t stage_start)
{
    if (!anim || anim->overlay_first > SC64_WEAPON_SOURCE_OVERLAY_COUNT ||
        anim->overlay_count > SC64_WEAPON_SOURCE_OVERLAY_COUNT - anim->overlay_first)
        return;
    for (uint16_t i = 0; i < anim->overlay_count; ++i) {
        const Sc64WeaponSourceOverlay *overlay =
            &sc64_weapon_source_overlays[anim->overlay_first + i];
        if (overlay->parent_image != parent_image ||
            !sc64_weapon_source_vfx_overlay_is_image(overlay->opcode)) continue;
        int64_t base_trigger = relative_to_release ? overlay->relative_tick :
                                                    (int64_t)overlay->source_tick;
        uint32_t loop_len = anim->loop_pc != UINT32_MAX &&
                            overlay->source_tick >= anim->loop_source_tick &&
                            anim->duration_ticks > anim->loop_source_tick ?
            anim->duration_ticks - anim->loop_source_tick : 0;
        if ((int64_t)timeline_tick < base_trigger) continue;
        uint32_t last_iteration = loop_len ?
            (uint32_t)(((int64_t)timeline_tick - base_trigger) / loop_len) : 0;
        int sprite = sc64_weapon_source_vfx_overlay_is_sprite(overlay->opcode);
        uint32_t first_iteration = sprite && last_iteration >= 7u ?
                                   last_iteration - 7u : 0u;
        int source_dx = overlay->x, source_dy = overlay->y;
        if (overlay->opcode == 13u || overlay->opcode == 14u) {
            int16_t lo_dx, lo_dy;
            if (!sc64_weapon_source_lo_lookup(parent_image, overlay->lo_index,
                                              parent_frame, overlay->aux,
                                              &lo_dx, &lo_dy)) continue;
            source_dx = lo_dx;
            source_dy = lo_dy;
        }
        if (parent_mirror) source_dx = -source_dx;
        const Sc64WeaponSourceAnimation *child_init =
            sc64_weapon_source_animation_lookup(overlay->image_id, 0u);
        uint32_t child_life = child_init ? child_init->duration_ticks : 1u;
        if (child_init && child_init->loop_pc != UINT32_MAX) child_life = UINT32_MAX;
        if (!child_life) child_life = 1u;
        for (uint32_t iteration = first_iteration; iteration <= last_iteration; ++iteration) {
            int64_t trigger = base_trigger + (int64_t)iteration * loop_len;
            int64_t child_age_signed = (int64_t)child_clock - trigger;
            if (child_age_signed < 0 || child_age_signed >= child_life) continue;
            uint32_t child_age = (uint32_t)child_age_signed;
            int draw_x = world_x, draw_y = world_y;
            if (sprite && motion) {
                int born_x, born_y;
                int64_t moving_age = (int64_t)stage_start + trigger;
                if (moving_age < 0) moving_age = 0;
                if (moving_age > UINT32_MAX ||
                    !sc64_weapon_source_trace_position(motion, x0, y0, x1, y1,
                        (uint32_t)moving_age, &born_x, &born_y)) continue;
                draw_x = born_x;
                draw_y = born_y;
            }
            sc64_weapon_source_vfx_draw_image(overlay->image_id, 0u, child_age,
                direction, draw_x + source_dx / 2, draw_y + source_dy / 2);
        }
    }
}

/* Draw the projectile stream from its generated Init/attack/Death image
 * timelines. This is presentation-only: it uses the packed tracer identity
 * and captured endpoints, and never changes simulation or saved fields. */
static inline void
sc64_weapon_source_vfx_draw_trace_ex(uint32_t packed,
                                     int x0, int y0, int x1, int y1, int live,
                                     uint32_t gba_age)
{
    Sc64WeaponSourceVfxKey key;
    if (!live || !sc64_weapon_source_vfx_unpack(packed, &key)) return;
    const Sc64WeaponSourceAttackProfile *p =
        sc64_weapon_source_trace_profile(packed);
    if (!p || !p->projectile_image) return;
    int launch_x, launch_y;
    sc64_weapon_source_trace_launch_origin(p, key.direction, x0, y0,
                                          &launch_x, &launch_y);
    const Sc64WeaponSourceImage *image =
        sc64_weapon_source_image_lookup(p->projectile_image);
    if (!image || !image->source_frame_count ||
        image->frame_first >= SC64_WEAPON_SOURCE_FRAME_COUNT) return;

    if (!sc64_weapon_source_trace_duration(p, launch_x, launch_y, x1, y1)) return;
    uint32_t attack_age = sc64_weapon_source_vfx_source_tick(gba_age);
    uint32_t release_ticks = sc64_weapon_source_trace_release_ticks(p);
    /* The captured tracer begins at attack-init. Keep the attacker animation
     * on its absolute source clock, then start the projectile at attackwith. */
    const Sc64WeaponSourceAnimation *actor_anim =
        sc64_weapon_source_animation_lookup(p->attack_image, p->attack_anim);
    const Sc64WeaponSourceImage *actor_image =
        sc64_weapon_source_image_lookup(p->attack_image);
    if (actor_anim && actor_image && p->attackwith_tick != UINT16_MAX) {
        int actor_mirror = 0;
        uint16_t actor_frame = sc64_weapon_source_vfx_source_frame(
            actor_image, actor_anim, attack_age, key.direction, &actor_mirror);
        (void)actor_mirror;
        sc64_weapon_source_vfx_draw_overlays(actor_anim,
            attack_age, attack_age, p->attack_image, actor_frame, x0, y0,
            key.direction, actor_mirror, 0, 0, x0, y0, x1, y1, 0);
    }
    if (attack_age < release_ticks) return;
    uint32_t source_age = attack_age - release_ticks;
    uint32_t init_ticks = sc64_weapon_source_trace_startup_ticks(p);
    const Sc64WeaponSourceAnimation *init =
        sc64_weapon_source_animation_lookup(image->image_id, 0u);
    int moving = !sc64_weapon_source_bullet_is_stationary(p->weapon_bullet_type);
    uint32_t flight_ticks = moving ?
        sc64_weapon_source_trace_flight_ticks(p, launch_x, launch_y, x1, y1) : 0;
    const Sc64WeaponSourceAnimation *flight_anim =
        sc64_weapon_source_animation_lookup(image->image_id, 2u);
    const Sc64WeaponSourceAnimation *death =
        sc64_weapon_source_animation_lookup(image->image_id, 1u);
    uint32_t death_ticks = death ?
        sc64_weapon_source_animation_visual_ticks(death) : 0u;
    uint64_t projectile_visual_end = (uint64_t)init_ticks +
        (moving ? flight_ticks : 0u) + death_ticks;
    if (projectile_visual_end > UINT32_MAX) projectile_visual_end = UINT32_MAX;
    /* The tracer can outlive its projectile to finish the actor recovery
     * pose. Stop the projectile and impact children at their own finite end. */
    if (source_age >= (uint32_t)projectile_visual_end) return;

    const Sc64WeaponSourceAnimation *active = init;
    uint32_t local_tick = source_age;
    if (source_age >= init_ticks) {
        uint32_t after_init = source_age - init_ticks;
        if (moving && after_init < flight_ticks) {
            active = flight_anim;
            local_tick = after_init;
        } else {
            active = death;
            local_tick = moving ? after_init - flight_ticks : after_init;
        }
    }
    int px, py;
    uint32_t projectile_age = source_age > init_ticks ? source_age - init_ticks : 0;
    if (!sc64_weapon_source_trace_position(p, launch_x, launch_y, x1, y1,
                                           projectile_age, &px, &py)) return;
    uint8_t active_id = active ? active->animation_id : 0u;
    int projectile_mirror = 0;
    uint16_t projectile_frame = sc64_weapon_source_vfx_source_frame(
        image, active, local_tick, key.direction, &projectile_mirror);
    (void)projectile_mirror;
    sc64_weapon_source_vfx_draw_image(image->image_id, active_id, local_tick,
                                       key.direction, px, py);

    /* Child images from each missile script remain independent while their
     * own Init animation is alive. This covers projectile trails and impact
     * sparks as well as ordinary IMGOL/SPROL records. */
    if (init && source_age < init_ticks)
        sc64_weapon_source_vfx_draw_overlays(init, source_age, source_age,
            image->image_id, projectile_frame, px, py, key.direction,
            projectile_mirror, 0, 0, launch_x, launch_y, x1, y1, 0);
    if (flight_anim && source_age >= init_ticks && projectile_age < flight_ticks)
        sc64_weapon_source_vfx_draw_overlays(flight_anim, projectile_age,
            projectile_age, image->image_id, projectile_frame, px, py,
            key.direction, projectile_mirror, 0, p, launch_x, launch_y, x1, y1, 0);
    if (death && source_age >= init_ticks + (moving ? flight_ticks : 0u)) {
        uint32_t death_age = source_age - init_ticks - (moving ? flight_ticks : 0u);
        sc64_weapon_source_vfx_draw_overlays(death, death_age, death_age,
            image->image_id, projectile_frame, px, py, key.direction,
            projectile_mirror, 0, p, launch_x, launch_y, x1, y1,
            moving ? flight_ticks : 0u);
    }

}

static inline void
sc64_weapon_source_vfx_draw_trace(uint32_t packed,
                                  int x0, int y0, int x1, int y1, int live,
                                  uint32_t gba_age)
{
    sc64_weapon_source_vfx_draw_trace_ex(packed, x0, y0, x1, y1, live, gba_age);
}
#endif

#endif
