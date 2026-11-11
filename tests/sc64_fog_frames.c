/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Real rendered frames for the soft-fog comparison. Build the same source
 * twice for one product (once with and once without -DSC64_SOFT_FOG), run each
 * with the same scenario, and compare the two dumps with
 * tests/test_sc64_fog_frames.py. The simulation is identical in both builds;
 * only the terrain presentation differs.
 *
 * usage: sc64_fog_frames <out.bin> <scenario>
 *   c0, c1   Custom Game on map 0 or 1 (race products)
 *   m0       first campaign mission of the product's campaign
 *
 * Each frame is a record of the 240x160 indexed screen, the camera, the
 * fog class of every map cell (0 unknown, 1 explored, 2 visible) and the
 * palette. The game runs from its normal start; the explored grid is then
 * widened to a deterministic shape so that every frame contains unknown,
 * explored-but-hidden and visible terrain (state injection, not natural
 * play: the shape is stated in the report). */
#include "../common/gba.c"
#include "../starcraft/game.c"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static u16 prior_held;

static void input(u16 held)
{
    u16 pressed = (u16)(held & (u16)~prior_held);
    game_update(held, pressed);
    prior_held = held;
}
#define TAP(k) do { input((k)); input(0); } while (0)

static void find_base(int *bx, int *by)
{
    *bx = WORLD_W / 2; *by = WORLD_H / 2;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == SIDE) { *bx = b[i].x; *by = b[i].y; return; }
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE) { *bx = u[i].x; *by = u[i].y; return; }
}

static int cell_class(int col, int row)
{
    int cell = col + row * MAP_COLS;
    if (SC64_GRID_GET(visible, cell)) return 2;
    return SC64_GRID_GET(explored, cell) ? 1 : 0;
}

static void write_u32(FILE *f, u32 v) { fwrite(&v, 4, 1, f); }

static void dump_frame(FILE *f)
{
    u8 pixels[240 * 160];
    u8 state[MAP_COLS * MAP_ROWS];
    u16 palette[256];
    for (int y = 0; y < 160; ++y)
        for (int x = 0; x < 240; ++x) pixels[y * 240 + x] = gfx_test_pixel(x, y);
    for (int row = 0; row < MAP_ROWS; ++row)
        for (int col = 0; col < MAP_COLS; ++col) state[col + row * MAP_COLS] = (u8)cell_class(col, row);
    for (int i = 0; i < 256; ++i) palette[i] = host_palette[i];
    write_u32(f, (u32)camx); write_u32(f, (u32)camy);
    write_u32(f, MAP_COLS); write_u32(f, MAP_ROWS);
    fwrite(pixels, 1, sizeof(pixels), f);
    fwrite(state, 1, sizeof(state), f);
    fwrite(palette, 2, 256, f);
}

/* Explored shape: a disc around the base with a notch, plus an island to the
 * south-east and a one-cell-wide corridor that joins them. */
static void widen_explored(int bx, int by)
{
    int bc = bx / 16, br = by / 16;
    for (int row = 0; row < MAP_ROWS; ++row)
        for (int col = 0; col < MAP_COLS; ++col) {
            int dx = col - bc, dy = row - br;
            int disc = dx * dx + dy * dy <= 13 * 13 && !(dx > 3 && dy > -2 && dy < 2);
            int island = (col - (bc + 17)) * (col - (bc + 17)) + (row - (br + 6)) * (row - (br + 6)) <= 25;
            int corridor = row == br + 3 && col >= bc && col <= bc + 17;
            if (disc || island || corridor) SC64_GRID_SET(explored, col + row * MAP_COLS, 1);
        }
}

int main(int argc, char **argv)
{
    if (argc != 3) { fprintf(stderr, "usage: %s out.bin scenario\n", argv[0]); return 2; }
    game_init();
#ifdef SC64_RACES
    if (argv[2][0] == 'c') {
        race_mode = 1; race_human = R_ZERG; race_bots = 1;
        race_skirmish_map = argv[2][1] - '0';
        race_skirmish_init();
        screen = SCREEN_PLAY;
    } else {
        TAP(KEY_A); TAP(KEY_A); TAP(KEY_START);
    }
#else
    (void)argv;
    screen = SCREEN_PLAY;
#endif
    if (screen != SCREEN_PLAY) { fprintf(stderr, "scenario did not reach play\n"); return 3; }
    for (int i = 0; i < 40; ++i) input(0);
    int bx, by;
    find_base(&bx, &by);
    widen_explored(bx, by);
    static const int offsets[][2] = {{0, 0}, {144, 64}, {-160, -96}, {0, 176}, {288, 32}};
    FILE *f = fopen(argv[1], "wb");
    if (!f) { perror(argv[1]); return 2; }
    fwrite("FOGF", 1, 4, f);
    write_u32(f, (u32)(sizeof(offsets) / sizeof(offsets[0])));
    for (unsigned k = 0; k < sizeof(offsets) / sizeof(offsets[0]); ++k) {
        cx = limit(bx + offsets[k][0], 0, WORLD_W - 1);
        cy = limit(by + offsets[k][1], 0, WORLD_H - 1);
        /* Twice: the second render takes the cached terrain path. */
        game_render();
        game_render();
        dump_frame(f);
    }
    fclose(f);
    /* Rendering must not feed back into the simulation: advance and draw
     * every update for a while and print a digest of the simulation state.
     * The test compares the digests of the two builds. */
    u32 h = 2166136261u;
    for (int step = 0; step < 600 && screen == SCREEN_PLAY; ++step) {
        input(0);
        game_render();
    }
#define MIX(v) do { int value_ = (int)(v); const u8 *p_ = (const u8 *)&value_; \
    for (unsigned n_ = 0; n_ < sizeof(value_); ++n_) { h ^= p_[n_]; h *= 16777619u; } } while (0)
    for (int i = 0; i < MAX_UNITS; ++i) {
        MIX(u[i].alive);
        if (!u[i].alive) continue;
        MIX(u[i].x); MIX(u[i].y); MIX(u[i].hp); MIX(u[i].type); MIX(u[i].owner);
        MIX(u[i].order); MIX(u[i].target); MIX(u[i].tx); MIX(u[i].ty);
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i) {
        MIX(b[i].alive);
        if (!b[i].alive) continue;
        MIX(b[i].x); MIX(b[i].y); MIX(b[i].hp); MIX(b[i].type); MIX(b[i].owner);
    }
    MIX(minerals); MIX(gas); MIX(frame); MIX(screen); MIX(won); MIX(lost);
    for (int cell = 0; cell < MAP_COLS * MAP_ROWS; ++cell)
        MIX(SC64_GRID_GET(visible, cell) | (SC64_GRID_GET(explored, cell) << 1));
    printf("STATE %08x frame %d screen %d\n", (unsigned)h, frame, screen);
    return 0;
}
