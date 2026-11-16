/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_GHOST_TECH_JOURNAL_H
#define SC64_GHOST_TECH_JOURNAL_H

#include "sc64_ghost_tech_save_wire.h"

#define SC64_GHOST_COVERT_TAG SC64_SAVE_TAG_GHOST_COVERT(SC64_TRANSPORT_SAVE_VERSION)
_Static_assert(SC64_GHOST_SAVE_DAMAGE_WORD == SC64_BUILDING_DAMAGE_SAVE_MARKER &&
               MAX_BUILDINGS <= SC64_GHOST_SAVE_BUILDINGS, "Covert journal field projection");

typedef struct {
    Sc64GhostSavePlan projected;
    u8 parents[SC64_GHOST_SAVE_CO_MAX];
} Sc64GhostJournalPlan;

/* There are no source tech seeds and the payer key equals the occupied
 * combat seat in a custom game. Do not extend these rules to campaign. */
static Sc64GhostSavePolicy race_ghost_save_policy(const RaceSave *candidate)
{
    Sc64GhostSavePolicy policy = {.neutral_bank=NEUTRAL};
    Sc64NukeSaveWorld world = race_nuke_save_world(candidate->state[0],candidate->state[4],1,1);
    for (unsigned i=0;i<SC64_GHOST_TECH_OWNER_COUNT;++i) {
        policy.bank[i]=(u8)i;
        if (i<SC64_GHOST_SAVE_BANKS && (world.owner_mask & (1u<<i)) &&
            candidate->players[i]==R_TER) {
            policy.active_banks|=1u<<i;
            policy.allowed[i]=SC64_GHOST_LIVE_UPGRADES;
        }
    }
    return policy;
}

static Sc64GhostSaveFacts race_ghost_save_facts(const Building *q)
{
    int empty = !q->queue_count && !q->queue_head;
    for (unsigned i=0;i<5;++i) if (q->queue[i]) empty=0;
    return (Sc64GhostSaveFacts){sc64_race_raw_building(q->type),q->raw_type,q->alive,
        q->alive && !q->under_construction,q->owner,q->owner,-1,-1,0,empty};
}

/* Inspect only the serialized world. Selecting the first matching structure
 * would silently admit ambiguous receipts or two overlapping Covert Ops. */
static int race_ghost_save_find_parent(const RaceSave *candidate, unsigned slot,
                                       const Building *q)
{
    int parent=-1;
    for (unsigned i=0;i<MAX_BUILDINGS;++i) {
        Building row;
        if (i==slot) continue;
        if (!race_save_decode_building(candidate,i,&row)) return -1;
        if (!row.alive) continue;
        int raw=sc64_race_raw_building(row.type);
        if (raw==SC64_NUKE_COVERT_RAW && row.x==q->x && row.y==q->y) return -1;
        if (raw!=116 || row.owner!=q->owner || row.x+48!=q->x || row.y+8!=q->y ||
            row.under_construction || (row.flags&4)) continue;
        if ((row.raw_type!=-1 && row.raw_type!=116) || parent>=0) return -1;
        parent=(int)i;
    }
    return parent;
}

static int race_ghost_save_energy_valid(const RaceSave *candidate,
    const Sc64GhostTechState *tech, unsigned width, unsigned height)
{
    for (unsigned i=0;i<MAX_UNITS;++i) {
        Unit q;
        if (!SC64_GUARD_DECODE(candidate,i,width,height,&q)) return 0;
        if (!q.alive || !sc64_race_ordinary_ghost(q.type)) continue;
        if (q.carry<0 || q.carry>sc64_ghost_energy_cap(tech,(unsigned)q.owner)) return 0;
    }
    return 1;
}

static int race_ghost_save_write(void)
{
    Sc64GhostSavePolicy policy=race_ghost_save_policy(&race_data);
    Sc64NukeSaveWorld world=race_nuke_save_world(1,race_bots,WORLD_W,WORLD_H);
    u32 word, pending=0;
    if (!sc64_ghost_save_encode_header(&word,SC64_GHOST_SAVE_COVERT_CANDIDATE,
            (u32)race_data.state[SC64_BUILDING_DAMAGE_SAVE_MARKER_INDEX],&policy,&sc64_ghost_tech) ||
        !race_ghost_save_energy_valid(&race_data,&sc64_ghost_tech,WORLD_W,WORLD_H)) return 0;
    for (unsigned i=0;i<SC64_GHOST_WORLD_JOB_LIMIT;++i) {
        const Sc64GhostCovertJob *job=&sc64_ghost_jobs[i];
        if (!job->slot_plus_one) continue;
        unsigned slot=(unsigned)job->slot_plus_one-1u;
        Building row;
        if (!race_save_decode_building(&race_data,slot,&row)) return 0;
        Sc64GhostSaveFacts facts=race_ghost_save_facts(&row);
        if (race_ghost_save_find_parent(&race_data,slot,&row)<0) return 0;
        facts.attached=1;
        facts.parent_key=facts.parent_owner=row.owner;
        Sc64GhostSaveReceipt receipt={job->paid,(u8)slot,job->bank};
        Sc64GhostSaveRow plain={row.queue_progress,row.level}, paid;
        u32 bit=sc64_ghost_tech_bit(job->paid.payer,job->paid.kind);
        if (!bit || (pending&bit) ||
            !sc64_ghost_save_encode_receipt(&paid,&plain,&policy,&facts,&receipt)) return 0;
        pending|=bit;
        row.queue_progress=paid.progress; row.level=paid.level;
        if (!race_nuke_save_replace_building(&race_data,slot,&row,&world)) return 0;
    }
    if (pending!=sc64_ghost_tech.pending) return 0;
    race_data.state[SC64_BUILDING_DAMAGE_SAVE_MARKER_INDEX]=(int)word;
    return 1;
}

/* Only the authenticated, checksum-selected scratch can enter the extended
 * path. Restore the ordinary fields before any of its validators see them. */
static int race_ghost_save_prepare(Sc64GhostJournalPlan *out, int extended)
{
    if (race_data.state[0]!=1) {
        if (extended) return 0;
        *out=(Sc64GhostJournalPlan){0};
        return 1;
    }
    Sc64GhostSavePolicy policy=race_ghost_save_policy(&race_data);
    Sc64GhostSaveScan scan;
    u8 parents[SC64_GHOST_SAVE_CO_MAX];
    if (!sc64_ghost_save_scan_begin(&scan,
            extended ? SC64_GHOST_SAVE_COVERT_CANDIDATE : SC64_GHOST_SAVE_BASE,
            (u32)race_data.state[SC64_BUILDING_DAMAGE_SAVE_MARKER_INDEX],&policy,MAX_BUILDINGS))
        return 0;
    for (unsigned i=0;i<MAX_BUILDINGS;++i) {
        Building row;
        if (!race_save_decode_building(&race_data,i,&row)) return 0;
        Sc64GhostSaveFacts facts=race_ghost_save_facts(&row);
        Sc64GhostSaveRow fields={row.queue_progress,row.level};
        int parent=-1;
        if (extended && sc64_ghost_save_claims_covert(&facts) &&
            (fields.progress || fields.level)) {
            parent=race_ghost_save_find_parent(&race_data,i,&row);
            if (parent<0) return 0;
            facts.attached=1;
            facts.parent_key=facts.parent_owner=row.owner;
        }
        unsigned next_receipt=scan.plan.count;
        if (!sc64_ghost_save_scan_row(&scan,&facts,&fields)) return 0;
        if (scan.plan.count!=next_receipt) {
            Sc64PackedBuildingSave normalized;
            if (parent<0) return 0;
            parents[next_receipt]=(u8)parent;
            row.queue_progress=row.level=0;
            if (!sc64_building_save_encode(&normalized,&row,2048,2048)) return 0;
        }
    }
    if (!sc64_ghost_save_scan_complete(&scan)) return 0;
    /* Only now edit scratch. Each receipt names a different row. Its
     * decoder and normalized encoder already passed on these same bytes. */
    for (unsigned i=0;i<scan.plan.count;++i) {
        unsigned slot=scan.plan.receipts[i].building;
        Building row;
        if (!race_save_decode_building(&race_data,slot,&row)) return 0;
        row.queue_progress=row.level=0;
        if (!sc64_building_save_encode(&race_data.buildings[slot],&row,2048,2048)) return 0;
    }
    race_data.state[SC64_BUILDING_DAMAGE_SAVE_MARKER_INDEX]=(int)scan.plan.canonical_damage_word;
    out->projected=scan.plan;
    memcpy(out->parents,parents,scan.plan.count*sizeof(*parents));
    return 1;
}

static void race_ghost_save_publish(const Sc64GhostJournalPlan *plan)
{
    race_ghost_tech_reset();
    if (race_mode!=1) return;
    sc64_ghost_tech=plan->projected.tech;
    for (unsigned i=0;i<plan->projected.count;++i) {
        const Sc64GhostSaveReceipt *receipt=&plan->projected.receipts[i];
        sc64_ghost_jobs[i]=(Sc64GhostCovertJob){receipt->paid,receipt->bank,
                    (u8)(receipt->building+1),(u8)(plan->parents[i]+1)};
    }
}
#endif
