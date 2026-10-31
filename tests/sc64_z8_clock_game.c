/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Focused ordinary-menu continuation check; does not run corruption fixtures. */
#define main sc64_z8_other_fixture_main
#include "sc64_z8_game.c"
#undef main

int main(void)
{
    /* Observe the authored camera action rather than fabricating its wait. */
    launch_z8_from_normal_menu();
    for (int i = 0; i < 2000 && !race_z8_camera_release; ++i) input(0);
    assert(race_z8_camera_release);
    {
        unsigned prior = (unsigned)frame;
        u32 native = race_z8_sched.frames;
        input(0);
        if (!sc64_z8_native_step(prior + 1u)) {
            assert(race_z8_camera_release && race_z8_sched.frames == native);
            input(0);
        }
        assert(!race_z8_camera_release && race_z8_sched.frames == native + 1u);
    }
    for (unsigned phase = 0; phase < 5; ++phase) {
        u32 expected;
        launch_z8_from_normal_menu();
        for (int i = 0; i < 120; ++i) input(0);
        while ((unsigned)frame % 5u != phase || race_z8_camera_release) input(0);
        press(KEY_START);
        assert(screen == SCREEN_PAUSE);
        unsigned paused_frame = (unsigned)frame;
        u32 paused_native = race_z8_sched.frames;
        for (int i = 0; i < 10; ++i) input(0);
        assert((unsigned)frame == paused_frame && race_z8_sched.frames == paused_native);
        press(KEY_B); /* ordinary pause-menu save */
        press(KEY_START);
        for (int i = 0; i < 20; ++i) input(0);
        expected = world_digest();
        game_init();
        held_keys = 0;
        press(KEY_L); /* ordinary title load */
        assert(screen == SCREEN_PLAY && race_z8_active());
        for (int i = 0; i < 20; ++i) input(0);
        assert(world_digest() == expected);
    }
    puts("Z8 ordinary save/load continuation at all five clock phases");
    return 0;
}
