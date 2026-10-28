/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_UNIT_WALK_H
#define SC64_UNIT_WALK_H

/* Presentation-only walk cycle for every unit in every profile.
 *
 * Neither the original-art shapes nor the imported single-pose sprite frames
 * carry a movement cycle, so a moving unit used to glide. A ground unit's
 * path timer counts down once on every tick it is moved, and freezes when
 * nothing moves it, so it doubles as a step counter. Three bits per actor
 * slot remember its low bits and bridge one tick, which costs no RAM where
 * the facing byte already exists (facing uses only its low five bits).
 * Flyers keep no state: they are moving while an order carries them toward
 * a different point. Nothing here is read by the simulation or saved; after
 * a load a unit may hold one stride frame. */

#define SC64_WALK_KEY_SHIFT 5
#define SC64_WALK_KEY_MASK 0x60
#define SC64_WALK_HOLD 0x80

#ifdef SC64_UNIT_FACING
static u8 sc64_facing[MAX_UNITS];
#define SC64_WALK_BITS sc64_facing
#else
static u8 sc64_walk_bits[MAX_UNITS];
#define SC64_WALK_BITS sc64_walk_bits
#endif

static void
sc64_walk_reset_slot(int slot)
{
    if ((unsigned)slot < MAX_UNITS)
        SC64_WALK_BITS[slot] &= (u8)~(SC64_WALK_KEY_MASK | SC64_WALK_HOLD);
}

static void
sc64_walk_reset_all(void)
{
    for (int i = 0; i < MAX_UNITS; ++i) sc64_walk_reset_slot(i);
}

/* Called before drawing a visible unit. Returns 0 while the unit stands,
 * else stride phase 1..4: 1 and 3 plant a foot, 2 and 4 lift the body. */
static int __attribute__((noinline))
sc64_walk_track(const Unit *q)
{
    int slot = (int)(q - u);
    if ((unsigned)slot >= MAX_UNITS) return 0;
#ifdef SC64_RACES
    if (sc64_race_unit_flying(q->type)) {
        if (q->order == ORDER_IDLE || (q->cd && q->order == ORDER_ATTACK_TARGET) ||
            (q->x == q->tx && q->y == q->ty)) return 0;
        return (int)(((unsigned)frame >> 2) & 3) + 1;
    }
#endif
    unsigned bits = SC64_WALK_BITS[slot];
    unsigned key = ((unsigned)q->path_timer << SC64_WALK_KEY_SHIFT) & SC64_WALK_KEY_MASK;
    unsigned stepped = (bits & SC64_WALK_KEY_MASK) != key;
    SC64_WALK_BITS[slot] = (u8)((bits & ~(SC64_WALK_KEY_MASK | SC64_WALK_HOLD)) |
                                key | (stepped ? SC64_WALK_HOLD : 0));
    if (!stepped && !(bits & SC64_WALK_HOLD)) return 0;
    return (int)(((unsigned)q->path_timer >> 1) & 3) + 1;
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
