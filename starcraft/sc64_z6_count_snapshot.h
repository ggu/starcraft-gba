#ifndef SC64_Z6_COUNT_SNAPSHOT_H
#define SC64_Z6_COUNT_SNAPSHOT_H

/* Optional one-pass accelerator for Z6's authenticated numeric count rows.
 * It is phase-local and transient: no actor buffer, BSS, or save state. The
 * existing full scan remains the fallback and differential oracle. This
 * helper consumes staged private numeric scenario metadata, not game assets. */
#include "sc64_z6_adapter.h"

enum {
    SC64_Z6_COUNT_QUERY_COUNT = 8,
    SC64_Z6_COUNT_SNAPSHOT_ROWS = SC64_Z6_SOURCE_CONDITION_COUNT
};

typedef struct {
    u16 count[SC64_Z6_COUNT_SNAPSHOT_ROWS];
    u16 valid_rows;
    u8 active;
} Sc64Z6CountSnapshot;

typedef struct {
    int left, top, right, bottom;
    u8 valid;
} Sc64Z6CountRect;

/* Fields copied from one live bank row by the caller. raw_type is the
 * nonnegative raw ID resolved by the existing adapter. type_known indicates
 * that raw ID maps to a runtime unit/building type for exact-ID queries. */
typedef struct {
    int x, y;
    s16 raw_type;
    s8 raw_owner;
    u8 alive, type_known;
} Sc64Z6CountActor;

/* Streaming builder: integration scans the real unit/building banks and
 * submits each row; it never materializes a 400-entry temporary array. */
typedef struct {
    u16 count[SC64_Z6_COUNT_SNAPSHOT_ROWS];
    u16 valid_rows;
    u16 owner_query_mask[12];
    u8 active;
} Sc64Z6CountSnapshotBuilder;

_Static_assert(SC64_Z6_SOURCE_CONDITION_COUNT <= 16u,
               "Z6 count validity mask bound");
_Static_assert(sizeof(Sc64Z6CountSnapshot) == 30u,
               "Z6 count snapshot must stay 30 bytes");
_Static_assert(sizeof(Sc64Z6CountSnapshotBuilder) <= 56u,
               "Z6 streaming count builder bound");

typedef struct {
    u8 trigger, index, selector, raw, location, row;
} Sc64Z6CountQueryKey;

static const Sc64Z6CountQueryKey sc64_z6_count_query_keys[
    SC64_Z6_COUNT_QUERY_COUNT] = {
    {0, 0, 13, 229, 0,  0}, /* Any Unit: Current Player */
    {2, 0, 19, 229, 4,  2}, /* Any Unit: source force selector 19 in loc 4 */
    {2, 1, 19, 229, 5,  3}, /* Any Unit: source force selector 19 in loc 5 */
    {4, 1, 13,  51, 1,  6}, /* Kerrigan: Current Player in location 1 */
    {4, 2, 13, 229, 6,  7}, /* Any Unit: Current Player in location 6 */
    {5, 1, 17,  79, 0,  9}, /* Tassadar: All Players */
    {6, 0, 13,  51, 0, 10}, /* Kerrigan: Current Player */
    {8, 0,  1, 229, 0, 12}  /* Any Unit: source player 1 */
};

static inline int sc64_z6_count_snapshot_source_rows_valid(void)
{
    unsigned count_rows = 0;
    if (SC64_Z6_SOURCE_CONDITION_COUNT != 13u) return 0;
    for (unsigned i = 0; i < SC64_Z6_COUNT_QUERY_COUNT; ++i) {
        const Sc64Z6CountQueryKey *key = &sc64_z6_count_query_keys[i];
        const Sc64Z6SourceCondition *c;
        if (!sc64_z6_condition_row(key->trigger, key->index, &c) ||
            c->opcode != (key->location ? 3u : 2u) ||
            c->player != key->selector || c->unit != key->raw ||
            c->location != key->location || c->mask || c->resource ||
            (unsigned)(c - sc64_z6_source_conditions) != key->row ||
            (c->comparison != 0u && c->comparison != 1u &&
             c->comparison != 10u) ||
            (c->flags != 0u && c->flags != 0x10u)) return 0;
    }
    for (unsigned row = 0; row < SC64_Z6_SOURCE_CONDITION_COUNT; ++row) {
        const Sc64Z6SourceCondition *c = &sc64_z6_source_conditions[row];
        if (c->opcode == 2u || c->opcode == 3u) {
            unsigned found = 0;
            for (unsigned i = 0; i < SC64_Z6_COUNT_QUERY_COUNT; ++i) {
                const Sc64Z6SourceCondition *expected;
                const Sc64Z6CountQueryKey *key = &sc64_z6_count_query_keys[i];
                if (sc64_z6_condition_row(key->trigger,key->index,&expected) &&
                    expected == c) ++found;
            }
            if (found != 1u) return 0;
            ++count_rows;
        }
    }
    return count_rows == SC64_Z6_COUNT_QUERY_COUNT;
}

static inline int sc64_z6_count_snapshot_trigger_executor(
    unsigned trigger, const u8 active[12], const u8 force_for_slot[8],
    unsigned *slot_out)
{
    unsigned resolved = 12u;
    if (trigger >= SC64_Z6_TRIGGER_COUNT || !active || !force_for_slot ||
        !slot_out) return 0;
    for (unsigned p = 0; p < 27u; ++p) {
        if (!sc64_z6_trigger_enabled(trigger, p)) continue;
        if (p < 8u) {
            if (!active[p] || resolved != 12u) return 0;
            resolved = p;
        } else if (p >= 18u && p <= 21u) {
            unsigned force = p - 18u;
            for (unsigned s = 0; s < 8u; ++s) {
                if (active[s] && force_for_slot[s] == force) {
                    if (resolved != 12u) return 0;
                    resolved = s;
                }
            }
            if (resolved == 12u) return 0;
        } else return 0;
    }
    if (resolved >= 12u) return 0;
    *slot_out = resolved;
    return 1;
}

static inline int sc64_z6_count_snapshot_owner_matches(
    unsigned selector, unsigned slot, const u8 active[12],
    unsigned executor, const u8 force_for_slot[8])
{
    if (slot >= 12u || !active[slot]) return 0;
    if (selector <= 11u) return selector == slot;
    if (selector == 12u) return 0;
    if (selector == 13u) return slot == executor;
    if (selector == 17u) return slot < 8u;
    if (selector >= 18u && selector <= 21u)
        return slot < 8u && force_for_slot[slot] == selector - 18u;
    return -1;
}

/* Initialize after computing the current integer-scaled half-open rectangles
 * with the world adapter. This intentionally preserves its center semantics. */
static inline int sc64_z6_count_snapshot_begin(
    Sc64Z6CountSnapshot *snapshot, Sc64Z6CountSnapshotBuilder *builder,
    const u8 source_slot_active[12], const u8 source_force_for_slot[8],
    const Sc64Z6CountRect *locations, unsigned location_count)
{
    if (snapshot) snapshot->active = 0;
    if (!builder) return 0;
    *builder = (Sc64Z6CountSnapshotBuilder){0};
    if (!source_slot_active || !source_force_for_slot || !locations ||
        location_count <= 6u || !sc64_z6_count_snapshot_source_rows_valid())
        return 0;
    for (unsigned qi = 0; qi < SC64_Z6_COUNT_QUERY_COUNT; ++qi) {
        const Sc64Z6CountQueryKey *key = &sc64_z6_count_query_keys[qi];
        const Sc64Z6SourceCondition *c;
        unsigned executor, row;
        if (!sc64_z6_condition_row(key->trigger,key->index,&c) ||
            !sc64_z6_count_snapshot_trigger_executor(key->trigger,
                source_slot_active,source_force_for_slot,&executor) ||
            (key->location && (!locations[key->location].valid ||
                locations[key->location].left > locations[key->location].right ||
                locations[key->location].top > locations[key->location].bottom)))
            return 0;
        row = key->row;
        if (row >= SC64_Z6_SOURCE_CONDITION_COUNT) return 0;
        builder->valid_rows |= (u16)1u << row;
        for (unsigned slot = 0; slot < 12u; ++slot) {
            int match = sc64_z6_count_snapshot_owner_matches(
                key->selector,slot,source_slot_active,executor,
                source_force_for_slot);
            if (match < 0) return 0;
            if (match) builder->owner_query_mask[slot] |= (u16)1u << row;
        }
    }
    builder->active = 1;
    return 1;
}

/* Submit one live bank row. `is_building` distinguishes raw unit IDs from
 * building IDs; only known raw51/raw79 units can match exact-type queries. */
static inline int sc64_z6_count_snapshot_add_actor(
    Sc64Z6CountSnapshotBuilder *builder, const Sc64Z6CountActor *actor,
    int is_building, const Sc64Z6CountRect *locations)
{
    u16 type_mask = 0, location_mask = 0, matched;
    unsigned owner;
    if (!builder || !builder->active || !actor || !locations) return 0;
    if (!actor->alive) return 1;
    if (actor->raw_owner < 0 || actor->raw_owner >= 12) {
        builder->active = 0;
        return 0;
    }
    owner = (unsigned)actor->raw_owner;
    if (!builder->owner_query_mask[owner] || actor->raw_type < 0) return 1;
    for (unsigned qi = 0; qi < SC64_Z6_COUNT_QUERY_COUNT; ++qi) {
        const Sc64Z6CountQueryKey *key = &sc64_z6_count_query_keys[qi];
        unsigned row;
        u16 bit;
        row = key->row;
        bit = (u16)1u << row;
        if (!(builder->owner_query_mask[owner] & builder->valid_rows & bit))
            continue;
        if (key->raw == 229u ||
            (!is_building && actor->type_known &&
             actor->raw_type == (s16)key->raw))
            type_mask |= bit;
        if (!key->location) location_mask |= bit;
        else {
            const Sc64Z6CountRect *rect = &locations[key->location];
            if (!rect->valid || rect->left > rect->right ||
                rect->top > rect->bottom) {
                builder->active = 0;
                return 0;
            }
            if (actor->x >= rect->left && actor->x < rect->right &&
                actor->y >= rect->top && actor->y < rect->bottom)
                location_mask |= bit;
        }
    }
    matched = builder->owner_query_mask[owner] & type_mask & location_mask &
              builder->valid_rows;
    for (unsigned row = 0; row < SC64_Z6_SOURCE_CONDITION_COUNT; ++row) {
        u16 bit = (u16)1u << row;
        if (matched & bit) {
            if (builder->count[row] == 0xffffu) {
                builder->active = 0;
                return 0;
            }
            ++builder->count[row];
        }
    }
    return 1;
}

static inline int sc64_z6_count_snapshot_finish(
    Sc64Z6CountSnapshotBuilder *builder, Sc64Z6CountSnapshot *snapshot)
{
    if (!builder || !builder->active || !snapshot) return 0;
    for (unsigned row=0; row<SC64_Z6_SOURCE_CONDITION_COUNT; ++row)
        snapshot->count[row]=builder->count[row];
    snapshot->valid_rows = builder->valid_rows;
    snapshot->active = 1;
    builder->active = 0;
    return 1;
}

static inline int sc64_z6_count_snapshot_lookup(
    const Sc64Z6CountSnapshot *snapshot,
    const Sc64Z6SourceCondition *condition, u32 *count)
{
    if (!snapshot || !snapshot->active || !condition || !count) return 0;
    for (unsigned row = 0; row < SC64_Z6_SOURCE_CONDITION_COUNT; ++row) {
        if (condition != &sc64_z6_source_conditions[row]) continue;
        if (!(snapshot->valid_rows & ((u16)1u << row))) return 0;
        *count = snapshot->count[row];
        return 1;
    }
    return 0;
}

#endif
