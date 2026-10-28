/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Moving units draw a walk cycle; standing units keep their idle art. */
#include "../starcraft/game.c"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static u8 shot[160][240];

static void rebuild_owner_lists(void)
{
    for (int owner = 0; owner < 5; ++owner)
        target_unit_head[owner] = target_building_head[owner] = -1;
    for (int i = 0; i < MAX_UNITS; ++i) target_unit_next[i] = -1;
    for (int i = 0; i < MAX_BUILDINGS; ++i) target_building_next[i] = -1;
    for (int i = MAX_UNITS - 1; i >= 0; --i) if (u[i].alive) {
        int owner = u[i].owner;
        target_unit_next[i] = target_unit_head[owner];
        target_unit_head[owner] = i;
    }
    for (int i = MAX_BUILDINGS - 1; i >= 0; --i) if (b[i].alive) {
        int owner = b[i].owner;
        target_building_next[i] = target_building_head[owner];
        target_building_head[owner] = i;
    }
    target_lists_valid = 1;
}

static void capture(int slot)
{
    gfx_begin(C_PANEL);
    draw_unit(&u[slot]);
    for (int y = 0; y < 160; ++y)
        for (int x = 0; x < 240; ++x) shot[y][x] = (u8)gfx_test_pixel(x, y);
}

static void test_type(int type, const char *name)
{
    static u8 walking[160][240];
    int seen[4] = {0};
    memset(u, 0, sizeof(u));
    sc64_walk_reset_all();
    camx = camy = 0;
    int slot = spawn_unit(SIDE, type, 32, 48);
    assert(slot >= 0);
    capture(slot);
    memcpy(walking, shot, sizeof(shot));
    capture(slot);
    assert(!memcmp(walking, shot, sizeof(shot)));   /* standing is stable */

    for (int step = 0; step < 32; ++step) {
        int x = u[slot].x;
        move_toward(&u[slot], 200, 48);
        assert(u[slot].x > x);
        capture(slot);
        int phase = sc64_walk_track(&u[slot]);     /* same tick: held */
        assert(phase >= 1 && phase <= 4);
        seen[phase - 1] = 1;
        memcpy(walking, shot, sizeof(shot));
        /* Compare with standing art at the same spot. */
        u8 bits = SC64_WALK_BITS[slot];
        sc64_walk_reset_slot(slot);
        SC64_WALK_BITS[slot] |= (u8)(((unsigned)u[slot].path_timer << SC64_WALK_KEY_SHIFT) &
                                     SC64_WALK_KEY_MASK);
        capture(slot);
        assert(memcmp(walking, shot, sizeof(shot)));
        SC64_WALK_BITS[slot] = bits;
    }
    assert(seen[0] && seen[1] && seen[2] && seen[3]);

    /* Once nothing moves it, the unit looks exactly as a freshly placed one. */
    capture(slot);
    capture(slot);
    memcpy(walking, shot, sizeof(shot));
    sc64_walk_reset_slot(slot);
    u[slot].path_timer = 0;
    capture(slot);
    assert(!memcmp(walking, shot, sizeof(shot)));
    printf("PASS %s walk cycle\n", name);
}

static void test_auto_target_fog(void)
{
    memset(u, 0, sizeof(u));
    memset(b, 0, sizeof(b));
    target_lists_valid = 0;
    SC64_GRID_CLEAR(visible);
    u[0].alive = u[1].alive = 1;
    u[0].owner = u[1].owner = ENEMY;
    u[0].type = u[1].type = U_ZERGLING;
    u[0].x = 176; u[0].y = 160;
    u[1].x = 208; u[1].y = 160;
    SC64_GRID_SET(visible, 208 / 16 + (160 / 16) * MAP_COLS, 1);
    assert(nearest_enemy_typed(160, 160, SIDE, 96, -1) == 1);
    SC64_GRID_CLEAR(visible);
    assert(nearest_enemy_typed(160, 160, SIDE, 96, -1) == -1);
    SC64_GRID_SET(visible, 176 / 16 + (160 / 16) * MAP_COLS, 1);
    assert(nearest_enemy_typed(160, 160, SIDE, 96, -1) == 0);
    SC64_GRID_SET(visible, 176 / 16 + (160 / 16) * MAP_COLS, 0);
    SC64_GRID_SET(visible, 208 / 16 + (160 / 16) * MAP_COLS, 1);
    rebuild_owner_lists();
    assert(nearest_enemy_typed(160, 160, SIDE, 96, -1) == 1);
    SC64_GRID_SET(visible, 208 / 16 + (160 / 16) * MAP_COLS, 0);
    assert(nearest_enemy_typed(160, 160, SIDE, 96, -1) == -1);
    SC64_GRID_SET(visible, 208 / 16 + (160 / 16) * MAP_COLS, 1);

    b[0].alive = 1; b[0].owner = ENEMY; b[0].type = B_HATCHERY;
    b[0].x = 168; b[0].y = 160;
    rebuild_owner_lists();
    assert(nearest_enemy_typed(160, 160, SIDE, 96, -1) == 1);
    SC64_GRID_SET(visible, 168 / 16 + (160 / 16) * MAP_COLS, 1);
    assert(nearest_enemy_typed(160, 160, SIDE, 96, -1) == MAX_UNITS);

    /* Non-player queries remain independent of SIDE's visibility bits. */
    SC64_GRID_CLEAR(visible);
    u[2].alive = 1; u[2].owner = SIDE; u[2].type = U_MARINE;
    u[2].x = 176; u[2].y = 160;
    rebuild_owner_lists();
    assert(nearest_enemy_typed(160, 160, ENEMY, 96, -1) == 2);
    target_lists_valid = 0;
}

int main(void)
{
    game_init();
    test_type(U_MARINE, "marine");
    test_type(U_SCV, "scv");
    test_type(U_ZERGLING, "zergling");
    test_type(U_FIREBAT, "firebat");
    test_type(U_MEDIC, "medic");
    test_type(U_HYDRALISK, "hydralisk");
    test_type(U_RAYNOR, "raynor");
    test_auto_target_fog();
    return 0;
}
