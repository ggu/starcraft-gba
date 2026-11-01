/* SPDX-License-Identifier: GPL-3.0-or-later
 * Current canonical Z7 ordinary save/load of paid Egg jobs. */
#define main sc64_z7_legacy_egg_fixture_main
#include "sc64_z7_egg_save_game.c"
#undef main
#if !defined(SC64_SHARED_TRANSPORT) || !defined(SC64_IMPACT_EVENTS) || defined(SC64_BATTLECRUISER_YAMATO_EXPERIMENT)
#error "This positive fixture requires the canonical Transport1 envelope"
#endif
int main(void)
{
    static unsigned char payload[16384]; u32 len=0;
    launch_z7();
    int hatch=add_source_hatchery(SIDE,3);
    int pool=spawn_build(SIDE,B_SPAWNING_POOL,470,420,0);
    int spire=spawn_build(SIDE,B_SPIRE,530,420,0);
    assert(pool>=0 && spire>=0);
    b[pool].raw_owner=b[spire].raw_owner=3;
    b[pool].raw_type=sc64_race_raw_building(B_SPAWNING_POOL);
    b[spire].raw_type=sc64_race_raw_building(B_SPIRE);
    b[hatch].larva=3;minerals=race_minerals[SIDE]=1000;gas=race_gas[SIDE]=1000;
    make_egg(SIDE,hatch,U_ZERGLING);make_egg(SIDE,hatch,U_SCOURGE);make_egg(SIDE,hatch,U_DRONE);
    int ling=find_egg(SIDE,U_ZERGLING),scourge=find_egg(SIDE,U_SCOURGE),drone=find_egg(SIDE,U_DRONE);
    assert(ling>=0 && scourge>=0 && drone>=0 && b[hatch].larva==0);
    for(int i=0;i<17;++i)game_update(0,0);
    int lt=u[ling].work_timer,st=u[scourge].work_timer,dt=u[drone].work_timer;
    int used=supply_used,ore=minerals,vespene=gas;
    assert(race_save_game());
    assert(save_read_latest(payload,sizeof(payload),SC64_TRANSPORT_SAVE_VERSION,&len));
    assert(len==sizeof(RaceSave)+(race_impact_count ? sc64_impact_save_bytes(race_impact_count) : 0u)+16u);
    for(unsigned i=0;i<16u;++i)assert(payload[len-16u+i]==0);
    assert(!save_read_latest(payload,sizeof(payload),SAVE_VERSION,&len));
    assert(race_load_game());
    assert(u[ling].alive && u[ling].raw_owner==3 && u[ling].work_timer==lt);
    assert(u[scourge].alive && u[scourge].raw_owner==3 && u[scourge].work_timer==st);
    assert(u[drone].alive && u[drone].raw_owner==3 && u[drone].work_timer==dt);
    assert(supply_used==used && minerals==ore && gas==vespene);
    for(int i=0;i<MAX_UNITS;++i)u[i].selected=0;
    u[drone].selected=1;assert(race_z7_egg_cancel_selected());
    assert(!u[drone].alive && minerals==ore+sc64_race_unit_rule(U_DRONE)->minerals);
    /* No other world tick runs here; existing alive slots cannot witness a
     * paired birth. Record occupied identities immediately before completion. */
    unsigned char occupied[MAX_UNITS];
    for(int i=0;i<MAX_UNITS;++i)occupied[i]=(unsigned char)!!u[i].alive;
    int steps=lt>st?lt:st;
    for(int i=0;i<steps;++i){update_orders();race_tick_state();}
    assert(u[ling].alive && u[ling].type==U_ZERGLING && u[ling].raw_owner==3);
    assert(u[scourge].alive && u[scourge].type==U_SCOURGE && u[scourge].raw_owner==3);
    int new_lings=0,new_scourges=0;
    for(int i=0;i<MAX_UNITS;++i)if(!occupied[i] && u[i].alive && u[i].owner==SIDE && u[i].raw_owner==3){
        new_lings+=u[i].type==U_ZERGLING;new_scourges+=u[i].type==U_SCOURGE;
    }
    assert(new_lings==1 && new_scourges==1);
    puts("canonical Transport1 paid Eggs: save/load, cancel and paired births passed");
    return 0;
}
