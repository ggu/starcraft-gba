/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Real world painter versus explicit mining-effect draw orders. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

enum { FRAME_PIXELS = 240 * 160 };
enum { EFFECT_ON_LAYER_FIVE, EFFECT_BEHIND_GROUND, EFFECT_ABOVE_AIR };
static u8 expected[FRAME_PIXELS], reverse_ground[FRAME_PIXELS];
static u8 reverse_air[FRAME_PIXELS], actual[FRAME_PIXELS];
static int worker, flyer;

static void capture(u8 *out)
{
    for (int y = 0; y < 160; ++y)
        for (int x = 0; x < 240; ++x)
            out[y * 240 + x] = gfx_test_pixel(x, y);
}

static void manual(int order, u8 *out)
{
    gfx_begin(C_BLACK);
    gfx_set_clip_y(12, 120);
    if (order == EFFECT_BEHIND_GROUND) sc64_draw_mining_effect(&u[worker]);
    draw_map_mineral(0);
    draw_unit(&u[worker]);
    if (order == EFFECT_ON_LAYER_FIVE) sc64_draw_mining_effect(&u[worker]);
    draw_unit(&u[flyer]);
    if (order == EFFECT_ABOVE_AIR) sc64_draw_mining_effect(&u[worker]);
    capture(out);
    gfx_end();
}

static void world(u8 *out)
{
    gfx_begin(C_BLACK);
    gfx_set_clip_y(12, 120);
    sc64_draw_depth_ordered_actors();
    capture(out);
    gfx_end();
}

static unsigned differences(const u8 *a, const u8 *b)
{
    unsigned count = 0;
    for (int i = 0; i < FRAME_PIXELS; ++i) count += a[i] != b[i];
    return count;
}

int main(void)
{
    game_init();
    race_mode = 1;
    race_human = R_TER;
    race_bots = 1;
    race_skirmish_map = 0;
    race_skirmish_init();
    memset(u, 0, sizeof(u));
    memset(b, 0, sizeof(b));
    memset(m, 0, sizeof(m));
    memset(visible, 0xff, sizeof(visible));
    geyser_count = 0;
    camx = camy = 0;
    screen = SCREEN_PLAY;
    selected_building = -1;
    worker = spawn_unit(SIDE, U_SCV, 100, 70);
    flyer = spawn_unit(SIDE, U_WRAITH, 100, 70);
    assert(worker >= 0 && flyer >= 0);
    Unit *q = &u[worker];
    q->order = ORDER_MINE;
    q->target = 0;
    q->work_timer = 1;
    q->carry = 0;
    m[0] = (Mineral){.x = 100, .y = 66, .amount = 1000};
    int x, y;
    assert(sc64_mining_effect_anchor(q, &x, &y));
    assert(x == 100 && y == 80);

    Unit inactive[] = {*q, *q, *q, *q, *q, *q, *q, *q, *q, *q};
    inactive[0].order = ORDER_IDLE;
    inactive[1].order = ORDER_MOVE;
    inactive[2].carry = 8;
    inactive[3].work_timer = 0;
    inactive[4].target = -1;
    inactive[5].target = MAX_MINERALS;
    inactive[6].alive = 0;
    inactive[7].type = U_MARINE;
    inactive[8].x = 108;
    inactive[9].flags |= 2u;
    for (unsigned i = 0; i < sizeof(inactive) / sizeof(inactive[0]); ++i)
        assert(!sc64_mining_effect_anchor(&inactive[i], &x, &y));
    m[0].amount = 0;
    assert(!sc64_mining_effect_anchor(q, &x, &y));
    m[0].amount = 1000;
    memset(visible, 0, sizeof(visible));
    assert(!sc64_mining_effect_anchor(q, &x, &y));
    memset(visible, 0xff, sizeof(visible));

    /* Search for actual overlaps with both ground and flyer sprites. */
    unsigned ground = 0, air = 0;
    for (int dy = -12; dy <= 12 && !(ground && air); ++dy) {
        for (int dx = -12; dx <= 12 && !(ground && air); ++dx) {
            u[flyer].x = 100 + dx;
            u[flyer].y = 70 + dy;
            manual(EFFECT_ON_LAYER_FIVE, expected);
            manual(EFFECT_BEHIND_GROUND, reverse_ground);
            manual(EFFECT_ABOVE_AIR, reverse_air);
            ground = differences(expected, reverse_ground);
            air = differences(expected, reverse_air);
        }
    }
    assert(ground && air);
    for (int page = 0; page < 2; ++page) {
        world(actual);
        assert(!differences(expected, actual));
        for (int yy = 0; yy < 160; ++yy) {
            if (yy >= 12 && yy < 120) continue;
            for (int xx = 0; xx < 240; ++xx)
                assert(actual[yy * 240 + xx] == C_BLACK);
        }
    }
    printf("mining world depth: %u ground and %u air pixels, both pages\n", ground, air);

    /* A visible midpoint cannot leak an enemy worker hidden across a cell edge. */
    q->owner = ENEMY;
    q->x = 15;
    m[0].x = 21;
    m[0].y = 70;
    u[flyer].alive = 0;
    memset(visible, 0, sizeof(visible));
    SC64_GRID_SET(visible, 1 + (70 / 16) * MAP_COLS, 1);
    assert(sc64_mining_effect_anchor(q, &x, &y));
    assert(!currently_visible(q->x, q->y));
    for (int page = 0; page < 2; ++page) {
        q->alive = 1;
        world(expected);
        q->alive = 0;
        world(actual);
        assert(!differences(expected, actual));
    }
    puts("hidden enemy worker does not leak effect on either page");
    return 0;
}
