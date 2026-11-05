/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Constructed Z7 HOST scenario: real Nydus transfer and Scarab references. */
#include "sc64_scarab_nydus_support.h"
#include "../starcraft/sc64_scarab_unit_adapter.h"

int main(void)
{
    launch_with_canals();
    int passenger=side_hydra(0);
    /* Place the passenger at the real entrance contact ring; ordinary input
     * issues the order and game_update performs the actual transfer. */
    select_unit_normal(passenger);
    assert(command_enter(ent)==1);
    u[passenger].x=b[ent].x+32;u[passenger].y=b[ent].y;
    int parent=spawn_unit(ENEMY,U_REAVER,u[passenger].x+50,u[passenger].y);
    assert(parent>=0 && race_scarab_parent_init(parent));
    race_minerals[ENEMY]=500;assert(race_scarab_enqueue(parent,0));
    assert(race_scarab_production_tick(parent,0)==SC64_SCARAB_ALLOCATED);
    Sc64ScarabParent p;Sc64ScarabCounts counts;
    assert(race_scarab_parent_read(parent,0,&p,&counts));int child=p.active_slot;
    for(int i=0;i<SC64_SCARAB_BUILD_TICKS;++i)assert(race_scarab_production_tick(parent,0)==SC64_SCARAB_PROGRESS);
    assert(race_scarab_production_tick(parent,0)==SC64_SCARAB_COMPLETED);
    race_scarab_probe_parent_attack(parent,passenger);
    assert(u[child].order==1 && u[child].target==passenger);
    int tx=u[child].tx,ty=u[child].ty;
    for(int i=0;i<80 && u[passenger].order==ORDER_NYDUS_ENTER;++i)input(0);
    assert(u[passenger].alive && near_exit(passenger,ex));
    assert(u[child].alive && u[child].target==-1 && u[child].tx==tx && u[child].ty==ty);
    assert(u[child].order>=1 && u[child].order<=4);
    puts("Nydus transit invalidates outside Scarab target without resetting its FSM");
    return 0;
}
