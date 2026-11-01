/* Candidate preflight preserves the caller's Protoss service binding. */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/game.c"
#if !defined(SC64_P7_OBSERVER_ENABLED) || !defined(SC64_P7_BATTERY_ENABLED) || \
    !defined(SC64_P7_PROJECTILES_ENABLED) || !defined(SC64_SHARED_TRANSPORT)
#error "requires canonical P7/P8 shared-service flags"
#endif

static Unit original_units[MAX_UNITS];
static Building original_buildings[MAX_BUILDINGS];
static Sc64P7PersistentState original_p7;
#ifdef SC64_P8_RUNTIME
static Sc64P8PersistentState original_p8;
#endif
static u8 original_clock[3];
static u8 original_projectiles[SC64_CODEC_BYTES];
static u8 original_projectile_fault;
static Sc64SkTransportTech original_transport;
static int original_minerals,original_gas,original_frame,original_mode,original_mission;

static void tap(u16 key){game_update(key,key);game_update(0,0);}
static void snapshot(void)
{
    memcpy(original_units,u,sizeof(u));memcpy(original_buildings,b,sizeof(b));
    original_p7=race_p7;
#ifdef SC64_P8_RUNTIME
    original_p8=race_p8;
#endif
    memcpy(original_clock,race_protoss_generic_clock,sizeof(original_clock));
    assert(sc64_codec_encode(&race_p7_projectiles,original_projectiles,SC64_CODEC_BYTES));
    original_projectile_fault=race_p7_projectile_fault;
    original_transport=race_skirmish_transport_tech;
    original_minerals=minerals;original_gas=gas;original_frame=frame;
    original_mode=race_mode;original_mission=race_selected_mission;
}
static void world_unchanged(void)
{
    u8 projectiles[SC64_CODEC_BYTES];
    assert(!memcmp(original_units,u,sizeof(u)) && !memcmp(original_buildings,b,sizeof(b)));
    assert(!memcmp(&original_p7,&race_p7,sizeof(race_p7)));
#ifdef SC64_P8_RUNTIME
    assert(!memcmp(&original_p8,&race_p8,sizeof(race_p8)));
#endif
    assert(!memcmp(original_clock,race_protoss_generic_clock,sizeof(original_clock)));
    assert(sc64_codec_encode(&race_p7_projectiles,projectiles,sizeof(projectiles)));
    assert(!memcmp(original_projectiles,projectiles,sizeof(projectiles)));
    assert(race_p7_projectile_fault==original_projectile_fault);
    assert(!memcmp(&original_transport,&race_skirmish_transport_tech,sizeof(original_transport)));
    assert(minerals==original_minerals && gas==original_gas && frame==original_frame);
    assert(race_mode==original_mode && race_selected_mission==original_mission);
}
int main(void)
{
#ifdef SC64_P8_RUNTIME
    const int mission=SC64_P8_GLOBAL_ID;
#else
    const int mission=SC64_P7_GLOBAL_ID;
#endif
    game_init();int ordinal=sc64_catalog_playable_ordinal(SC64_FACTION_PROTOSS,mission);
    assert(ordinal>=0);tap(KEY_A);
    for(int i=0;i<ordinal/2;i++)tap(KEY_RIGHT);
    if(ordinal&1)tap(KEY_DOWN);
    tap(KEY_A);game_update(KEY_START,KEY_START);
    assert(screen==SCREEN_PLAY && !race_mode && race_selected_mission==mission);
    assert(race_save_game());
    snapshot();
    for(int i=0;i<2;i++){
        signed char prior=i?SC64_PROTOSS_BIND_GENERIC:-1;
        int prior_mission=i?SC64_P7_GLOBAL_ID:mission+1;
        RaceLoadPlan plan={0};
        race_protoss_binding_override=prior;race_protoss_override_mission=prior_mission;
        int binding_before=race_protoss_service_binding();
        assert(race_load_preflight(&plan));
        assert(race_protoss_binding_override==prior && race_protoss_override_mission==prior_mission);
        assert(race_protoss_service_binding()==binding_before);
        world_unchanged();
        int saved_bots=race_data.state[4];race_data.state[4]=-1;
        assert(!race_load_preflight(&plan));
        assert(race_protoss_binding_override==prior && race_protoss_override_mission==prior_mission);
        assert(race_protoss_service_binding()==binding_before);
        world_unchanged();race_data.state[4]=saved_bots;
    }
    printf("P%d preflight restores both binding fields on accept and reject\n",mission==SC64_P7_GLOBAL_ID?7:8);
    return 0;
}
