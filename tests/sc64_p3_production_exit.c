/* SPDX-License-Identifier: GPL-3.0-or-later */
#define SC64_P3_PRODUCTION_EXIT 1
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

static int test_find_site(int *x, int *y)
{
    for (int row = 8; row < MAP_ROWS - 8; ++row)
        for (int col = 8; col < MAP_COLS - 8; ++col) {
            int px = col * 16, py = row * 16;
            int ex = px + 20, ey = py + 4;
            if (!terrain_blocked(ex / 16, ey / 16) &&
                !terrain_blocked(px / 16, py / 16) &&
                !terrain_blocked((px + 32) / 16, py / 16)) {
                *x = px; *y = py;
                return 1;
            }
        }
    return 0;
}

static void fixture_start(int p3)
{
    game_init();
    race_mode = 0;
    race_selected_mission = p3 ? SC64_P3_GLOBAL_ID : 0;
    init_state();
    assert(screen == SCREEN_PLAY);
    memset(u, 0, sizeof(u));
    memset(b, 0, sizeof(b));
    invalidate_path_grid();
    race_mode = 0;
    race_selected_mission = p3 ? SC64_P3_GLOBAL_ID : 0;
}

static int install_queue(int x, int y, int type, int owner)
{
    const Sc64RaceUnitRule *rule = sc64_race_unit_rule(type);
    int frames;
    assert(rule != 0);
    frames = (rule->build_frames * 5 + 3) / 4;
    b[0].alive = 1;
    b[0].owner = owner;
    b[0].type = B_HATCHERY;
    b[0].x = x; b[0].y = y;
    b[0].queue_count = 1;
    b[0].queue_head = 0;
    b[0].queue[0] = type;
    b[0].queue_progress = frames - 1;
    invalidate_path_grid();
    return frames;
}

static int live_units(void)
{
    int count = 0;
    for (int i = 0; i < MAX_UNITS; ++i) count += !!u[i].alive;
    return count;
}

typedef struct { int x, y; } TestExitPoint;

static unsigned test_exit_points(TestExitPoint *points, int producer_x, int producer_y,
                                 int preferred_x, int preferred_y)
{
    unsigned count = 0;
    points[count++] = (TestExitPoint){ preferred_x, preferred_y };
    points[count++] = (TestExitPoint){ producer_x, producer_y };
    int base_x = sc64_production_exit_floor_step(producer_x);
    int base_y = sc64_production_exit_floor_step(producer_y);
    for (int radius = SC64_PRODUCTION_EXIT_STEP;
         radius <= SC64_PRODUCTION_EXIT_MAX_RADIUS;
         radius += 2 * SC64_PRODUCTION_EXIT_STEP) {
        int left = base_x - radius, right = base_x + radius;
        int top = base_y - radius, bottom = base_y + radius;
        for (int px = left; px <= right; px += SC64_PRODUCTION_EXIT_STEP)
            points[count++] = (TestExitPoint){ px, bottom };
        for (int py = bottom - SC64_PRODUCTION_EXIT_STEP; py > top; py -= SC64_PRODUCTION_EXIT_STEP)
            points[count++] = (TestExitPoint){ right, py };
        for (int px = right; px >= left; px -= SC64_PRODUCTION_EXIT_STEP)
            points[count++] = (TestExitPoint){ px, top };
        for (int py = top + SC64_PRODUCTION_EXIT_STEP; py < bottom; py += SC64_PRODUCTION_EXIT_STEP)
            points[count++] = (TestExitPoint){ left, py };
    }
    assert(count <= SC64_PRODUCTION_EXIT_MAX_CANDIDATES);
    return count;
}

static void block_pair_exit_except_first(int first_x, int first_y, int producer_x, int producer_y)
{
    TestExitPoint points[SC64_PRODUCTION_EXIT_MAX_CANDIDATES];
    unsigned count = test_exit_points(points, producer_x, producer_y, first_x, first_y);
    int building_slot = 1;
    /* Large-radius building blockers cover the broad search area; leave the
     * preferred first point clear so the second-plan failure is isolated. */
    for (int dy = -56; dy <= 56; dy += 28)
        for (int dx = -56; dx <= 56; dx += 28) {
            if (dist2(producer_x + dx, producer_y + dy, first_x, first_y) < 400) continue;
            assert(building_slot < MAX_BUILDINGS);
            b[building_slot].alive = 1; b[building_slot].owner = ENEMY;
            b[building_slot].type = B_HATCHERY;
            b[building_slot].x = producer_x + dx; b[building_slot].y = producer_y + dy;
            ++building_slot;
        }
    invalidate_path_grid();
    for (unsigned p = 0; p < count; ++p) {
        int x = points[p].x, y = points[p].y, covered = 0;
        if (x < 0 || y < 0 || x >= WORLD_W || y >= WORLD_H ||
            dist2(x, y, first_x, first_y) < 36) continue;
        for (int i = 0; i < MAX_BUILDINGS; ++i)
            if (b[i].alive && dist2(x, y, b[i].x, b[i].y) < 400) { covered = 1; break; }
        for (int i = 0; i < MAX_UNITS; ++i)
            if (u[i].alive && dist2(x, y, u[i].x, u[i].y) < 36) { covered = 1; break; }
        if (!covered) {
            int slot = find_unit_slot();
            assert(slot >= 0);
            u[slot].alive = 1; u[slot].owner = ENEMY; u[slot].type = U_ZERGLING;
            u[slot].x = x; u[slot].y = y;
        }
    }
    int second_x = first_x + 8, second_y = first_y, covered = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && dist2(second_x, second_y, u[i].x, u[i].y) < 36) { covered = 1; break; }
    if (!covered) {
        int slot = find_unit_slot();
        assert(slot >= 0);
        u[slot].alive = 1; u[slot].owner = ENEMY; u[slot].type = U_ZERGLING;
        u[slot].x = second_x; u[slot].y = second_y;
    }
    assert(live_units() <= MAX_UNITS - 2);
}

static void test_free_and_occupied_exit(void)
{
    int x, y, before, first = -1;
    fixture_start(1);
    assert(test_find_site(&x, &y));
    int frames = install_queue(x, y, U_ZERGLING, ENEMY);
    race_tick_buildings();
    assert(b[0].queue_count == 0 && live_units() == 2);
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && first < 0) first = i;
    assert(first >= 0 && u[first].x == x + 20 && u[first].y == y + 4);
    int second = first;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (i != first && u[i].alive) second = i;
    assert(second != first && dist2(u[first].x, u[first].y, u[second].x, u[second].y) >= 36);

    fixture_start(1);
    assert(test_find_site(&x, &y));
    frames = install_queue(x, y, U_HYDRALISK, ENEMY);
    int blocker = spawn_unit(SIDE, U_ZERGLING, x + 20, y + 4);
    assert(blocker >= 0);
    before = live_units();
    race_tick_buildings();
    assert(b[0].queue_count == 0 && live_units() == before + 1);
    int spawned = -1;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && i != blocker) spawned = i;
    assert(spawned >= 0 && (u[spawned].x != x + 20 || u[spawned].y != y + 4));
    assert(b[0].queue_progress == 0 && frames > 0);
}

static void test_blocked_retry_unblocks_and_keeps_paid_queue(void)
{
    int x, y, frames;
    fixture_start(1);
    assert(test_find_site(&x, &y));
    frames = install_queue(x, y, U_HYDRALISK, ENEMY);
    /* Dense buildings cover every bounded candidate; no completed unit or
     * refund may occur until the physical exit becomes usable. */
    int slot = 1;
    for (int dy = -50; dy <= 50; dy += 20)
        for (int dx = -50; dx <= 50; dx += 20) {
            if (!dx && !dy) continue;
            assert(slot < MAX_BUILDINGS);
            b[slot].alive = 1; b[slot].owner = ENEMY; b[slot].type = B_HATCHERY;
            b[slot].x = x + dx; b[slot].y = y + dy;
            ++slot;
        }
    invalidate_path_grid();
    race_minerals[ENEMY] = 123; race_gas[ENEMY] = 45;
    supply_cap = 77;
    race_tick_buildings();
    assert(b[0].queue_count == 1 && b[0].queue_head == 0);
    assert(b[0].queue_progress == frames - 8);
    assert(race_minerals[ENEMY] == 123 && race_gas[ENEMY] == 45);
    assert(supply_cap == 77 && live_units() == 0);

    memset(&b[1], 0, sizeof(b) - sizeof(b[0]));
    b[0].queue_progress = frames - 1;
    invalidate_path_grid();
    race_tick_buildings();
    assert(b[0].queue_count == 0 && live_units() == 1);
}

static void test_zergling_pair_failure_is_atomic(void)
{
    int x, y;
    fixture_start(1);
    assert(test_find_site(&x, &y));
    int frames = install_queue(x, y, U_ZERGLING, ENEMY);
    block_pair_exit_except_first(x + 20, y + 4, x, y);
    int occupied = live_units();
    race_tick_buildings();
    assert(live_units() == occupied);
    assert(b[0].queue_count == 1 && b[0].queue_head == 0);
    assert(b[0].queue_progress == frames - 8);
}

static void test_air_ignores_ground_terrain_and_gateoff_keeps_legacy(void)
{
    int x = -1, y = -1;
    fixture_start(1);
    for (int row = 8; row < MAP_ROWS - 8 && x < 0; ++row)
        for (int col = 8; col < MAP_COLS - 8; ++col)
            if (terrain_blocked(col, row)) { x = col * 16; y = row * 16; break; }
    assert(x >= 0);
    install_queue(x, y, U_OVERLORD, ENEMY);
    race_tick_buildings();
    assert(live_units() == 1 && u[0].x == x + 20 && u[0].y == y + 4);

    /* A flying unit can emerge over a ground structure at the preferred
     * point. The same structure remains a blocker for ground exits. */
    fixture_start(1);
    assert(test_find_site(&x, &y));
    install_queue(x, y, U_OVERLORD, ENEMY);
    b[1].alive = 1;
    b[1].owner = SIDE;
    b[1].type = B_GATEWAY;
    b[1].x = x + 20;
    b[1].y = y + 4;
    Sc64P3ProductionExitContext ctx = { U_OVERLORD, 0, 0, 0, 0 };
    assert(sc64_p3_exit_building_blocked(&ctx, x + 20, y + 4, 0));
    assert(!sc64_p3_exit_building_blocked(&ctx, x + 20, y + 4, 1));
    race_tick_buildings();
    assert(live_units() == 1 && u[0].x == x + 20 && u[0].y == y + 4);

    fixture_start(1);
    int terrain_x = -1, terrain_y = -1;
    for (int row = 8; row < MAP_ROWS - 8 && terrain_x < 0; ++row)
        for (int col = 10; col < MAP_COLS - 8; ++col) {
            int px = col * 16 - 20, py = row * 16 - 4;
            if (terrain_blocked(col, row) && px > 32 && py > 32 &&
                px < WORLD_W - 80 && py < WORLD_H - 80) {
                terrain_x = px; terrain_y = py; break;
            }
        }
    assert(terrain_x >= 0);
    install_queue(terrain_x, terrain_y, U_HYDRALISK, ENEMY);
    race_tick_buildings();
    assert(live_units() == 1);
    assert(!terrain_blocked(u[0].x / 16, u[0].y / 16));

    fixture_start(1);
    race_mode = 1;
    assert(test_find_site(&x, &y));
    install_queue(x, y, U_HYDRALISK, ENEMY);
    int blocker = spawn_unit(SIDE, U_ZERGLING, x + 20, y + 4);
    assert(blocker >= 0);
    race_tick_buildings();
    assert(live_units() == 2);
    int spawned = blocker == 0 ? 1 : 0;
    assert(u[spawned].alive && u[spawned].x == x + 20 && u[spawned].y == y + 4);
}

int main(void)
{
    test_free_and_occupied_exit();
    test_blocked_retry_unblocks_and_keeps_paid_queue();
    test_zergling_pair_failure_is_atomic();
    test_air_ignores_ground_terrain_and_gateoff_keeps_legacy();
    puts("P3 production exit integration tests passed");
    return 0;
}
