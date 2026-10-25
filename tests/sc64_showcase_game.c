/* Host coverage for the hidden unified-profile showcase scenes.
 * Compile with SC64_SHOWCASE_ENABLED and the full P7 typed/rect profile. */
#include <stdio.h>
#include "../starcraft/game.c"

static u16 previous;

static void frame_input(u16 held)
{
    game_update(held, held & (u16)~previous);
    previous = held;
}

static void press(u16 key)
{
    frame_input(key);
    frame_input(0);
}

static int fail(const char *message)
{
    fprintf(stderr, "Showcase: %s\n", message);
    return 1;
}

static int unit_count(int type)
{
    int count = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].type == type) ++count;
    return count;
}

static int owner_count(int owner)
{
    int count = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == owner) ++count;
    return count;
}

static int building_count(int owner)
{
    int count = 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && (owner < 0 || b[i].owner == owner)) ++count;
    return count;
}

static int showcase_has_building(int owner, int type)
{
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == owner && b[i].type == type) return 1;
    return 0;
}

static int valid_world(void)
{
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive) {
        if (u[i].hp <= 0 || u[i].hp > u[i].maxhp ||
            u[i].x < 0 || u[i].x >= WORLD_W ||
            u[i].y < 0 || u[i].y >= WORLD_H ||
            u[i].owner < SIDE || u[i].owner > 4 ||
            u[i].type < 0 || u[i].type >= SC_RU_COUNT) return 0;
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive) {
        if (b[i].hp <= 0 || b[i].hp > b[i].maxhp ||
            b[i].x < 0 || b[i].x >= WORLD_W ||
            b[i].y < 0 || b[i].y >= WORLD_H ||
            b[i].owner < SIDE || b[i].owner > 4) return 0;
    }
    return 1;
}

static int check_scene_contents(int scene)
{
    switch (scene) {
    case 0:
        return unit_count(U_P7_CARRIER) == 3 &&
               unit_count(U_HYDRALISK) >= 12 &&
               unit_count(U_ZP_ULTRALISK) >= 1;
    case 1:
        return unit_count(U_SIEGE_TANK) == 6 &&
               unit_count(U_ZERGLING) == 32;
    case 2: /* Terran last stand */
        return showcase_has_building(SIDE, B_COMMAND) && unit_count(U_SCV) == 4 &&
               owner_count(ENEMY) == 5 && unit_count(U_MARINE) == 1 &&
               unit_count(U_ZERGLING) == 4;
    case 3: /* Zerg last stand */
        return showcase_has_building(SIDE, B_HATCHERY) && unit_count(U_DRONE) == 4 &&
               owner_count(ENEMY) == 5 && unit_count(U_MARINE) == 1 &&
               unit_count(U_ZERGLING) == 4;
    case 4: /* Protoss last stand */
        return showcase_has_building(SIDE, B_NEXUS) && unit_count(U_PROBE) == 4 &&
               owner_count(ENEMY) == 5 && unit_count(U_MARINE) == 1 &&
               unit_count(U_ZERGLING) == 4;
    case 5: /* Protoss mixed force */
        return unit_count(U_ZEALOT) > 0 && unit_count(U_DRAGOON) > 0;
    case 6: /* Three-way battle */
        return owner_count(SIDE) > 0 && owner_count(ENEMY) > 0 &&
               owner_count(RESCUABLE) > 0;
    case 7: /* Base siege */
        return unit_count(U_SIEGE_TANK) > 0 && building_count(-1) > 0;
    default:
        return 0;
    }
}

static int enter_showcase_menu(void)
{
    game_init();
    previous = 0;
    frame_input(KEY_L | KEY_R);
    frame_input(KEY_L | KEY_R | KEY_SELECT);
    frame_input(0);
    return screen == SCREEN_TITLE && race_front.page == SC_FRONT_SHOWCASE;
}

int main(void)
{
    gba_init();
    if (!enter_showcase_menu()) return fail("HOME shoulder chord did not open showcase");
    press(KEY_A);
    if (screen != SCREEN_PAUSE || sc64_showcase_scene != 0)
        return fail("launch did not start scene 0 in paused state");

    /* Seed an actual ordinary race-skirmish save, then prove showcase mode
     * cannot replace it. The menu's default row is not the skirmish row. */
    game_init();
    previous = 0;
    race_mode = 1;
    race_human = R_TER;
    race_bots = 1;
    race_skirmish_map = 0;
    init_state();
    if (!race_mode || !save_game()) return fail("could not seed ordinary skirmish save");
    int saved_minerals = minerals;
    int saved_side_units = owner_count(SIDE);

    if (!enter_showcase_menu()) return fail("could not reopen showcase menu");
    press(KEY_A);
    if (screen != SCREEN_PAUSE || sc64_showcase_scene != 0 || save_game())
        return fail("showcase save guard failed");

    for (int scene = 0; scene < 8; ++scene) {
        if (sc64_showcase_scene != scene || screen != SCREEN_PAUSE ||
            !check_scene_contents(scene) || !valid_world()) {
            fprintf(stderr, "Showcase: bad initial scene %d (scene=%d, screen=%d, units=%d)\n",
                    scene, sc64_showcase_scene, screen, owner_count(SIDE) + owner_count(ENEMY));
            return 1;
        }
        static Unit replay_units[MAX_UNITS];
        static Building replay_buildings[MAX_BUILDINGS];
        memcpy(replay_units,u,sizeof(u));
        memcpy(replay_buildings,b,sizeof(b));
        unsigned frozen_ticks = sc64_showcase_ticks;
        for (int i = 0; i < 30; ++i) frame_input(0);
        if (sc64_showcase_ticks != frozen_ticks)
            return fail("scene advanced while paused");
        game_render();
        press(KEY_R); /* replay current scene */
        if (screen != SCREEN_PAUSE || sc64_showcase_scene != scene ||
            !check_scene_contents(scene) || !valid_world() ||
            memcmp(replay_units,u,sizeof(u)) ||
            memcmp(replay_buildings,b,sizeof(b)))
            return fail("pause replay did not deterministically reseed scene");
        press(KEY_A); /* resume; release update is simulation tick 1 */
        if (screen != SCREEN_PLAY) return fail("A did not resume the scene");
        for (int tick = 0; tick < 300 && screen == SCREEN_PLAY; ++tick) {
            frame_input(0);
            if (!valid_world()) return fail("invalid actor state during scene simulation");
        }
        game_render();

        if (scene >= 2 && scene <= 4) {
            if (screen != SCREEN_RESULT || !lost || sc64_showcase_ticks != 150) {
                fprintf(stderr, "Showcase: last-stand scene %d result=%d won=%d lost=%d tick=%u\n",
                        scene, screen, won, lost, sc64_showcase_ticks);
                return 1;
            }
        }
        if (scene < 7) {
            if (screen == SCREEN_RESULT) press(KEY_B);
            else if (screen == SCREEN_PAUSE) press(KEY_B);
            else { press(KEY_START); press(KEY_B); }
            if (screen != SCREEN_TITLE || race_front.page != SC_FRONT_SHOWCASE ||
                race_front.row != scene)
                return fail("B did not return to the current showcase menu row");
            press(KEY_DOWN);
            press(KEY_A);
            if (screen != SCREEN_PAUSE || sc64_showcase_scene != scene + 1)
                return fail("DOWN/A did not select the next paused scene");
        }
    }

    /* SELECT exits directly to the showcase list and preserves its row. */
    press(KEY_SELECT);
    if (screen != SCREEN_TITLE || race_front.page != SC_FRONT_SHOWCASE ||
        race_front.row != 7) return fail("SELECT did not preserve the current showcase row");

    game_init();
    previous = 0;
    press(KEY_B); /* HOME load: restore the actual ordinary skirmish save */
    if (!race_mode || minerals != saved_minerals || owner_count(SIDE) != saved_side_units)
        return fail("showcase changed or invalidated the ordinary skirmish save");
    puts("Showcase menu, scene runtime, pause controls, and save isolation: ok");
    return 0;
}
