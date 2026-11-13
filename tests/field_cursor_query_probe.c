/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/game.c"

static u32 state = 0x1132aabb, digest = 2166136261u;
static unsigned pick(unsigned max)
{
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state % max;
}

static void record(int value)
{
    digest = (digest ^ (u32)value) * 16777619u;
}

int main(void)
{
    game_init();
#ifndef SC64_RACES
    init_state();
#endif
    for (int iteration = 0; iteration < 1200; ++iteration) {
        cx = 128 + pick(128);
        cy = 128 + pick(128);
        frame = iteration + 100;
        mode = MODE_NORMAL;
        unit_tap = pick(3) == 0 ? 0 : -1;
        unit_tap_frame = frame - pick(48);
        unit_tap_x = cx + (int)pick(9) - 4;
        unit_tap_y = cy + (int)pick(9) - 4;
        selected_building = pick(4) == 0 ? 0 : -1;
        for (int i = 0; i < MAX_UNITS; ++i) {
            Unit *q = &u[i];
            q->alive = pick(7) != 0;
            q->owner = pick(3);
            q->selected = pick(5) == 0;
            q->type = pick(2) ? U_SCV : U_MARINE;
            q->order = ORDER_IDLE;
            q->x = cx + (int)pick(161) - 80;
            q->y = cy + (int)pick(161) - 80;
        }
        for (int i = 0; i < MAX_BUILDINGS; ++i) {
            Building *q = &b[i];
            q->alive = pick(5) != 0;
            q->owner = pick(3);
            q->type = pick(2) ? B_COMMAND : B_REFINERY;
            q->under_construction = pick(2);
            q->x = cx + (int)pick(161) - 80;
            q->y = cy + (int)pick(161) - 80;
        }
        int x = -1, y = -1;
        /* Leave native visibility untouched on one phase; set it on the
         * other to exercise first-slot hostile priority near the cursor. */
        memset(visible, iteration & 1 ? 255 : 0, sizeof visible);
        record(selection_target());
        record(ui_hostile_at(cx, cy, &x, &y));
        record(x);
        record(y);
        record(selected_worker_mineral_target());
        field_ui_refresh();
        record(ui_action);
#ifndef SC64_RACES
        if (iteration % 20 == 0) {
            game_render();
            for (int py = 0; py < 160; ++py)
                for (int px = 0; px < 240; ++px)
                    record(gfx_test_pixel(px, py));
        }
#endif
    }
    printf("%08x\n", digest);
}
