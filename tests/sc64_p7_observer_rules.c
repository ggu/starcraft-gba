#define SC64_P7_ACTORS 1
#define SC64_ZP_CAMPAIGN_ACTORS 1
#include "../starcraft/sc64_p7_observer_rules.h"

#include <assert.h>

int main(void)
{
    Sc64P7DetectorState state = {1, 1, 0, 1, 0};
    assert(sc64_p7_observer_is_cloaked(84));
    assert(!sc64_p7_observer_is_cloaked(83));
    assert(sc64_p7_detector_usable(&state, 1));
    assert(sc64_p7_detector_usable(&state, 0));
    assert(sc64_p7_detector_range_source_pixels(84, 1, 0) == 288);
    assert(sc64_p7_detector_range_source_pixels(84, 1, 1) == 352);
    assert(sc64_p7_detector_range_source_pixels(162, 1, 1) == 224);
    assert(sc64_p7_detector_range_source_pixels(162, 0, 1) == 0);
    assert(sc64_p7_detector_range_source_pixels(84, 0, -1) == 288);
    assert(sc64_p7_detector_range_source_pixels(7, 1, 0) == 0);
    assert(!sc64_p7_observer_target_allowed(84, 1, 0));
    assert(!sc64_p7_observer_target_allowed(84, 0, 1));
    assert(sc64_p7_observer_target_allowed(84, 1, 1));
    assert(!sc64_p7_observer_target_allowed(0, 0, 0));
    assert(sc64_p7_observer_target_allowed(0, 1, 0));
    state.completed = 0;
    assert(!sc64_p7_detector_usable(&state, 1));
    state.completed = 1; state.enabled = 0;
    assert(!sc64_p7_detector_usable(&state, 1));
    state.enabled = 1; state.blinded = 1;
    assert(!sc64_p7_detector_usable(&state, 1));
    state.blinded = 0; state.hallucinated = 1;
    assert(!sc64_p7_detector_usable(&state, 0));
    state.hallucinated = 0; state.powered = 0;
    assert(!sc64_p7_detector_usable(&state, 1));
    assert(sc64_p7_detector_usable(&state, 0));
    return 0;
}
