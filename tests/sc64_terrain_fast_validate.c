/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Differential test for SC64_TERRAIN_FAST_VALIDATE. Both sides (see
 * tests/sc64_terrain_fast_validate_side.c) see the same worlds frame by
 * frame: moving, jumping and edge cameras, fog reveals and resets, minerals
 * depleting, buildings appearing, completing, dying and changing owner,
 * geysers, doodads and mission/map changes. Every frame they must agree on
 * the static key generation, the cache hit/miss decision, the dirty and
 * partial tile sets, every per-tile draw answer, the fog signatures and the
 * cache metadata. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sc64_terrain_fast_validate.h"

static FvWorld world;
static FvRecord ref, fast;
static FvModes modes;
static uint32_t rng = 0x2468aceu;

static uint32_t next(void)
{
    rng ^= rng << 13;
    rng ^= rng >> 17;
    rng ^= rng << 5;
    return rng;
}
static int below(int n) { return n > 0 ? (int)(next() % (uint32_t)n) : 0; }
static int chance(int percent) { return below(100) < percent; }

static const int map_sizes[][2] = { {64, 64}, {96, 64}, {128, 128}, {128, 96}, {12, 10}, {15, 7} };

static int near_camera_x(void) { return world.camx - 40 + below(320); }
static int near_camera_y(void) { return world.camy - 40 + below(190); }

static void reveal(int col, int row, int radius, int bits)
{
    for (int r = row - radius; r <= row + radius; ++r)
        for (int c = col - radius; c <= col + radius; ++c)
            if (c >= 0 && r >= 0 && c < world.cols && r < world.rows)
                world.fog[c + r * world.cols] = (uint8_t)bits;
}

static void new_map(void)
{
    int pick = below((int)(sizeof(map_sizes) / sizeof(map_sizes[0])));
    world.cols = map_sizes[pick][0];
    world.rows = map_sizes[pick][1];
    world.mission = below(4);
    world.race_mode = chance(20);
    memset(world.fog, 0, sizeof(world.fog));
    for (int i = 0; i < 6; ++i)
        reveal(below(world.cols), below(world.rows), 2 + below(6), chance(70) ? 3 : 2);
}

/* The map changes size but keeps its fog where the maps overlap: the old
 * edge cells stop (or start) being border cells under unchanged fog bits. */
static void resize_map(void)
{
    static uint8_t old_fog[FV_GRID_BYTES];
    int old_cols = world.cols, old_rows = world.rows;
    int pick = below((int)(sizeof(map_sizes) / sizeof(map_sizes[0])));
    memcpy(old_fog, world.fog, sizeof(old_fog));
    world.cols = map_sizes[pick][0];
    world.rows = map_sizes[pick][1];
    memset(world.fog, 0, sizeof(world.fog));
    for (int r = 0; r < world.rows && r < old_rows; ++r)
        for (int c = 0; c < world.cols && c < old_cols; ++c)
            world.fog[c + r * world.cols] = old_fog[c + r * old_cols];
}

static void move_camera(void)
{
    int max_x = world.cols * 16 - 240, max_y = world.rows * 16 - 108;
    int mode = below(100);
    if (mode < 35) return; /* still */
    if (mode < 75) {       /* scroll */
        int step = chance(50) ? 1 + below(8) : 16 * (1 + below(2));
        world.camx += chance(50) ? (chance(50) ? step : -step) : 0;
        world.camy += chance(50) ? (chance(50) ? step : -step) : 0;
    } else if (mode < 85) { /* jump */
        world.camx = below(max_x > 0 ? max_x + 1 : 1);
        world.camy = below(max_y > 0 ? max_y + 1 : 1);
    } else if (mode < 95) { /* edges */
        world.camx = chance(50) ? 0 : max_x;
        world.camy = chance(50) ? 0 : max_y;
    } else {               /* outside the cache's valid range */
        world.camx = chance(50) ? -1 - below(20) : max_x + 1 + below(20);
        world.camy = chance(50) ? -1 - below(20) : max_y + 1 + below(20);
    }
    if (chance(97)) { /* the game clamps; occasionally do not */
        if (world.camx > max_x) world.camx = max_x;
        if (world.camy > max_y) world.camy = max_y;
        if (world.camx < 0) world.camx = 0;
        if (world.camy < 0) world.camy = 0;
    }
}

static void change_fog(void)
{
    int col0 = world.camx / 16, row0 = world.camy / 16;
    int flips = chance(40) ? 0 : below(6);
    for (int i = 0; i < flips; ++i) {
        int col = col0 - 2 + below(20), row = row0 - 2 + below(12);
        if (col >= 0 && row >= 0 && col < world.cols && row < world.rows)
            world.fog[col + row * world.cols] = (uint8_t)below(4);
    }
    if (chance(4)) reveal(col0 + below(16), row0 + below(8), below(5), chance(50) ? 3 : 2);
    if (chance(3)) /* units leave: visible -> explored */
        for (int cell = 0; cell < world.cols * world.rows; ++cell)
            if (world.fog[cell] & 1) world.fog[cell] = 2;
    if (chance(1)) memset(world.fog, 0, sizeof(world.fog)); /* load/restart */
}

static void change_objects(void)
{
    for (int k = 0; k < 4; ++k) {
        int i = below(FV_MAX_MINERALS);
        switch (below(6)) {
        case 0: world.mineral_amount[i] = chance(70) ? 1 + below(1500) : 0; break;
        case 1: world.mineral_amount[i] -= 8 + below(400); break; /* depletes */
        case 2: world.mineral_x[i] = near_camera_x(); world.mineral_y[i] = near_camera_y(); break;
        case 3: world.mineral_style[i] = chance(90) ? below(4) : 40000 + below(4); break;
        default: break;
        }
    }
    for (int k = 0; k < 5; ++k) {
        int i = below(FV_MAX_BUILDINGS);
        switch (below(9)) {
        case 0: /* appears, often under construction */
            world.building_alive[i] = 1;
            world.building_construction[i] = chance(60);
            world.building_x[i] = near_camera_x();
            world.building_y[i] = near_camera_y();
            break;
        case 1: world.building_construction[i] = 0; break;  /* completes */
        case 2: world.building_alive[i] = 0; break;         /* dies */
        case 3: world.building_owner[i] = below(3); break;
        case 4: world.building_type[i] = below(40); break;
        case 5: world.building_raw[i] = chance(30) ? 173 + below(2) :
                (chance(95) ? below(200) : -40000 - below(9)); break;
        case 6: world.building_x[i] += below(9) - 4; world.building_y[i] += below(9) - 4; break;
        default: break;
        }
    }
    if (chance(5)) world.geyser_count = below(FV_MAX_GEYSERS + 1);
    if (chance(8)) {
        int i = below(FV_MAX_GEYSERS);
        world.geyser_x[i] = near_camera_x();
        world.geyser_y[i] = near_camera_y();
    }
    if (chance(3)) world.doodad[below(FV_DOODADS)][below(2)] = below(5);
}

static void probe_window(void)
{
    if (chance(50)) { /* exactly the draw window */
        world.probe_col0 = world.camx / 16;
        world.probe_row0 = world.camy / 16;
        world.probe_col1 = (world.camx + 240) / 16 + 1;
        world.probe_row1 = (world.camy + 108) / 16 + 1;
        if (world.probe_col1 > world.cols) world.probe_col1 = world.cols;
        if (world.probe_row1 > world.rows) world.probe_row1 = world.rows;
        if (world.probe_col0 < 0) world.probe_col0 = 0;
        if (world.probe_row0 < 0) world.probe_row0 = 0;
    } else {
        world.probe_col0 = world.camx / 16 - 2 + below(5);
        world.probe_row0 = world.camy / 16 - 2 + below(5);
        world.probe_col1 = world.probe_col0 + below(19);
        world.probe_row1 = world.probe_row0 + below(11);
    }
    /* Like draw_map(), stay on the map: state_view reads the cell itself. */
    if (world.probe_col0 < 0) world.probe_col0 = 0;
    if (world.probe_row0 < 0) world.probe_row0 = 0;
    if (world.probe_col1 > world.cols) world.probe_col1 = world.cols;
    if (world.probe_row1 > world.rows) world.probe_row1 = world.rows;
}

#define SAME(field) do { \
    if (memcmp(&ref.field, &fast.field, sizeof(ref.field)) != 0) { \
        fprintf(stderr, "frame %d: %s differs (cam %d,%d map %dx%d)\n", \
                frame, #field, world.camx, world.camy, world.cols, world.rows); \
        exit(1); \
    } \
} while (0)

int main(int argc, char **argv)
{
    int frames = argc > 1 ? atoi(argv[1]) : 30000;
    unsigned hits = 0, partial = 0, misses = 0, key_changes = 0, scroll_partial = 0;
    unsigned last_key = 0;
    int last_x = -999, last_y = -999;
    new_map();
    for (int i = 0; i < FV_MAX_MINERALS; ++i) {
        world.mineral_x[i] = below(world.cols * 16);
        world.mineral_y[i] = below(world.rows * 16);
        world.mineral_amount[i] = chance(60) ? 1500 : 0;
    }
    for (int i = 0; i < FV_MAX_BUILDINGS; ++i) {
        world.building_x[i] = below(world.cols * 16);
        world.building_y[i] = below(world.rows * 16);
        world.building_alive[i] = chance(50);
        world.building_owner[i] = below(3);
    }
    world.geyser_count = 4;
    for (int frame = 0; frame < frames; ++frame) {
        if (chance(1)) new_map();
        if (chance(2)) resize_map();
        move_camera();
        change_fog();
        if (chance(60)) change_objects();
        probe_window();
        fv_ref_frame(&world, &ref);
        fv_fast_frame(&world, &fast, &modes);
        SAME(key); SAME(hit); SAME(reused); SAME(valid); SAME(stored);
        SAME(mask_count); SAME(cache_camx); SAME(cache_camy);
        SAME(requests); SAME(hits); SAME(partial);
        SAME(clean); SAME(part); SAME(state); SAME(dirty); SAME(part_cells);
        SAME(mask); SAME(now); SAME(now_valid); SAME(now_col0); SAME(now_row0);
        SAME(now_cols); SAME(now_rows); SAME(fp);
        SAME(probe_clean); SAME(probe_part); SAME(probe_state);
#ifdef SC64_DIAGNOSTIC_TERRAIN
        SAME(diag);
#endif
        assert(memcmp(&ref, &fast, sizeof(ref)) == 0);
        if (ref.key != last_key) ++key_changes;
        last_key = ref.key;
        if (ref.hit) ++hits;
        else if (ref.reused) {
            ++partial;
            if (world.camx != last_x || world.camy != last_y) ++scroll_partial;
        } else ++misses;
        last_x = world.camx;
        last_y = world.camy;
    }
    printf("terrain fast validate: %d frames identical; hits %u, partial %u "
           "(scrolled %u), misses %u, key changes %u; fast windows indexed %u, "
           "call %u, none %u; probes indexed %u, call %u, none %u;\n",
           frames, hits, partial, scroll_partial, misses, key_changes,
           modes.indexed, modes.call, modes.none,
           modes.probe_indexed, modes.probe_call, modes.probe_none);
    /* The scenario must reach every decision. */
    assert(hits > 100 && partial > 100 && misses > 100 && key_changes > 100);
#ifdef SC64_TERRAIN_SCROLL
    assert(scroll_partial > 100);
#endif
    assert(modes.indexed > 100 && modes.none > 100);
    assert(modes.probe_indexed > 100 && modes.probe_call > 100);
    return 0;
}
