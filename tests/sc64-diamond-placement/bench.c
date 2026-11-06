/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Isolated placement cost, not a game-scene performance measurement. */
#include "gba.h"
volatile u32 bench_cycles[4], bench_checksum[4], bench_done_flag;
static u32 checksum(void) {
    const volatile u16 *p = (const volatile u16 *)0x0600a000;
    u32 h = 2166136261u;
    for (unsigned i = 0; i < 240u * 160u / 2u; ++i) h = (h ^ p[i]) * 16777619u;
    return h;
}
__attribute__((noinline)) void diamond_bench_done(void) { __asm__ volatile("" ::: "memory"); }
void game_init(void) {
    const int count[4] = {1, 32, 128, 128};
    for (int batch = 0; batch < 4; ++batch) {
        gfx_begin(7);
        if (batch == 3) gfx_set_clip_y(25, 140);
        u32 start = gba_clock();
        for (int i = 0; i < count[batch]; ++i) {
            int x = (i * 37 % 300) - 30, y = (i * 29 % 200) - 20;
            int rx = (i % 3 == 0) ? 4 : ((i % 3 == 1) ? 12 : 31);
            int ry = (i % 3 == 0) ? 3 : ((i % 3 == 1) ? 6 : 13);
            gfx_diamond(x, y, rx, ry, (u8)(i * 17 + 5));
        }
        bench_cycles[batch] = gba_clock() - start;
        bench_checksum[batch] = checksum();
    }
    bench_done_flag = 1;
    diamond_bench_done();
}
void game_update(u16 held, u16 pressed) { (void)held; (void)pressed; }
void game_render(void) {}
