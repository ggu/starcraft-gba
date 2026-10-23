/* Asset-free tests for source-derived Zerg 3 trigger rules. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/sc64_z3.h"

static Sc64Z3Input
safe_input(void)
{
    Sc64Z3Input in;
    memset(&in, 0, sizeof(in));
    in.foes_any_units = 7;
    in.foes_buildings = 1;
    in.force2_any_units = 1;
    in.human_chrysalis = 1;
    return in;
}

static void
test_start_pause_and_force2_one_shot(void)
{
    Sc64Z3State s;
    Sc64Z3Input in = safe_input();
    assert(sc64_z3_init(&s) == SC64_Z3_EVENT_MISSION_START);
    assert(sc64_z3_state_valid(&s));
    assert(SC64_Z3_MISSION_ORE == 150 && SC64_Z3_MISSION_GAS == 50);
    assert(SC64_Z3_FORCE2_RESOURCES == 8000 &&
           SC64_Z3_FORCE2_RESOURCE_SELECTOR_ORE_AND_GAS == 2);
    in.paused = 1;
    assert(sc64_z3_step(&s, &in) == 0);
    assert(!(s.fired & SC64_Z3_F_FORCE2_START));
    in.paused = 0;
    assert(sc64_z3_step(&s, &in) == SC64_Z3_EVENT_FORCE2_START);
    assert(sc64_z3_step(&s, &in) == 0);
    in.force2_any_units = 0;
    assert(sc64_z3_step(&s, &in) == 0); /* condition becoming false doesn't re-fire */
    assert(sc64_z3_state_valid(&s));
}

static void
test_both_source_victory_gates_share_switch_and_sequence(void)
{
    unsigned gate;
    for (gate = 0; gate < 2; ++gate) {
        Sc64Z3State s;
        Sc64Z3Input in = safe_input();
        sc64_z3_init(&s);
        if (gate == 0) in.foes_any_units = 6; /* Foes Command <= 6 Any Unit */
        else in.foes_buildings = 0;          /* Foes Command <= 0 Buildings */
        assert(sc64_z3_step(&s, &in) ==
               (SC64_Z3_EVENT_FORCE2_START | SC64_Z3_EVENT_WIN_WAIT));
        assert(s.switch7 && (s.fired & SC64_Z3_F_SWITCH_7));
        assert(s.sequence == SC64_Z3_SEQ_WIN_WAIT_1000);
        /* Raw Wait 1000 is exposed as phase data, never converted to frames. */
        assert(SC64_Z3_WIN_WAIT_FIRST == 1000);
        assert(SC64_Z3_WIN_TX1_LOCATION == 3 &&
               SC64_Z3_WIN_TX2_LOCATION == 1 &&
               SC64_Z3_WIN_TX3_LOCATION == 2);
        assert(SC64_Z3_WIN_TX_WAV_ID == 25 && SC64_Z3_WIN_TX_TIME_FIELD == 0 &&
               SC64_Z3_WIN_TX_GROUP1 == 0 && SC64_Z3_WIN_TX_GROUP2 == 100 &&
               SC64_Z3_WIN_TX_MODIFIER_FIELD == 9 &&
               SC64_Z3_WIN_TX_FLAGS_FIELD == 16);
        assert(SC64_Z3_WIN_TX_OPCODE == 7 && SC64_Z3_WIN_WAIT_OPCODE == 4 &&
               SC64_Z3_WIN_RESULT_OPCODE == 1 &&
               SC64_Z3_WIN_FIRST_WAIT_FLAGS == 0 &&
               SC64_Z3_WIN_FINAL_WAIT_FLAGS == 4);
        assert(sc64_z3_step(&s, &in) == 0); /* pending action is one-shot */
        assert(sc64_z3_complete_action(&s, 0) == SC64_Z3_EVENT_WIN_TX);
        assert(s.transmission == 1 && SC64_Z3_WIN_TX1_STRING_ID == 13);
        assert(sc64_z3_state_valid(&s));
        assert(sc64_z3_complete_action(&s, 0) == SC64_Z3_EVENT_WIN_TX);
        assert(s.transmission == 2 && SC64_Z3_WIN_TX2_STRING_ID == 18);
        assert(sc64_z3_complete_action(&s, 0) == SC64_Z3_EVENT_WIN_TX);
        assert(s.transmission == 3 && SC64_Z3_WIN_TX3_STRING_ID == 19);
        assert(sc64_z3_complete_action(&s, 0) == SC64_Z3_EVENT_WIN_WAIT);
        assert(s.sequence == SC64_Z3_SEQ_WIN_WAIT_500);
        assert(sc64_z3_complete_action(&s, 0) == SC64_Z3_EVENT_WON);
        assert(s.outcome == SC64_Z3_WON && sc64_z3_state_valid(&s));
        assert(sc64_z3_step(&s, &in) == 0);
        assert(sc64_z3_complete_action(&s, 0) == 0);
    }
}

static void
test_victory_is_not_triggered_by_more_than_six_units_and_buildings(void)
{
    Sc64Z3State s;
    Sc64Z3Input in = safe_input();
    sc64_z3_init(&s);
    in.foes_any_units = 7;
    in.foes_buildings = 1;
    assert(sc64_z3_step(&s, &in) == SC64_Z3_EVENT_FORCE2_START);
    assert(!s.switch7 && s.sequence == SC64_Z3_SEQ_NONE);
    assert(sc64_z3_state_valid(&s));
}

static void
test_chrysalis_loss_action_order_and_one_shot(void)
{
    Sc64Z3State s;
    Sc64Z3Input in = safe_input();
    sc64_z3_init(&s);
    in.human_chrysalis = 0;
    assert(sc64_z3_step(&s, &in) ==
           (SC64_Z3_EVENT_FORCE2_START | SC64_Z3_EVENT_LOSS_WAIT));
    assert(s.sequence == SC64_Z3_SEQ_LOSS_WAIT_1000);
    assert(SC64_Z3_LOSS_WAIT_FIRST == 1000);
    assert(sc64_z3_complete_action(&s, 0) == SC64_Z3_EVENT_LOSS_WAV);
    assert(SC64_Z3_LOSS_WAV_ID == 29 && SC64_Z3_LOSS_WAV_DURATION == 403);
    assert(sc64_z3_complete_action(&s, 0) == SC64_Z3_EVENT_LOSS_MESSAGE);
    assert(SC64_Z3_LOSS_MESSAGE_ID == 23);
    assert(sc64_z3_complete_action(&s, 0) == SC64_Z3_EVENT_LOSS_WAIT);
    assert(SC64_Z3_LOSS_WAIT_LAST == 500);
    assert(sc64_z3_complete_action(&s, 0) == SC64_Z3_EVENT_LOST);
    assert(s.outcome == SC64_Z3_LOST && sc64_z3_state_valid(&s));
    assert(sc64_z3_step(&s, &in) == 0);
}

static void
test_explicit_loss_priority_when_gates_coincide(void)
{
    Sc64Z3State s;
    Sc64Z3Input in = safe_input();
    sc64_z3_init(&s);
    in.foes_any_units = 6;
    in.human_chrysalis = 0;
    assert(sc64_z3_step(&s, &in) ==
           (SC64_Z3_EVENT_FORCE2_START | SC64_Z3_EVENT_LOSS_WAIT));
    assert(s.switch7 && s.sequence == SC64_Z3_SEQ_LOSS_WAIT_1000);
    assert(sc64_z3_state_valid(&s));
}

static void
test_sequence_pauses_and_save_validation(void)
{
    Sc64Z3State s, restored;
    Sc64Z3Input in = safe_input();
    sc64_z3_init(&s);
    in.foes_any_units = 6;
    assert(sc64_z3_step(&s, &in) ==
               (SC64_Z3_EVENT_FORCE2_START | SC64_Z3_EVENT_WIN_WAIT));
    in.paused = 1;
    assert(sc64_z3_step(&s, &in) == 0);
    assert(s.sequence == SC64_Z3_SEQ_WIN_WAIT_1000);
    assert(sc64_z3_complete_action(&s, 1) == 0); /* pause freezes presentation */
    restored = s;
    assert(sc64_z3_state_valid(&restored));
    in.paused = 0;
    assert(sc64_z3_complete_action(&restored, 0) == SC64_Z3_EVENT_WIN_TX);
    assert(sc64_z3_state_valid(&restored));
    restored.switch7 = 0;
    assert(!sc64_z3_state_valid(&restored));
    restored = s;
    restored.transmission = 3;
    assert(!sc64_z3_state_valid(&restored));
    restored = s;
    restored.fired &= (u16)~SC64_Z3_F_SEQUENCE_ACTION;
    assert(!sc64_z3_state_valid(&restored));
    restored = s;
    restored.sequence = SC64_Z3_SEQ_DONE_WIN;
    restored.outcome = SC64_Z3_WON;
    restored.fired |= SC64_Z3_F_RESULT;
    assert(sc64_z3_state_valid(&restored));
    restored.sequence = SC64_Z3_SEQ_DONE_LOSS;
    assert(!sc64_z3_state_valid(&restored));
}

static void
test_adversarial_noncanonical_or_unreachable_saves(void)
{
    Sc64Z3State s;
    sc64_z3_init(&s);

    s.switch7 = 2; /* byte-sized booleans must be canonical */
    assert(!sc64_z3_state_valid(&s));

    sc64_z3_init(&s);
    s.fired |= SC64_Z3_F_SWITCH_7; /* switch byte and fired bit must agree */
    assert(!sc64_z3_state_valid(&s));

    sc64_z3_init(&s);
    s.switch7 = 1;
    s.fired |= SC64_Z3_F_SWITCH_7;
    assert(!sc64_z3_state_valid(&s)); /* a set gate starts a result sequence */

    sc64_z3_init(&s);
    s.sequence = SC64_Z3_SEQ_WIN_TX_1; /* no win gate / sequence marker */
    assert(!sc64_z3_state_valid(&s));

    sc64_z3_init(&s);
    s.switch7 = 1;
    s.fired |= SC64_Z3_F_SWITCH_7 | SC64_Z3_F_SEQUENCE_ACTION;
    s.sequence = SC64_Z3_SEQ_WIN_TX_1;
    s.transmission = 2; /* tx ordinal must match its action cursor */
    assert(!sc64_z3_state_valid(&s));

    sc64_z3_init(&s);
    s.switch7 = 1;
    s.fired |= SC64_Z3_F_SWITCH_7 | SC64_Z3_F_SEQUENCE_ACTION |
               SC64_Z3_F_RESULT;
    s.outcome = SC64_Z3_WON; /* cannot skip the ordered action sequence */
    assert(!sc64_z3_state_valid(&s));

    sc64_z3_init(&s);
    s.fired |= SC64_Z3_F_SEQUENCE_ACTION; /* pending marker with no action/result */
    assert(!sc64_z3_state_valid(&s));

    sc64_z3_init(&s);
    s.outcome = SC64_Z3_LOST;
    s.fired |= SC64_Z3_F_RESULT;
    assert(!sc64_z3_state_valid(&s)); /* terminal result requires a completed sequence */

    sc64_z3_init(&s);
    s.fired &= (u16)~SC64_Z3_F_MISSION_START;
    assert(!sc64_z3_state_valid(&s));
}

int
main(void)
{
    test_start_pause_and_force2_one_shot();
    test_both_source_victory_gates_share_switch_and_sequence();
    test_victory_is_not_triggered_by_more_than_six_units_and_buildings();
    test_chrysalis_loss_action_order_and_one_shot();
    test_explicit_loss_priority_when_gates_coincide();
    test_sequence_pauses_and_save_validation();
    test_adversarial_noncanonical_or_unreachable_saves();
    puts("Z3 trigger rules: ok");
    return 0;
}
