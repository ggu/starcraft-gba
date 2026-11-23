/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Authored, asset-free packed FIFO journal selection and normalization. */
#define main packed_codec_fixture_main
#include "sc64_mineral_fifo_packed.c"
#undef main
#include "../common/gba.h"
#include "../starcraft/sc64_stage_save_auth.h"

#if defined(SC64_P7_TYPED_COMPACT_ENABLED)
typedef Sc64P7TypedUnitSave Row;
#define READ sc64_p7_typed_unit_decode
#elif defined(SC64_Z7_DERIVED_SAVE)
typedef Sc64DerivedUnitSave Row;
#define READ sc64_derived_unit_decode
#elif defined(SC64_ZP_FULL_STORAGE)
typedef Sc64ZpPackedUnitSave Row;
#define READ sc64_zp_unit_save_decode
#else
typedef Sc64PackedUnitSave Row;
#define READ sc64_unit_save_decode
#endif

typedef struct { u32 version; Row units[MAX_UNITS]; } RaceSave;
enum { ORDER_MINE=3, ORDER_REPAIR=4, WORKER=U_OVERLORD };
#define IS_WORKER(t) ((t)==WORKER)
static int sc64_worker_cargo_state_valid(const Unit *q) {
    if (!q) return 0;
    if (!q->alive || !IS_WORKER(q->type) || q->order==ORDER_REPAIR) return 1;
    int kind=q->flags & 0x60;
    return kind!=0x60 && q->carry>=0 && q->carry<=8 &&
        (q->carry || !kind) && (!(q->flags & 0x80) || q->order==ORDER_MINE);
}
static int sc64_worker_cargo_matches_order(const Unit *q) {
    return q && q->carry>0 && !(q->flags & (0x20 | 0x40));
}
static int race_save_decode_unit(const RaceSave *s,unsigned i,unsigned w,unsigned h,Unit *q) {
    return READ(q,&s->units[i],w,h);
}
#include "../starcraft/sc64_packed_mineral_fifo_save.h"

void guardian_test_reset(void);
void guardian_test_sequence(int,u32);
void guardian_test_corrupt(int);
void guardian_test_snapshot(void *);

#define FIFO_TAG 0x53434631u
#define OTHER_TAG 0x53434632u
#define INNER 0x53434633u
#define LEGACY_TAG 0x53434634u
#define LEGACY_INNER 0x53434635u
static const u32 tags[]={FIFO_TAG,OTHER_TAG,LEGACY_TAG};
static const Sc64StageTagPair pairs[]={
    {FIFO_TAG,INNER,SC64_STAGE_SAVE_MINERAL_FIFO},
    {OTHER_TAG,INNER,0},
    {LEGACY_TAG,LEGACY_INNER,0}
};
typedef struct { Sc64MineralFifo queue; Sc64StageSaveAuth auth; } Plan;
static RaceSave scratch;
static int eligible(const RaceSave *s,unsigned slot,const Unit *q,unsigned w,unsigned h) {
    (void)s; (void)slot;
    return q->target==4 && q->x< (int)w && q->y<(int)h && q->x==30 && q->y==40;
}
static int prepare(const void *raw,u32 len,u32 tag,void *ctx) {
    Plan *p=ctx;
    memset(p,0,sizeof(*p));
    if (raw!=&scratch || len!=sizeof scratch ||
        !sc64_stage_save_authenticate(&p->auth,&scratch,tag,scratch.version,pairs,3)) return 0;
    if (tag==LEGACY_TAG) {
        for (unsigned slot=0; slot<MAX_UNITS; ++slot) {
            Unit q;
            if (!race_save_decode_unit(&scratch,slot,2048,2048,&q) ||
                !sc64_worker_cargo_state_valid(&q)) return 0;
        }
        return 1;
    }
    return race_save_prepare_mineral_fifo(&scratch,2048,2048,&p->auth,&p->queue,eligible);
}
static int select_one(Plan *p,GbaSaveSelection *s) {
    return save_select_prepared(&scratch,sizeof scratch,tags,3,prepare,p,s);
}
static void row(RaceSave *s,unsigned slot,Unit q) {
    assert(race_save_encode_unit(s,slot,2048,2048,&q));
}
static RaceSave make_save(void) {
    RaceSave s={0}; s.version=INNER;
    Unit a=sample(), b=sample(), other=sample(), repair=sample();
    a.flags |= 0x20; b.flags |= 0x40;
    a.carry=5; a.work_timer=17; b.carry=2; b.work_timer=0;
    assert(sc64_packed_mineral_fifo_rank_unit(&a,1));
    assert(sc64_packed_mineral_fifo_rank_unit(&b,2));
    row(&s,7,a); row(&s,3,b);
    other.type=1; other.carry=137; other.work_timer=31; row(&s,9,other);
    repair.order=ORDER_REPAIR; repair.carry=137; repair.work_timer=31; row(&s,11,repair);
    return s;
}
static void check_winner(Plan *p) {
    Unit q, expected=sample();
    assert(p->queue.head[4]==8 && p->queue.next[7]==4 && p->queue.tail[4]==4 && !p->queue.next[3]);
    expected.flags |= 0x20; expected.carry=5; expected.work_timer=17;
    assert(READ(&q,&scratch.units[7],2048,2048)); assert(!memcmp(&q,&expected,sizeof q));
    expected.flags=(expected.flags & ~0x20) | 0x40; expected.carry=2; expected.work_timer=0;
    assert(READ(&q,&scratch.units[3],2048,2048)); assert(!memcmp(&q,&expected,sizeof q));
    expected=sample(); expected.type=1; expected.carry=137; expected.work_timer=31;
    assert(READ(&q,&scratch.units[9],2048,2048)); assert(!memcmp(&q,&expected,sizeof q));
    expected=sample(); expected.order=ORDER_REPAIR; expected.carry=137; expected.work_timer=31;
    assert(READ(&q,&scratch.units[11],2048,2048)); assert(!memcmp(&q,&expected,sizeof q));
}

static void check_legacy_predecessor(const RaceSave *bad)
{
    RaceSave legacy={.version=LEGACY_INNER};
    Unit first=sample(), second=first;
    first.work_timer=27;
    second.work_timer=18;
    row(&legacy,3,first);
    row(&legacy,7,second);
    guardian_test_reset();
    save_write(&legacy,sizeof legacy,LEGACY_TAG);
    Plan plan;
    GbaSaveSelection previous, selected;
    assert(select_one(&plan,&previous));
    assert(save_write_selected(bad,sizeof(*bad),FIFO_TAG,&previous,tags,3));
    assert(select_one(&plan,&selected));
    assert(selected.tag==LEGACY_TAG && selected.sequence==previous.sequence);
    assert(!memcmp(&scratch,&legacy,sizeof legacy));
    Sc64MineralFifo empty={0};
    assert(!memcmp(&plan.queue,&empty,sizeof empty));
}

int main(void) {
    sc64_race_unit_rules[WORKER].hp=200; sc64_race_unit_rules[1].hp=200;
    guardian_test_reset();
    RaceSave valid=make_save(), bad=valid; Plan p; GbaSaveSelection old,chosen;
    save_write(&valid,sizeof valid,FIFO_TAG);
    assert(select_one(&p,&old)); check_winner(&p);
    /* A checksum-valid newer duplicate rank must fall back to the older bank. */
    Unit q; assert(READ(&q,&bad.units[3],2048,2048)); q.carry=(1<<4)|2; row(&bad,3,q);
    assert(save_write_selected(&bad,sizeof bad,FIFO_TAG,&old,tags,3));
    assert(select_one(&p,&chosen)); assert(chosen.slot==old.slot); check_winner(&p);
    /* The half-range case must restore the valid winner's scratch and plan. */
    guardian_test_sequence(1-old.slot,old.sequence+0x80000000u);
    assert(select_one(&p,&chosen)); assert(chosen.slot==old.slot); check_winner(&p);
    check_legacy_predecessor(&bad);
    /* Valid first row followed by a malformed cargo nibble must reject. */
    bad=valid; assert(READ(&q,&bad.units[7],2048,2048)); q.carry=(1<<4)|9; row(&bad,7,q);
    guardian_test_reset(); save_write(&bad,sizeof bad,FIFO_TAG);
    assert(!select_one(&p,&chosen));
    /* Ranked mineral cargo matches the order and cannot be queued. */
    bad=valid; assert(READ(&q,&bad.units[7],2048,2048)); q.flags &= ~0x60; row(&bad,7,q);
    guardian_test_reset(); save_write(&bad,sizeof bad,FIFO_TAG);
    assert(!select_one(&p,&chosen));
    guardian_test_reset(); save_write(&valid,sizeof valid,FIFO_TAG);
    assert(select_one(&p,&old));
    bad=valid; assert(READ(&q,&bad.units[3],2048,2048)); q.carry=(1<<4)|2; row(&bad,3,q);
    assert(save_write_selected(&bad,sizeof bad,FIFO_TAG,&old,tags,3));
    guardian_test_corrupt(old.slot);
    static u8 before[32768], after[32768];
    guardian_test_snapshot(before);
    GbaSaveSelection previous=chosen;
    memset(&p,0xa5,sizeof p); assert(!select_one(&p,&chosen));
    guardian_test_snapshot(after);
    assert(!memcmp(before,after,sizeof before));
    assert(!memcmp(&chosen,&previous,sizeof chosen));
    /* A recognized outer tag without the authenticated feature is rejected. */
    guardian_test_reset(); save_write(&valid,sizeof valid,OTHER_TAG);
    assert(!select_one(&p,&chosen));
    /* A valid FIFO outer tag cannot authorize a different inner layout. */
    guardian_test_reset(); valid.version=LEGACY_INNER;
    save_write(&valid,sizeof valid,FIFO_TAG);
    assert(!select_one(&p,&chosen));
    puts("packed mineral FIFO journal passed"); return 0;
}
