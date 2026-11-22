/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_IMPACT_SAVE_TYPES_H
#define SC64_IMPACT_SAVE_TYPES_H

/* Capacity and SRAM record for pending projectile impacts.
 *
 * Every source-projectile product carries the same 32-row queue, so combat
 * and its overflow deferral behave identically. A save normally appends the
 * live rows to RaceSave. Standalone P6 uses reserved building-row bits for
 * larger queues. A record that cannot hold the pending impacts is refused
 * (SAVE FAILED) without truncating the queue or resolving impacts early. */
#include "sc64_impact_events.h"

#define RACE_IMPACT_RECORD_MAX 16368 /* common/gba.c SAVE_PAYLOAD_MAX */
#define RACE_IMPACT_ROWS 32
#ifdef SC64_SHARED_TRANSPORT
#define SC64_TRANSPORT_TRAILER_BYTES 16
#else
#define SC64_TRANSPORT_TRAILER_BYTES 0
#endif

#ifdef SC64_P6_PACKED_IMPACT_SAVE
#if !defined(SC64_P6_RUNTIME) || !defined(SC64_ZP_FULL_STORAGE) || !defined(SC64_UNIFIED) || \
    SC64_CAMPAIGN_RACE != 2 || defined(SC64_P7_BATTERY_ENABLED) || \
    defined(SC64_SHARED_TRANSPORT) || defined(SC64_WORKER_CARGO_KIND) || \
    defined(SC64_BATTLECRUISER_ENERGY_EXPERIMENT) || defined(SC64_BATTLECRUISER_YAMATO_EXPERIMENT) || \
    defined(SC64_EXPERIMENTAL_RESEARCH_JOURNAL)
#error "packed P6 impacts require the standalone Protoss P6 save layout"
#endif
enum {
    SC64_P6_IMPACT_PAD_BITS = 7,
    SC64_P6_IMPACT_PAD_BYTES = MAX_BUILDINGS * SC64_P6_IMPACT_PAD_BITS / 8,
    SC64_P6_IMPACT_MARKER_BYTES = 4
};
_Static_assert(sizeof(RaceSave) == 16348 && MAX_BUILDINGS == 160 &&
               sizeof(((RaceSave *)0)->buildings[0]) == SC64_PACKED_BUILDING_SAVE_BYTES &&
               SC64_PACKED_BUILDING_SAVE_USED_BITS + SC64_P6_IMPACT_PAD_BITS ==
               SC64_PACKED_BUILDING_SAVE_BYTES * 8,
               "P6 impact envelope must preserve the legacy save and building fields");
#endif

enum {
    RACE_IMPACT_RAW_SAVE_ROOM = (RACE_IMPACT_RECORD_MAX - SC64_TRANSPORT_TRAILER_BYTES - ((int)sizeof(RaceSave) - (int)SC64_BC_SAVE_EXTRA_BYTES) -
                                 (int)sizeof(Sc64ImpactSaveHeader)) /
                                (int)sizeof(Sc64ImpactEvent),
#ifdef SC64_P6_PACKED_IMPACT_SAVE
    RACE_IMPACT_SAVE_ROOM = (SC64_P6_IMPACT_PAD_BYTES - sizeof(Sc64ImpactSaveHeader)) /
                            sizeof(Sc64ImpactEvent),
#else
    RACE_IMPACT_SAVE_ROOM = RACE_IMPACT_RAW_SAVE_ROOM,
#endif
#ifdef SC64_IMPACT_TEST_CAPACITY
    RACE_IMPACT_CAPACITY = SC64_IMPACT_TEST_CAPACITY,
#else
    RACE_IMPACT_CAPACITY = RACE_IMPACT_ROWS,
#endif
    RACE_IMPACT_RAW_SAVE_CAPACITY = RACE_IMPACT_RAW_SAVE_ROOM < RACE_IMPACT_CAPACITY ?
                                    RACE_IMPACT_RAW_SAVE_ROOM : RACE_IMPACT_CAPACITY,
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
               RACE_IMPACT_RAW_SAVE_CAPACITY * sizeof(Sc64ImpactEvent) +
               SC64_TRANSPORT_TRAILER_BYTES];
} RaceImpactRecord;
_Static_assert(sizeof(RaceImpactRecord) <= RACE_IMPACT_RECORD_MAX + SC64_BC_SAVE_EXTRA_BYTES,
               "impact record exceeds one SRAM record");
#ifdef SC64_P6_PACKED_IMPACT_SAVE
_Static_assert(SC64_P6_IMPACT_MARKER_BYTES > 0 &&
               SC64_P6_IMPACT_MARKER_BYTES < sizeof(Sc64ImpactSaveHeader) + sizeof(Sc64ImpactEvent) &&
               SC64_P6_IMPACT_MARKER_BYTES <= sizeof(((RaceImpactRecord *)0)->impacts),
               "a packed P6 record has a distinct length and fits the raw record buffer");
#endif

#ifdef SC64_SHOWCASE_CARRIER_PROBE
_Static_assert(RACE_IMPACT_SAVE_CAPACITY == RACE_IMPACT_ROWS,
               "Showcase Carrier holds every pending impact");
_Static_assert(sizeof(RaceSave) - SC64_SHOWCASE_PROJECTILE_EXTENSION == 14228,
               "Showcase Carrier must retain the predecessor layout");
_Static_assert(offsetof(RaceSave,battlecruiser_shots) - offsetof(RaceSave,interceptor_size) ==
               SC64_SHOWCASE_PROJECTILE_EXTENSION, "Showcase Carrier extension layout");
_Static_assert(sizeof(RaceImpactRecord) <= RACE_IMPACT_RECORD_MAX,
               "Showcase Carrier must also hold both Yamato shots");
#endif

#endif
