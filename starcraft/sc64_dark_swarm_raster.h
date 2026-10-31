/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_DARK_SWARM_RASTER_H
#define SC64_DARK_SWARM_RASTER_H
#include "dark_swarm_source_generated.h"
/* Image 337, IScript 299: Init frames 0..4 wait one; the subsequent
 * 5,6,7,8,9,8,7,6 loop waits three per frame. The inactive reservation
 * tail renders Death frames 4..1; the final post-retirement image is open. */
static unsigned sc64_dark_swarm_raster_frame(unsigned remaining)
{
    if(remaining>900) return remaining-900;
    unsigned age=900u-remaining;
    if(age<5)return age;
    static const u8 frames[8]={5,6,7,8,9,8,7,6};
    return frames[((age-5u)/3u)%8u];
}
static void sc64_dark_swarm_raster_draw(const Sc64DarkSwarmEffect *effect)
{
    if(!effect || !effect->remaining)return;
    unsigned frame=sc64_dark_swarm_raster_frame(effect->remaining);
    if(frame>=SC64_DARK_SWARM_FRAME_COUNT)return;
    const Sc64DarkSwarmFrame *f=&sc64_dark_swarm_frames[frame];
    Sc64WeaponSourceVfxRaster r={sc64_dark_swarm_pixels+f->offset,0,sc64_dark_swarm_light_lut,
        f->image,f->x,f->y,(int16_t)(f->cw-f->x-f->w),f->w,f->h,f->cw,f->ch,
        f->modifier,SC64_DARK_SWARM_LIGHT_ROWS};
    sc64_weapon_source_vfx_draw_raster(&r,effect->x/2,effect->y/2,0);
}
#endif
