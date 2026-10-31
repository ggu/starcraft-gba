/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_DARK_SWARM_STATE_H
#define SC64_DARK_SWARM_STATE_H

/* This isolated state stage does not enable casting, targeting or rendering.
 * OpenBW creates a neutral, non-colliding effect with remove_timer=900; the
 * caller owns allocation, coordinates, update order and capacity. */
#define SC64_DARK_SWARM_REMOVE_TICKS 900u

typedef struct {
    int x, y;
    unsigned short remaining;
} Sc64DarkSwarmEffect;

/* Creation happens only after the caller has successfully allocated a slot
 * and clamped the position using the actual source unit bounds. */
static inline void sc64_dark_swarm_created(Sc64DarkSwarmEffect *effect, int x, int y)
{
    effect->x=x; effect->y=y; effect->remaining=SC64_DARK_SWARM_REMOVE_TICKS;
}

/* Source decrements a nonzero completed-unit remove timer once per update and
 * removes the effect on the update that takes it to zero. */
static inline int sc64_dark_swarm_tick(Sc64DarkSwarmEffect *effect)
{
    if (!effect->remaining) return 0;
    return --effect->remaining == 0;
}

/* Save validation is deliberately independent of a chosen serialization or
 * slot limit; an inactive slot has no lifetime to validate. */
static inline int sc64_dark_swarm_remaining_valid(unsigned remaining)
{
    return remaining <= SC64_DARK_SWARM_REMOVE_TICKS;
}
#endif
