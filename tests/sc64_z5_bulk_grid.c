/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Actual Z5 door action and save/load, scalar versus packed terrain grids. */
#define main sc64_existing_z5_runtime_main
#include "sc64_z5_runtime.c"
#undef main
#ifndef SC64_BULK_TERRAIN_GRID_TEST
#error "requires the opt-in bulk-grid test profile"
#endif
static u8 expected_grid[sizeof(path_walkable)];
static void compare_grid(void)
{
    memset(path_walkable, 0xa5, sizeof(path_walkable));
    sc64_bulk_terrain_force_scalar = 1;
    invalidate_path_grid(); build_path_grid();
    memcpy(expected_grid, path_walkable, sizeof(expected_grid));
    memset(path_walkable, 0xa5, sizeof(path_walkable));
    sc64_bulk_terrain_force_scalar = 0;
    invalidate_path_grid(); build_path_grid();
    assert(!memcmp(expected_grid, path_walkable, sizeof(expected_grid)));
}
int main(void)
{
    game_init(); race_mode = 0; race_selected_mission = SC64_Z5_GLOBAL_ID;
    init_state(); assert(screen == SCREEN_PLAY && race_z5_active());
    compare_grid();
    unsigned door = SC64_Z5_DOODAD_CAPACITY;
    for (unsigned i = 0; i < SC64_Z5_DOODAD_CAPACITY; ++i)
        if (race_z5_is_door_type(sc64_race_building_from_raw(race_z5_doodads[i].raw_type))) {
            door = i; break;
        }
    assert(door < SC64_Z5_DOODAD_CAPACITY);
    unsigned slot = race_z5_doodads[door].building_slot;
    assert(slot < MAX_BUILDINGS && b[slot].alive);
    Sc64Z5SourceAction action = {0}; action.opcode = 42;
    action.unit = race_z5_doodads[door].raw_type; action.group1 = 17;
    for (unsigned i = 0; i < SC64_Z5_SOURCE_LOCATION_COUNT; ++i) {
        const Sc64Z5SourceLocation *loc = &sc64_z5_source_locations[i];
        if (race_z5_doodads[door].x >= loc->left && race_z5_doodads[door].x < loc->right &&
            race_z5_doodads[door].y >= loc->top && race_z5_doodads[door].y < loc->bottom) {
            action.location = (u16)(i + 1); break;
        }
    }
    assert(action.location);
    for (unsigned n = 0; n < 2; ++n) {
        unsigned enabled = race_z5_doodads[door].enabled;
        action.modifier = enabled ? 5 : 4;
        assert(sc64_z5_world_action(&race_z5_world, &action) == 1);
        assert(race_z5_doodads[door].enabled != enabled);
        assert(race_z5_building_collidable((int)slot) == race_z5_doodads[door].enabled);
        compare_grid();
    }
    assert(race_save_game());
    memset(b, 0, sizeof(b)); memset(&race_z5, 0, sizeof(race_z5));
    assert(race_load_game() && race_z5_active()); compare_grid();
    puts("Z5 door actions and save/load scalar/packed grid parity: PASS");
    return 0;
}
