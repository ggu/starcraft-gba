#ifndef SC64_TRIGGER_TYPES_H
#define SC64_TRIGGER_TYPES_H

#include "sc64_types.h"

#ifdef SC64_T4_RUNTIME
typedef struct {
    u32 location;
    u32 player;
    u32 amount;
    u16 unit;
    u8 comparison;
    u8 opcode;
    u8 resource;
    u8 flags;
    u16 mask;
} Sc64T4Condition;

typedef struct {
    u32 location;
    u32 string_id;
    u32 wav_id;
    u32 time;
    u32 player;
    u32 argument;
    u16 unit;
    u8 opcode;
    u8 modifier;
    u8 flags;
    u8 trailing[3];
} Sc64T4Action;

typedef struct {
    u32 left;
    u32 top;
    u32 right;
    u32 bottom;
    u16 name_id;
    u16 elevation;
    u8 raw_record[20];
} Sc64T4Location;

typedef struct {
    u16 type;
    u16 x;
    u16 y;
    u8 owner;
    u8 unused;
    u16 flags;
    u8 raw_record[10];
} Sc64T4Doodad;

/* UPRP records are independent of UNIT-placement flags. Retain both the
 * decoded operands and the exact source bytes for round-trip validation. */
typedef struct {
    u16 valid_special, valid_fields;
    u8 owner, hp_percent, shield_percent, energy_percent;
    u32 resources;
    u16 hangar_count, special;
    u32 unused;
    u8 raw_record[20];
} Sc64T4UnitProperties;

typedef struct {
    u8 controllers[12], races[12];
    u8 force_by_slot[8]; /* Zero-based FORC membership. */
    u8 force_raw[20];
    u8 property_used[64];
} Sc64T4SourceSetup;

typedef struct {
    const Sc64T4Condition *conditions; /* Fixed 16-slot source row. */
    const Sc64T4Action *actions;       /* Fixed 64-slot source row. */
    u8 enabled_players[27];
    u8 initial_cursor;
    u32 execution_flags;
} Sc64T4Trigger;

typedef struct {
    const Sc64T4Location *locations;
    u16 location_count;
    const Sc64T4Trigger *triggers;
    u16 trigger_count;
    const Sc64T4Doodad *doodads;
    u16 doodad_count;
} Sc64T4Scenario;

_Static_assert(sizeof(Sc64T4Condition) == 20, "CHK condition schema size");
_Static_assert(sizeof(Sc64T4Action) == 32, "CHK action schema size");
_Static_assert(sizeof(Sc64T4Location) == 40, "T4 location schema size");
_Static_assert(sizeof(Sc64T4Doodad) == 20, "T4 doodad schema size");
_Static_assert(sizeof(Sc64T4UnitProperties) == 40, "T4 UPRP schema size");
_Static_assert(sizeof(Sc64T4SourceSetup) == 116, "T4 source setup size");
#endif /* SC64_T4_RUNTIME */

#endif
