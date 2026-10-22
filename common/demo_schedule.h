#ifndef DEMO_SCHEDULE_H
#define DEMO_SCHEDULE_H

#include "gba.h"

#define DEMO_SCHEDULE_WINDOW_CAPACITY 256u

/* Consume input transitions for exactly one absolute scheduler tick. The
 * held value and cursor are caller-owned so they can persist across pages. */
static inline u16
demo_schedule_window_held(const volatile u32 events[][2], u32 count,
                          volatile u32 *cursor, u32 tick, u16 *held)
{
    while (*cursor < count && *cursor < DEMO_SCHEDULE_WINDOW_CAPACITY &&
           events[*cursor][0] == tick) {
        *held = (u16)events[*cursor][1];
        ++*cursor;
    }
    return *held;
}

/* Host-side contract check for a page before it reaches the runtime queue. */
static inline int
demo_schedule_window_valid(const volatile u32 events[][2], u32 count,
                           u32 start_tick, u32 end_tick)
{
    u32 i, previous = 0;
    if (count > DEMO_SCHEDULE_WINDOW_CAPACITY || start_tick >= end_tick)
        return 0;
    for (i = 0; i < count; ++i) {
        u32 tick = events[i][0];
        if (tick < start_tick || tick >= end_tick || events[i][1] > 1023u ||
            (i && tick <= previous))
            return 0;
        previous = tick;
    }
    return 1;
}

#endif
