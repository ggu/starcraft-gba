/* Actual UI/trigger checks for the experimental P7 runtime profile. */
#include "../starcraft/game.c"
#include <assert.h>
#include <stdio.h>

static void tap(u16 key)
{
    game_update(key, key);
    game_update(0, 0);
}

static unsigned live_units(void)
{
    unsigned n=0;
    for (int i=0;i<MAX_UNITS;++i) n += u[i].alive != 0;
    return n;
}

static unsigned live_buildings(void)
{
    unsigned n=0;
    for (int i=0;i<MAX_BUILDINGS;++i) n += b[i].alive != 0;
    return n;
}

static unsigned live_condition_vector(void)
{
    unsigned bits=0;
    for (unsigned t=0;t<SC64_P7_TRIGGER_COUNT;++t) {
        const Sc64P7SourceCondition *c;
        assert(sc64_p7_condition_row(t,0,&c));
        int value=sc64_p7_world_condition(&race_p7_world,&race_p7.runtime,c);
        assert(value==0 || value==1);
        if (value) bits|=1u<<t;
    }
    return bits;
}

static void kill_source_condition(unsigned trigger)
{
    const Sc64P7SourceCondition *c;
    u32 count=0;
    assert(sc64_p7_condition_row(trigger,0,&c));
    assert(c->opcode==SC64_P7_CONDITION_COMMAND);
    assert(sc64_p7_world_count(&race_p7_world,c->player,c->unit,0,&count));
    assert(count>0);
    for (int i=0;i<MAX_UNITS;++i)
        if (u[i].alive && u[i].raw_type==(int)c->unit &&
            sc64_p7_world_owner_matches(&race_p7_world,c->player,
                                        u[i].raw_owner,u[i].owner)==1)
            hit_target(i,100000,U_MARINE,SIDE);
    assert(sc64_p7_world_count(&race_p7_world,c->player,c->unit,0,&count));
    assert(count==0);
}

static unsigned trace_hash(void)
{
    const unsigned char *parts[]={ (const unsigned char *)&race_p7,
        (const unsigned char *)u, (const unsigned char *)b };
    const size_t lengths[]={sizeof(race_p7),sizeof(u),sizeof(b)};
    unsigned h=2166136261u;
    for (unsigned p=0;p<3;++p)
        for (size_t i=0;i<lengths[p];++i) { h^=parts[p][i]; h*=16777619u; }
    h^=(unsigned)frame; h*=16777619u;
    h^=(unsigned)race_minerals[SIDE]; h*=16777619u;
    h^=(unsigned)race_gas[SIDE]; h*=16777619u;
    return h;
}

#ifdef SC64_P7_PREVALIDATED_TICK
static void check_mutable_validator_parity(void)
{
    /* Exercise every byte value at every persistent-state offset. This also
     * includes padding; neither validator may manufacture extra saved state. */
    const Sc64P7PersistentState original=race_p7;
    for (size_t byte=0;byte<sizeof(original);++byte)
        for (unsigned value=0;value<256;++value) {
            Sc64P7PersistentState candidate=original;
            ((u8 *)&candidate)[byte]=(u8)value;
            assert(sc64_p7_persistent_state_valid(&candidate)==
                   sc64_p7_persistent_state_valid_prevalidated(&candidate));
        }
    assert(!memcmp(&original,&race_p7,sizeof(original)));
}
#endif

int main(void)
{
    game_init();
    assert(screen==SCREEN_TITLE);
    assert(race_front.page==SC_FRONT_HOME);
    assert(sc64_catalog_mission_allowed(SC64_P7_GLOBAL_ID,SC64_FACTION_PROTOSS));
    tap(KEY_A); /* HOME to Protoss mission page in the single-race profile. */
    assert(screen==SCREEN_TITLE && race_front.page==SC_FRONT_MISSION);
    assert(race_front.faction==SC_FRONT_PROTOSS);
    tap(KEY_RIGHT); tap(KEY_RIGHT); tap(KEY_RIGHT); /* final P7 mission page */
    assert(race_front.page==SC_FRONT_MISSION && race_front.mission_page==3);
    assert(sc64_front_mission_rows(&race_front)==1);
    tap(KEY_A); /* mission page to briefing */
    assert(race_front.page==SC_FRONT_BRIEFING && race_front.mission==SC64_P7_GLOBAL_ID);
    game_update(KEY_A,KEY_A); /* launch; inspect before another engine tick */

#ifdef SC64_P7_PREVALIDATED_TICK
    if(screen!=SCREEN_PLAY) fprintf(stderr,"launch screen=%d fault=%u statevalid=%d bind=%d frame=%d\n",screen,race_p7.runtime.fault,race_p7_runtime_state_valid(),race_p7_world_prevalidated_bindings_valid(),frame);
#endif
    assert(screen==SCREEN_PLAY);
    assert(!race_mode && race_selected_mission==SC64_P7_GLOBAL_ID);
    assert(race_p7_active() && race_p7_runtime_state_valid());
    assert(race_p7_map==&sc64_race_maps[SC64_P7_GLOBAL_ID]);
    assert(sc64_race_maps[SC64_P7_GLOBAL_ID].width==128);
    assert(sc64_race_maps[SC64_P7_GLOBAL_ID].height==128);
    assert(sc64_race_maps[SC64_P7_GLOBAL_ID].unit_count==357);
    assert(live_units()==SC64_P7_MAP_UNITS);
    assert(live_buildings()==SC64_P7_MAP_BUILDINGS);
#ifdef SC64_P7_COUNT_SNAPSHOT
    {
        int building=-1,was_constructing;
        for (int i=0;i<MAX_BUILDINGS;++i) if (b[i].alive) { building=i; break; }
        assert(building>=0);
        was_constructing=b[building].under_construction;
        b[building].under_construction=1; /* Command includes unfinished buildings. */
        assert(sc64_p7_count_snapshot_begin(&race_p7_world));
        for (unsigned t=0;t<SC64_P7_TRIGGER_COUNT;++t) {
            const Sc64P7SourceCondition *c;
            u32 cached=0,checked=0;
            assert(sc64_p7_condition_row(t,0,&c));
            if (c->opcode!=SC64_P7_CONDITION_COMMAND) continue;
            assert(!sc64_p7_count_snapshot_lookup(&race_p7_world,c,&cached));
            race_p7_world.prevalidated_tick=1;
            assert(sc64_p7_count_snapshot_lookup(&race_p7_world,c,&cached));
            assert(sc64_p7_world_count(&race_p7_world,c->player,c->unit,0,&checked));
            assert(cached==checked);
            race_p7_world.prevalidated_tick=0;
        }
        sc64_p7_count_snapshot_end(&race_p7_world);
        assert(!race_p7_world.count_snapshot.active);
        b[building].under_construction=was_constructing;
    }
#endif
    {
        u32 ignored=0;
        int actor=-1,old_owner;
        assert(!sc64_p7_world_count(&race_p7_world,14,229,0,&ignored));
        for (int i=0;i<MAX_UNITS;++i) if (u[i].alive) { actor=i; break; }
        assert(actor>=0);
        old_owner=u[actor].raw_owner;
        u[actor].raw_owner=-1;
        assert(!sc64_p7_world_count(&race_p7_world,13,229,0,&ignored));
#ifdef SC64_P7_COUNT_SNAPSHOT
        race_p7_world.prevalidated_tick=1;
        assert(!sc64_p7_count_snapshot_begin(&race_p7_world));
        {
            const Sc64P7SourceCondition *c;
            assert(sc64_p7_condition_row(0,0,&c));
            assert(sc64_p7_world_condition(&race_p7_world,&race_p7.runtime,c)<0);
        }
        race_p7_world.prevalidated_tick=0;
        assert(!race_p7_world.count_snapshot.active);
#endif
        u[actor].raw_owner=old_owner;
    }
#ifdef SC64_P7_ARBITER_CLOAK_CACHE_ENABLED
    /* Normal menu launch occurs between ticks; first cache refresh happens
       on the first ordinary play tick before that tick's unit orders. */
    game_update(0,0);
    assert(sc64_p7_arbiter_cache_initialized);
    for (int i=0;i<MAX_UNITS;++i)
        assert(sc64_p7_arbiter_cache_unit_valid(&u[i]));
#endif

    for (int i=0;i<120;++i) {
        game_update(0,0);
#ifdef SC64_P7_PREVALIDATED_TICK
        if(screen!=SCREEN_PLAY) { fprintf(stderr,"tickfail i=%d screen=%d fault=%u dynamic=%d bind=%d frame=%d cursor4=%u ticks4=%u\n",i,screen,race_p7.runtime.fault,sc64_p7_persistent_state_valid_prevalidated(&race_p7),race_p7_world_prevalidated_bindings_valid(),frame,race_p7.runtime.model.action_cursor[4],race_p7.runtime.action_ticks[4]); break; }
#endif
    }
    assert(screen==SCREEN_PLAY && race_p7_runtime_state_valid());
#ifdef SC64_P7_PREVALIDATED_TICK
    check_mutable_validator_parity();
#endif
    {
        const u8 reserved=race_p7.reserved[0];
        race_p7.reserved[0]=255;
        assert(!save_game()); /* save candidate remains fully validated */
        race_p7.reserved[0]=reserved;
    }
    {
        const int saved_frame=frame;
        tap(KEY_START);
        assert(screen==SCREEN_PAUSE);
        tap(KEY_B);
        assert(screen==SCREEN_PAUSE);
        assert(save_read(&race_data,sizeof(race_data),SAVE_VERSION));
#ifdef SC64_P7_ARBITER_CLOAK_CACHE_ENABLED
        {
            static RaceSave valid_candidate;
            int slot=-1;
            Unit decoded;
            Sc64ZpPackedUnitSave malformed_row;
            valid_candidate=race_data;
            for(int i=0;i<MAX_UNITS;++i) if(u[i].alive) { slot=i; break; }
            assert(slot>=0);
            assert(sc64_zp_unit_save_decode(&decoded,&race_data.units[slot],
                                             128u*16u,128u*16u));
            decoded.weapon=(int)((unsigned)decoded.weapon&~(31u<<8))|(31u<<8);
            assert(!sc64_p7_arbiter_cache_unit_valid(&decoded));
            assert(sc64_zp_unit_save_encode(&malformed_row,&decoded,
                                             128u*16u,128u*16u));
            race_data.units[slot]=malformed_row;
            save_write(&race_data,sizeof(race_data),SAVE_VERSION);
            const unsigned before_bad_cache_load=trace_hash();
            game_update(KEY_L,KEY_L);
            assert(screen==SCREEN_PAUSE && trace_hash()==before_bad_cache_load);
            race_data=valid_candidate;
            save_write(&race_data,sizeof(race_data),SAVE_VERSION);
        }
#endif
        race_data.p7.reserved[0]=255;
        save_write(&race_data,sizeof(race_data),SAVE_VERSION);
        game_update(KEY_L,KEY_L); /* structurally corrupt but checksummed save */
        assert(screen==SCREEN_PAUSE && race_p7_runtime_state_valid());
        tap(KEY_B); /* replace the corrupt candidate with a genuine save */
        tap(KEY_A); /* resume once; the release update advances one tick */
        /* Re-enter pause, then load with L through game_update. */
        tap(KEY_START);
        assert(screen==SCREEN_PAUSE);
        game_update(KEY_L,KEY_L); /* load edge; inspect before the next simulation tick */
        assert(screen==SCREEN_PLAY); /* load restores the saved play state */
        assert(frame==saved_frame);
        assert(race_selected_mission==SC64_P7_GLOBAL_ID && !race_mode);
        assert(race_p7_active() && race_p7_runtime_state_valid());
        assert(race_p7_map==&sc64_race_maps[SC64_P7_GLOBAL_ID]);
        assert(live_units()==SC64_P7_MAP_UNITS);
        assert(live_buildings()==SC64_P7_MAP_BUILDINGS);
#ifdef SC64_P7_ARBITER_CLOAK_CACHE_ENABLED
        {
            static Sc64P7PersistentState saved_p7;
            static Unit saved_units[MAX_UNITS];
            static Building saved_buildings[MAX_BUILDINGS];
            static int saved_players[5],saved_minerals[5],saved_gas[5];
            static int saved_frame,saved_seed,saved_mineral_total,saved_gas_total;
            static int saved_supply_used,saved_supply_cap;
            static u16 saved_weapons[MAX_UNITS];
            unsigned cached_detection_masks=0;
            unsigned saved_clock;

            saved_p7=race_p7;
            memcpy(saved_units,u,sizeof(u));
            memcpy(saved_buildings,b,sizeof(b));
            memcpy(saved_players,race_player,sizeof(saved_players));
            memcpy(saved_minerals,race_minerals,sizeof(saved_minerals));
            memcpy(saved_gas,race_gas,sizeof(saved_gas));
            saved_frame=frame; saved_seed=(int)seed;
            saved_mineral_total=minerals; saved_gas_total=gas;
            saved_supply_used=supply_used; saved_supply_cap=supply_cap;
            for(int i=0;i<MAX_UNITS;++i) {
                saved_weapons[i]=(u16)u[i].weapon;
                if(u[i].alive && SC64_P7_ARBITER_CACHE_VALID(u[i].weapon) &&
                   SC64_P7_ARBITER_DETECTED_MASK(u[i].weapon))
                    ++cached_detection_masks;
            }
            saved_clock=(unsigned)race_p7.reserved[1] |
                        ((unsigned)race_p7.reserved[2]<<8);
            assert(sc64_p7_arbiter_cache_initialized);

            /* Simulate a process restart while host SRAM remains intact. */
            game_init();
            memset(&race_p7,0,sizeof(race_p7));
            memset(&race_p7_world,0,sizeof(race_p7_world));
            memset(&race_p7_ops,0,sizeof(race_p7_ops));
            race_p7_map=0;
            sc64_p7_arbiter_cache_due=0;
            sc64_p7_arbiter_cache_initialized=0;
            memset(&race_data,0,sizeof(race_data));
            target_lists_valid=0;
            sc64_target_index_initialized=0;
            sc64_target_index_active=0;
            sc64_target_dirty_count=0;
            sc64_target_dirty_overflowed=0;
            memset(sc64_target_head,0xff,sizeof(sc64_target_head));
            memset(sc64_target_next,0xff,sizeof(sc64_target_next));
            memset(sc64_target_prev,0xff,sizeof(sc64_target_prev));
            memset(sc64_target_cell,0xff,sizeof(sc64_target_cell));
            memset(sc64_target_dirty,0,sizeof(sc64_target_dirty));
#if defined(SC64_LATE_TERRAN_STORAGE) || defined(SC64_TARGET_OCCUPANCY)
            memset(sc64_target_owner_columns,0,sizeof(sc64_target_owner_columns));
            memset(sc64_target_owner_rows,0,sizeof(sc64_target_owner_rows));
#endif
            assert(screen==SCREEN_TITLE && !race_p7_map &&
                   !sc64_p7_arbiter_cache_initialized);
            game_update(KEY_L,KEY_L); /* ordinary title-menu load path */
            assert(screen==SCREEN_PLAY && race_p7_active() &&
                   race_p7_runtime_state_valid());
            assert(race_p7_map==&sc64_race_maps[SC64_P7_GLOBAL_ID]);
            assert(sc64_p7_arbiter_cache_initialized &&
                   !sc64_p7_arbiter_cache_due);
            assert(!memcmp(&race_p7,&saved_p7,sizeof(race_p7)));
            assert(saved_clock==((unsigned)race_p7.reserved[1] |
                                 ((unsigned)race_p7.reserved[2]<<8)));
            assert(!memcmp(u,saved_units,sizeof(u)));
            assert(!memcmp(b,saved_buildings,sizeof(b)));
            assert(!memcmp(race_player,saved_players,sizeof(saved_players)));
            assert(!memcmp(race_minerals,saved_minerals,sizeof(saved_minerals)));
            assert(!memcmp(race_gas,saved_gas,sizeof(saved_gas)));
            for(int i=0;i<MAX_UNITS;++i)
                assert((u16)u[i].weapon==saved_weapons[i]);
            assert(frame==saved_frame && seed==(u32)saved_seed &&
                   minerals==saved_mineral_total && gas==saved_gas_total &&
                   supply_used==saved_supply_used && supply_cap==saved_supply_cap);
            assert(cached_detection_masks>0);
            printf("P7 cold SRAM reload clock=%u phase=%u target_masks=%u\n",
                   saved_clock,(unsigned)saved_p7.reserved[0],cached_detection_masks);

            /* One ordinary tick follows the saved phase, without a forced
               launch refresh; if source cadence is due, it is a natural tick. */
            unsigned old_phase=race_p7.reserved[0];
            unsigned old_clock=(unsigned)race_p7.reserved[1] |
                               ((unsigned)race_p7.reserved[2]<<8);
            unsigned phase=old_phase+4u;
            game_update(0,0);
            assert(screen==SCREEN_PLAY && sc64_p7_arbiter_cache_initialized);
            if(phase<5u) {
                assert(race_p7.reserved[0]==phase);
                assert(((unsigned)race_p7.reserved[1] |
                        ((unsigned)race_p7.reserved[2]<<8))==old_clock);
                for(int i=0;i<MAX_UNITS;++i)
                    assert((u16)u[i].weapon==saved_weapons[i]);
            } else {
                unsigned expected=old_clock ? old_clock-1u : 300u;
                assert(race_p7.reserved[0]==phase-5u);
                assert(((unsigned)race_p7.reserved[1] |
                        ((unsigned)race_p7.reserved[2]<<8))==expected);
            }
        }
#endif
    }
    for (int i=0;i<600;++i) game_update(0,0);
    /* Exercise the authenticated switch-gated action row and its CreateUnit
       effects over real subsequent engine ticks. */
    {
        unsigned before=live_units();
        race_p7.runtime.switch_bits|=SC64_P7_SWITCH_MASK;
        for (int i=0;i<1200 && screen==SCREEN_PLAY && race_p7.created_mask!=7u;++i)
            game_update(0,0);
        assert(race_p7.created_mask==7u);
        assert(live_units()>before);
    }
    assert(screen==SCREEN_PLAY && race_p7_runtime_state_valid());
    unsigned conditions_before=live_condition_vector();
    assert(race_p7.runtime.model.started & (1u<<4));
    /* Host-driven legitimate world change: remove the human force and let the
       source's zero-unit condition and its defeat action dispatch. */
    for (int i=0;i<MAX_UNITS;++i)
        if (u[i].alive && u[i].raw_owner==7) u[i].alive=0;
    for (int i=0;i<MAX_BUILDINGS;++i)
        if (b[i].alive && b[i].raw_owner==7) b[i].alive=0;
    for (int i=0;i<12;++i) game_update(0,0);
    { u32 remaining=999; assert(sc64_p7_world_count(&race_p7_world,13,229,0,&remaining)); assert(remaining==0); }
    unsigned conditions_after=live_condition_vector();
    assert((conditions_before&1u)==0 && (conditions_after&1u)!=0);
    assert(race_p7.runtime.model.started & 1u);
    assert(race_p7_runtime_state_valid());
    printf("P7 trace %08x conditions=%03x->%03x started=%03x outcome=%u cursor=%u,%u,%u,%u wallets=%u/%u,%u/%u\n",
        trace_hash(),conditions_before,conditions_after,race_p7.runtime.model.started,race_p7.runtime.model.outcome,
        race_p7.runtime.model.action_cursor[0],race_p7.runtime.model.action_cursor[1],
        race_p7.runtime.model.action_cursor[2],race_p7.runtime.model.action_cursor[3],
        race_p7.source_minerals[0],race_p7.source_gas[0],
        race_p7.source_minerals[4],race_p7.source_gas[4]);
    /* Reload the genuine saved opening and drive each source hero-absence
       predicate through real combat death hooks, then compare dispatch state. */
    game_init();
    tap(KEY_A); tap(KEY_RIGHT); tap(KEY_RIGHT); tap(KEY_RIGHT);
    tap(KEY_A); game_update(KEY_A,KEY_A);
    assert(screen==SCREEN_PLAY && race_p7_runtime_state_valid());
    for (unsigned t=1;t<=3;++t) {
        kill_source_condition(t);
        for (int i=0;i<12;++i) game_update(0,0);
        assert(race_p7.runtime.model.started & (1u<<t));
    }
    for (unsigned t=8;t<=9;++t) {
        const Sc64P7SourceCondition *c;
        assert(sc64_p7_condition_row(t,0,&c));
        for (int i=0;i<MAX_BUILDINGS;++i) if (b[i].alive &&
            b[i].raw_type==(int)c->unit &&
            sc64_p7_world_owner_matches(&race_p7_world,c->player,
                                        b[i].raw_owner,b[i].owner)==1)
            b[i].alive=0;
        for (int i=0;i<12;++i) game_update(0,0);
        assert(race_p7.runtime.model.started & (1u<<t));
    }
    printf("P7 hero-condition trace %08x cond=%03x started=%03x outcome=%u\n",
        trace_hash(),live_condition_vector(),race_p7.runtime.model.started,
        race_p7.runtime.model.outcome);
    {
        Sc64RaceMap corrupt=sc64_race_maps[SC64_P7_GLOBAL_ID];
        --corrupt.width;
        assert(!race_p7_runtime_init(&corrupt)); /* init fails closed on bad descriptor */
    }
    puts("P7 menu entry/save-load UI path passed");
    return 0;
}
