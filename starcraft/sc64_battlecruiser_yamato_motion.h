/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_BATTLECRUISER_YAMATO_MOTION_H
#define SC64_BATTLECRUISER_YAMATO_MOTION_H
/* Adaptation of this project's independently tested interceptor movement
 * kernel to flingy 158's authenticated speed/acceleration. This models one
 * source movement tick only; launch and IScript scheduling are caller work. */
#include "sc64_battlecruiser_yamato_motion_codec.h"
#include "sc64_interceptor_direction_tables.h"
static int sc64_yamato_abs(int x) { return x<0?-x:x; }
static int sc64_yamato_integer(int x) { return x>=0?x/256:-(((-x)+255)/256); }
static int sc64_yamato_length(int x,int y) {
    unsigned a=(unsigned)sc64_yamato_abs(x),b=(unsigned)sc64_yamato_abs(y),t;
    if (a<b) {t=a;a=b;b=t;}
    if (a/4u<b) a=a-a/16u+b*3u/8u-a/64u+b*3u/256u;
    return (int)a;
}
static unsigned char sc64_yamato_direction(int x,int y) {
    int q,num=y,den=x; unsigned mag; unsigned char r;
    if (!x) return y<=0?0:128;
    if (den<0) {num=-num;den=-den;}
    q=num*256/den; mag=(unsigned)sc64_yamato_abs(q);
    for (r=0;r<64 && sc64_interceptor_tangent_cutoff[r]<mag;++r) {}
    return (unsigned char)(x>0?64+(q<0?-(int)r:(int)r): -64+(q<0?-(int)r:(int)r));
}
static int sc64_yamato_turn_delta(unsigned char from,unsigned char to) {
    int d=(unsigned char)(to-from); return d>=128?d-256:d;
}
/* Return 1 on arrival/expiry, 0 while moving, -1 on invalid input. */
static int sc64_yamato_motion_step(Sc64BcMotionFields *m,unsigned width,unsigned height)
{
    int px,py,dx,dy,d,turn,desired,delta,speed,vx,vy,remain;
    if (!m || !width || width>4096 || !height || height>4096 ||
        m->x_q8>=width*256u || m->y_q8>=height*256u ||
        m->destination_x< -32 || m->destination_x>4159 ||
        m->destination_y< -32 || m->destination_y>4159 ||
        m->speed>8533 || m->turn_rate==0 || m->turn_rate>510 || m->turning>1) return -1;
    px=(int)(m->x_q8/256); py=(int)(m->y_q8/256);
    dx=(int)m->destination_x-px; dy=(int)m->destination_y-py;
    desired=dx||dy?sc64_yamato_direction(dx,dy):m->heading;
    delta=sc64_yamato_turn_delta(m->velocity_direction,(unsigned char)desired);
    turn=m->turn_rate/2;
    if (delta>turn) delta=turn; else if (delta< -turn) delta= -turn;
    m->velocity_direction=(unsigned char)(m->velocity_direction+delta);
    speed=m->speed;
    if (!dx && !dy) speed=0;
    else {
        d=sc64_yamato_length(dx,dy);
        if (m->velocity_direction==desired || d>=32) speed+=8533;
        else {
            int diff=sc64_yamato_abs(sc64_yamato_turn_delta(m->velocity_direction,(unsigned char)desired));
            int quotient=(diff*2+(int)m->turn_rate-1)/(int)m->turn_rate;
            int val=sc64_yamato_integer(quotient*(int)m->speed);
            if (((unsigned)val*3u)/2u<=(unsigned)d) speed+=8533;
            else speed=speed>8533?speed-8533:0;
        }
        if (speed>8533) speed=8533;
    }
    if (speed && (dx||dy)) {
        Sc64InterceptorDirectionVector v=sc64_interceptor_direction_vector[m->velocity_direction];
        vx=sc64_yamato_integer((int)v.x*speed); vy=sc64_yamato_integer((int)v.y*speed);
        remain=sc64_yamato_length((int)m->destination_x*256-(int)m->x_q8,
                                  (int)m->destination_y*256-(int)m->y_q8);
        int x=speed>=remain?(int)m->destination_x*256:(int)m->x_q8+vx;
        int y=speed>=remain?(int)m->destination_y*256:(int)m->y_q8+vy;
        int ix=sc64_yamato_integer(x),iy=sc64_yamato_integer(y);
        if (ix<0 || iy<0 || ix>=(int)width || iy>=(int)height) {
            m->lifetime=0;
            if (ix<0) x=0; else if (ix>=(int)width) x=((int)width-1)*256;
            if (iy<0) y=0; else if (iy>=(int)height) y=((int)height-1)*256;
        }
        m->x_q8=(uint32_t)x; m->y_q8=(uint32_t)y;
    }
    m->speed=(uint16_t)speed;
    if ((!dx&&!dy)||m->turning) {
        int max=(int)(m->turn_rate&255u),min=(-((int)m->turn_rate))&255;
        if (max>=128) max-=256;
        if (min>=128) min-=256;
        turn=sc64_yamato_turn_delta(m->heading,(unsigned char)desired);
        if (turn>max) turn=max; else if (turn<min) turn=-max;
        m->heading=(unsigned char)(m->heading+turn);
        ++m->turn_rate;
        if (m->velocity_direction==desired && m->heading==desired) m->turning=0;
    }
    if (m->lifetime--==0) return 1;
    return (m->x_q8/256u==m->destination_x && m->y_q8/256u==m->destination_y)?1:0;
}
#endif
