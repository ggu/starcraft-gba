/* SPDX-License-Identifier: GPL-3.0-or-later
 * Private-input renderer check: no extracted pixels are stored in source. */
#include <assert.h>
#include "../starcraft/game.c"

static u8 before[240 * 160], after[240 * 160];

static void capture(u8 *out)
{
    for (int y = 0; y < 160; ++y)
        for (int x = 0; x < 240; ++x)
            out[y * 240 + x] = gfx_test_pixel(x, y);
}

static int cannon(int under_construction)
{
    game_init();
    race_mode = 1;
    race_human = R_PROTOSS;
    race_bots = 1;
    race_skirmish_init();
    screen = SCREEN_PLAY;
    assert(spawn_build(SIDE,B_PYLON,160,208,0)>=0);
    int slot = spawn_build(SIDE, B_PHOTON_CANNON, 160, 160,
                           under_construction);
    assert(slot >= 0);
    assert(race_site_powered(SIDE,160,160));
    b[slot].raw_type = 162;
    camx = 80;
    camy = 100;
    memset(visible, 0xff, sizeof(visible));
    return slot;
}

static unsigned compare_source_frame(const u8 *pixels, const Sc64SpriteFrame *s,
                                     int cx, int cy)
{
    int left = cx - s->canvas_w / 2 + s->offset_x;
    int top = cy - s->canvas_h / 2 + s->offset_y;
    unsigned matches = 0;
    for (int y = 0; y < s->h; ++y)
        for (int x = 0; x < s->w; ++x) {
            u8 pixel = s->pixels[y * s->w + x];
            int sx = left + x, sy = top + y;
            if (pixel && sx >= 0 && sx < 240 && sy >= 0 && sy < 160) {
                if (pixels[sy * 240 + sx] == pixel) ++matches;
            }
        }
    return matches;
}

static unsigned source_pixel_count(const Sc64SpriteFrame *s)
{
    unsigned count = 0;
    for (unsigned i = 0; i < (unsigned)s->w * s->h; ++i)
        count += s->pixels[i] != 0;
    return count;
}

#ifdef SC64_CONSTRUCTION_SOURCE_ART
static unsigned compare_warp_anchor_frame(const u8 *pixels,
                                          const u8 *destinations,
                                          const SC64ConstructionModifierFrame *frame,
                                          const u8 *lookup, unsigned rows,
                                          int cx, int cy,
                                          int ui_top, int ui_height,
                                          unsigned *expected_pixels)
{
    assert(frame && frame->strengths && lookup && rows == 40);
    int left = cx - frame->canvas_w / 2 + frame->x;
    int top = cy - frame->canvas_h / 2 + frame->y;
    unsigned expected = 0, matches = 0;
    for (int y = 0; y < frame->h; ++y)
        for (int x = 0; x < frame->w; ++x) {
            u8 strength = frame->strengths[y * frame->w + x];
            int sx = left + x, sy = top + y;
            if (!strength || strength > rows || sx < 0 || sx >= 240 ||
                sy < 12 || sy >= 120 ||
                (sy >= ui_top - 8 && sy < ui_top + 2) ||
                (sy >= ui_top + ui_height + 3 &&
                 sy < ui_top + ui_height + 5)) continue;
            ++expected;
            u8 wanted = lookup[(strength - 1u) * 256u +
                               destinations[sy * 240 + sx]];
            if (pixels[sy * 240 + sx] == wanted) ++matches;
        }
    if (expected_pixels) *expected_pixels = expected;
    return matches;
}
#endif

#ifdef SC64_STATIC_DIRTY_ROWS
static void check_dirty_partial_with_dynamic_cannon(int under_construction)
{
    int slot = cannon(under_construction);
    cx = 160;
    cy = 160;
    memset(explored, 0xff, sizeof(explored));
    sc64_terrain_cache_valid = 0;
    sc64_hud_valid = 0;
    gfx_begin(C_BLACK);
    game_render();

    int cell = (b[slot].x / 16) + 1 + (b[slot].y / 16) * MAP_COLS;
    assert(cell >= 0 && cell < MAP_CELLS);
    SC64_GRID_SET(visible, cell, 0);
    SC64_GRID_SET(explored, cell, 0);
    unsigned before_partial = sc64_terrain_partial;
    unsigned before_bands = sc64_static_dirty_rows_host_bands;
    gfx_begin(C_BLACK);
    game_render();
    capture(before);
    assert(sc64_terrain_partial > before_partial);
    assert(sc64_static_dirty_rows_host_bands > before_bands);

    sc64_terrain_cache_valid = 0;
    sc64_hud_valid = 0;
    gfx_begin(C_BLACK);
    game_render();
    capture(after);
    assert(memcmp(before, after, sizeof(before)) == 0);
}
#endif

int main(void)
{
    int slot = cannon(0);
    const Sc64SpriteFrame *idle = sc64_race_unit_sprite(162);
    assert(idle && idle->w && idle->h);
    const int queue_progress[] = {28, 24, 16, 0};
    const unsigned frames[] = {2, 1, 3, 0};
    for (unsigned i = 0; i < sizeof(frames) / sizeof(*frames); ++i) {
        b[slot].queue_progress = queue_progress[i];
        gfx_begin(C_BLACK);
        draw_building_art(&b[slot]);
        draw_building_overlay(&b[slot]);
        capture(before);
        const Sc64SpriteFrame *expected = sc64_race_building_sprite(162, frames[i]);
        assert(expected && expected->w && expected->h);
        unsigned matches = compare_source_frame(before, expected,
                            b[slot].x - camx, b[slot].y - camy + 12);
        assert(matches * 5 >= source_pixel_count(expected) * 4);
    }
    /* Power loss during the firing clock must show idle rather than leave
     * a permanent attack frame while defense cooldown is paused. */
    for (int i=0;i<MAX_BUILDINGS;++i)
        if (b[i].alive && b[i].type==B_PYLON) b[i].alive=0;
    assert(!race_site_powered(SIDE,b[slot].x,b[slot].y));
    b[slot].queue_progress=16;
    gfx_begin(C_BLACK);
    draw_building_art(&b[slot]); draw_building_overlay(&b[slot]); capture(after);
    assert(compare_source_frame(after,idle,b[slot].x-camx,b[slot].y-camy+12)*5>=
           source_pixel_count(idle)*4);

    slot = cannon(1);
    b[slot].build_progress = building_build_time(B_PHOTON_CANNON) / 2;
    gfx_begin(C_BLACK);
    draw_building_art(&b[slot]);
    capture(before);
    draw_building_overlay(&b[slot]);
    capture(after);
#ifdef SC64_CONSTRUCTION_SOURCE_ART
    /* The source Protoss build order installs WarpAnchor until the timer
       expires; compare the selected source frame while excluding UI bars. */
    unsigned source_stage, source_frame;
    uint32_t source_elapsed;
    assert(sc64_construction_source_phase(R_PROTOSS + 1,
        b[slot].build_progress,building_build_time(b[slot].type),
        &source_stage,&source_elapsed));
    const SC64ConstructionSourceStage *stage=
        sc64_construction_stage(162,source_stage);
    assert(stage && stage->image_id==211 && stage->layer_count==1);
    assert(sc64_construction_animation_frame(stage->animation,
                                              source_elapsed,&source_frame));
    const SC64ConstructionModifierFrame *warp_anchor=
        sc64_construction_modifier_frame(stage->image_id,
                                         (unsigned short)source_frame);
    unsigned lut_rows=0;
    const u8 *warp_lut=sc64_construction_modifier_lookup(stage->image_id,
                                                          &lut_rows);
    assert(warp_anchor && warp_anchor->strengths && warp_lut && lut_rows==40);
    const Sc64SpriteFrame *completed=mission_building_sprite(b[slot].type);
    assert(completed);
    int cx=b[slot].x-camx, cy=b[slot].y-camy+12;
    int ui_top=cy-completed->canvas_h/2+completed->offset_y;
    unsigned expected=0;
    unsigned matches=compare_warp_anchor_frame(after,before,warp_anchor,
        warp_lut,lut_rows,cx,cy,ui_top,completed->h,&expected);
    assert(expected>8 && matches==expected);
#else
    /* Diagnostic builds without source construction art retain the authored
       fallback expectation for the unfinished Cannon. */
    unsigned matches = compare_source_frame(after, idle, b[slot].x - camx,
                                             b[slot].y - camy + 12);
    assert(matches * 4 >= source_pixel_count(idle));
#endif
#ifdef SC64_STATIC_DIRTY_ROWS
    check_dirty_partial_with_dynamic_cannon(0);
    check_dirty_partial_with_dynamic_cannon(1);
#endif
    return 0;
}
