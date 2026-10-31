/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_IMPACT_SAVE_TYPES_H
#define SC64_IMPACT_SAVE_TYPES_H

/* Capacity and SRAM record for pending projectile impacts.
 *
 * Every source-projectile product carries the same 32-row queue, so combat
 * and its overflow deferral behave identically. A save stores RaceSave
 * followed by the live rows. Products whose RaceSave nearly fills an SRAM
 * record can store fewer rows; a save holding more pending impacts than that
 * is refused (SAVE FAILED), never truncated or resolved early. */
#include "sc64_impact_events.h"

#define RACE_IMPACT_RECORD_MAX 16368 /* common/gba.c SAVE_PAYLOAD_MAX */
#define RACE_IMPACT_ROWS 32

enum {
    RACE_IMPACT_SAVE_ROOM = (RACE_IMPACT_RECORD_MAX - ((int)sizeof(RaceSave) - (int)SC64_BC_SAVE_EXTRA_BYTES) -
                             (int)sizeof(Sc64ImpactSaveHeader)) /
                            (int)sizeof(Sc64ImpactEvent),
#ifdef SC64_IMPACT_TEST_CAPACITY
    RACE_IMPACT_CAPACITY = SC64_IMPACT_TEST_CAPACITY,
#else
    RACE_IMPACT_CAPACITY = RACE_IMPACT_ROWS,
#endif
    RACE_IMPACT_SAVE_CAPACITY = RACE_IMPACT_SAVE_ROOM < RACE_IMPACT_CAPACITY ?
                                RACE_IMPACT_SAVE_ROOM : RACE_IMPACT_CAPACITY
};
_Static_assert(sizeof(Sc64ImpactEvent) == 6, "packed impact row");
_Static_assert(RACE_IMPACT_CAPACITY >= 1 &&
               RACE_IMPACT_CAPACITY <= (int)SC64_IMPACT_CAPACITY_LIMIT,
               "impact queue capacity");
#ifdef SC64_BATTLECRUISER_YAMATO_EXPERIMENT
/* The shot mask reserves two bits in state[28]. */
_Static_assert(SC64_BC_SHOT_LIMIT == 2, "BC save shot mask");
_Static_assert(RACE_IMPACT_SAVE_CAPACITY >= 8 || RACE_IMPACT_SAVE_CAPACITY == RACE_IMPACT_CAPACITY,
               "BC-free saves retain historical impact capacity");
#else
_Static_assert(RACE_IMPACT_SAVE_CAPACITY >= 8 || RACE_IMPACT_SAVE_CAPACITY == RACE_IMPACT_CAPACITY,
               "a save must hold at least eight pending impacts");
#endif
_Static_assert(MAX_UNITS + MAX_BUILDINGS <= SC64_IMPACT_TARGET_LIMIT,
               "impact target references use nine bits");
_Static_assert(SC_RU_COUNT <= SC64_IMPACT_TYPE_LIMIT,
               "impact source types use one byte below 64");

typedef struct {
    RaceSave save;
    u8 impacts[sizeof(Sc64ImpactSaveHeader) +
               RACE_IMPACT_SAVE_CAPACITY * sizeof(Sc64ImpactEvent)];
} RaceImpactRecord;
_Static_assert(sizeof(RaceImpactRecord) <= RACE_IMPACT_RECORD_MAX + SC64_BC_SAVE_EXTRA_BYTES,
               "impact record exceeds one SRAM record");

#endif
