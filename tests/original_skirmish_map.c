/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <stdio.h>
#include <string.h>
#include "../starcraft/original_skirmish_map.h"

static int clear_area(int map, int x, int y, int radius)
{
    int col = x / 16, row = y / 16, dx, dy;
    for (dy = -radius; dy <= radius; ++dy)
        for (dx = -radius; dx <= radius; ++dx)
            if (original_skirmish_blocked(map, col + dx, row + dy)) return 0;
    return 1;
}

int main(void)
{
    static const int dx[4] = {56, 42, 56, 42}, dy[4] = {24, 40, 8, 20};
    static const int step_x[4] = {1, -1, 0, 0}, step_y[4] = {0, 0, 1, -1};
    unsigned char seen[64 * 64];
    int queue[64 * 64], map, seat, i, different = 0;
    if (!original_skirmish_blocked(-1, 0, 0) ||
        !original_skirmish_blocked(ORIGINAL_MAP_COUNT, 0, 0) ||
        !original_skirmish_blocked(0, -1, 0) || !original_skirmish_blocked(0, 64, 0) ||
        !original_skirmish_blocked(0, 0, -1) || !original_skirmish_blocked(0, 0, 64)) return 1;
    for (i = 0; i < 64 * 64; ++i)
        different += original_skirmish_blocked(0, i % 64, i / 64) !=
                     original_skirmish_blocked(1, i % 64, i / 64);
    if (different < 100) return 2;
    for (map = 0; map < ORIGINAL_MAP_COUNT; ++map) {
        const OriginalSkirmishMap *m = &original_skirmish_maps[map];
        int head = 0, tail = 0;
        memset(seen, 0, sizeof(seen));
        i = m->start_x[0] / 16 + 64 * (m->start_y[0] / 16);
        queue[tail++] = i; seen[i] = 1;
        while (head < tail) {
            int cell = queue[head++], k;
            for (k = 0; k < 4; ++k) {
                int x = cell % 64 + step_x[k], y = cell / 64 + step_y[k], next;
                if (original_skirmish_blocked(map, x, y)) continue;
                next = x + y * 64;
                if (!seen[next]) { seen[next] = 1; queue[tail++] = next; }
            }
        }
        for (seat = 0; seat < 4; ++seat) {
            int x = m->start_x[seat], y = m->start_y[seat];
            int sx = seat % 2 ? 1 : -1, sy = (seat == 0 || seat == 2) ? 1 : -1;
            if (!clear_area(map, x, y, 2) || !clear_area(map, x - sx * 48, y, 1) ||
                !clear_area(map, x - sx * 64, y, 1) || !seen[x / 16 + 64 * (y / 16)]) {
                fprintf(stderr, "map %d seat %d start, depot, geyser or route blocked\n", map, seat);
                return 3;
            }
            for (i = 0; i < 4; ++i)
                if (!clear_area(map, x + sx * dx[i], y + sy * dy[i], 1)) {
                    fprintf(stderr, "map %d seat %d patch %d blocked\n", map, seat, i);
                    return 4;
                }
        }
    }
    puts("original skirmish maps: ok");
    return 0;
}
