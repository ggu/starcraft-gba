#ifndef SC64_COLLISION_H
#define SC64_COLLISION_H

_Static_assert(MAX_UNITS < 255, "Spatial unit indices must fit below the sentinel");

/* Exact broad-phase index for moving units. Built only for update_orders();
 * all candidates still use the engine's original overlap predicate. */
static u8 sc64_unit_bucket_head[MAP_CELLS];
static u8 sc64_unit_bucket_next[MAX_UNITS];
static u8 sc64_unit_bucket_prev[MAX_UNITS];
static u16 sc64_unit_bucket_cell[MAX_UNITS];
static u16 sc64_unit_bucket_dirty[MAX_UNITS * 2];
static u16 sc64_unit_bucket_dirty_count;
static int sc64_unit_bucket_initialized;
static int sc64_unit_bucket_active;

static int GBA_HOT
sc64_unit_bucket_cell_at(int x, int y)
{
    return (x >> 4) + (y >> 4) * MAP_COLS;
}

static void GBA_HOT
sc64_unit_bucket_insert(int slot, int cell)
{
    u8 head = sc64_unit_bucket_head[cell];
    if (head == 0xff) {
        if (sc64_unit_bucket_dirty_count >= MAX_UNITS * 2) {
            sc64_unit_bucket_active = 0;
            return;
        }
        sc64_unit_bucket_dirty[sc64_unit_bucket_dirty_count++] = (u16)cell;
    }
    sc64_unit_bucket_prev[slot] = 0xff;
    sc64_unit_bucket_next[slot] = head;
    if (head != 0xff) sc64_unit_bucket_prev[head] = (u8)slot;
    sc64_unit_bucket_head[cell] = (u8)slot;
    sc64_unit_bucket_cell[slot] = (u16)cell;
}

#ifdef SC64_T5_RUNTIME
/* Once-per-frame setup stays in ROM to preserve the unchanged IWRAM stack
 * reserve as transport checks expand the hot actor loop. */
static void __attribute__((noinline))
#else
static void GBA_HOT
#endif
sc64_unit_buckets_begin(void)
{
    int i;
    if (!sc64_unit_bucket_initialized) {
        memset(sc64_unit_bucket_head, 0xff, sizeof(sc64_unit_bucket_head));
        sc64_unit_bucket_initialized = 1;
    } else {
        for (i = 0; i < sc64_unit_bucket_dirty_count; ++i)
            sc64_unit_bucket_head[sc64_unit_bucket_dirty[i]] = 0xff;
    }
    sc64_unit_bucket_dirty_count = 0;
    sc64_unit_bucket_active = 0;
    for (i = 0; i < MAX_UNITS; ++i) {
        sc64_unit_bucket_next[i] = 0xff;
        sc64_unit_bucket_prev[i] = 0xff;
    }
    for (i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && !SC64_TRANSPORT_LOADED(&u[i])) {
            int x = u[i].x, y = u[i].y;
            if (x < 0 || y < 0 || x >= WORLD_W || y >= WORLD_H) return;
            sc64_unit_bucket_insert(i, sc64_unit_bucket_cell_at(x, y));
            if (!sc64_unit_bucket_active && sc64_unit_bucket_dirty_count >= MAX_UNITS * 2) return;
        }
    sc64_unit_bucket_active = 1;
}

static void GBA_HOT
sc64_unit_bucket_move(Unit *q)
{
    int slot = (int)(q - u), old_cell, new_cell;
    u8 before, after;
    if (!sc64_unit_bucket_active || slot < 0 || slot >= MAX_UNITS) return;
    old_cell = sc64_unit_bucket_cell[slot];
    new_cell = sc64_unit_bucket_cell_at(q->x, q->y);
    if (old_cell == new_cell) return;
    before = sc64_unit_bucket_prev[slot];
    after = sc64_unit_bucket_next[slot];
    if (before == 0xff) sc64_unit_bucket_head[old_cell] = after;
    else sc64_unit_bucket_next[before] = after;
    if (after != 0xff) sc64_unit_bucket_prev[after] = before;
    sc64_unit_bucket_insert(slot, new_cell);
}

static int GBA_HOT
sc64_unit_bucket_step_clear(const Unit *q, int nx, int ny)
{
#ifdef SC64_RACES
    if(IS_WORKER(q->type)&&(q->order==ORDER_MINE||q->order==ORDER_GAS))return 1;
#endif
    int minx = (nx - 5) >> 4, maxx = (nx + 5) >> 4;
    int miny = (ny - 5) >> 4, maxy = (ny + 5) >> 4;
    int row, col;
    if (minx < 0) minx = 0;
    if (miny < 0) miny = 0;
    if (maxx >= MAP_COLS) maxx = MAP_COLS - 1;
    if (maxy >= MAP_ROWS) maxy = MAP_ROWS - 1;
    for (row = miny; row <= maxy; ++row)
        for (col = minx; col <= maxx; ++col) {
            int i;
            for (i = sc64_unit_bucket_head[col + row * MAP_COLS]; i != 0xff;
                 i = sc64_unit_bucket_next[i])
                if (&u[i] != q && u[i].alive && !SC64_TRANSPORT_LOADED(&u[i])) {
#ifdef SC64_RACES
                    if(u[i].type==U_CHRYSALIS||sc64_race_unit_flying(u[i].type)!=sc64_race_unit_flying(q->type))continue;
#ifdef SC64_T4_RUNTIME
                    if ((sc64_race_t4_actor_flags(u[i].type) &&
                         !(sc64_race_t4_actor_flags(u[i].type)&SC64_T4_ACTOR_COLLIDABLE)) ||
                        (sc64_race_t4_actor_flags(q->type) &&
                         !(sc64_race_t4_actor_flags(q->type)&SC64_T4_ACTOR_COLLIDABLE))) continue;
#endif
#endif
                    int dx = u[i].x - nx, dy = u[i].y - ny;
                    if (dx <= -6 || dx >= 6 || dy <= -6 || dy >= 6) continue;
                    dx *= dx;
                    dy *= dy;
                    if (dx + dy < 36) {
                        int ox = u[i].x - q->x, oy = u[i].y - q->y;
                        if (dx + dy <= ox * ox + oy * oy) return 0;
                    }
                }
        }
    return 1;
}

static void
sc64_unit_buckets_end(void)
{
    sc64_unit_bucket_active = 0;
}

#endif
