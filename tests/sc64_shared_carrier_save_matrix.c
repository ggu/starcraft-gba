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

static void test_full_envelope(void)
{
    static Unit saved_units[MAX_UNITS];
    static Building saved_buildings[MAX_BUILDINGS];
    static u8 saved_sram[sizeof(host_save)];
    memset(host_save,0,sizeof(host_save));
    game_init(); race_mode=1; race_human=R_PROTOSS; race_bots=1;
    race_skirmish_map=0; race_skirmish_init(); screen=SCREEN_PLAY;
    int carrier=spawn_unit(SIDE,SC64_CARRIER_TYPE,600,600);
    int target=spawn_unit(ENEMY,U_ZERGLING,660,600);
    assert(carrier>=0 && target>=0 && race_p7_carrier_training_stock(carrier,1)==1);
    for (int i=0;i<240 && sc64_pool_active_head(&race_p7_projectiles)==SC64_END;++i)
        game_update(0,0);
    assert(sc64_pool_active_head(&race_p7_projectiles)!=SC64_END && u[target].alive);
    for (int i=0;i<MAX_UNITS;++i)
        if (spawn_unit(i%5,U_MARINE,300+(i%16)*8,300+(i/16)*8)<0) break;
    for (int i=0;i<MAX_BUILDINGS;++i)
        if (spawn_build(i%5,B_PYLON,320+(i%16)*16,320+(i/16)*16,0)<0) break;
    int unit_count=0,building_count=0;
    for (int i=0;i<MAX_UNITS;++i) unit_count+=u[i].alive!=0;
    for (int i=0;i<MAX_BUILDINGS;++i) building_count+=b[i].alive!=0;
    assert(unit_count==MAX_UNITS && building_count==MAX_BUILDINGS);
    memcpy(saved_units,u,sizeof(u)); memcpy(saved_buildings,b,sizeof(b));
    size_t omitted_begin, omitted_end;
    assert(race_save_serialize(1));
    assert(sc64_shared_carrier_custom_omission_bounds(&omitted_begin,&omitted_end));
    for (size_t i=omitted_begin;i<omitted_end;++i) assert(!((const u8 *)&race_data)[i]);
    printf("envelope fixed=%zu carrier_packet=%u units=%zu buildings=%zu sram_payload=%u",
           sizeof(RaceSave),(unsigned)SC64_CODEC_BYTES,
           sizeof(race_data.carrier_custom.units),sizeof(race_data.carrier_custom.buildings),
           (unsigned)SAVE_PAYLOAD_MAX);
    printf(" custom_zero_span=%zu",omitted_end-omitted_begin);
#ifdef SC64_IMPACT_EVENTS
    static Sc64ImpactEvent expected[RACE_IMPACT_CAPACITY];
    race_impact_reset();
    for (int i=0;i<RACE_IMPACT_SAVE_CAPACITY;++i)
        race_impact_launch(target,3,1,SIDE,0,SC64_CARRIER_TYPE,10);
    assert(race_impact_count==RACE_IMPACT_SAVE_CAPACITY);
    memcpy(expected,race_impact_rows,sizeof(expected));
    printf(" impact_saved=%d impact_runtime=%d impact_header=%zu impact_row=%zu worst_bytes=%zu",
           RACE_IMPACT_SAVE_CAPACITY,RACE_IMPACT_CAPACITY,sizeof(Sc64ImpactSaveHeader),
           sizeof(Sc64ImpactEvent),sizeof(RaceSave)+sizeof(Sc64ImpactSaveHeader)+
           (size_t)RACE_IMPACT_SAVE_CAPACITY*sizeof(Sc64ImpactEvent));
#else
    printf(" impact_sidecar=disabled");
#endif
    putchar('\n');
    assert(race_save_game());
    memset(u,0,sizeof(u)); memset(b,0,sizeof(b));
#ifdef SC64_IMPACT_EVENTS
    race_impact_reset();
#endif
    assert(race_load_game());
    for (int i=0;i<MAX_UNITS;++i)
        if (saved_units[i].alive && !IS_WORKER(saved_units[i].type))
            assert(!memcmp(&saved_units[i],&u[i],sizeof(Unit)));
    assert(!memcmp(saved_buildings,b,sizeof(b)));
#ifdef SC64_IMPACT_EVENTS
    assert(race_impact_count==RACE_IMPACT_SAVE_CAPACITY &&
           !memcmp(expected,race_impact_rows,race_impact_count*sizeof(expected[0])));
    if (RACE_IMPACT_SAVE_CAPACITY<RACE_IMPACT_CAPACITY) {
        memcpy(saved_sram,host_save,sizeof(host_save));
        race_impact_launch(target,3,1,SIDE,0,SC64_CARRIER_TYPE,10);
        assert(race_impact_count==RACE_IMPACT_SAVE_CAPACITY+1);
        assert(!race_save_game() && !memcmp(saved_sram,host_save,sizeof(host_save)));
    }
#else
    (void)saved_sram;
#endif
}

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
    test_full_envelope();
    printf("shared Carrier save matrix: %u cases ok\n",cases);
    return 0;
}
