/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "sc64_p8_request_queue.h"

_Static_assert(sizeof(sc64_p8_request) == 8u,
               "native request entries occupy 8 bytes");

static sc64_p8_request request(uint8_t priority, uint8_t category,
                               uint16_t raw, uint32_t payload) {
    sc64_p8_request value = {priority, category, raw, payload};
    return value;
}

static void assert_heap_valid(const sc64_p8_request_queue *queue) {
    assert(queue->count <= SC64_P8_REQUEST_QUEUE_CAPACITY);
    for (size_t i = 1; i < queue->count; ++i) {
        size_t parent = (i - 1u) / 2u;
        assert(queue->entries[parent].priority >= queue->entries[i].priority);
    }
}

static void native_insert_pop_vector(void) {
    static const sc64_p8_request input[] = {
        {30, 3, 101, 0xa5000000u},
        {80, 3, 102, 0xa5000001u},
        {150, 3, 103, 0xa5000002u},
        {80, 3, 104, 0xa5000003u},
        {0, 3, 105, 0xa5000004u},
        {255, 3, 106, 0xa5000005u},
    };
    /* Captured by synthetic execution of native 8004AB20/8004ACA0. */
    static const uint8_t expected_priority[] = {255, 150, 80, 80, 30, 0};
    static const uint16_t expected_raw[] = {106, 103, 102, 104, 101, 105};
    static const uint32_t expected_payload[] = {
        0xa5000005u, 0xa5000002u, 0xa5000001u,
        0xa5000003u, 0xa5000000u, 0xa5000004u,
    };
    sc64_p8_request_queue queue;
    sc64_p8_request_queue_init(&queue);
    assert(queue.count == 0u);

    for (size_t i = 0; i < sizeof(input) / sizeof(input[0]); ++i) {
        assert(sc64_p8_request_enqueue(&queue, input[i], true) ==
               SC64_P8_QUEUE_INSERTED);
        assert(queue.count == i + 1u);
        assert_heap_valid(&queue);
    }
    const sc64_p8_request *head = sc64_p8_request_peek(&queue);
    assert(head != NULL && head->priority == 255u && head->raw_unit == 106u);
    assert(head->category == 3u && head->payload == 0xa5000005u);
    assert(queue.count == 6u); /* failed build attempts can retain the head */

    for (size_t i = 0; i < sizeof(expected_priority); ++i) {
        sc64_p8_request popped = {0};
        assert(sc64_p8_request_pop(&queue, &popped) == SC64_P8_QUEUE_OK);
        assert(popped.priority == expected_priority[i]);
        assert(popped.raw_unit == expected_raw[i]);
        assert(popped.payload == expected_payload[i]);
        assert_heap_valid(&queue);
    }
    assert(queue.count == 0u);
}

static void saturated_native_tail_replacement(void) {
    sc64_p8_request_queue queue;
    sc64_p8_request_queue_init(&queue);
    for (uint16_t raw = 0; raw < SC64_P8_REQUEST_QUEUE_CAPACITY; ++raw) {
        assert(sc64_p8_request_enqueue(&queue,
                   request((uint8_t)raw, 3, (uint16_t)(1000u + raw), raw), true)
               == SC64_P8_QUEUE_INSERTED);
    }
    assert(queue.count == 63u);
    assert_heap_valid(&queue);
    /* Native behavior at capacity replaces array slot 62 without growing. */
    assert(sc64_p8_request_enqueue(&queue, request(1, 3, 9999, 0xfeedu), true)
           == SC64_P8_QUEUE_REPLACED_TAIL);
    assert(queue.count == 63u);
    assert_heap_valid(&queue);

    /* Native replaces old tail request 57, even though it is not minimum. */
    /* Full native synthetic vector produced these final pops. */
    sc64_p8_request popped = {0};
    for (uint8_t priority = 62; priority >= 58; --priority) {
        assert(sc64_p8_request_pop(&queue, &popped) == SC64_P8_QUEUE_OK);
        assert(popped.priority == priority);
        assert(popped.raw_unit == (uint16_t)(1000u + priority));
        assert_heap_valid(&queue);
    }
    for (uint8_t priority = 56; priority >= 2; --priority) {
        assert(sc64_p8_request_pop(&queue, &popped) == SC64_P8_QUEUE_OK);
        assert(popped.priority == priority);
        assert(popped.raw_unit == (uint16_t)(1000u + priority));
        assert_heap_valid(&queue);
    }
    assert(sc64_p8_request_pop(&queue, &popped) == SC64_P8_QUEUE_OK);
    assert(popped.priority == 1u && popped.raw_unit == 9999u);
    assert(popped.payload == 0xfeedu && popped.category == 3u);
    assert_heap_valid(&queue);
    assert(sc64_p8_request_pop(&queue, &popped) == SC64_P8_QUEUE_OK);
    assert(popped.priority == 1u && popped.raw_unit == 1001u);
    assert_heap_valid(&queue);
    assert(sc64_p8_request_pop(&queue, &popped) == SC64_P8_QUEUE_OK);
    assert(popped.priority == 0u && popped.raw_unit == 1000u);
    assert_heap_valid(&queue);
    assert(queue.count == 0u);
}

static void saturated_high_replacement(void) {
    sc64_p8_request_queue queue;
    sc64_p8_request_queue_init(&queue);
    for (uint16_t raw = 0; raw < SC64_P8_REQUEST_QUEUE_CAPACITY; ++raw) {
        assert(sc64_p8_request_enqueue(&queue,
                   request((uint8_t)raw, 3, (uint16_t)(1000u + raw), raw), true)
               == SC64_P8_QUEUE_INSERTED);
    }
    assert(sc64_p8_request_enqueue(&queue, request(255, 3, 9999, 0xbeefu), true)
           == SC64_P8_QUEUE_REPLACED_TAIL);
    assert(queue.count == 63u);
    assert_heap_valid(&queue);

    sc64_p8_request popped = {0};
    assert(sc64_p8_request_pop(&queue, &popped) == SC64_P8_QUEUE_OK);
    assert(popped.priority == 255u && popped.raw_unit == 9999u);
    assert(popped.payload == 0xbeefu && popped.category == 3u);
    assert_heap_valid(&queue);
    for (int priority = 62; priority >= 58; --priority) {
        assert(sc64_p8_request_pop(&queue, &popped) == SC64_P8_QUEUE_OK);
        assert(popped.priority == (uint8_t)priority);
        assert(popped.raw_unit == (uint16_t)(1000 + priority));
        assert_heap_valid(&queue);
    }
    for (int priority = 56; priority >= 0; --priority) {
        assert(sc64_p8_request_pop(&queue, &popped) == SC64_P8_QUEUE_OK);
        assert(popped.priority == (uint8_t)priority);
        assert(popped.raw_unit == (uint16_t)(1000 + priority));
        assert_heap_valid(&queue);
    }
    assert(queue.count == 0u);
}

static void category_gate_matrix(void) {
    /* Native branch coverage: only these four category values consult CDEC. */
    for (unsigned category = 0; category < 256u; ++category) {
        sc64_p8_request_queue queue = {0};
        sc64_p8_request value = request(80, (uint8_t)category, 11, 99);
        int gated = category == 1u || category == 2u ||
                    category == 4u || category == 8u;
        sc64_p8_queue_result result = sc64_p8_request_enqueue(&queue, value, false);
        assert(result == (gated ? SC64_P8_QUEUE_SKIPPED_INELIGIBLE :
                                 SC64_P8_QUEUE_INSERTED));
        assert(queue.count == (gated ? 0u : 1u));
        sc64_p8_request_queue_init(&queue);
        assert(sc64_p8_request_enqueue(&queue, value, true) ==
               SC64_P8_QUEUE_INSERTED);
        assert(queue.count == 1u && queue.entries[0].category == category);
    }

    /* A rejected request at capacity must not overwrite the physical tail. */
    sc64_p8_request_queue full = {0};
    for (unsigned i = 0; i < SC64_P8_REQUEST_QUEUE_CAPACITY; ++i)
        assert(sc64_p8_request_enqueue(&full, request(42, 3, (uint16_t)i, i),
                                       true) == SC64_P8_QUEUE_INSERTED);
    sc64_p8_request_queue before = full;
    for (unsigned category = 1; category <= 8; category *= 2) {
        assert(sc64_p8_request_enqueue(&full,
                   request(255, (uint8_t)category, 900, 800), false) ==
               SC64_P8_QUEUE_SKIPPED_INELIGIBLE);
        assert(memcmp(&before, &full, sizeof(full)) == 0);
    }
}

static void gate_and_empty_contract(void) {
    sc64_p8_request_queue queue;
    sc64_p8_request_queue_init(&queue);
    sc64_p8_request special4 = request(42, 4, 7, 0x12345678u);
    sc64_p8_request special8 = request(41, 8, 9, 0x87654321u);
    sc64_p8_request ordinary = request(40, 3, 11, 0xabcdef01u);

    assert(sc64_p8_request_enqueue(&queue, special4, false) ==
           SC64_P8_QUEUE_SKIPPED_INELIGIBLE);
    assert(queue.count == 0u);
    assert(sc64_p8_request_enqueue(&queue, special8, false) ==
           SC64_P8_QUEUE_SKIPPED_INELIGIBLE);
    assert(queue.count == 0u);
    assert(sc64_p8_request_enqueue(&queue, special4, true) ==
           SC64_P8_QUEUE_INSERTED);
    assert(queue.count == 1u);
    queue.count = 0u;
    /* Categories other than 1, 2, 4 and 8 bypass the native eligibility gate. */
    assert(sc64_p8_request_enqueue(&queue, ordinary, false) ==
           SC64_P8_QUEUE_INSERTED);

    sc64_p8_request before = queue.entries[0];
    assert(sc64_p8_request_pop(&queue, NULL) == SC64_P8_QUEUE_ERROR);
    assert(queue.count == 1u);
    assert(memcmp(&before, &queue.entries[0], sizeof(before)) == 0);

    sc64_p8_request popped = {0};
    assert(sc64_p8_request_pop(&queue, &popped) == SC64_P8_QUEUE_OK);
    assert(popped.raw_unit == ordinary.raw_unit);
    assert(sc64_p8_request_pop(&queue, &popped) == SC64_P8_QUEUE_EMPTY);
    assert(queue.count == 0u); /* safe wrapper; raw native pop assumes nonempty */

    queue.count = SC64_P8_REQUEST_QUEUE_CAPACITY + 1u;
    assert(sc64_p8_request_enqueue(&queue, ordinary, true) == SC64_P8_QUEUE_ERROR);
    assert(sc64_p8_request_peek(&queue) == NULL);
    assert(sc64_p8_request_pop(&queue, &popped) == SC64_P8_QUEUE_ERROR);
    assert(queue.count == SC64_P8_REQUEST_QUEUE_CAPACITY + 1u);
}

int main(void) {
    native_insert_pop_vector();
    saturated_native_tail_replacement();
    saturated_high_replacement();
    category_gate_matrix();
    gate_and_empty_contract();
    puts("sc64_p8_request_queue: native heap vectors and safe boundaries pass");
    return 0;
}
