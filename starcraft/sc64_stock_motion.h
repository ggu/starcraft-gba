/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_STOCK_MOTION_H
#define SC64_STOCK_MOTION_H
#include "sc64_stock_profiles.h"

/* Nominal top speed at half spatial scale, 24 source / 30 runtime updates.
 * Absolute tick phase replaces a per-unit fractional accumulator. It is
 * deterministic across saves and costs no actor RAM. This does not model
 * native acceleration, turning, speed upgrades or IScript move instructions.
 * Eight-direction navigation normalizes diagonal axes by 1/sqrt(2).
 */
static unsigned __attribute__((noinline))
sc64_stock_motion_step(int raw, unsigned tick, unsigned slot, int diagonal)
{
    const Sc64StockUnitProfile *p=sc64_stock_unit_profile(raw);
    unsigned rate=sc64_stock_runtime_speed_q10(p);
    unsigned phase=(tick+slot*67u)&1023u;
    if (diagonal) rate=(rate*724u+512u)>>10;
    return ((phase+1u)*rate>>10)-(phase*rate>>10);
}

static inline int sc64_stock_motion_axis(int from,int to,unsigned step)
{
    unsigned distance=(unsigned)(from>to?from-to:to-from);
    if (step>=distance) return to;
    return from+(from<to?(int)step:-(int)step);
}
#endif
