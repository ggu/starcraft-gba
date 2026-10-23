/* Independent byte reconstruction of the retained T5 numeric scenario.
 * Golden digests cover private source records; this contains no placements,
 * coordinates, dialogue, or action arrays and is not a gameplay proof. */
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "sc64_t4_triggers.h"
#include "sc64_t4_actions.h"
#define SC64_T4_SCENARIO_RUNTIME
#include "sc64_t4_runtime.h"
#include "sc64_t5_scenario_generated.h"

static uint64_t digest(uint64_t hash, const void *data, size_t size)
{
    const u8 *bytes = data;
    while (size--) hash = (hash ^ *bytes++) * UINT64_C(0x100000001b3);
    return hash;
}

#define FNV_OFFSET UINT64_C(0xcbf29ce484222325)

int main(void)
{
    const u32 little_endian = 1;
    assert(*(const u8 *)&little_endian == 1);
    assert(offsetof(Sc64T4Condition, opcode) == 15);
    assert(offsetof(Sc64T4Action, opcode) == 26);
    assert(offsetof(Sc64T4Action, trailing) == 29);
    assert(sc64_t5_scenario.location_count == 64);
    assert(sc64_t5_scenario.trigger_count == 16);
    assert(sc64_t5_scenario.doodad_count == 14);
    assert(sizeof(sc64_t5_conditions) / sizeof(sc64_t5_conditions[0]) == 16 * 16);
    assert(sizeof(sc64_t5_actions) / sizeof(sc64_t5_actions[0]) == 16 * 64);
    assert(sizeof(sc64_t5_properties) / sizeof(sc64_t5_properties[0]) == 64);
    assert(sc64_t4_trigger_program_valid(&sc64_t5_scenario));

    const Sc64T4SourceSetup *setup = &sc64_t5_source_setup;
    assert(digest(FNV_OFFSET, setup->controllers, 12) == UINT64_C(0xd8e9a71f480767c0));
    assert(digest(FNV_OFFSET, setup->races, 12) == UINT64_C(0x141d2a9157386e9c));
    assert(digest(FNV_OFFSET, setup->force_raw, 20) == UINT64_C(0xa8d85b4823fbe1c6));
    assert(memcmp(setup->force_by_slot, setup->force_raw, 8) == 0);
    assert(digest(FNV_OFFSET, setup->property_used, 64) == UINT64_C(0xb9b23f3a46fd0825));

    uint64_t uprp = FNV_OFFSET;
    for (unsigned i = 0; i < 64; ++i) {
        assert(memcmp(&sc64_t5_properties[i], sc64_t5_properties[i].raw_record, 20) == 0);
        uprp = digest(uprp, &sc64_t5_properties[i], 20);
    }
    assert(uprp == UINT64_C(0x963548a7b7b20725));

    uint64_t trig = FNV_OFFSET, mrgn = FNV_OFFSET, thg2 = FNV_OFFSET;
    unsigned conditions = 0, actions = 0;
    for (unsigned i = 0; i < sc64_t5_scenario.trigger_count; ++i) {
        const Sc64T4Trigger *t = &sc64_t5_scenario.triggers[i];
        /* Reconstruct the source TRIG record order: 16 conditions, 64
         * actions, execution flags, enabled-player bytes, then cursor. */
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
    assert(trig == UINT64_C(0x791a1a629eed0987));
    assert(conditions == 18 && actions == 68);

    for (unsigned i = 0; i < sc64_t5_scenario.location_count; ++i) {
        const Sc64T4Location *loc = &sc64_t5_scenario.locations[i];
        assert(memcmp(loc, loc->raw_record, 20) == 0);
        mrgn = digest(mrgn, loc, 20);
    }
    assert(mrgn == UINT64_C(0xbbf6f440563c0126));
    for (unsigned i = 0; i < sc64_t5_scenario.doodad_count; ++i) {
        const Sc64T4Doodad *d = &sc64_t5_scenario.doodads[i];
        assert(memcmp(d, d->raw_record, 10) == 0);
        thg2 = digest(thg2, d, 10);
    }
    assert(thg2 == UINT64_C(0xd640a1eb6efad9b0));

    /* Check source force mapping and full initial trigger-state validity for
     * every source-player slot, including controller-inactive slots. */
    assert(setup->controllers[0] == 6 && setup->controllers[6] == 5);
    for (unsigned player = 0; player < 8; ++player) {
        unsigned force = setup->force_by_slot[player] + 1;
        Sc64T4TriggerState state;
        assert(sc64_t4_trigger_init(&state, &sc64_t5_scenario, player, force));
        assert(sc64_t4_trigger_state_valid(&state, &sc64_t5_scenario));
        for (unsigned i = 0; i < sc64_t5_scenario.trigger_count; ++i) {
            int enabled = sc64_t4_trigger_owner_enabled(
                &sc64_t5_scenario.triggers[i], player, force);
            assert(enabled == (sc64_t5_scenario.triggers[i].enabled_players[player] ||
                                sc64_t5_scenario.triggers[i].enabled_players[17] ||
                                sc64_t5_scenario.triggers[i].enabled_players[17 + force]));
        }
    }

    printf("T5 numeric sections reconstruct the retained source records; import-only evidence\n");
    return 0;
}
