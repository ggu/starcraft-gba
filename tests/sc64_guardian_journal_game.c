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
static u8 envelope[sizeof(RaceImpactRecord)];
static u8 snapshot[32768];

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
    Unit *q=&u[slot];q->type=U_GUARDIAN_COCOON;q->raw_type=59;q->hp=120;q->maxhp=200;
    q->armor=0;q->order=ORDER_IDLE;q->target=44;q->build=-1;q->carry=0;q->flags=0;q->work_timer=351;
    Unit expected=*q;clear_journal();assert(race_save_game());
    GbaSaveSelection selected={0};RaceLoadPlan plan;
    assert(race_worker_cargo_select(&plan,&selected));assert(selected.tag==SC64_GUARDIAN_TRANSPORT_SAVE_VERSION);
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
    Unit before=u[slot];assert(!race_load_game());assert(!memcmp(&before,&u[slot],sizeof(before)));
    assert(!race_guardian_stage_wire);guardian_test_restore(snapshot);
    /* Two valid half-range candidates remain ambiguous and cannot publish. */
    guardian_test_sequence(0,1);guardian_test_sequence(1,0x80000001u);
    assert(!race_load_game());assert(!race_guardian_stage_wire);guardian_test_restore(snapshot);
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
    clear_journal();
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
    puts("Guardian staged HOST journal and death/slot reuse passed");return 0;
}
