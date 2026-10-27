/* Default-profile menus and controls (starcraft/field_ui.h). Every gesture is
 * delivered through game_update() key edges; each UI state is also rendered
 * under the host sanitizers. */
#include <stdio.h>
#include "../starcraft/game.c"

static u16 previous_held;
static int failures;

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

static void
check(int ok, const char *what)
{
    if (!ok) {
        fprintf(stderr, "FAIL field ui: %s\n", what);
        failures++;
    }
}

static int
count_color(int x, int y, int w, int h, u8 color)
{
    int n = 0;
    for (int py = y; py < y + h; py++)
        for (int px = x; px < x + w; px++)
            n += gfx_test_pixel(px, py) == color;
    return n;
}

/* The cursor and HUD read a per-frame snapshot; take a fresh one. */
static int
a_action(void)
{
    field_ui_refresh();
    return ui_action;
}

static void
render(void)
{
    game_render();
}

/* Mission start, with every friendly actor parked and the camera fixed. */
static void
fixture(void)
{
    previous_held = 0;
    init_state();
    for (int i = 0; i < MAX_UNITS; i++)
        if (u[i].alive) { u[i].order = ORDER_IDLE; u[i].target = -1; }
    minerals = 500;
    gas = 0;
    input(0);
    previous_held = 0;
}

static void
move_cursor(int x, int y)
{
    cx = x; cy = y;
    input(0);
}

static int
slot_of(int owner, int type)
{
    for (int i = 0; i < MAX_UNITS; i++)
        if (u[i].alive && u[i].owner == owner && u[i].type == type) return i;
    return -1;
}

static int
building_of(int owner, int type)
{
    for (int i = 0; i < MAX_BUILDINGS; i++)
        if (b[i].alive && b[i].owner == owner && b[i].type == type) return i;
    return -1;
}

static void
deselect_tests(void)
{
    int barracks;
    fixture();
    barracks = building_of(SIDE, B_BARRACKS);
    move_cursor(b[barracks].x, b[barracks].y);
    check(a_action() == UI_A_SELECT, "A on an unselected Barracks selects it");
    tap(KEY_A);
    check(selected_building == barracks, "Barracks selected");
    tap(KEY_A);
    check(b[barracks].queue_count == 1, "A on the selected Barracks trains");
    render();
    move_cursor(200, 100);
    check(a_action() == UI_A_DESELECT, "hint offers deselect off the building");
    tap(KEY_A);
    check(selected_building == -1, "A on empty ground clears the building");
    check(b[barracks].queue_count == 1, "deselecting does not train");

    /* A on a unit while a building is selected selects that unit. */
    fixture();
    barracks = building_of(SIDE, B_BARRACKS);
    move_cursor(b[barracks].x, b[barracks].y);
    tap(KEY_A);
    {
        int scv = slot_of(SIDE, U_SCV);
        move_cursor(u[scv].x, u[scv].y);
        tap(KEY_A);
        check(selected_building == -1 && u[scv].selected, "A on a unit replaces the building selection");
        check(b[barracks].queue_count == 0, "reselecting does not train");
    }
}

static void
smart_attack_tests(void)
{
    int marine, ling;
    fixture();
    marine = slot_of(SIDE, U_MARINE);
    move_cursor(u[marine].x, u[marine].y);
    tap(KEY_A);
    check(u[marine].selected, "marine selected");
    ling = spawn_unit(ENEMY, U_ZERGLING, u[marine].x + 40, u[marine].y);
    u[ling].order = ORDER_IDLE;
    update_exploration();
    move_cursor(u[ling].x + 3, u[ling].y + 2);
    check(a_action() == UI_A_ATTACK, "cursor over a visible enemy offers attack");
    render();
    check(count_color(0, 148, 120, 12, C_RED) > 0, "attack hint is drawn in red");
    tap(KEY_A);
    check(u[marine].order == ORDER_ATTACK_MOVE && u[marine].selected, "A on an enemy attack-moves the selection");
    check(dist2(u[marine].tx, u[marine].ty, u[ling].x, u[ling].y) < 4 * 4, "attack targets the enemy");
    check(ping_life && ping_color == C_RED, "attack leaves a red ping");

    /* Without a selection there is nothing to send, so no attack hint. */
    fixture();
    ling = spawn_unit(ENEMY, U_ZERGLING, 160, 90);
    update_exploration();
    move_cursor(u[ling].x, u[ling].y);
    check(a_action() != UI_A_ATTACK, "no attack hint without a selection");
}

static void
r_gesture_tests(void)
{
    int scv;
    fixture();
    scv = slot_of(SIDE, U_SCV);
    move_cursor(u[scv].x, u[scv].y);
    tap(KEY_A);
    check(u[scv].selected, "SCV selected");
    input(KEY_R);
    check(u[scv].selected, "R does not act on press");
    for (int i = 0; i < 6; i++) input(KEY_R | KEY_RIGHT);
    input(0);
    check(u[scv].selected && selected_count() == 1, "R held for a fast cursor keeps the selection");
    tap(KEY_R);
    check(!u[scv].selected && selected_count() == 8, "a clean R tap selects the army");
}

static void
menu_memory_tests(void)
{
    int scv, before, slot;
    fixture();
    scv = slot_of(SIDE, U_SCV);
    move_cursor(u[scv].x, u[scv].y);
    tap(KEY_A);
    tap(KEY_B);
    check(menu_open && menu_cursor == MENU_MOVE, "first list opens on Move");
    render();
    tap(KEY_DOWN); tap(KEY_DOWN); tap(KEY_DOWN);
    tap(KEY_A);
    check(mode == MODE_BUILD && menu == B_DEPOT, "Supply Depot placement armed");
    move_cursor(41, 27);
    render();
    check(ui_site == UI_SITE_OK && ui_site_x == 40 && ui_site_y == 24, "site snaps to the 8-pixel grid");
    check(count_color(0, 121, 178, 26, C_GREEN) > 0, "HUD reports a ready site");
    before = minerals;
    tap(KEY_A);
    slot = -1;
    for (int i = 0; i < MAX_BUILDINGS; i++)
        if (b[i].alive && b[i].type == B_DEPOT && b[i].under_construction) slot = i;
    check(slot >= 0 && b[slot].x == 40 && b[slot].y == 24, "depot placed on the snapped site");
    check(minerals == before - 100 && u[scv].tx == 40 && u[scv].ty == 24, "worker walks to the snapped site");
    check(ping_life && ping_color == C_GOLD, "placement leaves a gold ping");
    tap(KEY_B);
    check(menu_open && menu_cursor == MENU_DEPOT, "B reopens the list on the last command");
    tap(KEY_A);
    check(mode == MODE_BUILD && menu == B_DEPOT, "B, A repeats the build");
    move_cursor(b[slot].x, b[slot].y);
    render();
    check(ui_site == UI_SITE_BLOCKED, "occupied site is reported blocked");
    tap(KEY_B);
    check(mode == MODE_NORMAL && menu_open, "B backs out of placement to the list");
    tap(KEY_B);
    check(!menu_open, "B closes the list");

    /* Move/attack modes are named in the HUD and shown by the cursor. */
    fixture();
    tap(KEY_R);
    tap(KEY_B);
    tap(KEY_DOWN);
    tap(KEY_A);
    check(mode == MODE_ATTACK, "attack-move armed");
    render();
    check(count_color(28, 121, 150, 24, C_RED) > 20, "HUD names attack-move in red");
    tap(KEY_A);
    check(mode == MODE_NORMAL && ping_life && ping_color == C_RED, "attack issued with a red ping");
    tap(KEY_B);
    check(menu_cursor == MENU_ATTACK, "list remembers attack-move");
    tap(KEY_UP);
    tap(KEY_A);
    render();
    check(mode == MODE_MOVE && count_color(28, 121, 150, 24, C_GREEN) > 20, "HUD names move in green");
}

static void
refinery_snap_tests(void)
{
    int sx = geysers[0].x + 6, sy = geysers[0].y - 5;
    field_ui_site(B_REFINERY, &sx, &sy);
    check(sx == geysers[0].x && sy == geysers[0].y, "Refinery snaps to the geyser center");
}

static void
title_and_pause_tests(void)
{
    init_state();
    screen = SCREEN_TITLE;
    title_cursor = 0;
    previous_held = 0;
    render();
    tap(KEY_DOWN);
    check(title_cursor == 1, "title cursor moves to Load");
    tap(KEY_A);
    check(screen == SCREEN_TITLE && title_notice, "loading without a save stays on the title with a notice");
    render();
    check(count_color(16, 120, 208, 12, C_RED) > 0, "title shows NO SAVE FOUND");
    tap(KEY_UP);
    tap(KEY_A);
    check(screen == SCREEN_PLAY, "New Mission starts");

    tap(KEY_START);
    check(screen == SCREEN_PAUSE && pause_cursor == 0, "START pauses on Resume");
    render();
    tap(KEY_DOWN);
    tap(KEY_A);
    check(screen == SCREEN_PAUSE && pause_notice == 1, "Save keeps the pause open and confirms");
    render();
    check(count_color(126, 60, 90, 16, C_GREEN) > 0, "pause shows GAME SAVED");
    tap(KEY_START);
    check(screen == SCREEN_PLAY, "START resumes");
    tap(KEY_START);
    tap(KEY_L);
    check(screen == SCREEN_PLAY, "L loads the save from the pause screen");
    tap(KEY_START);
    tap(KEY_UP);
    check(pause_cursor == 3, "pause cursor wraps to Restart");
    tap(KEY_A);
    check(screen == SCREEN_PLAY && frame <= 1, "Restart starts a new mission");

    screen = SCREEN_RESULT;
    won = 1;
    result_outro_end = 1;
    result_outro = 200;
    render();
    check(count_color(28, 30, 184, 16, C_GOLD_DARK) > 100, "victory panel header");
    won = 0; lost = 1;
    render();
    check(count_color(28, 30, 184, 16, C_RED_DARK) > 100, "defeat panel header");
}

/* start_build()'s toast for each placement status, so the HUD can be checked
 * against what A actually reports. */
static int
site_toast(int status, int type)
{
    switch (status) {
    case UI_SITE_TECH: return type == B_ACADEMY ? 10 : 8;
    case UI_SITE_MINERALS: return 1;
    case UI_SITE_GAS: return 12;
    case UI_SITE_BLOCKED: return 6;
    case UI_SITE_WORKER: return 5;
    case UI_SITE_FULL: return 11;
    default: return 3;
    }
}

static void
arm_site(int type, int x, int y, int with_worker, int money, int keep_depot)
{
    int depot;
    fixture();
    depot = building_of(SIDE, B_DEPOT);
    if (!keep_depot) b[depot].alive = 0;
    clear_selection();
    if (with_worker) u[slot_of(SIDE, U_SCV)].selected = 1;
    minerals = money;
    mode = MODE_BUILD;
    menu = type;
    cx = x; cy = y;
    toast_timer = 0; toast_id = -1;
}

/* Several requirements fail at once: the HUD names start_build()'s first
 * failure, and the footprint stays red whenever the ground is blocked. */
static void
simultaneous_failure_tests(void)
{
    static const struct {
        int type, x, y, worker, money, depot, status, blocked;
        const char *what;
    } cases[] = {
        {B_BARRACKS, 40, 24, 0, 500, 0, UI_SITE_TECH, 0, "no SCV + no Depot: prerequisite first"},
        {B_BARRACKS, 120, 48, 0, 0, 0, UI_SITE_TECH, 1, "everything missing on a blocked site: prerequisite first"},
        {B_DEPOT, 40, 24, 0, 50, 1, UI_SITE_MINERALS, 0, "no SCV + short minerals: minerals first"},
        {B_DEPOT, 120, 48, 1, 50, 1, UI_SITE_MINERALS, 1, "blocked + short minerals: minerals first, red site"},
        {B_DEPOT, 120, 48, 0, 500, 1, UI_SITE_BLOCKED, 1, "blocked + no SCV: blocked first"},
        {B_DEPOT, 40, 24, 0, 500, 1, UI_SITE_WORKER, 0, "clear site, only the SCV missing"},
        {B_DEPOT, 40, 24, 1, 500, 1, UI_SITE_OK, 0, "everything present"},
    };
    for (unsigned i = 0; i < sizeof cases / sizeof cases[0]; i++) {
        int before;
        arm_site(cases[i].type, cases[i].x, cases[i].y, cases[i].worker, cases[i].money, cases[i].depot);
        render();
        check(ui_site == cases[i].status, cases[i].what);
        check(ui_site_blocked == cases[i].blocked, cases[i].what);
        {
            int sx = ui_site_x - camx, sy = ui_site_y - camy + 12;
            int red = count_color(sx - 10, sy - 10, 21, 20, C_RED);
            int green = count_color(sx - 10, sy - 10, 21, 20, C_GREEN);
            check(cases[i].blocked ? red > 20 && !green :
                  cases[i].status == UI_SITE_OK ? green > 20 && !red : !green && !red,
                  "footprint colour follows the site policy");
        }
        before = minerals;
        start_build(cases[i].type);
        check(toast_id == site_toast(cases[i].status, cases[i].type), "HUD status matches start_build");
        check((cases[i].status == UI_SITE_OK) == (minerals != before), "only a ready site spends");
    }

    /* The command list uses the same order, minus the placement checks. */
    arm_site(B_BARRACKS, 40, 24, 0, 0, 0);
    mode = MODE_NORMAL;
    field_ui_refresh();
    {
        int cost;
        check(ui_command_state(MENU_BARRACKS, &cost) == UI_SITE_TECH, "list: prerequisite before minerals and SCV");
        check(ui_command_state(MENU_DEPOT, &cost) == UI_SITE_MINERALS, "list: minerals before SCV");
        minerals = 500;
        check(ui_command_state(MENU_DEPOT, &cost) == UI_SITE_WORKER, "list: SCV last");
    }
}

static void
render_smoke(void)
{
    /* Box drag, a selected group and a building under construction. */
    fixture();
    input(KEY_L);
    for (int i = 0; i < 20; i++) input(KEY_L | KEY_RIGHT | KEY_DOWN);
    render();
    input(0);
    render();
    for (int i = 0; i < MAX_BUILDINGS; i++) if (b[i].alive && b[i].owner == SIDE) {
        selected_building = i;
        b[i].under_construction = b[i].type == B_DEPOT;
        render();
    }
    selected_building = -1;
    for (int i = 0; i < MENU_COUNT; i++) {
        menu_open = 1;
        menu_cursor = i;
        render();
    }
}

int
main(void)
{
    gba_init();
    game_init();
    deselect_tests();
    smart_attack_tests();
    r_gesture_tests();
#ifndef SC_COMMAND_RING /* the ring build opens its list on Move */
    menu_memory_tests();
#endif
    simultaneous_failure_tests();
    refinery_snap_tests();
    title_and_pause_tests();
    render_smoke();
    if (failures) return 1;
#ifdef SC_COMMAND_RING
    puts("Default menus and controls (ring build): deselect, smart attack, R tap, placement status order, snapping, title/pause/result: ok");
#else
    puts("Default menus and controls: deselect, smart attack, R tap, list memory, placement status order, snapping, title/pause/result: ok");
#endif
    return 0;
}
