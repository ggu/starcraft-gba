#ifndef SC64_T6_MISSION_STATE_H
#define SC64_T6_MISSION_STATE_H

#ifdef SC64_T6_RUNTIME
#include "sc64_t6_runtime.h"
#include "sc64_resource_actions.h"

enum {
    SC64_T6_GLOBAL_ID = 9,
#ifdef SC64_LATE_TERRAN_STORAGE
    SC64_T6_UNIT_LIMIT = 192,
#else
    SC64_T6_UNIT_LIMIT = 160,
#endif
    SC64_T6_AI_RSCU = 1,
    SC64_T6_AI_T6A = 2,
    SC64_T6_AI_T6B = 4
};

/* Mission-world state is separate from the immutable source program and
 * the other campaigns. Burrow state is not a generic Unit.flags bit. */
typedef struct {
    Sc64T6RuntimeState triggers;
    Sc64ResourceActionState resources[8];
    u32 deaths[8];
    u8 ai_requests[8];
    u8 burrowed[(SC64_T6_UNIT_LIMIT + 7) / 8];
    u16 message_ticks;
    u16 ping_x, ping_y, ping_ticks;
    u8 message, rescue_mask;
    u8 reserved[2];
} Sc64T6MissionState;
_Static_assert(sizeof(Sc64T6MissionState) == 1364 + (SC64_T6_UNIT_LIMIT + 7) / 8,
               "T6 mission-world state budget");

static inline int sc64_t6_burrowed_get(const Sc64T6MissionState *state,
                                      unsigned slot)
{
    return state && slot < SC64_T6_UNIT_LIMIT &&
           (state->burrowed[slot >> 3] & (1u << (slot & 7))) != 0;
}

static inline void sc64_t6_burrowed_set(Sc64T6MissionState *state,
                                       unsigned slot, int value)
{
    if (!state || slot >= SC64_T6_UNIT_LIMIT) return;
    u8 bit = (u8)(1u << (slot & 7));
    if (value) state->burrowed[slot >> 3] |= bit;
    else state->burrowed[slot >> 3] &= (u8)~bit;
}
#endif
#endif
