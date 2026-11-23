/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_WORKER_MINERALS_H
#define SC64_WORKER_MINERALS_H

enum { SC64_MINERAL_SEARCH_RADIUS = 192 };

#ifdef SC64_MINERAL_PATCH_CONTENTION_PROBE
/* Slot plus one, rebuilt for each order pass. Multiple partial timers can
 * exist in older saves. The lowest eligible slot takes the patch first;
 * the other timers remain intact until their workers can resume. */
_Static_assert(MAX_UNITS <= 255, "mineral service claim must fit in a byte");
static u8 mineral_patch_claims[MAX_MINERALS];

static void __attribute__((noinline))
worker_mineral_claims_begin(void)
{
    memset(mineral_patch_claims, 0, sizeof(mineral_patch_claims));
    for (int i = 0; i < MAX_UNITS; ++i) {
        const Unit *worker = &u[i];
        if (!worker->work_timer || worker->order != ORDER_MINE || !worker->alive)
            continue;
        const int patch = worker->target;
        if ((unsigned)patch >= MAX_MINERALS || mineral_patch_claims[patch] ||
            m[patch].amount <= 0 || !IS_WORKER(worker->type) ||
            SC64_TRANSPORT_LOADED(worker) || sc64_worker_cargo_matches_order(worker))
            continue;
#ifdef SC64_P6_COMBAT_ENABLED
        if (race_p6_combat_unit_burrowed(i)) continue;
#endif
#ifdef SC64_T6_RUNTIME
        if (race_t6_unit_burrowed(i)) continue;
#endif
#ifdef SC64_NEAREST_RESOURCE_DEPOT
        if (worker->build == -2 && !sc64_worker_gather_pending(worker)) continue;
#endif
        if (dist2(worker->x, worker->y, m[patch].x, m[patch].y) <=
            MINERAL_WORK_RANGE * MINERAL_WORK_RANGE)
            mineral_patch_claims[patch] = (u8)(i + 1);
    }
}
#endif

/* Continue an established mineral job within its former mineral cluster.
 * OpenBW bounds depleted-field searches to 12 source tiles (192 pixels at
 * this engine's half scale). Reuse the normal route predicate; never assign
 * an undiscovered human field or select an unreachable closer field.
 * The engine does not retain source terrain elevation/worker field queues,
 * so this is bounded continuation, not an exact native allocation policy. */
static int __attribute__((noinline))
worker_continue_minerals(Unit *q)
{
    const int distance_limit = SC64_MINERAL_SEARCH_RADIUS * SC64_MINERAL_SEARCH_RADIUS + 1;
    int best = -1, best_d = distance_limit;
    int origin_x, origin_y;
    if (q->target < 0 || q->target >= MAX_MINERALS) return 0;
    origin_x = m[q->target].x;
    origin_y = m[q->target].y;
#ifdef SC64_MINERAL_PATCH_CONTENTION_PROBE
    /* A worker leaving a busy patch needs a free alternative. A depleted
     * patch can still hand off to an occupied field and wait there. */
    const int look_for_free = m[q->target].amount > 0;
#endif
    /* Rank in slot order before routing. A reachable minimum wins; on a
     * miss, repeat the original scan without the unreachable minimum. */
    int known_miss = -1;
    do {
        for (int i = 0; i < MAX_MINERALS; ++i) {
            int d, nx, ny;
            if (i == known_miss) continue;
#ifdef SC64_MINERAL_PATCH_CONTENTION_PROBE
            if (look_for_free && mineral_patch_claims[i]) continue;
#endif
            if (m[i].amount <= 0 || m[i].x < 0 || m[i].y < 0 ||
                m[i].x >= WORLD_W || m[i].y >= WORLD_H) continue;
            if (q->owner == SIDE) {
                int cell = m[i].x / 16 + (m[i].y / 16) * MAP_COLS;
                if (!SC64_GRID_GET(explored, cell) && !SC64_GRID_GET(visible, cell))
                    continue;
            }
            d = dist2(origin_x, origin_y, m[i].x, m[i].y);
            if (d >= best_d) continue;
            if (known_miss >= 0 && !path_next_tile(q->x, q->y, m[i].x, m[i].y, &nx, &ny))
                continue;
            best = i;
            best_d = d;
        }
        int nx, ny;
        if (known_miss >= 0 || best < 0 ||
            path_next_tile(q->x, q->y, m[best].x, m[best].y, &nx, &ny)) break;
        known_miss = best;
        best = -1;
        best_d = distance_limit;
    } while (1);
    if (best < 0) return 0;
    q->target = best;
    q->tx = m[best].x; q->ty = m[best].y;
    q->path_timer = q->path_retry = 0;
    q->work_timer = 0;
    q->build = -1;
    return 1;
}

/* A resource wait advances only while the worker is at its target. Existing
 * saved nonzero timers remain remaining work; no new persistent state. */
static int
worker_gather_ready(Unit *q)
{
#ifdef SC64_MINERAL_PATCH_CONTENTION_PROBE
    if (q->order == ORDER_MINE && IS_WORKER(q->type)) {
        const unsigned slot = (unsigned)(q - u);
        u8 *claim = &mineral_patch_claims[q->target];
        if (*claim && *claim != slot + 1u) {
            /* Spread alternate-field searches across updates. Only an
             * empty worker without partial service can change targets. */
            if (!q->carry && !q->work_timer && !((frame + slot) & 15u))
                (void)worker_continue_minerals(q);
            return 0;
        }
        *claim = (u8)(slot + 1u);
    }
#endif
    if (!q->work_timer) q->work_timer = q->order == ORDER_MINE ? 45 : 60;
    return --q->work_timer == 0;
}

#endif
