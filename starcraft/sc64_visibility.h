#ifndef SC64_VISIBILITY_H
#define SC64_VISIBILITY_H

/* Exact incremental replacement for the SC64 update_exploration() scan.
 * Include after Unit/Building declarations and the globals u, b, visible,
 * explored, MAX_UNITS/MAX_BUILDINGS, SIDE, U_SCV, and B_COMMAND are defined.
 *
 * explored is monotonic and is only added to, matching the original update.
 * visible is maintained from a per-cell source count; changed source
 * footprints are removed and re-added using the same circles and coordinates
 * as the original full recomputation.
 */

typedef struct {
    int x, y, type;
    u8 active;
} Sc64VisibilitySnapshot;

static u8 sc64_visible_sources[MAP_CELLS];
static u8 sc64_minimap_visible[24 * 24];
static u8 sc64_minimap_explored[24 * 24];
static Sc64VisibilitySnapshot sc64_unit_visibility[MAX_UNITS];
static Sc64VisibilitySnapshot sc64_building_visibility[MAX_BUILDINGS];

static int GBA_HOT
sc64_minimap_cell(int cell)
{
#ifdef SC64_RACES
    int col=sc64_cell_col(cell),row=sc64_cell_row(cell);
#else
    int col=cell%MAP_COLS,row=cell/MAP_COLS;
#endif
    int mx =
#ifdef SC64_RACES
        MAP_COLS==64?(col*3)>>3:col>>2;
#else
        col*24/MAP_COLS;
#endif
    int my =
#ifdef SC64_RACES
        MAP_ROWS==64?(row*3)>>3:row>>2;
#else
        row*24/MAP_ROWS;
#endif
    return mx + my * 24;
}

static void
sc64_visibility_reset(void)
{
    int i;
    memset(sc64_visible_sources, 0, sizeof(sc64_visible_sources));
    memset(sc64_minimap_visible, 0, sizeof(sc64_minimap_visible));
    memset(sc64_minimap_explored, 0, sizeof(sc64_minimap_explored));
    memset(visible, 0, sizeof(visible));
    /* `explored` is save-persistent, so rebuild aggregate occupancy from the
       restored full-resolution grid before sources are re-added. */
    for (i = 0; i < MAP_COLS * MAP_ROWS; i++)
        if (explored[i])
            sc64_minimap_explored[sc64_minimap_cell(i)]++;
    for (i = 0; i < MAX_UNITS; i++)
        sc64_unit_visibility[i].active = 0;
    for (i = 0; i < MAX_BUILDINGS; i++)
        sc64_building_visibility[i].active = 0;
}

static int GBA_HOT
sc64_visibility_same(const Sc64VisibilitySnapshot *old, int active,
                     int x, int y, int type)
{
    return old->active == (u8)active && (!active ||
        (old->x == x && old->y == y && old->type == type));
}

static void GBA_HOT
sc64_explore_unit(int x, int y, int type)
{
    int col, row, c = x / 16, r = y / 16;
    int radius =
#ifdef SC64_RACES
        IS_WORKER(type)
#else
        type==U_SCV
#endif
        ? 4 : 5;
    for (row = r - radius; row <= r + radius; row++)
        for (col = c - radius; col <= c + radius; col++)
            if (col >= 0 && row >= 0 && col < MAP_COLS && row < MAP_ROWS &&
                (col - c) * (col - c) + (row - r) * (row - r) <= radius * radius) {
                int cell = col + row * MAP_COLS;
                if (!explored[cell]) {
                    explored[cell] = 1;
                    sc64_minimap_explored[sc64_minimap_cell(cell)]++;
                }
            }
}

static void GBA_HOT
sc64_explore_building(int x, int y, int type)
{
    int col, row, c = x / 16, r = y / 16;
    int radius = type == B_COMMAND ? 7 : 3;
    for (row = r - radius; row <= r + radius; row++)
        for (col = c - radius; col <= c + radius; col++)
            if (col >= 0 && row >= 0 && col < MAP_COLS && row < MAP_ROWS &&
                (col - c) * (col - c) + (row - r) * (row - r) <= radius * radius) {
                int cell = col + row * MAP_COLS;
                if (!explored[cell]) {
                    explored[cell] = 1;
                    sc64_minimap_explored[sc64_minimap_cell(cell)]++;
                }
            }
}

static void GBA_HOT
sc64_adjust_unit_visibility(int x, int y, int type, int delta)
{
    int col, row, c = x / 16, r = y / 16;
    int radius =
#ifdef SC64_RACES
        IS_WORKER(type)
#else
        type==U_SCV
#endif
        ? 56 : 72;
    for (row = r - 5; row <= r + 5; row++)
        for (col = c - 5; col <= c + 5; col++)
            if (col >= 0 && row >= 0 && col < MAP_COLS && row < MAP_ROWS) {
                int dx = col * 16 + 8 - x;
                int dy = row * 16 + 8 - y;
                int cell = col + row * MAP_COLS;
                if (dx * dx + dy * dy <= radius * radius) {
                    if (delta > 0) {
                        if (sc64_visible_sources[cell] == 0)
                            sc64_minimap_visible[sc64_minimap_cell(cell)]++;
                        if (sc64_visible_sources[cell] != 0xFF)
                            sc64_visible_sources[cell]++;
                    } else if (sc64_visible_sources[cell] > 0) {
                        sc64_visible_sources[cell]--;
                        if (sc64_visible_sources[cell] == 0)
                            sc64_minimap_visible[sc64_minimap_cell(cell)]--;
                    }
                    visible[cell] = sc64_visible_sources[cell] != 0;
                }
            }
}

/* Update a nearby unit's old and new footprints together. Pixels can move
 * within a tile every frame, but only the thin symmetric difference of the
 * two sight circles needs source-count work. */
static void GBA_HOT
sc64_transition_unit_visibility(int old_x, int old_y, int new_x, int new_y,
                                int type)
{
    int radius =
#ifdef SC64_RACES
        IS_WORKER(type)
#else
        type==U_SCV
#endif
        ? 56 : 72;
    int old_col = old_x / 16, old_row = old_y / 16;
    int new_col = new_x / 16, new_row = new_y / 16;
    int col0 = (old_col < new_col ? old_col : new_col) - 5;
    int row0 = (old_row < new_row ? old_row : new_row) - 5;
    int col1 = (old_col > new_col ? old_col : new_col) + 5;
    int row1 = (old_row > new_row ? old_row : new_row) + 5;
    int col, row;
    for (row = row0; row <= row1; row++)
        for (col = col0; col <= col1; col++)
            if (col >= 0 && row >= 0 && col < MAP_COLS && row < MAP_ROWS) {
                int old_dx = col * 16 + 8 - old_x;
                int old_dy = row * 16 + 8 - old_y;
                int new_dx = col * 16 + 8 - new_x;
                int new_dy = row * 16 + 8 - new_y;
                int old_in = old_dx * old_dx + old_dy * old_dy <= radius * radius;
                int new_in = new_dx * new_dx + new_dy * new_dy <= radius * radius;
                if (old_in != new_in) {
                    int cell = col + row * MAP_COLS;
                    if (new_in) {
                        if (sc64_visible_sources[cell] == 0)
                            sc64_minimap_visible[sc64_minimap_cell(cell)]++;
                        if (sc64_visible_sources[cell] != 0xFF)
                            sc64_visible_sources[cell]++;
                    } else if (sc64_visible_sources[cell] > 0) {
                        sc64_visible_sources[cell]--;
                        if (sc64_visible_sources[cell] == 0)
                            sc64_minimap_visible[sc64_minimap_cell(cell)]--;
                    }
                    visible[cell] = sc64_visible_sources[cell] != 0;
                }
            }
}

static void GBA_HOT
sc64_adjust_building_visibility(int x, int y, int type, int delta)
{
    int col, row, c = x / 16, r = y / 16;
#ifdef SC64_RACES
    int radius=(type==B_COMMAND||type==B_HATCHERY||type==B_LAIR||type==B_NEXUS)?112:64;
#else
    if (type != B_COMMAND)return;
    int radius=112;
#endif
    for (row = r - 8; row <= r + 8; row++)
        for (col = c - 8; col <= c + 8; col++)
            if (col >= 0 && row >= 0 && col < MAP_COLS && row < MAP_ROWS) {
                int dx = col * 16 + 8 - x;
                int dy = row * 16 + 8 - y;
                int cell = col + row * MAP_COLS;
                if (dx * dx + dy * dy <= radius * radius) {
                    if (delta > 0) {
                        if (sc64_visible_sources[cell] == 0)
                            sc64_minimap_visible[sc64_minimap_cell(cell)]++;
                        if (sc64_visible_sources[cell] != 0xFF)
                            sc64_visible_sources[cell]++;
                    } else if (sc64_visible_sources[cell] > 0) {
                        sc64_visible_sources[cell]--;
                        if (sc64_visible_sources[cell] == 0)
                            sc64_minimap_visible[sc64_minimap_cell(cell)]--;
                    }
                    visible[cell] = sc64_visible_sources[cell] != 0;
                }
            }
}

static void GBA_HOT
sc64_update_exploration(void)
{
    int i;
    for (i = 0; i < MAX_UNITS; i++) {
        Sc64VisibilitySnapshot *old = &sc64_unit_visibility[i];
        int active = u[i].alive && u[i].owner == SIDE;
        int x = active ? u[i].x : 0;
        int y = active ? u[i].y : 0;
        int type = active ? u[i].type : 0;
        if (sc64_visibility_same(old, active, x, y, type))
            continue;
        if (old->active && active && old->type == type &&
            old->x / 16 == x / 16 && old->y / 16 == y / 16) {
            /* Exploration is tile-centered, so sub-tile motion adds nothing. */
        } else if (active) {
            sc64_explore_unit(x, y, type);
        }
        if (old->active && active && old->type == type &&
            old->x - x > -16 && old->x - x < 16 &&
            old->y - y > -16 && old->y - y < 16) {
            sc64_transition_unit_visibility(old->x, old->y, x, y, type);
        } else {
            if (old->active)
                sc64_adjust_unit_visibility(old->x, old->y, old->type, -1);
            if (active)
                sc64_adjust_unit_visibility(x, y, type, 1);
        }
        old->active = (u8)active;
        if (active) {
            old->x = x;
            old->y = y;
            old->type = type;
        }
    }
    for (i = 0; i < MAX_BUILDINGS; i++) {
        Sc64VisibilitySnapshot *old = &sc64_building_visibility[i];
        int active = b[i].alive && b[i].owner == SIDE;
        int x = active ? b[i].x : 0;
        int y = active ? b[i].y : 0;
        int type = active ? b[i].type : 0;
        if (sc64_visibility_same(old, active, x, y, type))
            continue;
        if (old->active)
            sc64_adjust_building_visibility(old->x, old->y, old->type, -1);
        if (active) {
            sc64_explore_building(x, y, type);
            sc64_adjust_building_visibility(x, y, type, 1);
        }
        old->active = (u8)active;
        if (active) {
            old->x = x;
            old->y = y;
            old->type = type;
        }
    }
}

#endif /* SC64_VISIBILITY_H */
