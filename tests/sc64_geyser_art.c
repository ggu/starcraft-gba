/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Source pixels remain private generated inputs, never fixture literals. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

static u8 expected[240 * 160], cached[240 * 160];

static void clear_objects(void)
{
    memset(u, 0, sizeof(u));
    memset(b, 0, sizeof(b));
    memset(m, 0, sizeof(m));
    memset(geysers, 0, sizeof(geysers));
    geyser_count = 0;
}

static void reveal(int value)
{
    for (int i = 0; i < MAP_CELLS; ++i) {
        SC64_GRID_SET(visible, i, value);
        SC64_GRID_SET(explored, i, 1);
    }
}

/* Independent scalar placement of a source crop onto a blank playfield. */
static unsigned expect_sprite(const Sc64SpriteFrame *s, int cx, int cy)
{
    assert(s && s->pixels && s->w && s->h);
    unsigned pixels = 0;
    int left = cx - s->canvas_w / 2 + s->offset_x;
    int top = cy - s->canvas_h / 2 + s->offset_y;
    for (int y = 0; y < s->h; ++y) for (int x = 0; x < s->w; ++x) {
        int dx = left + x, dy = top + y;
        u8 c = s->pixels[y * s->w + x];
        if (c && dx >= 0 && dx < 240 && dy >= 12 && dy < 120) {
            expected[dy * 240 + dx] = c;
            ++pixels;
        }
    }
    return pixels;
}

static void compare_pixels(void)
{
    for (int y = 0; y < 160; ++y) for (int x = 0; x < 240; ++x)
        assert(gfx_test_pixel(x, y) == expected[y * 240 + x]);
}

static void static_case(int x, int y, int is_visible, int gas_raw)
{
    clear_objects();
    reveal(is_visible);
    geyser_count = 1;
    geysers[0].x = camx + x;
    geysers[0].y = camy + y - 12;
    memset(expected, C_BLACK, sizeof(expected));
    if (is_visible) assert(expect_sprite(&sc64_race_sprite_geyser, x, y) > 0);
    if (gas_raw >= 0) {
        Building *q = &b[MAX_BUILDINGS - 1];
        q->alive = 1; q->owner = SIDE; q->raw_type = gas_raw;
        q->type = gas_raw == 110 ? B_REFINERY :
                  gas_raw == 149 ? B_EXTRACTOR : B_ASSIMILATOR;
        q->hp = q->maxhp = 750;
        q->x = geysers[0].x; q->y = geysers[0].y;
        assert(expect_sprite(sc64_race_unit_sprite(gas_raw), x, y) > 100);
    }
    gfx_begin(C_BLACK);
    gfx_set_clip_y(12, 120);
    draw_map_static_objects();
    gfx_end();
    compare_pixels();
}

static void compare_cache(void)
{
    game_render(); /* Refresh after the preceding mutation. */
    game_render(); /* Exercise a warm restore on the other framebuffer page. */
    for (int y = 0; y < 160; ++y) for (int x = 0; x < 240; ++x)
        cached[y * 240 + x] = gfx_test_pixel(x, y);
    sc64_terrain_cache_valid = 0;
    sc64_hud_valid = 0;
    game_render();
    for (int y = 0; y < 160; ++y) for (int x = 0; x < 240; ++x)
        assert(cached[y * 240 + x] == gfx_test_pixel(x, y));
}

int main(void)
{
    gba_init(); game_init();
    race_mode = 0; race_selected_mission = SC64_P5_GLOBAL_ID;
    race_human = R_PROTOSS; init_state();
    cx = WORLD_W / 2; cy = WORLD_H / 2;
    camx = 1024; camy = 1024;
    static_case(120, 66, 1, -1);
    const int edges[][2] = {{-10,66}, {245,66}, {120,8}, {120,125}};
    for (unsigned i = 0; i < sizeof(edges) / sizeof(edges[0]); ++i)
        static_case(edges[i][0], edges[i][1], 1, -1);
    static_case(120, 66, 0, -1);
    const int gas_raws[] = {110, 149, 157};
    for (unsigned i = 0; i < sizeof(gas_raws) / sizeof(gas_raws[0]); ++i)
        static_case(120, 66, 1, gas_raws[i]);

    clear_objects(); reveal(1);
    camx = viewport_left(); camy = viewport_top();
    geyser_count = 1;
    geysers[0].x = camx + 120; geysers[0].y = camy + 54;
    compare_cache();
    geysers[0].x = camx - 10; compare_cache();
    geysers[0].x = camx + 245; compare_cache();
    reveal(0); compare_cache();
    reveal(1); compare_cache();
    geyser_count = 0; compare_cache();
    puts("PASS source geyser pixels, edges, fog, three gas buildings and warm cache");
    return 0;
}
