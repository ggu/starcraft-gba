/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stdio.h>
#include "../common/gba.c"
#include "../starcraft/game.c"

#if !defined(SC64_Z8_DIRECT_COMMAND_CENSUS) || !defined(SC64_P10_RUNTIME)
#error "Requires the shared Command census and prepared P10 host flags"
#endif

static int compare(const Sc64Z8WorldView *world, unsigned player, unsigned raw,
                   unsigned cmp, unsigned trigger_owner, unsigned opcode,
                   const Sc64Z8Rect *rect, u32 *count)
{
    Sc64Z8WorldView candidate = *world, reference = *world;
    Sc64Z8Condition c = {.player = player, .unit = raw, .comparison = cmp, .opcode = opcode};
    u32 a = 0xdeadbeef, b = a;
    reference.command_count = 0;
    int expected = sc64_z8_world_count(&reference, &c, trigger_owner, rect, &a);
    int actual = sc64_z8_world_count(&candidate, &c, trigger_owner, rect, &b);
    assert(actual == expected && b == a);
    if (actual < 0) assert(b == 0xdeadbeef);
    if (count) *count = b;
    return actual;
}

static int custom_actor(void *context, unsigned index, Sc64Z8ActorView *out)
{
    (void)context;
    assert(index == 0);
    *out = (Sc64Z8ActorView){.alive = 1, .type_known = 1, .completed = 1,
                             .source_owner = 0, .raw_type = 42, .men = 1};
    return 1;
}

int main(void)
{
    game_init();
    race_mode = 0;
    race_selected_mission = SC64_P10_GLOBAL_ID;
    init_state();
    assert(screen == SCREEN_PLAY && race_p10_active());
    assert(race_p10_view.command_count == sc64_z8_game_command_count);

    Sc64Z8WorldView world = race_p10_view;
    const unsigned selectors[] = {0, 1, 2, 7, 8, 12, 13, 17, 18, 19, 20, 21};
    for (unsigned i = 0; i < sizeof selectors / sizeof *selectors; ++i)
        for (unsigned raw = 0; raw <= 231; ++raw)
            for (unsigned cmp = 0; cmp < 3; ++cmp)
                for (unsigned runner = 0; runner < 8; ++runner)
                    compare(&world, selectors[i], raw, cmp, runner, 2, 0, 0);

    unsigned slot = MAX_UNITS - 1u, building = MAX_BUILDINGS - 1u;
    memset(u, 0, sizeof u);
    memset(b, 0, sizeof b);
    world.active_mask = 3;
    u32 count;
    assert(compare(&world, 0, 0, 0, 0, 2, 0, &count) == 1 && count == 0);
    u[slot] = (Unit){.alive = 1, .raw_owner = 0, .raw_type = 0, .type = U_MARINE,
                     .x = 42, .y = 42};
    assert(compare(&world, 0, 0, 0, 0, 2, 0, &count) == 1 && count == 1);
    u[slot].raw_owner = 1;
    assert(compare(&world, 0, 0, 0, 0, 2, 0, &count) == 1 && count == 0);
    u[slot].raw_owner = 11;
    assert(compare(&world, 17, 229, 0, 0, 2, 0, &count) == 1 && count == 0);
    u[slot].raw_owner = -1;
    assert(compare(&world, 0, 64, 0, 0, 2, 0, 0) == -1);
    u[slot].raw_owner = 1;
    u[slot].raw_type = 255;
    assert(compare(&world, 0, 64, 0, 0, 2, 0, 0) == -1);
    u[slot].raw_type = 0;
    u[slot].x = -1;
    assert(compare(&world, 0, 64, 0, 0, 2, 0, 0) == -1);
    u[slot].alive = 0;
    assert(compare(&world, 0, 0, 0, 0, 2, 0, &count) == 1 && count == 0);

    b[building] = (Building){.alive = 1, .raw_owner = 0, .raw_type = 154,
                             .x = 48, .y = 48, .under_construction = 1};
    assert(compare(&world, 0, 231, 0, 0, 2, 0, &count) == 1 && count == 0);
    assert(compare(&world, 0, 231, 1, 0, 2, 0, &count) == 1 && count == 1);
    b[building].under_construction = 0;
    assert(compare(&world, 0, 231, 0, 0, 2, 0, &count) == 1 && count == 1);
    b[building].y = -1;
    assert(compare(&world, 1, 64, 0, 0, 2, 0, 0) == -1);
    b[building].alive = 0;
    assert(compare(&world, 0, 231, 0, 0, 2, 0, &count) == 1 && count == 0);

    world.actor_at = custom_actor;
    world.actor_count = 1;
    assert(compare(&world, 0, 42, 0, 0, 2, 0, &count) == 1 && count == 1);
    const Sc64Z8Rect rect = {0, 0, 1, 1};
    assert(compare(&world, 0, 42, 0, 0, 3, &rect, &count) == 1 && count == 1);
    puts("Shared direct Command census matches the original actor provider");
}
