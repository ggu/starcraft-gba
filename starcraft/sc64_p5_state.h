#ifndef SC64_P5_STATE_H
#define SC64_P5_STATE_H

/* Persisted continuation for the authored P5 adapter. The CHK Deaths
 * predicate is event based, so these source-player counters cannot be
 * reconstructed from the surviving world. */
#include "sc64_p5_adapter.h"

enum { SC64_P5_SOURCE_SLOT_COUNT = 12 };

typedef struct {
    Sc64P5RuntimeState runtime;
    u32 deaths_by_source[SC64_P5_SOURCE_SLOT_COUNT];
    u8 pressure_dispatched_mask; /* Authored bounded replacements for opaque AI. */
    u8 reserved[3];
} Sc64P5PersistentState;

_Static_assert(sizeof(Sc64P5PersistentState) <= 128,
               "P5 persistent runtime state budget");

static inline int sc64_p5_persistent_state_valid(const Sc64P5PersistentState *s)
{
    static const u8 ai_trigger[3] = {8, 9, 10};
    static const u8 ai_action[3] = {0, 1, 1};
    u8 expected = 0;
    if (!s || !sc64_p5_runtime_state_valid(&s->runtime) ||
        (s->pressure_dispatched_mask & (u8)~7u) ||
        s->reserved[0] || s->reserved[1] || s->reserved[2]) return 0;
    for (unsigned i = 0; i < 3; ++i) {
        unsigned trigger = ai_trigger[i], action = ai_action[i];
        const Sc64P5SourceTrigger *t = &sc64_p5_source_triggers[trigger];
        u8 cursor = s->runtime.model.action_cursor[trigger];
        int started = sc64_p5_trigger_mask_test(s->runtime.model.started,
                                                (u8)trigger);
        if (started && (!cursor || cursor > action))
            expected |= (u8)(1u << i);
        if (cursor > t->action_count) return 0;
    }
    return s->pressure_dispatched_mask == expected;
}

#endif
