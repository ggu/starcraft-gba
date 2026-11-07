/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Authored HOST P1 rescue -> real Nexus product -> real Probe foundation. */
#define main sc64_old_creation_fixture_main
#include "sc64_creation_provenance.c"
#undef main
int main(void)
{
    int visitor=-1,nexus=-1,worker=-1,x,y,forge=-1;
    gba_init(); game_init();
    race_mode=0;race_selected_mission=-1;assert(sc64_controller_human_source()==-1);
    race_selected_mission=SC64_RACE_MAP_COUNT;assert(sc64_controller_human_source()==-1);
    race_mode=1;race_selected_mission=SC64_RACE_MISSION_P1;assert(sc64_controller_human_source()==-1);
    assert(!race_load_game());
    race_mode=0;race_selected_mission=SC64_RACE_MISSION_P3;init_state();
    assert(screen==SCREEN_PAUSE && mission_unsupported_count>0);
    race_mode=0;race_selected_mission=SC64_RACE_MISSION_P1;init_state();
    assert(screen==SCREEN_PLAY && sc64_controller_human_source()==7);
    for(int i=0;i<MAX_UNITS;++i) if(u[i].alive && u[i].owner==SIDE && sc64_controller_unit(i)==7) { visitor=i; break; }
    for(int i=0;i<MAX_BUILDINGS;++i) if(b[i].alive && b[i].owner==RESCUABLE && b[i].raw_owner==1 && b[i].type==B_NEXUS && b[i].x>=224 && b[i].x<=784 && b[i].y>=848 && b[i].y<=1200) { nexus=i; break; }
    assert(visitor>=0 && nexus>=0);u[visitor].x=400;u[visitor].y=900;race_campaign_tick();
    assert(b[nexus].owner==SIDE && b[nexus].raw_owner==1 && sc64_controller_building(nexus)==7);
    minerals=gas=10000;assert(spawn_build(SIDE,B_PYLON,b[nexus].x+48,b[nexus].y,0)>=0);
    assert(race_queue_unit(SIDE,nexus,U_PROBE));
    unsigned char before[MAX_UNITS];for(int i=0;i<MAX_UNITS;++i)before[i]=(unsigned char)u[i].alive;
    b[nexus].queue_progress=(sc64_race_unit_rule(U_PROBE)->build_frames*5+3)/4-1;
    for(int tick=0;tick<10 && b[nexus].queue_count;++tick)race_tick_buildings();
    assert(!b[nexus].queue_count);
    for(int i=0;i<MAX_UNITS;++i)if(!before[i]&&u[i].alive&&u[i].type==U_PROBE){worker=i;break;}
    assert(worker>=0 && sc64_controller_unit(worker)==7);
    minerals=gas=10000;assert(find_site(worker,B_FORGE,&x,&y));
    unsigned char old[MAX_BUILDINGS];for(int i=0;i<MAX_BUILDINGS;++i)old[i]=(unsigned char)b[i].alive;
    assert(race_start_build(SIDE,worker,B_FORGE,x,y));u[worker].x=x;u[worker].y=y;race_pending_build_tick(worker);
    for(int i=0;i<MAX_BUILDINGS;++i)if(!old[i]&&b[i].alive&&b[i].type==B_FORGE){forge=i;break;}
    assert(forge>=0 && sc64_controller_building(forge)==7);
    assert(!race_save_game() && !race_load_game());
    puts("P1 actual rescue and creator controller: PASS");return 0;
}
