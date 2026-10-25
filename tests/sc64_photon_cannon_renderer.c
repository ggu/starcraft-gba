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
    draw_building_overlay(&b[slot]);
    capture(after);
    /* Under-construction presentation must contain the actual cannon art,
       while construction effects and progress UI remain dynamic. */
    unsigned matches = compare_source_frame(after, idle, b[slot].x - camx,
                                             b[slot].y - camy + 12);
    assert(matches * 4 >= source_pixel_count(idle));
#ifdef SC64_STATIC_DIRTY_ROWS
    check_dirty_partial_with_dynamic_cannon(0);
    check_dirty_partial_with_dynamic_cannon(1);
#endif
    return 0;
}
