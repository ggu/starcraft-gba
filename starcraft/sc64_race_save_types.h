#ifndef SC64_RACE_SAVE_TYPES_H
#define SC64_RACE_SAVE_TYPES_H

/* Save payload types are declared before the large-map scratch storage so a
 * combined experimental profile can use a typed union with the route queue. */
#define RACE_UNIT_WORDS (sizeof(Unit)/sizeof(int))
#define RACE_BUILD_WORDS (sizeof(Building)/sizeof(int))

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

typedef struct {
    u32 version;
    s16 units[MAX_UNITS][RACE_UNIT_WORDS];
    s16 buildings[MAX_BUILDINGS][RACE_BUILD_WORDS];
    Mineral minerals_map[MAX_MINERALS];
    u8 fog[(MAP_CELLS+7)/8];
    int state[32],players[5],ore[5],vespene[5];
    Geyser geysers[4];
    Sc64RaceCampaignState campaign;
#ifdef SC64_UNIFIED
    Sc64BackwaterState backwater;
    u8 invincible_units[MAX_UNITS],invincible_buildings[MAX_BUILDINGS];
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
#ifdef SC64_T4_SCENARIO_RUNTIME
#if defined(SC64_T3_ENABLED) && defined(SC64_UNIFIED)
    Sc64T4SaveState t4;
#else
    Sc64T4RuntimeState t4;
#endif
#endif
} RaceSave;
_Static_assert(sizeof(RaceSave)<=16368,"Race save exceeds one SRAM record");

#endif
