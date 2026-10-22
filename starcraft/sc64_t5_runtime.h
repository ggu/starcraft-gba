#ifndef SC64_T5_RUNTIME_STATE_H
#define SC64_T5_RUNTIME_STATE_H

#include "sc64_t4_triggers.h"
#include "sc64_resource_actions.h"
#include "sc64_t5_ai.h"

#if defined(SC64_T5_RUNTIME) && !defined(SC64_T4_RUNTIME)
#error "T5 trigger runtime currently shares the validated T4 trigger core"
#endif

#ifdef SC64_T5_RUNTIME
/* T5 uses the generic bounded trigger cursor format, but owns its own runtime
 * state. In particular it has no T4 mutable location 30 or THG2 door table. */
typedef struct {
    Sc64T4TriggerState player[8];
    Sc64T4TriggerSwitches switches;
    Sc64ResourceActionState resources[8];
    Sc64T5AiState ai;
    u16 message_ticks;
    u16 ping_x, ping_y, ping_ticks;
    u8 message, active_mask;
} Sc64T5RuntimeState;

_Static_assert(sizeof(Sc64T5RuntimeState) == 1340,
               "T5 trigger state budget / save schema");

enum {
    SC64_T5_GLOBAL_ID = 8,
    SC64_T5_SOURCE_HUMAN = 0,
    SC64_T5_SOURCE_RESCUE = 2,
    SC64_T5_SOURCE_SPECIAL_NEUTRAL = 3,
    SC64_T5_SOURCE_ENEMY = 6,
    SC64_T5_ACTIVE_SOURCE_MASK = 0x4d
};

#endif /* SC64_T5_RUNTIME */
#endif
