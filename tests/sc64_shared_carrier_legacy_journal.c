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

static void test_staged_journal_migration(const GbaSaveSelection *previous, int campaign,
                                          const char *custom_output)
{
    static u8 predecessor[SAVE_SLOT_SIZE];
    static Unit before_units[MAX_UNITS];
    static Building before_buildings[MAX_BUILDINGS];
    static u8 before_vram[GBA_VRAM_SCRATCH_BYTES];
    static RaceLoadPlan plan;
    const u32 tags[]={SAVE_VERSION,SC64_SHARED_CARRIER_SAVE_VERSION,SC64_SHARED_CARRIER_COMPACT_SAVE_VERSION};
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
#ifdef SC64_IMPACT_EVENTS
    race_impact_reset();
    race_impact_launch(target,3,1,SIDE,0,SC64_CARRIER_TYPE,10);
    assert(race_impact_count==1);
    Sc64ImpactEvent expected_impact=race_impact_rows[0];
#endif
#ifdef SC64_SHARED_CARRIER_SAVE_EXPERIMENT
    /* Drive the actual gated game entry points with an existing predecessor. */
    Unit saved_carrier=u[carrier], saved_target=u[target];
    int entry_minerals=minerals, entry_frame=frame;
    u8 entry_clock[3]; memcpy(entry_clock,race_protoss_generic_clock,sizeof(entry_clock));
    static u8 entry_packet[SC64_CODEC_BYTES], entry_restored[SC64_CODEC_BYTES];
    static u8 journal_before_rejection[sizeof(host_save)];
    int invalid_child=-1;
    for (int i=0;i<MAX_UNITS;++i)
        if (u[i].alive && u[i].type==SC64_INTERCEPTOR_TYPE &&
            sc64_p7_carrier_child_state(&u[i])==SC64_P7_INTERCEPTOR_OUTSIDE)
            { invalid_child=i; break; }
    assert(invalid_child>=0);
    memcpy(journal_before_rejection,host_save,sizeof(host_save));
    int saved_parent=u[invalid_child].build; u[invalid_child].build=-1;
    assert(!race_save_game());
    assert(!memcmp(journal_before_rejection,host_save,sizeof(host_save)));
    u[invalid_child].build=saved_parent;
    assert(sc64_codec_encode(&race_p7_projectiles,entry_packet,sizeof(entry_packet)));
    assert(race_save_game());
    if (!campaign && custom_output) {
        FILE *output=fopen(custom_output,"wb"); assert(output);
        assert(fwrite(host_save,1,sizeof(host_save),output)==sizeof(host_save)); assert(!fclose(output));
    }
    assert(!memcmp(predecessor,host_save+(unsigned)previous->slot*SAVE_SLOT_SIZE,
                   sizeof(predecessor)));
    u[carrier].hp=1; minerals=0; frame=0;
    memset(race_protoss_generic_clock,0,sizeof(entry_clock));
    assert(sc64_pool_init(&race_p7_projectiles));
#ifdef SC64_IMPACT_EVENTS
    race_impact_reset();
#endif
    assert(race_load_game());
    assert(!memcmp(&saved_carrier,&u[carrier],sizeof(Unit)) &&
           !memcmp(&saved_target,&u[target],sizeof(Unit)) &&
           minerals==entry_minerals && frame==entry_frame &&
           !memcmp(entry_clock,race_protoss_generic_clock,sizeof(entry_clock)));
    assert(sc64_codec_encode(&race_p7_projectiles,entry_restored,sizeof(entry_restored)) &&
           !memcmp(entry_packet,entry_restored,sizeof(entry_packet)));
#ifdef SC64_IMPACT_EVENTS
    assert(race_impact_count==1 && !memcmp(&race_impact_rows[0],&expected_impact,sizeof(expected_impact)));
#endif
#endif
    race_p7_projectile_fault=1;
    assert(!race_save_serialize(1));
    race_p7_projectile_fault=0;
    assert(race_save_serialize(1));
    assert(race_data.version==SC64_SHARED_CARRIER_COMPACT_SAVE_VERSION);
    race_data.version=SC64_SHARED_CARRIER_SAVE_VERSION;
    assert(race_data.state[0]==race_mode && race_data.state[1]==race_human &&
           race_data.state[5]==minerals && race_data.state[6]==gas &&
           race_data.state[7]==frame && (u32)race_data.state[15]==seed &&
           race_data.state[SC64_PROTOSS_CLOCK_STATE_INDEX]==
               race_protoss_clock_word(race_protoss_generic_clock));
    memcpy(before_units,u,sizeof(u)); memcpy(before_buildings,b,sizeof(b));
    memcpy(before_vram,GBA_VRAM_SCRATCH,sizeof(before_vram));
#ifdef SC64_IMPACT_EVENTS
    u32 packed_length=0;
    assert(race_impact_save_pack(&packed_length));
    assert(save_write_selected(&race_impact_record,packed_length,SC64_SHARED_CARRIER_SAVE_VERSION,
                               previous,tags,3));
#else
    assert(save_write_selected(&race_data,sizeof(race_data),SC64_SHARED_CARRIER_SAVE_VERSION,
                               previous,tags,3));
#endif
    assert(race_shared_carrier_stage_select(&plan,&selected));
    assert(selected.tag==SC64_SHARED_CARRIER_SAVE_VERSION && selected.slot!=previous->slot);
#ifdef SC64_IMPACT_EVENTS
    assert(plan.impact_candidate_count==1 &&
           !memcmp(&plan.impact_candidate[0],&expected_impact,sizeof(expected_impact)));
#endif
    assert(!memcmp(before_units,u,sizeof(u)) && !memcmp(before_buildings,b,sizeof(b)) &&
           !memcmp(before_vram,GBA_VRAM_SCRATCH,sizeof(before_vram)));
    assert(!memcmp(predecessor,host_save+(unsigned)previous->slot*SAVE_SLOT_SIZE,
                   sizeof(predecessor)));
    /* A checksummed full envelope with a broken child link must fall back to
     * the real predecessor after all sidecars have been decoded. */
    int full_linked=-1;
    for (int i=0;i<MAX_UNITS;++i)
        if (u[i].alive && u[i].type==SC64_INTERCEPTOR_TYPE &&
            sc64_p7_carrier_child_state(&u[i])==SC64_P7_INTERCEPTOR_OUTSIDE)
            { full_linked=i; break; }
    assert(full_linked>=0);
    Unit full_invalid=u[full_linked]; full_invalid.build=-1;
    assert(sc64_p7_typed_unit_encode(&race_data.carrier_custom.units[full_linked],
                                     &full_invalid,(unsigned)race_cols*16u,(unsigned)race_rows*16u));
#ifdef SC64_IMPACT_EVENTS
    assert(race_impact_save_pack(&packed_length));
    assert(save_write_selected(&race_impact_record,packed_length,SC64_SHARED_CARRIER_SAVE_VERSION,
                               previous,tags,3));
#else
    assert(save_write_selected(&race_data,sizeof(race_data),SC64_SHARED_CARRIER_SAVE_VERSION,
                               previous,tags,3));
#endif
    assert(race_shared_carrier_stage_select(&plan,&selected));
    assert(selected.tag==SAVE_VERSION && selected.slot==previous->slot);
    assert(!memcmp(predecessor,host_save+(unsigned)previous->slot*SAVE_SLOT_SIZE,
                   sizeof(predecessor)));
    assert(!memcmp(before_units,u,sizeof(u)) && !memcmp(before_buildings,b,sizeof(b)) &&
           !memcmp(before_vram,GBA_VRAM_SCRATCH,sizeof(before_vram)));
    assert(race_save_serialize(1));
    race_data.version=SC64_SHARED_CARRIER_SAVE_VERSION;
#ifdef SC64_IMPACT_EVENTS
    assert(race_impact_save_pack(&packed_length));
    assert(save_write_selected(&race_impact_record,packed_length,SC64_SHARED_CARRIER_SAVE_VERSION,
                               previous,tags,3));
#else
    assert(save_write_selected(&race_data,sizeof(race_data),SC64_SHARED_CARRIER_SAVE_VERSION,
                               previous,tags,3));
#endif
    assert(race_shared_carrier_stage_select(&plan,&selected));
    assert(selected.tag==SC64_SHARED_CARRIER_SAVE_VERSION);
    /* Exercise the detached commit separately from the still-disabled game
     * loader. Route scratch is consumed only after all saved fields copy. */
    static u8 expected_packet[SC64_CODEC_BYTES], restored_packet[SC64_CODEC_BYTES];
    memcpy(expected_packet,race_data.carrier_custom.projectiles,sizeof(expected_packet));
    int saved_minerals=minerals, saved_frame=frame;
    u8 saved_clock[3]; memcpy(saved_clock,race_protoss_generic_clock,sizeof(saved_clock));
    u[carrier].hp=1; minerals=0; frame=0;
    memset(race_protoss_generic_clock,0,sizeof(saved_clock));
    assert(sc64_pool_init(&race_p7_projectiles));
    race_p7_projectile_fault=1;
#ifdef SC64_IMPACT_EVENTS
    race_impact_reset();
#endif
    race_shared_carrier_stage_commit(&plan);
    assert(!memcmp(&u[carrier],&before_units[carrier],sizeof(Unit)) &&
           !memcmp(&u[target],&before_units[target],sizeof(Unit)) &&
           !memcmp(b,before_buildings,sizeof(b)));
    assert(minerals==saved_minerals && frame==saved_frame &&
           !memcmp(saved_clock,race_protoss_generic_clock,sizeof(saved_clock)));
    assert(sc64_codec_encode(&race_p7_projectiles,restored_packet,sizeof(restored_packet)) &&
           !memcmp(expected_packet,restored_packet,sizeof(expected_packet)));
    assert(!race_p7_projectile_fault);
#ifdef SC64_IMPACT_EVENTS
    assert(race_impact_count==1 && !memcmp(&race_impact_rows[0],&expected_impact,sizeof(expected_impact)));
#endif
    static Unit replay_units[MAX_UNITS];
    static Building replay_buildings[MAX_BUILDINGS];
    static u8 replay_packet[SC64_CODEC_BYTES];
    u8 replay_clock[3];
    for (int i=0;i<30;++i) game_update(0,0);
    memcpy(replay_units,u,sizeof(u)); memcpy(replay_buildings,b,sizeof(b));
    memcpy(replay_clock,race_protoss_generic_clock,sizeof(replay_clock));
    int replay_minerals=minerals, replay_gas=gas, replay_frame=frame;
    u32 replay_seed=seed;
    assert(sc64_codec_encode(&race_p7_projectiles,replay_packet,sizeof(replay_packet)));
#ifdef SC64_IMPACT_EVENTS
    static Sc64ImpactEvent replay_impacts[RACE_IMPACT_CAPACITY];
    unsigned replay_impact_count=race_impact_count;
    memcpy(replay_impacts,race_impact_rows,sizeof(replay_impacts));
#endif
    assert(race_shared_carrier_stage_select(&plan,&selected));
    assert(selected.tag==SC64_SHARED_CARRIER_SAVE_VERSION);
    race_shared_carrier_stage_commit(&plan);
    for (int i=0;i<30;++i) game_update(0,0);
    assert(!memcmp(replay_units,u,sizeof(u)) && !memcmp(replay_buildings,b,sizeof(b)) &&
           !memcmp(replay_clock,race_protoss_generic_clock,sizeof(replay_clock)) &&
           minerals==replay_minerals && gas==replay_gas && frame==replay_frame && seed==replay_seed);
    assert(sc64_codec_encode(&race_p7_projectiles,restored_packet,sizeof(restored_packet)) &&
           !memcmp(replay_packet,restored_packet,sizeof(replay_packet)));
#ifdef SC64_IMPACT_EVENTS
    assert(race_impact_count==replay_impact_count &&
           !memcmp(replay_impacts,race_impact_rows,race_impact_count*sizeof(replay_impacts[0])));
#endif
    assert(race_shared_carrier_stage_select(&plan,&selected));
    assert(selected.tag==SC64_SHARED_CARRIER_SAVE_VERSION);
    race_shared_carrier_stage_commit(&plan);
    assert(race_shared_carrier_stage_select(&plan,&selected));
    race_data.state[31]=0x13579;
    assert(race_save_encode_actors());
    assert(race_data.version==SC64_SHARED_CARRIER_SAVE_VERSION && race_data.state[31]==0x13579);
    for (unsigned i=0;i<MAX_UNITS;++i) {
        Unit decoded;
        assert(race_save_decode_unit(&race_data,i,(unsigned)race_cols*16u,
                                     (unsigned)race_rows*16u,&decoded));
        if (u[i].alive) assert(!memcmp(&decoded,&u[i],sizeof(decoded)));
        else assert(!decoded.alive);
    }
    for (unsigned i=0;i<MAX_BUILDINGS;++i) {
        Building decoded;
        assert(race_save_decode_building(&race_data,i,&decoded));
        if (b[i].alive) assert(!memcmp(&decoded,&b[i],sizeof(decoded)));
        else assert(!decoded.alive);
    }
    assert(save_write_selected(&race_data,sizeof(race_data),SC64_SHARED_CARRIER_SAVE_VERSION,
                               previous,tags,3));
    assert(!memcmp(predecessor,host_save+(unsigned)previous->slot*SAVE_SLOT_SIZE,
                   sizeof(predecessor)));
    memcpy(before_units,u,sizeof(u)); memcpy(before_buildings,b,sizeof(b));
    memcpy(before_vram,GBA_VRAM_SCRATCH,sizeof(before_vram));
    int old_frame=frame, old_minerals=minerals, old_gas=gas, old_mode=race_mode;
    u32 old_seed=seed;
    u8 old_clock[3]; memcpy(old_clock,race_protoss_generic_clock,sizeof(old_clock));
#ifdef SC64_IMPACT_EVENTS
    assert(save_select_validated(&race_impact_record,sizeof(race_impact_record),tags,3,
                                 check_probe_candidate,&plan,&selected));
#else
    assert(save_select_validated(&race_data,sizeof(race_data),tags,3,
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
                               previous,tags,3));
#ifdef SC64_IMPACT_EVENTS
    assert(save_select_validated(&race_impact_record,sizeof(race_impact_record),tags,3,
                                 check_probe_candidate,&plan,&selected));
#else
    assert(save_select_validated(&race_data,sizeof(race_data),tags,3,
                                 check_probe_candidate,&plan,&selected));
#endif
    assert(selected.tag==SAVE_VERSION && selected.slot==previous->slot);
    memset(&race_data,0,sizeof(race_data));
    assert(sc64_shared_carrier_candidate_encode(&race_data,u,b,&race_p7_projectiles,
                                                (unsigned)race_cols*16u,
                                                (unsigned)race_rows*16u));
    race_data.version=SC64_SHARED_CARRIER_SAVE_VERSION;
    assert(save_write_selected(&race_data,sizeof(race_data),SC64_SHARED_CARRIER_SAVE_VERSION,
                               previous,tags,3));
    save_test_corrupt_latest(sizeof(RaceSave),SC64_SHARED_CARRIER_SAVE_VERSION);
#ifdef SC64_IMPACT_EVENTS
    assert(save_select_validated(&race_impact_record,sizeof(race_impact_record),tags,3,
                                 check_probe_candidate,&plan,&selected));
#else
    assert(save_select_validated(&race_data,sizeof(race_data),tags,3,
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
#ifdef SC64_SHARED_CARRIER_SAVE_EXPERIMENT
    /* The game entry point must fall back to the intact predecessor, then
     * leave the live world untouched when neither journal is valid. */
    assert(race_load_game());
    assert(race_mode==!campaign && sc64_pool_active_head(&race_p7_projectiles)==SC64_END);
    memcpy(before_units,u,sizeof(u)); memcpy(before_buildings,b,sizeof(b));
    memcpy(before_vram,GBA_VRAM_SCRATCH,sizeof(before_vram));
    old_frame=frame; old_minerals=minerals; old_gas=gas; old_mode=race_mode; old_seed=seed;
    memcpy(old_clock,race_protoss_generic_clock,sizeof(old_clock));
    host_save[(unsigned)previous->slot*SAVE_SLOT_SIZE+8u]^=1u;
    assert(!race_load_game());
    assert(!memcmp(before_units,u,sizeof(u)) && !memcmp(before_buildings,b,sizeof(b)) &&
           !memcmp(before_vram,GBA_VRAM_SCRATCH,sizeof(before_vram)) &&
           frame==old_frame && minerals==old_minerals && gas==old_gas &&
           race_mode==old_mode && seed==old_seed &&
           !memcmp(old_clock,race_protoss_generic_clock,sizeof(old_clock)));
#else
    (void)campaign;
    (void)custom_output;
#endif
}
#endif

int main(int argc,char **argv)
{
    assert((argc==3 || argc==4) && (!strcmp(argv[2],"custom") || !strcmp(argv[2],"campaign") ||
                                    !strcmp(argv[2],"verify_campaign") || !strcmp(argv[2],"verify_custom")));
#ifdef SC64_SHARED_CARRIER_SAVE_EXPERIMENT
    if (!strcmp(argv[2],"verify_custom")) {
        FILE *input=fopen(argv[1],"rb"); assert(input);
        assert(fread(host_save,1,sizeof(host_save),input)==sizeof(host_save)); assert(!fclose(input));
        game_init(); assert(race_load_game() && race_mode);
        int carrier=0,interceptor=0;
        for (int i=0;i<MAX_UNITS;++i) if (u[i].alive) {
            carrier+=u[i].type==SC64_CARRIER_TYPE;
            interceptor+=u[i].type==SC64_INTERCEPTOR_TYPE;
        }
        assert(carrier && interceptor && sc64_pool_active_head(&race_p7_projectiles)!=SC64_END);
#ifdef SC64_IMPACT_EVENTS
        assert(race_impact_count==1);
#endif
        for (int i=0;i<30;++i) game_update(0,0);
        puts("fresh Carrier journal load and continuation: ok");
        return 0;
    }
#endif
#if !defined(SC64_SHARED_CARRIER_EXPERIMENT) || defined(SC64_P7_PROJECTILES_ENABLED)
    if (!strcmp(argv[2],"verify_campaign")) {
        FILE *input=fopen(argv[1],"rb"); assert(input);
        assert(fread(host_save,1,sizeof(host_save),input)==sizeof(host_save)); assert(!fclose(input));
        game_init(); assert(race_load_game() && !race_mode);
        puts("predecessor reader loaded campaign journal: ok");
        return 0;
    }
#endif
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
#ifdef SC64_SHARED_CARRIER_SAVE_EXPERIMENT
    /* Actual entry point loads the predecessor; campaign writes retain the
     * old envelope so a separate unmodified reader can consume them. */
    u8 stale_ordinal, stale_index;
    race_p7_projectile_reset();
    assert(sc64_pool_acquire(&race_p7_projectiles,&stale_ordinal,&stale_index));
    race_protoss_generic_clock[0]=2; race_protoss_generic_clock[1]=7;
    race_p7_projectile_fault=1;
    assert(race_load_game());
    assert(race_mode==!campaign);
    assert(sc64_pool_active_head(&race_p7_projectiles)==SC64_END && !race_p7_projectile_fault);
#ifndef SC64_P7_BATTERY_ENABLED
    assert(!race_protoss_generic_clock[0] && !race_protoss_generic_clock[1] &&
           !race_protoss_generic_clock[2]);
#endif
    if (campaign) {
        static u8 rejected_campaign_sram[sizeof(host_save)];
        int saved_minerals=minerals;
        memcpy(rejected_campaign_sram,host_save,sizeof(host_save));
        minerals=-1;
        assert(!race_save_game());
        assert(!memcmp(rejected_campaign_sram,host_save,sizeof(host_save)));
        minerals=saved_minerals;
        assert(race_save_game());
        if (argc==4) {
            FILE *output=fopen(argv[3],"wb"); assert(output);
            assert(fwrite(host_save,1,sizeof(host_save),output)==sizeof(host_save)); assert(!fclose(output));
        }
        memcpy(host_save,before_sram,sizeof(host_save));
    }
#endif
    test_staged_journal_migration(&selected,campaign,argc==4 && !campaign ? argv[3] : 0);
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
