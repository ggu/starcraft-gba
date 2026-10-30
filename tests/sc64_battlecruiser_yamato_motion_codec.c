/* SPDX-License-Identifier: GPL-3.0-or-later */
#define SC64_BATTLECRUISER_YAMATO_EXPERIMENT
#include "../starcraft/sc64_battlecruiser_yamato_motion_codec.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void) {
    Sc64BcYamatoShot row={.state=SC64_BC_SHOT_FLIGHT};
    uint32_t seed=123456789u;
    for (int i=0;i<100000;++i) {
#define NEXT() (seed=seed*1664525u+1013904223u)
        Sc64BcMotionFields a={0},b={0};
        a.x_q8=NEXT()&0xfffffu; a.y_q8=NEXT()&0xfffffu;
        a.destination_x=(int16_t)((int)(NEXT()%4192u)-32); a.destination_y=(int16_t)((int)(NEXT()%4192u)-32);
        a.speed=(NEXT()&1u)?8533:0; a.turn_rate=(uint16_t)(NEXT()&511u);
        a.heading=(uint8_t)(NEXT()>>24); a.velocity_direction=(uint8_t)(NEXT()>>24);
        a.lifetime=(uint8_t)(NEXT()>>24); a.turning=(uint8_t)(NEXT()>>31);
        assert(sc64_bc_motion_pack(&row,&a) && sc64_bc_motion_unpack(&row,&b));
        assert(a.x_q8==b.x_q8 && a.y_q8==b.y_q8 &&
               a.destination_x==b.destination_x && a.destination_y==b.destination_y &&
               a.speed==b.speed && a.turn_rate==b.turn_rate && a.heading==b.heading &&
               a.velocity_direction==b.velocity_direction && a.lifetime==b.lifetime &&
               a.turning==b.turning);
    }
    Sc64BcMotionFields bad={.speed=1};
    Sc64BcYamatoShot original=row;
    assert(!sc64_bc_motion_pack(&row,&bad) && !memcmp(&row,&original,sizeof row));
    puts("private 20-byte Yamato flight field codec: 100000 round trips pass");
}
