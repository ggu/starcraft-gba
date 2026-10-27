/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_UNIT_WALK_H
#define SC64_UNIT_WALK_H

/* Presentation-only walk cycle for every unit in every profile.
 *
 * Neither the original-art shapes nor the imported single-pose sprite frames
 * carry a movement cycle, so a moving unit used to glide. This keeps two
 * bytes per actor slot: a position key, and a packed counter of observed
 * steps plus a short hold that bridges ticks in which a slow unit does not
 * advance. The stride phase advances with distance walked, so it stops when
 * the unit or the game stops. Nothing here is read by the simulation or
 * saved; after a load the first observation only primes the slot. */

/* Draws a moving unit stays in stride after its last observed step. */
#define SC64_WALK_HOLD 6
#define SC64_WALK_PRIMED 0x80
#define SC64_WALK_HOLD_MASK 0x07
#define SC64_WALK_STEP_SHIFT 3
#define SC64_WALK_STEP_MASK 0x0f

static u8 sc64_walk_key[MAX_UNITS];
static u8 sc64_walk_state[MAX_UNITS];

static void
sc64_walk_reset_all(void)
{
    memset(sc64_walk_state, 0, sizeof(sc64_walk_state));
}

static void
sc64_walk_reset_slot(int slot)
{
    if ((unsigned)slot < MAX_UNITS) sc64_walk_state[slot] = 0;
}

/* Called before drawing a visible unit. Returns 0 while the unit stands,
 * else stride phase 1..4: 1 and 3 plant a foot, 2 and 4 lift the body. */
static int __attribute__((noinline))
sc64_walk_track(const Unit *q)
{
    int slot = (int)(q - u);
    if ((unsigned)slot >= MAX_UNITS) return 0;
    /* Steps are at most a few pixels, so the low position bits change. */
    u8 key = (u8)((unsigned)q->x ^ ((unsigned)q->y << 4));
    unsigned state = sc64_walk_state[slot];
    unsigned hold = state & SC64_WALK_HOLD_MASK;
    unsigned steps = (state >> SC64_WALK_STEP_SHIFT) & SC64_WALK_STEP_MASK;
    if (!(state & SC64_WALK_PRIMED)) {
        hold = 0;
        steps = 0;
    } else if (key != sc64_walk_key[slot]) {
        hold = SC64_WALK_HOLD;
        steps = (steps + 1) & SC64_WALK_STEP_MASK;
    } else if (hold) {
        --hold;
    }
    sc64_walk_key[slot] = key;
    sc64_walk_state[slot] = (u8)(SC64_WALK_PRIMED |
        (steps << SC64_WALK_STEP_SHIFT) | hold);
    return hold ? (int)((steps >> 1) & 3) + 1 : 0;
}

/* Lift the body one pixel between footfalls. */
#define SC64_WALK_LIFT(phase) (!((phase) & 1) && (phase) ? 1 : 0)

/* Two legs under an upright body whose feet sit at row y. */
static void
sc64_walk_legs(int x, int y, int phase, int color)
{
    if (phase == 1) {
        gfx_pixel(x - 2, y, (u8)color);
        gfx_pixel(x - 3, y + 1, (u8)color);
        gfx_pixel(x + 1, y, (u8)color);
    } else if (phase == 3) {
        gfx_pixel(x + 1, y, (u8)color);
        gfx_pixel(x + 2, y + 1, (u8)color);
        gfx_pixel(x - 2, y, (u8)color);
    } else {
        gfx_pixel(x - 1, y, (u8)color);
        gfx_pixel(x + 1, y, (u8)color);
    }
}

/* Alternating pairs of legs under a low, wide body. */
static void
sc64_walk_scuttle(int x, int y, int half, int phase, int color)
{
    int lead = phase & 1;
    gfx_pixel(x - half + lead, y, (u8)color);
    gfx_pixel(x + half - 1 + lead, y, (u8)color);
    gfx_pixel(x - half / 2 - lead, y + lead, (u8)color);
    gfx_pixel(x + half / 2 + 1 - lead, y + lead, (u8)color);
}

/* Tread marks that roll under a worker. */
static void
sc64_walk_treads(int x, int y, int phase, int color)
{
    for (int i = -3 + (phase & 1); i <= 3; i += 2)
        gfx_pixel(x + i, y, (u8)color);
}

#endif
