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
        if (worker >= 0 && u[worker].type == U_PROBE &&
            u[worker].order != ORDER_BUILD && !SC64_PENDING_BUILD_ORDER(u[worker].order) &&
            selected_count() == 1) return worker;
        /* L tap cycles the normal field selection to the next unit. */
        tap_rendered(KEY_L);
    }
    return -1;
}

/* choose_build_site() checks terrain and buildings but not arriving units.
 * Leave a short movement buffer around the UI site so another unit does not
 * enter the Pylon footprint before its builder arrives. */
static int choose_pylon_site(int worker, int *x, int *y)
{
    int best_distance = 0x7fffffff, found = 0;
    for (int ring = 0; ring < 3; ++ring)
        for (int sy = race_base_y[0] - 96 - ring * 32;
             sy <= race_base_y[0] + 96 + ring * 32; sy += 16)
            for (int sx = race_base_x[0] - 96 - ring * 32;
                 sx <= race_base_x[0] + 96 + ring * 32; sx += 16) {
                if (sx < 24 || sy < 24 || sx >= WORLD_W - 24 || sy >= WORLD_H - 24 ||
                    !race_can_site(SIDE, B_PYLON, sx, sy) || !can_place(B_PYLON, sx, sy)) continue;
                int clear = 1;
                for (int i = 0; i < MAX_UNITS; ++i)
                    if (i != worker && u[i].alive &&
                        dist2(u[i].x, u[i].y, sx, sy) < 40 * 40) clear = 0;
                if (!clear) continue;
                int d = dist2(cx, cy, sx, sy);
                if (d < best_distance) { best_distance = d; *x = sx; *y = sy; found = 1; }
            }
    return found;
}

static int issue_pylon(void)
{
    int worker, x = -1, y = -1;
    int before = count_buildings(SIDE, B_PYLON);
    const Sc64RaceBuildingRule *rule = sc64_race_building_rule(B_PYLON);
    if (!rule || minerals < rule->minerals) return 0;
    worker = select_free_probe();
    if (worker < 0) return 0;
    if (!choose_pylon_site(worker, &x, &y)) return 0;

    tap_rendered(KEY_B);
#ifdef SC64_COMMAND_ICONS /* the command card: right to Pylon's column, then down */
    for (int guard = 0; menu_cursor != MENU_BARRACKS + 1 && guard < 16; ++guard)
        tap_rendered(card_fits() && card_command_slot(menu_cursor) % CARD_COLUMNS !=
                     card_command_slot(MENU_BARRACKS + 1) % CARD_COLUMNS ? KEY_RIGHT : KEY_DOWN);
#else
    for (int guard = 0; menu_cursor != MENU_BARRACKS + 1 && guard < 8; ++guard)
        tap_rendered(KEY_DOWN);
#endif
    if (!menu_open || menu_cursor != MENU_BARRACKS + 1) return 0;
    tap_rendered(KEY_A);
    if (mode != MODE_BUILD || menu != B_PYLON) return 0;
    move_cursor_rendered(x, y);
    int before_click_minerals = minerals;
    int before_click_pylons = count_buildings(SIDE, B_PYLON);
    input(KEY_A);
    render_snapshot();
    int click_buildings = count_buildings(SIDE, B_PYLON);
    int click_mineral_delta = before_click_minerals - minerals;
    if (click_buildings > before_click_pylons)
        return click_mineral_delta > 0 && click_mineral_delta <= rule->minerals;
    int pending = u[worker].order == ORDER_BUILD_PENDING && u[worker].target == B_PYLON;
    if (!pending || click_mineral_delta != 0) return 0;

    int before_release_minerals = minerals;
    int before_release_pylons = click_buildings;
    input(0);
    render_snapshot();
    int release_buildings = count_buildings(SIDE, B_PYLON);
    if (release_buildings > before_release_pylons)
        return before_release_minerals - minerals > 0 &&
               before_release_minerals - minerals <= rule->minerals;
    for (int wait = 0; wait < 2400 && count_buildings(SIDE, B_PYLON) == before; ++wait) {
        int minerals_before_tick = minerals;
        int pylons_before_tick = count_buildings(SIDE, B_PYLON);
        int order_before_tick = u[worker].order;
        input(0);
        render_snapshot();
        int pylons_after_tick = count_buildings(SIDE, B_PYLON);
        if (pylons_after_tick > pylons_before_tick) {
            int debit = minerals_before_tick - minerals;
            /* A Probe can deposit minerals on the same tick as foundation
             * commit, so the observed net debit can be smaller than cost. */
            return debit > 0 && debit <= rule->minerals;
        }
        if (order_before_tick == ORDER_BUILD_PENDING &&
            u[worker].order != ORDER_BUILD_PENDING) return 0;
    }
    return 0;
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
           completed >= starting_pylons + TARGET_NEW_PYLONS && host_ticks >= RUN_UPDATES ? 0 : 2;
}
