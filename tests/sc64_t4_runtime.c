/* Internal full-world T4 integration. Script-completion fixtures below move
 * actors directly and are explicitly not controller-played mission proofs. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

static Unit checkpoint_units[MAX_UNITS];
static Building checkpoint_buildings[MAX_BUILDINGS];
static Sc64T4RuntimeState checkpoint_t4;
static u8 open_door_grid[SC64_GRID_STORAGE_BYTES];

static void start_t4(void)
{
    game_init(); race_mode = 0; race_selected_mission = SC64_T4_GLOBAL_ID;
    init_state();
    assert(screen == SCREEN_PLAY && race_t4_active());
    assert(!sc64_catalog_mission_allowed(SC64_T4_GLOBAL_ID, R_TER));
}

static void check_core(void)
{
    for (unsigned player = 0; player < 8; ++player) {
        const Sc64T4TriggerState *s = &race_t4.player[player];
        if (s->fault) fprintf(stderr, "T4 fault owner%u trigger%u action/condition%u code%u\n",
                              player, s->fault_trigger, s->fault_slot, s->fault);
        assert(!s->fault && sc64_t4_trigger_state_valid(s, &sc64_t4_scenario));
    }
}

static void source_world_and_doodad_effects(void)
{
    start_t4();
    int units = 0, buildings = 0;
    for (int i = 0; i < MAX_UNITS; ++i) units += !!u[i].alive;
    for (int i = 0; i < MAX_BUILDINGS; ++i) buildings += !!b[i].alive;
    assert(units == 87 && buildings == 34 && race_t4.active_mask == 0xdf);
    unsigned expected_slot = 8, toggles = 0;
    for (unsigned row = 0; row < 32; ++row) {
        const Sc64T4Doodad *d = &sc64_t4_scenario.doodads[row];
        if (d->flags & 0x1000) {
            assert(race_t4.doodad_slot[row] == 255);
            assert(!race_t4.doodad_enabled[row]);
        } else {
            unsigned slot = race_t4.doodad_slot[row];
            assert(slot == expected_slot++ && b[slot].alive);
            assert(b[slot].x == d->x / 2 && b[slot].y == d->y / 2);
            assert(b[slot].raw_type == d->type && b[slot].raw_owner == d->owner);
            assert(b[slot].flags == 0);
            assert(race_t4.doodad_enabled[row] == !(d->flags & 0x8000));
        }
    }
    assert(expected_slot == 34);
    for (unsigned t = 0; t < sc64_t4_scenario.trigger_count; ++t)
        for (unsigned a = 0; a < 64; ++a) {
            const Sc64T4Action *action = &sc64_t4_scenario.triggers[t].actions[a];
            if (action->opcode != 42) continue;
            u8 before[32]; memcpy(before, race_t4.doodad_enabled, sizeof(before));
            assert(race_t4_doodad_action(action, 0) == 1);
            unsigned changed = 0;
            for (unsigned d = 0; d < 32; ++d)
                changed += before[d] != race_t4.doodad_enabled[d];
            assert(changed == 1 && !path_grid_valid);
            assert(race_t4_doodad_action(action, 0) == 1);
            assert(memcmp(before, race_t4.doodad_enabled, sizeof(before)) == 0);
            ++toggles;
        }
    assert(toggles == 20);
    check_core();
}

static void door_collision_restores_source_terrain(void)
{
    start_t4();
    u8 initial[32]; memcpy(initial, race_t4.doodad_enabled, sizeof(initial));
    for (unsigned row = 0; row < 32; ++row) {
        unsigned slot = race_t4.doodad_slot[row];
        if (slot < MAX_BUILDINGS && sc64_race_t4_is_door(b[slot].type))
            race_t4.doodad_enabled[row] = 0;
    }
    invalidate_path_grid(); build_path_grid();
    memcpy(open_door_grid, path_walkable, sizeof(open_door_grid));
    unsigned closed_cells = 0, doors = 0;
    for (unsigned row = 0; row < 32; ++row) {
        unsigned slot = race_t4.doodad_slot[row];
        if (slot >= MAX_BUILDINGS || !sc64_race_t4_is_door(b[slot].type)) continue;
        assert(!sc64_t4_building_collidable((int)slot));
        race_t4.doodad_enabled[row] = 1;
        assert(sc64_t4_building_collidable((int)slot));
        invalidate_path_grid(); build_path_grid();
        for (unsigned cell = 0; cell < MAP_CELLS; ++cell) {
            int original = SC64_GRID_GET(open_door_grid, cell);
            int closed = SC64_GRID_GET(path_walkable, cell);
            assert(!closed || original); /* Closing never adds floor. */
            closed_cells += original && !closed;
        }
        race_t4.doodad_enabled[row] = 0;
        invalidate_path_grid(); build_path_grid();
        assert(memcmp(open_door_grid, path_walkable, sizeof(open_door_grid)) == 0);
        ++doors;
    }
    assert(doors == 14 && closed_cells > 0);
    memcpy(race_t4.doodad_enabled, initial, sizeof(initial));
    invalidate_path_grid();
}

static void real_update_pause_save_resume(void)
{
    start_t4();
    for (int tick = 0; tick < 120; ++tick) {
        game_update(0, 0); check_core();
        assert(screen == SCREEN_PLAY && !won && !lost);
    }
    game_update(KEY_START, KEY_START);
    assert(screen == SCREEN_PAUSE && frame == 120);
    checkpoint_t4 = race_t4;
    memcpy(checkpoint_units, u, sizeof(u));
    memcpy(checkpoint_buildings, b, sizeof(b));
    for (int tick = 0; tick < 60; ++tick) game_update(0, 0);
    assert(frame == 120 && memcmp(&checkpoint_t4, &race_t4, sizeof(race_t4)) == 0);
    assert(race_save_game());
    memset(u, 0, sizeof(u)); memset(b, 0, sizeof(b));
    memset(&race_t4, 0, sizeof(race_t4));
    assert(race_load_game());
    assert(memcmp(checkpoint_units, u, sizeof(u)) == 0);
    assert(memcmp(checkpoint_buildings, b, sizeof(b)) == 0);
    assert(memcmp(&checkpoint_t4, &race_t4, sizeof(race_t4)) == 0);
    assert(screen == SCREEN_PLAY);
    game_update(0, 0); check_core();
    assert(frame == 121 && race_t4.player[0].elapsed_ticks == 121);
}

static void script_win_and_hero_loss(void)
{
    start_t4();
    const Sc64T4Location *goal = &sc64_t4_scenario.locations[19];
    int human = -1;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].type == U_MARINE) {
            human = i; break;
        }
    assert(human >= 0);
    u[human].x = (int)((goal->left + goal->right) / 4);
    u[human].y = (int)((goal->top + goal->bottom) / 4);
    u[human].tx = u[human].x; u[human].ty = u[human].y;
    for (int tick = 0; tick < 600 && !won && !lost; ++tick) {
        game_update(0, 0); check_core();
        assert(screen == SCREEN_PLAY || screen == SCREEN_RESULT);
    }
    assert(won && !lost && race_t4.player[0].outcome == SC64_T4_TR_WON);
    assert(race_campaign.phase == SC64_RACE_PHASE_COMPLETE);
    assert(race_campaign.objective_flags & SC64_RACE_OBJECTIVE_PRIMARY);
    assert(race_save_game());

    start_t4();
    int hero = -1;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].raw_type == 20) { hero = i; break; }
    assert(hero >= 0); u[hero].alive = 0;
    for (int tick = 0; tick < 100 && !lost; ++tick) {
        game_update(0, 0); check_core();
    }
    assert(lost && !won && race_t4.player[0].outcome == SC64_T4_TR_LOST);
    assert(race_campaign.phase == SC64_RACE_PHASE_FAILED);
    assert(race_save_game());
}

int main(void)
{
    source_world_and_doodad_effects();
    door_collision_restores_source_terrain();
    real_update_pause_save_resume();
    script_win_and_hero_loss();
    puts("T4 full-world/update/save and synthetic source-script outcomes pass; controller completion remains unproven");
    return 0;
}
