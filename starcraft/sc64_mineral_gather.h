/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Included by game.c after the common gather timer and resource state. */
#ifndef SC64_MINERAL_GATHER_H
#define SC64_MINERAL_GATHER_H

#if defined(SC64_MINERAL_CONTENTION_PROBE) && \
    !defined(SC64_NEAREST_RESOURCE_DEPOT) && !defined(SC64_WORKER_CARGO_KIND)
#error "Mineral contention requires the shared resource-worker path"
#endif
#if defined(SC64_NEAREST_RESOURCE_DEPOT) || defined(SC64_WORKER_CARGO_KIND)
static void
race_collect_mineral(Unit *q)
{
    int collected = m[q->target].amount < 8 ? m[q->target].amount : 8;
#ifdef SC64_WORKER_CARGO_KIND
    if (collected > 0) {
        q->carry = collected;
        m[q->target].amount -= collected;
        if (IS_WORKER(q->type)) {
            sc64_worker_cargo_clear(q);
            sc64_worker_gather_pending_set(q, 0);
        }
    }
#else
    q->carry = collected;
    m[q->target].amount -= collected;
#endif
    if (q->carry > 0) q->build = -1;
    if (collected <= 0) q->order = ORDER_IDLE;
}

#ifdef SC64_MINERAL_CONTENTION_PROBE
_Static_assert(MAX_UNITS < 256, "One byte holds a mineral requester plus the zero sentinel");

/* Rebuilt for each orders pass. Only existing worker timers carry progress
 * across updates or saves. In a legacy overlap, both remaining times survive,
 * but only the first ready slot with partial work advances. */
static u8 sc64_mineral_requests[MAX_MINERALS];

static void
sc64_mineral_request(Unit *q)
{
    u8 *request = &sc64_mineral_requests[q->target];
    if (!*request || (q->work_timer && !u[*request - 1].work_timer))
        *request = (u8)((q - u) + 1);
}

static void __attribute__((noinline))
sc64_mineral_finish_orders(void)
{
    for (int patch = 0; patch < MAX_MINERALS; ++patch) {
        if (!sc64_mineral_requests[patch]) continue;
        Unit *q = &u[sc64_mineral_requests[patch] - 1];
        /* A later actor may have killed, transported or displaced the
         * requester. Leave this patch to the next pass in that case. */
        if (!q->alive || SC64_TRANSPORT_LOADED(q) ||
            q->order != ORDER_MINE || q->target != patch ||
            sc64_worker_cargo_matches_order(q) || m[patch].amount <= 0 ||
            dist2(q->x, q->y, m[patch].x, m[patch].y) > 7 * 7) continue;
        if (worker_gather_ready(q)) race_collect_mineral(q);
    }
}
#endif
#endif

#endif
