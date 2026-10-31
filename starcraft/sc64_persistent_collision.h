#ifndef SC64_PERSISTENT_COLLISION_H
#define SC64_PERSISTENT_COLLISION_H
#include "sc64_scarab_presence.h"

_Static_assert(MAX_UNITS < 255, "Spatial unit indices must fit below the sentinel");

/* Exact broad-phase index for moving units. Built only for update_orders();
 * all candidates still use the engine's original overlap predicate. */
static u8 sc64_unit_bucket_head[MAP_CELLS];
static u8 sc64_unit_bucket_next[MAX_UNITS];
static u8 sc64_unit_bucket_prev[MAX_UNITS];
static u16 sc64_unit_bucket_cell[MAX_UNITS];
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
    sc64_unit_bucket_prev[slot] = 0xff;
    sc64_unit_bucket_next[slot] = head;
    if (head != 0xff) sc64_unit_bucket_prev[head] = (u8)slot;
    sc64_unit_bucket_head[cell] = (u8)slot;
    sc64_unit_bucket_cell[slot] = (u16)cell;
}

#ifdef SC64_T5_RUNTIME
/* Removal is needed on cell transitions and lifecycle changes, rather than
 * every collision query. Keep it out of the late profile's fast-code budget. */
static void __attribute__((noinline))
#else
static void GBA_HOT __attribute__((noinline))
#endif
sc64_unit_bucket_remove(int slot)
{
    u16 cell = sc64_unit_bucket_cell[slot];
    u8 before, after;
    if (cell == 0xffffu) return;
    before = sc64_unit_bucket_prev[slot];
    after = sc64_unit_bucket_next[slot];
    if (before == 0xff) sc64_unit_bucket_head[cell] = after;
    else sc64_unit_bucket_next[before] = after;
    if (after != 0xff) sc64_unit_bucket_prev[after] = before;
    sc64_unit_bucket_next[slot] = sc64_unit_bucket_prev[slot] = 0xff;
    sc64_unit_bucket_cell[slot] = 0xffffu;
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
#ifdef SC64_CONTACT_MEMO_ENABLED
    SC64_CONTACT_MEMO_INVALIDATE();
#endif

    int i;
    if (!sc64_unit_bucket_initialized) {
        memset(sc64_unit_bucket_head, 0xff, sizeof(sc64_unit_bucket_head));
        for (i = 0; i < MAX_UNITS; ++i) {
            sc64_unit_bucket_next[i] = 0xff;
            sc64_unit_bucket_prev[i] = 0xff;
            sc64_unit_bucket_cell[i] = 0xffffu;
        }
        sc64_unit_bucket_initialized = 1;
    }
    sc64_unit_bucket_active = 0;
    for (i = 0; i < MAX_UNITS; ++i) {
        int x = u[i].x, y = u[i].y;
        u16 old_cell = sc64_unit_bucket_cell[i];
        if (!u[i].alive || SC64_TRANSPORT_LOADED(&u[i]) || SC64_SCARAB_NONCOLLIDING(&u[i])) {
            sc64_unit_bucket_remove(i);
            continue;
        }
        if (x < 0 || y < 0 || x >= WORLD_W || y >= WORLD_H) {
            sc64_unit_bucket_remove(i);
            return;
        }
        int new_cell = sc64_unit_bucket_cell_at(x, y);
        if (old_cell != (u16)new_cell) {
            sc64_unit_bucket_remove(i);
            sc64_unit_bucket_insert(i, new_cell);
        }
    }
    sc64_unit_bucket_active = 1;
}

static void GBA_HOT
#if defined(SC64_T5_RUNTIME) && !defined(GBA_HOST)
__attribute__((optimize("Os")))
#endif
sc64_unit_bucket_move(Unit *q)
{
#ifdef SC64_CONTACT_MEMO_ENABLED
    SC64_CONTACT_MEMO_INVALIDATE();
#endif

    int slot = (int)(q - u), old_cell, new_cell;
    if (!sc64_unit_bucket_active || slot < 0 || slot >= MAX_UNITS) return;
    if (!q->alive || SC64_TRANSPORT_LOADED(q) || SC64_SCARAB_NONCOLLIDING(q)) {
        sc64_unit_bucket_remove(slot);
        return;
    }
    if (q->x < 0 || q->y < 0 || q->x >= WORLD_W || q->y >= WORLD_H) {
        sc64_unit_bucket_remove(slot);
        sc64_unit_bucket_active = 0;
        return;
    }
    old_cell = sc64_unit_bucket_cell[slot];
    new_cell = sc64_unit_bucket_cell_at(q->x, q->y);
    if (old_cell == new_cell) return;
    sc64_unit_bucket_remove(slot);
    sc64_unit_bucket_insert(slot, new_cell);
}

#ifdef SC64_CONTACT_CACHE
static int __attribute__((noinline))
sc64_unit_bucket_step_clear_reference(const Unit *q, int nx, int ny, int strict)
#else
static int GBA_HOT
sc64_unit_bucket_step_clear(const Unit *q, int nx, int ny)
#endif
{
#ifdef SC64_RACES
#ifdef SC64_CONTACT_CACHE
    if (!strict && IS_WORKER(q->type) &&
        (q->order == ORDER_MINE || q->order == ORDER_GAS)) return 1;
#else
    if(IS_WORKER(q->type)&&(q->order==ORDER_MINE||q->order==ORDER_GAS))return 1;
#endif
#ifdef SC64_ZP_CAMPAIGN_ACTORS
    int mover_radius = sc64_game_unit_collision_radius(q->type);
#endif
    /* These facts depend only on the mover, so resolve them once rather than
     * repeating the same table lookups for every nearby bucket candidate. */
    int mover_flying = sc64_race_unit_flying(q->type);
#if defined(SC64_T4_RUNTIME) || defined(SC64_ZP_CAMPAIGN_ACTORS)
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
                if (&u[i] != q && u[i].alive && !(SC64_TRANSPORT_LOADED(&u[i]) || SC64_SCARAB_NONCOLLIDING(&u[i]))) {
#ifdef SC64_P6_COMBAT_ENABLED
                    if (race_p6_combat_unit_burrowed(i)) continue;
#endif
                    int radius = 6;
#ifdef SC64_ZP_CAMPAIGN_ACTORS
                    int candidate_radius = sc64_game_unit_collision_radius(u[i].type);
                    radius = mover_radius > candidate_radius ? mover_radius : candidate_radius;
#endif
                    int dx = u[i].x - nx, dy = u[i].y - ny;
                    if (dx <= -radius || dx >= radius || dy <= -radius || dy >= radius) continue;
                    dx *= dx;
                    dy *= dy;
                    if (dx + dy >= radius * radius) continue;
#ifdef SC64_RACES
                    if(u[i].type==U_CHRYSALIS||sc64_race_unit_flying(u[i].type)!=mover_flying)continue;
#if defined(SC64_T4_RUNTIME) || defined(SC64_ZP_CAMPAIGN_ACTORS)
                    unsigned candidate_flags = sc64_game_unit_actor_flags(u[i].type);
                    if ((candidate_flags &&
                         !(candidate_flags & SC64_GAME_ACTOR_COLLIDABLE)) ||
                        !mover_collidable) continue;
#endif
#endif
                    int ox = u[i].x - q->x, oy = u[i].y - q->y;
#ifdef SC64_CONTACT_CACHE
                    if (strict || dx + dy <= ox * ox + oy * oy) return 0;
#else
                    if (dx + dy <= ox * ox + oy * oy) return 0;
#endif
                }
        }
    return 1;
}

#ifdef SC64_CONTACT_CACHE
#include "sc64_contact.h"
static int GBA_HOT
sc64_unit_bucket_step_clear(const Unit *q, int nx, int ny)
{
    if (IS_WORKER(q->type) && (q->order == ORDER_MINE || q->order == ORDER_GAS)) return 1;
    return sc64_contact_step_clear(q, nx, ny, 0);
}
#endif

static void
sc64_unit_buckets_end(void)
{
#ifdef SC64_CONTACT_MEMO_ENABLED
    SC64_CONTACT_MEMO_INVALIDATE();
#endif

    sc64_unit_bucket_active = 0;
}

#endif
