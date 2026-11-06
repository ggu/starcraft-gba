/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_Q8_SHIELD_PHASE_H
#define SC64_Q8_SHIELD_PHASE_H
#include <stdint.h>
/* Proof-only arithmetic for actors whose remainder really denotes a shield
 * fraction. Caller must establish actor identity and phase denominator. */
static inline int sc64_q8_shield_damage(unsigned *shield, unsigned *phase,
                                        uint32_t requested_q8,
                                        uint32_t *absorbed_q8)
{
    uint64_t amount, available, used;
    if (!shield || !phase || !absorbed_q8 || *phase>=1280u) return 0;
    available=(uint64_t)*shield*1280u+*phase;
    /* Source damage only consumes complete Q8 quanta. */
    amount=available/5u;
    used=requested_q8<amount ? requested_q8 : amount;
    available-=used*5u;
    *shield=(unsigned)(available/1280u);
    *phase=(unsigned)(available%1280u);
    *absorbed_q8=(uint32_t)used;
    return 1;
}
static inline int sc64_q8_phase_320_to_1280(unsigned old, unsigned *out)
{
    if (!out || old>=320u) return 0;
    *out=old*4u;
    return 1;
}
#endif
