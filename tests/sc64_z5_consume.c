#include <assert.h>
#include <limits.h>
#include <string.h>

#define SC64_Z5_RUNTIME 1
#define SC64_Z5_CONSUME_ENABLED 1
#include "../starcraft/sc64_z5_consume.h"

static Sc64Z5ConsumeActor hero(void)
{
    Sc64Z5ConsumeActor a = {51, 3, 100 * 256, 100 * 256,
                            1, 1, 0, 0, 0, 0, 0, 0, 0};
    return a;
}

static Sc64Z5ConsumeActor target(void)
{
    Sc64Z5ConsumeActor a = {37, 3, 35 * 256, 0,
                            1, 1, 0, 0, 0, 0, 0, 0, 0};
    return a;
}

static void test_range_and_native_target_filters(void)
{
    assert(sc64_z5_consume_in_range(16, 0));
    assert(!sc64_z5_consume_in_range(17, 0));
    assert(sc64_z5_consume_in_range(22, 6));
    assert(!sc64_z5_consume_in_range(-1, 0));
    assert(!sc64_z5_consume_in_range(1, -1));
    assert(sc64_z5_consume_in_range(INT_MAX, INT_MAX));
    assert(!sc64_z5_consume_in_range(INT_MAX, 0));

    Sc64Z5ConsumeActor c = hero(), t = target();
    assert(sc64_z5_consume_cast(&c, &t, 1, 16, 0) == SC64_Z5_CONSUME_APPLIED);
    assert(!t.alive && c.energy_q8 == 150 * 256);

    /* Pinned unit_tech_target_valid rejects these categories, without an
     * additional organic or Protoss-race requirement. */
    for (unsigned mode = 0; mode < 6; ++mode) {
        c = hero(); t = target();
        switch (mode) {
        case 0: t.owner = 2; break;
        case 1: t.building = 1; break;
        case 2: t.terran_race = 1; break;
        case 3: t.larva = 1; break;
        case 4: t.invincible = 1; break;
        default: t.stasis = 1; break;
        }
        Sc64Z5ConsumeActor before = t;
        int energy_before = c.energy_q8;
        assert(sc64_z5_consume_cast(&c, &t, 1, 16, 0) == SC64_Z5_CONSUME_DENIED);
        assert(memcmp(&t, &before, sizeof(t)) == 0);
        assert(c.energy_q8 == energy_before);
    }
    /* Protoss/non-Terran and non-organic facts are deliberately not native
     * eligibility predicates in the pinned engine check. */
    c = hero(); t = target();
    assert(sc64_z5_consume_cast(&c, &t, 1, 16, 0) == SC64_Z5_CONSUME_APPLIED);
}

static void test_caster_preconditions_and_no_energy_cost(void)
{
    for (unsigned mode = 0; mode < 6; ++mode) {
        Sc64Z5ConsumeActor c = hero(), t = target();
        if (mode == 0) c.raw_type = 50;
        if (mode == 1) c.completed = 0;
        if (mode == 2) c.hallucinated = 1;
        if (mode == 3) c.disabled = 1;
        if (mode == 4) c.alive = 0;
        if (mode == 5) c.hp = 0;
        Sc64Z5ConsumeActor before = t;
        int result = sc64_z5_consume_cast(&c, &t, 1, 0, 0);
        assert(result == SC64_Z5_CONSUME_DENIED);
        assert(memcmp(&t, &before, sizeof(t)) == 0);
    }
    Sc64Z5ConsumeActor c = hero(), t = target();
    c.energy_q8 = 0;
    assert(sc64_z5_consume_cast(&c, &t, 0, 0, 0) == SC64_Z5_CONSUME_DENIED);
    assert(c.energy_q8 == 0 && t.alive);
    assert(sc64_z5_consume_cast(&c, &t, 1, 0, 0) == SC64_Z5_CONSUME_APPLIED);
    assert(c.energy_q8 == 50 * 256);
}

static void test_energy_gain_cap_and_hallucination(void)
{
    Sc64Z5ConsumeActor c = hero(), t = target();
    c.energy_q8 = 240 * 256;
    assert(sc64_z5_consume_cast(&c, &t, 1, 0, 0) == SC64_Z5_CONSUME_APPLIED);
    assert(c.energy_q8 == 250 * 256);

    c = hero(); t = target();
    c.energy_q8 = 250 * 256;
    assert(sc64_z5_consume_cast(&c, &t, 1, 0, 0) == SC64_Z5_CONSUME_APPLIED);
    assert(c.energy_q8 == 250 * 256);

    c = hero(); t = target();
    t.hallucinated = 1;
    assert(sc64_z5_consume_cast(&c, &t, 1, 0, 0) == SC64_Z5_CONSUME_APPLIED);
    assert(!t.alive && c.energy_q8 == 100 * 256);
}

static void test_transactional_invalid_inputs_and_dead_target(void)
{
    Sc64Z5ConsumeActor c = hero(), t = target();
    c.energy_q8 = -1;
    Sc64Z5ConsumeActor cbefore = c, tbefore = t;
    assert(sc64_z5_consume_cast(&c, &t, 1, 0, 0) == SC64_Z5_CONSUME_ERROR);
    assert(memcmp(&c, &cbefore, sizeof(c)) == 0);
    assert(memcmp(&t, &tbefore, sizeof(t)) == 0);

    c = hero(); t = target(); t.disabled = 2;
    tbefore = t;
    assert(sc64_z5_consume_cast(&c, &t, 1, 0, 0) == SC64_Z5_CONSUME_ERROR);
    assert(memcmp(&t, &tbefore, sizeof(t)) == 0);

    c = hero(); t = target(); t.hp = 0;
    tbefore = t;
    assert(sc64_z5_consume_cast(&c, &t, 1, 0, 0) == SC64_Z5_CONSUME_DENIED);
    assert(memcmp(&t, &tbefore, sizeof(t)) == 0);

    c = hero(); t = target();
    tbefore = t; cbefore = c;
    assert(sc64_z5_consume_cast(&c, &t, 1, 17, 0) == SC64_Z5_CONSUME_DENIED);
    assert(memcmp(&t, &tbefore, sizeof(t)) == 0);
    assert(memcmp(&c, &cbefore, sizeof(c)) == 0);
    assert(sc64_z5_consume_cast(&c, &t, 2, 0, 0) == SC64_Z5_CONSUME_ERROR);
    assert(memcmp(&t, &tbefore, sizeof(t)) == 0);
}

int main(void)
{
    test_range_and_native_target_filters();
    test_caster_preconditions_and_no_energy_cost();
    test_energy_gain_cap_and_hallucination();
    test_transactional_invalid_inputs_and_dead_target();
    return 0;
}
