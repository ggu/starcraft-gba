/* SPDX-License-Identifier: GPL-3.0-or-later */
/* HOST-only journal records; never seed these constructed rows into guest SRAM. */
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"
extern void guardian_test_reset(void);
extern void guardian_test_snapshot(void *);
extern void guardian_test_retag(int,u32,u32);
extern void guardian_test_replace(int,const void *,u32);
static int add_actor(int type,int raw,int target,int timer,int x)
{
    int slot=spawn_unit(SIDE,U_OVERLORD,x,100);assert(slot>=0);
    Unit *q=&u[slot];
    /* Preserve the cache initialized by spawn, including P10 Arbiter state. */
    u16 spawned_weapon=q->weapon;memset(q,0,sizeof(*q));q->weapon=spawned_weapon;
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
static void assert_failed_load_atomic(void)
{
    static Unit old_units[MAX_UNITS];static Building old_buildings[MAX_BUILDINGS];
    static u8 old_explored[sizeof(explored)],old_visible[sizeof(visible)];
    static u8 old_minerals[sizeof(m)],old_geysers[sizeof(geysers)];
    static u8 old_impacts[sizeof(race_impact_rows)],old_front[sizeof(race_front)];
    static int old_players[5],old_ore[5],old_gas[5],old_bots[3];
    Sc64SkTransportTech old_tech=race_skirmish_transport_tech;
    memcpy(old_units,u,sizeof(u));memcpy(old_buildings,b,sizeof(b));
    memcpy(old_explored,explored,sizeof(explored));memcpy(old_visible,visible,sizeof(visible));
    memcpy(old_minerals,m,sizeof(m));memcpy(old_geysers,geysers,sizeof(geysers));
    memcpy(old_impacts,race_impact_rows,sizeof(race_impact_rows));memcpy(old_front,&race_front,sizeof(race_front));
    memcpy(old_players,race_player,sizeof(race_player));memcpy(old_ore,race_minerals,sizeof(race_minerals));
    memcpy(old_gas,race_gas,sizeof(race_gas));memcpy(old_bots,race_bot_races,sizeof(race_bot_races));
    u32 old_seed=seed;int old_count=race_impact_count;
    int before[]={minerals,gas,supply_used,supply_cap,race_selected_mission,race_mode,race_human,
        race_bots,race_completed,race_skirmish_map,frame,screen,mode,menu,menu_open,menu_cursor,
        help,won,lost,result_timer,selected_building,geyser_count,(int)race_front_transition,race_front_load_error};
    assert(!race_load_game());
    assert(!memcmp(old_units,u,sizeof(u)) && !memcmp(old_buildings,b,sizeof(b)));
    assert(!memcmp(old_explored,explored,sizeof(explored)) && !memcmp(old_visible,visible,sizeof(visible)));
    assert(!memcmp(old_minerals,m,sizeof(m)) && !memcmp(old_geysers,geysers,sizeof(geysers)));
    assert(!memcmp(old_impacts,race_impact_rows,sizeof(race_impact_rows)) && old_count==race_impact_count);
    assert(!memcmp(old_front,&race_front,sizeof(race_front)) && !memcmp(&old_tech,&race_skirmish_transport_tech,sizeof(old_tech)));
    assert(!memcmp(old_players,race_player,sizeof(race_player)) && !memcmp(old_ore,race_minerals,sizeof(race_minerals)));
    assert(!memcmp(old_gas,race_gas,sizeof(race_gas)) && !memcmp(old_bots,race_bot_races,sizeof(race_bot_races)));
    int after[]={minerals,gas,supply_used,supply_cap,race_selected_mission,race_mode,race_human,
        race_bots,race_completed,race_skirmish_map,frame,screen,mode,menu,menu_open,menu_cursor,
        help,won,lost,result_timer,selected_building,geyser_count,(int)race_front_transition,race_front_load_error};
    assert(!memcmp(before,after,sizeof(before)) && old_seed==seed);
}
static void reject_unauthenticated_legacy_egg(void)
{
    static u8 before[32768],after[32768];
    start_custom();
    assert(add_actor(SC64_SHARED_EGG_TYPE,36,U_DRONE,1,140)>=0);
    assert(race_save_game());
    RaceLoadPlan plan; GbaSaveSelection selected;
    assert(race_worker_cargo_select(&plan,&selected));
    assert(selected.tag==SC64_SHARED_EGG_TRANSPORT_SAVE_VERSION);
    guardian_test_retag(selected.slot,SC64_TRANSPORT_SAVE_VERSION,SAVE_VERSION);
    guardian_test_snapshot(before);
    assert_failed_load_atomic();
    guardian_test_snapshot(after);assert(!memcmp(before,after,sizeof(before)));

    /* An invalid newer legacy-tagged Egg must fall back to the older valid
     * ordinary journal, without rewriting either SRAM record. */
    start_custom();int old_slot=spawn_unit(SIDE,U_OVERLORD,100,100);assert(old_slot>=0);
    u[old_slot].raw_type=36;Unit old=u[old_slot];assert(race_save_game());
    assert(race_worker_cargo_select(&plan,&selected));
    guardian_test_retag(selected.slot,SC64_TRANSPORT_SAVE_VERSION,SAVE_VERSION);
    assert(add_actor(SC64_SHARED_EGG_TYPE,36,U_DRONE,1,140)>=0);
    assert(race_save_game());assert(race_worker_cargo_select(&plan,&selected));
    guardian_test_retag(selected.slot,SC64_TRANSPORT_SAVE_VERSION,SAVE_VERSION);
    guardian_test_snapshot(before);assert(race_load_game());
    guardian_test_snapshot(after);assert(!memcmp(before,after,sizeof(before)));
    assert(!memcmp(&u[old_slot],&old,sizeof(old)));
    assert(race_worker_cargo_select(&plan,&selected));
    assert(selected.tag==SC64_TRANSPORT_SAVE_VERSION && plan.egg_auth.features==0);
}

int main(void)
{
    reject_unauthenticated_legacy_egg();
    start_custom();int egg=add_actor(SC64_SHARED_EGG_TYPE,36,U_DRONE,1,140);
    assert(race_save_game());RaceLoadPlan plan;GbaSaveSelection selected;
    assert(race_worker_cargo_select(&plan,&selected));
    assert(selected.tag==SC64_SHARED_EGG_TRANSPORT_SAVE_VERSION && plan.egg_auth.features==SC64_STAGE_SAVE_EGG);
    u[egg].work_timer=2;assert(race_load_game() && u[egg].work_timer==1);
    guardian_test_retag(selected.slot,SC64_TRANSPORT_SAVE_VERSION,SC64_SHARED_EGG_SAVE_VERSION);
    assert_failed_load_atomic();
    puts("Egg-only HOST journal: round trip, tag mismatch, legacy rejection and fallback PASS");return 0;
}
