#ifndef SC64_TARGETS_H
#define SC64_TARGETS_H

/* Short-lived 64px spatial index for nearest-enemy queries in update_orders.
 * The default 24x24 grid covers 96x96; SC64_LARGE_MAPS uses 32x24 for
 * 128x96; the T4 storage probe uses 32x32 for 128x128. Entries are compact
 * unified indices: units, then buildings. */
#define SC64_TARGET_CELL_SIZE 64
#ifdef SC64_LARGE_MAPS
#define SC64_TARGET_GRID_COLS 32
#ifdef SC64_T4_RUNTIME
#define SC64_TARGET_GRID_ROWS 32
#else
#define SC64_TARGET_GRID_ROWS 24
#endif
#else
#define SC64_TARGET_GRID_COLS 24
#define SC64_TARGET_GRID_ROWS 24
#endif
#define SC64_TARGET_CELL_COUNT (SC64_TARGET_GRID_COLS * SC64_TARGET_GRID_ROWS)
#define SC64_TARGET_ENTITY_COUNT (MAX_UNITS + MAX_BUILDINGS)
#define SC64_TARGET_DIRTY_CAPACITY (MAX_UNITS * 2 + MAX_BUILDINGS)

_Static_assert(SC64_TARGET_ENTITY_COUNT < 255,
               "Target index sentinel must not overlap an entity slot");

static u8 sc64_target_head[5][SC64_TARGET_CELL_COUNT];
static u8 sc64_target_next[SC64_TARGET_ENTITY_COUNT];
static u8 sc64_target_prev[SC64_TARGET_ENTITY_COUNT];
static u16 sc64_target_cell[SC64_TARGET_ENTITY_COUNT];
static u16 sc64_target_dirty[SC64_TARGET_DIRTY_CAPACITY];
static u16 sc64_target_dirty_count;
static int sc64_target_index_initialized;
static int sc64_target_dirty_overflowed;
static int sc64_target_index_active;

static int GBA_HOT
sc64_target_cell_for(int x, int y, int *cell)
{
    int col, row, cols = (WORLD_W + SC64_TARGET_CELL_SIZE - 1) / SC64_TARGET_CELL_SIZE;
    int rows = (WORLD_H + SC64_TARGET_CELL_SIZE - 1) / SC64_TARGET_CELL_SIZE;
    if (x < 0 || y < 0 || x >= WORLD_W || y >= WORLD_H ||
        cols > SC64_TARGET_GRID_COLS || rows > SC64_TARGET_GRID_ROWS)
        return 0;
    col = x / SC64_TARGET_CELL_SIZE;
    row = y / SC64_TARGET_CELL_SIZE;
    *cell = col + row * SC64_TARGET_GRID_COLS;
    return 1;
}

static int GBA_HOT
sc64_target_insert(int entity, int owner, int x, int y)
{
    int cell;
    u8 old;
    if (owner < 0 || owner >= 5 || !sc64_target_cell_for(x, y, &cell)) return 0;
    old = sc64_target_head[owner][cell];
    if (old == 0xff) {
        if (sc64_target_dirty_count >= SC64_TARGET_DIRTY_CAPACITY) {
            sc64_target_dirty_overflowed = 1;
            sc64_target_index_active = 0;
            return 0;
        }
        sc64_target_dirty[sc64_target_dirty_count++] =
            (u16)(owner * SC64_TARGET_CELL_COUNT + cell);
    }
    sc64_target_next[entity] = old;
    sc64_target_prev[entity] = 0xff;
    if (old != 0xff) sc64_target_prev[old] = (u8)entity;
    sc64_target_head[owner][cell] = (u8)entity;
    sc64_target_cell[entity] = (u16)cell;
    return 1;
}

static void
sc64_target_remove(int entity, int owner)
{
    int cell = sc64_target_cell[entity];
    u8 before = sc64_target_prev[entity], after = sc64_target_next[entity];
    if (before == 0xff) sc64_target_head[owner][cell] = after;
    else sc64_target_next[before] = after;
    if (after != 0xff) sc64_target_prev[after] = before;
    sc64_target_next[entity] = sc64_target_prev[entity] = 0xff;
}

static void
sc64_target_index_begin(void)
{
    int i;
    sc64_target_index_active = 0;
    if (!sc64_target_index_initialized || sc64_target_dirty_overflowed) {
        memset(sc64_target_head, 0xff, sizeof(sc64_target_head));
        sc64_target_index_initialized = 1;
    } else {
        for (i = 0; i < sc64_target_dirty_count; ++i) {
            /* The stored owner-cell code is already a byte offset into the
             * complete head array; avoid software division on ARM7TDMI. */
            ((u8 *)sc64_target_head)[sc64_target_dirty[i]] = 0xff;
        }
    }
    sc64_target_dirty_count = 0;
    sc64_target_dirty_overflowed = 0;
    for (i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && !sc64_target_insert(i, u[i].owner, u[i].x, u[i].y)) return;
    for (i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && !sc64_target_insert(MAX_UNITS + i, b[i].owner, b[i].x, b[i].y)) return;
    sc64_target_index_active = 1;
}

static void GBA_HOT
sc64_target_index_move(Unit *q)
{
    int entity = (int)(q - u), cell;
    if (!sc64_target_index_active || entity < 0 || entity >= MAX_UNITS) return;
    if (!sc64_target_cell_for(q->x, q->y, &cell)) {
        sc64_target_index_active = 0;
        return;
    }
    if (sc64_target_cell[entity] == cell) return;
    sc64_target_remove(entity, q->owner);
    if (!sc64_target_insert(entity, q->owner, q->x, q->y)) sc64_target_index_active = 0;
}

static void
sc64_target_index_end(void)
{
    sc64_target_index_active = 0;
}

#endif
