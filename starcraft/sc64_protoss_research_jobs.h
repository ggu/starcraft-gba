/* SPDX-License-Identifier: GPL-3.0-or-later
 * Host-only integration stage for producer-bound custom research. This is not
 * a save format and must not be enabled in a ROM before job/tag admission,
 * lifecycle hooks, UI, combat and ordinary-play qualification are complete.
 * Included after the actual Building definition and race building enums. */
#ifndef SC64_PROTOSS_RESEARCH_JOBS_H
#define SC64_PROTOSS_RESEARCH_JOBS_H
#include <limits.h>
#if !defined(GBA_HOST) || !defined(SC64_RACES) || !defined(SC64_EXPERIMENTAL_RESEARCH_JOURNAL)
#error "Research job integration stage is HOST-only and requires the research journal"
#endif

/* level = family + 1; queue[0] = compact payer; queue[1] = target level.
 * queue_count/head and the remaining entries must be zero. These are live
 * staged fields only, not an authenticated persisted encoding. Campaign
 * current-controller binding is intentionally unsupported here: raw origin
 * remains trigger provenance and is not a controller identity. */
static int sc64_pr_job_family_producer(unsigned family, int type)
{
    if (family == SC64_PR_GROUND_ARMOR || family == SC64_PR_GROUND_WEAPONS ||
        family == SC64_PR_SHIELDS) return type == B_FORGE;
    if (family == SC64_PR_AIR_ARMOR || family == SC64_PR_AIR_WEAPONS ||
        family == SC64_PR_SINGULARITY) return type == B_CYBERNETICS_CORE;
    return 0;
}

/* Existing generic levels remain floors for both corresponding typed families;
 * a pending old job blocks new jobs of that group. No shields or range are
 * invented, and loading an old save does not seed or convert the new levels. */
static unsigned sc64_pr_effective_level(unsigned family, unsigned typed,
                                        unsigned legacy_weapons, unsigned legacy_armor)
{
    unsigned floor = 0;
    if (family == SC64_PR_GROUND_WEAPONS || family == SC64_PR_AIR_WEAPONS)
        floor = legacy_weapons;
    if (family == SC64_PR_GROUND_ARMOR || family == SC64_PR_AIR_ARMOR)
        floor = legacy_armor;
    return typed > floor ? typed : floor;
}

/* Caller supplies independently checked custom owner activity, power,
 * prerequisite, scenario maximum and a complete scan for duplicate jobs. */
static int sc64_pr_job_begin_custom(Building *producer, unsigned owner,
    unsigned family, Sc64ProtossResearchLevels *levels, int *minerals, int *gas,
    unsigned max_level, int owner_active, int powered, int prerequisite,
    int duplicate, unsigned legacy_weapons, unsigned legacy_armor,
    int legacy_weapons_pending, int legacy_armor_pending)
{
    Sc64ProtossResearchQuote quote;
    unsigned level;
    if (!producer || !levels || !minerals || !gas || minerals == gas || owner >= SC64_PR_CUSTOM_OWNERS ||
        family >= SC64_PR_FAMILY_COUNT || !owner_active || !powered || !prerequisite || duplicate ||
        !producer->alive || producer->under_construction || producer->owner != (int)owner ||
        !sc64_pr_job_family_producer(family, producer->type) || producer->queue_count ||
        producer->queue_head || producer->queue_progress || producer->level ||
        producer->queue[0] || producer->queue[1] || producer->queue[2] ||
        producer->queue[3] || producer->queue[4] ||
        !sc64_pr_levels_valid(levels, SC64_PR_CUSTOM_OWNERS) ||
        legacy_weapons > 2 || legacy_armor > 2 ||
        (owner != SIDE && (legacy_weapons || legacy_armor ||
                           legacy_weapons_pending || legacy_armor_pending))) return 0;
    if ((legacy_weapons_pending && (family == SC64_PR_GROUND_WEAPONS || family == SC64_PR_AIR_WEAPONS)) ||
        (legacy_armor_pending && (family == SC64_PR_GROUND_ARMOR || family == SC64_PR_AIR_ARMOR))) return 0;
    level = sc64_pr_effective_level(family, levels->level[owner][family], legacy_weapons, legacy_armor);
    if (level >= max_level || !sc64_pr_quote(family, level, &quote) ||
        *minerals < quote.mineral || *gas < quote.gas) return 0;
    *minerals -= quote.mineral; *gas -= quote.gas;
    producer->level = family + 1; producer->queue[0] = owner;
    producer->queue[1] = level + 1; producer->queue_progress = quote.ticks;
    return 1;
}

/* Validate before every tick or refund. If ownership changed, callers must
 * suspend and resolve the original authenticated payer; never use the new
 * owner's wallet. No mutation occurs for malformed or ambiguous state. */
static int sc64_pr_job_valid_custom(const Building *p, unsigned payer,
    const Sc64ProtossResearchLevels *levels, unsigned legacy_weapons,
    unsigned legacy_armor, int legacy_weapons_pending, int legacy_armor_pending)
{
    Sc64ProtossResearchQuote quote;
    unsigned family, target;
    if (!p || !levels || payer >= SC64_PR_CUSTOM_OWNERS || !p->alive ||
        p->under_construction || p->owner != (int)payer || p->queue_count || p->queue_head ||
        p->queue[0] != (int)payer || p->queue[2] || p->queue[3] || p->queue[4] ||
        p->level < 1 || p->level > SC64_PR_FAMILY_COUNT || !p->queue_progress ||
        !sc64_pr_levels_valid(levels, SC64_PR_CUSTOM_OWNERS) || legacy_weapons > 2 || legacy_armor > 2 ||
        (payer != SIDE && (legacy_weapons || legacy_armor ||
                           legacy_weapons_pending || legacy_armor_pending))) return 0;
    family = (unsigned)p->level - 1; target = (unsigned)p->queue[1];
    if ((legacy_weapons_pending && (family == SC64_PR_GROUND_WEAPONS || family == SC64_PR_AIR_WEAPONS)) ||
        (legacy_armor_pending && (family == SC64_PR_GROUND_ARMOR || family == SC64_PR_AIR_ARMOR))) return 0;
    if (!target || !sc64_pr_job_family_producer(family,p->type) ||
        sc64_pr_effective_level(family,levels->level[payer][family],legacy_weapons,legacy_armor) != target-1 ||
        !sc64_pr_quote(family,target-1,&quote) || p->queue_progress > quote.ticks) return 0;
    return 1;
}

/* Power loss pauses. Completion increments only the authenticated family. */
static int sc64_pr_job_tick_custom(Building *p, unsigned payer,
    Sc64ProtossResearchLevels *levels, unsigned legacy_weapons, unsigned legacy_armor,
    int legacy_weapons_pending, int legacy_armor_pending, int powered)
{
    if (!sc64_pr_job_valid_custom(p,payer,levels,legacy_weapons,legacy_armor,
                                  legacy_weapons_pending,legacy_armor_pending)) return -1;
    if (!powered) return 0;
    if (--p->queue_progress) return 0;
    levels->level[payer][(unsigned)p->level-1] = (uint8_t)p->queue[1];
    p->level=0; p->queue[0]=0; p->queue[1]=0;
    return 1;
}
/* Volatile HOST-only receipt; it is never inferred from a decoded row. */
typedef struct {
    uint16_t mineral, gas;
    uint8_t live, payer, family, target;
} Sc64PrPaidReceipt;

/* Settlement validates paid metadata rather than current completed levels:
 * a later external level change must not steal or duplicate the refund. */
static int sc64_pr_job_receipt_valid(const Building *p, unsigned payer,
                                    const Sc64PrPaidReceipt *paid)
{
    Sc64ProtossResearchQuote quote;
    if (!p || !paid || paid->live!=1 || payer>=SC64_PR_CUSTOM_OWNERS ||
        paid->payer!=payer || paid->family>=SC64_PR_FAMILY_COUNT || !paid->target ||
        !p->alive || p->under_construction || p->owner!=(int)payer ||
        p->queue_count || p->queue_head || p->queue[0]!=(int)payer ||
        p->queue[1]!=(int)paid->target || p->queue[2] || p->queue[3] || p->queue[4] ||
        p->level!=(int)paid->family+1 || !p->queue_progress ||
        !sc64_pr_job_family_producer(paid->family,p->type) ||
        (p->raw_type>=0 && p->raw_type!=sc64_race_raw_building(p->type)) ||
        !sc64_pr_quote(paid->family,paid->target-1,&quote) ||
        p->queue_progress>quote.ticks || paid->mineral!=quote.mineral || paid->gas!=quote.gas) return 0;
    return 1;
}

/* The 3/4 destruction fraction is pinned PC-source behavior; N64 remains
 * unverified. Refund the recorded original payment once, or mutate nothing. */
static int sc64_pr_job_cancel_custom(Building *p, unsigned payer, Sc64PrPaidReceipt *paid,
    int destroyed, int *minerals, int *gas, int wallet_limit)
{
    int ore, vespene;
    if (!minerals || !gas || minerals == gas || wallet_limit < 0 ||
        *minerals < 0 || *gas < 0 || *minerals > wallet_limit || *gas > wallet_limit ||
        (destroyed != 0 && destroyed != 1) || !sc64_pr_job_receipt_valid(p,payer,paid)) return 0;
    ore=destroyed ? (int)paid->mineral*3/4 : (int)paid->mineral;
    vespene=destroyed ? (int)paid->gas*3/4 : (int)paid->gas;
    if (ore > wallet_limit-*minerals || vespene > wallet_limit-*gas) return 0;
    *minerals+=ore; *gas+=vespene;
    p->level=0; p->queue[0]=0; p->queue[1]=0; p->queue_progress=0;
    paid->live=0;
    return 1;
}
/* Destruction must not be vetoed by a full bank. Each resource saturates at
 * its established representation limit; malformed receipts are no-ops. */
static int sc64_pr_job_destroy_custom(Building *p,unsigned payer,Sc64PrPaidReceipt *paid,
    int *minerals,int *gas,int wallet_limit)
{
    int ore,vespene,room;
    if (!minerals || !gas || minerals==gas || wallet_limit<0 ||
        *minerals<0 || *gas<0 || *minerals>wallet_limit || *gas>wallet_limit ||
        !sc64_pr_job_receipt_valid(p,payer,paid)) return 0;
    ore=(int)paid->mineral*3/4; vespene=(int)paid->gas*3/4;
    room=wallet_limit-*minerals; if (ore>room) ore=room;
    room=wallet_limit-*gas; if (vespene>room) vespene=room;
    *minerals+=ore; *gas+=vespene;
    p->level=0; p->queue[0]=0; p->queue[1]=0; p->queue_progress=0;
    paid->live=0;
    return 1;
}

/* Detached custom candidate audit. The caller supplies only candidate
 * context and may feed decoded rows one at a time; no live globals or full
 * second Building array are needed. This does not itself authenticate a tag. */
typedef struct {
    const Sc64ProtossResearchLevels *levels;
    const int *players;
    unsigned bots, legacy_weapons, legacy_armor;
    int pending_weapons, pending_armor;
    uint8_t seen[SC64_PR_CUSTOM_OWNERS][SC64_PR_FAMILY_COUNT];
} Sc64PrCandidateScan;
static int sc64_pr_candidate_scan_init(Sc64PrCandidateScan *scan,
    const Sc64ProtossResearchLevels *levels, const int players[5], unsigned bots,
    unsigned legacy_weapons, unsigned legacy_armor, int pending_weapons, int pending_armor)
{
    if (!scan || !levels || !players || bots<1 || bots>3 ||
        legacy_weapons>2 || legacy_armor>2 || !sc64_pr_levels_valid(levels,SC64_PR_CUSTOM_OWNERS)) return 0;
    for (unsigned owner=0;owner<SC64_PR_CUSTOM_OWNERS;++owner) {
        int active=owner==SIDE || (owner==ENEMY && bots>=1) ||
                   (owner==RESCUABLE && bots>=2) || (owner==4 && bots>=3);
        for (unsigned family=0;family<SC64_PR_FAMILY_COUNT;++family)
            if (levels->level[owner][family] && (!active || players[owner]!=R_PROTOSS)) return 0;
    }
    memset(scan,0,sizeof(*scan)); scan->levels=levels; scan->players=players;
    scan->bots=bots; scan->legacy_weapons=legacy_weapons; scan->legacy_armor=legacy_armor;
    scan->pending_weapons=pending_weapons; scan->pending_armor=pending_armor;
    return 1;
}
static int sc64_pr_candidate_scan_row(Sc64PrCandidateScan *scan,const Building *q)
{
    unsigned owner,family;
    if (!scan || !q || !scan->levels || !scan->players) return 0;
    if (q->type!=B_FORGE && q->type!=B_CYBERNETICS_CORE) return 1;
    /* A reused producer can retain inert queue entries from its previous
     * occupant. Preserve them until a successful new job admission. */
    if (!q->level && !q->queue_progress && !q->queue_count) return 1;
    owner=(unsigned)q->owner;
    if (owner>=SC64_PR_CUSTOM_OWNERS ||
        !(owner==SIDE || (owner==ENEMY && scan->bots>=1) ||
          (owner==RESCUABLE && scan->bots>=2) || (owner==4 && scan->bots>=3)) ||
        scan->players[owner]!=R_PROTOSS || q->level<1 || q->level>SC64_PR_FAMILY_COUNT ||
        (q->raw_type>=0 && q->raw_type!=sc64_race_raw_building(q->type))) return 0;
    family=(unsigned)q->level-1;
    if (scan->seen[owner][family] ||
        !sc64_pr_job_valid_custom(q,owner,scan->levels,owner==SIDE?scan->legacy_weapons:0,
            owner==SIDE?scan->legacy_armor:0,owner==SIDE&&scan->pending_weapons,
            owner==SIDE&&scan->pending_armor)) return 0;
    scan->seen[owner][family]=1;
    return 1;
}
static int sc64_pr_job_candidate_valid_custom(const Building *rows, unsigned count,
    const Sc64ProtossResearchLevels *levels, const int players[5], unsigned bots,
    unsigned legacy_weapons, unsigned legacy_armor, int pending_weapons, int pending_armor)
{
    Sc64PrCandidateScan scan;
    if (!rows || count>MAX_BUILDINGS ||
        !sc64_pr_candidate_scan_init(&scan,levels,players,bots,legacy_weapons,legacy_armor,
                                     pending_weapons,pending_armor)) return 0;
    for (unsigned i=0;i<count;++i) if (!sc64_pr_candidate_scan_row(&scan,&rows[i])) return 0;
    return 1;
}

#endif
