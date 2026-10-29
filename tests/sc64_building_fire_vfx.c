/* Asset-free damage-overlay renderer checks.
 * SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#define SC64_BUILDING_FIRE_ART
#define SC64_BUILDING_FIRE_BANK_HEADER "sc64_building_fire_synthetic.h"
#include "../starcraft/sc64_building_fire_vfx.h"

static unsigned calls, pixels, seen_banks[20], seen_flip;
static int first_x, first_y;
/* These stubs emulate the scene clip; actual blitters own clipping. */
void gfx_pixel(int x, int y, u8 c) {
    if ((unsigned)x < 240u && y >= 12 && y < 120 && c) ++pixels;
}
void gfx_blit_sprite(int x, int y, int w, int h, const u8 *src) {
    if (!calls) { first_x=x; first_y=y; }
    ++calls; ++seen_banks[src[0]-1u];
    for (int row=0;row<h;++row) for(int col=0;col<w;++col)
        gfx_pixel(x+col,y+row,src[row*w+col]);
}
#ifdef SC64_UNIT_FACING
void gfx_blit_sprite_hflip(int x, int y, int w, int h, const u8 *src) {
    ++calls; ++seen_flip; ++seen_banks[src[0]-1u];
    for (int row=0;row<h;++row) for(int col=0;col<w;++col)
        gfx_pixel(x+col,y+row,src[row*w+w-1-col]);
}
#endif
static void reset(void) {
    calls = pixels = seen_flip = 0;
    for (unsigned i=0;i<20;++i) seen_banks[i]=0;
}
static void stages(void) {
    unsigned active, large;
    for (unsigned count=1;count<=22;++count) {
        unsigned previous_active=0, previous_large=0;
        for (unsigned hp=90; ; --hp) {
            int maxq=90*256, hpq=(int)hp*256, n=(int)count*2;
            int two=maxq-maxq/3, per=two/(n+1);
            int state=(hpq-two%per-maxq/3-1)/per;
            if(state<0) state=0;
            if(state>n) state=n;
            unsigned damage=(unsigned)(n-state);
            unsigned expect_active=damage>count?count:damage;
            unsigned expect_large=damage>count?damage-count:0;
            if(hp==90) expect_active=expect_large=0;
            assert(sc64_building_damage_stages(hp,90,count,&active,&large));
            assert(active==expect_active && large==expect_large);
            assert(active>=previous_active && large>=previous_large);
            previous_active=active;previous_large=large;
            if (!hp) break;
        }
    }
    assert(!sc64_building_damage_stages(0,90,0,&active,&large));
    assert(!sc64_building_damage_stages(0,90,23,&active,&large));
    assert(!sc64_building_damage_stages(0,0,1,&active,&large));
    assert(!sc64_building_damage_stages(0,0x800000u,1,&active,&large));
    assert(!sc64_building_damage_stages(0,90,1,0,&large));
}
static void render(void) {
    reset(); assert(!sc64_building_damage_draw(255,100,60,0,90,2)); assert(!calls);
    assert(!sc64_building_damage_draw(107,100,60,0,90,2)); assert(!calls);
    assert(!sc64_building_damage_draw(106,100,60,0,0,2)); assert(!calls);
    assert(sc64_building_damage_draw(106,100,60,90,90,2)); assert(!calls);
    assert(sc64_building_damage_draw(106,100,60,0,90,2));
    assert(seen_banks[3]==1 && seen_banks[9]==1);
    assert(first_x==69 && first_y==42); /* LO (-40,-30) projected by 1/2, frame 1 */
#ifdef SC64_UNIT_FACING
    assert(calls==3 && seen_flip==1 && seen_banks[16]==1);
#else
    assert(calls==2 && pixels==6); /* flipped 2-pixel frame uses gfx_pixel */
#endif
    reset(); assert(sc64_building_damage_draw(106,-100,-100,0,90,2)); assert(pixels==0);
}
int main(void) { stages(); render(); puts("building damage overlay checks passed"); return 0; }
