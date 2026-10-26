/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Production WarpAnchor compositor fixture; requires prepared private art. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "gba.h"
#include "../starcraft/game.c"
#include "gba.c"

static u8 before_pixels[240 * 160];

static void set_map_clip(void)
{
#ifdef GBA_SCENE_CLIP
    gfx_set_clip_y(12, 120);
#endif
}
static uint32_t run_warp_frame(Building *building, unsigned duration,
                               int progress, int capture)
{
    int saved_progress = building->build_progress;
    building->build_progress = progress;
    unsigned stage_index, source_frame;
    uint32_t source_elapsed;
    int center_x = building->x - camx;
    int center_y = building->y - camy + 12;
    assert(sc64_construction_source_phase(3, progress, (int)duration,
                                          &stage_index, &source_elapsed));
    int raw_id = building->raw_type >= 0 ? building->raw_type :
                 sc64_race_raw_building(building->type);
    const SC64ConstructionSourceStage *stage =
        sc64_construction_stage(raw_id, stage_index);
    assert(stage && stage->image_id == 211 && stage->layer_count == 1);
    assert(sc64_construction_animation_frame(stage->animation, source_elapsed,
                                              &source_frame));
    const SC64ConstructionModifierFrame *modifier_frame =
        sc64_construction_modifier_frame(211, (unsigned short)source_frame);
    assert(modifier_frame && modifier_frame->strengths);
    unsigned rows = 0;
    const unsigned char *lookup = sc64_construction_modifier_lookup(211, &rows);
    assert(lookup && rows == 40);
    for (unsigned y = 0; y < modifier_frame->h; ++y)
        for (unsigned x = 0; x < modifier_frame->w; ++x) {
            unsigned value = modifier_frame->strengths[y * modifier_frame->w + x];
            assert(value == 0 || (value >= 32 && value <= 40));
        }

    /* Test against a full-range destination pattern so every GBA destination
       slot participates; this is separate from the flat-background capture. */
    gfx_begin(C_BLACK);
    set_map_clip();
    for (int y = 0; y < 160; ++y)
        for (int x = 0; x < 240; ++x)
            gfx_pixel(x, y, (u8)((x * 29 + y * 47 + x * y) % 256));
    for (int y = 0; y < 160; ++y)
        for (int x = 0; x < 240; ++x)
            before_pixels[y * 240 + x] = gfx_test_pixel(x, y);

    Building snapshot = *building;
    draw_building_overlay(building);
    assert(!memcmp(&snapshot, building, sizeof(snapshot)));
    unsigned expected = 0, blue = 0;
    uint32_t visual_hash = 2166136261u;
    int left = center_x - modifier_frame->canvas_w / 2 + modifier_frame->x;
    int top = center_y - modifier_frame->canvas_h / 2 + modifier_frame->y;
    int vital_top = top - 4;
    for (unsigned y = 0; y < modifier_frame->h; ++y)
        for (unsigned x = 0; x < modifier_frame->w; ++x) {
            unsigned strength = modifier_frame->strengths[y * modifier_frame->w + x];
            int px = left + (int)x, py = top + (int)y;
            if (!strength || px < 0 || px >= 240 || py < 12 || py >= 120 ||
                (py >= vital_top && py < vital_top + 3)) continue;
            u8 actual = gfx_test_pixel(px, py);
            if (strength) {
                assert(strength <= rows);
                u8 wanted = lookup[(strength - 1u) * 256u + before_pixels[py * 240 + px]];
                assert(actual == wanted);
                ++expected;
            }
            visual_hash = (visual_hash ^ actual) * 16777619u;
            unsigned r = host_palette[actual] & 31u;
            unsigned g = (host_palette[actual] >> 5) & 31u;
            unsigned b = (host_palette[actual] >> 10) & 31u;
            if (b > r && b > g) ++blue;
        }
    assert(expected > 100);
    assert(blue > 20);
    printf("BLUE progress=%d blue_pixels=%u expected_pixels=%u\n", progress, blue, expected);

    if (capture) {
        /* Uniform imported terrain-green palette index, captured from the
           exact same production draw_building_overlay call. */
        gfx_begin(C_GRASS);
        set_map_clip();
        draw_building_overlay(building);
        unsigned capture_blue = 0, capture_pixels = 0;
        for (unsigned yy = 0; yy < modifier_frame->h; ++yy)
            for (unsigned xx = 0; xx < modifier_frame->w; ++xx) {
                unsigned strength = modifier_frame->strengths[yy * modifier_frame->w + xx];
                int px = left + (int)xx, py = top + (int)yy;
                if (!strength || px < 0 || px >= 240 || py < 12 || py >= 120 ||
                    (py >= vital_top && py < vital_top + 3)) continue;
                u16 color = host_palette[gfx_test_pixel(px, py)];
                unsigned r = color & 31u, g = (color >> 5) & 31u,
                         b = (color >> 10) & 31u;
                ++capture_pixels;
                if (b > r && b > g) ++capture_blue;
            }
        assert(capture_pixels > 100 && capture_blue > 30);
        printf("CAPTURE background=C_GRASS blue_pixels=%u source_pixels=%u\n",
               capture_blue, capture_pixels);
        gfx_end();

    }
    building->build_progress = saved_progress;
    printf("FRAME progress=%d source_elapsed=%lu image_frame=%u source_pixels=%u\n",
           progress, (unsigned long)source_elapsed, source_frame, expected);
    printf("VISUAL_HASH=%08lx\n", (unsigned long)visual_hash);
    return visual_hash;
}

static void check_modifier_map_clip(Building *building, unsigned duration)
{
    int saved_progress = building->build_progress;
    int saved_camy = camy;
    building->build_progress = (int)(duration / 2u);
    camy = building->y; /* The building anchor is at map row 12. */
    unsigned stage_index, source_frame;
    uint32_t elapsed;
    assert(sc64_construction_source_phase(3, building->build_progress,
                                          (int)duration, &stage_index, &elapsed));
    int raw = building->raw_type >= 0 ? building->raw_type :
              sc64_race_raw_building(building->type);
    const SC64ConstructionSourceStage *stage = sc64_construction_stage(raw, stage_index);
    assert(stage && stage->image_id == 211);
    assert(sc64_construction_animation_frame(stage->animation, elapsed, &source_frame));
    const SC64ConstructionModifierFrame *frame =
        sc64_construction_modifier_frame(211, (unsigned short)source_frame);
    assert(frame && frame->strengths);

    gfx_begin(C_BLACK);
    set_map_clip();
    for (int y = 0; y < 160; ++y)
        for (int x = 0; x < 240; ++x)
            gfx_pixel(x, y, (u8)(32 + ((x * 17 + y * 31) % 224)));
    for (int y = 0; y < 160; ++y)
        for (int x = 0; x < 240; ++x)
            before_pixels[y * 240 + x] = gfx_test_pixel(x, y);
    draw_building_overlay(building);
    int left = building->x - camx - frame->canvas_w / 2 + frame->x;
    int top = building->y - camy + 12 - frame->canvas_h / 2 + frame->y;
    int vital_top = top - 4;
    unsigned checked = 0;
    for (unsigned y = 0; y < frame->h; ++y)
        for (unsigned x = 0; x < frame->w; ++x) {
            if (!frame->strengths[y * frame->w + x]) continue;
            int px = left + (int)x, py = top + (int)y;
            if (px < 0 || px >= 240 || py < 0 || py >= 12 ||
                (py >= vital_top && py < vital_top + 3)) continue;
            assert(gfx_test_pixel(px, py) == before_pixels[py * 240 + px]);
            ++checked;
        }
    assert(checked > 0);
    printf("PASS modifier art stays out of HUD rows: %u clipped pixels\n", checked);
    building->build_progress = saved_progress;
    camy = saved_camy;
}

int main(void)
{
    assert(SC64_CONSTRUCTION_COLOR_MODIFIERS_VERSION == 1u);
    assert(sc64_construction_image_modifier(211) == 9u);
    assert(sc64_construction_image_color_shift(211) == 3u);
    assert(sc64_construction_image_modifier(210) == 0u);
    unsigned rows = 0;
    const unsigned char *lut = sc64_construction_modifier_lookup(211, &rows);
    assert(lut && rows == 40u);
    for (unsigned frame = 0; frame < 14; ++frame) {
        const SC64ConstructionModifierFrame *f =
            sc64_construction_modifier_frame(211, (unsigned short)frame);
        assert(f && f->w && f->h && f->canvas_w == 76 && f->canvas_h == 76);
    }

    gba_init();
    game_init();
    race_mode = 1; race_human = R_PROTOSS; race_bots = 1; race_skirmish_map = 0;
    race_skirmish_init(); screen = SCREEN_PLAY;
    minerals = 3000; gas = 1000;
    const int site_x = 320, site_y = 320;
    camx = site_x - 100; camy = site_y - 53;
    memset(visible, 0xff, sizeof(visible));
    int worker = spawn_unit(SIDE, U_PROBE, site_x + 32, site_y);
    assert(worker >= 0 && u[worker].alive);
    assert(race_start_build(SIDE, worker, B_PYLON, site_x, site_y));
    int slot = u[worker].target;
    assert(slot >= 0 && b[slot].alive && b[slot].under_construction);
    unsigned duration = (unsigned)building_build_time(b[slot].type);
    assert(duration > 16);
    uint32_t first = run_warp_frame(&b[slot], duration, 1, 1);
    uint32_t mid = run_warp_frame(&b[slot], duration, (int)(duration / 2u), 2);
    uint32_t late = run_warp_frame(&b[slot], duration, (int)(duration - 1u), 3);
    assert(first != mid && mid != late && first != late);
    check_modifier_map_clip(&b[slot], duration);
    printf("PASS WarpAnchor modifier9 compositor; raw=156 source=211 frames=14 LUT=40x256\n");
    return 0;
}
