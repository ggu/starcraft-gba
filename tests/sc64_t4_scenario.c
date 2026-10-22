/* Independent byte reconstruction of all retained T4 numeric scenario data.
 * Digests cover private source sections; no placements/actions are embedded
 * here. This is import evidence, not a playable scenario or N64 timing proof. */
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "sc64_t4_triggers.h"
#include "sc64_t4_actions.h"
#include "sc64_t4_world.h"
#define SC64_T4_SCENARIO_RUNTIME
#include "sc64_t4_runtime.h"
#include "sc64_t4_scenario_generated.h"

static uint64_t
digest(uint64_t hash, const void *data, size_t size)
{
    const u8 *bytes = data;
    while (size--) hash = (hash ^ *bytes++) * UINT64_C(0x100000001b3);
    return hash;
}

int main(void)
{
    uint64_t trig = UINT64_C(0xcbf29ce484222325);
    uint64_t mrgn = trig, thg2 = trig;
    unsigned conditions = 0, actions = 0, units = 0, sprites = 0, disabled = 0;
    unsigned creates = 0, toggles = 0;
    const u32 little_endian = 1;
    assert(*(const u8 *)&little_endian == 1);
    assert(sc64_t4_scenario.trigger_count == 38);
    assert(sc64_t4_scenario.location_count == 255);
    assert(sc64_t4_scenario.doodad_count == 32);
    assert(offsetof(Sc64T4Condition, opcode) == 15);
    assert(offsetof(Sc64T4Action, opcode) == 26);
    assert(offsetof(Sc64T4Action, trailing) == 29);
    assert(sc64_t4_trigger_program_valid(&sc64_t4_scenario));
    const Sc64T4SourceSetup *setup = &sc64_t4_source_setup;
    assert(memcmp(setup->force_by_slot, setup->force_raw, 8) == 0);
    assert(digest(UINT64_C(0xcbf29ce484222325), setup->force_raw, 20) == UINT64_C(0x9d968b7fa0513e2d));
    assert(digest(UINT64_C(0xcbf29ce484222325), setup->controllers, 12) == UINT64_C(0xcd05340d49ff4e7b));
    assert(digest(UINT64_C(0xcbf29ce484222325), setup->races, 12) == UINT64_C(0x2dae1e0964e7fe16));
    assert(digest(UINT64_C(0xcbf29ce484222325), setup->property_used, 64) == UINT64_C(0xab165ceafad04724));
    uint64_t uprp = UINT64_C(0xcbf29ce484222325);
    for (unsigned i = 0; i < 64; ++i) {
        assert(memcmp(&sc64_t4_properties[i], sc64_t4_properties[i].raw_record, 20) == 0);
        uprp = digest(uprp, &sc64_t4_properties[i], 20);
    }
    assert(uprp == UINT64_C(0xcae8be725a0f762c));
    for (unsigned player = 0; player < 8; ++player)
        assert(sc64_t4_source_force((int)player) == setup->force_by_slot[player] + 1);
    for (unsigned i = 0; i < sc64_t4_scenario.trigger_count; ++i) {
        const Sc64T4Trigger *t = &sc64_t4_scenario.triggers[i];
        /* T4 uses direct Human-slot or AllPlayers enables. The latter must
         * not be collapsed into the human's trigger cursor. */
        for (unsigned player = 0; player < 8; ++player)
            assert(sc64_t4_trigger_owner_enabled(t, player,
                       setup->force_by_slot[player] + 1) ==
                   (player == 0 || t->enabled_players[17]));
        trig = digest(trig, t->conditions, 16 * 20);
        trig = digest(trig, t->actions, 64 * 32);
        trig = digest(trig, &t->execution_flags, 4);
        trig = digest(trig, t->enabled_players, 27);
        trig = digest(trig, &t->initial_cursor, 1);
        for (unsigned slot = 0; slot < 16; ++slot)
            conditions += t->conditions[slot].opcode != 0;
        for (unsigned slot = 0; slot < 64; ++slot) {
            actions += t->actions[slot].opcode != 0;
            if (t->actions[slot].opcode == 11) {
                unsigned amount;
                assert(sc64_t4_creation_count(t->actions[slot].modifier, &amount));
                creates += amount;
            } else if (t->actions[slot].opcode == 42) {
                unsigned enabled;
                assert(sc64_t4_doodad_set_state(0, 1, t->actions[slot].modifier, &enabled));
                assert(enabled == 0); ++toggles;
            }
        }
    }
    for (unsigned i = 0; i < sc64_t4_scenario.location_count; ++i) {
        const Sc64T4Location *loc = &sc64_t4_scenario.locations[i];
        assert(memcmp(loc, loc->raw_record, 20) == 0);
        mrgn = digest(mrgn, loc, 20);
    }
    for (unsigned i = 0; i < sc64_t4_scenario.doodad_count; ++i) {
        const Sc64T4Doodad *d = &sc64_t4_scenario.doodads[i];
        assert(memcmp(d, d->raw_record, 10) == 0);
        thg2 = digest(thg2, d, 10);
        unsigned enabled = 99;
        if (d->flags & 0x1000) {
            assert(!sc64_t4_doodad_initial_enabled(d->flags, &enabled));
            assert(enabled == 99); ++sprites;
        } else {
            assert(sc64_t4_doodad_initial_enabled(d->flags, &enabled));
            ++units; disabled += !enabled;
        }
    }
    assert(trig == UINT64_C(0x724e391d1ef5a052));
    assert(mrgn == UINT64_C(0xf90adeabec71769d));
    assert(thg2 == UINT64_C(0x6ed0b13b1503a24e));
    assert(conditions == 48 && actions == 141);
    assert(units == 26 && sprites == 6 && disabled == 2);
    assert(creates == 5 && toggles == 20);
    Sc64T4TriggerState state;
    assert(sc64_t4_trigger_init(&state, &sc64_t4_scenario, 0, 1));
    assert(sc64_t4_trigger_state_valid(&state, &sc64_t4_scenario));
    printf("T4 scenario bytes match all eight numeric scenario/setup sections; player state %zu, world effects %zu bytes; mission gated\n",
           sizeof(state), sizeof(Sc64T4RuntimeState));
    return 0;
}
