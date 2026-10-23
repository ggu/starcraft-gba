#include <assert.h>
#include <stdint.h>
#include "../common/gba_present.h"

static u16
line_at(u32 cycle)
{
    return (u16)((cycle % GBA_PRESENT_CYCLES_PER_FRAME) / 1232u);
}

static u32
next_present(u32 previous, u32 work_finished)
{
    u32 now = work_finished;
    while (!gba_present_ready(now - previous, line_at(now))) ++now;
    return now;
}

static void
test_ready_window_and_minimum_interval(void)
{
    const u32 minimum = GBA_PRESENT_MIN_FRAME_CYCLES;
    assert(!gba_present_ready(minimum - 1, 160));
    assert(!gba_present_ready(minimum, 159));
    assert(gba_present_ready(minimum, 160));
    assert(gba_present_ready(minimum, 226));
    assert(!gba_present_ready(minimum, 227));
    assert(!gba_present_ready(minimum + 10, 0));
    assert(!gba_present_ready(minimum + 10, 159));

    /* Any amount of late work still cannot flip during visible scanlines. */
    for (u16 line = 0; line < 160; ++line)
        assert(!gba_present_ready(UINT32_MAX, line));
}

static void
test_wraparound_elapsed(void)
{
    u32 previous = UINT32_MAX - 100000u;
    u32 now = previous + GBA_PRESENT_MIN_FRAME_CYCLES;
    assert(now < previous);
    assert(gba_present_ready(now - previous, 160));
}

static void
test_underbudget_just_over_and_steady_work(void)
{
    u32 previous = 12u * GBA_PRESENT_CYCLES_PER_FRAME;
    u32 shown = next_present(previous, previous + 10000u);
    assert(shown - previous >= GBA_PRESENT_MIN_FRAME_CYCLES);
    assert(gba_present_ready(shown - previous, line_at(shown)));

    /* Finishing just after the two-frame threshold reuses the current blank
     * interval if it is still in the safe VBlank window. */
    previous = 20u * GBA_PRESENT_CYCLES_PER_FRAME + 160u * 1232u;
    u32 finish = previous + GBA_PRESENT_MIN_FRAME_CYCLES + 5u;
    shown = next_present(previous, finish);
    assert(shown == finish);
    assert(shown - previous >= GBA_PRESENT_MIN_FRAME_CYCLES);
    assert(gba_present_ready(shown - previous, line_at(shown)));

    /* Sustained one-frame rendering work remains capped at one flip per two
     * hardware frames, even if it completes before the gate opens. */
    previous = 30u * GBA_PRESENT_CYCLES_PER_FRAME;
    for (unsigned frame = 0; frame < 12; ++frame) {
        finish = previous + GBA_PRESENT_CYCLES_PER_FRAME;
        shown = next_present(previous, finish);
        assert(shown - previous >= GBA_PRESENT_MIN_FRAME_CYCLES);
        assert(gba_present_ready(shown - previous, line_at(shown)));
        previous = shown;
    }
}

int
main(void)
{
    test_ready_window_and_minimum_interval();
    test_wraparound_elapsed();
    test_underbudget_just_over_and_steady_work();
    return 0;
}
