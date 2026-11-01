/* Prepared-profile fixture: common actor service, not a mission replay. */
#include <assert.h>
#include "../starcraft/game.c"
#if !defined(SC64_SHARED_HP_REGEN) || !defined(SC64_RACES)
#error "requires shared HP regeneration in a race-engine profile"
#endif
/* Actor-owned phases must not share a global clock or owner restriction.
 * This is controlled fixture setup, not ordinary-input play. */
/* A host-only ordinary custom save supplies rows; all preparation and row
 * decodes below operate on an isolated copy, without SRAM modification. */
static RaceSave hp_candidate_copy, hp_candidate_before;
static Unit hp_world_units_before[MAX_UNITS];
static Building hp_world_buildings_before[MAX_BUILDINGS];
static int fixture_encode_hp_unit(RaceSave *save,unsigned slot,const Unit *q,
                                  unsigned width,unsigned height)
{
#ifdef SC64_ZP_FULL_STORAGE
#ifdef SC64_P7_TYPED_COMPACT_ENABLED
    return sc64_p7_typed_unit_encode(&save->units[slot],q,width,height);
#elif defined(SC64_Z7_DERIVED_SAVE)
    return sc64_derived_unit_encode(&save->units[slot],q,width,height);
#else
    return sc64_zp_unit_save_encode(&save->units[slot],q,width,height);
#endif
#elif defined(SC64_LATE_TERRAN_STORAGE) || defined(SC64_ZP_CAMPAIGN_STORAGE)
    return sc64_unit_save_encode(&save->units[slot],q,width,height);
#else
    (void)width;(void)height;
    for(unsigned j=0;j<RACE_UNIT_WORDS;++j) {
        int n;memcpy(&n,(const u8 *)q+j*sizeof(int),sizeof(n));
        save->units[slot][j]=(s16)n;
    }
    return 1;
#endif
}
static void test_candidate_geometry_and_rows(void)
{
    game_init();race_mode=1;race_human=R_TER;race_bots=1;init_state();
    assert(screen==SCREEN_PLAY);
    int witness=spawn_unit(SIDE,U_ZERGLING,160,160);assert(witness>=0);
    u[witness].hp--;u[witness].shield_remainder=79;
    assert(race_save_game());
    hp_candidate_copy=race_data;hp_candidate_before=hp_candidate_copy;
    memcpy(hp_world_units_before,u,sizeof(u));
    memcpy(hp_world_buildings_before,b,sizeof(b));
    RaceHpPreparedRows rows={0};
    assert(race_hp_prepare_rows(&hp_candidate_copy,&rows));
    assert(rows.candidate==&hp_candidate_copy && rows.width==(unsigned)race_cols &&
           rows.height==(unsigned)race_rows);
    for(unsigned i=0;i<MAX_UNITS;++i) {
        Unit a;const Unit *live=&hp_world_units_before[i];
        assert(race_hp_prepared_unit(&rows,i,&a));
        assert(a.alive==live->alive);
        if(!live->alive) continue;
        assert(a.type==live->type && a.owner==live->owner && a.hp==live->hp &&
               a.maxhp==live->maxhp && a.x==live->x && a.y==live->y &&
               a.shield_remainder==live->shield_remainder);
    }
    for(unsigned i=0;i<MAX_BUILDINGS;++i) {
        Building a;const Building *live=&hp_world_buildings_before[i];
        assert(race_hp_prepared_building(&rows,i,&a));
        assert(a.alive==live->alive);
        if(!live->alive) continue;
        assert(a.type==live->type && a.owner==live->owner && a.hp==live->hp &&
               a.maxhp==live->maxhp && a.x==live->x && a.y==live->y);
    }
    assert(!memcmp(&hp_candidate_copy,&hp_candidate_before,sizeof(hp_candidate_copy)));
    assert(!memcmp(hp_world_units_before,u,sizeof(u)) && !memcmp(hp_world_buildings_before,b,sizeof(b)));
#ifdef SC64_SCARAB_NAMESPACE_STAGE
    Unit untouched;memset(&untouched,0x5a,sizeof(untouched));Unit sentinel=untouched;
    int wire_before=race_scarab_stage_wire;race_scarab_stage_wire=!wire_before;
    assert(!race_hp_prepared_unit(&rows,(unsigned)witness,&untouched));
    assert(!memcmp(&untouched,&sentinel,sizeof(untouched)));
    race_scarab_stage_wire=wire_before;
#endif
    RaceHpPreparedRows unchanged=rows;
    hp_candidate_copy.state[0]=2;
    assert(!race_hp_prepare_rows(&hp_candidate_copy,&rows));
    assert(rows.candidate==unchanged.candidate && rows.width==unchanged.width &&
           rows.height==unchanged.height);
#ifdef SC64_SCARAB_NAMESPACE_STAGE
    assert(rows.scarab_wire==unchanged.scarab_wire);
#endif
    hp_candidate_copy=hp_candidate_before;
    Unit invalid=hp_world_units_before[witness];invalid.shield_remainder=80;
    assert(fixture_encode_hp_unit(&hp_candidate_copy,(unsigned)witness,&invalid,
                                  rows.width*16u,rows.height*16u));
    assert(!race_hp_prepare_rows(&hp_candidate_copy,&rows));
    assert(rows.candidate==unchanged.candidate && rows.width==unchanged.width &&
           rows.height==unchanged.height);
#ifdef SC64_SCARAB_NAMESPACE_STAGE
    assert(rows.scarab_wire==unchanged.scarab_wire);
#endif
    assert(!memcmp(hp_world_units_before,u,sizeof(u)) && !memcmp(hp_world_buildings_before,b,sizeof(b)));
}
static void test_actor_phase_owners_and_reuse(void)
{
    int slots[5];
    memset(u,0,sizeof(u)); memset(b,0,sizeof(b));
    for (int owner=0;owner<5;++owner) {
        int slot=spawn_unit(owner,U_ZERGLING,100+owner*20,100);
        assert(slot>=0);slots[owner]=slot;
        u[slot].hp=u[slot].maxhp-2;
        u[slot].shield_remainder=(unsigned)owner;
    }
    for (int tick=0;tick<79;++tick) race_shared_hp_regen_tick();
    for (int owner=0;owner<5;++owner) {
        Unit *q=&u[slots[owner]];
        assert(q->hp==q->maxhp-2+(owner>0));
        assert(q->shield_remainder==(unsigned)((owner+79)%80));
    }
    race_shared_hp_regen_tick();
    for (int owner=0;owner<5;++owner) {
        Unit *q=&u[slots[owner]];
        assert(q->hp==q->maxhp-1 && q->shield_remainder==(unsigned)owner);
    }
    /* A dead slot must not inherit the old occupant's phase on spawn. */
    int slot=slots[0];u[slot].alive=0;u[slot].shield_remainder=63;
    assert(spawn_unit(SIDE,U_ZERGLING,200,200)==slot);
    assert(u[slot].shield_remainder==0);
    u[slot].hp=u[slot].maxhp-1;
    for(int tick=0;tick<79;++tick) race_shared_hp_regen_tick();
    assert(u[slot].hp==u[slot].maxhp-1 && u[slot].shield_remainder==79);
    race_shared_hp_regen_tick();
    assert(u[slot].hp==u[slot].maxhp && u[slot].shield_remainder==0);
}
#ifdef SC64_SHARED_TRANSPORT
static void test_valid_loaded_passenger_regenerates(void)
{
    /* Fixture setup places a captured Zergling with a same-owner Dropship;
     * boarding and its persistent carrier link use the real transport path. */
    game_init();race_mode=1;race_human=R_TER;race_bots=1;init_state();
    assert(screen==SCREEN_PLAY && race_mode);
    memset(u,0,sizeof(u));
    assert(sc64_race_unit_from_raw(11)==U_DROPSHIP);
    assert(sc64_race_unit_from_raw(37)==U_ZERGLING);
    int carrier=spawn_unit(SIDE,U_DROPSHIP,100,100);
    int passenger=spawn_unit(SIDE,U_ZERGLING,108,100);
    assert(carrier>=0 && passenger>=0 && race_transport_carrier_valid(carrier));
    assert(race_transport_request_board(passenger,carrier));
    assert(u[passenger].order==ORDER_BOARD && u[passenger].target==carrier);
    assert(race_transport_tick_unit(passenger));
    assert(SC64_TRANSPORT_LOADED(&u[passenger]));
    assert(u[passenger].target==carrier && race_transport_cargo_space(carrier)==1);
    u[passenger].hp=u[passenger].maxhp-1;u[passenger].shield_remainder=0;
    for(int tick=0;tick<80;++tick) {
        assert(race_transport_tick_unit(passenger));
        assert(SC64_TRANSPORT_LOADED(&u[passenger]) && u[passenger].target==carrier);
        assert(race_transport_cargo_space(carrier)==1);
        race_shared_hp_regen_tick();
        if(tick<79) assert(u[passenger].hp==u[passenger].maxhp-1);
    }
    assert(u[passenger].hp==u[passenger].maxhp && u[passenger].shield_remainder==0);
}
#endif
static void test_campaign_and_skirmish_bindings(void)
{
    for(int custom=0;custom<=1;++custom) {
        game_init();memset(u,0,sizeof(u));memset(b,0,sizeof(b));
        race_mode=custom;
        int slot=spawn_unit(SIDE,U_ZERGLING,100,100);assert(slot>=0);
        u[slot].hp=u[slot].maxhp-1;u[slot].shield_remainder=79;
        race_shared_hp_regen_tick();
        assert(u[slot].hp==u[slot].maxhp && u[slot].shield_remainder==0);
    }
}
#ifdef SC64_SHOWCASE_ENABLED
static void test_showcase_live_update(void)
{
    game_init();sc64_showcase_start(5);
    assert(screen==SCREEN_PAUSE && sc64_showcase_scene==5);
    int slot=-1;
    for(int i=0;i<MAX_UNITS;++i)
        if(u[i].alive && u[i].type==U_ZERGLING) {slot=i;break;}
    assert(slot>=0 && u[slot].hp==u[slot].maxhp);
    u[slot].hp--;u[slot].shield_remainder=79;
    game_update(KEY_A,KEY_A);assert(screen==SCREEN_PLAY);
    game_update(0,0);
    assert(u[slot].alive && u[slot].hp==u[slot].maxhp && u[slot].shield_remainder==0);
}
#endif
#ifdef SC64_P6_COMBAT_ENABLED
static void test_p6_burrowed_regenerates(void)
{
    game_init();memset(u,0,sizeof(u));memset(b,0,sizeof(b));
    race_mode=0;race_selected_mission=SC64_P6_GLOBAL_ID;
    assert(race_p6_active());
    int slot=spawn_unit(ENEMY,U_ZERGLING,100,100);assert(slot>=0);
    u[slot].raw_type=37;u[slot].flags|=2u;
    assert(race_p6_combat_unit_burrowed(slot));
    u[slot].hp=u[slot].maxhp-1;u[slot].shield_remainder=79;
    race_shared_hp_regen_tick();
    assert(race_p6_combat_unit_burrowed(slot));
    assert(u[slot].hp==u[slot].maxhp && u[slot].shield_remainder==0);
}
#endif
#ifdef SC64_Z5_RUNTIME
static void test_z5_legacy_tick_disabled(void)
{
    game_init();memset(u,0,sizeof(u));memset(b,0,sizeof(b));
    race_mode=0;race_selected_mission=SC64_Z5_GLOBAL_ID;
    assert(race_z5_active());
    int slot=spawn_unit(SIDE,U_ZERGLING,100,100);assert(slot>=0);
    u[slot].hp=u[slot].maxhp-2;u[slot].shield_remainder=0;
    for(int i=0;i<79;++i) { race_shared_hp_regen_tick();race_tick_state(); }
    assert(u[slot].hp==u[slot].maxhp-2 && u[slot].shield_remainder==79);
    race_shared_hp_regen_tick();race_tick_state();
    assert(u[slot].hp==u[slot].maxhp-1 && u[slot].shield_remainder==0);
}
#endif
static void test_paused_and_live_update(void)
{
    /* Controlled host setup isolates the pause return and real update path. */
    game_init();memset(u,0,sizeof(u));memset(b,0,sizeof(b));
    int slot=spawn_unit(SIDE,U_ZERGLING,100,100);assert(slot>=0);
    u[slot].hp=u[slot].maxhp-1;u[slot].shield_remainder=79;
    int hp=u[slot].hp;
    screen=SCREEN_PAUSE;
    for(int i=0;i<10;++i)game_update(0,0);
    assert(screen==SCREEN_PAUSE && u[slot].hp==hp && u[slot].shield_remainder==79);
    screen=SCREEN_PLAY;menu_open=0;help=0;won=0;lost=0;
    game_update(0,0);
    assert(u[slot].hp==hp+1 && u[slot].shield_remainder==0);
}
int main(void)
{
    game_init();
    memset(u,0,sizeof(u)); memset(b,0,sizeof(b));
    int z=spawn_unit(SIDE,U_ZERGLING,100,100);
    int enemy=spawn_unit(ENEMY,U_ZERGLING,200,200);
    int marine=spawn_unit(SIDE,U_MARINE,300,300);
    int colony=spawn_build(SIDE,B_CREEP_COLONY,400,400,0);
    assert(z>=0 && enemy>=0 && marine>=0 && colony>=0);
    u[z].hp--;u[enemy].hp--;u[marine].hp--;b[colony].hp--;
    u[z].shield_remainder=79;u[enemy].shield_remainder=79;
    u[marine].shield_remainder=79;b[colony].shield_remainder=79;
    assert(race_shared_hp_regen_unit_phase_valid(&u[z]));
    u[z].shield_remainder=80;
    assert(!race_shared_hp_regen_unit_phase_valid(&u[z]));
    u[z].shield_remainder=79;
    int zh=u[z].hp,eh=u[enemy].hp,mh=u[marine].hp,bh=b[colony].hp;
    b[colony].under_construction=1;
    b[colony].raw_type=sc64_race_raw_building(B_SUNKEN_COLONY);
    assert(race_shared_hp_regen_building_phase_valid(&b[colony],1));
    assert(!race_shared_hp_regen_building_phase_valid(&b[colony],0));
    race_shared_hp_regen_tick();
    assert(u[z].hp==zh+1 && u[enemy].hp==eh+1 && u[marine].hp==mh);
    assert(b[colony].hp==bh && b[colony].shield_remainder==79);
    b[colony].under_construction=0;
    b[colony].raw_type=sc64_race_raw_building(B_CREEP_COLONY);
    race_shared_hp_regen_tick();
    assert(b[colony].hp==bh+1 && b[colony].shield_remainder==0);
    assert(u[marine].shield_remainder==79);
    test_candidate_geometry_and_rows();
    test_actor_phase_owners_and_reuse();
    test_paused_and_live_update();
    test_campaign_and_skirmish_bindings();
#ifdef SC64_SHARED_TRANSPORT
    test_valid_loaded_passenger_regenerates();
#endif
#ifdef SC64_SHOWCASE_ENABLED
    test_showcase_live_update();
#endif
#ifdef SC64_P6_COMBAT_ENABLED
    test_p6_burrowed_regenerates();
#endif
#ifdef SC64_Z5_RUNTIME
    test_z5_legacy_tick_disabled();
#endif
    return 0;
}
