/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Actual engine firing path and independent source-LUT/frame oracle. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "game.c"

static u8 before_pixels[240 * 160];

static void reset_world(void)
{
    init_state();
    memset(u,0,sizeof(u)); memset(b,0,sizeof(b));
    memset(invincible_units,0,sizeof(invincible_units));
    memset(invincible_buildings,0,sizeof(invincible_buildings));
    memset(path_walkable,0xff,sizeof(path_walkable));
    memset(visible,0xff,sizeof(visible));
    path_grid_valid=1; target_lists_valid=0; sc64_target_index_active=0;
    race_mode=0; race_selected_mission=SC64_RACE_MISSION_T1;
    weapons_level=armor_level=0; supply_used=0; camx=camy=0;
    memset(tracers,0,sizeof(tracers));
}

static int add_raw_unit(int owner,int raw,int x,int y)
{
    int type=sc64_race_unit_from_raw(raw);
    assert(type>=0);
    int slot=spawn_unit(owner,type,x,y);
    assert(slot>=0);
    u[slot].raw_type=raw;
    u[slot].raw_owner=owner==ENEMY?1:0;
    return slot;
}

static void patterned_backdrop(void)
{
    gfx_begin(C_GRASS);
    for(int y=0;y<120;y+=16) for(int x=0;x<240;x+=16) {
        int c=((x/16+y/16)&1)?C_DIRT:C_GRASS;
        gfx_rect(x,y,16,16,(u8)c);
        if((x+3*y)%7==0) gfx_rect(x+5,y+6,3,2,C_STONE);
    }
    gfx_rect(0,120,240,40,C_PANEL);
#ifdef GBA_SCENE_CLIP
    gfx_set_clip_y(12,120);
#endif
    for(int y=0;y<160;y++) for(int x=0;x<240;x++)
        before_pixels[y*240+x]=gfx_test_pixel(x,y);
}

/* Compute each expected pixel from source strength, raw source LUT and the
 * actual pre-draw destination. This does not consult the projected LUT. */
static int assert_source_frame(int image,unsigned frame_index,int cx,int cy,int direction)
{
    const Sc64TankSourceFrame *f=sc64_private_tank_frame(image,frame_index);
    assert(f);
    int flipped=direction>16;
    unsigned src_offset=flipped?f->flip_offset:f->offset;
    int frame_x=flipped?f->flip_x:f->x;
    const u8 *src=sc64_tank_source_pixels+src_offset;
    int left=cx-camx-(int)f->cw/2+frame_x;
    int top=cy-camy+12-(int)f->ch/2+f->y;
    int changed=0;
    for(int yy=0;yy<f->h;yy++) for(int xx=0;xx<f->w;xx++) {
        int x=left+xx,y=top+yy;
        if(x<0||x>=240||y<12||y>=120) continue;
        unsigned strength=src[(yy+f->y)*f->cw+frame_x+xx];
        unsigned expected=before_pixels[y*240+x];
        if(strength) {
            unsigned old=before_pixels[y*240+x];
            expected=sc64_tank_palette_remap[
                sc64_tank_raw_lut[(strength-1)*256+sc64_tank_reverse_palette[old]]];
        }
        unsigned actual=gfx_test_pixel(x,y);
        if(actual!=expected) {
            fprintf(stderr,"source VFX mismatch image=%d frame=%u at=%d,%d strength=%u before=%u rev=%u raw=%u projected=%u actual=%u expected=%u\n",
                    image,frame_index,x,y,strength,before_pixels[y*240+x],sc64_tank_reverse_palette[before_pixels[y*240+x]],sc64_tank_raw_lut[(strength-1)*256+sc64_tank_reverse_palette[before_pixels[y*240+x]]],sc64_tank_projected_lut[(strength-1)*256+before_pixels[y*240+x]],actual,expected);
            abort();
        }
        if(actual!=before_pixels[y*240+x]) ++changed;
    }
    return changed;
}

static void assert_projected_lut_from_source(void)
{
    for(unsigned strength=1;strength<=63;strength++)
        for(unsigned dest=0;dest<256;dest++) {
            unsigned expected=sc64_tank_palette_remap[
                sc64_tank_raw_lut[(strength-1)*256+sc64_tank_reverse_palette[dest]]];
            assert(sc64_tank_projected_lut[(strength-1)*256+dest]==expected);
        }
}

static void fire_and_check(int raw,int age,int muzzle_image,unsigned muzzle_frame,
                           int impact_image,unsigned impact_frame,int target_x,int target_y)
{
    reset_world();
    int attacker=add_raw_unit(SIDE,raw,100,80);
    int target=add_raw_unit(ENEMY,5,target_x,target_y);
    u[target].shield=0; u[target].hp=u[target].maxhp=1000;
    tick_combat(&u[attacker],target);
    int style=sc64_weapon_vfx_style(tracers[0].color);
    assert(style==(raw==30?SC64_TANK_SOURCE_SIEGE:SC64_TANK_SOURCE_TANK));
    assert(tracers[0].x0==100 && tracers[0].y0==80);
    assert(tracers[0].x1==u[target].x && tracers[0].y1==u[target].y);
    int duration=raw==30?20:28;
    tracers[0].life=duration-age;
    patterned_backdrop();
    draw_tracers();
    int direction=sc64_weapon_vfx_color(tracers[0].color)&31;
    int frame=direction<=16?direction:32-direction;
    if(muzzle_image==536 && age==0) assert(assert_source_frame(muzzle_image,muzzle_frame,100,80,direction)>0);
    if(muzzle_image==537) {
        int lo=sc64_tank_siege_lo[frame][0], ly=sc64_tank_siege_lo[frame][1];
        if(direction>16) lo=-lo;
        assert(assert_source_frame(muzzle_image,muzzle_frame,100+lo,80+ly,direction)>0);
    }
    if(impact_image) assert(assert_source_frame(impact_image,impact_frame,u[target].x,u[target].y,0)>0);
    for(int y=120;y<160;y++) for(int x=0;x<240;x++)
        assert(gfx_test_pixel(x,y)==before_pixels[y*240+x]);
}

static void generic_weapon_still_renders(void)
{
    reset_world();
    int attacker=add_raw_unit(SIDE,0,100,80); /* ordinary Marine */
    int target=add_raw_unit(ENEMY,5,152,80);
    tick_combat(&u[attacker],target);
    assert(tracers[0].life>0);
    assert(sc64_weapon_vfx_style(tracers[0].color)!=SC64_TANK_SOURCE_TANK);
    assert(sc64_weapon_vfx_style(tracers[0].color)!=SC64_TANK_SOURCE_SIEGE);
    patterned_backdrop();
    draw_tracers();
    int changed=0;
    for(int y=12;y<120;y++) for(int x=0;x<240;x++)
        changed += gfx_test_pixel(x,y)!=before_pixels[y*240+x];
    assert(changed>0);
}

/* Fog gates source frames per 16x16 cell, whether the renderer takes the
 * whole-frame fast path (every covered cell visible) or the per-cell path.
 * Expected pixels apply the source LUT only where the world cell is visible,
 * using the renderer's documented rule (x in the map, currently_visible). */
static int assert_fogged_frame(int image,unsigned frame_index,int cx,int cy,int direction)
{
    const Sc64TankSourceFrame *f=sc64_private_tank_frame(image,frame_index);
    assert(f);
    int flipped=direction>16;
    int frame_x=flipped?f->flip_x:f->x;
    const u8 *src=sc64_tank_source_pixels+(flipped?f->flip_offset:f->offset);
    int left=cx-camx-(int)f->cw/2+frame_x, top=cy-camy+12-(int)f->ch/2+f->y;
    int changed=0;
    for(int y=0;y<160;y++) for(int x=0;x<240;x++) {
        unsigned expected=before_pixels[y*240+x];
        int xx=x-left, yy=y-top, wx=x+camx, wy=y+camy-12;
        if(xx>=0 && xx<f->w && yy>=0 && yy<f->h && y>=12 && y<120 &&
           wx>=0 && wx<MAP_COLS*16 && currently_visible(wx,wy)) {
            unsigned strength=src[(yy+f->y)*f->cw+frame_x+xx];
            if(strength) expected=sc64_tank_palette_remap[
                sc64_tank_raw_lut[(strength-1)*256+sc64_tank_reverse_palette[expected]]];
        }
        assert(gfx_test_pixel(x,y)==expected);
        changed+=expected!=before_pixels[y*240+x];
    }
    return changed;
}

static void fog_paths_match_the_oracle(void)
{
    static const struct { int image; unsigned frame; int cx, cy, direction; } cases[] = {
        { 533, 6, 120, 70, 0 }, { 533, 13, 40, 50, 0 }, { 537, 93, 100, 80, 20 },
        { 531, 4, 8, 20, 0 },   /* crosses the map's left edge */
        { 533, 3, 60, 4, 0 },   /* rows above world row 0 */
    };
    int partial=0, whole=0;
    for(unsigned i=0;i<sizeof(cases)/sizeof(cases[0]);++i)
        for(int pattern=0;pattern<4;++pattern) {
            reset_world();
            for(int r=0;r<MAP_ROWS;++r) for(int c=0;c<MAP_COLS;++c) {
                int v=pattern==0 ? 1 : pattern==1 ? ((r+c)&1) :
                      pattern==2 ? !(c==cases[i].cx/16 && r==cases[i].cy/16) : c>cases[i].cx/16;
                SC64_GRID_SET(visible,c+r*MAP_COLS,v);
            }
            patterned_backdrop();
            sc64_private_tank_blit(cases[i].image,cases[i].frame,cases[i].cx,cases[i].cy,cases[i].direction);
            int changed=assert_fogged_frame(cases[i].image,cases[i].frame,cases[i].cx,cases[i].cy,cases[i].direction);
            if(pattern) partial+=changed>0; else whole+=changed>0;
        }
    assert(whole==5 && partial>=10);
}

static void test_clock(void)
{
    assert((2*4)/5==1 && (3*4)/5==2 && (4*4)/5==3);
    assert((2*5+3)/4==3 && (3*5+3)/4==4);
}

int main(void)
{
    test_clock();
    assert_projected_lut_from_source();
    /* The normal weapon and Siege weapon each exercise tick_combat/add_tracer,
     * then the production tracer updater/renderer at source-derived ages. */
    fire_and_check(5,0,536,8,0,0,152,80);
    fire_and_check(5,0,536,8,0,0,48,80); /* mirrored direction */
    fire_and_check(5,3,0,0,531,0,152,80);
    fire_and_check(30,0,537,42,0,0,212,80);
    fire_and_check(30,4,537,93,533,1,212,80);
    fire_and_check(30,4,537,93,533,1,12,80); /* mirrored direction */
    fire_and_check(30,10,0,0,533,6,212,118);
    generic_weapon_still_renders();
    fog_paths_match_the_oracle();
    puts("Tank source VFX engine path, fog paths and raw-LUT oracle passed");
    return 0;
}
