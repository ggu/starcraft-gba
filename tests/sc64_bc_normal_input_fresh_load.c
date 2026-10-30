/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Import the normal-input pause save into a new host process and load via HOME.
 * Link tests/sc64_yamato_host_sram_shim.c instead of common/gba.c. */
#include "../starcraft/game.c"
#include <assert.h>
#include <stdio.h>
extern int sc64_test_sram_transfer(const char *,int);
int main(int argc,char **argv) {
    assert(argc==2);gba_init();game_init();assert(screen==SCREEN_TITLE);
    assert(sc64_test_sram_transfer(argv[1],0));game_update(KEY_L,KEY_L);
    assert(screen==SCREEN_PLAY && race_mode && race_human==R_TER && race_bots==1 && !won && !lost);
    int cruiser=-1,flight=0,target=-1,caster=-1;
    for(int i=0;i<MAX_UNITS;++i) if(u[i].alive && u[i].owner==SIDE && sc64_race_raw_unit(u[i].type)==12) cruiser=i;
    for(int i=0;i<SC64_BC_SHOT_LIMIT;++i) if(sc64_bc_shots[i].state==SC64_BC_SHOT_FLIGHT){assert(!flight);flight=1;target=sc64_bc_shots[i].target;caster=sc64_bc_shots[i].caster;}
    assert(cruiser>=0 && flight && caster==cruiser && target>=0 && target<MAX_UNITS+MAX_BUILDINGS && target_alive(target));
    int key=sc64_battlecruiser_owner_key(race_mode,race_selected_mission,u[cruiser].owner,u[cruiser].raw_owner);
    assert(key>=0 && sc64_battlecruiser_tech_has(sc64_battlecruiser_tech_bits,(unsigned)key,SC64_BC_TECH_YAMATO));
    int before=target<MAX_UNITS?u[target].hp:b[target-MAX_UNITS].hp, initial_frame=frame;
    int active=1;
    for(int n=0;n<200 && active && !won && !lost;++n) {
        game_update(0,0); active=0;
        for(int i=0;i<SC64_BC_SHOT_LIMIT;++i) if(sc64_bc_shots[i].state) active=1;
    }
    int after=target<MAX_UNITS?u[target].hp:b[target-MAX_UNITS].hp, alive=target_alive(target);
    fprintf(stderr,"fresh HOME load frame=%d cruiser=%d target=%d hp_before=%d hp_after=%d alive=%d won=%d lost=%d\n",initial_frame,cruiser,target,before,after,alive,won,lost);
    assert(!active && after<before && !won && !lost);return 0;
}
