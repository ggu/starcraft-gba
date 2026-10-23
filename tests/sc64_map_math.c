#include <assert.h>
#include <stdio.h>

#include "../starcraft/sc64_map_math.h"

static const unsigned dimensions[] = {64u, 96u, 128u};

static void test_cells(void)
{
    unsigned wi, hi;
    for (wi = 0; wi < sizeof(dimensions) / sizeof(dimensions[0]); ++wi) {
        unsigned width = dimensions[wi];
        for (hi = 0; hi < sizeof(dimensions) / sizeof(dimensions[0]); ++hi) {
            unsigned height = dimensions[hi];
            unsigned cell, count = width * height;
            for (cell = 0; cell < count; ++cell) {
                assert(sc64_map_cell_row(cell, width) == cell / width);
                assert(sc64_map_cell_col(cell, width) == cell % width);
            }
        }
    }
    assert(sc64_map_cell_row(0, 0) == SC64_MAP_MATH_INVALID);
    assert(sc64_map_cell_col(0, 32) == SC64_MAP_MATH_INVALID);
    assert(sc64_map_cell_row(96u * 128u, 96) == SC64_MAP_MATH_INVALID);
}

static void test_tile_minimap(void)
{
    unsigned wi, hi;
    for (wi = 0; wi < sizeof(dimensions) / sizeof(dimensions[0]); ++wi) {
        unsigned width = dimensions[wi];
        for (hi = 0; hi < sizeof(dimensions) / sizeof(dimensions[0]); ++hi) {
            unsigned height = dimensions[hi];
            unsigned row, col;
            for (row = 0; row < height; ++row) {
                for (col = 0; col < width; ++col) {
                    unsigned expected = (col * 24u / width) +
                                        (row * 24u / height) * 24u;
                    assert(sc64_map_tile_minimap_index(col, row, width, height) == expected);
                }
            }
            assert(sc64_map_tile_minimap_index(width, 0, width, height) ==
                   SC64_MAP_MATH_INVALID);
            assert(sc64_map_tile_minimap_index(0, height, width, height) ==
                   SC64_MAP_MATH_INVALID);
        }
    }
    assert(sc64_map_tile_minimap_index(0, 0, 80, 64) == SC64_MAP_MATH_INVALID);
}

static void test_pixel_minimap(void)
{
    unsigned wi, hi;
    for (wi = 0; wi < sizeof(dimensions) / sizeof(dimensions[0]); ++wi) {
        unsigned width = dimensions[wi];
        for (hi = 0; hi < sizeof(dimensions) / sizeof(dimensions[0]); ++hi) {
            unsigned height = dimensions[hi];
            unsigned y, x;
            for (y = 0; y < height * 16u; ++y) {
                for (x = 0; x < width * 16u; ++x) {
                    unsigned expected = (x * 24u / (width * 16u)) +
                                        (y * 24u / (height * 16u)) * 24u;
                    assert(sc64_map_pixel_minimap_index(x, y, width, height) == expected);
                }
            }
            assert(sc64_map_pixel_minimap_index(width * 16u, 0, width, height) ==
                   SC64_MAP_MATH_INVALID);
            assert(sc64_map_pixel_minimap_index(0, height * 16u, width, height) ==
                   SC64_MAP_MATH_INVALID);
        }
    }
    assert(sc64_map_pixel_minimap_index(0, 0, 96, 80) == SC64_MAP_MATH_INVALID);
}

int main(void)
{
    test_cells();
    test_tile_minimap();
    test_pixel_minimap();
    puts("SC64 map math tests passed");
    return 0;
}
