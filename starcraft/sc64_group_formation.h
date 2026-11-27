/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_GROUP_FORMATION_H
#define SC64_GROUP_FORMATION_H

/* Group move targets. StarCraft 64 keeps a clustered group's formation when
 * the ordered point lies outside the group: in the T1 parity capture all five
 * Marines moved by the same vector and ended in their original 37 x 72 px
 * cluster. A point inside the group's box, or a group spread wider than
 * SC64_GROUP_FORMATION_MAX_SPAN, still gathers on the point through the
 * engine's 5-wide slot grid. The span limit is this engine's bound, not a
 * measured N64 value. game.c includes this only in the ROM-derived
 * profiles, after sc64_command_delay.h; it reads u[] and the selection. */
enum { SC64_GROUP_FORMATION_MAX_SPAN = 128 }; /* GBA px (256 source px) */

/* A kept formation's ground slots must connect to the ordered point:
 * a slot across a cliff, wall or closed door, which the unit could reach
 * only by a long detour or not at all, is not kept. The test is a
 * four-neighbour flood from the ordered point's tile over a window of
 * SC64_FORMATION_REACH_RADIUS tiles around the translated group's centre.
 * Every slot lies within half the span (4 tiles) of that centre, and the
 * ordered point within 5 tiles even after the clamp at a map edge, so both
 * are always inside the window. Reached through the ordered point, a slot
 * is reachable from the unit whenever the point is. Bounded: one 13 x 13
 * flood per group order, no route queries and no grid rebuild (route state
 * is unchanged). */
enum { SC64_FORMATION_REACH_RADIUS = 6,
       SC64_FORMATION_REACH_SIDE = 2 * SC64_FORMATION_REACH_RADIUS + 1,
       SC64_FORMATION_REACH_CELLS = SC64_FORMATION_REACH_SIDE * SC64_FORMATION_REACH_SIDE,
       /* Buildings considered for a window with a stale walk grid: those
        * centred within this many tiles of it (footprints and doors are
        * smaller). */
       SC64_FORMATION_REACH_BUILDING_MARGIN = 16 };

typedef struct {
    int keep, dx, dy;
    int ground;           /* the ordered point is open: ground slots use reach */
    int col0, row0;       /* window origin, in tiles */
    u8 reach[(SC64_FORMATION_REACH_CELLS + 7) / 8];
} Sc64GroupFormation;

static int sc64_formation_building_covers(int i, int col, int row);
static int sc64_formation_cell_built(int col, int row);

/* Open for a ground slot: terrain, then buildings and doors from the walk
 * grid when it is current, else from b[] directly. */
static int
sc64_formation_cell_open(int col, int row)
{
    if (col < 0 || row < 0 || col >= MAP_COLS || row >= MAP_ROWS || terrain_blocked(col, row))
        return 0;
    if (path_grid_valid) return SC64_GRID_GET(path_walkable, col + row * MAP_COLS);
    return !sc64_formation_cell_built(col, row);
}

/* Window cell (nx, ny) for the flood: terrain, plus the walk grid when it
 * is current or else the footprints of the buildings near the window (a
 * bounded candidate list, no grid rebuild). */
static int
sc64_group_formation_window_open(const Sc64GroupFormation *f, const u8 *near, int near_count,
                                 int nx, int ny)
{
    int col = f->col0 + nx, row = f->row0 + ny;
    if (col < 0 || row < 0 || col >= MAP_COLS || row >= MAP_ROWS || terrain_blocked(col, row))
        return 0;
    if (path_grid_valid) return SC64_GRID_GET(path_walkable, col + row * MAP_COLS);
    for (int k = 0; k < near_count; ++k)
        if (sc64_formation_building_covers(near[k], col, row)) return 0;
    return 1;
}

/* Returns 0, with nothing reached, when the ordered point's tile is
 * blocked. */
static int __attribute__((noinline))
sc64_group_formation_flood(Sc64GroupFormation *f, int tx, int ty, int center_x, int center_y)
{
    static const s8 step_x[4] = {1, -1, 0, 0}, step_y[4] = {0, 0, 1, -1};
    u8 queue[SC64_FORMATION_REACH_CELLS], near[MAX_BUILDINGS];
    int head = 0, tail = 0, near_count = 0, sx, sy;
    f->col0 = center_x / 16 - SC64_FORMATION_REACH_RADIUS;
    f->row0 = center_y / 16 - SC64_FORMATION_REACH_RADIUS;
    memset(f->reach, 0, sizeof f->reach);
    if (!path_grid_valid)
        for (int i = 0; i < MAX_BUILDINGS; ++i)
            if (b[i].alive &&
                FAST_ABS(b[i].x / 16 - center_x / 16) <=
                    SC64_FORMATION_REACH_RADIUS + SC64_FORMATION_REACH_BUILDING_MARGIN &&
                FAST_ABS(b[i].y / 16 - center_y / 16) <=
                    SC64_FORMATION_REACH_RADIUS + SC64_FORMATION_REACH_BUILDING_MARGIN)
                near[near_count++] = (u8)i;
    sx = tx / 16 - f->col0; sy = ty / 16 - f->row0;
    if (sx < 0 || sy < 0 || sx >= SC64_FORMATION_REACH_SIDE || sy >= SC64_FORMATION_REACH_SIDE ||
        !sc64_group_formation_window_open(f, near, near_count, sx, sy)) return 0;
    queue[tail++] = (u8)(sx + sy * SC64_FORMATION_REACH_SIDE);
    f->reach[queue[0] >> 3] |= (u8)(1u << (queue[0] & 7));
    while (head < tail) {
        int at = queue[head++];
        for (int d = 0; d < 4; ++d) {
            int nx = at % SC64_FORMATION_REACH_SIDE + step_x[d];
            int ny = at / SC64_FORMATION_REACH_SIDE + step_y[d], next;
            if (nx < 0 || ny < 0 || nx >= SC64_FORMATION_REACH_SIDE || ny >= SC64_FORMATION_REACH_SIDE)
                continue;
            next = nx + ny * SC64_FORMATION_REACH_SIDE;
            if ((f->reach[next >> 3] >> (next & 7)) & 1) continue;
            if (!sc64_group_formation_window_open(f, near, near_count, nx, ny)) continue;
            f->reach[next >> 3] |= (u8)(1u << (next & 7));
            queue[tail++] = (u8)next;
        }
    }
    return 1;
}

static int
sc64_group_formation_reaches(const Sc64GroupFormation *f, int x, int y)
{
    int col = x / 16 - f->col0, row = y / 16 - f->row0, at;
    if (col < 0 || row < 0 || col >= SC64_FORMATION_REACH_SIDE || row >= SC64_FORMATION_REACH_SIDE)
        return 0;
    at = col + row * SC64_FORMATION_REACH_SIDE;
    return (f->reach[at >> 3] >> (at & 7)) & 1;
}

/* Move orders only: an attack-move keeps gathering on its point, so no unit
 * stops outside acquisition range of it. The group moves by one translation,
 * clamped as a whole so a formation at a map edge keeps its spacing. */
static inline Sc64GroupFormation
sc64_group_formation_begin(int order, int group_order, int tx, int ty)
{
    Sc64GroupFormation f;
    int left = WORLD_W, top = WORLD_H, right = -1, bottom = -1;
    memset(&f, 0, sizeof f);
    if (!group_order || order != ORDER_MOVE) return f;
    for (int i = 0; i < MAX_UNITS; ++i) {
        if (!sc64_command_delay_mover(i)) continue;
        if (u[i].x < left) left = u[i].x;
        if (u[i].x > right) right = u[i].x;
        if (u[i].y < top) top = u[i].y;
        if (u[i].y > bottom) bottom = u[i].y;
    }
    if (right < 0 || right - left > SC64_GROUP_FORMATION_MAX_SPAN ||
        bottom - top > SC64_GROUP_FORMATION_MAX_SPAN ||
        (tx >= left && tx <= right && ty >= top && ty <= bottom)) return f;
    f.keep = 1;
    f.dx = limit(tx - (left + right) / 2, 8 - left, WORLD_W - 8 - right);
    f.dy = limit(ty - (top + bottom) / 2, 8 - top, WORLD_H - 8 - bottom);
    /* Flyers keep the translation regardless; ground units gather on the
     * grid, as before, when the ordered point itself is blocked. */
    f.ground = sc64_group_formation_flood(&f, tx, ty, (left + right) / 2 + f.dx,
                                          (top + bottom) / 2 + f.dy);
    return f;
}

/* Whether building slot i covers walk cell (col, row): the per-building
 * tests path_grid_build_full() (path_grid_build.h) applies, read from b[]
 * so neither the walk grid nor the route state is rebuilt at order time.
 * The Terran command-delay test compares sc64_formation_cell_built() with a
 * freshly built grid on every cell, doors included. */
static int __attribute__((noinline))
sc64_formation_building_covers(int i, int col, int row)
{
    int left, up, right, down;
    if (!b[i].alive) return 0;
    (void)left; (void)up; (void)right; (void)down;
#ifdef SC64_BATTLECRUISER_FLIGHT_EXPERIMENT
    if (race_bc_facility_airborne(i)) return 0;
#endif
#ifdef SC64_RACES
    if (b[i].type == B_ZERG_BEACON) return 0;
#if defined(SC64_T4_RUNTIME) || defined(SC64_ZP_CAMPAIGN_ACTORS)
    if (!sc64_t4_building_collidable(i)) return 0;
#if defined(SC64_T4_RUNTIME) && defined(SC64_T4_SCENARIO_RUNTIME)
    if (race_t4_active() && sc64_race_t4_is_door(b[i].type)) {
        /* race_t4_block_door_grid(): every cell the bounds touch. */
        if (sc64_race_t4_building_bounds_source(b[i].type, &left, &up, &right, &down) &&
            col >= limit((b[i].x * 2 - left) / 32, 0, MAP_COLS - 1) &&
            col <= limit((b[i].x * 2 + right) / 32, 0, MAP_COLS - 1) &&
            row >= limit((b[i].y * 2 - up) / 32, 0, MAP_ROWS - 1) &&
            row <= limit((b[i].y * 2 + down) / 32, 0, MAP_ROWS - 1)) return 1;
        return 0;
    }
#endif
#ifdef SC64_Z5_RUNTIME
    if (race_z5_active() && race_z5_is_door_type(b[i].type)) {
        if (race_z5_building_collidable(i) &&
            sc64_game_building_bounds_source(b[i].type, &left, &up, &right, &down) &&
            col >= limit((b[i].x * 2 - left) / 32, 0, MAP_COLS - 1) &&
            col <= limit((b[i].x * 2 + right) / 32, 0, MAP_COLS - 1) &&
            row >= limit((b[i].y * 2 - up) / 32, 0, MAP_ROWS - 1) &&
            row <= limit((b[i].y * 2 + down) / 32, 0, MAP_ROWS - 1)) return 1;
        return 0;
    }
#endif
#endif
#endif
#ifdef SC64_ZP_CAMPAIGN_ACTORS
    if (sc64_game_building_bounds_source(b[i].type, &left, &up, &right, &down)) {
        int x0 = b[i].x * 2 - left, x1 = b[i].x * 2 + right;
        int y0 = b[i].y * 2 - up, y1 = b[i].y * 2 + down;
        if (col >= limit(x0 / 32, 0, MAP_COLS - 1) && col <= limit(x1 / 32, 0, MAP_COLS - 1) &&
            row >= limit(y0 / 32, 0, MAP_ROWS - 1) && row <= limit(y1 / 32, 0, MAP_ROWS - 1) &&
            col * 32 <= x1 && col * 32 + 32 >= x0 && row * 32 <= y1 && row * 32 + 32 >= y0)
            return 1;
        return 0;
    }
#endif
    if (col >= b[i].x / 16 - 2 && col <= b[i].x / 16 + 2 &&
        row >= b[i].y / 16 - 2 && row <= b[i].y / 16 + 2 &&
        dist2(col * 16 + 8, row * 16 + 8, b[i].x, b[i].y) < 18 * 18) return 1;
    return 0;
}

/* One bounded pass over the building table. */
static int __attribute__((noinline))
sc64_formation_cell_built(int col, int row)
{
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (sc64_formation_building_covers(i, col, row)) return 1;
    return 0;
}

/* The nearest open tile centre within 2 tiles of (*x, *y), for a fallback
 * slot on blocked ground (a Move ends only within 5 px of its point, so a
 * point inside a wall is never reached). Unchanged when it is open or no
 * open tile is near. */
static void __attribute__((noinline))
sc64_group_formation_open_near(int *x, int *y)
{
    int col = *x / 16, row = *y / 16;
    if (sc64_formation_cell_open(col, row)) return;
    for (int r = 1; r <= 2; ++r)
        for (int dy = -r; dy <= r; ++dy)
            for (int dx = -r; dx <= r; ++dx) {
                if (FAST_ABS(dx) != r && FAST_ABS(dy) != r) continue;
                if (!sc64_formation_cell_open(col + dx, row + dy)) continue;
                *x = (col + dx) * 16 + 8;
                *y = (row + dy) * 16 + 8;
                return;
            }
}

/* The n-th selected unit's destination for a group order to (tx, ty).
 * With check_tile, a kept formation's ground slot must be open (terrain,
 * buildings and doors: the walk grid when it is current, else b[]
 * directly) and connect to the ordered point (the flood above); otherwise
 * the unit takes its grid slot, or the ordered point itself when that grid
 * slot does not connect either. With the ordered point itself blocked
 * nothing can connect to it: an open translated slot is kept, and a unit
 * whose slot is blocked takes its grid slot, moved to the nearest open
 * tile when that is blocked too. Flyers keep the translation. Both callers
 * set check_tile: the reachable-formation route check can count a blocked
 * tile reachable through a neighbour and keep a point no unit reaches. */
static inline void
sc64_group_formation_slot(const Sc64GroupFormation *f, const Unit *q, int n,
                          int tx, int ty, int check_tile, int *slot_x, int *slot_y)
{
    int ground = check_tile;
#ifdef SC64_RACES
    if (sc64_race_unit_flying(q->type)) ground = 0;
#endif
    if (f->keep) {
        *slot_x = q->x + f->dx;
        *slot_y = q->y + f->dy;
        if (!ground) return;
        if ((!f->ground || sc64_group_formation_reaches(f, *slot_x, *slot_y)) &&
            sc64_formation_cell_open(*slot_x / 16, *slot_y / 16)) return;
    }
    *slot_x = limit(tx + ((n % 5) - 2) * 8, 8, WORLD_W - 8);
    *slot_y = limit(ty + ((n / 5) - 1) * 8, 8, WORLD_H - 8);
    if (!f->keep || !ground) return;
    if (!f->ground) {
        sc64_group_formation_open_near(slot_x, slot_y);
    } else if (!(sc64_group_formation_reaches(f, *slot_x, *slot_y) &&
                 sc64_formation_cell_open(*slot_x / 16, *slot_y / 16))) {
        *slot_x = tx;
        *slot_y = ty;
    }
}

#endif
