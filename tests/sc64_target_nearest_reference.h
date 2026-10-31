/* SPDX-License-Identifier: GPL-3.0-or-later
 * Unmodified nearest-target query from bb55a170 (renamed for differential testing).
 */
static int GBA_HOT __attribute__((noinline))
#if defined(SC64_T6_RUNTIME) && !defined(GBA_HOST)
__attribute__((optimize("Os")))
#endif
sc64_target_nearest_reference(int x, int y, int owner, int max_distance, int attacker
                    SC64_P7_SOURCE_OWNER_PARAM)
{
#ifdef SC64_Z7_EGG_PRODUCTION
    int egg_best = -1, egg_distance = max_distance * max_distance;
#endif
#if defined(SC64_LATE_TERRAN_STORAGE) || defined(SC64_TARGET_OCCUPANCY)
    int best = -1, best_distance = max_distance * max_distance, d;
    int cols = (WORLD_W + SC64_TARGET_CELL_SIZE - 1) / SC64_TARGET_CELL_SIZE;
    int rows = (WORLD_H + SC64_TARGET_CELL_SIZE - 1) / SC64_TARGET_CELL_SIZE;
    int col0 = (x - max_distance) / SC64_TARGET_CELL_SIZE;
    int row0 = (y - max_distance) / SC64_TARGET_CELL_SIZE;
    int col1 = (x + max_distance) / SC64_TARGET_CELL_SIZE;
    int row1 = (y + max_distance) / SC64_TARGET_CELL_SIZE;
    u32 col_mask, row_mask;
    int p, col, row;
    u8 hostile_mask;
    if (col0 < 0) col0 = 0;
    if (row0 < 0) row0 = 0;
    if (col1 >= cols) col1 = cols - 1;
    if (row1 >= rows) row1 = rows - 1;
    col_mask = (0xffffffffu >> (31 - col1)) & (0xffffffffu << col0);
    row_mask = (0xffffffffu >> (31 - row1)) & (0xffffffffu << row0);
    if (target_hostile_masks_valid && owner >= 0 && owner < 5) {
        hostile_mask = target_hostile_mask[owner];
    } else {
        hostile_mask = 0;
        for (p = 0; p < 5; ++p)
            if (owners_hostile(owner, p)) hostile_mask |= (u8)(1u << p);
    }
    for (p = 0; hostile_mask; ++p, hostile_mask >>= 1) {
        if (!(hostile_mask & 1u)) continue;
        if (!(sc64_target_owner_columns[p] & col_mask) ||
            !(sc64_target_owner_rows[p] & row_mask)) continue;
        for (row = row0; row <= row1; ++row)
            for (col = col0; col <= col1; ++col) {
                int cell = col + row * SC64_TARGET_GRID_COLS;
                int entity;
                for (entity = sc64_target_head[p][cell]; entity != SC64_TARGET_NONE;
                     entity = sc64_target_next[entity]) {
                    int candidate_x, candidate_y;
                    if (entity < MAX_UNITS) {
                        if (!u[entity].alive) continue;
                        candidate_x = u[entity].x; candidate_y = u[entity].y;
                    } else {
                        int bi = entity - MAX_UNITS;
                        if (!b[bi].alive) continue;
                        candidate_x = b[bi].x; candidate_y = b[bi].y;
                    }
                    if (FAST_ABS(x - candidate_x) >= max_distance ||
                        FAST_ABS(y - candidate_y) >= max_distance) continue;
                    d = dist2(x, y, candidate_x, candidate_y);
#ifdef SC64_Z7_EGG_PRODUCTION
                    if (entity < MAX_UNITS && u[entity].type == U_Z7_EGG) {
                        if (d < egg_distance &&
                            sc64_auto_target_candidate_allowed(
                                attacker, entity, owner,
                                SC64_P7_SOURCE_OWNER_VALUE)) {
                            egg_distance = d;
                            egg_best = entity;
                        }
                        continue;
                    }
#endif
                    if (d > best_distance ||
                        (d == best_distance && (best < 0 || entity >= best)) ||
                        !sc64_auto_target_candidate_allowed(
                            attacker, entity, owner,
                            SC64_P7_SOURCE_OWNER_VALUE)) continue;
                    best_distance = d;
                    best = entity;
                }
            }
    }
#ifdef SC64_Z7_EGG_PRODUCTION
    return best >= 0 ? best : egg_best;
#else
    return best;
#endif
#else
    int best = -1, best_distance = max_distance * max_distance, d;
    int cols = (WORLD_W + SC64_TARGET_CELL_SIZE - 1) / SC64_TARGET_CELL_SIZE;
    int rows = (WORLD_H + SC64_TARGET_CELL_SIZE - 1) / SC64_TARGET_CELL_SIZE;
    int col0 = (x - max_distance) / SC64_TARGET_CELL_SIZE;
    int row0 = (y - max_distance) / SC64_TARGET_CELL_SIZE;
    int col1 = (x + max_distance) / SC64_TARGET_CELL_SIZE;
    int row1 = (y + max_distance) / SC64_TARGET_CELL_SIZE;
    int p, col, row;
    if (col0 < 0) col0 = 0;
    if (row0 < 0) row0 = 0;
    if (col1 >= cols) col1 = cols - 1;
    if (row1 >= rows) row1 = rows - 1;
    for (p = 0; p < 5; ++p) {
        if (!owners_hostile(owner, p)) continue;
        for (row = row0; row <= row1; ++row)
            for (col = col0; col <= col1; ++col) {
                int cell = col + row * SC64_TARGET_GRID_COLS, entity;
                for (entity = sc64_target_head[p][cell]; entity != SC64_TARGET_NONE;
                     entity = sc64_target_next[entity]) {
                    int candidate_x, candidate_y;
                    if (entity < MAX_UNITS) {
                        if (!u[entity].alive) continue;
                        candidate_x = u[entity].x; candidate_y = u[entity].y;
                    } else {
                        int bi = entity - MAX_UNITS;
                        if (!b[bi].alive) continue;
                        candidate_x = b[bi].x; candidate_y = b[bi].y;
                    }
                    if (FAST_ABS(x - candidate_x) >= max_distance ||
                        FAST_ABS(y - candidate_y) >= max_distance ||
                        !sc64_auto_target_candidate_allowed(
                            attacker, entity, owner,
                            SC64_P7_SOURCE_OWNER_VALUE)) continue;
                    d = dist2(x, y, candidate_x, candidate_y);
#ifdef SC64_Z7_EGG_PRODUCTION
                    if (entity < MAX_UNITS && u[entity].type == U_Z7_EGG) {
                        if (d < egg_distance) {
                            egg_distance = d;
                            egg_best = entity;
                        }
                        continue;
                    }
#endif
                    if (d < best_distance ||
                        (best >= 0 && d == best_distance && entity < best)) {
                        best_distance = d;
                        best = entity;
                    }
                }
            }
    }
#ifdef SC64_Z7_EGG_PRODUCTION
    return best >= 0 ? best : egg_best;
#else
    return best;
#endif
#endif
}
