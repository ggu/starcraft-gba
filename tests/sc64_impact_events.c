/* SPDX-License-Identifier: GPL-3.0-or-later
 * Asset-free contract for the bounded launch-to-impact queue. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/sc64_impact_events.h"

static Sc64ImpactHit hit_of(int target, int damage, int hits, int owner,
                            int damage_class, int type)
{
    Sc64ImpactHit h = { target, damage, hits, owner, damage_class, type };
    return h;
}

static void test_pack_round_trip(void)
{
    const Sc64ImpactHit cases[] = {
        hit_of(0, 0, 1, 0, -1, 0),
        hit_of(511, 4095, 16, 7, 5, 63),
        hit_of(239, 20, 2, 1, 1, 11),
        hit_of(400, 9, 1, 4, 3, 20),
    };
    for (unsigned i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        Sc64ImpactEvent e;
        Sc64ImpactHit out;
        assert(sc64_impact_pack(&e, &cases[i], 1u + i * 80u));
        sc64_impact_unpack(&e, &out);
        assert(!memcmp(&out, &cases[i], sizeof(out)));
        assert(e.steps == 1u + i * 80u);
    }
    /* Out-of-range packets are refused, never truncated. */
    const Sc64ImpactHit bad[] = {
        hit_of(-1, 1, 1, 0, 0, 0), hit_of(512, 1, 1, 0, 0, 0),
        hit_of(1, -1, 1, 0, 0, 0), hit_of(1, 4096, 1, 0, 0, 0),
        hit_of(1, 1, 0, 0, 0, 0), hit_of(1, 1, 17, 0, 0, 0),
        hit_of(1, 1, 1, -1, 0, 0), hit_of(1, 1, 1, 8, 0, 0),
        hit_of(1, 1, 1, 0, -2, 0), hit_of(1, 1, 1, 0, 7, 0),
        hit_of(1, 1, 1, 0, 0, -1), hit_of(1, 1, 1, 0, 0, 64),
    };
    for (unsigned i = 0; i < sizeof(bad) / sizeof(bad[0]); ++i) {
        Sc64ImpactEvent e;
        assert(!sc64_impact_pack(&e, &bad[i], 5));
    }
    Sc64ImpactEvent e;
    Sc64ImpactHit ok = hit_of(1, 1, 1, 0, 0, 0);
    assert(!sc64_impact_pack(&e, &ok, 0));
    assert(!sc64_impact_pack(&e, &ok, 256));
}

/* Rows resolve exactly on their step, in launch order, and survivors keep
 * their order. */
static void test_step_order(void)
{
    Sc64ImpactEvent rows[8], due[8];
    uint8_t count = 0;
    const unsigned steps[] = { 3, 1, 3, 2, 1 };
    for (unsigned i = 0; i < 5; ++i) {
        Sc64ImpactHit h = hit_of((int)i, (int)(10 + i), 1, 0, 0, 1);
        assert(sc64_impact_push(rows, 8, &count, &h, steps[i]) == SC64_IMPACT_QUEUED);
    }
    unsigned n = sc64_impact_step(rows, &count, due);
    assert(n == 2 && count == 3);
    assert(sc64_impact_event_target(&due[0]) == 1 && sc64_impact_event_target(&due[1]) == 4);
    n = sc64_impact_step(rows, &count, due);
    assert(n == 1 && sc64_impact_event_target(&due[0]) == 3 && count == 2);
    assert(sc64_impact_event_target(&rows[0]) == 0 && sc64_impact_event_target(&rows[1]) == 2);
    n = sc64_impact_step(rows, &count, due);
    assert(n == 2 && count == 0);
    assert(sc64_impact_event_target(&due[0]) == 0 && sc64_impact_event_target(&due[1]) == 2);
    assert(sc64_impact_step(rows, &count, due) == 0);
}

/* A full queue refuses the launch and leaves every queued row untouched. */
static void test_overflow_is_refusal(void)
{
    Sc64ImpactEvent rows[4], snapshot[4];
    uint8_t count = 0;
    for (int i = 0; i < 4; ++i) {
        Sc64ImpactHit h = hit_of(i, 5, 1, 1, 0, 2);
        assert(sc64_impact_push(rows, 4, &count, &h, 9) == SC64_IMPACT_QUEUED);
    }
    memcpy(snapshot, rows, sizeof(rows));
    Sc64ImpactHit extra = hit_of(9, 5, 1, 1, 0, 2);
    assert(sc64_impact_push(rows, 4, &count, &extra, 1) == SC64_IMPACT_FULL);
    assert(count == 4 && !memcmp(rows, snapshot, sizeof(rows)));
    Sc64ImpactHit bad = hit_of(9, 5000, 1, 1, 0, 2);
    assert(sc64_impact_push(rows, 4, &count, &bad, 1) == SC64_IMPACT_UNREPRESENTABLE);
    assert(count == 4);
}

static void test_clear_target(void)
{
    Sc64ImpactEvent rows[6];
    uint8_t count = 0;
    const int targets[] = { 7, 3, 7, 5, 7, 2 };
    for (int i = 0; i < 6; ++i) {
        Sc64ImpactHit h = hit_of(targets[i], i, 1, 0, 0, 0);
        assert(sc64_impact_push(rows, 6, &count, &h, 4) == SC64_IMPACT_QUEUED);
    }
    assert(sc64_impact_clear_target(rows, &count, 7) == 3 && count == 3);
    Sc64ImpactHit h;
    sc64_impact_unpack(&rows[0], &h); assert(h.target == 3 && h.damage == 1);
    sc64_impact_unpack(&rows[1], &h); assert(h.target == 5 && h.damage == 3);
    sc64_impact_unpack(&rows[2], &h); assert(h.target == 2 && h.damage == 5);
    assert(sc64_impact_clear_target(rows, &count, 99) == 0 && count == 3);
}

static void test_save_codec(void)
{
    Sc64ImpactEvent rows[4], candidate[4];
    uint8_t count = 0;
    uint8_t block[64];
    for (int i = 0; i < 3; ++i) {
        Sc64ImpactHit h = hit_of(100 + i, 20 + i, 1 + i, i, i - 1, 10 + i);
        assert(sc64_impact_push(rows, 4, &count, &h, 3u + (unsigned)i) == SC64_IMPACT_QUEUED);
    }
    uint32_t bytes = sc64_impact_encode(rows, count, block);
    assert(bytes == 8u + 3u * 6u && bytes == sc64_impact_save_bytes(3));
    assert(sc64_impact_decode(block, bytes, 4, 400, 5, 64, candidate) == 3);
    assert(!memcmp(candidate, rows, 3 * sizeof(rows[0])));

    uint8_t bad[64];
    /* Truncated, padded, oversized and wrong-count blocks. */
    assert(sc64_impact_decode(block, bytes - 1, 4, 400, 5, 64, candidate) < 0);
    assert(sc64_impact_decode(block, bytes + 6, 4, 400, 5, 64, candidate) < 0);
    assert(sc64_impact_decode(block, bytes, 2, 400, 5, 64, candidate) < 0);
    assert(sc64_impact_decode(block, 4, 4, 400, 5, 64, candidate) < 0);
    memcpy(bad, block, bytes); bad[0] ^= 1;
    assert(sc64_impact_decode(bad, bytes, 4, 400, 5, 64, candidate) < 0);
    memcpy(bad, block, bytes); bad[4] = 0;
    assert(sc64_impact_decode(bad, 8, 4, 400, 5, 64, candidate) < 0);
    memcpy(bad, block, bytes); bad[6] = 1;
    assert(sc64_impact_decode(bad, bytes, 4, 400, 5, 64, candidate) < 0);
    /* Row fields: zero steps, target, owner, class and type limits. */
    memcpy(bad, block, bytes); bad[8 + 4] = 0;
    assert(sc64_impact_decode(bad, bytes, 4, 400, 5, 64, candidate) < 0);
    assert(sc64_impact_decode(block, bytes, 4, 101, 5, 64, candidate) < 0);
    assert(sc64_impact_decode(block, bytes, 4, 400, 2, 64, candidate) < 0);
    assert(sc64_impact_decode(block, bytes, 4, 400, 5, 12, candidate) < 0);
    memcpy(bad, block, bytes); bad[8 + 1] |= 0x80; /* spare class bit */
    assert(sc64_impact_decode(bad, bytes, 4, 400, 5, 64, candidate) < 0);
}

int main(void)
{
    test_pack_round_trip();
    test_step_order();
    test_overflow_is_refusal();
    test_clear_target();
    test_save_codec();
    puts("Bounded launch-to-impact queue contract: PASS");
    return 0;
}
