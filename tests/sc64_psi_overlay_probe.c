/* SPDX-License-Identifier: GPL-3.0-or-later
 * HOST-only probe for the optional, sampled Pylon power boundary. */
#define main sc64_skirmish_runtime_other_main
#include "sc64_skirmish_runtime.c"
#undef main
int main(void)
{
    u8 mask[UI_PSI_ROWS][UI_PSI_COLS];
    gba_init(); start_race(R_PROTOSS);
    memset(b,0,sizeof(b)); camx=camy=0;
    int p=fixture_building_raw(SIDE,156,120,24);
    assert(p>=0);
    ui_psi_overlay_mask(mask);
    assert(mask[1][7]);                 /* Pylon center */
    assert(mask[5][7]);                 /* +64 vertical */
    assert(!mask[6][7]);                /* +80 vertical */
    assert(!mask[5][13]);               /* +96,+64 masked corner */
    gfx_begin(0); ui_draw_psi_overlay();
    int marked=0;
    for (int y=0;y<160;++y) for (int x=0;x<240;++x) {
        u8 pixel=gfx_test_pixel(x,y);
        if (y<12 || y>=120) assert(pixel==0);
        else if (pixel==C_CYAN) ++marked;
    }
    assert(marked>0);
    b[p].owner=ENEMY; ui_psi_overlay_mask(mask);
    for (int y=0;y<UI_PSI_ROWS;++y) for (int x=0;x<UI_PSI_COLS;++x) assert(!mask[y][x]);
    b[p].owner=SIDE; b[p].under_construction=1; ui_psi_overlay_mask(mask);
    for (int y=0;y<UI_PSI_ROWS;++y) for (int x=0;x<UI_PSI_COLS;++x) assert(!mask[y][x]);
    puts("PASS sampled psi overlay geometry, ownership and clipping");
    return 0;
}
