#ifndef SC64_T6_SCENARIO_ADAPTER_H
#define SC64_T6_SCENARIO_ADAPTER_H

#if defined(SC64_T6_RUNTIME) && defined(SC64_T4_RUNTIME) && defined(SC64_T6_ENABLED)
#include "sc64_t6_runtime.h"
#include "sc64_t6_scenario_generated.h"

/* Game-independent binding to the private, verified T6 numeric records.
 * Callbacks remain owned by the world adapter; this header has no game-global
 * references and does not enable the campaign catalog. */
static int
sc64_t6_scenario_source_valid(void)
{
    return sc64_t6_runtime_program_valid(&sc64_t6_scenario,
                                          &sc64_t6_source_setup) &&
           sc64_t6_runtime_properties_valid(sc64_t6_properties, 64,
                                            &sc64_t6_source_setup);
}

static int
sc64_t6_scenario_runtime_init(Sc64T6RuntimeState *state)
{
    if (!sc64_t6_scenario_source_valid()) return 0;
    return sc64_t6_runtime_init(state, &sc64_t6_scenario,
                                &sc64_t6_source_setup);
}

static int
sc64_t6_scenario_state_valid(const Sc64T6RuntimeState *state)
{
    return sc64_t6_scenario_source_valid() &&
           sc64_t6_runtime_state_valid(state, &sc64_t6_scenario,
                                       &sc64_t6_source_setup);
}

static int
sc64_t6_scenario_runtime_step(Sc64T6RuntimeState *state,
                              const Sc64T6RuntimeOps *ops,
                              void *context, int paused)
{
    if (!sc64_t6_scenario_state_valid(state)) return 0;
    return sc64_t6_runtime_step(state, &sc64_t6_scenario,
                                &sc64_t6_source_setup, ops, context, paused);
}

/* Runtime hot path: call only after the generated immutable scenario and the
 * complete state have passed source_valid/state_valid at init or load. */
static int
sc64_t6_scenario_runtime_step_prevalidated(Sc64T6RuntimeState *state,
                                          const Sc64T6RuntimeOps *ops,
                                          void *context, int paused)
{
    return sc64_t6_runtime_step_prevalidated(state, &sc64_t6_scenario,
                    &sc64_t6_source_setup, ops, context, paused);
}

#endif
#endif
