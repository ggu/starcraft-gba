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

typedef struct { int keep, dx, dy; } Sc64GroupFormation;

/* Move orders only: an attack-move keeps gathering on its point, so no unit
 * stops outside acquisition range of it. The group moves by one translation,
 * clamped as a whole so a formation at a map edge keeps its spacing. */
static inline Sc64GroupFormation
sc64_group_formation_begin(int order, int group_order, int tx, int ty)
{
    Sc64GroupFormation f = {0, 0, 0};
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
 * With check_tile a translated ground slot on blocked terrain, or under a
 * building or door, takes its grid slot. Both callers set it: the
 * reachable-formation route check can count a blocked tile reachable
 * through a neighbour and keep a point no unit reaches. Buildings and doors
 * come from the walk grid when it is current, else from b[] directly. */
static inline void
sc64_group_formation_slot(const Sc64GroupFormation *f, const Unit *q, int n,
                          int tx, int ty, int check_tile, int *slot_x, int *slot_y)
{
    if (f->keep) {
        int col, row;
        *slot_x = q->x + f->dx;
        *slot_y = q->y + f->dy;
        col = *slot_x / 16; row = *slot_y / 16;
        if (!check_tile ||
#ifdef SC64_RACES
            sc64_race_unit_flying(q->type) ||
#endif
            (!terrain_blocked(col, row) &&
             (path_grid_valid ? SC64_GRID_GET(path_walkable, col + row * MAP_COLS)
                              : !sc64_formation_cell_built(col, row)))) return;
    }
    *slot_x = limit(tx + ((n % 5) - 2) * 8, 8, WORLD_W - 8);
    *slot_y = limit(ty + ((n / 5) - 1) * 8, 8, WORLD_H - 8);
}

#endif
