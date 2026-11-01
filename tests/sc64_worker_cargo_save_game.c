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

#ifdef SC64_SHARED_TRANSPORT
#define CARGO_CURRENT_TAG SC64_TRANSPORT_SAVE_VERSION
#define CARGO_LEGACY_TAG SC64_WORKER_CARGO_LEGACY_TRANSPORT_SAVE_VERSION
#else
#define CARGO_CURRENT_TAG SAVE_VERSION
#define CARGO_LEGACY_TAG SC64_WORKER_CARGO_LEGACY_SAVE_VERSION
#endif
static u8 fixture_envelope[sizeof(RaceImpactRecord)];
static u32 fixture_length;

static void clear_fixture_journal(void)
{
    save_test_corrupt_latest(0,CARGO_CURRENT_TAG);
    save_test_corrupt_latest(0,CARGO_CURRENT_TAG);
    save_test_corrupt_latest(0,CARGO_LEGACY_TAG);
    save_test_corrupt_latest(0,CARGO_LEGACY_TAG);
#if defined(SC64_BATTLECRUISER_ENERGY_EXPERIMENT) && !defined(SC64_SHARED_TRANSPORT)
    save_test_corrupt_latest(0,SC64_LEGACY_SAVE_VERSION);
    save_test_corrupt_latest(0,SC64_LEGACY_SAVE_VERSION);
#endif
}

/* Retain the real writer's variable-length Yamato, impact and transport
 * extension while replacing the complete fixed body with the fixture. */
static u8 authored_envelope[sizeof(RaceImpactRecord)];
static void prepare_fixture(const RaceSave *body)
{
    size_t body_length = sizeof(*body);
#ifdef SC64_BATTLECRUISER_YAMATO_EXPERIMENT
    body_length -= SC64_BC_SAVE_EXTRA_BYTES;
#endif
    assert(fixture_length >= body_length);
    memcpy(authored_envelope,fixture_envelope,fixture_length);
    memcpy(authored_envelope,body,body_length);
}

static void write_fixture(const RaceSave *body, u32 tag)
{
    prepare_fixture(body);
    save_write(authored_envelope,fixture_length,tag);
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
    write_fixture(bad,tag);
    assert(!race_load_game());
    assert(!memcmp(before_units,u,sizeof(u)));
    assert(!memcmp(before_buildings,b,sizeof(b)));
    assert(minerals==ore && gas==vespene && frame==clock && seed==before_seed);
}

static void cargo_save_case(int human)
{
    static RaceSave checkpoint,legacy,bad;
    Unit expected[5];
    int slots[5],count=0,patch=-1;
    race_mode=1; race_human=human; race_bots=1; race_skirmish_map=0;
    race_bot_races[0]=(human+1)%3;
    init_state();
    assert(screen==SCREEN_PLAY && race_mode && race_human==human);
    for (int i=0;i<MAX_UNITS && count<4;++i)
        if (u[i].alive && u[i].owner==SIDE && IS_WORKER(u[i].type))
            slots[count++]=i;
    assert(count==4);
    slots[4]=spawn_unit(SIDE,u[slots[0]].type,u[slots[0]].x,u[slots[0]].y);
    assert(slots[4]>=0);
    for (int i=0;i<MAX_MINERALS;++i)
        if (m[i].amount>0) { patch=i; break; }
    assert(patch>=0 && geyser_count>0);
    int gas_type=human==0 ? B_REFINERY :
                 human==R_ZERG ? B_EXTRACTOR : B_ASSIMILATOR;
    int refinery=spawn_build(SIDE,gas_type,geysers[0].x,geysers[0].y,0);
    assert(refinery>=0 && building_complete(refinery));
    for (int j=0;j<5;++j) {
        Unit *q=&u[slots[j]];
        q->carry=j==4 ? 0 : 8; q->flags&=~SC64_WORKER_CARGO_MASK; q->build=-1; q->work_timer=0;
        q->order=j==0 ? ORDER_MINE : j==1 ? ORDER_GAS :
                 j==2 ? ORDER_MOVE : j==3 ? ORDER_IDLE : ORDER_MINE;
        q->target=j==1 ? refinery : (j==0 || j==4) ? patch : -1;
        if (j==1) q->flags|=0x20;
        if (j==2 || j==3) q->flags|=0x40;
        if (j==1 || j==4) {
            q->flags|=SC64_WORKER_GATHER_PENDING;
            q->work_timer=2;
        }
        expected[j]=*q;
    }
    clear_fixture_journal();
    assert(race_save_game());
    assert(save_read_latest(fixture_envelope,sizeof(fixture_envelope),CARGO_CURRENT_TAG,&fixture_length));
    { RaceLoadPlan plan; GbaSaveSelection selection;
      assert(race_worker_cargo_select(&plan,&selection));
      assert(selection.tag==CARGO_CURRENT_TAG); checkpoint=race_data; }
    assert(checkpoint.version==SAVE_VERSION);
    for (int j=0;j<5;++j) {
        u[slots[j]].carry=0; u[slots[j]].flags&=~0x60;
    }
    assert(race_load_game());
    for (int j=0;j<5;++j) {
        assert(u[slots[j]].carry==expected[j].carry);
        assert(u[slots[j]].flags==expected[j].flags);
        assert(u[slots[j]].work_timer==expected[j].work_timer);
        assert(u[slots[j]].order==expected[j].order);
        assert(u[slots[j]].target==expected[j].target);
    }

    /* Old explicit gather orders identify their cargo. Old interrupted
     * orders do not: retain UNKNOWN instead of crediting a guessed bank. */
    legacy=checkpoint;
    legacy.version=SC64_WORKER_CARGO_LEGACY_SAVE_VERSION;
    for (int j=0;j<5;++j) {
        Unit old=expected[j]; old.flags&=~SC64_WORKER_CARGO_MASK;
        encode_unit(&legacy,(unsigned)slots[j],&old);
    }
    clear_fixture_journal();
    write_fixture(&legacy,CARGO_LEGACY_TAG);
    /* Publish a checksum-valid newer record with invalid cargo bits. The
     * loader must fall back, and the next save must retain that predecessor. */
    {
        RaceLoadPlan plan;
        GbaSaveSelection old_selection, fallback, newest;
        static u8 retained[sizeof(RaceImpactRecord)];
        u32 retained_length;
        const u32 tags[] = { CARGO_LEGACY_TAG, CARGO_CURRENT_TAG };
        Unit invalid = expected[0];
        assert(race_worker_cargo_select(&plan,&old_selection));
        assert(old_selection.tag == CARGO_LEGACY_TAG);
        bad = checkpoint;
        invalid.flags |= SC64_WORKER_CARGO_KIND_MASK;
        encode_unit(&bad,(unsigned)slots[0],&invalid);
        prepare_fixture(&bad);
        assert(save_write_selected(authored_envelope,fixture_length,CARGO_CURRENT_TAG,
                                   &old_selection,tags,2));
        assert(save_read_latest(retained,sizeof(retained),CARGO_CURRENT_TAG,&retained_length));
        assert(retained_length == fixture_length);
        assert(race_worker_cargo_select(&plan,&fallback));
        assert(fallback.tag == old_selection.tag && fallback.slot == old_selection.slot &&
               fallback.sequence == old_selection.sequence);
        assert(race_load_game());
        assert(u[slots[0]].carry == expected[0].carry &&
               !(u[slots[0]].flags & SC64_WORKER_CARGO_KIND_MASK));
        assert(u[slots[1]].carry == expected[1].carry &&
               sc64_worker_cargo_is_gas(&u[slots[1]]));
        assert(race_save_game());
        assert(save_read_latest(retained,sizeof(retained),CARGO_LEGACY_TAG,&retained_length));
        assert(race_worker_cargo_select(&plan,&newest));
        assert(newest.tag == CARGO_CURRENT_TAG && newest.slot != old_selection.slot);
        assert((s32)(newest.sequence-old_selection.sequence)>0);
        clear_fixture_journal();
        write_fixture(&legacy,CARGO_LEGACY_TAG);
    }
    assert(race_load_game());
    for (int j=0;j<5;++j) {
        assert(u[slots[j]].carry==expected[j].carry);
        assert((u[slots[j]].flags&0x60)==(j==1 ? 0x20 : (j==2 || j==3) ? 0x40 : 0));
        assert(u[slots[j]].order==expected[j].order);
    }
    assert(race_save_game());
    { RaceLoadPlan plan; GbaSaveSelection selection;
      assert(race_worker_cargo_select(&plan,&selection));
      assert(selection.tag==CARGO_CURRENT_TAG); checkpoint=race_data; }
    assert(checkpoint.version==SAVE_VERSION);

#if defined(SC64_BATTLECRUISER_ENERGY_EXPERIMENT) && !defined(SC64_SHARED_TRANSPORT)
    /* The pre-energy journal predates both BC energy and cargo semantics. */
    {
        static RaceSave pre_energy;
        RaceLoadPlan plan; GbaSaveSelection selection;
        pre_energy=legacy;
        pre_energy.version=SC64_LEGACY_SAVE_VERSION;
        pre_energy.state[27]=0;
#ifdef SC64_BATTLECRUISER_YAMATO_EXPERIMENT
        assert(fixture_length==sizeof(RaceSave)-SC64_BC_SAVE_EXTRA_BYTES);
#endif
        clear_fixture_journal();
        write_fixture(&pre_energy,SC64_LEGACY_SAVE_VERSION);
        assert(race_worker_cargo_select(&plan,&selection));
        assert(selection.tag==SC64_LEGACY_SAVE_VERSION && plan.legacy_worker_cargo);
        assert(race_load_game());
        assert(race_save_game());
        assert(race_worker_cargo_select(&plan,&selection));
        assert(selection.tag==SAVE_VERSION);
    }
#endif

    Unit corrupt=expected[0]; corrupt.flags|=0x60;
    bad=checkpoint; encode_unit(&bad,(unsigned)slots[0],&corrupt);
    assert_reject_atomic(&bad,CARGO_CURRENT_TAG);
    bad=legacy; corrupt=expected[1];
    encode_unit(&bad,(unsigned)slots[1],&corrupt); /* reserved bit in old row */
    assert_reject_atomic(&bad,CARGO_LEGACY_TAG);
    bad=checkpoint; bad.version=SC64_WORKER_CARGO_LEGACY_SAVE_VERSION;
    assert_reject_atomic(&bad,CARGO_CURRENT_TAG); /* journal/body mismatch */
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
