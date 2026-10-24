#include <assert.h>
#include <stdio.h>
#include "../starcraft/sc64_psionic_storm.h"

int main(void)
{
    Sc64PsionicStormState s;
    Sc64PsionicStormPulse pulses[SC64_STORM_MAX_ACTIVE];
    int caster_energy[SC64_STORM_MAX_ACTIVE];
    int energy, before, hp, shields;
    u8 storm_hit_tick, regen_remainder;
    u32 t;

    assert(SC64_STORM_TECH_ID == 19 && SC64_STORM_WEAPON_ID == 84);
    assert(SC64_STORM_ENERGY_COST == 75 && SC64_STORM_DAMAGE_PER_PULSE == 14);
    assert(SC64_STORM_RADIUS_SOURCE_PX == 48 && SC64_STORM_RADIUS_GBA_PX == 24);
    assert(SC64_STORM_CAST_RANGE_SOURCE_PX == 288 && SC64_STORM_CAST_RANGE_GBA_PX == 144);
    assert(sc64_psionic_storm_target_class(1, 0) ==
           (SC64_STORM_TARGET_AIR | SC64_STORM_TARGET_NONBUILDING));
    assert(sc64_psionic_storm_target_class(0, 0) ==
           (SC64_STORM_TARGET_GROUND | SC64_STORM_TARGET_NONBUILDING));
    assert(sc64_psionic_storm_target_class(1, 1) == 0);
    assert(sc64_psionic_storm_target_class(0, 1) == 0);
    assert(sc64_psionic_storm_ms_to_ticks(0) == 0);
    assert(sc64_psionic_storm_ms_to_ticks(1) == 1);
    assert(sc64_psionic_storm_ms_to_ticks(34) == 2);
    assert(sc64_psionic_storm_ms_to_ticks(1000) == 30);
    assert(sc64_psionic_storm_ms_to_ticks(0xffffffffu) == 128849019u);
    assert(sc64_psionic_storm_source_frames_to_ticks(16) == 20);
    assert(sc64_psionic_storm_source_frames_to_ticks(8) == 10);
    assert(sc64_psionic_storm_source_frames_to_ticks(1) == 2);

    sc64_psionic_storm_init(&s);
    assert(sc64_psionic_storm_state_valid(&s));
    energy = 200 * SC64_STORM_ENERGY_ONE;
    before = energy;
    assert(!sc64_psionic_storm_cast(&s, 100, 100, 100, 100, &energy, 0));
    assert(energy == before && sc64_psionic_storm_state_valid(&s));
    assert(!sc64_psionic_storm_cast(&s, 250, 100, 100, 100, &energy, 1));
    assert(energy == before);
    assert(sc64_psionic_storm_cast(&s, 100, 100, 100, 100, &energy, 1));
    assert(energy == 125 * SC64_STORM_ENERGY_ONE);
    assert(s.storm[0].age_ticks == 0 && s.storm[0].pulses_left == 8);
    assert(sc64_psionic_storm_contains(&s.storm[0], 124, 100));
    assert(!sc64_psionic_storm_contains(&s.storm[0], 125, 100));
    assert(sc64_psionic_storm_contains(&s.storm[0], 100, 124));
    assert(!sc64_psionic_storm_contains(&s.storm[0], 124, 124));

    /* Armor is ignored, shields absorb first, and same-owner units are valid
     * targets because Storm splash does not filter by owner. The marker gives
     * the native storm_timer one-hit-per-target-per-update behavior. */
    hp = 100 * 256; shields = 20 * 256; storm_hit_tick = 0;
    assert(sc64_psionic_storm_apply_hit(sc64_psionic_storm_target_class(0, 0),
                                        0, &storm_hit_tick, &hp, &shields));
    assert(shields == 6 * 256 && hp == 100 * 256);
    assert(!sc64_psionic_storm_apply_hit(sc64_psionic_storm_target_class(0, 0),
                                         0, &storm_hit_tick, &hp, &shields));
    assert(shields == 6 * 256 && hp == 100 * 256);
    storm_hit_tick = 0;
    assert(sc64_psionic_storm_apply_hit(sc64_psionic_storm_target_class(1, 0),
                                        0, &storm_hit_tick, &hp, &shields));
    assert(shields == 0 && hp == 92 * 256);
    storm_hit_tick = 0;
    assert(!sc64_psionic_storm_apply_hit(sc64_psionic_storm_target_class(1, 1),
                                         0, &storm_hit_tick, &hp, &shields));
    assert(!sc64_psionic_storm_apply_hit(sc64_psionic_storm_target_class(1, 0),
                                         1, &storm_hit_tick, &hp, &shields));

    /* First and final pulses retain their exact target point, including the
     * eighth event after its storm slot has been cleared. */
    for (t = 1; t < SC64_STORM_FIRST_PULSE_TICKS; ++t) {
        assert(sc64_psionic_storm_step(&s, pulses) == 0);
        assert(sc64_psionic_storm_state_valid(&s));
    }
    assert(sc64_psionic_storm_step(&s, pulses) == 1);
    assert(pulses[0].x == 100 && pulses[0].y == 100 && pulses[0].pulse_number == 1);
    assert(s.storm[0].pulses_left == 7);
    for (t = 0; t < 7; ++t) {
        unsigned wait;
        for (wait = 0; wait < SC64_STORM_PULSE_INTERVAL_TICKS - 1; ++wait)
            assert(sc64_psionic_storm_step(&s, pulses) == 0);
        assert(sc64_psionic_storm_step(&s, pulses) == 1);
        assert(pulses[0].x == 100 && pulses[0].y == 100);
        assert(pulses[0].pulse_number == t + 2);
        storm_hit_tick = 0; hp = 10000 * 256; shields = 0;
        assert(sc64_psionic_storm_apply_hit(sc64_psionic_storm_target_class(0, 0),
                                            0, &storm_hit_tick, &hp, &shields));
        assert(sc64_psionic_storm_state_valid(&s));
    }
    assert(pulses[0].pulse_number == 8 && pulses[0].x == 100 && pulses[0].y == 100);
    assert(!s.storm[0].active && sc64_psionic_storm_state_valid(&s));

    /* Four effect slots are bounded; each can be cast by a distinct caster. */
    for (t = 0; t < SC64_STORM_MAX_ACTIVE; ++t) {
        caster_energy[t] = 250 * SC64_STORM_ENERGY_ONE;
        assert(sc64_psionic_storm_cast(&s, (u16)(t * 100), 50, (u16)(t * 100), 50,
                                       &caster_energy[t], 1));
    }
    energy = 250 * SC64_STORM_ENERGY_ONE; before = energy;
    assert(!sc64_psionic_storm_cast(&s, 500, 50, 500, 50, &energy, 1));
    assert(energy == before);
    sc64_psionic_storm_init(&s);
    energy = 74 * SC64_STORM_ENERGY_ONE; before = energy;
    assert(!sc64_psionic_storm_cast(&s, 0, 0, 0, 0, &energy, 1));
    assert(energy == before);

    /* 8 source-Q8 units/frame × 24/30 frames/tick = 32/5 Q8/tick. */
    regen_remainder = 0; energy = 0;
    for (t = 0; t < 5; ++t)
        energy = sc64_psionic_storm_regen_q8(energy, 200, &regen_remainder);
    assert(energy == 32 && regen_remainder == 0);
    regen_remainder = 0;
    assert(sc64_psionic_storm_regen_q8(200 * 256 - 4, 200, &regen_remainder) == 200 * 256);
    assert(regen_remainder == 2);
    assert(sc64_psionic_storm_regen_q8(200 * 256, 200, &regen_remainder) == 200 * 256);
    assert(sc64_psionic_storm_regen_q8(0, 250, 0) == -1);
    assert(sc64_psionic_storm_regen_q8(-1, 200, &regen_remainder) == -1);
    regen_remainder = 0;
    assert(sc64_psionic_storm_regen_q8(0, 251, &regen_remainder) == -1);

    /* Malformed saves fail validation. */
    s.storm[0].active = 1;
    s.storm[0].pulses_left = 0;
    assert(!sc64_psionic_storm_state_valid(&s));
    puts("Psionic Storm model tests passed");
    return 0;
}
