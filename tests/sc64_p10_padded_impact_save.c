/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define MAX_UNITS 240
#define MAX_BUILDINGS 160
#define SC_RU_COUNT 64
typedef struct { uint8_t bytes[28]; } UnitRow;
typedef struct { uint8_t bytes[25]; } BuildingRow;
typedef struct {
    uint32_t prefix;
    UnitRow units[MAX_UNITS];
    BuildingRow buildings[MAX_BUILDINGS];
    uint8_t sentinel[64];
} RaceSave;

#include "../starcraft/sc64_p10_padded_impact_payload.h"

static RaceSave original, wire, bad;
static uint32_t state = 0x13579bdfu;

static uint32_t random_word(void)
{
    state = state * 1664525u + 1013904223u;
    return state ^ (state >> 16);
}

static void prepare(void)
{
    memset(&original, 0, sizeof(original));
    original.prefix = random_word();
    for (unsigned i = 0; i < MAX_UNITS; ++i) {
        if (i % 3 == 0) continue;
        uint8_t *p = original.units[i].bytes;
        for (unsigned j = 0; j < sizeof(original.units[i].bytes); ++j)
            p[j] = (uint8_t)random_word();
        sc64_p10_pi_put(p, 40, 3, random_word() % 5);
        sc64_p10_pi_put(p, 146, 5, random_word() % 25);
        sc64_p10_pi_put(p, 151, 4, random_word() % 13);
        sc64_p10_pi_put(p, 212, 11, random_word() % 1280);
        sc64_p10_pi_put(p, 223, 1, 1);
    }
    for (unsigned i = 0; i < MAX_BUILDINGS; ++i) {
        if (i % 3 == 0) continue;
        uint8_t *p = original.buildings[i].bytes;
        for (unsigned j = 0; j < sizeof(original.buildings[i].bytes); ++j)
            p[j] = (uint8_t)random_word();
        sc64_p10_pi_put(p, 40, 3, random_word() % 5);
        sc64_p10_pi_put(p, 182, 11, random_word() % 1280);
        sc64_p10_pi_put(p, 80, 3, random_word() % 5);
        sc64_p10_pi_put(p, 83, 3, random_word() % 6);
        sc64_p10_pi_put(p, 193, 1, 1);
        sc64_p10_pi_put(p, 195, 5, 0);
    }
    for (unsigned i = 0; i < 96; ++i)
        sc64_p10_pi_put(original.buildings[i / 5].bytes, 195 + i % 5, 1, random_word() & 1);
    for (unsigned i = 0; i < sizeof(original.sentinel); ++i)
        original.sentinel[i] = (uint8_t)random_word();
}

int main(void)
{
    Sc64ImpactEvent rows[SC64_P10_PI_ROWS], decoded[SC64_P10_PI_ROWS];
    uint8_t tail[SC64_P10_PI_TAIL];
    int count;
    for (unsigned i = 0; i < SC64_P10_PI_ROWS; ++i) {
        Sc64ImpactHit hit = {(int)(i * 12), (int)(i * 17), (int)(i % 16 + 1),
                             (int)(i % 5), (int)(i % 7) - 1, (int)(i % 64)};
        assert(sc64_impact_pack(&rows[i], &hit, i + 1));
    }
    for (unsigned n = 1; n <= SC64_P10_PI_ROWS; ++n) {
        prepare();
        wire = original;
        assert(sc64_p10_pi_encode(&wire, tail, rows, n));
        for (unsigned i = 0; i < 96; ++i) {
            unsigned index = i / 5, bit = 195 + i % 5;
            assert(sc64_p10_pi_get(wire.buildings[index].bytes, bit, 1) ==
                   sc64_p10_pi_get(original.buildings[index].bytes, bit, 1));
        }
        assert(sc64_p10_pi_decode(&wire, tail, sizeof(tail), decoded, &count));
        assert(count == (int)n && !memcmp(rows, decoded, n * sizeof(*rows)));
        assert(!memcmp(&wire, &original, sizeof(wire)));
    }

    prepare();
    wire = original;
    assert(sc64_p10_pi_encode(&wire, tail, rows, SC64_P10_PI_ROWS));
    bad = wire;
    assert(sc64_p10_pi_normalize_rows(&bad));
    assert(!memcmp(&bad, &original, sizeof(bad)));

    bad = wire;
    assert(!sc64_p10_pi_decode(&bad, tail, sizeof(tail) - 1, decoded, &count));
    bad = wire;
    unsigned bit;
    uint8_t *p = sc64_p10_pi_side(&bad, SC64_P10_PI_BITS - 1, &bit);
    sc64_p10_pi_put(p, bit, 1, 1);
    assert(!sc64_p10_pi_decode(&bad, tail, sizeof(tail), decoded, &count));
    bad = wire;
    p = sc64_p10_pi_side(&bad, 0, &bit);
    sc64_p10_pi_put(p, bit, 1, !sc64_p10_pi_get(p, bit, 1));
    assert(!sc64_p10_pi_decode(&bad, tail, sizeof(tail), decoded, &count));

    wire = original;
    rows[0].steps = 0;
    assert(!sc64_p10_pi_encode(&wire, tail, rows, SC64_P10_PI_ROWS));
    assert(!memcmp(&wire, &original, sizeof(wire)));
    puts("P10 detached padded impacts: 1..32 ordered rows, research, sentinels and malformed packets PASS");
    return 0;
}
