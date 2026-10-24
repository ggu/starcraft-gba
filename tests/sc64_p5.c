#include <assert.h>
#include <string.h>
#include "../starcraft/sc64_p5.h"

static void test_source_inventory_and_predicate_rows(void)
{
    static const u8 actions[SC64_P5_TRIGGER_COUNT] = {1, 5, 2, 18, 3, 1, 1, 1, 1, 2, 2};
    static const u8 conditions[SC64_P5_TRIGGER_COUNT] = {1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1};
    Sc64P5State s;
    Sc64P5Input in;
    u16 action_total = 0, condition_total = 0;
    for (u8 i = 0; i < SC64_P5_TRIGGER_COUNT; ++i) {
        assert(sc64_p5_action_count(i) == actions[i]);
        assert(sc64_p5_condition_count(i) == conditions[i]);
        action_total = (u16)(action_total + actions[i]);
        condition_total = (u16)(condition_total + conditions[i]);
    }
    assert(action_total == SC64_P5_ACTION_TOTAL);
    assert(condition_total == SC64_P5_CONDITION_TOTAL);
    assert(SC64_P5_TRIGGER_VICTORY == 4);
    assert(SC64_P5_RAW_ANY_UNIT == 229 && SC64_P5_RAW_HERO == 79 &&
           SC64_P5_RAW_ZEALOT == 65);
    assert(SC64_P5_SELECTOR_CURRENT_PLAYER == 13 &&
           SC64_P5_SELECTOR_ALL_PLAYERS == 17);

    (void)sc64_p5_init(&s);
    assert(sc64_p5_state_valid(&s));
    s.action_cursor[1] = 6;
    assert(!sc64_p5_state_valid(&s));
    (void)sc64_p5_init(&s);
    s.started = 1u << 11;
    assert(!sc64_p5_state_valid(&s));

    memset(&in, 0, sizeof(in));
    in.condition_true[4] = 1;
    assert(sc64_p5_input_valid(&in));
    assert(!sc64_p5_conditions_met(&in, 4)); /* both Bring rows are required */
    in.condition_true[4] = 3;
    assert(sc64_p5_conditions_met(&in, 4));
    in.condition_true[4] = 4;
    assert(!sc64_p5_input_valid(&in)); /* no third row exists */
    in.condition_true[4] = 0;
    in.action_complete = 1u << 11;
    assert(!sc64_p5_input_valid(&in));
}

static void test_independent_programs_and_save_resume(void)
{
    Sc64P5State s, saved;
    Sc64P5Input in;
    Sc64P5Output out;
    memset(&in, 0, sizeof(in));
    (void)sc64_p5_init(&s);
    in.condition_true[2] = 1; /* source Always record */
    in.condition_true[5] = 1;
    in.condition_true[6] = 1;
    out = sc64_p5_step(&s, &in);
    assert(out.action_count == 3);
    assert(out.actions[0].trigger == 2 && out.actions[0].action_index == 0);
    assert(out.actions[1].trigger == 5 && out.actions[1].action_index == 0);
    assert(out.actions[2].trigger == 6 && out.actions[2].action_index == 0);
    assert(sc64_p5_state_valid(&s));

    saved = s;
    memset(&in, 0, sizeof(in));
    in.action_complete = (u16)((1u << 2) | (1u << 5) | (1u << 6));
    out = sc64_p5_step(&saved, &in);
    assert(out.action_count == 1 && out.actions[0].trigger == 2 &&
           out.actions[0].action_index == 1);
    assert(saved.action_cursor[2] == 2);
    assert(saved.action_cursor[5] == 0 && saved.action_cursor[6] == 0);
    assert(sc64_p5_state_valid(&saved));

    /* A completed one-shot record does not restart while its condition stays true. */
    in.action_complete = 0;
    in.condition_true[2] = 1;
    out = sc64_p5_step(&saved, &in);
    assert(out.action_count == 0);
    assert(sc64_p5_state_valid(&saved));

    /* Pause preserves the exact cursor and suppresses output and advancement. */
    saved = s;
    memset(&in, 0, sizeof(in));
    in.action_complete = 1u << 2;
    in.paused = 1;
    out = sc64_p5_step(&saved, &in);
    assert(out.action_count == 0 && saved.action_cursor[2] == 1);
}

static void test_terminal_source_order_and_completion(void)
{
    Sc64P5State s;
    Sc64P5Input in;
    Sc64P5Output out;
    (void)sc64_p5_init(&s);
    memset(&in, 0, sizeof(in));
    in.condition_true[0] = 1;
    in.condition_true[1] = 1;
    in.condition_true[4] = 3;
    out = sc64_p5_step(&s, &in);
    assert(out.action_count == 3);
    assert(out.actions[0].trigger == 0 && out.actions[1].trigger == 1 &&
           out.actions[2].trigger == 4);
    assert(s.outcome == SC64_P5_RUNNING);

    /* Simultaneous terminal completion resolves by ascending source record. */
    (void)sc64_p5_init(&s);
    s.started = (u16)((1u << 0) | (1u << 1) | (1u << 4));
    s.action_cursor[0] = 1;
    s.action_cursor[1] = SC64_P5_ACTIONS_HERO_ABSENT_DEFEAT;
    s.action_cursor[4] = SC64_P5_ACTIONS_VICTORY;
    assert(sc64_p5_state_valid(&s));
    memset(&in, 0, sizeof(in));
    in.action_complete = (u16)((1u << 0) | (1u << 1) | (1u << 4));
    out = sc64_p5_step(&s, &in);
    assert(out.events & SC64_P5_EVENT_LOST);
    assert(!(out.events & SC64_P5_EVENT_WON));
    assert(s.outcome == SC64_P5_LOST && sc64_p5_state_valid(&s));

    /* The first completion wins even if a lower source index is still active. */
    (void)sc64_p5_init(&s);
    s.started = (u16)((1u << 0) | (1u << 4));
    s.action_cursor[0] = 1;
    s.action_cursor[4] = SC64_P5_ACTIONS_VICTORY;
    memset(&in, 0, sizeof(in));
    in.action_complete = 1u << 4;
    out = sc64_p5_step(&s, &in);
    assert(out.events & SC64_P5_EVENT_WON);
    assert(s.outcome == SC64_P5_WON && s.action_cursor[0] == 1);
    assert(sc64_p5_state_valid(&s));

    /* A terminal save may contain exactly the completing terminal program;
     * another started terminal must still have a pending action cursor. */
    (void)sc64_p5_init(&s);
    s.started = (u16)((1u << 0) | (1u << 1));
    s.outcome = SC64_P5_LOST;
    assert(!sc64_p5_state_valid(&s)); /* Both defeat programs claim completion. */
    (void)sc64_p5_init(&s);
    s.started = (u16)((1u << 0) | (1u << 4));
    s.outcome = SC64_P5_WON;
    assert(!sc64_p5_state_valid(&s)); /* Loss is also marked complete. */
}

static void test_loss_sequence_save_resume(void)
{
    Sc64P5State s, resumed;
    Sc64P5Input in;
    Sc64P5Output out;
    (void)sc64_p5_init(&s);
    memset(&in, 0, sizeof(in));
    in.condition_true[1] = 1;
    out = sc64_p5_step(&s, &in);
    assert(out.action_count == 1 && out.actions[0].trigger == 1 &&
           out.actions[0].action_index == 0);
    resumed = s;
    for (u8 index = 1; index < SC64_P5_ACTIONS_HERO_ABSENT_DEFEAT; ++index) {
        memset(&in, 0, sizeof(in));
        in.action_complete = 1u << 1;
        out = sc64_p5_step(&resumed, &in);
        assert(out.action_count == 1 && out.actions[0].action_index == index);
        assert(resumed.outcome == SC64_P5_RUNNING);
        assert(sc64_p5_state_valid(&resumed));
    }
    memset(&in, 0, sizeof(in));
    in.action_complete = 1u << 1;
    out = sc64_p5_step(&resumed, &in);
    assert(out.events & SC64_P5_EVENT_LOST);
    assert(resumed.outcome == SC64_P5_LOST && sc64_p5_state_valid(&resumed));
}

int main(void)
{
    test_source_inventory_and_predicate_rows();
    test_independent_programs_and_save_resume();
    test_terminal_source_order_and_completion();
    test_loss_sequence_save_resume();
    return 0;
}
