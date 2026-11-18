/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_CROWD_SLICE_H
#define SC64_CROWD_SLICE_H
#include "sc64_unit_index.h"

/* Target acquisition for crowds in large unit tables.
 *
 * Only a unit table wider than the release one (SC64_UNIT_IX_WIDE) can hold
 * more than SC64_CROWD_SLICE_THRESHOLD live units. While it does, an
 * attack-moving unit searches for the nearest enemy on one update in
 * SC64_CROWD_SLICE_PERIOD (staggered by slot); on the other updates it keeps
 * the target its last search found while that target is alive and in its
 * search range, and otherwise continues its move. The kept target is stored in
 * Unit.target, which attack-move does not otherwise use, so saves resume it.
 * Movement, collision and combat still run every update. With at most the
 * threshold alive, every unit searches every update exactly as the release
 * engine does, so such games are identical. */
#if SC64_UNIT_IX_WIDE
#define SC64_CROWD_SLICE_THRESHOLD 240
#define SC64_CROWD_SLICE_PERIOD 4u
static u8 sc64_crowd_slicing;
static int sc64_crowd_live;
/* update_orders() counts map units while it builds its owner lists. An
 * invalid owner stops that pass early; the partial count can only leave the
 * update searching more units, as the release engine does. */
#define SC64_CROWD_SLICE_BEGIN() (sc64_crowd_live = 0, sc64_crowd_slicing = 0)
#define SC64_CROWD_SLICE_COUNT() \
    (sc64_crowd_slicing = ++sc64_crowd_live > SC64_CROWD_SLICE_THRESHOLD)

static inline int sc64_crowd_searches_now(int slot)
{
    return !sc64_crowd_slicing ||
           !(((unsigned)frame + (unsigned)slot) % SC64_CROWD_SLICE_PERIOD);
}

static int __attribute__((noinline)) sc64_crowd_kept_target(const Unit *q, int range)
{
    int target = q->target;
    if (target < 0 || target >= MAX_UNITS + MAX_BUILDINGS || !target_alive(target))
        return -1;
    if (dist2(q->x, q->y, target_x(target), target_y(target)) > range * range) return -1;
    return target;
}
#else
#define SC64_CROWD_SLICE_BEGIN() ((void)0)
#define SC64_CROWD_SLICE_COUNT() ((void)0)
#endif

#endif
