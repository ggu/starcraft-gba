/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef GBA_ELAPSED_CLOCK_H
#define GBA_ELAPSED_CLOCK_H
#include <stdint.h>

typedef struct { uint32_t latest, wraps; } GbaElapsedClock;

/* A frame-loop sample arrives long before the cascaded 32-bit counter
 * could wrap twice. A wrap represents exactly 256 seconds at 2^24 Hz. */
static inline void gba_elapsed_observe(GbaElapsedClock *clock, uint32_t cycles)
{
    if (cycles < clock->latest) ++clock->wraps;
    clock->latest = cycles;
}

static inline uint32_t gba_elapsed_milliseconds_at(const GbaElapsedClock *clock)
{
    /* 1000 / 2^24 equals 125 / 2^21; the fractional product fits in u32. */
    uint32_t fraction = (clock->latest & 0xffffffu) * 125u;
    return clock->wraps * 256000u + (clock->latest >> 24) * 1000u + (fraction >> 21);
}
#endif
