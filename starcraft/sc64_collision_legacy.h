#ifndef SC64_COLLISION_LEGACY_H
#define SC64_COLLISION_LEGACY_H

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

#if defined(SC64_COLLISION_CLASS_CACHE)
#if !defined(SC64_ZP_CAMPAIGN_ACTORS) || !defined(SC64_RACES)
#error "SC64_COLLISION_CLASS_CACHE requires Z/P campaign actor facts"
#endif
/* Per bucket phase, cache only immutable type-derived collision facts.  The
 * six radius bits and two flags occupy one byte; zero is the uncached
 * sentinel (the only unrepresentable fact tuple is radius=0/flying=0/
 * collidable=0, which is safely recomputed). */
static u8 sc64_unit_collision_class_cache[MAX_UNITS];

static void
sc64_unit_collision_class_invalidate(int slot)
{
    if (slot >= 0 && slot < MAX_UNITS)
        sc64_unit_collision_class_cache[slot] = 0;
}

static int GBA_HOT
sc64_unit_collision_class_values(int type, int *radius, int *flying,
                                 int *collidable)
{
    unsigned flags;
    *radius = sc64_game_unit_collision_radius(type);
    *flying = sc64_race_unit_flying(type);
    flags = sc64_game_unit_actor_flags(type);
    *collidable = !flags || (flags & SC64_GAME_ACTOR_COLLIDABLE);
    return *radius >= 0 && *radius < 64;
}

static u8 GBA_HOT
sc64_unit_collision_class_pack(int radius, int flying, int collidable)
{
    if (radius < 0 || radius >= 64) return 0;
    return (u8)((radius << 2) | (!!flying << 1) | !!collidable);
}

static u8 GBA_HOT
sc64_unit_collision_class(int slot, int type, int *radius, int *flying,
                          int *collidable)
{
    u8 packed = slot >= 0 && slot < MAX_UNITS
        ? sc64_unit_collision_class_cache[slot] : 0;
    if (packed) {
        *radius = packed >> 2;
        *flying = (packed >> 1) & 1;
        *collidable = packed & 1;
        return packed;
    }
    if (!sc64_unit_collision_class_values(type, radius, flying, collidable))
        return 0;
    packed = sc64_unit_collision_class_pack(*radius, *flying, *collidable);
    if (packed && slot >= 0 && slot < MAX_UNITS)
        sc64_unit_collision_class_cache[slot] = packed;
    return packed;
}
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

#ifdef SC64_T5_RUNTIME
/* Once-per-frame setup stays in ROM to preserve the unchanged IWRAM stack
 * reserve as transport checks expand the hot actor loop. */
static void __attribute__((noinline))
#elif defined(SC64_FACT_TABLES)
/* The smaller table-backed update_orders() would otherwise absorb this
 * once-per-frame IWRAM setup into its ROM Thumb body. */
static void GBA_HOT __attribute__((noinline))
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
#ifdef SC64_COLLISION_CLASS_CACHE
    memset(sc64_unit_collision_class_cache, 0,
           sizeof(sc64_unit_collision_class_cache));
#endif
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
#if defined(SC64_COLLISION_CLASS_CACHE)
    int mover_radius, mover_flying, mover_collidable;
    (void)sc64_unit_collision_class((int)(q - u), q->type,
        &mover_radius, &mover_flying, &mover_collidable);
#elif defined(SC64_COLLISION_FACT_HOIST) && defined(SC64_ZP_CAMPAIGN_ACTORS)
    /* These mover facts are immutable during this read-only collision query.
     * Resolve them once instead of repeating ROM fact lookups per candidate. */
    int mover_radius = sc64_game_unit_collision_radius(q->type);
    int mover_flying = sc64_race_unit_flying(q->type);
    unsigned mover_flags = sc64_game_unit_actor_flags(q->type);
    int mover_collidable = !mover_flags ||
        (mover_flags & SC64_GAME_ACTOR_COLLIDABLE);
#endif
#endif
    /* The audited Z/P roster has a maximum ten-pixel collision radius.
     * Search every cell that can contain a point strictly inside it. */
#ifdef SC64_ZP_CAMPAIGN_ACTORS
    const int reach = 9;
#else
    const int reach = 5;
#endif
    int minx = (nx - reach) >> 4, maxx = (nx + reach) >> 4;
    int miny = (ny - reach) >> 4, maxy = (ny + reach) >> 4;
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
#ifdef SC64_P6_COMBAT_ENABLED
                    if (race_p6_combat_unit_burrowed(i)) continue;
#endif
#ifdef SC64_ZP_CAMPAIGN_ACTORS
#if defined(SC64_COLLISION_CLASS_CACHE)
                    int candidate_radius, candidate_flying, candidate_collidable;
                    (void)sc64_unit_collision_class(i, u[i].type,
                        &candidate_radius, &candidate_flying,
                        &candidate_collidable);
                    int radius = mover_radius > candidate_radius ? mover_radius : candidate_radius;
#elif defined(SC64_COLLISION_FACT_HOIST) && defined(SC64_RACES)
                    int candidate_radius = sc64_game_unit_collision_radius(u[i].type);
                    int radius = mover_radius > candidate_radius ? mover_radius : candidate_radius;
#else
                    int mover_radius = sc64_game_unit_collision_radius(q->type);
                    int candidate_radius = sc64_game_unit_collision_radius(u[i].type);
                    int radius = mover_radius > candidate_radius ? mover_radius : candidate_radius;
#endif
#else
                    int radius = 6;
#endif
#ifdef SC64_RACES
#if defined(SC64_COLLISION_CLASS_CACHE)
                    if(u[i].type==U_CHRYSALIS||candidate_flying!=mover_flying)continue;
#elif defined(SC64_COLLISION_FACT_HOIST) && defined(SC64_ZP_CAMPAIGN_ACTORS)
                    if(u[i].type==U_CHRYSALIS||sc64_race_unit_flying(u[i].type)!=mover_flying)continue;
#else
                    if(u[i].type==U_CHRYSALIS||sc64_race_unit_flying(u[i].type)!=sc64_race_unit_flying(q->type))continue;
#endif
#if defined(SC64_T4_RUNTIME) || defined(SC64_ZP_CAMPAIGN_ACTORS)
#if defined(SC64_COLLISION_CLASS_CACHE)
                    if (!candidate_collidable || !mover_collidable) continue;
#elif defined(SC64_COLLISION_FACT_HOIST) && defined(SC64_ZP_CAMPAIGN_ACTORS)
                    unsigned candidate_flags = sc64_game_unit_actor_flags(u[i].type);
                    if ((candidate_flags &&
                         !(candidate_flags & SC64_GAME_ACTOR_COLLIDABLE)) ||
                        !mover_collidable) continue;
#else
                    if ((sc64_game_unit_actor_flags(u[i].type) &&
                         !(sc64_game_unit_actor_flags(u[i].type)&SC64_GAME_ACTOR_COLLIDABLE)) ||
                        (sc64_game_unit_actor_flags(q->type) &&
                         !(sc64_game_unit_actor_flags(q->type)&SC64_GAME_ACTOR_COLLIDABLE))) continue;
#endif
#endif
#endif
                    int dx = u[i].x - nx, dy = u[i].y - ny;
                    if (dx <= -radius || dx >= radius || dy <= -radius || dy >= radius) continue;
                    dx *= dx;
                    dy *= dy;
                    if (dx + dy < radius * radius) {
                        int ox = u[i].x - q->x, oy = u[i].y - q->y;
                        if (dx + dy <= ox * ox + oy * oy) return 0;
                    }
                }
        }
    return 1;
}

#ifdef SC64_COLLISION_STEER_LIST
#if !defined(SC64_COLLISION_CLASS_CACHE) || !defined(SC64_ZP_CAMPAIGN_ACTORS)
#error "SC64_COLLISION_STEER_LIST requires the Z/P collision-class cache"
#endif
/* A blocked mover tests up to eight one-step offsets around one position.
 * Resolve the candidate filters of sc64_unit_bucket_step_clear() once, over
 * the cells reachable from every offset, and keep each candidate's bucket
 * cell so each offset still sees exactly its own cell window. The query is
 * a pure any-blocker test, so candidate order cannot change its result. */
#define SC64_STEER_LIST_CAPACITY 48
typedef struct {
    s16 x, y;
    u8 radius, col, row, pad;
} Sc64SteerCandidate;
#if defined(SC64_Z7_RUNTIME)
/* Z7 keeps the 48 candidates on the blocked mover's stack frame rather than
 * adding 388 bytes to BSS, whose product reserve is intentionally tight. */
typedef struct {
    Sc64SteerCandidate entries[SC64_STEER_LIST_CAPACITY];
    int count;
} Sc64SteerList;
_Static_assert(sizeof(Sc64SteerList) == 388,
               "Z7 blocked-steering context must remain a 388-byte stack local");
#define SC64_STEER_BUILD_ARGS Sc64SteerList *list, const Unit *q, int speed
#define SC64_STEER_STEP_ARGS const Sc64SteerList *list, const Unit *q, int nx, int ny
#define SC64_STEER_COUNT (list->count)
#define SC64_STEER_ENTRY(i) (list->entries[i])
#define SC64_STEER_STEP_COUNT(list_arg) ((list_arg)->count)
#define SC64_STEER_STEP_ENTRY(list_arg, i) ((list_arg)->entries[i])
#else
static Sc64SteerCandidate sc64_steer_list[SC64_STEER_LIST_CAPACITY];
static int sc64_steer_list_count;
#define SC64_STEER_BUILD_ARGS const Unit *q, int speed
#define SC64_STEER_STEP_ARGS const Unit *q, int nx, int ny
#define SC64_STEER_COUNT (sc64_steer_list_count)
#define SC64_STEER_ENTRY(i) (sc64_steer_list[i])
#define SC64_STEER_STEP_COUNT(list_arg) (sc64_steer_list_count)
#define SC64_STEER_STEP_ENTRY(list_arg, i) (sc64_steer_list[i])
#endif

/* Returns 0 when the caller must use unit_step_clear() unchanged. */
#if defined(SC64_Z7_RUNTIME) && !defined(GBA_HOST)
static int __attribute__((noinline))
#else
static int GBA_HOT
#endif
sc64_steer_list_build(SC64_STEER_BUILD_ARGS)
{
    int mover_radius, mover_flying, mover_collidable;
    const int reach = 9;
    int minx, maxx, miny, maxy, row, col, count = 0;
    if (!sc64_unit_bucket_active || speed < 0 || speed > 2 ||
        MAP_COLS > 256 || MAP_ROWS > 256) return 0;
    if (IS_WORKER(q->type) && (q->order == ORDER_MINE || q->order == ORDER_GAS))
        return 0;
#ifndef SC64_Z7_RUNTIME
    (void)sc64_unit_collision_class((int)(q - u), q->type,
        &mover_radius, &mover_flying, &mover_collidable);
#endif
    minx = (q->x - speed - reach) >> 4; maxx = (q->x + speed + reach) >> 4;
    miny = (q->y - speed - reach) >> 4; maxy = (q->y + speed + reach) >> 4;
    if (minx < 0) minx = 0;
    if (miny < 0) miny = 0;
    if (maxx >= MAP_COLS) maxx = MAP_COLS - 1;
    if (maxy >= MAP_ROWS) maxy = MAP_ROWS - 1;
#ifdef SC64_Z7_RUNTIME
    /* Dense raw buckets conservatively fall back before class lookup/list
     * writes. The caller then uses the original exact step-clear path. */
    {
        int raw_nodes = 0;
        for (row = miny; row <= maxy; ++row)
            for (col = minx; col <= maxx; ++col) {
                int i;
                for (i = sc64_unit_bucket_head[col + row * MAP_COLS]; i != 0xff;
                     i = sc64_unit_bucket_next[i])
                    if (++raw_nodes > SC64_STEER_LIST_CAPACITY) return 0;
            }
    }
    (void)sc64_unit_collision_class((int)(q - u), q->type,
        &mover_radius, &mover_flying, &mover_collidable);
#endif
    for (row = miny; row <= maxy; ++row)
        for (col = minx; col <= maxx; ++col) {
            int i;
            for (i = sc64_unit_bucket_head[col + row * MAP_COLS]; i != 0xff;
                 i = sc64_unit_bucket_next[i]) {
                int candidate_radius, candidate_flying, candidate_collidable;
                if (&u[i] == q || !u[i].alive || SC64_TRANSPORT_LOADED(&u[i]))
                    continue;
#ifdef SC64_P6_COMBAT_ENABLED
                if (race_p6_combat_unit_burrowed(i)) continue;
#endif
                (void)sc64_unit_collision_class(i, u[i].type,
                    &candidate_radius, &candidate_flying, &candidate_collidable);
                if (u[i].type == U_CHRYSALIS || candidate_flying != mover_flying ||
                    !candidate_collidable || !mover_collidable) continue;
                int radius = mover_radius > candidate_radius ? mover_radius : candidate_radius;
                if (count >= SC64_STEER_LIST_CAPACITY || radius < 0 || radius > 255)
                    return 0;
                SC64_STEER_ENTRY(count).x = (s16)u[i].x;
                SC64_STEER_ENTRY(count).y = (s16)u[i].y;
                SC64_STEER_ENTRY(count).radius = (u8)radius;
                SC64_STEER_ENTRY(count).col = (u8)col;
                SC64_STEER_ENTRY(count).row = (u8)row;
                ++count;
            }
        }
    SC64_STEER_COUNT = count;
    return 1;
}

/* Same result as unit_step_clear(q, nx, ny) for the movable mover after a
 * successful build; move_toward() has already returned for immovable ones. */
static int GBA_HOT
sc64_steer_list_step_clear(SC64_STEER_STEP_ARGS)
{
    const int reach = 9;
    int minx, maxx, miny, maxy, k;
    if (nx < 0 || ny < 0 || nx >= WORLD_W || ny >= WORLD_H) return 0;
    minx = (nx - reach) >> 4; maxx = (nx + reach) >> 4;
    miny = (ny - reach) >> 4; maxy = (ny + reach) >> 4;
    for (k = 0; k < SC64_STEER_STEP_COUNT(list); ++k) {
        const Sc64SteerCandidate *c = &SC64_STEER_STEP_ENTRY(list, k);
        int radius = c->radius, dx, dy;
        if (c->col < minx || c->col > maxx || c->row < miny || c->row > maxy)
            continue;
        dx = c->x - nx; dy = c->y - ny;
        if (dx <= -radius || dx >= radius || dy <= -radius || dy >= radius) continue;
        dx *= dx;
        dy *= dy;
        if (dx + dy < radius * radius) {
            int ox = c->x - q->x, oy = c->y - q->y;
            if (dx + dy <= ox * ox + oy * oy) return 0;
        }
    }
    return 1;
}
#undef SC64_STEER_BUILD_ARGS
#undef SC64_STEER_STEP_ARGS
#undef SC64_STEER_COUNT
#undef SC64_STEER_ENTRY
#undef SC64_STEER_STEP_COUNT
#undef SC64_STEER_STEP_ENTRY

#endif

static void
sc64_unit_buckets_end(void)
{
    sc64_unit_bucket_active = 0;
}

#endif
