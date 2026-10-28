/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_P8_WORLD_ADAPTER_H
#define SC64_P8_WORLD_ADAPTER_H

#include "sc64_p8_source_adapter.h"
#include "sc64_z8_world_adapter.h"

enum {
    SC64_P8_PAUSE_IGNORED = 0,
    SC64_P8_PAUSE_APPLIED = 1
};

/* Live-world services for the bounded P8 source contract. This is an adapter,
 * not mission wiring: the caller owns these mutable views and any save format.
 * Actor census, owner resolution, placement dispatch, SetResources,
 * SetInvincibility and presentation reuse the native-backed Z8 world bridge.
 */
typedef struct {
    Sc64Z8WorldView *actors;
    Sc64Z8Rect *locations; /* mutable source-pixel rectangles, indexed from 0 */
    unsigned location_count;
    unsigned map_pixel_width, map_pixel_height;
    u8 *switches;          /* caller-owned 256-switch bank */
    const u32 *elapsed_seconds;
    void *context;
    /* P8 RunAIScriptAtLocation uses mission-specific IDs. No generic fallback
     * is provided; a caller must implement each supported ID explicitly. */
    int (*run_ai)(void *context, const Sc64P8SourceAction *action,
                  unsigned trigger_owner, const Sc64Z8Rect *rect);
    /* Wait, Transmission and CenterView share one wait timer per owner. The
     * service owns that timer and each action's armed marker. COMPLETE means
     * no wait was started; PENDING means started and must resume the same
     * action; BLOCKED means it was not started because the owner is waiting.
     */
    int (*async_action)(void *context, const Sc64P8SourceAction *action,
                        unsigned trigger_owner, const Sc64Z8Rect *rect,
                        int resume);
    void *async_context;
    /* Optional property-aware P8 creation service. The selector is one-based;
     * property_bytes points to the authenticated immutable UPRP record. A
     * service must interpret every requested field it supports exactly and
     * reject the action otherwise. This adapter does not define override
     * semantics or copy/serialize the record. */
    int (*create_with_property)(void *context,
                                const Sc64P8SourceAction *action,
                                unsigned source_owner, unsigned count,
                                unsigned property_id,
                                const u8 property_bytes[SC64_P8_SOURCE_PROPERTY_BYTES],
                                const Sc64Z8Rect *rect);
    void *create_context;
    /* PauseGame/UnpauseGame need explicit engine and trigger-tail bindings.
     * Return APPLIED when native eligibility permits the side effect, IGNORED
     * when native control/global gates make it a successful no-op, and a
     * negative value on an unsupported/error path. The tail callback uses the
     * same result contract so an ignored native Unpause preserves its marker. */
    int (*pause_game)(void *context, const Sc64P8SourceAction *action,
                      unsigned trigger_owner, int paused);
    void *pause_context;
    int (*pause_trigger_end)(void *context, unsigned trigger,
                             unsigned trigger_owner);
    void *pause_end_context;
} Sc64P8WorldContext;

static inline int
sc64_p8_world_context_valid(const Sc64P8WorldContext *ctx)
{
    return ctx && ctx->actors && ctx->locations &&
           ctx->location_count <= SC64_P8_SOURCE_LOCATION_COUNT &&
           ctx->map_pixel_width == SC64_P8_SOURCE_MAP_COLS * 32u &&
           ctx->map_pixel_height == SC64_P8_SOURCE_MAP_ROWS * 32u &&
           ctx->actors->actor_count <= SC64_Z8_WORLD_ACTOR_LIMIT &&
           (ctx->actors->actor_at || ctx->actors->actors ||
            !ctx->actors->actor_count);
}

/* Initialize caller-owned mutable rectangles from the authenticated compact
 * source table. This copies only coordinates; it does not create save state. */
static inline int
sc64_p8_world_locations_init(Sc64Z8Rect *out, unsigned capacity)
{
    if (!out || capacity < SC64_P8_SOURCE_LOCATION_COUNT ||
        !sc64_p8_source_metadata_valid()) return 0;
    for (unsigned i = 0; i < SC64_P8_SOURCE_LOCATION_COUNT; ++i) {
        const Sc64P8SourceLocation *src = &sc64_p8_source_locations[i];
        if (src->left > 32767u || src->top > 32767u ||
            src->right > 32767u || src->bottom > 32767u ||
            src->left > src->right || src->top > src->bottom ||
            src->right > SC64_P8_SOURCE_MAP_COLS * 32u ||
            src->bottom > SC64_P8_SOURCE_MAP_ROWS * 32u) return 0;
    }
    for (unsigned i = 0; i < SC64_P8_SOURCE_LOCATION_COUNT; ++i) {
        const Sc64P8SourceLocation *src = &sc64_p8_source_locations[i];
        out[i] = (Sc64Z8Rect){(s16)src->left, (s16)src->top,
                              (s16)src->right, (s16)src->bottom};
    }
    return 1;
}

static inline int
sc64_p8_world_get_rect(const Sc64P8WorldContext *ctx, unsigned location,
                       const Sc64Z8Rect **rect)
{
    if (!ctx || !rect || location > ctx->location_count) return 0;
    *rect = location ? &ctx->locations[location - 1u] : 0;
    if (*rect && ((*rect)->left < 0 || (*rect)->top < 0 ||
                  (*rect)->left > (*rect)->right ||
                  (*rect)->top > (*rect)->bottom ||
                  (unsigned)(*rect)->right > ctx->map_pixel_width ||
                  (unsigned)(*rect)->bottom > ctx->map_pixel_height))
        return 0;
    return 1;
}

static inline Sc64Z8Condition
sc64_p8_world_condition_as_z8(const Sc64P8SourceCondition *source)
{
    Sc64Z8Condition out = {0};
    if (source) {
        out.location = source->location;
        out.player = source->player;
        out.amount = source->amount;
        out.unit = source->unit;
        out.mask = source->mask;
        out.comparison = source->comparison;
        out.opcode = source->opcode;
        out.resource = source->resource;
        out.flags = source->flags;
        out.index = source->index;
    }
    return out;
}

static inline Sc64Z8Action
sc64_p8_world_action_as_z8(const Sc64P8SourceAction *source)
{
    Sc64Z8Action out = {0};
    if (source) {
        out.location = source->location;
        out.string_id = source->string_id;
        out.wav_id = source->wav_id;
        out.time = source->time;
        out.group1 = source->group1;
        out.group2 = source->group2;
        out.unit = source->unit;
        out.opcode = source->opcode;
        out.modifier = source->modifier;
        out.flags = source->flags;
        out.index = source->index;
    }
    return out;
}

/* Pure Command/Bring/Switch/ElapsedTime bridge. Returns 1/0, or -1 if a
 * required owner, location, actor fact or input bank is unavailable. */
static inline int
sc64_p8_world_condition(void *context,
                        const Sc64P8SourceCondition *source,
                        unsigned trigger_owner)
{
    Sc64P8WorldContext *ctx = (Sc64P8WorldContext *)context;
    const Sc64Z8Rect *rect;
    u32 count = 0;
    Sc64Z8Condition c;
    if (!source) return -1;
    if (source->flags & 2u) return 1; /* disabled source slot */
    if (trigger_owner >= SC64_Z8_OWNER_LIMIT ||
        !sc64_p8_world_context_valid(ctx) ||
        !sc64_p8_world_get_rect(ctx, source->location, &rect)) return -1;
    c = sc64_p8_world_condition_as_z8(source);
    /* This bridge does not implement condition-only flag bits or any
     * comparison outside the native-backed Z8 comparator contract. */
    if ((c.flags & ~0x12u) ||
        !sc64_z8_comparison_known(c.opcode, c.comparison)) return -1;
    switch (c.opcode) {
    case 2: /* Command */
        if (sc64_z8_world_count(ctx->actors, &c, trigger_owner, 0, &count) != 1)
            return -1;
        return sc64_z8_compare(c.comparison, count, c.amount);
    case 3: /* Bring */
        if (!c.location) return 0; /* native Bring at location zero is false */
        if (sc64_z8_world_count(ctx->actors, &c, trigger_owner, rect,
                                &count) != 1) return -1;
        return sc64_z8_compare(c.comparison, count, c.amount);
    case 11: { /* Switch: comparison 2=set, 3=cleared. */
        if (!ctx->switches) return -1;
        int set = (ctx->switches[c.resource >> 3] >> (c.resource & 7u)) & 1;
        if (c.comparison == 2u) return set;
        if (c.comparison == 3u) return !set;
        return -1;
    }
    case 12: /* ElapsedTime */
        if (!ctx->elapsed_seconds) return -1;
        return sc64_z8_compare(c.comparison, *ctx->elapsed_seconds, c.amount);
    case 22: return 1;
    case 23: return 0;
    default: return -1;
    }
}

/* Dispatch source actions that have a bounded shared implementation. Creation
 * preserves the source property selector and requires a caller service that
 * can interpret it. P8's three UPRP variants are not flattened to defaults.
 */
static inline int
sc64_p8_world_action(void *context, const Sc64P8SourceAction *source,
                     unsigned trigger_owner, int resume)
{
    Sc64P8WorldContext *ctx = (Sc64P8WorldContext *)context;
    const Sc64Z8Rect *rect;
    Sc64Z8Action a;
    if (!source) return -1;
    if (source->flags & 2u) return SC64_P8_ACTION_COMPLETE;
    if (trigger_owner >= SC64_Z8_OWNER_LIMIT ||
        !sc64_p8_world_context_valid(ctx) ||
        (resume != 0 && resume != 1) ||
        !sc64_p8_world_get_rect(ctx, source->location, &rect)) return -1;
    /* The Z8 action shape has no trailing-byte field. P8 currently has zero
     * in these reserved bytes; refuse to silently discard future semantics. */
    if (source->trailing[0] || source->trailing[1] || source->trailing[2])
        return -1;
    if (resume && source->opcode != 4u && source->opcode != 7u &&
        source->opcode != 10u) return -1;
    a = sc64_p8_world_action_as_z8(source);
    switch (a.opcode) {
    case 1: return SC64_P8_ACTION_WON;
    case 2: return SC64_P8_ACTION_LOST;
    case 4:
    case 7: case 10:
        if (a.opcode != 4u && !a.location)
            return SC64_P8_ACTION_COMPLETE;
        if (!ctx->async_action) return -1;
        return ctx->async_action(ctx->async_context ? ctx->async_context :
                                 ctx->context, source, trigger_owner, rect,
                                 resume);
    case 5: case 6: /* PauseGame / UnpauseGame. */
        if (!ctx->pause_game || a.location || a.string_id || a.wav_id ||
            a.time || a.group1 || a.group2 || a.unit || a.modifier || a.flags)
            return -1;
        {
            int result = ctx->pause_game(ctx->pause_context ?
                ctx->pause_context : ctx->context, source, trigger_owner,
                a.opcode == 5u);
            if (result == SC64_P8_PAUSE_APPLIED ||
                result == SC64_P8_PAUSE_IGNORED)
                return SC64_P8_ACTION_COMPLETE;
            return -1;
        }
    case 11: case 28: case 43:
        /* Native location-zero actions return success before touching the
         * world; in particular, don't call an AI/create service here. */
        if (!a.location) return SC64_P8_ACTION_COMPLETE;
        if (a.opcode == 11u && a.group2) {
            const u8 *property_bytes;
            unsigned owner;
            if (a.unit >= SC64_Z8_RAW_UNIT_LIMIT || !rect ||
                a.flags != 0x18u || !ctx->create_with_property ||
                !sc64_p8_source_property(a.group2, &property_bytes) ||
                !sc64_z8_world_resolve_one(ctx->actors, a.group1,
                                           trigger_owner, &owner)) return -1;
            return ctx->create_with_property(ctx->create_context ?
                    ctx->create_context : ctx->context, source, owner,
                    sc64_z8_create_count(&a), a.group2, property_bytes, rect) == 1
                ? SC64_P8_ACTION_COMPLETE : -1;
        }
        if (a.opcode == 11u && a.flags != 0x10u) return -1;
        return sc64_z8_world_action(ctx->actors, &a, trigger_owner, rect) == 1
            ? SC64_P8_ACTION_COMPLETE : -1;
    case 13: { /* 4=set, 5=clear, 6=toggle over the shared 256 switches. */
        if (!ctx->switches || a.group2 >= 256u) return -1;
        u8 *byte = &ctx->switches[a.group2 >> 3];
        u8 bit = (u8)(1u << (a.group2 & 7u));
        if (a.modifier == 4u) *byte |= bit;
        else if (a.modifier == 5u) *byte &= (u8)~bit;
        else if (a.modifier == 6u) *byte ^= bit;
        else return -1;
        return SC64_P8_ACTION_COMPLETE;
    }
    case 38: {
        s16 x, y;
        if (!a.location || !a.group2) return SC64_P8_ACTION_COMPLETE;
        if (a.group2 > ctx->location_count || !rect) return -1;
        int found = sc64_z8_world_find_unit(ctx->actors, &a, trigger_owner,
                                             rect, &x, &y);
        if (found < 0) return -1;
        if (!found) {
            x = (s16)(((s32)rect->left + rect->right) / 2);
            y = (s16)(((s32)rect->top + rect->bottom) / 2);
        }
        sc64_z8_move_location(&ctx->locations[a.group2 - 1u], x, y,
                              (s32)ctx->map_pixel_width,
                              (s32)ctx->map_pixel_height);
        return SC64_P8_ACTION_COMPLETE;
    }
    case 16:
        if (!a.location) return SC64_P8_ACTION_COMPLETE;
        if (!rect || !ctx->run_ai) return -1;
        return ctx->run_ai(ctx->context, source, trigger_owner, rect) == 1
            ? SC64_P8_ACTION_COMPLETE : -1;
    case 8: case 9: case 12: case 26:
        return sc64_z8_world_action(ctx->actors, &a, trigger_owner, rect) == 1
            ? SC64_P8_ACTION_COMPLETE : -1;
    default:
        return -1;
    }
}

static inline int
sc64_p8_world_trigger_complete(void *context, unsigned trigger,
                               unsigned trigger_owner)
{
    Sc64P8WorldContext *ctx = (Sc64P8WorldContext *)context;
    if (!ctx || trigger >= SC64_P8_TRIGGER_LIMIT ||
        trigger_owner >= SC64_Z8_OWNER_LIMIT) return -1;
    return ctx->pause_trigger_end
        ? ctx->pause_trigger_end(ctx->pause_end_context ?
              ctx->pause_end_context : ctx->context, trigger, trigger_owner)
        : 1;
}

static inline Sc64P8SourceWorldOps
sc64_p8_world_ops(Sc64P8WorldContext *context)
{
    Sc64P8SourceWorldOps ops = {context, sc64_p8_world_condition,
                                sc64_p8_world_action,
                                sc64_p8_world_trigger_complete};
    return ops;
}

#endif /* SC64_P8_WORLD_ADAPTER_H */
