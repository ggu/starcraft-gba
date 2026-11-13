/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_VISIBLE_MINERAL_CURSOR_H
#define SC64_VISIBLE_MINERAL_CURSOR_H

/* Try eight nearby fields in distance/slot order. A friendly selection
 * radius can cover a whole patch, so keep going when its rim is blocked. */
static int
order_selected_worker_to_visible_minerals(int worker, void (*move)(int, int),
                                          void (*press)(u16))
{
    static const signed char offsets[][2] = {
        {0,0}, {8,0}, {-8,0}, {0,8}, {0,-8},
        {8,8}, {-8,8}, {8,-8}, {-8,-8},
        {15,0}, {-15,0}, {0,15}, {0,-15},
        {13,7}, {-13,7}, {13,-7}, {-13,-7},
        {7,13}, {-7,13}, {7,-13}, {-7,-13},
        {11,11}, {-11,11}, {11,-11}, {-11,-11}
    };
    if (worker < 0 || worker >= MAX_UNITS || !u[worker].alive ||
        u[worker].owner != SIDE || !IS_WORKER(u[worker].type) || !move || !press) return -1;
    int x = u[worker].x, y = u[worker].y, last_distance = -1, last_patch = -1;
    for (int attempt = 0; attempt < 8; ++attempt) {
        int patch = -1, distance = 0x7fffffff;
        for (int p = 0; p < MAX_MINERALS; ++p) {
            if (m[p].amount <= 0 || !currently_visible(m[p].x, m[p].y)) continue;
            int d = dist2(x, y, m[p].x, m[p].y);
            if (d < last_distance || (d == last_distance && p <= last_patch)) continue;
            if (d < distance) { patch = p; distance = d; }
        }
        if (patch < 0) break;
        last_distance = distance;
        last_patch = patch;
        for (unsigned k = 0; k < sizeof(offsets) / sizeof(offsets[0]); ++k) {
            if (screen != SCREEN_PLAY || !u[worker].alive || !u[worker].selected ||
                selected_count() != 1) return -1;
            move(m[patch].x + offsets[k][0], m[patch].y + offsets[k][1]);
            if (screen != SCREEN_PLAY || !u[worker].alive || !u[worker].selected ||
                selected_count() != 1) return -1;
            if (selected_worker_mineral_target() != patch) continue;
#ifdef SC64_FIELD_CONTROLS
            if (field_ui_a_action(1) != UI_A_MINE) continue;
#endif
            press(KEY_A);
            if (u[worker].alive && u[worker].order == ORDER_MINE &&
                u[worker].target == patch) return patch;
        }
    }
    return -1;
}
#endif
