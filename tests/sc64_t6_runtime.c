/* Internal T6 trigger adapter regression over the authenticated numeric
 * scenario. Fake world callbacks exercise trigger behavior, not gameplay. */
#include <assert.h>
#include <string.h>
#include "sc64_t6_scenario_adapter.h"

enum {
    TEST_NEUTRAL = 0,
    TEST_WIN,
    TEST_LOSS_NORAD,
    TEST_REINFORCEMENT,
    TEST_LOSS_RAYNOR,
    TEST_EMPTY_HUMAN,
    TEST_ONE_DROPSHIP
};

typedef struct {
    unsigned mode;
    unsigned create_calls, successful_creates;
    unsigned resource_calls, ai_calls, presentation_calls;
    unsigned retry_create_once;
    u32 properties_created;
    unsigned resource_by_owner[8], ai_by_owner[8];
    unsigned create_by_owner[8];
} FakeWorld;

static u32
fake_count_for(const FakeWorld *world, const Sc64T4Condition *c,
               unsigned owner)
{
    switch (c->opcode) {
    case 2: /* Command */
        if (c->unit == 229) return world->mode == TEST_EMPTY_HUMAN ? 0 : 1;
        if (c->unit == 126) return world->mode == TEST_LOSS_NORAD ? 0 : 1;
        if (c->unit == 19)
            return world->mode == TEST_LOSS_RAYNOR ? 0 : 1;
        return 0;
    case 3: /* Bring */
        if (c->unit == 19)
            return (owner == 0 && (world->mode == TEST_WIN ||
                    world->mode == TEST_ONE_DROPSHIP)) ? 1 : 0;
        if (c->unit == 11 && owner == 0)
            return world->mode == TEST_WIN ? 2 :
                   world->mode == TEST_ONE_DROPSHIP ? 1 : 0;
        if (c->unit == 229)
            return (owner == 0 && world->mode == TEST_REINFORCEMENT &&
                    c->location == 2) ? 1 : 0;
        if (c->unit == 106) return 0;
        if (c->unit == 122) return 0;
        return 0;
    case 15: /* Deaths, forwarded without changing its typed condition. */
        return world->mode == TEST_LOSS_NORAD ||
               world->mode == TEST_LOSS_RAYNOR ||
               world->mode == TEST_EMPTY_HUMAN ? 0 : 1;
    default: assert(!"unexpected typed condition"); return 0;
    }
}

static int
fake_count(void *context, const Sc64T4Condition *c, unsigned owner, u32 *count)
{
    FakeWorld *world = context;
    assert(world && c && count && owner < 8);
    *count = fake_count_for(world, c, owner);
    return 1;
}

static int
fake_create(void *context, const Sc64T4Action *a, unsigned owner)
{
    FakeWorld *world = context;
    assert(world && a && owner == 0 && a->opcode == 11);
    assert(a->argument >= 1 && a->argument <= 30);
    ++world->create_calls;
    ++world->create_by_owner[owner];
    if (world->retry_create_once) {
        world->retry_create_once = 0;
        return 0;
    }
    u32 bit = (u32)1 << (a->argument - 1);
    assert(!(world->properties_created & bit));
    world->properties_created |= bit;
    ++world->successful_creates;
    return 1;
}

static int
fake_resources(void *context, const Sc64T4Action *a, unsigned owner)
{
    FakeWorld *world = context;
    assert(world && a && a->opcode == 26 && owner < 8);
    ++world->resource_calls;
    ++world->resource_by_owner[owner];
    return 1;
}

static int
fake_ai(void *context, const Sc64T4Action *a, unsigned owner)
{
    FakeWorld *world = context;
    assert(world && a && (a->opcode == 15 || a->opcode == 16) && owner < 8);
    ++world->ai_calls;
    ++world->ai_by_owner[owner];
    return 1;
}

static int
fake_presentation(void *context, const Sc64T4Action *a, unsigned owner)
{
    FakeWorld *world = context;
    assert(world && a && owner < 8);
    assert(a->opcode == 7 || a->opcode == 8 || a->opcode == 9 ||
           a->opcode == 10 || a->opcode == 12);
    ++world->presentation_calls;
    return 1;
}

static const Sc64T6RuntimeOps fake_ops = {
    fake_count, fake_create, fake_resources, fake_ai, fake_presentation
};

static void
test_source_program_and_context_expansion(void)
{
    Sc64T6RuntimeState state;
    assert(sc64_t6_scenario_source_valid());
    assert(sc64_t6_runtime_init(&state, &sc64_t6_scenario,
                                &sc64_t6_source_setup));
    assert(sc64_t6_runtime_state_valid(&state, &sc64_t6_scenario,
                                       &sc64_t6_source_setup));
    unsigned enabled_pairs = 0;
    for (unsigned owner = 0; owner < 8; ++owner) {
        const Sc64T4TriggerState *p = &state.player[owner];
        assert(p->owner == owner);
        assert(p->force == sc64_t6_source_setup.force_by_slot[owner] + 1);
        for (unsigned t = 0; t < 20; ++t)
            enabled_pairs += sc64_t4_trigger_owner_enabled(
                &sc64_t6_scenario.triggers[t], owner, p->force);
    }
    assert(enabled_pairs == 24);
    assert(sc64_t4_trigger_owner_enabled(&sc64_t6_scenario.triggers[7], 3, 4));
    assert(sc64_t4_trigger_owner_enabled(&sc64_t6_scenario.triggers[7], 5, 4));
    assert(sc64_t4_trigger_owner_enabled(&sc64_t6_scenario.triggers[7], 7, 4));
    /* Set the source AI-start gate in this rules fixture. Inactive controller
     * slots must execute their Force4 instances rather than being filtered. */
    FakeWorld world = {.mode = TEST_NEUTRAL};
    state.switches.bits[10 >> 3] |= 1u << (10 & 7);
    assert(sc64_t6_scenario_runtime_step(&state, &fake_ops, &world, 0));
    const unsigned force4[] = {1, 3, 5, 7};
    for (unsigned i = 0; i < sizeof(force4) / sizeof(force4[0]); ++i) {
        unsigned owner = force4[i];
        assert(world.resource_by_owner[owner] == 1 && world.ai_by_owner[owner] == 1);
        assert(state.player[owner].flags[7] == SC64_T4_TR_DONE);
    }
    assert(world.ai_by_owner[2] == 1 && world.ai_by_owner[6] == 1);
}

static void
test_pause_wait_and_terminal_outcomes(void)
{
    Sc64T6RuntimeState state, before;
    FakeWorld world = {.mode = TEST_WIN};
    assert(sc64_t6_scenario_runtime_init(&state));
    before = state;
    assert(sc64_t6_runtime_step(&state, &sc64_t6_scenario,
                                &sc64_t6_source_setup, &fake_ops, &world, 1));
    assert(memcmp(&state, &before, sizeof(state)) == 0);
    assert(world.presentation_calls == 0 && world.resource_calls == 0);
    for (unsigned tick = 0; tick < 80 && !state.player[0].outcome; ++tick)
        assert(sc64_t6_runtime_step(&state, &sc64_t6_scenario,
                                    &sc64_t6_source_setup, &fake_ops,
                                    &world, 0));
    assert(state.player[0].outcome == SC64_T4_TR_WON);
    assert(state.player[0].wait_ticks == 0);
    assert(world.presentation_calls >= 12);
    assert(world.resource_calls == 6); /* Direct slot, Force 3, four Force 4 slots. */
    assert(sc64_t6_runtime_state_valid(&state, &sc64_t6_scenario,
                                       &sc64_t6_source_setup));

    assert(sc64_t6_scenario_runtime_init(&state));
    memset(&world, 0, sizeof(world)); world.mode = TEST_LOSS_NORAD;
    for (unsigned tick = 0; tick < 80 && !state.player[0].outcome; ++tick)
        assert(sc64_t6_runtime_step(&state, &sc64_t6_scenario,
                                    &sc64_t6_source_setup, &fake_ops,
                                    &world, 0));
    assert(state.player[0].outcome == SC64_T4_TR_LOST);
    assert(world.presentation_calls >= 2);

    /* The distinct Raynor-loss condition also terminates the human context. */
    assert(sc64_t6_scenario_runtime_init(&state));
    memset(&world, 0, sizeof(world)); world.mode = TEST_LOSS_RAYNOR;
    for (unsigned tick = 0; tick < 80 && !state.player[0].outcome; ++tick)
        assert(sc64_t6_runtime_step(&state, &sc64_t6_scenario,
                                    &sc64_t6_source_setup, &fake_ops,
                                    &world, 0));
    assert(state.player[0].outcome == SC64_T4_TR_LOST);

    /* Source AnyUnit==0 defeat is immediate, before the delayed losses. */
    assert(sc64_t6_scenario_runtime_init(&state));
    memset(&world, 0, sizeof(world)); world.mode = TEST_EMPTY_HUMAN;
    assert(sc64_t6_runtime_step(&state, &sc64_t6_scenario,
                                &sc64_t6_source_setup, &fake_ops, &world, 0));
    assert(state.player[0].outcome == SC64_T4_TR_LOST);
}

static void
test_checked_and_prevalidated_execution_match(void)
{
    Sc64T6RuntimeState checked, fast;
    FakeWorld checked_world = {.mode = TEST_WIN};
    FakeWorld fast_world = {.mode = TEST_WIN};
    assert(sc64_t6_scenario_runtime_init(&checked));
    fast = checked;
    for (unsigned tick = 0; tick < 70; ++tick) {
        assert(sc64_t6_runtime_step(&checked, &sc64_t6_scenario,
                    &sc64_t6_source_setup, &fake_ops, &checked_world, 0));
        assert(sc64_t6_runtime_step_prevalidated(&fast, &sc64_t6_scenario,
                    &sc64_t6_source_setup, &fake_ops, &fast_world, 0));
        assert(memcmp(&checked, &fast, sizeof(checked)) == 0);
        assert(memcmp(&checked_world, &fast_world, sizeof(checked_world)) == 0);
    }
}

static void
test_wait_and_atomic_reinforcement_retry(void)
{
    Sc64T6RuntimeState state;
    FakeWorld world = {.mode = TEST_REINFORCEMENT};
    world.retry_create_once = 1;
    assert(sc64_t6_scenario_runtime_init(&state));
    for (unsigned tick = 0; tick < 900 &&
         !(state.player[0].wait_ticks && state.player[0].active_wait == 10); ++tick) {
        assert(sc64_t6_runtime_step(&state, &sc64_t6_scenario,
                                    &sc64_t6_source_setup, &fake_ops,
                                    &world, 0));
    }
    assert(state.player[0].wait_ticks && state.player[0].active_wait == 10);
    assert(!world.successful_creates);
    /* Snapshot an actually pending reinforcement wait, not its completed
     * state. This is an in-memory continuation fixture, not SRAM validation. */
    Sc64T6RuntimeState resumed = state;
    FakeWorld resumed_world = world;
    for (unsigned tick = 0; tick < 900; ++tick) {
        assert(sc64_t6_runtime_step(&state, &sc64_t6_scenario,
                                    &sc64_t6_source_setup, &fake_ops,
                                    &world, 0));
        assert(sc64_t6_runtime_step_prevalidated(&resumed, &sc64_t6_scenario,
                                    &sc64_t6_source_setup, &fake_ops,
                                    &resumed_world, 0));
        assert(memcmp(&state, &resumed, sizeof(state)) == 0);
        assert(memcmp(&world, &resumed_world, sizeof(world)) == 0);
    }
    assert(world.create_calls == 6 && world.successful_creates == 5);
    assert(state.player[0].flags[10] & SC64_T4_TR_DONE);
    assert(sc64_t6_runtime_state_valid(&state, &sc64_t6_scenario,
                                       &sc64_t6_source_setup));
}

static void
test_two_dropship_gate_and_one_shot_reinforcements(void)
{
    Sc64T6RuntimeState state;
    FakeWorld world = {.mode = TEST_ONE_DROPSHIP};
    assert(sc64_t6_scenario_runtime_init(&state));
    for (unsigned tick = 0; tick < 180; ++tick)
        assert(sc64_t6_runtime_step(&state, &sc64_t6_scenario,
                                    &sc64_t6_source_setup, &fake_ops,
                                    &world, 0));
    assert(state.player[0].outcome != SC64_T4_TR_WON);

    /* Test fixture advances the source clock to make all timed groups ready;
     * this is trigger-engine coverage, not a claim about a live mission. */
    assert(sc64_t6_scenario_runtime_init(&state));
    memset(&world, 0, sizeof(world)); world.mode = TEST_REINFORCEMENT;
    for (unsigned owner = 0; owner < 8; ++owner)
        state.player[owner].elapsed_ticks = 300000;
    state.switches.bits[0] |= 1u << 2;
    for (unsigned tick = 0; tick < 1200; ++tick)
        assert(sc64_t6_runtime_step(&state, &sc64_t6_scenario,
                                    &sc64_t6_source_setup, &fake_ops,
                                    &world, 0));
    assert(world.successful_creates == 30);
    assert(world.properties_created == (((u32)1 << 30) - 1));
    for (unsigned trigger = 10; trigger < 20; ++trigger) {
        if (trigger != 10 && trigger < 15) continue;
        unsigned enabled = 0;
        for (unsigned owner = 0; owner < 8; ++owner) {
            if (!sc64_t4_trigger_owner_enabled(&sc64_t6_scenario.triggers[trigger],
                    owner, state.player[owner].force)) continue;
            ++enabled;
            assert(state.player[owner].flags[trigger] & SC64_T4_TR_DONE);
        }
        assert(enabled);
    }
    unsigned calls = world.create_calls;
    for (unsigned tick = 0; tick < 90; ++tick)
        assert(sc64_t6_runtime_step(&state, &sc64_t6_scenario,
                                    &sc64_t6_source_setup, &fake_ops,
                                    &world, 0));
    assert(world.create_calls == calls); /* completed source groups do not replay */
}

static void
test_malformed_program_and_state_rejected(void)
{
    Sc64T6RuntimeState state;
    Sc64T4Scenario bad = sc64_t6_scenario;
    Sc64T4Trigger triggers[20];
    Sc64T4Action actions[20 * 64];
    memcpy(triggers, sc64_t6_triggers, sizeof(triggers));
    memcpy(actions, sc64_t6_actions, sizeof(actions));
    triggers[0].actions = actions;
    actions[0].opcode = 99;
    bad.triggers = triggers;
    assert(!sc64_t6_runtime_program_valid(&bad, &sc64_t6_source_setup));
    assert(!sc64_t6_runtime_init(&state, &bad, &sc64_t6_source_setup));

    assert(sc64_t6_scenario_runtime_init(&state));
    state.player[7].force = 1;
    assert(!sc64_t6_runtime_state_valid(&state, &sc64_t6_scenario,
                                        &sc64_t6_source_setup));
    assert(!sc64_t6_runtime_step(&state, &sc64_t6_scenario,
                                 &sc64_t6_source_setup, &fake_ops, 0, 0));
}

int main(void)
{
    test_source_program_and_context_expansion();
    test_pause_wait_and_terminal_outcomes();
    test_checked_and_prevalidated_execution_match();
    test_wait_and_atomic_reinforcement_retry();
    test_two_dropship_gate_and_one_shot_reinforcements();
    test_malformed_program_and_state_rejected();
    return 0;
}
