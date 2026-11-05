/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Host-only component evidence, not a selected journal or guest save. */
#include <assert.h>
#include <string.h>
#include "../common/gba.h"
#include "../starcraft/sc64_scarab_steer_motion.h"
enum { U_SCARAB=51 };
/* Actual compact campaign runtime field types, including unsigned timers. */
typedef struct {
    int hp, maxhp, carry, shield;
    s16 x, y, tx, ty, build, target, path_x, path_y, raw_type, raw_owner;
    u16 cd, work_timer, weapon, path_timer, path_retry, flags, shield_remainder;
    u8 owner, type, alive, selected, order, armor;
} Unit;
#include "../starcraft/sc64_scarab_waypoint_bridge.h"

int main(void)
{
    Unit u={.type=U_SCARAB,.raw_type=85,.carry=2<<8,.weapon=2,
            .path_timer=4000,.path_retry=3000,.path_x=201,.path_y=180,.shield=7};
    Unit wire, back, untouched;
    assert(sc64_scarab_waypoint_to_wire(&u,&wire,2048,2048,15,SC64_SCARAB_WAYPOINT_TYPED));
    assert(wire.path_x>=0 && wire.path_x<=2047);
    assert(sc64_scarab_waypoint_from_wire(&wire,&back,2048,2048,15,SC64_SCARAB_WAYPOINT_TYPED));
    assert(back.path_timer==u.path_timer && back.path_retry==u.path_retry &&
           back.path_x==u.path_x && back.path_y==u.path_y && back.shield==u.shield);
    Sc64ScarabSteer a={.x=120*256,.y=100*256,.heading=8,.velocity=8,
                       .age=10,.waypoint_known=1,.waypoint_x=u.path_timer,.waypoint_y=u.path_retry};
    Sc64ScarabSteer b=a; b.waypoint_x=back.path_timer; b.waypoint_y=back.path_retry;
    /* A new route is supplied at the first tick, including a changed route. */
    assert(sc64_scarab_steer_tick(&a,200,180,1));
    assert(sc64_scarab_steer_tick(&b,200,180,1));
    assert(a.x==b.x && a.y==b.y && a.heading==b.heading && a.velocity==b.velocity &&
           a.turning==b.turning && a.waypoint_x==b.waypoint_x && a.waypoint_y==b.waypoint_y);
    untouched=wire;
    u.path_timer=4001; assert(!sc64_scarab_waypoint_to_wire(&u,&wire,2048,2048,15,SC64_SCARAB_WAYPOINT_TYPED));
    assert(!memcmp(&wire,&untouched,sizeof wire));
    u.path_timer=4096; assert(!sc64_scarab_waypoint_to_wire(&u,&wire,2048,2048,15,SC64_SCARAB_WAYPOINT_TYPED));
    assert(!sc64_scarab_waypoint_from_wire(&untouched,&back,2048,2048,0,SC64_SCARAB_WAYPOINT_TYPED));
    u.path_timer=4000; u.path_x=200; u.weapon=1;
    assert(sc64_scarab_waypoint_to_wire(&u,&wire,2048,2048,15,SC64_SCARAB_WAYPOINT_TYPED));
    assert(sc64_scarab_waypoint_from_wire(&wire,&back,2048,2048,15,SC64_SCARAB_WAYPOINT_TYPED));
    assert(back.path_x==u.path_x && back.path_timer==u.path_timer);
    u.path_timer=u.path_retry=0; u.path_y=180; assert(sc64_scarab_waypoint_to_wire(&u,&wire,2048,2048,15,SC64_SCARAB_WAYPOINT_TYPED));
    assert(sc64_scarab_waypoint_from_wire(&wire,&back,2048,2048,15,SC64_SCARAB_WAYPOINT_TYPED));
    assert(!back.path_timer && !back.path_retry && back.path_x==200 && back.path_y==180);
    u.flags=0; u.path_timer=200; u.path_retry=300;
    untouched=wire;
    const unsigned old_features[]={0,1,5,7};
    for (unsigned i=0;i<sizeof(old_features)/sizeof(old_features[0]);++i) {
        assert(!sc64_scarab_waypoint_to_wire(&u,&wire,2048,2048,old_features[i],SC64_SCARAB_WAYPOINT_TYPED));
        assert(!memcmp(&wire,&untouched,sizeof wire));
    }
    u.flags=1; assert(!sc64_scarab_waypoint_to_wire(&u,&wire,2048,2048,15,SC64_SCARAB_WAYPOINT_TYPED));
    u.flags=0; u.shield=11; assert(!sc64_scarab_waypoint_to_wire(&u,&wire,2048,2048,15,SC64_SCARAB_WAYPOINT_TYPED));
    u.shield=0;
    /* Check the complete mixed-radix domain at boundaries and deterministic
     * sampled combinations, including different current/prior waypoints. */
    unsigned state=0x12345678u;
    for (int codec=SC64_SCARAB_WAYPOINT_DERIVED; codec<=SC64_SCARAB_WAYPOINT_TYPED; ++codec) {
        for (unsigned i=0; i<100000; ++i) {
#define NEXT() (state=state*1664525u+1013904223u)
            u.type=U_SCARAB; u.raw_type=85; u.carry=2<<8; u.weapon=(u16)(NEXT()&3u);
            u.path_timer=(u16)((NEXT()&2047u)*2u); u.path_retry=(u16)((NEXT()&2047u)*2u);
            u.path_x=(s16)(NEXT()&4095u); u.path_y=(s16)(NEXT()&4095u);
            u.shield=(int)(NEXT()%11u); u.flags=0;
            assert(sc64_scarab_waypoint_to_wire(&u,&wire,2048,2048,15,codec));
            assert(sc64_scarab_waypoint_from_wire(&wire,&back,2048,2048,15,codec));
            assert(back.path_x==u.path_x && back.path_y==u.path_y &&
                   back.path_timer==u.path_timer && back.path_retry==u.path_retry &&
                   back.shield==u.shield && back.flags==0 && back.weapon==u.weapon);
        }
        u.path_timer=4094;u.path_retry=4094;u.path_x=4095;u.path_y=4095;u.shield=10;u.flags=0;
        assert(sc64_scarab_waypoint_to_wire(&u,&wire,2048,2048,15,codec));
        assert(sc64_scarab_waypoint_from_wire(&wire,&back,2048,2048,15,codec));
        assert(back.path_x==4095 && back.path_y==4095 && back.path_timer==4094 &&
               back.path_retry==4094 && back.shield==10);
        /* Largest possible digits exceed the canonical state count. */
        wire.path_x=wire.path_y=2047;wire.path_timer=24;wire.path_retry=12;
        wire.flags=(u16)(codec==SC64_SCARAB_WAYPOINT_DERIVED?7:2047);
        wire.shield=codec==SC64_SCARAB_WAYPOINT_DERIVED?131071:511;
        untouched=back;
        assert(!sc64_scarab_waypoint_from_wire(&wire,&back,2048,2048,15,codec));
        assert(!memcmp(&back,&untouched,sizeof back));
#undef NEXT
    }
    return 0;
}
