/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "sc64_p8_request_settlement.h"

static sc64_p8_request make_request(uint8_t priority, uint16_t raw) {
    sc64_p8_request request = {priority, 3u, raw, 0xabc00000u | raw};
    return request;
}

static bool oracle_skips_accounting(uint16_t raw) {
    static const uint16_t native_skip_cases[] = {44u, 103u, 62u, 68u, 63u};
    for (size_t i = 0; i < sizeof(native_skip_cases) /
                            sizeof(native_skip_cases[0]); ++i) {
        if (raw == native_skip_cases[i]) {
            return true;
        }
    }
    return false;
}

static uint32_t boundary_word(size_t index) {
    static const uint32_t values[] = {
        0u, 1u, 0x7fffffffu, 0x80000000u, UINT32_MAX, 255u, 256u
    };
    return values[index % (sizeof(values) / sizeof(values[0]))];
}

static uint8_t boundary_byte(size_t index) {
    static const uint8_t values[] = {0u, 1u, 127u, 128u, 255u};
    return values[index % (sizeof(values) / sizeof(values[0]))];
}

static sc64_p8_ai_accounting boundary_accounting(uint32_t step) {
    return (sc64_p8_ai_accounting){
        boundary_word(step), boundary_word(step + 1u),
        boundary_word(step + 2u), boundary_word(step + 3u),
        boundary_word(step + 4u), boundary_word(step + 5u)
    };
}

static void assert_same_live_queue(const sc64_p8_request_queue *a,
                                   const sc64_p8_request_queue *b) {
    assert(a->count == b->count);
    for (size_t i = 0; i < a->count; ++i) {
        assert(a->entries[i].priority == b->entries[i].priority);
        assert(a->entries[i].category == b->entries[i].category);
        assert(a->entries[i].raw_unit == b->entries[i].raw_unit);
        assert(a->entries[i].payload == b->entries[i].payload);
    }
}

static void raw_skip_pop_vectors(void) {
    static const uint16_t skipped[] = {44u, 103u, 62u, 68u, 63u};
    for (size_t i = 0; i < sizeof(skipped) / sizeof(skipped[0]); ++i) {
        for (int mode = 0; mode < 2; ++mode) {
            sc64_p8_request_queue queue = {0};
            sc64_p8_ai_accounting accounting = {
                3u, 5u, 7u, 11u, 13u, 17u
            };
            sc64_p8_ai_accounting before = accounting;
            assert(sc64_p8_request_enqueue(
                       &queue, make_request(100u, skipped[i]), true) ==
                   SC64_P8_QUEUE_INSERTED);
            assert(sc64_p8_request_settle(
                       &queue, &accounting,
                       mode == 0 ? SC64_P8_SETTLE_POP_BASE :
                                   SC64_P8_SETTLE_POP_EXTENDED,
                       NULL) == SC64_P8_SETTLE_POPPED);
            assert(queue.count == 0u);
            assert(memcmp(&accounting, &before, sizeof(before)) == 0);
        }
    }
}

static void exhaustive_raw_skip_table(void) {
    for (uint32_t raw = 0; raw <= UINT16_MAX; ++raw) {
        assert(sc64_p8_request_raw_skips_accounting((uint16_t)raw) ==
               oracle_skips_accounting((uint16_t)raw));
    }
}

static void accounting_vectors(void) {
    sc64_p8_request_queue queue = {0};
    sc64_p8_ai_accounting accounting = {
        5u, 7u, 1u, 0xfffffff0u, 0u, 0u
    };
    const sc64_p8_request_cost costs = {5u, 8u, 2u};
    assert(sc64_p8_request_enqueue(&queue, make_request(99u, 11u), true) ==
           SC64_P8_QUEUE_INSERTED);

    assert(sc64_p8_request_settle(&queue, &accounting,
                                  SC64_P8_SETTLE_POP_BASE, &costs) ==
           SC64_P8_SETTLE_POPPED);
    assert(queue.count == 0u);
    assert(accounting.word0 == 0u && accounting.word1 == 0u &&
           accounting.word2 == 0u);
    assert(accounting.word3 == 0xfffffff0u && accounting.word4 == 0u &&
           accounting.word5 == 0u);

    queue = (sc64_p8_request_queue){0};
    accounting = (sc64_p8_ai_accounting){
        6u, 8u, 2u, 0u, 2u, 1u
    };
    const sc64_p8_request_cost wrap_costs = {7u, 3u, 255u};
    assert(sc64_p8_request_enqueue(&queue, make_request(99u, 12u), true) ==
           SC64_P8_QUEUE_INSERTED);
    assert(sc64_p8_request_settle(&queue, &accounting,
                                  SC64_P8_SETTLE_POP_EXTENDED,
                                  &wrap_costs) == SC64_P8_SETTLE_POPPED);
    assert(accounting.word0 == 0u && accounting.word1 == 5u &&
           accounting.word2 == 0u);
    assert(accounting.word3 == UINT32_MAX - 6u);
    assert(accounting.word4 == UINT32_MAX - 0u);
    assert(accounting.word5 == UINT32_MAX - 253u);
}

static void retention_and_atomic_error_vectors(void) {
    sc64_p8_request_queue queue = {0};
    sc64_p8_ai_accounting accounting = {1u, 2u, 3u, 4u, 5u, 6u};
    assert(sc64_p8_request_enqueue(&queue, make_request(50u, 17u), true) ==
           SC64_P8_QUEUE_INSERTED);
    sc64_p8_request_queue before_queue = queue;
    sc64_p8_ai_accounting before_accounting = accounting;

    assert(sc64_p8_request_settle(&queue, &accounting,
                                  SC64_P8_SETTLE_RETAIN, NULL) ==
           SC64_P8_SETTLE_RETAINED);
    assert_same_live_queue(&queue, &before_queue);
    assert(memcmp(&accounting, &before_accounting, sizeof(accounting)) == 0);

    assert(sc64_p8_request_settle(&queue, NULL,
                                  SC64_P8_SETTLE_RETAIN, NULL) ==
           SC64_P8_SETTLE_ERROR);
    assert_same_live_queue(&queue, &before_queue);

    assert(sc64_p8_request_settle(&queue, &accounting,
                                  (sc64_p8_settlement_outcome)99, NULL) ==
           SC64_P8_SETTLE_ERROR);
    assert_same_live_queue(&queue, &before_queue);
    assert(memcmp(&accounting, &before_accounting, sizeof(accounting)) == 0);

    assert(sc64_p8_request_settle(&queue, &accounting,
                                  SC64_P8_SETTLE_POP_BASE, NULL) ==
           SC64_P8_SETTLE_ERROR);
    assert_same_live_queue(&queue, &before_queue);
    assert(memcmp(&accounting, &before_accounting, sizeof(accounting)) == 0);

    assert(sc64_p8_request_settle(&queue, NULL,
                                  SC64_P8_SETTLE_POP_BASE,
                                  &(sc64_p8_request_cost){1u, 2u, 3u}) ==
           SC64_P8_SETTLE_ERROR);
    assert_same_live_queue(&queue, &before_queue);

    sc64_p8_request_queue empty = {0};
    assert(sc64_p8_request_settle(&empty, NULL,
                                  SC64_P8_SETTLE_POP_EXTENDED, NULL) ==
           SC64_P8_SETTLE_ERROR);
    assert(empty.count == 0u);

    assert(sc64_p8_request_settle(&empty, &accounting,
                                  SC64_P8_SETTLE_POP_EXTENDED, NULL) ==
           SC64_P8_SETTLE_EMPTY);

    queue.count = SC64_P8_REQUEST_QUEUE_CAPACITY + 1u;
    assert(sc64_p8_request_settle(&queue, &accounting,
                                  SC64_P8_SETTLE_RETAIN, NULL) ==
           SC64_P8_SETTLE_ERROR);
    assert(queue.count == SC64_P8_REQUEST_QUEUE_CAPACITY + 1u);
    assert(memcmp(&accounting, &before_accounting, sizeof(accounting)) == 0);
}

static uint32_t next_random(uint32_t *state) {
    uint32_t x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return *state = x;
}

static uint32_t saturating_sub(uint32_t value, uint32_t cost) {
    uint64_t wide_value = value;
    uint64_t wide_cost = cost;
    if (wide_cost > wide_value) {
        return 0u;
    }
    uint64_t difference = wide_value - wide_cost;
    assert(difference <= UINT32_MAX);
    return (uint32_t)difference;
}

static uint32_t wrapping_sub(uint32_t value, uint32_t cost) {
    return (uint32_t)((uint64_t)value - (uint64_t)cost);
}

static void random_settlement_oracle(void) {
    sc64_p8_request_queue queue = {0};
    sc64_p8_request_queue oracle_queue = {0};
    sc64_p8_ai_accounting accounting = {0};
    sc64_p8_ai_accounting oracle = {0};
    uint32_t state = 0x98a31f27u;

    for (uint32_t step = 0; step < 20000u; ++step) {
        uint32_t bits = next_random(&state);
        unsigned op = bits % 5u;
        if (op < 2u) {
            sc64_p8_request item = make_request((uint8_t)(bits >> 24),
                                               (uint16_t)(bits >> 8));
            assert(sc64_p8_request_enqueue(&queue, item, true) ==
                   sc64_p8_request_enqueue(&oracle_queue, item, true));
        } else {
            sc64_p8_settlement_result actual;
            if (op == 2u) {
                actual = sc64_p8_request_settle(
                    &queue, &accounting, SC64_P8_SETTLE_RETAIN, NULL);
                if (queue.count == 0u) {
                    assert(actual == SC64_P8_SETTLE_EMPTY);
                } else {
                    assert(actual == SC64_P8_SETTLE_RETAINED);
                }
            } else if (queue.count == 0u) {
                actual = sc64_p8_request_settle(
                    &queue, &accounting,
                    op == 3u ? SC64_P8_SETTLE_POP_BASE :
                               SC64_P8_SETTLE_POP_EXTENDED,
                    NULL);
                assert(actual == SC64_P8_SETTLE_EMPTY);
            } else {
                uint16_t raw = queue.entries[0].raw_unit;
                accounting = boundary_accounting(step);
                oracle = accounting;
                sc64_p8_request_cost costs = {
                    boundary_word(step + 6u), boundary_word(step + 7u),
                    boundary_byte(step)
                };
                sc64_p8_settlement_outcome outcome =
                    op == 3u ? SC64_P8_SETTLE_POP_BASE :
                               SC64_P8_SETTLE_POP_EXTENDED;
                actual = sc64_p8_request_settle(&queue, &accounting,
                                                outcome, &costs);
                assert(actual == SC64_P8_SETTLE_POPPED);

                sc64_p8_request discarded;
                assert(sc64_p8_request_pop(&oracle_queue, &discarded) ==
                       SC64_P8_QUEUE_OK);
                if (!oracle_skips_accounting(raw)) {
                    oracle.word0 = saturating_sub(oracle.word0, costs.first);
                    oracle.word1 = saturating_sub(oracle.word1, costs.second);
                    oracle.word2 = saturating_sub(oracle.word2, costs.third);
                    if (outcome == SC64_P8_SETTLE_POP_EXTENDED) {
                        oracle.word3 = wrapping_sub(oracle.word3,
                                                    costs.first);
                        oracle.word4 = wrapping_sub(oracle.word4,
                                                    costs.second);
                        oracle.word5 = wrapping_sub(oracle.word5,
                                                    costs.third);
                    }
                }
            }
        }
        assert_same_live_queue(&queue, &oracle_queue);
        assert(memcmp(&accounting, &oracle, sizeof(accounting)) == 0);
    }
}

int main(void) {
    exhaustive_raw_skip_table();
    raw_skip_pop_vectors();
    accounting_vectors();
    retention_and_atomic_error_vectors();
    random_settlement_oracle();
    puts("sc64_p8_request_settlement: caller outcomes and AI-record accounting pass");
    return 0;
}
