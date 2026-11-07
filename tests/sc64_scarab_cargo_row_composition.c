/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Actual profile row-codec witness only. No selector, journal or admission. */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/game.c"
#include "../starcraft/sc64_scarab_flight_bridge.h"
#include "../starcraft/sc64_scarab_fields.h"
#if !defined(GBA_HOST) || !defined(SC64_WORKER_CARGO_KIND) || !defined(SC64_SCARAB_RUNTIME) || defined(SC64_SCARAB_NAMESPACE_STAGE)
#error "Use the isolated cargo/Scarab row composition fixture"
#endif
static RaceSave row_candidate;
static int row_encode(unsigned slot,const Unit *unit)
{
#ifdef SC64_P7_TYPED_COMPACT_ENABLED
    return sc64_p7_typed_unit_encode(&row_candidate.units[slot],unit,2048,2048);
#else
    return sc64_zp_unit_save_encode(&row_candidate.units[slot],unit,2048,2048);
#endif
}
static int row_decode(unsigned slot,Unit *unit)
{
#ifdef SC64_P7_TYPED_COMPACT_ENABLED
    return sc64_p7_typed_unit_decode(unit,&row_candidate.units[slot],2048,2048);
#else
    return sc64_zp_unit_save_decode(unit,&row_candidate.units[slot],2048,2048);
#endif
}
int main(void)
{
    Unit worker,child,wire,decoded,restored;int carry,timer;
    gba_init();game_init();race_mode=1;race_human=R_PROTOSS;race_bots=1;race_skirmish_init();
    int w=spawn_unit(SIDE,U_SCV,100,100),c=spawn_unit(ENEMY,U_SCARAB,120,100);
    assert(w>=0 && c>=0);worker=u[w];child=u[c];
    worker.order=ORDER_GAS;worker.carry=8;worker.flags|=SC64_WORKER_CARGO_GAS;
    assert(sc64_worker_cargo_state_valid(&worker));
    assert(sc64_scarab_flight_to_wire(&worker,&wire,1,0));
    assert(row_encode(0,&wire) && row_decode(0,&decoded));
    assert(sc64_worker_cargo_state_valid(&decoded) && decoded.carry==8 &&
           (decoded.flags&SC64_WORKER_CARGO_GAS) && decoded.order==ORDER_GAS);
    for(unsigned phase=0;phase<=1;++phase) {
        Sc64ScarabChildFields f={(unsigned)w,phase,0,0};
        assert(sc64_scarab_child_fields_pack(&f,&carry,&timer));
        child.raw_type=85;child.raw_owner=-1;child.carry=carry;child.work_timer=timer;
        child.build=-1;child.order=ORDER_IDLE;child.target=-1;
        assert(sc64_worker_cargo_state_valid(&child));
        assert(sc64_scarab_flight_to_wire(&child,&wire,1,0));
        assert(row_encode(1,&wire) && row_decode(1,&decoded));
        assert(sc64_scarab_flight_from_wire(&decoded,&restored,1,0));
        assert(restored.type==child.type && restored.raw_type==85 &&
               restored.carry==child.carry && restored.work_timer==child.work_timer &&
               restored.build==-1 && sc64_worker_cargo_state_valid(&restored));
    }
    puts("actual worker cargo and Scarab row composition: PASS");return 0;
}
