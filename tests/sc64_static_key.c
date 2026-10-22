/* Full-render oracle for culling irrelevant static-art cache-key payloads.
 * These are render fixtures; world changes do not claim gameplay completion. */
#define SC64_STATIC_KEY_CULL 1
#include "../starcraft/game.c"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static u8 cached[240 * 160];
static unsigned checks;

static void compare_full_render(void)
{
    game_render();
    for (int y = 0; y < 160; ++y)
        for (int x = 0; x < 240; ++x)
            cached[y * 240 + x] = gfx_test_pixel(x, y);
    sc64_terrain_cache_valid = sc64_hud_valid = 0;
    game_render();
    for (int y = 0; y < 160; ++y)
        for (int x = 0; x < 240; ++x)
            assert(cached[y * 240 + x] == gfx_test_pixel(x, y));
    ++checks;
}

static void reveal(int x, int y, int value)
{
    int cell = x / 16 + y / 16 * MAP_COLS;
    SC64_GRID_SET(visible, cell, value);
    SC64_GRID_SET(explored, cell, 1);
}

static void fixture(void)
{
    game_init();
    race_mode = 0;
    race_selected_mission = SC64_P3_GLOBAL_ID;
    race_human = R_PROTOSS;
    init_state();
    cx = WORLD_W / 2;
    cy = WORLD_H / 2;
    sc64_terrain_cache_valid = sc64_hud_valid = 0;
    compare_full_render();
}

static void test_minerals(void)
{
    int i = MAX_MINERALS - 1;
    m[i].x = cx + 24;
    m[i].y = cy + 8;
    m[i].amount = 1000;
    m[i].style = SC64_RAW_MINERAL_FIELD_1;
    reveal(m[i].x, m[i].y, 1);
    compare_full_render();
    m[i].style = SC64_RAW_MINERAL_FIELD_3;
    compare_full_render();
    m[i].x += 12;
    reveal(m[i].x, m[i].y, 1);
    compare_full_render();
    m[i].amount = 0;
    compare_full_render();
    m[i].amount = 500;
    compare_full_render();
    reveal(m[i].x, m[i].y, 0);
    compare_full_render();
    reveal(m[i].x, m[i].y, 1);
    compare_full_render();
    m[i].x = 8;
    m[i].y = 8;
    compare_full_render();
    unsigned key = sc64_static_objects_key();
    m[i].style = SC64_RAW_MINERAL_FIELD_2;
    m[i].x += 8;
    assert(sc64_static_objects_key() == key);
    compare_full_render();
}

static void test_buildings_and_raw_art(void)
{
    Building *q = &b[MAX_BUILDINGS - 1];
    memset(q, 0, sizeof(*q));
    q->alive = 1;
    q->owner = SIDE;
    q->type = B_PYLON;
    q->raw_type = sc64_race_raw_building(B_PYLON);
    q->hp = q->maxhp = 200;
    q->x = cx;
    q->y = cy;
    compare_full_render();
    /* Type conversion and retained raw art identity are separate inputs. */
    q->raw_type = sc64_race_raw_building(B_GATEWAY);
    compare_full_render();
    q->type = B_GATEWAY;
    compare_full_render();
    q->x += 9;
    q->y -= 5;
    compare_full_render();
    reveal(q->x, q->y, 0);
    q->owner = ENEMY;
    compare_full_render();
    reveal(q->x, q->y, 1);
    compare_full_render();
    q->alive = 0;
    compare_full_render();
    q->alive = 1;
    compare_full_render();
    q->x = q->y = 8;
    compare_full_render();
    unsigned key = sc64_static_objects_key();
    q->raw_type = sc64_race_raw_building(B_NEXUS);
    q->type = B_NEXUS;
    q->owner = SIDE;
    q->x += 8;
    assert(sc64_static_objects_key() == key);
    compare_full_render();
}

static void test_geysers_and_camera(void)
{
    geyser_count = 4;
    geysers[3].x = cx - 24;
    geysers[3].y = cy + 16;
    reveal(geysers[3].x, geysers[3].y, 1);
    compare_full_render();
    geysers[3].x += 9;
    reveal(geysers[3].x, geysers[3].y, 1);
    compare_full_render();
    geyser_count = 3;
    compare_full_render();
    const int offsets[][2] = {
        {-WORLD_W, -WORLD_H}, {WORLD_W, 0}, {0, WORLD_H},
        {-WORLD_W, 0}, {WORLD_W / 2, -WORLD_H / 2}, {1, 3}, {-5, -7}
    };
    for (unsigned i = 0; i < sizeof(offsets) / sizeof(offsets[0]); ++i) {
        cx = limit(cx + offsets[i][0], 0, WORLD_W - 1);
        cy = limit(cy + offsets[i][1], 0, WORLD_H - 1);
        compare_full_render();
    }
}

static void test_scrolling_layer_order(void)
{
    cx = WORLD_W / 2;
    cy = WORLD_H / 2;
    for (int i = 0; i < MAP_CELLS; ++i) {
        SC64_GRID_SET(visible, i, 1);
        SC64_GRID_SET(explored, i, 1);
    }
    Building *first = &b[MAX_BUILDINGS - 2];
    Building *last = &b[MAX_BUILDINGS - 1];
    memset(first, 0, sizeof(*first));
    first->alive = 1;
    first->owner = SIDE;
    first->type = B_NEXUS;
    first->raw_type = sc64_race_raw_building(B_NEXUS);
    first->hp = first->maxhp = 750;
    first->x = cx;
    first->y = cy;
    *last = *first;
    last->type = B_PYLON;
    last->raw_type = sc64_race_raw_building(B_PYLON);
    last->x += 12;
    last->y += 8;
    m[MAX_MINERALS - 1].x = cx + 8;
    m[MAX_MINERALS - 1].y = cy + 4;
    m[MAX_MINERALS - 1].amount = 500;
    geyser_count = 4;
    geysers[3].x = cx - 8;
    geysers[3].y = cy - 8;
    compare_full_render();
    const int steps[][2] = {
        {1, 0}, {0, 1}, {-2, -3}, {4, 2}, {16, 16}, {-17, 4},
        {-9, -21}, {121, 0}, {-123, 0}, {0, 56}, {0, -57}
    };
    for (unsigned round = 0; round < 3; ++round)
        for (unsigned i = 0; i < sizeof(steps) / sizeof(steps[0]); ++i) {
            cx += steps[i][0];
            cy += steps[i][1];
            compare_full_render();
            int cell = (viewport_left() + 32) / 16 +
                       (viewport_top() + 16) / 16 * MAP_COLS;
            SC64_GRID_SET(visible, cell, !SC64_GRID_GET(visible, cell));
            compare_full_render();
        }
}

int main(void)
{
    gba_init();
    fixture();
    test_minerals();
    test_buildings_and_raw_art();
    test_geysers_and_camera();
    test_scrolling_layer_order();
    printf("Culled static-art key matches full rendering across %u transitions\n", checks);
    return 0;
}
