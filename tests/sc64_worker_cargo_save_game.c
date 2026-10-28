/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Optional actual-engine fixture: old/new cargo semantics over real codecs.
 * Compile with a prepared canonical profile and SC64_WORKER_CARGO_KIND.
 * Direct fixture setup is not campaign or normal-controller win evidence. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

#ifndef SC64_WORKER_CARGO_KIND
#error "Enable explicit cargo semantics for this optional fixture"
#endif

static void encode_unit(RaceSave *out, unsigned slot, const Unit *q)
{
#if defined(SC64_LATE_TERRAN_STORAGE) || defined(SC64_ZP_CAMPAIGN_STORAGE)
#ifdef SC64_ZP_FULL_STORAGE
#ifdef SC64_P7_TYPED_COMPACT_ENABLED
    assert(sc64_p7_typed_unit_encode(&out->units[slot],q,
                                   race_cols*16u,race_rows*16u));
#elif defined(SC64_Z7_DERIVED_SAVE)
    assert(sc64_derived_unit_encode(&out->units[slot],q,
                                   race_cols*16u,race_rows*16u));
#else
    assert(sc64_zp_unit_save_encode(&out->units[slot],q,
                                   race_cols*16u,race_rows*16u));
#endif
#else
    assert(sc64_unit_save_encode(&out->units[slot],q,
                                race_cols*16u,race_rows*16u));
#endif
#else
    for (unsigned j=0;j<RACE_UNIT_WORDS;++j) {
        int value;
        memcpy(&value,(const u8 *)q+j*sizeof(int),sizeof(value));
        out->units[slot][j]=(s16)value;
    }
#endif
}

static void clear_fixture_journal(void)
{
    /* The host backend has two slots. Invalidate both generations explicitly
     * so each case tests the loader fallback rather than an earlier new save. */
    save_test_corrupt_latest(sizeof(RaceSave),SAVE_VERSION);
    save_test_corrupt_latest(sizeof(RaceSave),SAVE_VERSION);
    save_test_corrupt_latest(sizeof(RaceSave),SC64_WORKER_CARGO_LEGACY_SAVE_VERSION);
    save_test_corrupt_latest(sizeof(RaceSave),SC64_WORKER_CARGO_LEGACY_SAVE_VERSION);
    assert(!save_read(&race_data,sizeof(race_data),SAVE_VERSION));
    assert(!save_read(&race_data,sizeof(race_data),SC64_WORKER_CARGO_LEGACY_SAVE_VERSION));
}

static void assert_reject_atomic(const RaceSave *bad, u32 tag)
{
    static Unit before_units[MAX_UNITS];
    static Building before_buildings[MAX_BUILDINGS];
    int ore=minerals,vespene=gas,clock=frame;
    u32 before_seed=seed;
    memcpy(before_units,u,sizeof(u));
    memcpy(before_buildings,b,sizeof(b));
    clear_fixture_journal();
    save_write(bad,sizeof(*bad),tag);
    assert(!race_load_game());
    assert(!memcmp(before_units,u,sizeof(u)));
    assert(!memcmp(before_buildings,b,sizeof(b)));
    assert(minerals==ore && gas==vespene && frame==clock && seed==before_seed);
}

static void cargo_save_case(int human)
{
    static RaceSave checkpoint,legacy,bad;
    Unit expected[4];
    int slots[4],count=0,patch=-1;
    race_mode=1; race_human=human; race_bots=1; race_skirmish_map=0;
    race_bot_races[0]=(human+1)%3;
    init_state();
    assert(screen==SCREEN_PLAY && race_mode && race_human==human);
    for (int i=0;i<MAX_UNITS && count<4;++i)
        if (u[i].alive && u[i].owner==SIDE && IS_WORKER(u[i].type))
            slots[count++]=i;
    assert(count==4);
    for (int i=0;i<MAX_MINERALS;++i)
        if (m[i].amount>0) { patch=i; break; }
    assert(patch>=0 && geyser_count>0);
    int gas_type=human==0 ? B_REFINERY :
                 human==R_ZERG ? B_EXTRACTOR : B_ASSIMILATOR;
    int refinery=spawn_build(SIDE,gas_type,geysers[0].x,geysers[0].y,0);
    assert(refinery>=0 && building_complete(refinery));
    for (int j=0;j<4;++j) {
        Unit *q=&u[slots[j]];
        q->carry=8; q->flags&=~0x60; q->build=-1; q->work_timer=0;
        q->order=j==0 ? ORDER_MINE : j==1 ? ORDER_GAS :
                 j==2 ? ORDER_MOVE : ORDER_IDLE;
        q->target=j==1 ? refinery : j==0 ? patch : -1;
        if (j==1) q->flags|=0x20;
        if (j>=2) q->flags|=0x40;
        expected[j]=*q;
    }
    clear_fixture_journal();
    assert(race_save_game());
    assert(save_read(&checkpoint,sizeof(checkpoint),SAVE_VERSION));
    assert(checkpoint.version==SAVE_VERSION);
    for (int j=0;j<4;++j) {
        u[slots[j]].carry=0; u[slots[j]].flags&=~0x60;
    }
    assert(race_load_game());
    for (int j=0;j<4;++j) {
        assert(u[slots[j]].carry==8);
        assert(u[slots[j]].flags==expected[j].flags);
        assert(u[slots[j]].order==expected[j].order);
        assert(u[slots[j]].target==expected[j].target);
    }

    /* Old explicit gather orders identify their cargo. Old interrupted
     * orders do not: retain UNKNOWN instead of crediting a guessed bank. */
    legacy=checkpoint;
    legacy.version=SC64_WORKER_CARGO_LEGACY_SAVE_VERSION;
    for (int j=0;j<4;++j) {
        Unit old=expected[j]; old.flags&=~0x60;
        encode_unit(&legacy,(unsigned)slots[j],&old);
    }
    clear_fixture_journal();
    save_write(&legacy,sizeof(legacy),SC64_WORKER_CARGO_LEGACY_SAVE_VERSION);
    assert(race_load_game());
    for (int j=0;j<4;++j) {
        assert(u[slots[j]].carry==8);
        assert((u[slots[j]].flags&0x60)==(j==1 ? 0x20 : j>=2 ? 0x40 : 0));
        assert(u[slots[j]].order==expected[j].order);
    }
    assert(race_save_game());
    assert(save_read(&checkpoint,sizeof(checkpoint),SAVE_VERSION));
    assert(checkpoint.version==SAVE_VERSION);

    Unit corrupt=expected[0]; corrupt.flags|=0x60;
    bad=checkpoint; encode_unit(&bad,(unsigned)slots[0],&corrupt);
    assert_reject_atomic(&bad,SAVE_VERSION);
    bad=legacy; corrupt=expected[1];
    encode_unit(&bad,(unsigned)slots[1],&corrupt); /* reserved bit in old row */
    assert_reject_atomic(&bad,SC64_WORKER_CARGO_LEGACY_SAVE_VERSION);
    bad=checkpoint; bad.version=SC64_WORKER_CARGO_LEGACY_SAVE_VERSION;
    assert_reject_atomic(&bad,SAVE_VERSION); /* journal/body mismatch */
    printf("worker cargo journal migration race=%d payload=%zu: ok\n",human,sizeof(RaceSave));
}

int main(void)
{
    _Static_assert(SAVE_VERSION != SC64_WORKER_CARGO_LEGACY_SAVE_VERSION,
                   "old readers must not accept explicit cargo semantics");
    game_init();
    for (int race=0;race<3;++race) cargo_save_case(race);
    puts("worker cargo actual-engine current/legacy save migration: ok");
    return 0;
}
