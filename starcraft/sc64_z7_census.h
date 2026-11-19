#ifndef SC64_Z7_CENSUS_H
#define SC64_Z7_CENSUS_H

/* Z7 trigger counts gathered during live-actor validation.
 *
 * A prevalidated tick walks the actor pools once to prove every live actor's
 * source provenance (race_z7_validate_live_world) and then, for the triggers
 * that have not started, walks them again to count the five source
 * predicates (sc64_z7_count_snapshot_begin_masked). The second walk visits
 * the same actors with the same data and no writes in between, so this file
 * collects those counts from the first walk. Per-actor work is a table
 * lookup; only actors that can contribute a count are examined further.
 *
 * Failure behavior is unchanged: whenever the separate census would return 0
 * (raw id outside the source table, missing unit type, no bounds for a
 * region probe, unusable source rows or regions) the fused census publishes
 * nothing and also returns 0, so the caller falls back exactly as before.
 * The two-counter late case keeps its own bookkeeping inside validation.
 *
 * SC64_Z7_CENSUS_VERIFY (HOST) additionally runs the separate census on
 * every update and aborts if the result, counts or active mask differ. */

#if defined(SC64_Z7_PREVALIDATED_TICK) && defined(SC64_Z7_COUNT_SNAPSHOT) && \
    defined(SC64_Z7_PRUNE_STARTED)

#ifdef SC64_Z7_VALIDATED_CENSUS

enum {
    SC64_Z7_CENSUS_CURRENT=1,   /* need13: source current player */
    SC64_Z7_CENSUS_HOSTILE=2,   /* need19 bit 1: buildings in force 1 */
    SC64_Z7_CENSUS_HOSTILE_REGION=4, /* need19 bit 2: ... touching region 4 */
    SC64_Z7_CENSUS_FORCE0_A=8,  /* need18 bit 3: force 0 touching region 3 */
    SC64_Z7_CENSUS_FORCE0_B=16, /* need18 bit 4: force 0 touching region 5 */
    SC64_Z7_CENSUS_BAD_SLOT=128
};

typedef struct {
    u8 armed, done, fail, needed;
    u8 flag[8];
    u16 count[5];
    u16 left[5], top[5], right[5], bottom[5];
} Sc64Z7Census;

static Sc64Z7Census sc64_z7_census;

#ifdef SC64_Z7_CENSUS_VERIFY
#include <stdio.h>
#include <stdlib.h>
static unsigned long sc64_z7_census_verify_updates, sc64_z7_census_verify_fused,
    sc64_z7_census_verify_late, sc64_z7_census_verify_failed;
static unsigned long sc64_z7_census_verify_masks[32];
static void sc64_z7_census_verify_report(void)
{
    fprintf(stderr,"Z7 census verify: %lu pruned updates, %lu geometry-census checks "
            "(%lu with reference failure), %lu two-counter checks\n",
            sc64_z7_census_verify_updates,sc64_z7_census_verify_fused,
            sc64_z7_census_verify_failed,sc64_z7_census_verify_late);
    for (unsigned m=0;m<32;++m) if (sc64_z7_census_verify_masks[m])
        fprintf(stderr,"Z7 census verify: needed mask %u on %lu updates\n",
                m,sc64_z7_census_verify_masks[m]);
}
#endif

/* Decide, before validation, how this tick's counts will be produced.
 * Returns nonzero when validation should collect them (the caller then also
 * passes its two-counter buffer). A zero result leaves the separate census
 * to run unchanged. */
static int sc64_z7_census_arm(const Sc64Z7WorldContext *ctx,u16 started_mask)
{
    static const u8 trigger[5]={0,2,4,6,7};
    static const u8 opcode[5]={2,2,3,3,3};
    static const u8 selector[5]={13,19,19,18,18};
    static const u16 wanted[5]={229,231,231,229,229};
    static const u8 location[5]={0,0,4,3,5};
    static const u8 comparison[5]={1,1,1,0,0};
    static const u32 amount[5]={0,0,0,1,1};
    unsigned needed=sc64_z7_count_snapshot_needed(started_mask);
    unsigned left[5]={0},top[5]={0},right[5]={0},bottom[5]={0};
    Sc64Z7Census *c=&sc64_z7_census;
    c->armed=c->done=c->fail=0;
    if (!ctx || (started_mask&~0xffu)) return 0;
    if (needed==3u) return 1; /* late two-counter case: validation counts it */
    if (!needed || needed==31u || !ctx->source_tables_prevalidated) return 0;
    /* The same canonical row and region checks as the separate census. */
    for (unsigned q=0;q<5;++q) {
        const Sc64Z7SourceCondition *row;
        if (!sc64_z7_condition_row(trigger[q],0,&row) ||
            row->opcode!=opcode[q] || row->player!=selector[q] ||
            row->unit!=wanted[q] || row->location!=location[q] ||
            row->comparison!=comparison[q] || row->amount!=amount[q] ||
            row->resource || row->flags!=16u || row->mask ||
            (location[q] && !sc64_z7_world_location_source(ctx,location[q],
                &left[q],&top[q],&right[q],&bottom[q]))) return 0;
        if (left[q]>0xffffu || top[q]>0xffffu || right[q]>0xffffu ||
            bottom[q]>0xffffu) return 0;
        c->left[q]=(u16)left[q]; c->top[q]=(u16)top[q];
        c->right[q]=(u16)right[q]; c->bottom[q]=(u16)bottom[q];
        c->count[q]=0;
    }
    /* Per-slot predicate bits, restricted to the counters still needed. A
     * slot that is not an active source slot cannot pass validation; mark it
     * so that it would still fail closed. */
    for (unsigned slot=0;slot<8;++slot) {
        unsigned f=0;
        if (!ctx->source_slot_active[slot]) f=SC64_Z7_CENSUS_BAD_SLOT;
        else {
            if (slot==ctx->source_current_player) f|=SC64_Z7_CENSUS_CURRENT;
            if (ctx->source_force_for_slot[slot]==1u)
                f|=SC64_Z7_CENSUS_HOSTILE|SC64_Z7_CENSUS_HOSTILE_REGION;
            if (ctx->source_force_for_slot[slot]==0u)
                f|=SC64_Z7_CENSUS_FORCE0_A|SC64_Z7_CENSUS_FORCE0_B;
            f&=needed|SC64_Z7_CENSUS_BAD_SLOT;
        }
        c->flag[slot]=(u8)f;
    }
    c->needed=(u8)needed;
    c->armed=1;
    return 1;
}

static inline int sc64_z7_census_overlap(unsigned q,int al,int at,int ar,int ab)
{
    const Sc64Z7Census *c=&sc64_z7_census;
    return (int)c->left[q]<ar && al<(int)c->right[q] &&
           (int)c->top[q]<ab && at<(int)c->bottom[q];
}

/* One validated live unit. Mirrors the unit pass of the separate census. */
static inline void sc64_z7_census_unit(unsigned slot,int raw,int type,int x,int y)
{
    Sc64Z7Census *c=&sc64_z7_census;
    unsigned f=c->flag[slot&7u];
    if (!f) return;
    if ((f&SC64_Z7_CENSUS_BAD_SLOT) || raw<0 || raw>227 || type<0) { c->fail=1; return; }
    if (f&SC64_Z7_CENSUS_CURRENT) ++c->count[0];
    if (f&(SC64_Z7_CENSUS_FORCE0_A|SC64_Z7_CENSUS_FORCE0_B)) {
        int al,at,ar,ab;
        if (!sc64_z7_world_actor_rect((unsigned)raw,x,y,&al,&at,&ar,&ab)) { c->fail=1; return; }
        if ((f&SC64_Z7_CENSUS_FORCE0_A) && sc64_z7_census_overlap(3,al,at,ar,ab)) ++c->count[3];
        if ((f&SC64_Z7_CENSUS_FORCE0_B) && sc64_z7_census_overlap(4,al,at,ar,ab)) ++c->count[4];
    }
}

/* One validated live building. Mirrors the building pass. */
static inline void sc64_z7_census_building(unsigned slot,int raw,int x,int y)
{
    Sc64Z7Census *c=&sc64_z7_census;
    unsigned f=c->flag[slot&7u];
    int al,at,ar,ab;
    if (!f) return;
    if ((f&SC64_Z7_CENSUS_BAD_SLOT) || raw<0 || raw>227) { c->fail=1; return; }
    if (f&SC64_Z7_CENSUS_CURRENT) ++c->count[0];
    if (f&SC64_Z7_CENSUS_HOSTILE) ++c->count[1];
    if (!(f&(SC64_Z7_CENSUS_HOSTILE_REGION|SC64_Z7_CENSUS_FORCE0_A|
             SC64_Z7_CENSUS_FORCE0_B))) return;
    if (!sc64_z7_world_building_rect((unsigned)raw,x,y,&al,&at,&ar,&ab)) { c->fail=1; return; }
    if ((f&SC64_Z7_CENSUS_HOSTILE_REGION) && sc64_z7_census_overlap(2,al,at,ar,ab)) ++c->count[2];
    if ((f&SC64_Z7_CENSUS_FORCE0_A) && sc64_z7_census_overlap(3,al,at,ar,ab)) ++c->count[3];
    if ((f&SC64_Z7_CENSUS_FORCE0_B) && sc64_z7_census_overlap(4,al,at,ar,ab)) ++c->count[4];
}

/* Validation visited every live actor and accepted all of them. */
static inline void sc64_z7_census_done(void) { sc64_z7_census.done=sc64_z7_census.armed; }

#ifdef SC64_Z7_CENSUS_VERIFY
static void sc64_z7_census_verify_check(const Sc64Z7WorldContext *ctx,unsigned needed,
                                        int ref_ok,int fused,const u32 *late)
{
    const Sc64Z7Census *c=&sc64_z7_census;
    int bad=0;
    if (fused) {
        ++sc64_z7_census_verify_fused;
        if (c->fail) { ++sc64_z7_census_verify_failed; bad=ref_ok; }
        else {
            bad=!ref_ok || ctx->count_snapshot.active!=needed;
            for (unsigned q=0;q<5 && !bad;++q) bad=ctx->count_snapshot.count[q]!=c->count[q];
        }
    } else if (late) {
        ++sc64_z7_census_verify_late;
        bad=!ref_ok || ctx->count_snapshot.active!=3u ||
            ctx->count_snapshot.count[0]!=late[0] || ctx->count_snapshot.count[1]!=late[1];
    } else return;
    if (bad) {
        fprintf(stderr,"Z7 fused census mismatch: needed %u ref_ok %d fail %d fused %u/%u/%u/%u/%u "
                "ref %u/%u/%u/%u/%u\n",needed,ref_ok,fused?c->fail:-1,
                c->count[0],c->count[1],c->count[2],c->count[3],c->count[4],
                (unsigned)ctx->count_snapshot.count[0],(unsigned)ctx->count_snapshot.count[1],
                (unsigned)ctx->count_snapshot.count[2],(unsigned)ctx->count_snapshot.count[3],
                (unsigned)ctx->count_snapshot.count[4]);
        abort();
    }
}
#endif

/* Replacement for sc64_z7_count_snapshot_begin_pruned() in the tick. */
static int sc64_z7_census_begin_pruned(Sc64Z7WorldContext *ctx,u16 started_mask,
                                       const u32 *validated_late)
{
    Sc64Z7Census *c=&sc64_z7_census;
    int fused=c->armed && c->done;
    unsigned needed=sc64_z7_count_snapshot_needed(started_mask);
    int ok;
    c->armed=c->done=0;
#ifdef SC64_Z7_CENSUS_VERIFY
    if (!sc64_z7_census_verify_updates++) atexit(sc64_z7_census_verify_report);
    ++sc64_z7_census_verify_masks[needed&31u];
    ok=sc64_z7_count_snapshot_begin_pruned(ctx,started_mask,0);
    sc64_z7_census_verify_check(ctx,needed,ok,fused,validated_late);
    return ok;
#else
    if (fused) {
        ctx->count_snapshot.active=0;
        for (unsigned q=0;q<5;++q) ctx->count_snapshot.count[q]=0;
        if (c->fail) return 0;
        for (unsigned q=0;q<5;++q) ctx->count_snapshot.count[q]=c->count[q];
        ctx->count_snapshot.active=(u8)c->needed;
        return 1;
    }
    (void)needed; (void)ok;
    return sc64_z7_count_snapshot_begin_pruned(ctx,started_mask,validated_late);
#endif
}

#else /* !SC64_Z7_VALIDATED_CENSUS */
#define sc64_z7_census_begin_pruned(ctx,started,late) \
    sc64_z7_count_snapshot_begin_pruned((ctx),(started),(late))
#endif

#endif /* prevalidated pruned snapshot */
#endif
