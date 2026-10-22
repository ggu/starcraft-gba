#ifndef SC64_T4_RUNTIME_STATE_H
#define SC64_T4_RUNTIME_STATE_H

#include "sc64_t4_triggers.h"

#ifdef SC64_T4_SCENARIO_RUNTIME
/* All Players trigger enables expand to independent player instances. Keep
 * their cursors and wait clocks separate, with only the switch bank shared.
 * The source controller table determines which slots are active. */
typedef struct {
    Sc64T4TriggerState player[8];
    Sc64T4TriggerSwitches switches;
    u16 location30[4]; /* Source pixels: left, top, right, bottom. */
    u16 message_ticks;
    u16 ping_x, ping_y, ping_ticks; /* GBA world pixels. */
    u8 location30_moved, message, active_mask;
    u8 doodad_slot[32]; /* Building slot, or 255 for a sprite-only row. */
    u8 doodad_enabled[32];
} Sc64T4RuntimeState;
_Static_assert(sizeof(Sc64T4RuntimeState) == 1268, "T4 world-effect state budget");
#endif

#endif
