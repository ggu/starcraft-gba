/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Prepared-input oracle for every selected pose and heading. */
#define main sc64_walk_original_fixture_main
#include "sc64_unit_walk_frames_game.c"
#undef main
#ifndef SC64_WALK_WORDS
#error "the packed walk oracle needs SC64_WALK_WORDS"
#endif
static u8 expected[240*160] __attribute__((aligned(4)));
static u8 actual[240*160] __attribute__((aligned(4)));
int main(void) {
    static const unsigned units[]={0,37,38,41,42,43,65};
    static const int xs[]={-17,1,111,237,119};
    static const int ys[]={-10,12,114,153,68};
    unsigned cases=0;
    gba_init();
    for (unsigned r=0;r<sizeof(units)/sizeof(units[0]);r++) {
        const Sc64UnitWalkCycle *cycle=sc64_unit_walk_cycle(units[r]); assert(cycle);
        for (unsigned step=0;step<cycle->count;step++) for(int dir=0;dir<32;dir++) {
            const Sc64SpriteFrame *s=&cycle->steps[step].frames[dir];
            const GbaSpriteWordFrame *word=sc64_walk_word_frame(units[r],step,(unsigned)dir);
            assert(word && word->width==s->w && word->height==s->h);
            for (int i=0;i<5;i++) {
                int x=xs[i]-s->canvas_w/2+s->offset_x, y=ys[i]-s->canvas_h/2+s->offset_y;
                gfx_begin(17); gfx_set_clip_y(i==4?12:0,i==4?120:160);
                gfx_blit_sprite(x,y,s->w,s->h,s->pixels); assert(gfx_read_band(0,160,expected));
                gfx_begin(17); gfx_set_clip_y(i==4?12:0,i==4?120:160);
                gfx_blit_sprite_words(x,y,word); assert(gfx_read_band(0,160,actual));
                assert(!memcmp(expected,actual,sizeof(actual))); cases++;
            }
        }
    }
    assert(!sc64_walk_word_frame(83,0,0));
    assert(!sc64_walk_word_frame(32,0,0));
    printf("walk word pixel parity: %u selected pose/heading/clip cases\n",cases);
    return 0;
}
