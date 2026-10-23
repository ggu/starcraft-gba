/* Independent numeric import check for T6 retained scenario records.
 * Golden digests cover private records; this is not a runtime or mission test. */
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "sc64_t4_triggers.h"
#include "sc64_t4_actions.h"
#define SC64_T4_SCENARIO_RUNTIME
#include "sc64_t4_runtime.h"
#include "sc64_t6_scenario_generated.h"

static uint64_t digest(uint64_t hash, const void *data, size_t size) {
    const u8 *bytes = data;
    while (size--) hash = (hash ^ *bytes++) * UINT64_C(0x100000001b3);
    return hash;
}

#define FNV_OFFSET UINT64_C(0xcbf29ce484222325)

int main(void) {
    const u32 little_endian = 1;
    assert(*(const u8 *)&little_endian == 1);
    assert(offsetof(Sc64T4Condition, opcode) == 15);
    assert(offsetof(Sc64T4Action, opcode) == 26);
    assert(sc64_t6_scenario.location_count == 64);
    assert(sc64_t6_scenario.trigger_count == 20);
    assert(sc64_t6_scenario.doodad_count == 54);
    assert(sizeof(sc64_t6_conditions) / sizeof(sc64_t6_conditions[0]) == 20 * 16);
    assert(sizeof(sc64_t6_actions) / sizeof(sc64_t6_actions[0]) == 20 * 64);
    assert(sizeof(sc64_t6_properties) / sizeof(sc64_t6_properties[0]) == 64);
    assert(sc64_t4_trigger_program_valid(&sc64_t6_scenario));

    unsigned count = 0, property_index = 99, enabled = 99, next_enabled = 99;
    assert(sc64_t4_creation_count(0, &count) && count == 1);
    assert(sc64_t4_creation_property_index(1, &property_index) && property_index == 0);
    assert(sc64_t4_doodad_initial_enabled(0, &enabled) && enabled == 1);
    assert(sc64_t4_doodad_set_state(0, 1, 0, &next_enabled) && next_enabled == 0);

    const Sc64T4SourceSetup *setup = &sc64_t6_source_setup;
    assert(digest(FNV_OFFSET, setup->controllers, 12) == UINT64_C(0x40d97bf7e80c0035));
    assert(digest(FNV_OFFSET, setup->races, 12) == UINT64_C(0xf9b0126ec779c246));
    assert(digest(FNV_OFFSET, setup->force_raw, 20) == UINT64_C(0xfaec8fd23e5575ef));
    assert(digest(FNV_OFFSET, setup->property_used, 64) == UINT64_C(0x78fcd3d1e3bcf0bb));
    assert(memcmp(setup->force_by_slot, setup->force_raw, 8) == 0);

    uint64_t uprp = FNV_OFFSET;
    for (unsigned i = 0; i < 64; ++i) {
        assert(memcmp(&sc64_t6_properties[i], sc64_t6_properties[i].raw_record, 20) == 0);
        uprp = digest(uprp, sc64_t6_properties[i].raw_record, 20);
    }
    assert(uprp == UINT64_C(0x8a2398151eeb3bdd));

    uint64_t trig = FNV_OFFSET, mrgn = FNV_OFFSET, thg2 = FNV_OFFSET;
    unsigned conditions = 0, actions = 0;
    for (unsigned i = 0; i < sc64_t6_scenario.trigger_count; ++i) {
        const Sc64T4Trigger *t = &sc64_t6_scenario.triggers[i];
        trig = digest(trig, t->conditions, 16 * 20);
        trig = digest(trig, t->actions, 64 * 32);
        trig = digest(trig, &t->execution_flags, 4);
        trig = digest(trig, t->enabled_players, 27);
        trig = digest(trig, &t->initial_cursor, 1);
        for (unsigned slot = 0; slot < 16; ++slot)
            conditions += t->conditions[slot].opcode != 0;
        for (unsigned slot = 0; slot < 64; ++slot)
            actions += t->actions[slot].opcode != 0;
    }
    assert(trig == UINT64_C(0xf15164fee12746c8));
    assert(conditions == 31 && actions == 75);

    for (unsigned i = 0; i < sc64_t6_scenario.location_count; ++i) {
        const Sc64T4Location *loc = &sc64_t6_scenario.locations[i];
        assert(memcmp(loc, loc->raw_record, 20) == 0);
        mrgn = digest(mrgn, loc->raw_record, 20);
    }
    assert(mrgn == UINT64_C(0xfff858c6b724a679));
    for (unsigned i = 0; i < sc64_t6_scenario.doodad_count; ++i) {
        const Sc64T4Doodad *d = &sc64_t6_scenario.doodads[i];
        assert(memcmp(d, d->raw_record, 10) == 0);
        thg2 = digest(thg2, d->raw_record, 10);
    }
    assert(thg2 == UINT64_C(0x563746df6ab53044));

    /* Ensure the immutable source table and its supported player slots can
     * seed bounded state; paused stepping does not interpret T6 behavior. */
    for (unsigned player = 0; player < 8; ++player) {
        Sc64T4TriggerState state;
        assert(sc64_t4_trigger_init(&state, &sc64_t6_scenario, player, 1));
        assert(sc64_t4_trigger_state_valid(&state, &sc64_t6_scenario));
        Sc64T4TriggerSwitches switches = {{0}};
        Sc64T4TriggerOps ops = {0};
        assert(sc64_t4_trigger_step(&state, &sc64_t6_scenario, &switches,
                                    &ops, 0, 1));
        assert(sc64_t4_trigger_step_prevalidated(
            &state, &sc64_t6_scenario, &switches, &ops, 0, 1));
    }
    return 0;
}
