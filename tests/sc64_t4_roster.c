/* Internal T4 world-load/save fixture. The mission is still unavailable in
 * the frontend: this does not prove doors, triggers or controller completion. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

static Unit expected_units[MAX_UNITS];
static Building expected_buildings[MAX_BUILDINGS];
static RaceSave valid_save;

static void check_source_roster(void)
{
    const Sc64RaceMap *map = &sc64_race_maps[SC64_T4_GLOBAL_ID];
    unsigned char used_units[MAX_UNITS] = {0}, used_buildings[MAX_BUILDINGS] = {0};
    int owners[5] = {0}, units = 0, buildings = 0;
    assert(map->unit_count == 95 && race_cols == 128 && race_rows == 128);
    assert(race_selected_mission == 7 && !sc64_catalog_mission_allowed(7, R_TER));
    assert(mission_unsupported_count == 0 && minerals == 0 && gas == 0);
    for (unsigned row = 0; row < map->unit_count; ++row) {
        const Sc64ImportedUnit *src = &map->units[row];
        int found = 0;
        for (int i = 0; i < MAX_UNITS; ++i)
            if (!used_units[i] && u[i].alive && u[i].raw_type == src->source_type &&
                u[i].raw_owner == src->owner && u[i].x == src->x_px && u[i].y == src->y_px) {
                used_units[i] = 1; found = 1;
                assert(u[i].flags == src->flags);
                assert(u[i].hp == u[i].maxhp * src->hp_percent / 100);
                break;
            }
        if (!found) for (int i = 0; i < MAX_BUILDINGS; ++i)
            if (!used_buildings[i] && b[i].alive && b[i].raw_type == src->source_type &&
                b[i].raw_owner == src->owner && b[i].x == src->x_px && b[i].y == src->y_px) {
                used_buildings[i] = 1; found = 1;
                assert(b[i].type == B_TERRAN_BEACON && b[i].flags == src->flags);
                assert(b[i].hp == 100000 && b[i].maxhp == 100000);
                assert(invincible_buildings[i]);
                break;
            }
        assert(found);
    }
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive) {
        assert(used_units[i]); ++units; ++owners[u[i].owner];
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive) {
        assert(used_buildings[i]); ++buildings; ++owners[b[i].owner];
    }
    assert(units == 87 && buildings == 8);
    assert(owners[SIDE] == 11 && owners[ENEMY] == 5 && owners[NEUTRAL] == 5);
    assert(owners[RESCUABLE] == 5 && owners[4] == 69);
    assert(race_player[SIDE] == R_TER && race_player[ENEMY] == R_ZERG && race_player[4] == R_TER);
}

static void check_beacon_save_and_atomic_rejection(void)
{
    int beacon = -1;
    for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive && b[i].type == B_TERRAN_BEACON) {
        beacon = i; break;
    }
    assert(beacon >= 0);
    SC64_GRID_SET(explored, MAP_CELLS - 1, 1);
    assert(race_save_game());
    valid_save = race_data;
    assert(valid_save.buildings[beacon][offsetof(Building,hp)/sizeof(int)] == 0);
    assert(valid_save.buildings[beacon][offsetof(Building,maxhp)/sizeof(int)] == 0);
    memcpy(expected_units, u, sizeof(u)); memcpy(expected_buildings, b, sizeof(b));
    memset(u, 0, sizeof(u)); memset(b, 0, sizeof(b)); SC64_GRID_CLEAR(explored);
    assert(race_load_game());
    assert(memcmp(expected_units, u, sizeof(u)) == 0);
    assert(memcmp(expected_buildings, b, sizeof(b)) == 0);
    assert(SC64_GRID_GET(explored, MAP_CELLS - 1));
    assert(race_selected_mission == 7 && race_campaign.phase == SC64_RACE_PHASE_ACTIVE);

    for (int bad = 0; bad < 4; ++bad) {
        RaceSave corrupt = valid_save;
        if (bad == 0) corrupt.buildings[beacon][offsetof(Building,hp)/sizeof(int)] = 1;
        if (bad == 1) corrupt.buildings[beacon][offsetof(Building,maxhp)/sizeof(int)] = 1;
        if (bad == 2) corrupt.invincible_buildings[beacon] = 0;
        if (bad == 3) corrupt.buildings[beacon][offsetof(Building,queue_count)/sizeof(int)] = 1;
        save_write(&corrupt, sizeof(corrupt), SAVE_VERSION);
        assert(!race_load_game());
        assert(memcmp(expected_units, u, sizeof(u)) == 0);
        assert(memcmp(expected_buildings, b, sizeof(b)) == 0);
    }
    save_write(&valid_save, sizeof(valid_save), SAVE_VERSION);
    assert(race_load_game());
    b[beacon].hp = 99999;
    assert(!race_save_game()); /* Never normalize an invalid live prop silently. */
}

int main(void)
{
    game_init(); race_mode = 0; race_selected_mission = 7; init_state();
    check_source_roster();
    check_beacon_save_and_atomic_rejection();
    puts("T4 full source roster and canonical Beacon SRAM pass; scenario remains gated");
    return 0;
}
