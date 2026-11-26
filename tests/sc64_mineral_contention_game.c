/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Authored worker scene, independent of campaign and map data. */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/game.c"

static void set_home(int alive)
{
    b[0].alive = alive;
    b[0].owner = SIDE;
    b[0].type = B_COMMAND;
    b[0].x = 64;
    b[0].y = 128;
    b[0].hp = b[0].maxhp = 1000;
}

static void reset_scene(void)
{
    game_init();
    init_state();
    memset(u, 0, sizeof(u));
    memset(b, 0, sizeof(b));
    memset(m, 0, sizeof(m));
    memset(path_walkable, 0xff, sizeof(path_walkable));
    memset(visible, 0xff, sizeof(visible));
    memset(explored, 0xff, sizeof(explored));
    path_grid_valid = 1;
    frame = 0;
#ifdef SC64_RACES
    race_mode = 0;
    race_bots = 0;
#endif
    set_home(1);
    m[0] = (Mineral){ .x = 128, .y = 128, .amount = 100 };
}

static int miner(int patch, int timer)
{
    int slot = spawn_unit(SIDE, U_SCV, m[patch].x, m[patch].y);
    assert(slot >= 0);
    u[slot].order = ORDER_MINE;
    u[slot].target = patch;
    u[slot].build = -1;
    u[slot].work_timer = timer;
    return slot;
}

static void tick(void)
{
    update_orders();
    ++frame;
}

static void held_and_waiting(void)
{
    reset_scene();
    int incumbent = miner(0, 2), waiter = miner(0, 0);
    tick();
    assert(u[incumbent].work_timer == 1 && u[waiter].work_timer == 0);
    tick();
    assert(u[incumbent].carry == 8 && u[waiter].work_timer == 0);
    tick();
    assert(u[waiter].work_timer == SC64_MINERAL_GATHER_UPDATES - 1);

    reset_scene();
    incumbent = miner(0, 2);
    waiter = miner(0, 5);
#if !defined(SC64_RACES) && !defined(SC64_SKIRMISH)
    /* Claims are reconstructed after load, rather than persisted. */
    supply_cap = 20;
    train_type = U_MARINE;
    assert(save_game());
    memset(mineral_patch_claims, 0xff, sizeof(mineral_patch_claims));
    u[waiter].work_timer = 0;
    assert(load_game());
#endif
    tick();
    assert(u[incumbent].work_timer == 1 && u[waiter].work_timer == 5);
    tick();
    assert(u[incumbent].carry == 8 && u[waiter].work_timer == 5);
    tick();
    assert(u[waiter].work_timer == 4);

    reset_scene();
    incumbent = miner(0, 0);
    waiter = miner(0, 0);
    tick();
    assert(u[incumbent].work_timer == SC64_MINERAL_GATHER_UPDATES - 1 && u[waiter].work_timer == 0);
}

static void free_reachable_alternate(void)
{
    reset_scene();
    int incumbent = miner(0, 20), waiter = miner(0, 0);
    m[1] = (Mineral){ .x = 160, .y = 128, .amount = 100 };
    m[2] = (Mineral){ .x = 192, .y = 128, .amount = 100 };
    m[3] = (Mineral){ .x = 224, .y = 128, .amount = 100 };
    SC64_GRID_SET(visible, 192 / 16 + (128 / 16) * MAP_COLS, 0);
    SC64_GRID_SET(explored, 192 / 16 + (128 / 16) * MAP_COLS, 0);
    int other = miner(1, 10);
    frame = (16 - (waiter & 15)) & 15;
    sc64_worker_gather_pending_set(&u[waiter], 1);
    tick();
    assert(u[incumbent].work_timer == 19 && u[other].work_timer == 9);
    assert(u[waiter].target == 3 && u[waiter].work_timer == 0);
#ifdef SC64_WORKER_CARGO_KIND
    assert(sc64_worker_gather_pending(&u[waiter]));
#endif

    reset_scene();
    (void)miner(0, 20);
    waiter = miner(0, 0);
    m[1] = (Mineral){ .x = 176, .y = 128, .amount = 100 };
    m[2] = (Mineral){ .x = 96, .y = 200, .amount = 100 };
    for (int row = 0; row < MAP_ROWS; ++row)
        SC64_GRID_SET(path_walkable, 9 + row * MAP_COLS, 0);
    frame = (16 - (waiter & 15)) & 15;
    tick();
    assert(u[waiter].target == 2 && u[waiter].work_timer == 0);
}

static void loaded_return(void)
{
    reset_scene();
    (void)miner(0, 3);
    int returning = miner(0, 0);
    u[returning].carry = 8;
    set_home(0);
    tick();
    assert(u[returning].carry == 8 && u[returning].target == 0);
    set_home(1);
    u[returning].build = -1;
    tick();
    assert(u[returning].x < 128 && u[returning].carry == 8 && u[returning].target == 0);
}

int main(void)
{
    held_and_waiting();
    free_reachable_alternate();
    loaded_return();
    puts("Mineral contention, legacy timers, alternate routes and loaded return passed.");
    return 0;
}
