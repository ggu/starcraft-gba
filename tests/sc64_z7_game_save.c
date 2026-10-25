/* Actual-engine Z7 save transaction checks. This fixture uses the ordinary
 * menu and production helpers, then verifies that bad candidates are rejected
 * before the loader changes any live world state. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

#ifdef SC64_Z7_DERIVED_SAVE
#define TEST_UNIT_ENCODE sc64_derived_unit_encode
#define TEST_BUILDING_ENCODE sc64_derived_building_encode
#else
#define TEST_UNIT_ENCODE sc64_zp_unit_save_encode
#define TEST_BUILDING_ENCODE sc64_building_save_encode
#endif

#ifndef SC64_Z7_RUNTIME
#error "Compile this fixture with the complete Z7 runtime profile flags"
#endif

static unsigned held_keys;

static void input(unsigned held)
{
    game_update((u16)held,(u16)(held&~held_keys));
    held_keys=held;
}

static void press(unsigned key)
{
    input(key);
    input(0);
}

static void launch_z7_from_normal_menu(void)
{
    int ordinal;
    game_init();
    held_keys=0;
    assert(race_front.page==SC_FRONT_HOME);
    ordinal=sc64_catalog_playable_ordinal(SC64_FACTION_ZERG,SC64_Z7_GLOBAL_ID);
    assert(ordinal==6);
    press(KEY_A);
    assert(race_front.page==SC_FRONT_MISSION && race_front.faction==SC_FRONT_ZERG);
    for (int page=0;page<ordinal/2;++page) press(KEY_RIGHT);
    if (ordinal&1) press(KEY_DOWN);
    press(KEY_A);
    assert(race_front.page==SC_FRONT_BRIEFING &&
           race_front.mission==SC64_Z7_GLOBAL_ID);
    press(KEY_A);
    assert(screen==SCREEN_PLAY && !race_mode && !won && !lost);
    assert(race_selected_mission==SC64_Z7_GLOBAL_ID && race_z7_active());
    assert(race_campaign.mission==SC64_Z7_GLOBAL_ID);
    assert(sc64_z7_persistent_state_valid(&race_z7));
    assert(SC64_RACE_SAVE_GENERATION==
#ifdef SC64_Z7_DERIVED_SAVE
           38u
#elif defined(SC64_Z7_EGG_PRODUCTION)
           36u
#elif defined(SC64_Z7_COLONY_DEFENSE_ENABLED)
           35u
#else
           33u
#endif
    );
}

static void reject_without_mutation(const RaceSave *bad)
{
    Unit units[MAX_UNITS];
    Building buildings[MAX_BUILDINGS];
    Mineral minerals_before[MAX_MINERALS];
    u8 fog_before[(MAP_CELLS+7)/8];
    Sc64Z7PersistentState state;
    Sc64RaceCampaignState campaign;
    int players[5],ore[5],gas[5],clock;
    Geyser geysers_before[MAX_RACE_GEYSERS];
    memcpy(units,u,sizeof(u));
    memcpy(buildings,b,sizeof(b));
    memcpy(minerals_before,m,sizeof(m));
    memcpy(fog_before,explored,sizeof(fog_before));
    state=race_z7;
    campaign=race_campaign;
    memcpy(players,race_player,sizeof(players));
    memcpy(ore,race_minerals,sizeof(ore));
    memcpy(gas,race_gas,sizeof(gas));
    memcpy(geysers_before,geysers,sizeof(geysers_before));
    clock=frame;
    save_write(bad,sizeof(*bad),SAVE_VERSION);
    assert(!race_load_game());
    assert(!memcmp(units,u,sizeof(u)));
    assert(!memcmp(buildings,b,sizeof(b)));
    assert(!memcmp(minerals_before,m,sizeof(m)));
    assert(!memcmp(fog_before,explored,sizeof(fog_before)));
    assert(!memcmp(&state,&race_z7,sizeof(race_z7)));
    assert(!memcmp(&campaign,&race_campaign,sizeof(race_campaign)));
    assert(!memcmp(players,race_player,sizeof(players)));
    assert(!memcmp(ore,race_minerals,sizeof(ore)));
    assert(!memcmp(gas,race_gas,sizeof(gas)));
    assert(!memcmp(geysers_before,geysers,sizeof(geysers_before)));
    assert(frame==clock);
}

#ifdef SC64_Z7_DERIVED_SAVE
static void launch_zerg_campaign_mission(int mission)
{
    int ordinal;
    game_init(); held_keys=0;
    ordinal=sc64_catalog_playable_ordinal(SC64_FACTION_ZERG,mission);
    assert(ordinal>=0);
    press(KEY_A);
    assert(race_front.page==SC_FRONT_MISSION);
    for(int page=0;page<ordinal/2;page++)press(KEY_RIGHT);
    if(ordinal&1)press(KEY_DOWN);
    press(KEY_A); assert(race_front.page==SC_FRONT_BRIEFING);
    press(KEY_A); assert(screen==SCREEN_PLAY&&!race_mode);
    assert(race_selected_mission==mission);
}

static void test_prefix_and_skirmish_save_layout(void)
{
    const int mission_ids[]={0,1,14,15,SC64_Z5_GLOBAL_ID,SC64_Z6_GLOBAL_ID,
                             SC64_Z7_GLOBAL_ID};
    for(unsigned n=0;n<sizeof(mission_ids)/sizeof(mission_ids[0]);n++) {
        RaceSave snapshot;
        launch_zerg_campaign_mission(mission_ids[n]);
        for(int i=0;i<4;i++)game_update(0,0);
        assert(race_save_game()); snapshot=race_data;
        assert(sizeof(snapshot.units[0])==29&&sizeof(snapshot.buildings[0])==25);
        assert(race_load_game()); game_update(0,0); assert(race_save_game());
        assert(race_data.version==snapshot.version);
        printf("derived rows survived campaign mission %d save/load/save\n",mission_ids[n]);
    }
    assert(SC64_FRONT_MAP_COUNT==2);
    for(int race=R_TER;race<=R_PROTOSS;race++)
      for(int bots=1;bots<=3;bots++)
        for(int map=0;map<2;map++) {
            RaceSave snapshot;
            game_init(); held_keys=0;
            press(KEY_DOWN); press(KEY_A);
            assert(race_front.page==SC_FRONT_SKIRMISH);
            if(map)press(KEY_RIGHT);
            while(race_front.row<1)press(KEY_DOWN);
            while(race_front.race!=race)press(KEY_RIGHT);
            press(KEY_DOWN);
            while(race_front.bots!=bots)
                press(race_front.bots<bots?KEY_RIGHT:KEY_LEFT);
            while(race_front.row<3+bots)press(KEY_DOWN);
            press(KEY_A);
            assert(screen==SCREEN_PLAY&&race_mode&&race_human==race&&
                   race_bots==bots&&race_skirmish_map==map);
            assert(race_save_game()); snapshot=race_data;
            assert(race_load_game()); game_update(0,0); assert(race_save_game());
            assert(race_data.version==snapshot.version);
            assert(sizeof(snapshot.units[0])==29&&sizeof(snapshot.buildings[0])==25);
            printf("derived rows survived skirmish race=%d bots=%d map=%d save/load/save\n",
                   race,bots,map);
        }
}
#endif

static void test_terminal_source_witnesses(const RaceSave *checkpoint)
{
    RaceSave forged;
    assert(checkpoint);

    forged=*checkpoint;
    forged.z7.runtime.model.started|=(u16)(1u<<SC64_Z7_TRIGGER_VICTORY);
    forged.z7.runtime.model.action_cursor[SC64_Z7_TRIGGER_VICTORY]=0;
    forged.z7.runtime.model.outcome=SC64_Z7_WON;
    forged.state[18]=1; forged.state[19]=0;
    forged.state[3]|=1<<SC64_Z7_GLOBAL_ID;
    forged.campaign.phase=SC64_RACE_PHASE_COMPLETE;
    forged.campaign.objective_flags|=SC64_RACE_OBJECTIVE_PRIMARY;
    assert(sc64_z7_persistent_state_valid(&forged.z7));
    assert(!race_z7_save_candidate_valid(&forged,&forged.z7,
                                         SC64_Z7_MAP_COLS,SC64_Z7_MAP_ROWS));
    reject_without_mutation(&forged);

    forged=*checkpoint;
    forged.z7.runtime.model.started|=(u16)(1u<<SC64_Z7_TRIGGER_LOSS);
    forged.z7.runtime.model.action_cursor[SC64_Z7_TRIGGER_LOSS]=0;
    forged.z7.runtime.model.outcome=SC64_Z7_LOST;
    forged.state[18]=0; forged.state[19]=1;
    forged.campaign.phase=SC64_RACE_PHASE_FAILED;
    forged.campaign.objective_flags&=~SC64_RACE_OBJECTIVE_PRIMARY;
    assert(sc64_z7_persistent_state_valid(&forged.z7));
    assert(!race_z7_save_candidate_valid(&forged,&forged.z7,
                                         SC64_Z7_MAP_COLS,SC64_Z7_MAP_ROWS));
    reject_without_mutation(&forged);

    /* Exercise both terminal outcomes through the real world condition,
     * scheduler action callback, and campaign result wiring. */
    save_write(checkpoint,sizeof(*checkpoint),SAVE_VERSION);
    assert(race_load_game());
    for (int i=0;i<MAX_BUILDINGS;++i)
        if (b[i].alive && b[i].owner==ENEMY && b[i].raw_owner==4) {
            int damage=b[i].hp+sc64_race_building_armor(b[i].type)+1;
            hit_target(MAX_UNITS+i,damage,U_ZERGLING,SIDE);
        }
    for (unsigned tick=0;tick<4 && race_z7.runtime.model.outcome==SC64_Z7_RUNNING;++tick)
        game_update(0,0);
    assert(race_z7.runtime.model.outcome==SC64_Z7_WON && won && !lost);
    if (!race_save_game()) {
        unsigned human=0,enemy_buildings=0;
        for(int i=0;i<MAX_UNITS;++i) human+=u[i].alive&&u[i].owner==SIDE&&u[i].raw_owner==3;
        for(int i=0;i<MAX_BUILDINGS;++i) enemy_buildings+=b[i].alive&&b[i].owner==ENEMY&&b[i].raw_owner==4;
        fprintf(stderr,"won save rejected: candidate=%d live=%d phase=%u flags=%x state=%u outcome=%u human=%u enemybuildings=%u\n",
                race_z7_save_candidate_valid(&race_data,&race_z7,SC64_Z7_MAP_COLS,SC64_Z7_MAP_ROWS),
                race_z7_live_world_valid(),race_campaign.phase,race_campaign.objective_flags,
                race_data.state[18],race_z7.runtime.model.outcome,human,enemy_buildings);
    }
    assert(race_save_game());
    assert(race_z7_save_candidate_valid(&race_data,&race_data.z7,
                                        SC64_Z7_MAP_COLS,SC64_Z7_MAP_ROWS));

    save_write(checkpoint,sizeof(*checkpoint),SAVE_VERSION);
    assert(race_load_game());
    for (int i=0;i<MAX_UNITS;++i)
        if (u[i].alive && u[i].owner==SIDE && u[i].raw_owner==3)
            hit_target(i,u[i].hp+u[i].armor+armor_level+1,U_ZERGLING,ENEMY);
    for (int i=0;i<MAX_BUILDINGS;++i)
        if (b[i].alive && b[i].owner==SIDE && b[i].raw_owner==3)
            hit_target(MAX_UNITS+i,b[i].hp+sc64_race_building_armor(b[i].type)+1,
                       U_ZERGLING,ENEMY);
    for (unsigned tick=0;tick<4 && race_z7.runtime.model.outcome==SC64_Z7_RUNNING;++tick)
        game_update(0,0);
    assert(race_z7.runtime.model.outcome==SC64_Z7_LOST && lost && !won);
    assert(race_save_game());
    assert(race_z7_save_candidate_valid(&race_data,&race_data.z7,
                                        SC64_Z7_MAP_COLS,SC64_Z7_MAP_ROWS));
}

static unsigned count_lings_for_owner(int owner,unsigned raw_owner)
{
    unsigned count=0;
    for (unsigned i=0;i<MAX_UNITS;++i)
        count+=(u[i].owner==owner && u[i].raw_owner==(int)raw_owner &&
                u[i].raw_type==37);
    return count;
}

static unsigned count_enemy_lings(void)
{ return count_lings_for_owner(ENEMY,4u); }

static unsigned candidate_live_units(const RaceSave *candidate)
{
    unsigned count=0;
    for (unsigned i=0;i<MAX_UNITS;++i) {
        Unit q;
        assert(race_save_decode_unit(candidate,i,SC64_Z7_MAP_COLS*16u,
                                     SC64_Z7_MAP_ROWS*16u,&q));
        count+=q.alive!=0;
    }
    return count;
}

static void candidate_fill_paid_units(RaceSave *candidate,unsigned target)
{
    Unit template={0};
    unsigned live=candidate_live_units(candidate);
    int have_template=0;
    assert(live<=target && target<=MAX_UNITS);
    for (unsigned i=0;i<MAX_UNITS;++i) {
        Unit q;
        assert(race_save_decode_unit(candidate,i,SC64_Z7_MAP_COLS*16u,
                                     SC64_Z7_MAP_ROWS*16u,&q));
        if (q.alive && q.owner==SIDE) { template=q; have_template=1; break; }
    }
    assert(have_template);
    template.type=U_ZERGLING;
    template.owner=SIDE;
    template.raw_type=template.raw_owner=-1;
#ifdef SC64_Z7_DERIVED_SAVE
    template.maxhp=sc64_derived_unit_hp(template.type);
    template.armor=sc64_race_unit_armor(template.type);
#else
    template.maxhp=1;
#endif
    template.hp=1; template.alive=1; template.selected=0;
    template.order=ORDER_IDLE; template.target=-1;
    for (unsigned i=0;i<MAX_UNITS && live<target;++i) {
        Unit old;
        assert(race_save_decode_unit(candidate,i,SC64_Z7_MAP_COLS*16u,
                                     SC64_Z7_MAP_ROWS*16u,&old));
        if (old.alive) continue;
        assert(TEST_UNIT_ENCODE(&candidate->units[i],&template,
               SC64_Z7_MAP_COLS*16u,SC64_Z7_MAP_ROWS*16u));
        ++live;
    }
    assert(live==target);
}

static void test_ling_egg_capacity(const RaceSave *checkpoint)
{
    RaceSave candidate;
    int producer=-1;
    candidate=*checkpoint;
    for (unsigned i=0;i<MAX_BUILDINGS;++i) {
        Building q;
        assert(race_save_decode_building(&candidate,i,&q));
        if (q.alive && q.owner==ENEMY && q.raw_owner==4 &&
            (q.type==B_HATCHERY || q.type==B_LAIR || q.type==B_HIVE)) {
            producer=(int)i;
            q.queue_head=0; q.queue_count=1; q.queue_progress=0;
            memset(q.queue,0,sizeof(q.queue)); q.queue[0]=U_ZERGLING;
            assert(TEST_BUILDING_ENCODE(&candidate.buildings[i],&q,
                   SC64_Z7_MAP_COLS*16u,SC64_Z7_MAP_ROWS*16u));
            break;
        }
    }
    assert(producer>=0);
    candidate_fill_paid_units(&candidate,MAX_UNITS-2u);
    assert(race_z7_save_candidate_valid(&candidate,&candidate.z7,
                                        SC64_Z7_MAP_COLS,SC64_Z7_MAP_ROWS));
    candidate_fill_paid_units(&candidate,MAX_UNITS-1u);
    assert(!race_z7_save_candidate_valid(&candidate,&candidate.z7,
                                         SC64_Z7_MAP_COLS,SC64_Z7_MAP_ROWS));
}

static void test_paid_ai_egg_roundtrip(const RaceSave *checkpoint)
{
    unsigned before,producer=(unsigned)-1;
    RaceSave saved,bad;
    assert(checkpoint);
    save_write(checkpoint,sizeof(*checkpoint),SAVE_VERSION);
    assert(race_load_game());
    for (unsigned i=0;i<MAX_BUILDINGS;++i)
        if (b[i].alive && b[i].owner==ENEMY && b[i].raw_owner==4 &&
            (b[i].type==B_HATCHERY || b[i].type==B_LAIR || b[i].type==B_HIVE) &&
            building_complete((int)i)) { producer=i; break; }
    assert(producer<MAX_BUILDINGS);

    /* Exercise the actual bounded paid-AI pulse from a valid started state.
     * Its source-AI replacement is runtime policy; this fixture isolates the
     * resulting ordinary queue and its two-unit Zergling egg accounting. */
    race_z7.runtime.model.started|=(u16)(1u<<SC64_Z7_TRIGGER_AI);
    race_z7.runtime.model.action_cursor[SC64_Z7_TRIGGER_AI]=0;
    race_z7.runtime.ai_script_mask=15;
    race_z7.runtime.ai_plan_ticks=SC64_Z7_AI_PERIOD_TICKS;
    race_minerals[ENEMY]=minerals=1000;
    b[producer].larva=1;
    before=count_enemy_lings();
    assert(race_z7_authored_ai_pulse());
    assert(b[producer].queue_count==1 &&
           b[producer].queue[(b[producer].queue_head)%5]==U_ZERGLING);
    assert(sc64_z7_persistent_state_valid(&race_z7));
    assert(race_save_game());
    saved=race_data;
    assert(race_z7_save_candidate_valid(&saved,&saved.z7,
                                        SC64_Z7_MAP_COLS,SC64_Z7_MAP_ROWS));
    test_ling_egg_capacity(&saved);
    save_write(&saved,sizeof(saved),SAVE_VERSION);
    assert(race_load_game());
    assert(b[producer].queue_count==1);

    /* A Zergling egg creates a pair. Run past its production time and save
     * the reconciled source-owner products as well as the queued-state case. */
    for (unsigned i=0;i<
#ifdef SC64_Z7_EGG_PRODUCTION
         (unsigned)sc64_z7_egg_duration(U_ZERGLING)+20u
#else
         540u
#endif
         ;++i) game_update(0,0);
    assert(screen==SCREEN_PLAY && race_z7_active());
    if (count_enemy_lings()<before+2u)
        fprintf(stderr,"AI egg did not hatch: before=%u after=%u frame=%d producer=%u q=%d progress=%u mask=%u timer=%u minerals=%d outcome=%u\n",
                before,count_enemy_lings(),frame,producer,b[producer].queue_count,
                b[producer].queue_progress,race_z7.runtime.ai_script_mask,
                race_z7.runtime.ai_plan_ticks,race_minerals[ENEMY],race_z7.runtime.model.outcome);
    assert(count_enemy_lings()>=before+2u);
    assert(race_z7_live_world_valid());
    assert(race_save_game());
    saved=race_data;
    assert(race_z7_save_candidate_valid(&saved,&saved.z7,
                                        SC64_Z7_MAP_COLS,SC64_Z7_MAP_ROWS));

    bad=saved;
    {
        int changed=0;
        for (unsigned i=0;i<MAX_BUILDINGS;++i) {
            Building q;
            assert(race_save_decode_building(&saved,i,&q));
            if (!q.alive || q.raw_owner<0) continue;
            for (int raw=0;raw<228;++raw) {
                int mapped=sc64_race_building_from_raw(raw);
                if (mapped>=0 && mapped!=q.type) {
                    q.raw_type=raw;
                    assert(TEST_BUILDING_ENCODE(&bad.buildings[i],&q,
                           SC64_Z7_MAP_COLS*16u,SC64_Z7_MAP_ROWS*16u));
                    changed=1;
                    break;
                }
            }
            if (changed) break;
        }
        assert(changed);
    }
    assert(!race_z7_save_candidate_valid(&bad,&bad.z7,
                                         SC64_Z7_MAP_COLS,SC64_Z7_MAP_ROWS));

    bad=saved;
    {
        int changed=0;
        for (unsigned i=0;i<MAX_BUILDINGS;++i) {
            Building q;
            assert(race_save_decode_building(&saved,i,&q));
            if (!q.alive || q.owner!=SIDE || q.raw_owner<0 || q.queue_count) continue;
            q.queue_head=0; q.queue_count=1; q.queue_progress=0;
            memset(q.queue,0,sizeof(q.queue)); q.queue[0]=U_ZP_INFESTED_KERRIGAN;
            assert(TEST_BUILDING_ENCODE(&bad.buildings[i],&q,
                   SC64_Z7_MAP_COLS*16u,SC64_Z7_MAP_ROWS*16u));
            changed=1;
            break;
        }
        assert(changed);
    }
    assert(!race_z7_save_candidate_valid(&bad,&bad.z7,
                                         SC64_Z7_MAP_COLS,SC64_Z7_MAP_ROWS));

    /* A source-only hero with a disabled producer cannot be fabricated as
     * an unprovenanced human production result. */
    bad=saved;
    {
        int changed=0;
        for (unsigned i=0;i<MAX_UNITS;++i) {
            Unit q;
            assert(race_save_decode_unit(&saved,i,SC64_Z7_MAP_COLS*16u,
                                         SC64_Z7_MAP_ROWS*16u,&q));
            if (!q.alive || q.owner!=SIDE) continue;
            q.type=U_ZP_INFESTED_KERRIGAN;
            q.raw_type=q.raw_owner=-1;
            assert(TEST_UNIT_ENCODE(&bad.units[i],&q,
                   SC64_Z7_MAP_COLS*16u,SC64_Z7_MAP_ROWS*16u));
            changed=1;
            break;
        }
        assert(changed);
    }
    assert(!race_z7_save_candidate_valid(&bad,&bad.z7,
                                         SC64_Z7_MAP_COLS,SC64_Z7_MAP_ROWS));
}
#ifdef SC64_Z7_DERIVED_SAVE
static void test_derived_rows(const RaceSave *saved)
{
    unsigned units=0,buildings=0;
    {
        Sc64DerivedUnitSave zero={{0}};
        Unit output,before;
        memset(&output,0xA5,sizeof(output)); before=output;
        assert(!sc64_derived_unit_decode(&output,&zero,0,64));
        assert(!memcmp(&output,&before,sizeof(output)));
        assert(!sc64_derived_unit_decode(&output,&zero,64,2049));
        assert(!memcmp(&output,&before,sizeof(output)));
        assert(sc64_derived_unit_decode(&output,&zero,64,64));
        { Unit expected={0}; assert(!memcmp(&output,&expected,sizeof(output))); }
    }
    for(unsigned i=0;i<MAX_UNITS;i++) {
        Unit a,bad,b;
        Sc64DerivedUnitSave row,keep;
        assert(race_save_decode_unit(saved,i,SC64_Z7_MAP_COLS*16u,
                                     SC64_Z7_MAP_ROWS*16u,&a));
        if(sc64_derived_zero(saved->units[i].bytes,sizeof(saved->units[i].bytes)))
            continue;
        assert(sc64_derived_unit_encode(&row,&a,SC64_Z7_MAP_COLS*16u,
                                        SC64_Z7_MAP_ROWS*16u));
        assert(sc64_derived_unit_decode(&b,&row,SC64_Z7_MAP_COLS*16u,
                                        SC64_Z7_MAP_ROWS*16u));
        assert(memcmp(&a,&b,sizeof(a))==0);
        /* weapon is intentionally omitted: a nonzero source field has no
         * behavior reads, and decode returns the documented canonical zero. */
        bad=a; bad.weapon=1;
        assert(sc64_derived_unit_encode(&row,&bad,SC64_Z7_MAP_COLS*16u,
                                        SC64_Z7_MAP_ROWS*16u));
        assert(sc64_derived_unit_decode(&b,&row,SC64_Z7_MAP_COLS*16u,
                                        SC64_Z7_MAP_ROWS*16u));
        assert(b.weapon==0);
        keep=row; bad=a; ++bad.maxhp;
        assert(!sc64_derived_unit_encode(&row,&bad,SC64_Z7_MAP_COLS*16u,
                                         SC64_Z7_MAP_ROWS*16u));
        assert(memcmp(&row,&keep,sizeof(row))==0);
        keep=row; bad=a; ++bad.armor;
        assert(!sc64_derived_unit_encode(&row,&bad,SC64_Z7_MAP_COLS*16u,
                                         SC64_Z7_MAP_ROWS*16u));
        assert(memcmp(&row,&keep,sizeof(row))==0);
        bad=a; bad.flags=256;
        assert(!sc64_derived_unit_encode(&row,&bad,SC64_Z7_MAP_COLS*16u,
                                         SC64_Z7_MAP_ROWS*16u));
        bad=a; bad.raw_type=255;
        assert(!sc64_derived_unit_encode(&row,&bad,SC64_Z7_MAP_COLS*16u,
                                         SC64_Z7_MAP_ROWS*16u));
        bad=a; bad.raw_owner=15;
        assert(!sc64_derived_unit_encode(&row,&bad,SC64_Z7_MAP_COLS*16u,
                                         SC64_Z7_MAP_ROWS*16u));
        ++units;
    }
    for(unsigned i=0;i<MAX_BUILDINGS;i++) {
        Building a,bad,b;
        Sc64DerivedBuildingSave row,keep;
        assert(race_save_decode_building(saved,i,&a));
        if(sc64_derived_zero(saved->buildings[i].bytes,sizeof(saved->buildings[i].bytes)))
            continue;
        assert(sc64_derived_building_encode(&row,&a,SC64_Z7_MAP_COLS*16u,
                                            SC64_Z7_MAP_ROWS*16u));
        assert(sc64_derived_building_decode(&b,&row));
        assert(memcmp(&a,&b,sizeof(a))==0);
        keep=row; bad=a; ++bad.maxhp;
        assert(!sc64_derived_building_encode(&row,&bad,SC64_Z7_MAP_COLS*16u,
                                             SC64_Z7_MAP_ROWS*16u));
        assert(memcmp(&row,&keep,sizeof(row))==0);
        bad=a; bad.flags|=0x8000;
        assert(sc64_derived_building_encode(&row,&bad,SC64_Z7_MAP_COLS*16u,
                                            SC64_Z7_MAP_ROWS*16u));
        assert(sc64_derived_building_decode(&b,&row));
        assert(b.flags==bad.flags);
        bad=a; bad.raw_type=255;
        assert(!sc64_derived_building_encode(&row,&bad,SC64_Z7_MAP_COLS*16u,
                                             SC64_Z7_MAP_ROWS*16u));
        ++buildings;
    }
    assert(units>0&&buildings>0);
    for(int type=0;type<SC_RU_COUNT;type++) {
        int hp=sc64_derived_unit_hp(type);
        Unit q,decoded;
        Sc64DerivedUnitSave row;
        if(hp<=0)continue;
        memset(&q,0,sizeof(q));
        q.x=q.y=q.tx=q.ty=q.path_x=q.path_y=1;
        q.type=type; q.owner=SIDE; q.hp=q.maxhp=hp; q.alive=1;
        q.armor=sc64_race_unit_armor(type); q.target=q.build=-1;
        q.raw_type=q.raw_owner=-1;
        assert(sc64_derived_unit_encode(&row,&q,SC64_Z7_MAP_COLS*16u,
                                        SC64_Z7_MAP_ROWS*16u));
        assert(sc64_derived_unit_decode(&decoded,&row,SC64_Z7_MAP_COLS*16u,
                                        SC64_Z7_MAP_ROWS*16u));
        assert(decoded.type==type&&decoded.maxhp==hp&&decoded.armor==q.armor);
        q.alive=0; q.hp=0;
        assert(sc64_derived_unit_encode(&row,&q,SC64_Z7_MAP_COLS*16u,
                                        SC64_Z7_MAP_ROWS*16u));
        assert(sc64_derived_unit_decode(&decoded,&row,SC64_Z7_MAP_COLS*16u,
                                        SC64_Z7_MAP_ROWS*16u));
        assert(!decoded.alive&&decoded.type==type&&decoded.maxhp==hp);
    }
    {
        int type=sc64_race_unit_from_raw(19);
        if (type>=0) {
            for (int raw=19;raw<=20;++raw) {
                const Sc64StockUnitProfile *stock=sc64_stock_unit_profile(raw);
                Unit q,decoded;
                Sc64DerivedUnitSave row;
                assert(stock && stock->hp);
                memset(&q,0,sizeof(q));
                q.x=q.y=q.tx=q.ty=q.path_x=q.path_y=1;
                q.type=type; q.owner=SIDE; q.hp=q.maxhp=(int)stock->hp;
                q.alive=1; q.armor=sc64_race_unit_armor(type);
                q.target=q.build=-1; q.raw_type=raw; q.raw_owner=0;
                assert(sc64_derived_unit_encode(&row,&q,SC64_Z7_MAP_COLS*16u,
                                                SC64_Z7_MAP_ROWS*16u));
                assert(sc64_derived_unit_decode(&decoded,&row,
                                                SC64_Z7_MAP_COLS*16u,
                                                SC64_Z7_MAP_ROWS*16u));
                assert(decoded.type==type && decoded.raw_type==raw &&
                       decoded.maxhp==(int)stock->hp);
            }
        }
    }
    for(int type=0;type<SC_RB_COUNT;type++) {
        int hp=sc64_derived_building_hp(type);
        Building q,decoded;
        Sc64DerivedBuildingSave row;
        if(hp<=0)continue;
        memset(&q,0,sizeof(q));
        q.x=q.y=1; q.type=type; q.owner=SIDE; q.hp=q.maxhp=hp;
        q.alive=1; q.build_progress=-1; q.raw_type=q.raw_owner=-1;
        assert(sc64_derived_building_encode(&row,&q,SC64_Z7_MAP_COLS*16u,
                                            SC64_Z7_MAP_ROWS*16u));
        assert(sc64_derived_building_decode(&decoded,&row));
        assert(decoded.type==type&&decoded.maxhp==hp);
        q.alive=0; q.hp=0;
        assert(sc64_derived_building_encode(&row,&q,SC64_Z7_MAP_COLS*16u,
                                            SC64_Z7_MAP_ROWS*16u));
        assert(sc64_derived_building_decode(&decoded,&row));
        assert(!decoded.alive&&decoded.type==type&&decoded.maxhp==hp);
    }
    {
        RaceSave malformed=*saved;
        Unit output,before;
        int slot=-1;
        for(unsigned i=0;i<MAX_UNITS;i++)
            if(!sc64_derived_zero(malformed.units[i].bytes,sizeof(malformed.units[i].bytes)))
                {slot=(int)i;break;}
        assert(slot>=0);
        malformed.units[slot].bytes[SC64_DERIVED_UNIT_BYTES-1]|=0x80u;
        memset(&output,0xa5,sizeof(output)); before=output;
        assert(!sc64_derived_unit_decode(&output,&malformed.units[slot],
                                         SC64_Z7_MAP_COLS*16u,SC64_Z7_MAP_ROWS*16u));
        assert(memcmp(&output,&before,sizeof(output))==0);
        assert(!race_z7_save_candidate_valid(&malformed,&malformed.z7,
                                             SC64_Z7_MAP_COLS,SC64_Z7_MAP_ROWS));
        reject_without_mutation(&malformed);
    }
    {
        RaceSave malformed=*saved;
        Building output,before;
        int slot=-1;
        for(unsigned i=0;i<MAX_BUILDINGS;i++)
            if(!sc64_derived_zero(malformed.buildings[i].bytes,sizeof(malformed.buildings[i].bytes)))
                {slot=(int)i;break;}
        assert(slot>=0);
        malformed.buildings[slot].bytes[SC64_DERIVED_BUILDING_BYTES-1]|=0x80u;
        memset(&output,0x5a,sizeof(output)); before=output;
        assert(!sc64_derived_building_decode(&output,&malformed.buildings[slot]));
        assert(memcmp(&output,&before,sizeof(output))==0);
        assert(!race_z7_save_candidate_valid(&malformed,&malformed.z7,
                                             SC64_Z7_MAP_COLS,SC64_Z7_MAP_ROWS));
        reject_without_mutation(&malformed);
    }
}
#endif

static void test_z7_candidate_and_live_roundtrip(void)
{
    RaceSave good,bad;
    int producer=-1;
#ifndef SC64_Z7_EGG_PRODUCTION
    unsigned human_lings_before;
#endif

    launch_z7_from_normal_menu();
    for (unsigned i=0;i<4;++i) game_update(0,0);
    assert(screen==SCREEN_PLAY && sc64_z7_persistent_state_valid(&race_z7));
    assert(race_minerals[SIDE]>=800 && race_gas[SIDE]>=200);
    assert(race_z7_source_row_counts(race_z7_map));

    /* Queue one normally paid Zergling on a live source Hatchery when its
     * ordinary larva/prerequisite state permits it. This proves the save
     * validator accepts a real production queue and reconciled product. */
    for (int i=0;i<MAX_BUILDINGS;++i)
        if (b[i].alive && b[i].owner==SIDE && b[i].raw_owner==3 &&
            b[i].raw_type==131) { producer=i; break; }
    if (producer<0) {
        /* The authenticated scenario starts without a human producer. Build
         * a minimal source-owned Zerg producer/prerequisite fixture so this
         * test exercises the ordinary paid queue path, without changing the
         * production rules or accepting an unbacked queue in saved state. */
        producer=spawn_build(SIDE,B_HATCHERY,420,420,0);
        int pool=spawn_build(SIDE,B_SPAWNING_POOL,470,420,0);
        assert(producer>=0 && pool>=0);
        b[producer].raw_owner=b[pool].raw_owner=3;
        b[producer].raw_type=sc64_race_raw_building(B_HATCHERY);
        b[pool].raw_type=sc64_race_raw_building(B_SPAWNING_POOL);
        b[producer].larva=1;
    }
    race_minerals[SIDE]=minerals=800;
    race_gas[SIDE]=gas=200;
#ifndef SC64_Z7_EGG_PRODUCTION
    human_lings_before=count_lings_for_owner(SIDE,3u);
#endif
    assert(race_queue_unit(SIDE,producer,U_ZERGLING));
#ifdef SC64_Z7_EGG_PRODUCTION
    { int egg=0; for(int i=0;i<MAX_UNITS;i++)
        if(u[i].alive&&u[i].owner==SIDE&&u[i].type==U_Z7_EGG&&u[i].target==U_ZERGLING)egg=1;
      assert(egg); }
#else
    assert(b[producer].queue_count>0);
#endif
#ifdef SC64_Z7_EGG_PRODUCTION
    for (unsigned i=0;i<4;++i) game_update(0,0);
    assert(race_z7_live_world_valid());
#else
    for (unsigned i=0;i<540;++i) game_update(0,0);
    assert(race_z7_live_world_valid());
    assert(b[producer].queue_count==0);
    assert(count_lings_for_owner(SIDE,3u)>=human_lings_before+2u);
#endif

    assert(race_campaign.elapsed_frames==(u32)frame);
    if (!race_save_game()) {
        fprintf(stderr,"save rejected: valid=%d rows=%u/%u resources=%d geysers=%d state=%d/%d/%d frame=%d elapsed=%u outcome=%u mask=%u\n",
            race_z7_save_candidate_valid(&race_data,&race_z7,SC64_Z7_MAP_COLS,SC64_Z7_MAP_ROWS),
            race_z7_map->unit_count,MAX_UNITS,race_minerals[SIDE],geyser_count,
            race_data.state[2],race_data.state[7],race_data.state[16],frame,
            race_campaign.elapsed_frames,race_z7.runtime.model.outcome,
            race_z7.runtime.ai_script_mask);
        assert(0);
    }
    good=race_data;
#ifdef SC64_Z7_DERIVED_SAVE
    test_derived_rows(&good);
#endif
    assert(race_z7_save_candidate_valid(&good,&good.z7,
                                        SC64_Z7_MAP_COLS,SC64_Z7_MAP_ROWS));
    assert(sizeof(RaceSave)<=16368u);
    for (unsigned i=sizeof(good.z7);i<sizeof(good.z5);++i)
        assert(((const u8 *)&good.z7)[i]==0);
    save_write(&good,sizeof(good),SAVE_VERSION);
    assert(race_load_game());
    assert(sc64_z7_persistent_state_valid(&race_z7));
    assert(race_z7_live_world_valid());
    game_update(0,0);
    assert(race_z7_live_world_valid());

    bad=good; bad.z7.runtime.ai_plan_ticks=SC64_Z7_AI_PERIOD_TICKS+1u;
    assert(!race_z7_save_candidate_valid(&bad,&bad.z7,
                                         SC64_Z7_MAP_COLS,SC64_Z7_MAP_ROWS));
    reject_without_mutation(&bad);

    bad=good; bad.players[SIDE]=R_PROTOSS;
    assert(!race_z7_save_candidate_valid(&bad,&bad.z7,
                                         SC64_Z7_MAP_COLS,SC64_Z7_MAP_ROWS));
    reject_without_mutation(&bad);

    bad=good; bad.campaign.mission=SC64_Z6_GLOBAL_ID;
    assert(!race_z7_save_candidate_valid(&bad,&bad.z7,
                                         SC64_Z7_MAP_COLS,SC64_Z7_MAP_ROWS));
    reject_without_mutation(&bad);

    bad=good; bad.version=SAVE_VERSION-1u;
    assert(!race_z7_save_candidate_valid(&bad,&bad.z7,
                                         SC64_Z7_MAP_COLS,SC64_Z7_MAP_ROWS));
    reject_without_mutation(&bad);

    /* Preserve the row codecs while corrupting authenticated world facts. */
    bad=good;
    {
        int changed=0;
        for (unsigned i=0;i<MAX_UNITS;++i) {
            Unit q;
            assert(race_save_decode_unit(&good,i,SC64_Z7_MAP_COLS*16u,
                                         SC64_Z7_MAP_ROWS*16u,&q));
            if (!q.alive || q.raw_owner<0) continue;
            q.raw_owner=q.raw_owner==3?4:3;
            assert(TEST_UNIT_ENCODE(&bad.units[i],&q,
                   SC64_Z7_MAP_COLS*16u,SC64_Z7_MAP_ROWS*16u));
            changed=1;
            break;
        }
        assert(changed);
    }
    assert(!race_z7_save_candidate_valid(&bad,&bad.z7,
                                         SC64_Z7_MAP_COLS,SC64_Z7_MAP_ROWS));
    reject_without_mutation(&bad);

    bad=good;
    {
        int changed=0;
        for (unsigned i=0;i<MAX_BUILDINGS;++i) {
            Building q;
            assert(race_save_decode_building(&good,i,&q));
            if (!q.alive || q.raw_owner==3) continue;
            ++q.x;
            assert(TEST_BUILDING_ENCODE(&bad.buildings[i],&q,
                   SC64_Z7_MAP_COLS*16u,SC64_Z7_MAP_ROWS*16u));
            changed=1;
            break;
        }
        assert(changed);
    }
    assert(!race_z7_save_candidate_valid(&bad,&bad.z7,
                                         SC64_Z7_MAP_COLS,SC64_Z7_MAP_ROWS));
    reject_without_mutation(&bad);

    bad=good;
    {
        int changed=0;
        for (unsigned i=0;i<MAX_MINERALS;++i) {
            Mineral q;
            assert(race_save_decode_mineral(&good,i,SC64_Z7_MAP_COLS*16u,
                                            SC64_Z7_MAP_ROWS*16u,&q));
            if (q.amount<=0) continue;
            q.amount=65535;
            assert(sc64_mineral_save_encode(&bad.minerals_map[i],&q,
                   SC64_Z7_MAP_COLS*16u,SC64_Z7_MAP_ROWS*16u));
            changed=1;
            break;
        }
        assert(changed);
    }
    assert(!race_z7_save_candidate_valid(&bad,&bad.z7,
                                         SC64_Z7_MAP_COLS,SC64_Z7_MAP_ROWS));
    reject_without_mutation(&bad);

#ifndef SC64_Z7_EGG_PRODUCTION
    test_paid_ai_egg_roundtrip(&good);
#endif
    test_terminal_source_witnesses(&good);

    printf("Z7 normal-menu candidate save/load and transactional rejection passed; RaceSave=%zu unitRow=%zu buildingRow=%zu\n",
           sizeof(RaceSave),sizeof(race_data.units[0]),sizeof(race_data.buildings[0]));
}

int main(void)
{
    test_z7_candidate_and_live_roundtrip();
#ifdef SC64_Z7_DERIVED_SAVE
    test_prefix_and_skirmish_save_layout();
#endif
    return 0;
}
