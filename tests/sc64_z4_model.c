#include <assert.h>
#include <string.h>
#include "../starcraft/sc64_z4.h"

static Sc64Z4Input quiet_input(void)
{
    Sc64Z4Input in;
    memset(&in, 0, sizeof(in));
    in.kerrigan_count = 1;
    in.chrysalis_count = 1;
    in.command_center_count = 1;
    return in;
}

static Sc64Z4ActionEvent find_action(Sc64Z4Output out, u8 trigger)
{
    u8 i;
    for (i = 0; i < out.action_count; ++i)
        if (out.actions[i].trigger == trigger) return out.actions[i];
    assert(!"expected source action event");
    return out.actions[0];
}

static void finish_sequence(Sc64Z4State *s, Sc64Z4Input in,
                           u8 trigger, u8 action_count, u16 terminal_event)
{
    u8 current;
    for (current = 0; current < action_count; ++current) {
        Sc64Z4Output out;
        in.complete_actions = (u8)(1u << trigger);
        out = sc64_z4_step(s, &in);
        if (current + 1u < action_count) {
            Sc64Z4ActionEvent action = find_action(out, trigger);
            assert(action.action_index == current + 1u);
        } else {
            assert(out.action_count == 0 || out.events & terminal_event);
            if (terminal_event) assert(out.events & terminal_event);
        }
        assert(sc64_z4_state_valid(s));
    }
}

static void test_source_action_programs(void)
{
    Sc64Z4State s;
    Sc64Z4Input in = quiet_input();
    Sc64Z4Output out;
    u8 i;
    sc64_z4_init(&s);
    out = sc64_z4_step(&s, &in);
    assert(out.events & SC64_Z4_EVENT_TRIGGER_STARTUP);
    assert(find_action(out, SC64_Z4_TRIGGER_STARTUP).action_index == 0);
    assert(SC64_Z4_ACTIONS_KERRIGAN_DEFEAT == 5);
    assert(SC64_Z4_ACTIONS_STARTUP == 14);
    assert(SC64_Z4_ACTIONS_HATCH_WARNING == 1);
    assert(SC64_Z4_ACTIONS_HATCH == 24);
    assert(SC64_Z4_ACTIONS_COMMAND_CENTER_WIN == 14);
    assert(SC64_Z4_ACTIONS_FORCE_START == 3);
    assert(SC64_Z4_ACTIONS_CHRYSALIS_DEFEAT == 5);

    /* The source setup record sets the 600-unit timer at action index 2. */
    for (i = 1; i <= SC64_Z4_ACTIONS_STARTUP; ++i) {
        in.complete_actions = (u8)(1u << SC64_Z4_TRIGGER_STARTUP);
        out = sc64_z4_step(&s, &in);
        if (i < SC64_Z4_ACTIONS_STARTUP) {
            Sc64Z4ActionEvent action = find_action(out, SC64_Z4_TRIGGER_STARTUP);
            assert(action.action_index == i);
        } else {
            assert(s.action_cursor[SC64_Z4_TRIGGER_STARTUP] == 0);
        }
        if (i == 2) {
            Sc64Z4ActionEvent action = find_action(out, SC64_Z4_TRIGGER_STARTUP);
            assert(action.action_index == 2);
            assert(SC64_Z4_MISSION_COUNTDOWN == 600);
        }
    }
    in.complete_actions = 0;
    assert(sc64_z4_state_valid(&s));
}

static void test_countdown_warning_and_hatch_predicates(void)
{
    Sc64Z4State s;
    Sc64Z4Input in = quiet_input();
    Sc64Z4Output out;
    sc64_z4_init(&s);
    (void)sc64_z4_step(&s, &in); /* ElapsedTime >= 0: startup. */
    in.countdown_active = 1;
    in.countdown_remaining = 31;
    out = sc64_z4_step(&s, &in);
    assert(!(out.events & SC64_Z4_EVENT_TRIGGER_HATCH_WARNING));
    in.countdown_remaining = SC64_Z4_HATCH_WARNING_COUNTDOWN;
    out = sc64_z4_step(&s, &in);
    assert(out.events & SC64_Z4_EVENT_TRIGGER_HATCH_WARNING);
    assert(find_action(out, SC64_Z4_TRIGGER_HATCH_WARNING).action_index == 0);
    out = sc64_z4_step(&s, &in);
    assert(!(out.events & SC64_Z4_EVENT_TRIGGER_HATCH_WARNING)); /* one shot */
    in.complete_actions = (u8)(1u << SC64_Z4_TRIGGER_HATCH_WARNING);
    out = sc64_z4_step(&s, &in);
    assert(s.action_cursor[SC64_Z4_TRIGGER_HATCH_WARNING] == 0);
    assert(out.action_count == 0);
    in.complete_actions = 0;
    in.countdown_remaining = 0;
    out = sc64_z4_step(&s, &in);
    assert(out.events & SC64_Z4_EVENT_TRIGGER_HATCH);
    assert(find_action(out, SC64_Z4_TRIGGER_HATCH).action_index == 0);
    finish_sequence(&s, in, SC64_Z4_TRIGGER_HATCH,
                    SC64_Z4_ACTIONS_HATCH, SC64_Z4_EVENT_NONE);
    assert(s.action_cursor[SC64_Z4_TRIGGER_HATCH] == 0);
    assert(s.outcome == SC64_Z4_RUNNING);
}

static void test_hatch_transaction_does_not_self_trigger_survival_losses(void)
{
    Sc64Z4State s;
    Sc64Z4Input in = quiet_input();
    Sc64Z4Output out;
    sc64_z4_init(&s);
    (void)sc64_z4_step(&s, &in);
    in.countdown_active = 1;
    in.countdown_remaining = 0;
    out = sc64_z4_step(&s, &in);
    assert(out.events & SC64_Z4_EVENT_TRIGGER_HATCH);

    /* The real hatch removes the sole Chrysalis, waits, clears switch 0,
     * then creates Kerrigan. Keep true zero counts and exercise those
     * hazardous intermediate states while the authored hatch runs. */
    in.chrysalis_count = 0;
    in.kerrigan_count = 0;
    in.switch_condition_2 = 1;
    in.switch_condition_3 = 0;
    in.suppress_chrysalis_loss = 1;
    in.kerrigan_survival_armed = 0;
    for (u8 action = 0; action < SC64_Z4_ACTIONS_HATCH; ++action) {
        in.complete_actions = (u8)(1u << SC64_Z4_TRIGGER_HATCH);
        out = sc64_z4_step(&s, &in);
        assert(!(out.events & (SC64_Z4_EVENT_TRIGGER_CHRYSALIS_DEFEAT |
                               SC64_Z4_EVENT_TRIGGER_KERRIGAN_DEFEAT |
                               SC64_Z4_EVENT_LOST)));
        assert(s.outcome == SC64_Z4_RUNNING);
    }
    assert(s.action_cursor[SC64_Z4_TRIGGER_HATCH] == 0);
    in.suppress_chrysalis_loss = 0;
    in.chrysalis_count = 0;
    in.kerrigan_count = 1; /* Adapter created the source Kerrigan actor. */
    in.switch_condition_2 = 0;
    in.switch_condition_3 = 1;
    in.complete_actions = 0;
    out = sc64_z4_step(&s, &in);
    assert(!(out.events & (SC64_Z4_EVENT_TRIGGER_CHRYSALIS_DEFEAT |
                           SC64_Z4_EVENT_TRIGGER_KERRIGAN_DEFEAT |
                           SC64_Z4_EVENT_LOST)));

    in.kerrigan_count = 0;
    in.elapsed_seconds = SC64_Z4_ELAPSED_DEFEAT_GATE;
    in.kerrigan_survival_armed = 1;
    out = sc64_z4_step(&s, &in);
    assert(out.events & SC64_Z4_EVENT_TRIGGER_KERRIGAN_DEFEAT);
    assert(!(out.events & SC64_Z4_EVENT_LOST));
}

static void test_force_start_action_sequence(void)
{
    Sc64Z4State s;
    Sc64Z4Input in = quiet_input();
    Sc64Z4Output out;
    sc64_z4_init(&s);
    in.force2_any_units = 0;
    out = sc64_z4_step(&s, &in);
    assert(!(out.events & SC64_Z4_EVENT_TRIGGER_FORCE_START));
    in.force2_any_units = 1;
    out = sc64_z4_step(&s, &in);
    assert(out.events & SC64_Z4_EVENT_TRIGGER_FORCE_START);
    assert(find_action(out, SC64_Z4_TRIGGER_FORCE_START).action_index == 0);
    finish_sequence(&s, in, SC64_Z4_TRIGGER_FORCE_START,
                    SC64_Z4_ACTIONS_FORCE_START, SC64_Z4_EVENT_NONE);
    out = sc64_z4_step(&s, &in);
    assert(!(out.events & SC64_Z4_EVENT_TRIGGER_FORCE_START));
}

static void test_kerrigan_and_chrysalis_survival_gates(void)
{
    Sc64Z4State s;
    Sc64Z4Input in = quiet_input();
    Sc64Z4Output out;
    sc64_z4_init(&s);
    in.elapsed_seconds = 0;
    in.kerrigan_count = 0;
    in.switch_condition_3 = 1;
    out = sc64_z4_step(&s, &in);
    assert(!(out.events & SC64_Z4_EVENT_TRIGGER_KERRIGAN_DEFEAT));
    in.elapsed_seconds = SC64_Z4_ELAPSED_DEFEAT_GATE;
    in.kerrigan_survival_armed = 1;
    in.kerrigan_count = 1;
    out = sc64_z4_step(&s, &in);
    assert(!(out.events & SC64_Z4_EVENT_TRIGGER_KERRIGAN_DEFEAT));
    in.kerrigan_count = 0;
    out = sc64_z4_step(&s, &in);
    assert(out.events & SC64_Z4_EVENT_TRIGGER_KERRIGAN_DEFEAT);
    assert(find_action(out, SC64_Z4_TRIGGER_KERRIGAN_DEFEAT).action_index == 0);
    finish_sequence(&s, in, SC64_Z4_TRIGGER_KERRIGAN_DEFEAT,
                    SC64_Z4_ACTIONS_KERRIGAN_DEFEAT, SC64_Z4_EVENT_LOST);
    assert(s.outcome == SC64_Z4_LOST);
    out = sc64_z4_step(&s, &in);
    assert(out.events == SC64_Z4_EVENT_NONE && out.action_count == 0);

    sc64_z4_init(&s);
    in = quiet_input();
    (void)sc64_z4_step(&s, &in);
    in.chrysalis_count = 0;
    in.switch_condition_2 = 0;
    out = sc64_z4_step(&s, &in);
    assert(!(out.events & SC64_Z4_EVENT_TRIGGER_CHRYSALIS_DEFEAT));
    in.switch_condition_2 = 1;
    out = sc64_z4_step(&s, &in);
    assert(out.events & SC64_Z4_EVENT_TRIGGER_CHRYSALIS_DEFEAT);
    finish_sequence(&s, in, SC64_Z4_TRIGGER_CHRYSALIS_DEFEAT,
                    SC64_Z4_ACTIONS_CHRYSALIS_DEFEAT, SC64_Z4_EVENT_LOST);
    assert(s.outcome == SC64_Z4_LOST);
}

static void test_command_center_objective_and_sticky_win(void)
{
    Sc64Z4State s;
    Sc64Z4Input in = quiet_input();
    Sc64Z4Output out;
    sc64_z4_init(&s);
    in.command_center_count = 1;
    out = sc64_z4_step(&s, &in);
    assert(!(out.events & SC64_Z4_EVENT_TRIGGER_COMMAND_CENTER_WIN));
    in.command_center_count = 0;
    out = sc64_z4_step(&s, &in);
    assert(out.events & SC64_Z4_EVENT_TRIGGER_COMMAND_CENTER_WIN);
    assert(find_action(out, SC64_Z4_TRIGGER_COMMAND_CENTER_WIN).action_index == 0);
    finish_sequence(&s, in, SC64_Z4_TRIGGER_COMMAND_CENTER_WIN,
                    SC64_Z4_ACTIONS_COMMAND_CENTER_WIN, SC64_Z4_EVENT_WON);
    assert(s.outcome == SC64_Z4_WON);
    out = sc64_z4_step(&s, &in);
    assert(out.events == SC64_Z4_EVENT_NONE && out.action_count == 0);
}

static void test_simultaneous_terminal_actions_choose_loss_policy(void)
{
    Sc64Z4State s;
    Sc64Z4Input in = quiet_input();
    Sc64Z4Output out;
    sc64_z4_init(&s);
    s.fired = (u16)((1u << SC64_Z4_TRIGGER_KERRIGAN_DEFEAT) |
                    (1u << SC64_Z4_TRIGGER_COMMAND_CENTER_WIN));
    s.action_cursor[SC64_Z4_TRIGGER_KERRIGAN_DEFEAT] =
        SC64_Z4_ACTIONS_KERRIGAN_DEFEAT;
    s.action_cursor[SC64_Z4_TRIGGER_COMMAND_CENTER_WIN] =
        SC64_Z4_ACTIONS_COMMAND_CENTER_WIN;
    assert(sc64_z4_state_valid(&s));
    in.complete_actions = (u8)((1u << SC64_Z4_TRIGGER_KERRIGAN_DEFEAT) |
                                (1u << SC64_Z4_TRIGGER_COMMAND_CENTER_WIN));
    out = sc64_z4_step(&s, &in);
    assert(out.events & SC64_Z4_EVENT_LOST);
    assert(!(out.events & SC64_Z4_EVENT_WON));
    assert(s.outcome == SC64_Z4_LOST);
}

static void test_pause_concurrency_and_validation(void)
{
    Sc64Z4State s;
    Sc64Z4Input in = quiet_input();
    Sc64Z4Output out;
    sc64_z4_init(&s);
    in.paused = 1;
    out = sc64_z4_step(&s, &in);
    assert(out.events == 0 && out.action_count == 0 && s.fired == 0);
    in.paused = 0;
    in.countdown_active = 1;
    in.countdown_remaining = 0;
    in.force2_any_units = 1;
    out = sc64_z4_step(&s, &in);
    assert((out.events & SC64_Z4_EVENT_TRIGGER_STARTUP) &&
           (out.events & SC64_Z4_EVENT_TRIGGER_HATCH_WARNING) &&
           (out.events & SC64_Z4_EVENT_TRIGGER_HATCH) &&
           (out.events & SC64_Z4_EVENT_TRIGGER_FORCE_START));
    assert(out.action_count == 4); /* Independent source trigger records. */
    assert(sc64_z4_state_valid(&s));

    s.fired |= (u16)(1u << 12);
    assert(!sc64_z4_state_valid(&s));
    sc64_z4_init(&s);
    s.fired = (u16)(1u << SC64_Z4_TRIGGER_STARTUP);
    s.action_cursor[SC64_Z4_TRIGGER_STARTUP] = SC64_Z4_ACTIONS_STARTUP + 1;
    assert(!sc64_z4_state_valid(&s));
    sc64_z4_init(&s);
    s.outcome = SC64_Z4_WON;
    assert(!sc64_z4_state_valid(&s));
}

int main(void)
{
    test_source_action_programs();
    test_countdown_warning_and_hatch_predicates();
    test_hatch_transaction_does_not_self_trigger_survival_losses();
    test_force_start_action_sequence();
    test_kerrigan_and_chrysalis_survival_gates();
    test_command_center_objective_and_sticky_win();
    test_simultaneous_terminal_actions_choose_loss_policy();
    test_pause_concurrency_and_validation();
    return 0;
}
