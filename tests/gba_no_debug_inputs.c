// SPDX-License-Identifier: GPL-3.0-or-later
#include <assert.h>
#include <setjmp.h>

#ifndef GBA_NO_DEBUG_INPUTS
#error "This fixture must compile the physical-input-only main loop"
#endif

#define main physical_input_main
#include "../common/main.c"
#undef main

enum { INPUT_TICKS = 8 };
static const u16 inputs[INPUT_TICKS] = {
    KEY_A, KEY_A, KEY_A | KEY_B, KEY_B, 0, KEY_RIGHT,
    KEY_RIGHT | KEY_A, 0
};
static const u16 pressed[INPUT_TICKS] = {
    KEY_A, 0, KEY_B, 0, 0, KEY_RIGHT, KEY_A, 0
};
static jmp_buf finished;
static unsigned tick;
static unsigned clock_reads;
static u32 clock_value;

void gba_init(void) {}
void game_init(void) {}
u16 gba_keys(void) { return tick < INPUT_TICKS ? inputs[tick] : 0; }
u32 gba_clock(void)
{
    if (tick == INPUT_TICKS && clock_reads % 3u == 0) longjmp(finished, 1);
    ++clock_reads;
    clock_value += 17;
    return clock_value;
}
void audio_update(void) {}

void game_update(u16 held, u16 rising)
{
    assert(tick < INPUT_TICKS);
    assert(held == inputs[tick]);
    assert(rising == pressed[tick]);
    assert(demo_keys == held);
    ++tick;
}

void game_render(void)
{
}

int main(void)
{
    if (setjmp(finished) == 0) (void)physical_input_main();

    assert(tick == INPUT_TICKS);
    assert(demo_frame == INPUT_TICKS);
    assert(demo_update_cycles == 17);
    assert(demo_last_cycles == 34);
    assert(demo_peak_cycles == 34);
    assert(demo_total_cycles == INPUT_TICKS * 34u);
    assert(demo_tick_buckets[0] == INPUT_TICKS);
    for (unsigned i = 1; i < 8; ++i) assert(demo_tick_buckets[i] == 0);
    return 0;
}
