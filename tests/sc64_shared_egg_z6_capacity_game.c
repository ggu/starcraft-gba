/* SPDX-License-Identifier: GPL-3.0-or-later
 * Constructed Z6 pool boundary around its persisted future-create count. */
#define SC64_Z6_EGG_NO_MAIN
#include "sc64_shared_egg_z6_journal_game.c"
#undef SC64_Z6_EGG_NO_MAIN
int main(void)
{
    start_z6();int hall=hall_for_owner(SIDE);
    if(hall<0) {hall=spawn_build(SIDE,B_HATCHERY,200,200,0);assert(hall>=0);b[hall].raw_owner=3;}
    minerals=1000;gas=1000;b[hall].larva=2;
    assert(spawn_unit(SIDE,U_OVERLORD,80,80)>=0);
    assert(spawn_build(SIDE,B_SPAWNING_POOL,b[hall].x+80,b[hall].y+80,0)>=0);
    for(int i=0;i<MAX_BUILDINGS;++i){b[i].queue_count=0;b[i].queue_head=0;}
    unsigned pending=race_z6_ai_pending_source_creates();
    assert(pending==race_z6_save_remaining_creates(&race_z6) && pending>0 && pending+2<MAX_UNITS);
    unsigned free=0;for(int i=0;i<MAX_UNITS;++i)free+=!u[i].alive;
    assert(free>pending+2);
    for(int i=0;i<MAX_UNITS && free>pending+1;++i) if(!u[i].alive) {
        u[i].alive=1;u[i].type=U_DRONE;u[i].owner=SIDE;u[i].raw_owner=-1;--free;
    }
    int ore=minerals,larva=b[hall].larva;
    assert(!race_queue_unit(SIDE,hall,U_ZERGLING));
    assert(minerals==ore && b[hall].larva==larva);
    int extra=-1;for(int i=MAX_UNITS-1;i>=0;--i) if(u[i].alive && u[i].type==U_DRONE && u[i].raw_owner==-1){extra=i;break;}
    assert(extra>=0);u[extra].alive=0;
    assert(race_queue_unit(SIDE,hall,U_ZERGLING));int egg=egg_for_owner(SIDE,-1);assert(egg>=0);
    u[egg].work_timer=1;int blocked=-1;
    for(int i=0;i<MAX_UNITS;++i)if(!u[i].alive){blocked=i;break;}
    assert(blocked>=0);u[blocked].alive=1;u[blocked].type=U_DRONE;u[blocked].owner=SIDE;
    assert(race_z7_egg_tick_unit(egg) && u[egg].type==U_Z7_EGG && u[egg].work_timer==1);
    u[blocked].alive=0;assert(race_z7_egg_tick_unit(egg));
    assert(u[egg].alive && u[egg].type==U_ZERGLING);
    puts("Z6 future CreateUnit and paired Egg capacity boundary passed");return 0;
}
