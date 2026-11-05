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
            .path_timer=4000,.path_retry=3000,.path_x=4000,.path_y=3000};
    Unit wire, back, untouched;
    assert(sc64_scarab_waypoint_to_wire(&u,&wire,2048,2048,1));
    assert(wire.path_x==2000 && wire.path_y==1500 && !wire.path_timer && !wire.path_retry);
    assert(sc64_scarab_waypoint_from_wire(&wire,&back,2048,2048,1));
    assert(back.path_timer==u.path_timer && back.path_retry==u.path_retry &&
           back.path_x==u.path_x && back.path_y==u.path_y);
    Sc64ScarabSteer a={.x=120*256,.y=100*256,.heading=8,.velocity=8,
                       .age=10,.waypoint_known=1,.waypoint_x=u.path_timer,.waypoint_y=u.path_retry};
    Sc64ScarabSteer b=a; b.waypoint_x=back.path_timer; b.waypoint_y=back.path_retry;
    /* A new route is supplied at the first tick, including a changed route. */
    assert(sc64_scarab_steer_tick(&a,200,180,1));
    assert(sc64_scarab_steer_tick(&b,200,180,1));
    assert(a.x==b.x && a.y==b.y && a.heading==b.heading && a.velocity==b.velocity &&
           a.turning==b.turning && a.waypoint_x==b.waypoint_x && a.waypoint_y==b.waypoint_y);
    untouched=wire;
    u.path_timer=4001; assert(!sc64_scarab_waypoint_to_wire(&u,&wire,2048,2048,1));
    assert(!memcmp(&wire,&untouched,sizeof wire));
    u.path_timer=4096; assert(!sc64_scarab_waypoint_to_wire(&u,&wire,2048,2048,1));
    assert(!sc64_scarab_waypoint_from_wire(&untouched,&back,2048,2048,0));
    u.path_timer=4000; u.path_x=200;
    assert(!sc64_scarab_waypoint_to_wire(&u,&wire,2048,2048,1));
    u.weapon=1;
    assert(!sc64_scarab_waypoint_to_wire(&u,&wire,2048,2048,1));
    u.path_timer=u.path_retry=0; u.path_y=180; assert(sc64_scarab_waypoint_to_wire(&u,&wire,2048,2048,1));
    assert(sc64_scarab_waypoint_from_wire(&wire,&back,2048,2048,1));
    assert(!back.path_timer && !back.path_retry && back.path_x==200 && back.path_y==180);
    return 0;
}
