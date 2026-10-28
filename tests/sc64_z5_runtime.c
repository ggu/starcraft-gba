/* Z5 storage/runtime integration checks. The startup and action probes use
 * the real engine arrays but do not establish ordinary-controller mission
 * completion or source-art pixel parity. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

#ifndef SC64_Z5_RUNTIME
#error "Compile this fixture with the complete opt-in Z5 runtime flags"
#endif

static unsigned prior_keys;

static void input(unsigned held)
{
    game_update((u16)held, (u16)(held & ~prior_keys));
    prior_keys = held;
}

static void press(unsigned key)
{
    input(key);
    input(0);
}

static void launch_z5_from_normal_menu(void)
{
    game_init();
    prior_keys = 0;
    assert(race_front.page == SC_FRONT_HOME);
    assert(sc64_catalog_playable_count(SC64_FACTION_ZERG) == 5);
    assert(sc64_catalog_playable_at(SC64_FACTION_ZERG, 4) ==
           SC64_Z5_GLOBAL_ID);
    press(KEY_A);
    assert(race_front.page == SC_FRONT_MISSION);
    assert(race_front.faction == SC_FRONT_ZERG);
    press(KEY_RIGHT);
    press(KEY_RIGHT);
    assert(race_front.mission_page == 2 && race_front.row == 0);
    press(KEY_A);
    assert(race_front.page == SC_FRONT_BRIEFING &&
           race_front.mission == SC64_Z5_GLOBAL_ID);
    input(KEY_START);
    assert(screen == SCREEN_PLAY && !race_mode && !won && !lost);
    assert(race_selected_mission == SC64_Z5_GLOBAL_ID && race_z5_active());
    assert(race_campaign.mission == SC64_Z5_GLOBAL_ID);
}

static void assert_imported_world_matches_source(void)
{
    const Sc64RaceMap *map = &sc64_race_maps[SC64_Z5_GLOBAL_ID];
    unsigned expected_units = 0, expected_buildings = 0;
    unsigned expected_minerals = 0, expected_geysers = 0;
    unsigned live_units = 0, live_buildings = 0, live_minerals = 0;
    assert(map->width == 128 && map->height == 128);
    for (unsigned i = 0; i < map->unit_count; ++i) {
        unsigned raw = map->units[i].source_type;
        if (raw == 214) continue; /* Start-location marker, not an actor. */
        if (raw >= 176 && raw <= 178) ++expected_minerals;
        else if (raw == 188) ++expected_geysers;
        else if (sc64_race_unit_from_raw((int)raw) >= 0) ++expected_units;
        else if (sc64_race_building_from_raw((int)raw) >= 0)
            ++expected_buildings;
        else assert(!"unmapped authenticated Z5 map row");
    }
    for (unsigned i = 0; i < MAX_UNITS; ++i) live_units += u[i].alive != 0;
    for (unsigned i = 0; i < MAX_BUILDINGS; ++i)
        live_buildings += b[i].alive != 0;
    for (unsigned i = 0; i < MAX_MINERALS; ++i)
        live_minerals += m[i].amount > 0;
    assert(live_units == expected_units);
    assert(live_buildings == expected_buildings + SC64_Z5_DOODAD_CAPACITY);
    assert(live_minerals == expected_minerals);
    assert((unsigned)geyser_count == expected_geysers);
    assert(race_z5_world.doodad_count == SC64_Z5_DOODAD_CAPACITY);
    assert(race_z5_preflight(map));
}

static void assert_save_load_rebind_and_reject_padding(void)
{
    Unit expected_units[MAX_UNITS];
    Building expected_buildings[MAX_BUILDINGS];
    Sc64Z5PersistentState expected_state;
    RaceSave expected_save;
    assert(race_save_game());
    expected_save = race_data;
    memcpy(expected_units, u, sizeof(u));
    memcpy(expected_buildings, b, sizeof(b));
    expected_state = race_z5;
    assert(sc64_z5_persistent_state_valid(&race_data.z5));
    assert(race_load_game());
    assert(!memcmp(expected_units, u, sizeof(u)));
    assert(!memcmp(expected_buildings, b, sizeof(b)));
    assert(!memcmp(&expected_state, &race_z5, sizeof(race_z5)));
    assert(race_z5_world.doodad_count == SC64_Z5_DOODAD_CAPACITY);
    assert(race_save_game());
    assert(!memcmp(&expected_save, &race_data, sizeof(expected_save)));

    RaceSave corrupt = race_data;
    corrupt.z5.doodad_enabled[SC64_Z5_DOODAD_ENABLED_BYTES - 1] |= 0x80u;
    save_write(&corrupt, sizeof(corrupt), SAVE_VERSION);
    memcpy(expected_units, u, sizeof(u));
    memcpy(expected_buildings, b, sizeof(b));
    expected_state = race_z5;
    assert(!race_load_game());
    assert(!memcmp(expected_units, u, sizeof(u)));
    assert(!memcmp(expected_buildings, b, sizeof(b)));
    assert(!memcmp(&expected_state, &race_z5, sizeof(race_z5)));
}

static void assert_regeneration_save_and_pause(void)
{
    int hero = -1;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].raw_type == 51) { hero = i; break; }
    assert(hero >= 0);
    u[hero].hp = 100; u[hero].shield_remainder = 31;
    assert(race_save_game());
    RaceSave retained = race_data;
    race_tick_state();
    assert(u[hero].hp == 100 && u[hero].shield_remainder == 32);
    assert(race_load_game());
    assert(u[hero].hp == 100 && u[hero].shield_remainder == 31);
    for (int tick = 0; tick < 48; ++tick) race_tick_state();
    assert(u[hero].hp == 100 && u[hero].shield_remainder == 79);
    race_tick_state();
    assert(u[hero].hp == 101 && u[hero].shield_remainder == 0);

    input(KEY_START);
    assert(screen == SCREEN_PAUSE);
    for (int tick = 0; tick < 80; ++tick) input(0);
    assert(u[hero].hp == 101 && u[hero].shield_remainder == 0);
    input(KEY_A);
    assert(screen == SCREEN_PLAY);
    input(0);

    u[hero].shield_remainder = 80;
    assert(!race_save_game());
    RaceSave untouched;
    assert(save_read(&untouched, sizeof(untouched), SAVE_VERSION));
    assert(!memcmp(&untouched, &retained, sizeof(retained)));
    u[hero].shield_remainder = 0;
    Unit bad = u[hero]; bad.shield_remainder = 80;
    RaceSave corrupt = retained;
    assert(sc64_zp_unit_save_encode(&corrupt.units[hero], &bad,
                                    (unsigned)WORLD_W, (unsigned)WORLD_H));
    save_write(&corrupt, sizeof(corrupt), SAVE_VERSION);
    Unit units_before[MAX_UNITS]; Building buildings_before[MAX_BUILDINGS];
    memcpy(units_before, u, sizeof(u)); memcpy(buildings_before, b, sizeof(b));
    Sc64Z5PersistentState state_before = race_z5;
    assert(!race_load_game());
    assert(!memcmp(units_before, u, sizeof(u)));
    assert(!memcmp(buildings_before, b, sizeof(b)));
    assert(!memcmp(&state_before, &race_z5, sizeof(race_z5)));
    u[hero].hp = u[hero].maxhp;
}

static void assert_destroyed_trap_continues_and_roundtrips(void)
{
    unsigned row, door = SC64_Z5_DOODAD_CAPACITY;
    for (row = 0; row < SC64_Z5_DOODAD_CAPACITY; ++row)
        if (race_z5_is_trap_type(b[race_z5_doodads[row].building_slot].type)) break;
    assert(row < SC64_Z5_DOODAD_CAPACITY);
    unsigned slot = race_z5_doodads[row].building_slot;
    hit_target(MAX_UNITS + (int)slot, 500, U_MARINE, SIDE);
    assert(!b[slot].alive && b[slot].hp <= 0);
    unsigned old_frame = (unsigned)frame;
    input(0);
    assert(screen == SCREEN_PLAY && (unsigned)frame == old_frame + 1);
    assert(race_z5_restore_doodads() && race_save_game());
    Building expected = b[slot];
    assert(race_load_game());
    assert(!memcmp(&expected, &b[slot], sizeof(expected)));
    assert(race_z5_doodads[row].building_slot == slot);
    input(0);
    assert(screen == SCREEN_PLAY);

    for (row = 0; row < SC64_Z5_DOODAD_CAPACITY; ++row)
        if (race_z5_is_door_type(b[race_z5_doodads[row].building_slot].type)) {
            door = row; break;
        }
    assert(door < SC64_Z5_DOODAD_CAPACITY && race_save_game());
    unsigned door_slot = race_z5_doodads[door].building_slot;
    RaceSave corrupt = race_data;
    Building dead_door = b[door_slot];
    dead_door.alive = 0; dead_door.hp = 0;
    assert(sc64_building_save_encode(&corrupt.buildings[door_slot], &dead_door,
                                    (unsigned)WORLD_W, (unsigned)WORLD_H));
    save_write(&corrupt, sizeof(corrupt), SAVE_VERSION);
    Unit units_before[MAX_UNITS]; Building buildings_before[MAX_BUILDINGS];
    memcpy(units_before, u, sizeof(u)); memcpy(buildings_before, b, sizeof(b));
    Sc64Z5PersistentState state_before = race_z5;
    assert(!race_load_game());
    assert(!memcmp(units_before, u, sizeof(u)));
    assert(!memcmp(buildings_before, b, sizeof(b)));
    assert(!memcmp(&state_before, &race_z5, sizeof(race_z5)));
}

static void assert_pause_ack_and_world_action_path(void)
{
    unsigned door = SC64_Z5_DOODAD_CAPACITY;
    for (unsigned i = 0; i < SC64_Z5_DOODAD_CAPACITY; ++i)
        if (race_z5_is_door_type(sc64_race_building_from_raw(
                                     race_z5_doodads[i].raw_type))) {
            door = i;
            break;
        }
    assert(door < SC64_Z5_DOODAD_CAPACITY);
    unsigned slot = race_z5_doodads[door].building_slot;
    assert(slot < MAX_BUILDINGS && b[slot].alive);
    unsigned old_enabled = race_z5_doodads[door].enabled;
    unsigned old_static_key = sc64_static_objects_key();
    assert(sc64_static_objects_key() == old_static_key);
    Sc64Z5SourceAction toggle = {0};
    const Sc64Z5SourceLocation *loc = NULL;
    toggle.opcode = 42;
    toggle.unit = race_z5_doodads[door].raw_type;
    toggle.group1 = 17;
    toggle.modifier = old_enabled ? 5 : 4;
    for (unsigned i = 0; i < SC64_Z5_SOURCE_LOCATION_COUNT; ++i) {
        const Sc64Z5SourceLocation *candidate = &sc64_z5_source_locations[i];
        if (race_z5_doodads[door].x >= candidate->left &&
            race_z5_doodads[door].x < candidate->right &&
            race_z5_doodads[door].y >= candidate->top &&
            race_z5_doodads[door].y < candidate->bottom) {
            toggle.location = (u16)(i + 1);
            loc = candidate;
            break;
        }
    }
    assert(loc && sc64_z5_world_action(&race_z5_world, &toggle) == 1);
    assert(race_z5_doodads[door].enabled != old_enabled);
    assert(sc64_static_objects_key() != old_static_key);
    unsigned changed_static_key = sc64_static_objects_key();
    assert(sc64_static_objects_key() == changed_static_key);
    assert(race_z5_building_collidable((int)slot) ==
           race_z5_doodads[door].enabled);
    /* Build a valid persisted model boundary for trigger 0's authored
     * PauseGame action, then use the same world-pause dispatch callback. */
    sc64_z5_runtime_init(&race_z5.runtime);
    sc64_z5_mask_set(&race_z5.runtime.model.started_lo,
                     &race_z5.runtime.model.started_hi, 0);
    race_z5.runtime.model.action_cursor[0] = 7;
    Sc64Z5WorldOps ops = sc64_z5_world_ops(&race_z5_world);
    Sc64Z5ActionEvent pause_event = {0, 6};
    assert(sc64_z5_dispatch_action(&race_z5.runtime, &ops, &pause_event) ==
           SC64_Z5_FAULT_NONE);
    assert(sc64_z5_runtime_state_valid(&race_z5.runtime));
    assert(race_z5_world_paused());
    input(0);
    press(KEY_A);
    assert(!race_z5_world_paused() && race_z5.pause_acknowledged);
    /* The callback path rejects unauthored SetInvincibility rather than
     * treating it as a successful no-op. */
    Sc64Z5SourceAction unsupported = {0};
    unsupported.opcode = 43;
    assert(sc64_z5_world_action(&race_z5_world, &unsupported) == 0);
}

int main(void)
{
    gba_init();
    launch_z5_from_normal_menu();
    assert_imported_world_matches_source();
    input(0); /* One ordinary live-engine update dispatches the Z5 scheduler. */
    assert(race_campaign.elapsed_frames == 1);
    assert(sc64_z5_runtime_state_valid(&race_z5.runtime));
    assert_save_load_rebind_and_reject_padding();
    assert_regeneration_save_and_pause();
    assert_destroyed_trap_continues_and_roundtrips();
    assert_pause_ack_and_world_action_path();
    puts("Z5 real-engine import/save/pause/action checks passed; no controller completion proof");
    return 0;
}
