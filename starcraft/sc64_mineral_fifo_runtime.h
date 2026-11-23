/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Shared runtime for standalone and packed mineral queues. */
enum { MINERAL_CARGO_MAX = 8, MINERAL_SERVICE_TICKS = 45 };
static u8 mineral_fifo_serviced[(MAX_MINERALS + 7) / 8];

static inline __attribute__((always_inline)) int
worker_mineral_fifo_matches_patch(const Unit *worker, const Mineral *patch,
                                  int index, unsigned width, unsigned height)
{
#ifdef SC64_PACKED_MINERAL_FIFO
    if (SC64_TRANSPORT_LOADED(worker) || sc64_worker_cargo_matches_order(worker)) return 0;
#ifdef SC64_NEAREST_RESOURCE_DEPOT
    if (worker->build == -2 && !sc64_worker_gather_pending(worker)) return 0;
#endif
#endif
    return worker->alive && IS_WORKER(worker->type) && worker->order == ORDER_MINE &&
        worker->target == index && patch->amount > 0 &&
        (unsigned)patch->x < width && (unsigned)patch->y < height &&
        (unsigned)worker->x < width && (unsigned)worker->y < height &&
        dist2(worker->x, worker->y, patch->x, patch->y) <= MINERAL_WORK_RANGE * MINERAL_WORK_RANGE;
}

static inline __attribute__((always_inline)) int
worker_mineral_fifo_live_slot_at_patch(unsigned slot, const Unit *worker,
                                       const Mineral *patch, int index)
{
    (void)slot;
#ifdef SC64_PACKED_MINERAL_FIFO
#ifdef SC64_P6_COMBAT_ENABLED
    if (race_p6_combat_unit_burrowed((int)slot)) return 0;
#endif
#ifdef SC64_T6_RUNTIME
    if (race_t6_unit_burrowed((int)slot)) return 0;
#endif
#endif
    return worker_mineral_fifo_matches_patch(worker, patch, index, WORLD_W, WORLD_H);
}

static void
worker_mineral_fifo_prune(Sc64MineralFifo *fifo, Unit *units, const Mineral *patches)
{
    for (int i = 0; i < MAX_MINERALS; ++i) {
        unsigned link = fifo->head[i], previous = 0;
        while (link) {
            unsigned slot = link - 1u;
            Unit *worker = &units[slot];
            link = fifo->next[slot];
            if (
#ifndef SC64_PACKED_MINERAL_FIFO
                !worker->carry &&
#endif
                worker_mineral_fifo_live_slot_at_patch(slot, worker, &patches[i], i)) {
                previous = slot + 1u;
                continue;
            }
            if (worker->order == ORDER_MINE &&
#ifdef SC64_PACKED_MINERAL_FIFO
                IS_WORKER(worker->type)
#else
                worker->target == i
#endif
               ) worker->work_timer = 0;
            sc64_mineral_fifo_unlink(fifo, (unsigned)i, slot, previous);
        }
    }
}

#ifdef SC64_PACKED_MINERAL_FIFO
/* The orders pass is IWRAM-sized in several profiles. Keep this walk in ROM. */
static void __attribute__((noinline))
#else
static void
#endif
worker_mineral_fifo_begin(void)
{
    memset(mineral_fifo_serviced, 0, sizeof(mineral_fifo_serviced));
    worker_mineral_fifo_prune(&mineral_fifo, u, m);
}

static int
worker_mineral_fifo_ready(Unit *worker)
{
    unsigned slot = (unsigned)(worker - u);
    unsigned patch = (unsigned)worker->target;
    if (mineral_fifo.patch[slot] != patch + 1u) {
        worker->work_timer = 0;
        if (!sc64_mineral_fifo_append(&mineral_fifo, patch, slot)) return 0;
    }
    if (mineral_fifo.head[patch] != slot + 1u) return 0;
    u8 bit = (u8)(1u << (patch & 7u));
    u8 *serviced = &mineral_fifo_serviced[patch >> 3];
    if (*serviced & bit) return 0;
    if (!worker->work_timer) worker->work_timer = MINERAL_SERVICE_TICKS;
    if (--worker->work_timer) return 0;
    /* Release for the next orders pass, independent of actor slot order. */
    *serviced |= bit;
    sc64_mineral_fifo_remove(&mineral_fifo, slot);
    return 1;
}
