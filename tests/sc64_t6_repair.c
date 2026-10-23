#define SC64_T6_ENABLED 1
#define SC64_T6_RUNTIME 1
#define SC64_UNIFIED 1

#include "../starcraft/sc64_t6_repair_adapter.h"

#include <assert.h>
#include <limits.h>
#include <stdio.h>

static void test_target_eligibility(void)
{
    assert(sc64_t6_repair_target_valid_full(126, 1, 1, 1, 1, 0, 0, 1,
                                            406, 700, 1));
    assert(!sc64_t6_repair_target_valid_full(126, 1, 1, 1, 1, 0, 0, 1,
                                             700, 700, 1));
    assert(!sc64_t6_repair_target_valid_full(126, 1, 0, 1, 1, 0, 0, 1,
                                             406, 700, 1));
    assert(!sc64_t6_repair_target_valid_full(126, 1, 1, 1, 0, 0, 0, 1,
                                             406, 700, 1));
    assert(!sc64_t6_repair_target_valid_full(126, 1, 1, 1, 1, 1, 0, 1,
                                             406, 700, 1));
    assert(!sc64_t6_repair_target_valid_full(126, 1, 1, 1, 1, 0, 1, 1,
                                             406, 700, 1));
    assert(!sc64_t6_repair_target_valid_full(126, 1, 1, 1, 1, 0, 0, 0,
                                             406, 700, 1));
    assert(!sc64_t6_repair_target_valid_full(126, 1, 1, 1, 1, 0, 0, 1,
                                             406, 700, 0));
    assert(!sc64_t6_repair_target_valid_full(126, 0, 1, 1, 1, 0, 0, 1,
                                             406, 700, 1));
    assert(!sc64_t6_repair_target_valid_full(126, 1, 1, 0, 1, 0, 0, 1,
                                             406, 700, 1));
    assert(!sc64_t6_repair_target_valid_full(126, 1, 1, 1, 1, 0, 0, 1,
                                             -1, 700, 1));
    assert(!sc64_t6_repair_target_valid_full(126, 1, 1, 1, 1, 0, 0, 1,
                                             0, 0, 1));
}

static void test_norad_costs(void)
{
    Sc64T6RepairCosts costs = { 0, 0, 0, 0 };
    assert(sc64_t6_repair_costs(700, 800, 600, 4800, &costs));
    assert(costs.rate_raw == 34);
    assert(costs.interval_frames == 26);
    assert(costs.mineral_cost == 1 && costs.gas_cost == 1);
    assert(sc64_t6_repair_costs(700, 600, 600, 4800, &costs));
    assert(costs.mineral_cost == 1 && costs.gas_cost == 1);
}

static void test_paid_progress_and_completion(void)
{
    Sc64T6RepairCosts costs;
    int hp = 406, remainder = 0, timer = 0, minerals = 20, gas = 20;
    int initial_total = hp * SC64_T6_REPAIR_FP_ONE + remainder;
    assert(sc64_t6_repair_costs(700, 800, 600, 4800, &costs));
    assert(sc64_t6_repair_step(&hp, 700, &remainder, &timer,
                               &minerals, &gas, &costs));
    assert(minerals == 19 && gas == 19 && timer == 26 && hp == 406);
    for (int i = 0; i < 100; ++i)
        (void)sc64_t6_repair_step(&hp, 700, &remainder, &timer,
                                  &minerals, &gas, &costs);
    assert(hp * SC64_T6_REPAIR_FP_ONE + remainder > initial_total);
    while (hp < 700 && (minerals >= 1 && gas >= 1))
        (void)sc64_t6_repair_step(&hp, 700, &remainder, &timer,
                                  &minerals, &gas, &costs);
    assert(hp <= 700 && remainder >= 0 && remainder < 256);
    if (hp == 700) {
        assert(remainder == 0);
        assert(!sc64_t6_repair_step(&hp, 700, &remainder, &timer,
                                    &minerals, &gas, &costs));
    }
}

static void test_reference_payment_cadence(void)
{
    Sc64T6RepairCosts costs;
    int hp = 406, remainder = 0, timer = 0, minerals = 10, gas = 10;
    assert(sc64_t6_repair_costs(700, 800, 600, 4800, &costs));
    assert(sc64_t6_repair_step(&hp, 700, &remainder, &timer,
                               &minerals, &gas, &costs));
    assert(minerals == 9 && gas == 9 && timer == 26);
    for (int i = 0; i < 26; ++i)
        assert(sc64_t6_repair_step(&hp, 700, &remainder, &timer,
                                   &minerals, &gas, &costs));
    /* OpenBW sets repair_timer=time_cost, then decrements on subsequent
     * updates; the next payment is therefore 27 update calls later. */
    assert(minerals == 9 && gas == 9 && timer == 0);
    assert(sc64_t6_repair_step(&hp, 700, &remainder, &timer,
                               &minerals, &gas, &costs));
    assert(minerals == 8 && gas == 8 && timer == 26);
}

static void test_no_funds_and_bad_inputs_are_atomic(void)
{
    Sc64T6RepairCosts costs, invalid = { 0, 0, 0, 0 };
    int hp = 406, remainder = 17, timer = 0, minerals = 0, gas = 9;
    assert(sc64_t6_repair_costs(700, 800, 600, 4800, &costs));
    assert(!sc64_t6_repair_step(&hp, 700, &remainder, &timer,
                                &minerals, &gas, &costs));
    assert(hp == 406 && remainder == 17 && timer == 0 &&
           minerals == 0 && gas == 9);
    assert(!sc64_t6_repair_step(&hp, 700, &remainder, &timer,
                                &minerals, &gas, &invalid));
    assert(hp == 406 && remainder == 17 && timer == 0 &&
           minerals == 0 && gas == 9);
    assert(!sc64_t6_repair_costs(0, 1, 1, 10, &costs));
    assert(!sc64_t6_repair_costs(700, 800, 600, 0, &costs));
    assert(!sc64_t6_repair_costs(700, 800, 600, 4800, NULL));
    assert(sc64_t6_repair_costs(1, 1, 0, 1, &costs));
    assert(costs.rate_raw == 1);
    assert(!sc64_t6_repair_step(NULL, 700, &remainder, &timer,
                                &minerals, &gas, &costs));
    hp = 406; remainder = 256; timer = 0; minerals = gas = 2;
    assert(!sc64_t6_repair_step(&hp, 700, &remainder, &timer,
                                &minerals, &gas, &costs));
    assert(hp == 406 && remainder == 256 && timer == 0 &&
           minerals == 2 && gas == 2);
}

static void test_free_cost_target(void)
{
    Sc64T6RepairCosts costs;
    int hp = 1, remainder = 0, timer = 0, minerals = 0, gas = 0;
    assert(sc64_t6_repair_costs(10, 0, 0, 10, &costs));
    assert(costs.interval_frames == 65535 && costs.mineral_cost == 0 &&
           costs.gas_cost == 0);
    assert(sc64_t6_repair_step(&hp, 10, &remainder, &timer,
                               &minerals, &gas, &costs));
    assert(hp >= 1 && hp <= 10 && minerals == 0 && gas == 0);
    for (int i = 0; i < 20 && hp < 10; ++i)
        (void)sc64_t6_repair_step(&hp, 10, &remainder, &timer,
                                  &minerals, &gas, &costs);
    assert(hp > 1 && hp <= 10);
}

int main(void)
{
    test_target_eligibility();
    test_norad_costs();
    test_paid_progress_and_completion();
    test_reference_payment_cadence();
    test_no_funds_and_bad_inputs_are_atomic();
    test_free_cost_target();
    puts("T6 repair arithmetic checks passed");
    return 0;
}
