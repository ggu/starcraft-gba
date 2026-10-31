/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Actual T4 door topology and save/load, scalar versus bulk base packing. */
#define main sc64_existing_t4_runtime_main
#include "sc64_t4_runtime.c"
#undef main
#ifndef SC64_BULK_TERRAIN_GRID_TEST
#error "requires the opt-in bulk-grid test profile"
#endif
static u8 expected_grid[sizeof(path_walkable)];
static void compare_grid(void)
{
    memset(path_walkable,0xa5,sizeof(path_walkable));sc64_bulk_terrain_force_scalar=1;
    invalidate_path_grid();build_path_grid();memcpy(expected_grid,path_walkable,sizeof(expected_grid));
    memset(path_walkable,0xa5,sizeof(path_walkable));sc64_bulk_terrain_force_scalar=0;
    invalidate_path_grid();build_path_grid();assert(!memcmp(expected_grid,path_walkable,sizeof(expected_grid)));
}
int main(void)
{
    game_init();race_mode=0;race_selected_mission=SC64_T4_GLOBAL_ID;init_state();
    assert(screen==SCREEN_PLAY && race_t4_active());compare_grid();unsigned doors=0;
    for(unsigned row=0;row<32;row++) {
        unsigned slot=race_t4.doodad_slot[row];
        if(slot>=MAX_BUILDINGS || !sc64_race_t4_is_door(b[slot].type))continue;
        u8 original=race_t4.doodad_enabled[row];
        race_t4.doodad_enabled[row]=0;compare_grid();
        race_t4.doodad_enabled[row]=1;compare_grid();
        race_t4.doodad_enabled[row]=original;++doors;
    }
    assert(doors==14);
    for(int tick=0;tick<120;tick++)game_update(0,0);
    game_update(KEY_START,KEY_START);assert(screen==SCREEN_PAUSE);
    assert(race_save_game());memset(b,0,sizeof(b));memset(&race_t4,0,sizeof(race_t4));
    assert(race_load_game());assert(race_t4_active());compare_grid();
    printf("T4 14 doors open/closed and actual save/load scalar parity: PASS\n");return 0;
}
