/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Moving units draw a walk cycle; standing units keep their idle art. */
#include "../starcraft/game.c"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static u8 shot[160][240];

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
    return 0;
}
