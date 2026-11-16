/* SPDX-License-Identifier: GPL-3.0-or-later */
#define main previous_world_fixture
#include "sc64_ghost_tech_world_game.c"
#undef main

static GbaSaveSelection select_tag(RaceLoadPlan *plan, u32 tag)
{
    GbaSaveSelection selected;
    assert(race_worker_cargo_select(plan,&selected) && selected.tag==tag);
    return selected;
}

static int ghost_at(void)
{
    for (int i=0;i<MAX_UNITS;++i)
        if (u[i].alive && u[i].owner==SIDE && sc64_race_ordinary_ghost(u[i].type)) return i;
    assert(0); return -1;
}

static void paint(int silo, int spotter)
{
    assert(race_nuke_arm(silo,SIDE)==4);
    while (race_nuke_silos[silo].ammo.remaining) { ++frame; race_nuke_tick(); }
    assert(race_nuke_armed_slot(SIDE)==silo && race_nuke_issue(spotter,SIDE,420,408));
    assert(race_nuke_track(spotter)->strike.phase==SC64_NUKE_PAINTING);
}

static void write_fixture(const char *file)
{
    race_mode=1; race_human=R_TER; race_bots=1; race_bot_races[0]=R_TER;
    race_skirmish_map=0; init_state();
    int parent=facility(SIDE,312,296), job=covert(SIDE,312,296), spotter=ghost(SIDE,408,408);
    static u8 prior[SAVE_SLOT_SIZE], snapshot[sizeof host_save];
    assert(spawn_build(SIDE,sc64_race_building_from_raw(106),624,720,0)>=0);
    assert(spawn_build(SIDE,sc64_race_building_from_raw(109),576,720,0)>=0);
    int silo=spawn_build(SIDE,sc64_race_building_from_raw(108),672,728,0);
    assert(silo>=0);
    minerals=gas=1500;
    paint(silo,spotter);
    assert(race_save_game());
    RaceLoadPlan plan;
    GbaSaveSelection old=select_tag(&plan,SC64_NUKE_SILO_TRANSPORT_TAG);
    memcpy(prior,host_save+old.slot*SAVE_SLOT_SIZE,sizeof prior);
    assert(race_nuke_abort(spotter));
    choose(job,SC64_GHOST_TECH_REACTOR,"MOEBIUS REACTOR");
    source_ticks(sc64_ghost_tech_fact(SC64_GHOST_TECH_REACTOR)->source_ticks);
    u[spotter].carry=250*256;
    paint(silo,spotter);
    choose(job,SC64_GHOST_TECH_OCULAR,"OCULAR IMPLANTS");
    assert(race_save_game());
    GbaSaveSelection selected=select_tag(&plan,SC64_GHOST_COVERT_TAG);
    assert(selected.length==old.length && selected.slot!=old.slot &&
           !memcmp(prior,host_save+old.slot*SAVE_SLOT_SIZE,sizeof prior));
    assert(plan.ghost_covert.projected.count==1);
    memcpy(snapshot,host_save,sizeof snapshot);
    b[parent].flags|=4;
    assert(!race_save_game() && !memcmp(snapshot,host_save,sizeof snapshot));
    FILE *out=fopen(file,"wb");
    assert(out && fwrite(snapshot,1,sizeof snapshot,out)==sizeof snapshot && !fclose(out));
    printf("Custom Terran journal: old nuclear-only bank intact; Covert+strike payload %u bytes\n",
           selected.length);
}

static void read_fixture(const char *file, int reject)
{
    static u8 original[sizeof host_save];
    FILE *in=fopen(file,"rb");
    assert(in && fread(host_save,1,sizeof host_save,in)==sizeof host_save && fgetc(in)==EOF && !fclose(in));
    RaceLoadPlan plan;
    GbaSaveSelection current=select_tag(&plan,SC64_GHOST_COVERT_TAG);
    const unsigned slot=plan.ghost_covert.projected.receipts[0].building;
    if (!reject) {
        memcpy(original,host_save,sizeof original);
        assert(race_load_game() && !memcmp(original,host_save,sizeof original));
        read_energy(ghost_at(),250*256,250*256);
        assert(race_nuke_track(ghost_at())->strike.phase==SC64_NUKE_PAINTING);
        Sc64GhostCovertJob *job=race_ghost_job_at((int)slot);
        assert(job && job->paid.kind==SC64_GHOST_TECH_OCULAR &&
               job->paid.remaining==plan.ghost_covert.projected.receipts[0].paid.remaining);
        assert(race_save_game() && select_tag(&plan,SC64_GHOST_COVERT_TAG).length==current.length);
        puts("Fresh Covert journal restored a paid job, earned Q8 cap and active painting");
        return;
    }
    /* Change only a receipt timer and recompute the outer checksum. The
     * old nuclear-only bank must still admit and remain byte-identical. */
    Building row;
    assert(race_save_decode_building(&race_data,slot,&row));
    row.queue_progress=sc64_ghost_tech_fact(SC64_GHOST_TECH_OCULAR)->source_ticks+1;
    row.level=1;
    Sc64NukeSaveWorld world=race_nuke_save_world(1,1,2048,2048);
    assert(race_nuke_save_replace_building(&race_data,slot,&row,&world));
    u8 *record=host_save+current.slot*SAVE_SLOT_SIZE;
    memcpy(record+16+offsetof(RaceSave,buildings)+slot*sizeof race_data.buildings[0],
           &race_data.buildings[slot],sizeof race_data.buildings[0]);
    sram_put32(record+8,hash_bytes(record+16,current.length));
    assert(valid_record(record,current.length,current.tag));
    memcpy(original,host_save,sizeof original);
    assert(race_load_game() && !memcmp(original,host_save,sizeof original));
    assert(!sc64_ghost_tech.completed && !race_ghost_job_at((int)slot));
    read_energy(ghost_at(),50*256,200*256);
    assert(race_nuke_track(ghost_at())->strike.phase==SC64_NUKE_PAINTING);
    memset(host_save+(1-current.slot)*SAVE_SLOT_SIZE,0xff,SAVE_SLOT_SIZE);
    memcpy(original,host_save,sizeof original);
    int prior_frame=frame;
    assert(!race_load_game() && !memcmp(original,host_save,sizeof original) &&
           frame==prior_frame && race_nuke_track(ghost_at())->strike.phase==SC64_NUKE_PAINTING);
    puts("Invalid newer paid receipt fell back; rejection without a predecessor kept live play and SRAM");
}

int main(int argc,char **argv)
{
    assert(argc==3);
    gba_init(); game_init();
    if (!strcmp(argv[1],"write")) write_fixture(argv[2]);
    else {
        assert(!strcmp(argv[1],"read") || !strcmp(argv[1],"reject"));
        read_fixture(argv[2],!strcmp(argv[1],"reject"));
    }
    return 0;
}
