#ifndef SC64_VISIBILITY_H
#define SC64_VISIBILITY_H

#if defined(SC64_Z7_SIGHT_SPANS) && \
    (!defined(SC64_Z7_SOURCE_SIGHT) || !defined(SC64_VISIBILITY_SPANS))
#error "SC64_Z7_SIGHT_SPANS requires SC64_Z7_SOURCE_SIGHT and SC64_VISIBILITY_SPANS"
#endif

#ifdef SC64_Z7_SOURCE_SIGHT
#include "sc64_z7_source_sight.h"
#endif

#ifndef SC64_TRANSPORT_LOADED
#define SC64_TRANSPORT_LOADED(q) 0
#endif

#ifndef SC64_GRID_GET
#define SC64_GRID_GET(grid, cell) ((grid)[(cell)] != 0)
#define SC64_GRID_SET(grid, cell, value) ((grid)[(cell)] = (u8)((value) != 0))
#define SC64_GRID_CLEAR(grid) memset((grid), 0, sizeof(grid))
#define SC64_VISIBILITY_LOCAL_GRID_ACCESS
#endif

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
#if defined(SC64_LATE_TERRAN_STORAGE) || defined(SC64_ZP_CAMPAIGN_STORAGE)
    s16 x, y, type;
#else
    int x, y, type;
#endif
    u8 active;
} Sc64VisibilitySnapshot;

static u8 sc64_visible_sources[MAP_CELLS];
#if defined(SC64_LATE_TERRAN_STORAGE) || defined(SC64_ZP_CAMPAIGN_STORAGE)
/* Exact nine-bit counts avoid saturation when every actor shares one cell. */
static u8 sc64_visible_sources_high[(MAP_CELLS + 7) / 8];
_Static_assert(MAX_UNITS + MAX_BUILDINGS <= 511, "nine-bit source count bound");
#endif
static u8 sc64_minimap_visible[24 * 24];
static u8 sc64_minimap_explored[24 * 24];
static Sc64VisibilitySnapshot sc64_unit_visibility[MAX_UNITS];
static Sc64VisibilitySnapshot sc64_building_visibility[MAX_BUILDINGS];

static int GBA_HOT
sc64_minimap_cell(int cell)
{
#ifdef SC64_RACES
#if defined(SC64_LARGE_MAPS) || defined(SC64_ZP_CAMPAIGN_STORAGE)
    int col = sc64_cell_col(cell), row = sc64_cell_row(cell);
    int mx = race_cols == 64 ? (col * 3) >> 3 :
             race_cols == 128 ? (col * 3) >> 4 : col >> 2;
#if defined(SC64_T4_RUNTIME) || defined(SC64_ZP_CAMPAIGN_STORAGE)
    int my = race_rows == 64 ? (row * 3) >> 3 :
             race_rows == 128 ? (row * 3) >> 4 : row >> 2;
#else
    int my = race_rows == 64 ? (row * 3) >> 3 : row >> 2;
#endif
    return mx + my * 24;
#else
    int col=sc64_cell_col(cell), row=sc64_cell_row(cell);
    int mx = MAP_COLS == 64 ? (col * 3) >> 3 : col >> 2;
    int my = MAP_ROWS == 64 ? (row * 3) >> 3 : row >> 2;
    return mx + my * 24;
#endif
#else
    int col=cell%MAP_COLS,row=cell/MAP_COLS;
    int mx = col*24/MAP_COLS;
    int my = row*24/MAP_ROWS;
    return mx + my * 24;
#endif
}

#if defined(SC64_LATE_TERRAN_STORAGE) || defined(SC64_ZP_CAMPAIGN_STORAGE)
static unsigned
sc64_visibility_source_count(int cell)
{
    return sc64_visible_sources[cell] |
           ((unsigned)((sc64_visible_sources_high[(unsigned)cell >> 3] >>
                        ((unsigned)cell & 7)) & 1) << 8);
}

static void
sc64_visibility_source_change(int cell, int add)
{
    unsigned count = sc64_visibility_source_count(cell);
    if (add) {
        if (!count) ++sc64_minimap_visible[sc64_minimap_cell(cell)];
        ++count; /* At most one source per bounded unit/building slot. */
    } else if (count) {
        --count;
        if (!count) --sc64_minimap_visible[sc64_minimap_cell(cell)];
    }
    sc64_visible_sources[cell] = (u8)count;
    u8 mask = (u8)(1u << ((unsigned)cell & 7));
    if (count & 256u) sc64_visible_sources_high[(unsigned)cell >> 3] |= mask;
    else sc64_visible_sources_high[(unsigned)cell >> 3] &= (u8)~mask;
    SC64_GRID_SET(visible, cell, count != 0);
}
#endif

static void
sc64_visibility_reset(void)
{
    int i;
    memset(sc64_visible_sources, 0, sizeof(sc64_visible_sources));
#if defined(SC64_LATE_TERRAN_STORAGE) || defined(SC64_ZP_CAMPAIGN_STORAGE)
    memset(sc64_visible_sources_high, 0, sizeof(sc64_visible_sources_high));
#endif
    memset(sc64_minimap_visible, 0, sizeof(sc64_minimap_visible));
    memset(sc64_minimap_explored, 0, sizeof(sc64_minimap_explored));
    SC64_GRID_CLEAR(visible);
    /* `explored` is save-persistent, so rebuild aggregate occupancy from the
       restored full-resolution grid before sources are re-added. */
    for (i = 0; i < MAP_COLS * MAP_ROWS; i++)
        if (SC64_GRID_GET(explored, i))
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

#if defined(SC64_T4_RUNTIME) || defined(SC64_ZP_CAMPAIGN_ACTORS)
static int __attribute__((noinline))
sc64_actor_unit_sight_pixels(int type)
{
#ifdef SC64_Z7_SOURCE_SIGHT
    int z7_pixels = sc64_z7_source_unit_sight_pixels(type);
    if (z7_pixels) return z7_pixels;
#endif
#ifdef SC64_ZP_CAMPAIGN_ACTORS
    int pixels = sc64_zp_actor_sight_source_pixels(type);
    if (pixels) return pixels;
#endif
#ifdef SC64_T4_RUNTIME
    return sc64_race_t4_sight_source_pixels(type);
#else
    return 0;
#endif
}

static int __attribute__((noinline))
sc64_actor_building_sight_pixels(int type)
{
#ifdef SC64_Z7_SOURCE_SIGHT
    int z7_pixels = sc64_z7_source_building_sight_pixels(type);
    if (z7_pixels) return z7_pixels;
#endif
#ifdef SC64_ZP_CAMPAIGN_ACTORS
    int pixels = sc64_zp_building_sight_source_pixels(type);
    if (pixels) return pixels;
#endif
#ifdef SC64_T4_RUNTIME
    return sc64_race_t4_building_sight_source_pixels(type);
#else
    return 0;
#endif
}

static void __attribute__((noinline))
sc64_t4_explore_with_radius(int x, int y, int radius)
{
    int col,row,c=x/16,r=y/16;
    for(row=r-radius;row<=r+radius;++row)
        for(col=c-radius;col<=c+radius;++col)
            if(col>=0&&row>=0&&col<MAP_COLS&&row<MAP_ROWS&&
               (col-c)*(col-c)+(row-r)*(row-r)<=radius*radius) {
                int cell=col+row*MAP_COLS;
                if(!SC64_GRID_GET(explored,cell)) {
                    SC64_GRID_SET(explored,cell,1);
                    ++sc64_minimap_explored[sc64_minimap_cell(cell)];
                }
            }
}

#ifdef SC64_Z7_SIGHT_SPANS
static int sc64_z7_visibility_radius_index(int radius);
static void sc64_z7_adjust_visibility_source(int x, int y, int radius, int delta);
#endif

/* Special T4 sources use exact BW source-radius circles. Keep this bounded
 * but cold because the wider footprints change only when an actor changes. */
static void __attribute__((noinline))
sc64_t4_adjust_visibility_source(int x, int y, int radius, int delta)
{
#ifdef SC64_Z7_SIGHT_SPANS
    if (race_z7_active() && sc64_z7_visibility_radius_index(radius) >= 0) {
        sc64_z7_adjust_visibility_source(x, y, radius, delta);
        return;
    }
#endif
    int col, row, c = x / 16, r = y / 16;
    int extent = (radius + 23) / 16;
    for (row = r - extent; row <= r + extent; ++row)
        for (col = c - extent; col <= c + extent; ++col)
            if (col >= 0 && row >= 0 && col < MAP_COLS && row < MAP_ROWS) {
                int dx = col * 16 + 8 - x, dy = row * 16 + 8 - y;
                int cell = col + row * MAP_COLS;
                if (dx*dx + dy*dy <= radius*radius) {
#if defined(SC64_LATE_TERRAN_STORAGE) || defined(SC64_ZP_CAMPAIGN_STORAGE)
                    sc64_visibility_source_change(cell, delta > 0);
#else
                    if (delta > 0) {
                        if (!sc64_visible_sources[cell])
                            ++sc64_minimap_visible[sc64_minimap_cell(cell)];
                        if (sc64_visible_sources[cell] != 0xff) ++sc64_visible_sources[cell];
                    } else if (sc64_visible_sources[cell]) {
                        --sc64_visible_sources[cell];
                        if (!sc64_visible_sources[cell])
                            --sc64_minimap_visible[sc64_minimap_cell(cell)];
                    }
                    SC64_GRID_SET(visible, cell, sc64_visible_sources[cell] != 0);
#endif
                }
            }
}
#endif

static void GBA_HOT
sc64_explore_unit(int x, int y, int type)
{
#if defined(SC64_T4_RUNTIME) || defined(SC64_ZP_CAMPAIGN_ACTORS)
    int source_pixels=sc64_actor_unit_sight_pixels(type);
    if(source_pixels>0) {
        sc64_t4_explore_with_radius(x,y,source_pixels/32);
        return;
    }
#endif
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
                if (!SC64_GRID_GET(explored, cell)) {
                    SC64_GRID_SET(explored, cell, 1);
                    sc64_minimap_explored[sc64_minimap_cell(cell)]++;
                }
            }
}

static void GBA_HOT
sc64_explore_building(int x, int y, int type)
{
#if defined(SC64_T4_RUNTIME) || defined(SC64_ZP_CAMPAIGN_ACTORS)
    int source_pixels=sc64_actor_building_sight_pixels(type);
    if(source_pixels>0) {
        sc64_t4_explore_with_radius(x,y,source_pixels/32);
        return;
    }
#endif
    int col, row, c = x / 16, r = y / 16;
    int radius = type == B_COMMAND ? 7 : 3;
    for (row = r - radius; row <= r + radius; row++)
        for (col = c - radius; col <= c + radius; col++)
            if (col >= 0 && row >= 0 && col < MAP_COLS && row < MAP_ROWS &&
                (col - c) * (col - c) + (row - r) * (row - r) <= radius * radius) {
                int cell = col + row * MAP_COLS;
                if (!SC64_GRID_GET(explored, cell)) {
                    SC64_GRID_SET(explored, cell, 1);
                    sc64_minimap_explored[sc64_minimap_cell(cell)]++;
                }
            }
}

static void GBA_HOT
sc64_adjust_unit_visibility(int x, int y, int type, int delta)
{
#if defined(SC64_T4_RUNTIME) || defined(SC64_ZP_CAMPAIGN_ACTORS)
    int source_pixels=sc64_actor_unit_sight_pixels(type);
    if (source_pixels>0) {
        sc64_t4_adjust_visibility_source(x,y,source_pixels/2,delta);
        return;
    }
#endif
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
#if defined(SC64_LATE_TERRAN_STORAGE) || defined(SC64_ZP_CAMPAIGN_STORAGE)
                    sc64_visibility_source_change(cell, delta > 0);
#else
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
                    SC64_GRID_SET(visible, cell, sc64_visible_sources[cell] != 0);
#endif
                }
            }
}

#ifdef SC64_VISIBILITY_SPANS
/* floor(sqrt(radius^2 - dy^2)), indexed by absolute pixel dy.  These are
 * exact integer bounds for the two supported sight radii. */
static const u8 sc64_visibility_halfwidth_56[57] = {
    56,55,55,55,55,55,55,55,55,55,55,54,54,54,54,53,53,53,53,52,
    52,51,51,51,50,50,49,49,48,47,47,46,45,45,44,43,42,42,41,40,
    39,38,37,35,34,33,31,30,28,27,25,23,20,18,14,10,0
};
static const u8 sc64_visibility_halfwidth_72[73] = {
    72,71,71,71,71,71,71,71,71,71,71,71,70,70,70,70,70,69,69,69,
    69,68,68,68,67,67,67,66,66,65,65,64,64,63,63,62,62,61,61,60,
    59,59,58,57,56,56,55,54,53,52,51,50,49,48,47,46,45,43,42,41,
    39,38,36,34,32,30,28,26,23,20,16,11,0
};

#ifdef SC64_Z7_SIGHT_SPANS
#include "sc64_z7_visibility_spans.h"
#endif

static int
sc64_visibility_floor_div16(int value)
{
    return value >= 0 ? value / 16 : -((-value + 15) / 16);
}

static int
sc64_visibility_ceil_div16(int value)
{
    return value >= 0 ? (value + 15) / 16 : -((-value) / 16);
}

/* Derive exact row intervals in IWRAM without duplicating the helper body. */
static void GBA_HOT __attribute__((noinline))
sc64_visibility_row_span(int x, int y, int row, int radius, int extent,
                         int *first, int *last)
{
    int c = x / 16, r = y / 16;
    int col0, col1, dy, halfwidth;
    *first = 1;
    *last = 0;
    if (row < r - extent || row > r + extent || row < 0 || row >= MAP_ROWS)
        return;
    dy = row * 16 + 8 - y;
    if (dy < 0) dy = -dy;
    if (dy > radius) return;
    halfwidth = radius == 56 ? sc64_visibility_halfwidth_56[dy] :
                               sc64_visibility_halfwidth_72[dy];
    col0 = sc64_visibility_ceil_div16(x - halfwidth - 8);
    col1 = sc64_visibility_floor_div16(x + halfwidth - 8);
    if (col0 < c - extent) col0 = c - extent;
    if (col1 > c + extent) col1 = c + extent;
    if (col0 < 0) col0 = 0;
    if (col1 >= MAP_COLS) col1 = MAP_COLS - 1;
    if (col0 > col1) return;
    *first = col0;
    *last = col1;
}

static void GBA_HOT __attribute__((noinline))
sc64_visibility_apply_span(int row, int first, int last, int add)
{
    int col;
    for (col = first; col <= last; ++col) {
        int cell = col + row * MAP_COLS;
#if defined(SC64_LATE_TERRAN_STORAGE) || defined(SC64_ZP_CAMPAIGN_STORAGE)
        sc64_visibility_source_change(cell, add);
#else
        if (add) {
            if (sc64_visible_sources[cell] == 0)
                sc64_minimap_visible[sc64_minimap_cell(cell)]++;
            if (sc64_visible_sources[cell] != 0xFF)
                sc64_visible_sources[cell]++;
        } else if (sc64_visible_sources[cell] > 0) {
            sc64_visible_sources[cell]--;
            if (sc64_visible_sources[cell] == 0)
                sc64_minimap_visible[sc64_minimap_cell(cell)]--;
        }
        SC64_GRID_SET(visible, cell, sc64_visible_sources[cell] != 0);
#endif
    }
}
#ifdef SC64_Z7_SIGHT_SPANS
/* Exact source-radius row spans for authored Z7 unit/building sight radii.
 * The LUTs contain floor(sqrt(radius^2-dy^2)); they stay in ROM. */
static int
sc64_z7_visibility_radius_index(int radius)
{
    switch (radius) {
    case 64: return 0;
    case 80: return 1;
    case 96: return 2;
    case 112: return 3;
    case 128: return 4;
    case 144: return 5;
    case 160: return 6;
    case 176: return 7;
    default: return -1;
    }
}

static void __attribute__((noinline))
sc64_z7_visibility_row_span(int x, int y, int row, int radius, int extent,
                            int *first, int *last)
{
    int c = x / 16, r = y / 16;
    int col0, col1, dy, halfwidth, index;
    *first = 1;
    *last = 0;
    if (row < r - extent || row > r + extent || row < 0 || row >= MAP_ROWS)
        return;
    dy = row * 16 + 8 - y;
    if (dy < 0) dy = -dy;
    index = sc64_z7_visibility_radius_index(radius);
    if (index < 0 || dy > radius) return;
    halfwidth = sc64_z7_visibility_halfwidth(index, dy);
    col0 = sc64_visibility_ceil_div16(x - halfwidth - 8);
    col1 = sc64_visibility_floor_div16(x + halfwidth - 8);
    if (col0 < c - extent) col0 = c - extent;
    if (col1 > c + extent) col1 = c + extent;
    if (col0 < 0) col0 = 0;
    if (col1 >= MAP_COLS) col1 = MAP_COLS - 1;
    if (col0 > col1) return;
    *first = col0;
    *last = col1;
}

static void __attribute__((noinline))
sc64_z7_adjust_visibility_source(int x, int y, int radius, int delta)
{
    int row, extent = (radius + 23) / 16;
    for (row = y / 16 - extent; row <= y / 16 + extent; ++row) {
        int first, last;
        sc64_z7_visibility_row_span(x, y, row, radius, extent, &first, &last);
        if (first <= last)
            sc64_visibility_apply_span(row, first, last, delta > 0);
    }
}

static void __attribute__((noinline))
sc64_z7_transition_visibility(int old_x, int old_y, int new_x, int new_y,
                              int radius)
{
    int extent = (radius + 23) / 16;
    int old_row = old_y / 16, new_row = new_y / 16;
    int row, row0 = (old_row < new_row ? old_row : new_row) - extent;
    int row1 = (old_row > new_row ? old_row : new_row) + extent;
    for (row = row0; row <= row1; ++row) {
        int of, ol, nf, nl;
        sc64_z7_visibility_row_span(old_x, old_y, row, radius, extent, &of, &ol);
        sc64_z7_visibility_row_span(new_x, new_y, row, radius, extent, &nf, &nl);
        if (of > ol) {
            sc64_visibility_apply_span(row, nf, nl, 1);
        } else if (nf > nl) {
            sc64_visibility_apply_span(row, of, ol, 0);
        } else {
            if (of < nf)
                sc64_visibility_apply_span(row, of, ol < nf - 1 ? ol : nf - 1, 0);
            if (ol > nl)
                sc64_visibility_apply_span(row, of > nl + 1 ? of : nl + 1, ol, 0);
            if (nf < of)
                sc64_visibility_apply_span(row, nf, nl < of - 1 ? nl : of - 1, 1);
            if (nl > ol)
                sc64_visibility_apply_span(row, nf > ol + 1 ? nf : ol + 1, nl, 1);
        }
    }
}
#endif

#endif

/* Update a nearby unit's old and new footprints together. Pixels can move
 * within a tile every frame, but only the thin symmetric difference of the
 * two sight circles needs source-count work. */
static void GBA_HOT
sc64_transition_unit_visibility(int old_x, int old_y, int new_x, int new_y,
                                int type)
{
#if defined(SC64_T4_RUNTIME) || defined(SC64_ZP_CAMPAIGN_ACTORS)
#ifdef SC64_Z7_SIGHT_SPANS
    int source_pixels = sc64_actor_unit_sight_pixels(type);
    if (source_pixels>0) {
        if (old_x != new_x || old_y != new_y) {
            int radius=source_pixels/2;
            if (race_z7_active() && sc64_z7_visibility_radius_index(radius) >= 0) {
                sc64_z7_transition_visibility(old_x, old_y, new_x, new_y, radius);
                return;
            }
            sc64_t4_adjust_visibility_source(old_x,old_y,radius,-1);
            sc64_t4_adjust_visibility_source(new_x,new_y,radius,1);
        }
        return;
    }
#else
    if (sc64_actor_unit_sight_pixels(type)>0) {
        if (old_x != new_x || old_y != new_y) {
            int radius=sc64_actor_unit_sight_pixels(type)/2;
            sc64_t4_adjust_visibility_source(old_x,old_y,radius,-1);
            sc64_t4_adjust_visibility_source(new_x,new_y,radius,1);
        }
        return;
    }
#endif
#endif
    int radius =
#ifdef SC64_RACES
        IS_WORKER(type)
#else
        type==U_SCV
#endif
        ? 56 : 72;
#ifdef SC64_VISIBILITY_SPANS
    int old_row = old_y / 16, new_row = new_y / 16;
    int row;
#else
    int old_col = old_x / 16, old_row = old_y / 16;
    int new_col = new_x / 16, new_row = new_y / 16;
    int col0 = (old_col < new_col ? old_col : new_col) - 5;
    int row0 = (old_row < new_row ? old_row : new_row) - 5;
    int col1 = (old_col > new_col ? old_col : new_col) + 5;
    int row1 = (old_row > new_row ? old_row : new_row) + 5;
    int col, row;
#endif
#ifdef SC64_VISIBILITY_SPANS
    int row0 = (old_row < new_row ? old_row : new_row) - 5;
    int row1 = (old_row > new_row ? old_row : new_row) + 5;
    for (row = row0; row <= row1; ++row) {
        int old_first, old_last, new_first, new_last;
        sc64_visibility_row_span(old_x, old_y, row, radius, 5,
                                 &old_first, &old_last);
        sc64_visibility_row_span(new_x, new_y, row, radius, 5,
                                 &new_first, &new_last);
        if (old_first > old_last) {
            sc64_visibility_apply_span(row, new_first, new_last, 1);
        } else if (new_first > new_last) {
            sc64_visibility_apply_span(row, old_first, old_last, 0);
        } else {
            if (old_first < new_first)
                sc64_visibility_apply_span(row, old_first,
                    old_last < new_first - 1 ? old_last : new_first - 1, 0);
            if (old_last > new_last)
                sc64_visibility_apply_span(row,
                    old_first > new_last + 1 ? old_first : new_last + 1,
                    old_last, 0);
            if (new_first < old_first)
                sc64_visibility_apply_span(row, new_first,
                    new_last < old_first - 1 ? new_last : old_first - 1, 1);
            if (new_last > old_last)
                sc64_visibility_apply_span(row,
                    new_first > old_last + 1 ? new_first : old_last + 1,
                    new_last, 1);
        }
    }
#else
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
#if defined(SC64_LATE_TERRAN_STORAGE) || defined(SC64_ZP_CAMPAIGN_STORAGE)
                    sc64_visibility_source_change(cell, new_in);
#else
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
                    SC64_GRID_SET(visible, cell, sc64_visible_sources[cell] != 0);
#endif
                }
            }
#endif
}

static void GBA_HOT
sc64_adjust_building_visibility(int x, int y, int type, int delta)
{
#if defined(SC64_T4_RUNTIME) || defined(SC64_ZP_CAMPAIGN_ACTORS)
    int source_pixels=sc64_actor_building_sight_pixels(type);
    if(source_pixels>0) {
        sc64_t4_adjust_visibility_source(x,y,source_pixels/2,delta);
        return;
    }
#endif
    int col, row, c = x / 16, r = y / 16;
#ifdef SC64_RACES
    int radius=(type==B_COMMAND||sc64_race_is_zerg_townhall(type)||type==B_NEXUS)?112:64;
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
#if defined(SC64_LATE_TERRAN_STORAGE) || defined(SC64_ZP_CAMPAIGN_STORAGE)
                    sc64_visibility_source_change(cell, delta > 0);
#else
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
                    SC64_GRID_SET(visible, cell, sc64_visible_sources[cell] != 0);
#endif
                }
            }
}

static void GBA_HOT
sc64_update_exploration(void)
{
    int i;
    for (i = 0; i < MAX_UNITS; i++) {
        Sc64VisibilitySnapshot *old = &sc64_unit_visibility[i];
        int active = u[i].alive && u[i].owner == SIDE &&
#ifdef SC64_P7_CARRIER_TRAINING_ENABLED
                     !SC64_CARRIER_HANGAR_HIDDEN(&u[i]) &&
#endif
                     !SC64_TRANSPORT_LOADED(&u[i]);
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
#ifdef SC64_Z7_SOURCE_SIGHT
        int sight_key = active ? sc64_z7_building_visibility_key(&b[i]) : 0;
        if (sc64_visibility_same(old, active, x, y, sight_key))
#else
        if (sc64_visibility_same(old, active, x, y, type))
#endif
            continue;
        if (old->active)
            sc64_adjust_building_visibility(old->x, old->y, old->type, -1);
        if (active) {
#ifdef SC64_Z7_SOURCE_SIGHT
            sc64_explore_building(x, y, sight_key);
            sc64_adjust_building_visibility(x, y, sight_key, 1);
#else
            sc64_explore_building(x, y, type);
            sc64_adjust_building_visibility(x, y, type, 1);
#endif
        }
        old->active = (u8)active;
        if (active) {
            old->x = x;
            old->y = y;
#ifdef SC64_Z7_SOURCE_SIGHT
            old->type = sight_key;
#else
            old->type = type;
#endif
        }
    }
}

#ifdef SC64_VISIBILITY_LOCAL_GRID_ACCESS
#undef SC64_GRID_GET
#undef SC64_GRID_SET
#undef SC64_GRID_CLEAR
#undef SC64_VISIBILITY_LOCAL_GRID_ACCESS
#endif
#endif /* SC64_VISIBILITY_H */
