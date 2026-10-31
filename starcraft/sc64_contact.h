#ifndef SC64_CONTACT_H
#define SC64_CONTACT_H
#include "sc64_scarab_presence.h"

/* SC64_CONTACT_CACHE: cached collision facts and one-pass blocked steering.
 *
 * The bucket collision query repeats the same type-derived fact lookups
 * (collision radius, flight, collidable flags, burrow eligibility) for every
 * nearby candidate, and a blocked mover repeats the whole query for each of
 * its eight steering offsets. This header caches the facts of each unit
 * type in one byte, indexed by type, and evaluates all eight offsets in a
 * single pass over the bucket candidates.
 *
 * The predicate is unchanged: every candidate is accepted or rejected by the
 * same alive, loaded, burrowed, chrysalis, flight, collidable, radius and
 * "do not move closer" tests as sc64_unit_bucket_step_clear(). The facts are
 * pure functions of the unit type within a build, so a type-indexed table
 * needs no invalidation on slot reuse, morphs or loads. Anything the byte
 * cannot describe falls back to the original lookups for that actor.
 *
 * Included by the collision headers after the bucket arrays are declared. */

#if !defined(SC64_RACES)
#error "SC64_CONTACT_CACHE requires the race collision predicate"
#endif
#if defined(SC64_COLLISION_CLASS_CACHE) || defined(SC64_COLLISION_STEER_LIST)
#error "SC64_CONTACT_CACHE supersedes the collision class cache and steering list"
#endif

#define SC64_CONTACT_RADIUS_MASK 0x1fu
#define SC64_CONTACT_IGNORE 0x1fu /* chrysalis or non-collidable candidate */
#define SC64_CONTACT_BURROWABLE 0x20u
#define SC64_CONTACT_FLYING 0x40u
#define SC64_CONTACT_VALID 0x80u

#ifdef SC64_CONTACT_MASK_TABLE
#if !defined(SC64_CONTACT_CACHE) || !defined(SC64_ZP_CAMPAIGN_ACTORS)
#error "SC64_CONTACT_MASK_TABLE requires CONTACT_CACHE and Z/P actor facts"
#endif
#include "sc64_contact_mask_table_generated.h"
_Static_assert(sizeof(sc64_contact_mask_table_data) ==
               14u * 15u * 65u * 65u, "contact mask table dimensions changed");
#endif

#define SC64_CONTACT_TYPES 256
static u8 sc64_contact_class[SC64_CONTACT_TYPES];

/* Placement: SC64_CONTACT_STEER_ROM keeps the one-pass steering mask in ROM
 * and SC64_CONTACT_STOCK_ROM keeps the stock step out of line in ROM. Both
 * are size choices only; the answers do not depend on them. The late Terran
 * profile has no IWRAM left for the steering pass, so it always keeps it in
 * ROM, where it still replaces eight full queries with one. The Z6 and P6
 * canonical products select both ROM placements in the Makefile. */
#if defined(SC64_T5_RUNTIME) && !defined(GBA_HOST) && !defined(SC64_CONTACT_STEER_ROM)
#define SC64_CONTACT_STEER_ROM 1
#endif

static int GBA_HOT unit_step_clear(const Unit *q, int nx, int ny);
static int sc64_unit_bucket_step_clear_reference(const Unit *q, int nx, int ny, int strict);

#ifdef SC64_P6_COMBAT_ENABLED
static int race_p6_active(void);
static int race_p6_combat_unit_burrowed(int slot);
#endif

/* Type-derived facts only; raw_type-specific burrow eligibility is resolved
 * per candidate from the unit record. Returns 0 when the byte cannot
 * describe the type exactly, which makes the caller use the reference path. */
static u8 __attribute__((noinline))
sc64_contact_class_compute(int type)
{
    unsigned radius = 6, flags = 0;
    int flying, collidable = 1, ignore;
    if (type < 0 || type > 255) return 0;
    flying = sc64_race_unit_flying(type) != 0;
#if defined(SC64_T4_RUNTIME) || defined(SC64_ZP_CAMPAIGN_ACTORS)
    {
        unsigned actor = sc64_game_unit_actor_flags(type);
        collidable = !actor || (actor & SC64_GAME_ACTOR_COLLIDABLE) != 0;
    }
#endif
#ifdef SC64_ZP_CAMPAIGN_ACTORS
    {
        int r = sc64_game_unit_collision_radius(type);
        if (r < 0 || r >= (int)SC64_CONTACT_IGNORE) return 0;
        radius = (unsigned)r;
    }
#endif
    ignore = type == U_CHRYSALIS || !collidable;
#ifdef SC64_P6_COMBAT_ENABLED
    {
        int raw = sc64_race_raw_unit(type);
        if (raw == 37 || raw == 38 || raw == SC64_P6_RAW_INFESTED_TERRAN)
            flags |= SC64_CONTACT_BURROWABLE;
    }
#endif
    if (flying) flags |= SC64_CONTACT_FLYING;
    return (u8)(SC64_CONTACT_VALID | flags |
                (ignore ? SC64_CONTACT_IGNORE : radius));
}

/* Returns 0 (no VALID bit) for undescribable types, which are never cached. */
static inline __attribute__((always_inline)) u8
sc64_contact_class_for(int type)
{
    u8 c;
    if ((unsigned)type >= SC64_CONTACT_TYPES) return 0;
    c = sc64_contact_class[type];
    if (c & SC64_CONTACT_VALID) return c;
    c = sc64_contact_class_compute(type);
    sc64_contact_class[type] = c;
    return c;
}

/* Exactly race_p6_combat_unit_burrowed(i) for an alive candidate: the raw
 * identity comes from the record when one is pinned, else from its type. */
static inline __attribute__((always_inline)) int
sc64_contact_burrowed(const Unit *c, u8 cc, int p6)
{
#ifdef SC64_P6_COMBAT_ENABLED
    int raw;
    if (!p6) return 0;
    raw = c->raw_type;
    if (raw >= 0) {
        if (raw != 37 && raw != 38 && raw != SC64_P6_RAW_INFESTED_TERRAN) return 0;
    } else if (!(cc & SC64_CONTACT_BURROWABLE)) return 0;
    return (c->flags & 2u) != 0;
#else
    (void)c; (void)cc; (void)p6;
    return 0;
#endif
}

static inline __attribute__((always_inline)) int
sc64_contact_p6_active(void)
{
#ifdef SC64_P6_COMBAT_ENABLED
    return race_p6_active();
#else
    return 0;
#endif
}

/* Reference per-candidate predicate with the original fact lookups, used
 * when a candidate's facts cannot be cached. Returns 1 when it blocks. */
static int __attribute__((noinline))
sc64_contact_candidate_reference(const Unit *q, int i, int nx, int ny,
                                 int mover_radius, int mover_flying, int strict)
{
    const Unit *c = &u[i];
    int radius = 6, dx, dy, ox, oy;
    (void)mover_radius;
    if (SC64_SCARAB_NONCOLLIDING(c)) return 0;
#ifdef SC64_P6_COMBAT_ENABLED
    if (race_p6_combat_unit_burrowed(i)) return 0;
#endif
#ifdef SC64_ZP_CAMPAIGN_ACTORS
    {
        int candidate_radius = sc64_game_unit_collision_radius(c->type);
        radius = mover_radius > candidate_radius ? mover_radius : candidate_radius;
    }
#endif
    if (c->type == U_CHRYSALIS || (sc64_race_unit_flying(c->type) != 0) != mover_flying)
        return 0;
#if defined(SC64_T4_RUNTIME) || defined(SC64_ZP_CAMPAIGN_ACTORS)
    {
        unsigned candidate_flags = sc64_game_unit_actor_flags(c->type);
        if (candidate_flags && !(candidate_flags & SC64_GAME_ACTOR_COLLIDABLE)) return 0;
    }
#endif
    dx = c->x - nx; dy = c->y - ny;
    if (dx <= -radius || dx >= radius || dy <= -radius || dy >= radius) return 0;
    dx *= dx; dy *= dy;
    if (dx + dy >= radius * radius) return 0;
    ox = c->x - q->x; oy = c->y - q->y;
    return strict || dx + dy <= ox * ox + oy * oy;
}

/* Mover facts shared by the step query and the steering mask. Returns 0
 * when the caller must use the reference query (undescribable mover type),
 * 1 when the mover collides with nothing (non-collidable actor), and 2 for
 * an ordinary collidable mover. */
static inline __attribute__((always_inline)) int
sc64_contact_mover(const Unit *q, int *radius, int *flying)
{
    if (SC64_SCARAB_NONCOLLIDING(q)) return 1;
    u8 mc = sc64_contact_class_for(q->type);
    if (!(mc & SC64_CONTACT_VALID) || q->type == U_CHRYSALIS) return 0;
    *radius = (int)(mc & SC64_CONTACT_RADIUS_MASK);
    *flying = (mc & SC64_CONTACT_FLYING) != 0;
    return *radius == (int)SC64_CONTACT_IGNORE ? 1 : 2;
}

/* Cells that can hold a candidate strictly inside the largest radius pair:
 * authored Z/P facts bound ground radii at 10 and flying radii at 19; the
 * plain race predicate uses radius 6. */
#ifdef SC64_ZP_CAMPAIGN_ACTORS
#define SC64_CONTACT_REACH(flying) ((flying) ? 18 : 9)
#else
#define SC64_CONTACT_REACH(flying) 5
#endif

/* Same answer as the reference bucket query for an unloaded alive mover
 * whose worker/immovable early returns the caller has already applied. */
static inline __attribute__((always_inline)) int
sc64_contact_step_clear(const Unit *q, int nx, int ny, int strict)
{
    int mover_radius, mover_flying, mover_slot = (int)(q - u), p6, reach;
    int minx, maxx, miny, maxy, row, col;
    int mover = sc64_contact_mover(q, &mover_radius, &mover_flying);
    if (mover == 1) return 1;
    if (mover == 0) return sc64_unit_bucket_step_clear_reference(q, nx, ny, strict);
    p6 = sc64_contact_p6_active();
    reach = SC64_CONTACT_REACH(mover_flying);
    minx = (nx - reach) >> 4; maxx = (nx + reach) >> 4;
    miny = (ny - reach) >> 4; maxy = (ny + reach) >> 4;
    if (minx < 0) minx = 0;
    if (miny < 0) miny = 0;
    if (maxx >= MAP_COLS) maxx = MAP_COLS - 1;
    if (maxy >= MAP_ROWS) maxy = MAP_ROWS - 1;
    for (row = miny; row <= maxy; ++row)
        for (col = minx; col <= maxx; ++col) {
            int i;
            for (i = sc64_unit_bucket_head[col + row * MAP_COLS]; i != 0xff;
                 i = sc64_unit_bucket_next[i]) {
                const Unit *c = &u[i];
                u8 cc;
                int radius, dx, dy, ox, oy;
                if (i == mover_slot || !c->alive || (SC64_TRANSPORT_LOADED(c) || SC64_SCARAB_NONCOLLIDING(c))) continue;
                cc = sc64_contact_class_for(c->type);
                if (!(cc & SC64_CONTACT_VALID)) {
                    if (sc64_contact_candidate_reference(q, i, nx, ny, mover_radius,
                                                         mover_flying, strict))
                        return 0;
                    continue;
                }
                radius = (int)(cc & SC64_CONTACT_RADIUS_MASK);
                if (radius == (int)SC64_CONTACT_IGNORE) continue;
                if (((cc & SC64_CONTACT_FLYING) != 0) != mover_flying) continue;
                if (sc64_contact_burrowed(c, cc, p6)) continue;
                if (radius < mover_radius) radius = mover_radius;
                dx = c->x - nx; dy = c->y - ny;
                if (dx <= -radius || dx >= radius || dy <= -radius || dy >= radius) continue;
                dx *= dx; dy *= dy;
                if (dx + dy >= radius * radius) continue;
                ox = c->x - q->x; oy = c->y - q->y;
                if (strict || dx + dy <= ox * ox + oy * oy) return 0;
            }
        }
    return 1;
}

static const s8 sc64_contact_dx[8] = {1,1,0,-1,-1,-1,0,1};
static const s8 sc64_contact_dy[8] = {0,1,1,1,0,-1,-1,-1};

/* Offsets that leave the world are blocked before any candidate is read. */
static unsigned __attribute__((noinline))
sc64_contact_bounds_mask(const Unit *q, int speed)
{
    unsigned mask = 0;
    int k;
    for (k = 0; k < 8; ++k) {
        int sx = q->x + sc64_contact_dx[k] * speed, sy = q->y + sc64_contact_dy[k] * speed;
        if (sx < 0 || sy < 0 || sx >= WORLD_W || sy >= WORLD_H) mask |= 1u << k;
    }
    return mask;
}

/* Cold mask setup: early returns, mover facts and the reference fallback.
 * Returns 1 when *mask is final, 0 when the hot candidate pass must run. */
static int __attribute__((noinline))
sc64_contact_mask_prepare(const Unit *q, int speed, unsigned *mask,
                          int *mover_radius, int *mover_flying)
{
    int k, mover;
    *mask = sc64_contact_bounds_mask(q, speed);
    if (IS_WORKER(q->type) && (q->order == ORDER_MINE || q->order == ORDER_GAS)) return 1;
#if defined(SC64_T4_RUNTIME) || defined(SC64_ZP_CAMPAIGN_ACTORS)
    if (sc64_game_unit_actor_flags(q->type) &&
        !(sc64_game_unit_actor_flags(q->type) & SC64_GAME_ACTOR_MOVABLE)) return 1;
#endif
    mover = sc64_unit_bucket_active ? sc64_contact_mover(q, mover_radius, mover_flying) : 0;
    if (mover == 2) return 0;
    if (mover == 0) {
        /* Inactive index or undescribable mover: the unchanged query decides. */
        for (k = 0; k < 8; ++k)
            if (!(*mask & (1u << k)) &&
                !unit_step_clear(q, q->x + sc64_contact_dx[k] * speed,
                                 q->y + sc64_contact_dy[k] * speed))
                *mask |= 1u << k;
    }
    return 1;
}

/* Reference predicate for one undescribable candidate over all offsets. */
static unsigned __attribute__((noinline))
sc64_contact_mask_reference(const Unit *q, int i, int speed, unsigned mask,
                            int mover_radius, int mover_flying)
{
    int k;
    for (k = 0; k < 8; ++k)
        if (!(mask & (1u << k)) &&
            sc64_contact_candidate_reference(
                q, i, q->x + sc64_contact_dx[k] * speed,
                q->y + sc64_contact_dy[k] * speed, mover_radius, mover_flying, 0))
            mask |= 1u << k;
    return mask;
}

/* Bit k is set when unit_step_clear(q, q->x + dx[k]*speed, q->y + dy[k]*speed)
 * would return 0. One pass over the union of the eight candidate windows
 * replaces eight queries; the any-blocker predicate makes order irrelevant. */
#ifdef SC64_CONTACT_MASK_TABLE
static unsigned
sc64_contact_mask_math_reference(unsigned mask, int radius, int speed,
                                 int ox, int oy)
{
    unsigned pending = ~mask & 0xffu;
    int od2 = ox * ox + oy * oy;
    int k;
    for (k = 0; pending; ++k, pending >>= 1) {
        int dx, dy, d2;
        if (!(pending & 1u)) continue;
        dx = ox - sc64_contact_dx[k] * speed;
        dy = oy - sc64_contact_dy[k] * speed;
        if (dx <= -radius || dx >= radius || dy <= -radius || dy >= radius) continue;
        d2 = dx * dx + dy * dy;
        if (d2 < radius * radius && d2 <= od2) mask |= 1u << k;
    }
    return mask;
}

static inline int
sc64_contact_mask_table_domain(int radius, int speed, int ox, int oy)
{
    return radius >= 6 && radius <= 19 && speed >= 0 && speed <= 14 &&
           ox >= -32 && ox <= 32 && oy >= -32 && oy <= 32;
}

static inline u8
sc64_contact_mask_table_lookup(int radius, int speed, int ox, int oy)
{
    unsigned ri = (unsigned)(radius - 6);
    unsigned oi = (unsigned)(oy + 32);
    unsigned xi = (unsigned)(ox + 32);
    unsigned index = (((ri * 15u + (unsigned)speed) * 65u + oi) * 65u + xi);
    return sc64_contact_mask_table_data[index];
}

static inline unsigned
sc64_contact_mask_apply_geometry(unsigned mask, int radius, int speed,
                                 int ox, int oy)
{
    if (sc64_contact_mask_table_domain(radius, speed, ox, oy))
        return mask | sc64_contact_mask_table_lookup(radius, speed, ox, oy);
    return sc64_contact_mask_math_reference(mask, radius, speed, ox, oy);
}
#endif

#ifdef SC64_CONTACT_MEMO_ENABLED
#if !defined(SC64_CONTACT_CACHE)
#error "contact memo requires the contact-mask implementation"
#endif
#define SC64_CONTACT_MEMO_INVALIDATE() sc64_contact_memo_invalidate()
typedef struct {
    s16 x, y, raw_type;
    u8 type, order, speed, p6, mask, valid;
} Sc64ContactMemoEntry;
_Static_assert(sizeof(Sc64ContactMemoEntry) <= 16, "contact memo entry exceeds bound");
static Sc64ContactMemoEntry sc64_contact_memo_entries[4];
static u8 sc64_contact_memo_count;
#ifdef SC64_CONTACT_MEMO_ORACLE
static u32 sc64_contact_memo_generation;
#endif

static void __attribute__((noinline))
sc64_contact_memo_invalidate(void)
{
    sc64_contact_memo_count = 0;
#ifdef SC64_CONTACT_MEMO_ORACLE
    ++sc64_contact_memo_generation;
#endif
}

static int __attribute__((noinline))
sc64_contact_memo_key_equal(const Sc64ContactMemoEntry *e, const Unit *q,
                            int speed, int p6)
{
    return e->valid && e->x == q->x && e->y == q->y &&
           e->type == q->type && e->raw_type == q->raw_type &&
           e->order == q->order && e->speed == speed && e->p6 == p6;
}

static int __attribute__((noinline))
sc64_contact_memo_lookup(const Unit *q, int speed, int p6, unsigned *mask)
{
    unsigned i, j;
    for (i = 0; i < sc64_contact_memo_count; ++i)
        if (sc64_contact_memo_key_equal(&sc64_contact_memo_entries[i], q, speed, p6))
            break;
    if (i == sc64_contact_memo_count) return 0;
    {
        Sc64ContactMemoEntry hit = sc64_contact_memo_entries[i];
        for (j = i; j; --j) sc64_contact_memo_entries[j] = sc64_contact_memo_entries[j - 1u];
        sc64_contact_memo_entries[0] = hit;
    }
    *mask = sc64_contact_memo_entries[0].mask;
    return 1;
}

static void __attribute__((noinline))
sc64_contact_memo_remember(const Unit *q, int speed, int p6, unsigned mask)
{
    unsigned i, j;
    for (i = 0; i < sc64_contact_memo_count; ++i)
        if (sc64_contact_memo_key_equal(&sc64_contact_memo_entries[i], q, speed, p6))
            break;
    if (i < sc64_contact_memo_count) {
        for (j = i; j; --j) sc64_contact_memo_entries[j] = sc64_contact_memo_entries[j - 1u];
    } else {
        if (sc64_contact_memo_count < 4u) ++sc64_contact_memo_count;
        for (j = sc64_contact_memo_count - 1u; j; --j)
            sc64_contact_memo_entries[j] = sc64_contact_memo_entries[j - 1u];
    }
    sc64_contact_memo_entries[0].x = (s16)q->x;
    sc64_contact_memo_entries[0].y = (s16)q->y;
    sc64_contact_memo_entries[0].raw_type = (s16)q->raw_type;
    sc64_contact_memo_entries[0].type = (u8)q->type;
    sc64_contact_memo_entries[0].order = (u8)q->order;
    sc64_contact_memo_entries[0].speed = (u8)speed;
    sc64_contact_memo_entries[0].p6 = (u8)p6;
    sc64_contact_memo_entries[0].mask = (u8)mask;
    sc64_contact_memo_entries[0].valid = 1;
}

#ifdef SC64_CONTACT_MEMO_ORACLE
#include <assert.h>
#include <stdio.h>
typedef struct { s16 x, y, type, raw_type; u16 flags; u8 present; } Sc64ContactMemoActorSnapshot;
typedef struct {
    u8 p6, buckets_active;
    Sc64ContactMemoActorSnapshot actors[MAX_UNITS];
} Sc64ContactMemoWorldSnapshot;
static Sc64ContactMemoWorldSnapshot sc64_contact_memo_last_world;
static u32 sc64_contact_memo_last_generation;
static u8 sc64_contact_memo_have_last_world, sc64_contact_memo_failed;
static u32 sc64_contact_memo_window;
static u32 sc64_contact_memo_counts[3][5];
static void __attribute__((noinline))
sc64_contact_memo_oracle_observe(void)
{
    Sc64ContactMemoWorldSnapshot world;
    unsigned i;
    memset(&world, 0, sizeof(world));
    world.p6 = (u8)!!sc64_contact_p6_active();
    world.buckets_active = (u8)!!sc64_unit_bucket_active;
    for (i = 0; i < MAX_UNITS; ++i) {
        const Unit *c = &u[i];
        Sc64ContactMemoActorSnapshot *a = &world.actors[i];
        if (!c->alive || (SC64_TRANSPORT_LOADED(c) || SC64_SCARAB_NONCOLLIDING(c))) continue;
                a->present = 1; a->x = (s16)c->x; a->y = (s16)c->y;
                a->type = (s16)c->type; a->raw_type = (s16)c->raw_type;
                a->flags = (u16)c->flags;
    }
    if (sc64_contact_memo_have_last_world &&
        memcmp(&world, &sc64_contact_memo_last_world, sizeof(world)) != 0 &&
        sc64_contact_memo_generation == sc64_contact_memo_last_generation) {
        ++sc64_contact_memo_counts[sc64_contact_memo_window][4];
        sc64_contact_memo_failed = 1;
        fprintf(stderr, "CONTACT_MEMO_UNHOOKED window=%u generation=%lu\n",
                (unsigned)sc64_contact_memo_window,
                (unsigned long)sc64_contact_memo_generation);
    }
    sc64_contact_memo_last_world = world;
    sc64_contact_memo_last_generation = sc64_contact_memo_generation;
    sc64_contact_memo_have_last_world = 1;
}
static void sc64_contact_memo_set_window(unsigned w)
{ sc64_contact_memo_window = w < 3u ? w : 2u; }
static void sc64_contact_memo_report(unsigned w)
{
    if (w >= 3u) return;
    fprintf(stderr, "CONTACT_MEMO window=%u calls=%lu eligible=%lu hits=%lu misses=%lu unhooked=%lu\n",
        w, (unsigned long)sc64_contact_memo_counts[w][0],
        (unsigned long)sc64_contact_memo_counts[w][1],
        (unsigned long)sc64_contact_memo_counts[w][2],
        (unsigned long)sc64_contact_memo_counts[w][3],
        (unsigned long)sc64_contact_memo_counts[w][4]);
}
static int sc64_contact_memo_oracle_passed(void)
{ return !sc64_contact_memo_failed; }
#define SC64_CONTACT_MEMO_ORACLE_CALL() do { \
    sc64_contact_memo_oracle_observe(); \
    ++sc64_contact_memo_counts[sc64_contact_memo_window][0]; \
} while (0)
#else
#define SC64_CONTACT_MEMO_ORACLE_CALL() ((void)0)
#endif

static int __attribute__((noinline))
sc64_contact_memo_eligible(const Unit *q, int speed)
{
    int radius, flying;
    if (!q || speed <= 0 || speed > 255 || !sc64_unit_bucket_active ||
        q->x < 0 || q->x >= WORLD_W || q->y < 0 || q->y >= WORLD_H ||
        q->raw_type < -32768 || q->raw_type > 32767 ||
        q->order < 0 || q->order > 255) return 0;
    if (IS_WORKER(q->type) && (q->order == ORDER_MINE || q->order == ORDER_GAS)) return 0;
#if defined(SC64_T4_RUNTIME) || defined(SC64_ZP_CAMPAIGN_ACTORS)
    { unsigned flags = sc64_game_unit_actor_flags(q->type);
      if (flags && !(flags & SC64_GAME_ACTOR_MOVABLE)) return 0; }
#endif
    return sc64_contact_mover(q, &radius, &flying) == 2;
}

#ifdef SC64_DIAGNOSTIC_CONTACT_SPLIT
/* Position change since the previous update_orders entry, not a prediction
 * of future movement. Diagnostic only; counters wrap modulo 2^32. */
volatile u32 diag_contact_visit_moved, diag_contact_visit_stationary;
volatile u32 diag_contact_geometry_moved, diag_contact_geometry_stationary;
volatile u32 diag_traversable_calls;
static s16 diag_contact_prev_x[MAX_UNITS], diag_contact_prev_y[MAX_UNITS];
static u8 diag_contact_moved[MAX_UNITS], diag_contact_prev_alive[MAX_UNITS];
static void __attribute__((noinline)) sc64_contact_split_begin(void)
{
    int i;
    diag_contact_visit_moved = diag_contact_visit_stationary = 0;
    diag_contact_geometry_moved = diag_contact_geometry_stationary = 0;
    diag_traversable_calls = 0;
    for (i = 0; i < MAX_UNITS; ++i) {
        diag_contact_moved[i] = u[i].alive && diag_contact_prev_alive[i] &&
            (u[i].x != diag_contact_prev_x[i] || u[i].y != diag_contact_prev_y[i]);
        diag_contact_prev_x[i] = u[i].x; diag_contact_prev_y[i] = u[i].y;
        diag_contact_prev_alive[i] = u[i].alive;
    }
}
static void __attribute__((noinline)) sc64_contact_split_visit(int i)
{ if (diag_contact_moved[i]) ++diag_contact_visit_moved; else ++diag_contact_visit_stationary; }
static void __attribute__((noinline)) sc64_contact_split_geometry(int i)
{ if (diag_contact_moved[i]) ++diag_contact_geometry_moved; else ++diag_contact_geometry_stationary; }
static void __attribute__((noinline)) sc64_diagnostic_traversable(void)
{ ++diag_traversable_calls; }
#define CONTACT_SPLIT(kind, i) sc64_contact_split_##kind(i)
#else
#define CONTACT_SPLIT(kind, i) ((void)0)
#endif

static unsigned __attribute__((noinline))
sc64_contact_blocked_mask_uncached(const Unit *q, int speed);
static unsigned __attribute__((noinline))
sc64_contact_blocked_mask_memo(const Unit *q, int speed)
{
    unsigned mask;
    int eligible = sc64_contact_memo_eligible(q, speed);
    int p6 = !!sc64_contact_p6_active();
#ifdef SC64_CONTACT_MEMO_ORACLE
    SC64_CONTACT_MEMO_ORACLE_CALL();
#endif
    if (eligible) {
#ifdef SC64_CONTACT_MEMO_ORACLE
        ++sc64_contact_memo_counts[sc64_contact_memo_window][1];
#endif
        if (sc64_contact_memo_lookup(q, speed, p6, &mask)) {
#ifdef SC64_CONTACT_MEMO_ORACLE
            unsigned oracle = sc64_contact_blocked_mask_uncached(q, speed);
            ++sc64_contact_memo_counts[sc64_contact_memo_window][2];
            assert(mask == oracle);
#endif
            return mask;
        }
#ifdef SC64_CONTACT_MEMO_ORACLE
        ++sc64_contact_memo_counts[sc64_contact_memo_window][3];
#endif
    }
    mask = sc64_contact_blocked_mask_uncached(q, speed);
    if (eligible) sc64_contact_memo_remember(q, speed, p6, mask);
    return mask;
}
#endif

static unsigned
#if defined(SC64_CONTACT_STEER_ROM)
__attribute__((noinline))
#else
GBA_HOT
#endif
sc64_contact_blocked_mask_uncached(const Unit *q, int speed)
{
    unsigned mask;
#ifndef SC64_CONTACT_MASK_TABLE
    unsigned pending;
    int k;
#endif
    int mover_radius, mover_flying, mover_slot = (int)(q - u), p6;
    int minx, maxx, miny, maxy, row, col, reach;
    if (sc64_contact_mask_prepare(q, speed, &mask, &mover_radius, &mover_flying))
        return mask;
    p6 = sc64_contact_p6_active();
    reach = SC64_CONTACT_REACH(mover_flying) + speed;
    minx = (q->x - reach) >> 4; maxx = (q->x + reach) >> 4;
    miny = (q->y - reach) >> 4; maxy = (q->y + reach) >> 4;
    if (minx < 0) minx = 0;
    if (miny < 0) miny = 0;
    if (maxx >= MAP_COLS) maxx = MAP_COLS - 1;
    if (maxy >= MAP_ROWS) maxy = MAP_ROWS - 1;
    for (row = miny; row <= maxy; ++row)
        for (col = minx; col <= maxx; ++col) {
            int i;
            for (i = sc64_unit_bucket_head[col + row * MAP_COLS]; i != 0xff;
                 i = sc64_unit_bucket_next[i]) {
                const Unit *c = &u[i];
                CONTACT_SPLIT(visit, i);
                u8 cc;
                int radius, ox, oy, limit_axis;
#ifndef SC64_CONTACT_MASK_TABLE
                int od2;
#endif
                if (i == mover_slot || !c->alive || (SC64_TRANSPORT_LOADED(c) || SC64_SCARAB_NONCOLLIDING(c))) continue;
                cc = sc64_contact_class_for(c->type);
                if (!(cc & SC64_CONTACT_VALID)) {
                    mask = sc64_contact_mask_reference(q, i, speed, mask,
                                                       mover_radius, mover_flying);
                    continue;
                }
                radius = (int)(cc & SC64_CONTACT_RADIUS_MASK);
                if (radius == (int)SC64_CONTACT_IGNORE) continue;
                if (((cc & SC64_CONTACT_FLYING) != 0) != mover_flying) continue;
                if (sc64_contact_burrowed(c, cc, p6)) continue;
                if (radius < mover_radius) radius = mover_radius;
                ox = c->x - q->x; oy = c->y - q->y;
                /* No offset moves the mover closer than |o| - speed. */
                limit_axis = radius + speed;
                if (ox <= -limit_axis || ox >= limit_axis ||
                    oy <= -limit_axis || oy >= limit_axis) continue;
                CONTACT_SPLIT(geometry, i);
#ifdef SC64_CONTACT_MASK_TABLE
                mask = sc64_contact_mask_apply_geometry(mask, radius, speed, ox, oy);
#else
                od2 = ox * ox + oy * oy;
                pending = ~mask & 0xffu;
                for (k = 0; pending; ++k, pending >>= 1) {
                    int dx, dy, d2;
                    if (!(pending & 1u)) continue;
                    dx = ox - sc64_contact_dx[k] * speed;
                    dy = oy - sc64_contact_dy[k] * speed;
                    if (dx <= -radius || dx >= radius || dy <= -radius || dy >= radius) continue;
                    d2 = dx * dx + dy * dy;
                    if (d2 < radius * radius && d2 <= od2) mask |= 1u << k;
                }
#endif
                if (mask == 0xffu) return mask;
            }
        }
    return mask;
}

#ifdef SC64_CONTACT_MEMO_ENABLED
static unsigned __attribute__((noinline))
sc64_contact_blocked_mask(const Unit *q, int speed)
{ return sc64_contact_blocked_mask_memo(q, speed); }
#else
#define sc64_contact_blocked_mask sc64_contact_blocked_mask_uncached
#endif

/* Stock motion rate without the software division: (N * 52429) >> 18 equals
 * N / 5 for every N below 81920, which covers the whole profile table
 * (largest top speed 34130, so N = 68262). Larger values keep the division. */
static inline __attribute__((always_inline)) unsigned
sc64_contact_stock_rate(int raw)
{
    const Sc64StockUnitProfile *p = sc64_stock_unit_profile(raw);
    unsigned n;
    if (!p) return 0u;
    n = p->top_speed_q10 * 2u + 2u;
    return n < 81920u ? (n * 52429u) >> 18 : n / 5u;
}

/* Identical to sc64_stock_motion_step(raw, tick, slot, diagonal). */
static inline __attribute__((always_inline)) unsigned
sc64_contact_stock_step(int raw, unsigned tick, unsigned slot, int diagonal)
{
    unsigned rate = sc64_contact_stock_rate(raw);
    unsigned phase = (tick + slot * 67u) & 1023u;
    if (diagonal) rate = (rate * 724u + 512u) >> 10;
    return ((phase + 1u) * rate >> 10) - (phase * rate >> 10);
}

#endif
