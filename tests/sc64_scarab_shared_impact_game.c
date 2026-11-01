/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Actual updates with five paid children; synthetic skirmish setup. */
#define main sc64_impact_unused_main
#include "sc64_impact_events_game.c"
#undef main
extern int sc64_test_sram_transfer(const char *,int);
extern int sc64_test_replace_payload(u32,const void *,u32);
extern int sc64_test_remove_sequence(u32);
int main(int argc,char **argv)
{
    assert(argc==1 || argc==3 || argc==5);
    setup();minerals=1000;
    int parent[5],child[5],target[5],hp[5];
    for(int k=0;k<5;k++) {
        int y=100+40*k;
        parent[k]=spawn_unit(SIDE,U_REAVER,128,y);target[k]=spawn_unit(ENEMY,U_MARINE,180,y);
        assert(parent[k]>=0 && target[k]>=0);u[target[k]].hp=u[target[k]].maxhp=200;hp[k]=u[target[k]].hp;
        assert(race_scarab_parent_init(parent[k]) && race_scarab_enqueue(parent[k],0));
        assert(race_scarab_production_tick(parent[k],0)==SC64_SCARAB_ALLOCATED);
        Sc64ScarabParent p;Sc64ScarabCounts c;assert(race_scarab_parent_read(parent[k],0,&p,&c));child[k]=p.active_slot;
        for(int j=0;j<105;j++)assert(race_scarab_production_tick(parent[k],0)==SC64_SCARAB_PROGRESS);
        assert(race_scarab_production_tick(parent[k],0)==SC64_SCARAB_COMPLETED);
        for(int j=0;j<MAX_UNITS;j++)u[j].selected=0;
        u[parent[k]].selected=1;assert(issue_order(ORDER_ATTACK_MOVE,u[target[k]].x,u[target[k]].y));
    }
    assert(spawn_unit(ENEMY,U_MARINE,700,700)>=0);
    int most=0;
    for(int n=0;n<200;n++) {
        screen=SCREEN_PLAY;game_update(0,0);int count=0;
        for(unsigned j=0;j<race_impact_count;j++)if(race_impact_rows[j].source_type==SC64_SCARAB_IMPACT_MARKER)count++;
        if(count>most)most=count;
    }
    assert(most>=5);
    for(int k=0;k<5;k++)assert(!u[child[k]].alive && u[target[k]].hp<hp[k]);
    /* Due ordinary hit kills the explicit target first. The later Scarab
     * packet must lose that reference but retain its positional splash. */
    setup();int first=spawn_unit(ENEMY,U_MARINE,160,160);
    int nearby=spawn_unit(ENEMY,U_MARINE,164,160);
    assert(first>=0 && nearby>=0);u[first].hp=1;u[nearby].hp=u[nearby].maxhp=200;
    Unit q={0};q.x=160;q.y=160;q.target=first;q.owner=SIDE;
    race_impact_launch(first,10,1,SIDE,SC64_STOCK_DMG_NORMAL,U_MARINE,1);
    assert(race_scarab_probe_bullet_create(&q) && race_impact_count==2);
    race_impact_rows[1].steps=(u8)((race_impact_rows[1].steps&15u)|16u);
    if(argc>=3)assert(sc64_test_sram_transfer(argv[1],1));
    assert(!race_save_game()); /* no tagged row is persisted */
    if(argc>=3)assert(sc64_test_sram_transfer(argv[2],1));
    frame=0;screen=SCREEN_PLAY;game_update(0,0);
    assert(!u[first].alive && u[nearby].hp==100 && race_impact_count==0);
    /* A checksummed new-tag record carrying the reserved marker must fail
     * impact decoding; a valid older slot wins. Removing it leaves the live
     * sample and SRAM untouched on rejection. */
    setup();first=spawn_unit(ENEMY,U_MARINE,160,160);assert(first>=0);
    assert(race_save_game());race_impact_launch(first,5,1,SIDE,SC64_STOCK_DMG_NORMAL,U_MARINE,200);
    assert(race_save_game());u32 len=0;
    assert(save_read_latest(&race_impact_record,sizeof(race_impact_record),SC64_SCARAB_SAVE_VERSION,&len));
    unsigned base=(unsigned)sizeof(RaceSave)-SC64_BC_SAVE_EXTRA_BYTES;
    assert(len==base+sc64_impact_save_bytes(1));
    ((u8 *)&race_impact_record)[base+sizeof(Sc64ImpactSaveHeader)+5]=SC64_SCARAB_IMPACT_MARKER;
    assert(sc64_test_replace_payload(SC64_SCARAB_SAVE_VERSION,&race_impact_record,len));
    RaceLoadPlan plan;GbaSaveSelection selected;
    assert(race_scarab_stage_select(&plan,&selected));u32 older_sequence=selected.sequence;
    assert(race_load_game() && race_impact_count==0);
    assert(sc64_test_remove_sequence(older_sequence));Unit before[MAX_UNITS];memcpy(before,u,sizeof(u));
    int old_m=minerals,old_g=gas,old_frame=frame;u32 old_seed=seed;
    if(argc==5)assert(sc64_test_sram_transfer(argv[3],1));
    assert(!race_load_game() && !memcmp(before,u,sizeof(u)) && old_m==minerals && old_g==gas && old_frame==frame && old_seed==seed);
    if(argc==5)assert(sc64_test_sram_transfer(argv[4],1));
    puts("five Scarabs, mixed due splash and marker save rejection passed");return 0;
}
