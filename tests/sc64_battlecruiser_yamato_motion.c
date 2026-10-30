/* SPDX-License-Identifier: GPL-3.0-or-later */
#define SC64_BATTLECRUISER_YAMATO_EXPERIMENT
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
typedef int8_t s8; typedef uint8_t u8; typedef int16_t s16; typedef uint16_t u16; typedef int32_t s32; typedef uint32_t u32;
#include "../starcraft/sc64_battlecruiser_yamato_motion.h"
/* Test-only copy of the existing project motion kernel, with flingy 158's
 * verified speed/acceleration substituted. */
#include "sc64_yamato_motion_reference.h"
int main(void) {
    uint32_t seed=7193;
    for (int i=0;i<20000;++i) {
#define NEXT() (seed=seed*1664525u+1013904223u)
        Sc64BcMotionFields m={0}; Sc64InterceptorMotion p={0};
        m.x_q8=(700u+(NEXT()%2500u))*256u; m.x_q8+=NEXT()&255u;
        m.y_q8=(700u+(NEXT()%2500u))*256u; m.y_q8+=NEXT()&255u;
        m.destination_x=(uint16_t)(700u+NEXT()%2500u);
        m.destination_y=(uint16_t)(700u+NEXT()%2500u);
        m.speed=(uint16_t)(NEXT()%8534u);m.turn_rate=(uint16_t)(1u+NEXT()%510u);
        m.heading=(uint8_t)(NEXT()>>24);m.velocity_direction=(uint8_t)(NEXT()>>24);
        m.turning=(uint8_t)(NEXT()>>31); m.lifetime=(uint8_t)(1u+NEXT()%254u);
        p.x_q8=(s32)m.x_q8;p.y_q8=(s32)m.y_q8;p.destination_x=m.destination_x;p.destination_y=m.destination_y;
        p.next_speed=m.speed;p.flingy_turn_rate_raw=m.turn_rate;p.heading=m.heading;
        p.next_velocity_direction=m.velocity_direction;p.movement_flags=m.turning?1:0;
        p.remaining_time=m.lifetime;p.state=SC64_INTERCEPTOR_MOTION_MOVE;p.alive=1;
        p.target_ref=1;p.owner_ref=1;p.map_width_px=p.map_height_px=4096;
        int actual=sc64_yamato_motion_step(&m,4096,4096);
        sc64_interceptor_projectile_tick(&p,1,1);
        if (!(actual>=0 && m.x_q8==(u32)p.x_q8 && m.y_q8==(u32)p.y_q8 &&
          m.speed==p.next_speed && m.heading==p.heading && m.velocity_direction==p.next_velocity_direction &&
          m.turn_rate==p.flingy_turn_rate_raw && m.turning==!!(p.movement_flags&1) &&
          m.lifetime==p.remaining_time && actual==(p.state==SC64_INTERCEPTOR_MOTION_DYING))) {
          fprintf(stderr,"i=%d x=%u/%d y=%u/%d speed=%u/%u heading=%u/%u vel=%u/%u rate=%u/%u turning=%u/%u life=%u/%d impact=%d/%d\n", i,m.x_q8,p.x_q8,m.y_q8,p.y_q8,m.speed,p.next_speed,m.heading,p.heading,m.velocity_direction,p.next_velocity_direction,m.turn_rate,p.flingy_turn_rate_raw,m.turning,!!(p.movement_flags&1),m.lifetime,p.remaining_time,actual,p.state==SC64_INTERCEPTOR_MOTION_DYING); return 1; }
    }
    /* Resume a moving shot from its 20-byte representation at every tick. */
    for (int i=0;i<1000;++i) {
        Sc64BcMotionFields uninterrupted={0}, resumed;
        uninterrupted.x_q8=(700u+NEXT()%2500u)*256u;
        uninterrupted.y_q8=(700u+NEXT()%2500u)*256u;
        uninterrupted.destination_x=(uint16_t)(700u+NEXT()%2500u);
        uninterrupted.destination_y=(uint16_t)(700u+NEXT()%2500u);
        uninterrupted.turn_rate=127; uninterrupted.heading=(uint8_t)(NEXT()>>24);
        uninterrupted.velocity_direction=(uint8_t)(NEXT()>>24);
        uninterrupted.lifetime=255; uninterrupted.turning=1; resumed=uninterrupted;
        for (int n=0;n<256;++n) {
            Sc64BcYamatoShot packed={.state=SC64_BC_SHOT_FLIGHT};
            assert(sc64_bc_motion_pack(&packed,&resumed));
            resumed=(Sc64BcMotionFields){0};
            assert(sc64_bc_motion_unpack(&packed,&resumed));
            int a=sc64_yamato_motion_step(&uninterrupted,4096,4096),b=sc64_yamato_motion_step(&resumed,4096,4096);
            assert(a==b && a>=0 && uninterrupted.x_q8==resumed.x_q8 &&
                uninterrupted.y_q8==resumed.y_q8 && uninterrupted.speed==resumed.speed &&
                uninterrupted.turn_rate==resumed.turn_rate && uninterrupted.heading==resumed.heading &&
                uninterrupted.velocity_direction==resumed.velocity_direction &&
                uninterrupted.lifetime==resumed.lifetime && uninterrupted.turning==resumed.turning);
            if (a) break;
        }
    }
    for (int side=0;side<2;++side) {
        Sc64BcMotionFields m={0}; Sc64InterceptorMotion p={0};
        m.x_q8=side?4095u*256u:0; m.y_q8=100u*256u;
        m.destination_x=(int16_t)(side?4125:-30);m.destination_y=100;
        m.turn_rate=127;m.heading=m.velocity_direction=(uint8_t)(side?64:192);
        m.lifetime=255;m.turning=1;
        p.x_q8=(s32)m.x_q8;p.y_q8=(s32)m.y_q8;p.destination_x=m.destination_x;p.destination_y=m.destination_y;
        p.flingy_turn_rate_raw=m.turn_rate;p.heading=m.heading;p.next_velocity_direction=m.velocity_direction;
        p.movement_flags=1;p.remaining_time=255;p.state=SC64_INTERCEPTOR_MOTION_MOVE;p.alive=1;
        p.target_ref=p.owner_ref=1;p.map_width_px=p.map_height_px=4096;
        Sc64BcYamatoShot row={.state=SC64_BC_SHOT_FLIGHT}; Sc64BcMotionFields roundtrip;
        assert(sc64_bc_motion_pack(&row,&m) && sc64_bc_motion_unpack(&row,&roundtrip));
        assert(roundtrip.destination_x==m.destination_x);
        int actual=sc64_yamato_motion_step(&m,4096,4096);
        sc64_interceptor_projectile_tick(&p,1,1);
        if (!(actual==1 && p.state==SC64_INTERCEPTOR_MOTION_DYING &&
               m.x_q8==(u32)p.x_q8 && m.y_q8==(u32)p.y_q8)) {
            fprintf(stderr,"edge side=%d result=%d/%d pos=%u,%u/%d,%d dest=%d,%d life=%u/%d\n",side,actual,p.state,m.x_q8,m.y_q8,p.x_q8,p.y_q8,m.destination_x,m.destination_y,m.lifetime,p.remaining_time);return 1;
        }
    }
    puts("Yamato private source-step adapter: reference, packed resumes and off-map endpoints pass");
}
