/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_UNIT_FACING_H
#define SC64_UNIT_FACING_H

/* Presentation-only unit facing for imported sprites.
 *
 * StarCraft unit art stores 17 frames per pose, from north clockwise to
 * south; the other 15 of its 32 directions are mirrored. Units here used
 * frame 0 only, so everything faced north. This keeps a 32-direction facing
 * per actor slot: it follows the direction of travel, measured over at least
 * 8 pixels so the 8-way movement steps do not jitter, and turns toward the
 * target when the unit fires. Nothing here is read by the simulation or saved;
 * after a load, units face north until they move or fire. */

#include "sc64_unit_facing_generated.h"

#define SC64_FACING_VALID 0x80u
#define SC64_FACING_STEP 8
#define SC64_FACING_RESET 64

static u8 sc64_facing[MAX_UNITS];
static u8 sc64_facing_ax[MAX_UNITS], sc64_facing_ay[MAX_UNITS];

/* Unit vectors for the 32 StarCraft directions, scaled by 256; y grows down. */
static const s16 sc64_facing_vector[32][2] = {
    {0,-256},{50,-251},{98,-237},{142,-213},{181,-181},{213,-142},{237,-98},{251,-50},
    {256,0},{251,50},{237,98},{213,142},{181,181},{142,213},{98,237},{50,251},
    {0,256},{-50,251},{-98,237},{-142,213},{-181,181},{-213,142},{-237,98},{-251,50},
    {-256,0},{-251,-50},{-237,-98},{-213,-142},{-181,-181},{-142,-213},{-98,-237},{-50,-251}
};

static int
sc64_facing_direction(int dx, int dy)
{
    int best = 0, best_dot = -0x7fffffff;
    for (int d = 0; d < 32; ++d) {
        int dot = dx * sc64_facing_vector[d][0] + dy * sc64_facing_vector[d][1];
        if (dot > best_dot) { best_dot = dot; best = d; }
    }
    return best;
}

/* Called when a unit fires: face the target. */
static void __attribute__((noinline))
sc64_facing_fire(const Unit *q, int tx, int ty)
{
    int slot = (int)(q - u);
    if (slot < 0 || slot >= MAX_UNITS || (tx == q->x && ty == q->y)) return;
    sc64_facing[slot] = (u8)((sc64_facing[slot] & SC64_FACING_VALID) |
                             sc64_facing_direction(tx - q->x, ty - q->y));
}

/* Called before drawing a visible unit: follow its travel direction. */
static void
sc64_facing_track(const Unit *q)
{
    int slot = (int)(q - u), dx, dy, ax, ay;
    u8 f;
    if (slot < 0 || slot >= MAX_UNITS) return;
    f = sc64_facing[slot];
    if (!(f & SC64_FACING_VALID)) {
        sc64_facing_ax[slot] = (u8)q->x;
        sc64_facing_ay[slot] = (u8)q->y;
        sc64_facing[slot] = (u8)(f | SC64_FACING_VALID);
        return;
    }
    /* Anchors keep the low byte; small wrapped differences are exact. */
    dx = (s8)(u8)(q->x - sc64_facing_ax[slot]);
    dy = (s8)(u8)(q->y - sc64_facing_ay[slot]);
    ax = dx < 0 ? -dx : dx;
    ay = dy < 0 ? -dy : dy;
    if (ax > SC64_FACING_RESET || ay > SC64_FACING_RESET) {
        /* Teleport, unload, slot reuse or a long off-screen trip. */
        sc64_facing_ax[slot] = (u8)q->x;
        sc64_facing_ay[slot] = (u8)q->y;
        return;
    }
    if (ax < SC64_FACING_STEP && ay < SC64_FACING_STEP) return;
    sc64_facing[slot] = (u8)(SC64_FACING_VALID | sc64_facing_direction(dx, dy));
    sc64_facing_ax[slot] = (u8)q->x;
    sc64_facing_ay[slot] = (u8)q->y;
}

static int
sc64_facing_blit(unsigned raw, int dir, int x, int y)
{
    const Sc64SpriteFrame *frames = sc64_unit_facing_frames((unsigned short)raw), *s;
    if (!frames) return 0;
    if (dir <= 16) {
        s = &frames[dir];
        gfx_blit_sprite(x - s->canvas_w / 2 + s->offset_x, y - s->canvas_h / 2 + s->offset_y,
                        s->w, s->h, s->pixels);
    } else {
        s = &frames[32 - dir];
        gfx_blit_sprite_hflip(x + s->canvas_w / 2 - s->offset_x - s->w,
                              y - s->canvas_h / 2 + s->offset_y, s->w, s->h, s->pixels);
    }
    return 1;
}

/* Draw a unit with its facing frame. Siege Tanks draw their hull and turret
 * subunits (raw 5/6 in tank mode, 30/31 when sieged); the sieged hull has a
 * single frame. Returns 0 when no facing art exists for this unit. */
static int
sc64_facing_draw(const Unit *q, int raw, int x, int y)
{
    int dir;
    if (raw < 0) return 0;
    dir = sc64_facing[(int)(q - u)] & 31;
#ifdef SC64_RACES
    if (q->type == U_SIEGE_TANK || q->type == U_TANK) {
        int sieged = q->type == U_SIEGE_TANK;
        if (!sc64_facing_blit(sieged ? 30u : 5u, dir, x, y)) return 0;
        (void)sc64_facing_blit(sieged ? 31u : 6u, dir, x, y);
        return 1;
    }
#endif
    return sc64_facing_blit((unsigned)raw, dir, x, y);
}

#define SC64_FACING_FIRE(q, target) sc64_facing_fire((q), target_x(target), target_y(target))
#endif
