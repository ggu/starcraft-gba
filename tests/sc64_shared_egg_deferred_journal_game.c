/* SPDX-License-Identifier: GPL-3.0-or-later */
/* HOST-only real Z9 scheduler, paid Egg, deferred create and journal. */
#define SC64_SHARED_EGG_NO_MAIN
#include "sc64_shared_egg_journal_game.c"
#undef SC64_SHARED_EGG_NO_MAIN
static int free_units(void)
{
    int n=0;for(int i=0;i<MAX_UNITS;i++) n+=!u[i].alive;return n;
}
static Sc64Z8Instance *trigger_four(void)
{
    for(unsigned i=0;i<race_zl_sched.core.instance_count;i++)
        if(race_zl_sched.core.instances[i].trigger==4 &&
           race_zl_sched.core.instances[i].owner==SC64_ZL_HUMAN_SLOT)
            return &race_zl_sched.core.instances[i];
    return 0;
}
int main(void)
{
    clear_records();launch_z9_from_normal_menu();int hall=hall_for_owner(SIDE);assert(hall>=0);
    minerals=1000;gas=1000;b[hall].larva=3;
    assert(spawn_build(SIDE,B_SPAWNING_POOL,120,120,0)>=0);
    for(int i=0;i<4;i++)assert(spawn_unit(SIDE,U_OVERLORD,90+i*8,90)>=0);
    assert(race_queue_unit(SIDE,hall,U_ZERGLING));
    int pair=-1;for(int i=0;i<MAX_UNITS;i++)
        if(u[i].alive && u[i].type==U_Z7_EGG && u[i].target==U_ZERGLING) {pair=i;break;}
    assert(pair>=0);
    int filler[2]={-1,-1};
    while(free_units()>1) {
        int slot=spawn_unit(SIDE,U_DRONE,140,140);assert(slot>=0);
        filler[0]=filler[1];filler[1]=slot;
    }
    assert(filler[0]>=0 && filler[1]>=0 && free_units()==1);
    Sc64Z8Instance *in=trigger_four();assert(in && !(in->flags & SC64_Z8_INST_DONE));
    race_zl_sched.countdown=0;race_zl_sched.core.switches[0]|=1;
    assert(sc64_zl_conditions(&race_zl_sched,race_zl_mission->program,in,
                              &race_zl_world_ops,&race_zl_view)==1);
    u8 outcomes[SC64_Z8_OWNER_LIMIT]={0};
    assert(sc64_zl_execute(&race_zl_sched,race_zl_mission->program,in,
                           &race_zl_world_ops,&race_zl_view,outcomes));
    assert((in->flags & SC64_Z8_INST_EXECUTING) && in->cursor==0 && free_units()==1);
    assert(race_shared_trigger_obligations()==2);
    assert(race_save_game());assert(race_load_game());
    in=trigger_four();assert(in && (in->flags & SC64_Z8_INST_EXECUTING) && in->cursor==0);
    assert(free_units()==1 && race_shared_trigger_obligations()==2);
    hit_target(filler[0],9999,U_DRONE,ENEMY);hit_target(filler[1],9999,U_DRONE,ENEMY);
    assert(free_units()>=3);
    assert(sc64_zl_execute(&race_zl_sched,race_zl_mission->program,in,
                           &race_zl_world_ops,&race_zl_view,outcomes));
    assert(in->cursor>0 && u[pair].alive && u[pair].type==U_Z7_EGG);
    int crystal=0;for(int i=0;i<MAX_UNITS;i++)
        if(u[i].alive && u[i].raw_type==SC64_ZL_RAW_CRYSTAL) crystal++;
    assert(crystal>0);
    puts("Z9 deferred trigger journal continuation passed");return 0;
}
