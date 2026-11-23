/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Mineral queue lifecycle and raw journal preparation. Included by game.c. */

enum { MINERAL_CARGO_MAX = 8, MINERAL_SERVICE_TICKS = 45 };
#define SC64_MINERAL_FIFO_SAVE_VERSION \
    ((SAVE_VERSION & ~SC64_SAVE_TAG_NAMESPACE_MASK) | SC64_SAVE_TAG_MINERAL_FIFO_NAMESPACE)

static const u32 mineral_fifo_save_tags[] = {SAVE_VERSION, SC64_MINERAL_FIFO_SAVE_VERSION};
static Sc64MineralFifo prepared_mineral_fifo;
static u8 mineral_fifo_serviced[(MAX_MINERALS + 7) / 8];

static inline __attribute__((always_inline)) int
worker_mineral_fifo_at_patch(const Unit *worker, const Mineral *patch, int index)
{
    return worker->alive && IS_WORKER(worker->type) && worker->order == ORDER_MINE &&
        worker->target == index && patch->amount > 0 &&
        (unsigned)patch->x < (unsigned)WORLD_W && (unsigned)patch->y < (unsigned)WORLD_H &&
        dist2(worker->x, worker->y, patch->x, patch->y) <= MINERAL_WORK_RANGE * MINERAL_WORK_RANGE;
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
            if (!worker->carry && worker_mineral_fifo_at_patch(worker, &patches[i], i)) {
                previous = slot + 1u;
                continue;
            }
            if (worker->order == ORDER_MINE && worker->target == i) worker->work_timer = 0;
            sc64_mineral_fifo_unlink(fifo, (unsigned)i, slot, previous);
        }
    }
}

static void
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

/* Encode only scratch rows. Retiring a canceled or depleted job here matches
 * pruning at the start of the uninterrupted world's next orders pass. */
static int
worker_mineral_fifo_encode(void)
{
    prepared_mineral_fifo = mineral_fifo;
    worker_mineral_fifo_prune(&prepared_mineral_fifo, data.units, data.minerals_map);
    for (int i = 0; i < MAX_UNITS; ++i) {
        const Unit *worker = &data.units[i];
        if (worker->alive && IS_WORKER(worker->type) && worker->order == ORDER_MINE &&
            !prepared_mineral_fifo.patch[i] && worker->work_timer) return 0;
    }
    for (int i = 0; i < MAX_MINERALS; ++i) {
        int rank = 1;
        for (unsigned link = prepared_mineral_fifo.head[i]; link;
             link = prepared_mineral_fifo.next[link - 1u], ++rank) {
            Unit *worker = &data.units[link - 1u];
            if (worker->work_timer < 0 || worker->work_timer > MINERAL_SERVICE_TICKS ||
                (rank != 1 && worker->work_timer)) return 0;
            worker->carry = MINERAL_CARGO_MAX + rank;
        }
    }
    return 1;
}

/* Candidate rows retain the original dimensions. Rebuild ranked lists before
 * publishing live state, rejecting duplicate or missing ranks. */
static int
worker_mineral_fifo_decode(void)
{
    u8 queued[MAX_MINERALS] = {0};
    for (int i = 0; i < MAX_UNITS; ++i) {
        const Unit *worker = &data.units[i];
        if (!IS_WORKER(worker->type)) continue;
        if (worker->carry < 0 || worker->carry > MINERAL_CARGO_MAX + MAX_UNITS) return 0;
        if (!worker->alive || worker->order != ORDER_MINE) {
            if (worker->carry > MINERAL_CARGO_MAX) return 0;
            continue;
        }
        if (worker->carry <= MINERAL_CARGO_MAX) {
            if (worker->work_timer) return 0;
            continue;
        }
        int patch = worker->target, rank = worker->carry - MINERAL_CARGO_MAX;
        if ((unsigned)patch >= MAX_MINERALS ||
            !worker_mineral_fifo_at_patch(worker, &data.minerals_map[patch], patch) ||
            worker->work_timer < 0 || worker->work_timer > MINERAL_SERVICE_TICKS ||
            (rank != 1 && worker->work_timer)) return 0;
        ++queued[patch];
    }
    for (int patch = 0; patch < MAX_MINERALS; ++patch) {
        for (int rank = 1; rank <= queued[patch]; ++rank) {
            int found = -1;
            for (int i = 0; i < MAX_UNITS; ++i) {
                const Unit *worker = &data.units[i];
                if (!worker->alive || !IS_WORKER(worker->type) || worker->order != ORDER_MINE ||
                    worker->target != patch || worker->carry != MINERAL_CARGO_MAX + rank)
                    continue;
                if (found >= 0) return 0;
                found = i;
            }
            if (found < 0) return 0;
            if (!sc64_mineral_fifo_append(&prepared_mineral_fifo, (unsigned)patch, (unsigned)found))
                return 0;
            data.units[found].carry = 0;
        }
    }
    return 1;
}

static int raw_game_save_apply(int apply);

static int
worker_mineral_fifo_prepare(const void *record, u32 length, u32 tag, void *context)
{
    (void)context;
    if (record != &data || length != sizeof(data) || data.version != tag ||
        !raw_game_save_apply(0)) return 0;
    memset(&prepared_mineral_fifo, 0, sizeof(prepared_mineral_fifo));
    return tag == SAVE_VERSION ||
        (tag == SC64_MINERAL_FIFO_SAVE_VERSION && worker_mineral_fifo_decode());
}

static int
worker_mineral_fifo_select(GbaSaveSelection *selected)
{
    return save_select_prepared(&data, sizeof(data), mineral_fifo_save_tags, 2,
                                worker_mineral_fifo_prepare, 0, selected);
}
