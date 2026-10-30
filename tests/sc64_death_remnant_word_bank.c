/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Private-input integration oracle: never embeds generated art in source control. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gba.h"
typedef struct { short offset_x, offset_y; unsigned short w, h, canvas_w, canvas_h; const unsigned char *pixels; } Sc64SpriteFrame;
#include "sc64_death_sprites_generated.h"
static u8 background[240 * 160], expected[240 * 160], actual[240 * 160];
static void reset(void) { gfx_begin(0); if (!gfx_blit_band(0,160,background)) abort(); gfx_set_clip_y(12,120); }
int main(void) {
    const unsigned ids[]={110,111,216,217,335,336}; unsigned count=0;
    for (int y=0;y<160;y++) for (int x=0;x<240;x++) background[y*240+x]=(u8)(x*13+y*29+(x^y)*7);
    for (unsigned i=0;i<sizeof(ids)/sizeof(*ids);i++) for (unsigned f=0;f<4;f++) {
        const Sc64SpriteFrame *s=sc64_death_image_frame(ids[i],f);
        const GbaSpriteWordFrame *w=sc64_death_image_word_frame(ids[i],f);
        if (!s || !w || w->width!=s->w || w->height!=s->h) abort();
        for (int p=0;p<4;p++) {
            int groups=(s->w+p+3)/4;
            for (int y=0;y<s->h;y++) for(int g=0;g<groups;g++) for(int lane=0;lane<4;lane++) {
                int x=g*4+lane-p; u8 c=x>=0&&x<s->w?s->pixels[y*s->w+x]:0;
                const u32 *pair=w->phase_words[p]+(y*groups+g)*2;
                if (((pair[0]>>(8*lane))&255u)!=c || ((pair[1]>>(8*lane))&255u)!=(c?255u:0u)) abort();
            }
        }
        const int xs[]={64,65,66,67,-3,238}; const int ys[]={40,40,40,40,11,119};
        for (int t=0;t<6;t++) {
            memcpy(expected,background,sizeof expected);
            for (int sy=0;sy<s->h;sy++) for(int sx=0;sx<s->w;sx++) {
                int x=xs[t]+sx,y=ys[t]+sy; u8 c=s->pixels[sy*s->w+sx];
                if(c&&x>=0&&x<240&&y>=12&&y<120) expected[y*240+x]=c;
            }
            reset(); gfx_blit_sprite(xs[t],ys[t],s->w,s->h,s->pixels);
            if (!gfx_read_band(0,160,actual) || memcmp(actual,expected,sizeof actual)) { fprintf(stderr,"indexed mismatch %u %u case %d\n",ids[i],f,t); return 1; }
            reset(); gfx_blit_sprite_words(xs[t],ys[t],w);
            if (!gfx_read_band(0,160,actual) || memcmp(actual,expected,sizeof actual)) { fprintf(stderr,"packed mismatch %u %u case %d\n",ids[i],f,t); return 1; }
            count++;
        }
    }
    printf("24 remnant frames, 4 packed phases and %u patterned framebuffer cases passed\n",count);
    return 0;
}
