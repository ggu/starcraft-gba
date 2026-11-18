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

int main(void)
{
    gba_init();
    for (int map = 0; map < ORIGINAL_MAP_COUNT; map++)
        for (int bots = 1; bots <= 3; bots++)
            play_match(map, bots);
    puts("Frontier: all six setups run, each CPU builds an economy, and a CPU army launches an attack.");
    puts("Frontier: all six setups save, reload, and restart.");
    return 0;
}
