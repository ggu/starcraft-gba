#include <assert.h>
#include <string.h>
#include "../starcraft/sc64_p4.h"

static void live_input(Sc64P4Input *in)
{
    memset(in, 0, sizeof(*in));
    in->current_any_units = 4;
    in->all_ray_vultures = 1;
    in->all_tassadars = 1;
}

static void test_raw_identity_and_profile_gaps(void)
{
    assert(SC64_P4_GLOBAL_ID == 23);
    assert(SC64_P4_WIDTH_TILES == 128 && SC64_P4_HEIGHT_TILES == 128);
    assert(SC64_P4_HUMAN_SOURCE_SLOT == 7);
    assert(SC64_P4_SELECTOR_CURRENT_PLAYER == 13);
    assert(SC64_P4_SELECTOR_ALL_PLAYERS == 17);
    assert(SC64_P4_SELECTOR_FORCE1 == 18 && SC64_P4_SELECTOR_FORCE2 == 19);
    assert(SC64_P4_RAW_RAYNOR_VULTURE == 19);
    assert(SC64_P4_RAW_TASSADAR == 79);
    assert(SC64_P4_RAW_ALDARIS == 87);
    assert(SC64_P4_RAW_HYDRALISK == 38);
    assert(SC64_P4_RAW_ULTRALISK == 39);
    assert(SC64_P4_TRIGGER_COUNT == 13);
}

static void test_startup_and_all_nonresult_trigger_gates(void)
{
    Sc64P4State s;
    Sc64P4Input in;
    u16 events;
    live_input(&in);
    sc64_p4_init(&s);
    in.elapsed_time_ge_zero = 1; /* Source record 11: elapsed >= 0. */
    in.current_ray_vultures = 1;
    in.current_tassadars = 1;
    in.switch0_set = 1;
    in.switch1_set = 1;
    events = sc64_p4_step(&s, &in);
    assert(events & SC64_P4_EVENT_TRIGGER_3);
    assert(events & SC64_P4_EVENT_TRIGGER_5);
    assert(events & SC64_P4_EVENT_TRIGGER_6);
    assert(events & SC64_P4_EVENT_TRIGGER_7);
    assert(events & SC64_P4_EVENT_TRIGGER_8);
    assert(events & SC64_P4_EVENT_TRIGGER_9);
    assert(events & SC64_P4_EVENT_TRIGGER_10);
    assert(events & SC64_P4_EVENT_TRIGGER_11);
    assert(events & SC64_P4_EVENT_TRIGGER_12);
    assert(sc64_p4_state_valid(&s));
    assert(sc64_p4_step(&s, &in) == 0); /* one-shot gates */
}

static void test_survival_loss_pause_and_stickiness(void)
{
    Sc64P4State s;
    Sc64P4Input in;
    live_input(&in);
    sc64_p4_init(&s);
    in.paused = 1;
    assert(sc64_p4_step(&s, &in) == 0);
    assert(s.fired == 0);
    in.paused = 0;
    in.current_any_units = 0;
    assert((sc64_p4_step(&s, &in) &
            (SC64_P4_EVENT_TRIGGER_0 | SC64_P4_EVENT_LOST)) ==
           (SC64_P4_EVENT_TRIGGER_0 | SC64_P4_EVENT_LOST));
    assert(s.outcome == SC64_P4_LOST && sc64_p4_state_valid(&s));
    in.current_any_units = 5;
    assert(sc64_p4_step(&s, &in) == 0);
    assert(s.outcome == SC64_P4_LOST);
}

static void test_hero_survival_loss_sequences_keep_raw_wait_facts(void)
{
    Sc64P4State s;
    Sc64P4Input in;
    unsigned i;
    live_input(&in);
    sc64_p4_init(&s);
    in.all_ray_vultures = 0;
    in.switch13_set = 1;
    assert(sc64_p4_step(&s, &in) & SC64_P4_EVENT_ACTION);
    assert(s.sequence_trigger == 1 && s.action_index == 0);
    assert(sc64_p4_current_wait_ms(&s) == SC64_P4_RESULT_WAIT_MS);
    assert(sc64_p4_complete_action(&s, 1) == 0);
    assert(s.action_index == 0);
    for (i = 1; i <= 6; ++i) {
        u16 event = sc64_p4_complete_action(&s, 0);
        assert(event & (i == 6 ? SC64_P4_EVENT_LOST : SC64_P4_EVENT_ACTION));
        if (i < 6) assert(s.action_index == i);
        if (i == 4) assert(sc64_p4_current_wait_ms(&s) == SC64_P4_RESULT_WAIT_MS);
    }
    assert(s.outcome == SC64_P4_LOST && sc64_p4_state_valid(&s));
    assert(sc64_p4_complete_action(&s, 0) == 0);

    /* The second character loss uses a separate source record/bit. */
    live_input(&in);
    sc64_p4_init(&s);
    in.all_tassadars = 0;
    assert(sc64_p4_step(&s, &in) & SC64_P4_EVENT_TRIGGER_2);
    assert(s.sequence_trigger == 2 && sc64_p4_state_valid(&s));
    assert(SC64_P4_LOSS_AUDIO_DURATION == 403);
}

static void test_escape_requires_both_actors_at_location_one(void)
{
    Sc64P4State s;
    Sc64P4Input in;
    unsigned i;
    live_input(&in);
    sc64_p4_init(&s);
    in.escape_ray_vultures = 1;
    assert(sc64_p4_step(&s, &in) == 0);
    in.escape_tassadars = 1;
    assert(sc64_p4_step(&s, &in) & SC64_P4_EVENT_TRIGGER_4);
    assert(s.sequence_trigger == 4 && sc64_p4_current_wait_ms(&s) == 0);
    for (i = 1; i <= 8; ++i) {
        u16 event = sc64_p4_complete_action(&s, 0);
        assert(event & (i == 8 ? SC64_P4_EVENT_WON : SC64_P4_EVENT_ACTION));
        if (i == 2 || i == 6)
            assert(sc64_p4_current_wait_ms(&s) == SC64_P4_RESULT_WAIT_MS);
    }
    assert(s.outcome == SC64_P4_WON && sc64_p4_state_valid(&s));
    in.current_any_units = 0;
    assert(sc64_p4_step(&s, &in) == 0);
    assert(s.outcome == SC64_P4_WON);
}

static void test_same_sample_result_priority_is_explicit(void)
{
    Sc64P4State s;
    Sc64P4Input in;
    live_input(&in);
    sc64_p4_init(&s);
    in.all_ray_vultures = 0;
    in.switch13_set = 1;
    in.all_tassadars = 0;
    in.escape_ray_vultures = 1;
    in.escape_tassadars = 1;
    assert(sc64_p4_step(&s, &in) & SC64_P4_EVENT_ACTION);
    assert(s.sequence_trigger == 1);
    assert(!(s.fired & SC64_P4_F_TRIGGER_2));
    assert(!(s.fired & SC64_P4_F_TRIGGER_4));
}

static void test_corrupt_persisted_state_fails_closed(void)
{
    Sc64P4State s;
    Sc64P4Input in;
    live_input(&in);
    sc64_p4_init(&s);
    s.fired = 0x8000;
    assert(!sc64_p4_state_valid(&s));
    assert(sc64_p4_step(&s, &in) == 0);
    sc64_p4_init(&s);
    s.sequence_active = 1;
    s.sequence_trigger = SC64_P4_SEQ_ESCAPE;
    s.action_index = 7;
    s.fired = SC64_P4_F_TRIGGER_4;
    assert(sc64_p4_state_valid(&s));
    s.action_index = 8;
    assert(!sc64_p4_state_valid(&s));
    s.action_index = 7;
    s.fired |= SC64_P4_F_TRIGGER_1;
    assert(!sc64_p4_state_valid(&s));
}

int main(void)
{
    test_raw_identity_and_profile_gaps();
    test_startup_and_all_nonresult_trigger_gates();
    test_survival_loss_pause_and_stickiness();
    test_hero_survival_loss_sequences_keep_raw_wait_facts();
    test_escape_requires_both_actors_at_location_one();
    test_same_sample_result_priority_is_explicit();
    test_corrupt_persisted_state_fails_closed();
    return 0;
}
