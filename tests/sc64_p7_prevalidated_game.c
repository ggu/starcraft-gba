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
    }
    for (int i=0;i<600;++i) game_update(0,0);
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
