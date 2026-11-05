/* SPDX-License-Identifier: GPL-3.0-or-later */
/* HOST-only journal records; never seed these constructed rows into guest SRAM. */
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"
extern void guardian_test_reset(void);
extern void guardian_test_retag(int,u32,u32);
extern void guardian_test_replace(int,const void *,u32);
static int add_actor(int type,int raw,int target,int timer,int x)
{
    int slot=spawn_unit(SIDE,U_OVERLORD,x,100);assert(slot>=0);
    Unit *q=&u[slot];memset(q,0,sizeof(*q));
    q->alive=1;q->selected=1;q->owner=SIDE;q->type=type;q->raw_type=raw;q->raw_owner=-1;
    q->x=q->tx=q->path_x=x;q->y=q->ty=q->path_y=100;
    q->hp=q->maxhp=200;q->armor=type==SC64_SHARED_EGG_TYPE ? 10 : 0;
    q->order=ORDER_IDLE;q->target=target;q->work_timer=timer;q->build=-1;
    return slot;
}
static void start_custom(void)
{
    guardian_test_reset();game_init();race_mode=1;race_human=R_ZERG;race_bots=1;
    race_skirmish_map=0;race_bot_races[0]=R_PROTOSS;init_state();
    assert(screen==SCREEN_PLAY && race_mode);
}
static void exercise(unsigned features)
{
    start_custom();int ordinary=spawn_unit(SIDE,U_OVERLORD,100,100);assert(ordinary>=0);
    u[ordinary].raw_type=36;Unit old=u[ordinary];int cocoon=-1,egg=-1;
    if(features&SC64_STAGE_SAVE_GUARDIAN)cocoon=add_actor(U_GUARDIAN_COCOON,59,44,750,120);
    if(features&SC64_STAGE_SAVE_EGG)egg=add_actor(SC64_SHARED_EGG_TYPE,36,U_DRONE,1,140);
    assert(race_save_game());RaceLoadPlan plan;GbaSaveSelection selected;
    assert(race_worker_cargo_select(&plan,&selected));
    assert(selected.tag==SC64_GUARDIAN_EGG_TRANSPORT_SAVE_VERSION);
    if(features!= (SC64_STAGE_SAVE_GUARDIAN|SC64_STAGE_SAVE_EGG)) {
        u32 outer=features==SC64_STAGE_SAVE_GUARDIAN ? SC64_GUARDIAN_TRANSPORT_SAVE_VERSION :
                  features==SC64_STAGE_SAVE_EGG ? SC64_SHARED_EGG_TRANSPORT_SAVE_VERSION : SC64_TRANSPORT_SAVE_VERSION;
        u32 inner=features==SC64_STAGE_SAVE_GUARDIAN ? SC64_GUARDIAN_SAVE_VERSION :
                  features==SC64_STAGE_SAVE_EGG ? SC64_SHARED_EGG_SAVE_VERSION : SAVE_VERSION;
        guardian_test_retag(selected.slot,outer,inner);
    }
    assert(race_worker_cargo_select(&plan,&selected));assert(plan.guardian_auth.features==features);
    u[ordinary].raw_type=42;if(cocoon>=0)u[cocoon].hp=1;if(egg>=0)u[egg].work_timer=2;
    assert(race_load_game());assert(!memcmp(&u[ordinary],&old,sizeof(old)));
    if(cocoon>=0)assert(u[cocoon].alive && u[cocoon].type==U_GUARDIAN_COCOON && u[cocoon].work_timer==750);
    if(egg>=0)assert(u[egg].alive && u[egg].type==SC64_SHARED_EGG_TYPE && u[egg].work_timer==1);
    if(features==(SC64_STAGE_SAVE_GUARDIAN|SC64_STAGE_SAVE_EGG)) {
        /* A checksum-valid outer/inner mismatch cannot publish either actor. */
        guardian_test_retag(selected.slot,SC64_GUARDIAN_TRANSPORT_SAVE_VERSION,SC64_GUARDIAN_EGG_SAVE_VERSION);
        static Unit before_units[MAX_UNITS];static Building before_buildings[MAX_BUILDINGS];
        memcpy(before_units,u,sizeof(u));memcpy(before_buildings,b,sizeof(b));
        int old_minerals=minerals,old_gas=gas,old_screen=screen,old_mode=race_mode;
        assert(!race_load_game());
        assert(!memcmp(before_units,u,sizeof(u)) && !memcmp(before_buildings,b,sizeof(b)));
        assert(minerals==old_minerals && gas==old_gas && screen==old_screen && race_mode==old_mode);
        assert(!race_worker_cargo_select(&plan,&selected) && !plan.guardian_auth.candidate);
    }
}
static void mixed_fallback(void)
{
    start_custom();int ordinary=spawn_unit(SIDE,U_OVERLORD,100,100);assert(ordinary>=0);
    u[ordinary].raw_type=36;Unit legacy=u[ordinary];assert(race_save_game());
    RaceLoadPlan plan;GbaSaveSelection selected;
    assert(race_worker_cargo_select(&plan,&selected));
    guardian_test_retag(selected.slot,SC64_TRANSPORT_SAVE_VERSION,SAVE_VERSION);
    assert(add_actor(U_GUARDIAN_COCOON,59,44,750,120)>=0);
    int egg=add_actor(SC64_SHARED_EGG_TYPE,36,U_DRONE,1,140);assert(race_save_game());
    assert(race_worker_cargo_select(&plan,&selected));
    assert(selected.tag==SC64_GUARDIAN_EGG_TRANSPORT_SAVE_VERSION);
    static u8 envelope[sizeof(RaceImpactRecord)];u32 length=0;
    assert(save_read_latest(envelope,sizeof(envelope),SC64_GUARDIAN_EGG_TRANSPORT_SAVE_VERSION,&length));
    Unit bad=u[egg];bad.raw_owner=3; /* checksum-valid, candidate-owner invalid */
#ifdef SC64_ZP_FULL_STORAGE
#ifdef SC64_P7_TYPED_COMPACT_ENABLED
    assert(sc64_p7_typed_unit_encode_for(&race_data.units[egg],&bad,race_cols*16u,race_rows*16u,1));
#elif defined(SC64_Z7_DERIVED_SAVE)
    assert(sc64_derived_unit_encode(&race_data.units[egg],&bad,race_cols*16u,race_rows*16u));
#else
    assert(sc64_zp_unit_save_encode_for(&race_data.units[egg],&bad,race_cols*16u,race_rows*16u,1));
#endif
#else
    assert(sc64_unit_save_encode_for(&race_data.units[egg],&bad,race_cols*16u,race_rows*16u,1));
#endif
    memcpy(envelope+offsetof(RaceSave,units)+egg*sizeof(race_data.units[0]),
           &race_data.units[egg],sizeof(race_data.units[0]));
    guardian_test_replace(selected.slot,envelope,length);
    assert(race_load_game());assert(!memcmp(&u[ordinary],&legacy,sizeof(legacy)));
    assert(race_worker_cargo_select(&plan,&selected));
    assert(selected.tag==SC64_TRANSPORT_SAVE_VERSION && plan.guardian_auth.features==0);
}
int main(void)
{
    exercise(0);exercise(SC64_STAGE_SAVE_GUARDIAN);exercise(SC64_STAGE_SAVE_EGG);
    exercise(SC64_STAGE_SAVE_GUARDIAN|SC64_STAGE_SAVE_EGG);mixed_fallback();
    puts("Guardian/Egg real journal feature contexts passed");return 0;
}
