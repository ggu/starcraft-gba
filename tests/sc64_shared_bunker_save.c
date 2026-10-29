/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Shared Bunker candidate validation and custom-game save round trips. */
#define main skirmish_runtime_fixture_main
#include "sc64_skirmish_runtime.c"
#undef main

#ifndef SC64_SHARED_BUNKER
#error "shared Bunker fixture requires the shared capability"
#endif
static RaceSave valid_candidate, invalid_candidate;
static Unit live_units_before[MAX_UNITS];
static Building live_buildings_before[MAX_BUILDINGS];

static void encode_unit(RaceSave *candidate, int slot, const Unit *q)
{
#if defined(SC64_ZP_FULL_STORAGE) && defined(SC64_P7_TYPED_COMPACT_ENABLED)
    assert(sc64_p7_typed_unit_encode(&candidate->units[slot],q,(unsigned)race_cols*16u,(unsigned)race_rows*16u));
#elif defined(SC64_ZP_FULL_STORAGE) && defined(SC64_Z7_DERIVED_SAVE)
    assert(sc64_derived_unit_encode(&candidate->units[slot],q,(unsigned)race_cols*16u,(unsigned)race_rows*16u));
#elif defined(SC64_ZP_FULL_STORAGE)
    assert(sc64_zp_unit_save_encode(&candidate->units[slot],q,(unsigned)race_cols*16u,(unsigned)race_rows*16u));
#elif defined(SC64_LATE_TERRAN_STORAGE) || defined(SC64_ZP_CAMPAIGN_STORAGE)
    assert(sc64_unit_save_encode(&candidate->units[slot],q,(unsigned)race_cols*16u,(unsigned)race_rows*16u));
#else
    for (unsigned j=0;j<RACE_UNIT_WORDS;++j) { int n; memcpy(&n,(const u8 *)q+j*sizeof(int),sizeof(n)); candidate->units[slot][j]=(s16)n; }
#endif
}
static void encode_building(RaceSave *candidate, int slot, const Building *q)
{
#if defined(SC64_ZP_FULL_STORAGE) && defined(SC64_P7_TYPED_COMPACT_ENABLED)
    assert(sc64_p7_typed_building_encode(&candidate->buildings[slot],q,(unsigned)race_cols*16u,(unsigned)race_rows*16u));
#elif defined(SC64_ZP_FULL_STORAGE) && defined(SC64_Z7_DERIVED_SAVE)
    assert(sc64_derived_building_encode(&candidate->buildings[slot],q,(unsigned)race_cols*16u,(unsigned)race_rows*16u));
#elif defined(SC64_T6_STORAGE) || defined(SC64_ZP_CAMPAIGN_STORAGE)
    assert(sc64_building_save_encode(&candidate->buildings[slot],q,(unsigned)race_cols*16u,(unsigned)race_rows*16u));
#else
    for (unsigned j=0;j<RACE_BUILD_WORDS;++j) { int n; memcpy(&n,(const u8 *)q+j*sizeof(int),sizeof(n)); candidate->buildings[slot][j]=(s16)n; }
#endif
}
static void reject_without_live_mutation(const RaceSave *candidate)
{
    int old_m=minerals,old_g=gas;
    memcpy(live_units_before,u,sizeof(u)); memcpy(live_buildings_before,b,sizeof(b));
    save_write(candidate,sizeof(*candidate),SAVE_VERSION);
    save_write(candidate,sizeof(*candidate),SAVE_VERSION);
    assert(!race_load_game());
    assert(!memcmp(live_units_before,u,sizeof(u)) && !memcmp(live_buildings_before,b,sizeof(b)));
    assert(minerals==old_m && gas==old_g);
}
int main(void)
{
    setbuf(stdout,NULL); gba_init(); game_init(); race_mode=1; race_human=R_TER; race_bots=1;
    init_state(); assert(screen==SCREEN_PLAY && race_mode);
    int bunker=spawn_build(SIDE,B_BUNKER,300,300,0); assert(bunker>=0);
    int actors[5];
    for(int i=0;i<5;++i) { actors[i]=spawn_unit(SIDE,U_MARINE,300,300); assert(actors[i]>=0); }
    for(int i=0;i<4;++i) {
        assert(race_bunker_request_board(actors[i],bunker));
        for(int t=0;t<120 && !SC64_TRANSPORT_LOADED(&u[actors[i]]);++t) game_update(0,0);
        assert(SC64_TRANSPORT_LOADED(&u[actors[i]]));
    }
    assert(race_bunker_cargo_space(bunker)==4);
    assert(race_save_game()); valid_candidate=race_data;
    assert(race_load_game());
    assert(race_bunker_cargo_space(bunker)==4);
    for(int i=0;i<4;++i)
        assert(SC64_TRANSPORT_LOADED(&u[actors[i]]) && u[actors[i]].target==MAX_UNITS+bunker);
    for(int mutation=0;mutation<9;++mutation) {
        invalid_candidate=valid_candidate;
        int slot=mutation==8?actors[4]:actors[0]; Unit actor=u[slot]; Building target=b[bunker];
        switch(mutation) {
        case 0: actor.target=MAX_UNITS+MAX_BUILDINGS-1; assert(!b[MAX_BUILDINGS-1].alive); break;
        case 1: actor.owner=ENEMY; break;
        case 2: actor.raw_type=7; break;
        case 3: actor.selected=1; break;
        case 4: actor.x=(actor.x+1)%(race_cols*16); break;
        case 5: actor.path_timer=1; break;
        case 6: target.owner=ENEMY; break;
        case 7: target.raw_type=156; break;
        case 8: actor.order=ORDER_TRANSPORTED; actor.target=MAX_UNITS+bunker;
                actor.x=b[bunker].x; actor.y=b[bunker].y; actor.selected=0;
                actor.path_timer=actor.path_retry=0; actor.build=-1; break;
        }
        if(mutation==6 || mutation==7) encode_building(&invalid_candidate,bunker,&target);
        else encode_unit(&invalid_candidate,slot,&actor);
        reject_without_live_mutation(&invalid_candidate);
    }
    puts("shared Bunker save roundtrip and malformed candidates PASS");
    return 0;
}
