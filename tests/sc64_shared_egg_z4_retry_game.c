/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Constructed HOST Z4 capacity witness; no native trigger-parity claim. */
#define SC64_Z4_EGG_NO_MAIN
#include "sc64_shared_egg_z4_journal_game.c"
#undef SC64_Z4_EGG_NO_MAIN
static Unit original[MAX_UNITS];
static void prepare_retry(void)
{
    start_z4();int hall=hall_for_owner(SIDE);assert(hall>=0);
    minerals=1000;gas=1000;b[hall].larva=2;
    assert(spawn_unit(SIDE,U_OVERLORD,80,80)>=0);
    assert(spawn_build(SIDE,B_SPAWNING_POOL,b[hall].x+80,b[hall].y+80,0)>=0);
    assert(race_queue_unit(SIDE,hall,U_ZERGLING));int egg=egg_for_owner(SIDE,-1);assert(egg>=0);
    memcpy(original,u,sizeof(u));int left=1;
    for(int i=0;i<MAX_UNITS;++i)if(!u[i].alive) {
        if(left){--left;continue;}u[i].alive=1;u[i].type=U_DRONE;u[i].owner=SIDE;
    }
    assert(!race_product_slots_available(1) && race_z4_paired_egg_conflict());
    u[egg].target=U_DRONE;assert(!race_z4_paired_egg_conflict());u[egg].target=U_ZERGLING;
    int last=find_unit_slot();assert(last>=0);u[last].alive=1;
    const Sc64Z4SourceAction *create=&sc64_z4_source_actions[
        sc64_z4_source_triggers[SC64_Z4_TRIGGER_STARTUP].first_action+5];
    assert(!race_z4_paired_egg_conflict());
    assert(race_z4_create_action(create)==-SC64_Z4_FAULT_CAPACITY);u[last].alive=0;
    memset(&race_z4,0,sizeof(race_z4));race_z4.model.fired=1u<<SC64_Z4_TRIGGER_STARTUP;
    race_z4.model.action_cursor[SC64_Z4_TRIGGER_STARTUP]=6;
    race_z4.action_ticks[SC64_Z4_TRIGGER_STARTUP]=1;
    race_z4.countdown_active=1;race_z4.countdown_ticks=100;
    race_z4.model.fired|=1u<<SC64_Z4_TRIGGER_FORCE_START;
    race_z4.model.action_cursor[SC64_Z4_TRIGGER_FORCE_START]=1;
    Sc64Z4Output out={0};out.action_count=2;
    out.actions[0].trigger=SC64_Z4_TRIGGER_STARTUP;out.actions[0].action_index=5;
    out.actions[1].trigger=SC64_Z4_TRIGGER_FORCE_START;out.actions[1].action_index=0;
    assert(race_z4_dispatch_events(&out));
    assert(race_z4.faults==(1u<<(24u+SC64_Z4_TRIGGER_STARTUP)));
    assert(race_z4.action_ticks[SC64_Z4_TRIGGER_FORCE_START]==1 && race_z4.ai_script_mask);
    memcpy(u,original,sizeof(u));assert(race_z4_runtime_state_valid(&race_z4));
}
int main(int argc,char **argv)
{
    assert(argc==3);
    if(!strcmp(argv[1],"write")) {
        prepare_retry();assert(race_save_game());assert(fixture_export_host_save(argv[2]));
        puts("Z4 pending-create writer passed");return 0;
    }
    if(!strcmp(argv[1],"terminalwrite")) {
        prepare_retry();race_z4.model.outcome=SC64_Z4_WON;
        race_z4.model.fired|=1u<<SC64_Z4_TRIGGER_COMMAND_CENTER_WIN;
        assert(race_z4_runtime_state_valid(&race_z4));assert(race_save_game());
        assert(fixture_export_host_save(argv[2]));puts("Z4 pending terminal writer passed");return 0;
    }
    if(!strcmp(argv[1],"terminalread")) {
        game_init();assert(fixture_import_host_save(argv[2]));assert(race_load_game());
        assert(!won && race_z4.model.outcome==SC64_Z4_WON && race_z4.faults);
        assert(race_z4_campaign_tick());assert(won && !race_z4.faults);
        puts("Z4 pending terminal fresh retry passed");return 0;
    }
    if(!strcmp(argv[1],"legacy")) {
        prepare_retry();assert(race_save_game());u32 length=0;
        assert(save_read_latest(saved.bytes,sizeof(saved.bytes),SC64_SHARED_EGG_TRANSPORT_SAVE_VERSION,&length));
        game_init();assert(fixture_import_host_save(argv[2]));
        RaceLoadPlan plan;GbaSaveSelection older,winner;
        assert(race_worker_cargo_select(&plan,&older) && older.tag==SC64_TRANSPORT_SAVE_VERSION);
        edited=saved;edited.record.save.version=SAVE_VERSION;
        const u32 known[]={SC64_WORKER_CARGO_LEGACY_TRANSPORT_SAVE_VERSION,SC64_TRANSPORT_SAVE_VERSION,SC64_SHARED_EGG_TRANSPORT_SAVE_VERSION};
        assert(save_write_selected(edited.bytes,length,SC64_TRANSPORT_SAVE_VERSION,&older,known,3));
        assert(race_worker_cargo_select(&plan,&winner) && winner.slot==older.slot);
        assert(race_load_game() && !race_z4.faults && race_z4_active());
        puts("Z4 old-tag marker rejected; genuine old save restored");return 0;
    }
    assert(!strcmp(argv[1],"read"));game_init();assert(fixture_import_host_save(argv[2]));
    assert(race_load_game());assert(race_z4.faults==(1u<<(24u+SC64_Z4_TRIGGER_STARTUP)));
    unsigned cursor=race_z4.model.action_cursor[SC64_Z4_TRIGGER_STARTUP];
    int before=0;for(int i=0;i<MAX_UNITS;++i)before+=u[i].alive && u[i].raw_type==101;
    assert(race_z4_campaign_tick());assert(!race_z4.faults);
    int after=0;for(int i=0;i<MAX_UNITS;++i)after+=u[i].alive && u[i].raw_type==101;
    assert(after==before+1);
    assert(race_z4.model.action_cursor[SC64_Z4_TRIGGER_STARTUP]==cursor);
    assert(race_z4.action_ticks[SC64_Z4_TRIGGER_STARTUP]==1);
    assert(race_z4_campaign_tick());
    assert(race_z4.model.action_cursor[SC64_Z4_TRIGGER_STARTUP]==cursor+1);
    int final=0;for(int i=0;i<MAX_UNITS;++i)final+=u[i].alive && u[i].raw_type==101;
    assert(final==after);
    puts("Z4 pending-create fresh retry and cursor passed");return 0;
}
