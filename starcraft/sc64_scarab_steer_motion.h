/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_SCARAB_STEER_MOTION_H
#define SC64_SCARAB_STEER_MOTION_H
/* Experimental source-Q8 clear-waypoint slice. A reference path provider must
 * supply the next waypoint and certify clear space on every call. This does
 * not implement source pathfinding, actor range, IScript or bullets. */
/* The existing kernel also defines one non-static frame helper. Give this
 * experimental translation unit its own name to avoid a link collision. */
#define sc64_interceptor_source_frame_due sc64_scarab_probe_source_frame_due
#include "sc64_interceptor_motion_kernel.h"
#undef sc64_interceptor_source_frame_due
typedef struct { s32 x,y;u8 heading,velocity,turning,age;int waypoint_x,waypoint_y;u8 waypoint_known; } Sc64ScarabSteer;
static int sc64_scarab_steer_tick(Sc64ScarabSteer *s,int wx,int wy,int clear)
{
    if (!s || !clear || s->age>89 || wx<0 || wy<0 || wx>32767 || wy>32767 ||
        s->x<0 || s->y<0 || s->x>0x7fffff || s->y>0x7fffff) return 0;
    if (s->waypoint_known && (s->waypoint_x!=wx || s->waypoint_y!=wy)) s->turning=1;
    s->waypoint_x=wx;s->waypoint_y=wy;s->waypoint_known=1;
    if (s->age>=2) {
        int px=sc64_interceptor_integer_part(s->x),py=sc64_interceptor_integer_part(s->y);
        int dx=wx-px,dy=wy-py;u8 desired=dx||dy?sc64_interceptor_dir(dx,dy):s->heading;
        int can_move=!s->turning || sc64_interceptor_abs32(sc64_interceptor_turn_delta(s->heading,desired))<32;
        sc64_interceptor_toward(&s->velocity,desired,27);
        if (s->turning) {
            sc64_interceptor_toward(&s->heading,desired,27);
            if (s->heading==desired && s->velocity==desired) s->turning=0;
        }
        if (s->age>=3 && can_move && (dx||dy)) {
            s32 qx=(s32)wx*256-s->x,qy=(s32)wy*256-s->y;
            if (sc64_interceptor_len_raw(qx,qy)<=16*256) {s->x=(s32)wx*256;s->y=(s32)wy*256;}
            else {
                Sc64InterceptorDirectionVector v=sc64_interceptor_direction_vector[s->velocity];
                s->x+=sc64_interceptor_integer_part((s32)v.x*(16*256));
                s->y+=sc64_interceptor_integer_part((s32)v.y*(16*256));
            }
        }
    }
    ++s->age;return 1;
}
#endif
