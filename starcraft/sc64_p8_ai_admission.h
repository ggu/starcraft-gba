/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_P8_AI_ADMISSION_H
#define SC64_P8_AI_ADMISSION_H

#include <stddef.h>
#include <stdint.h>

typedef enum {
    SC64_P8_AI_ADMISSION_ERROR = -1,
    SC64_P8_AI_ADMISSION_NO_BLOCK = 0,
    SC64_P8_AI_ADMISSION_BLOCK = 1
} sc64_p8_ai_admission_result;

/*
 * Apply the bounded part of the native AI request-admission gate.
 *
 * Inputs are one player's cap and native per-type count rows. The caller owns
 * the rows and is responsible for selecting a valid player. The native gate
 * returns a block flag: zero cap means no block, 0xff means block, and other
 * caps block when the compared count reaches the cap. Native raw type 5 also
 * includes the type-30 count. Widen before adding, since the native getter
 * returns two independent 16-bit counts and the sum is compared in 32 bits.
 *
 * Invalid pointers, raw-type selectors, or missing type-30 census data return
 * ERROR. Treat any nonzero result as blocked to fail closed. This function
 * does not decide whether those native counters mean completed or
 * in-progress actors, nor does it spend resources or operate the request heap.
 */
static inline sc64_p8_ai_admission_result sc64_p8_ai_admission_blocked(
    const uint8_t *unit_caps, size_t unit_cap_count,
    const uint16_t *unit_counts, size_t unit_count_count,
    unsigned raw_unit)
{
    if (unit_caps == NULL || unit_counts == NULL ||
        raw_unit > UINT16_MAX ||
        (size_t)raw_unit >= unit_cap_count ||
        (size_t)raw_unit >= unit_count_count) {
        return SC64_P8_AI_ADMISSION_ERROR;
    }

    uint8_t cap = unit_caps[raw_unit];
    if (cap == 0u) return SC64_P8_AI_ADMISSION_NO_BLOCK;
    if (cap == UINT8_MAX) return SC64_P8_AI_ADMISSION_BLOCK;

    uint32_t count = unit_counts[raw_unit];
    if (raw_unit == 5u) {
        if (30u >= unit_count_count) return SC64_P8_AI_ADMISSION_ERROR;
        count += (uint32_t)unit_counts[30u];
    }
    return count >= (uint32_t)cap ? SC64_P8_AI_ADMISSION_BLOCK :
                                    SC64_P8_AI_ADMISSION_NO_BLOCK;
}

#endif
