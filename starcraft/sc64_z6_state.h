#ifndef SC64_Z6_STATE_H
#define SC64_Z6_STATE_H

/* Persistent part of the Z6 engine adapter. Source slots and live pool
 * bindings are rebuilt from the authenticated map after load; event deaths
 * and the two mutable MRGN locations cannot be reconstructed from the map. */
#include "sc64_z6_adapter.h"

enum { SC64_Z6_SOURCE_SLOT_COUNT = 12, SC64_Z6_MAX_SOURCE_DEATHS = 65535u };

typedef struct {
    Sc64Z6RuntimeState runtime;
    u32 deaths_by_source[SC64_Z6_SOURCE_SLOT_COUNT];
    u16 moved_location[2][4]; /* CHK source pixels, half-scale of engine px. */
    u8 moved_locations_mask;
    /* Explicit small replacement for opaque Zer6: one unit plan per 240
       active mission ticks, cycling only after a paid queue succeeds. */
    u8 ai_plan_ticks;
    u8 ai_plan_phase;
} Sc64Z6PersistentState;

_Static_assert(sizeof(Sc64Z6PersistentState) <= 136,
               "Z6 persisted runtime must remain within mission budget");

static inline int sc64_z6_persistent_state_valid(const Sc64Z6PersistentState *s)
{
    if (!s || !sc64_z6_runtime_state_valid(&s->runtime) ||
        s->ai_plan_ticks > 240u || s->ai_plan_phase > 3u ||
        (s->moved_locations_mask & (u8)~3u)) return 0;
    for (unsigned i = 0; i < SC64_Z6_SOURCE_SLOT_COUNT; ++i)
        if (s->deaths_by_source[i] > SC64_Z6_MAX_SOURCE_DEATHS) return 0;
    for (unsigned i = 0; i < 2; ++i) {
        const u16 *p = s->moved_location[i];
        if (!(s->moved_locations_mask & (1u << i))) {
            if (p[0] || p[1] || p[2] || p[3]) return 0;
            continue;
        }
        if (p[0] >= p[2] || p[1] >= p[3] ||
            p[2] > SC64_Z6_MAP_COLS * 32u ||
            p[3] > SC64_Z6_MAP_ROWS * 32u) return 0;
    }
    return 1;
}

#endif
