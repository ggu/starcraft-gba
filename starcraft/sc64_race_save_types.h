#ifndef SC64_RACE_SAVE_TYPES_H
#define SC64_RACE_SAVE_TYPES_H

/* Save payload types are declared before the large-map scratch storage so a
 * combined experimental profile can use a typed union with the route queue. */
#define RACE_UNIT_WORDS (sizeof(Unit)/sizeof(int))
#define RACE_BUILD_WORDS (sizeof(Building)/sizeof(int))

#ifdef SC64_P7_TYPED_COMPACT_ENABLED
#if !defined(SC64_P7_BATTERY_ENABLED) || !defined(SC64_P7_ARBITER_CLOAK_CACHE_ENABLED) || !defined(SC64_P7_RUNTIME) || !defined(SC64_UNIFIED) || !defined(SC64_ZP_FULL_STORAGE) || !defined(SC64_ZP_CAMPAIGN_STORAGE) || !defined(SC64_CAMPAIGN_RACE) || SC64_CAMPAIGN_RACE != 2
#error "P7 typed compact save requires unified P7 battery and Arbiter cache profile"
#endif
#endif

#if defined(SC64_T6_STORAGE) || defined(SC64_ZP_CAMPAIGN_STORAGE)
#ifndef SC64_Z7_DERIVED_SAVE
#include "sc64_compact_building_save.h"
#endif
#endif
#ifdef SC64_Z7_DERIVED_SAVE
#include "sc64_derived_save.h"
#endif
#if defined(SC64_LATE_TERRAN_STORAGE) || defined(SC64_ZP_CAMPAIGN_STORAGE)
#ifdef SC64_ZP_FULL_STORAGE
#ifndef SC64_Z7_DERIVED_SAVE
#include "sc64_zp_unit_save.h"
#endif
#else
#include "sc64_compact_unit_save.h"
#endif
_Static_assert(SC_RU_COUNT <= 64, "late Terran unit codec uses six-bit types");
#endif
#ifdef SC64_ZP_FULL_STORAGE
#include "sc64_compact_mineral_save.h"
#endif
#ifdef SC64_P7_TYPED_COMPACT_ENABLED
_Static_assert(SC_RU_COUNT <= 64, "P7 typed unit type field uses six bits");
_Static_assert(SC_RB_COUNT <= 64, "P7 typed building type field uses six bits");
#include "sc64_p7_typed_unit_save.h"
#include "sc64_p7_typed_building_save.h"
#endif
#ifdef SC64_T6_RUNTIME
#include "sc64_t6_mission_state.h"
#endif
#ifdef SC64_TERRAN_LATE_RUNTIME
#include "sc64_terran_late_mission_state.h"
#endif
#ifdef SC64_Z5_RUNTIME
#include "sc64_z5_state.h"
#endif
#ifdef SC64_P5_RUNTIME
#include "sc64_p5_state.h"
#endif
#ifdef SC64_P6_RUNTIME
#include "sc64_p6_state.h"
#endif
#ifdef SC64_P7_BATTERY_ENABLED
#include "sc64_p7_state.h"
#endif
#ifdef SC64_Z8_RUNTIME
#include "sc64_z8_state.h"
#endif

#if defined(SC64_T3_ENABLED) && defined(SC64_T4_SCENARIO_RUNTIME)
/* Source player 5 is inactive in the T4 scenario. The save validator requires
 * it to equal its canonical initial state, so store the seven active players
 * and reconstruct player 5 on load. */
typedef struct {
    Sc64T4TriggerState player[7]; /* players 0..4, 6, 7 */
    Sc64T4TriggerSwitches switches;
    u16 location30[4];
    u16 message_ticks;
    u16 ping_x, ping_y, ping_ticks;
    u8 location30_moved, message, active_mask;
    u8 doodad_slot[32];
    u8 doodad_enabled[32];
} Sc64T4SaveState;
_Static_assert(sizeof(Sc64T4SaveState) == 1124, "compact combined T4 save state");
#endif

#ifdef SC64_T5_RUNTIME
/* Only source slots 0, 2, 3 and 6 execute T5 triggers. Inactive contexts are
 * canonical-checked when packing and reconstructed when loading. The same
 * payload region stores T4 or T5; unused union bytes must remain zero. */
typedef struct {
    Sc64T4TriggerState player[4]; /* source slots 0, 2, 3, 6 */
    Sc64T4TriggerSwitches switches;
    Sc64ResourceActionState resources[8];
    Sc64T5AiState ai;
    u16 message_ticks;
    u16 ping_x, ping_y, ping_ticks;
    u8 message, active_mask;
} Sc64T5SaveState;
_Static_assert(sizeof(Sc64T5SaveState) == 764, "compact T5 save state budget");
#endif

typedef struct {
    u32 version;
#if defined(SC64_LATE_TERRAN_STORAGE) || defined(SC64_ZP_CAMPAIGN_STORAGE)
#ifdef SC64_ZP_FULL_STORAGE
#ifdef SC64_P7_TYPED_COMPACT_ENABLED
    Sc64P7TypedUnitSave units[MAX_UNITS];
#else
#ifdef SC64_Z7_DERIVED_SAVE
    Sc64DerivedUnitSave units[MAX_UNITS];
#else
    Sc64ZpPackedUnitSave units[MAX_UNITS];
#endif
#endif
#else
    Sc64PackedUnitSave units[MAX_UNITS];
#endif
#else
    s16 units[MAX_UNITS][RACE_UNIT_WORDS];
#endif
#if defined(SC64_T6_STORAGE) || defined(SC64_ZP_CAMPAIGN_STORAGE)
#ifdef SC64_P7_TYPED_COMPACT_ENABLED
    Sc64P7TypedBuildingSave buildings[MAX_BUILDINGS];
#else
#ifdef SC64_Z7_DERIVED_SAVE
    Sc64DerivedBuildingSave buildings[MAX_BUILDINGS];
#else
    Sc64PackedBuildingSave buildings[MAX_BUILDINGS];
#endif
#endif
#else
    s16 buildings[MAX_BUILDINGS][RACE_BUILD_WORDS];
#endif
#ifdef SC64_ZP_FULL_STORAGE
    Sc64PackedMineralSave minerals_map[MAX_MINERALS];
#else
    Mineral minerals_map[MAX_MINERALS];
#endif
    u8 fog[(MAP_CELLS+7)/8];
    int state[32],players[5],ore[5],vespene[5];
    Geyser geysers[MAX_RACE_GEYSERS];
    Sc64RaceCampaignState campaign;
#ifdef SC64_UNIFIED
    Sc64BackwaterState backwater;
#ifdef SC64_ZP_FULL_STORAGE
    u8 invincible_bits[(MAX_UNITS + MAX_BUILDINGS + 7) / 8];
#else
    u8 invincible_units[MAX_UNITS],invincible_buildings[MAX_BUILDINGS];
#endif
#endif
#if defined(SC64_UNIFIED) && defined(SC64_T3_ENABLED)
    Sc64T3State t3;
    u16 t3_buildings_killed;
#endif
#if defined(SC64_UNIFIED) && defined(SC64_Z3_ENABLED)
    Sc64Z3State z3;
    u16 z3_action_ticks;
#endif
#if defined(SC64_P3_RUNTIME) && defined(SC64_P3_ENABLED) && defined(SC64_UNIFIED)
    Sc64P3State p3;
    u16 p3_action_ticks;
#endif
#ifdef SC64_Z4_RUNTIME
    Sc64Z4RuntimeState z4;
#endif
#ifdef SC64_P4_RUNTIME
    Sc64P4RuntimeState p4;
#endif
#if defined(SC64_Z5_RUNTIME) && defined(SC64_Z6_RUNTIME)
    /* Mutually exclusive campaign missions share the larger Z5 payload. */
    union {
        Sc64Z5PersistentState z5;
        Sc64Z6PersistentState z6;
#ifdef SC64_Z7_RUNTIME
        Sc64Z7PersistentState z7;
#endif
#ifdef SC64_Z8_RUNTIME
        Sc64Z8PersistentState z8;
#endif
    };
#elif defined(SC64_Z5_RUNTIME)
    Sc64Z5PersistentState z5;
#elif defined(SC64_Z6_RUNTIME)
    Sc64Z6PersistentState z6;
#endif
#if defined(SC64_P5_RUNTIME) && defined(SC64_P6_RUNTIME) && defined(SC64_P7_BATTERY_ENABLED)
    union {
        Sc64P5PersistentState p5;
        Sc64P6PersistentState p6;
        Sc64P7PersistentState p7;
    };
#elif defined(SC64_P5_RUNTIME) && defined(SC64_P6_RUNTIME)
    union {
        Sc64P5PersistentState p5;
        Sc64P6PersistentState p6;
    };
#elif defined(SC64_P5_RUNTIME) && defined(SC64_P7_BATTERY_ENABLED)
    union {
        Sc64P5PersistentState p5;
        Sc64P7PersistentState p7;
    };
#elif defined(SC64_P6_RUNTIME) && defined(SC64_P7_BATTERY_ENABLED)
    union {
        Sc64P6PersistentState p6;
        Sc64P7PersistentState p7;
    };
#elif defined(SC64_P5_RUNTIME)
    Sc64P5PersistentState p5;
#elif defined(SC64_P6_RUNTIME)
    Sc64P6PersistentState p6;
#elif defined(SC64_P7_BATTERY_ENABLED)
    Sc64P7PersistentState p7;
#endif
#ifdef SC64_T4_SCENARIO_RUNTIME
#if defined(SC64_T3_ENABLED) && defined(SC64_UNIFIED)
#ifdef SC64_T5_RUNTIME
#ifdef SC64_T6_RUNTIME
#ifdef SC64_TERRAN_LATE_RUNTIME
    union {
        Sc64T4SaveState t4;
        Sc64T5SaveState t5;
        Sc64T6MissionState t6;
        Sc64TerranLateMissionState terran_late;
    };
#else
    union {
        Sc64T4SaveState t4;
        Sc64T5SaveState t5;
        Sc64T6MissionState t6;
    };
#endif
#else
    union {
        Sc64T4SaveState t4;
        Sc64T5SaveState t5;
    };
#endif
#else
    Sc64T4SaveState t4;
#endif
#else
    Sc64T4RuntimeState t4;
#endif
#endif
#ifdef SC64_P7_PROJECTILES_ENABLED
    u8 interceptor_projectiles[SC64_CODEC_BYTES];
    u8 interceptor_padding;
#endif
} RaceSave;
#if defined(SC64_Z5_RUNTIME) && defined(SC64_Z6_RUNTIME)
_Static_assert(sizeof(Sc64Z6PersistentState) <= sizeof(Sc64Z5PersistentState),
               "Z6 must fit in the existing Z5 save payload");
#endif
#ifdef SC64_Z8_RUNTIME
_Static_assert(sizeof(Sc64Z8PersistentState) <= sizeof(Sc64Z5PersistentState),
               "Z8 must fit in the existing Z5 save payload");
#endif
_Static_assert(sizeof(RaceSave)<=16368,"Race save exceeds one SRAM record");
#ifdef SC64_T6_STORAGE
#ifdef SC64_LATE_TERRAN_STORAGE
#ifdef SC64_TERRAN_LATE_RUNTIME
_Static_assert(sizeof(RaceSave) <= 16368, "generation-18 late mission payload budget");
#else
_Static_assert(sizeof(RaceSave) == 15312, "generation-17 late Terran save payload");
#endif
#elif defined(SC64_T6_RUNTIME)
_Static_assert(sizeof(RaceSave) == 15644, "generation-16 T6 save payload");
#else
_Static_assert(sizeof(RaceSave) == 15384, "generation-15 expanded storage payload");
#endif
#endif
#if defined(SC64_T5_RUNTIME) && !defined(SC64_T6_STORAGE)
_Static_assert(sizeof(RaceSave) == 16328, "T5 shares the existing SRAM payload");
#endif

#endif
