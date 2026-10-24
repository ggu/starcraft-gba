#ifndef SC64_Z5_STATE_H
#define SC64_Z5_STATE_H

#include "sc64_z5_adapter.h"

enum {
    SC64_Z5_UNIT_BACKED_DOODADS = 41,
    SC64_Z5_DOODAD_ENABLED_BYTES = (SC64_Z5_UNIT_BACKED_DOODADS + 7) / 8
};

/* Source rows and their live pool indices are rebuilt from the authenticated
 * map on load. Only the mutable state needs to survive an SRAM round trip. */
typedef struct {
    Sc64Z5RuntimeState runtime;
    u8 doodad_enabled[SC64_Z5_DOODAD_ENABLED_BYTES];
    u8 pause_acknowledged;
} Sc64Z5PersistentState;

_Static_assert(sizeof(Sc64Z5PersistentState) <= 180,
               "Z5 persistent mission state bound");

static inline int sc64_z5_persistent_state_valid(const Sc64Z5PersistentState *s)
{
    if (!s || !sc64_z5_runtime_state_valid(&s->runtime) ||
        s->pause_acknowledged > 1 ||
        (s->pause_acknowledged && !s->runtime.source_paused) ||
        (s->doodad_enabled[SC64_Z5_DOODAD_ENABLED_BYTES - 1] & 0xfeu))
        return 0;
    return 1;
}

#endif
