#ifndef SC64_DISPLAY_H
#define SC64_DISPLAY_H

#include "../common/gba.h"

/* A milder LCD lift than the Den display curve. Retain true black and bright
   accents, while opening the dark terrain/unit details on an original GBA/SP.
   Only palette installation does this work; drawing and simulation stay as-is. */
static void sc64_display_palette(const u16 *source) {
    static const u8 lcd[32]={
        0,3,5,6,7,9,10,11,12,13,14,15,16,17,18,19,
        20,20,21,22,23,24,24,25,26,27,27,28,29,30,30,31
    };
    u16 palette[224];
    for(int i=0;i<224;i++) {
        u16 color=source[i];
        palette[i]=(u16)(lcd[color&31]|(lcd[(color>>5)&31]<<5)|(lcd[(color>>10)&31]<<10));
    }
    gfx_set_palette(32,224,palette);

    /* Keep unknown space and shadows dark, with readable panel boundaries,
       secondary text and explored fog distinct from the uncharted map. */
    static const struct {u8 index,r,g,b;} ui[]={
        {C_DIM,21,23,25},{C_EDGE,13,18,21},{C_GOLD_DARK,20,15,5},
        {C_FOG,5,7,10},{C_GRASS,8,15,9},{C_TERRAN,11,21,31},
        {C_ZERG,25,11,22}
    };
    for(unsigned i=0;i<sizeof(ui)/sizeof(ui[0]);i++) {
        u16 color=(u16)(ui[i].r|(ui[i].g<<5)|(ui[i].b<<10));
        gfx_set_palette(ui[i].index,1,&color);
    }
}

#endif
