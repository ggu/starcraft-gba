/* SPDX-License-Identifier: GPL-3.0-or-later
 * Scratch-only migration probe. The non-experimental executable creates a
 * real predecessor journal; a fresh experimental process checks CRC, expands
 * its old layout and runs the existing complete game preflight. It does not
 * publish a migrated record or apply the candidate to live actors. */
#include "../common/gba.c"
#include "../starcraft/game.c"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#if defined(SC64_SHARED_CARRIER_EXPERIMENT) && !defined(SC64_P7_PROJECTILES_ENABLED)
static int check_legacy_candidate(const void *data,u32 length,u32 tag,void *context)
{
    RaceLoadPlan *plan=context;
    size_t expanded=0;
#ifdef SC64_IMPACT_EVENTS
    if (data!=(const void *)&race_impact_record) return 0;
    if (!sc64_shared_carrier_expand_legacy(&race_impact_record,length,
                                            sizeof(race_impact_record),&expanded)) {
        fprintf(stderr,"legacy expand failed length=%u capacity=%zu\n",length,sizeof(race_impact_record)); return 0;
    }
    memset(plan,0,sizeof(*plan));
    if (!race_impact_save_decode_selected((u32)expanded,tag,plan->impact_candidate,
                                           &plan->impact_candidate_count)) {
        fprintf(stderr,"legacy impact decode failed length=%u expanded=%zu tag=%08x\n",length,expanded,tag); return 0;
    }
#else
    if (data!=(const void *)&race_data || tag!=SAVE_VERSION) return 0;
    if (!sc64_shared_carrier_expand_legacy(&race_data,length,sizeof(race_data),&expanded)) return 0;
    if (expanded!=sizeof(race_data)) return 0;
    memset(plan,0,sizeof(*plan));
#endif
    if (!race_load_preflight(plan)) { fprintf(stderr,"legacy game preflight failed length=%u expanded=%zu tag=%08x\n",length,expanded,tag); return 0; }
    return 1;
}

/* This intentionally validates only the staged actor/shot payload. The
 * product's complete metadata, sidecars and non-failing live commit are not
 * wired to this probe, so it is not a game-save implementation. */
static int check_probe_candidate(const void *data,u32 length,u32 tag,void *context)
{
    if (tag==SAVE_VERSION) return check_legacy_candidate(data,length,tag,context);
    if (tag!=SC64_SHARED_CARRIER_SAVE_VERSION || length!=sizeof(RaceSave) ||
        ((const RaceSave *)data)->version!=tag) return 0;
#ifdef SC64_ZP_CAMPAIGN_STORAGE
    Unit *scratch=sc64_zp_route_save_scratch.carrier_candidate.units;
#else
    Unit *scratch=sc64_combined_route_save_scratch.carrier_candidate.units;
#endif
    return sc64_shared_carrier_candidate_valid(data,scratch,MAX_UNITS,
                                                (unsigned)race_cols*16u,
                                                (unsigned)race_rows*16u);
}

static void test_staged_journal_migration(const GbaSaveSelection *previous)
{
    static u8 predecessor[SAVE_SLOT_SIZE];
    static Unit before_units[MAX_UNITS];
    static Building before_buildings[MAX_BUILDINGS];
    static u8 before_vram[GBA_VRAM_SCRATCH_BYTES];
    static RaceLoadPlan plan;
    const u32 tags[]={SAVE_VERSION,SC64_SHARED_CARRIER_SAVE_VERSION};
    GbaSaveSelection selected;
    assert(previous->slot==0 || previous->slot==1);
    memcpy(predecessor,host_save+(unsigned)previous->slot*SAVE_SLOT_SIZE,sizeof(predecessor));

    game_init(); race_mode=1; race_human=R_PROTOSS; race_bots=1;
    race_skirmish_map=0; race_skirmish_init(); screen=SCREEN_PLAY;
    int carrier=spawn_unit(SIDE,SC64_CARRIER_TYPE,600,600);
    int target=spawn_unit(ENEMY,U_ZERGLING,660,600);
    assert(carrier>=0 && target>=0 && race_p7_carrier_training_stock(carrier,1)==1);
    for (int i=0;i<240 && sc64_pool_active_head(&race_p7_projectiles)==SC64_END;++i)
        game_update(0,0);
    assert(sc64_pool_active_head(&race_p7_projectiles)!=SC64_END);
    memset(&race_data,0,sizeof(race_data));
    assert(sc64_shared_carrier_candidate_encode(&race_data,u,b,&race_p7_projectiles,
                                                (unsigned)race_cols*16u,
                                                (unsigned)race_rows*16u));
    race_data.version=SC64_SHARED_CARRIER_SAVE_VERSION;
    assert(save_write_selected(&race_data,sizeof(race_data),SC64_SHARED_CARRIER_SAVE_VERSION,
                               previous,tags,2));
    assert(!memcmp(predecessor,host_save+(unsigned)previous->slot*SAVE_SLOT_SIZE,
                   sizeof(predecessor)));
    memcpy(before_units,u,sizeof(u)); memcpy(before_buildings,b,sizeof(b));
    memcpy(before_vram,GBA_VRAM_SCRATCH,sizeof(before_vram));
    int old_frame=frame, old_minerals=minerals, old_gas=gas, old_mode=race_mode;
    u32 old_seed=seed;
    u8 old_clock[3]; memcpy(old_clock,race_protoss_generic_clock,sizeof(old_clock));
#ifdef SC64_IMPACT_EVENTS
    assert(save_select_validated(&race_impact_record,sizeof(race_impact_record),tags,2,
                                 check_probe_candidate,&plan,&selected));
#else
    assert(save_select_validated(&race_data,sizeof(race_data),tags,2,
                                 check_probe_candidate,&plan,&selected));
#endif
    assert(selected.tag==SC64_SHARED_CARRIER_SAVE_VERSION && selected.slot!=previous->slot);
    /* A checksummed but semantically broken newer candidate must not mask the
     * real predecessor. Keep the test candidate independent of live actors. */
    memset(&race_data,0,sizeof(race_data));
    assert(sc64_shared_carrier_candidate_encode(&race_data,u,b,&race_p7_projectiles,
                                                (unsigned)race_cols*16u,
                                                (unsigned)race_rows*16u));
    int linked=-1;
    for (int i=0;i<MAX_UNITS;++i)
        if (u[i].alive && u[i].type==SC64_INTERCEPTOR_TYPE &&
            sc64_p7_carrier_child_state(&u[i])==SC64_P7_INTERCEPTOR_OUTSIDE)
            { linked=i; break; }
    assert(linked>=0);
    Unit invalid=u[linked]; invalid.build=-1;
    assert(sc64_p7_typed_unit_encode(&race_data.carrier_custom.units[linked],&invalid,
                                     (unsigned)race_cols*16u,(unsigned)race_rows*16u));
    race_data.version=SC64_SHARED_CARRIER_SAVE_VERSION;
    assert(save_write_selected(&race_data,sizeof(race_data),SC64_SHARED_CARRIER_SAVE_VERSION,
                               previous,tags,2));
#ifdef SC64_IMPACT_EVENTS
    assert(save_select_validated(&race_impact_record,sizeof(race_impact_record),tags,2,
                                 check_probe_candidate,&plan,&selected));
#else
    assert(save_select_validated(&race_data,sizeof(race_data),tags,2,
                                 check_probe_candidate,&plan,&selected));
#endif
    assert(selected.tag==SAVE_VERSION && selected.slot==previous->slot);
    memset(&race_data,0,sizeof(race_data));
    assert(sc64_shared_carrier_candidate_encode(&race_data,u,b,&race_p7_projectiles,
                                                (unsigned)race_cols*16u,
                                                (unsigned)race_rows*16u));
    race_data.version=SC64_SHARED_CARRIER_SAVE_VERSION;
    assert(save_write_selected(&race_data,sizeof(race_data),SC64_SHARED_CARRIER_SAVE_VERSION,
                               previous,tags,2));
    save_test_corrupt_latest(sizeof(RaceSave),SC64_SHARED_CARRIER_SAVE_VERSION);
#ifdef SC64_IMPACT_EVENTS
    assert(save_select_validated(&race_impact_record,sizeof(race_impact_record),tags,2,
                                 check_probe_candidate,&plan,&selected));
#else
    assert(save_select_validated(&race_data,sizeof(race_data),tags,2,
                                 check_probe_candidate,&plan,&selected));
#endif
    assert(selected.tag==SAVE_VERSION && selected.slot==previous->slot);
    assert(!memcmp(predecessor,host_save+(unsigned)previous->slot*SAVE_SLOT_SIZE,
                   sizeof(predecessor)));
    assert(!memcmp(before_units,u,sizeof(u)) && !memcmp(before_buildings,b,sizeof(b)) &&
           !memcmp(before_vram,GBA_VRAM_SCRATCH,sizeof(before_vram)));
    assert(frame==old_frame && minerals==old_minerals && gas==old_gas &&
           race_mode==old_mode && seed==old_seed &&
           !memcmp(old_clock,race_protoss_generic_clock,sizeof(old_clock)));
}
#endif

int main(int argc,char **argv)
{
    assert(argc==3 && (!strcmp(argv[2],"custom") || !strcmp(argv[2],"campaign")));
    int campaign=!strcmp(argv[2],"campaign");
    FILE *f;
#if defined(SC64_SHARED_CARRIER_EXPERIMENT) && !defined(SC64_P7_PROJECTILES_ENABLED)
    f=fopen(argv[1],"rb"); assert(f);
    assert(fread(host_save,1,sizeof(host_save),f)==sizeof(host_save)); assert(!fclose(f));
    static u8 before_sram[sizeof(host_save)];
    static u8 before_vram[GBA_VRAM_SCRATCH_BYTES];
    static Unit before_units[MAX_UNITS];
    static Building before_buildings[MAX_BUILDINGS];
    static RaceLoadPlan plan;
    GbaSaveSelection selected;
    const u32 tags[]={SAVE_VERSION};
    memcpy(before_sram,host_save,sizeof(host_save));
    game_init();
    memcpy(before_units,u,sizeof(u)); memcpy(before_buildings,b,sizeof(b));
    memcpy(before_vram,GBA_VRAM_SCRATCH,sizeof(before_vram));
    int old_frame=frame, old_minerals=minerals, old_gas=gas, old_mode=race_mode;
    int old_human=race_human, old_bots=race_bots, old_screen=screen;
    u32 old_seed=seed;
    u8 old_clock[3]; memcpy(old_clock,race_protoss_generic_clock,sizeof(old_clock));
#ifdef SC64_IMPACT_EVENTS
    assert(save_select_validated(&race_impact_record,sizeof(race_impact_record),tags,1,
                                 check_legacy_candidate,&plan,&selected));
    assert(check_legacy_candidate(&race_impact_record,selected.length,selected.tag,&plan));
#else
    assert(save_select_validated(&race_data,sizeof(race_data),tags,1,
                                 check_legacy_candidate,&plan,&selected));
    assert(check_legacy_candidate(&race_data,selected.length,selected.tag,&plan));
#endif
    assert(race_data.state[0]==!campaign);
    if (!campaign) assert(race_data.state[1]==R_PROTOSS);
    assert(!memcmp(before_sram,host_save,sizeof(host_save)));
    assert(!memcmp(before_units,u,sizeof(u)) && !memcmp(before_buildings,b,sizeof(b)));
    assert(!memcmp(before_vram,GBA_VRAM_SCRATCH,sizeof(before_vram)));
    assert(frame==old_frame && minerals==old_minerals && gas==old_gas && race_mode==old_mode &&
           race_human==old_human && race_bots==old_bots && screen==old_screen && seed==old_seed);
    assert(!memcmp(old_clock,race_protoss_generic_clock,sizeof(old_clock)));
    puts("legacy journal preflight: ok");
    test_staged_journal_migration(&selected);
    puts("staged actor/shot journal migration: ok");
#else
    game_init();
    if (campaign) {
        race_mode=0;
        race_selected_mission=sc64_frontend_first_mission(SC64_CAMPAIGN_RACE);
        init_state();
        assert(screen==SCREEN_PLAY);
    } else {
        race_mode=1; race_human=R_PROTOSS; race_bots=1;
        race_skirmish_map=0; race_skirmish_init(); screen=SCREEN_PLAY;
    }
    assert(race_save_game());
    f=fopen(argv[1],"wb"); assert(f);
    assert(fwrite(host_save,1,sizeof(host_save),f)==sizeof(host_save)); assert(!fclose(f));
    puts("legacy journal written");
#endif
    return 0;
}
