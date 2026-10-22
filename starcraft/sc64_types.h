#ifndef SC64_TYPES_H
#define SC64_TYPES_H

#include "../common/gba.h"

/* Shared authored schema for optional StarCraft CHK-derived mission data.
 * x_px/y_px are original 32 px tile-map pixel coordinates floor-divided by
 * two, yielding the engine's 16 px world coordinates. */
typedef struct {
    u16 x_px;
    u16 y_px;
    u16 source_type; /* Original Brood War unit type id. */
    u8 owner;        /* Original CHK player slot. */
    u8 hp_percent;
    u16 flags;       /* Raw CHK UNIT flags. */
    u16 resources;   /* Original UNIT resource field (not runtime minerals). */
} Sc64ImportedUnit;

/* Source MTXM tile ids are retained for provenance. A separate verified
 * collision mask may be supplied; unknown tiles must not be guessed. */
enum {
    SC64_OWNER_PLAYER_1 = 0,
    SC64_OWNER_PLAYER_2 = 1,
    SC64_OWNER_PLAYER_3 = 2,
    SC64_OWNER_PLAYER_4 = 3,
    SC64_OWNER_NEUTRAL = 11,

    SC64_RAW_MARINE = 0,
    SC64_RAW_ZERGLING = 37,
    SC64_RAW_RAYNOR_VULTURE = 19,
    SC64_RAW_COMMAND_CENTER = 106,
    SC64_RAW_SUPPLY_DEPOT = 109,
    SC64_RAW_BARRACKS = 111,
    SC64_RAW_MINERAL_FIELD_1 = 176,
    SC64_RAW_MINERAL_FIELD_2 = 177,
    SC64_RAW_MINERAL_FIELD_3 = 178,
    SC64_RAW_SCV = 7,
    SC64_RAW_START_LOCATION = 214
};

#endif
