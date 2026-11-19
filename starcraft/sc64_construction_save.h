/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_CONSTRUCTION_SAVE_H
#define SC64_CONSTRUCTION_SAVE_H

/* Standalone save fixtures compile without game.c's runtime order enum. */
#ifndef SC64_PENDING_BUILD_ORDER
#define SC64_PENDING_BUILD_ORDER(o) 0
#endif

#ifdef SC64_RACES
#include "sc64_skirmish_menu.h"

/* Pending construction stores a building type in target and the canonical
 * destination in tx/ty. Validate only durable identity here: resources,
 * prerequisites, placement and free slots can change while the worker moves. */
static int
sc64_pending_build_save_identity_valid(const RaceSave *candidate,
                                       int alive, int owner, int worker_type,
                                       int target_type, int tx, int ty,
                                       int world_width, int world_height)
{
    if (!candidate || !alive || owner < 0 || owner >= 5 ||
        target_type < 0 || target_type >= SC_RB_COUNT ||
        tx < 0 || tx >= world_width || ty < 0 || ty >= world_height)
        return 0;
    int owner_race = candidate->players[owner];
    if (owner_race < R_TER || owner_race > R_PROTOSS) return 0;
    const Sc64RaceBuildingRule *building = sc64_race_building_rule(target_type);
    if (!building || building->race != owner_race) return 0;
    if (building->worker != 255) return building->worker == worker_type;

    /* Some scenario variants, including Science Facility, use the
     * unavailable-worker sentinel. Only custom games can get the worker
     * from their own catalog, and only for the resolved runtime type. */
    static const unsigned char minimum_bots[5] = {0, 1, 4, 2, 3};
    int bots = candidate->state[4];
    /* Custom games number the CPUs 1, 3 and 4. Owner 2 is neutral. */
    if (candidate->state[0] != 1 || bots < 1 || bots > 3 ||
        bots < minimum_bots[owner] || !sc64_race_is_worker(worker_type))
        return 0;
    int raw = sc64_race_raw_building(target_type);
    if (sc64_race_building_from_raw(raw) != target_type) return 0;
    const Sc64SkirmishRecord *record = sc64_skirmish_menu_record((unsigned)owner_race, (unsigned)raw);
    return sc64_skirmish_menu_supported(record) && record->kind == SC64_SK_WORKER_BUILD &&
           record->tech_key == SC64_SK_NONE && sc64_race_raw_unit(worker_type) == record->producer_raw;
}

#else
/* Existing standalone cargo fixtures have no race/building schema. */
#define sc64_pending_build_save_identity_valid(...) 0
#endif

#endif
