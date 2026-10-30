/* Authenticated N64 damage-overlay frames, with an opaque GBA palette ramp.
 * SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_BUILDING_FIRE_VFX_H
#define SC64_BUILDING_FIRE_VFX_H
#include <stdint.h>
#include "../common/gba.h"

typedef struct {
    uint8_t x, y, w, h, canvas_w, canvas_h;
    const unsigned char *pixels;
} Sc64BuildingFireFrame;

typedef struct { int8_t x, y; uint8_t slot; } Sc64BuildingFireAnchor;
typedef struct { uint8_t raw; uint16_t first; uint8_t count; } Sc64BuildingFireActor;

/* Calculate source damage stages from integer HP. Source HP is 8-bit fixed
 * point; missing fractional HP and deterministic slot order are approximations.
 * A zero return means the supplied metadata cannot safely be evaluated. */
static inline int sc64_building_damage_stages(unsigned hp, unsigned max_hp,
                                                unsigned count, unsigned *active,
                                                unsigned *large_count)
{
    unsigned states, state, two_thirds, per_state;
    if (!active || !large_count || !count || count > 22u || !max_hp || max_hp > 0x7fffffu)
        return 0;
    *active = 0; *large_count = 0;
    if (hp >= max_hp) return 1;
    max_hp *= 256u; hp *= 256u;
    states = 2u * count;
    two_thirds = max_hp - max_hp / 3u;
    per_state = two_thirds / (states + 1u);
    if (!per_state) per_state = 1;
    {
        int32_t numerator = (int32_t)hp - (int32_t)(two_thirds % per_state)
                            - (int32_t)(max_hp / 3u) - 1;
        state = numerator <= 0 ? 0u : (unsigned)numerator / per_state;
    }
    if (state > states) state = states;
    *active = states - state;
    *large_count = *active > count ? *active - count : 0u;
    if (*active > count) *active = count;
    return 1;
}

#ifdef SC64_BUILDING_FIRE_ART
#ifndef SC64_BUILDING_FIRE_BANK_HEADER
#define SC64_BUILDING_FIRE_BANK_HEADER "sc64_building_fire_generated.h"
#endif
#include SC64_BUILDING_FIRE_BANK_HEADER
/* LO placement is source frame 0. Slot allocation is deterministic rather
 * than tracking the source engine's random and repair history. Hit points
 * supplied here are integer values, so fractional source HP is approximated.
 * Source LO coordinates use the 32-pixel grid; this engine projects those
 * offsets onto its 16-pixel world grid with division toward zero. */
static inline int sc64_building_damage_draw(unsigned raw, int x, int y,
                                             unsigned hp, unsigned max_hp,
                                             uint32_t elapsed)
{
    const Sc64BuildingFireActor *a = 0;
    unsigned i, active, large_count;
    for (i = 0; i < SC64_BUILDING_FIRE_ACTOR_COUNT; ++i)
        if (sc64_building_fire_actors[i].raw == raw) { a = &sc64_building_fire_actors[i]; break; }
    if (!a || !sc64_building_damage_stages(hp, max_hp, a->count, &active, &large_count))
        return 0;
    for (i = 0; i < active; ++i) {
        const Sc64BuildingFireAnchor *p = &sc64_building_fire_anchors[a->first + i];
        unsigned slot = p->slot, big = i < large_count, bank, frame_count = 12u;
        int flip = 0;
        if (slot < 8u) bank = (big ? 3u : 0u) + (slot < 2u ? slot : 2u);
        else if (slot < 16u) {
            bank = (big ? 16u : 12u) + (slot - 8u) % 4u;
            frame_count = 6u;
            flip = slot >= 12u;
        } else bank = (big ? 9u : 6u) + (slot - 16u) % 3u;
        const Sc64BuildingFireFrame *f = &sc64_building_fire_banks[bank][(elapsed / 2u) % frame_count];
        int anchor_x = x + p->x / 2, anchor_y = y + p->y / 2;
        int fx = anchor_x - f->canvas_w / 2 + f->x;
        int fy = anchor_y - f->canvas_h / 2 + f->y;
        if (flip) {
            fx = anchor_x + f->canvas_w / 2 - f->x - f->w;
#ifdef SC64_UNIT_FACING
            gfx_blit_sprite_hflip(fx, fy, f->w, f->h, f->pixels);
#else
            for (unsigned row = 0; row < f->h; ++row)
                for (unsigned col = 0; col < f->w; ++col) {
                    u8 color = f->pixels[row * f->w + f->w - 1u - col];
                    if (color) gfx_pixel(fx + (int)col, fy + (int)row, color);
                }
#endif
        } else gfx_blit_sprite(fx, fy, f->w, f->h, f->pixels);
    }
    return 1;
}

#endif
#endif
