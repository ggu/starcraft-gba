/* Synthetic, asset-free storage test for the opt-in 128x96 geometry profile.
 * Compile with SC64_RACES and SC64_LARGE_MAPS. No imported map is indexed as
 * though it were 128 columns wide; race_mode uses the authored test arena. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

static void test_minimap_all_supported_geometries(void)
{
    static const int widths[] = {64, 96, 128};
    static const int heights[] = {64, 96};
    for (int wi = 0; wi < (int)(sizeof(widths) / sizeof(widths[0])); ++wi) {
        for (int hi = 0; hi < (int)(sizeof(heights) / sizeof(heights[0])); ++hi) {
            int width = widths[wi], height = heights[hi];
            race_cols = width;
            race_rows = height;
            for (int row = 0; row < height; ++row) {
                for (int col = 0; col < width; ++col) {
                    int cell = col + row * width;
                    int expected = (col * 24) / width +
                                   ((row * 24) / height) * 24;
                    int actual = sc64_minimap_cell(cell);
                    assert(actual == expected);
                    assert(actual >= 0 && actual < 24 * 24);
                }
            }
        }
    }
}

static void test_large_geometry_and_indexes(void)
{
    int last = 128 * 96 - 1;
    int target_cell = -1;

    race_mode = 1;
    race_cols = 128;
    race_rows = 96;
#ifdef SC64_IMPORTED_SKIRMISH
    race_skirmish_map = 0; /* Use the game's authored synthetic arena. */
#endif
    memset(u, 0, sizeof(u));
    memset(b, 0, sizeof(b));
    SC64_GRID_CLEAR(explored);
    SC64_GRID_CLEAR(visible);
    SC64_GRID_CLEAR(path_walkable);
    sc64_visibility_reset();

    assert(MAP_CELLS == 12288);
    assert(sc64_map_cell_row(last, 128) == 95);
    assert(sc64_map_cell_col(last, 128) == 127);
    assert(sc64_minimap_cell(last) == 23 + 23 * 24);
    assert(sc64_race_storage_geometry_valid(128, 96));
    assert(!sc64_race_storage_geometry_valid(129, 96));
    assert(!sc64_race_storage_geometry_valid(128, 97));

    sc64_explore_unit(127 * 16 + 8, 95 * 16 + 8, U_PROBE);
    sc64_adjust_unit_visibility(127 * 16 + 8, 95 * 16 + 8, U_PROBE, 1);
    assert(SC64_GRID_GET(explored, last));
    assert(SC64_GRID_GET(visible, last));
    assert(sc64_minimap_explored[sc64_minimap_cell(last)] != 0);
    assert(sc64_minimap_visible[sc64_minimap_cell(last)] != 0);

    invalidate_path_grid();
    build_path_grid();
    assert(SC64_GRID_GET(path_walkable, last));

    assert(sc64_target_cell_for(2047, 1535, &target_cell));
    assert(target_cell == 31 + 23 * SC64_TARGET_GRID_COLS);
    u[0].alive = 1;
    u[0].owner = SIDE;
    u[0].x = 2047;
    u[0].y = 1535;
    sc64_target_index_begin();
    assert(sc64_target_index_active);
    assert(sc64_target_head[SIDE][target_cell] == 0);
    sc64_target_index_end();
}

static void test_save_roundtrip_for_packed_grid_and_high_resources(void)
{
    int last = MAP_CELLS - 1;
    assert(MAX_MINERALS == 48);
    game_init();
    race_mode = 0;
    race_selected_mission = 2; /* P1 is present in the fixed Protoss probe. */
    race_human = R_PROTOSS;
    init_state();
    memset(u, 0, sizeof(u));
    memset(b, 0, sizeof(b));
    memset(m, 0, sizeof(m));
    SC64_GRID_CLEAR(explored);

    for (int i = 32; i < 48; ++i) {
        m[i].x = 2000 - i;
        m[i].y = 1500 - i;
        m[i].amount = 1000 + i;
    }
    /* The save stores all max-geometry exploration bits even though the
     * catalog fixture below is a smaller map; this only tests packed-buffer
     * persistence, while the width-limit helper is tested separately. */
    SC64_GRID_SET(explored, last, 1);
    assert(race_save_game());

    for (int i = 32; i < 48; ++i) m[i].amount = 0;
    SC64_GRID_CLEAR(explored);
    assert(race_load_game());
    for (int i = 32; i < 48; ++i) assert(m[i].amount == 1000 + i);
    assert(SC64_GRID_GET(explored, last));
}

int main(void)
{
    test_minimap_all_supported_geometries();
    test_large_geometry_and_indexes();
    test_save_roundtrip_for_packed_grid_and_high_resources();
    puts("large-map storage tests passed");
    return 0;
}
