/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stdio.h>

#include "../starcraft/sc64_p8_timed_service.h"

typedef struct {
    Sc64P8TriggerInfo trigger[SC64_P8_TRIGGER_LIMIT];
    Sc64P8SourceAction action[SC64_P8_TRIGGER_LIMIT][4];
    Sc64P8TimedService *timer;
    Sc64P8WorldContext *world;
    unsigned calls, starts, finishes, eligible;
    unsigned trigger_calls[8], slot_calls[8];
    unsigned pause_calls, pause_owner, pause_end_calls, pause_end_trigger;
    int pause_value;
    int eligible_result;
    u32 transmission_ms;
} Fixture;

static int trigger_info(void *opaque, unsigned trigger, Sc64P8TriggerInfo *out)
{
    Fixture *f = opaque;
    if (!f || !out || trigger >= SC64_P8_TRIGGER_LIMIT) return 0;
    *out = f->trigger[trigger];
    return 1;
}

static int condition(void *opaque, unsigned trigger, unsigned index,
                     unsigned owner)
{
    (void)opaque; (void)trigger; (void)index;
    return owner == 0 ? 1 : -1;
}

static int presentation(void *opaque, int phase,
                        const Sc64P8SourceAction *action, unsigned owner,
                        const Sc64Z8Rect *rect, u32 *duration_ms)
{
    Fixture *f = opaque;
    assert(action && rect && duration_ms && owner < SC64_P8_OWNER_COUNT);
    if (phase == SC64_P8_TIMED_ELIGIBLE) {
        ++f->eligible;
        return f->eligible_result;
    }
    if (phase == SC64_P8_TIMED_START) {
        ++f->starts;
        if (action->opcode == 10u)
            *duration_ms = 0; /* CenterView service enforces indefinite wait. */
        else *duration_ms = f->transmission_ms;
        return 1;
    }
    if (phase == SC64_P8_TIMED_FINISH) {
        ++f->finishes;
        return 1;
    }
    return -1;
}

static int pause_request(void *opaque, const Sc64P8SourceAction *action,
                         unsigned owner, int paused)
{
    Fixture *f = opaque;
    if (!f || !action || owner >= SC64_P8_OWNER_COUNT ||
        (action->opcode != 5u && action->opcode != 6u) ||
        paused != (action->opcode == 5u)) return -1;
    ++f->pause_calls;
    f->pause_owner = owner;
    f->pause_value = paused;
    return 1;
}

static int pause_trigger_end(void *opaque, unsigned trigger, unsigned owner)
{
    Fixture *f = opaque;
    if (!f || trigger >= SC64_P8_TRIGGER_LIMIT ||
        owner >= SC64_P8_OWNER_COUNT) return -1;
    ++f->pause_end_calls;
    f->pause_end_trigger = trigger;
    f->pause_owner = owner;
    f->pause_value = 0;
    return 1;
}

static int action(void *opaque, unsigned trigger, unsigned slot,
                  unsigned owner, int resume)
{
    Fixture *f = opaque;
    if (!f || trigger >= SC64_P8_TRIGGER_LIMIT || slot >= 4 || owner != 0)
        return -1;
    if (f->calls < 8) {
        f->trigger_calls[f->calls] = trigger;
        f->slot_calls[f->calls] = slot;
    }
    ++f->calls;
    Sc64P8SourceAction *a = &f->action[trigger][slot];
    if (a->opcode == 4u || a->opcode == 7u || a->opcode == 10u)
        return sc64_p8_timed_service_action(f->timer, a, owner,
                                             a->location ? &(Sc64Z8Rect){0,0,4,4} : 0,
                                             resume);
    if ((a->opcode == 5u || a->opcode == 6u) && f->world)
        return sc64_p8_world_action(f->world, a, owner, resume);
    return SC64_P8_ACTION_COMPLETE;
}

static Sc64P8WorldOps make_ops(Fixture *f)
{
    Sc64P8WorldOps ops = {f, trigger_info, condition, action, 0};
    return ops;
}

static int trigger_complete(void *opaque, unsigned trigger, unsigned owner)
{
    Fixture *f = opaque;
    if (!f || !f->timer) return -1;
    return sc64_p8_timed_service_trigger_complete(f->timer, trigger, owner);
}

static Sc64P8SourceAction make_action(unsigned opcode, u32 time,
                                      unsigned location)
{
    Sc64P8SourceAction a = {0};
    a.opcode = (u8)opcode;
    a.time = time;
    a.location = location;
    return a;
}

int main(void)
{
    Sc64P8TimedService timer;
    assert(sc64_p8_timed_service_init(&timer, 42));
    assert(!sc64_p8_timed_service_init(&timer, 40));
    assert(sc64_p8_timed_service_init(&timer, 42));
    assert(!sc64_p8_timed_service_set_frame_ms(&timer, 40) &&
           timer.frame_ms == 42);
    assert(sc64_p8_timed_service_set_frame_ms(&timer, 83));
    assert(sc64_p8_timed_service_set_frame_ms(&timer, 42));

    /* A pending owner wait blocks only later timed actions. Later triggers
     * still run; a blocked cursor does not skip to a later action. */
    Fixture f = {0}; f.timer = &timer;
    for (unsigned t = 0; t < 3; ++t) {
        f.trigger[t].enabled_players[0] = 1;
        f.trigger[t].action_count = t == 1 ? 2 : 1;
    }
    f.action[0][0] = make_action(4, 84, 0);
    f.action[1][0] = make_action(8, 0, 0); /* immediate source action */
    f.action[1][1] = make_action(4, 126, 0);
    f.action[2][0] = make_action(8, 0, 0);
    Sc64P8WorldOps ops = make_ops(&f);
    Sc64P8RuntimeState runtime;
    assert(sc64_p8_runtime_init(&runtime, 0, 1));
    assert(sc64_p8_runtime_step(&runtime, &ops, 0));
    assert(runtime.waiting == 1 && runtime.cursor[1] == 1 &&
           runtime.done == 4 && timer.wait_ms[0] == 84 &&
           f.calls == 4 && f.trigger_calls[0] == 0 &&
           f.trigger_calls[1] == 1 && f.slot_calls[1] == 0 &&
           f.trigger_calls[2] == 1 && f.slot_calls[2] == 1 &&
           f.trigger_calls[3] == 2);

    assert(sc64_p8_timed_service_tick(&timer, 0));
    assert(timer.wait_ms[0] == 42);
    assert(sc64_p8_runtime_step(&runtime, &ops, 0));
    assert(runtime.waiting == 1 && runtime.cursor[1] == 1 &&
           timer.wait_ms[0] == 42);
    assert(sc64_p8_timed_service_tick(&timer, 0));
    assert(timer.waiting_mask == 0 && timer.wait_ms[0] == 0);
    assert(sc64_p8_runtime_step(&runtime, &ops, 0));
    assert(runtime.waiting == 2 && runtime.cursor[1] == 1 &&
           timer.wait_ms[0] == 126);

    /* User pause freezes the trigger/timer clock unless the native source
     * PauseGame marker is active. */
    u32 before = timer.wait_ms[0];
    assert(sc64_p8_timed_service_tick(&timer, 1));
    assert(timer.wait_ms[0] == before);

    Sc64Z8WorldView actors = {0};
    Sc64Z8Rect locations[SC64_P8_SOURCE_LOCATION_COUNT] = {{0}};
    assert(sc64_p8_timed_service_release_wait(&timer, 0));
    Sc64P8WorldContext world = {
        .actors=&actors, .locations=locations,
        .location_count=SC64_P8_SOURCE_LOCATION_COUNT,
        .map_pixel_width=SC64_P8_SOURCE_MAP_COLS * 32u,
        .map_pixel_height=SC64_P8_SOURCE_MAP_ROWS * 32u
    };
    Fixture pause_fixture = {0};
    world.pause_game = pause_request;
    world.pause_context = &pause_fixture;
    world.pause_trigger_end = pause_trigger_end;
    world.pause_end_context = &pause_fixture;
    sc64_p8_timed_service_bind_world(&world, &timer);
    Sc64P8SourceAction pause = make_action(5, 0, 0);
    pause.trigger = 0;
    /* Native returns COMPLETE from PauseGame; a following Wait can remain
     * pending while source pause keeps the trigger/timer clock running. */
    Fixture pf = {0}; pf.timer = &timer; pf.world = &world;
    pf.trigger[0].enabled_players[0] = 1;
    pf.trigger[0].action_count = 3;
    pf.trigger[1].enabled_players[0] = 1;
    pf.trigger[1].action_count = 1;
    pf.action[0][0] = pause;
    pf.action[0][1] = make_action(4, 84, 0);
    pf.action[0][2] = make_action(8, 0, 0);
    pf.action[1][0] = make_action(8, 0, 0);
    Sc64P8WorldOps pops = make_ops(&pf);
    pops.trigger_complete = trigger_complete;
    Sc64P8RuntimeState pruntime;
    assert(sc64_p8_runtime_init(&pruntime, 0, 1));
    assert(sc64_p8_runtime_step(&pruntime, &pops,
               sc64_p8_timed_service_clock_paused(&timer, 0)));
    assert(pruntime.waiting == 1u && pruntime.cursor[0] == 1u &&
           pruntime.done == 2u && timer.wait_ms[0] == 84u &&
           timer.source_pause_active && timer.pause_trigger_mask[0] == 1u &&
           pause_fixture.pause_calls == 1u && pause_fixture.pause_value == 1 &&
           pause_fixture.pause_end_calls == 0u && pf.calls == 3u &&
           pf.trigger_calls[2] == 1u);
    /* Repeated PauseGame is a native no-op while the source marker is active. */
    assert(sc64_p8_world_action(&world, &pause, 0, 0) ==
           SC64_P8_ACTION_COMPLETE);
    assert(pause_fixture.pause_calls == 1);
    assert(sc64_p8_timed_service_clock_paused(&timer, 1) == 0);
    before = timer.wait_ms[0];
    assert(sc64_p8_timed_service_tick(&timer, 1));
    assert(timer.wait_ms[0] == before - 42u);
    assert(sc64_p8_runtime_step(&pruntime, &pops,
               sc64_p8_timed_service_clock_paused(&timer, 1)));
    assert(pruntime.waiting == 1u && timer.wait_ms[0] == 42u &&
           timer.source_pause_active && pause_fixture.pause_end_calls == 0u);
    assert(sc64_p8_timed_service_tick(&timer, 1));
    assert(timer.waiting_mask == 0u && timer.wait_ms[0] == 0u);
    assert(sc64_p8_runtime_step(&pruntime, &pops,
               sc64_p8_timed_service_clock_paused(&timer, 1)));
    assert(pruntime.done == 3u && pruntime.outcome == 0u &&
           !pruntime.waiting && pf.calls == 6u && pf.slot_calls[5] == 2u &&
           pause_fixture.pause_end_calls == 1u &&
           pause_fixture.pause_end_trigger == 0u &&
           pause_fixture.pause_value == 0 && !timer.source_pause_active &&
           timer.pause_trigger_mask[0] == 0u);
    /* An explicit UnpauseGame with no source marker is a native no-op. */
    Sc64P8SourceAction unpause = make_action(6, 0, 0);
    unpause.trigger = 0;
    assert(sc64_p8_world_action(&world, &unpause, 0, 0) ==
           SC64_P8_ACTION_COMPLETE && pause_fixture.pause_calls == 1u);
    /* A second owner is tracked independently; explicit unpause clears its
     * trigger bit before its action-list tail. */
    Sc64P8SourceAction other_owner_pause = make_action(5, 0, 0);
    other_owner_pause.trigger = 2;
    assert(sc64_p8_world_action(&world, &other_owner_pause, 1, 0) ==
           SC64_P8_ACTION_COMPLETE);
    assert(timer.source_pause_active && timer.pause_trigger_mask[1] == 4u &&
           timer.pause_trigger_mask[0] == 0u);
    Sc64P8SourceAction other_owner_unpause = make_action(6, 0, 0);
    other_owner_unpause.trigger = 2;
    assert(sc64_p8_world_action(&world, &other_owner_unpause, 1, 0) ==
           SC64_P8_ACTION_COMPLETE);
    assert(!timer.source_pause_active && timer.pause_trigger_mask[1] == 0u);
    before = timer.wait_ms[0];
    assert(sc64_p8_timed_service_clock_paused(&timer, 1) == 1);
    assert(sc64_p8_timed_service_tick(&timer, 1));
    assert(timer.wait_ms[0] == before);
    pause.group2 = 1;
    assert(sc64_p8_world_action(&world, &pause, 0, 0) == -1 &&
           pause_fixture.pause_calls == 3);
    pause.group2 = 0;
    int (*saved_end)(void *, unsigned, unsigned) = timer.pause_end_backend;
    timer.pause_end_backend = 0;
    Sc64P8SourceAction unsupported_pause = make_action(5, 0, 0);
    unsupported_pause.trigger = 3;
    assert(sc64_p8_world_action(&world, &unsupported_pause, 0, 0) == -1 &&
           pause_fixture.pause_calls == 3 && !timer.source_pause_active);
    timer.pause_end_backend = saved_end;
    /* Missing callback fails closed on a separate unbound world. */
    Sc64P8WorldContext no_pause = world;
    no_pause.pause_game = 0;
    assert(sc64_p8_world_action(&no_pause, &pause, 0, 0) == -1);

    /* Transmission starts once, shares the owner timer, resumes while it is
     * active, and finishes once after expiry. */
    f = (Fixture){0}; f.timer = &timer; f.eligible_result = 1;
    f.transmission_ms = 100;
    timer.presentation = presentation;
    timer.callback_context = &f;
    Sc64P8SourceAction shared_wait = make_action(4, 42, 0);
    Sc64P8SourceAction transmission = make_action(7, 0, 1);
    Sc64Z8Rect rect = {0, 0, 4, 4};
    assert(sc64_p8_timed_service_action(&timer, &shared_wait, 0, 0, 0) ==
           SC64_P8_ACTION_PENDING);
    assert(sc64_p8_timed_service_action(&timer, &transmission, 0, &rect, 0) ==
           SC64_P8_ACTION_BLOCKED);
    assert(f.starts == 0);
    assert(sc64_p8_timed_service_tick(&timer, 0));
    assert(sc64_p8_timed_service_action(&timer, &shared_wait, 0, 0, 1) ==
           SC64_P8_ACTION_COMPLETE);
    assert(sc64_p8_timed_service_action(&timer, &transmission, 0, &rect, 0) ==
           SC64_P8_ACTION_PENDING);
    assert(f.starts == 1 && timer.wait_ms[0] == 100);
    assert(sc64_p8_timed_service_action(&timer, &transmission, 0, &rect, 0) ==
           SC64_P8_ACTION_BLOCKED);
    assert(sc64_p8_timed_service_action(&timer, &transmission, 0, &rect, 1) ==
           SC64_P8_ACTION_PENDING);
    assert(sc64_p8_timed_service_tick(&timer, 0));
    assert(sc64_p8_timed_service_tick(&timer, 0));
    assert(sc64_p8_timed_service_tick(&timer, 0));
    assert(timer.waiting_mask == 0);
    assert(sc64_p8_timed_service_action(&timer, &transmission, 0, &rect, 1) ==
           SC64_P8_ACTION_COMPLETE);
    assert(f.starts == 1 && f.finishes == 1);

    f.eligible_result = 0;
    assert(sc64_p8_timed_service_action(&timer, &transmission, 0, &rect, 0) ==
           SC64_P8_ACTION_COMPLETE);
    assert(f.starts == 1);

    /* CenterView is released by its external camera-pan completion, not by a
     * finite message duration. Per-owner waits remain independent. */
    f.eligible_result = 1;
    Sc64P8SourceAction center = make_action(10, 0, 1);
    assert(sc64_p8_timed_service_action(&timer, &center, 0, &rect, 0) ==
           SC64_P8_ACTION_PENDING);
    assert(timer.wait_ms[0] == SC64_Z8_WAIT_INDEFINITE);
    assert(sc64_p8_timed_service_action(&timer, &shared_wait, 0, 0, 0) ==
           SC64_P8_ACTION_BLOCKED);
    assert(sc64_p8_timed_service_action(&timer, &center, 1, &rect, 0) ==
           SC64_P8_ACTION_PENDING);
    assert((timer.waiting_mask & 3u) == 3u &&
           timer.wait_ms[1] == SC64_Z8_WAIT_INDEFINITE);
    assert(sc64_p8_timed_service_tick(&timer, 0));
    assert(timer.wait_ms[0] == SC64_Z8_WAIT_INDEFINITE);
    assert(sc64_p8_timed_service_release_wait(&timer, 0));
    assert(sc64_p8_timed_service_action(&timer, &center, 0, &rect, 1) ==
           SC64_P8_ACTION_COMPLETE);
    assert(f.finishes == 2 && (timer.waiting_mask & 2u));
    assert(sc64_p8_timed_service_release_wait(&timer, 1));

    /* If presentation eligibility changes after an action started, refuse
     * the ambiguous native state transition rather than clearing the marker. */
    assert(sc64_p8_timed_service_action(&timer, &transmission, 0, &rect, 0) ==
           SC64_P8_ACTION_PENDING);
    f.eligible_result = 0;
    assert(sc64_p8_timed_service_action(&timer, &transmission, 0, &rect, 1) == -1);
    assert(timer.waiting_mask & 1u);
    puts("sc64_p8_timed_service: shared waits and source-pause tail pass");
    return 0;
}
