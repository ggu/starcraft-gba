/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_P8_REQUEST_SETTLEMENT_H
#define SC64_P8_REQUEST_SETTLEMENT_H

#include <stdbool.h>
#include <stdint.h>

#include "sc64_p8_request_queue.h"

/*
 * Caller-selected transitions observed in native queue handlers. This pure
 * service does not decide which transition a P8 caller should select.
 */
typedef enum {
    SC64_P8_SETTLE_ERROR = -1,
    SC64_P8_SETTLE_EMPTY = 0,
    SC64_P8_SETTLE_RETAINED = 1,
    SC64_P8_SETTLE_POPPED = 2
} sc64_p8_settlement_result;

typedef enum {
    SC64_P8_SETTLE_RETAIN = 0,
    SC64_P8_SETTLE_POP_BASE = 1,
    SC64_P8_SETTLE_POP_EXTENDED = 2
} sc64_p8_settlement_outcome;

/*
 * Opaque per-player AI-record words at offsets 0,4,8,C,10,14 from the native
 * record base. Their relationship to the engine's player banks is unknown.
 */
typedef struct {
    uint32_t word0;
    uint32_t word1;
    uint32_t word2;
    uint32_t word3;
    uint32_t word4;
    uint32_t word5;
} sc64_p8_ai_accounting;

/* The native cost helper supplies two words and one byte. */
typedef struct {
    uint32_t first;
    uint32_t second;
    uint8_t third;
} sc64_p8_request_cost;

static inline bool sc64_p8_request_raw_skips_accounting(uint16_t raw_unit) {
    return raw_unit == 44u || raw_unit == 103u || raw_unit == 62u ||
           raw_unit == 68u || raw_unit == 63u;
}

/*
 * Apply an explicit RETAIN, POP_BASE, or POP_EXTENDED outcome. For a pop, the
 * queue head's raw id controls whether costs are required. Queue and
 * accounting are required for every call. RETAIN and EMPTY do not touch either;
 * an invalid call also returns before mutation.
 *
 * The first three accounting words use native saturating subtraction. The
 * extended outcome subtracts the same costs from words 3..5 with uint32_t
 * wraparound. The skip list bypasses all accounting but still pops the head.
 * Costs may be NULL only for RETAIN, EMPTY, or a skipped-raw POP. For a POP,
 * the queue, accounting record, and any required costs must be valid and
 * disjoint. This helper owns no storage and cannot safely validate overlapping
 * caller allocations.
 */
static inline sc64_p8_settlement_result sc64_p8_request_settle(
    sc64_p8_request_queue *queue,
    sc64_p8_ai_accounting *accounting,
    sc64_p8_settlement_outcome outcome,
    const sc64_p8_request_cost *costs) {
    if (queue == NULL || accounting == NULL ||
        queue->count > SC64_P8_REQUEST_QUEUE_CAPACITY ||
        (outcome != SC64_P8_SETTLE_RETAIN &&
         outcome != SC64_P8_SETTLE_POP_BASE &&
         outcome != SC64_P8_SETTLE_POP_EXTENDED)) {
        return SC64_P8_SETTLE_ERROR;
    }
    if (queue->count == 0u) {
        return SC64_P8_SETTLE_EMPTY;
    }
    if (outcome == SC64_P8_SETTLE_RETAIN) {
        return SC64_P8_SETTLE_RETAINED;
    }
    uint16_t raw_unit = queue->entries[0].raw_unit;
    bool skip_accounting = sc64_p8_request_raw_skips_accounting(raw_unit);
    if (!skip_accounting && costs == NULL) {
        return SC64_P8_SETTLE_ERROR;
    }

    /* Snapshot caller inputs before the queue mutates. */
    uint32_t first = skip_accounting ? 0u : costs->first;
    uint32_t second = skip_accounting ? 0u : costs->second;
    uint32_t third = skip_accounting ? 0u : costs->third;
    sc64_p8_ai_accounting next_accounting = *accounting;
    if (!skip_accounting) {
        next_accounting.word0 = next_accounting.word0 >= first ?
                                next_accounting.word0 - first : 0u;
        next_accounting.word1 = next_accounting.word1 >= second ?
                                next_accounting.word1 - second : 0u;
        next_accounting.word2 = next_accounting.word2 >= third ?
                                next_accounting.word2 - third : 0u;
        if (outcome == SC64_P8_SETTLE_POP_EXTENDED) {
            next_accounting.word3 -= first;
            next_accounting.word4 -= second;
            next_accounting.word5 -= third;
        }
    }

    sc64_p8_request removed;
    if (sc64_p8_request_pop(queue, &removed) != SC64_P8_QUEUE_OK) {
        return SC64_P8_SETTLE_ERROR;
    }
    *accounting = next_accounting;
    return SC64_P8_SETTLE_POPPED;
}

#endif
