/* Differential test of sc64_depot_cell_point's 32-bit fast path against the
 * original 64-bit computation (kept below as the oracle): every answer and
 * both output coordinates must agree, for in-range arguments (centers and
 * cells near each other and far apart, edges of the fast-path range) and for
 * arguments outside it (negative, huge) that take the general path. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

static int oracle(int cx, int cy, int col, int row, int *px, int *py)
{
    long long left = (long long)col * 16, top = (long long)row * 16;
    long long x = cx, y = cy, dx, dy;
    if (x < left) x = left;
    else if (x > left + 15) x = left + 15;
    if (y < top) y = top;
    else if (y > top + 15) y = top + 15;
    dx = x - cx; dy = y - cy;
    if (dx < -32 || dx > 32 || dy < -32 || dy > 32 ||
        dx * dx + dy * dy > 32 * 32 || x < INT_MIN || x > INT_MAX ||
        y < INT_MIN || y > INT_MAX) return 0;
    *px = (int)x; *py = (int)y;
    return 1;
}

#include "../starcraft/sc64_depot_goal.h"

static uint32_t s = 0x2545f491u;
static uint32_t rnd(void) { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s; }

int main(void)
{
    static const int edges[] = { -1, 0, 1, 15, 16, 0x3fff, 0x4000, 0x4001, 0x3ff, 0x400, 0x401,
                                 -0x40000000, 0x40000000, INT_MIN, INT_MAX, 100000, -100000 };
    long n = 0, yes = 0;
    long iter;
    for (iter = 0; iter < 120000000L; ++iter) {
        int cx, cy, col, row, a1 = -7, b1 = -7, a2 = -9, b2 = -9, r1, r2;
        switch (rnd() % 8) {
        case 0: case 1: case 2: case 3:      /* near the center */
            cx = (int)(rnd() % 4200); cy = (int)(rnd() % 4200);
            col = cx / 16 + (int)(rnd() % 9) - 4; row = cy / 16 + (int)(rnd() % 9) - 4; break;
        case 4: cx = (int)(rnd() % 0x4200); cy = (int)(rnd() % 0x4200);
            col = (int)(rnd() % 0x500); row = (int)(rnd() % 0x500); break;
        case 5: cx = edges[rnd() % (sizeof edges / sizeof *edges)]; cy = (int)(rnd() % 4200);
            col = edges[rnd() % (sizeof edges / sizeof *edges)]; row = (int)(rnd() % 300); break;
        case 6: cx = (int)rnd(); cy = (int)rnd(); col = (int)rnd(); row = (int)rnd(); break;
        default: cx = (int)(rnd() % 4200); cy = edges[rnd() % (sizeof edges / sizeof *edges)];
            col = (int)(rnd() % 300); row = edges[rnd() % (sizeof edges / sizeof *edges)]; break;
        }
        r1 = oracle(cx, cy, col, row, &a1, &b1);
        r2 = sc64_depot_cell_point(cx, cy, col, row, &a2, &b2);
        ++n; yes += r1;
        if (r1 != r2 || (r1 && (a1 != a2 || b1 != b2))) {
            fprintf(stderr, "MISMATCH cx %d cy %d col %d row %d: oracle %d (%d,%d) got %d (%d,%d)\n",
                    cx, cy, col, row, r1, a1, b1, r2, a2, b2);
            return 1;
        }
    }
    printf("depot cell point: %ld cases, %ld inside the ring, 0 mismatches\n", n, yes);
    assert(yes > 1000000);
    return 0;
}
