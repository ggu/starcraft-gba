#ifndef SC64_WEAPON_VFX_RENDER_H
#define SC64_WEAPON_VFX_RENDER_H

#include <stdint.h>
#include "../common/gba.h"
#include "sc64_weapon_vfx.h"

/* Renderer-only authored palette motion. Every write goes through this
 * viewport gate; coordinates are already camera adjusted by the caller. */
#define SC64_WEAPON_VFX_VIEW_LEFT 0
#define SC64_WEAPON_VFX_VIEW_RIGHT 239
#define SC64_WEAPON_VFX_VIEW_TOP 12
#define SC64_WEAPON_VFX_VIEW_BOTTOM 119

#ifndef SC64_WEAPON_VFX_VISIBLE
#define SC64_WEAPON_VFX_VISIBLE(x,y) 1
#define SC64_WEAPON_VFX_VISIBLE_LOCAL_DEFAULT 1
#endif

static void
sc64_weapon_vfx_pixel(int64_t x, int64_t y, int color)
{
    if (x < SC64_WEAPON_VFX_VIEW_LEFT || x > SC64_WEAPON_VFX_VIEW_RIGHT ||
        y < SC64_WEAPON_VFX_VIEW_TOP || y > SC64_WEAPON_VFX_VIEW_BOTTOM)
        return;
    if (!SC64_WEAPON_VFX_VISIBLE((int)x,(int)y)) return;
    gfx_pixel((int)x, (int)y, (u8)color);
}

/* All authored line endpoints are at most 24 pixels apart. Plot through the
 * viewport gate rather than passing possibly off-screen coordinates to the
 * general line primitive. */
static void
sc64_weapon_vfx_line(int64_t x0, int64_t y0, int64_t x1, int64_t y1,
                     int color)
{
    int64_t dx=x1>=x0?x1-x0:x0-x1, sx=x0<x1?1:-1;
    int64_t dy=y1>=y0?y0-y1:y1-y0, sy=y0<y1?1:-1;
    int64_t err=dx+dy;
    for (;;) {
        sc64_weapon_vfx_pixel(x0,y0,color);
        if(x0==x1&&y0==y1) break;
        int64_t e2=2*err;
        if(e2>=dy){err+=dy;x0+=sx;}
        if(e2<=dx){err+=dx;y0+=sy;}
    }
}

static void
sc64_weapon_vfx_impact(int64_t x, int64_t y, int color, int phase)
{
    int radius=phase==1?2:1;
    sc64_weapon_vfx_line(x-radius,y,x+radius,y,color);
    sc64_weapon_vfx_line(x,y-radius,x,y+radius,color);
    sc64_weapon_vfx_pixel(x,y,C_WHITE);
    if(phase==1) {
        sc64_weapon_vfx_pixel(x-2,y-2,C_GOLD);
        sc64_weapon_vfx_pixel(x+2,y+2,C_GOLD);
    } else {
        sc64_weapon_vfx_pixel(x+1,y-1,C_GOLD);
    }
}

static void
sc64_weapon_vfx_blast(int64_t x,int64_t y,int color,int radius)
{
    /* Hollow expanding diamond with hot core and four sparks. Pixel work is
     * bounded by the authored radius (at most eight), including offscreen. */
    for(int i=0;i<=radius;++i) {
        int j=radius-i;
        sc64_weapon_vfx_pixel(x+i,y+j,color);
        sc64_weapon_vfx_pixel(x-i,y+j,color);
        sc64_weapon_vfx_pixel(x+i,y-j,color);
        sc64_weapon_vfx_pixel(x-i,y-j,color);
    }
    sc64_weapon_vfx_impact(x,y,C_GOLD,1);
    sc64_weapon_vfx_pixel(x-radius,y-radius,C_ORANGE);
    sc64_weapon_vfx_pixel(x+radius,y+radius,C_ORANGE);
}

static void
sc64_weapon_vfx_plasma_orb(int64_t x,int64_t y,int radius)
{
    sc64_weapon_vfx_pixel(x,y,C_CYAN);
    sc64_weapon_vfx_pixel(x-1,y,C_CYAN);
    sc64_weapon_vfx_pixel(x+1,y,C_CYAN);
    sc64_weapon_vfx_pixel(x,y-1,C_CYAN);
    sc64_weapon_vfx_pixel(x,y+1,C_CYAN);
    if(radius>1) {
        sc64_weapon_vfx_pixel(x-1,y-1,C_CYAN);
        sc64_weapon_vfx_pixel(x+1,y-1,C_CYAN);
        sc64_weapon_vfx_pixel(x-1,y+1,C_CYAN);
        sc64_weapon_vfx_pixel(x+1,y+1,C_CYAN);
    }
    sc64_weapon_vfx_pixel(x,y,C_WHITE);
}

static void
sc64_weapon_vfx_acid_blob(int64_t x,int64_t y,int x0,int y0,int x1,int y1)
{
    int64_t dx=(int64_t)x1-x0,dy=(int64_t)y1-y0;
    int64_t major=dx<0?-dx:dx;
    if((dy<0?-dy:dy)>major)major=dy<0?-dy:dy;
    int64_t tx=major?x-dx*3/major:x-3;
    int64_t ty=major?y-dy*3/major:y;
    sc64_weapon_vfx_pixel(x,y,C_GREEN);
    sc64_weapon_vfx_pixel(x-1,y,C_GREEN);
    sc64_weapon_vfx_pixel(x+1,y,C_GREEN);
    sc64_weapon_vfx_pixel(x,y-1,C_GREEN);
    sc64_weapon_vfx_pixel(x,y+1,C_GREEN);
    sc64_weapon_vfx_pixel(tx,ty,C_GREEN_DARK);
    sc64_weapon_vfx_pixel(tx-(major?dx/major:1),
                          ty-(major?dy/major:0),C_GREEN);
}

static void
sc64_weapon_vfx_shell_head(int64_t x,int64_t y,int x0,int y0,int x1,int y1)
{
    int64_t dx=(int64_t)x1-x0,dy=(int64_t)y1-y0;
    int64_t major=dx<0?-dx:dx;
    if((dy<0?-dy:dy)>major)major=dy<0?-dy:dy;
    int64_t tx=major?x-dx*2/major:x-2;
    int64_t ty=major?y-dy*2/major:y;
    sc64_weapon_vfx_pixel(x,y,C_ORANGE);
    sc64_weapon_vfx_pixel(x-1,y,C_ORANGE);
    sc64_weapon_vfx_pixel(x+1,y,C_ORANGE);
    sc64_weapon_vfx_pixel(x,y-1,C_ORANGE);
    sc64_weapon_vfx_pixel(x,y+1,C_ORANGE);
    sc64_weapon_vfx_pixel(x,y,C_WHITE);
    sc64_weapon_vfx_pixel(tx,ty,C_RED);
}

static void
sc64_weapon_vfx_move_point(int style, int x0, int y0, int x1, int y1,
                           int life, int *x, int *y)
{
    int duration=sc64_weapon_vfx_lifetime(style);
    int travel=duration-2;
    int phase=duration-life;
    int64_t dx=(int64_t)x1-x0,dy=(int64_t)y1-y0;
    if(phase<0)phase=0;
    if(phase>travel)phase=travel;
    *x=(int)((int64_t)x0+dx*phase/travel);
    *y=(int)((int64_t)y0+dy*phase/travel);
}

static void
sc64_weapon_vfx_moving(int style, int color, int x0, int y0, int x1, int y1,
                       int life)
{
    int hx,hy;
    int64_t tailx,taily;
    if(life<=2) {
        if(style==SC64_WEAPON_VFX_STYLE_SHELL || style==SC64_WEAPON_VFX_STYLE_MISSILE)
            sc64_weapon_vfx_blast(x1,y1,color,life==2?4:6);
        else if(style==SC64_WEAPON_VFX_STYLE_PLASMA)
            sc64_weapon_vfx_plasma_orb(x1,y1,life==2?1:2);
        else if(style==SC64_WEAPON_VFX_STYLE_ACID)
            sc64_weapon_vfx_acid_blob(x1,y1,x0,y0,x1,y1);
        else sc64_weapon_vfx_impact(x1,y1,color,life);
        return;
    }
    sc64_weapon_vfx_move_point(style,x0,y0,x1,y1,life,&hx,&hy);
    if(style==SC64_WEAPON_VFX_STYLE_PLASMA) {
        sc64_weapon_vfx_plasma_orb(hx,hy,1);
        return;
    }
    if(style==SC64_WEAPON_VFX_STYLE_ACID) {
        sc64_weapon_vfx_acid_blob(hx,hy,x0,y0,x1,y1);
        return;
    }
    if(style==SC64_WEAPON_VFX_STYLE_SHELL) {
        sc64_weapon_vfx_shell_head(hx,hy,x0,y0,x1,y1);
        return;
    }
    /* Recreate a short tail by stepping back along the signed travel vector.
     * Dominant-axis normalization keeps every line local and at most five. */
    int duration=sc64_weapon_vfx_lifetime(style),travel=duration-2;
    int phase=duration-life;
    int px=(int)((int64_t)x0+((int64_t)x1-x0)*(phase>0?phase-1:0)/travel);
    int py=(int)((int64_t)y0+((int64_t)y1-y0)*(phase>0?phase-1:0)/travel);
    int64_t vx=(int64_t)hx-px,vy=(int64_t)hy-py;
    int64_t major=vx<0?-vx:vx;
    if((vy<0?-vy:vy)>major)major=vy<0?-vy:vy;
    if(major>0) {
        tailx=(int64_t)hx-vx*5/major;
        taily=(int64_t)hy-vy*5/major;
    } else { tailx=(int64_t)hx-1; taily=hy; }
    sc64_weapon_vfx_line(tailx,taily,hx,hy,color);
    sc64_weapon_vfx_pixel(hx,hy,C_WHITE);
    if(style==SC64_WEAPON_VFX_STYLE_MISSILE) {
        sc64_weapon_vfx_pixel(hx,(int64_t)hy-1,C_ORANGE);
        sc64_weapon_vfx_pixel(tailx,taily+1,C_RED);
    } else if(style==SC64_WEAPON_VFX_STYLE_PLASMA) {
        sc64_weapon_vfx_pixel((int64_t)hx-1,hy,C_CYAN);
    } else if(style==SC64_WEAPON_VFX_STYLE_ACID) {
        sc64_weapon_vfx_pixel((int64_t)hx+1,hy,C_GREEN);
    } else if(style==SC64_WEAPON_VFX_STYLE_SHELL) {
        sc64_weapon_vfx_pixel(hx,(int64_t)hy+1,C_GOLD);
    } else {
        sc64_weapon_vfx_pixel(hx,(int64_t)hy-1,C_GOLD);
    }
}

/* The function is intentionally not GBA_HOT: these short authored effects
 * are optional presentation work and should remain in ROM. */
static void __attribute__((noinline))
sc64_weapon_vfx_draw(int style, int color, int x0, int y0,
                     int x1, int y1, int life)
{
    int duration=sc64_weapon_vfx_lifetime(style);
    if(duration==0||life<=0)return;
    if(life>duration)life=duration;
    if(style==SC64_WEAPON_VFX_STYLE_NEEDLE ||
       style==SC64_WEAPON_VFX_STYLE_MISSILE ||
       style==SC64_WEAPON_VFX_STYLE_PLASMA ||
       style==SC64_WEAPON_VFX_STYLE_ACID ||
       style==SC64_WEAPON_VFX_STYLE_SHELL) {
        sc64_weapon_vfx_moving(style,color,x0,y0,x1,y1,life);
        return;
    }
    if(style==SC64_WEAPON_VFX_STYLE_MUZZLE) {
        int sx=x1>x0?1:x1<x0?-1:0, sy=y1>y0?1:y1<y0?-1:0;
        sc64_weapon_vfx_pixel(x0,y0,C_WHITE);
        sc64_weapon_vfx_line((int64_t)x0+sx,(int64_t)y0+sy,
                             (int64_t)x0+sx*4,(int64_t)y0+sy*4,color);
        sc64_weapon_vfx_pixel((int64_t)x0-(int64_t)sy*2,
                              (int64_t)y0+(int64_t)sx*2,C_GOLD);
        if(life<=2)sc64_weapon_vfx_impact(x1,y1,color,life);
        return;
    }
    if(style==SC64_WEAPON_VFX_STYLE_FLAME) {
        int sx=x1>x0?1:x1<x0?-1:0, sy=y1>y0?1:y1<y0?-1:0;
        int spread=life&1?2:3;
        sc64_weapon_vfx_line(x0,y0,(int64_t)x0+(int64_t)sx*6-(int64_t)sy*spread,
                             (int64_t)y0+(int64_t)sy*6+(int64_t)sx*spread,color);
        sc64_weapon_vfx_line(x0,y0,(int64_t)x0+(int64_t)sx*5+(int64_t)sy*spread,
                             (int64_t)y0+(int64_t)sy*5-(int64_t)sx*spread,C_ORANGE);
        sc64_weapon_vfx_pixel(x0,y0,C_WHITE);
        return;
    }
    if(style==SC64_WEAPON_VFX_STYLE_SLASH) {
        if(life<=2)sc64_weapon_vfx_impact(x1,y1,color,life);
        else {
            sc64_weapon_vfx_line((int64_t)x1-4,(int64_t)y1+3,
                                 (int64_t)x1+4,(int64_t)y1-3,color);
            sc64_weapon_vfx_pixel(x1,y1,C_WHITE);
        }
        return;
    }
    if(style==SC64_WEAPON_VFX_STYLE_ARC) {
        if(life<=2) { sc64_weapon_vfx_impact(x1,y1,C_CYAN,life); return; }
        int px,py;
        sc64_weapon_vfx_move_point(style,x0,y0,x1,y1,life,&px,&py);
        int j=(life&1)?2:-2;
        sc64_weapon_vfx_line((int64_t)px-4,py,(int64_t)px-2,(int64_t)py+j,C_CYAN);
        sc64_weapon_vfx_line((int64_t)px-2,(int64_t)py+j,px,(int64_t)py-j,C_CYAN);
        sc64_weapon_vfx_line(px,(int64_t)py-j,(int64_t)px+2,(int64_t)py+j,C_CYAN);
        sc64_weapon_vfx_line((int64_t)px+2,(int64_t)py+j,(int64_t)px+4,py,C_CYAN);
        sc64_weapon_vfx_pixel(px,py,C_WHITE);
        return;
    }
    if(style==SC64_WEAPON_VFX_STYLE_BEAM) {
        if(life<=2) { sc64_weapon_vfx_impact(x1,y1,color,life); return; }
        int px,py;
        sc64_weapon_vfx_move_point(style,x0,y0,x1,y1,life,&px,&py);
        int64_t dx=(int64_t)x1-x0,dy=(int64_t)y1-y0;
        int64_t major=dx<0?-dx:dx;
        if((dy<0?-dy:dy)>major)major=dy<0?-dy:dy;
        int ox=major?(int)(dx*4/major):1,oy=major?(int)(dy*4/major):0;
        sc64_weapon_vfx_line((int64_t)px-ox,(int64_t)py-oy,
                            (int64_t)px+ox,(int64_t)py+oy,color);
        sc64_weapon_vfx_pixel(px,py,C_WHITE);
        return;
    }
    if(style==SC64_WEAPON_VFX_STYLE_BURST) {
        int r=life>4?3:life>2?6:8;
        sc64_weapon_vfx_blast(x1,y1,color,r);
        return;
    }
    if(style==SC64_WEAPON_VFX_STYLE_SPINES) {
        if(life<=2) { sc64_weapon_vfx_impact(x1,y1,color,life); return; }
        sc64_weapon_vfx_line((int64_t)x1-3,(int64_t)y1+2,x1,(int64_t)y1-3,color);
        sc64_weapon_vfx_line(x1,(int64_t)y1-3,(int64_t)x1+3,(int64_t)y1+2,color);
        sc64_weapon_vfx_pixel(x1,y1,C_WHITE);
    }
}

#ifdef SC64_WEAPON_VFX_VISIBLE_LOCAL_DEFAULT
#undef SC64_WEAPON_VFX_VISIBLE_LOCAL_DEFAULT
#undef SC64_WEAPON_VFX_VISIBLE
#endif

#endif
