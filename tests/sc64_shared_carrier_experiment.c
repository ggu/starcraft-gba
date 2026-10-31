/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/game.c"
#ifndef SC64_SHARED_CARRIER_EXPERIMENT
#error "This fixture exercises the gated shared carrier backend"
#endif
static int children(int carrier, unsigned state)
{
    int count=0;
    for (int i=0;i<MAX_UNITS;++i)
        count += u[i].alive && u[i].type==SC64_INTERCEPTOR_TYPE &&
                 u[i].build==carrier && sc64_p7_carrier_child_state(&u[i])==state;
    return count;
}
static void test_render_arena(void)
{
    static u16 arena[SC64_ARENA_WORDS];
    unsigned end=SC64_OBJ_VRAM_CACHE_OFFSET;
#ifdef SC64_TERRAIN_CACHE_VRAM
    end += 240u*(unsigned)SC64_TERRAIN_CACHE_VRAM_ROWS;
    memset(SC64_OBJ_VRAM_CACHE,0x5a,240u*(unsigned)SC64_TERRAIN_CACHE_VRAM_ROWS);
    sc64_terrain_cache_valid=0;
#endif
    assert(end+16u<=GBA_VRAM_SCRATCH_BYTES);
    for (unsigned i=0;i<16;++i) GBA_VRAM_SCRATCH[end+i]=(u8)(0xa0u+i);
    memcpy(arena,race_p7_projectile_host_words,sizeof(arena));
    int old_cx=cx, old_cy=cy;
    for (int i=0;i<3;++i) {
        cx=old_cx+i*16; cy=old_cy+i*8;
        game_render();
        assert(!memcmp(arena,race_p7_projectile_host_words,sizeof(arena)));
        for (unsigned j=0;j<16;++j) assert(GBA_VRAM_SCRATCH[end+j]==(u8)(0xa0u+j));
    }
#ifdef SC64_TERRAIN_CACHE_VRAM
    int cache_written=0;
    for (unsigned i=0;i<240u*(unsigned)SC64_TERRAIN_CACHE_VRAM_ROWS;++i)
        if (SC64_OBJ_VRAM_CACHE[i]!=0x5a) { cache_written=1; break; }
    assert(cache_written);
#endif
    cx=old_cx; cy=old_cy;
}
static void test_typed_rows(void)
{
    for (int i=0;i<MAX_UNITS;++i) if (u[i].alive) {
        Sc64P7TypedUnitSave row;
        Unit decoded;
        assert(sc64_p7_typed_unit_encode(&row,&u[i],race_cols*16,race_rows*16));
        assert(sc64_p7_typed_unit_decode(&decoded,&row,race_cols*16,race_rows*16));
        assert(!memcmp(&decoded,&u[i],sizeof(decoded)));
    }
    for (int i=0;i<MAX_BUILDINGS;++i) if (b[i].alive) {
        Sc64P7TypedBuildingSave row;
        Building decoded;
        assert(sc64_p7_typed_building_encode(&row,&b[i],race_cols*16,race_rows*16));
        assert(sc64_p7_typed_building_decode(&decoded,&row,race_cols*16,race_rows*16));
        assert(!memcmp(&decoded,&b[i],sizeof(decoded)));
    }
}
#if !defined(SC64_P7_PROJECTILES_ENABLED)
static void test_staged_candidate(void)
{
    RaceSave *candidate=&race_data;
#ifdef SC64_ZP_CAMPAIGN_STORAGE
    Unit *scratch=sc64_zp_route_save_scratch.carrier_candidate.units;
#else
    Unit *scratch=sc64_combined_route_save_scratch.carrier_candidate.units;
#endif
    static Unit before[MAX_UNITS];
    static Building before_buildings[MAX_BUILDINGS];
    static u16 before_pool[SC64_ARENA_WORDS];
    memcpy(before,u,sizeof(before));
    memcpy(before_buildings,b,sizeof(before_buildings));
    memcpy(before_pool,race_p7_projectile_host_words,sizeof(before_pool));
    assert(sc64_shared_carrier_candidate_encode(candidate,u,b,&race_p7_projectiles,
                                                race_cols*16,race_rows*16));
    assert(sc64_shared_carrier_candidate_valid(candidate,scratch,MAX_UNITS,
                                               race_cols*16,race_rows*16));
    race_mode=0;
    assert(sc64_shared_carrier_candidate_valid(candidate,scratch,MAX_UNITS,
                                               race_cols*16,race_rows*16));
    race_mode=1;
    assert(!sc64_shared_carrier_candidate_valid(candidate,u,MAX_UNITS,
                                                race_cols*16,race_rows*16));
    candidate->carrier_custom.projectiles[2]^=0x80;
    assert(!sc64_shared_carrier_candidate_valid(candidate,scratch,MAX_UNITS,
                                                race_cols*16,race_rows*16));
    candidate->carrier_custom.projectiles[2]^=0x80;
    int child=-1;
    for (int i=0;i<MAX_UNITS;++i)
        if (scratch[i].alive && scratch[i].type==SC64_INTERCEPTOR_TYPE &&
            sc64_p7_carrier_child_state(&scratch[i])==SC64_P7_INTERCEPTOR_OUTSIDE)
            { child=i; break; }
    assert(child>=0);
    scratch[child].build=-1;
    assert(sc64_p7_typed_unit_encode(&candidate->carrier_custom.units[child],
                                     &scratch[child],race_cols*16,race_rows*16));
    assert(!sc64_shared_carrier_candidate_valid(candidate,scratch,MAX_UNITS,
                                                race_cols*16,race_rows*16));
    assert(!memcmp(before,u,sizeof(before)));
    assert(!memcmp(before_buildings,b,sizeof(before_buildings)));
    assert(!memcmp(before_pool,race_p7_projectile_host_words,sizeof(before_pool)));
}
#endif
static void test_catalog_rows(void)
{
    for (unsigned n=0;n<sc64_skirmish_record_count();++n) {
        const Sc64SkirmishRecord *record=sc64_skirmish_record_at(n);
        int unit=n<SC64_SKIRMISH_UNIT_COUNT;
        if (unit && record->kind!=SC64_SK_TRAIN &&
            record->kind!=SC64_SK_UNIT_MORPH && record->kind!=SC64_SK_MODE) continue;
        int type=unit ? sc64_race_unit_from_raw(record->raw_id) :
                        sc64_race_building_from_raw(record->raw_id);
        if (type<0) continue;
        for (int owner=0;owner<5;++owner) {
            if (unit) {
                int slot=spawn_unit(owner,type,400,400);
                assert(slot>=0);
                Unit *q=&u[slot], decoded;
                Sc64P7TypedUnitSave row;
                q->selected=1; q->hp=(q->maxhp+1)/2; q->shield/=2;
                if (!sc64_p7_typed_unit_encode(&row,q,race_cols*16,race_rows*16)) {
                    fprintf(stderr,"typed unit rejected raw=%u owner=%d\n",record->raw_id,owner);
                    assert(0);
                }
                assert(sc64_p7_typed_unit_decode(&decoded,&row,race_cols*16,race_rows*16));
                assert(!memcmp(q,&decoded,sizeof(decoded)));
                q->alive=0;
            } else {
                int slot=spawn_build(owner,type,400,400,0);
                assert(slot>=0);
                Building *q=&b[slot], decoded;
                Sc64P7TypedBuildingSave row;
                q->hp=(q->maxhp+1)/2; q->shield/=2;
                if (!sc64_p7_typed_building_encode(&row,q,race_cols*16,race_rows*16)) {
                    fprintf(stderr,"typed building rejected raw=%u owner=%d\n",record->raw_id,owner);
                    assert(0);
                }
                assert(sc64_p7_typed_building_decode(&decoded,&row,race_cols*16,race_rows*16));
                assert(!memcmp(q,&decoded,sizeof(decoded)));
                q->alive=0;
            }
        }
    }
}
static void test_custom_game_matrix(void)
{
    static const int owners[3]={ENEMY,RESCUABLE,4};
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
            int carrier[3];
            game_init(); race_mode=1; race_human=human; race_bots=bots;
            race_skirmish_map=map;
            int races=combination;
            for (int i=0;i<bots;++i) { race_bot_races[i]=races%3; races/=3; }
            race_skirmish_init(); screen=SCREEN_PLAY;
            assert(race_skirmish_map==map);
            assert(race_bots==bots && race_player[SIDE]==human);
#ifdef SC64_IMPORTED_SKIRMISH
            for (int i=0;i<bots;++i)
                assert(race_player[owners[i]]==race_bot_races[i]);
#endif
            for (int i=0;i<bots;++i) {
                carrier[i]=spawn_unit(owners[i],SC64_CARRIER_TYPE,400+i*80,400);
                assert(carrier[i]>=0);
                race_minerals[owners[i]]=25;
            }
            for (int i=0;i<96 && screen==SCREEN_PLAY;++i) game_update(0,0);
            for (int i=0;i<bots;++i) {
                assert(u[carrier[i]].alive && u[carrier[i]].owner==owners[i] &&
                       u[carrier[i]].type==SC64_CARRIER_TYPE);
                assert(u[carrier[i]].carry>0 ||
                       children(carrier[i],SC64_P7_INTERCEPTOR_TRAINING)>0 ||
                       children(carrier[i],SC64_P7_INTERCEPTOR_INSIDE)>0 ||
                       children(carrier[i],SC64_P7_INTERCEPTOR_OUTSIDE)>0);
            }
          }
        }
}

int main(void)
{
    assert(SC64_OBJ_VRAM_PROJECTILE_BYTES == SC64_ARENA_WORDS*sizeof(u16));
    assert(SC64_OBJ_VRAM_CACHE_OFFSET >= SC64_OBJ_VRAM_PROJECTILE_BYTES);
    game_init(); race_mode=1; race_human=R_PROTOSS; race_bots=1; race_skirmish_map=0;
    race_skirmish_init(); screen=SCREEN_PLAY;
#ifndef SC64_P7_PROJECTILES_ENABLED
    assert(race_carrier_services_active());
    race_mode=0; race_selected_mission=SC64_P7_GLOBAL_ID;
    assert(!race_carrier_services_active());
    race_mode=1;
#endif
    test_typed_rows();
    test_catalog_rows();
    int carrier=spawn_unit(SIDE,SC64_CARRIER_TYPE,600,600);
    assert(carrier>=0); minerals=25;
    assert(race_p7_carrier_training_queue_owner(carrier,SIDE));
#ifndef SC64_P7_PROJECTILES_ENABLED
    assert(!race_save_game());
#endif
    for (int i=0;i<430 && screen==SCREEN_PLAY;++i) game_update(0,0);
    assert(children(carrier,SC64_P7_INTERCEPTOR_INSIDE)==1);
    int prey=spawn_unit(ENEMY,U_ZERGLING,660,600); assert(prey>=0);
    int hp=u[prey].hp, launched=0;
    for (int i=0;i<400 && screen==SCREEN_PLAY && u[prey].alive;++i) {
        game_update(0,0);
        if (children(carrier,SC64_P7_INTERCEPTOR_OUTSIDE)) launched=1;
    }
    assert(launched && (!u[prey].alive || u[prey].hp<hp));
    int doomed=spawn_unit(SIDE,SC64_CARRIER_TYPE,700,700);
    int victim=spawn_unit(ENEMY,U_ZERGLING,790,700);
    assert(doomed>=0 && victim>=0 && race_p7_carrier_training_stock(doomed,1)==1);
    for (int i=0;i<80 && !children(doomed,SC64_P7_INTERCEPTOR_OUTSIDE);++i)
        game_update(0,0);
    int child=-1;
    for (int i=0;i<MAX_UNITS;++i)
        if (u[i].alive && u[i].type==SC64_INTERCEPTOR_TYPE && u[i].build==doomed &&
            sc64_p7_carrier_child_state(&u[i])==SC64_P7_INTERCEPTOR_OUTSIDE) child=i;
    assert(child>=0);
    test_typed_rows();
    for (int i=0;i<160 && sc64_pool_active_head(&race_p7_projectiles)==SC64_END;++i)
        game_update(0,0);
    assert(sc64_pool_active_head(&race_p7_projectiles)!=SC64_END);
    test_render_arena();
#if !defined(SC64_P7_PROJECTILES_ENABLED)
    test_staged_candidate();
#endif
    hit_target(doomed,10000,U_MARINE,ENEMY);
    assert(!u[doomed].alive && u[child].alive && u[child].build==-1 &&
           sc64_p7_carrier_child_state(&u[child])==SC64_P7_INTERCEPTOR_DETACHED);
    /* Each custom-game opponent uses its own wallet and actor binding. */
    for (int owner=1; owner<5; ++owner) {
        if (owner==SIDE) continue;
        int bot=spawn_unit(owner,SC64_CARRIER_TYPE,500+owner*16,500);
        assert(bot>=0); race_minerals[owner]=25;
        assert(!race_p7_carrier_training_queue_owner(bot,SIDE));
        race_ai_plan(owner);
        assert(u[bot].carry==1);
        assert(race_minerals[owner]==0);
        assert(!race_p7_carrier_training_queue_owner(bot,owner));
        race_p7_carrier_training_source_tick();
        assert(children(bot,SC64_P7_INTERCEPTOR_TRAINING)==1);
    }
    test_custom_game_matrix();
    puts("shared carrier experiment: ok"); return 0;
}
