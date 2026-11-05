/* SPDX-License-Identifier: GPL-3.0-or-later */
/* HOST component fixture: exact P10 candidate layout; no guest saves. */
#include <assert.h>
#include <stdint.h>
#include <string.h>
typedef uint16_t u16; typedef int16_t s16;
#define SC64_SHARED_ACTOR_PROFILE_CODEC_HOST_STAGE 1
#define SC64_P7_TYPED_COMPACT_ENABLED 1
enum { U_SCARAB=43 };
typedef struct { int hp,maxhp,carry,shield; s16 x,y,tx,ty,build,target,path_x,path_y,raw_type,raw_owner; u16 cd,work_timer,weapon,path_timer,path_retry,flags,shield_remainder; unsigned char owner,type,alive,selected,order,armor; } Unit;
#include "../starcraft/sc64_scarab_waypoint_bridge.h"
int main(void)
{
    Unit q={0}, wire, out, sentinel;
    q.type=U_SCARAB;q.raw_type=85;q.carry=2<<8;q.alive=1;q.weapon=0xc7a5;
    for(unsigned motion=0;motion<4;++motion)
      for(unsigned x=0;x<2;++x) for(unsigned y=0;y<2;++y)
       for(unsigned px=0;px<2;++px) for(unsigned py=0;py<2;++py)
        for(int shield=0;shield<=10;++shield) {
          q.path_x=(s16)(x?4095:0);q.path_y=(s16)(y?4095:0);
          q.path_timer=(u16)((px?4094:0)|(motion<<14));q.path_retry=(u16)(py?4094:0);
          q.shield=shield;
          assert(sc64_scarab_waypoint_to_wire(&q,&wire,2048,2048,15,SC64_SCARAB_WAYPOINT_TYPED));
          assert(sc64_scarab_waypoint_from_wire(&wire,&out,2048,2048,15,SC64_SCARAB_WAYPOINT_TYPED));
          assert(out.path_x==q.path_x && out.path_y==q.path_y && out.path_timer==q.path_timer &&
                 out.path_retry==q.path_retry && out.shield==q.shield && out.weapon==q.weapon && !out.flags);
        }
    wire.path_x=wire.path_y=2047;wire.path_timer=24;wire.path_retry=12;
    wire.shield=511;wire.flags=8191;memset(&out,0xa5,sizeof out);sentinel=out;
    assert(!sc64_scarab_waypoint_from_wire(&wire,&out,2048,2048,15,SC64_SCARAB_WAYPOINT_TYPED));
    assert(!memcmp(&out,&sentinel,sizeof out));
    assert(!sc64_scarab_waypoint_to_wire(&q,&wire,2048,2048,7,SC64_SCARAB_WAYPOINT_TYPED));
    q.path_timer|=0x1000u;
    assert(!sc64_scarab_waypoint_to_wire(&q,&wire,2048,2048,15,SC64_SCARAB_WAYPOINT_TYPED));
    return 0;
}
