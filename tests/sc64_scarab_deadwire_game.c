/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Actual journal, synthetic hidden-child corruption and refusal checks. */
#define main sc64_impact_unused_main
#include "sc64_impact_events_game.c"
#undef main
#include <stdlib.h>
#include "../starcraft/sc64_scarab_unit_adapter.h"
extern int sc64_test_sram_transfer(const char *,int);
extern void sc64_test_sram_snapshot(void *);
extern int sc64_test_sram_matches(const void *);
extern int sc64_test_replace_payload(u32,const void *,u32);
extern int sc64_test_remove_sequence(u32);
static u8 snapshot[32768];
static int raw_encode(unsigned slot,const Unit *q) {
#if defined(SC64_ZP_FULL_STORAGE)
    return race_scarab_zp_encode(&race_data,slot,q,race_cols*16u,race_rows*16u);
#else
    return sc64_unit_save_encode(&race_data.units[slot],q,race_cols*16u,race_rows*16u);
#endif
}
int main(int argc,char **argv) {
    assert(argc==4);int map=atoi(argv[3]);assert(map==0 || map==1);
    setup();race_skirmish_map=map;race_skirmish_init();memset(u,0,sizeof(u));memset(b,0,sizeof(b));screen=SCREEN_PLAY;minerals=500;
    int p=spawn_unit(SIDE,U_REAVER,128,128);assert(p==0 && race_scarab_parent_init(p) && race_scarab_enqueue(p,0));
    assert(race_scarab_production_tick(p,0)==SC64_SCARAB_ALLOCATED);
    for(int i=0;i<20;i++)assert(race_scarab_production_tick(p,0)==SC64_SCARAB_PROGRESS);
    Sc64ScarabParent parent;Sc64ScarabCounts counts;assert(race_scarab_parent_read(p,0,&parent,&counts));int c=parent.active_slot;assert(c==1);
    assert(race_save_game() && race_save_game());Unit original=u[c],original_parent=u[p];
    for(int i=20;i<105;i++)assert(race_scarab_production_tick(p,0)==SC64_SCARAB_PROGRESS);
    assert(race_scarab_production_tick(p,0)==SC64_SCARAB_COMPLETED && race_scarab_launch_child(p,0,c));
    Unit launched=u[c],launched_parent=u[p],checked;
    assert(sc64_scarab_flight_to_wire(&launched,&checked,1,0));
    sc64_test_sram_snapshot(snapshot);assert(!race_save_game() && sc64_test_sram_matches(snapshot));
    assert(!memcmp(&launched,&u[c],sizeof(Unit)) && !memcmp(&launched_parent,&u[p],sizeof(Unit)));
    u[c]=original;u[p]=original_parent;
    sc64_test_sram_snapshot(snapshot);u[c].alive=0;u[c].raw_type=84;assert(!race_save_game() && sc64_test_sram_matches(snapshot));u[c]=original;
    sc64_test_sram_snapshot(snapshot);u[c].alive=0;u[c].carry=-1;assert(!race_save_game() && sc64_test_sram_matches(snapshot));u[c]=original;
    /* A valid dead outside child still cannot satisfy a parent's live
     * training reference: canonicalization may not repair that graph. */
    sc64_test_sram_snapshot(snapshot);u[c].alive=0;u[c].carry=(p|(2<<8));u[c].work_timer=50;u[c].order=2;
    assert(!race_save_game() && sc64_test_sram_matches(snapshot));u[c]=original;
    /* A pending marker refuses save without touching SRAM. */
    Unit bullet={0};bullet.x=160;bullet.y=160;bullet.owner=SIDE;bullet.target=-1;
    assert(race_scarab_probe_bullet_create(&bullet));sc64_test_sram_snapshot(snapshot);
    assert(!race_save_game() && sc64_test_sram_matches(snapshot));race_impact_reset();
    /* Replace the newest authenticated new-tag child with a syntactically
     * encodable dead outside row. The unchanged reader must reject it. */
    u32 len=0;assert(save_read_latest(&race_impact_record,sizeof(race_impact_record),SC64_SCARAB_SAVE_VERSION,&len));
    race_scarab_stage_wire=1;Unit logical,wire;
    assert(race_save_decode_unit(&race_data,(unsigned)c,race_cols*16u,race_rows*16u,&logical));
    logical.alive=0;logical.carry=(p|(2<<8));logical.work_timer=50;logical.order=2;
    assert(sc64_scarab_flight_to_wire(&logical,&wire,1,0) && raw_encode((unsigned)c,&wire));
    assert(sc64_test_replace_payload(SC64_SCARAB_SAVE_VERSION,&race_impact_record,len));
    RaceLoadPlan plan;GbaSaveSelection selected;assert(race_scarab_stage_select(&plan,&selected));u32 old=selected.sequence;
    assert(race_load_game());assert(sc64_test_remove_sequence(old));
    Unit before[MAX_UNITS];Building before_b[MAX_BUILDINGS];memcpy(before,u,sizeof(u));memcpy(before_b,b,sizeof(b));
    int m=minerals,g=gas,f=frame;u32 rng=seed;sc64_test_sram_snapshot(snapshot);assert(sc64_test_sram_transfer(argv[1],1));
    assert(!race_load_game() && !memcmp(before,u,sizeof(u)) && !memcmp(before_b,b,sizeof(b)) && m==minerals && g==gas && f==frame && rng==seed && sc64_test_sram_matches(snapshot));
    assert(sc64_test_sram_transfer(argv[2],1));puts("live/dead/marker refusal and malformed dead fallback passed");return 0;
}
