#ifndef SC64_COLLISION_H
#define SC64_COLLISION_H

_Static_assert(MAX_UNITS < 255, "Spatial unit indices must fit below the sentinel");

#if defined(SC64_COLLISION_CACHE_ENABLED) && !defined(SC64_RACES)
#error SC64_COLLISION_CACHE_ENABLED requires SC64_RACES
#endif

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

#ifdef SC64_COLLISION_CACHE_ENABLED
/* move_toward checks up to eight adjacent steering steps for one unit. Cache
 * the exact local actor set once, then reuse it for those nearby candidates.
 * A move anywhere invalidates the snapshot before it can become stale. */
#define SC64_COLLISION_CANDIDATES 32
typedef struct Sc64CollisionCandidate {
    u8 slot;
    s16 x, y;
    u8 old_distance2;
} Sc64CollisionCandidate;
static Sc64CollisionCandidate sc64_collision_candidates[SC64_COLLISION_CANDIDATES];
static u8 sc64_collision_candidate_count;
static u8 sc64_collision_candidate_q;
static s16 sc64_collision_candidate_qx, sc64_collision_candidate_qy;
static int sc64_collision_candidate_type;
static int sc64_collision_candidate_valid;
#endif

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

static void GBA_HOT
sc64_unit_buckets_begin(void)
{
    int i;
#ifdef SC64_COLLISION_CACHE_ENABLED
    sc64_collision_candidate_valid = 0;
#endif
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
        if (u[i].alive) {
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
#ifdef SC64_COLLISION_CACHE_ENABLED
    /* This must precede even the inactive and same-cell early returns: a
     * neighbor can move within its current bucket and still change a result. */
    sc64_collision_candidate_valid = 0;
#endif
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

/* Keep cache construction and iteration out of the IWRAM hot path. */
#ifdef SC64_COLLISION_CACHE_ENABLED
static int __attribute__((noinline))
sc64_unit_bucket_cached_step_clear(const Unit *q, int nx, int ny)
{
    int qslot = (int)(q - u);
    int reusable = qslot >= 0 && qslot < MAX_UNITS &&
        sc64_collision_candidate_valid &&
        sc64_collision_candidate_q == (u8)qslot &&
        sc64_collision_candidate_qx == q->x &&
        sc64_collision_candidate_qy == q->y &&
        sc64_collision_candidate_type == q->type;
    if (!reusable && qslot >= 0 && qslot < MAX_UNITS) {
        int minx = (q->x - 8) >> 4, maxx = (q->x + 8) >> 4;
        int miny = (q->y - 8) >> 4, maxy = (q->y + 8) >> 4;
        int row, col, overflow = 0;
        unsigned int count = 0;
        if (minx < 0) minx = 0;
        if (miny < 0) miny = 0;
        if (maxx >= MAP_COLS) maxx = MAP_COLS - 1;
        if (maxy >= MAP_ROWS) maxy = MAP_ROWS - 1;
        for (row = miny; row <= maxy && !overflow; ++row)
            for (col = minx; col <= maxx && !overflow; ++col) {
                int i;
                for (i = sc64_unit_bucket_head[col + row * MAP_COLS];
                     i != 0xff; i = sc64_unit_bucket_next[i])
                    if (i != qslot && u[i].alive &&
                        u[i].type != U_CHRYSALIS &&
                        sc64_race_unit_flying(u[i].type) ==
                            sc64_race_unit_flying(q->type)) {
                        int dx = u[i].x - q->x;
                        int dy = u[i].y - q->y;
                        if (dx < -7 || dx > 7 || dy < -7 || dy > 7)
                            continue;
                        if (count >= SC64_COLLISION_CANDIDATES) {
                            overflow = 1;
                            break;
                        }
                        sc64_collision_candidates[count].slot = (u8)i;
                        sc64_collision_candidates[count].x = (s16)u[i].x;
                        sc64_collision_candidates[count].y = (s16)u[i].y;
                        sc64_collision_candidates[count].old_distance2 =
                            (u8)(dx * dx + dy * dy);
                        ++count;
                    }
            }
        if (!overflow) {
            sc64_collision_candidate_count = (u8)count;
            sc64_collision_candidate_q = (u8)qslot;
            sc64_collision_candidate_qx = (s16)q->x;
            sc64_collision_candidate_qy = (s16)q->y;
            sc64_collision_candidate_type = q->type;
            sc64_collision_candidate_valid = 1;
            reusable = 1;
        } else {
            sc64_collision_candidate_valid = 0;
        }
    }
    if (!reusable) return -1;
    {
        unsigned int i;
        for (i = 0; i < sc64_collision_candidate_count; ++i) {
            const Sc64CollisionCandidate *candidate =
                &sc64_collision_candidates[i];
            int dx, dy, next_distance2;
            if (!u[candidate->slot].alive) continue;
            dx = candidate->x - nx;
            dy = candidate->y - ny;
            if (dx <= -6 || dx >= 6 || dy <= -6 || dy >= 6) continue;
            next_distance2 = dx * dx + dy * dy;
            if (next_distance2 < 36 &&
                next_distance2 <= candidate->old_distance2) return 0;
        }
    }
    return 1;
}
#endif

static int GBA_HOT
sc64_unit_bucket_step_clear(const Unit *q, int nx, int ny)
{
#ifdef SC64_RACES
    if(IS_WORKER(q->type)&&(q->order==ORDER_MINE||q->order==ORDER_GAS))return 1;
#endif
#ifdef SC64_COLLISION_CACHE_ENABLED
    if (sc64_unit_bucket_active && nx >= 0 && ny >= 0 &&
        nx < WORLD_W && ny < WORLD_H &&
        nx >= q->x - 2 && nx <= q->x + 2 &&
        ny >= q->y - 2 && ny <= q->y + 2) {
        int cached = sc64_unit_bucket_cached_step_clear(q, nx, ny);
        if (cached >= 0) return cached;
    }
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
                if (&u[i] != q && u[i].alive) {
#ifdef SC64_RACES
                    if(u[i].type==U_CHRYSALIS||sc64_race_unit_flying(u[i].type)!=sc64_race_unit_flying(q->type))continue;
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
#ifdef SC64_COLLISION_CACHE_ENABLED
    sc64_collision_candidate_valid = 0;
#endif
}

#endif
