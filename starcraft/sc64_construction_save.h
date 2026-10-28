/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_CONSTRUCTION_SAVE_H
#define SC64_CONSTRUCTION_SAVE_H

/* Standalone save fixtures compile without game.c's runtime order enum. */
#ifndef SC64_PENDING_BUILD_ORDER
#define SC64_PENDING_BUILD_ORDER(o) 0
#endif

#ifdef SC64_RACES
/* Pending construction stores a building type in target and the canonical
 * destination in tx/ty. Validate only durable identity here: resources,
 * prerequisites, placement and free slots can change while the worker moves. */
static int
sc64_pending_build_save_identity_valid(const RaceSave *candidate,
                                       int alive, int owner, int worker_type,
                                       int target_type, int tx, int ty,
                                       int world_width, int world_height)
{
    const Sc64RaceBuildingRule *building;
    int owner_race;

    if (!candidate || !alive || owner < 0 || owner >= 5 ||
        target_type < 0 || target_type >= SC_RB_COUNT ||
        tx < 0 || tx >= world_width || ty < 0 || ty >= world_height)
        return 0;
    owner_race = candidate->players[owner];
    if (owner_race < R_TER || owner_race > R_PROTOSS) return 0;
    building = sc64_race_building_rule(target_type);
    return building && building->worker == worker_type &&
           building->race == owner_race;
}

#else
/* Existing standalone cargo fixtures have no race/building schema. */
#define sc64_pending_build_save_identity_valid(...) 0
#endif

#endif
