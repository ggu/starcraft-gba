/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_GHOST_TECH_SAVE_WIRE_H
#define SC64_GHOST_TECH_SAVE_WIRE_H

/* Detached host experiment. The caller selects the layout from an
 * authenticated journal envelope. No game tag is assigned here, and an
 * existing transport or nuclear tag must never select the extension. */
#ifndef GBA_HOST
#error "Ghost tech save wire is a detached host-only experiment"
#endif
#include "sc64_ghost_tech.h"

enum {
    SC64_GHOST_SAVE_BASE,
    SC64_GHOST_SAVE_COVERT_CANDIDATE,
    SC64_GHOST_SAVE_CO_MAX = 2 * SC64_GHOST_TECH_OWNER_COUNT,
    SC64_GHOST_SAVE_BUILDINGS = 160,
    SC64_GHOST_SAVE_BANKS = 5,
    SC64_GHOST_SAVE_UPGRADES = 12
};
#define SC64_GHOST_SAVE_DAMAGE_WORD UINT32_C(0x42443031)
#define SC64_GHOST_SAVE_UPGRADE_BITS UINT32_C(0xcccccccc)

_Static_assert(SC64_GHOST_TECH_OWNER_COUNT == 8 &&
               SC64_GHOST_TECH_REACTOR == SC64_GHOST_TECH_OCULAR + 1,
               "Ghost tech upgrade projection");

typedef struct {
    uint32_t seeded;
    uint8_t allowed[SC64_GHOST_TECH_OWNER_COUNT]; /* source permission nibbles */
    uint8_t bank[SC64_GHOST_TECH_OWNER_COUNT]; /* mode-specific payer-to-bank map */
    unsigned active_banks, neutral_bank;
} Sc64GhostSavePolicy;

typedef struct {
    int mapped_raw, raw, alive, complete;
    int owner, key, parent_owner, parent_key, attached;
    int queue_empty;
} Sc64GhostSaveFacts;

typedef struct {
    int progress, level;
} Sc64GhostSaveRow;

typedef struct {
    Sc64GhostTechJob paid;
    uint8_t building, bank;
} Sc64GhostSaveReceipt;

typedef struct {
    Sc64GhostTechState tech;
    Sc64GhostSaveReceipt receipts[SC64_GHOST_SAVE_CO_MAX];
    uint32_t canonical_damage_word;
    unsigned count;
} Sc64GhostSavePlan;

static inline int sc64_ghost_save_policy_valid(const Sc64GhostSavePolicy *p)
{
    if (!p || (p->seeded & ~SC64_GHOST_SAVE_UPGRADE_BITS) ||
        p->neutral_bank >= SC64_GHOST_SAVE_BANKS ||
        (p->active_banks >> SC64_GHOST_SAVE_BANKS) ||
        (p->active_banks & (1u << p->neutral_bank))) return 0;
    for (unsigned k = 0; k < SC64_GHOST_TECH_OWNER_COUNT; ++k)
        if (p->allowed[k] & ~(unsigned)SC64_GHOST_SAVE_UPGRADES) return 0;
    return 1;
}

static inline uint32_t sc64_ghost_save_allowed(const Sc64GhostSavePolicy *p)
{
    uint32_t mask = 0;
    for (unsigned k = 0; k < SC64_GHOST_TECH_OWNER_COUNT; ++k)
        if (p->bank[k] < SC64_GHOST_SAVE_BANKS &&
            (p->active_banks & (1u << p->bank[k])))
            mask |= (uint32_t)p->allowed[k] << (k * 4);
    return mask;
}

static inline uint32_t sc64_ghost_save_rank(uint32_t bits)
{
    uint32_t rank = 0;
    for (unsigned k = 0; k < SC64_GHOST_TECH_OWNER_COUNT; ++k)
        rank |= ((bits >> (k * 4 + 2)) & 3u) << (k * 2);
    return rank;
}

static inline uint32_t sc64_ghost_save_unrank(uint32_t rank)
{
    uint32_t bits = 0;
    for (unsigned k = 0; k < SC64_GHOST_TECH_OWNER_COUNT; ++k)
        bits |= ((rank >> (k * 2)) & 3u) << (k * 4 + 2);
    return bits;
}

/* Project only earned deltas; source seeds are restored by the caller's
 * mission policy. Preserve the predecessor's zero or canonical marker. */
static inline int sc64_ghost_save_encode_header(uint32_t *out, unsigned format,
    uint32_t damage_word, const Sc64GhostSavePolicy *p, const Sc64GhostTechState *tech)
{
    if (!out || !sc64_ghost_save_policy_valid(p) ||
        !sc64_ghost_tech_state_valid(tech) || (tech->completed & p->seeded) != p->seeded)
        return 0;
    uint32_t earned = tech->completed & ~p->seeded;
    if ((earned | tech->pending) & ~sc64_ghost_save_allowed(p)) return 0;
    if (format == SC64_GHOST_SAVE_BASE) {
        if (earned || tech->pending || (damage_word && damage_word != SC64_GHOST_SAVE_DAMAGE_WORD))
            return 0;
    } else if (format == SC64_GHOST_SAVE_COVERT_CANDIDATE) {
        if ((!earned && !tech->pending) || damage_word != SC64_GHOST_SAVE_DAMAGE_WORD)
            return 0;
        damage_word ^= sc64_ghost_save_rank(earned);
    } else return 0;
    *out = damage_word;
    return 1;
}

static inline int sc64_ghost_save_claims_covert(const Sc64GhostSaveFacts *f)
{
    return f->mapped_raw == SC64_NUKE_COVERT_RAW || f->raw == SC64_NUKE_COVERT_RAW;
}

static inline int sc64_ghost_save_valid_covert(const Sc64GhostSaveFacts *f)
{
    return f->mapped_raw == SC64_NUKE_COVERT_RAW &&
        (f->raw == -1 || f->raw == SC64_NUKE_COVERT_RAW);
}

/* Preserve only receipts with an independently validated live socket and
 * payer. A detached or foreign-controlled Covert Ops is deliberately
 * refused until provenance and resume behavior can be independently shown.
 * The bank can be derived again on load; the paid key is carried on wire. */
static inline int sc64_ghost_save_receipt_valid(const Sc64GhostSavePolicy *p,
    const Sc64GhostSaveFacts *f, const Sc64GhostTechJob *job)
{
    const Sc64GhostTechFact *fact = sc64_ghost_tech_fact(job->kind);
    return fact && fact->upgrade && job->payer < SC64_GHOST_TECH_OWNER_COUNT &&
        job->remaining && job->remaining <= fact->source_ticks &&
        sc64_ghost_save_valid_covert(f) && f->alive == 1 && f->complete == 1 &&
        f->attached == 1 && f->queue_empty == 1 && f->key == job->payer &&
        f->parent_key == f->key && f->parent_owner == f->owner &&
        f->owner >= 0 && f->owner < SC64_GHOST_SAVE_BANKS && f->owner == p->bank[job->payer] &&
        (p->active_banks & (1u << (unsigned)f->owner)) &&
        (p->allowed[job->payer] & (1u << (job->kind - 1)));
}

/* Row encoder for the writer's scratch only. The ordinary building codec
 * still encodes all other fields. level=1..16 fits its 7-bit field, and the
 * unscaled timer fits even the legacy signed-16 queue_progress field. */
static inline int sc64_ghost_save_encode_receipt(Sc64GhostSaveRow *out,
    const Sc64GhostSaveRow *base, const Sc64GhostSavePolicy *p,
    const Sc64GhostSaveFacts *facts, const Sc64GhostSaveReceipt *receipt)
{
    if (!out || !base || base->level || base->progress || !facts || !receipt ||
        !sc64_ghost_save_policy_valid(p) || receipt->building >= SC64_GHOST_SAVE_BUILDINGS ||
        receipt->bank != facts->owner ||
        !sc64_ghost_save_receipt_valid(p, facts, &receipt->paid)) return 0;
    *out = (Sc64GhostSaveRow){receipt->paid.remaining,
        1 + 2 * receipt->paid.payer + receipt->paid.kind - SC64_GHOST_TECH_OCULAR};
    return 1;
}

/* Validate the whole detached projection before publishing it. Non-Covert
 * rows (including the Silo's receipt and any nuclear missile unit row) are
 * left to their existing codecs. Normalization is only legal after success. */
static inline int sc64_ghost_save_prepare(Sc64GhostSavePlan *out, unsigned format,
    uint32_t word, const Sc64GhostSavePolicy *p, const Sc64GhostSaveFacts *facts,
    const Sc64GhostSaveRow *rows, unsigned count)
{
    if (!out || !facts || !rows || count > SC64_GHOST_SAVE_BUILDINGS ||
        !sc64_ghost_save_policy_valid(p)) return 0;
    Sc64GhostSavePlan plan = {0};
    plan.tech.completed = p->seeded;
    uint32_t earned = 0;
    if (format == SC64_GHOST_SAVE_BASE) {
        if (word && word != SC64_GHOST_SAVE_DAMAGE_WORD) return 0;
        plan.canonical_damage_word = word;
    } else if (format == SC64_GHOST_SAVE_COVERT_CANDIDATE) {
        uint32_t projection = word ^ SC64_GHOST_SAVE_DAMAGE_WORD;
        if (projection >> 16) return 0;
        earned = sc64_ghost_save_unrank(projection);
        if ((earned & p->seeded) || (earned & ~sc64_ghost_save_allowed(p))) return 0;
        plan.tech.completed |= earned;
        plan.canonical_damage_word = SC64_GHOST_SAVE_DAMAGE_WORD;
    } else return 0;
    for (unsigned i = 0; i < count; ++i) {
        const Sc64GhostSaveFacts *f = &facts[i];
        const Sc64GhostSaveRow *row = &rows[i];
        if (!sc64_ghost_save_claims_covert(f)) continue;
        if (!sc64_ghost_save_valid_covert(f) || f->queue_empty != 1) return 0;
        if (!row->level && !row->progress) continue;
        if (format != SC64_GHOST_SAVE_COVERT_CANDIDATE ||
            row->level <= 0 || row->level > SC64_GHOST_SAVE_CO_MAX ||
            row->progress <= 0 || row->progress > UINT16_MAX ||
            plan.count >= SC64_GHOST_SAVE_CO_MAX) return 0;
        unsigned packed = (unsigned)row->level - 1;
        Sc64GhostTechJob job = {(uint16_t)row->progress,
            (uint8_t)(SC64_GHOST_TECH_OCULAR + (packed & 1u)), (uint8_t)(packed >> 1)};
        uint32_t bit = sc64_ghost_tech_bit(job.payer, job.kind);
        if (!sc64_ghost_save_receipt_valid(p, f, &job) ||
            ((plan.tech.completed | plan.tech.pending) & bit)) return 0;
        plan.tech.pending |= bit;
        plan.receipts[plan.count++] = (Sc64GhostSaveReceipt){job, (uint8_t)i, (uint8_t)f->owner};
    }
    if (format == SC64_GHOST_SAVE_COVERT_CANDIDATE && !earned && !plan.count) return 0;
    *out = plan;
    return 1;
}

#endif
