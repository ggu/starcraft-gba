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

/* The n-th selected unit's destination for a group order to (tx, ty).
 * With check_tile a translated ground slot on blocked terrain, or under a
 * building or door, takes its grid slot. Both callers set it: the reachable-formation route check can count
 * a blocked tile reachable through a neighbour and keep a point no unit
 * reaches. */
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
            (!terrain_blocked(*slot_x / 16, *slot_y / 16) &&
             /* Buildings and doors, from the walk grid when it is current:
              * rebuilding it here would change route state. */
             (!path_grid_valid ||
              SC64_GRID_GET(path_walkable, *slot_x / 16 + (*slot_y / 16) * MAP_COLS)))) return;
    }
    *slot_x = limit(tx + ((n % 5) - 2) * 8, 8, WORLD_W - 8);
    *slot_y = limit(ty + ((n / 5) - 1) * 8, 8, WORLD_H - 8);
}

#endif
