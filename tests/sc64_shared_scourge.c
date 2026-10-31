/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Real race-engine Scourge admission, pair reservation and suicide impact. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"
#ifndef SC64_SHARED_SCOURGE_COMBAT
#error "This fixture requires shared Scourge combat"
#endif
static void setup(void)
{
    game_init(); race_mode=1; race_human=R_ZERG; race_bots=0; init_state();
    minerals=gas=5000; weapons_level=0;
}
static int raw_count(int raw)
{
    int n=0;for(int i=0;i<MAX_UNITS;i++) if(u[i].alive && u[i].owner==SIDE && sc64_race_raw_unit(u[i].type)==raw) n++;return n;
}
static void production(void)
{
    setup();int hatch=-1;
    for(int i=0;i<MAX_BUILDINGS;i++) if(b[i].alive&&b[i].owner==SIDE&&b[i].type==B_HATCHERY){hatch=i;break;}
    assert(hatch>=0);int spire=spawn_build(SIDE,B_SPIRE,400,400,0);assert(spire>=0);
    assert(spawn_unit(SIDE,U_OVERLORD,450,450)>=0);
    int before=raw_count(47),bank_m=minerals,bank_g=gas,larvae=b[hatch].larva,reserved=race_owner_reserved_supply(SIDE);
    assert(race_queue_unit(SIDE,hatch,U_SCOURGE));
    assert(minerals==bank_m-25 && gas==bank_g-75 && b[hatch].larva==larvae-1);
    assert(race_owner_reserved_supply(SIDE)==reserved+2);
    assert(b[hatch].queue_count==1 && b[hatch].queue[b[hatch].queue_head]==U_SCOURGE);
    for(int i=0;i<800 && b[hatch].queue_count;i++) race_tick_buildings();
    assert(!b[hatch].queue_count && raw_count(47)==before+2);
    assert(race_owner_reserved_supply(SIDE)==reserved+2);
    /* Existing commitments must prevent oversubscription of the last slot. */
    setup(); hatch=-1;for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==SIDE&&b[i].type==B_HATCHERY){hatch=i;break;}
    assert(hatch>=0 && spawn_build(SIDE,B_SPIRE,400,400,0)>=0);
    assert(spawn_unit(SIDE,U_OVERLORD,450,450)>=0);
    assert(race_queue_unit(SIDE,hatch,U_SCOURGE));
    while(race_product_slots_available(1)) assert(spawn_unit(SIDE,U_MARINE,700,700)>=0);
    bank_m=minerals;bank_g=gas;larvae=b[hatch].larva;
    assert(!race_queue_unit(SIDE,hatch,U_SCOURGE));
    assert(minerals==bank_m && gas==bank_g && b[hatch].larva==larvae);
    /* Independent spawns may consume previously reserved slots. A completed
     * queue waits for both slots rather than birthing only half a pair. */
    int first=spawn_unit(SIDE,U_MARINE,700,700),second=spawn_unit(SIDE,U_MARINE,700,700);
    assert(first>=0 && second>=0);
    for(int i=0;i<800;i++) race_tick_buildings();
    assert(b[hatch].queue_count==1 && raw_count(47)==0);
    u[first].alive=0;for(int i=0;i<4;i++)race_tick_buildings();
    assert(b[hatch].queue_count==1 && raw_count(47)==0);
    u[second].alive=0;for(int i=0;i<4;i++)race_tick_buildings();
    assert(!b[hatch].queue_count && raw_count(47)==2);
}
static void combat(void)
{
    setup();memset(u,0,sizeof(u));memset(b,0,sizeof(b));memset(invincible_units,0,sizeof(invincible_units));
    supply_used=0;int sc=spawn_unit(SIDE,U_SCOURGE,400,400),ground=spawn_unit(ENEMY,U_MARINE,401,400),air=spawn_unit(ENEMY,U_WRAITH,402,400),ally=spawn_unit(SIDE,U_WRAITH,402,400);
    assert(sc>=0&&ground>=0&&air>=0&&ally>=0);
    assert(sc64_race_unit_damage_vs(U_SCOURGE,0)==0 && sc64_race_unit_damage_vs(U_SCOURGE,1)==110);
    assert(sc64_race_unit_range_vs(U_SCOURGE,1)==3 && sc64_race_unit_cooldown_vs(U_SCOURGE,1)==1);
    int invalid[]={-1,MAX_UNITS+MAX_BUILDINGS,0x7fffffff,ground,ally};
    for(unsigned i=0;i<sizeof(invalid)/sizeof(*invalid);i++) { tick_combat(&u[sc],invalid[i]);assert(u[sc].alive); }
    u[air].flags|=16;tick_combat(&u[sc],air);assert(u[sc].alive);u[air].flags&=~16;
    invincible_units[air]=1;tick_combat(&u[sc],air);assert(u[sc].alive);invincible_units[air]=0;
#ifdef SC64_T6_RUNTIME
    /* A P3-enabled build must still honor T6's separate burrow bit. */
    race_mode=0;race_selected_mission=SC64_T6_GLOBAL_ID;
    sc64_t6_burrowed_set(&race_t6,(unsigned)air,1);
    assert(!SC64_CAN_ATTACK_FROM(U_SCOURGE,air,SIDE));
    sc64_t6_burrowed_set(&race_t6,(unsigned)air,0);race_mode=1;
#endif
#if defined(SC64_T5_RUNTIME) || defined(SC64_P5_SHUTTLE_ENABLED) || defined(SC64_Z6_TRANSPORT_ENABLED)
    u[air].order=ORDER_TRANSPORTED;tick_combat(&u[sc],air);assert(u[sc].alive);u[air].order=ORDER_IDLE;
#endif
    u[air].hp=u[air].maxhp=80;u[air].shield=50;u[air].armor=3;weapons_level=2;
    int before=supply_used;tick_combat(&u[sc],air);
    assert(!u[sc].alive && supply_used==before-1 && u[air].shield==0 && u[air].hp==23);
    tick_combat(&u[sc],air);assert(u[air].hp==23);
}
int main(void){production();combat();puts("shared Scourge: pair and suicide passed");return 0;}
