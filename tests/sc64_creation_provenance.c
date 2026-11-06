/* SPDX-License-Identifier: GPL-3.0-or-later
 * Actual campaign and custom creation paths; source data remains private. */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/game.c"

static int find_site(int worker, int type, int *x, int *y)
{
    for (int ty=3;ty<race_rows-3;++ty) for(int tx=3;tx<race_cols-3;++tx) {
        int px=tx*16,py=ty*16;
        if (race_build_request_valid(SIDE,worker,type,&px,&py,0)) { *x=px;*y=py;return 1; }
    }
    return 0;
}
static int first_unit(int owner,int type)
{
    for(int i=0;i<MAX_UNITS;++i)if(u[i].alive&&u[i].owner==owner&&u[i].type==type)return i;
    return -1;
}
static void test_foundation(int mission)
{
    int worker, x, y, created;
    game_init();race_mode=0;race_selected_mission=mission;init_state();
    assert(screen==SCREEN_PLAY);
    fprintf(stderr,"expected mission=%d active=%d race=%d map=%s\n", mission,race_selected_mission,race_human,sc64_race_maps[race_selected_mission].mission_id);
    for(int j=0;j<MAX_UNITS;++j) if(u[j].alive && (u[j].raw_type==64 || sc64_race_raw_unit(u[j].type)==64))
        fprintf(stderr,"probe slot=%d live=%d raw_owner=%d type=%d raw_type=%d\n",j,u[j].owner,u[j].raw_owner,u[j].type,u[j].raw_type);
    assert(race_selected_mission==mission);
    const Sc64RaceMap *map=&sc64_race_maps[mission];
    assert(map->human_slot==7 && race_campaign_owner(map,7)==SIDE);
    worker=-1;
    for(int j=0;j<MAX_UNITS;++j) if(u[j].alive && u[j].owner==SIDE &&
        (u[j].raw_type==64 || sc64_race_raw_unit(u[j].type)==64)) { worker=j; break; }
    if(mission==SC64_RACE_MISSION_P1) {
        /* Source P1 has no placed Probe. Move a real SIDE unit into the
         * Antioch rescue region, then use the rescued Nexus production path. */
        assert(worker<0);
        int visitor=-1,nexus=-1,pylon=-1;
        for(int j=0;j<MAX_UNITS;++j) if(u[j].alive&&u[j].owner==SIDE) {visitor=j;break;}
        for(int j=0;j<MAX_BUILDINGS;++j) if(b[j].alive&&b[j].owner==RESCUABLE&&b[j].raw_owner==1) {
            if(b[j].x>=224&&b[j].x<=784&&b[j].y>=848&&b[j].y<=1200) {
                if(b[j].type==B_NEXUS)nexus=j;
                if(b[j].type==B_PYLON)pylon=j;
            }
        }
        assert(visitor>=0&&nexus>=0&&pylon>=0);
        u[visitor].x=400;u[visitor].y=900;race_campaign_tick();
        assert(b[nexus].owner==SIDE&&b[nexus].raw_owner==1);
        assert(b[pylon].owner==SIDE&&b[pylon].raw_owner==1);
        minerals=gas=10000;
        /* The rescued force starts at 44/34 supply. Add one completed
         * fixture Pylon so ordinary Probe admission can proceed. */
        int extra_pylon=spawn_build(SIDE,B_PYLON,b[nexus].x+48,b[nexus].y,0);
        assert(extra_pylon>=0);b[extra_pylon].raw_owner=7;
        fprintf(stderr,"Nexus owner=%d raw=%d complete=%d race=%d queue=%d supply=%d/%d resources=%d/%d\n",b[nexus].owner,b[nexus].raw_owner,building_complete(nexus),race_owner_identity(SIDE),b[nexus].queue_count,race_owner_reserved_supply(SIDE),race_owner_supply_cap(SIDE),minerals,gas);
        assert(race_queue_unit(SIDE,nexus,U_PROBE));
        unsigned char before[MAX_UNITS];for(int j=0;j<MAX_UNITS;++j)before[j]=(unsigned char)u[j].alive;
        b[nexus].queue_progress=(sc64_race_unit_rule(U_PROBE)->build_frames*5+3)/4-1;
        for(int tick=0;tick<10 && b[nexus].queue_count;++tick)race_tick_buildings();
        assert(!b[nexus].queue_count);
        for(int j=0;j<MAX_UNITS;++j)if(!before[j]&&u[j].alive&&u[j].type==U_PROBE) {worker=j;break;}
    }
    assert(worker>=0&&u[worker].raw_owner==7);
    minerals=gas=10000;
    assert(find_site(worker,B_FORGE,&x,&y));
    unsigned char old_buildings[MAX_BUILDINGS];
    for(int j=0;j<MAX_BUILDINGS;++j)old_buildings[j]=(unsigned char)b[j].alive;
    assert(race_start_build(SIDE,worker,B_FORGE,x,y));
    u[worker].x=x;u[worker].y=y;race_pending_build_tick(worker);
    created=-1;
    for(int j=0;j<MAX_BUILDINGS;++j)if(!old_buildings[j]&&b[j].alive&&b[j].type==B_FORGE){created=j;break;}
    /* Protoss builder may already be released after warp-in begins. */
    assert(created>=0&&created<MAX_BUILDINGS&&b[created].type==B_FORGE);
    assert(b[created].raw_owner==7);
    assert(race_save_game());
    b[created].raw_owner=-1;
    assert(race_load_game());
    assert(b[created].raw_owner==7);
}
static void test_product(int mission)
{
    game_init();race_mode=0;race_selected_mission=mission;init_state();
    int gateway=-1;
    for(int i=0;i<MAX_BUILDINGS;++i) if(b[i].alive&&b[i].owner==SIDE&&b[i].type==B_GATEWAY) {gateway=i;break;}
    if(gateway<0) { int worker=first_unit(SIDE,U_PROBE);if(worker<0){worker=spawn_unit(SIDE,U_PROBE,128,128);assert(worker>=0);u[worker].raw_owner=7;} gateway=spawn_build(SIDE,B_GATEWAY,u[worker].x+24,u[worker].y+24,0);assert(gateway>=0);b[gateway].raw_owner=7; }
    if (!race_site_powered(SIDE,b[gateway].x,b[gateway].y)) {
        int pylon=spawn_build(SIDE,B_PYLON,b[gateway].x+24,b[gateway].y,0);
        assert(pylon>=0); b[pylon].raw_owner=7;
    }
    b[gateway].under_construction=0;b[gateway].queue_head=0;b[gateway].queue_count=1;
    b[gateway].queue[0]=U_DRAGOON;b[gateway].queue_progress=(sc64_race_unit_rule(U_DRAGOON)->build_frames*5+3)/4-1;
    /* Isolate provenance from tech/power admission; exercise actual completed queue. */
    unsigned char before[MAX_UNITS];for(int i=0;i<MAX_UNITS;++i)before[i]=(unsigned char)u[i].alive;
    for(int tick=0;tick<10 && b[gateway].queue_count;++tick)race_tick_buildings();
    assert(!b[gateway].queue_count);
    int found=0;for(int i=0;i<MAX_UNITS;++i)if(!before[i]&&u[i].alive&&u[i].type==U_DRAGOON&&u[i].raw_owner==7)++found;
    assert(found==1);
}
#ifdef SC64_P4_RUNTIME
static void test_p4_rescue(void)
{
    game_init();race_mode=0;race_selected_mission=SC64_RACE_MISSION_P4;init_state();
    assert(race_p4_active());race_p4.source_paused=0;
    int human=spawn_unit(SIDE,U_PROBE,500,500);
    int rescued=spawn_unit(RESCUABLE,U_PROBE,502,500);
    int building=spawn_build(RESCUABLE,B_FORGE,504,500,0);
    assert(human>=0 && rescued>=0 && building>=0);
    u[human].raw_type=64;u[human].raw_owner=7;
    u[rescued].raw_type=64;u[rescued].raw_owner=1;
    b[building].raw_type=166;b[building].raw_owner=1;
    race_p4_rescue_tick();
    assert(u[rescued].owner==SIDE&&u[rescued].raw_owner==1);
    assert(b[building].owner==SIDE&&b[building].raw_owner==1);
    assert(race_creation_source_owner(SIDE,u[rescued].raw_owner)==7);
}
#endif
int main(void)
{
    gba_init();
    test_foundation(SC64_RACE_MISSION_P1); test_foundation(SC64_RACE_MISSION_P3);
    test_product(SC64_RACE_MISSION_P1); test_product(SC64_RACE_MISSION_P3);
    race_mode=0; race_selected_mission=SC64_RACE_MISSION_P1;
    assert(race_creation_source_owner(SIDE,1)==7); /* rescued origin stays on actor */
    assert(race_creation_source_owner(ENEMY,-1)==-1);
    race_selected_mission=SC64_RACE_MISSION_P3;
    assert(race_creation_source_owner(ENEMY,0)==0);
    assert(race_creation_source_owner(4,6)==6);
    race_mode=1;
    for(int owner=0;owner<5;++owner) {
        assert(race_creation_source_owner(owner,owner)==-1);
        int slot=race_spawn_product(owner,U_DRAGOON,540+owner*8,500,-1);
        assert(slot>=0&&u[slot].raw_owner==-1);
    }
#ifdef SC64_P4_RUNTIME
    test_p4_rescue();
#endif
    puts("shared creation provenance: P1/P3 PASS"); return 0;
}
