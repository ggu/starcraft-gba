/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Shared Battery candidate validation and custom-game save round trips. */
#define main skirmish_runtime_fixture_main
#include "sc64_skirmish_runtime.c"
#undef main

#ifndef SC64_SHARED_BATTERY
#error "shared Battery fixture requires the shared capability"
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
    gba_init(); game_init(); race_mode=1; race_human=R_PROTOSS; race_bots=1;
    init_state(); assert(screen==SCREEN_PLAY && race_mode); minerals=gas=5000;
    int battery_type=building_raw(172), zealot_type=unit_raw(65);
    if (battery_type<0 || zealot_type<0) {
        puts("SKIP shared Battery save: profile lacks actor identity"); return 0;
    }
    int battery=spawn_build(SIDE,battery_type,300,300,0);
    int unit=spawn_unit(SIDE,zealot_type,304,300);
    int unshielded=spawn_unit(SIDE,U_MARINE,350,300);
    assert(battery>=0 && unit>=0 && unshielded>=0);
    u[unshielded].shield_remainder=9;
    b[battery].raw_type=-1; b[battery].raw_owner=-1;
    b[battery].queue_head=0; b[battery].queue_count=0;
    b[battery].queue_progress=12345; b[battery].level=3;
    u[unit].raw_type=-1; u[unit].raw_owner=-1;
    u[unit].shield=1; u[unit].shield_remainder=7;
    u[unit].order=ORDER_RECHARGE; u[unit].target=battery; u[unit].work_timer=0;
    assert(race_save_game()); valid_candidate=race_data;
    assert(valid_candidate.state[SC64_SHIELD_PHASE_SAVE_MARKER_INDEX]==SC64_SHIELD_PHASE_SAVE_MARKER);
    assert(race_load_game());
    assert(b[battery].queue_progress==12345 && b[battery].level==3);
    assert(u[unit].order==ORDER_RECHARGE && u[unit].target==battery &&
           u[unit].shield==1 && u[unit].shield_remainder==7);
    for (int mutation=0;mutation<8;++mutation) {
        invalid_candidate=valid_candidate;
        Unit actor=u[unit]; Building target=b[battery];
        switch (mutation) {
        case 0: actor.target=-1; break;
        case 1: actor.owner=ENEMY; break;
        case 2: target.queue_progress=SC64_P7_BATTERY_MAX_ENERGY_Q8+1; break;
        case 3: target.level=SC64_P7_BATTERY_REGEN_DEN; break;
        case 4: target.queue_count=1; target.queue[0]=U_SCV; break;
        case 5: actor.shield=sc64_race_unit_rule(actor.type)->shields;
                actor.shield_remainder=7; break;
        case 6: target.raw_type=156; break;
        case 7: target.queue[3]=U_MARINE; break;
        }
        if (mutation<2 || mutation==5) encode_unit(&invalid_candidate,unit,&actor);
        else encode_building(&invalid_candidate,battery,&target);
        reject_without_live_mutation(&invalid_candidate);
    }
    /* A genuine legacy non-P7 candidate migrates only shielded units. */
    invalid_candidate=valid_candidate;
    invalid_candidate.state[SC64_SHIELD_PHASE_SAVE_MARKER_INDEX]=0;
    Unit phase_actor=u[unit]; phase_actor.order=ORDER_IDLE; phase_actor.target=-1;
    phase_actor.shield_remainder=319;
    encode_unit(&invalid_candidate,unit,&phase_actor);
    Building legacy_battery=b[battery];
    legacy_battery.queue_progress=0;
    legacy_battery.queue[0]=U_MARINE; legacy_battery.queue[3]=U_SCV;
    encode_building(&invalid_candidate,battery,&legacy_battery);
    save_write(&invalid_candidate,sizeof(invalid_candidate),SAVE_VERSION);
    save_write(&invalid_candidate,sizeof(invalid_candidate),SAVE_VERSION);
    assert(race_load_game());
    assert(u[unit].shield_remainder==1276 && u[unshielded].shield_remainder==9);
    assert(b[battery].queue_count==0 && b[battery].queue_head==0 &&
           b[battery].queue_progress==0); /* Do not invent legacy energy. */
    for (int j=0;j<5;++j) assert(b[battery].queue[j]==0);
    invalid_candidate=valid_candidate;
    invalid_candidate.state[SC64_SHIELD_PHASE_SAVE_MARKER_INDEX]=0;
    phase_actor.shield_remainder=319; encode_unit(&invalid_candidate,unit,&phase_actor);
    legacy_battery=b[battery]; legacy_battery.queue_count=1;
    encode_building(&invalid_candidate,battery,&legacy_battery);
    reject_without_live_mutation(&invalid_candidate);
    legacy_battery.queue_count=0; legacy_battery.queue_head=1;
    encode_building(&invalid_candidate,battery,&legacy_battery);
    reject_without_live_mutation(&invalid_candidate);

    /* New-format phases are not multiplied; unknown markers and an old
     * image containing the new order fail before changing live state. */
    invalid_candidate=valid_candidate;
    phase_actor=u[unit]; phase_actor.order=ORDER_IDLE; phase_actor.target=-1;
    phase_actor.shield_remainder=317; encode_unit(&invalid_candidate,unit,&phase_actor);
    save_write(&invalid_candidate,sizeof(invalid_candidate),SAVE_VERSION);
    save_write(&invalid_candidate,sizeof(invalid_candidate),SAVE_VERSION);
    assert(race_load_game() && u[unit].shield_remainder==317);
    invalid_candidate=valid_candidate;
    invalid_candidate.state[SC64_SHIELD_PHASE_SAVE_MARKER_INDEX]=0x12345678;
    reject_without_live_mutation(&invalid_candidate);
    invalid_candidate=valid_candidate;
    invalid_candidate.state[SC64_SHIELD_PHASE_SAVE_MARKER_INDEX]=0;
    reject_without_live_mutation(&invalid_candidate); /* new order 15 */
    phase_actor.order=ORDER_IDLE; phase_actor.target=-1; phase_actor.shield_remainder=320;
    encode_unit(&invalid_candidate,unit,&phase_actor);
    reject_without_live_mutation(&invalid_candidate);
    save_write(&valid_candidate,sizeof(valid_candidate),SAVE_VERSION);
    save_write(&valid_candidate,sizeof(valid_candidate),SAVE_VERSION);
    assert(race_load_game());
    /* The candidate-only validator must not interpret or overwrite the shared
     * mission union when a non-P7 campaign carries a recharge order. */
#if defined(SC64_P5_RUNTIME) || defined(SC64_P6_RUNTIME)
    invalid_candidate=valid_candidate;
    invalid_candidate.state[0]=0;
#ifdef SC64_P5_RUNTIME
    invalid_candidate.state[2]=SC64_P5_GLOBAL_ID;
#else
    invalid_candidate.state[2]=SC64_P6_GLOBAL_ID;
#endif
    RaceSave union_before=invalid_candidate;
    Unit candidate_actor;
    assert(race_battery_save_unit(&invalid_candidate,(unsigned)unit,
        (unsigned)race_cols*16u,(unsigned)race_rows*16u,&candidate_actor));
    assert(race_battery_saved_recharge_valid(&invalid_candidate,&candidate_actor));
    assert(!memcmp(&union_before,&invalid_candidate,sizeof(invalid_candidate)));
#ifdef SC64_P6_RUNTIME
    invalid_candidate.state[2]=SC64_P6_GLOBAL_ID;
    union_before=invalid_candidate;
    assert(race_battery_saved_recharge_valid(&invalid_candidate,&candidate_actor));
    assert(!memcmp(&union_before,&invalid_candidate,sizeof(invalid_candidate)));
#endif
#endif
#if defined(SC64_P7_BATTERY_ENABLED) && defined(SC64_P5_SHUTTLE_ENABLED)
    /* P7 keeps its historical order and exact source ownership. */
    invalid_candidate=valid_candidate;
    invalid_candidate.state[0]=0; invalid_candidate.state[2]=SC64_P7_GLOBAL_ID;
    Unit legacy=u[unit]; Building p7_legacy_battery=b[battery];
    legacy.order=ORDER_P7_RECHARGE; legacy.raw_type=65; legacy.raw_owner=7;
    p7_legacy_battery.raw_type=172; p7_legacy_battery.raw_owner=7;
    encode_unit(&invalid_candidate,unit,&legacy);
    encode_building(&invalid_candidate,battery,&p7_legacy_battery);
    assert(race_p5_saved_p7_recharge_valid(&invalid_candidate,&legacy));
    /* Markerless P7 already used the 1/1280 scale. */
    invalid_candidate.state[SC64_SHIELD_PHASE_SAVE_MARKER_INDEX]=0;
    legacy.shield_remainder=317; encode_unit(&invalid_candidate,unit,&legacy);
    assert(race_battery_migrate_shield_phase(&invalid_candidate,race_cols,race_rows));
    Unit migrated;
    assert(race_battery_save_unit(&invalid_candidate,(unsigned)unit,
           (unsigned)race_cols*16u,(unsigned)race_rows*16u,&migrated));
    assert(migrated.shield_remainder==317 && migrated.order==ORDER_P7_RECHARGE);
    legacy.order=ORDER_RECHARGE;
    assert(!race_battery_saved_recharge_valid(&invalid_candidate,&legacy));
#endif
    puts("shared Battery save: partial energy, recharge and malformed candidates PASS");
    return 0;
}
