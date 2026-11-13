/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_NUKE_SAVE_ROWS_H
#define SC64_NUKE_SAVE_ROWS_H
#include "sc64_nuke_rules.h"
#include "sc64_save_tag_namespace.h"
#include <stddef.h>

/* No additional journal bytes: these are projections of existing actor rows,
 * not structures to memcpy to SRAM. Never infer the interpretation from a
 * marker inside a legacy payload. */
#define SC64_NUKE_SAVE_NAMESPACE 0x004b0000u
enum {
    SC64_NUKE_SAVE_OWNER_COUNT = 5,
    SC64_NUKE_SAVE_UNIT_LIMIT = 240,
    SC64_NUKE_SAVE_BUILDING_LIMIT = 160,
    SC64_NUKE_SAVE_FLIGHT_LIMIT = 8,
    SC64_NUKE_SAVE_WORLD_LIMIT = 2048,
    SC64_NUKE_SAVE_AGE_LIMIT = 2047, /* the narrowest work_timer field */
    SC64_NUKE_SAVE_NO_PAYER = UINT8_MAX,
    SC64_NUKE_SAVE_NO_GHOST = UINT16_MAX,
    SC64_NUKE_SAVE_ARMED_PROGRESS = SC64_NUKE_MISSILE_SOURCE_TICKS + 1,
    SC64_NUKE_SAVE_BUILD_PAINTING = 0,
    SC64_NUKE_SAVE_BUILD_DESCENDING = 1
};
_Static_assert(SC64_NUKE_SAVE_ARMED_PROGRESS <= INT16_MAX, "legacy signed building progress");
_Static_assert(SC64_NUKE_SAVE_UNIT_LIMIT <= UINT8_MAX, "census unit slot width");

typedef struct {
    const void *candidate;
    uint32_t outer, inner;
} Sc64NukeSaveAuth;

static inline uint32_t sc64_nuke_save_tag(uint32_t current_outer)
{
    return SC64_SAVE_TAG_BASE_VALID(current_outer) ?
        (current_outer & ~SC64_SAVE_TAG_NAMESPACE_MASK) | SC64_NUKE_SAVE_NAMESPACE : 0;
}

/* Call only inside the checksum-validated journal callback, before inner
 * version migration, or for the writer's own prepared candidate. The exact
 * profile/current predecessor is required; matching namespace is insufficient.
 * For a P10 custom record, the adapter must also validate its research rows
 * under that product's custom-game semantics. */
static inline int sc64_nuke_save_authenticate(
    Sc64NukeSaveAuth *out, const void *candidate, uint32_t outer, uint32_t inner,
    uint32_t current_outer, uint32_t current_inner)
{
    if (!out) return 0;
    *out = (Sc64NukeSaveAuth){0};
    if (!candidate || !outer || !SC64_SAVE_TAG_BASE_VALID(current_inner) ||
        inner != current_inner || outer != sc64_nuke_save_tag(current_outer)) return 0;
    *out = (Sc64NukeSaveAuth){candidate, outer, inner};
    return 1;
}

static inline int sc64_nuke_save_auth_matches(
    const Sc64NukeSaveAuth *auth, const void *candidate, uint32_t unchanged_inner)
{
    return auth && candidate && auth->candidate == candidate &&
        auth->inner == unchanged_inner && SC64_SAVE_TAG_BASE_VALID(unchanged_inner) &&
        (auth->outer & SC64_SAVE_TAG_NAMESPACE_MASK) == SC64_NUKE_SAVE_NAMESPACE;
}

typedef struct {
    unsigned width, height, units, buildings, neutral_owner, owner_mask;
    int missile_actor_type;
} Sc64NukeSaveWorld;

static inline int sc64_nuke_save_world_valid(const Sc64NukeSaveWorld *world)
{
    return world && world->width && world->width <= SC64_NUKE_SAVE_WORLD_LIMIT &&
        world->height && world->height <= SC64_NUKE_SAVE_WORLD_LIMIT &&
        world->units && world->units <= SC64_NUKE_SAVE_UNIT_LIMIT &&
        world->buildings && world->buildings <= SC64_NUKE_SAVE_BUILDING_LIMIT &&
        world->neutral_owner < SC64_NUKE_SAVE_OWNER_COUNT &&
        (world->owner_mask >> SC64_NUKE_SAVE_OWNER_COUNT) == 0 &&
        (world->owner_mask & (1u << world->neutral_owner)) == 0 &&
        (unsigned)world->missile_actor_type < 64u;
}

static inline int sc64_nuke_save_owner_allowed(const Sc64NukeSaveWorld *world, int owner)
{
    return world && (unsigned)owner < SC64_NUKE_SAVE_OWNER_COUNT &&
        (world->owner_mask & (1u << (unsigned)owner)) != 0;
}

typedef struct {
    /* The adapter supplies decoded row facts, including exact mapped raw
     * identity and a matching, completed, grounded parent of the same owner.
     * train_allowed is the candidate's mission admission for raw 14. */
    int raw_type, owner, alive, complete, has_parent, train_allowed;
    int queue_count, queue_head;
} Sc64NukeSaveSiloFacts;

typedef struct {
    int queue_progress, level;
} Sc64NukeSaveSiloWire;

typedef struct {
    Sc64NukeSilo ammo;
    uint8_t payer;
} Sc64NukeSaveSilo;

/* Apply only to a recognized Silo row. A legacy reader must not interpret
 * paid state as ordinary build progress or discard it on load. */
static inline int sc64_nuke_save_legacy_silo_clear(const Sc64NukeSaveSiloWire *wire)
{
    return wire && !wire->queue_progress && !wire->level;
}

static inline int sc64_nuke_save_silo_decode(
    Sc64NukeSaveSilo *out, const Sc64NukeSaveSiloWire *wire,
    const Sc64NukeSaveSiloFacts *facts, const Sc64NukeSaveWorld *world,
    const Sc64NukeSaveAuth *auth, const void *candidate, uint32_t unchanged_inner)
{
    if (!out || !wire || !facts || !sc64_nuke_save_world_valid(world) ||
        !sc64_nuke_save_auth_matches(auth, candidate, unchanged_inner) ||
        facts->raw_type != SC64_NUKE_SILO_RAW ||
        (unsigned)facts->owner >= SC64_NUKE_SAVE_OWNER_COUNT ||
        (unsigned)facts->alive > 1 || (unsigned)facts->complete > 1 ||
        (unsigned)facts->has_parent > 1 || (unsigned)facts->train_allowed > 1 ||
        (facts->complete && !facts->alive) ||
        (facts->has_parent && (!facts->alive || (unsigned)facts->owner == world->neutral_owner)) ||
        facts->queue_count || facts->queue_head ||
        (unsigned)wire->queue_progress > SC64_NUKE_SAVE_ARMED_PROGRESS) return 0;
    Sc64NukeSaveSilo decoded = {{0, 0}, SC64_NUKE_SAVE_NO_PAYER};
    if (wire->queue_progress) {
        if (wire->level <= 0 || wire->level > SC64_NUKE_SAVE_OWNER_COUNT ||
            !sc64_nuke_save_owner_allowed(world, wire->level - 1) ||
            !facts->alive || !facts->complete) return 0;
        decoded.payer = (uint8_t)(wire->level - 1);
        if (wire->queue_progress == SC64_NUKE_SAVE_ARMED_PROGRESS) {
            decoded.ammo.armed = 1;
            /* A detached Silo can retain a paid missile and its old payer.
             * Reconnection moves payer and building owner together. */
            if ((unsigned)facts->owner != world->neutral_owner &&
                (!facts->has_parent || facts->owner != decoded.payer)) return 0;
        } else {
            if (!facts->train_allowed || !facts->has_parent || facts->owner != decoded.payer)
                return 0;
            decoded.ammo.remaining = (uint16_t)wire->queue_progress;
        }
    } else if (wire->level) return 0;
    *out = decoded;
    return 1;
}

static inline int sc64_nuke_save_silo_encode(
    Sc64NukeSaveSiloWire *out, const Sc64NukeSaveSilo *state,
    const Sc64NukeSaveSiloFacts *facts, const Sc64NukeSaveWorld *world,
    const Sc64NukeSaveAuth *auth, const void *candidate, uint32_t unchanged_inner)
{
    if (!out || !state || state->ammo.armed > 1 ||
        state->ammo.remaining > SC64_NUKE_MISSILE_SOURCE_TICKS ||
        (state->ammo.armed && state->ammo.remaining)) return 0;
    Sc64NukeSaveSiloWire wire = {0, 0};
    if (state->ammo.armed || state->ammo.remaining) {
        wire.queue_progress = state->ammo.armed ?
            SC64_NUKE_SAVE_ARMED_PROGRESS : state->ammo.remaining;
        wire.level = (int)state->payer + 1;
    } else if (state->payer != SC64_NUKE_SAVE_NO_PAYER) return 0;
    Sc64NukeSaveSilo checked;
    if (!sc64_nuke_save_silo_decode(&checked, &wire, facts, world,
                                   auth, candidate, unchanged_inner)) return 0;
    *out = wire;
    return 1;
}

typedef struct {
    /* x/y are the launch origin, not the moving sprite's screen position.
     * tx/ty are the fixed strike point; work_timer is source age. */
    int type, raw_type, owner, alive, selected, idle;
    int x, y, tx, ty, work_timer, target, build;
} Sc64NukeSaveMissileWire;

typedef struct {
    Sc64NukeFlight strike;
    uint16_t origin_x, origin_y;
} Sc64NukeSaveMissile;

typedef struct {
    int alive, ordinary, owner, can_track;
} Sc64NukeSaveGhostFacts;
typedef int (*Sc64NukeSaveReadGhost)(void *context, unsigned slot, Sc64NukeSaveGhostFacts *out);
typedef int (*Sc64NukeSaveCadenceForPoint)(
    void *context, unsigned origin_x, unsigned origin_y, unsigned target_x, unsigned target_y,
    Sc64NukeCadence *out);

/* Check both identities before allowing an ordinary actor decoder/legacy
 * world to accept a row. For authentic rows they must agree. */
static inline int sc64_nuke_save_missile_claimed(
    const Sc64NukeSaveMissileWire *wire, const Sc64NukeSaveWorld *world)
{
    return wire && world && (wire->type == world->missile_actor_type ||
                             wire->raw_type == SC64_NUKE_MISSILE_RAW);
}

static inline int sc64_nuke_save_missile_decode(
    Sc64NukeSaveMissile *out, unsigned slot, const Sc64NukeSaveMissileWire *wire,
    const Sc64NukeSaveWorld *world, const Sc64NukeSaveAuth *auth,
    const void *candidate, uint32_t unchanged_inner, Sc64NukeSaveCadenceForPoint cadence_for,
    Sc64NukeSaveReadGhost read_ghost, void *context)
{
    if (!out || !wire || !sc64_nuke_save_world_valid(world) ||
        !sc64_nuke_save_auth_matches(auth, candidate, unchanged_inner) ||
        slot >= world->units || wire->type != world->missile_actor_type ||
        wire->raw_type != SC64_NUKE_MISSILE_RAW || wire->alive != 1 ||
        wire->selected != 0 || wire->idle != 1 ||
        !sc64_nuke_save_owner_allowed(world, wire->owner) ||
        (unsigned)wire->x >= world->width || (unsigned)wire->tx >= world->width ||
        (unsigned)wire->y >= world->height || (unsigned)wire->ty >= world->height ||
        (unsigned)wire->work_timer > SC64_NUKE_SAVE_AGE_LIMIT || !cadence_for)
        return 0;
    Sc64NukeCadence cadence;
    if (!cadence_for(context, (unsigned)wire->x, (unsigned)wire->y,
                     (unsigned)wire->tx, (unsigned)wire->ty, &cadence) ||
        !sc64_nuke_cadence_valid(cadence) ||
        (unsigned)cadence.paint + cadence.descent > SC64_NUKE_SAVE_AGE_LIMIT + 1u)
        return 0;
    Sc64NukeSaveMissile decoded = {
        .strike = {.x = (uint16_t)wire->tx, .y = (uint16_t)wire->ty,
                   .spotter = SC64_NUKE_SAVE_NO_GHOST, .age = (uint16_t)wire->work_timer,
                   .owner = (uint8_t)wire->owner, .cadence = cadence},
        .origin_x = (uint16_t)wire->x, .origin_y = (uint16_t)wire->y
    };
    if (wire->build == SC64_NUKE_SAVE_BUILD_PAINTING) {
        Sc64NukeSaveGhostFacts ghost;
        if (decoded.strike.age >= cadence.paint || (unsigned)wire->target >= world->units ||
            (unsigned)wire->target == slot || !read_ghost ||
            !read_ghost(context, (unsigned)wire->target, &ghost) ||
            ghost.alive != 1 || ghost.ordinary != 1 || ghost.can_track != 1 ||
            ghost.owner != wire->owner) return 0;
        decoded.strike.phase = SC64_NUKE_PAINTING;
        decoded.strike.spotter = (uint16_t)wire->target;
    } else if (wire->build == SC64_NUKE_SAVE_BUILD_DESCENDING) {
        if (wire->target != -1 || decoded.strike.age < cadence.paint ||
            (unsigned)decoded.strike.age >= (unsigned)cadence.paint + cadence.descent)
            return 0;
        decoded.strike.phase = SC64_NUKE_DESCENDING;
    } else return 0;
    *out = decoded;
    return 1;
}

static inline int sc64_nuke_save_missile_encode(
    Sc64NukeSaveMissileWire *out, unsigned slot, const Sc64NukeSaveMissile *state,
    const Sc64NukeSaveWorld *world, const Sc64NukeSaveAuth *auth,
    const void *candidate, uint32_t unchanged_inner, Sc64NukeSaveCadenceForPoint cadence_for,
    Sc64NukeSaveReadGhost read_ghost, void *context)
{
    if (!out || !state || !world ||
        (state->strike.phase != SC64_NUKE_PAINTING && state->strike.phase != SC64_NUKE_DESCENDING))
        return 0;
    int painting = state->strike.phase == SC64_NUKE_PAINTING;
    const Sc64NukeSaveMissileWire wire = {
        .type = world->missile_actor_type, .raw_type = SC64_NUKE_MISSILE_RAW,
        .owner = state->strike.owner, .alive = 1, .selected = 0, .idle = 1,
        .x = state->origin_x, .y = state->origin_y,
        .tx = state->strike.x, .ty = state->strike.y, .work_timer = state->strike.age,
        .target = painting ? (int)state->strike.spotter : -1,
        .build = painting ? SC64_NUKE_SAVE_BUILD_PAINTING : SC64_NUKE_SAVE_BUILD_DESCENDING
    };
    Sc64NukeSaveMissile checked;
    if (!sc64_nuke_save_missile_decode(&checked, slot, &wire, world, auth, candidate,
                                      unchanged_inner, cadence_for, read_ghost, context) ||
        checked.strike.cadence.paint != state->strike.cadence.paint ||
        checked.strike.cadence.descent != state->strike.cadence.descent ||
        checked.strike.cadence.warning != state->strike.cadence.warning) return 0;
    *out = wire;
    return 1;
}

/* Zero-init once per detached validation pass. Call only for rows that have
 * passed the codecs above. Counts are for reconstruction, not a supply cap:
 * stock can go over the admission limit when losing providers or gaining a
 * captured armed Silo. A pending approach also holds a track slot, but has
 * not paid for or reserved a second missile. */
typedef struct {
    uint16_t reserved[SC64_NUKE_SAVE_OWNER_COUNT];
    uint8_t paid_silos[(SC64_NUKE_SAVE_BUILDING_LIMIT + 7) / 8];
    uint8_t missile_slots[SC64_NUKE_SAVE_FLIGHT_LIMIT];
    uint8_t tracked_ghosts[SC64_NUKE_SAVE_FLIGHT_LIMIT];
    uint8_t missiles, approaches, tracks;
} Sc64NukeSaveCensus;

static inline int sc64_nuke_save_count_silo(
    Sc64NukeSaveCensus *census, const Sc64NukeSaveWorld *world,
    unsigned slot, const Sc64NukeSaveSilo *state)
{
    if (!census || !state || !sc64_nuke_save_world_valid(world) ||
        slot >= world->buildings || state->ammo.armed > 1 ||
        state->ammo.remaining > SC64_NUKE_MISSILE_SOURCE_TICKS ||
        (state->ammo.armed && state->ammo.remaining)) return 0;
    if (!sc64_nuke_silo_supply(&state->ammo))
        return state->payer == SC64_NUKE_SAVE_NO_PAYER;
    unsigned mask = 1u << (slot & 7u);
    if (!sc64_nuke_save_owner_allowed(world, state->payer) ||
        (census->paid_silos[slot >> 3] & mask)) return 0;
    census->paid_silos[slot >> 3] |= (uint8_t)mask;
    census->reserved[state->payer] += SC64_NUKE_MISSILE_SUPPLY;
    return 1;
}

static inline int sc64_nuke_save_count_track(
    const Sc64NukeSaveCensus *census, const Sc64NukeSaveWorld *world, unsigned ghost)
{
    if (ghost >= world->units || census->tracks >= SC64_NUKE_SAVE_FLIGHT_LIMIT) return 0;
    for (unsigned i = 0; i < census->tracks; ++i)
        if (census->tracked_ghosts[i] == ghost) return 0;
    for (unsigned i = 0; i < census->missiles; ++i)
        if (census->missile_slots[i] == ghost) return 0;
    return 1;
}

static inline int sc64_nuke_save_count_approach(
    Sc64NukeSaveCensus *census, const Sc64NukeSaveWorld *world, unsigned ghost)
{
    if (!census || !sc64_nuke_save_world_valid(world) ||
        (unsigned)census->missiles + census->approaches >= SC64_NUKE_SAVE_FLIGHT_LIMIT ||
        !sc64_nuke_save_count_track(census, world, ghost)) return 0;
    census->tracked_ghosts[census->tracks++] = (uint8_t)ghost;
    ++census->approaches;
    return 1;
}

static inline int sc64_nuke_save_count_missile(
    Sc64NukeSaveCensus *census, const Sc64NukeSaveWorld *world,
    unsigned slot, const Sc64NukeSaveMissile *state)
{
    if (!census || !state || !sc64_nuke_save_world_valid(world) ||
        slot >= world->units || !sc64_nuke_save_owner_allowed(world, state->strike.owner) ||
        (unsigned)census->missiles + census->approaches >= SC64_NUKE_SAVE_FLIGHT_LIMIT)
        return 0;
    for (unsigned i = 0; i < census->missiles; ++i)
        if (census->missile_slots[i] == slot) return 0;
    for (unsigned i = 0; i < census->tracks; ++i)
        if (census->tracked_ghosts[i] == slot) return 0;
    if (state->strike.phase == SC64_NUKE_PAINTING) {
        if (state->strike.spotter == slot ||
            !sc64_nuke_save_count_track(census, world, state->strike.spotter)) return 0;
    } else if (state->strike.phase != SC64_NUKE_DESCENDING ||
               state->strike.spotter != SC64_NUKE_SAVE_NO_GHOST) return 0;
    if (state->strike.phase == SC64_NUKE_PAINTING)
        census->tracked_ghosts[census->tracks++] = (uint8_t)state->strike.spotter;
    census->missile_slots[census->missiles++] = (uint8_t)slot;
    census->reserved[state->strike.owner] += SC64_NUKE_MISSILE_SUPPLY;
    return 1;
}
#endif
