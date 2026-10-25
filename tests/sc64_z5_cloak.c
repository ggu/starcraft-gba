#include <assert.h>
#include <string.h>

#define SC64_Z5_RUNTIME 1
#define SC64_Z5_CLOAK_ENABLED 1
#include "../starcraft/sc64_z5_cloak.h"

static void test_activation_toggle_denial_and_pause(void)
{
    Sc64Z5CloakState s = {0, 0};
    int energy = 250 * 256;
    assert(sc64_z5_cloak_toggle(&s, &energy) == SC64_Z5_CLOAK_ADVANCED);
    assert(s.active == 1 && s.regen_remainder == 0);
    assert(energy == 225 * 256);
    assert(sc64_z5_cloak_tick(&s, &energy, 1) == SC64_Z5_CLOAK_UNCHANGED);
    assert(s.active == 1 && energy == 225 * 256);
    assert(sc64_z5_cloak_toggle(&s, &energy) == SC64_Z5_CLOAK_ADVANCED);
    assert(!s.active && energy == 225 * 256);

    energy = 24 * 256 + 255;
    assert(sc64_z5_cloak_toggle(&s, &energy) == SC64_Z5_CLOAK_DENIED_ENERGY);
    assert(!s.active && energy == 24 * 256 + 255);
}

static void test_drain_and_depleted_auto_decloak(void)
{
    Sc64Z5CloakState s = {1, 0};
    int energy = 15;
    assert(sc64_z5_cloak_tick(&s, &energy, 0) == SC64_Z5_CLOAK_ADVANCED);
    assert(s.active && energy == 7);
    /* Below native 10-raw-Q8 threshold: turn off without another drain. */
    assert(sc64_z5_cloak_tick(&s, &energy, 0) == SC64_Z5_CLOAK_ADVANCED);
    assert(!s.active && energy == 7 && s.regen_remainder == 0);
}

static void test_exact_regeneration_and_saved_phase(void)
{
    Sc64Z5CloakState s = {0, 0};
    int energy = 0;
    for (unsigned i = 0; i < 5; ++i) {
        assert(sc64_z5_cloak_tick(&s, &energy, 0) == SC64_Z5_CLOAK_ADVANCED);
        assert(energy == (int)((32u * (i + 1u) / 5u)));
    }
    assert(energy == 32 && s.regen_remainder == 0);

    Sc64Z5CloakState resumed = s;
    int resumed_energy = energy;
    for (unsigned i = 0; i < 100; ++i) {
        assert(sc64_z5_cloak_tick(&s, &energy, 0) ==
               sc64_z5_cloak_tick(&resumed, &resumed_energy, 0));
        assert(energy == resumed_energy);
        assert(memcmp(&s, &resumed, sizeof(s)) == 0);
    }

    s.active = 0;
    s.regen_remainder = 4;
    energy = SC64_Z5_CLOAK_MAX_ENERGY_Q8 - 1;
    assert(sc64_z5_cloak_tick(&s, &energy, 0) == SC64_Z5_CLOAK_ADVANCED);
    assert(energy == SC64_Z5_CLOAK_MAX_ENERGY_Q8 && s.regen_remainder == 0);
    assert(sc64_z5_cloak_tick(&s, &energy, 0) == SC64_Z5_CLOAK_UNCHANGED);
}

static void test_energy_duration_and_five_tick_regen(void)
{
    Sc64Z5CloakState s = {0, 0};
    int energy = SC64_Z5_CLOAK_MAX_ENERGY_Q8;
    assert(sc64_z5_cloak_toggle(&s, &energy) == SC64_Z5_CLOAK_ADVANCED);
    assert(energy == 225 * 256);
    /* 225 energy / 8 raw Q8 per tick spans 7200 30Hz updates. The final
     * update observes energy below the raw 10-unit source threshold and
     * decloaks without draining that final 8-unit fraction. */
    for (unsigned tick = 0; tick < 7199; ++tick) {
        assert(sc64_z5_cloak_tick(&s, &energy, 0) == SC64_Z5_CLOAK_ADVANCED);
        assert(s.active);
    }
    assert(energy == 8);
    assert(sc64_z5_cloak_tick(&s, &energy, 0) == SC64_Z5_CLOAK_ADVANCED);
    assert(!s.active && energy == 8);

    energy = 0;
    s.regen_remainder = 0;
    for (unsigned tick = 0; tick < 5; ++tick)
        assert(sc64_z5_cloak_tick(&s, &energy, 0) == SC64_Z5_CLOAK_ADVANCED);
    assert(energy == 32);
}

static void test_invalid_state_is_transactional(void)
{
    Sc64Z5CloakState s = {2, 0};
    int energy = 100 * 256;
    Sc64Z5CloakState before = s;
    assert(sc64_z5_cloak_toggle(&s, &energy) == SC64_Z5_CLOAK_ERROR);
    assert(memcmp(&s, &before, sizeof(s)) == 0 && energy == 100 * 256);

    s.active = 1;
    s.regen_remainder = 1;
    before = s;
    assert(sc64_z5_cloak_tick(&s, &energy, 0) == SC64_Z5_CLOAK_ERROR);
    assert(memcmp(&s, &before, sizeof(s)) == 0 && energy == 100 * 256);

    s.active = 0;
    s.regen_remainder = 5;
    before = s;
    assert(sc64_z5_cloak_tick(&s, &energy, 0) == SC64_Z5_CLOAK_ERROR);
    assert(memcmp(&s, &before, sizeof(s)) == 0 && energy == 100 * 256);

    s.regen_remainder = 0;
    energy = -1;
    assert(sc64_z5_cloak_tick(&s, &energy, 0) == SC64_Z5_CLOAK_ERROR);
    assert(energy == -1 && s.regen_remainder == 0);
    energy = SC64_Z5_CLOAK_MAX_ENERGY_Q8 + 1;
    assert(sc64_z5_cloak_toggle(&s, &energy) == SC64_Z5_CLOAK_ERROR);
    assert(energy == SC64_Z5_CLOAK_MAX_ENERGY_Q8 + 1 && !s.active);
    energy = 100 * 256;
    assert(sc64_z5_cloak_tick(&s, &energy, 2) == SC64_Z5_CLOAK_ERROR);
    assert(energy == 100 * 256 && !s.active);
}

int main(void)
{
    test_activation_toggle_denial_and_pause();
    test_drain_and_depleted_auto_decloak();
    test_exact_regeneration_and_saved_phase();
    test_energy_duration_and_five_tick_regen();
    test_invalid_state_is_transactional();
    return 0;
}
