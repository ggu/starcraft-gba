#ifndef SC64_Z5_HP_REGEN_H
#define SC64_Z5_HP_REGEN_H

#include "../common/gba.h"

#ifdef SC64_Z5_RUNTIME
enum {
    SC64_Z5_HP_REGEN_RAW_ZERGLING = 37,
    SC64_Z5_HP_REGEN_RAW_INFESTED_KERRIGAN = 51,
    SC64_Z5_HP_REGEN_TICKS_PER_HP = 80,
    SC64_Z5_HP_REGEN_ERROR = -1,
    SC64_Z5_HP_REGEN_UNCHANGED = 0,
    SC64_Z5_HP_REGEN_ADVANCED = 1
};

/* Pinned OpenBW regeneration tables provide 4 Q8 HP per source 24Hz tick for
 * both raw37 and raw51. At the game's 30Hz update cadence, the equivalent
 * fixed-point adaptation is one whole HP per 80 updates. This deliberately
 * uses integer hit points and preserves the fractional cadence in the
 * existing persisted shield_remainder field; equivalence of native damage
 * and regeneration subframe ordering has not been established. */
static inline int sc64_z5_hp_regen_tick(int raw_type, int *hp, int maxhp,
                                         u16 *phase, int paused)
{
    u16 next_phase;
    if (!hp || !phase) return SC64_Z5_HP_REGEN_ERROR;
    if (raw_type != SC64_Z5_HP_REGEN_RAW_ZERGLING &&
        raw_type != SC64_Z5_HP_REGEN_RAW_INFESTED_KERRIGAN)
        return SC64_Z5_HP_REGEN_UNCHANGED;
    if (maxhp <= 0 || *hp > maxhp || *phase >= SC64_Z5_HP_REGEN_TICKS_PER_HP ||
        paused < 0 || paused > 1) return SC64_Z5_HP_REGEN_ERROR;
    if (paused || *hp <= 0) return SC64_Z5_HP_REGEN_UNCHANGED;
    if (*hp == maxhp) {
        if (*phase) {
            *phase = 0;
            return SC64_Z5_HP_REGEN_ADVANCED;
        }
        return SC64_Z5_HP_REGEN_UNCHANGED;
    }
    next_phase = (u16)(*phase + 1u);
    if (next_phase == SC64_Z5_HP_REGEN_TICKS_PER_HP) {
        next_phase = 0;
        ++*hp;
    }
    *phase = next_phase;
    return SC64_Z5_HP_REGEN_ADVANCED;
}
#endif

#endif
