/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Source Tank compositor using the shared destination-dependent blitter. */
#ifndef SC64_TANK_SOURCE_VFX_RENDER_H
#define SC64_TANK_SOURCE_VFX_RENDER_H
#include "sc64_tank_source_vfx_generated.h"
#if !defined(SC64_TANK_SOURCE_VFX_VERSION) || SC64_TANK_SOURCE_VFX_VERSION != 1
#error "Regenerate the versioned source Tank effect bank"
#endif
static const Sc64TankSourceFrame *sc64_private_tank_frame(int image,unsigned frame)
{
    if(image==531) return frame<10 ? &sc64_tank_frames_531[frame] : 0;
    if(image==533) return frame<14 ? &sc64_tank_frames_533[frame] : 0;
    if(image==536) return frame<17 ? &sc64_tank_frames_536[frame] : 0;
    if(image==537) return frame<119 ? &sc64_tank_frames_537[frame] : 0;
    return 0;
}
static void sc64_private_tank_blit(int image,unsigned frame_index,int world_x,int world_y,int direction)
{
    const Sc64TankSourceFrame *f=sc64_private_tank_frame(image,frame_index); if(!f) return;
    int flip=direction>16;
    const u8 *src=sc64_tank_source_pixels+(flip?f->flip_offset:f->offset);
    int left=world_x-camx-(int)f->cw/2;
    int top=world_y-camy+12-(int)f->ch/2+f->y;
    int first=flip?f->flip_x:f->x, last=first+f->w;
    for(int row=0;row<f->h;++row) {
        int sy=top+row, col=0;
        if(sy<12 || sy>=120) continue;
        while(first+col<last) {
            int wx=left+first+col+camx, wy=sy+camy-12;
            int vis=wx>=0 && wx<MAP_COLS*16 && currently_visible(wx,wy);
            int run;
            if(wx<0) run=-wx;
            else run=16-(wx&15);
            if(run>last-first-col) run=last-first-col;
            if(run<=0) run=1;
            if(vis) gfx_blit_sprite_mapped(left+first+col,sy,run,1,
                src+(row+f->y)*f->cw+first+col,sc64_tank_projected_lut,63);
            col+=run;
        }
    }
}
/* life is aged by update_tracers: first rendered frame has age 0. */
static void sc64_private_tank_vfx_draw(int style,int direction,int x0,int y0,int x1,int y1,int life)
{
    int duration=style==SC64_TANK_SOURCE_SIEGE?20:28, age=duration-life;
    if(age<0) age=0; if(age>=duration) return;
    int source_age=(age*4)/5; /* BW source updates at 24 Hz; GBA update is 30 Hz. */
    if(style==SC64_TANK_SOURCE_TANK) {
        int d=direction&31, frame=d<=16?d:32-d;
        if(age==0) sc64_private_tank_blit(536,(unsigned)frame,x0,y0,d);
        if(source_age>=2 && source_age<22) { unsigned impact=(unsigned)((source_age-2)/2); if(impact>9)impact=9; sc64_private_tank_blit(531,impact,x1,y1,0); }
    } else if(style==SC64_TANK_SOURCE_SIEGE) {
        int d=direction&31, frame=d<=16?d:32-d;
        if(source_age<5) {
            unsigned pose=(unsigned)(2+source_age); if(pose>6)pose=6;
            unsigned parent=(unsigned)frame; /* match the GBA parent renderer frame (0..16) */
            int lo_x=sc64_tank_siege_lo[parent][0], lo_y=sc64_tank_siege_lo[parent][1];
            if(d>16) lo_x=-lo_x;
            sc64_private_tank_blit(537,pose*17u+(unsigned)frame,x0+lo_x,y0+lo_y,d);
        }
        if(source_age>=2 && source_age<16) { unsigned impact=(unsigned)(source_age-2); if(impact>13)impact=13; sc64_private_tank_blit(533,impact,x1,y1,0); }
    }
}
#endif
