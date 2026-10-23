#ifndef SC64_TERRAN_LATE_MISSION_STATE_H
#define SC64_TERRAN_LATE_MISSION_STATE_H

#if defined(SC64_TERRAN_LATE_RUNTIME) && defined(SC64_T4_RUNTIME)
#include "sc64_terran_late_runtime.h"
#include "sc64_resource_actions.h"

enum {
    SC64_TERRAN_LATE_GLOBAL_T7 = 10,
    SC64_TERRAN_LATE_GLOBAL_T8 = 11,
    SC64_TERRAN_LATE_GLOBAL_T9 = 12,
    SC64_TERRAN_LATE_GLOBAL_T10 = 13,
    SC64_TERRAN_LATE_NO_SLOT = 0xffffu,
    SC64_TERRAN_LATE_NO_LOCATION = 0xffffu
};
enum {
    SC64_TERRAN_LATE_SCRIPT_T7_TER8 = 0,
    SC64_TERRAN_LATE_SCRIPT_ENBK,
    SC64_TERRAN_LATE_SCRIPT_TR9H,
    SC64_TERRAN_LATE_SCRIPT_TR9L,
    SC64_TERRAN_LATE_SCRIPT_T11Z,
    SC64_TERRAN_LATE_SCRIPT_T11A,
    SC64_TERRAN_LATE_SCRIPT_T11B,
    SC64_TERRAN_LATE_SCRIPT_SUIR,
    SC64_TERRAN_LATE_SCRIPT_T12N,
    SC64_TERRAN_LATE_SCRIPT_T12P,
    SC64_TERRAN_LATE_SCRIPT_COUNT
};
#define SC64_TERRAN_LATE_SCRIPT_MASK ((1u << SC64_TERRAN_LATE_SCRIPT_COUNT) - 1u)

/* Mutable mission state is separate from immutable generated CHK data. Source
 * rectangles remain in source pixels. Invincibility itself lives in the
 * engine's existing save-persistent actor arrays, not in a duplicate bitmap.
 * `emitter_unit` names the retained T7 item actor; `emitter_carrier` is either
 * a live unit slot or NO_SLOT. `kills` and `deaths` are source-slot ledgers
 * for the corresponding trigger conditions, not actor-owner aliases. */
typedef struct {
    Sc64TerranLateRuntimeState triggers;
    Sc64ResourceActionState resources[8];
    u32 kills[8];          /* all-unit selector 229 */
    u32 deaths[8];         /* all-unit selector 229 */
    u32 building_kills[8]; /* building selector 231 */
    u32 building_deaths[8];
    u32 mutable_location[4]; /* left, top, right, bottom; source pixels */
    u32 flight_goals[96]; /* x:11, y:11, active:1, land:1; upper bits zero */
    u32 ai_requests[8]; /* Opaque source script requests, by trigger owner. */
    u16 mutable_location_id; /* one-based MRGN id, or NO_LOCATION */
    u16 emitter_unit; /* live T7 raw-217 actor slot, or NO_SLOT */
    u16 emitter_carrier; /* carrier slot, or NO_SLOT */
    u16 message_ticks;
    u16 ping_x, ping_y, ping_ticks;
    u8 message; /* Authored presentation class; source dialogue is omitted. */
    u8 reserved[3];
} Sc64TerranLateMissionState;

_Static_assert(sizeof(Sc64TerranLateMissionState) == 1892,
               "late Terran mission state budget");
_Static_assert(sizeof(Sc64TerranLateMissionState) <= 2000,
               "late Terran mission state must fit the bounded save union");

static int
sc64_terran_late_mission_init(Sc64TerranLateMissionState *state,
                              const Sc64T4Scenario *program,
                              const Sc64T4SourceSetup *setup)
{
    if (!state || !sc64_terran_late_runtime_init(&state->triggers, program,
                                                 setup)) return 0;
    for (unsigned i = 0; i < sizeof(*state) - sizeof(state->triggers); ++i)
        ((u8 *)&state->resources)[i] = 0;
    state->mutable_location_id = SC64_TERRAN_LATE_NO_LOCATION;
    state->emitter_unit = SC64_TERRAN_LATE_NO_SLOT;
    state->emitter_carrier = SC64_TERRAN_LATE_NO_SLOT;
    return 1;
}

static int
sc64_terran_late_mission_state_valid(const Sc64TerranLateMissionState *state,
                                     const Sc64T4Scenario *program,
                                     const Sc64T4SourceSetup *setup,
                                     unsigned map_width, unsigned map_height,
                                     unsigned max_units)
{
    if (!state || !sc64_terran_late_runtime_state_valid(&state->triggers,
                                                         program, setup) ||
        !map_width || !map_height || map_width > 128 || map_height > 128 ||
        !max_units || max_units > 255) return 0;
    if (state->mutable_location_id == SC64_TERRAN_LATE_NO_LOCATION) {
        for (unsigned i = 0; i < 4; ++i)
            if (state->mutable_location[i]) return 0;
    } else {
        unsigned id = state->mutable_location_id;
        if (!id || id > program->location_count) return 0;
        if (state->mutable_location[0] >= state->mutable_location[2] ||
            state->mutable_location[1] >= state->mutable_location[3] ||
            state->mutable_location[2] > map_width * 32u ||
            state->mutable_location[3] > map_height * 32u) return 0;
    }
    if ((state->emitter_unit != SC64_TERRAN_LATE_NO_SLOT &&
         state->emitter_unit >= max_units) ||
        (state->emitter_carrier != SC64_TERRAN_LATE_NO_SLOT &&
         state->emitter_carrier >= max_units) ||
        (state->emitter_carrier != SC64_TERRAN_LATE_NO_SLOT &&
         state->emitter_unit == SC64_TERRAN_LATE_NO_SLOT)) return 0;
    for (unsigned i = 0; i < 96; ++i) {
        u32 goal = state->flight_goals[i];
        if (goal && (!(goal & (1u << 22)) || (goal >> 24) ||
                     (goal & 2047u) >= map_width * 16u ||
                     ((goal >> 11) & 2047u) >= map_height * 16u)) return 0;
    }
    for (unsigned i = 0; i < 8; ++i)
        if (state->ai_requests[i] & ~SC64_TERRAN_LATE_SCRIPT_MASK) return 0;
    if ((state->message != 0 && state->message != 7 &&
         state->message != 8 && state->message != 9 &&
         state->message != 10 && state->message != 12) ||
        (state->message == 0) != (state->message_ticks == 0) ||
        state->message_ticks > 750 || state->ping_ticks > 90 ||
        (!state->ping_ticks && (state->ping_x || state->ping_y)) ||
        state->ping_x >= map_width * 16u || state->ping_y >= map_height * 16u)
        return 0;
    for (unsigned i = 0; i < sizeof(state->reserved); ++i)
        if (state->reserved[i]) return 0;
    return 1;
}
#endif

#endif
