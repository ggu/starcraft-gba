/* Controller-only selection gesture regression. The fixture is built directly;
 * every gesture itself is delivered through game_update() key edges. */
#include <stdio.h>
#include "../starcraft/game.c"

static u16 previous_held;

static void
input(u16 held)
{
    u16 pressed = held & (u16)~previous_held;
    game_update(held, pressed);
    previous_held = held;
}

static void
tap(u16 key)
{
    input(key);
    input(0);
}

static int
fail(int code, const char *message)
{
    fprintf(stderr, "FAIL selection %d: %s\n", code, message);
    return code;
}

static u32
selection_mask(void)
{
    int i;
    u32 result = 0;
    for (i = 0; i < MAX_UNITS && i < 32; i++)
        if (u[i].alive && u[i].selected)
            result |= (u32)1 << i;
    return result;
}

static int
selected_units(void)
{
    int i, count = 0;
    for (i = 0; i < MAX_UNITS; i++)
        if (u[i].alive && u[i].selected)
            count++;
    return count;
}

static void
put_unit(int slot, int owner, int type, int x, int y, int selected, int alive)
{
    u[slot].owner = owner;
    u[slot].type = type;
    u[slot].x = x;
    u[slot].y = y;
    u[slot].hp = u[slot].maxhp = 40;
    u[slot].alive = alive;
    u[slot].selected = selected;
    u[slot].order = ORDER_IDLE;
    u[slot].target = -1;
}

static void
fixture(void)
{
    previous_held = 0;
    init_state();
    memset(u, 0, sizeof(u));
    memset(b, 0, sizeof(b));
    memset(m, 0, sizeof(m));
    selected_building = -1;
    menu_open = 0;
    help = 0;
    mode = MODE_NORMAL;
    screen = SCREEN_PLAY;
    frame = 0;
    supply_used = 0;
    supply_cap = 20;
    minerals = 500;
    gas = 0;
    cx = 80;
    cy = 80;
    camx = camy = 0;
    /* Keep imported outcome checks alive without allowing these distant
     * fixture actors into the low-slot selection assertions. */
    u[MAX_UNITS - 1].type = U_RAYNOR;
    u[MAX_UNITS - 1].owner = NEUTRAL;
    u[MAX_UNITS - 1].x = WORLD_W - 48;
    u[MAX_UNITS - 1].y = WORLD_H - 48;
    u[MAX_UNITS - 1].hp = u[MAX_UNITS - 1].maxhp = 200;
    u[MAX_UNITS - 1].alive = 1;
    u[MAX_UNITS - 1].target = -1;
    u[MAX_UNITS - 3].owner = ENEMY;
    u[MAX_UNITS - 3].type = U_ZERGLING;
    u[MAX_UNITS - 3].x = WORLD_W - 16;
    u[MAX_UNITS - 3].y = WORLD_H - 16;
    u[MAX_UNITS - 3].hp = u[MAX_UNITS - 3].maxhp = 40;
    u[MAX_UNITS - 3].alive = 1;
    u[MAX_UNITS - 3].target = -1;
    u[MAX_UNITS - 2].owner = SIDE;
    u[MAX_UNITS - 2].type = U_SCV;
    u[MAX_UNITS - 2].x = WORLD_W - 32;
    u[MAX_UNITS - 2].y = WORLD_H - 32;
    u[MAX_UNITS - 2].hp = u[MAX_UNITS - 2].maxhp = 60;
    u[MAX_UNITS - 2].alive = 1;
    u[MAX_UNITS - 2].order = ORDER_MOVE;
    u[MAX_UNITS - 2].tx = u[MAX_UNITS - 2].x;
    u[MAX_UNITS - 2].ty = u[MAX_UNITS - 2].y;
    b[MAX_BUILDINGS - 1].owner = SIDE;
    b[MAX_BUILDINGS - 1].type = B_COMMAND;
    b[MAX_BUILDINGS - 1].x = WORLD_W - 48;
    b[MAX_BUILDINGS - 1].y = WORLD_H - 48;
    b[MAX_BUILDINGS - 1].hp = b[MAX_BUILDINGS - 1].maxhp = 200;
    b[MAX_BUILDINGS - 1].alive = 1;
    input(0);
    previous_held = 0;
}

static void
drag_to(int x, int y, int modifiers)
{
    int guard = 0;
    while ((cx != x || cy != y) && guard++ < 1000) {
        u16 held = (u16)(KEY_L | modifiers);
        if (cx < x) held |= KEY_RIGHT;
        else if (cx > x) held |= KEY_LEFT;
        if (cy < y) held |= KEY_DOWN;
        else if (cy > y) held |= KEY_UP;
        input(held);
    }
}

static void
begin_drag(void)
{
    input(KEY_L);
}

static void
end_drag(void)
{
    input(0);
}

static int
box_tests(void)
{
    const u32 expected = ((u32)1 << 0) | ((u32)1 << 1);
    fixture();
    put_unit(0, SIDE, U_MARINE, 100, 100, 0, 1);
    put_unit(1, SIDE, U_SCV, 120, 120, 0, 1); /* inclusive lower/right edge */
    put_unit(2, SIDE, U_FIREBAT, 124, 120, 0, 1); /* just outside */
    put_unit(3, ENEMY, U_ZERGLING, 110, 110, 0, 1);
    put_unit(4, NEUTRAL, U_SCV, 110, 120, 0, 1);
    put_unit(5, SIDE, U_MARINE, 112, 112, 0, 0);
    b[0].owner = SIDE;
    b[0].type = B_BARRACKS;
    b[0].x = cx;
    b[0].y = cy;
    b[0].hp = b[0].maxhp = 100;
    b[0].alive = 1;
    selected_building = 0; /* box begins over a selected building */
    begin_drag();
    drag_to(120, 120, KEY_R);
    end_drag();
    if (selection_mask() != expected) return 1;

    /* Reverse direction must produce the same normalized inclusive box. */
    cx = 120; cy = 120;
    previous_held = 0;
    selected_building = -1;
    u[0].selected = u[1].selected = 0;
    begin_drag();
    drag_to(80, 80, 0);
    end_drag();
    if (selection_mask() != expected) return 2;

    /* A box containing no friendly actor clears the old unit selection. */
    cx = 80; cy = 80;
    previous_held = 0;
    begin_drag();
    drag_to(90, 90, 0);
    end_drag();
    if (selected_units() != 0 || selected_building != -1) return 3;
    return 0;
}

static int
drag_suppression_and_cancel_tests(void)
{
    fixture();
    put_unit(0, SIDE, U_MARINE, 100, 100, 1, 1);
    put_unit(1, SIDE, U_SCV, 120, 120, 0, 1);
    begin_drag();
    input(KEY_L | KEY_RIGHT | KEY_DOWN | KEY_A | KEY_R);
    if (selection_mask() != 1 || u[0].order != ORDER_IDLE || mode != MODE_NORMAL || menu_open)
        return 1;
    drag_to(120, 120, KEY_R);
    if (selection_mask() != 1)
        return 2;
    end_drag();
    if (selection_mask() != 3)
        return 3;

    fixture();
    put_unit(0, SIDE, U_MARINE, 100, 100, 1, 1);
    put_unit(1, SIDE, U_SCV, 120, 120, 0, 1);
    mode = MODE_MOVE;
    begin_drag();
    input(KEY_L | KEY_RIGHT | KEY_DOWN | KEY_A);
    drag_to(120, 120, KEY_A);
    input(KEY_A); /* release L while A remains held */
    if (selection_mask() != 3 || mode != MODE_NORMAL || u[0].order != ORDER_IDLE || u[1].order != ORDER_IDLE)
        return 4;
    input(0);
    if (selection_mask() != 3 || u[0].order != ORDER_IDLE || u[1].order != ORDER_IDLE)
        return 5;

    fixture();
    put_unit(0, SIDE, U_MARINE, 100, 100, 1, 1);
    put_unit(1, SIDE, U_SCV, 120, 120, 0, 1);
    mode = MODE_ATTACK;
    begin_drag();
    input(KEY_L | KEY_RIGHT | KEY_DOWN);
    input(KEY_L | KEY_B);
    input(0);
    if (selection_mask() != 1 || menu_open || selected_building != -1 || mode != MODE_ATTACK)
        return 6;
    return 0;
}

static int
tap_tests(void)
{
    fixture();
    put_unit(0, SIDE, U_MARINE, 80, 80, 1, 1);
    put_unit(1, SIDE, U_SCV, 120, 80, 0, 1);
    input(KEY_L);
    if (selection_mask() != 1) return 1; /* cycle is deferred until release */
    input(0);
    if (selection_mask() != 2) return 2;

    fixture();
    put_unit(0, SIDE, U_MARINE, 80, 80, 1, 1);
    b[0].owner = SIDE;
    b[0].type = B_BARRACKS;
    b[0].alive = 1;
    b[0].queue_count = 2;
    selected_building = 0;
    input(KEY_L);
    if (b[0].queue_count != 2) return 3;
    input(0);
    if (b[0].queue_count != 1 || selected_building != 0) return 4;

    fixture();
    put_unit(0, SIDE, U_MARINE, 80, 80, 1, 1);
    put_unit(1, SIDE, U_SCV, 120, 80, 0, 1);
    put_unit(2, SIDE, U_FIREBAT, 140, 80, 0, 1);
    tap(KEY_R);
    if (selection_mask() != (((u32)1 << 0) | ((u32)1 << 2))) return 5;

    fixture();
    b[0].owner = SIDE;
    b[0].type = B_BARRACKS;
    b[0].alive = 1;
    selected_building = 0;
    train_type = U_MARINE;
    tap(KEY_R);
    if (train_type != U_FIREBAT || selected_building != 0) return 6;
    return 0;
}

static int
double_tap_selection_tests(void)
{
    u32 viewport_group = ((u32)1 << 0) | ((u32)1 << 1) | ((u32)1 << 2);
    fixture();
    cx = 200; cy = 180;
    put_unit(0, SIDE, U_MARINE, 200, 180, 0, 1);
    put_unit(1, SIDE, U_MARINE, 80, 126, 0, 1);  /* inclusive viewport top-left */
    put_unit(2, SIDE, U_MARINE, 319, 233, 0, 1); /* last visible pixel */
    put_unit(3, SIDE, U_MARINE, 320, 180, 0, 1); /* right edge is exclusive */
    put_unit(4, SIDE, U_MARINE, 100, 234, 0, 1); /* bottom edge is exclusive */
    put_unit(5, SIDE, U_MARINE, 79, 180, 0, 1);  /* left of viewport */
    put_unit(6, SIDE, U_FIREBAT, 210, 190, 0, 1);
    put_unit(7, ENEMY, U_MARINE, 210, 190, 0, 1);
    put_unit(8, NEUTRAL, U_MARINE, 210, 190, 0, 1);
    put_unit(9, SIDE, U_MARINE, 210, 190, 0, 0);
    tap(KEY_A);
    if (selection_mask() != 1) return 1; /* first click selects immediately */
    tap(KEY_A);
    if (selection_mask() != viewport_group) return 2;
    tap(KEY_A);
    if (selection_mask() != viewport_group) return 3;

    /* Exactly twelve field-update ticks remains a double tap; thirteen does
     * not. Both taps target the same friendly unit. */
    fixture();
    cx = 200; cy = 180;
    put_unit(0, SIDE, U_MARINE, 200, 180, 0, 1);
    put_unit(1, SIDE, U_MARINE, 220, 180, 0, 1);
    tap(KEY_A);
    { int i; for (i = 0; i < 10; i++) input(0); }
    tap(KEY_A);
    if (selection_mask() != 3) return 4;

    fixture();
    cx = 200; cy = 180;
    put_unit(0, SIDE, U_MARINE, 200, 180, 0, 1);
    put_unit(1, SIDE, U_MARINE, 220, 180, 0, 1);
    tap(KEY_A);
    { int i; for (i = 0; i < 11; i++) input(0); }
    tap(KEY_A);
    if (selection_mask() != 1) return 5;

    /* Cursor drift of exactly eight pixels is allowed; nine pixels cancels
     * the pending candidate. Movement is through controller inputs. */
    fixture();
    cx = 200; cy = 180;
    put_unit(0, SIDE, U_MARINE, 200, 180, 0, 1);
    put_unit(1, SIDE, U_MARINE, 220, 180, 0, 1);
    tap(KEY_A);
    { int i; for (i = 0; i < 4; i++) input(KEY_RIGHT); }
    input(0);
    tap(KEY_A);
    if (selection_mask() != 3) return 6;

    fixture();
    cx = 200; cy = 180;
    put_unit(0, SIDE, U_MARINE, 200, 180, 0, 1);
    put_unit(1, SIDE, U_MARINE, 220, 180, 0, 1);
    tap(KEY_A);
    { int i; for (i = 0; i < 5; i++) input(KEY_RIGHT); }
    input(0);
    tap(KEY_A);
    if (selection_mask() != 1) return 7;

    /* Near a completed Refinery, the second tap is still a unit double-tap;
     * a later click directly on the Refinery assigns gas as usual. */
    fixture();
    cx = 100; cy = 100;
    put_unit(0, SIDE, U_SCV, 100, 100, 0, 1);
    put_unit(1, SIDE, U_SCV, 140, 100, 0, 1);
    b[0].owner = SIDE;
    b[0].type = B_REFINERY;
    b[0].x = 120; b[0].y = 100;
    b[0].hp = b[0].maxhp = 100;
    b[0].alive = 1;
    tap(KEY_A);
    tap(KEY_A);
    if (selection_mask() != 3 || u[0].order == ORDER_GAS || u[1].order == ORDER_GAS) return 8;
    { int i; for (i = 0; i < 10; i++) input(KEY_RIGHT); }
    input(0);
    tap(KEY_A);
    if (u[0].order != ORDER_GAS || u[0].target != 0) return 9;

    /* A replacement actor in the same slot cannot inherit a pending tap. */
    fixture();
    cx = 100; cy = 100;
    put_unit(0, SIDE, U_MARINE, 100, 100, 0, 1);
    put_unit(1, SIDE, U_MARINE, 140, 100, 0, 1);
    tap(KEY_A);
    u[0].alive = 0;
    if (spawn_unit(SIDE, U_MARINE, 100, 100) != 0) return 10;
    tap(KEY_A);
    if (selection_mask() != 1) return 11;

    /* Even a canceled L gesture consumes the pending double-click. A held A
     * through that gesture remains suppressed and cannot issue an order. */
    fixture();
    cx = 100; cy = 100;
    put_unit(0, SIDE, U_MARINE, 100, 100, 0, 1);
    put_unit(1, SIDE, U_MARINE, 120, 100, 0, 1);
    tap(KEY_A);
    input(KEY_L | KEY_RIGHT | KEY_DOWN | KEY_A);
    input(KEY_L | KEY_B | KEY_A);
    input(KEY_A); /* release L while A remains held */
    input(0);
    tap(KEY_A);
    if (selection_mask() != 1 || u[0].order != ORDER_IDLE || u[1].selected) return 7;

    /* Outside a box, R remains the army shortcut and resets the click pair. */
    fixture();
    cx = 100; cy = 100;
    put_unit(0, SIDE, U_MARINE, 100, 100, 0, 1);
    put_unit(1, SIDE, U_MARINE, 130, 100, 0, 1);
    tap(KEY_A);
    tap(KEY_R);
    tap(KEY_A);
    if (selection_mask() != 1) return 12;

    /* A command menu breaks the click pair; its A still executes Move. */
    fixture();
    cx = 100; cy = 100;
    put_unit(0, SIDE, U_MARINE, 100, 100, 0, 1);
    put_unit(1, SIDE, U_MARINE, 130, 100, 0, 1);
    tap(KEY_A);
    tap(KEY_B);
    tap(KEY_A);
    if (mode != MODE_MOVE || selection_mask() != 1) return 13;
    { int i; for (i = 0; i < 5; i++) input(KEY_RIGHT); }
    input(0);
    tap(KEY_A);
    if (selection_mask() != 1 || u[0].order != ORDER_MOVE || u[1].selected) return 14;

    /* A building's A actions retain selection/queue semantics and never
     * become a same-type unit selection. */
    fixture();
    cx = 100; cy = 100;
    b[0].owner = SIDE;
    b[0].type = B_BARRACKS;
    b[0].x = cx; b[0].y = cy;
    b[0].hp = b[0].maxhp = 100;
    b[0].alive = 1;
    tap(KEY_A);
    if (selected_building != 0 || b[0].queue_count != 0) return 15;
    tap(KEY_A);
    if (selected_building != 0 || b[0].queue_count != 1 || selected_units()) return 16;
    return 0;
}

static int
cancel_lifecycle_tests(void)
{
    u32 saved_selection;
    fixture();
    put_unit(0, SIDE, U_MARINE, 100, 100, 1, 1);
    put_unit(1, SIDE, U_SCV, 120, 120, 0, 1);
    save_game(); /* fixture snapshot; the tested load is a real controller input */
    begin_drag();
    input(KEY_L | KEY_RIGHT | KEY_DOWN);
    input(KEY_L | KEY_START);
    input(0);
    tap(KEY_A);
    if (selection_mask() != 1) return 1; /* pause/resume canceled the transient box */

    /* A fresh drag followed by pause+load restores the saved selection and
     * cannot leave a stale drag active after load. */
    begin_drag();
    input(KEY_L | KEY_RIGHT | KEY_DOWN);
    input(KEY_L | KEY_START);
    input(0);
    input(KEY_L);
    if (selection_mask() != 1 || screen != SCREEN_PLAY) return 2;
    input(0);
    if (selection_mask() != 1) return 3;

    /* Restart via the pause menu also clears the unfinished gesture. */
    fixture();
    put_unit(0, SIDE, U_MARINE, 100, 100, 1, 1);
    begin_drag();
    input(KEY_L | KEY_RIGHT);
    input(KEY_L | KEY_START);
    input(0);
    input(KEY_R);
    if (screen != SCREEN_PLAY) return 4;
    input(0);
    saved_selection = selection_mask();
    if (saved_selection != 0) return 5;
    return 0;
}

static int
camera_anchor_test(void)
{
    int i;
    fixture();
    cx = 200; cy = 180;
    put_unit(0, SIDE, U_MARINE, 210, 190, 0, 1);
    put_unit(1, SIDE, U_SCV, 226, 206, 0, 1);
    input(KEY_L);
    for (i = 0; i < 18; i++) {
        input(KEY_L | KEY_R | KEY_RIGHT | KEY_DOWN);
        if (i == 7) {
            game_render();
            if (camx == 0 || camy == 0 || box_state != 2) return 1;
#ifdef SC64_IMPORTED_MISSION
            if (sc64_hud_previous[18] != 1) return 3;
#endif
        }
    }
    {
        int old_camx = camx, old_camy = camy;
        game_render();
        if (camx == old_camx && camy == old_camy) return 4;
    }
    end_drag();
    game_render(); /* update camera exactly as the display loop does */
#ifdef SC64_IMPORTED_MISSION
    if (sc64_hud_previous[18] != 0) return 7;
#endif
    if (camx == 0 || camy == 0) return 5;
    if (selection_mask() != 3) return 6;
    return 0;
}

int
main(void)
{
    int result;
    game_init();
    result = box_tests();
    if (result) return fail(result, "normalized box bounds, filtering, or empty box");
    result = drag_suppression_and_cancel_tests();
    if (result) return fail(10 + result, "A/R suppression or B cancellation");
    result = tap_tests();
    if (result) return fail(20 + result, "tap-L cycle or Barracks queue cancel");
    result = double_tap_selection_tests();
    if (result) return fail(25 + result, "double-tap selection timing, viewport, or input reset");
    result = cancel_lifecycle_tests();
    if (result) return fail(30 + result, "pause, load, or restart cancellation");
    result = camera_anchor_test();
    if (result) return fail(40 + result, "camera-scroll world anchors");
    printf("PASS selection gestures: boxes, tap-L, cancellation, camera anchors\n");
    return 0;
}
