/* Actual-engine Z7 save transaction checks. This fixture uses the ordinary
 * menu and production helpers, then verifies that bad candidates are rejected
 * before the loader changes any live world state. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

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
    assert(SC64_RACE_SAVE_GENERATION==33u);
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
    template.hp=1; template.maxhp=1; template.alive=1; template.selected=0;
    template.order=ORDER_IDLE; template.target=-1;
    for (unsigned i=0;i<MAX_UNITS && live<target;++i) {
        Unit old;
        assert(race_save_decode_unit(candidate,i,SC64_Z7_MAP_COLS*16u,
                                     SC64_Z7_MAP_ROWS*16u,&old));
        if (old.alive) continue;
        assert(sc64_zp_unit_save_encode(&candidate->units[i],&template,
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
            assert(sc64_building_save_encode(&candidate.buildings[i],&q,
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
    for (unsigned i=0;i<540;++i) game_update(0,0);
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
                    assert(sc64_building_save_encode(&bad.buildings[i],&q,
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
            assert(sc64_building_save_encode(&bad.buildings[i],&q,
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
            assert(sc64_zp_unit_save_encode(&bad.units[i],&q,
                   SC64_Z7_MAP_COLS*16u,SC64_Z7_MAP_ROWS*16u));
            changed=1;
            break;
        }
        assert(changed);
    }
    assert(!race_z7_save_candidate_valid(&bad,&bad.z7,
                                         SC64_Z7_MAP_COLS,SC64_Z7_MAP_ROWS));
}

static void test_z7_candidate_and_live_roundtrip(void)
{
    RaceSave good,bad;
    int producer=-1;
    unsigned human_lings_before;

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
    human_lings_before=count_lings_for_owner(SIDE,3u);
    assert(race_queue_unit(SIDE,producer,U_ZERGLING));
    assert(b[producer].queue_count>0);
    for (unsigned i=0;i<540;++i) game_update(0,0);
    assert(race_z7_live_world_valid());
    assert(b[producer].queue_count==0);
    if (count_lings_for_owner(SIDE,3u)<human_lings_before+2u)
        fprintf(stderr,"human egg did not hatch: before=%u after=%u producer=%d alive=%d raw=%d/%d q=%d progress=%d frame=%d\n",
                human_lings_before,count_lings_for_owner(SIDE,3u),producer,
                b[producer].alive,b[producer].raw_owner,b[producer].raw_type,
                b[producer].queue_count,b[producer].queue_progress,frame);
    assert(count_lings_for_owner(SIDE,3u)>=human_lings_before+2u);

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
    assert(race_z7_save_candidate_valid(&good,&good.z7,
                                        SC64_Z7_MAP_COLS,SC64_Z7_MAP_ROWS));
    assert(sizeof(RaceSave)<=16368u);
    for (unsigned i=sizeof(good.z7);i<sizeof(good.z5);++i)
        assert(((const u8 *)&good.z7)[i]==0);
    save_write(&good,sizeof(good),SAVE_VERSION);
    assert(race_load_game());
    assert(sc64_z7_persistent_state_valid(&race_z7));
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
            assert(sc64_zp_unit_save_encode(&bad.units[i],&q,
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
            assert(sc64_building_save_encode(&bad.buildings[i],&q,
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

    test_paid_ai_egg_roundtrip(&good);
    test_terminal_source_witnesses(&good);

    puts("Z7 normal-menu candidate save/load and transactional rejection passed");
}

int main(void)
{
    test_z7_candidate_and_live_roundtrip();
    return 0;
}
