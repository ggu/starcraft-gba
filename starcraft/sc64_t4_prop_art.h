#ifndef SC64_T4_PROP_ART_H
#define SC64_T4_PROP_ART_H

#ifdef SC64_T4_SCENARIO_RUNTIME
#include "sc64_t4_actions.h"
/* Draw the static art for a THG2-backed door or trap. The source's doodad
 * state controls door endpoints; this GBA policy uses frame 0 when enabled
 * and frame 9 when disabled. Door orientation follows the retained THG2
 * type pairing; native flip-bit interpretation is not claimed. */
static int
race_t4_draw_prop(Building *q, int x, int y)
{
    unsigned slot, row;
    unsigned raw;
    int door, trap, mirror;
    const Sc64SpriteFrame *sprite;

    if (!q || !race_t4_active()) return 0;
    for (slot = 0; slot < MAX_BUILDINGS; ++slot)
        if (q == &b[slot]) break;
    if (slot == MAX_BUILDINGS || !q->alive) return 0;
#ifdef SC64_RACES
    if (q->raw_type < 0) return 0;
    raw = (unsigned)q->raw_type;
#else
    return 0;
#endif
    door = raw == 205 || raw == 206 || raw == 207 || raw == 208;
    trap = raw == 203 || raw == 209 || raw == 211 || raw == 212;
    if (!door && !trap) return 0;

    for (row = 0; row < 32; ++row)
        if (race_t4.doodad_slot[row] == slot) break;
    if (row == 32 || row >= sc64_t4_scenario.doodad_count ||
        (sc64_t4_scenario.doodads[row].flags & SC64_T4_THG2_SPRITE_FLAG) ||
        sc64_t4_scenario.doodads[row].type != raw) return 0;

    if (trap && !race_t4.doodad_enabled[row]) return 1;
    sprite = sc64_race_building_sprite((unsigned short)raw,
                                        (unsigned short)(door &&
                                            !race_t4.doodad_enabled[row] ? 9 : 0));
    if (!sprite || !sprite->pixels || !sprite->w || !sprite->h) return 1;

    mirror = raw == 206 || raw == 208;
    if (!mirror) {
        draw_mission_sprite(sprite, x, y);
        return 1;
    }

    /* Reflect pixels and their frame offset around the original canvas. */
    {
        int left = x - sprite->canvas_w / 2 +
                   (int)sprite->canvas_w - (sprite->offset_x + sprite->w);
        int top = y - sprite->canvas_h / 2 + sprite->offset_y;
        for (unsigned sy = 0; sy < sprite->h; ++sy) {
            for (unsigned sx = 0; sx < sprite->w; ++sx) {
                u8 color = sprite->pixels[sy * sprite->w + sx];
                if (color) gfx_pixel(left + (int)(sprite->w - 1 - sx),
                                     top + (int)sy, color);
            }
        }
    }
    return 1;
}
#endif /* SC64_T4_SCENARIO_RUNTIME */

#endif
