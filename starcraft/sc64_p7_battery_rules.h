#ifndef SC64_P7_BATTERY_RULES_H
#define SC64_P7_BATTERY_RULES_H

/* P7 Shield Battery fixed-point rules. The caller stores energy_q8 in a
 * Battery Building.queue_progress and the 0..4 regeneration phase in level;
 * units keep their existing 1/1280 shield_remainder. */
enum {
    SC64_P7_BATTERY_MAX_ENERGY_Q8 = 200 * 256,
    SC64_P7_BATTERY_REGEN_NUM = 32,
    SC64_P7_BATTERY_REGEN_DEN = 5,
    SC64_P7_BATTERY_STEP_CAP_Q8 = 4 * 256
};

static inline int sc64_p7_battery_energy_valid(unsigned energy_q8,
                                               unsigned regen_remainder)
{
    return energy_q8 <= SC64_P7_BATTERY_MAX_ENERGY_Q8 &&
           regen_remainder < SC64_P7_BATTERY_REGEN_DEN &&
           (energy_q8 != SC64_P7_BATTERY_MAX_ENERGY_Q8 ||
            regen_remainder == 0);
}

static inline void sc64_p7_battery_passive_shield_tick(unsigned *shield,
                                                        unsigned max_shield,
                                                        unsigned *rem)
{
    if (!shield || !rem || max_shield > 0xffffffffu / 1280u ||
        *shield > max_shield || *rem >= 1280u) return;
    if (*shield >= max_shield) { *rem=0; return; }
    /* Native +7 Q8 per 24 Hz tick becomes +7/320 points at 30 Hz. */
    unsigned total=*rem+28u;
    *shield+=total/1280u;
    *rem=total%1280u;
    if (*shield>=max_shield) { *shield=max_shield; *rem=0; }
}

static inline void sc64_p7_battery_regenerate(unsigned *energy_q8,
                                               unsigned *remainder)
{
    if (!energy_q8 || !remainder ||
        !sc64_p7_battery_energy_valid(*energy_q8, *remainder)) return;
    if (*energy_q8 == SC64_P7_BATTERY_MAX_ENERGY_Q8) return;
    unsigned phase = *remainder + SC64_P7_BATTERY_REGEN_NUM;
    *energy_q8 += phase / SC64_P7_BATTERY_REGEN_DEN;
    *remainder = phase % SC64_P7_BATTERY_REGEN_DEN;
    if (*energy_q8 >= SC64_P7_BATTERY_MAX_ENERGY_Q8) {
        *energy_q8 = SC64_P7_BATTERY_MAX_ENERGY_Q8;
        *remainder = 0;
    }
}

/* Apply one 30 Hz service step. `shield` is integer shield points and `rem`
 * is the engine's 1/1280 point phase. Return 1 if a transfer occurred. The
 * source Q8 energy charge intentionally retains native integer-floor cost. */
static inline int sc64_p7_battery_transfer(unsigned *energy_q8,
                                            unsigned *shield,
                                            unsigned max_shield,
                                            unsigned *rem)
{
    if (!energy_q8 || !shield || !rem ||
        !sc64_p7_battery_energy_valid(*energy_q8, 0) ||
        max_shield > 0xffffffffu / 1280u ||
        *shield >= max_shield || *rem >= 1280u) return 0;
    unsigned deficit = (max_shield - *shield) * 1280u - *rem;
    if (!deficit || !*energy_q8) return 0;
    unsigned requested = (deficit + 4u) / 5u;
    if (requested > SC64_P7_BATTERY_STEP_CAP_Q8)
        requested = SC64_P7_BATTERY_STEP_CAP_Q8;
    unsigned cost = requested / 2u;
    if (*energy_q8 < cost) {
        cost = *energy_q8;
        requested = cost * 2u;
    }
    *energy_q8 -= cost;
    unsigned amount = requested * 5u;
    if (amount > deficit) amount = deficit;
    unsigned total = *rem + amount;
    *shield += total / 1280u;
    *rem = total % 1280u;
    if (*shield >= max_shield) {
        *shield = max_shield;
        *rem = 0;
    }
    return amount != 0;
}

#endif
