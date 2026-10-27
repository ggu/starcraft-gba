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
    int slot = spawn_unit(SIDE, type, 64, 48);
    assert(slot >= 0);
    capture(slot);
    memcpy(walking, shot, sizeof(shot));
    capture(slot);
    assert(!memcmp(walking, shot, sizeof(shot)));   /* standing is stable */

    for (int step = 0; step < 16; ++step) {
        u[slot].x += 2;
        int phase = sc64_walk_track(&u[slot]);
        assert(phase >= 1 && phase <= 4);
        seen[phase - 1] = 1;
        /* draw_unit observes the same position again; stride is held. */
        capture(slot);
        memcpy(walking, shot, sizeof(shot));
        /* Compare with standing art at the same spot. */
        u8 key = sc64_walk_key[slot], state = sc64_walk_state[slot];
        sc64_walk_reset_slot(slot);
        capture(slot);
        assert(memcmp(walking, shot, sizeof(shot)));
        sc64_walk_key[slot] = key; sc64_walk_state[slot] = state;
    }
    assert(seen[0] && seen[1] && seen[2] && seen[3]);

    /* Once stopped, the unit looks exactly as a freshly placed one. */
    for (int i = 0; i <= SC64_WALK_HOLD; ++i) capture(slot);
    memcpy(walking, shot, sizeof(shot));
    sc64_walk_reset_slot(slot);
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
