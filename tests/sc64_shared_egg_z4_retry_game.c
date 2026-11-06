/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Constructed HOST Z4 capacity witness; no native trigger-parity claim. */
#define main sc64_z4_egg_journal_main
#include "sc64_shared_egg_z4_journal_game.c"
#undef main
static Unit original[MAX_UNITS];
static void prepare_retry(void)
{
    start_z4();int hall=hall_for_owner(SIDE);assert(hall>=0);
    minerals=1000;gas=1000;b[hall].larva=2;
    assert(spawn_unit(SIDE,U_OVERLORD,80,80)>=0);
    assert(spawn_build(SIDE,B_SPAWNING_POOL,b[hall].x+80,b[hall].y+80,0)>=0);
    assert(race_queue_unit(SIDE,hall,U_ZERGLING));
    memcpy(original,u,sizeof(u));int left=1;
    for(int i=0;i<MAX_UNITS;++i)if(!u[i].alive) {
        if(left){--left;continue;}u[i].alive=1;u[i].type=U_DRONE;u[i].owner=SIDE;
    }
    assert(!race_product_slots_available(1));
    memset(&race_z4,0,sizeof(race_z4));race_z4.model.fired=1u<<SC64_Z4_TRIGGER_STARTUP;
    race_z4.model.action_cursor[SC64_Z4_TRIGGER_STARTUP]=6;
    race_z4.action_ticks[SC64_Z4_TRIGGER_STARTUP]=1;
    race_z4.countdown_active=1;race_z4.countdown_ticks=100;
    Sc64Z4Output out={0};out.action_count=1;
    out.actions[0].trigger=SC64_Z4_TRIGGER_STARTUP;out.actions[0].action_index=5;
    assert(race_z4_dispatch_events(&out));
    assert(race_z4.faults==(1u<<(24u+SC64_Z4_TRIGGER_STARTUP)));
    memcpy(u,original,sizeof(u));assert(race_z4_runtime_state_valid(&race_z4));
}
int main(int argc,char **argv)
{
    assert(argc==3);
    if(!strcmp(argv[1],"write")) {
        prepare_retry();assert(race_save_game());assert(fixture_export_host_save(argv[2]));
        puts("Z4 pending-create writer passed");return 0;
    }
    assert(!strcmp(argv[1],"read"));game_init();assert(fixture_import_host_save(argv[2]));
    assert(race_load_game());assert(race_z4.faults==(1u<<(24u+SC64_Z4_TRIGGER_STARTUP)));
    unsigned cursor=race_z4.model.action_cursor[SC64_Z4_TRIGGER_STARTUP];
    assert(race_z4_campaign_tick());assert(!race_z4.faults);
    assert(race_z4.model.action_cursor[SC64_Z4_TRIGGER_STARTUP]==cursor);
    assert(race_z4.action_ticks[SC64_Z4_TRIGGER_STARTUP]==1);
    assert(race_z4_campaign_tick());
    assert(race_z4.model.action_cursor[SC64_Z4_TRIGGER_STARTUP]==cursor+1);
    puts("Z4 pending-create fresh retry and cursor passed");return 0;
}
