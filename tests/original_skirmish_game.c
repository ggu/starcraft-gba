/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <stdio.h>
#include <stdlib.h>
#include "../starcraft/game.c"

static void check(int condition, const char *message)
{
    if (!condition) {
        fprintf(stderr, "Frontier check failed: %s (map %d, bots %d, frame %d)\n",
                message, original_skirmish_map, skirmish_bots, frame);
        exit(1);
    }
}

static void press(u16 button)
{
    game_update(button, button);
    game_update(0, 0);
}

/* Independent cell-wise oracle for the original terrain and circular building
 * collision rule, including unfinished buildings and overlapping footprints. */
static void check_walkability_grid(void)
{
    invalidate_path_grid();
    build_path_grid();
    for (int row = 0; row < MAP_ROWS; ++row)
        for (int col = 0; col < MAP_COLS; ++col) {
            int open = !terrain_blocked(col, row);
            for (int i = 0; i < MAX_BUILDINGS && open; ++i) {
                int dx = col * 16 + 8 - b[i].x;
                int dy = row * 16 + 8 - b[i].y;
                if (b[i].alive && dx * dx + dy * dy < 18 * 18) open = 0;
            }
            check(SC64_GRID_GET(path_walkable, col + row * MAP_COLS) == open,
                  "walkability grid matches terrain and building oracle");
        }
}

static void check_walkability_maps(void)
{
    game_init();
    for (int map = 0; map < ORIGINAL_MAP_COUNT; ++map) {
        original_skirmish_map = map;
        memset(b, 0, sizeof b);
        check_walkability_grid();
        /* Overlap and an unfinished structure, plus both clipped map edges. */
        b[0].alive = b[1].alive = b[2].alive = b[3].alive = 1;
        b[0].x = 320; b[0].y = 320;
        b[1].x = 328; b[1].y = 320; b[1].under_construction = 1;
        b[2].x = 0; b[2].y = 0;
        b[3].x = MAP_COLS * 16 - 1; b[3].y = MAP_ROWS * 16 - 1;
        check_walkability_grid();
        b[0].alive = 0;
        check_walkability_grid();
        b[0].alive = 1; b[0].x = 337; b[0].y = 337;
        b[1].under_construction = 0;
        check_walkability_grid();
    }
    /* Switching back with the same structures must restore the other map. */
    original_skirmish_map = 0;
    check_walkability_grid();
}

static void play_match(int map, int bots)
{
    game_init();
    if (map) press(KEY_RIGHT);
    if (bots != 2) press(bots == 1 ? KEY_DOWN : KEY_UP);
    check(screen == SCREEN_TITLE && original_skirmish_map == map && skirmish_bots == bots,
          "title buttons select the expected map and opponent count");
    press(KEY_A);
    check(screen == SCREEN_PLAY && geyser_count == 4, "start a populated match");
    for (int seat = 0; seat < 4; seat++) {
        int owner = seat == 0 ? SIDE : skirmish_owner_ids[seat - 1];
        int hall = owner_command_center(owner);
        if (seat > bots) {
            check(!skirmish_owner_alive(owner), "unused seat is empty");
            continue;
        }
        check(hall >= 0 && b[hall].x == original_skirmish_maps[map].start_x[seat] &&
              b[hall].y == original_skirmish_maps[map].start_y[seat], "hall matches the chosen map");
        check(skirmish_unit_supply_for_owner(owner) == 8, "four workers are active");
        if (seat) check(owners_hostile(SIDE, owner), "each CPU is an opponent");
    }
    check(!won && !lost, "match is running");
    game_render();
    for (int i = 0; i < 900; i++) {
        game_update(0, 0);
        if (i % 180 == 0) game_render();
    }
    for (int seat = 0; seat < bots; seat++) {
        int owner = skirmish_owner_ids[seat];
        int spent_patch = 0;
        for (int i = (seat + 1) * 4; i < (seat + 2) * 4; i++)
            spent_patch |= m[i].amount < 1200;
        check(skirmish_ai[seat].active && spent_patch, "each CPU gathers resources");
        int started = 0;
        for (int i = 0; i < MAX_BUILDINGS; i++)
            if (b[i].alive && b[i].owner == owner && b[i].type == B_BARRACKS &&
                (!b[i].under_construction || b[i].build_progress > 0))
                started = 1;
        check(started, "each CPU advances construction on a production building");
    }
    if (map == ORIGINAL_MAP_COUNT - 1 && bots == 3) {
        int attack = 0;
        while (screen == SCREEN_PLAY && frame < 12000 && !attack) {
            game_update(0, 0);
            for (int slot = 0; slot < MAX_UNITS; slot++)
                if (u[slot].alive && u[slot].owner != SIDE &&
                    u[slot].type == U_MARINE && u[slot].order == ORDER_ATTACK_MOVE)
                    attack = 1;
        }
        check(attack, "a CPU trains an army and sends it to attack");
    }
    press(KEY_START);
    check(screen == SCREEN_PAUSE, "normal pause");
    press(KEY_B);
    int saved_frame = frame;
    check(pause_notice == 1, "normal save");
    game_init();
    game_update(KEY_B, KEY_B);
    check(screen == SCREEN_PLAY && original_skirmish_map == map && skirmish_bots == bots &&
          frame == saved_frame, "load restores the map, AI count, and tick");
    game_update(0, 0);
    check(frame == saved_frame + 1, "a loaded match advances");
    press(KEY_START);
    press(KEY_R);
    check(screen == SCREEN_PLAY && original_skirmish_map == map && skirmish_bots == bots &&
          frame < saved_frame, "restart retains the match setup");
}

static void check_exploration_restore(void)
{
    int worker = -1;
    int target = 32 + 32 * MAP_COLS;
    game_init();
    press(KEY_A);
    for (int i = 0; i < MAX_UNITS; i++)
        if (u[i].alive && u[i].owner == SIDE && IS_WORKER(u[i].type)) {
            worker = i;
            break;
        }
    check(worker >= 0 && !SC64_GRID_GET(explored, target), "scout starts outside unexplored center");
    press(KEY_START);
    press(KEY_B);
    press(KEY_START);
    for (int pass = 0; pass < 3; pass++) {
        /* Move the same source to the same cell after each world replacement. */
        u[worker].x = u[worker].y = 512;
        update_exploration();
        check(SC64_GRID_GET(explored, target) && SC64_GRID_GET(visible, target),
              "a revisited scout center marks both grids");
        if (pass == 2) break;
        press(KEY_START);
        press(pass == 0 ? KEY_L : KEY_R);
        check(screen == SCREEN_PLAY && !SC64_GRID_GET(explored, target) &&
              !SC64_GRID_GET(visible, target), "load and restart restore unexplored center");
    }
}

/* Seed depletion rather than waiting for four full mineral fields to run out.
 * The match itself, and the save/load boundary, use normal button input. */
static void check_cpu_local_mining_recovery(int map, int bots)
{
    int worker = -1, nx, ny;
    game_init();
    if (map) press(KEY_RIGHT);
    if (bots != 2) press(bots == 1 ? KEY_DOWN : KEY_UP);
    press(KEY_A);
    check(screen == SCREEN_PLAY && skirmish_bots == bots, "recovery fixture launches normally");
    int hall = owner_command_center(ENEMY);
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == ENEMY && u[i].type == U_SCV) {
            worker = i;
            break;
        }
    check(hall >= 0 && worker >= 0, "recovery fixture has a CPU hall and worker");
    /* Seat 1's four fields are local; seat 0's fields remain live and remote. */
    for (int i = 4; i < 8; ++i) m[i].amount = 0;
    check(m[0].amount > 0 && dist2(b[hall].x, b[hall].y, m[0].x, m[0].y) > 192 * 192,
          "remote fields remain live outside the local radius");
    u[worker].order = ORDER_IDLE;
    u[worker].target = -1;
    u[worker].carry = 0;
    frame = 60;
    skirmish_update();
    check(u[worker].order == ORDER_IDLE, "CPU does not recover onto a remote field");

    /* Save the exhausted case, then reload by title button and retry recovery. */
    press(KEY_START);
    press(KEY_B);
    check(pause_notice == 1, "recovery fixture saves normally");
    int saved_frame = frame;
    game_init();
    game_update(KEY_B, KEY_B);
    check(screen == SCREEN_PLAY && frame == saved_frame && u[worker].alive &&
          u[worker].owner == ENEMY && u[worker].order == ORDER_IDLE && m[4].amount == 0,
          "load restores the exhausted idle CPU worker");
    skirmish_update();
    check(u[worker].order == ORDER_IDLE, "reloaded CPU does not recover onto a remote field");

    m[4].amount = 1200;
    check(path_next_tile(u[worker].x, u[worker].y, m[4].x, m[4].y, &nx, &ny),
          "local recovery target is reachable");
    b[hall].under_construction = 1;
    skirmish_update();
    check(u[worker].order == ORDER_IDLE, "CPU without a completed hall stays idle");
    b[hall].under_construction = 0;
    skirmish_update();
    check(u[worker].order == ORDER_MINE && u[worker].target == 4,
          "CPU recovers onto an eligible reachable local field");
}

int main(void)
{
    gba_init();
    check_walkability_maps();
    check_exploration_restore();
    for (int map = 0; map < ORIGINAL_MAP_COUNT; map++)
        for (int bots = 1; bots <= 3; bots++) {
            check_cpu_local_mining_recovery(map, bots);
            play_match(map, bots);
        }
    puts("Frontier: all six setups run, each CPU builds an economy, and a CPU army launches an attack.");
    puts("Frontier: all six setups save, reload, and restart.");
    return 0;
}
