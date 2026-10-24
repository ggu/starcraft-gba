#ifndef SC64_P6_COUNT_SNAPSHOT_H
#define SC64_P6_COUNT_SNAPSHOT_H

/* Phase-local P6 Command/Bring results. This object is built after the
 * campaign's pre-condition world updates and discarded before actions run.
 * It never survives a tick or enters a save payload. */
#ifdef SC64_P6_COUNT_SNAPSHOT

#include "sc64_p6_world_adapter.h"

enum { SC64_P6_COUNT_SNAPSHOT_RECTS = SC64_P6_SOURCE_CONDITION_COUNT };

typedef struct {
    u16 count[SC64_P6_SOURCE_CONDITION_COUNT];
    u32 cached_rows;
    u32 fallback_rows;
    u32 selector_rows[22];
    u32 selectors;
    u16 rects[SC64_P6_COUNT_SNAPSHOT_RECTS][4];
    u8 rect_ids[SC64_P6_COUNT_SNAPSHOT_RECTS];
    u8 row_rect[SC64_P6_SOURCE_CONDITION_COUNT];
    u8 rect_count;
    u8 active;
} Sc64P6CountSnapshot;

_Static_assert(sizeof(Sc64P6CountSnapshot) <= 512,
               "P6 count snapshot must remain within its transient bound");

typedef struct {
    Sc64P6WorldContext *world;
    Sc64P6CountSnapshot *snapshot;
} Sc64P6CountSnapshotOpsContext;

static int sc64_p6_count_snapshot_rect(const Sc64P6WorldContext *ctx,
                                        Sc64P6CountSnapshot *snapshot,
                                        unsigned location, unsigned *index)
{
    int left, top, right, bottom;
    if (!snapshot || !index || !location) return 0;
    for (unsigned i = 0; i < snapshot->rect_count; ++i) {
        if (snapshot->rect_ids[i] == location) {
            *index = i;
            return 1;
        }
    }
    if (snapshot->rect_count >= SC64_P6_COUNT_SNAPSHOT_RECTS ||
        !sc64_p6_world_location(ctx, location,
                                &left, &top, &right, &bottom) ||
        left < 0 || top < 0 || right > 65535 || bottom > 65535)
        return 0;
    *index = snapshot->rect_count++;
    snapshot->rect_ids[*index] = (u8)location;
    snapshot->rects[*index][0] = (u16)left;
    snapshot->rects[*index][1] = (u16)top;
    snapshot->rects[*index][2] = (u16)right;
    snapshot->rects[*index][3] = (u16)bottom;
    return 1;
}

static int sc64_p6_count_snapshot_row_supported(
    const Sc64P6WorldContext *ctx, Sc64P6CountSnapshot *snapshot,
    const Sc64P6SourceCondition *condition)
{
    const Sc64P6SourceCondition *source;
    unsigned ignored;
    if (!ctx || !snapshot || !condition ||
        !sc64_p6_condition_row(condition->trigger, condition->index, &source) ||
        source != condition || (condition->opcode != SC64_P6_CONDITION_COMMAND &&
        condition->opcode != SC64_P6_CONDITION_BRING) ||
        (condition->flags != 0 && condition->flags != 0x10u) || condition->mask ||
        condition->resource || !sc64_p6_selector_valid(condition->player) ||
        condition->unit > 231u ||
        sc64_p6_count_comparison(0, condition->comparison,
                                 condition->amount) < 0 ||
        (condition->opcode == SC64_P6_CONDITION_COMMAND &&
         (!condition->unit || condition->location)) ||
        (condition->opcode == SC64_P6_CONDITION_BRING &&
         (!condition->location ||
          !sc64_p6_count_snapshot_rect(ctx, snapshot,
                                        condition->location, &ignored))))
        return 0;
    return 1;
}

/* Start a one-tick cache. Rows that are copied, malformed, unsupported, or
 * outside the bounded P6 query rectangle bank remain on the original
 * full-scan path. */
static int sc64_p6_count_snapshot_begin(Sc64P6CountSnapshot *snapshot,
                                         const Sc64P6WorldContext *ctx,
                                         const Sc64P6State *state,
                                         int prune_started)
{
    if (!snapshot) return 0;
    memset(snapshot, 0, sizeof(*snapshot));
    if (!ctx || !state || prune_started < 0 || prune_started > 1) return 0;
    for (unsigned row = 0; row < SC64_P6_SOURCE_CONDITION_COUNT; ++row) {
        const Sc64P6SourceCondition *condition = &sc64_p6_source_conditions[row];
        u32 bit = (u32)1u << row;
        if (condition->opcode != SC64_P6_CONDITION_COMMAND &&
            condition->opcode != SC64_P6_CONDITION_BRING) continue;
        if (prune_started && sc64_p6_trigger_started(state, condition->trigger) &&
            sc64_p6_trigger_one_shot(condition->trigger)) continue;
        if (sc64_p6_count_snapshot_row_supported(ctx, snapshot, condition)) {
            snapshot->cached_rows |= bit;
            snapshot->selector_rows[condition->player] |= bit;
            snapshot->selectors |= (u32)1u << condition->player;
            if (condition->opcode == SC64_P6_CONDITION_BRING) {
                unsigned rect;
                if (!sc64_p6_count_snapshot_rect(ctx, snapshot, condition->location, &rect)) {
                    snapshot->cached_rows &= ~bit;
                    continue;
                }
                snapshot->row_rect[row] = (u8)rect;
            }
        }
    }
    if (!snapshot->cached_rows) return 0;

    for (unsigned category = 0; category < 2; ++category) {
        int building = (int)category;
        unsigned limit = building ? MAX_BUILDINGS : MAX_UNITS;
        for (unsigned slot = 0; slot < limit; ++slot) {
            int alive, raw, raw_owner, live_owner, x, y;
            if (building) {
                alive = b[slot].alive;
                if (!alive) continue;
                raw = b[slot].raw_type >= 0 ? b[slot].raw_type :
                      sc64_race_raw_building(b[slot].type);
                raw_owner = b[slot].raw_owner; live_owner = b[slot].owner;
                x = b[slot].x; y = b[slot].y;
            } else {
                alive = u[slot].alive;
                if (!alive) continue;
                raw = u[slot].raw_type >= 0 ? u[slot].raw_type :
                      sc64_race_raw_unit(u[slot].type);
                raw_owner = u[slot].raw_owner; live_owner = u[slot].owner;
                x = u[slot].x; y = u[slot].y;
            }
            /* Resolve each distinct source selector once per actor. Most
               P6 Bring rows share CurrentPlayer, so hostile actors need not
               repeat that same owner test for every objective rectangle. */
            u32 actor_rows=0, selectors=snapshot->selectors;
            while (selectors) {
                unsigned selector=(unsigned)__builtin_ctz(selectors);
                u32 rows=snapshot->selector_rows[selector] & snapshot->cached_rows;
                int owner_match=sc64_p6_world_owner_matches(ctx,selector,raw_owner,live_owner);
                selectors &= selectors-1u;
                if (owner_match<0) snapshot->fallback_rows |= rows;
                else if (owner_match) actor_rows |= rows;
            }
            actor_rows &= ~snapshot->fallback_rows;
            if (raw<0) continue;
            while (actor_rows) {
                unsigned row=(unsigned)__builtin_ctz(actor_rows);
                u32 bit=(u32)1u<<row;
                const Sc64P6SourceCondition *condition=&sc64_p6_source_conditions[row];
                int type_match;
                actor_rows &= actor_rows-1u;
                type_match = sc64_p6_world_type_match(ctx, condition->unit,
                                                       (unsigned)raw, building);
                if (type_match < 0) {
                    snapshot->fallback_rows |= bit;
                    continue;
                }
                if (!type_match) continue;
                if (condition->opcode == SC64_P6_CONDITION_BRING) {
                    unsigned rect = snapshot->row_rect[row];
                    if (rect >= snapshot->rect_count) {
                        snapshot->fallback_rows |= bit;
                        continue;
                    }
                    if (x < snapshot->rects[rect][0] ||
                        x >= snapshot->rects[rect][2] ||
                        y < snapshot->rects[rect][1] ||
                        y >= snapshot->rects[rect][3]) continue;
                }
                if (snapshot->count[row] == 0xffffu) {
                    snapshot->fallback_rows |= bit;
                    continue;
                }
                ++snapshot->count[row];
            }
        }
    }
    snapshot->active = 1;
    return 1;
}

/* A miss means the caller must use sc64_p6_world_count unchanged. */
static int sc64_p6_count_snapshot_lookup(const Sc64P6CountSnapshot *snapshot,
                                          const Sc64P6SourceCondition *condition,
                                          u32 *count)
{
    if (!snapshot || !snapshot->active || !condition || !count) return 0;
    for (unsigned row = 0; row < SC64_P6_SOURCE_CONDITION_COUNT; ++row) {
        u32 bit = (u32)1u << row;
        if (condition != &sc64_p6_source_conditions[row]) continue;
        if (!(snapshot->cached_rows & bit) || (snapshot->fallback_rows & bit))
            return 0;
        *count = snapshot->count[row];
        return 1;
    }
    return 0;
}

static void sc64_p6_count_snapshot_end(Sc64P6CountSnapshot *snapshot)
{
    if (!snapshot) return;
    snapshot->active = 0;
    snapshot->cached_rows = 0;
    snapshot->fallback_rows = 0;
}

/* Optional WorldOps forwarding layer. It lets the campaign keep the original
 * action/AI callbacks while the condition callback sees this stack snapshot. */
static int sc64_p6_count_snapshot_condition(void *opaque,
                                            const Sc64P6RuntimeState *state,
                                            const Sc64P6SourceCondition *condition)
{
    Sc64P6CountSnapshotOpsContext *context = opaque;
    u32 count;
    if (!context || !context->world || !condition || !state) return -1;
    if ((condition->opcode == SC64_P6_CONDITION_COMMAND ||
         condition->opcode == SC64_P6_CONDITION_BRING) &&
        sc64_p6_count_snapshot_lookup(context->snapshot, condition, &count))
        return sc64_p6_count_comparison(count, condition->comparison,
                                        condition->amount);
    return sc64_p6_world_condition(context->world, state, condition);
}

static int sc64_p6_count_snapshot_action(void *opaque,
                                         const Sc64P6SourceAction *action)
{
    Sc64P6CountSnapshotOpsContext *context = opaque;
    if (!context || !context->world) return 0;
    /* A source effect can change Command/Bring counts. Invalidate before
       forwarding so every later condition in this tick falls back to a scan. */
    sc64_p6_count_snapshot_end(context->snapshot);
    return sc64_p6_world_action(context->world, action);
}

static int sc64_p6_count_snapshot_opaque_ai(void *opaque,
                                             const Sc64P6SourceAction *action)
{
    Sc64P6CountSnapshotOpsContext *context = opaque;
    if (!context || !context->world || !context->world->run_ai) return 0;
    sc64_p6_count_snapshot_end(context->snapshot);
    return context->world->run_ai(context->world->callback_context, action) == 1;
}

#endif /* SC64_P6_COUNT_SNAPSHOT */
#endif
