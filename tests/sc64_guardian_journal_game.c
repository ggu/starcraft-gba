/* SPDX-License-Identifier: GPL-3.0-or-later */
/* HOST-only constructed journal fixture; never use this as guest SRAM. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

extern void guardian_test_snapshot(void *);
extern void guardian_test_restore(const void *);
extern void guardian_test_retag(int,u32,u32);
extern void guardian_test_sequence(int,u32);
extern void guardian_test_corrupt(int);
extern void guardian_test_replace(int,const void *,u32);
extern void guardian_test_reset(void);
static u8 envelope[sizeof(RaceImpactRecord)];
static u8 snapshot[32768];

/* No rejected slot may publish a partial world or UI. HOST only. */
static void assert_failed_load_atomic(void)
{
    static Unit old_units[MAX_UNITS];static Building old_buildings[MAX_BUILDINGS];
    static u8 old_explored[sizeof(explored)],old_visible[sizeof(visible)];
    static u8 old_minerals[sizeof(m)],old_geysers[sizeof(geysers)];
    static u8 old_impacts[sizeof(race_impact_rows)],old_front[sizeof(race_front)];
    static int old_players[5],old_ore[5],old_gas[5],old_bots[3];
    Sc64SkTransportTech old_tech=race_skirmish_transport_tech;
    memcpy(old_units,u,sizeof(u));memcpy(old_buildings,b,sizeof(b));
    memcpy(old_explored,explored,sizeof(explored));memcpy(old_visible,visible,sizeof(visible));
    memcpy(old_minerals,m,sizeof(m));memcpy(old_geysers,geysers,sizeof(geysers));
    memcpy(old_impacts,race_impact_rows,sizeof(race_impact_rows));memcpy(old_front,&race_front,sizeof(race_front));
    memcpy(old_players,race_player,sizeof(race_player));memcpy(old_ore,race_minerals,sizeof(race_minerals));
    memcpy(old_gas,race_gas,sizeof(race_gas));memcpy(old_bots,race_bot_races,sizeof(race_bot_races));
    u32 old_seed=seed;int old_count=race_impact_count;
    int before[]={minerals,gas,supply_used,supply_cap,race_selected_mission,race_mode,race_human,
        race_bots,race_completed,race_skirmish_map,frame,screen,mode,menu,menu_open,menu_cursor,
        help,won,lost,result_timer,selected_building,geyser_count,(int)race_front_transition,race_front_load_error};
    assert(!race_load_game());
    assert(!memcmp(old_units,u,sizeof(u)) && !memcmp(old_buildings,b,sizeof(b)));
    assert(!memcmp(old_explored,explored,sizeof(explored)) && !memcmp(old_visible,visible,sizeof(visible)));
    assert(!memcmp(old_minerals,m,sizeof(m)) && !memcmp(old_geysers,geysers,sizeof(geysers)));
    assert(!memcmp(old_impacts,race_impact_rows,sizeof(race_impact_rows)) && old_count==race_impact_count);
    assert(!memcmp(old_front,&race_front,sizeof(race_front)) && !memcmp(&old_tech,&race_skirmish_transport_tech,sizeof(old_tech)));
    assert(!memcmp(old_players,race_player,sizeof(race_player)) && !memcmp(old_ore,race_minerals,sizeof(race_minerals)));
    assert(!memcmp(old_gas,race_gas,sizeof(race_gas)) && !memcmp(old_bots,race_bot_races,sizeof(race_bot_races)));
    int after[]={minerals,gas,supply_used,supply_cap,race_selected_mission,race_mode,race_human,
        race_bots,race_completed,race_skirmish_map,frame,screen,mode,menu,menu_open,menu_cursor,
        help,won,lost,result_timer,selected_building,geyser_count,(int)race_front_transition,race_front_load_error};
    assert(!memcmp(before,after,sizeof(before)) && old_seed==seed);
}
static void clear_journal(void)
{
    const u32 tags[]={SC64_GUARDIAN_TRANSPORT_SAVE_VERSION,SC64_TRANSPORT_SAVE_VERSION,SC64_WORKER_CARGO_LEGACY_TRANSPORT_SAVE_VERSION};
    for (unsigned t=0;t<sizeof(tags)/sizeof(tags[0]);++t)
        for (int i=0;i<2;++i) save_test_corrupt_latest(0,tags[t]);
}
int main(void)
{
    game_init(); race_mode=1;race_human=R_ZERG;race_bots=1;race_skirmish_map=0;race_bot_races[0]=R_PROTOSS;
    init_state();assert(screen==SCREEN_PLAY);
    int slot=spawn_unit(SIDE,U_OVERLORD,400,400);assert(slot>=0);
    Unit *q=&u[slot];
#if !defined(SC64_Z6_ACTORS) && !defined(SC64_SKIRMISH_ROSTER)
    /* Legacy preflight accepts this raw/type mismatch. It cannot be used as
     * an unescaped Guardian wire marker without changing a loadable save. */
    q->raw_type=44;Unit legacy=*q;clear_journal();assert(race_save_game());
    guardian_test_retag(0,SC64_TRANSPORT_SAVE_VERSION,SAVE_VERSION);
    guardian_test_retag(1,SC64_TRANSPORT_SAVE_VERSION,SAVE_VERSION);
    q->raw_type=42;assert(race_load_game());assert(!memcmp(&u[slot],&legacy,sizeof(legacy)));
    guardian_test_reset();q=&u[slot];
    q->type=U_GUARDIAN_STAGE;q->raw_type=44;q->hp=100;q->maxhp=150;
    q->armor=2;q->target=-1;q->order=ORDER_IDLE;q->work_timer=0;
    Unit staged=*q;assert(race_save_game());q->hp=1;
    assert(race_load_game());assert(!memcmp(&u[slot],&staged,sizeof(staged)));
    q=&u[slot];q->alive=0;q->hp=-1;staged=*q;assert(race_save_game());q->hp=1;
    assert(race_load_game());assert(!memcmp(&u[slot],&staged,sizeof(staged)));
    guardian_test_retag(0,SC64_TRANSPORT_SAVE_VERSION,SAVE_VERSION);
    guardian_test_retag(1,SC64_TRANSPORT_SAVE_VERSION,SAVE_VERSION);
    assert_failed_load_atomic();guardian_test_reset();q=&u[slot];
#endif
    q->type=U_GUARDIAN_COCOON;q->raw_type=59;q->alive=1;q->hp=120;q->maxhp=200;
    q->armor=0;q->order=ORDER_IDLE;q->target=44;q->build=-1;q->carry=0;q->flags=0;q->work_timer=351;
    Unit expected=*q;clear_journal();assert(race_save_game());
    GbaSaveSelection selected={0};RaceLoadPlan plan;
    assert(race_worker_cargo_select(&plan,&selected));assert(selected.tag==SC64_GUARDIAN_TRANSPORT_SAVE_VERSION);
    /* The authenticated payload cannot be presented under a different
     * normalized impact tag, even if the candidate bytes are identical. */
    Sc64ImpactEvent impact_probe[RACE_IMPACT_SAVE_CAPACITY ? RACE_IMPACT_SAVE_CAPACITY : 1];
    int impact_count=-1;
    assert(!race_impact_save_decode_selected_guardian(selected.length,
        SC64_WORKER_CARGO_LEGACY_TRANSPORT_SAVE_VERSION,impact_probe,&impact_count,&plan.guardian_auth));
    q->hp=1;q->work_timer=1;assert(race_load_game());assert(!memcmp(&u[slot],&expected,sizeof(expected)));
    q=&u[slot];q->alive=0;q->hp=-2;q->work_timer=0;expected=*q;
    assert(race_save_game());q->type=U_OVERLORD;assert(race_load_game());assert(!memcmp(&u[slot],&expected,sizeof(expected)));
    guardian_test_snapshot(snapshot);
    /* Checksum-valid but semantically invalid newest falls back to the
     * older valid live Cocoon, without publishing its rejected plan. */
    assert(race_worker_cargo_select(&plan,&selected));
    u32 length=0;assert(save_read_latest(envelope,sizeof(envelope),SC64_GUARDIAN_TRANSPORT_SAVE_VERSION,&length));
    Unit bad=expected;bad.type=U_OVERLORD;bad.alive=1;bad.hp=30;bad.work_timer=1;bad.target=43;
#if defined(SC64_ZP_FULL_STORAGE)
#if defined(SC64_P7_TYPED_COMPACT_ENABLED)
    assert(sc64_p7_typed_unit_encode(&race_data.units[slot],&bad,race_cols*16u,race_rows*16u));
#elif defined(SC64_Z7_DERIVED_SAVE)
    assert(sc64_derived_unit_encode(&race_data.units[slot],&bad,race_cols*16u,race_rows*16u));
#else
    assert(sc64_zp_unit_save_encode(&race_data.units[slot],&bad,race_cols*16u,race_rows*16u));
#endif
#else
    assert(sc64_unit_save_encode(&race_data.units[slot],&bad,race_cols*16u,race_rows*16u));
#endif
    memcpy(envelope+offsetof(RaceSave,units)+slot*sizeof(race_data.units[0]),&race_data.units[slot],sizeof(race_data.units[0]));
    guardian_test_replace(selected.slot,envelope,length);
    assert(race_load_game());assert(u[slot].alive && u[slot].hp==120 && u[slot].target==44);
    guardian_test_restore(snapshot);
    /* Old authenticated namespace cannot reinterpret a Cocoon wire row. */
    guardian_test_retag(0,SC64_TRANSPORT_SAVE_VERSION,SAVE_VERSION);
    guardian_test_retag(1,SC64_TRANSPORT_SAVE_VERSION,SAVE_VERSION);
    Unit before=u[slot];assert_failed_load_atomic();assert(!memcmp(&before,&u[slot],sizeof(before)));
    assert(!race_worker_cargo_select(&plan,&selected) && !plan.guardian_auth.candidate);guardian_test_restore(snapshot);
    /* Two valid half-range candidates remain ambiguous and cannot publish. */
    guardian_test_sequence(0,1);guardian_test_sequence(1,0x80000001u);
    assert_failed_load_atomic();assert(!race_worker_cargo_select(&plan,&selected) && !plan.guardian_auth.candidate);guardian_test_restore(snapshot);
    /* Unknown committed neighbor is never overwritten by the stage writer. */
    guardian_test_retag(0,0x11223344u,SAVE_VERSION);guardian_test_snapshot(snapshot);
    assert(!race_save_game());u8 after[32768];guardian_test_snapshot(after);assert(!memcmp(snapshot,after,sizeof(after)));
    /* Stale predecessor cannot authorize publication. */
    guardian_test_restore(snapshot);guardian_test_retag(0,SC64_GUARDIAN_TRANSPORT_SAVE_VERSION,SC64_GUARDIAN_SAVE_VERSION);
    assert(race_worker_cargo_select(&plan,&selected));guardian_test_sequence(selected.slot,selected.sequence+1);
    const u32 allowed[]={SC64_WORKER_CARGO_LEGACY_TRANSPORT_SAVE_VERSION,SC64_TRANSPORT_SAVE_VERSION,SC64_GUARDIAN_TRANSPORT_SAVE_VERSION};
    assert(!save_write_selected(&race_impact_record,selected.length,SC64_GUARDIAN_TRANSPORT_SAVE_VERSION,&selected,allowed,3));
    /* Torn newest candidate falls back to the other valid row. */
    guardian_test_restore(snapshot);guardian_test_retag(0,SC64_GUARDIAN_TRANSPORT_SAVE_VERSION,SC64_GUARDIAN_SAVE_VERSION);
    assert(race_worker_cargo_select(&plan,&selected));guardian_test_corrupt(selected.slot);
    assert(race_load_game());
    guardian_test_reset();
    q=&u[slot];q->alive=1;q->hp=120;q->work_timer=351;q->target=43;
    assert(!race_save_game());q->target=44;
    /* Actual engine damage/death, then a save/load of the dead row and slot reuse. */
    supply_used=race_owner_supply(SIDE);hit_target(slot,500,U_MARINE,ENEMY);
    assert(!u[slot].alive && u[slot].type==U_GUARDIAN_COCOON && u[slot].hp<=0);
    expected=u[slot];assert(race_save_game());u[slot].type=U_OVERLORD;
    assert(race_load_game());assert(!memcmp(&u[slot],&expected,sizeof(expected)));
    int reuse=spawn_unit(SIDE,U_OVERLORD,400,400);assert(reuse==slot);
    assert(u[slot].type==U_OVERLORD && u[slot].raw_type!=59);
    assert(race_save_game());assert(race_load_game());assert(u[slot].type==U_OVERLORD);
    /* Two distinct namespaces: a bad newer Guardian slot must not lend its
     * authorization to the older valid ordinary legacy Overlord. */
    guardian_test_reset();q=&u[slot];assert(q->alive && q->type==U_OVERLORD);
    q->raw_type=36; /* Existing ordinary/raw36 rows are valid legacy data. */
    Unit legacy_ordinary=*q;assert(race_save_game());
    assert(race_worker_cargo_select(&plan,&selected));
    guardian_test_retag(selected.slot,SC64_TRANSPORT_SAVE_VERSION,SAVE_VERSION);
    q->type=U_GUARDIAN_COCOON;q->raw_type=59;q->hp=q->maxhp=200;q->armor=0;
    q->order=ORDER_IDLE;q->target=44;q->build=-1;q->carry=0;q->flags=0;q->work_timer=300;
    assert(race_save_game());assert(race_worker_cargo_select(&plan,&selected));
    assert(selected.tag==SC64_GUARDIAN_TRANSPORT_SAVE_VERSION);
    length=0;assert(save_read_latest(envelope,sizeof(envelope),SC64_GUARDIAN_TRANSPORT_SAVE_VERSION,&length));
    Unit invalid_wire;assert(sc64_guardian_wire_encode(q,&invalid_wire,1));invalid_wire.target=43;
#ifdef SC64_ZP_FULL_STORAGE
#ifdef SC64_P7_TYPED_COMPACT_ENABLED
    assert(sc64_p7_typed_unit_encode(&race_data.units[slot],&invalid_wire,race_cols*16u,race_rows*16u));
#elif defined(SC64_Z7_DERIVED_SAVE)
    assert(sc64_derived_unit_encode(&race_data.units[slot],&invalid_wire,race_cols*16u,race_rows*16u));
#else
    assert(sc64_zp_unit_save_encode(&race_data.units[slot],&invalid_wire,race_cols*16u,race_rows*16u));
#endif
#else
    assert(sc64_unit_save_encode(&race_data.units[slot],&invalid_wire,race_cols*16u,race_rows*16u));
#endif
    memcpy(envelope+offsetof(RaceSave,units)+slot*sizeof(race_data.units[0]),&race_data.units[slot],sizeof(race_data.units[0]));
    guardian_test_replace(selected.slot,envelope,length);
    assert(race_load_game());assert(!memcmp(&u[slot],&legacy_ordinary,sizeof(legacy_ordinary)));
    assert(race_worker_cargo_select(&plan,&selected));
    assert(selected.tag==SC64_TRANSPORT_SAVE_VERSION && plan.guardian_auth.features==0);
    puts("Guardian staged HOST journal and death/slot reuse passed");return 0;
}
