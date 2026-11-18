/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <stdio.h>
#include <stdlib.h>
#include "../starcraft/game.c"

static void check(int condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "Frontier factions: %s (cartridge %d, map %d, bots %d, row %d, frame %d)\n",
                message, SC_FRONTIER_FACTION, original_skirmish_map, skirmish_bots,
                frontier_setup_row, frame);
        exit(1);
    }
}

static void press(u16 key)
{
    game_update(key, key);
    game_update(0, 0);
}

static void row(int target)
{
    int steps = 0;
    while (frontier_setup_row != target && steps++ < 8) press(KEY_DOWN);
    check(frontier_setup_row == target, "setup row reachable with D-pad");
}

static void set_cpu(int seat, unsigned faction)
{
    int steps = 0;
    row(seat + 3);
    while (frontier_cpu_faction[seat] != faction && steps++ < 3) press(KEY_RIGHT);
    check(frontier_cpu_faction[seat] == faction, "CPU faction cycles with D-pad");
}

static void check_mapping(const unsigned expected[3], int bots, int human)
{
    check(frontier_owner_faction(SIDE) == human, "selected player faction");
    for (int i = 0; i < 3; ++i) {
        check(frontier_cpu_faction[i] == expected[i], "CPU configuration retained");
        if (i < bots)
            check(frontier_owner_faction(skirmish_owner_ids[i]) == (int)expected[i],
                  "active CPU owner has selected faction");
    }
}

static void play_match(int map, int bots, int human)
{
    unsigned expected[3] = {1, 2, 0};
    game_init();
    check(screen == SCREEN_TITLE && original_skirmish_map == 0 && skirmish_bots == 2 &&
          frontier_setup_row == 0 && frontier_human_faction == SC_FRONTIER_FACTION, "default title setup");
    for (int i = 0; i < 3; ++i)
        check(frontier_cpu_faction[i] == expected[i], "default CPU factions");
    if (map) press(KEY_RIGHT);
    row(1);
    if (bots == 1) press(KEY_LEFT);
    if (bots == 3) press(KEY_RIGHT);
    check(original_skirmish_map == map && skirmish_bots == bots, "map and bot count selected");
    row(2);
    for (int steps = 0; frontier_human_faction != human && steps < 3; ++steps) press(KEY_RIGHT);
    check(frontier_human_faction == human, "player faction cycles with D-pad");
    for (int i = 0; i < bots; ++i) {
        expected[i] = (unsigned)((map + bots + i / 2) % 3);
        set_cpu(i, expected[i]);
    }
    /* Reducing and restoring the count must not erase an inactive choice. */
    if (bots == 3) {
        row(1); press(KEY_LEFT); press(KEY_RIGHT);
        check(skirmish_bots == 3 && frontier_cpu_faction[2] == expected[2],
              "inactive CPU faction survives bot count changes");
    }
    press(KEY_A);
    check(screen == SCREEN_PLAY && geyser_count == 4, "ordinary A starts match");
    check_mapping(expected, bots, human);
    for (int seat = 0; seat < 4; ++seat) {
        int owner = seat ? skirmish_owner_ids[seat - 1] : SIDE;
        int hall = owner_command_center(owner);
        if (seat > bots) {
            check(!skirmish_owner_alive(owner), "inactive seat empty");
            continue;
        }
        check(hall >= 0 && b[hall].x == original_skirmish_maps[map].start_x[seat] &&
              b[hall].y == original_skirmish_maps[map].start_y[seat], "correct starting hall");
        check(skirmish_unit_supply_for_owner(owner) == 8, "four starting workers");
    }
    /* Exercise every setup above; reserve the longer economy and persistence run
       for one non-default faction per map and bot count. */
    if (human != (SC_FRONTIER_FACTION + 1) % 3) return;
    /* Observe each CPU actually gathering, constructing and training its fighter. */
    int trained[3] = {0, 0, 0};
    for (int tick = 0; tick < 12000 && screen == SCREEN_PLAY; ++tick) {
        game_update(0, 0);
        for (int i = 0; i < MAX_UNITS; ++i)
            if (u[i].alive && u[i].type == U_MARINE)
                for (int seat = 0; seat < bots; ++seat)
                    if (u[i].owner == skirmish_owner_ids[seat]) {
                        static const int hp[] = {40, 32, 85};
                        check(u[i].maxhp == hp[expected[seat]], "trained fighter uses owner HP");
                        trained[seat] = 1;
                    }
        int all = 1;
        for (int seat = 0; seat < bots; ++seat) all &= trained[seat];
        if (all && frame >= 900) break;
    }
    for (int seat = 0; seat < bots; ++seat) {
        int spent = 0;
        for (int i = (seat + 1) * 4; i < (seat + 2) * 4; ++i)
            spent |= m[i].amount < 1200;
        check(skirmish_ai[seat].active && spent, "CPU economy gathers minerals");
        check(trained[seat], "CPU economy trains a fighter");
    }
    check(screen == SCREEN_PLAY, "match remains playable for save");
    press(KEY_START);
    check(screen == SCREEN_PAUSE, "pause with Start");
    int paused_frame = frame, paused_minerals = minerals;
    for (int tick = 0; tick < 800; ++tick) game_update(0, 0);
    check(screen == SCREEN_PAUSE && frame == paused_frame && minerals == paused_minerals,
          "paused match holds the game clock and minerals");
    press(KEY_B);
    int saved_frame = frame;
    check(pause_notice == 1, "save with B");
    game_init();
    game_update(KEY_B, KEY_B);
    check(screen == SCREEN_PLAY && original_skirmish_map == map && skirmish_bots == bots &&
          frame == saved_frame, "title B loads exact match");
    check_mapping(expected, bots, human);
    game_update(0, 0);
    check(frame == saved_frame + 1, "loaded match advances");
    press(KEY_START); press(KEY_R);
    check(screen == SCREEN_PLAY && original_skirmish_map == map && skirmish_bots == bots &&
          frame < saved_frame, "restart retains setup");
    check_mapping(expected, bots, human);
}

static void check_fighter_combat(int human)
{
    static const int hp[] = {40, 32, 85};
    static const int range[] = {64, 48, 32};
    static const int damage[] = {6, 5, 11};
    static const int cooldown[] = {19, 12, 25};
    static const int speed[] = {1, 2, 1};
    game_init();
    row(2);
    for (int steps = 0; frontier_human_faction != human && steps < 3; ++steps) press(KEY_RIGHT);
    check(frontier_human_faction == human, "combat player faction selected");
    press(KEY_A);
    int fighter = spawn_unit(SIDE, U_MARINE, 420, 440);
    int target = spawn_unit(ENEMY, U_SCV, 420 + range[human], 440);
    check(fighter >= 0 && target >= 0, "fighter and attack target spawned");
    check(u[fighter].maxhp == hp[human], "player fighter HP");
    check(frontier_actor_range(&u[fighter]) == range[human], "target acquisition range");
    tick_combat(&u[fighter], target);
    check(u[target].hp == u[target].maxhp - damage[human] &&
          u[fighter].cd == cooldown[human], "fighter hits at its own range and cadence");
    move_toward(&u[fighter], 390, 440);
    check(u[fighter].x == 420 - speed[human] && u[fighter].y == 440,
          "fighter moves at its own speed");
}

static void check_setup_codec(void)
{
    const unsigned char choices[3] = {2, 1, 2};
    unsigned char decoded[3];
    int map, human;
    for (int selected = 0; selected < 3; ++selected) {
        int packed = frontier_pack_setup(1, SC_FRONTIER_FACTION, selected, choices);
        check(packed == (1 | (2 << 1) | (1 << 3) | (2 << 5) |
                         (selected << 7) | ((SC_FRONTIER_FACTION + 1) << 9)),
              "stable setup encoding");
        map = human = -1;
        decoded[0] = decoded[1] = decoded[2] = 9;
        check(frontier_unpack_setup(packed, SC_FRONTIER_FACTION, &map, &human, decoded) &&
              map == 1 && human == selected && decoded[0] == 2 &&
              decoded[1] == 1 && decoded[2] == 2, "setup codec round trip");
        const int invalid[] = {packed | (1 << 11), (packed & ~(3 << 1)) | (3 << 1),
                               (packed & ~(3 << 3)) | (3 << 3),
                               (packed & ~(3 << 5)) | (3 << 5),
                               (packed & ~(3 << 7)) | (3 << 7), -1};
        for (unsigned i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i) {
            map = human = 17; decoded[0] = decoded[1] = decoded[2] = 9;
            check(!frontier_unpack_setup(invalid[i], SC_FRONTIER_FACTION, &map, &human, decoded) &&
                  map == 17 && human == 17 && decoded[0] == 9 && decoded[1] == 9 && decoded[2] == 9,
                  "corrupt setup rejected without mutating outputs");
        }
        map = human = 17; decoded[0] = decoded[1] = decoded[2] = 9;
        check(!frontier_unpack_setup(packed, (SC_FRONTIER_FACTION + 1) % 3, &map, &human, decoded) &&
              map == 17 && human == 17 && decoded[0] == 9 && decoded[1] == 9 && decoded[2] == 9,
              "other cartridge setup rejected without mutating outputs");
    }
    for (int old_map = 0; old_map < 2; ++old_map) {
        int legacy = old_map | (2 << 1) | (1 << 3) | (2 << 5) | (SC_FRONTIER_FACTION << 7);
        map = human = 17; decoded[0] = decoded[1] = decoded[2] = 9;
        check(frontier_unpack_setup(legacy, SC_FRONTIER_FACTION, &map, &human, decoded) &&
              map == old_map && human == SC_FRONTIER_FACTION && decoded[0] == 2 &&
              decoded[1] == 1 && decoded[2] == 2, "legacy cartridge setup accepted");
        map = human = 17; decoded[0] = decoded[1] = decoded[2] = 9;
        int ok = frontier_unpack_setup(old_map, SC_FRONTIER_FACTION, &map, &human, decoded);
        if (SC_FRONTIER_FACTION == 0)
            check(ok && map == old_map && human == 0 && decoded[0] == 0 && decoded[1] == 0 && decoded[2] == 0,
                  "earliest setup defaults to Foundry");
        else
            check(!ok && map == 17 && human == 17 && decoded[0] == 9 && decoded[1] == 9 && decoded[2] == 9,
                  "other cartridges reject earliest setup");
    }
}

int main(void)
{
    gba_init();
    check_setup_codec();
    for (int human = 0; human < 3; ++human)
        check_fighter_combat(human);
    for (int map = 0; map < ORIGINAL_MAP_COUNT; ++map)
        for (int bots = 1; bots <= 3; ++bots)
            for (int human = 0; human < 3; ++human)
                play_match(map, bots, human);
    printf("Frontier faction cartridge %d: eighteen mixed setups, economy, saves and restart passed.\n",
           SC_FRONTIER_FACTION);
    return 0;
}
