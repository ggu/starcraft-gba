/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Asset-free identity checks for persistent construction travel intents. */
#include <assert.h>
#include <stdio.h>
#define SC64_RACES 1
#include "../starcraft/race_rules.h"

typedef struct {
    int state[5];
    int players[5];
} RaceSave;

#include "../starcraft/sc64_construction_save.h"

static int valid(const RaceSave *candidate, int owner, int worker, int target)
{
    return sc64_pending_build_save_identity_valid(candidate, 1, owner, worker,
                                                   target, 100, 120, 512, 384);
}

int main(void)
{
    RaceSave candidate = {
        .state = {[4] = 3},
        .players = {R_TER, R_ZERG, -1, R_PROTOSS, R_TER}
    };
    int lab = sc64_race_building_from_raw(118);
    int science = sc64_race_building_from_raw(116);
    int silo = sc64_race_building_from_raw(108);
    int support_bay = sc64_race_building_from_raw(171);

    assert(valid(&candidate, 0, U_SCV, B_BARRACKS));
    assert(valid(&candidate, 1, U_DRONE, B_SPAWNING_POOL));
    assert(valid(&candidate, 3, U_PROBE, B_PYLON));
    assert(!valid(&candidate, 0, U_DRONE, B_BARRACKS));
    assert(!valid(&candidate, 0, U_SCV, B_SPAWNING_POOL));
    assert(!valid(&candidate, 0, U_SCV, SC_RB_COUNT));
    assert(!valid(&candidate, 5, U_SCV, B_BARRACKS));
    assert(!valid(0, 0, U_SCV, B_BARRACKS));
    assert(!sc64_pending_build_save_identity_valid(&candidate, 0, 0, U_SCV,
                                                    B_BARRACKS, 100, 120, 512, 384));
    assert(!sc64_pending_build_save_identity_valid(&candidate, 1, 0, U_SCV,
                                                    B_BARRACKS, 512, 120, 512, 384));
    assert(!sc64_pending_build_save_identity_valid(&candidate, 1, 0, U_SCV,
                                                    B_BARRACKS, 100, -1, 512, 384));
    candidate.players[0] = -1;
    assert(!valid(&candidate, 0, U_SCV, B_BARRACKS));
    candidate.players[0] = R_TER;

#ifdef SC64_TERRAN_LATE_ACTORS
    assert(science == B_SCIENCE_FACILITY);
#elif defined(SC64_SKIRMISH_ROSTER)
    assert(science == B_SK_SCIENCE_FACILITY);
#elif defined(SC64_BATTLECRUISER_EXPERIMENT)
    assert(science == B_SHARED_SCIENCE_FACILITY);
#else
    assert(science == B_NUKE_SCIENCE_FACILITY);
#endif
    assert(sc64_race_building_rule(science)->worker == 255);
    assert(!valid(&candidate, 0, U_SCV, science));

    candidate.state[0] = 1;
    assert(valid(&candidate, 0, U_SCV, science));
    assert(valid(&candidate, 4, U_SCV, science));
    assert(!valid(&candidate, 0, U_MARINE, science));
    assert(!valid(&candidate, 0, U_PROBE, science));
    assert(!valid(&candidate, 1, U_DRONE, science));
    assert(!valid(&candidate, 1, U_SCV, science));
    assert(!valid(&candidate, 3, U_PROBE, science));
    assert(!valid(&candidate, 0, 255, science));
    assert(!valid(&candidate, 0, U_SCV, lab));
    assert(!valid(&candidate, 0, U_SCV, silo));
    assert(!valid(&candidate, 3, U_PROBE, support_bay));
#ifdef SC64_TERRAN_LATE_ACTORS
    assert(valid(&candidate, 0, U_SCV, B_ARMORY));
    assert(valid(&candidate, 3, U_PROBE, B_SHIELD_BATTERY));
#endif
    candidate.players[2] = R_TER;
    assert(!valid(&candidate, 2, U_SCV, science));
    candidate.state[4] = 2;
    assert(!valid(&candidate, 4, U_SCV, science));
    candidate.state[0] = 0;
    assert(!valid(&candidate, 0, 255, science));
    candidate.state[0] = 2;
    assert(!valid(&candidate, 0, U_SCV, science));

    printf("construction save identity: Science Facility internal type %d and raw 116 pass\n",
           science);
    return 0;
}
