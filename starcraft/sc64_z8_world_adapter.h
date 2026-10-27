/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * Typed world bridge for the isolated Z8 trigger scheduler.
 *
 * The caller supplies a live ordered view of the game actor/building pools.
 * Views are not serialized; refresh them before each scheduler pass. Mutable
 * invincibility pointers and wallet pointers must refer to the engine's real
 * state. Creation, AI and presentation remain explicit engine callbacks. A
 * missing callback or unmappable actor faults instead of silently succeeding.
 * This header is not wired into game.c, a mission profile, menus or saves.
 */
#ifndef SC64_Z8_WORLD_ADAPTER_H
#define SC64_Z8_WORLD_ADAPTER_H

#include "sc64_z8_triggers.h"
#include "sc64_resource_actions.h"

enum {
    SC64_Z8_RAW_UNIT_LIMIT = 228u,
    SC64_Z8_WORLD_ACTOR_LIMIT = 512u,
    SC64_Z8_SOURCE_OWNER_LIMIT = 12u
};

typedef struct {
    u16 raw_type;
    s16 source_owner;
    s16 x, y;                 /* Engine world pixels; CHK source pixels / 2. */
    s16 left, top, right, bottom; /* Pinned unit-dimension offsets, source px. */
    u8 alive, completed, building, men, loaded, type_known;
    u8 *invincible;           /* Dedicated live invincible_* slot, never Unit.flags. */
} Sc64Z8ActorView;

typedef struct {
    void *context; /* Shared context for non-create services by default. */
    /* Apply a CreateUnitWithProperties row through the engine's creation
     * routine. property_id is the source UPRP selector; do not ignore it. */
    int (*create)(void *context, const Sc64Z8Action *action,
                  unsigned source_owner, unsigned count, unsigned property_id,
                  unsigned x1, unsigned y1, unsigned x2, unsigned y2);
    /* Implement the named native script policy. No default/generic AI exists. */
    int (*run_ai)(void *context, u32 fourcc, unsigned trigger_owner,
                  unsigned location, const Sc64Z8Rect *rect);
    /* PlayWAV, DisplayTextMessage, objectives and MinimapPing are dispatched
     * unchanged so the engine renderer/audio layer owns their semantics. */
    int (*present)(void *context, const Sc64Z8Action *action,
                   unsigned trigger_owner, const Sc64Z8Rect *rect);
    /* Native Kill conditions read the player's accumulated kill census. */
    int (*kill_count)(void *context, unsigned source_owner, unsigned raw_type,
                      u32 *count);
    /* Native timed presentation (Transmission / CenterView) and completion. */
    int (*timed)(void *context, int phase, const Sc64Z8Action *action,
                 unsigned trigger_owner, const Sc64Z8Rect *rect,
                 u32 *duration_ms);
    /* Optional per-service context. NULL keeps the shared `context` above.
     * Appended to preserve existing positional initializers. */
    void *create_context;
} Sc64Z8WorldServices;

typedef struct {
    Sc64Z8ActorView *actors; /* Canonical engine enumeration order. */
    u16 actor_count; /* Provider mode may expose pool capacity, including dead slots. */
    void *actor_context;
    /* Optional zero-copy bridge to private engine pools. Returns 1 and fills
     * the row, or -1 when that actor cannot be represented. To expose actors
     * created during this scheduler pass, provider mode must enumerate pool
     * capacity (including dead slots); each read must reflect current state. */
    int (*actor_at)(void *context, unsigned index, Sc64Z8ActorView *out);
    u8 active_mask;
    u8 force[SC64_Z8_OWNER_LIMIT]; /* CHK force IDs, 1..4; 0 means unassigned. */
    Sc64ResourceActionState *wallet[SC64_Z8_OWNER_LIMIT];
    /* Optional independent current-resource storage for source slots that
     * collapse to one live SIDE/ENEMY owner. The caller owns persistence. */
    u32 *source_minerals[SC64_Z8_OWNER_LIMIT];
    u32 *source_gas[SC64_Z8_OWNER_LIMIT];
    Sc64Z8WorldServices services;
} Sc64Z8WorldView;

static inline int
sc64_z8_world_actor_at(const Sc64Z8WorldView *w, unsigned index,
                       Sc64Z8ActorView *out)
{
    if (!w || !out || index >= w->actor_count) return -1;
    if (w->actor_at) return w->actor_at(w->actor_context, index, out);
    if (!w->actors) return -1;
    *out = w->actors[index];
    return 1;
}

static inline int
sc64_z8_world_actor_source_available(const Sc64Z8WorldView *w)
{
    return w && w->actor_count <= SC64_Z8_WORLD_ACTOR_LIMIT &&
           (w->actor_at || w->actors || !w->actor_count);
}

static inline int
sc64_z8_world_owner_matches(const Sc64Z8WorldView *w, unsigned selector,
                            unsigned current_owner, unsigned owner)
{
    if (!w) return -1;
    /* CHK neutral/doodad owners 8..11 are valid actors but are not entries in
     * the eight-player enabled-player table. */
    if (owner >= SC64_Z8_OWNER_LIMIT) return owner < SC64_Z8_SOURCE_OWNER_LIMIT ? 0 : -1;
    if (!(w->active_mask & (1u << owner))) return 0;
    if (selector < SC64_Z8_OWNER_LIMIT) return selector == owner;
    if (selector == 13u)
        return current_owner < SC64_Z8_OWNER_LIMIT &&
               (w->active_mask & (1u << current_owner)) ? owner == current_owner : -1;
    if (selector == SC64_Z8_SELECT_ALL_PLAYERS) return owner < 8u;
    if (selector >= SC64_Z8_SELECT_FORCE1 && selector <= SC64_Z8_SELECT_FORCE4) {
        if (w->force[owner] > 4u) return -1;
        return owner < 8u && w->force[owner] == selector - SC64_Z8_SELECT_FORCE1 + 1u;
    }
    return -1;
}

/* Action target selectors that mutate one owner must identify exactly one
 * active source slot. Multi-owner groups are rejected until their native
 * action-specific expansion is modeled. */
static inline int
sc64_z8_world_resolve_one(const Sc64Z8WorldView *w, unsigned selector,
                          unsigned current_owner, unsigned *owner_out)
{
    unsigned found = SC64_Z8_OWNER_LIMIT;
    if (!w || !owner_out) return 0;
    for (unsigned owner = 0; owner < SC64_Z8_OWNER_LIMIT; ++owner) {
        int match = sc64_z8_world_owner_matches(w, selector, current_owner, owner);
        if (match < 0) return 0;
        if (match) {
            if (found != SC64_Z8_OWNER_LIMIT) return 0;
            found = owner;
        }
    }
    if (found == SC64_Z8_OWNER_LIMIT) return 0;
    *owner_out = found;
    return 1;
}

static inline int
sc64_z8_world_actor_type(const Sc64Z8ActorView *a, unsigned wanted)
{
    if (!a || !a->type_known || a->raw_type >= SC64_Z8_RAW_UNIT_LIMIT) return -1;
    if (wanted == 229u) return 1; /* Any Unit, including buildings. */
    if (wanted == 230u) return !a->building && a->men;
    if (wanted == 231u) return a->building;
    if (wanted >= 228u || wanted != a->raw_type) return 0;
    return 1;
}

static inline int
sc64_z8_world_actor_rect(const Sc64Z8ActorView *a, int *left, int *top,
                         int *right, int *bottom)
{
    if (!a || !left || !top || !right || !bottom || a->x < 0 || a->y < 0 ||
        a->left < 0 || a->top < 0 || a->right < 0 || a->bottom < 0)
        return 0;
    *left = (int)a->x * 2 - a->left;
    *top = (int)a->y * 2 - a->top;
    *right = (int)a->x * 2 + a->right + 1;
    *bottom = (int)a->y * 2 + a->bottom + 1;
    return 1;
}

static inline int
sc64_z8_world_overlap(const Sc64Z8ActorView *a, const Sc64Z8Rect *r)
{
    int l, t, right, bottom;
    if (!r || !sc64_z8_world_actor_rect(a, &l, &t, &right, &bottom)) return -1;
    return l < r->right && r->left < right && t < r->bottom && r->top < bottom;
}

static inline int
sc64_z8_world_count(void *context, const Sc64Z8Condition *c,
                    unsigned trigger_owner, const Sc64Z8Rect *rect, u32 *out)
{
    Sc64Z8WorldView *w = (Sc64Z8WorldView *)context;
    unsigned source_owner;
    u32 count = 0;
    (void)trigger_owner;
    if (!w || !c || !out || !sc64_z8_world_actor_source_available(w) ||
        c->unit > 231u || (c->player > 13u && c->player < 17u) ||
        c->player > 21u ||
        (c->opcode != 2 && c->opcode != 3 && c->opcode != 5)) return -1;
    if (c->opcode == 5) {
        if (!w->services.kill_count ||
            !sc64_z8_world_resolve_one(w, c->player, trigger_owner, &source_owner) ||
            w->services.kill_count(w->services.context, source_owner,
                                   c->unit, &count) != 1) return -1;
        *out = count;
        return 1;
    }
    if (c->opcode == 3 && !rect) return -1;
    for (unsigned i = 0; i < w->actor_count; ++i) {
        Sc64Z8ActorView actor;
        Sc64Z8ActorView *a = &actor;
        int owner, type, overlap = 1;
        if (sc64_z8_world_actor_at(w, i, a) != 1) return -1;
        if (!a->alive) continue;
        if (a->source_owner < 0 || a->source_owner >= SC64_Z8_SOURCE_OWNER_LIMIT)
            return -1;
        owner = sc64_z8_world_owner_matches(w, c->player, trigger_owner,
                                             (unsigned)a->source_owner);
        if (owner < 0) return -1;
        if (!owner) continue;
        type = sc64_z8_world_actor_type(a, c->unit);
        if (type < 0) return -1;
        if (!type) continue;
        if (sc64_z8_condition_counts_completed(c) && !a->completed) continue;
        /* Bring checks world presence; cargo still counts for Command, as in
         * the existing P5 provider contract. */
        if (c->opcode == 3 && a->loaded) continue;
        if (c->opcode == 3) {
            overlap = sc64_z8_world_overlap(a, rect);
            if (overlap < 0) return -1;
        }
        if (overlap) ++count;
    }
    *out = count;
    return 1;
}

static inline int
sc64_z8_world_find_unit(void *context, const Sc64Z8Action *a,
                        unsigned trigger_owner, const Sc64Z8Rect *rect,
                        s16 *x, s16 *y)
{
    Sc64Z8WorldView *w = (Sc64Z8WorldView *)context;
    (void)trigger_owner;
    if (!w || !a || a->unit >= SC64_Z8_RAW_UNIT_LIMIT || !rect || !x || !y ||
        !sc64_z8_world_actor_source_available(w)) return -1;
    /* The supplied array order must be the caller's native actor enumeration
     * order; this adapter does not invent a unit-before-building ordering. */
    for (unsigned i = 0; i < w->actor_count; ++i) {
        Sc64Z8ActorView actor_row;
        Sc64Z8ActorView *actor = &actor_row;
        int owner, type, overlap;
        if (sc64_z8_world_actor_at(w, i, actor) != 1) return -1;
        if (!actor->alive) continue;
        if (actor->source_owner < 0 || actor->source_owner >= SC64_Z8_SOURCE_OWNER_LIMIT)
            return -1;
        owner = sc64_z8_world_owner_matches(w, a->group1, trigger_owner,
                                             (unsigned)actor->source_owner);
        if (owner < 0) return -1;
        if (!owner) continue;
        type = sc64_z8_world_actor_type(actor, a->unit);
        if (type < 0) return -1;
        if (!type) continue;
        overlap = sc64_z8_world_overlap(actor, rect);
        if (overlap < 0) return -1;
        if (!overlap) continue;
        if (actor->x > 32767 || actor->y > 32767) return -1;
        *x = (s16)((unsigned)actor->x * 2u);
        *y = (s16)((unsigned)actor->y * 2u);
        return 1;
    }
    return 0;
}

static inline int
sc64_z8_world_action(void *context, const Sc64Z8Action *a,
                     unsigned trigger_owner, const Sc64Z8Rect *rect)
{
    Sc64Z8WorldView *w = (Sc64Z8WorldView *)context;
    unsigned owner;
    if (!w || !a || !sc64_z8_world_actor_source_available(w)) return -1;
    switch (a->opcode) {
    case 11: /* CreateUnitWithProperties */
        if (a->unit >= SC64_Z8_RAW_UNIT_LIMIT || !rect || rect->left < 0 || rect->top < 0 ||
            rect->right < rect->left || rect->bottom < rect->top ||
            !w->services.create ||
            !sc64_z8_world_resolve_one(w, a->group1, trigger_owner, &owner)) return -1;
        return w->services.create(w->services.create_context ?
                                  w->services.create_context : w->services.context,
                                  a, owner,
                                  sc64_z8_create_count(a), a->group2,
                                  (unsigned)rect->left, (unsigned)rect->top,
                                  (unsigned)rect->right, (unsigned)rect->bottom) == 1
            ? 1 : -1;
    case 16: /* Unknown native policy must remain an explicit failure. */
        if (!rect || !sc64_z8_ai_script_known(a->group2) ||
            !w->services.run_ai) return -1;
        return w->services.run_ai(w->services.context, a->group2,
                                  trigger_owner, a->location, rect) == 1 ? 1 : -1;
    case 43: { /* Native modifier 4 enables; 5 disables. */
        int enabled;
        if (a->unit >= SC64_Z8_RAW_UNIT_LIMIT || !rect ||
            (a->modifier != 4u && a->modifier != 5u)) return -1;
        enabled = a->modifier == 4u;
        /* Validate the whole matching set before the first write so a bad
         * late view entry cannot leave a partial action behind. */
        for (unsigned pass = 0; pass < 2; ++pass) {
            for (unsigned i = 0; i < w->actor_count; ++i) {
                Sc64Z8ActorView actor_row;
                Sc64Z8ActorView *actor = &actor_row;
                int owner, type, overlap;
                if (sc64_z8_world_actor_at(w, i, actor) != 1) return -1;
                if (!actor->alive) continue;
                if (actor->source_owner < 0 || actor->source_owner >= SC64_Z8_SOURCE_OWNER_LIMIT)
                    return -1;
                owner = sc64_z8_world_owner_matches(w, a->group1, trigger_owner,
                                                     (unsigned)actor->source_owner);
                if (owner < 0) return -1;
                if (!owner) continue;
                type = sc64_z8_world_actor_type(actor, a->unit);
                if (type < 0) return -1;
                if (!type) continue;
                overlap = sc64_z8_world_overlap(actor, rect);
                if (overlap < 0) return -1;
                if (!overlap) continue;
                if (!actor->invincible) return -1;
                if (pass) *actor->invincible = (u8)enabled;
            }
        }
        return 1;
    }
    case 26: { /* Apply to exactly one resolved source owner. */
        if (!sc64_z8_world_resolve_one(w, a->group1, trigger_owner, &owner) ||
            a->unit > SC64_RESOURCE_BOTH ||
            (a->modifier != SC64_RESOURCE_MOD_SET &&
             a->modifier != SC64_RESOURCE_MOD_ADD &&
             a->modifier != SC64_RESOURCE_MOD_SUBTRACT)) return -1;
        if (w->wallet[owner])
            return sc64_resource_action_apply_one_player(w->wallet[owner],
                       a->unit, a->modifier, a->group2) ? 1 : -1;
        if (w->source_minerals[owner] && w->source_gas[owner]) {
            Sc64ResourceActionState resources = {
                *w->source_minerals[owner], *w->source_gas[owner], 0, 0
            };
            if (!sc64_resource_action_apply_one_player(&resources,
                    a->unit, a->modifier, a->group2)) return -1;
            *w->source_minerals[owner] = resources.current_minerals;
            *w->source_gas[owner] = resources.current_gas;
            return 1;
        }
        return -1;
    }
    case 8: case 9: case 12: case 28:
        if (!w->services.present || (a->location && !rect)) return -1;
        return w->services.present(w->services.context, a,
                                   trigger_owner, rect) == 1 ? 1 : -1;
    default:
        return -1; /* Scheduler handles switch/wait/terminal/timed/move. */
    }
}

static inline int
sc64_z8_world_timed(void *context, int phase, const Sc64Z8Action *a,
                    unsigned owner, const Sc64Z8Rect *rect, u32 *duration_ms)
{
    Sc64Z8WorldView *w = (Sc64Z8WorldView *)context;
    if (!w || !a || !w->services.timed || !duration_ms ||
        (a->location && !rect)) return -1;
    return w->services.timed(w->services.context, phase, a, owner, rect,
                             duration_ms);
}

static const Sc64Z8WorldOps sc64_z8_world_ops = {
    sc64_z8_world_count,
    sc64_z8_world_action,
    sc64_z8_world_timed,
    sc64_z8_world_find_unit
};

#endif /* SC64_Z8_WORLD_ADAPTER_H */
