/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Authored full-trip state checks, not an observation of native timing. */
#define main worker_gather_fixture_main
#include "sc64_worker_gather_game.c"
#undef main

static void cycle(int race, int worker_type, int depot_type, int refinery_type, int is_gas)
{
    reset_scene(race, race == R_TER ? R_ZERG : R_TER);
    int w = spawn_unit(SIDE, worker_type, 128, 96);
    assert(w >= 0);
    set_depot(0, SIDE, depot_type, 128, 96);
    set_depot(1, SIDE, refinery_type, 128, 96);
    m[2].x=128; m[2].y=96; m[2].amount=100;
    u[w].order=is_gas ? ORDER_GAS : ORDER_MINE;
    u[w].target=is_gas ? 1 : 2;
    u[w].build=-1; u[w].work_timer=0;
    int duration=is_gas ? 60 : 45;
    for(int trip=0; trip<2; ++trip) {
        for(int i=0; i<duration-1; ++i) {
            one_tick();
            assert(!u[w].carry && minerals==(is_gas?0:trip*8) && gas==(is_gas?trip*8:0));
            if (!is_gas) assert(m[2].amount==100-trip*8);
        }
        one_tick(); assert(u[w].carry==8 && u[w].work_timer==0);
        if (!is_gas) assert(m[2].amount==100-(trip+1)*8);
        assert(minerals==(is_gas?0:trip*8) && gas==(is_gas?trip*8:0));
        one_tick(); assert(!u[w].carry && u[w].work_timer==0);
        assert(minerals==(is_gas?0:(trip+1)*8) && gas==(is_gas?(trip+1)*8:0));
    }
    /* Progress is frozen during travel. */
    u[w].work_timer=9; u[w].x=256; one_tick(); assert(u[w].work_timer==9);
    u[w].x=128; u[w].y=96;
    /* A legacy saved partial wait resumes without an extra initialization. */
    u[w].work_timer=2; one_tick(); assert(u[w].work_timer==1 && !u[w].carry);
    one_tick(); assert(u[w].carry==8 && !u[w].work_timer);
    one_tick();
    /* STOP cancels work; the next explicit assignment starts fresh. */
    one_tick(); assert(u[w].work_timer==(unsigned)(duration-1));
    u[w].selected=1; stop_selected(); assert(!u[w].work_timer && u[w].order==ORDER_IDLE);
}
static void mismatch_and_no_home(int race, int worker_type)
{
    reset_scene(race, race == R_TER ? R_ZERG : R_TER);
    int w=spawn_unit(SIDE,worker_type,128,96); assert(w>=0);
    m[2].x=128; m[2].y=96; m[2].amount=5;
    u[w].order=ORDER_MINE; u[w].target=2; u[w].build=-1;
    u[w].carry=6; u[w].flags |= SC64_WORKER_CARGO_GAS | SC64_WORKER_GATHER_PENDING;
    for(int i=0;i<44;i++) {
        one_tick(); assert(u[w].carry==6 && m[2].amount==5 && !minerals && !gas);
        assert(sc64_worker_cargo_is_gas(&u[w]));
    }
    one_tick(); assert(u[w].carry==5 && m[2].amount==0);
    assert(!sc64_worker_cargo_is_gas(&u[w]) && !minerals && !gas);
    one_tick(); assert(u[w].carry==5 && !minerals && !gas); /* no home */
}

int main(void)
{
    cycle(R_TER,U_SCV,B_COMMAND,B_REFINERY,0);
    cycle(R_TER,U_SCV,B_COMMAND,B_REFINERY,1);
    cycle(R_ZERG,U_DRONE,B_HATCHERY,B_EXTRACTOR,0);
    cycle(R_ZERG,U_DRONE,B_HATCHERY,B_EXTRACTOR,1);
    cycle(R_PROTOSS,U_PROBE,B_NEXUS,B_ASSIMILATOR,0);
    cycle(R_PROTOSS,U_PROBE,B_NEXUS,B_ASSIMILATOR,1);
    mismatch_and_no_home(R_TER,U_SCV);
    mismatch_and_no_home(R_ZERG,U_DRONE);
    mismatch_and_no_home(R_PROTOSS,U_PROBE);
    puts("worker resource arrival, two trips, legacy timer and stop: ok");
    return 0;
}
