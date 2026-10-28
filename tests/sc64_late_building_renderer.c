/* SPDX-License-Identifier: GPL-3.0-or-later
 * Source-pixel coverage for late Terran buildings in the completed renderer. */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../starcraft/game.c"

static u8 actual[240 * 160];
static u8 expected[240 * 160];

typedef struct {
    int type;
    unsigned short raw;
} LateBuildingCase;

static const LateBuildingCase cases[] = {
    {B_NUCLEAR_SILO, 108},
    {B_COVERT_OPS, 117},
    {B_MACHINE_SHOP, 120},
    {B_ARMORY, 123},
    {B_SPECIAL_ION_CANNON, 127},
    {B_SCIENCE_FACILITY, 116},
    {B_PHYSICS_LAB, 118},
    {B_ROBOTICS_SUPPORT_BAY, 171},
    {B_SHIELD_BATTERY, 172},
};

static const unsigned short t4_prop_raw_types[] = {
    203, 205, 206, 207, 208, 209, 211, 212,
};

static void
assert_opaque_frame(const Sc64SpriteFrame *sprite, const char *identity,
                    int raw)
{
    assert(sprite && sprite->pixels && sprite->w && sprite->h);
    unsigned opaque = 0;
    for (unsigned i = 0; i < (unsigned)sprite->w * sprite->h; ++i)
        opaque += sprite->pixels[i] != 0;
    if (!opaque)
        fprintf(stderr, "%s raw %d has no opaque source pixels\n", identity, raw);
    assert(opaque != 0);
}

static int
is_t4_prop(unsigned short raw)
{
    for (unsigned i = 0;
         i < sizeof(t4_prop_raw_types) / sizeof(t4_prop_raw_types[0]); ++i)
        if (t4_prop_raw_types[i] == raw) return 1;
    return 0;
}

static void
test_full_terran_roster(void)
{
    unsigned visible_units = 0, regular_buildings = 0, props = 0;

    for (int type = 0; type < SC_RU_COUNT; ++type) {
        int raw = sc64_race_raw_unit(type);
        if (raw < 0) continue;
        if (raw == 101) {
            assert(sc64_race_unit_sprite_mode((unsigned short)raw) == 2);
            assert(sc64_race_unit_sprite((unsigned short)raw) == 0);
            continue;
        }
        assert_opaque_frame(sc64_race_unit_sprite((unsigned short)raw),
                            sc64_race_unit_name(type), raw);
        ++visible_units;
    }

    for (int type = 0; type < SC_RB_COUNT; ++type) {
        int raw = sc64_race_raw_building(type);
        if (raw < 0) continue;
        if (is_t4_prop((unsigned short)raw)) {
            assert(sc64_race_building_art_sprite(raw) == 0);
            assert_opaque_frame(sc64_race_building_sprite((unsigned short)raw, 0),
                                sc64_race_building_name(type), raw);
            ++props;
            continue;
        }
        assert_opaque_frame(sc64_race_building_art_sprite(raw),
                            sc64_race_building_name(type), raw);
        ++regular_buildings;
    }

    assert(visible_units == 43);
    assert(regular_buildings == 53);
    assert(props == sizeof(t4_prop_raw_types) / sizeof(t4_prop_raw_types[0]));
    printf("full Terran roster: %u visible units, %u regular buildings, "
           "%u T4 props; raw101 intentionally invisible\n",
           visible_units, regular_buildings, props);
}

static void
capture(u8 *pixels)
{
    for (int y = 0; y < 160; ++y)
        for (int x = 0; x < 240; ++x)
            pixels[y * 240 + x] = gfx_test_pixel(x, y);
}

static void
composite_source_sprite(u8 *pixels, const Sc64SpriteFrame *sprite,
                        int center_x, int center_y)
{
    int left = center_x - sprite->canvas_w / 2 + sprite->offset_x;
    int top = center_y - sprite->canvas_h / 2 + sprite->offset_y;
    for (int sy = 0; sy < sprite->h; ++sy)
        for (int sx = 0; sx < sprite->w; ++sx) {
            u8 color = sprite->pixels[sy * sprite->w + sx];
            int x = left + sx, y = top + sy;
            if (color && x >= 0 && x < 240 && y >= 0 && y < 160)
                pixels[y * 240 + x] = color;
        }
}

static void
test_late_building(const LateBuildingCase *test)
{
    const Sc64SpriteFrame *source =
        sc64_race_building_sprite(test->raw, 0);
    assert(source && source->pixels && source->w && source->h);
    assert(sc64_race_building_art_sprite(test->raw) == source);
    assert(sc64_race_raw_building(test->type) == test->raw);
    assert(mission_building_sprite(test->type) == source);

    Building building;
    memset(&building, 0, sizeof(building));
    building.x = 120;
    building.y = 80;
    building.owner = SIDE;
    building.type = test->type;
    building.raw_type = (short)test->raw;
    building.alive = 1;
    building.maxhp = 1000;
    building.hp = 1000;
    building.shield = 0;

    camx = 0;
    camy = 0;
    gfx_begin(C_BLACK);
    draw_building_art(&building);
    capture(actual);

    gfx_begin(C_BLACK);
    memset(expected, C_BLACK, sizeof(expected));
    composite_source_sprite(expected, source, building.x, building.y + 12);
    assert(memcmp(actual, expected, sizeof(actual)) == 0);

    /* Only a selected building shows vitals, below its source frame. */
    int screen_x = building.x - camx;
    int screen_y = building.y - camy + 12;
    int sprite_y = screen_y - source->canvas_h / 2 + source->offset_y;
    gfx_begin(C_BLACK);
    draw_building_overlay(&building);
    capture(actual);
    memset(expected, C_BLACK, sizeof(expected));
    assert(memcmp(actual, expected, sizeof(actual)) == 0);

    b[0] = building;
    selected_building = 0;
    gfx_begin(C_BLACK);
    draw_building_overlay(&b[0]);
    capture(actual);
    selected_building = -1;

    gfx_begin(C_BLACK);
    draw_actor_vitals(screen_x, sprite_y + source->h + 3,
                      source->w > 46 ? 46 : source->w, building.hp,
                      building.maxhp, building.shield,
                      sc64_actor_max_shields(building.type, 1), 0, 0);
    capture(expected);
    assert(memcmp(actual, expected, sizeof(actual)) == 0);
}

int
main(void)
{
    test_full_terran_roster();
    assert(sc64_race_building_art_sprite(106) == sc64_race_unit_sprite(106));
    for (unsigned i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i)
        test_late_building(&cases[i]);
    puts("late Terran buildings match source frames and vital alignment");
    return 0;
}
