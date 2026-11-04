/* Default-profile menus and controls (starcraft/field_ui.h). Every gesture is
 * delivered through game_update() key edges; each UI state is also rendered
 * under the host sanitizers. */
#include <stdio.h>
#include <string.h>
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
unfinished_producer_tests(void)
{
    const int producers[] = {B_COMMAND, B_BARRACKS};
    for (unsigned n = 0; n < sizeof producers / sizeof producers[0]; ++n) {
        fixture();
        int slot = building_of(SIDE, producers[n]);
        check(slot >= 0, "producer exists in fixture");
        if (slot < 0) continue;
        selected_building = slot;
        b[slot].under_construction = 1;
        move_cursor(b[slot].x, b[slot].y);
        int before = minerals;
        check(a_action() == UI_A_NONE, "unfinished producer does not offer TRAIN");
        tap(KEY_A);
        check(b[slot].queue_count == 0 && minerals == before,
              "unfinished producer cannot queue or spend minerals");
        check(toast_id == 9, "unfinished producer reports building unfinished");
        b[slot].under_construction = 0;
        check(a_action() == UI_A_TRAIN, "completed producer offers TRAIN again");
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

static int
free_building_slot(void)
{
    for (int i = 0; i < MAX_BUILDINGS; i++)
        if (!b[i].alive) return i;
    return -1;
}

/* A completed friendly structure beside the opening base. */
static int
add_building(int type, int x, int y)
{
    int slot = free_building_slot();
    memset(&b[slot], 0, sizeof b[slot]);
    b[slot].alive = 1;
    b[slot].owner = SIDE;
    b[slot].type = type;
    b[slot].x = x;
    b[slot].y = y;
    b[slot].hp = b[slot].maxhp = 500;
    return slot;
}

static void
select_building(int slot)
{
    move_cursor(b[slot].x, b[slot].y);
    if (selected_building != slot) tap(KEY_A);
    check(selected_building == slot, "structure selected");
}

/* The selected structure's five slots: x, top, size and progress bar row. */
enum { QUEUE_X = 30, QUEUE_Y = 134, QUEUE_PITCH = 12, QUEUE_SLOT = 11, QUEUE_BAR_Y = 145, QUEUE_BAR_W = 59 };

static int
slot_filled(int i)
{
    return count_color(QUEUE_X + i * QUEUE_PITCH + 1, QUEUE_Y + 1, QUEUE_SLOT - 2, QUEUE_SLOT - 2, C_FOG) > 0;
}

static int
slot_empty(int i)
{
    return count_color(QUEUE_X + i * QUEUE_PITCH, QUEUE_Y, QUEUE_SLOT, QUEUE_SLOT, C_SHADOW) == QUEUE_SLOT * QUEUE_SLOT;
}

static int
slot_framed(int i)
{
    return gfx_test_pixel(QUEUE_X + i * QUEUE_PITCH, QUEUE_Y) == C_GOLD;
}

static int
queue_bar_fill(void)
{
    return count_color(QUEUE_X, QUEUE_BAR_Y, QUEUE_BAR_W, 2, C_GOLD) / 2;
}

/* Words are gone from the hint row: only key caps and marks remain, so its
 * text colours stay off the row. The caps' own shadow sits below y 158. */
static void
check_hint_row_has_no_words(const char *what)
{
    check(count_color(0, 150, 178, 8, C_DIM) == 0, what);
    check(count_color(0, 150, 178, 8, C_WHITE) == 0, what);
    check(count_color(0, 149, 12, 9, C_SILVER) > 0, "the hint row starts with a key cap");
}

static void
queue_panel_tests(void)
{
    int barracks, command, time;
    fixture();
    add_building(B_ACADEMY, 120, 40);
    minerals = 1000;
    gas = 200;
    supply_cap = 200;
    barracks = building_of(SIDE, B_BARRACKS);
    select_building(barracks);
    render();
    for (int i = 0; i < 5; i++) check(slot_empty(i), "an idle Barracks shows five empty slots");
    check(!slot_framed(0) && queue_bar_fill() == 0, "an idle Barracks has no active slot or progress");
    check_hint_row_has_no_words("training hint is marks, not words");

    /* A trains the next unit into the first slot, framed as active. */
    tap(KEY_A);
    check(b[barracks].queue_count == 1, "A queues a Marine");
    render();
    check(slot_filled(0) && slot_framed(0), "the unit in production fills the framed first slot");
    check(slot_empty(1), "the rest of the queue stays empty");

    /* R, then A: the queued icons keep production order. */
    tap(KEY_R);
    check(train_type == U_FIREBAT, "R reaches the Firebat");
    tap(KEY_A);
    render();
    check(slot_filled(1) && !slot_framed(1), "a waiting unit fills the next slot unframed");
    check(count_color(QUEUE_X, QUEUE_Y, QUEUE_SLOT, QUEUE_SLOT, C_ORANGE) == 0,
          "the first slot still shows the Marine");
    check(count_color(QUEUE_X + QUEUE_PITCH, QUEUE_Y, QUEUE_SLOT, QUEUE_SLOT, C_ORANGE) > 0,
          "the second slot shows the Firebat");

    /* The bar follows the active unit's progress. */
    time = unit_build_time(b[barracks].queue[b[barracks].queue_head]);
    for (int quarter = 0; quarter <= 3; quarter++) {
        b[barracks].queue_progress = time * quarter / 4;
        render();
        check(queue_bar_fill() == QUEUE_BAR_W * (time * quarter / 4) / time, "the bar tracks production");
    }

    /* Finishing the Marine advances the queue, and the Firebat takes the frame. */
    b[barracks].queue_progress = time - 1;
    input(0);
    check(b[barracks].queue_count == 1, "the Marine finished");
    render();
    check(count_color(QUEUE_X, QUEUE_Y, QUEUE_SLOT, QUEUE_SLOT, C_ORANGE) > 0 && slot_framed(0),
          "the Firebat moves to the active slot");
    check(slot_empty(1) && queue_bar_fill() == 0, "the queue shortens and progress restarts");

    /* A full queue fills all five slots; L removes the oldest. */
    train_type = U_MARINE;
    for (int i = 0; i < 6; i++) tap(KEY_A);
    check(b[barracks].queue_count == 5, "the queue holds five");
    render();
    for (int i = 0; i < 5; i++) check(slot_filled(i), "all five slots are filled");
    tap(KEY_L);
    render();
    check(b[barracks].queue_count == 4 && slot_empty(4) && slot_filled(3), "L cancels into a shorter queue");
    check(count_color(QUEUE_X, QUEUE_Y, QUEUE_SLOT, QUEUE_SLOT, C_ORANGE) == 0, "L removes the oldest unit");

    /* A Command Center's queue is drawn the same way. */
    fixture();
    command = building_of(SIDE, B_COMMAND);
    select_building(command);
    tap(KEY_A);
    tap(KEY_A);
    render();
    check(b[command].queue_count == 2 && slot_filled(0) && slot_filled(1) && slot_empty(2),
          "a Command Center shows its queue");
    check(slot_empty(4), "it keeps five slots");
}

static void
research_and_construction_panel_tests(void)
{
    int bay, site;
    fixture();
    bay = add_building(B_ENG_BAY, 120, 40);
    select_building(bay);
    render();
    check(!slot_framed(0) && count_color(QUEUE_X, QUEUE_BAR_Y, 48, 2, C_GOLD) == 0,
          "an idle Engineering Bay has no active research");
    weapons_research = 300;
    render();
    check(slot_framed(0), "weapons research frames its slot");
    check(count_color(QUEUE_X, QUEUE_BAR_Y, QUEUE_SLOT, 2, C_GOLD) == 2 * (QUEUE_SLOT * 300 / 600),
          "weapons research fills its bar");
    check(gfx_test_pixel(QUEUE_X + 24, QUEUE_Y) != C_GOLD, "armor stays idle");
    armor_research = 150;
    render();
    check(gfx_test_pixel(QUEUE_X + 24, QUEUE_Y) == C_GOLD, "armor research frames its own slot");
    check(count_color(QUEUE_X + 24, QUEUE_BAR_Y, QUEUE_SLOT, 2, C_GOLD) == 2 * (QUEUE_SLOT * 150 / 600),
          "armor research fills its own bar");

    /* A structure going up shows its progress and no queue. */
    fixture();
    site = add_building(B_BARRACKS, 120, 40);
    b[site].under_construction = 1;
    b[site].build_progress = building_build_time(B_BARRACKS) / 2;
    select_building(site);
    render();
    check(count_color(QUEUE_X, QUEUE_BAR_Y, 148, 2, C_GOLD) == 2 * 148 / 2, "construction fills the long bar");
    check(count_color(QUEUE_X, QUEUE_Y, 5 * QUEUE_PITCH, QUEUE_SLOT, C_SHADOW) == 0, "no queue while it builds");
}

static void
panel_marks_tests(void)
{
    int scv;
    fixture();
    render();
    check_hint_row_has_no_words("the empty field's hints are marks");
    scv = slot_of(SIDE, U_SCV);
    move_cursor(u[scv].x, u[scv].y);
    tap(KEY_A);
    render();
    check_hint_row_has_no_words("a unit's hints are marks");
    move_cursor(200, 100);
    render();
    check(count_color(0, 148, 40, 12, C_GREEN) > 0, "A's move shows the green order mark");
    selected_building = -1;
    tap(KEY_SELECT);
    tap(KEY_R);
    render();
    check(selected_count() == 8, "the army is selected");
    check_hint_row_has_no_words("a group's hints are marks");
    check(count_color(QUEUE_X, 125, 60, 8, C_GOLD) > 0, "a group shows its size as a number");
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
        {B_DEPOT, 40, 24, 0, 500, 1, UI_SITE_OK, 0, "clear site, no SCV selected: the nearest free SCV builds"},
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
    menu_open = 1; /* the list resolves the SCV that would build */
    field_ui_refresh();
    {
        int cost;
        check(ui_command_state(MENU_BARRACKS, &cost) == UI_SITE_TECH, "list: prerequisite before minerals and SCV");
        check(ui_command_state(MENU_DEPOT, &cost) == UI_SITE_MINERALS, "list: minerals before SCV");
        minerals = 500;
        check(ui_command_state(MENU_DEPOT, &cost) == UI_SITE_OK, "list: any free SCV can build");
        for (int i = 0; i < MAX_UNITS; i++)
            if (u[i].alive && u[i].owner == SIDE && u[i].type == U_SCV) u[i].order = ORDER_BUILD;
        field_ui_refresh();
        check(ui_command_state(MENU_DEPOT, &cost) == UI_SITE_WORKER, "list: SCV last");
    }

    /* With every SCV already building, placement reports the missing SCV. */
    arm_site(B_DEPOT, 40, 24, 0, 500, 1);
    for (int i = 0; i < MAX_UNITS; i++)
        if (u[i].alive && u[i].owner == SIDE && u[i].type == U_SCV) u[i].order = ORDER_BUILD;
    render();
    check(ui_site == UI_SITE_WORKER, "no free SCV: placement names it");
    {
        int before = minerals;
        start_build(B_DEPOT);
        check(toast_id == 5 && minerals == before, "no free SCV: nothing is spent");
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

/* A on open ground moves the selection; SELECT drops it, then pauses. */
static void
smart_move_tests(void)
{
    int scv;
    fixture();
    scv = slot_of(SIDE, U_SCV);
    move_cursor(u[scv].x, u[scv].y);
    tap(KEY_A);
    check(u[scv].selected, "smart move fixture selects an SCV");
    move_cursor(200, 100);
    check(selection_target() < 0 && a_action() == UI_A_MOVE, "A on open ground offers a move");
    tap(KEY_A);
    check(u[scv].selected && u[scv].order == ORDER_MOVE && ping_life && ping_color == C_GREEN,
          "A on open ground moves the selected units");
    render();
    tap(KEY_SELECT);
    check(!selected_count() && screen == SCREEN_PLAY, "SELECT drops the selection");
    check(a_action() == UI_A_NONE, "nothing selected: A on open ground does nothing");
    tap(KEY_SELECT);
    check(screen == SCREEN_PAUSE, "SELECT with nothing selected pauses");
    tap(KEY_A);
    check(screen == SCREEN_PLAY, "A resumes");
}

/* A far arc covered by art already on screen must match one drawn under it. */
static void
selection_cover_tests(void)
{
    enum { ART_W = 24, ART_H = 12 };
    static const int spots[][2] = {
        { 120, 60 }, { 0, 60 }, { 239, 60 }, { 120, 18 }, { 120, 116 },
    };
    static u8 art[ART_W * ART_H];
    static u8 under[240 * 160];
    for (int row = 0; row < ART_H; row++)
        for (int col = 0; col < ART_W; col++)
            art[row * ART_W + col] = (row % 5 == 4 || col % 7 == 6) ? 0 : C_STONE;
    for (unsigned i = 0; i < sizeof(spots) / sizeof(spots[0]); i++) {
        int x = spots[i][0], y = spots[i][1];
        SelectionCover cover = { art, x - ART_W / 2, y - ART_H / 2, ART_W, ART_H };
        gfx_begin(C_BLACK);
        draw_selection_ellipse(x, y, 16, 10, C_GREEN, SELECTION_FAR, 0);
        gfx_blit_sprite(cover.x, cover.y, ART_W, ART_H, art);
        for (int py = 0; py < 160; py++)
            for (int px = 0; px < 240; px++)
                under[py * 240 + px] = gfx_test_pixel(px, py);
        gfx_begin(C_BLACK);
        gfx_blit_sprite(cover.x, cover.y, ART_W, ART_H, art);
        draw_selection_ellipse(x, y, 16, 10, C_GREEN, SELECTION_FAR, &cover);
        int same = 1;
        for (int py = 0; py < 160; py++)
            for (int px = 0; px < 240; px++)
                same &= under[py * 240 + px] == gfx_test_pixel(px, py);
        check(same, "a covered far arc matches one drawn under the art");
    }
}

/* Selections are StarCraft ellipses at the actor's base, not boxes. */
static void
selection_circle_tests(void)
{
    check(selection_color(SIDE) == C_GREEN, "own selection is green");
    check(selection_color(NEUTRAL) == C_GOLD, "neutral selection is yellow");
    check(selection_color(ENEMY) == C_RED, "enemy selection is red");

    gfx_begin(C_BLACK);
    draw_selection_ellipse(120, 60, 10, 6, C_GREEN, SELECTION_RING, 0);
    check(gfx_test_pixel(110, 60) == C_GREEN && gfx_test_pixel(130, 60) == C_GREEN,
          "ellipse reaches its horizontal radius");
    check(gfx_test_pixel(120, 54) == C_GREEN && gfx_test_pixel(120, 66) == C_GREEN,
          "ellipse reaches its vertical radius");
    check(gfx_test_pixel(120, 60) == C_BLACK, "ellipse is an outline");
    for (int y = 54; y <= 66; y++) {
        check(count_color(110, y, 10, 1, C_GREEN) > 0 &&
              count_color(121, y, 10, 1, C_GREEN) > 0, "ellipse has no gaps");
        for (int x = 110; x <= 130; x++)
            check(gfx_test_pixel(x, y) == gfx_test_pixel(240 - x, y),
                  "ellipse is symmetric");
    }
    gfx_begin(C_BLACK);
    draw_selection_ellipse(120, 60, 10, 6, C_GREEN, SELECTION_NEAR, 0);
    check(count_color(100, 40, 41, 20, C_GREEN) == 0 &&
          gfx_test_pixel(120, 66) == C_GREEN, "near arc stays below centre");
    gfx_begin(C_BLACK);
    draw_selection_ellipse(120, 60, 10, 6, C_GREEN, SELECTION_FAR, 0);
    check(count_color(100, 61, 41, 20, C_GREEN) == 0 &&
          gfx_test_pixel(120, 54) == C_GREEN, "far arc stays above centre");
    selection_cover_tests();

    fixture();
    int marine = slot_of(SIDE, U_MARINE);
    if (marine < 0) marine = slot_of(SIDE, U_SCV);
    check(marine >= 0, "a friendly unit to select");
    if (marine < 0) return;
    Unit *q = &u[marine];
    camx = q->x - 120; camy = q->y - 50;
    /* Default art is 8x8: rx 5, ry 3, centred two rows below the unit. */
    for (int i = 0; i < MAX_UNITS; i++) u[i].selected = 0;
    selected_building = -1;
    render(); /* clamps the camera */
    int x = q->x - camx, y = q->y - camy + 12;
    check(gfx_test_pixel(x - 5, y + 2) != C_GREEN &&
          gfx_test_pixel(x + 5, y + 2) != C_GREEN, "unselected unit has no ellipse");
    q->selected = 1;
    render();
    check(gfx_test_pixel(x - 5, y + 2) == C_GREEN &&
          gfx_test_pixel(x + 5, y + 2) == C_GREEN, "selected unit has a green ellipse");
    check(count_color(x - 6, y - 6, 13, 1, C_HIGHLIGHT) == 0, "no selection box");

    q->selected = 0;
    int cc = building_of(SIDE, B_COMMAND);
    check(cc >= 0, "a friendly command center to select");
    if (cc < 0) return;
    Building *c = &b[cc];
    camx = c->x - 120; camy = c->y - 50;
    /* Default building art is 24x24: rx 14, ry 8, centred five rows down. */
    selected_building = cc;
    render();
    x = c->x - camx; y = c->y - camy + 12;
    check(gfx_test_pixel(x, y + 13) == C_GREEN &&
          gfx_test_pixel(x - 14, y + 5) == C_GREEN &&
          gfx_test_pixel(x + 14, y + 5) == C_GREEN,
          "selected building has a green front arc");
    /* The art spans x-12..x+12 from y-8, so it hides the top of the ring. */
    check(gfx_test_pixel(x - 13, y + 4) == C_GREEN &&
          gfx_test_pixel(x + 13, y + 4) == C_GREEN,
          "selected building has a far arc beside its art");
    check(count_color(x - 12, y - 3, 25, 1, C_GREEN) == 0,
          "building art covers the far arc");
    selected_building = -1;
}

int
main(void)
{
    gba_init();
    game_init();
    deselect_tests();
    smart_move_tests();
    unfinished_producer_tests();
    smart_attack_tests();
    r_gesture_tests();
#ifndef SC_COMMAND_RING /* the ring build opens its list on Move */
    menu_memory_tests();
#endif
    simultaneous_failure_tests();
    refinery_snap_tests();
    title_and_pause_tests();
    render_smoke();
    selection_circle_tests();
    queue_panel_tests();
    research_and_construction_panel_tests();
    panel_marks_tests();
    if (failures) return 1;
#ifdef SC_COMMAND_RING
    puts("Default menus and controls (ring build): deselect, smart move, smart attack, R tap, placement status order, snapping, title/pause/result, selection ellipses, queue panel: ok");
#else
    puts("Default menus and controls: deselect, smart move, smart attack, R tap, list memory, placement status order, snapping, title/pause/result, selection ellipses, queue panel: ok");
#endif
    return 0;
}
