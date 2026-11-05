/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Independently compiled old writer / new selected reader, HOST only.
 * Constructed raw-36 Overlord and test SRAM are never guest evidence. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#ifdef SC64_PRIOR_WRITER
#include SC64_PRIOR_GAME
#else
#include "../starcraft/game.c"
#endif
extern void guardian_test_reset(void);
extern void guardian_test_snapshot(void *);
extern void guardian_test_restore(const void *);
extern void guardian_test_retag(int,u32,u32);
extern void guardian_test_corrupt(int);
static unsigned char original[32768],before[32768],after[32768];
#ifndef SC64_PRIOR_WRITER
static Unit prior_units[MAX_UNITS];
static Building prior_buildings[MAX_BUILDINGS];
#endif
static void start(void)
{
    guardian_test_reset();game_init();race_mode=1;race_human=R_ZERG;race_bots=1;
    race_skirmish_map=0;race_bot_races[0]=R_PROTOSS;init_state();assert(screen==SCREEN_PLAY);
}
int main(int argc,char **argv)
{
    assert(argc==2);start();
#ifdef SC64_PRIOR_WRITER
    int old=spawn_unit(SIDE,U_OVERLORD,100,100);assert(old>=0);
    u[old].raw_type=36;u[old].raw_owner=-1;
    assert(race_save_game());guardian_test_snapshot(original);
    FILE *f=fopen(argv[1],"wb");assert(f);
    assert(fwrite(original,1,sizeof(original),f)==sizeof(original));assert(!fclose(f));
    puts("actual prior feature0 HOST writer: ordinary Overlord/raw36");
#else
    FILE *f=fopen(argv[1],"rb");assert(f);
    assert(fread(original,1,sizeof(original),f)==sizeof(original));assert(!fclose(f));
    guardian_test_restore(original);RaceLoadPlan plan;GbaSaveSelection predecessor,newest,fallback;
    guardian_test_snapshot(before);assert(race_worker_cargo_select(&plan,&predecessor));
    assert(predecessor.tag==SC64_TRANSPORT_SAVE_VERSION && plan.guardian_auth.features==0);
    guardian_test_snapshot(after);assert(!memcmp(before,after,sizeof(before)));
    assert(race_load_game());int old=-1;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive && u[i].type==U_OVERLORD && u[i].raw_type==36)old=i;
    assert(old>=0);Unit overlord=u[old];
    assert(race_save_game());assert(race_worker_cargo_select(&plan,&newest));
    assert(newest.slot!=predecessor.slot && newest.tag==SC64_SHARED_ACTOR_FLIGHT_TRANSPORT_SAVE_VERSION &&
           plan.guardian_auth.features==15u);
    assert(race_load_game() && !memcmp(&u[old],&overlord,sizeof(overlord)));
    guardian_test_snapshot(original);
    /* These are adversarial retags, not producers for historical formats. */
    guardian_test_retag(newest.slot,SC64_SHARED_ACTOR_FLIGHT_TRANSPORT_SAVE_VERSION,SAVE_VERSION);
    guardian_test_snapshot(before);memcpy(prior_units,u,sizeof(u));memcpy(prior_buildings,b,sizeof(b));
    assert(race_worker_cargo_select(&plan,&fallback));
    assert(fallback.slot==predecessor.slot && fallback.tag==SC64_TRANSPORT_SAVE_VERSION);
    guardian_test_snapshot(after);assert(!memcmp(before,after,sizeof(before)) &&
        !memcmp(prior_units,u,sizeof(u)) && !memcmp(prior_buildings,b,sizeof(b)));
    assert(race_load_game() && u[old].alive && u[old].type==U_OVERLORD && u[old].raw_type==36);
    const struct {u32 outer,inner;} unsupported[]={
        {SC64_SHARED_ACTOR_TRANSPORT_SAVE_VERSION,SC64_SHARED_ACTOR_SAVE_VERSION},
        {SC64_SHARED_ACTOR_FULL_TRANSPORT_SAVE_VERSION,SC64_SHARED_ACTOR_FULL_SAVE_VERSION}
    };
    for(unsigned i=0;i<sizeof(unsupported)/sizeof(unsupported[0]);++i) {
        guardian_test_restore(original);
        guardian_test_retag(newest.slot,unsupported[i].outer,unsupported[i].inner);
        guardian_test_snapshot(before);assert(race_worker_cargo_select(&plan,&fallback));
        assert(fallback.slot==predecessor.slot && fallback.tag==SC64_TRANSPORT_SAVE_VERSION);
        guardian_test_snapshot(after);assert(!memcmp(before,after,sizeof(before)));
    }
    guardian_test_corrupt(predecessor.slot);
    guardian_test_snapshot(before);memcpy(prior_units,u,sizeof(u));memcpy(prior_buildings,b,sizeof(b));
    int old_frame=frame,old_minerals=minerals,old_gas=gas;
    assert(!race_load_game());guardian_test_snapshot(after);
    assert(!memcmp(before,after,sizeof(before)) && !memcmp(prior_units,u,sizeof(u)) &&
           !memcmp(prior_buildings,b,sizeof(b)) && frame==old_frame &&
           minerals==old_minerals && gas==old_gas);
    puts("feature0/raw36 predecessor, mismatched-newer and unsupported feature5/7 fallback passed");
#endif
    return 0;
}
