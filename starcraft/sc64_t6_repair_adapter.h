#ifndef SC64_T6_REPAIR_ADAPTER_H
#define SC64_T6_REPAIR_ADAPTER_H

/* Small, T6-only repair arithmetic helpers.  These encode the observable
 * eligibility and fixed-point cost rules from the pinned OpenBW
 * order_Repair reference (bwgame.h), using the T6 runtime's integer HP and
 * source building facts.  Callers own selection, pathing and order state. */
#if defined(SC64_T6_RUNTIME) && defined(SC64_T6_ENABLED) && \
    defined(SC64_UNIFIED)

#include <stdint.h>

enum { SC64_T6_REPAIR_FP_SHIFT = 8, SC64_T6_REPAIR_FP_ONE = 256 };

typedef struct {
    uint16_t interval_frames;
    uint16_t rate_raw; /* HP in 8-bit fixed-point per game frame. */
    uint16_t mineral_cost;
    uint16_t gas_cost;
} Sc64T6RepairCosts;

/* OpenBW order_Repair rejects non-workers at the command boundary, then
 * requires a completed, unloaded, damaged mechanical Terran target.  The
 * caller supplies facts that this compact engine stores outside the target. */
static inline int
sc64_t6_repair_target_valid_full(unsigned raw_type, int worker, int target_alive,
                                 int is_building, int completed,
                                 int target_loaded, int stasis, int mechanical,
                                 int current_hp, int max_hp, int terran_target)
{
    (void)raw_type; /* Raw identity is retained for caller-side T6 gating. */
    return worker && target_alive && is_building && completed && !target_loaded &&
           !stasis && mechanical && terran_target &&
           current_hp >= 0 && max_hp > 0 && current_hp < max_hp;
}

/* Convenience form for scenario callers that have already validated live
 * slot, ownership and load state. Kept compact for the T6 world binding. */
static inline int
sc64_t6_repair_target_valid(unsigned raw_type, int is_building, int completed,
                            int mechanical, int current_hp, int max_hp,
                            int terran_target, int worker)
{
    return sc64_t6_repair_target_valid_full(raw_type, worker, 1, is_building,
                                            completed, 0, 0, mechanical,
                                            current_hp, max_hp, terran_target);
}

/* Reproduce the reference's per-frame HP construction rate, then its
 * interval payment rule.  `build_frames` is the type's normal build time.
 * Integer products are widened before division; malformed/unsupported facts
 * fail without modifying `out`. */
static inline int
sc64_t6_repair_costs(unsigned max_hp, unsigned mineral_value,
                     unsigned gas_value, unsigned build_frames,
                     Sc64T6RepairCosts *out)
{
    uint64_t max_raw, rate_numerator, denominator, interval;
    unsigned lowest;
    Sc64T6RepairCosts candidate;
    if (!out || !max_hp || !build_frames || max_hp > 65535u ||
        mineral_value > 65535u || gas_value > 65535u ||
        build_frames > 65535u)
        return 0;

    max_raw = (uint64_t)max_hp * SC64_T6_REPAIR_FP_ONE;
    /* OpenBW's source expression is
     * (max_hp - max_hp/10 + fp8(build_time) - 1) / build_time.
     * Integer HP is divided by ten before conversion to fp8; keep that
     * ordering so small HP values cannot underflow the unsigned numerator. */
    rate_numerator = max_raw - (uint64_t)(max_hp / 10u) *
                     SC64_T6_REPAIR_FP_ONE + build_frames;
    if (rate_numerator < SC64_T6_REPAIR_FP_ONE) return 0;
    rate_numerator -= SC64_T6_REPAIR_FP_ONE;
    rate_numerator /= build_frames;
    if (rate_numerator > 65535u) return 0;
    candidate.rate_raw = (uint16_t)rate_numerator;
    if (!candidate.rate_raw) candidate.rate_raw = 1;

    if (!mineral_value && !gas_value) {
        candidate.interval_frames = 65535u;
        candidate.mineral_cost = 0;
        candidate.gas_cost = 0;
        *out = candidate;
        return 1;
    }

    lowest = gas_value == 0 || mineral_value <= gas_value ?
             mineral_value : gas_value;
    if (lowest == 0) lowest = mineral_value ? mineral_value : gas_value;
    denominator = (uint64_t)lowest * candidate.rate_raw;
    interval = max_raw * 3u / denominator;
    if (interval != 0) {
        candidate.interval_frames = interval > 65535u ? 65535u :
                                    (uint16_t)interval;
        if (lowest == mineral_value) {
            candidate.mineral_cost = 1;
            candidate.gas_cost = mineral_value ?
                (uint16_t)(gas_value / mineral_value) : 0;
        } else {
            candidate.mineral_cost = gas_value ?
                (uint16_t)(mineral_value / gas_value) : 0;
            candidate.gas_cost = 1;
        }
    } else {
        /* The reference's sub-frame-cost fallback charges proportionally and
         * clamps each nonzero resource to at least one. */
        uint64_t cost_denominator = max_raw * 3u;
        uint64_t mc = (uint64_t)mineral_value * candidate.rate_raw /
                      cost_denominator;
        uint64_t gc = (uint64_t)gas_value * candidate.rate_raw /
                      cost_denominator;
        candidate.mineral_cost = mineral_value ?
            (uint16_t)(mc ? mc : 1u) : 0;
        candidate.gas_cost = gas_value ? (uint16_t)(gc ? gc : 1u) : 0;
        candidate.interval_frames = 1;
    }
    *out = candidate;
    return 1;
}

/* One ordinary game-frame repair update. `hp_remainder` stores fractional
 * HP in the worker's carry field while its order is REPAIR; callers must clear
 * it whenever that order is cancelled.  Returns 1 while repair can continue,
 * 0 when complete or unable to pay. Wallet/HP/timer are unchanged on invalid
 * input, insufficient funds, or a null required pointer. */
static inline int
sc64_t6_repair_step(int *hp, int max_hp, int *hp_remainder,
                    int *payment_timer, int *minerals, int *gas,
                    const Sc64T6RepairCosts *costs)
{
    int remainder, timer, next_hp;
    if (!hp || !hp_remainder || !payment_timer || !minerals || !gas ||
        !costs || max_hp <= 0 || costs->rate_raw == 0 ||
        costs->interval_frames == 0)
        return 0;
    if (*hp < 0 || *hp >= max_hp) return 0;
    if (*hp_remainder < 0 || *hp_remainder >= SC64_T6_REPAIR_FP_ONE ||
        *payment_timer < 0 || *payment_timer > 65535 ||
        *minerals < 0 || *gas < 0)
        return 0;

    remainder = *hp_remainder;
    timer = *payment_timer;
    if (timer == 0) {
        if (*minerals < costs->mineral_cost || *gas < costs->gas_cost)
            return 0;
        *minerals -= costs->mineral_cost;
        *gas -= costs->gas_cost;
        timer = costs->interval_frames;
    } else {
        --timer;
    }

    remainder += costs->rate_raw;
    next_hp = *hp + remainder / SC64_T6_REPAIR_FP_ONE;
    remainder %= SC64_T6_REPAIR_FP_ONE;
    if (next_hp >= max_hp) {
        *hp = max_hp;
        *hp_remainder = 0;
        *payment_timer = 0;
        return 0;
    }
    *hp = next_hp;
    *hp_remainder = remainder;
    *payment_timer = timer;
    return 1;
}

#endif /* T6 runtime */
#endif /* SC64_T6_REPAIR_ADAPTER_H */
