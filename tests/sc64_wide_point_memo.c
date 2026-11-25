/* SPDX-License-Identifier: GPL-3.0-or-later */
#define main sc64_route_oracle_suite
#include "sc64_route_keep_fields.c"
#undef main

static void check_memo_goal(int goal)
{
    const unsigned bytes = sc64_route_active_bytes;
    static _Alignas(4) u8 output[SC64_MEMO_BYTES + 8];
    memset(output, 0x93, sizeof(output));
    int reached = sc64_route_fill_wide_cached(output + 4, goal);
    reference_bfs(&oracle[POOL], goal);
    int count = 0;
    for (int i = 0; i < MAP_COLS * MAP_ROWS; ++i) {
        unsigned bit = (unsigned)i * 3, at = bit >> 3;
        unsigned packed = output[4 + at] | (unsigned)output[5 + at] << 8;
        assert(((packed >> (bit & 7)) & 7u) == oracle[POOL].dir[i]);
        count += oracle[POOL].dir[i] != SC64_ROUTE_UNREACHABLE;
    }
    assert(reached == count);
    for (unsigned i = 0; i < 4; ++i) {
        assert(output[i] == 0x93 && output[4 + bytes + i] == 0x93);
    }
    check_grid_unchanged();
}

int main(void)
{
    race_cols = race_rows = 128;
    sc64_routes_reset();
    for (int y = 0; y < 128; y++)
        for (int x = 0; x < 128; x++)
            desired[y*128+x] = !(y & 1) || x == (y & 2 ? 0 : 127);
    build_path_grid();
    check_memo_goal(1);
    for (int i = 2; i < 40; ++i) {
        check_memo_goal(i);
        check_memo_goal(1);
        check_memo_goal(i);
    }
    assert(sc64_wide_memo.used == SC64_MEMO_GOALS);
    assert(sc64_route_configure() && sc64_wide_memo.reached);
    race_rows = 96;
    assert(sc64_route_configure() && !sc64_wide_memo.reached);
    race_rows = 128;
    assert(sc64_route_configure());
    check_memo_goal(1);
    race_rows = 0;
    assert(!sc64_route_configure() && !sc64_wide_memo.reached);
    race_rows = 128;
    assert(sc64_route_configure());
    for (int edit = 0; edit < 64; ++edit) {
        int cell = rn(16384);
        desired[cell] ^= 1;
        invalidate();
        build_path_grid();
        check_memo_goal(1);
        check_memo_goal(cell);
        check_memo_goal(cell);
    }
    memset(desired, 0, sizeof(desired));
    invalidate(); build_path_grid();
    check_memo_goal(1);
    race_cols = 96;
    sc64_routes_reset();
    memset(desired, 1, sizeof(desired)); build_path_grid();
    check_memo_goal(81);
    puts("Packed directions, discovery counts, canaries and edited walk grids matched scalar BFS.");
}
