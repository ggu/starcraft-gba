/* Full Higher Ground importer/runtime/controller smoke. Compile only with the
 * private P3 map data and the P3 runtime profile. */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/game.c"

static u16 previous_buttons;
static unsigned char matched_units[MAX_UNITS];
static unsigned char matched_buildings[MAX_BUILDINGS];

static void frame_input(u16 held)
{
    game_update(held, held & (u16)~previous_buttons);
    previous_buttons = held;
}

static void press(u16 key)
{
    frame_input(key);
    frame_input(0);
}

static void walk_cursor_to(int x, int y)
{
    int guard = 0;
    /* Cursor movement is 2px/frame without R, so normalize target parity. */
    x = (x + 1) & ~1;
    y = (y + 1) & ~1;
    while (cx < x && guard++ < WORLD_W + WORLD_H) frame_input(KEY_RIGHT);
    while (cx > x && guard++ < WORLD_W + WORLD_H) frame_input(KEY_LEFT);
    while (cy < y && guard++ < WORLD_W + WORLD_H) frame_input(KEY_DOWN);
    while (cy > y && guard++ < WORLD_W + WORLD_H) frame_input(KEY_UP);
    frame_input(0);
    if (cx != x || cy != y)
        fprintf(stderr, "cursor move failed: at %d,%d target %d,%d ticks=%d\n", cx, cy, x, y, guard);
    assert(cx == x && cy == y);
}

static void select_unit_with_controller(int index)
{
    for (int guard = 0; guard < 500; ++guard) {
        int dx = u[index].x - cx, dy = u[index].y - cy;
        if (selection_target() == index) {
            press(KEY_A);
            assert(u[index].selected);
            return;
        }
        if (FAST_ABS(dx) >= FAST_ABS(dy))
            frame_input(KEY_R | (dx >= 0 ? KEY_RIGHT : KEY_LEFT));
        else
            frame_input(KEY_R | (dy >= 0 ? KEY_DOWN : KEY_UP));
    }
    assert(selection_target() == index);
}

static int expected_owner(const Sc64RaceMap *map, int raw_owner)
{
    return race_campaign_owner(map, raw_owner);
}

static int find_unit_source(const Sc64ImportedUnit *src)
{
    int owner = expected_owner(&sc64_race_maps[SC64_P3_GLOBAL_ID], src->owner);
    for (int i = 0; i < MAX_UNITS; ++i)
        if (!matched_units[i] && u[i].alive && u[i].raw_type == src->source_type &&
            u[i].raw_owner == src->owner && u[i].owner == owner &&
            u[i].x == src->x_px && u[i].y == src->y_px && u[i].flags == src->flags &&
            (!src->hp_percent || u[i].hp == u[i].maxhp * src->hp_percent / 100)) {
            matched_units[i] = 1;
            return i;
        }
    return -1;
}

static int find_building_source(const Sc64ImportedUnit *src)
{
    int owner = expected_owner(&sc64_race_maps[SC64_P3_GLOBAL_ID], src->owner);
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (!matched_buildings[i] && b[i].alive && b[i].raw_type == src->source_type &&
            b[i].raw_owner == src->owner && b[i].owner == owner &&
            b[i].x == src->x_px && b[i].y == src->y_px && b[i].flags == src->flags &&
            (!src->hp_percent || b[i].hp == b[i].maxhp * src->hp_percent / 100)) {
            matched_buildings[i] = 1;
            return i;
        }
    return -1;
}

static void enter_higher_ground(void)
{
    game_init();
    previous_buttons = 0;
    assert(screen == SCREEN_TITLE && race_front.page == SC_FRONT_HOME);
    press(KEY_A); /* campaign */
    assert(race_front.page == SC_FRONT_MISSION);
    assert(race_front.faction == SC_FRONT_PROTOSS);
    press(KEY_RIGHT); /* P3 is ordinal two, on page two */
    assert(race_front.mission_page == 1);
    assert(sc64_catalog_playable_at(SC64_FACTION_PROTOSS,
                                    race_front.mission_page * 2 + race_front.row) ==
           SC64_P3_GLOBAL_ID);
    frame_input(KEY_A); /* briefing */
    assert(race_front.page == SC_FRONT_BRIEFING &&
           race_front.mission == SC64_P3_GLOBAL_ID);
    frame_input(0);
    frame_input(KEY_START); /* explicit launch */
    assert(screen == SCREEN_PLAY && !race_mode);
    assert(race_selected_mission == SC64_P3_GLOBAL_ID);
    assert(screen == SCREEN_PLAY && !won && !lost);
}

static void test_full_import_and_diplomacy(void)
{
    const Sc64RaceMap *map;
    int units = 0, buildings = 0, mineral_nodes = 0, gas_nodes = 0;
    int unit_owners[5] = {0}, building_owners[5] = {0};
    memset(matched_units, 0, sizeof(matched_units));
    memset(matched_buildings, 0, sizeof(matched_buildings));
    assert(SC64_P3_GLOBAL_ID < SC64_RACE_MAP_COUNT);
    map = &sc64_race_maps[SC64_P3_GLOBAL_ID];
    assert(map->width == SC64_P3_WIDTH_TILES && map->height == SC64_P3_HEIGHT_TILES);
    assert(map->unit_count == 189);
    assert(map->owner_races[SC64_P3_FORCE2_SLOT] == R_ZERG);
    assert(map->owner_races[SC64_P3_FORCE3_SLOT] == R_ZERG);
    assert(map->owner_races[SC64_P3_HUMAN_SLOT] == R_PROTOSS);
    enter_higher_ground();
    assert(race_campaign_owner(map, SC64_P3_HUMAN_SLOT) == SIDE);
    assert(race_campaign_owner(map, SC64_P3_FORCE2_SLOT) == ENEMY);
    assert(race_campaign_owner(map, SC64_P3_FORCE3_SLOT) == 4);
    assert(race_campaign_owner(map, SC64_P3_NEUTRAL_SLOT) == NEUTRAL);
    assert(race_cols == 128 && race_rows == 96);
    assert(race_p3.outcome == SC64_P3_RUNNING);
    assert(minerals == SC64_P3_HUMAN_MINERALS && gas == SC64_P3_HUMAN_GAS);
    assert(mission_unsupported_count == 0);

    for (int i = 0; i < map->unit_count; ++i) {
        const Sc64ImportedUnit *src = &map->units[i];
        if (src->source_type == 176 || src->source_type == 177 || src->source_type == 178) {
            int found = 0;
            ++mineral_nodes;
            for (int j = 0; j < MAX_MINERALS; ++j)
                if (m[j].amount == (src->resources ? src->resources : 1500) &&
                    m[j].style == src->source_type && m[j].x == src->x_px && m[j].y == src->y_px) {
                    found = 1; break;
                }
            assert(found);
        } else if (src->source_type == 188) {
            int found = 0;
            ++gas_nodes;
            for (int j = 0; j < geyser_count; ++j)
                if (geysers[j].x == src->x_px && geysers[j].y == src->y_px) found = 1;
            assert(found);
        } else {
            int slot = find_unit_source(src);
            if (slot >= 0) {
                ++units;
                ++unit_owners[u[slot].owner];
                continue;
            }
            slot = find_building_source(src);
            if (slot < 0)
                fprintf(stderr, "P3 source record missing: raw=%u owner=%u at %u,%u hp=%u flags=%u\n",
                        src->source_type, src->owner, src->x_px, src->y_px,
                        src->hp_percent, src->flags);
            assert(slot >= 0);
            ++buildings;
            ++building_owners[b[slot].owner];
        }
    }
    assert(units == 96 && buildings == 42 && mineral_nodes == 48 && gas_nodes == 3);
    for (int i = 0; i < MAX_UNITS; ++i) assert(matched_units[i] == !!u[i].alive);
    for (int i = 0; i < MAX_BUILDINGS; ++i) assert(matched_buildings[i] == !!b[i].alive);
    assert(geyser_count == 3);
    assert(unit_owners[SIDE] > 0 && building_owners[SIDE] > 0);
    assert(unit_owners[ENEMY] > 0 && building_owners[ENEMY] > 0);
    assert(unit_owners[4] > 0 && building_owners[4] > 0);
    assert(race_player[ENEMY] == R_ZERG && race_player[4] == R_ZERG);
    assert(!owners_hostile(ENEMY, 4) && !owners_hostile(4, ENEMY));
    assert(owners_hostile(SIDE, ENEMY) && owners_hostile(SIDE, 4));
    assert(owners_hostile(ENEMY, SIDE) && owners_hostile(4, SIDE));
}

static void test_normal_paid_production_and_construction(void)
{
    int nexus = -1, probe = -1, build_x = -1, build_y = -1;
    int before_minerals;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == SIDE && b[i].type == B_NEXUS) { nexus = i; break; }
    assert(nexus >= 0);
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].type == U_PROBE) { probe = i; break; }
    assert(probe >= 0);
    for (int dy = -160; dy <= 160 && build_x < 0; dy += 16)
        for (int dx = -160; dx <= 160; dx += 16) {
            int x = b[nexus].x + dx, y = b[nexus].y + dy;
            if (x < 32 || x >= WORLD_W - 32 || y < 32 || y >= WORLD_H - 32) continue;
            if (race_can_site(SIDE, B_PYLON, x, y) && can_place(B_PYLON, x, y)) {
                build_x = x; build_y = y; break;
            }
        }
    assert(build_x >= 0);

    select_unit_with_controller(probe);
    press(KEY_B); /* open command menu */
    for (int i = 0; i < MENU_DEPOT; ++i) press(KEY_DOWN);
    press(KEY_A); /* race-specific Depot row is Pylon */
    assert(mode == MODE_BUILD && menu == B_PYLON);
    walk_cursor_to(build_x, build_y);
    before_minerals = minerals;
    press(KEY_A); /* place Pylon */
    assert(mode == MODE_NORMAL);
    assert(minerals < before_minerals);
    int found = 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == SIDE && b[i].type == B_PYLON &&
            b[i].x == build_x && b[i].y == build_y) found = 1;
    assert(found);

    /* Selecting the Nexus replaces the worker selection; its second A edge
     * queues a paid Probe through the ordinary producer controls. */
    walk_cursor_to(b[nexus].x, b[nexus].y);
    press(KEY_A);
    assert(selected_building == nexus);
    before_minerals = minerals;
    press(KEY_A);
    assert(b[nexus].queue_count > 0 && b[nexus].queue[(b[nexus].queue_head + b[nexus].queue_count - 1) % 5] == U_PROBE);
    assert(minerals < before_minerals);
}

int main(void)
{
    test_full_import_and_diplomacy();
    test_normal_paid_production_and_construction();
    puts("P3 full-roster, diplomacy, controller and paid production smoke passed");
    return 0;
}
