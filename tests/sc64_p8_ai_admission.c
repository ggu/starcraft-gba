/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "sc64_p8_ai_admission.h"

static void special_caps(void)
{
    uint8_t caps[32] = {0};
    uint16_t counts[32] = {0};

    caps[7] = 0u;
    counts[7] = UINT16_MAX;
    assert(sc64_p8_ai_admission_blocked(caps, 32u, counts, 32u, 7u) ==
           SC64_P8_AI_ADMISSION_NO_BLOCK);

    caps[7] = UINT8_MAX;
    counts[7] = 0u;
    assert(sc64_p8_ai_admission_blocked(caps, 32u, counts, 32u, 7u) ==
           SC64_P8_AI_ADMISSION_BLOCK);
}

static void finite_threshold(void)
{
    uint8_t caps[32] = {0};
    uint16_t counts[32] = {0};
    caps[12] = 4u;

    counts[12] = 3u;
    assert(sc64_p8_ai_admission_blocked(caps, 32u, counts, 32u, 12u) ==
           SC64_P8_AI_ADMISSION_NO_BLOCK);
    counts[12] = 4u;
    assert(sc64_p8_ai_admission_blocked(caps, 32u, counts, 32u, 12u) ==
           SC64_P8_AI_ADMISSION_BLOCK);
    counts[12] = 5u;
    assert(sc64_p8_ai_admission_blocked(caps, 32u, counts, 32u, 12u) ==
           SC64_P8_AI_ADMISSION_BLOCK);
}

static void raw_five_combines_type_thirty(void)
{
    uint8_t caps[32] = {0};
    uint16_t counts[32] = {0};
    caps[5] = 10u;
    counts[5] = 6u;
    counts[30] = 3u;
    assert(sc64_p8_ai_admission_blocked(caps, 32u, counts, 32u, 5u) ==
           SC64_P8_AI_ADMISSION_NO_BLOCK);

    counts[30] = 4u;
    assert(sc64_p8_ai_admission_blocked(caps, 32u, counts, 32u, 5u) ==
           SC64_P8_AI_ADMISSION_BLOCK);

    /* The native sum is widened: 65535 + 1 must not wrap to zero. */
    caps[5] = 254u;
    counts[5] = UINT16_MAX;
    counts[30] = 1u;
    assert(sc64_p8_ai_admission_blocked(caps, 32u, counts, 32u, 5u) ==
           SC64_P8_AI_ADMISSION_BLOCK);

    counts[30] = UINT16_MAX;
    assert(sc64_p8_ai_admission_blocked(caps, 32u, counts, 32u, 5u) ==
           SC64_P8_AI_ADMISSION_BLOCK);
}

static void invalid_selectors_fail_closed(void)
{
    uint8_t caps[31] = {0};
    uint16_t counts[31] = {0};
    caps[5] = 10u;
    counts[5] = 1u;

    assert(sc64_p8_ai_admission_blocked(NULL, 31u, counts, 31u, 5u) !=
           SC64_P8_AI_ADMISSION_NO_BLOCK);
    assert(sc64_p8_ai_admission_blocked(caps, 31u, NULL, 31u, 5u) !=
           SC64_P8_AI_ADMISSION_NO_BLOCK);
    assert(sc64_p8_ai_admission_blocked(caps, 31u, counts, 31u, 31u) !=
           SC64_P8_AI_ADMISSION_NO_BLOCK);
    assert(sc64_p8_ai_admission_blocked(caps, 31u, counts, 31u, UINT16_MAX +
                                        1u) != SC64_P8_AI_ADMISSION_NO_BLOCK);
    /* Type 5 requires the native type-30 counter even when this row is short. */
    assert(sc64_p8_ai_admission_blocked(caps, 31u, counts, 30u, 5u) !=
           SC64_P8_AI_ADMISSION_NO_BLOCK);
}

int main(void)
{
    special_caps();
    finite_threshold();
    raw_five_combines_type_thirty();
    invalid_selectors_fail_closed();
    puts("P8 AI admission boundary checks passed");
    return 0;
}
