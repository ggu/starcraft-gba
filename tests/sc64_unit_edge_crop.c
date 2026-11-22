/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../starcraft/game.c"

#if !defined(SC64_WORLD_DEPTH) || !defined(SC64_UNIT_FACING) || !defined(GBA_SCENE_CLIP)
#error "Use the normal clipped world renderer for the edge crop fixture"
#endif

enum { WIDTH = 240, HEIGHT = 160, TOP = 12, BOTTOM = 120 };
static u8 carrier[WIDTH * HEIGHT], wraith[WIDTH * HEIGHT];

static void
begin_field(void)
{
    gfx_begin(C_PANEL);
    gfx_set_clip_y(TOP, BOTTOM);
}

static unsigned
capture_isolated(unsigned raw, unsigned direction, int x, int y, u8 *frame_pixels)
{
    begin_field();
    assert(sc64_facing_blit(raw, direction, x, y));
    unsigned visible_pixels = 0;
    for (int row = 0; row < HEIGHT; ++row) {
        for (int col = 0; col < WIDTH; ++col) {
            u8 pixel = gfx_test_pixel(col, row);
            frame_pixels[row * WIDTH + col] = pixel;
            if (row < TOP || row >= BOTTOM) assert(pixel == C_PANEL);
            visible_pixels += pixel != C_PANEL;
        }
    }
    gfx_end();
    return visible_pixels;
}

static void
check_edge(int x, int y)
{
    const unsigned direction = 24; /* west, mirrored in the byte renderer */
    unsigned carrier_pixels = capture_isolated(72, 0, x, y, carrier);
    unsigned wraith_pixels = capture_isolated(8, direction, x, y, wraith);
    assert(carrier_pixels && wraith_pixels);
    unsigned overlap = 0;
    for (unsigned n = 0; n < sizeof(carrier); ++n)
        overlap += carrier[n] != C_PANEL && wraith[n] != C_PANEL && carrier[n] != wraith[n];
    assert(overlap);

    for (int carrier_slot = 0; carrier_slot < 2; ++carrier_slot) {
        memset(u, 0, sizeof(u));
        for (int slot = 0; slot < 2; ++slot) {
            int type = slot == carrier_slot ? U_P7_CARRIER : U_WRAITH;
            assert(spawn_unit(SIDE, type, camx + x, camy + y - TOP) == slot);
        }
        Unit *ship = &u[carrier_slot], *fighter = &u[1 - carrier_slot];
        assert(sc64_unit_world_layer(ship) < sc64_unit_world_layer(fighter));
        fighter->order = ORDER_MOVE;
        fighter->tx = fighter->x - 32;
        fighter->ty = fighter->y;
        assert(sc64_unit_body_anchor_in_view(ship, x, y));
        assert(sc64_unit_body_anchor_in_view(fighter, x, y));
        assert((sc64_facing[1 - carrier_slot] & SC64_FACING_MASK) == 0);

        unsigned pages = 0;
        for (int page = 0; page < 2; ++page) {
            begin_field();
            pages |= 1u << gfx_backbuffer_index();
            sc64_draw_depth_ordered_actors();
            for (int row = 0; row < HEIGHT; ++row) {
                for (int col = 0; col < WIDTH; ++col) {
                    int n = row * WIDTH + col;
                    u8 expected = wraith[n] != C_PANEL ? wraith[n] : carrier[n];
                    assert(gfx_test_pixel(col, row) == expected);
                }
            }
            gfx_end();
        }
        assert(pages == 3);
        fighter->owner = ENEMY;
        assert(!currently_visible(fighter->x, fighter->y));
        begin_field();
        sc64_draw_depth_ordered_actors();
        for (int row = 0; row < HEIGHT; ++row)
            for (int col = 0; col < WIDTH; ++col)
                assert(gfx_test_pixel(col, row) == carrier[row * WIDTH + col]);
        gfx_end();
    }
    printf("Unit crops at (%d,%d): Carrier %u, Wraith %u, foreground overlap %u\n",
           x, y, carrier_pixels, wraith_pixels, overlap);
}

int
main(void)
{
    game_init();
    race_mode = 1;
    race_human = R_PROTOSS;
    race_bots = 1;
    race_skirmish_map = 0;
    race_skirmish_init();
    memset(b, 0, sizeof(b));
    memset(m, 0, sizeof(m));
    memset(visible, 0, sizeof(visible));
    geyser_count = 0;
    camx = camy = 256;
    frame = 0;
    check_edge(-9, 66);
    check_edge(248, 66);
    check_edge(120, 9);
    check_edge(120, 126);
    return 0;
}
