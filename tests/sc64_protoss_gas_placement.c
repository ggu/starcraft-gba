/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Normal-controller gas building placement on the imported Blood Bath map. */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/game.c"

static u16 prior;

static void input(u16 held)
{
    u16 pressed = (u16)(held & (u16)~prior);
    game_update(held, pressed);
    prior = held;
}

static void tap(u16 key)
{
    input(key);
    input(0);
}

static void move_cursor_to(int x, int y)
{
    unsigned guard = 0;
    while ((cx != x || cy != y) && guard++ < 4096) {
        u16 key = cx < x ? KEY_RIGHT : cx > x ? KEY_LEFT :
                  cy < y ? KEY_DOWN : KEY_UP;
        input(key);
        if (cx == x || cy == y) input(0);
    }
    input(0);
    assert(cx == x && cy == y);
}

static int gas_building_for_race(int race)
{
    return race == R_TER ? B_REFINERY :
           race == R_ZERG ? B_EXTRACTOR : B_ASSIMILATOR;
}

static int worker_for_race(int race)
{
    return race == R_TER ? U_SCV : race == R_ZERG ? U_DRONE : U_PROBE;
}

static void setup_skirmish(int race)
{
    gba_init();
    game_init();
    race_mode = 1;
    race_human = race;
    race_bots = 1;
    race_bot_races[0] = R_TER;
    race_skirmish_map = 1; /* Imported Blood Bath. */
    init_state();
    screen = SCREEN_PLAY;
    prior = 0;
    minerals = 1000;
    gas = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && IS_WORKER(u[i].type))
            u[i].order = ORDER_IDLE;
}

static int choose_far_opaque_pixel(const Sc64SpriteFrame *sprite, int center_x,
                                   int center_y, int parity_x, int parity_y,
                                   int require_parity, int *out_x, int *out_y)
{
    int left = center_x - sprite->canvas_w / 2 + sprite->offset_x;
    int top = center_y - sprite->canvas_h / 2 + sprite->offset_y;
    int best = 15 * 15;
    int found = 0;
    for (int y = 0; y < sprite->h; ++y) for (int x = 0; x < sprite->w; ++x) {
        int px = left + x, py = top + y;
        int dx = px - center_x, dy = py - center_y;
        int distance = dx * dx + dy * dy;
        if (sprite->pixels[y * sprite->w + x] && distance > best &&
            (!require_parity || ((px & 1) == parity_x && (py & 1) == parity_y))) {
            best = distance;
            *out_x = px;
            *out_y = py;
            found = 1;
        }
    }
    return found;
}

static int find_worker(int race)
{
    int type = worker_for_race(race);
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].type == type) return i;
    return -1;
}

static int find_gas_building(int type, int x, int y)
{
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == SIDE && b[i].type == type &&
            b[i].x == x && b[i].y == y) return i;
    return -1;
}

/* Exercise opaque-art hit testing, common preview validation, and adapter
 * snapping for all four Blood Bath geysers and all three playable races. */
static void test_edge_site(int race, unsigned geyser_index)
{
    int type = gas_building_for_race(race);
    const Sc64SpriteFrame *sprite = sc64_race_unit_sprite(188);
    int x, y, worker, before, building;
    setup_skirmish(race);
    assert(sprite && sprite->pixels && sprite->w == 56 && sprite->h == 26);
    assert(geyser_count == 4 && geyser_index < (unsigned)geyser_count);
    assert(choose_far_opaque_pixel(sprite, geysers[geyser_index].x,
                                   geysers[geyser_index].y, 0, 0, 0, &x, &y));
    assert(geyser_at(x, y) == (int)geyser_index);
    assert(geyser_at(geysers[geyser_index].x,
                     geysers[geyser_index].y) == (int)geyser_index);
    assert(geyser_at(geysers[geyser_index].x - sprite->canvas_w / 2 - 2,
                     geysers[geyser_index].y) == -1);
    assert(race_can_site(SIDE, type, x, y));
    assert(can_place(type, x, y));
    worker = find_worker(race);
    assert(worker >= 0);
    before = minerals;
    assert(race_start_build(SIDE, worker, type, x, y));
    assert(minerals == before);
    assert(u[worker].order == ORDER_BUILD_PENDING && u[worker].target == type);
    assert(find_gas_building(type, geysers[geyser_index].x,
                             geysers[geyser_index].y) < 0);
    /* This placement fixture advances directly to the authored geyser arrival
       point, then ticks the real pending-order commit path. */
    u[worker].x = geysers[geyser_index].x;
    u[worker].y = geysers[geyser_index].y;
    update_orders();
    assert(minerals == before - building_cost(type));
    building = find_gas_building(type, geysers[geyser_index].x,
                                 geysers[geyser_index].y);
    assert(building >= 0 && b[building].under_construction);
    if (race == R_PROTOSS)
        assert(u[worker].alive && u[worker].order == ORDER_IDLE &&
               u[worker].target == -1);
    else if (race == R_ZERG)
        assert(!u[worker].alive);
    else
        assert(u[worker].order == ORDER_BUILD && u[worker].target == building);
    assert(!can_place(type, x, y));
    assert(!race_start_build(SIDE, worker, type, x, y));
}

/* Use ordinary field input for a Protoss Assimilator on a clickable edge
 * pixel, then complete it and gather gas with the same selected Probe. */
static void test_protoss_menu_and_harvest(void)
{
    const int race = R_PROTOSS;
    const int type = B_ASSIMILATOR;
    const unsigned geyser_index = 1; /* (32,208), nearest Blood Bath geyser. */
    const Sc64SpriteFrame *sprite = sc64_race_unit_sprite(188);
    int worker, cursor_x, cursor_y, building, minerals_before, arrival_minerals;
    int geyser_x, geyser_y;
    setup_skirmish(race);
    assert(sprite && sprite->pixels);
    assert(geyser_index < (unsigned)geyser_count);
    geyser_x = geysers[geyser_index].x;
    geyser_y = geysers[geyser_index].y;
    worker = find_worker(race);
    assert(worker >= 0);
    assert(choose_far_opaque_pixel(sprite, geyser_x, geyser_y,
                                   u[worker].x & 1, u[worker].y & 1, 1,
                                   &cursor_x, &cursor_y));
    assert((cursor_x - geyser_x) * (cursor_x - geyser_x) +
           (cursor_y - geyser_y) * (cursor_y - geyser_y) > 15 * 15);

    cx = u[worker].x;
    cy = u[worker].y;
    tap(KEY_A);
    assert(u[worker].selected);
    tap(KEY_B);
    assert(menu_open);
    for (int row = 0; row < 4; ++row) tap(KEY_DOWN);
    assert(menu_cursor == 4);
    tap(KEY_A);
    assert(mode == MODE_BUILD && menu == type);
    move_cursor_to(cursor_x, cursor_y);
    assert(cx == cursor_x && cy == cursor_y);
    assert(geyser_at(cx, cy) == (int)geyser_index);
    assert(race_can_site(SIDE, type, cx, cy) && can_place(type, cx, cy));

    /* Placement preview follows the centered building even while the cursor
       is on an offset edge pixel of the visible geyser crop. */
    game_render();
    int ghost_x = geyser_x - camx - 8;
    int ghost_y = geyser_y - camy + 12 - 6;
    assert(gfx_test_pixel(ghost_x, ghost_y) == C_GREEN);

    minerals_before = minerals;
    tap(KEY_A);
    assert(mode == MODE_NORMAL && minerals == minerals_before);
    assert(u[worker].order == ORDER_BUILD_PENDING && u[worker].target == type);
    assert(find_gas_building(type, geyser_x, geyser_y) < 0);

    /* This menu/render fixture pins arrival explicitly so background workers'
       mineral deliveries cannot blur the construction debit assertion. */
    arrival_minerals = minerals;
    u[worker].x = geyser_x; u[worker].y = geyser_y;
    input(0);
    assert(minerals == arrival_minerals - 100);
    building = find_gas_building(type, geyser_x, geyser_y);
    assert(building >= 0 && b[building].under_construction);
    assert(u[worker].alive && u[worker].order == ORDER_IDLE &&
           u[worker].target == -1);
    assert(!can_place(type, cx, cy));
    for (int i = 0; i < 1200 && !building_complete(building); ++i)
        input(0);
    if (!building_complete(building))
        fprintf(stderr, "Assimilator build stalled worker=%d order=%d target=%d xy=%d,%d path=%d,%d progress=%d\n",
                worker, u[worker].order, u[worker].target, u[worker].x, u[worker].y,
                u[worker].path_x, u[worker].path_y, b[building].build_progress);
    assert(building_complete(building));
    assert(u[worker].order != ORDER_BUILD);

    move_cursor_to(geyser_x, geyser_y);
    tap(KEY_A);
    assert(u[worker].order == ORDER_GAS && u[worker].target == building);
    for (int i = 0; i < 1200 && gas == 0; ++i)
        input(0);
    assert(gas > 0);
}

int main(void)
{
    static const int races[] = {R_TER, R_ZERG, R_PROTOSS};
    for (unsigned ri = 0; ri < sizeof(races) / sizeof(races[0]); ++ri)
        for (unsigned geyser = 0; geyser < 4; ++geyser)
            test_edge_site(races[ri], geyser);
    test_protoss_menu_and_harvest();
    puts("PASS Blood Bath gas placement/snap on all races/geysers; Protoss UI and harvest (13 cases)");
    return 0;
}
