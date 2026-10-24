#ifndef SC64_Z7_STATE_H
#define SC64_Z7_STATE_H

#include "sc64_z7_adapter.h"

/* Z7 has no source-unit death counters or moving locations. Only trigger
 * cursors, the switch, and the bounded authored-AI cadence need persistence. */
typedef struct {
    Sc64Z7RuntimeState runtime;
} Sc64Z7PersistentState;

_Static_assert(sizeof(Sc64Z7PersistentState)<=36,"Z7 persistent state budget");

static inline int sc64_z7_persistent_state_valid(const Sc64Z7PersistentState *s)
{ return s && sc64_z7_runtime_state_valid(&s->runtime); }

#endif
