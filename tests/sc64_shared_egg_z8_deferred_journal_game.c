/* SPDX-License-Identifier: GPL-3.0-or-later */
/* HOST-only real Z8 trigger wait, deferred create and journal. */
#define main sc64_z8_existing_fixture_main
#include "sc64_z8_game.c"
#undef main
static int free_units(void){int n=0;for(int i=0;i<MAX_UNITS;i++)n+=!u[i].alive;return n;}
static Sc64Z8Instance *trigger_24(void)
{
    for(unsigned i=0;i<race_z8_sched.instance_count;i++)
        if(race_z8_sched.instances[i].trigger==24 && race_z8_sched.instances[i].owner==SC64_Z8_HUMAN_SLOT)
            return &race_z8_sched.instances[i];
    return 0;
}
int main(void)
{
    fixture_clear_candidate_records();game_init();held_keys=0;
    int ordinal=sc64_catalog_playable_ordinal(SC64_FACTION_ZERG,SC64_Z8_GLOBAL_ID);
    assert(ordinal>=0);press(KEY_A);for(int i=0;i<ordinal/2;i++)press(KEY_RIGHT);
    if(ordinal&1)press(KEY_DOWN);press(KEY_A);press(KEY_START);assert(race_z8_active());
    int hall=-1;for(int i=0;i<MAX_BUILDINGS;i++)if(building_complete(i) && b[i].owner==SIDE &&
        sc64_race_is_zerg_townhall(b[i].type)){hall=i;break;}
    assert(hall>=0);minerals=1000;gas=1000;b[hall].larva=3;
    assert(spawn_build(SIDE,B_SPAWNING_POOL,120,120,0)>=0);
    for(int i=0;i<4;i++)assert(spawn_unit(SIDE,U_OVERLORD,90+i*8,90)>=0);
    assert(race_queue_unit(SIDE,hall,U_ZERGLING));
    int filler[6]={0};while(free_units()>1) {
        int slot=spawn_unit(SIDE,U_DRONE,140,140);assert(slot>=0);
        for(int i=0;i<5;i++)filler[i]=filler[i+1];filler[5]=slot;
    }
    for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive && b[i].raw_type==SC64_Z8_RAW_NEXUS && b[i].owner!=SIDE)
        hit_target(MAX_UNITS+i,9999,U_DRONE,SIDE);
    Sc64Z8Instance *in=trigger_24();assert(in && !(in->flags&SC64_Z8_INST_DONE));
    assert(sc64_z8_conditions(&race_z8_sched,&race_z8_program,in,&sc64_z8_world_ops,&race_z8_view)==1);
    u8 outcomes[SC64_Z8_OWNER_LIMIT]={0};
    assert(sc64_z8_execute(&race_z8_sched,&race_z8_program,in,&sc64_z8_world_ops,&race_z8_view,outcomes));
    assert(sc64_z8_release_wait(&race_z8_sched,SC64_Z8_HUMAN_SLOT));
    assert(sc64_z8_execute(&race_z8_sched,&race_z8_program,in,&sc64_z8_world_ops,&race_z8_view,outcomes));
    assert((in->flags&SC64_Z8_INST_EXECUTING) && in->cursor==1 && free_units()==1);
    assert(race_save_game() && race_load_game());in=trigger_24();assert(in && in->cursor==1);
    for(int i=0;i<6;i++)hit_target(filler[i],9999,U_DRONE,ENEMY);
    assert(free_units()>=7);
    assert(sc64_z8_execute(&race_z8_sched,&race_z8_program,in,&sc64_z8_world_ops,&race_z8_view,outcomes));
    assert(in->cursor>1);puts("Z8 deferred trigger journal continuation passed");return 0;
}
