/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Authored damage scenes on actual campaign adapters; no saved Q8 state. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../common/gba.c"
#include "../starcraft/game.c"
static unsigned scenes;
static void start(const char *mission)
{
    int selected=-1;
    for(int n=0;n<SC64_RACE_MAP_COUNT;++n)
        if(sc64_race_maps[n].mission_id&&!strcmp(sc64_race_maps[n].mission_id,mission))selected=n;
    assert(selected>=0);game_init();race_mode=0;race_selected_mission=selected;init_state();
    assert(screen==SCREEN_PLAY);
}
static int shielded_actor(void)
{
    for(int n=0;n<MAX_UNITS;++n) {
        const Sc64StockUnitProfile *stock=sc64_pr_q8_actor_stock(n);
        if(u[n].alive&&stock&&stock->shields&&u[n].maxhp>30&&
           u[n].raw_type!=67&&u[n].raw_type!=79&&!invincible_units[n]&&!(u[n].flags&16)&&
           !SC64_TRANSPORT_LOADED(&u[n])&&!SC64_CARRIER_STATE_HIDDEN(&u[n])&&
           !(sc64_game_unit_actor_flags(u[n].type)&(SC64_GAME_ACTOR_HIDDEN|SC64_GAME_ACTOR_INVINCIBLE)))return n;
    }
    assert(!"source scene needs an ordinary shielded actor");return -1;
}
static void isolate(int slot)
{
    int chosen_x=-1,chosen_y=-1;
    for(int y=64;y<WORLD_H-64&&chosen_x<0;y+=48)for(int x=64;x<WORLD_W-64;x+=48) {
        int clear=1;
        for(int n=0;n<MAX_UNITS;++n)if(n!=slot&&u[n].alive&&abs(u[n].x-x)<48&&abs(u[n].y-y)<48)clear=0;
        for(int n=0;n<MAX_BUILDINGS;++n)if(b[n].alive&&abs(b[n].x-x)<48&&abs(b[n].y-y)<48)clear=0;
        if(clear){chosen_x=x;chosen_y=y;break;}
    }
    assert(chosen_x>=0);u[slot].x=u[slot].tx=chosen_x;u[slot].y=u[slot].ty=chosen_y;
    u[slot].order=ORDER_IDLE;u[slot].target=-1;
    sc64_unit_bucket_move(&u[slot]);sc64_target_index_move(&u[slot]);
}
static void pulse(Sc64PsionicStormState *state,int x,int y,unsigned count)
{
    sc64_psionic_storm_init(state);assert(count<=SC64_STORM_MAX_ACTIVE);
    for(unsigned n=0;n<count;++n) {
        Sc64PsionicStorm *storm=&state->storm[n];storm->x=(u16)x;storm->y=(u16)y;
        storm->active=1;storm->pulses_left=SC64_STORM_PULSE_COUNT;
        storm->age_ticks=SC64_STORM_FIRST_PULSE_TICKS-1;
    }
    assert(sc64_psionic_storm_state_valid(state));
}
static unsigned hp(int slot)
{unsigned result;assert(sc64_pr_q8_actor_hp(slot,&result));return result;}
static unsigned shield(int slot)
{unsigned result;assert(sc64_pr_q8_actor_shield(slot,&result));return result;}
static int tick(const char *mission)
{
#ifdef SC64_P4_STORM_ENABLED
    if(!strcmp(mission,"P4"))return race_p4_storm_tick();
#endif
#ifdef SC64_P6_STORM_ENABLED
    if(!strcmp(mission,"P6"))return race_p6_storm_tick(0);
#endif
#ifdef SC64_Z6_STORM_ENABLED
    if(!strcmp(mission,"Z6"))return race_z6_storm_tick();
#endif
    assert(!"adapter absent");return 0;
}
static Sc64PsionicStormState *effects(const char *mission)
{
#ifdef SC64_P4_STORM_ENABLED
    if(!strcmp(mission,"P4"))return &race_p4.storm;
#endif
#ifdef SC64_P6_STORM_ENABLED
    if(!strcmp(mission,"P6"))return &race_p6.storm;
#endif
#ifdef SC64_Z6_STORM_ENABLED
    if(!strcmp(mission,"Z6"))return &race_z6.storm;
#endif
    assert(!"adapter absent");return 0;
}
static void adapter_scene(const char *mission)
{
    start(mission);int v=shielded_actor();isolate(v);
    unsigned before=(unsigned)u[v].maxhp*256u-64u;
    assert(sc64_pr_q8_commit_hp(v,before)&&sc64_pr_q8_commit_shield(v,384));
    u[v].armor=99; /* Ignore Armor must remain independent of this value. */
    pulse(effects(mission),u[v].x,u[v].y,2);
    assert(tick(mission));
    assert(hp(v)==before-(SC64_STORM_DAMAGE_PER_PULSE*256u-384u)&&!shield(v));
    before=hp(v);assert(sc64_pr_q8_commit_shield(v,128));
    pulse(effects(mission),u[v].x,u[v].y,1);assert(tick(mission));
    assert(hp(v)==before-SC64_STORM_DAMAGE_PER_PULSE*256u&&shield(v)==128);
    /* Overlapping pulses hit once; a later lethal pair accounts death once. */
    assert(sc64_pr_q8_commit_hp(v,128)&&sc64_pr_q8_commit_shield(v,0));
    int before_supply=supply_used,cost=u[v].owner==SIDE?unit_supply(u[v].type):0;
    pulse(effects(mission),u[v].x,u[v].y,2);
    assert(tick(mission));assert(!u[v].alive&&!u[v].hp&&!sc64_pr_hp_debt[v]);
    assert(supply_used==before_supply-cost);
    ++scenes;printf("%s fractional Storm, overlap and death: PASS\n",mission);
}
#ifdef SC64_P6_STORM_ENABLED
static void p6_trap(void)
{
    start("P6");int slot=-1;
    for(int n=0;n<MAX_BUILDINGS;++n) {
        int raw=b[n].raw_type;
        if(b[n].alive&&raw==SC64_P6_RAW_WALL_MISSILE_TRAP&&
           !invincible_buildings[n]&&!(b[n].flags&16)&&b[n].maxhp>30){slot=n;break;}
    }
    assert(slot>=0);unsigned before=(unsigned)b[slot].maxhp*256u-64u;
    b[slot].shield_remainder=13;
    assert(sc64_pr_q8_commit_hp(MAX_UNITS+slot,before));
    pulse(&race_p6.storm,b[slot].x,b[slot].y,2);
    assert(race_p6_storm_tick(0));
    assert(hp(MAX_UNITS+slot)==before-SC64_STORM_DAMAGE_PER_PULSE*256u);
    assert(b[slot].shield_remainder==13); /* Trap cooldown is not a shield phase. */
    puts("P6 trap fractional Storm and phase alias: PASS");
}
#endif
#ifdef SC64_Z6_STORM_ENABLED
static void z6_hallucination(void)
{
    start("Z6");int owner=race_z6_source_owner(2);
    int v=spawn_unit(owner,U_TASSADAR,64,64);assert(v>=0);
    u[v].raw_type=79;u[v].raw_owner=2;
    assert(race_z6_hallucination_mark_slot((unsigned)v));isolate(v);
    unsigned before=(unsigned)u[v].maxhp*256u-64u;
    assert(sc64_pr_q8_commit_hp(v,before)&&sc64_pr_q8_commit_shield(v,14*256+128));
    pulse(&race_z6.storm,u[v].x,u[v].y,2);assert(race_z6_storm_tick());
    assert(hp(v)==before-(28*256-(14*256+128))&&!shield(v));
    assert(sc64_pr_q8_commit_hp(v,128));
    pulse(&race_z6.storm,u[v].x,u[v].y,1);assert(race_z6_storm_tick());
    assert(!u[v].alive&&!u[v].hp&&!sc64_pr_hp_debt[v]);
    assert(!(u[v].flags&SC64_Z6_HALLUCINATION_FLAG)&&!u[v].work_timer);
    puts("Z6 one-packet hallucination and death cleanup: PASS");
}
#endif
int main(void)
{
    gba_init();
#ifdef SC64_P4_STORM_ENABLED
    adapter_scene("P4");
#endif
#ifdef SC64_P6_STORM_ENABLED
    adapter_scene("P6");p6_trap();
#endif
#ifdef SC64_Z6_STORM_ENABLED
    adapter_scene("Z6");z6_hallucination();
#endif
    assert(scenes==Q8_STORM_EXPECTED_SCENES);
    return 0;
}
