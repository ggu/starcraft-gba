/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_Z8_GAME_BRIDGE_H
#define SC64_Z8_GAME_BRIDGE_H

#if defined(SC64_Z8_GAME_POOL_BRIDGE)
#if !defined(SC64_RACES) || !defined(SC64_UNIFIED) || \
    !defined(SC64_ZP_CAMPAIGN_ACTORS) || \
    !defined(SC64_ZP_FULL_STORAGE) || !defined(SC64_ZP_CAMPAIGN_STORAGE) || \
    !defined(SC64_P6_ACTORS) || !defined(SC64_P7_ACTORS) || \
    !((defined(SC64_Z7_RUNTIME) && defined(SC64_NYDUS_ENABLED)) || \
      defined(SC64_P8_RUNTIME))
#error "Z8 live pool bridge requires full ZP pools, P6/P7 source actors and the Z7/Nydus or P8 runtime"
#endif

#include "sc64_z8_world_adapter.h"
#include "sc64_z8_source.h"
#include "sc64_p7_geometry.h"
#ifndef SC64_Z7_RUNTIME
#include "sc64_siege_source_bounds.h"
#endif

typedef struct {
    s8 live_owner_for_source[SC64_Z8_SOURCE_OWNER_LIMIT];
    u16 map_pixel_width, map_pixel_height;
} Sc64Z8GameContext;

static int
sc64_z8_game_context_init(Sc64Z8GameContext *context,
                          const s8 live_owner_for_source[SC64_Z8_SOURCE_OWNER_LIMIT],
                          unsigned map_pixel_width, unsigned map_pixel_height)
{
    if (!context || !live_owner_for_source || !map_pixel_width ||
        !map_pixel_height || map_pixel_width > (unsigned)WORLD_W * 2u ||
        map_pixel_height > (unsigned)WORLD_H * 2u ||
        map_pixel_width != SC64_Z8_SOURCE_MAP_COLS * 32u ||
        map_pixel_height != SC64_Z8_SOURCE_MAP_ROWS * 32u)
        return 0;
    for (unsigned owner = 0; owner < SC64_Z8_SOURCE_OWNER_LIMIT; ++owner) {
        int live = live_owner_for_source[owner];
        if (owner < SC64_Z8_OWNER_LIMIT) {
            if (live < -1 || live > 4) return 0;
        } else if (live != -1) {
            return 0;
        }
        context->live_owner_for_source[owner] = (s8)live;
    }
    context->map_pixel_width = (u16)map_pixel_width;
    context->map_pixel_height = (u16)map_pixel_height;
    return 1;
}

/* Return actor extents in the world-view's source-pixel offset form. Prefer
 * feature facts, then the pinned P7 raw helper and Z7's audited raw table for
 * base actors that have no ZP feature-fact row. */
static int
sc64_z8_game_source_extents(unsigned raw, int x, int y, int building,
                            int *left, int *top, int *right, int *bottom)
{
    int world_left, world_top, world_right, world_bottom;
    if (!left || !top || !right || !bottom || x < 0 || y < 0) return 0;
    if (building && sc64_race_building_from_raw((int)raw) >= 0 &&
        sc64_game_building_bounds_source(sc64_race_building_from_raw((int)raw),
                                         left, top, right, bottom))
        return *left >= 0 && *top >= 0 && *right >= 0 && *bottom >= 0;
    if (!sc64_p7_actor_rect_source(raw, x, y, building,
                                   &world_left, &world_top,
                                   &world_right, &world_bottom)) {
        /* Z7 already carries pinned raw dimensions for standard Zerg actors
         * and structures that are outside the ZP feature-fact table. */
#ifdef SC64_Z7_RUNTIME
        int ok = building ? sc64_z7_world_building_rect(raw, x, y,
                    &world_left, &world_top, &world_right, &world_bottom) :
                            sc64_z7_world_actor_rect(raw, x, y,
                    &world_left, &world_top, &world_right, &world_bottom);
        if (!ok) return 0;
#else
        /* Without the Z7 table, fall back to the pinned BWAPI
         * unitDimensions rows (inclusive edge offsets). Every P8 actor has
         * a feature fact or P7 rect; P9's Zerg roster uses this path. */
        if (raw >= 228u) return 0;
        world_left = x * 2 - siege_bounds[raw][2];
        world_top = y * 2 - siege_bounds[raw][3];
        world_right = x * 2 + siege_bounds[raw][4] + 1;
        world_bottom = y * 2 + siege_bounds[raw][5] + 1;
#endif
    }
    *left = x * 2 - world_left;
    *top = y * 2 - world_top;
    *right = world_right - x * 2 - 1;
    *bottom = world_bottom - y * 2 - 1;
    return *left >= 0 && *top >= 0 && *right >= 0 && *bottom >= 0;
}

/* Read directly from the bounded game pools. The provider enumerates pool
 * capacity so newly created actors are visible to later trigger callbacks in
 * the same pass. `raw_owner` preserves the CHK slot even when engine owner
 * modes collapse multiple source players to SIDE/ENEMY. */
static int
sc64_z8_game_actor_at(void *context, unsigned index, Sc64Z8ActorView *out)
{
    int left, up, right, down;
    (void)context;
    if (!out || index >= MAX_UNITS + MAX_BUILDINGS) return -1;
    if (index < MAX_UNITS) {
        const Unit *actor = &u[index];
        int type;
        *out = (Sc64Z8ActorView){0};
        out->alive = actor->alive != 0;
        if (!out->alive) return 1;
        if (actor->raw_owner < 0 || actor->raw_owner >= SC64_Z8_SOURCE_OWNER_LIMIT)
            return -1;
        type = sc64_race_unit_from_raw(actor->raw_type);
        if (type < 0 || !sc64_z8_game_source_extents((unsigned)actor->raw_type,
                actor->x, actor->y, 0, &left, &up, &right, &down))
            return -1;
        out->raw_type = (u16)actor->raw_type;
        out->source_owner = actor->raw_owner;
        out->x = (s16)actor->x; out->y = (s16)actor->y;
        out->left = (s16)left; out->top = (s16)up;
        out->right = (s16)right; out->bottom = (s16)down;
        out->completed = 1;
#ifdef SC64_Z7_EGG_PRODUCTION
        /* An Egg is in progress until the unit tick changes its type to the
         * paid product. P7's separate Interceptor training state does not
         * change generic completion for hidden hangar children. */
        if (actor->type == U_Z7_EGG) out->completed = 0;
#endif
        out->men = (sc64_game_unit_actor_flags(type) & SC64_GAME_ACTOR_MEN) != 0;
        out->loaded = SC64_TRANSPORT_LOADED(actor) != 0;
        out->type_known = 1;
        out->invincible = &invincible_units[index];
        return 1;
    }
    {
        unsigned slot = index - MAX_UNITS;
        const Building *actor = &b[slot];
        int type;
        *out = (Sc64Z8ActorView){0};
        out->alive = actor->alive != 0;
        if (!out->alive) return 1;
        if (actor->raw_owner < 0 || actor->raw_owner >= SC64_Z8_SOURCE_OWNER_LIMIT)
            return -1;
        type = sc64_race_building_from_raw(actor->raw_type);
        if (type < 0 || !sc64_z8_game_source_extents((unsigned)actor->raw_type,
                actor->x, actor->y, 1, &left, &up, &right, &down))
            return -1;
        out->raw_type = (u16)actor->raw_type;
        out->source_owner = actor->raw_owner;
        out->x = (s16)actor->x; out->y = (s16)actor->y;
        out->left = (s16)left; out->top = (s16)up;
        out->right = (s16)right; out->bottom = (s16)down;
        out->completed = !actor->under_construction;
        out->building = 1; out->type_known = 1;
        out->invincible = &invincible_buildings[slot];
        return 1;
    }
}

static void
sc64_z8_game_bind_actor_pool(Sc64Z8WorldView *world)
{
    if (!world) return;
    world->actors = 0;
    world->actor_count = (u16)(MAX_UNITS + MAX_BUILDINGS);
    world->actor_context = 0;
    world->actor_at = sc64_z8_game_actor_at;
}

static void
sc64_z8_game_bind_source_wallets(Sc64Z8WorldView *world,
                                u32 minerals[SC64_Z8_OWNER_LIMIT],
                                u32 gas[SC64_Z8_OWNER_LIMIT])
{
    if (!world || !minerals || !gas) return;
    for (unsigned owner = 0; owner < SC64_Z8_OWNER_LIMIT; ++owner) {
        world->wallet[owner] = 0;
        world->source_minerals[owner] = &minerals[owner];
        world->source_gas[owner] = &gas[owner];
    }
}

/* CreateUnit rows currently authored in Z8 use only raw 74 (Hero Dark
 * Templar) and raw 101 (Special Map Revealer), one unit per action, with no
 * UPRP selector. Both types had bit 0 clear in an initialized retail P1
 * snapshot at native table 0x800C99DC + 4*type. Native code centers the
 * requested location and clamps the actor footprint to the map; these rows
 * bypass the building grid/placement preflight and retain the unsnapped
 * center. The hidden raw-101 actor is created with the configured live owner
 * and exact source owner. This does not implement owner-specific enemy fog
 * awareness or enable the Z8 gameplay profile.
 */
static int
sc64_z8_game_create(void *opaque, const Sc64Z8Action *action,
                    unsigned source_owner, unsigned count,
                    unsigned property_id, unsigned x1, unsigned y1,
                    unsigned x2, unsigned y2)
{
    Sc64Z8GameContext *context = (Sc64Z8GameContext *)opaque;
    int raw, type, live_owner, left, top, right, bottom;
    int x, y, slot;
    if (!context || !action || action->opcode != 11u ||
        action->flags != 0x10u || action->modifier != 0u ||
        action->group2 != 0u || property_id != 0u || count != 1u ||
        !action->location || action->string_id || action->wav_id ||
        action->time || source_owner >= SC64_Z8_OWNER_LIMIT ||
        source_owner >= SC64_Z8_SOURCE_OWNER_LIMIT ||
        context->live_owner_for_source[source_owner] < SIDE ||
        context->live_owner_for_source[source_owner] > 4 ||
        x2 < x1 || y2 < y1 || x2 > context->map_pixel_width ||
        y2 > context->map_pixel_height ||
        context->map_pixel_width != (unsigned)WORLD_W * 2u ||
        context->map_pixel_height != (unsigned)WORLD_H * 2u)
        return -1;

    raw = action->unit;
    if (raw != 74 && raw != 101) return -1;
    type = sc64_race_unit_from_raw(raw);
    if (type < 0 || !sc64_z8_game_source_extents((unsigned)raw, 0, 0, 0,
                                                 &left, &top, &right, &bottom))
        return -1;

    /* The native midpoint is integer-truncated for these nonnegative source
     * coordinates. Its unit center is then clamped so inclusive footprint
     * edges lie inside [0, map_dimension). No tile-grid snap is used here. */
    x = (int)((x1 + x2) / 2u);
    y = (int)((y1 + y2) / 2u);
    if ((unsigned)left > context->map_pixel_width ||
        (unsigned)top > context->map_pixel_height ||
        (unsigned)right >= context->map_pixel_width ||
        (unsigned)bottom >= context->map_pixel_height)
        return -1;
    if (x < left) x = left;
    if (y < top) y = top;
    if (x + right >= context->map_pixel_width)
        x = context->map_pixel_width - right - 1;
    if (y + bottom >= context->map_pixel_height)
        y = context->map_pixel_height - bottom - 1;
    if (x < left || y < top || x + right >= context->map_pixel_width ||
        y + bottom >= context->map_pixel_height)
        return -1;

    live_owner = context->live_owner_for_source[source_owner];
    slot = find_unit_slot();
    if (slot < 0) return -1;
    slot = spawn_unit(live_owner, type, x / 2, y / 2);
    if (slot < 0) return -1;
    u[slot].raw_type = (s16)raw;
    u[slot].raw_owner = (s16)source_owner;
    return 1;
}

static void
sc64_z8_game_bind_create_service(Sc64Z8WorldView *world,
                                 Sc64Z8GameContext *context)
{
    if (!world || !context) return;
    world->services.create = sc64_z8_game_create;
    world->services.create_context = context;
}
#endif /* SC64_Z8_GAME_POOL_BRIDGE */

#endif /* SC64_Z8_GAME_BRIDGE_H */
