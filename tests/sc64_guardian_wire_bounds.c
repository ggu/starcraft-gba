/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Signed-field helper contract; real product tests use the actual Unit layout. */
#include <assert.h>
#define SC64_GUARDIAN_COCOON_STAGE 1
enum { U_GUARDIAN_COCOON=64, U_OVERLORD=8, ORDER_IDLE=0 };
typedef struct { int type,raw_type,hp,maxhp,target,work_timer,build,carry,flags,order,armor,shield,alive; } Unit;
#include "../starcraft/sc64_guardian_wire.h"
int main(void)
{
    Unit q={64,59,120,200,44,1,-1,0,0,0,0,0,1},w;
    assert(sc64_guardian_wire_encode(&q,&w,1));
    q.alive=-1;assert(!sc64_guardian_wire_encode(&q,&w,1));
    q.alive=0;q.hp=0;q.work_timer=-1;assert(!sc64_guardian_wire_encode(&q,&w,1));
    q.work_timer=0;assert(sc64_guardian_wire_encode(&q,&w,1));
    return 0;
}
