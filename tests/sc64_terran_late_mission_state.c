#define SC64_T4_RUNTIME 1
#define SC64_TERRAN_LATE_RUNTIME 1
#include "../starcraft/sc64_terran_late_mission_state.h"
#include <assert.h>
#include <string.h>

int main(void)
{
    Sc64T4Condition conditions[16] = {{0}};
    Sc64T4Action actions[64] = {{0}};
    Sc64T4Trigger trigger = {conditions, actions, {0}, 0, 0};
    Sc64T4Location locations[4] = {{0}};
    Sc64T4Scenario program = {locations, 4, &trigger, 1, 0, 0};
    Sc64T4SourceSetup setup = {{0}, {0}, {0}, {0}, {0}};
    Sc64TerranLateMissionState state;
    memset(&state, 0xa5, sizeof(state));
    assert(sc64_terran_late_mission_init(&state, &program, &setup));
    assert(state.mutable_location_id == SC64_TERRAN_LATE_NO_LOCATION);
    assert(state.emitter_unit == SC64_TERRAN_LATE_NO_SLOT);
    assert(state.emitter_carrier == SC64_TERRAN_LATE_NO_SLOT);
    for (unsigned i = 0; i < 8; ++i) {
        assert(!state.kills[i] && !state.deaths[i]);
        assert(!state.building_kills[i] && !state.building_deaths[i]);
        assert(!state.resources[i].current_minerals);
        assert(!state.resources[i].current_gas);
    }
    assert(sc64_terran_late_mission_state_valid(&state, &program, &setup,
                                                 128, 128, 192));

    state.mutable_location_id = 4;
    state.mutable_location[0] = 128;
    state.mutable_location[1] = 64;
    state.mutable_location[2] = 384;
    state.mutable_location[3] = 192;
    assert(sc64_terran_late_mission_state_valid(&state, &program, &setup,
                                                 128, 128, 192));
    state.mutable_location[2] = 64;
    assert(!sc64_terran_late_mission_state_valid(&state, &program, &setup,
                                                  128, 128, 192));
    state.mutable_location[2] = 384;
    state.mutable_location[3] = 128 * 32u + 1;
    assert(!sc64_terran_late_mission_state_valid(&state, &program, &setup,
                                                  128, 128, 192));
    state.mutable_location_id = SC64_TERRAN_LATE_NO_LOCATION;
    memset(state.mutable_location, 0, sizeof(state.mutable_location));

    state.emitter_unit = 191;
    state.emitter_carrier = 0;
    assert(sc64_terran_late_mission_state_valid(&state, &program, &setup,
                                                 128, 128, 192));
    state.emitter_carrier = 192;
    assert(!sc64_terran_late_mission_state_valid(&state, &program, &setup,
                                                  128, 128, 192));
    state.emitter_unit = SC64_TERRAN_LATE_NO_SLOT;
    state.emitter_carrier = 0;
    assert(!sc64_terran_late_mission_state_valid(&state, &program, &setup,
                                                  128, 128, 192));
    return 0;
}
