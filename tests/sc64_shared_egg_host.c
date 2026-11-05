/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Constructed HOST-only worlds; never seed guest SRAM. */
#define main sc64_z9_existing_fixture_main
#include "sc64_z9_game.c"
#undef main
#ifndef SC64_SHARED_EGG_HOST_STAGE
#error "This fixture requires the HOST-only shared Egg stage"
#endif
static int count_type(int owner,int type)
{
    int n=0;for(int i=0;i<MAX_UNITS;i++)n+=u[i].alive && u[i].owner==owner && u[i].type==type;
    return n;
}
static int find_egg(int owner,int product,int except)
{
    for(int i=0;i<MAX_UNITS;i++)if(i!=except && u[i].alive && u[i].owner==owner &&
        u[i].type==U_Z7_EGG && u[i].target==product)return i;
    return -1;
}
static int hall_for(int owner)
{
    for(int i=0;i<MAX_BUILDINGS;i++)if(building_complete(i) && b[i].owner==owner &&
        sc64_race_is_zerg_townhall(b[i].type))return i;
    return -1;
}
static void add_supply(int owner,int required)
{
    for(int i=0;i<8 && race_owner_supply_cap(owner)<race_owner_reserved_supply(owner)+required;i++)
        assert(spawn_unit(owner,U_OVERLORD,80+i*8,80)>=0);
    assert(race_owner_supply_cap(owner)>=race_owner_reserved_supply(owner)+required);
}
static void test_late_z9(void)
{
    launch_z9_from_normal_menu();int hall=hall_for(SIDE);assert(hall>=0);
    assert(b[hall].raw_owner==SC64_ZL_HUMAN_SLOT);
    add_supply(SIDE,4);minerals=1000;gas=1000;b[hall].larva=3;
    fixture_clear_candidate_records();assert(race_save_game());
    static u8 before[16368],after[16368];u32 n1=0,n2=0;
    assert(save_read_latest(before,sizeof(before),SC64_TRANSPORT_SAVE_VERSION,&n1));
    int queued=b[hall].queue_count,orig_ore=minerals,orig_supply=race_owner_supply(SIDE);
    assert(race_queue_unit(SIDE,hall,U_DRONE));int a=find_egg(SIDE,U_DRONE,-1);assert(a>=0);
    assert(race_queue_unit(SIDE,hall,U_DRONE));int z=find_egg(SIDE,U_DRONE,a);assert(z>=0);
    assert(a!=z && b[hall].queue_count==queued && b[hall].larva==1 && minerals==orig_ore-100);
    assert(race_owner_supply(SIDE)==orig_supply+4);
    assert(u[a].raw_owner==b[hall].raw_owner && u[z].raw_owner==b[hall].raw_owner);
    u[a].selected=1;assert(!race_z7_egg_cancel_available()); /* menu remains Z7-only */
    assert(!race_save_game());assert(save_read_latest(after,sizeof(after),SC64_TRANSPORT_SAVE_VERSION,&n2));
    assert(n1==n2 && !memcmp(before,after,n1));
    /* HOST-only synthetic candidate: the unchanged old record is valid, but
     * adding a live stage Egg to its row must fail before world publication. */
    static RaceSave baseline;memcpy(&baseline,&race_data,sizeof(baseline));
    RaceLoadPlan plan={0};assert(race_load_preflight(&plan));
    memcpy(&race_data,&baseline,sizeof(baseline));memset(&plan,0,sizeof(plan));
    assert(sc64_zp_unit_save_encode(&race_data.units[a],&u[a],race_cols*16u,race_rows*16u));
    Unit live_before=u[a];assert(!race_load_preflight(&plan));assert(!memcmp(&u[a],&live_before,sizeof(live_before)));
    memcpy(&race_data,&baseline,sizeof(baseline));
    int duration=sc64_z7_egg_duration(U_DRONE);
    for(int i=0;i<30;i++)update_orders();
    assert(u[a].work_timer==duration-30 && u[z].work_timer==duration-30);
    for(int i=30;i<duration;i++)update_orders();
    assert(u[a].alive && u[a].type==U_DRONE && u[a].raw_owner==SC64_ZL_HUMAN_SLOT);
    assert(u[z].alive && u[z].type==U_DRONE && u[z].raw_owner==SC64_ZL_HUMAN_SLOT);
    /* A late-mission paired birth must retain the authenticated source slot
     * on both bodies, not the transient producer context at completion. */
    assert(spawn_build(SIDE,B_SPAWNING_POOL,120,120,0)>=0);add_supply(SIDE,2);
    b[hall].larva=1;int before_lings=count_type(SIDE,U_ZERGLING);
    assert(race_queue_unit(SIDE,hall,U_ZERGLING));int pair=find_egg(SIDE,U_ZERGLING,-1);assert(pair>=0);
    int timer=u[pair].work_timer;for(int i=0;i<timer;i++)update_orders();
    assert(u[pair].alive && u[pair].type==U_ZERGLING && u[pair].raw_owner==SC64_ZL_HUMAN_SLOT);
    assert(count_type(SIDE,U_ZERGLING)==before_lings+2);
    int matched=0;for(int i=0;i<MAX_UNITS;i++)if(u[i].alive && u[i].owner==SIDE &&
        u[i].type==U_ZERGLING && u[i].raw_owner==SC64_ZL_HUMAN_SLOT)matched++;
    assert(matched>=2);
}
static void setup_custom(void)
{
    game_init();race_mode=1;race_human=R_ZERG;race_bots=1;race_skirmish_map=0;
    race_bot_races[0]=R_ZERG;init_state();assert(screen==SCREEN_PLAY && race_mode);
    minerals=1000;gas=1000;race_minerals[ENEMY]=1000;race_gas[ENEMY]=1000;
}
static void test_custom_owners(void)
{
    setup_custom();int side=hall_for(SIDE),bot=hall_for(ENEMY);assert(side>=0 && bot>=0);
    add_supply(SIDE,4);add_supply(ENEMY,4);b[side].larva=3;b[bot].larva=3;
    int side_ore=minerals,bot_ore=race_minerals[ENEMY],ss=race_owner_supply(SIDE),bs=race_owner_supply(ENEMY);
    assert(race_queue_unit(SIDE,side,U_DRONE));int a=find_egg(SIDE,U_DRONE,-1);assert(a>=0);
    assert(race_queue_unit(ENEMY,bot,U_DRONE));int z=find_egg(ENEMY,U_DRONE,-1);assert(z>=0);
    assert(minerals==side_ore-50 && race_minerals[ENEMY]==bot_ore-50);
    assert(race_owner_supply(SIDE)==ss+2 && race_owner_supply(ENEMY)==bs+2);
    assert(u[a].raw_owner==b[side].raw_owner && u[z].raw_owner==b[bot].raw_owner);
    int timer=u[z].work_timer;assert(!race_shared_egg_cancel(SIDE,z));
    assert(race_shared_egg_cancel(SIDE,a));assert(!u[a].alive);
    assert(minerals==side_ore && race_minerals[ENEMY]==bot_ore-50);
    assert(race_owner_supply(SIDE)==ss && race_owner_supply(ENEMY)==bs+2);
    assert(!race_save_game()); /* dead Egg remains a forbidden legacy row */
    int reused=spawn_unit(SIDE,U_DRONE,100,100);assert(reused==a && u[a].type==U_DRONE);
    /* Freeze unrelated harvesting orders so wallet isolation is not confused
     * with normal resource deposits during the independent Egg timer. */
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive && u[i].type!=U_Z7_EGG)u[i].order=ORDER_IDLE;
    for(int i=0;i<timer;i++)update_orders();
    assert(u[z].alive && u[z].type==U_DRONE && u[z].raw_owner==b[bot].raw_owner);
    assert(race_minerals[ENEMY]==bot_ore-50);
    b[bot].larva=1;assert(race_queue_unit(ENEMY,bot,U_DRONE));
    int doomed=find_egg(ENEMY,U_DRONE,-1);assert(doomed>=0);
    int paid_ore=race_minerals[ENEMY];hit_target(doomed,500,U_MARINE,SIDE);
    assert(!u[doomed].alive && race_minerals[ENEMY]==paid_ore);
    assert(!race_shared_egg_cancel(ENEMY,doomed));
    assert(spawn_unit(ENEMY,U_DRONE,120,120)==doomed);
}
static void test_pair_and_denials(void)
{
    setup_custom();int side=hall_for(SIDE);assert(side>=0);add_supply(SIDE,2);
    assert(spawn_build(SIDE,B_SPAWNING_POOL,200,200,0)>=0);b[side].larva=2;
    int ore=minerals,larva=b[side].larva;
    minerals=0;assert(!race_queue_unit(SIDE,side,U_ZERGLING));
    assert(minerals==0 && b[side].larva==larva);minerals=ore;
    b[side].larva=0;assert(!race_queue_unit(SIDE,side,U_ZERGLING));
    assert(minerals==ore && b[side].larva==0);b[side].larva=larva;
    int before=count_type(SIDE,U_ZERGLING);
    assert(race_queue_unit(SIDE,side,U_ZERGLING));int egg=find_egg(SIDE,U_ZERGLING,-1);assert(egg>=0);
    int source=u[egg].raw_owner,timer=u[egg].work_timer;
    for(int i=0;i<timer;i++)update_orders();
    assert(u[egg].alive && u[egg].type==U_ZERGLING && u[egg].raw_owner==source);
    assert(count_type(SIDE,U_ZERGLING)==before+2);
    /* At the cap, ordinary positive-supply production must not debit; a
     * zero-supply Overlord remains admissible to restore supply. */
    while(race_owner_reserved_supply(SIDE)+2<=race_owner_supply_cap(SIDE))
        assert(spawn_unit(SIDE,U_MUTALISK,100,100)>=0);
    b[side].larva=2;ore=minerals;
    assert(!race_queue_unit(SIDE,side,U_DRONE));assert(minerals==ore && b[side].larva==2);
    assert(race_queue_unit(SIDE,side,U_OVERLORD));int provider=find_egg(SIDE,U_OVERLORD,-1);assert(provider>=0);
    assert(race_shared_egg_cancel(SIDE,provider));assert(minerals==ore);
    /* Exhaust pool using real spawn allocation; admission must not debit. */
    while(spawn_unit(ENEMY,U_ZERGLING,900,900)>=0){}
    b[side].larva=1;ore=minerals;
    assert(!race_queue_unit(SIDE,side,U_DRONE));
    assert(minerals==ore && b[side].larva==1);
}
int main(void)
{
    test_late_z9();test_custom_owners();test_pair_and_denials();
    puts("shared Egg HOST lifecycle and save fence passed");return 0;
}
