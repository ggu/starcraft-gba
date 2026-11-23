/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_PACKED_MINERAL_FIFO_SAVE_H
#define SC64_PACKED_MINERAL_FIFO_SAVE_H

#include "sc64_mineral_fifo_rank.h"
#include "sc64_race_save_unit_edit.h"
#include "sc64_save_decode_context.h"
#include "sc64_stage_save_auth.h"

enum {
    SC64_FIFO_PACKED_CARGO_BITS = 4,
    SC64_FIFO_PACKED_CARGO_MASK = (1 << SC64_FIFO_PACKED_CARGO_BITS) - 1,
    SC64_FIFO_PACKED_CARGO_MAX = 8,
    SC64_FIFO_PACKED_SERVICE_TICKS = 45
};

/* The caller owns a temporary row and has already selected its ranked job. */
static inline int
sc64_packed_mineral_fifo_rank_unit(Unit *worker, unsigned rank)
{
    if (!worker || !worker->alive || !IS_WORKER(worker->type) ||
        worker->order != ORDER_MINE || (unsigned)worker->target >= MAX_MINERALS ||
        !rank || rank > MAX_UNITS ||
        (unsigned)worker->work_timer > SC64_FIFO_PACKED_SERVICE_TICKS ||
        (rank != 1 && worker->work_timer) ||
        (unsigned)worker->carry > SC64_FIFO_PACKED_CARGO_MAX ||
        !sc64_worker_cargo_state_valid(worker) ||
        sc64_worker_cargo_matches_order(worker)) return 0;
    worker->carry = (int)((rank << SC64_FIFO_PACKED_CARGO_BITS) | (unsigned)worker->carry);
    return 1;
}

/* The caller supplies the authenticated payload and its contextual service
 * check. Rows and the reconstructed queue are scratch until selection and
 * world load both succeed. Every retry starts from a fresh queue. */
static inline int
race_save_prepare_mineral_fifo(RaceSave *candidate, unsigned width,
                               unsigned height, const Sc64StageSaveAuth *auth,
                               Sc64MineralFifo *queue,
                               int (*eligible)(const RaceSave *, unsigned,
                                               const Unit *, unsigned,
                                               unsigned) SC64_GCTX_DECL)
{
    if (!queue) return 0;
    memset(queue, 0, sizeof(*queue));
    if (!candidate || !eligible ||
        !sc64_stage_save_auth_matches(auth, candidate, candidate->version,
                                      SC64_STAGE_SAVE_MINERAL_FIFO)) return 0;
    for (unsigned slot = 0; slot < MAX_UNITS; ++slot) {
        Unit worker;
        if (!SC64_GUARD_DECODE(candidate, slot, width, height, &worker)) return 0;
        if (!worker.alive || !IS_WORKER(worker.type) || worker.order != ORDER_MINE)
            continue;
        unsigned word = (unsigned)worker.carry;
        if (word <= SC64_FIFO_PACKED_CARGO_MAX) {
            if (worker.work_timer || !sc64_worker_cargo_state_valid(&worker)) return 0;
            continue;
        }
        unsigned rank = word >> SC64_FIFO_PACKED_CARGO_BITS;
        unsigned patch = (unsigned)worker.target;
        worker.carry = (int)(word & SC64_FIFO_PACKED_CARGO_MASK);
        if (!rank || rank > MAX_UNITS || patch >= MAX_MINERALS ||
            (unsigned)worker.carry > SC64_FIFO_PACKED_CARGO_MAX ||
            !sc64_worker_cargo_state_valid(&worker) ||
            sc64_worker_cargo_matches_order(&worker) ||
            !eligible(candidate, slot, &worker, width, height) ||
            !sc64_mineral_fifo_stage_rank(queue, slot, patch, rank,
                                         (unsigned)worker.work_timer,
                                         SC64_FIFO_PACKED_SERVICE_TICKS) ||
            !race_save_encode_unit(candidate, slot, width, height, &worker)) return 0;
    }
    return sc64_mineral_fifo_finish_ranks(queue);
}

#endif
