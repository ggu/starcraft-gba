/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Constructed HOST setup: owner wallets, menu offsets and paid reservations. */
#define main sc64_shared_egg_live_fixture_main
#include "sc64_shared_egg_live_game.c"
#undef main

static int owner_hall(int owner)
{
    for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive && b[i].owner==owner &&
        sc64_race_is_zerg_townhall(b[i].type))return i;
    return -1;
}
static void gas_owner_refunds(void)
{
    static const int owners[]={SIDE,ENEMY,RESCUABLE,4};
    guardian_test_reset();game_init();race_mode=1;race_human=R_ZERG;race_bots=3;
    race_skirmish_map=0;for(int i=0;i<3;i++)race_bot_races[i]=R_ZERG;
    init_state();assert(screen==SCREEN_PLAY);
    const Sc64RaceUnitRule *r=sc64_race_unit_rule(U_HYDRALISK);
    assert(r && r->minerals>0 && r->gas>0);
    for(unsigned j=0;j<4;j++) {
        int owner=owners[j],hall=owner_hall(owner);assert(hall>=0 && race_owner_identity(owner)==R_ZERG);
        assert(spawn_build(owner,B_HYDRALISK_DEN,b[hall].x+100,b[hall].y,0)>=0);
        b[hall].larva=2;
        if(owner==SIDE){minerals=1000;gas=1000;}else{race_minerals[owner]=1000;race_gas[owner]=1000;}
    }
    for(unsigned j=0;j<4;j++) {
        int owner=owners[j],hall=owner_hall(owner),ore[4],vespene[4];
        for(unsigned k=0;k<4;k++){ore[k]=race_owner_minerals(owners[k]);vespene[k]=race_owner_gas(owners[k]);}
        int larvae=b[hall].larva;assert(race_queue_unit(owner,hall,U_HYDRALISK));
        int egg=egg_for(owner,U_HYDRALISK,-1);assert(egg>=0 && u[egg].raw_owner==-1);
        for(unsigned k=0;k<4;k++) {
            assert(race_owner_minerals(owners[k])==ore[k]-(k==j?r->minerals:0));
            assert(race_owner_gas(owners[k])==vespene[k]-(k==j?r->gas:0));
        }
        assert(b[hall].larva==larvae-1);
        int wrong=owners[(j+1)%4];assert(!race_shared_egg_cancel(wrong,egg));
        assert(race_owner_minerals(owner)==ore[j]-r->minerals && race_owner_gas(owner)==vespene[j]-r->gas);
        assert(race_shared_egg_cancel(owner,egg));assert(!race_shared_egg_cancel(owner,egg));
        for(unsigned k=0;k<4;k++)assert(race_owner_minerals(owners[k])==ore[k] && race_owner_gas(owners[k])==vespene[k]);
        assert(b[hall].larva==larvae-1);
    }
    puts("four active owners: gas-bearing charge, owner isolation and refund passed");
}
#ifdef SC64_P7_CARRIER_TRAINING_ENABLED
static void carrier_before_egg(void)
{
    guardian_test_reset();game_init();race_mode=1;race_human=R_PROTOSS;race_bots=1;
    race_skirmish_map=0;race_bot_races[0]=R_ZERG;init_state();assert(screen==SCREEN_PLAY);
    int hall=owner_hall(ENEMY);assert(hall>=0);b[hall].larva=2;
    race_minerals[ENEMY]=1000;race_gas[ENEMY]=1000;
    assert(spawn_build(ENEMY,B_SPAWNING_POOL,b[hall].x+100,b[hall].y,0)>=0);
    minerals=1000;int carrier=spawn_unit(SIDE,U_P7_CARRIER,100,100);assert(carrier>=0);
    assert(race_p7_carrier_training_queue(carrier));assert(u[carrier].carry==1);
    int free_slots=0,last=-1;for(int i=0;i<MAX_UNITS;i++)free_slots+=!u[i].alive;
    while(free_slots>2){last=spawn_unit(SIDE,U_MARINE,160,160);assert(last>=0);--free_slots;}
    int ore=race_minerals[ENEMY],larvae=b[hall].larva;
    assert(race_owner_reserved_supply(ENEMY)+2<=race_owner_supply_cap(ENEMY));
    assert(race_product_slots_available(2));
    assert(!race_p7_carrier_training_slots_available(2));
    assert(!race_queue_unit(ENEMY,hall,U_ZERGLING));
    assert(race_minerals[ENEMY]==ore && b[hall].larva==larvae && u[carrier].carry==1);
    assert(last>=0);hit_target(last,9999,U_MARINE,ENEMY);
    assert(race_product_slots_available(2) && race_p7_carrier_training_slots_available(2));
    assert(race_queue_unit(ENEMY,hall,U_ZERGLING));
    assert(egg_for(ENEMY,U_ZERGLING,-1)>=0 && u[carrier].carry==1);
    puts("Carrier-first paired Egg slot reservation passed");
}
static void mixed_context_rows(void)
{
    guardian_test_reset();game_init();race_mode=1;race_human=R_ZERG;race_bots=1;
    race_skirmish_map=0;race_bot_races[0]=R_PROTOSS;init_state();assert(screen==SCREEN_PLAY);
    int hall=owner_hall(SIDE);assert(hall>=0);minerals=1000;gas=1000;b[hall].larva=2;
    assert(race_queue_unit(SIDE,hall,U_DRONE));int egg=egg_for(SIDE,U_DRONE,-1);assert(egg>=0);
    int carrier=spawn_unit(SIDE,U_P7_CARRIER,100,100);assert(carrier>=0);
    assert(spawn_unit(SIDE,U_OVERLORD,120,100)>=0); /* supply for captured Carrier */
    selected_building=-1;for(int i=0;i<MAX_UNITS;i++)u[i].selected=0;
    u[egg].selected=1;u[carrier].selected=1;
    assert(race_p7_carrier_training_menu_available() && race_z7_egg_cancel_available());
    int cr=race_context_command_base(RACE_CONTEXT_CARRIER),er=race_context_command_base(RACE_CONTEXT_EGG);
    assert(er==cr+1 && er<race_skirmish_command_count());
    assert(!strcmp(race_menu_label(cr),"BUILD INTERCEPTOR") && !strcmp(race_menu_label(er),"CANCEL EGG"));
    int before=minerals;choose_command(er);
    assert(!u[egg].alive && u[carrier].carry==0 && minerals==before+sc64_race_unit_rule(U_DRONE)->minerals);
    assert(race_queue_unit(SIDE,hall,U_DRONE));egg=egg_for(SIDE,U_DRONE,-1);assert(egg>=0);
    u[egg].selected=1;u[carrier].selected=1;
    cr=race_context_command_base(RACE_CONTEXT_CARRIER);er=race_context_command_base(RACE_CONTEXT_EGG);
    before=minerals;choose_command(cr);
    assert(u[egg].alive && u[carrier].carry==1 && minerals==before-SC64_P7_INTERCEPTOR_COST && er==cr+1);
    puts("mixed Carrier/Egg contextual count, labels and dispatch passed");
}
#endif
int main(void)
{
    gas_owner_refunds();
#ifdef SC64_P7_CARRIER_TRAINING_ENABLED
    carrier_after_paid_egg(); /* Reuse existing Egg-first/deferral fixture. */
    carrier_before_egg();mixed_context_rows();
#endif
    puts("constructed HOST owner, context and reservation checks passed");return 0;
}
