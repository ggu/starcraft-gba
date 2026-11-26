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

/* Admission must produce the same packets for every destination alignment.
 * The maze makes goal 2 a multi-run difference from goal 1. */
static void check_admission_alignment(void)
{
    static _Alignas(4) u8 output[SC64_MEMO_BYTES + 8];
    static u8 expected_field[SC64_MEMO_BYTES];
    static u8 expected_memo[sizeof(sc64_wide_memo)];
    int expected_reached = 0;
    for (unsigned phase = 0; phase < 4; ++phase) {
        memset(&sc64_wide_memo, 0, sizeof(sc64_wide_memo));
        memset(output, 0x93, sizeof(output));
        u8 *field = output + phase;
        assert(sc64_route_fill_wide_cached(field, 1) > SC64_MEMO_SMALL_FILL);
        int reached = sc64_route_fill_wide_cached(field, 2);
        assert(sc64_wide_memo.used == 1 && sc64_wide_memo.diff[0].count > 1);
        if (!phase) {
            expected_reached = reached;
            memcpy(expected_field, field, sizeof(expected_field));
            memcpy(expected_memo, &sc64_wide_memo, sizeof(expected_memo));
        } else {
            assert(reached == expected_reached);
            assert(!memcmp(field, expected_field, sizeof(expected_field)));
            assert(!memcmp(&sc64_wide_memo, expected_memo, sizeof(expected_memo)));
        }
        for (unsigned i = 0; i < phase; ++i) assert(output[i] == 0x93);
        for (unsigned i = phase + SC64_MEMO_BYTES; i < sizeof(output); ++i)
            assert(output[i] == 0x93);
        memset(field, 0, SC64_MEMO_BYTES);
        assert(sc64_route_fill_wide_cached(field, 2) == expected_reached);
        assert(!memcmp(field, expected_field, sizeof(expected_field)));
        check_grid_unchanged();
    }
    sc64_wide_memo_reset();
}

int main(void)
{
    race_cols = race_rows = 128;
    sc64_routes_reset();
    for (int y = 0; y < 128; y++)
        for (int x = 0; x < 128; x++)
            desired[y*128+x] = !(y & 1) || x == (y & 2 ? 0 : 127);
    build_path_grid();
    check_admission_alignment();
    check_memo_goal(1);
    check_memo_goal(2);
    assert(sc64_wide_memo.diff[0].count > 1);
    sc64_wide_memo.write_at = sizeof(sc64_wide_memo.packets) - 1;
    check_memo_goal(3);
    assert(!sc64_wide_memo.diff[0].reached);
    assert(sc64_wide_memo.diff[1].offset == 0);
    check_memo_goal(2);
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
