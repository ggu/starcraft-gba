/* Asset-free tests of the bounded trigger model for Terran 3. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/sc64_t3.h"

static Sc64T3Input
safe_input(void)
{
    Sc64T3Input in;
    memset(&in, 0, sizeof(in));
    in.human_buildings = 1;
    in.force2_hatcheries = 1;
    return in;
}

static void
test_start_pause_and_one_shot_ai(void)
{
    Sc64T3State s;
    Sc64T3Input in = safe_input();
    u16 events = sc64_t3_init(&s);
    assert(events == SC64_T3_EVENT_INTRO);
    assert(s.remaining_ticks == 54000 && sc64_t3_state_valid(&s));
    in.paused = 1;
    in.force2_any_units = 1;
    assert(sc64_t3_step(&s, &in) == 0 && s.remaining_ticks == 54000);
    in.paused = 0;
    events = sc64_t3_step(&s, &in);
    assert(events == SC64_T3_EVENT_FORCE2_AI_AND_RESOURCES);
    assert(sc64_t3_step(&s, &in) == 0);
}

static void
test_alternate_hatchery_gate(void)
{
    Sc64T3State s;
    Sc64T3Input in = safe_input();
    sc64_t3_init(&s);
    in.force1_buildings_killed = 1;
    assert(sc64_t3_step(&s, &in) == SC64_T3_EVENT_ALTERNATE_OBJECTIVE);
    assert(s.objective == 1 && sc64_t3_state_valid(&s));
    assert(sc64_t3_step(&s, &in) == 0); /* objective and its briefing fire once */
    in.force2_hatcheries = 0;
    assert(sc64_t3_step(&s, &in) == SC64_T3_EVENT_VICTORY);
    assert(s.outcome == SC64_T3_WON && sc64_t3_state_valid(&s));
    assert(sc64_t3_step(&s, &in) == 0);
}

static void
test_time_gate_boundary(void)
{
    Sc64T3State s;
    Sc64T3Input in = safe_input();
    sc64_t3_init(&s);
    /* At 120 seconds remaining, CHK timer comparison 0 (at least) still holds
     * after one 30 Hz tick; the integer countdown's displayed second is 120. */
    s.elapsed_ticks = SC64_T3_DURATION_TICKS - 120u * 30u;
    s.remaining_ticks = 120u * 30u;
    s.fired |= SC64_T3_F_10_MIN | SC64_T3_F_5_MIN;
    in.force1_buildings_killed = 1;
    assert(sc64_t3_state_valid(&s));
    assert(sc64_t3_step(&s, &in) ==
           (SC64_T3_EVENT_ALTERNATE_OBJECTIVE | SC64_T3_EVENT_AI_START));

    sc64_t3_init(&s);
    s.elapsed_ticks = SC64_T3_DURATION_TICKS - 120u * 30u + 30u;
    s.remaining_ticks = 120u * 30u - 30u;
    s.fired |= SC64_T3_F_10_MIN | SC64_T3_F_5_MIN;
    assert(sc64_t3_state_valid(&s));
    assert(sc64_t3_step(&s, &in) == SC64_T3_EVENT_AI_START);
    assert(s.objective == 0);
}

static void
test_loss_precedes_same_tick_win(void)
{
    Sc64T3State s;
    Sc64T3Input in = safe_input();
    sc64_t3_init(&s);
    in.force1_buildings_killed = 1;
    assert(sc64_t3_step(&s, &in) == SC64_T3_EVENT_ALTERNATE_OBJECTIVE);
    in.human_buildings = 0;
    in.force2_hatcheries = 0;
    assert(sc64_t3_step(&s, &in) == SC64_T3_EVENT_DEFEAT);
    assert(s.outcome == SC64_T3_LOST && sc64_t3_state_valid(&s));
}

static void
test_reminders_and_save_continuity(void)
{
    Sc64T3State s, loaded;
    Sc64T3Input in = safe_input();
    u32 i;
    sc64_t3_init(&s);
    for (i = 0; i < (1800u - 600u) * 30u - 1u; ++i) sc64_t3_step(&s, &in);
    assert(s.remaining_ticks == 600u * 30u + 1u);
    assert(sc64_t3_step(&s, &in) == SC64_T3_EVENT_REMINDER_10_MIN);
    assert(sc64_t3_step(&s, &in) == 0);
    loaded = s; /* Mirrors the compact state copied through SRAM. */
    assert(sc64_t3_state_valid(&loaded));
    s = loaded;
    for (i = 0; i < (600u - 300u) * 30u - 2u; ++i) sc64_t3_step(&s, &in);
    assert(sc64_t3_step(&s, &in) == SC64_T3_EVENT_REMINDER_5_MIN);
    for (i = 0; i < (300u - 60u) * 30u - 1u; ++i) sc64_t3_step(&s, &in);
    assert(sc64_t3_step(&s, &in) == SC64_T3_EVENT_REMINDER_1_MIN);
}

static void
test_survive_full_countdown_and_invalid_saves(void)
{
    Sc64T3State s;
    Sc64T3Input in = safe_input();
    u32 i;
    sc64_t3_init(&s);
    for (i = 0; i < SC64_T3_DURATION_TICKS - 2u; ++i) {
        u16 events = sc64_t3_step(&s, &in);
        assert(!(events & SC64_T3_EVENT_VICTORY));
    }
    assert(s.remaining_ticks == 2 && s.outcome == SC64_T3_RUNNING);
    assert(!(sc64_t3_step(&s, &in) & SC64_T3_EVENT_VICTORY));
    assert(s.remaining_ticks == 1 && s.outcome == SC64_T3_RUNNING);
    assert(sc64_t3_step(&s, &in) == SC64_T3_EVENT_VICTORY);
    assert(s.elapsed_ticks == SC64_T3_DURATION_TICKS &&
           s.remaining_ticks == 0 && s.outcome == SC64_T3_WON);
    assert(sc64_t3_state_valid(&s));
    ++s.remaining_ticks;
    assert(!sc64_t3_state_valid(&s));
}

static void
test_survival_switch_is_independent_of_optional_objective(void)
{
    Sc64T3State s;
    Sc64T3Input in = safe_input();
    u32 i;
    sc64_t3_init(&s);
    assert((s.fired & SC64_T3_F_SWITCH_0) != 0);
    assert((s.fired & SC64_T3_F_SWITCH_1) == 0);
    /* No Force 1 building kill: switch #1 and the alternate objective remain
     * off, but the source's switch #0 countdown-survival branch still wins. */
    in.force1_buildings_killed = 0;
    for (i = 0; i < SC64_T3_DURATION_TICKS - 1u; ++i)
        assert(!(sc64_t3_step(&s, &in) & SC64_T3_EVENT_VICTORY));
    assert(sc64_t3_step(&s, &in) == SC64_T3_EVENT_VICTORY);
    assert(s.outcome == SC64_T3_WON && s.objective == 0);
    assert(!(s.fired & SC64_T3_F_SWITCH_1) && sc64_t3_state_valid(&s));
}

static void
test_save_validation_enforces_objective_switch_and_early_win_gate(void)
{
    Sc64T3State s;
    sc64_t3_init(&s);

    /* A won save with time remaining is only valid after TRIG 3 set switch 1.
     * The initially set survival switch 0 cannot authorize an early win. */
    s.outcome = SC64_T3_WON;
    s.fired |= SC64_T3_F_RESULT;
    assert(!sc64_t3_state_valid(&s));

    /* Objective presentation, its one-shot bit, and switch 1 must agree in
     * both directions; reject each corrupt save independently. */
    s.outcome = SC64_T3_RUNNING;
    s.fired &= (u16)~SC64_T3_F_RESULT;
    s.objective = 1;
    assert(!sc64_t3_state_valid(&s));
    s.objective = 0;
    s.fired |= SC64_T3_F_ALTERNATE;
    assert(!sc64_t3_state_valid(&s));
    s.fired &= (u16)~SC64_T3_F_ALTERNATE;
    s.fired |= SC64_T3_F_SWITCH_1;
    assert(!sc64_t3_state_valid(&s));
    s.fired |= SC64_T3_F_ALTERNATE;
    s.objective = 1;
    assert(sc64_t3_state_valid(&s));
}

static void
test_ai_start_uses_authored_120_second_threshold(void)
{
    Sc64T3State s;
    Sc64T3Input in = safe_input();
    u32 i;
    sc64_t3_init(&s);
    for (i = 0; i < (1800u - 120u) * 30u - 1u; ++i)
        (void)sc64_t3_step(&s, &in);
    assert(!(s.fired & SC64_T3_F_AI_START));
    assert(sc64_t3_step(&s, &in) == SC64_T3_EVENT_AI_START);
    assert(s.fired & SC64_T3_F_AI_START);
}

int
main(void)
{
    test_start_pause_and_one_shot_ai();
    test_alternate_hatchery_gate();
    test_time_gate_boundary();
    test_loss_precedes_same_tick_win();
    test_reminders_and_save_continuity();
    test_survive_full_countdown_and_invalid_saves();
    test_survival_switch_is_independent_of_optional_objective();
    test_save_validation_enforces_objective_switch_and_early_win_gate();
    test_ai_start_uses_authored_120_second_threshold();
    puts("T3 trigger rules: ok");
    return 0;
}
