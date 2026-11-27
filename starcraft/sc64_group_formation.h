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

/* A kept formation's slots lie within half the span of the ordered point
 * (4 tiles). A slot is accepted only if it connects to the ordered point
 * through open tiles inside a window of SC64_FORMATION_REACH_RADIUS tiles
 * around it, so a slot across a cliff or wall, which the unit could reach
 * only by a long detour or not at all, is not kept. Reached through the
 * ordered point, a slot is reachable from the unit whenever the point is.
 * Bounded: one 13 x 13 flood per group order, no route queries and no grid
 * rebuild (route state is unchanged). */
enum { SC64_FORMATION_REACH_RADIUS = 6,
       SC64_FORMATION_REACH_SIDE = 2 * SC64_FORMATION_REACH_RADIUS + 1,
       SC64_FORMATION_REACH_CELLS = SC64_FORMATION_REACH_SIDE * SC64_FORMATION_REACH_SIDE };

typedef struct {
    int keep, dx, dy, col0, row0;
    u8 reach[(SC64_FORMATION_REACH_CELLS + 7) / 8];
} Sc64GroupFormation;

/* Open for the formation: terrain, and buildings or doors from the walk grid
 * when it is current, else from b[] (sc64_formation_cell_built below). */
static int sc64_formation_cell_built(int col, int row);
static int
sc64_formation_cell_open(int col, int row, int buildings)
{
    if (col < 0 || row < 0 || col >= MAP_COLS || row >= MAP_ROWS || terrain_blocked(col, row))
        return 0;
    if (path_grid_valid) return SC64_GRID_GET(path_walkable, col + row * MAP_COLS);
    return !buildings || !sc64_formation_cell_built(col, row);
}

/* Four-neighbour flood from the ordered point's tile over the window, on
 * terrain and a current walk grid (a stale grid's buildings are left to the
 * slot's own footprint test). Returns 0 when the point itself is blocked. */
static int __attribute__((noinline))
sc64_group_formation_flood(Sc64GroupFormation *f, int tx, int ty)
{
    u8 queue[SC64_FORMATION_REACH_CELLS];
    int head = 0, tail = 0, col = tx / 16, row = ty / 16;
    f->col0 = col - SC64_FORMATION_REACH_RADIUS;
    f->row0 = row - SC64_FORMATION_REACH_RADIUS;
    memset(f->reach, 0, sizeof f->reach);
    if (!sc64_formation_cell_open(col, row, 0)) return 0;
    queue[tail++] = (u8)(SC64_FORMATION_REACH_RADIUS * SC64_FORMATION_REACH_SIDE + SC64_FORMATION_REACH_RADIUS);
    f->reach[queue[0] >> 3] |= (u8)(1u << (queue[0] & 7));
    while (head < tail) {
        static const s8 step_x[4] = {1, -1, 0, 0}, step_y[4] = {0, 0, 1, -1};
        int at = queue[head++], ax = at % SC64_FORMATION_REACH_SIDE, ay = at / SC64_FORMATION_REACH_SIDE;
        for (int d = 0; d < 4; ++d) {
            int nx = ax + step_x[d], ny = ay + step_y[d], next;
            if (nx < 0 || ny < 0 || nx >= SC64_FORMATION_REACH_SIDE || ny >= SC64_FORMATION_REACH_SIDE) continue;
            next = nx + ny * SC64_FORMATION_REACH_SIDE;
            if ((f->reach[next >> 3] >> (next & 7)) & 1) continue;
            if (!sc64_formation_cell_open(f->col0 + nx, f->row0 + ny, 0)) continue;
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
    memset(&f, 0, sizeof f);
    int left = WORLD_W, top = WORLD_H, right = -1, bottom = -1;
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
    /* An ordered point on blocked ground gathers on the grid, as before. */
    if (!sc64_group_formation_flood(&f, tx, ty)) return f;
    f.keep = 1;
    f.dx = limit(tx - (left + right) / 2, 8 - left, WORLD_W - 8 - right);
    f.dy = limit(ty - (top + bottom) / 2, 8 - top, WORLD_H - 8 - bottom);
    return f;
}

/* Whether a building or door covers walk cell (col, row): the per-building
 * tests path_grid_build_full() (path_grid_build.h) applies, read from b[]
 * so neither the walk grid nor the route state is rebuilt at order time.
 * One bounded pass over the building table. The Terran command-delay test
 * compares it with a freshly built grid, doors included. */
static int __attribute__((noinline))
sc64_formation_cell_built(int col, int row)
{
    for (int i = 0; i < MAX_BUILDINGS; ++i) {
        int left, up, right, down;
        if (!b[i].alive) continue;
        (void)left; (void)up; (void)right; (void)down;
#ifdef SC64_BATTLECRUISER_FLIGHT_EXPERIMENT
        if (race_bc_facility_airborne(i)) continue;
#endif
#ifdef SC64_RACES
        if (b[i].type == B_ZERG_BEACON) continue;
#if defined(SC64_T4_RUNTIME) || defined(SC64_ZP_CAMPAIGN_ACTORS)
        if (!sc64_t4_building_collidable(i)) continue;
#if defined(SC64_T4_RUNTIME) && defined(SC64_T4_SCENARIO_RUNTIME)
        if (race_t4_active() && sc64_race_t4_is_door(b[i].type)) {
            /* race_t4_block_door_grid(): every cell the bounds touch. */
            if (sc64_race_t4_building_bounds_source(b[i].type, &left, &up, &right, &down) &&
                col >= limit((b[i].x * 2 - left) / 32, 0, MAP_COLS - 1) &&
                col <= limit((b[i].x * 2 + right) / 32, 0, MAP_COLS - 1) &&
                row >= limit((b[i].y * 2 - up) / 32, 0, MAP_ROWS - 1) &&
                row <= limit((b[i].y * 2 + down) / 32, 0, MAP_ROWS - 1)) return 1;
            continue;
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
            continue;
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
            continue;
        }
#endif
        if (col >= b[i].x / 16 - 2 && col <= b[i].x / 16 + 2 &&
            row >= b[i].y / 16 - 2 && row <= b[i].y / 16 + 2 &&
            dist2(col * 16 + 8, row * 16 + 8, b[i].x, b[i].y) < 18 * 18) return 1;
    }
    return 0;
}

/* The n-th selected unit's destination for a group order to (tx, ty).
 * With check_tile, a kept formation's ground slot must be open (terrain,
 * buildings and doors: the walk grid when it is current, else b[]
 * directly) and connect to the ordered point (the flood above); otherwise
 * the unit takes its grid slot, or the ordered point itself when that
 * grid slot does not connect either. Both callers set check_tile: the
 * reachable-formation route check can count a blocked tile reachable
 * through a neighbour and keep a point no unit reaches. */
static inline void
sc64_group_formation_slot(const Sc64GroupFormation *f, const Unit *q, int n,
                          int tx, int ty, int check_tile, int *slot_x, int *slot_y)
{
    if (f->keep) {
        *slot_x = q->x + f->dx;
        *slot_y = q->y + f->dy;
        if (!check_tile ||
#ifdef SC64_RACES
            sc64_race_unit_flying(q->type) ||
#endif
            (sc64_formation_cell_open(*slot_x / 16, *slot_y / 16, 1) &&
             sc64_group_formation_reaches(f, *slot_x, *slot_y))) return;
    }
    *slot_x = limit(tx + ((n % 5) - 2) * 8, 8, WORLD_W - 8);
    *slot_y = limit(ty + ((n / 5) - 1) * 8, 8, WORLD_H - 8);
    if (f->keep && check_tile &&
#ifdef SC64_RACES
        !sc64_race_unit_flying(q->type) &&
#endif
        !(sc64_formation_cell_open(*slot_x / 16, *slot_y / 16, 1) &&
          sc64_group_formation_reaches(f, *slot_x, *slot_y))) {
        *slot_x = tx;
        *slot_y = ty;
    }
}

#endif
