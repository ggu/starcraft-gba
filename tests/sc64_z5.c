#include <assert.h>
#include <string.h>
#include "../starcraft/sc64_z5.h"

static void input_set(u32 *lo, u32 *hi, u8 trigger)
{
    sc64_z5_mask_set(lo, hi, trigger);
}

static void condition_set(Sc64Z5Input *in, u8 trigger, u16 mask)
{
    assert(trigger < SC64_Z5_TRIGGER_COUNT);
    in->condition_true[trigger] = mask;
}

static void test_source_inventory_and_validation(void)
{
    static const u8 expected[SC64_Z5_TRIGGER_COUNT] = {
        41, 6, 5, 2, 7, 11, 7, 5, 5, 1, 5, 1, 1, 1, 1, 1, 1, 2, 6, 3,
        1, 1, 1, 1, 1, 1, 7, 1, 1, 1, 1, 2, 2, 2, 5, 8, 5, 0, 2, 0
    };
    Sc64Z5State s;
    Sc64Z5Input in;
    u16 total = 0;
    u16 condition_total = 0;
    u8 active = 0;
    u8 i;
    for (i = 0; i < SC64_Z5_TRIGGER_COUNT; ++i) {
        assert(sc64_z5_action_count(i) == expected[i]);
        total = (u16)(total + expected[i]);
        condition_total = (u16)(condition_total + sc64_z5_condition_count(i));
        if (expected[i]) ++active;
    }
    assert(total == SC64_Z5_ACTION_TOTAL);
    assert(condition_total == SC64_Z5_CONDITION_TOTAL);
    assert(active == SC64_Z5_ACTIVE_TRIGGER_COUNT);
    assert(sc64_z5_trigger_preserved(7));
    assert(sc64_z5_trigger_preserved(8));
    assert(sc64_z5_trigger_preserved(34));
    assert(!sc64_z5_trigger_preserved(37));
    assert(!sc64_z5_action_count(37) && !sc64_z5_action_count(39));

    (void)sc64_z5_init(&s);
    assert(sc64_z5_state_valid(&s));
    s.action_cursor[0] = 42;
    assert(!sc64_z5_state_valid(&s));
    (void)sc64_z5_init(&s);
    s.started_hi = 1u << 8;
    assert(!sc64_z5_state_valid(&s));
    (void)sc64_z5_init(&s);
    s.started_hi = 1u << (37 - 32);
    assert(!sc64_z5_state_valid(&s));
    (void)sc64_z5_init(&s);
    s.action_cursor[7] = 1;
    assert(!sc64_z5_state_valid(&s));

    memset(&in, 0, sizeof(in));
    in.condition_true[7] = 1; /* Both source conditions are required. */
    assert(sc64_z5_input_valid(&in));
    assert(!sc64_z5_conditions_met(&in, 7));
    in.condition_true[7] = 3;
    assert(sc64_z5_conditions_met(&in, 7));
    in.condition_true[37] = 1; /* Empty trigger has no legal condition rows. */
    assert(!sc64_z5_input_valid(&in));
    memset(&in, 0, sizeof(in));
    in.action_complete_hi = 1u << 5; /* Trigger 37 has no action program. */
    assert(!sc64_z5_input_valid(&in));
    in.action_complete_hi = 1u << 8; /* Out of the two-word trigger range. */
    assert(!sc64_z5_input_valid(&in));

    (void)sc64_z5_init(&s);
    sc64_z5_mask_set(&s.started_lo, &s.started_hi, SC64_Z5_TRIGGER_VICTORY);
    assert(!sc64_z5_state_valid(&s)); /* Started terminal with no pending action. */
    s.action_cursor[SC64_Z5_TRIGGER_VICTORY] = 1;
    s.outcome = SC64_Z5_WON;
    assert(!sc64_z5_state_valid(&s)); /* Won before the terminal program completes. */
    (void)sc64_z5_init(&s);
    sc64_z5_mask_set(&s.started_lo, &s.started_hi, SC64_Z5_TRIGGER_VICTORY);
    sc64_z5_mask_set(&s.started_lo, &s.started_hi, SC64_Z5_TRIGGER_DEFEAT);
    s.outcome = SC64_Z5_WON; /* Both terminal programs claim completion. */
    assert(!sc64_z5_state_valid(&s));
    s.outcome = SC64_Z5_LOST; /* The wrong terminal record completed first. */
    assert(!sc64_z5_state_valid(&s));
}

static void test_pause_one_shot_and_resume(void)
{
    Sc64Z5State state, restored;
    Sc64Z5Input in;
    Sc64Z5Output out;
    memset(&in, 0, sizeof(in));
    (void)sc64_z5_init(&state);
    condition_set(&in, 4, 1);
    in.paused = 1;
    out = sc64_z5_step(&state, &in);
    assert(out.action_count == 0 && state.started_lo == 0);
    in.paused = 0;
    out = sc64_z5_step(&state, &in);
    assert(out.action_count == 1 && out.actions[0].trigger == 4 &&
           out.actions[0].action_index == 0);
    assert(state.action_cursor[4] == 1);
    assert(sc64_z5_state_valid(&state));

    /* A byte-exact save/restore while action 0 is pending resumes at action 1. */
    restored = state;
    memset(&in, 0, sizeof(in));
    input_set(&in.action_complete_lo, &in.action_complete_hi, 4);
    out = sc64_z5_step(&restored, &in);
    assert(out.action_count == 1 && out.actions[0].action_index == 1);
    assert(restored.action_cursor[4] == 2);
    assert(sc64_z5_state_valid(&restored));

    /* Completing the final action leaves a one-shot trigger latched. */
    for (u8 n = 2; n < sc64_z5_action_count(4); ++n) {
        in.action_complete_lo = 0;
        in.action_complete_hi = 0;
        input_set(&in.action_complete_lo, &in.action_complete_hi, 4);
        out = sc64_z5_step(&restored, &in);
        assert(out.action_count == 1 && out.actions[0].action_index == n);
    }
    memset(&in, 0, sizeof(in));
    input_set(&in.action_complete_lo, &in.action_complete_hi, 4);
    out = sc64_z5_step(&restored, &in);
    assert(out.action_count == 0 && restored.action_cursor[4] == 0);
    condition_set(&in, 4, 1);
    out = sc64_z5_step(&restored, &in);
    assert(out.action_count == 0);
    assert(sc64_z5_state_valid(&restored));
}

static void test_multiple_active_and_preserve_rearm(void)
{
    Sc64Z5State s;
    Sc64Z5Input in;
    Sc64Z5Output out;
    memset(&in, 0, sizeof(in));
    (void)sc64_z5_init(&s);
    condition_set(&in, 7, 3);
    condition_set(&in, 8, 1);
    condition_set(&in, 34, 1);
    out = sc64_z5_step(&s, &in);
    assert(out.action_count == 3);
    assert(out.actions[0].trigger == 7 && out.actions[1].trigger == 8 &&
           out.actions[2].trigger == 34);
    assert(sc64_z5_state_valid(&s));

    /* Advance all three independent action programs from a copied save. */
    {
        Sc64Z5State saved = s;
        memset(&in, 0, sizeof(in));
        input_set(&in.action_complete_lo, &in.action_complete_hi, 7);
        input_set(&in.action_complete_lo, &in.action_complete_hi, 8);
        input_set(&in.action_complete_lo, &in.action_complete_hi, 34);
        out = sc64_z5_step(&saved, &in);
        assert(out.action_count == 3);
        assert(out.actions[0].trigger == 7 && out.actions[0].action_index == 1);
        assert(out.actions[1].trigger == 8 && out.actions[1].action_index == 1);
        assert(out.actions[2].trigger == 34 && out.actions[2].action_index == 1);
        assert(sc64_z5_state_valid(&saved));
    }

    /* A preserved record restarts after completion while its predicate remains true. */
    {
        Sc64Z5State preserve;
        (void)sc64_z5_init(&preserve);
        memset(&in, 0, sizeof(in));
        condition_set(&in, 7, 3);
        out = sc64_z5_step(&preserve, &in);
        assert(out.action_count == 1 && out.actions[0].action_index == 0);
        for (u8 action = 1; action < 5; ++action) {
            memset(&in, 0, sizeof(in));
            input_set(&in.action_complete_lo, &in.action_complete_hi, 7);
            out = sc64_z5_step(&preserve, &in);
            assert(out.action_count == 1 && out.actions[0].action_index == action);
        }
        assert(sc64_z5_state_valid(&preserve));
        memset(&in, 0, sizeof(in));
        input_set(&in.action_complete_lo, &in.action_complete_hi, 7);
        out = sc64_z5_step(&preserve, &in); /* complete action 4 */
        assert(out.action_count == 0 && preserve.action_cursor[7] == 0);
        memset(&in, 0, sizeof(in));
        condition_set(&in, 7, 3);
        out = sc64_z5_step(&preserve, &in);
        assert(out.action_count == 1 && out.actions[0].action_index == 0);
        assert(sc64_z5_state_valid(&preserve));
    }
}

static void test_terminal_ordering(void)
{
    Sc64Z5State s;
    Sc64Z5Input in;
    Sc64Z5Output out;
    memset(&in, 0, sizeof(in));
    (void)sc64_z5_init(&s);
    condition_set(&in, SC64_Z5_TRIGGER_VICTORY, 1);
    condition_set(&in, SC64_Z5_TRIGGER_DEFEAT, 1);
    out = sc64_z5_step(&s, &in);
    assert(out.action_count == 2 && out.actions[0].trigger == SC64_Z5_TRIGGER_VICTORY &&
           out.actions[1].trigger == SC64_Z5_TRIGGER_DEFEAT);
    assert(s.action_cursor[SC64_Z5_TRIGGER_VICTORY] == 1);
    assert(s.action_cursor[SC64_Z5_TRIGGER_DEFEAT] == 1);

    /* If both terminal actions complete together, source record order gives
     * the earlier victory record precedence. */
    (void)sc64_z5_init(&s);
    sc64_z5_mask_set(&s.started_lo, &s.started_hi, SC64_Z5_TRIGGER_VICTORY);
    sc64_z5_mask_set(&s.started_lo, &s.started_hi, SC64_Z5_TRIGGER_DEFEAT);
    s.action_cursor[SC64_Z5_TRIGGER_VICTORY] = SC64_Z5_ACTIONS_VICTORY;
    s.action_cursor[SC64_Z5_TRIGGER_DEFEAT] = SC64_Z5_ACTIONS_DEFEAT;
    assert(sc64_z5_state_valid(&s));
    memset(&in, 0, sizeof(in));
    input_set(&in.action_complete_lo, &in.action_complete_hi,
              SC64_Z5_TRIGGER_VICTORY);
    input_set(&in.action_complete_lo, &in.action_complete_hi,
              SC64_Z5_TRIGGER_DEFEAT);
    out = sc64_z5_step(&s, &in);
    assert(out.events & SC64_Z5_EVENT_WON);
    assert(!(out.events & SC64_Z5_EVENT_LOST));
    assert(s.outcome == SC64_Z5_WON && sc64_z5_state_valid(&s));

    /* The first terminal action may win the race even when it is the later
     * source record; completion time takes precedence over source order. */
    (void)sc64_z5_init(&s);
    sc64_z5_mask_set(&s.started_lo, &s.started_hi, SC64_Z5_TRIGGER_VICTORY);
    sc64_z5_mask_set(&s.started_lo, &s.started_hi, SC64_Z5_TRIGGER_DEFEAT);
    s.action_cursor[SC64_Z5_TRIGGER_VICTORY] = 1;
    s.action_cursor[SC64_Z5_TRIGGER_DEFEAT] = SC64_Z5_ACTIONS_DEFEAT;
    assert(sc64_z5_state_valid(&s));
    memset(&in, 0, sizeof(in));
    input_set(&in.action_complete_lo, &in.action_complete_hi,
              SC64_Z5_TRIGGER_DEFEAT);
    out = sc64_z5_step(&s, &in);
    assert(out.events & SC64_Z5_EVENT_LOST);
    assert(s.outcome == SC64_Z5_LOST && s.action_cursor[SC64_Z5_TRIGGER_VICTORY] == 1);
    assert(sc64_z5_state_valid(&s));

    /* Acknowledgements for idle programs are malformed and leave state intact. */
    (void)sc64_z5_init(&s);
    memset(&in, 0, sizeof(in));
    input_set(&in.action_complete_lo, &in.action_complete_hi, 3);
    assert(!sc64_z5_input_state_valid(&s, &in));
    out = sc64_z5_step(&s, &in);
    assert(out.action_count == 0 && sc64_z5_state_valid(&s));

    /* Loss by itself commits only after its final action is acknowledged. */
    (void)sc64_z5_init(&s);
    memset(&in, 0, sizeof(in));
    condition_set(&in, SC64_Z5_TRIGGER_DEFEAT, 1);
    out = sc64_z5_step(&s, &in);
    assert(out.action_count == 1 && out.actions[0].trigger == SC64_Z5_TRIGGER_DEFEAT);
    {
        Sc64Z5State saved = s;
        for (u8 index = 1; index < SC64_Z5_ACTIONS_DEFEAT; ++index) {
            memset(&in, 0, sizeof(in));
            input_set(&in.action_complete_lo, &in.action_complete_hi,
                      SC64_Z5_TRIGGER_DEFEAT);
            out = sc64_z5_step(&saved, &in);
            assert(out.action_count == 1 && out.actions[0].action_index == index);
            assert(saved.outcome == SC64_Z5_RUNNING);
        }
        memset(&in, 0, sizeof(in));
        input_set(&in.action_complete_lo, &in.action_complete_hi,
                  SC64_Z5_TRIGGER_DEFEAT);
        out = sc64_z5_step(&saved, &in);
        assert(out.events & SC64_Z5_EVENT_LOST);
        assert(saved.outcome == SC64_Z5_LOST && sc64_z5_state_valid(&saved));
    }
}

int main(void)
{
    test_source_inventory_and_validation();
    test_pause_one_shot_and_resume();
    test_multiple_active_and_preserve_rearm();
    test_terminal_ordering();
    return 0;
}
