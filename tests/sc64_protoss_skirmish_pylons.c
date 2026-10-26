/* SPDX-License-Identifier: GPL-3.0-or-later
 * Normal-input Blood Bath/Pylon-construction regression.
 * Uses the existing host controller's menu, selection, cursor and input
 * helpers; no unit/building/resource fields are written by the fixture. */
#define main sc64_skirmish_campaign_run_unused_main
#include "sc64_skirmish_campaign_run.c"
#undef main

static int render_snapshots;

static void render_snapshot(void)
{
    game_render();
    ++render_snapshots;
}

static void tap_rendered(u16 key)
{
    input(key);
    render_snapshot();
    input(0);
    render_snapshot();
}

static void move_cursor_rendered(int x, int y)
{
    unsigned guard = 0;
    while ((cx != x || cy != y) && guard++ < 2000u && host_ticks < match_limit &&
           screen == SCREEN_PLAY && !won && !lost) {
        u16 held = KEY_R;
        if (cx < x) held |= KEY_RIGHT;
        if (cx > x) held |= KEY_LEFT;
        if (cy < y) held |= KEY_DOWN;
        if (cy > y) held |= KEY_UP;
        input(held);
        render_snapshot();
    }
    input(0);
    render_snapshot();
}

static int select_free_probe(void)
{
    for (int tries = 0; tries < MAX_UNITS; ++tries) {
        int worker = find_selected_worker();
        if (worker >= 0 && u[worker].type == U_PROBE && u[worker].order != ORDER_BUILD &&
            selected_count() == 1) return worker;
        /* L tap cycles the normal field selection to the next unit. */
        tap_rendered(KEY_L);
    }
    return -1;
}

static int issue_pylon(void)
{
    int worker, x = -1, y = -1;
    int before = count_buildings(SIDE, B_PYLON);
    if (minerals < sc64_race_building_rule(B_PYLON)->minerals ||
        !choose_build_site(B_PYLON, &x, &y)) return 0;
    worker = select_free_probe();
    if (worker < 0) return 0;

    tap_rendered(KEY_B);
    for (int guard = 0; menu_cursor != MENU_BARRACKS + 1 && guard < 8; ++guard)
        tap_rendered(KEY_DOWN);
    if (!menu_open || menu_cursor != MENU_BARRACKS + 1) return 0;
    tap_rendered(KEY_A);
    if (mode != MODE_BUILD || menu != B_PYLON) return 0;
    move_cursor_rendered(x, y);
    tap_rendered(KEY_A);
    /* A nearby Probe may arrive and return to mining during the click's
     * update; count the world result rather than assuming it remains busy. */
    return count_buildings(SIDE, B_PYLON) > before;
}

int main(void)
{
    enum { TARGET_NEW_PYLONS = 2, RUN_UPDATES = 5400 };
    int starting_pylons, issued = 0;
    gba_init();
    option_map = 1;              /* Imported Blood Bath. */
    option_human = R_PROTOSS;
    option_bots = 1;
    option_bot_races[0] = R_TER; /* The normal menu's default Terran AI. */
    match_limit = RUN_UPDATES;
    trace = NULL;
    if (!launch_match()) {
        fprintf(stderr, "pylon regression: normal Blood Bath launch failed\n");
        return 1;
    }
    starting_pylons = count_buildings(SIDE, B_PYLON);
    if (starting_pylons < 1) {
        fprintf(stderr, "pylon regression: no starting Protoss Pylon\n");
        return 1;
    }

    while (host_ticks < RUN_UPDATES && screen == SCREEN_PLAY && !won && !lost) {
        input(0);
        render_snapshot();
        if (issued < TARGET_NEW_PYLONS && issue_pylon()) {
            ++issued;
            fprintf(stderr, "pylon regression: accepted Pylon %d/%d at tick %u\n",
                    issued, TARGET_NEW_PYLONS, host_ticks);
        }
    }

    int pylons = count_buildings(SIDE, B_PYLON);
    int completed = 0, under_construction = 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive && b[i].owner == SIDE &&
                                                  b[i].type == B_PYLON) {
        if (building_complete(i)) ++completed;
        else ++under_construction;
    }
    printf("pylon-regression map=BloodBath human=Protoss bots=1 race=Terran "
           "updates=%u snapshots=%d started=%d pylons=%d completed=%d constructing=%d "
           "buildings=%d/%d units=%d/%d screen=%d won=%d lost=%d\n",
           host_ticks, render_snapshots, issued, pylons, completed, under_construction,
           count_buildings(SIDE, -1), MAX_BUILDINGS, count_units(SIDE, -1), MAX_UNITS,
           screen, won, lost);

    /* This bounded fixture validates setup and UI-issued work; it does not
     * exercise slot exhaustion or qualify the physical-cartridge report. */
    return issued == TARGET_NEW_PYLONS && pylons >= starting_pylons + TARGET_NEW_PYLONS &&
           host_ticks >= RUN_UPDATES ? 0 : 2;
}
