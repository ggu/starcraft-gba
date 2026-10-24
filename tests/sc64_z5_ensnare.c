#include <assert.h>
#include <string.h>

#define SC64_Z5_RUNTIME 1
#define SC64_Z5_ENSNARE_ENABLED 1
#include "../starcraft/sc64_z5_ensnare.h"

static Sc64Z5EnsnareActor actor(int x, int y, unsigned owner)
{
    Sc64Z5EnsnareActor a;
    memset(&a, 0, sizeof(a));
    a.x = x; a.y = y;
    a.bbox_left = x - 8; a.bbox_top = y - 8;
    a.bbox_right = x + 9; a.bbox_bottom = y + 9;
    a.owner = (u8)owner;
    a.alive = 1; a.completed = 1;
    a.energy_q8 = 100 * 256;
    return a;
}

static void test_authenticated_cast_and_square_effect(void)
{
    Sc64Z5EnsnareActor actors[6] = {
        actor(100, 100, 3), actor(200, 200, 3), actor(264, 264, 7),
        actor(265, 200, 2), actor(200, 265, 4), actor(201, 201, 3)
    };
    Sc64Z5EnsnareStatus statuses[6] = {
        {0, 0}, {4, 3}, {9, 2}, {7, 0}, {3, 1}, {0, 7}
    };
    actors[0].raw_type = SC64_Z5_ENSNARE_HERO_RAW_TYPE;
    actors[0].energy_q8 = 100 * 256;
    actors[2].building = 1;
    actors[3].invincible = 1;
    actors[4].burrowed = 1;
    /* The square is centered on the commanded point, not the hero. Actors on
     * other owners are affected; caster/building/invincible/burrowed are not. */
    int result = sc64_z5_ensnare_apply(actors, statuses, 6, 0, 1,
                                        200, 200, 288, 0);
    assert(result == SC64_Z5_ENSNARE_CAST);
    assert(actors[0].energy_q8 == 25 * 256);
    assert(statuses[0].timer == 0);
    assert(statuses[1].timer == 75);
    assert(statuses[2].timer == 9);
    assert(statuses[3].timer == 7);
    assert(statuses[4].timer == 3);
    assert(statuses[5].timer == 75);
    assert(statuses[1].cycle_counter == 3 && statuses[5].cycle_counter == 7);
}

static void test_ptec_energy_range_and_empty_area(void)
{
    Sc64Z5EnsnareActor actors[2] = {actor(100, 100, 3), actor(600, 600, 7)};
    Sc64Z5EnsnareStatus statuses[2] = {{0, 0}, {0, 0}};
    actors[0].raw_type = 51;
    actors[0].energy_q8 = 75 * 256;
    Sc64Z5EnsnareActor before = actors[0];
    assert(sc64_z5_ensnare_apply(actors, statuses, 2, 0, 0,
                                  100, 100, 0, 0) == SC64_Z5_ENSNARE_UNCHANGED);
    assert(memcmp(&actors[0], &before, sizeof(before)) == 0);
    assert(sc64_z5_ensnare_apply(actors, statuses, 2, 0, 1,
                                  100, 100, 289, 0) == SC64_Z5_ENSNARE_UNCHANGED);
    assert(memcmp(&actors[0], &before, sizeof(before)) == 0);
    assert(!sc64_z5_ensnare_in_range(289, 0));
    assert(sc64_z5_ensnare_in_range(289, 1));
    assert(sc64_z5_ensnare_in_range(288, 0));
    assert(!sc64_z5_ensnare_in_range(-1, 0));
    assert(!sc64_z5_ensnare_in_range(1, -1));

    /* An empty area still spends the 75-energy cast cost. */
    assert(sc64_z5_ensnare_apply(actors, statuses, 2, 0, 1,
                                  100, 100, 288, 0) == SC64_Z5_ENSNARE_CAST);
    assert(actors[0].energy_q8 == 0 && statuses[1].timer == 0);
}

static void test_caster_constraints_and_timer_refresh(void)
{
    for (unsigned mode = 0; mode < 5; ++mode) {
        Sc64Z5EnsnareActor actors[2] = {actor(0, 0, 3), actor(10, 10, 3)};
        Sc64Z5EnsnareStatus statuses[2] = {{0, 0}, {2, 6}};
        actors[0].raw_type = 51;
        if (mode == 0) actors[0].raw_type = 50;
        if (mode == 1) actors[0].completed = 0;
        if (mode == 2) actors[0].hallucinated = 1;
        if (mode == 3) actors[0].disabled = 1;
        if (mode == 4) actors[0].burrowed = 1;
        Sc64Z5EnsnareActor before = actors[0];
        Sc64Z5EnsnareStatus target_before = statuses[1];
        assert(sc64_z5_ensnare_apply(actors, statuses, 2, 0, 1,
                                      10, 10, 0, 0) == SC64_Z5_ENSNARE_UNCHANGED);
        assert(memcmp(&actors[0], &before, sizeof(before)) == 0);
        assert(memcmp(&statuses[1], &target_before, sizeof(target_before)) == 0);
    }
    Sc64Z5EnsnareActor actors[2] = {actor(0, 0, 3), actor(10, 10, 7)};
    Sc64Z5EnsnareStatus statuses[2] = {{0, 0}, {1, 6}};
    actors[0].raw_type = 51;
    assert(sc64_z5_ensnare_apply(actors, statuses, 2, 0, 1,
                                  10, 10, 0, 0) == SC64_Z5_ENSNARE_CAST);
    assert(statuses[1].timer == 75 && statuses[1].cycle_counter == 6);
    assert(actors[0].energy_q8 == 25 * 256);
}

static void test_native_timer_and_30hz_phase_conversion(void)
{
    Sc64Z5EnsnareActor actors[1] = {actor(0, 0, 3)};
    Sc64Z5EnsnareStatus status[1] = {{75, 0}};
    u8 frame_phase = 0;
    unsigned native_frames = 0;
    for (unsigned tick = 0; tick < SC64_Z5_ENSNARE_NOMINAL_GBA_TICKS_30HZ; ++tick) {
        int result = sc64_z5_ensnare_tick_30hz(actors, status, 1,
                                               &frame_phase, 0);
        assert(result == SC64_Z5_ENSNARE_UNCHANGED ||
               result == SC64_Z5_ENSNARE_ADVANCED);
        if (result == SC64_Z5_ENSNARE_ADVANCED) ++native_frames;
    }
    assert(native_frames == SC64_Z5_ENSNARE_NOMINAL_NATIVE_FRAMES);
    assert(status[0].timer == 0 && status[0].cycle_counter == 0);
    assert(frame_phase == 0);

    /* Unit cycle phase may shorten the time from cast to first decrement by
     * up to seven native frames; all subsequent intervals remain eight. */
    status[0].timer = 75;
    status[0].cycle_counter = 7;
    frame_phase = 0;
    native_frames = 0;
    unsigned gba_ticks = 0;
    while (status[0].timer && gba_ticks < 800) {
        if (sc64_z5_ensnare_tick_30hz(actors, status, 1,
                                      &frame_phase, 0) == SC64_Z5_ENSNARE_ADVANCED)
            ++native_frames;
        ++gba_ticks;
    }
    assert(status[0].timer == 0);
    assert(native_frames == 593 && gba_ticks >= 741 && gba_ticks <= 742);

    status[0].timer = 75; status[0].cycle_counter = 0; frame_phase = 0;
    Sc64Z5EnsnareStatus before = status[0];
    for (unsigned i = 0; i < 100; ++i)
        assert(sc64_z5_ensnare_tick_30hz(actors, status, 1,
                                         &frame_phase, 1) == SC64_Z5_ENSNARE_UNCHANGED);
    assert(memcmp(&before, &status[0], sizeof(before)) == 0 && frame_phase == 0);
}

static void test_invalid_inputs_are_transactional(void)
{
    Sc64Z5EnsnareActor actors[2] = {actor(0, 0, 3), actor(10, 10, 3)};
    Sc64Z5EnsnareStatus statuses[2] = {{0, 0}, {5, 3}};
    actors[0].raw_type = 51;
    Sc64Z5EnsnareActor actor_before[2];
    Sc64Z5EnsnareStatus status_before[2];
    memcpy(actor_before, actors, sizeof(actors));
    memcpy(status_before, statuses, sizeof(statuses));
    assert(sc64_z5_ensnare_apply(actors, statuses, 2, 2, 1,
                                  10, 10, 0, 0) == SC64_Z5_ENSNARE_ERROR);
    assert(memcmp(actor_before, actors, sizeof(actors)) == 0);
    assert(memcmp(status_before, statuses, sizeof(statuses)) == 0);
    assert(sc64_z5_ensnare_apply(actors, statuses, 2, 0, 2,
                                  10, 10, 0, 0) == SC64_Z5_ENSNARE_ERROR);
    assert(memcmp(actor_before, actors, sizeof(actors)) == 0);

    statuses[1].timer = 76;
    memcpy(status_before, statuses, sizeof(statuses));
    assert(sc64_z5_ensnare_tick_30hz(actors, statuses, 2,
                                     &(u8){0}, 0) == SC64_Z5_ENSNARE_ERROR);
    assert(memcmp(status_before, statuses, sizeof(statuses)) == 0);
}

int main(void)
{
    test_authenticated_cast_and_square_effect();
    test_ptec_energy_range_and_empty_area();
    test_caster_constraints_and_timer_refresh();
    test_native_timer_and_30hz_phase_conversion();
    test_invalid_inputs_are_transactional();
    return 0;
}
