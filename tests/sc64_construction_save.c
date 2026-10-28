/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Asset-free identity checks for persistent construction travel intents. */
#include <assert.h>
#include <stdio.h>

enum { R_TER, R_ZERG, R_PROTOSS };
enum { SC_RB_COUNT = 3 };
typedef struct {
    int players[5];
} RaceSave;
typedef struct {
    int race, worker;
} Sc64RaceBuildingRule;

static const Sc64RaceBuildingRule rules[SC_RB_COUNT] = {
    { R_TER, 10 }, { R_ZERG, 11 }, { R_PROTOSS, 12 }
};

static const Sc64RaceBuildingRule *sc64_race_building_rule(int type)
{
    return type >= 0 && type < SC_RB_COUNT ? &rules[type] : 0;
}

#include "../starcraft/sc64_construction_save.h"

int main(void)
{
    RaceSave candidate = {{R_TER, R_ZERG, R_PROTOSS, R_TER, R_ZERG}};

    assert(sc64_pending_build_save_identity_valid(&candidate, 1, 0, 10,
                                                   0, 100, 120, 512, 384));
    assert(sc64_pending_build_save_identity_valid(&candidate, 1, 1, 11,
                                                   1, 100, 120, 512, 384));
    assert(sc64_pending_build_save_identity_valid(&candidate, 1, 2, 12,
                                                   2, 100, 120, 512, 384));
    assert(!sc64_pending_build_save_identity_valid(&candidate, 0, 0, 10,
                                                    0, 100, 120, 512, 384));
    assert(!sc64_pending_build_save_identity_valid(&candidate, 1, 0, 11,
                                                    0, 100, 120, 512, 384));
    assert(!sc64_pending_build_save_identity_valid(&candidate, 1, 0, 10,
                                                    1, 100, 120, 512, 384));
    assert(!sc64_pending_build_save_identity_valid(&candidate, 1, 0, 10,
                                                    3, 100, 120, 512, 384));
    assert(!sc64_pending_build_save_identity_valid(&candidate, 1, 0, 10,
                                                    0, 512, 120, 512, 384));
    assert(!sc64_pending_build_save_identity_valid(&candidate, 1, 0, 10,
                                                    0, 100, -1, 512, 384));
    candidate.players[0] = -1;
    assert(!sc64_pending_build_save_identity_valid(&candidate, 1, 0, 10,
                                                    0, 100, 120, 512, 384));
    puts("construction save identity: valid race/worker/site and malformed intent cases pass");
    return 0;
}
