/* SPDX-License-Identifier: GPL-3.0-or-later */
/* HOST-only release/flight save probe with a genuine old custom Egg record. */
#define main prior_full_fixture_main
#include "sc64_shared_actor_full_journal.c"
#undef main
int main(int argc,char **argv)
{
    assert(argc==2);FILE *f=fopen(argv[1],"rb");assert(f);
    assert(fread(sram,1,sizeof(sram),f)==sizeof(sram));assert(!fclose(f));
    guardian_test_reset();game_init();guardian_test_restore(sram);
    if (!race_load_game()) {
        u32 len=0;RaceLoadPlan diagnostic;
        int read=save_read_latest(&race_impact_record,sizeof(race_impact_record),SC64_SHARED_EGG_TRANSPORT_SAVE_VERSION,&len);
        int prep=read && race_worker_cargo_prepare_selected(&race_impact_record,len,SC64_SHARED_EGG_TRANSPORT_SAVE_VERSION,&diagnostic);
        fprintf(stderr,"old candidate read=%d prepared=%d length=%u\n",read,prep,(unsigned)len);
        if(read && diagnostic.guardian_auth.candidate) {
            const Sc64StageSaveAuth *guard_auth=&diagnostic.guardian_auth;
            fprintf(stderr,"old candidate guardian=%d scarab=%d auth=%u\n",
                race_guardian_candidate_valid(&race_data,1024,1024,guard_auth),
                race_scarab_save_candidate_valid(&race_data,1024,1024,guard_auth),guard_auth->features);
        }
        return 1;
    }
    int p=-1,egg=-1;for(int i=0;i<MAX_UNITS;i++)if(u[i].alive) {
        if(u[i].type==U_REAVER && u[i].x==300)p=i;
        if(u[i].type==SC64_SHARED_EGG_TYPE)egg=i;
    }
    assert(p>=0 && egg>=0);minerals=100;assert(race_scarab_enqueue(p,0));
    assert(race_scarab_production_tick(p,0)==SC64_SCARAB_ALLOCATED);
    Sc64ScarabParent parent;Sc64ScarabCounts counts;
    assert(race_scarab_parent_read(p,0,&parent,&counts));int child=parent.active_slot;
    for(int i=0;i<SC64_SCARAB_BUILD_TICKS;i++)assert(race_scarab_production_tick(p,0)==SC64_SCARAB_PROGRESS);
    assert(race_scarab_production_tick(p,0)==SC64_SCARAB_COMPLETED);
    int target=spawn_unit(ENEMY,U_ZERGLING,u[p].x,u[p].y+60);assert(target>=0);
    race_scarab_probe_parent_attack(p,target);assert(u[child].order==1 && u[child].target==target);
    assert(race_save_game());RaceLoadPlan plan;GbaSaveSelection selected;
    assert(race_worker_cargo_select(&plan,&selected));
    assert(selected.tag==SC64_SHARED_ACTOR_FLIGHT_TRANSPORT_SAVE_VERSION &&
           plan.guardian_auth.inner==SC64_SHARED_ACTOR_FLIGHT_SAVE_VERSION &&
           plan.guardian_auth.features==15u);
    guardian_test_snapshot(before);memset(u,0,sizeof(u));assert(race_load_game());
    int owner;unsigned phase;assert(race_scarab_child_lookup(child,&owner,&phase) && owner==p && phase==SC64_SCARAB_CHILD_OUTSIDE && u[child].order==1);
    assert(race_scarab_flight_control_tick(child,0,0)==SC64_SCARAB_CONTROL_CONTINUE);
    Sc64ScarabChildFields fields;assert(race_scarab_child_fields_read(u[child].carry,u[child].work_timer,&fields));
    assert(fields.life==90 && fields.no_collision==7 && u[child].order==2);
    assert(race_save_game());memset(u,0,sizeof(u));assert(race_load_game());
    assert(race_scarab_child_fields_read(u[child].carry,u[child].work_timer,&fields));
    assert(fields.life==90 && fields.no_collision==7 && u[child].order==2);
    /* Advance the actual autonomous dispatcher to produce a route waypoint;
     * unlike the earlier initialization checks this exercises nonzero packed
     * motion, including both previous source-coordinate waypoint fields. */
    race_scarab_control_dispatch(child);
    assert(u[child].alive && u[child].order==2 &&
           (u[child].path_timer>24 || u[child].path_retry>12));
    Unit motion=u[child];assert(race_save_game());memset(u,0,sizeof(u));assert(race_load_game());
    assert(u[child].x==motion.x && u[child].y==motion.y && u[child].carry==motion.carry &&
           u[child].cd==motion.cd && u[child].weapon==motion.weapon &&
           u[child].path_timer==motion.path_timer && u[child].path_retry==motion.path_retry &&
           u[child].path_x==motion.path_x && u[child].path_y==motion.path_y);
    /* Drive the real FSM to its arrived phase with a controlled arrival signal. */
    assert(race_scarab_flight_control_tick(child,1,0)==SC64_SCARAB_CONTROL_ARRIVED);
    assert(u[child].order==4 && u[child].target==target);
    assert(race_save_game());memset(u,0,sizeof(u));assert(race_load_game());
    assert(u[child].order==4 && u[child].target==target);
    /* A dead slot can be reused before this child dispatches again. */
    int tx=u[child].tx,ty=u[child].ty;
    hit_target(target,10000,U_REAVER,SIDE);assert(!u[target].alive);
    assert(spawn_unit(ENEMY,U_ZERGLING,700,700)==target);
    assert(u[child].target==-1 && u[child].order==4 && u[child].tx==tx && u[child].ty==ty);
    assert(race_save_game());memset(u,0,sizeof(u));assert(race_load_game());
    assert(u[child].target==-1 && u[child].order==4 && u[child].tx==tx && u[child].ty==ty);
    guardian_test_restore(before);assert(race_load_game());
    /* Retagging a flight row as feature7 must not reinterpret the row. */
    guardian_test_retag(selected.slot,SC64_SHARED_ACTOR_FULL_TRANSPORT_SAVE_VERSION,SC64_SHARED_ACTOR_FULL_SAVE_VERSION);
    GbaSaveSelection fallback;assert(race_worker_cargo_select(&plan,&fallback));
    assert(fallback.tag==SC64_SHARED_EGG_TRANSPORT_SAVE_VERSION && fallback.slot!=selected.slot);
    /* Independently launch from paid stock at a building and save after the
     * same real FSM receives a controlled arrival signal. */
    guardian_test_restore(sram);assert(race_load_game());minerals=100;
    assert(race_scarab_enqueue(p,0));
    assert(race_scarab_production_tick(p,0)==SC64_SCARAB_ALLOCATED);
    assert(race_scarab_parent_read(p,0,&parent,&counts));child=parent.active_slot;
    for(int i=0;i<SC64_SCARAB_BUILD_TICKS;i++)assert(race_scarab_production_tick(p,0)==SC64_SCARAB_PROGRESS);
    assert(race_scarab_production_tick(p,0)==SC64_SCARAB_COMPLETED);
    int building=spawn_build(ENEMY,B_HATCHERY,u[p].x+10,u[p].y,0);assert(building>=0);
    race_scarab_probe_parent_attack(p,MAX_UNITS+building);
    assert(u[child].order==1 && u[child].target==MAX_UNITS+building);
    assert(race_scarab_flight_control_tick(child,0,0)==SC64_SCARAB_CONTROL_CONTINUE);
    assert(race_scarab_flight_control_tick(child,1,0)==SC64_SCARAB_CONTROL_ARRIVED);
    assert(race_save_game());memset(u,0,sizeof(u));assert(race_load_game());
    assert(u[child].order==4 && u[child].target==MAX_UNITS+building);
    /* The per-owner upgrade is injected by this probe; research is not wired. */
    sc64_scarab_probe_damage_upgrades=(u8)(1u<<SIDE);
    assert(race_scarab_probe_bullet_create(&u[child]));
    assert(race_impact_count==1);Sc64ImpactEvent expected=race_impact_rows[0];
    Sc64ScarabImpact packet;assert(sc64_scarab_impact_unpack(&expected,&packet) && packet.ticks==6);
    /* A real reserved Yamato shot makes the compact tail contain one BC row,
     * one impact packet and the transport trailer, all before selection. */
    int bc=spawn_unit(ENEMY,U_BATTLECRUISER,u[p].x,u[p].y);assert(bc>=0);
    u[bc].work_timer=64<<3;u[bc].carry=200*256;u[bc].cd=100;
    sc64_battlecruiser_tech_bits|=(u16)(SC64_BC_TECH_YAMATO<<(2*ENEMY));
    assert(race_bc_yamato_issue(bc,p));
    Sc64BcYamatoShot expected_bc=sc64_bc_shots[0];assert(expected_bc.state);
    assert(race_save_game());
    static u8 raw_payload[sizeof(RaceImpactRecord)];u32 raw_length=0;
    assert(save_read_latest(raw_payload,sizeof(raw_payload),SC64_SHARED_ACTOR_FLIGHT_TRANSPORT_SAVE_VERSION,&raw_length));
    assert(race_worker_cargo_select(&plan,&selected));
    assert(selected.tag==SC64_SHARED_ACTOR_FLIGHT_TRANSPORT_SAVE_VERSION && selected.length==raw_length);
    assert(!memcmp(raw_payload,&race_impact_record,raw_length));
    assert(!memcmp(plan.bc_candidate,&expected_bc,sizeof(expected_bc)));
    /* An invalid compact shot must also leave every candidate byte alone. */
    static u8 invalid_payload[sizeof(RaceImpactRecord)];
    memcpy(invalid_payload,raw_payload,raw_length);
    invalid_payload[sizeof(RaceSave)-SC64_BC_SAVE_EXTRA_BYTES+offsetof(Sc64BcYamatoShot,phase)]|=0x80u;
    memcpy(&race_impact_record,invalid_payload,raw_length);
    assert(!race_worker_cargo_prepare_selected(&race_impact_record,raw_length,selected.tag,&plan));
    assert(!memcmp(invalid_payload,&race_impact_record,raw_length));
    race_impact_reset();memset(sc64_bc_shots,0,sizeof(sc64_bc_shots));sc64_scarab_probe_damage_upgrades=0;
    assert(race_load_game());assert(sc64_scarab_probe_damage_upgrades==(1u<<SIDE));
    assert(!memcmp(&sc64_bc_shots[0],&expected_bc,sizeof(expected_bc)));
    assert(race_impact_count==1 && !memcmp(&race_impact_rows[0],&expected,sizeof(expected)));
    for (int step=6;step>0;--step) {
        while (!sc64_battlecruiser_source_tick_due()) { ++frame; }
        race_impact_tick();++frame;
        assert(race_impact_count==(step>1));
        if (step>1) { assert(sc64_scarab_impact_unpack(&race_impact_rows[0],&packet));assert(packet.ticks==(unsigned)step-1); }
    }
    /* Test each new element independently under the old authenticated pairs.
     * These are deliberately retagged rejection probes; the fallback is the
     * independently produced genuine predecessor, not a retagged fixture. */
    static const struct {u32 outer,inner;} old_pairs[]={
        {SC64_TRANSPORT_SAVE_VERSION,SAVE_VERSION},
        {SC64_SHARED_EGG_TRANSPORT_SAVE_VERSION,SC64_SHARED_EGG_SAVE_VERSION},
        {SC64_SHARED_ACTOR_TRANSPORT_SAVE_VERSION,SC64_SHARED_ACTOR_SAVE_VERSION},
        {SC64_SHARED_ACTOR_FULL_TRANSPORT_SAVE_VERSION,SC64_SHARED_ACTOR_FULL_SAVE_VERSION}
    };
    for (int marker=0;marker<2;++marker) {
        guardian_test_restore(sram);assert(race_load_game());
        assert(!sc64_scarab_probe_damage_upgrades && !race_impact_count);
        if (!marker) sc64_scarab_probe_damage_upgrades=1u<<SIDE;
        else { u[p].target=-1;assert(race_scarab_probe_bullet_create(&u[p])); }
        assert(race_save_game());assert(race_worker_cargo_select(&plan,&selected));
        guardian_test_snapshot(before);
        for (unsigned i=0;i<sizeof(old_pairs)/sizeof(old_pairs[0]);++i) {
            guardian_test_restore(before);
            guardian_test_retag(selected.slot,old_pairs[i].outer,old_pairs[i].inner);
            assert(race_worker_cargo_select(&plan,&fallback));
            assert(fallback.slot!=selected.slot && fallback.tag==SC64_SHARED_EGG_TRANSPORT_SAVE_VERSION);
        }
    }
    guardian_test_corrupt(fallback.slot);guardian_test_snapshot(before);
    memcpy(old_units,u,sizeof(u));memcpy(old_buildings,b,sizeof(b));
    int saved_minerals=minerals,saved_gas=gas,saved_frame=frame;
    assert(!race_load_game());guardian_test_snapshot(after);
    assert(!memcmp(before,after,sizeof(before)) && !memcmp(old_units,u,sizeof(u)) &&
           !memcmp(old_buildings,b,sizeof(b)) && minerals==saved_minerals &&
           gas==saved_gas && frame==saved_frame);
    puts("actual HOST paid unit/building launches, controlled arrival and target reuse passed");return 0;
}
