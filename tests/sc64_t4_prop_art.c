/* Source-backed static prop endpoint renderer fixtures. These tests compare
 * against the private generated frame table; they are not source-gameplay
 * or animation fidelity claims. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"
#include "../starcraft/sc64_t4_prop_art.h"

static u8 expected[240 * 160] __attribute__((aligned(4)));
static u8 actual[240 * 160] __attribute__((aligned(4)));

static unsigned row_for_raw(unsigned raw)
{
    for (unsigned row = 0; row < sc64_t4_scenario.doodad_count; ++row)
        if (!(sc64_t4_scenario.doodads[row].flags & SC64_T4_THG2_SPRITE_FLAG) &&
            sc64_t4_scenario.doodads[row].type == raw) return row;
    assert(!"T4 scenario has no matching doodad row");
    return 32;
}

static void reset_prop(unsigned raw, unsigned slot, int enabled)
{
    unsigned row = row_for_raw(raw);
    game_init();
    memset(u, 0, sizeof(u));
    memset(b, 0, sizeof(b));
    memset(&race_t4, 0, sizeof(race_t4));
    race_mode = 0;
    race_selected_mission = SC64_T4_GLOBAL_ID;
    race_cols = race_rows = 128;
    assert(slot < MAX_BUILDINGS && row < 32);
    b[slot].alive = 1;
    b[slot].raw_type = (int)raw;
    b[slot].type = sc64_race_building_from_raw(raw);
    b[slot].x = 160;
    b[slot].y = 120;
    race_t4.doodad_slot[row] = (u8)slot;
    race_t4.doodad_enabled[row] = (u8)enabled;
}

static void capture(u8 *out)
{
    assert(gfx_read_band(0, 160, out));
}

static void compare_frames(void)
{
    assert(memcmp(expected, actual, sizeof(actual)) == 0);
}

static void draw_expected(const Sc64SpriteFrame *sprite, int x, int y)
{
    draw_mission_sprite(sprite, x, y);
}

/* Independent oracle: render the ordinary source frame, then reflect the
 * captured canvas in the framebuffer. This does not repeat the prop
 * renderer's source-pixel placement calculation. */
static void reflect_expected_canvas(const Sc64SpriteFrame *sprite, int x, int y)
{
    int left = x - sprite->canvas_w / 2;
    int top = y - sprite->canvas_h / 2;
    for (unsigned row = 0; row < sprite->canvas_h; ++row) {
        int screen_y = top + (int)row;
        if (screen_y < 0 || screen_y >= 160) continue;
        for (unsigned col = 0; col < sprite->canvas_w / 2; ++col) {
            int left_x = left + (int)col;
            int right_x = left + (int)(sprite->canvas_w - 1 - col);
            if (left_x < 0 || left_x >= 240 || right_x < 0 || right_x >= 240)
                continue;
            u8 *line = expected + screen_y * 240;
            u8 tmp = line[left_x];
            line[left_x] = line[right_x];
            line[right_x] = tmp;
        }
    }
}

static void test_door_state_endpoints(unsigned raw, int enabled, int mirror)
{
    unsigned frame = enabled ? 0 : 9;
    const Sc64SpriteFrame *sprite;
    reset_prop(raw, 17, enabled);
    sprite = sc64_race_building_sprite((unsigned short)raw,
                                        (unsigned short)frame);
    assert(sprite && sprite->pixels);

    gfx_begin(C_BLACK);
    draw_expected(sprite, b[17].x, b[17].y + 12);
    capture(expected);
    if (mirror) reflect_expected_canvas(sprite, b[17].x, b[17].y + 12);

    gfx_begin(C_BLACK);
    assert(race_t4_draw_prop(&b[17], b[17].x, b[17].y + 12) == 1);
    capture(actual);
    compare_frames();
}

static void test_trap_state(void)
{
    const Sc64SpriteFrame *sprite;
    unsigned row = row_for_raw(203);
    reset_prop(203, 9, 0);
    gfx_begin(C_BLACK);
    assert(race_t4_draw_prop(&b[9], b[9].x, b[9].y + 12) == 1);
    capture(actual);
    for (unsigned i = 0; i < sizeof(actual); ++i) assert(actual[i] == 0);

    race_t4.doodad_enabled[row] = 1;
    sprite = sc64_race_building_sprite(203, 0);
    assert(sprite);
    gfx_begin(C_BLACK);
    draw_expected(sprite, b[9].x, b[9].y + 12);
    capture(expected);
    gfx_begin(C_BLACK);
    assert(race_t4_draw_prop(&b[9], b[9].x, b[9].y + 12) == 1);
    capture(actual);
    compare_frames();
}

static void test_only_bound_t4_props_are_handled(void)
{
    Building external;
    reset_prop(205, 3, 1);
    gfx_begin(C_BLACK);
    memset(&external, 0, sizeof(external));
    assert(race_t4_draw_prop(&external, 120, 80) == 0);
    b[3].raw_type = 106;
    assert(race_t4_draw_prop(&b[3], 120, 80) == 0);
    assert(race_t4_draw_prop(&b[4], 120, 80) == 0);
    capture(actual);
    for (unsigned i = 0; i < sizeof(actual); ++i) assert(actual[i] == 0);

    b[3].raw_type = 205;
    race_mode = 1;
    assert(race_t4_draw_prop(&b[3], 120, 80) == 0);
}

int main(void)
{
    test_door_state_endpoints(205, 1, 0);
    test_door_state_endpoints(205, 0, 0);
    test_door_state_endpoints(206, 1, 1);
    test_door_state_endpoints(206, 0, 1);
    test_door_state_endpoints(207, 1, 0);
    test_door_state_endpoints(208, 0, 1);
    test_trap_state();
    test_only_bound_t4_props_are_handled();
    puts("T4 prop art policy fixtures passed");
    return 0;
}
