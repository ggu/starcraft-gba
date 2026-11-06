/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_Q8_FRACTION_WORD_H
#define SC64_Q8_FRACTION_WORD_H
#include <stdint.h>

/* Standalone lossless representation of an existing 0..1279 phase and two
 * independent Q8 remainders. This is a codec experiment, not a save format.
 * It does not identify which actor kinds can use the existing phase field. */
#define SC64_Q8_PHASE_COUNT 1280u
#define SC64_Q8_WORD_LIMIT (SC64_Q8_PHASE_COUNT * 256u * 256u)
static inline int sc64_q8_fraction_pack(uint32_t *out, unsigned phase,
                                        unsigned hp, unsigned shield)
{
    if (!out || phase >= SC64_Q8_PHASE_COUNT || hp >= 256u || shield >= 256u)
        return 0;
    *out = ((uint32_t)phase << 16) | ((uint32_t)hp << 8) | shield;
    return 1;
}
static inline int sc64_q8_fraction_unpack(uint32_t word, unsigned *phase,
                                          unsigned *hp, unsigned *shield)
{
    if (!phase || !hp || !shield || word >= SC64_Q8_WORD_LIMIT) return 0;
    *phase = word >> 16;
    *hp = (word >> 8) & 255u;
    *shield = word & 255u;
    return 1;
}
#endif
