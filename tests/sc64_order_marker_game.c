/* Actual field-order acknowledgement and lifetime checks.
 * SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <string.h>
#include "../starcraft/game.c"
#if !defined(SC64_RACES) || !defined(SC64_FIELD_CONTROLS) || !defined(SC64_ORDER_MARKER_ART)
#error "Use a race-engine production vector with the order marker bank"
#endif

static unsigned framebuffer_hash(void)
{
    unsigned h = 2166136261u;
    for (int y = 0; y < 160; ++y)
        for (int x = 0; x < 240; ++x)
            h = (h ^ gfx_test_pixel(x, y)) * 16777619u;
    return h;
}

static void setup(void)
{
    gba_init(); game_init();
    race_mode = 1; race_human = R_TER; race_bots = 1; race_skirmish_map = 0;
    race_skirmish_init(); screen = SCREEN_PLAY;
    memset(u, 0, sizeof(u)); memset(b, 0, sizeof(b));
#if defined(SC64_BACKWATER) || defined(SC64_UNIFIED)
    memset(invincible_units, 0, sizeof(invincible_units));
    memset(invincible_buildings, 0, sizeof(invincible_buildings));
#endif
    race_player[SIDE] = R_TER; race_player[ENEMY] = R_ZERG;
    selected_building = -1; menu_open = 0; mode = MODE_NORMAL;
    ping_life = 0; camx = camy = 0;
}

static int add_unit(int owner, int type, int x, int y)
{
    int i = spawn_unit(owner, type, x, y);
    assert(i >= 0);
    return i;
}

static void test_orders_and_lifetime(void)
{
    setup();
    int a = add_unit(SIDE, U_MARINE, 80, 80);
    u[a].selected = 1;
    cx = 150; cy = 90; mode = MODE_MOVE;
    game_update(0, KEY_A);
    assert(u[a].order == ORDER_MOVE && ping_x == 150 && ping_y == 90);
    assert(ping_color == C_GREEN && ping_life == SC64_ORDER_MARKER_GBA_TICKS);
    cx = 200; cy = 110;
    assert(ping_x == 150 && ping_y == 90);
    gfx_begin(0); field_controls_draw_cursor(cx, cy + 12);
    unsigned first = framebuffer_hash();
    gfx_begin(0); field_controls_draw_cursor(cx, cy + 12);
    assert(framebuffer_hash() == first && ping_life == SC64_ORDER_MARKER_GBA_TICKS);
    camx = 20; camy = 10;
    gfx_begin(0); field_controls_draw_cursor(cx-camx, cy-camy+12);
    assert(framebuffer_hash() != first && ping_x == 150 && ping_y == 90);
    screen = SCREEN_PAUSE;
    game_update(0, 0);
    assert(ping_life == SC64_ORDER_MARKER_GBA_TICKS);
    screen = SCREEN_PLAY;
    for (unsigned remaining = SC64_ORDER_MARKER_GBA_TICKS; remaining; --remaining) {
        unsigned phase;
        assert(ping_life == remaining && sc64_order_marker_phase(ping_life, &phase));
        assert(phase == (SC64_ORDER_MARKER_GBA_TICKS-remaining)*4u/5u);
        screen = SCREEN_PLAY;
        game_update(0, 0);
    }
    assert(!ping_life);
    mode = MODE_ATTACK; cx = 230; cy = 200;
    interact();
    assert(u[a].order == ORDER_ATTACK_MOVE && ping_color == C_RED &&
           ping_x == 230 && ping_y == 200 && ping_life == SC64_ORDER_MARKER_GBA_TICKS);
    /* Smart actions use the same accepted-order acknowledgement. */
    mode = MODE_NORMAL; cx = 260; cy = 200;
    assert(field_ui_smart_move());
    assert(ping_x == 260 && ping_y == 200 && ping_color == C_GREEN);
    int enemy = add_unit(ENEMY, U_ZERGLING, 280, 200);
    SC64_GRID_SET(visible, (200/16)*MAP_COLS + 280/16, 1);
    cx = 282; cy = 200;
    assert(field_ui_smart_attack());
    assert(u[a].order == ORDER_ATTACK_MOVE && ping_color == C_RED &&
           ping_x == u[enemy].x && ping_y == u[enemy].y);
    u[a].selected = 0;
    int old_x = ping_x, old_y = ping_y, old_life = ping_life;
    mode = MODE_MOVE; cx = 250; cy = 220; interact();
    assert(ping_x == old_x && ping_y == old_y && ping_life == old_life);
    field_ui_ping(140, 80, C_GOLD);
    assert(ping_life == 18 && ping_color == C_GOLD);
}

#ifdef SC64_DIRECT_ATTACK
static void test_direct_attack(void)
{
    setup();
    int a = add_unit(SIDE, U_MARINE, 80, 80);
    int t = add_unit(ENEMY, U_ZERGLING, 120, 80);
    u[a].selected = 1;
    SC64_GRID_SET(visible, (80/16)*MAP_COLS + 120/16, 1);
    mode = MODE_ATTACK; cx = 120; cy = 80; interact();
    assert(u[a].order == ORDER_ATTACK_TARGET && u[a].target == t);
    assert(ping_x == 120 && ping_y == 80 && ping_color == C_RED &&
           ping_life == SC64_ORDER_MARKER_GBA_TICKS);
    u[t].x += 16;
    assert(ping_x == 120 && ping_y == 80);
    u[a].selected = 0;
    int noncombat = add_unit(SIDE, U_OVERLORD, 80, 80);
    u[noncombat].selected = 1;
    int old_life = ping_life;
    mode = MODE_ATTACK; cx = u[t].x; cy = u[t].y; interact();
    assert(ping_x == 120 && ping_y == 80 && ping_life == old_life);
}
#endif

static void test_marker_clips_and_preserves_fog(void)
{
    setup();
    u8 old_visible[sizeof(visible)], old_explored[sizeof(explored)];
    memcpy(old_visible, visible, sizeof(visible));
    memcpy(old_explored, explored, sizeof(explored));
    const int ys[] = {0, 107};
    for (unsigned i = 0; i < sizeof(ys)/sizeof(ys[0]); ++i) {
        camx = camy = 0;
        mode = MODE_NORMAL; menu_open = 0; ui_action = UI_A_NONE;
        field_ui_ping(100, ys[i], C_GREEN);
        gfx_begin(3);
        field_controls_draw_cursor(-100, -100);
        for (int y = 0; y < 160; ++y)
            if (y < 12 || y >= 120)
                for (int x = 0; x < 240; ++x)
                    assert(gfx_test_pixel(x, y) == 3);
    }
    assert(!memcmp(old_visible, visible, sizeof(visible)));
    assert(!memcmp(old_explored, explored, sizeof(explored)));
}

static void test_load_clears_only_on_success(void)
{
    setup();
    assert(save_game());
    field_ui_ping(120, 80, C_GREEN);
    assert(load_game());
    assert(!ping_life);
    field_ui_ping(100, 70, C_RED);
    /* Invalidate both alternating records so no fallback can load. */
    save_test_corrupt_latest(sizeof(RaceSave), SAVE_VERSION);
    save_test_corrupt_latest(sizeof(RaceSave), SAVE_VERSION);
    assert(!load_game());
    assert(ping_life == SC64_ORDER_MARKER_GBA_TICKS && ping_x == 100 && ping_y == 70);
}

int main(void)
{
    test_orders_and_lifetime();
#ifdef SC64_DIRECT_ATTACK
    test_direct_attack();
#endif
    test_marker_clips_and_preserves_fog();
    test_load_clears_only_on_success();
    return 0;
}
