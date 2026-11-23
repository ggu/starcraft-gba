/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Mineral queue lifecycle and raw journal preparation. Included by game.c. */
#include "sc64_mineral_fifo_rank.h"

#include "sc64_mineral_fifo_runtime.h"
#define SC64_MINERAL_FIFO_SAVE_VERSION \
    ((SAVE_VERSION & ~SC64_SAVE_TAG_NAMESPACE_MASK) | SC64_SAVE_TAG_MINERAL_FIFO_NAMESPACE)
static const u32 mineral_fifo_save_tags[] = {SAVE_VERSION, SC64_MINERAL_FIFO_SAVE_VERSION};
static Sc64MineralFifo prepared_mineral_fifo;

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
    int has_ranks = 0;
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
            !worker_mineral_fifo_matches_patch(worker, &data.minerals_map[patch], patch,
                                               WORLD_W, WORLD_H) ||
            !sc64_mineral_fifo_stage_rank(&prepared_mineral_fifo, (unsigned)i,
                (unsigned)patch, (unsigned)rank, (unsigned)worker->work_timer,
                MINERAL_SERVICE_TICKS)) return 0;
        has_ranks = 1;
    }
    if (!has_ranks) return 1;
    if (!sc64_mineral_fifo_finish_ranks(&prepared_mineral_fifo)) return 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (prepared_mineral_fifo.patch[i]) data.units[i].carry = 0;
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
