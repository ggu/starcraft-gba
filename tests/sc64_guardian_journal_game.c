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
    puts("Guardian staged HOST journal roundtrip and dead row passed");return 0;
}
