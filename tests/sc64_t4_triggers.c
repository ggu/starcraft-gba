#include <assert.h>
#include <stdio.h>
#include <string.h>

#define SC64_T4_RUNTIME 1
#include "../starcraft/sc64_t4_triggers.h"

typedef struct {
    Sc64T4Condition conditions[8][16];
    Sc64T4Action actions[8][64];
    Sc64T4Trigger triggers[8];
    Sc64T4Scenario program;
    Sc64T4TriggerSwitches switches;
    unsigned count_calls;
    unsigned action_calls;
    unsigned log[256];
    unsigned log_count;
    int fail_once;
    int count_result;
    int action_result;
    u32 query_count;
} Fixture;

static int
fixture_count(void *context, const Sc64T4Condition *condition,
              unsigned owner, u32 *count)
{
    Fixture *f = context;
    (void)condition;
    assert(owner == 0);
    ++f->count_calls;
    if (f->count_result != 1) return f->count_result;
    *count = f->query_count;
    return 1;
}

static int
fixture_action(void *context, const Sc64T4Action *action, unsigned owner)
{
    Fixture *f = context;
    assert(owner == 0);
    ++f->action_calls;
    if (f->log_count < sizeof(f->log) / sizeof(f->log[0]))
        f->log[f->log_count++] = action->argument;
    if (f->fail_once && action->argument == 2) {
        f->fail_once = 0;
        return 0;
    }
    if (f->action_result != 1) return f->action_result;
    return action->opcode == 99 ? 1 : -1;
}

static const Sc64T4TriggerOps ops = {fixture_count, fixture_action};

static void
fixture_init(Fixture *f, unsigned trigger_count)
{
    memset(f, 0, sizeof(*f));
    f->program.triggers = f->triggers;
    f->program.trigger_count = (u16)trigger_count;
    f->program.locations = NULL;
    f->program.location_count = 0;
    f->program.doodads = NULL;
    f->program.doodad_count = 0;
    f->count_result = 1;
    f->action_result = 1;
    for (unsigned i = 0; i < trigger_count; ++i) {
        f->triggers[i].conditions = f->conditions[i];
        f->triggers[i].actions = f->actions[i];
        f->triggers[i].enabled_players[0] = 1;
    }
}

static void
effect(Sc64T4Action *action, unsigned argument)
{
    memset(action, 0, sizeof(*action));
    action->opcode = 99; /* Authored fixture effect handled by callback. */
    action->argument = argument;
}

static void
wait_ms(Sc64T4Action *action, u32 milliseconds)
{
    memset(action, 0, sizeof(*action));
    action->opcode = 4;
    action->time = milliseconds;
}

static void
condition_switch(Sc64T4Condition *condition, unsigned index,
                 unsigned comparison)
{
    memset(condition, 0, sizeof(*condition));
    condition->opcode = 11;
    condition->resource = (u8)index;
    condition->comparison = (u8)comparison;
}

static void
test_pause_cooldown_and_30hz_wait(void)
{
    Fixture f;
    Sc64T4TriggerState state;
    fixture_init(&f, 1);
    wait_ms(&f.actions[0][0], 1000);
    effect(&f.actions[0][1], 10);
    assert(sc64_t4_trigger_init(&state, &f.program, 0, 1));
    assert(sc64_t4_trigger_step(&state, &f.program, &f.switches, &ops, &f, 0));
    assert(state.elapsed_ticks == 1 && state.wait_ticks == 30);
    assert(state.cursor[0] == 0 && (state.flags[0] & SC64_T4_TR_WAIT));
    assert(f.action_calls == 0);

    u32 elapsed = state.elapsed_ticks;
    u16 wait = state.wait_ticks;
    u8 cooldown = state.cooldown;
    for (unsigned i = 0; i < 7; ++i)
        assert(sc64_t4_trigger_step(&state, &f.program, &f.switches, &ops, &f, 1));
    assert(state.elapsed_ticks == elapsed && state.wait_ticks == wait &&
           state.cooldown == cooldown && f.action_calls == 0);

    for (unsigned i = 0; i < 29; ++i)
        assert(sc64_t4_trigger_step(&state, &f.program, &f.switches, &ops, &f, 0));
    assert(state.wait_ticks == 1 && f.action_calls == 0);
    assert(sc64_t4_trigger_step(&state, &f.program, &f.switches, &ops, &f, 0));
    assert(state.wait_ticks == 0 && f.action_calls == 1);
    assert(state.flags[0] == SC64_T4_TR_DONE && state.cursor[0] == 0);
}

static void
test_source_order_shared_wait_and_no_condition_recheck(void)
{
    Fixture f;
    Sc64T4TriggerState state;
    fixture_init(&f, 3);
    f.conditions[0][0].opcode = 2;
    f.conditions[0][0].comparison = 0;
    f.conditions[0][0].amount = 0;
    wait_ms(&f.actions[0][0], 1000);
    effect(&f.actions[0][1], 11);
    wait_ms(&f.actions[1][0], 1000);
    effect(&f.actions[1][1], 22);
    effect(&f.actions[2][0], 33);
    assert(sc64_t4_trigger_init(&state, &f.program, 0, 1));

    assert(sc64_t4_trigger_step(&state, &f.program, &f.switches, &ops, &f, 0));
    assert(f.count_calls == 1);
    assert(f.log_count == 1 && f.log[0] == 33); /* Other triggers still run. */
    assert(state.cursor[1] == 0 && (state.flags[1] & SC64_T4_TR_RUNNING));

    /* Pause does not consume the shared wait or its source-ordered marker. */
    u16 remaining = state.wait_ticks;
    for (unsigned i = 0; i < 3; ++i)
        assert(sc64_t4_trigger_step(&state, &f.program, &f.switches, &ops, &f, 1));
    assert(state.wait_ticks == remaining && f.count_calls == 1);

    for (unsigned i = 0; i < 30; ++i)
        assert(sc64_t4_trigger_step(&state, &f.program, &f.switches, &ops, &f, 0));
    assert(f.count_calls == 1); /* Running trigger resumes without conditions. */
    assert(f.log_count == 2 && f.log[1] == 11);
    assert(state.wait_ticks == 30); /* Trigger 1 begins its own wait afterward. */
    assert(state.active_wait == 1 && (state.flags[1] & SC64_T4_TR_WAIT));
}

static void
test_switch_boundaries_conditions_and_action_slots(void)
{
    Fixture f;
    Sc64T4TriggerState state;
    fixture_init(&f, 1);
    assert(sc64_t4_trigger_init(&state, &f.program, 0, 1));

    f.actions[0][0].opcode = 13; f.actions[0][0].argument = 0;
    f.actions[0][0].modifier = 4;
    f.actions[0][1].opcode = 13; f.actions[0][1].argument = 255;
    f.actions[0][1].modifier = 4;
    f.actions[0][2].opcode = 13; f.actions[0][2].argument = 0;
    f.actions[0][2].modifier = 6;
    f.actions[0][3].opcode = 13; f.actions[0][3].argument = 255;
    f.actions[0][3].modifier = 6;
    /* Empty opcode terminates the action list; slot 63 is not visited. */
    f.actions[0][63].opcode = 13; f.actions[0][63].argument = 100;
    f.actions[0][63].modifier = 4;
    assert(sc64_t4_trigger_step(&state, &f.program, &f.switches, &ops, &f, 0));
    assert(f.switches.bits[0] == 0 && f.switches.bits[31] == 0);
    assert(state.flags[0] == SC64_T4_TR_DONE);

    /* Explicitly exercise slot 63 and switch 255's clear/toggle operations. */
    fixture_init(&f, 1);
    for (unsigned i = 0; i < 63; ++i) {
        f.actions[0][i].opcode = 13;
        f.actions[0][i].argument = 0;
        f.actions[0][i].modifier = 4;
    }
    f.actions[0][63].opcode = 13;
    f.actions[0][63].argument = 255;
    f.actions[0][63].modifier = 5;
    assert(sc64_t4_trigger_init(&state, &f.program, 0, 1));
    f.switches.bits[31] = 0xff;
    assert(sc64_t4_trigger_step(&state, &f.program, &f.switches, &ops, &f, 0));
    assert(f.switches.bits[31] == 0x7f);
    assert(state.flags[0] == SC64_T4_TR_DONE);

    fixture_init(&f, 1);
    condition_switch(&f.conditions[0][0], 255, 2);
    f.actions[0][0].opcode = 1;
    assert(sc64_t4_trigger_init(&state, &f.program, 0, 1));
    f.switches.bits[31] = 0x80;
    assert(sc64_t4_trigger_step(&state, &f.program, &f.switches, &ops, &f, 0));
    assert(state.outcome == SC64_T4_TR_WON);
}

static void
test_retry_does_not_repeat_completed_effect(void)
{
    Fixture f;
    Sc64T4TriggerState state;
    fixture_init(&f, 1);
    effect(&f.actions[0][0], 1);
    effect(&f.actions[0][1], 2);
    f.fail_once = 1;
    assert(sc64_t4_trigger_init(&state, &f.program, 0, 1));
    assert(sc64_t4_trigger_step(&state, &f.program, &f.switches, &ops, &f, 0));
    assert(f.log_count == 2 && f.log[0] == 1 && f.log[1] == 2);
    assert(state.cursor[0] == 1 && (state.flags[0] & SC64_T4_TR_RUNNING));
    for (unsigned i = 0; i < 29; ++i)
        assert(sc64_t4_trigger_step(&state, &f.program, &f.switches, &ops, &f, 0));
    assert(f.log_count == 2);
    assert(sc64_t4_trigger_step(&state, &f.program, &f.switches, &ops, &f, 0));
    assert(f.log_count == 3 && f.log[2] == 2);
    assert(state.flags[0] == SC64_T4_TR_DONE);
}

static void
test_preserve_rechecks_on_later_pass(void)
{
    Fixture f;
    Sc64T4TriggerState state;
    fixture_init(&f, 1);
    condition_switch(&f.conditions[0][0], 0, 2);
    f.actions[0][0].opcode = 3; /* Preserve this trigger. */
    effect(&f.actions[0][1], 44);
    assert(sc64_t4_trigger_init(&state, &f.program, 0, 1));
    f.switches.bits[0] = 1;
    assert(sc64_t4_trigger_step(&state, &f.program, &f.switches, &ops, &f, 0));
    assert(state.flags[0] == 0 && state.cursor[0] == 0 && f.action_calls == 1);

    /* It remains eligible, but its conditions are freshly sampled each pass. */
    f.switches.bits[0] = 0;
    for (unsigned i = 0; i < 30; ++i)
        assert(sc64_t4_trigger_step(&state, &f.program, &f.switches, &ops, &f, 0));
    assert(state.flags[0] == 0 && f.action_calls == 1);
    f.switches.bits[0] = 1;
    for (unsigned i = 0; i < 30; ++i)
        assert(sc64_t4_trigger_step(&state, &f.program, &f.switches, &ops, &f, 0));
    assert(f.action_calls == 2 && state.flags[0] == 0);
}

static void
test_unknown_disabled_and_zero_opcode_semantics(void)
{
    Fixture f;
    Sc64T4TriggerState state;
    fixture_init(&f, 1);
    f.conditions[0][0].opcode = 250;
    assert(sc64_t4_trigger_init(&state, &f.program, 0, 1));
    assert(!sc64_t4_trigger_step(&state, &f.program, &f.switches, &ops, &f, 0));
    assert(state.fault == SC64_T4_TR_BAD_CONDITION && state.fault_slot == 0);

    fixture_init(&f, 1);
    f.conditions[0][0].opcode = 250;
    f.conditions[0][0].flags = 2; /* Disabled unknown condition is ignored. */
    f.actions[0][0].opcode = 250;
    f.actions[0][0].flags = 2; /* Disabled unknown action is ignored. */
    effect(&f.actions[0][2], 77);
    assert(sc64_t4_trigger_init(&state, &f.program, 0, 1));
    assert(sc64_t4_trigger_step(&state, &f.program, &f.switches, &ops, &f, 0));
    assert(state.flags[0] == SC64_T4_TR_DONE && f.action_calls == 0);

    fixture_init(&f, 1);
    f.actions[0][0].opcode = 250;
    assert(sc64_t4_trigger_init(&state, &f.program, 0, 1));
    assert(!sc64_t4_trigger_step(&state, &f.program, &f.switches, &ops, &f, 0));
    assert(state.fault == SC64_T4_TR_BAD_ACTION && state.fault_slot == 0);
}

static void
test_callback_return_contract_is_exact(void)
{
    Fixture f;
    Sc64T4TriggerState state;
    fixture_init(&f, 1);
    f.conditions[0][0].opcode = 2;
    f.conditions[0][0].comparison = 0;
    f.conditions[0][0].amount = 0;
    f.count_result = 2; /* Invalid: callback must write count and return 1. */
    effect(&f.actions[0][0], 5);
    assert(sc64_t4_trigger_init(&state, &f.program, 0, 1));
    assert(!sc64_t4_trigger_step(&state, &f.program, &f.switches, &ops, &f, 0));
    assert(state.fault == SC64_T4_TR_BAD_CONDITION && state.fault_slot == 0);
    assert(state.cursor[0] == 0 && state.flags[0] == 0 && f.action_calls == 0);

    fixture_init(&f, 1);
    effect(&f.actions[0][0], 6);
    f.action_result = 2; /* Invalid: only 1 means the effect was committed. */
    assert(sc64_t4_trigger_init(&state, &f.program, 0, 1));
    assert(!sc64_t4_trigger_step(&state, &f.program, &f.switches, &ops, &f, 0));
    assert(state.fault == SC64_T4_TR_BAD_ACTION && state.fault_slot == 0);
    assert(state.cursor[0] == 0 && (state.flags[0] & SC64_T4_TR_RUNNING));
    assert(state.flags[0] != SC64_T4_TR_DONE);
}

static void
test_expired_later_wait_can_be_deferred_by_earlier_trigger(void)
{
    Fixture a, b;
    Sc64T4TriggerState state_a, state_b;
    fixture_init(&a, 2);
    fixture_init(&b, 2);
    condition_switch(&a.conditions[0][0], 0, 2);
    condition_switch(&b.conditions[0][0], 0, 2);
    wait_ms(&a.actions[0][0], 1000);
    wait_ms(&b.actions[0][0], 1000);
    effect(&a.actions[0][1], 1);
    effect(&b.actions[0][1], 1);
    wait_ms(&a.actions[1][0], 2000);
    wait_ms(&b.actions[1][0], 2000);
    effect(&a.actions[1][1], 2);
    effect(&b.actions[1][1], 2);
    assert(sc64_t4_trigger_init(&state_a, &a.program, 0, 1));
    assert(sc64_t4_trigger_init(&state_b, &b.program, 0, 1));

    /* Trigger 0 is initially false. The later trigger starts the shared
     * 60-tick wait; make trigger 0 eligible before it expires. */
    assert(sc64_t4_trigger_step(&state_a, &a.program, &a.switches, &ops, &a, 0));
    assert(sc64_t4_trigger_step(&state_b, &b.program, &b.switches, &ops, &b, 0));
    assert(state_a.active_wait == 1 && state_a.wait_ticks == 60);
    assert(state_b.active_wait == 1 && state_b.wait_ticks == 60);
    a.switches.bits[0] |= 1;
    b.switches.bits[0] |= 1;
    for (unsigned tick = 0; tick < 60; ++tick) {
        assert(sc64_t4_trigger_step(&state_a, &a.program, &a.switches, &ops, &a, 0));
        assert(sc64_t4_trigger_step(&state_b, &b.program, &b.switches, &ops, &b, 0));
    }
    /* On expiration, earlier trigger 0 reaches its Wait first and starts a
     * fresh 30-tick wait. Trigger 1's completed marker remains pending. */
    assert(state_a.wait_ticks == 30 && state_a.active_wait == 0);
    assert((state_a.flags[0] & SC64_T4_TR_WAIT) &&
           (state_a.flags[1] & SC64_T4_TR_WAIT));
    assert(sc64_t4_trigger_state_valid(&state_a, &a.program));

    /* A saved copy with both pending wait markers is valid and deterministic. */
    memcpy(&state_b, &state_a, sizeof(state_b));
    memcpy(&b.switches, &a.switches, sizeof(b.switches));
    assert(sc64_t4_trigger_state_valid(&state_b, &b.program));
    for (unsigned tick = 0; tick < 30; ++tick) {
        assert(sc64_t4_trigger_step(&state_a, &a.program, &a.switches, &ops, &a, 0));
        assert(sc64_t4_trigger_step(&state_b, &b.program, &b.switches, &ops, &b, 0));
        assert(memcmp(&state_a, &state_b, sizeof(state_a)) == 0);
        assert(memcmp(&a.switches, &b.switches, sizeof(a.switches)) == 0);
        assert(a.log_count == b.log_count);
        assert(memcmp(a.log, b.log, a.log_count * sizeof(a.log[0])) == 0);
    }
    assert(state_a.wait_ticks == 0 && state_a.active_wait == 255);
    assert(state_a.flags[0] == SC64_T4_TR_DONE &&
           state_a.flags[1] == SC64_T4_TR_DONE);
    assert(a.log_count == 2 && a.log[0] == 1 && a.log[1] == 2);
}

static void
test_disabled_final_slot_still_finishes_trigger(void)
{
    Fixture f;
    Sc64T4TriggerState state;
    fixture_init(&f, 1);
    for (unsigned i = 0; i < 64; ++i) {
        effect(&f.actions[0][i], i);
        f.actions[0][i].flags = 2;
    }
    assert(sc64_t4_trigger_init(&state, &f.program, 0, 1));
    assert(sc64_t4_trigger_step(&state, &f.program, &f.switches, &ops, &f, 0));
    assert(state.flags[0] == SC64_T4_TR_DONE && state.cursor[0] == 0);
    assert(f.action_calls == 0);
}

static void
test_malformed_shared_wait_state_is_rejected(void)
{
    Fixture f;
    Sc64T4TriggerState state;
    fixture_init(&f, 1);
    wait_ms(&f.actions[0][0], 1000);
    assert(sc64_t4_trigger_init(&state, &f.program, 0, 1));
    assert(sc64_t4_trigger_step(&state, &f.program, &f.switches, &ops, &f, 0));
    assert(state.wait_ticks == 30);
    state.wait_ticks = 31;
    assert(!sc64_t4_trigger_state_valid(&state, &f.program));
}

static void
test_enable_gates_terminal_precedence_and_state_resume(void)
{
    Fixture f;
    Sc64T4TriggerState state;
    fixture_init(&f, 4);
    f.triggers[0].enabled_players[0] = 0;
    f.triggers[0].enabled_players[3] = 1;
    f.triggers[1].enabled_players[SC64_T4_ENABLED_ALL_PLAYERS] = 1;
    f.triggers[2].enabled_players[0] = 0;
    f.triggers[2].enabled_players[SC64_T4_ENABLED_FORCE4] = 1; /* Force 4. */
    f.triggers[3].enabled_players[0] = 0;
    f.triggers[3].enabled_players[2] = 1;
    for (unsigned i = 0; i < 4; ++i) f.actions[i][0].opcode = 1;
    assert(sc64_t4_trigger_init(&state, &f.program, 0, 4));
    /* Trigger 0 is player 3 only; triggers 1/2 apply to all and Force 4. */
    f.actions[1][0].opcode = 1;
    f.actions[2][0].opcode = 2;
    assert(sc64_t4_trigger_step(&state, &f.program, &f.switches, &ops, &f, 0));
    assert(state.outcome == SC64_T4_TR_LOST); /* Defeat dominates Victory. */
    assert(state.flags[0] == 0 && state.flags[1] == SC64_T4_TR_DONE &&
           state.flags[2] == SC64_T4_TR_DONE && state.flags[3] == 0);

    fixture_init(&f, 1);
    f.triggers[0].enabled_players[0] = 0;
    f.triggers[0].enabled_players[3] = 1;
    f.actions[0][0].opcode = 1;
    assert(sc64_t4_trigger_init(&state, &f.program, 3, 1));
    assert(sc64_t4_trigger_step(&state, &f.program, &f.switches, &ops, &f, 0));
    assert(state.outcome == SC64_T4_TR_WON);

    /* The force-4 bit alone gates this trigger: force 1 skips it. */
    fixture_init(&f, 1);
    f.triggers[0].enabled_players[0] = 0;
    f.triggers[0].enabled_players[SC64_T4_ENABLED_FORCE4] = 1;
    effect(&f.actions[0][0], 91);
    assert(sc64_t4_trigger_init(&state, &f.program, 0, 1));
    assert(sc64_t4_trigger_step(&state, &f.program, &f.switches, &ops, &f, 0));
    assert(f.action_calls == 0 && state.flags[0] == 0);
    assert(sc64_t4_trigger_init(&state, &f.program, 0, 4));
    assert(sc64_t4_trigger_step(&state, &f.program, &f.switches, &ops, &f, 0));
    assert(f.action_calls == 1 && state.flags[0] == SC64_T4_TR_DONE);
}

static void
test_enabled_owner_selector_namespace_exhaustively(void)
{
    Sc64T4Trigger tr;
    Fixture f;
    memset(&tr, 0, sizeof(tr));
    for (unsigned selector = 0; selector < 27; ++selector) {
        memset(tr.enabled_players, 0, sizeof(tr.enabled_players));
        tr.enabled_players[selector] = 1;
        for (unsigned owner = 0; owner < 8; ++owner) {
            for (unsigned force = 1; force <= 4; ++force) {
                int expected = selector == owner || selector == 17 ||
                               selector == 17 + force;
                /* Unsupported selectors remain fail-closed. */
                assert(sc64_t4_trigger_owner_enabled(&tr, owner, force) == expected);
            }
        }
    }
    assert(!sc64_t4_trigger_owner_enabled(&tr, 8, 1));
    assert(!sc64_t4_trigger_owner_enabled(&tr, 0, 0));
    assert(!sc64_t4_trigger_owner_enabled(&tr, 0, 5));

    /* Unsupported selector bytes must invalidate the source program instead
       of being silently dropped by the runner. */
    for (unsigned selector = 0; selector < 27; ++selector) {
        fixture_init(&f, 1);
        f.triggers[0].enabled_players[0] = 0;
        f.triggers[0].enabled_players[selector] = 1;
        int supported = selector < 8 ||
                        selector == SC64_T4_ENABLED_ALL_PLAYERS ||
                        (selector >= SC64_T4_ENABLED_FORCE1 &&
                         selector <= SC64_T4_ENABLED_FORCE4);
        assert(sc64_t4_trigger_program_valid(&f.program) == supported);
    }
}

static void
test_source_cursor_and_trigger_limit_validation(void)
{
    Fixture f;
    Sc64T4TriggerState state;
    fixture_init(&f, 1);
    f.triggers[0].initial_cursor = 1;
    assert(!sc64_t4_trigger_init(&state, &f.program, 0, 1));
    f.triggers[0].initial_cursor = 0;
    f.triggers[0].execution_flags = 0x80000000u;
    assert(!sc64_t4_trigger_init(&state, &f.program, 0, 1));

    static Sc64T4Trigger triggers[SC64_T4_TRIGGER_LIMIT + 1];
    static Sc64T4Condition conditions[SC64_T4_TRIGGER_LIMIT][16];
    static Sc64T4Action actions[SC64_T4_TRIGGER_LIMIT][64];
    Sc64T4Scenario program;
    memset(triggers, 0, sizeof(triggers));
    for (unsigned i = 0; i < SC64_T4_TRIGGER_LIMIT; ++i) {
        triggers[i].conditions = conditions[i];
        triggers[i].actions = actions[i];
        triggers[i].enabled_players[0] = 1;
    }
    memset(&program, 0, sizeof(program));
    program.triggers = triggers;
    program.trigger_count = SC64_T4_TRIGGER_LIMIT;
    assert(sc64_t4_trigger_init(&state, &program, 0, 1));
    program.trigger_count = SC64_T4_TRIGGER_LIMIT + 1;
    assert(!sc64_t4_trigger_init(&state, &program, 0, 1));
}

static void
test_copied_state_resume_replays_effects_once(void)
{
    Fixture a, b;
    Sc64T4TriggerState state_a, state_b;
    fixture_init(&a, 1);
    fixture_init(&b, 1);
    wait_ms(&a.actions[0][0], 1000);
    wait_ms(&b.actions[0][0], 1000);
    effect(&a.actions[0][1], 1);
    effect(&b.actions[0][1], 1);
    effect(&a.actions[0][2], 2);
    effect(&b.actions[0][2], 2);
    a.fail_once = b.fail_once = 1;
    assert(sc64_t4_trigger_init(&state_a, &a.program, 0, 1));
    assert(sc64_t4_trigger_init(&state_b, &b.program, 0, 1));

    assert(sc64_t4_trigger_step(&state_a, &a.program, &a.switches, &ops, &a, 0));
    assert(sc64_t4_trigger_step(&state_b, &b.program, &b.switches, &ops, &b, 0));
    assert(memcmp(&state_a, &state_b, sizeof(state_a)) == 0);
    assert(state_a.wait_ticks == 30);
    /* Simulate save/load: state bytes and world callbacks are independent. */
    memcpy(&state_b, &state_a, sizeof(state_b));
    memcpy(&b.switches, &a.switches, sizeof(b.switches));

    for (unsigned i = 0; i < 30; ++i) {
        assert(sc64_t4_trigger_step(&state_a, &a.program, &a.switches, &ops, &a, 0));
        assert(sc64_t4_trigger_step(&state_b, &b.program, &b.switches, &ops, &b, 0));
        assert(memcmp(&state_a, &state_b, sizeof(state_a)) == 0);
        assert(memcmp(&a.switches, &b.switches, sizeof(a.switches)) == 0);
        assert(a.log_count == b.log_count);
        assert(memcmp(a.log, b.log, a.log_count * sizeof(a.log[0])) == 0);
    }
    assert(state_a.cursor[0] == 2 && state_a.wait_ticks == 0);
    assert(a.log_count == 2 && a.log[0] == 1 && a.log[1] == 2);
    /* The first effect succeeded once; only the second effect is retried. */
    for (unsigned i = 0; i < 30; ++i) {
        assert(sc64_t4_trigger_step(&state_a, &a.program, &a.switches, &ops, &a, 0));
        assert(sc64_t4_trigger_step(&state_b, &b.program, &b.switches, &ops, &b, 0));
        assert(memcmp(&state_a, &state_b, sizeof(state_a)) == 0);
        assert(memcmp(&a.switches, &b.switches, sizeof(a.switches)) == 0);
        assert(a.log_count == b.log_count);
        assert(memcmp(a.log, b.log, a.log_count * sizeof(a.log[0])) == 0);
    }
    assert(state_a.flags[0] == SC64_T4_TR_DONE);
    assert(a.log_count == 3 && a.log[0] == 1 && a.log[1] == 2 && a.log[2] == 2);
    assert(sc64_t4_trigger_state_valid(&state_a, &a.program));
    state_b.reserved = 1;
    assert(!sc64_t4_trigger_state_valid(&state_b, &b.program));
}

static void
test_switch_bank_is_global_across_player_instances(void)
{
    Fixture f;
    Sc64T4TriggerState player3, player0;
    fixture_init(&f, 2);
    f.triggers[0].enabled_players[0] = 0;
    f.triggers[0].enabled_players[3] = 1;
    f.triggers[1].enabled_players[0] = 1;
    f.actions[0][0].opcode = 13;
    f.actions[0][0].argument = 255;
    f.actions[0][0].modifier = 4;
    condition_switch(&f.conditions[1][0], 255, 2);
    f.actions[1][0].opcode = 1;

    assert(sc64_t4_trigger_init(&player3, &f.program, 3, 1));
    assert(sc64_t4_trigger_init(&player0, &f.program, 0, 1));
    assert(sc64_t4_trigger_step(&player3, &f.program, &f.switches,
                                &ops, &f, 0));
    assert(f.switches.bits[31] & 0x80);
    assert(player3.elapsed_ticks == 1 && player0.elapsed_ticks == 0);
    assert(sc64_t4_trigger_step(&player0, &f.program, &f.switches,
                                &ops, &f, 0));
    assert(player0.outcome == SC64_T4_TR_WON);
    assert(player0.elapsed_ticks == 1 && player3.elapsed_ticks == 1);
}

int
main(void)
{
    test_pause_cooldown_and_30hz_wait();
    test_source_order_shared_wait_and_no_condition_recheck();
    test_switch_boundaries_conditions_and_action_slots();
    test_retry_does_not_repeat_completed_effect();
    test_preserve_rechecks_on_later_pass();
    test_unknown_disabled_and_zero_opcode_semantics();
    test_callback_return_contract_is_exact();
    test_expired_later_wait_can_be_deferred_by_earlier_trigger();
    test_disabled_final_slot_still_finishes_trigger();
    test_malformed_shared_wait_state_is_rejected();
    test_enable_gates_terminal_precedence_and_state_resume();
    test_enabled_owner_selector_namespace_exhaustively();
    test_source_cursor_and_trigger_limit_validation();
    test_copied_state_resume_replays_effects_once();
    test_switch_bank_is_global_across_player_instances();
    puts("sc64 T4 trigger runner tests passed");
    return 0;
}
