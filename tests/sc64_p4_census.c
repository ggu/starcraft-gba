/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "../starcraft/game.c"
#include <assert.h>
#ifndef SC64_P4_COUNT_CENSUS_ENABLED
#error "P4 census fixture must test the enabled implementation"
#endif
#include <stdio.h>

static u32 rng_state=0x91e10da5u;
static u32 rnd(void) { rng_state=rng_state*1664525u+1013904223u; return rng_state; }

int main(void)
{
    static const int owners[]={SIDE,ENEMY,RESCUABLE,4,NEUTRAL};
    static const int rawowners[]={-1,0,1,2,3,4,5,6,7,12};
    static const int unittypes[]={U_ZP_RAYNOR_VULTURE,U_TASSADAR,U_MARINE,U_ZEALOT,U_PROBE};
    static const int unitraws[]={-1,19,79,0,7,130,229};
    static const int buildingtypes[]={B_NEXUS,B_GATEWAY,B_ASSIMILATOR,B_PYLON};
    static const int buildingraws[]={-1,154,160,157,156,231};
    int l,t,r,b0;
    gba_init(); game_init();
    assert(race_p4_location(SC64_P4_LOCATION_ESCAPE,&l,&t,&r,&b0));
    for(unsigned round=0;round<500;++round) {
        memset(u,0,sizeof(u)); memset(b,0,sizeof(b));
        for(int i=0;i<MAX_UNITS;++i) {
            if(rnd()%4u==0) continue;
            u[i].alive=1; u[i].owner=owners[rnd()%5u]; u[i].raw_owner=rawowners[rnd()%10u];
            u[i].type=unittypes[rnd()%5u]; u[i].raw_type=unitraws[rnd()%7u];
            u[i].flags=(u16)rnd();
            u[i].x=(int)(rnd()%(WORLD_W+41u))-20;
            u[i].y=(int)(rnd()%(WORLD_H+41u))-20;
        }
        for(int i=0;i<MAX_BUILDINGS;++i) {
            if(rnd()%4u==0) continue;
            b[i].alive=1; b[i].owner=owners[rnd()%5u]; b[i].raw_owner=rawowners[rnd()%10u];
            b[i].type=buildingtypes[rnd()%4u]; b[i].raw_type=buildingraws[rnd()%6u];
            b[i].flags=(u16)rnd();
            b[i].x=(int)(rnd()%(WORLD_W+41u))-20;
            b[i].y=(int)(rnd()%(WORLD_H+41u))-20;
        }
        /* Exercise exact left/top inclusion and right/bottom exclusion. */
        for(int i=0;i<5 && i<MAX_UNITS;++i) {
            u[i].alive=1; u[i].owner=SIDE; u[i].raw_owner=(i&1)?0:7;
            u[i].raw_type=(i&1)?79:19; u[i].type=(i&1)?U_TASSADAR:U_ZP_RAYNOR_VULTURE;
            if(i==0) { u[i].x=l; u[i].y=t; }
            if(i==1) { u[i].x=r-1; u[i].y=b0-1; }
            if(i==2) { u[i].x=r; u[i].y=t; }
            if(i==3) { u[i].x=l; u[i].y=b0; }
            if(i==4) { u[i].x=l-1; u[i].y=t; }
        }
        RaceP4CountSnapshot got;
        assert(!race_p4_count_snapshot(NULL));
        assert(race_p4_count_snapshot(&got));
        assert(got.side_any==race_p4_count_type(13,229,0,0));
        assert(got.all_ray==race_p4_count_type(17,19,0,0));
        assert(got.all_tass==race_p4_count_type(17,79,0,0));
        assert(got.cur_ray==race_p4_count_type(13,19,0,0));
        assert(got.cur_tass==race_p4_count_type(13,79,0,0));
        assert(got.esc_ray==race_p4_count_type(13,19,0,1));
        assert(got.esc_tass==race_p4_count_type(13,79,0,1));
    }
    puts("PASS P4 census matches all seven reference predicates over 500 randomized pools and escape-boundary placements");
    return 0;
}
