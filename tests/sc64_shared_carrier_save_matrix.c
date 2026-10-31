/* SPDX-License-Identifier: GPL-3.0-or-later
 * Gated host-only custom-game save/load matrix with real update cadence. */
#include "../common/gba.c"
#include "../starcraft/game.c"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#if !defined(SC64_SHARED_CARRIER_SAVE_EXPERIMENT) || !defined(SC64_SHARED_CARRIER_SAVE_VERSION)
#error "This fixture requires the gated shared Carrier save experiment"
#endif

int main(void)
{
    static const int owners[3]={ENEMY,RESCUABLE,4};
    unsigned cases=0;
    int last_map=0;
#ifdef SC64_IMPORTED_SKIRMISH
    last_map=SC64_SKIRMISH_IMPORTED_COUNT;
#endif
    for (int human=R_TER;human<=R_PROTOSS;++human)
      for (int bots=1;bots<=3;++bots)
        for (int map=0;map<=last_map;++map) {
          int combinations=1;
          for (int i=0;i<bots;++i) combinations*=3;
          for (int combination=0;combination<combinations;++combination) {
            int carrier[3]; Unit expected[3];
            memset(host_save,0,sizeof(host_save));
            game_init(); race_mode=1; race_human=human; race_bots=bots;
            race_skirmish_map=map;
            int races=combination;
            for (int i=0;i<bots;++i) { race_bot_races[i]=races%3; races/=3; }
            race_skirmish_init(); screen=SCREEN_PLAY;
            assert(race_skirmish_map==map && race_bots==bots && race_player[SIDE]==human);
            for (int i=0;i<bots;++i) {
                carrier[i]=spawn_unit(owners[i],SC64_CARRIER_TYPE,400+i*80,400);
                assert(carrier[i]>=0); race_minerals[owners[i]]=25;
            }
            for (int i=0;i<96 && screen==SCREEN_PLAY;++i) game_update(0,0);
            assert(screen==SCREEN_PLAY);
            for (int i=0;i<bots;++i) {
                assert(u[carrier[i]].alive && u[carrier[i]].owner==owners[i] &&
                       u[carrier[i]].type==SC64_CARRIER_TYPE);
                expected[i]=u[carrier[i]];
            }
            int saved_frame=frame, saved_minerals=minerals;
            u8 saved_clock[3]; memcpy(saved_clock,race_protoss_generic_clock,sizeof(saved_clock));
            if (!race_save_game()) {
                fprintf(stderr,"save rejected human=%d bots=%d map=%d combination=%d\n",human,bots,map,combination);
                return 1;
            }
            for (int i=0;i<bots;++i) u[carrier[i]].hp=1;
            frame=0; minerals=0; memset(race_protoss_generic_clock,0,sizeof(saved_clock));
            if (!race_load_game()) {
                fprintf(stderr,"load rejected human=%d bots=%d map=%d combination=%d\n",human,bots,map,combination);
                return 1;
            }
            assert(race_human==human && race_bots==bots && race_skirmish_map==map &&
                   frame==saved_frame && minerals==saved_minerals &&
                   !memcmp(saved_clock,race_protoss_generic_clock,sizeof(saved_clock)));
            for (int i=0;i<bots;++i) {
                assert(race_player[owners[i]]==race_bot_races[i]);
                assert(!memcmp(&expected[i],&u[carrier[i]],sizeof(Unit)));
            }
            ++cases;
          }
        }
    printf("shared Carrier save matrix: %u cases ok\n",cases);
    return 0;
}
