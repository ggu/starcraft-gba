/* Pixel parity fixture for Backwater's cached versus full map rendering. */
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

static _Alignas(4) u8 cached_frame[240 * 160];
static _Alignas(4) u8 full_frame[240 * 160];

static int
fail(const char *label)
{
    fprintf(stderr, "Backwater render cache mismatch: %s\n", label);
    return 1;
}

static int
compare_cached_to_full(const char *label, int expect_reuse)
{
    game_render();
    if (expect_reuse && !sc64_terrain_cache_reused_base)
        { fprintf(stderr, "Backwater render cache mismatch: expected cached band reuse (%s)\n", label); return 1; }
    if (!gfx_read_band(0, 160, cached_frame))
        return fail("read cached frame");

    sc64_terrain_cache_valid = 0;
    game_render();
    if (!gfx_read_band(0, 160, full_frame))
        return fail("read full frame");
    if (memcmp(cached_frame, full_frame, sizeof(cached_frame)) != 0)
        return fail(label);
    return 0;
}

static void
set_tile_state(int col, int row, int is_visible, int is_explored)
{
    int cell = col + row * MAP_COLS;
    if (cell < 0 || cell >= MAP_CELLS) return;
    visible[cell] = is_visible;
    explored[cell] = is_explored;
}

static int
tile_has_resource_center(int col, int row)
{
    int i;
    for (i = 0; i < MAX_MINERALS; i++)
        if (m[i].amount > 0 && m[i].x / 16 == col && m[i].y / 16 == row)
            return 1;
    for (i = 0; i < geyser_count; i++)
        if (geysers[i].x / 16 == col && geysers[i].y / 16 == row)
            return 1;
    return 0;
}

static void
find_fog_test_tiles(int *cols, int *rows)
{
    int col, row, found = 0;
    int min_col = camx / 16, min_row = camy / 16;
    for (row = min_row; row < MAP_ROWS && row < min_row + 8; row++)
        for (col = min_col; col < MAP_COLS && col < min_col + 16; col++)
            if (!tile_has_resource_center(col, row)) {
                cols[found] = col; rows[found] = row; found++;
                if (found == 3) return;
            }
    while (found < 3) {
        cols[found] = min_col + found;
        rows[found] = min_row;
        found++;
    }
}

int
main(void)
{
    int i, live_building = -1, fog_cols[3], fog_rows[3];
    gba_init();
    game_init();
    game_update(KEY_A, KEY_A);

    if (compare_cached_to_full("opening frame", 0)) return 1;
    if (compare_cached_to_full("unchanged viewport", 1)) return 2;

    /* Exercise a real controller-driven scroll with the normal sight update. */
    for (i = 0; i < 100; i++) game_update(KEY_RIGHT, 0);
    if (compare_cached_to_full("controller camera pan", 0)) return 3;

    /* Make a fog cell newly unknown, reveal another, and change explored-only
       state. Redrawing dirty tiles must erase old terrain where sight resets. */
    find_fog_test_tiles(fog_cols, fog_rows);
    set_tile_state(fog_cols[0], fog_rows[0], 0, 0);
    set_tile_state(fog_cols[1], fog_rows[1], 1, 1);
    set_tile_state(fog_cols[2], fog_rows[2], 0, 1);
    if (compare_cached_to_full("fog toggle and reveal", 0)) return 4;
    memset(visible, 0, sizeof(visible));
    memset(explored, 0, sizeof(explored));
    if (compare_cached_to_full("exploration reset", 0)) return 5;

    /* Real simulation updates move units and change current visibility. */
    for (i = 0; i < 180; i++) game_update(i & 1 ? KEY_RIGHT : 0, 0);
    if (compare_cached_to_full("units and sight after real updates", 0)) return 6;

    /* Move/deplete/retype visible resources to invalidate the static-object
       generation and ensure neither cached art nor fog leaves ghosts. */
    if (u[0].alive) {
        int col = u[0].x / 16, row = u[0].y / 16;
        set_tile_state(col, row, 1, 1);
        m[0].x = u[0].x; m[0].y = u[0].y; m[0].amount = 900; m[0].style = 176;
        if (compare_cached_to_full("mineral placed in visible cell", 0)) return 7;
        m[0].amount = 0;
        if (compare_cached_to_full("mineral depletion", 0)) return 8;
        m[0].x += 16; m[0].style = 178;
        if (compare_cached_to_full("mineral position and style", 0)) return 9;

        geyser_count = 1;
        geysers[0].x = u[0].x + 16; geysers[0].y = u[0].y;
        set_tile_state(geysers[0].x / 16, geysers[0].y / 16, 1, 1);
        if (compare_cached_to_full("geyser position", 0)) return 10;
        set_tile_state(geysers[0].x / 16, geysers[0].y / 16, 0, 0);
        if (compare_cached_to_full("geyser visibility change", 0)) return 11;
    }

    for (i = 0; i < MAX_BUILDINGS; i++)
        if (b[i].alive) { live_building = i; break; }
    if (live_building >= 0) {
        b[live_building].hp /= 2;
        selected_building = live_building;
        if (compare_cached_to_full("building HP and selection", 0)) return 12;
        b[live_building].owner = ENEMY;
        if (compare_cached_to_full("building owner change", 0)) return 13;
        b[live_building].alive = 0;
        if (compare_cached_to_full("building death", 0)) return 14;
    }

    puts("PASS: Backwater cached/full 240x160 renderer parity");
    return 0;
}
