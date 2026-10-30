/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_IMPACT_EVENTS_H
#define SC64_IMPACT_EVENTS_H

#include <stdint.h>

/* Bounded launch-to-impact queue for ordinary moving projectiles.
 *
 * A launch records the weapon packet that the immediate path would have
 * applied; the engine resolves it on the update whose presentation reaches
 * the projectile's impact. Live rows stay compact and in launch order, so
 * impacts due on the same update resolve in launch order and a save can
 * restore the queue exactly. The queue never drops or advances damage: a
 * full queue refuses the launch and the caller keeps its weapon ready.
 *
 * This header holds no game state. Callers own the row storage, its
 * capacity and the resolution callback. */

#define SC64_IMPACT_TARGET_MASK 0x01ffu
#define SC64_IMPACT_TARGET_LIMIT 512u
#define SC64_IMPACT_OWNER_SHIFT 9u
#define SC64_IMPACT_OWNER_LIMIT 8u
#define SC64_IMPACT_CLASS_SHIFT 12u
#define SC64_IMPACT_CLASS_LIMIT 7     /* stored as class+1; 0 is the legacy path */
#define SC64_IMPACT_DAMAGE_MASK 0x0fffu
#define SC64_IMPACT_DAMAGE_LIMIT 4096
#define SC64_IMPACT_HITS_SHIFT 12u
#define SC64_IMPACT_HITS_LIMIT 16
#define SC64_IMPACT_TYPE_LIMIT 64u
#define SC64_IMPACT_MAX_STEPS 255u
#define SC64_IMPACT_CAPACITY_LIMIT 255u

typedef struct {
    uint16_t target_meta;  /* target | owner << 9 | (damage class + 1) << 12 */
    uint16_t damage_hits;  /* per-impact damage | (impacts - 1) << 12 */
    uint8_t steps;         /* end-of-update steps left; zero never appears */
    uint8_t source_type;   /* engine actor type that launched the weapon */
} Sc64ImpactEvent;

typedef struct {
    int target;
    int damage;
    int hits;
    int owner;
    int damage_class;      /* -1 selects the legacy, non-stock damage path */
    int source_type;
} Sc64ImpactHit;

typedef enum {
    SC64_IMPACT_QUEUED = 0,
    SC64_IMPACT_FULL,         /* caller must not launch this update */
    SC64_IMPACT_UNREPRESENTABLE
} Sc64ImpactPushResult;

static inline int
sc64_impact_pack(Sc64ImpactEvent *out, const Sc64ImpactHit *hit, unsigned steps)
{
    if (!out || !hit || steps < 1u || steps > SC64_IMPACT_MAX_STEPS ||
        hit->target < 0 || (unsigned)hit->target >= SC64_IMPACT_TARGET_LIMIT ||
        hit->owner < 0 || (unsigned)hit->owner >= SC64_IMPACT_OWNER_LIMIT ||
        hit->damage_class < -1 || hit->damage_class >= SC64_IMPACT_CLASS_LIMIT ||
        hit->damage < 0 || hit->damage >= SC64_IMPACT_DAMAGE_LIMIT ||
        hit->hits < 1 || hit->hits > SC64_IMPACT_HITS_LIMIT ||
        hit->source_type < 0 || (unsigned)hit->source_type >= SC64_IMPACT_TYPE_LIMIT)
        return 0;
    out->target_meta = (uint16_t)((unsigned)hit->target |
        ((unsigned)hit->owner << SC64_IMPACT_OWNER_SHIFT) |
        ((unsigned)(hit->damage_class + 1) << SC64_IMPACT_CLASS_SHIFT));
    out->damage_hits = (uint16_t)((unsigned)hit->damage |
        ((unsigned)(hit->hits - 1) << SC64_IMPACT_HITS_SHIFT));
    out->steps = (uint8_t)steps;
    out->source_type = (uint8_t)hit->source_type;
    return 1;
}

static inline void
sc64_impact_unpack(const Sc64ImpactEvent *e, Sc64ImpactHit *hit)
{
    hit->target = (int)(e->target_meta & SC64_IMPACT_TARGET_MASK);
    hit->owner = (int)((e->target_meta >> SC64_IMPACT_OWNER_SHIFT) & 7u);
    hit->damage_class = (int)(e->target_meta >> SC64_IMPACT_CLASS_SHIFT) - 1;
    hit->damage = (int)(e->damage_hits & SC64_IMPACT_DAMAGE_MASK);
    hit->hits = (int)(e->damage_hits >> SC64_IMPACT_HITS_SHIFT) + 1;
    hit->source_type = e->source_type;
}

static inline int
sc64_impact_event_target(const Sc64ImpactEvent *e)
{
    return (int)(e->target_meta & SC64_IMPACT_TARGET_MASK);
}

/* Structural row check shared by restore and tests. `target_limit` and
 * `type_limit` are the caller's actor namespaces. */
static inline int
sc64_impact_event_valid(const Sc64ImpactEvent *e, unsigned target_limit,
                        unsigned owner_limit, unsigned type_limit)
{
    unsigned class_bits = (unsigned)(e->target_meta >> SC64_IMPACT_CLASS_SHIFT);
    return e->steps >= 1u &&
           (unsigned)sc64_impact_event_target(e) < target_limit &&
           ((e->target_meta >> SC64_IMPACT_OWNER_SHIFT) & 7u) < owner_limit &&
           class_bits <= (unsigned)SC64_IMPACT_CLASS_LIMIT &&
           e->source_type < type_limit;
}

static inline Sc64ImpactPushResult
sc64_impact_push(Sc64ImpactEvent *rows, unsigned capacity, uint8_t *count,
                 const Sc64ImpactHit *hit, unsigned steps)
{
    Sc64ImpactEvent e;
    if (!sc64_impact_pack(&e, hit, steps)) return SC64_IMPACT_UNREPRESENTABLE;
    if (*count >= capacity) return SC64_IMPACT_FULL;
    rows[(*count)++] = e;
    return SC64_IMPACT_QUEUED;
}

/* Slot reuse: forget every pending impact aimed at `target`. Order of the
 * surviving rows is preserved. Returns the number of rows removed. */
static inline unsigned
sc64_impact_clear_target(Sc64ImpactEvent *rows, uint8_t *count, int target)
{
    unsigned keep = 0, removed = 0;
    for (unsigned i = 0; i < *count; ++i) {
        if (sc64_impact_event_target(&rows[i]) == target) { ++removed; continue; }
        if (keep != i) rows[keep] = rows[i];
        ++keep;
    }
    *count = (uint8_t)keep;
    return removed;
}

/* Advance every row by one end-of-update step. Rows reaching zero move, in
 * launch order, to `due`; the queue keeps the rest in launch order. The
 * caller resolves `due` afterwards, so resolution side effects (deaths,
 * slot reuse) never mutate the array being iterated. */
static inline unsigned
sc64_impact_step(Sc64ImpactEvent *rows, uint8_t *count, Sc64ImpactEvent *due)
{
    unsigned keep = 0, ready = 0;
    for (unsigned i = 0; i < *count; ++i) {
        Sc64ImpactEvent e = rows[i];
        if (--e.steps == 0) { due[ready++] = e; continue; }
        rows[keep++] = e;
    }
    *count = (uint8_t)keep;
    return ready;
}

/* Save block. Only live rows are stored, in launch order; `count` and the
 * record length must agree. */
#define SC64_IMPACT_SAVE_MAGIC 0x31504d49u /* "IMP1" */
typedef struct {
    uint32_t magic;
    uint8_t count;
    uint8_t reserved[3];
} Sc64ImpactSaveHeader;

static inline uint32_t
sc64_impact_save_bytes(unsigned count)
{
    return (uint32_t)sizeof(Sc64ImpactSaveHeader) +
           (uint32_t)count * (uint32_t)sizeof(Sc64ImpactEvent);
}

/* Little-endian bytes, matching the RaceSave payload it follows. */
static inline uint32_t
sc64_impact_encode(const Sc64ImpactEvent *rows, unsigned count, uint8_t *out)
{
    uint32_t magic = SC64_IMPACT_SAVE_MAGIC;
    for (unsigned i = 0; i < 4u; ++i) out[i] = (uint8_t)(magic >> (8u * i));
    out[4] = (uint8_t)count;
    out[5] = out[6] = out[7] = 0;
    for (unsigned i = 0; i < count; ++i) {
        uint8_t *row = out + sizeof(Sc64ImpactSaveHeader) + i * sizeof(Sc64ImpactEvent);
        row[0] = (uint8_t)rows[i].target_meta;
        row[1] = (uint8_t)(rows[i].target_meta >> 8);
        row[2] = (uint8_t)rows[i].damage_hits;
        row[3] = (uint8_t)(rows[i].damage_hits >> 8);
        row[4] = rows[i].steps;
        row[5] = rows[i].source_type;
    }
    return sc64_impact_save_bytes(count);
}

/* Validate a saved block into `candidate` without touching live state.
 * Returns the row count, or -1 when the block is malformed. */
static inline int
sc64_impact_decode(const uint8_t *in, uint32_t bytes, unsigned capacity,
                   unsigned target_limit, unsigned owner_limit,
                   unsigned type_limit, Sc64ImpactEvent *candidate)
{
    unsigned count;
    if (bytes < sizeof(Sc64ImpactSaveHeader)) return -1;
    count = in[4];
    if (((uint32_t)in[0] | (uint32_t)in[1] << 8 | (uint32_t)in[2] << 16 |
         (uint32_t)in[3] << 24) != SC64_IMPACT_SAVE_MAGIC ||
        in[5] || in[6] || in[7] || count < 1u || count > capacity ||
        bytes != sc64_impact_save_bytes(count)) return -1;
    for (unsigned i = 0; i < count; ++i) {
        const uint8_t *row = in + sizeof(Sc64ImpactSaveHeader) + i * sizeof(Sc64ImpactEvent);
        candidate[i].target_meta = (uint16_t)(row[0] | row[1] << 8);
        candidate[i].damage_hits = (uint16_t)(row[2] | row[3] << 8);
        candidate[i].steps = row[4];
        candidate[i].source_type = row[5];
        if (!sc64_impact_event_valid(&candidate[i], target_limit, owner_limit,
                                     type_limit)) return -1;
    }
    return (int)count;
}

#endif
