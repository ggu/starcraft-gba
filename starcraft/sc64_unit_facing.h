/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_UNIT_FACING_H
#define SC64_UNIT_FACING_H

/* Presentation-only unit facing for imported sprites.
 *
 * StarCraft unit art stores 17 frames per pose, from north clockwise to
 * south; the other 15 of its 32 directions are mirrored. Units here used
 * frame 0 only, so everything faced north. This keeps one 32-direction facing
 * byte per actor slot. A ground unit faces the path waypoint it is walking
 * to and a flyer faces its move target, so the 8-way movement steps do not
 * jitter. A unit turns toward its target when it fires. Movement orders keep
 * updating facing during cooldown; stationary firing units keep their aim.
 * With SC64_UNIT_WALK_FRAMES a moving unit also plays the poses of its
 * source Walking animation, imported into a second private header.
 * Nothing here is read by the simulation or saved; after a load, units face
 * north until they move or fire. */

#include "sc64_unit_facing_generated.h"
#ifdef SC64_UNIT_WALK_FRAMES
#include "sc64_unit_walk_generated.h"
#endif

#if defined(SC64_UNIT_WORD_SPRITES) && !defined(SC64_UNIT_FACING_WORD_METADATA)
#error "SC64_UNIT_WORD_SPRITES requires generated unit-facing word sidecars"
#endif

/* Ignore waypoints this close; the unit has arrived or is turning in place. */
#define SC64_FACING_STEP 4

/* sc64_facing[] is declared by sc64_unit_walk.h, whose stride bits share the
 * high three bits of each byte. */
#define SC64_FACING_MASK 31
#define SC64_FACING_SET(slot, dir) \
    (sc64_facing[slot] = (u8)((sc64_facing[slot] & ~SC64_FACING_MASK) | (dir)))

/* Presentation-only direction is not part of Unit or any save row. Resets
 * clear the stride bits too; they run where the walk cycle resets anyway. */
static void
sc64_facing_reset_all(void)
{
    memset(sc64_facing, 0, sizeof(sc64_facing));
}

static void
sc64_facing_reset_slot(int slot)
{
    if ((unsigned)slot < MAX_UNITS) sc64_facing[slot] = 0;
}

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
    SC64_FACING_SET(slot, sc64_facing_direction(tx - q->x, ty - q->y));
}

/* Called before drawing a visible unit: face where it is going. */
static void
sc64_facing_track(const Unit *q)
{
    int slot = (int)(q - u), dx, dy;
    if (slot < 0 || slot >= MAX_UNITS || q->order == ORDER_IDLE) return;
    if (q->cd && q->order == ORDER_ATTACK_TARGET) return;
    if (sc64_race_unit_flying(q->type)) {
        dx = q->tx - q->x;
        dy = q->ty - q->y;
    } else {
        dx = q->path_x - q->x;
        dy = q->path_y - q->y;
    }
    if (FAST_ABS(dx) < SC64_FACING_STEP && FAST_ABS(dy) < SC64_FACING_STEP) return;
    SC64_FACING_SET(slot, sc64_facing_direction(dx, dy));
}

static void
sc64_facing_blit_frame(const Sc64SpriteFrame *s, int dir, int x, int y)
{
    if (dir <= 16) {
        gfx_blit_sprite(x - s->canvas_w / 2 + s->offset_x,
                        y - s->canvas_h / 2 + s->offset_y,
                        s->w, s->h, s->pixels);
    } else {
        gfx_blit_sprite_hflip(x + s->canvas_w / 2 - s->offset_x - s->w,
                              y - s->canvas_h / 2 + s->offset_y,
                              s->w, s->h, s->pixels);
    }
}

static int
sc64_facing_blit(unsigned raw, int dir, int x, int y)
{
#if defined(SC64_UNIT_WORD_SPRITES) && defined(SC64_UNIT_FACING_WORD_METADATA)
    const Sc64UnitFacingWordFrame *word =
        sc64_unit_facing_word_frame((unsigned short)raw, (unsigned char)dir);
    if (word) {
        gfx_blit_sprite_words(x - word->canvas_w / 2 + word->offset_x,
                              y - word->canvas_h / 2 + word->offset_y,
                              word->sprite);
        return 1;
    }
#endif
    const Sc64SpriteFrame *frames = sc64_unit_facing_frames((unsigned short)raw);
    if (!frames) return 0;
    sc64_facing_blit_frame(&frames[dir <= 16 ? dir : 32 - dir], dir, x, y);
    return 1;
}

#ifdef SC64_UNIT_WALK_FRAMES
/* Source ticks run at four per five game ticks. Cache the conversion so a
 * frame divides once, not once per unit. */
static int sc64_walk_clock_frame = -1;
static unsigned sc64_walk_clock_ticks;

/* The 32 directional frames (western ones pre-mirrored) of the walk pose a
 * moving unit shows now, or 0 when the source has no walk cycle for it. The
 * cycle runs on the game clock while the unit moves; the slot offsets its
 * phase so a group does not march in lockstep. */
static const Sc64SpriteFrame *
sc64_walk_pose(unsigned raw, int slot)
{
    const Sc64UnitWalkCycle *cycle = sc64_unit_walk_cycle((unsigned short)raw);
    if (!cycle) return 0;
    if (sc64_walk_clock_frame != frame) {
        sc64_walk_clock_frame = frame;
        sc64_walk_clock_ticks = (unsigned)frame * 4u / 5u;
    }
    unsigned tick = (sc64_walk_clock_ticks + (unsigned)slot * 3u) % cycle->cycle_ticks;
    unsigned step = 0;
    while (step + 1 < cycle->count && cycle->steps[step + 1].tick <= tick) ++step;
    return cycle->steps[step].frames;
}

/* Walk frames are stored for every heading, so they never need the mirrored
 * blitter, which runs from ROM. */
static void
sc64_walk_blit(const Sc64SpriteFrame *pose, int dir, int x, int y)
{
    const Sc64SpriteFrame *s = &pose[dir];
    gfx_blit_sprite(x - s->canvas_w / 2 + s->offset_x, y - s->canvas_h / 2 + s->offset_y,
                    s->w, s->h, s->pixels);
}
#endif

/* Draw a unit with its facing frame, or its walk pose while it moves
 * (walk is sc64_walk_track's stride phase) or flies. Units without a source walk
 * cycle keep the one-pixel stride lift above ground row y. Siege Tanks draw
 * their hull and turret subunits (raw 5/6 in tank mode, 30/31 when sieged);
 * the sieged hull has a single frame. Muzzle/impact effects remain with the
 * stock VFX renderer. Returns 0 when no facing art exists for this unit. */
static int
sc64_facing_draw(const Unit *q, int raw, int x, int y, int walk)
{
    int dir, slot = (int)(q - u);
    if (raw < 0) return 0;
    dir = sc64_facing[slot] & SC64_FACING_MASK;
#ifdef SC64_RACES
    if (q->type == U_SIEGE_TANK || q->type == U_TANK) {
        int sieged = q->type == U_SIEGE_TANK, lift = SC64_WALK_LIFT(walk);
#ifdef SC64_UNIT_WALK_FRAMES
        /* Rolling treads replace the lift for the hull and its turret. */
        const Sc64SpriteFrame *pose = walk && !sieged ? sc64_walk_pose(5u, slot) : 0;
        if (pose) { sc64_walk_blit(pose, dir, x, y); lift = 0; }
        else
#endif
        if (!sc64_facing_blit(sieged ? 30u : 5u, dir, x, y - lift)) return 0;
        if (!sc64_facing_blit(sieged ? 31u : 6u, dir, x, y - lift)) return 0;
        return 1;
    }
#endif
#ifdef SC64_UNIT_WALK_FRAMES
    /* A flyer's source cycle is its wingbeat or pulse, which StarCraft plays
     * while it hovers too, so idle flyers keep cycling instead of freezing. */
    if (walk || sc64_race_unit_flying(q->type)) {
        const Sc64SpriteFrame *pose = sc64_walk_pose((unsigned)raw, slot);
        if (pose) {
            sc64_walk_blit(pose, dir, x, y);
            return 1;
        }
    }
#endif
    return sc64_facing_blit((unsigned)raw, dir, x, y - SC64_WALK_LIFT(walk));
}

#define SC64_FACING_FIRE(q, target) sc64_facing_fire((q), target_x(target), target_y(target))
#endif
