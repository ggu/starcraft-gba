#ifndef SC64_P6_RENDER_ADAPTER_H
#define SC64_P6_RENDER_ADAPTER_H

#ifdef SC64_P6_RUNTIME
/* Keep static terrain caching sensitive to the current prop bindings and
 * enabled endpoints after source actions, restart and SRAM rebind. */
static unsigned race_p6_prop_cache_field(unsigned row, int enabled)
{
    if (row >= SC64_P6_DOODAD_SOURCE_COUNT) return 0;
    return enabled ? race_p6_doodad_bindings[row].enabled :
                     race_p6_doodad_bindings[row].building_slot;
}

#ifdef SC64_HAS_SPRITES
static int race_p6_draw_prop(Building *q, int x, int y)
{
    unsigned row;
    int door, mirror;
    const Sc64SpriteFrame *sprite;
    if (!race_p6_active() || !q || !q->alive || q->raw_type < 0) return 0;
    for (row = 0; row < SC64_P6_DOODAD_SOURCE_COUNT; ++row)
        if (!sc64_p6_doodads[row].sprite_only &&
            race_p6_doodad_bindings[row].building_slot == (unsigned)(q - b)) break;
    if (row == SC64_P6_DOODAD_SOURCE_COUNT ||
        sc64_p6_doodads[row].raw_type != (unsigned)q->raw_type) return 0;
    door = q->raw_type >= 205 && q->raw_type <= 208;
    if (!door && !race_p6_doodad_bindings[row].enabled) return 1;
    /* Use the same imported door endpoints as Z5. Intermediate native iscript
     * animation is outside this bounded renderer. Trap idle art is frame 0. */
    sprite = sc64_race_building_sprite((u16)q->raw_type,
        (u16)(door && !race_p6_doodad_bindings[row].enabled ? 9 : 0));
    if (!sprite || !sprite->pixels || !sprite->w || !sprite->h) return 1;
    mirror = q->raw_type == 206 || q->raw_type == 208;
    if (!mirror) draw_mission_sprite(sprite, x, y);
    else {
        int left = x - sprite->canvas_w / 2 + (int)sprite->canvas_w -
                   (sprite->offset_x + sprite->w);
        int top = y - sprite->canvas_h / 2 + sprite->offset_y;
        for (unsigned sy = 0; sy < sprite->h; ++sy)
            for (unsigned sx = 0; sx < sprite->w; ++sx) {
                u8 color = sprite->pixels[sy * sprite->w + sx];
                if (color) gfx_pixel(left + (int)(sprite->w - 1 - sx),
                                     top + (int)sy, color);
            }
    }
    return 1;
}
#endif
#endif
#endif
