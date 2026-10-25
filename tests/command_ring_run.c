/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Hold-B command ring regression for the default profile. Every gesture goes
 * through game_update() key edges. Each ring command must leave the game in
 * exactly the state the classic B command list produces for the same
 * command, after the same number of simulation updates. */
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

/* Hold B, press each direction in turn while holding it, then release. */
static void
ring(const u16 *steps, int count)
{
    input(KEY_B);
    for (int i = 0; i < count; ++i) {
        input((u16)(KEY_B | steps[i]));
        input(KEY_B);
    }
    input(0);
}

/* Classic list: tap B, move down to the row, tap A. */
static void
list(int row)
{
    tap(KEY_B);
    for (int i = 0; i < row; ++i) tap(KEY_DOWN);
    tap(KEY_A);
}

static int
fail(const char *name, const char *message)
{
    fprintf(stderr, "FAIL command ring %s: %s\n", name, message);
    return 1;
}

typedef struct {
    Unit units[MAX_UNITS];
    Building buildings[MAX_BUILDINGS];
    int minerals, gas, supply_used, frame, weapons_research, armor_research, mode, menu;
} Snapshot;

static void
snapshot(Snapshot *s)
{
    memset(s, 0, sizeof(*s));
    memcpy(s->units, u, sizeof(u));
    memcpy(s->buildings, b, sizeof(b));
    s->minerals = minerals;
    s->gas = gas;
    s->supply_used = supply_used;
    s->frame = frame;
    s->weapons_research = weapons_research;
    s->armor_research = armor_research;
    s->mode = mode;
    s->menu = menu;
}

static int
find_building(int type)
{
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == SIDE && b[i].type == type) return i;
    return -1;
}

enum { SELECT_ARMY, SELECT_WORKER, SELECT_BARRACKS, SELECT_ENG_BAY };

static void
fixture(int selection, int x, int y)
{
    previous_held = 0;
    init_state();
    spawn_build(SIDE, B_ENG_BAY, 40, 150, 0);
    spawn_build(SIDE, B_ACADEMY, 40, 40, 0);
    minerals = 900;
    gas = 300;
    cx = x;
    cy = y;
    for (int i = 0; i < MAX_UNITS; ++i) u[i].selected = 0;
    selected_building = -1;
    if (selection == SELECT_ARMY) {
        for (int i = 0; i < MAX_UNITS; ++i)
            if (u[i].alive && u[i].owner == SIDE && u[i].type == U_MARINE) u[i].selected = 1;
    } else if (selection == SELECT_WORKER) {
        for (int i = 0; i < MAX_UNITS; ++i)
            if (u[i].alive && u[i].owner == SIDE && u[i].type == U_SCV) { u[i].selected = 1; break; }
    } else
        selected_building = find_building(selection == SELECT_BARRACKS ? B_BARRACKS : B_ENG_BAY);
}

static void
settle(int updates)
{
    for (int i = 0; i < updates; ++i) input(0);
}

typedef struct {
    const char *name;
    int selection, x, y;
    u16 steps[3];
    int step_count;
    int ring_extra_a;   /* A taps while B is held */
    int ring_confirm_a; /* A taps at the cursor after release */
    int list_row;
    int list_confirm_a; /* taps of A at the cursor after the list choice */
    int list_extra_a;   /* A taps near a building to queue more */
} Case;

static int
run_case(const Case *c)
{
    Snapshot by_ring, by_list;
    int updates = 240;
    /* The list's A release after choosing a targeted command runs one update
     * before the confirming A; start the ring one update later to match. */
    int lead = c->list_row >= 0 && c->list_confirm_a ? 1 : 0;

    fixture(c->selection, c->x, c->y);
    settle(lead);
    input(KEY_B);
    for (int i = 0; i < c->step_count; ++i) {
        input((u16)(KEY_B | c->steps[i]));
        input(KEY_B);
    }
    for (int i = 0; i < c->ring_extra_a; ++i) {
        input((u16)(KEY_B | KEY_A));
        input(KEY_B);
    }
    if (frame != lead) return fail(c->name, "simulation advanced while B was held");
    input(0);
    if (menu_open) return fail(c->name, "ring release opened the command list");
    for (int i = 0; i < c->ring_confirm_a; ++i) tap(KEY_A);
    settle(updates - frame);
    snapshot(&by_ring);

    fixture(c->selection, c->x, c->y);
    if (c->list_row >= 0) list(c->list_row);
    for (int i = 0; i < c->list_confirm_a + c->list_extra_a; ++i) tap(KEY_A);
    settle(updates - frame);
    snapshot(&by_list);

    if (by_ring.frame != updates || by_list.frame != updates)
        return fail(c->name, "unexpected update count");
    if (memcmp(&by_ring, &by_list, sizeof(by_ring))) {
        fprintf(stderr, "ring minerals=%d gas=%d supply=%d research=%d/%d mode=%d\n",
                by_ring.minerals, by_ring.gas, by_ring.supply_used,
                by_ring.weapons_research, by_ring.armor_research, by_ring.mode);
        fprintf(stderr, "list minerals=%d gas=%d supply=%d research=%d/%d mode=%d\n",
                by_list.minerals, by_list.gas, by_list.supply_used,
                by_list.weapons_research, by_list.armor_research, by_list.mode);
        return fail(c->name, "ring and list paths diverged");
    }
    return 0;
}

static int
find_site(int type, int *x, int *y)
{
    for (int yy = 150; yy >= 40; yy -= 8)
        for (int xx = 20; xx < 200; xx += 8)
            if (can_place(type, xx, yy)) { *x = xx; *y = yy; return 1; }
    return 0;
}

int
main(void)
{
    int depot_x, depot_y, academy_x, academy_y, failures = 0;
    gba_init();
    init_state();
    if (!find_site(B_DEPOT, &depot_x, &depot_y) || !find_site(B_ACADEMY, &academy_x, &academy_y))
        return fail("setup", "no free building site");
    int barracks = find_building(B_BARRACKS);
    if (barracks < 0) return fail("setup", "no starting Barracks");
    int bx = b[barracks].x, by = b[barracks].y;

    const Case cases[] = {
        { "move", SELECT_ARMY, 200, 120, { KEY_UP }, 1, 0, 0, 0, 1, 0 },
        { "attack move", SELECT_ARMY, 260, 70, { KEY_RIGHT }, 1, 0, 0, 1, 1, 0 },
        { "stop", SELECT_ARMY, 200, 120, { KEY_DOWN }, 1, 0, 0, 9, 0, 0 },
        { "build depot", SELECT_WORKER, depot_x, depot_y, { KEY_LEFT, KEY_UP }, 2, 0, 1, 3, 1, 0 },
        { "build academy", SELECT_WORKER, academy_x, academy_y,
          { KEY_LEFT, KEY_LEFT, KEY_UP }, 3, 0, 1, 5, 1, 0 },
        /* Release queues one Marine and A queues another; on the list path,
         * A on the selected Barracks queues its current training type. */
        { "train marines", SELECT_BARRACKS, bx, by, { KEY_UP }, 1, 1, 0, -1, 0, 2 },
        { "weapons upgrade", SELECT_ENG_BAY, 60, 120, { KEY_UP }, 1, 0, 0, 7, 0, 0 },
        { "armor upgrade", SELECT_ENG_BAY, 60, 120, { KEY_RIGHT }, 1, 0, 0, 8, 0, 0 },
    };
    for (unsigned i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i)
        failures += run_case(&cases[i]);

    /* Ring-only commands with no list equivalent change the expected state. */
    fixture(SELECT_BARRACKS, bx, by);
    {
        static const u16 firebat[] = { KEY_RIGHT };
        ring(firebat, 1);
        if (train_type != U_FIREBAT || b[barracks].queue_count != 1)
            failures += fail("train firebat", "Firebat was not queued and selected");
    }

    /* A short tap still opens the classic list; it starts on the first row. */
    fixture(SELECT_ARMY, 200, 120);
    tap(KEY_B);
    if (!menu_open || menu_cursor != 0) failures += fail("tap", "tap did not open the list");
    tap(KEY_B);
    if (menu_open) failures += fail("tap", "second tap did not close the list");

    /* Holding with no direction and releasing cancels, after drawing the ring. */
    fixture(SELECT_ARMY, 200, 120);
    input(KEY_B);
    for (int i = 0; i < 20; ++i) input(KEY_B);
    if (!command_ring_visible()) failures += fail("hold", "ring not shown while held");
    game_render();
    input(0);
    if (menu_open || command_ring_visible() || frame != 0)
        failures += fail("hold", "release without a choice was not a cancel");
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].selected && u[i].tx == 200 && u[i].ty == 120)
            failures += fail("hold", "cancel issued an order");

    /* L backs out of a submenu; directions with no slot clear the choice. */
    fixture(SELECT_WORKER, 200, 120);
    input(KEY_B);
    input(KEY_B | KEY_LEFT);
    input(KEY_B);
    for (int i = 0; i < 8; ++i) input(KEY_B);
    game_render();
    input(KEY_B | KEY_L);
    input(KEY_B);
    input(KEY_B | KEY_UP);
    input(KEY_B);
    input(0);
    if (mode != MODE_NORMAL) failures += fail("back", "L did not return to the order page");
    {
        int moved = 0;
        for (int i = 0; i < MAX_UNITS; ++i)
            if (u[i].alive && u[i].selected && u[i].order == ORDER_MOVE &&
                u[i].tx == 200 && u[i].ty == 120) moved = 1;
        if (!moved) failures += fail("back", "Move after L did not issue");
    }

    /* Without a worker, the build page is not offered. */
    fixture(SELECT_ARMY, 200, 120);
    {
        static const u16 left_up[] = { KEY_LEFT, KEY_UP };
        ring(left_up, 2);
        if (mode != MODE_NORMAL) failures += fail("no worker", "army opened build placement");
    }

    /* The ring does not open over an active order mode; B still cancels it. */
    fixture(SELECT_ARMY, 200, 120);
    list(0);
    if (mode != MODE_MOVE) failures += fail("mode", "list Move did not enter targeting");
    input(KEY_B);
    if (mode != MODE_NORMAL || ring_active) failures += fail("mode", "B did not cancel targeting");
    input(0);

    if (failures) return 1;
    puts("Command ring orders, building, training, upgrades and tap/hold/back gestures: ok");
    return 0;
}
