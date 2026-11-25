/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Standalone observation and save lifecycle, using real sight and painters. */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/game.c"

static void move_scout(int x, int y)
{
    u[0].x = x; u[0].y = y;
    update_exploration();
}

static void check_pixels(int world_color, int minimap_color)
{
    for (int page = 0; page < 2; ++page) {
        gfx_begin(C_BLACK);
        draw_map_mineral(0);
        assert(gfx_test_pixel(80, 92) == world_color);
        draw_minimap();
        assert(gfx_test_pixel(195, 136) == minimap_color);
        gfx_end();
    }
}

int main(void)
{
    game_init();
    init_state();
    memset(u, 0, sizeof(u)); memset(b, 0, sizeof(b));
    memset(m, 0, sizeof(m));
    memset(sc64_mineral_memory, 0, sizeof(sc64_mineral_memory));
    SC64_GRID_CLEAR(explored); SC64_GRID_CLEAR(visible);
    camx = camy = 0; supply_cap = 20;
    m[0].x = 80; m[0].y = 80; m[0].amount = 100;
    u[0].alive = 1; u[0].owner = SIDE; u[0].type = U_SCV;
    u[0].hp = 40; u[0].order = ORDER_IDLE;
    move_scout(200, 80);
    SC64_GRID_SET(explored, 5 + 5 * MAP_COLS, 1);
    update_exploration();
    assert(!sc64_mineral_memory_present(0));
    assert(sc64_mineral_draw_state(0) == SC64_MINERAL_HIDDEN);
    check_pixels(C_BLACK, C_FOG);
    move_scout(60, 80);
    assert(sc64_mineral_memory_present(0));
    assert(sc64_mineral_draw_state(0) == SC64_MINERAL_LIVE);
    check_pixels(C_WHITE, C_CYAN);
    move_scout(200, 80);
    m[0].amount = 0;
    update_exploration();
    assert(sc64_mineral_memory_present(0));
    assert(sc64_mineral_draw_state(0) == SC64_MINERAL_REMEMBERED);
    check_pixels(C_BLUE, C_CYAN);

    /* Save and restore while depletion remains hidden. */
    assert(save_game());
#ifdef SC64_MINERAL_FIFO
    assert(mineral_fifo_enabled);
    assert(save_read(&data, sizeof(data), SC64_MINERAL_FIFO_SAVE_VERSION));
#else
    assert(save_read(&data, sizeof(data), SAVE_VERSION));
#endif
    assert(((unsigned int)data.selected_building & 0xff000040u) ==
           (SC64_STANDALONE_MEMORY_MARKER | 0x40u));
    memset(sc64_mineral_memory, 0, sizeof(sc64_mineral_memory));
    assert(load_game());
    assert(sc64_mineral_memory_present(0));
    assert(sc64_mineral_draw_state(0) == SC64_MINERAL_REMEMBERED);
    move_scout(60, 80);
    assert(!sc64_mineral_memory_present(0));
    assert(sc64_mineral_draw_state(0) == SC64_MINERAL_HIDDEN);
    check_pixels(C_BLACK, C_GRASS);
    m[0].amount = 50; update_exploration();
    assert(sc64_mineral_memory_present(0));
    m[0].amount = 0; update_exploration();
    assert(!sc64_mineral_memory_present(0));

    /* Legacy payload has no memory: only current sight can repopulate it. */
    m[0].amount = 100;
    update_exploration();
    move_scout(200, 80);
    assert(sc64_mineral_draw_state(0) == SC64_MINERAL_REMEMBERED);
    assert(save_game());
#ifdef SC64_MINERAL_FIFO
    GbaSaveSelection previous;
    assert(worker_mineral_fifo_select(&previous));
    assert(previous.tag == SC64_MINERAL_FIFO_SAVE_VERSION);
    data.version = SAVE_VERSION;
    data.selected_building = -1;
    assert(save_write_selected(&data, sizeof(data), SAVE_VERSION,
                               &previous, mineral_fifo_save_tags, 2));
#else
    data.selected_building = -1;
    save_write(&data, sizeof(data), SAVE_VERSION);
#endif
    memset(sc64_mineral_memory, 0xff, sizeof(sc64_mineral_memory));
    assert(load_game());
#ifdef SC64_MINERAL_FIFO
    assert(!mineral_fifo_enabled);
#endif
    assert(!sc64_mineral_memory_present(0));
    move_scout(60, 80);
    assert(sc64_mineral_memory_present(0));
    init_state();
    /* Fresh initialization may immediately observe nearby starting patches,
     * but the former fixture's hidden knowledge must not survive. */
    assert(!sc64_mineral_memory_present(0) || currently_visible(m[0].x, m[0].y));
    puts("mineral memory game: ok");
    return 0;
}
