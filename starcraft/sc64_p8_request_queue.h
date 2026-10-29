/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_P8_REQUEST_QUEUE_H
#define SC64_P8_REQUEST_QUEUE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * Fixed-capacity storage for the native source-script request heap.
 *
 * Native evidence: 8004AB20 stores an 8-byte request (priority, category,
 * raw unit id, opaque 32-bit payload) in a max heap and saturates at 63
 * entries. 8004ACA0 removes the highest-priority entry. Category eligibility
 * for native categories 1, 2, 4 and 8 is checked by the caller before insertion;
 * the heap deliberately does not implement that game-state query.
 *
 * When full, native insertion replaces array slot 62 and sifts it upward.
 * This preserves the heap invariant for valid heap contents but discards the
 * previous slot-62 request. Equal priorities do not bubble during insertion;
 * during removal the right child wins a child-priority tie. Equal-priority
 * ordering is therefore heap-operation order, not FIFO or stable ordering.
 *
 * Native pop assumes a nonempty queue (an empty native pop underflows its
 * count byte). This wrapper returns EMPTY without mutation for empty queues.
 * Queue storage alone does not define whether an action should retain or pop a
 * request. Caller-specific outcomes and AI-record accounting are modeled by
 * sc64_p8_request_settlement.h; no gameplay caller is wired to that service.
 */
#define SC64_P8_REQUEST_QUEUE_CAPACITY 63u

typedef struct {
    uint8_t priority;
    uint8_t category;
    uint16_t raw_unit;
    uint32_t payload;
} sc64_p8_request;

_Static_assert(sizeof(sc64_p8_request) == 8u,
               "request records must stay eight bytes");

typedef struct {
    size_t count;
    sc64_p8_request entries[SC64_P8_REQUEST_QUEUE_CAPACITY];
} sc64_p8_request_queue;

typedef enum {
    SC64_P8_QUEUE_ERROR = -1,
    SC64_P8_QUEUE_EMPTY = 0,
    SC64_P8_QUEUE_INSERTED = 1,
    SC64_P8_QUEUE_OK = 1,
    SC64_P8_QUEUE_REPLACED_TAIL = 2,
    SC64_P8_QUEUE_SKIPPED_INELIGIBLE = 3
} sc64_p8_queue_result;

static inline void sc64_p8_request_queue_init(sc64_p8_request_queue *queue) {
    if (queue != NULL) {
        queue->count = 0;
    }
}

/*
 * `native_gate_allows` is consulted only for categories 1, 2, 4 and 8. The native
 * game-state eligibility routine is outside this storage API. Other category
 * values bypass that native gate, matching 8004AB20.
 */
static inline sc64_p8_queue_result sc64_p8_request_enqueue(
    sc64_p8_request_queue *queue,
    sc64_p8_request request,
    bool native_gate_allows) {
    if (queue == NULL || queue->count > SC64_P8_REQUEST_QUEUE_CAPACITY) {
        return SC64_P8_QUEUE_ERROR;
    }
    if ((request.category == 1u || request.category == 2u ||
         request.category == 4u || request.category == 8u) &&
        !native_gate_allows) {
        return SC64_P8_QUEUE_SKIPPED_INELIGIBLE;
    }

    size_t index;
    sc64_p8_queue_result result;
    if (queue->count == SC64_P8_REQUEST_QUEUE_CAPACITY) {
        index = SC64_P8_REQUEST_QUEUE_CAPACITY - 1u;
        result = SC64_P8_QUEUE_REPLACED_TAIL;
    } else {
        index = queue->count++;
        result = SC64_P8_QUEUE_INSERTED;
    }

    queue->entries[index] = request;
    while (index != 0u) {
        size_t parent = (index - 1u) / 2u;
        if (queue->entries[parent].priority >= queue->entries[index].priority) {
            break;
        }
        sc64_p8_request swap = queue->entries[parent];
        queue->entries[parent] = queue->entries[index];
        queue->entries[index] = swap;
        index = parent;
    }
    return result;
}

static inline const sc64_p8_request *sc64_p8_request_peek(
    const sc64_p8_request_queue *queue) {
    if (queue == NULL || queue->count == 0u ||
        queue->count > SC64_P8_REQUEST_QUEUE_CAPACITY) {
        return NULL;
    }
    return &queue->entries[0];
}

static inline sc64_p8_queue_result sc64_p8_request_pop(
    sc64_p8_request_queue *queue,
    sc64_p8_request *out_request) {
    if (queue == NULL || out_request == NULL ||
        queue->count > SC64_P8_REQUEST_QUEUE_CAPACITY) {
        return SC64_P8_QUEUE_ERROR;
    }
    if (queue->count == 0u) {
        return SC64_P8_QUEUE_EMPTY;
    }

    *out_request = queue->entries[0];
    --queue->count;
    if (queue->count == 0u) {
        return SC64_P8_QUEUE_OK;
    }

    queue->entries[0] = queue->entries[queue->count];
    size_t index = 0u;
    for (;;) {
        size_t left = index * 2u + 1u;
        if (left >= queue->count) {
            break;
        }
        size_t right = left + 1u;
        size_t child = left;
        if (right < queue->count &&
            queue->entries[right].priority >= queue->entries[left].priority) {
            child = right;
        }
        if (queue->entries[index].priority >= queue->entries[child].priority) {
            break;
        }
        sc64_p8_request swap = queue->entries[index];
        queue->entries[index] = queue->entries[child];
        queue->entries[child] = swap;
        index = child;
    }
    return SC64_P8_QUEUE_OK;
}

#endif
