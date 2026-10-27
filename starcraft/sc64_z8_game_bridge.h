/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_Z8_GAME_BRIDGE_H
#define SC64_Z8_GAME_BRIDGE_H

#if defined(SC64_Z8_GAME_POOL_BRIDGE)
#if !defined(SC64_RACES) || !defined(SC64_UNIFIED) || \
    !defined(SC64_ZP_CAMPAIGN_ACTORS) || \
    !defined(SC64_ZP_FULL_STORAGE) || !defined(SC64_ZP_CAMPAIGN_STORAGE) || \
    !defined(SC64_Z7_RUNTIME) || \
    !defined(SC64_P6_ACTORS) || !defined(SC64_P7_ACTORS) || \
    !defined(SC64_NYDUS_ENABLED)
#error "Z8 live pool bridge requires full ZP pools, P6/P7 source actors and Nydus"
#endif

#include "sc64_z8_world_adapter.h"
#include "sc64_p7_geometry.h"

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
        int ok = building ? sc64_z7_world_building_rect(raw, x, y,
                    &world_left, &world_top, &world_right, &world_bottom) :
                            sc64_z7_world_actor_rect(raw, x, y,
                    &world_left, &world_top, &world_right, &world_bottom);
        if (!ok) return 0;
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
        out->completed = 1; out->men =
            (sc64_game_unit_actor_flags(type) & SC64_GAME_ACTOR_MEN) != 0;
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
#endif /* SC64_Z8_GAME_POOL_BRIDGE */

#endif /* SC64_Z8_GAME_BRIDGE_H */
