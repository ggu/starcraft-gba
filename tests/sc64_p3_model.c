/* Asset-free Higher Ground trigger state model tests. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/sc64_p3_model.h"

static Sc64P3Input input_safe(void)
{
    Sc64P3Input in;
    memset(&in, 0, sizeof(in));
    in.human_any_units = 1;
    in.foes_buildings = 3; /* Aggregate across all source Foes members. */
    return in;
}

static void test_source_facts_and_start_one_shot(void)
{
    Sc64P3State s;
    assert(SC64_P3_HUMAN_MINERALS == 300 && SC64_P3_HUMAN_GAS == 100);
    assert(SC64_P3_FORCE2_COMBINED_RESOURCES == 5000);
    assert(SC64_P3_FORCE3_COMBINED_RESOURCES == 8000);
    assert(SC64_P3_FOES_ZERO_BUILDINGS_WAIT_MS == 2000);
    assert(SC64_P3_FOES_ZERO_BUILDINGS_POST_TX_WAIT_MS == 3000);
    assert(SC64_P3_FOES_ZERO_BUILDINGS_TRANSMISSIONS == 2);
    assert(SC64_P3_RAW_ANY_UNIT == 229 && SC64_P3_RAW_BUILDINGS == 231);
    assert(SC64_P3_CONDITION_COMMAND == 2 && SC64_P3_CONDITION_ELAPSED_TIME == 12);
    assert(SC64_P3_ACTION_DEFEAT == 2 && SC64_P3_ACTION_WAIT == 4 &&
           SC64_P3_ACTION_TRANSMISSION == 7 && SC64_P3_ACTION_VICTORY == 1 &&
           SC64_P3_ACTION_SET_RESOURCES == 26);
    assert(sc64_p3_source_conditions[0].player_group ==
               SC64_P3_TRIGGER_CURRENT_PLAYER &&
           sc64_p3_source_conditions[0].unit_type == SC64_P3_RAW_ANY_UNIT &&
           sc64_p3_source_conditions[0].amount == 0 &&
           sc64_p3_source_conditions[0].comparison == SC64_P3_CONDITION_AT_MOST);
    assert(sc64_p3_source_conditions[2].player_group == SC64_P3_TRIGGER_FOES &&
           sc64_p3_source_conditions[2].unit_type == SC64_P3_RAW_BUILDINGS &&
           sc64_p3_source_conditions[2].amount == 0);
    assert(sc64_p3_source_conditions[3].player_group == SC64_P3_TRIGGER_FORCE3 &&
           sc64_p3_source_conditions[3].unit_type == SC64_P3_RAW_ANY_UNIT &&
           sc64_p3_source_conditions[3].amount == 1 &&
           sc64_p3_source_conditions[4].player_group == SC64_P3_TRIGGER_FORCE2 &&
           sc64_p3_source_conditions[4].unit_type == SC64_P3_RAW_ANY_UNIT &&
           sc64_p3_source_conditions[4].amount == 1);
    assert(sc64_p3_model_init(&s) == SC64_P3_EVENT_MISSION_START);
    assert(sc64_p3_model_state_valid(&s));
    assert(s.fired == SC64_P3_F_MISSION_START);
}

static void test_force2_and_force3_rewards_are_independent_one_shots(void)
{
    Sc64P3State s;
    Sc64P3Input in = input_safe();
    sc64_p3_model_init(&s);
    assert(sc64_p3_model_step(&s, &in) == SC64_P3_EVENT_NONE);
    assert(!(s.fired & (SC64_P3_F_FORCE2_START | SC64_P3_F_FORCE3_START)));

    in.force2_any_units = 1;
    assert(sc64_p3_model_step(&s, &in) == SC64_P3_EVENT_FORCE2_START);
    assert(s.fired & SC64_P3_F_FORCE2_START);
    assert(sc64_p3_model_step(&s, &in) == SC64_P3_EVENT_NONE);
    in.force2_any_units = 0;
    in.force3_any_units = 1;
    assert(sc64_p3_model_step(&s, &in) == SC64_P3_EVENT_FORCE3_START);
    assert(s.fired & SC64_P3_F_FORCE3_START);
    assert(sc64_p3_model_step(&s, &in) == SC64_P3_EVENT_NONE);

    assert(sc64_p3_source_force2_actions[0].group1 == SC64_P3_TRIGGER_FORCE2);
    assert(sc64_p3_source_force2_actions[0].group2 ==
           SC64_P3_FORCE2_COMBINED_RESOURCES);
    assert(sc64_p3_source_force3_actions[0].group1 == SC64_P3_TRIGGER_FORCE3);
    assert(sc64_p3_source_force3_actions[0].group2 ==
           SC64_P3_FORCE3_COMBINED_RESOURCES);
    assert(sc64_p3_source_rewards[0].group2 == SC64_P3_HUMAN_MINERALS);
    assert(sc64_p3_source_rewards[1].group2 == SC64_P3_HUMAN_GAS);
}

static void test_pause_freezes_independent_triggers_and_pending_sequence(void)
{
    Sc64P3State s, before;
    Sc64P3Input in = input_safe();
    sc64_p3_model_init(&s);
    in.paused = 1;
    in.force2_any_units = 1;
    in.force3_any_units = 1;
    in.foes_buildings = 0;
    assert(sc64_p3_model_step(&s, &in) == SC64_P3_EVENT_NONE);
    assert(s.fired == SC64_P3_F_MISSION_START && s.sequence == SC64_P3_SEQ_NONE);

    in.paused = 0;
    assert(sc64_p3_model_step(&s, &in) ==
           (SC64_P3_EVENT_FORCE2_START | SC64_P3_EVENT_FORCE3_START |
            SC64_P3_EVENT_WIN_WAIT));
    before = s;
    in.paused = 1;
    in.complete_action = 1;
    assert(sc64_p3_model_step(&s, &in) == SC64_P3_EVENT_NONE);
    assert(memcmp(&s, &before, sizeof(s)) == 0);
}

static void test_foes_buildings_aggregate_and_win_actions_are_ordered(void)
{
    Sc64P3State s, restored;
    Sc64P3Input in = input_safe();
    sc64_p3_model_init(&s);
    in.foes_buildings = 2; /* Both forces still contribute to Foes. */
    in.force2_any_units = 1;
    assert(sc64_p3_model_step(&s, &in) == SC64_P3_EVENT_FORCE2_START);
    assert(s.sequence == SC64_P3_SEQ_NONE && s.outcome == SC64_P3_RUNNING);
    in.foes_buildings = 0;
    assert(sc64_p3_model_step(&s, &in) == SC64_P3_EVENT_WIN_WAIT);
    assert(s.sequence == SC64_P3_SEQ_WIN_WAIT_2000);
    assert(sc64_p3_source_foes_zero_actions[0].time_ms == 2000);
    assert(sc64_p3_source_foes_zero_actions[1].location == 64 &&
           sc64_p3_source_foes_zero_actions[1].unit_type == 77);
    assert(sc64_p3_source_foes_zero_actions[2].location == 1 &&
           sc64_p3_source_foes_zero_actions[2].unit_type == 87);
    assert(sc64_p3_source_foes_zero_actions[3].time_ms == 3000);

    /* Serialized adapter policy: every sequence action waits for explicit
     * completion. No millisecond-to-30Hz conversion is asserted here. */
    restored = s; /* Save/restore midway through a pending action. */
    assert(sc64_p3_model_state_valid(&restored));
    assert(sc64_p3_model_step(&restored, &in) == SC64_P3_EVENT_NONE);
    in.complete_action = 1;
    assert(sc64_p3_model_step(&restored, &in) == SC64_P3_EVENT_WIN_TRANSMISSION);
    assert(restored.sequence == SC64_P3_SEQ_WIN_TX_1 && restored.transmission == 1);
    in.complete_action = 0;
    assert(sc64_p3_model_step(&restored, &in) == SC64_P3_EVENT_NONE);
    in.complete_action = 1;
    assert(sc64_p3_model_step(&restored, &in) == SC64_P3_EVENT_WIN_TRANSMISSION);
    assert(restored.sequence == SC64_P3_SEQ_WIN_TX_2 && restored.transmission == 2);
    assert(sc64_p3_model_step(&restored, &in) == SC64_P3_EVENT_WIN_POST_WAIT);
    assert(restored.sequence == SC64_P3_SEQ_WIN_WAIT_3000);
    assert(sc64_p3_model_step(&restored, &in) == SC64_P3_EVENT_WON);
    assert(restored.outcome == SC64_P3_WON &&
           restored.sequence == SC64_P3_SEQ_DONE_WIN);
    assert(sc64_p3_model_state_valid(&restored));
    assert(sc64_p3_model_step(&restored, &in) == SC64_P3_EVENT_NONE);
}

static void test_human_anyunit_loss_and_explicit_simultaneous_priority(void)
{
    Sc64P3State s;
    Sc64P3Input in = input_safe();
    sc64_p3_model_init(&s);
    in.human_any_units = 0; /* Units plus buildings are aggregated by adapter. */
    in.foes_buildings = 0;
    in.force3_any_units = 1;
    assert(sc64_p3_model_step(&s, &in) ==
           (SC64_P3_EVENT_FORCE3_START | SC64_P3_EVENT_LOST));
    assert(s.outcome == SC64_P3_LOST && s.sequence == SC64_P3_SEQ_DONE_LOSS);
    assert(sc64_p3_model_state_valid(&s));

    /* Adapter policy chooses defeat on a same-sample conflict; it does not
     * claim the separate source trigger records establish that ordering. */
    sc64_p3_model_init(&s);
    in.human_any_units = 1;
    in.force3_any_units = 0;
    assert(sc64_p3_model_step(&s, &in) == SC64_P3_EVENT_WIN_WAIT);
    in.human_any_units = 0;
    assert(sc64_p3_model_step(&s, &in) == SC64_P3_EVENT_LOST);
    assert(s.outcome == SC64_P3_LOST && sc64_p3_model_state_valid(&s));
}

static void test_state_validator_rejects_noncanonical_and_unreachable_saves(void)
{
    Sc64P3State s;
    sc64_p3_model_init(&s);
    s.reserved = 1;
    assert(!sc64_p3_model_state_valid(&s));

    sc64_p3_model_init(&s);
    s.fired |= 1u << 15;
    assert(!sc64_p3_model_state_valid(&s));

    sc64_p3_model_init(&s);
    s.sequence = SC64_P3_SEQ_WIN_TX_1;
    s.fired |= SC64_P3_F_WIN_SEQUENCE;
    assert(!sc64_p3_model_state_valid(&s)); /* TX index absent. */

    sc64_p3_model_init(&s);
    s.sequence = SC64_P3_SEQ_DONE_WIN;
    s.outcome = SC64_P3_WON;
    s.fired |= SC64_P3_F_RESULT;
    assert(!sc64_p3_model_state_valid(&s)); /* No source win sequence. */

    sc64_p3_model_init(&s);
    s.sequence = SC64_P3_SEQ_DONE_LOSS;
    s.outcome = SC64_P3_LOST;
    s.fired |= SC64_P3_F_RESULT;
    s.transmission = 1;
    assert(!sc64_p3_model_state_valid(&s)); /* Terminal state cannot keep TX active. */
}

int main(void)
{
    test_source_facts_and_start_one_shot();
    test_force2_and_force3_rewards_are_independent_one_shots();
    test_pause_freezes_independent_triggers_and_pending_sequence();
    test_foes_buildings_aggregate_and_win_actions_are_ordered();
    test_human_anyunit_loss_and_explicit_simultaneous_priority();
    test_state_validator_rejects_noncanonical_and_unreachable_saves();
    puts("Higher Ground model tests passed");
    return 0;
}
