#ifndef SC64_P6_STATE_H
#define SC64_P6_STATE_H

/* Persisted state owned by the P6 campaign adapter. Transient bindings from
 * THG2 rows to the building pool are rebuilt from authenticated coordinates
 * after load. The doodad bitset is indexed by source doodad row; bits for
 * sprite-only rows and the three unused high bits are required to remain 0. */
#include "sc64_p6_adapter.h"
#include "sc64_p6_doodads_generated.h"
#ifdef SC64_P6_STORM_ENABLED
#include "sc64_psionic_storm.h"
#endif

enum {
    SC64_P6_PERSISTED_DOODAD_BYTES = 5,
    SC64_P6_SOURCE_SLOT_COUNT = 12,
    SC64_P6_MAX_SOURCE_DEATHS = 65535
};

typedef struct {
    Sc64P6RuntimeState runtime;
    u32 deaths_by_source[SC64_P6_SOURCE_SLOT_COUNT];
    u8 doodad_enabled[SC64_P6_PERSISTED_DOODAD_BYTES];
    u8 pause_acknowledged;
    u8 reserved[2];
#ifdef SC64_P6_STORM_ENABLED
    Sc64PsionicStormState storm;
#endif
} Sc64P6PersistentState;

_Static_assert(sizeof(Sc64P6PersistentState) <=
#ifdef SC64_P6_STORM_ENABLED
               180,
#else
               148,
#endif
               "P6 persistent state bound");

static inline int sc64_p6_persistent_state_valid(const Sc64P6PersistentState *s)
{
    if (!s || !sc64_p6_runtime_state_valid(&s->runtime) ||
        s->pause_acknowledged > 1 || s->reserved[0] || s->reserved[1]) return 0;
    for (unsigned i = 0; i < SC64_P6_SOURCE_SLOT_COUNT; ++i)
        if (s->deaths_by_source[i] > SC64_P6_MAX_SOURCE_DEATHS) return 0;
#ifdef SC64_P6_STORM_ENABLED
    if (!sc64_psionic_storm_state_valid(&s->storm)) return 0;
    for (unsigned i = 0; i < SC64_STORM_MAX_ACTIVE; ++i)
        if (s->storm.storm[i].active &&
            (s->storm.storm[i].x >= SC64_P6_MAP_COLS * 16u ||
             s->storm.storm[i].y >= SC64_P6_MAP_ROWS * 16u)) return 0;
#endif
    /* Current staged doodads use rows 0..36; upper bits 37..39 are unused. */
    if (s->doodad_enabled[4] & 0xe0u) return 0;
    for (unsigned i = 0; i < SC64_P6_DOODAD_SOURCE_COUNT; ++i)
        if (sc64_p6_doodads[i].sprite_only &&
            (s->doodad_enabled[i >> 3] & (u8)(1u << (i & 7u)))) return 0;
    return 1;
}

static inline unsigned sc64_p6_remaining_creations(const Sc64P6RuntimeState *s)
{
    unsigned remaining = 0;
    if (!s || !sc64_p6_runtime_state_valid(s)) return SC64_P6_CREATE_ACTIONS;
    for (unsigned t = 0; t < SC64_P6_TRIGGER_COUNT; ++t) {
        const Sc64P6SourceTrigger *tr = &sc64_p6_source_triggers[t];
        int started = sc64_p6_trigger_started(&s->model, t);
        u8 cursor = s->model.action_cursor[t];
        if (started && !cursor) continue; /* one-shot script is complete */
        for (unsigned i = 0; i < tr->action_count; ++i) {
            const Sc64P6SourceAction *a = &sc64_p6_source_actions[tr->first_action + i];
            if (a->opcode == SC64_P6_ACTION_CREATE_UNIT && (!started || i >= cursor))
                ++remaining; /* P6 create actions all use modifier zero => one */
        }
    }
    return remaining;
}

#endif
