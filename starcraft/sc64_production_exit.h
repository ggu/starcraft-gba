/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_PRODUCTION_EXIT_H
#define SC64_PRODUCTION_EXIT_H

/* Bounded production placement search.
 *
 * Reference: pinned OpenBW commit 4b046d5f65302b10cb0a745f0fecd37ec85b20a8,
 * bwgame.h:8153-8173, 8194-8221, 16501-16680, 16711-16729. Its training
 * path creates a unit at the producer, then placement-checks and searches
 * outward when that unit completes. The GBA runtime has no unit dimensions or
 * native placement object, so callers provide the relevant bounds, terrain,
 * blocker, and reachability predicates. Coordinates here are GBA pixels.
 * The source's 8-pixel search lattice/range is represented as 4-pixel steps
 * and a 60-pixel radius after the source-to-GBA half-scale conversion.
 *
 * Search order deliberately favors the runtime's established exit point if
 * it is clear. Fallback then checks the producer center and square rings in
 * bottom/right/top/left order, retaining the pinned reference's outward
 * growth and edge order. It is a compact bounded approximation: it does not
 * reproduce native unit bounding boxes, dynamic obstacle skipping, or its
 * full 128-source-pixel rectangular search bounds. A false result means the
 * producer must keep the completed item queued; callers must not spawn at an
 * unchecked coordinate.
 */

enum {
    SC64_PRODUCTION_EXIT_STEP = 4,
    SC64_PRODUCTION_EXIT_MAX_RADIUS = 60,
    /* Preferred + center + the perimeter points for radii 4..60 by 8. */
    SC64_PRODUCTION_EXIT_MAX_CANDIDATES = 514
};

typedef struct {
    void *context;
    int (*in_bounds)(void *context, int x, int y);
    int (*terrain_fits)(void *context, int x, int y);
    /* Return nonzero if an actor of the requested flight class is blocked. */
    int (*building_blocked)(void *context, int x, int y, int flying);
    int (*unit_blocked)(void *context, int x, int y, int flying);
    int (*reachable)(void *context, int from_x, int from_y, int x, int y);
} Sc64ProductionExitOps;

static int sc64_production_exit_floor_step(int value)
{
    int rem = value % SC64_PRODUCTION_EXIT_STEP;
    if (rem < 0) rem += SC64_PRODUCTION_EXIT_STEP;
    return value - rem;
}

static int sc64_production_exit_candidate_valid(const Sc64ProductionExitOps *ops,
                                                 int from_x, int from_y,
                                                 int x, int y, int flying)
{
    if (!ops->in_bounds(ops->context, x, y)) return 0;
    if (ops->building_blocked(ops->context, x, y, flying)) return 0;
    if (ops->unit_blocked(ops->context, x, y, flying)) return 0;
    if (!ops->reachable(ops->context, from_x, from_y, x, y)) return 0;
    if (!flying && !ops->terrain_fits(ops->context, x, y)) return 0;
    return 1;
}

static int sc64_production_exit_try(const Sc64ProductionExitOps *ops,
                                    int from_x, int from_y, int x, int y,
                                    int flying, int *out_x, int *out_y)
{
    if (!sc64_production_exit_candidate_valid(ops, from_x, from_y, x, y, flying))
        return 0;
    *out_x = x;
    *out_y = y;
    return 1;
}

static int sc64_production_exit_find(const Sc64ProductionExitOps *ops,
                                     int producer_x, int producer_y,
                                     int preferred_x, int preferred_y,
                                     int flying, int *out_x, int *out_y)
{
    int base_x, base_y;
    if (!ops || !ops->in_bounds || !ops->terrain_fits ||
        !ops->building_blocked || !ops->unit_blocked || !ops->reachable ||
        !out_x || !out_y || (flying != 0 && flying != 1)) return 0;

    if (sc64_production_exit_try(ops, producer_x, producer_y,
                                 preferred_x, preferred_y, flying, out_x, out_y))
        return 1;
    if (sc64_production_exit_try(ops, producer_x, producer_y,
                                 producer_x, producer_y, flying, out_x, out_y))
        return 1;

    base_x = sc64_production_exit_floor_step(producer_x);
    base_y = sc64_production_exit_floor_step(producer_y);
    for (int radius = SC64_PRODUCTION_EXIT_STEP;
         radius <= SC64_PRODUCTION_EXIT_MAX_RADIUS; radius += 2 * SC64_PRODUCTION_EXIT_STEP) {
        int left = base_x - radius, right = base_x + radius;
        int top = base_y - radius, bottom = base_y + radius;

        /* Bottom edge, left to right. */
        for (int x = left; x <= right; x += SC64_PRODUCTION_EXIT_STEP)
            if (sc64_production_exit_try(ops, producer_x, producer_y, x, bottom,
                                         flying, out_x, out_y)) return 1;
        /* Right edge, bottom to top, without repeating corners. */
        for (int y = bottom - SC64_PRODUCTION_EXIT_STEP; y > top; y -= SC64_PRODUCTION_EXIT_STEP)
            if (sc64_production_exit_try(ops, producer_x, producer_y, right, y,
                                         flying, out_x, out_y)) return 1;
        /* Top edge, right to left. */
        for (int x = right; x >= left; x -= SC64_PRODUCTION_EXIT_STEP)
            if (sc64_production_exit_try(ops, producer_x, producer_y, x, top,
                                         flying, out_x, out_y)) return 1;
        /* Left edge, top to bottom, without repeating corners. */
        for (int y = top + SC64_PRODUCTION_EXIT_STEP; y < bottom; y += SC64_PRODUCTION_EXIT_STEP)
            if (sc64_production_exit_try(ops, producer_x, producer_y, left, y,
                                         flying, out_x, out_y)) return 1;
    }
    return 0;
}

#endif
