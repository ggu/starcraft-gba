/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Cargo badge is a UI cue; the private source bank contains no cargo art. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

static u8 pixels[240 * 160];

static void render_unit(Unit *q)
{
    memset(u, 0, sizeof(u));
    u[0] = *q;
    Unit before = u[0];
    gfx_begin(C_BLACK);
#ifdef GBA_SCENE_CLIP
    gfx_set_clip_y(12, 120);
#endif
    draw_unit(&u[0]);
    assert(memcmp(&before, &u[0], sizeof(before)) == 0);
    gfx_end();
    for (int y = 0; y < 160; ++y)
        for (int x = 0; x < 240; ++x)
            pixels[y * 240 + x] = gfx_test_pixel(x, y);
}

static void check_worker(int type, int raw, int order, int gas)
{
    Unit q;
    memset(&q, 0, sizeof(q));
    q.alive = 1;
    q.owner = SIDE;
    q.type = type;
    q.raw_type = raw;
    q.x = 90;
    q.y = 70;
    q.hp = q.maxhp = 60;
    q.carry = 8;
    q.order = order;
    camx = camy = 0;

    const Sc64SpriteFrame *s = raw >= 0 ? sc64_race_unit_sprite(raw) : 0;
    if (!s) s = mission_unit_sprite(type);
    assert(s && s->pixels && s->w && s->h);
    int sx = q.x - s->canvas_w / 2 + s->offset_x;
    int sy = q.y + 12 - s->canvas_h / 2 + s->offset_y;
    int bx = sx + s->w + 2;
    int by = sy + s->h / 2;

    Unit before = q;
    render_unit(&q);
    assert(memcmp(&before, &q, sizeof(q)) == 0);
    if (gas) {
        assert(gfx_test_pixel(bx, by - 2) == C_GREEN_DARK);
        assert(gfx_test_pixel(bx - 1, by - 1) == C_WHITE);
        assert(gfx_test_pixel(bx, by) == C_GREEN);
    } else {
        assert(gfx_test_pixel(bx, by - 3) == C_CYAN);
        assert(gfx_test_pixel(bx, by - 2) == C_WHITE);
        assert(gfx_test_pixel(bx, by) == C_BLUE);
    }

    u8 carried[240 * 160];
    memcpy(carried, pixels, sizeof(carried));
    int top_row = gas ? by - 2 : by - 3;
    u8 top_color = gas ? C_GREEN_DARK : C_CYAN;
    q.carry = 0;
    render_unit(&q);
    assert(memcmp(carried, pixels, sizeof(carried)) != 0);
    assert(gfx_test_pixel(bx, top_row) != top_color);

    q.carry = 8;
    q.order = ORDER_IDLE;
    render_unit(&q);
    assert(gfx_test_pixel(bx, by) != C_WHITE);

    /* Caster energy and non-worker cargo fields do not draw a cargo marker. */
    q.type = U_MARINE;
    q.raw_type = -1;
    q.carry = 8;
    q.order = ORDER_IDLE;
    const Sc64SpriteFrame *fallback = mission_unit_sprite(q.type);
    assert(fallback);
    sx = q.x - fallback->canvas_w / 2 + fallback->offset_x;
    sy = q.y + 12 - fallback->canvas_h / 2 + fallback->offset_y;
    bx = sx + fallback->w + 2;
    by = sy + fallback->h / 2;
    render_unit(&q);
    assert(gfx_test_pixel(bx, by) != C_WHITE);
    q.order = ORDER_MINE;
    render_unit(&q);
    assert(gfx_test_pixel(bx, by) != C_WHITE);
}

static void check_fog_and_map_clip(void)
{
    Unit q;
    memset(&q, 0, sizeof(q));
    q.alive = 1; q.owner = 1; q.type = U_PROBE; q.raw_type = 64;
    q.x = 90; q.y = 70; q.hp = q.maxhp = 20;
    q.carry = 8; q.order = ORDER_MINE;
    memset(visible, 0, sizeof(visible));
    render_unit(&q);
    for (int i = 0; i < 240 * 160; ++i)
        assert(pixels[i] == C_BLACK);

    q.owner = SIDE;
    /* Place the badge center at battlefield row 12; no cargo pixels may
     * leak into the cached top HUD even in builds without scene clipping. */
    const Sc64SpriteFrame *s = sc64_race_unit_sprite(64);
    assert(s);
    q.y = 12 - 12 + s->canvas_h / 2 - s->offset_y - s->h / 2;
    render_unit(&q);
    int sx = q.x - s->canvas_w / 2 + s->offset_x;
    int sy = q.y + 12 - s->canvas_h / 2 + s->offset_y;
    int bx = sx + s->w + 2;
    int by = sy + s->h / 2;
    assert(by == 12);
    assert(gfx_test_pixel(bx, by - 3) != C_CYAN);
    assert(gfx_test_pixel(bx - 2, by - 1) != C_GREEN);
}

int main(void)
{
    gba_init(); game_init();
    race_mode = 1; race_human = R_PROTOSS;
    race_selected_mission = 0;
    init_state();
    check_worker(U_SCV, 7, ORDER_MINE, 0);
    check_worker(U_DRONE, 41, ORDER_GAS, 1);
    check_worker(U_PROBE, 64, ORDER_MINE, 0);
    check_worker(U_PROBE, -1, ORDER_GAS, 1); /* generic worker fallback */
    check_fog_and_map_clip();
    puts("PASS authored worker cargo badges, clearing, fog, and HUD clipping");
    return 0;
}
