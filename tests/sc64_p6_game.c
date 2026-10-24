/* Actual imported-world launch and live scheduler smoke. Completion remains
 * a separate ordinary-controller and ARM qualification. */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/game.c"

int main(void)
{
    unsigned units = 0, buildings = 0, props = 0;
    game_init();
    race_mode = 0;
    race_selected_mission = SC64_P6_GLOBAL_ID;
    race_campaign_init();
    printf("P6 launch screen=%d unsupported=%d mission=%d valid=%d fault=%u\n",
           screen, mission_unsupported_count, race_selected_mission,
           race_p6_runtime_state_valid(), (unsigned)race_p6.runtime.fault);
    assert(screen == SCREEN_PLAY && !mission_unsupported_count);
    assert(race_selected_mission == SC64_P6_GLOBAL_ID);
    for (unsigned i = 0; i < MAX_UNITS; ++i) units += u[i].alive;
    for (unsigned i = 0; i < MAX_BUILDINGS; ++i) buildings += b[i].alive;
    for (unsigned i = 0; i < SC64_P6_DOODAD_SOURCE_COUNT; ++i)
        props += !sc64_p6_doodads[i].sprite_only;
    assert(units == 200 && buildings == 31 && props == 27);
    assert(race_p6_doodad_bindings_still_valid());
    assert(race_p6_runtime_state_valid());
    for (unsigned i = 0; i < 60; ++i) {
        game_update(0, 0);
        assert(screen == SCREEN_PLAY && !won && !lost);
        assert(race_p6_runtime_state_valid());
        assert(race_p6_doodad_bindings_still_valid());
    }
    game_render();
    printf("P6 real source world and 60 live updates passed at frame %u\n",
           (unsigned)frame);
    {
        const Sc64P6SourceTrigger *gate = &sc64_p6_source_triggers[2];
        const Sc64P6SourceAction *open = &sc64_p6_source_actions[gate->first_action];
        const Sc64P6SourceAction *absent = open + 1;
        u8 after[sizeof(race_p6.doodad_enabled)];
        int row = -1;
        assert(open->opcode == SC64_P6_ACTION_SET_DOODAD && open->unit == 207 &&
               open->location == 1 && open->group1 == 17);
        assert(absent->opcode == SC64_P6_ACTION_SET_DOODAD && absent->unit == 208 &&
               absent->location == 1 && absent->group1 == 17);
        for (unsigned i = 0; i < SC64_P6_DOODAD_SOURCE_COUNT; ++i)
            if (sc64_p6_doodads[i].raw_type == 207 &&
                sc64_p6_doodads[i].x == 1696 && sc64_p6_doodads[i].y == 1072)
                row = (int)i;
        assert(row >= 0 && sc64_p6_doodads[row].raw_owner == 0 &&
               race_p6_world.source_slot_active[0]);
        assert(race_p6_doodad_row_enabled((unsigned)row));
        Sc64P6WorldOps ops = {&race_p6_world, sc64_p6_world_condition,
                             sc64_p6_world_action, race_p6_source_ai};
        Sc64P6ActionEvent event = {2, 0};
        race_p6.runtime.model.started |= sc64_p6_trigger_bit(2);
        race_p6.runtime.model.action_cursor[2] = 1;
        race_p6.runtime.action_ticks[2] = 0;
        assert(!race_p6_runtime_state_valid()); /* In-dispatch transaction. */
        assert(sc64_p6_dispatch_action(&race_p6.runtime, &ops, &event) == 0);
        assert(race_p6_runtime_state_valid());
        assert(!race_p6_doodad_row_enabled((unsigned)row));
        memcpy(after, race_p6.doodad_enabled, sizeof(after));
        race_p6.runtime.model.action_cursor[2] = 2;
        race_p6.runtime.action_ticks[2] = 0;
        event.action_index = 1;
        assert(sc64_p6_dispatch_action(&race_p6.runtime, &ops, &event) == 0);
        assert(race_p6_runtime_state_valid());
        assert(!memcmp(after, race_p6.doodad_enabled, sizeof(after)));
        assert(race_p6_doodad_bindings_still_valid());
        puts("P6 neutral-owned gate and empty orientation action passed");
    }
    return 0;
}
