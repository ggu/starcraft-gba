/* SPDX-License-Identifier: GPL-3.0-or-later
 * Synthetic regressions in the actual imported-race movement/combat code.
 * This fixture is not campaign completion or performance evidence. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../starcraft/game.c"

static int attacker, victim;

static void setup_scene(int type, int raw)
{
    game_init();
    init_state();
    memset(u, 0, sizeof(u)); memset(b, 0, sizeof(b));
    memset(invincible_units, 0, sizeof(invincible_units));
    memset(invincible_buildings, 0, sizeof(invincible_buildings));
    memset(visible, 0xff, sizeof(visible));
    memset(explored, 0xff, sizeof(explored));
    memset(path_walkable, 0xff, sizeof(path_walkable));
    memset(race_player, 0xff, sizeof(race_player));
    race_player[SIDE] = R_ZERG; race_player[ENEMY] = R_ZERG;
    race_mode = 1; race_human = R_ZERG;
    path_grid_valid = 1; target_lists_valid = 0; sc64_target_index_active = 0;
    sc64_routes_reset();
    weapons_level = armor_level = 0;
    screen = SCREEN_PLAY; won = lost = 0;
    attacker = spawn_unit(SIDE, type, 400, 400);
    victim = spawn_unit(ENEMY, U_HYDRALISK, 400, 240);
    assert(attacker >= 0 && victim >= 0);
    u[attacker].raw_type = raw; u[attacker].raw_owner = 0;
    u[victim].raw_type = 38; u[victim].raw_owner = 1;
    u[attacker].hp = u[attacker].maxhp = 500;
    u[victim].hp = u[victim].maxhp = 500;
    u[victim].shield = 0;
    /* An open target cell is isolated by terrain, with reachable firing
     * positions outside the ring. This used to retry the center forever. */
    for (int row = 14; row <= 16; ++row) for (int col = 24; col <= 26; ++col)
        SC64_GRID_SET(path_walkable, col + row * MAP_COLS, 0);
    SC64_GRID_SET(path_walkable, 25 + 15 * MAP_COLS, 1);
    u[attacker].path_retry = u[attacker].path_timer = 0;
}

static void approach_and_fire(void)
{
    int nx, ny, fired = 0;
    setup_scene(U_HYDRALISK, 38);
    assert(!direct_path_clear(400, 400, 400, 240));
    assert(!path_next_tile(400, 400, 400, 240, &nx, &ny));
    for (int tick = 0; tick < 500; ++tick) {
        int x = u[attacker].x, y = u[attacker].y, hp = u[victim].hp;
        int distance = dist2(x, y, u[victim].x, u[victim].y);
        tick_combat(&u[attacker], victim);
        assert(abs(u[attacker].x - x) <= 4 && abs(u[attacker].y - y) <= 4);
        if (u[victim].hp != hp) {
            /* A Hydra's unupgraded 128-source-pixel reach stays 64 here. */
            assert(distance <= 64 * 64);
            fired = 1; break;
        }
        if (distance > 64 * 64) assert(u[victim].hp == 500);
    }
#ifdef SC64_ATTACK_RANGE_ROUTES
    assert(fired);
    /* Stock explosive 10 versus a medium Hydralisk takes the 75% factor. */
    assert(u[victim].hp == 493);
#else
    assert(!fired && u[attacker].x == 400 && u[attacker].y == 400);
#endif
}

static void preserve_other_movement(void)
{
    setup_scene(U_ZERGLING, 37);
    for (int tick = 0; tick < 100; ++tick) tick_combat(&u[attacker], victim);
    assert(u[victim].hp == 500);
    assert(u[attacker].x == 400 && u[attacker].y == 400);
#ifdef SC64_ATTACK_RANGE_ROUTES
    assert(sc64_route_valid[SC64_ROUTE_ACTIVE_SLOTS - 1] == 0);
#endif

    setup_scene(U_HYDRALISK, 38);
    /* Explicit MOVE toward that same unreachable center is still a MOVE;
     * the combat fallback does not silently substitute a firing position. */
    for (int tick = 0; tick < 100; ++tick) move_toward(&u[attacker], 400, 240);
    assert(u[attacker].x == 400 && u[attacker].y == 400);
    assert(u[victim].hp == 500);
#ifdef SC64_ATTACK_RANGE_ROUTES
    assert(sc64_route_valid[SC64_ROUTE_ACTIVE_SLOTS - 1] == 0);
#endif
}

#ifdef SC64_DIRECT_ATTACK
static void fog_pursuit_uses_last_seen_only(void)
{
    setup_scene(U_HYDRALISK, 38);
    u[attacker].selected = 1;
    assert(sc64_direct_attack_issue_target(victim) == 1);
    SC64_GRID_CLEAR(visible);
    u[victim].x = 800; u[victim].y = 600;
    for (int tick = 0; tick < 100; ++tick) sc64_direct_attack_tick_unit(attacker);
    assert(u[attacker].tx == 400 && u[attacker].ty == 240);
    assert(u[victim].hp == 500);
#ifdef SC64_ATTACK_RANGE_ROUTES
    assert(sc64_route_valid[SC64_ROUTE_ACTIVE_SLOTS - 1] == 0);
#endif
}
#endif

int main(void)
{
    gba_init();
    approach_and_fire();
    preserve_other_movement();
#ifdef SC64_DIRECT_ATTACK
    fog_pursuit_uses_last_seen_only();
#endif
    puts("Attack-range actual movement/combat regression passed");
    return 0;
}
