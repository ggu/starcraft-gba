#include <assert.h>
#include <setjmp.h>
#include <stdio.h>
#include <string.h>

#define SC64_RACES
#define SC64_UNIFIED
#define SC64_Z3_ENABLED
#define SC64_CAMPAIGN_RACE 1
#define SC64_Z3_WINDOWED_SCHEDULE
#define main scheduled_main
#include "../common/main.c"
#undef main

/* Run the actual main loop against deterministic hardware stubs. The render
 * stub installs a page after its last input has been consumed; main's real
 * observer-return path must reset the cursor before the next update. */
static const u32 transitions[][2] = {
    {0, KEY_A}, {999, KEY_B}, {1000, KEY_A | KEY_B}, {1001, KEY_B},
    {1999, KEY_A}, {2000, KEY_A | KEY_RIGHT},
    {5000, KEY_RIGHT}, {5001, KEY_L}, {5999, KEY_L | KEY_R},
    {6000, KEY_R}, {6999, KEY_R | KEY_START}, {7000, KEY_R}
};
enum { STOP_TICK = 7501, INPUT_COUNT = sizeof(transitions) / sizeof(transitions[0]) };
static jmp_buf finished;
static u32 iterations, previous_expected, checks, page_count;
static int windowed;

static u16 reference_held(u32 tick)
{
    u16 held = 0;
    for (unsigned i = 0; i < INPUT_COUNT; ++i)
        if (transitions[i][0] <= tick) held = (u16)transitions[i][1];
    return held;
}

static void install_page(u32 start)
{
    u32 count = 0, end = start + 1000u;
    if (end > STOP_TICK) end = STOP_TICK;
    for (unsigned i = 0; i < INPUT_COUNT; ++i)
        if (transitions[i][0] >= start && transitions[i][0] < end) {
            demo_schedule_events[count][0] = transitions[i][0];
            demo_schedule_events[count++][1] = transitions[i][1];
        }
    demo_schedule_count = count;
    ++page_count;
}

void gba_init(void) {}
void game_init(void)
{
    demo_schedule_length = STOP_TICK;
    if (windowed) {
        install_page(0);
        demo_schedule_active = 1;
    } else {
        for (unsigned i = 0; i < INPUT_COUNT; ++i) {
            demo_schedule_events[i][0] = transitions[i][0];
            demo_schedule_events[i][1] = transitions[i][1];
        }
        demo_schedule_count = INPUT_COUNT;
    }
}
u16 gba_keys(void)
{
    if (iterations < STOP_TICK) return KEY_SELECT; /* Scheduled input overrides it. */
    return iterations == STOP_TICK ? KEY_R : KEY_SELECT;
}
u32 gba_clock(void) { return iterations * 100u; }
void audio_update(void) {}
void game_update(u16 held, u16 pressed)
{
    u16 expected = iterations < STOP_TICK ? reference_held(iterations) :
        (u16)(gba_keys() | demo_inject_keys);
    assert(held == expected);
    assert(pressed == (u16)(expected & (u16)~previous_expected));
    assert(demo_keys == held);
    assert(demo_schedule_tick == (iterations < STOP_TICK ? iterations : STOP_TICK));
    previous_expected = expected;
    ++checks;
}
void game_render(void)
{
    u32 next = iterations + 1u;
    if (next < STOP_TICK && next % 1000u == 0 && windowed) {
        assert(demo_schedule_index == demo_schedule_count);
        install_page(next); /* Deliberately leave the old cursor unchanged. */
    }
    if (next == STOP_TICK) {
        assert(demo_schedule_index == demo_schedule_count);
        demo_schedule_active = demo_schedule_count = 0;
    }
    if (next == STOP_TICK + 1u) demo_inject_keys = KEY_L;
    iterations = next;
    if (iterations == STOP_TICK + 3u) longjmp(finished, 1);
}

static void run_case(int paged)
{
    windowed = paged;
    iterations = previous_expected = checks = page_count = 0;
    demo_frame = demo_schedule_count = demo_schedule_length = demo_schedule_tick = 0;
    demo_schedule_active = demo_schedule_index = 0;
    demo_inject_keys = demo_keys = 0;
    memset((void *)demo_schedule_events, 0, sizeof(demo_schedule_events));
    if (setjmp(finished) == 0) (void)scheduled_main();
    assert(checks == STOP_TICK + 3u);
    assert(page_count == (paged ? 8u : 0u));
}

int main(void)
{
    assert(sizeof(demo_schedule_events) == 256u * 8u);
    run_case(1);
    run_case(0);
    puts("Actual main loop preserves paged/legacy held keys, edges and empty windows.");
    return 0;
}
