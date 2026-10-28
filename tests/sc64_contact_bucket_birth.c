/* SPDX-License-Identifier: GPL-3.0-or-later */
/* A first movement after index construction must not unlink a stale slot. */
#include <assert.h>
#include <string.h>
#include "../common/gba.h"

/* Exercise the production index without requiring imported maps or art. */
#define MAX_UNITS 240
#define MAP_COLS 64
#define MAP_ROWS 64
#define MAP_CELLS (MAP_COLS * MAP_ROWS)
#define WORLD_W (MAP_COLS * 16)
#define WORLD_H (MAP_ROWS * 16)
enum { ORDER_IDLE, ORDER_TRANSPORTED };
typedef struct { int x, y, alive, order; } Unit;
static Unit u[MAX_UNITS];
#define SC64_TRANSPORT_LOADED(q) ((q)->order == ORDER_TRANSPORTED)
#include "../starcraft/sc64_collision_legacy.h"

static void
actor(int slot, int x, int y)
{
    memset(&u[slot], 0, sizeof(u[slot]));
    u[slot].alive = 1;
    u[slot].x = x;
    u[slot].y = y;
    u[slot].order = ORDER_IDLE;
}

static int
cell_has(int cell, int wanted)
{
    int i;
    for (i = sc64_unit_bucket_head[cell]; i != 0xff;
         i = sc64_unit_bucket_next[i])
        if (i == wanted) return 1;
    return 0;
}

int
main(void)
{
    int old_cell, new_cell;
    memset(u, 0, sizeof(u));
    actor(0, 10, 10);
    actor(1, 12, 10);
    sc64_unit_buckets_begin();
    assert(sc64_unit_bucket_active);
    old_cell = sc64_unit_bucket_cell_at(10, 10);
    assert(cell_has(old_cell, 0) && cell_has(old_cell, 1));

    /* A new slot has no bucket links despite its static old cell=0 value. */
    actor(2, 13, 10);
    sc64_unit_bucket_move(&u[2]);
    assert(cell_has(old_cell, 0) && cell_has(old_cell, 1) &&
           cell_has(old_cell, 2));

    /* Moving that linked actor must remove only its own old membership. */
    u[2].x = 100;
    sc64_unit_bucket_move(&u[2]);
    new_cell = sc64_unit_bucket_cell_at(100, 10);
    assert(cell_has(old_cell, 0) && cell_has(old_cell, 1));
    assert(!cell_has(old_cell, 2) && cell_has(new_cell, 2));
    assert(!sc64_unit_bucket_step_clear(&u[0], 12, 10));

    /* Another first movement crosses cells immediately after creation. */
    actor(3, 500, 500);
    sc64_unit_bucket_move(&u[3]);
    assert(cell_has(old_cell, 0) && cell_has(old_cell, 1));
    assert(cell_has(sc64_unit_bucket_cell_at(500, 500), 3));
    sc64_unit_buckets_end();
    assert(!sc64_unit_bucket_active);
    return 0;
}
