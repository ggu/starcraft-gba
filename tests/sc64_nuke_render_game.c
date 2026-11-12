/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/sc64_nuke_render.h"

static u8 page[240 * 160] __attribute__((aligned(4)));

static void background(void)
{
    gfx_begin(0);
    gfx_set_clip_y(0, 160);
    gfx_rect(0, 0, 240, 160, 17);
    gfx_set_clip_y(12, 120);
}

static void check_frames(const Sc64NukeFrame *frames, unsigned n, int x, int y)
{
    for (unsigned i = 0; i < n; ++i)
        assert(sc64_nuke_draw_frame(&frames[i], x, y));
}

int main(void)
{
    gba_init();
    background();
    check_frames(sc64_nuke_missile_frames, SC64_NUKE_MISSILE_FRAME_COUNT, 20, 30);
    check_frames(sc64_nuke_dot_frames, SC64_NUKE_DOT_FRAME_COUNT, 36, 52);
    assert(gfx_read_band(0, 160, page));
    assert(page[52 * 240 + 36] != 17 || page[51 * 240 + 35] != 17);
    background();
    check_frames(sc64_nuke_burst_frames, SC64_NUKE_BURST_FRAME_COUNT, 190, 92);
    check_frames(sc64_nuke_impact_frames, SC64_NUKE_IMPACT_FRAME_COUNT, 0, 85);
    assert(gfx_read_band(0, 160, page));
    unsigned changed = 0;
    for (unsigned y = 0; y < 160; ++y)
        for (unsigned x = 0; x < 240; ++x) {
            if (y < 12 || y >= 120) assert(page[y * 240 + x] == 17);
            else changed += page[y * 240 + x] != 17;
        }
    assert(changed);
    const u8 invalid[] = { 0xc1 };
    const u8 *cursor = invalid;
    u8 scratch[1];
    assert(!sc64_nuke_expand_row(&cursor, invalid + sizeof(invalid), scratch, 1));
    puts("source nuke rasters: 40 decoded frames, palette effects and HUD clipping");
    return 0;
}
