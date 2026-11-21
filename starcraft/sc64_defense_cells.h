/* SPDX-License-Identifier: GPL-3.0-or-later */
/* A late mission can have dozens of defenses looking for a short-range
 * target. Group the current unit slots by 256-pixel cells once per defense
 * pass, after trigger/controller mutations. An ordinary 112-pixel search
 * then needs at most four cells. Slot ties remain independent of list order. */
enum {
    DEFENSE_CELL_SHIFT = 8,
    DEFENSE_CELL_SIDE = 8,
    DEFENSE_CELL_LIMIT = DEFENSE_CELL_SIDE << DEFENSE_CELL_SHIFT,
    DEFENSE_OWNER_LIMIT = 5,
    DEFENSE_HOSTILE_MASK_READY = 1 << DEFENSE_OWNER_LIMIT,
};
_Static_assert((DEFENSE_CELL_LIMIT & (DEFENSE_CELL_LIMIT - 1)) == 0,
               "defense cell bounds are a shared power of two");

typedef struct {
    u8 head[DEFENSE_CELL_SIDE * DEFENSE_CELL_SIDE];
    u8 next[MAX_UNITS];
} RaceDefenseCellLinks;

typedef struct {
    RaceDefenseCellLinks links;
    u8 flying[MAX_UNITS];
} RaceDefenseCells;

#ifdef SC64_DEFENSE_CUSTOM_CELLS
static int __attribute__((noinline))
race_defense_target_from_custom_cells(const Building *q, int ground, int air,
                                     int range, const RaceDefenseCellLinks *cells,
                                     u8 hostile_mask)
{
    int x = q->x, y = q->y;
    if (x <= -range || y <= -range ||
        x >= DEFENSE_CELL_LIMIT + range - 1 ||
        y >= DEFENSE_CELL_LIMIT + range - 1) return -1;
    int left = x - range + 1, top = y - range + 1;
    int right = x + range - 1, bottom = y + range - 1;
    int best = -1, bestd = range * range;
    left = (left < 0 ? 0 : left) >> DEFENSE_CELL_SHIFT;
    top = (top < 0 ? 0 : top) >> DEFENSE_CELL_SHIFT;
    right = (right >= DEFENSE_CELL_LIMIT ? DEFENSE_CELL_LIMIT - 1 : right) >> DEFENSE_CELL_SHIFT;
    bottom = (bottom >= DEFENSE_CELL_LIMIT ? DEFENSE_CELL_LIMIT - 1 : bottom) >> DEFENSE_CELL_SHIFT;

    for (int row = top; row <= bottom; ++row) {
        for (int col = left; col <= right; ++col) {
            int cell = row * DEFENSE_CELL_SIDE + col;
            for (int j = cells->head[cell]; j != 0xff; j = cells->next[j]) {
                const Unit *target = &u[j];
                unsigned owner = target->owner;
                if (!target->alive || owner >= DEFENSE_OWNER_LIMIT ||
                    !(hostile_mask & (1u << owner))) continue;
                int dx = x - target->x, dy = y - target->y;
                if (FAST_ABS(dx) >= range || FAST_ABS(dy) >= range ||
                    (target->flags & 16)) continue;
#ifdef SC64_COMBAT_FACT_TABLE_PROBE
                int flying = (sc64_combat_target_facts(target->type) & SC64_COMBAT_FLYING) != 0;
#else
                int flying = sc64_race_unit_flying(target->type);
#endif
                if ((flying ? !air : !ground) ||
                    !sc64_auto_target_candidate_allowed(-1, j, q->owner,
                                                        q->raw_owner)) continue;
                int d = dx * dx + dy * dy;
                if (d < bestd || (best >= 0 && d == bestd && j < best)) {
                    bestd = d;
                    best = j;
                }
            }
        }
    }
    return best;
}

#endif
