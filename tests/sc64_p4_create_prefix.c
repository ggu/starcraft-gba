/* Differential checks for the generated P4 opcode-11 prefix fast path. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

#if !defined(SC64_P4_RUNTIME) || !defined(SC64_P4_CREATE_PREFIX) || \
    !defined(SC64_P4_VALIDATOR_HOT)
#error "Compile with the canonical P4 create-prefix and hot-validator flags"
#endif
#if !defined(SC64_P4_CREATE_PREFIX_VERSION) || \
    SC64_P4_CREATE_PREFIX_VERSION != 1
#error "The generated P4 create-prefix version is missing or unsupported"
#endif

static unsigned slow_prefix(unsigned endpoint)
{
    unsigned count = 0;
    assert(endpoint <= sizeof(sc64_p4_source_actions) /
                        sizeof(sc64_p4_source_actions[0]));
    for (unsigned i = 0; i < endpoint; ++i)
        if (sc64_p4_source_actions[i].opcode == 11) ++count;
    return count;
}

static void check_every_generated_edge(void)
{
    const unsigned actions = sizeof(sc64_p4_source_actions) /
                             sizeof(sc64_p4_source_actions[0]);
    assert(sizeof(sc64_p4_source_create_prefix) /
           sizeof(sc64_p4_source_create_prefix[0]) == actions + 1);
    for (unsigned endpoint = 0; endpoint <= actions; ++endpoint)
        assert(sc64_p4_source_create_prefix[endpoint] == slow_prefix(endpoint));
    assert(slow_prefix(actions) == SC64_P4_SOURCE_CREATE_COUNT);
}

static void check_runtime_validator_for_each_cursor(void)
{
    for (unsigned trigger = 0; trigger < SC64_P4_TRIGGER_COUNT; ++trigger) {
        const Sc64P4SourceTrigger *tr = &sc64_p4_source_triggers[trigger];
        for (unsigned cursor = 0; cursor <= tr->action_count; ++cursor) {
            Sc64P4RuntimeState state;
            memset(&state, 0, sizeof(state));
            sc64_p4_init(&state.model);
            state.model.fired = (u16)(1u << trigger);
            if (trigger <= 2) state.model.outcome = SC64_P4_LOST;
            else if (trigger == 4) state.model.outcome = SC64_P4_WON;
            state.action_cursor[trigger] = (u8)cursor;

            unsigned dispatched = cursor ? cursor : tr->action_count;
            state.created_count = (u16)(slow_prefix(tr->first_action + dispatched) -
                                        slow_prefix(tr->first_action));
            if (cursor) {
                unsigned duration = race_p4_action_duration_limit(trigger, cursor - 1);
                if (!duration) continue; /* No valid in-flight state for this endpoint. */
                state.action_ticks[trigger] = 1;
            }
            assert(race_p4_runtime_state_valid(&state));

            Sc64P4RuntimeState malformed = state;
            if (malformed.created_count < SC64_P4_SOURCE_CREATE_COUNT)
                ++malformed.created_count;
            else
                --malformed.created_count;
            assert(!race_p4_runtime_state_valid(&malformed));
        }
    }
}

int main(void)
{
    check_every_generated_edge();
    race_mode = 0;
    race_selected_mission = SC64_P4_GLOBAL_ID;
    assert(race_p4_init());
    check_runtime_validator_for_each_cursor();
    puts("P4 generated create-prefix edges and hot validator passed");
    return 0;
}
