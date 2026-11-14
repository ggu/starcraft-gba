/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * Observer purity for host controllers.
 *
 * The engine's route cache (starcraft/sc64_routes.h) is part of the
 * simulation: weapon-range movement reads a route field only when one is
 * resident, so which fields are cached changes game behaviour. A controller
 * that calls path_next_tile() or any helper that floods, touches cache age
 * or the clock, rebuilds the path grid or uses the BFS scratch therefore
 * perturbs the world it is reading, and a tape replay without the controller
 * cannot reproduce its run.
 *
 * Every static that route queries or the grid rebuild read or write must be
 * in route_pure_regions below (the route fields and bookkeeping, the BFS
 * scratch, path_walkable and path_grid_valid, and the incremental grid
 * update's record of what path_walkable was built from). A region left out
 * is silently kept across the restore, which breaks the purity assertion or,
 * worse, makes the restored state inconsistent.
 *
 * Include this header AFTER "../starcraft/game.c". It defines
 * route_pure_path_next_tile(), which runs path_next_tile() and then restores
 * every byte of engine route state to what it was. Nested use is allowed: only
 * the outermost scope snapshots and restores, so a caller may open
 * ROUTE_PURE_BEGIN()/ROUTE_PURE_END() around a loop of queries to pay for one
 * snapshot. route_purity_hash() covers the same regions and backs the assertion
 * that the state is unchanged between a game_update() return and the next call.
 * Host test code only; nothing here runs on the device.
 */
#ifndef SC64_ROUTE_PURITY_H
#define SC64_ROUTE_PURITY_H
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct { void *addr; size_t size; } RoutePureRegion;

#define RP_REGION(v) { (void *)&(v), sizeof(v) }
static const RoutePureRegion route_pure_regions[] = {
    /* Route cache: fields (or the shared pool and its base pointers), the
     * per-slot valid/goal/age arrays, the clock and the shape. */
#ifdef SC64_ROUTE_ACTIVE_STRIDE
    RP_REGION(sc64_route_pool),
    RP_REGION(sc64_route_field),
#ifdef SC64_ROUTE_POINT_POINTER_PROBE
    RP_REGION(sc64_route_point_plane),
    RP_REGION(sc64_route_point_spare),
    RP_REGION(sc64_route_point_saved),
#endif
    RP_REGION(sc64_route_active_bytes),
    RP_REGION(sc64_route_active_slots),
    RP_REGION(sc64_route_config_cols),
    RP_REGION(sc64_route_config_rows),
#else
    RP_REGION(sc64_route_field),
#endif
#ifdef SC64_ROUTE_ACTIVE_STRIDE
    /* Donor bookkeeping: logical->physical table, per-physical flags, log
     * counts, owner, goal and stamp, and the donor clock. */
    RP_REGION(sc64_route_lphys),
    RP_REGION(sc64_route_pcount),
    RP_REGION(sc64_route_pf_flags),
    RP_REGION(sc64_route_pf_logn),
    RP_REGION(sc64_route_pf_owner),
    RP_REGION(sc64_route_pf_goal),
    RP_REGION(sc64_route_pf_stamp),
    RP_REGION(sc64_route_donor_clock),
#ifdef SC64_ROUTE_KEEP_FIELDS
    /* Opened-cell log, grid-known/copied flags, remembered shape and the
     * query origin. The previous-grid copy lives in the BFS queue tail. */
    RP_REGION(sc64_route_open_log),
    RP_REGION(sc64_route_open_head),
    RP_REGION(sc64_route_open_count),
    RP_REGION(sc64_route_grid_known),
    RP_REGION(sc64_route_grid_copied),
    RP_REGION(sc64_route_keep_cols),
    RP_REGION(sc64_route_keep_rows),
    RP_REGION(sc64_route_query_origin),
#endif
#endif
    RP_REGION(sc64_route_valid),
    RP_REGION(sc64_route_goal),
    RP_REGION(sc64_route_age),
    RP_REGION(sc64_route_clock),
    RP_REGION(sc64_route_hits),
    RP_REGION(sc64_route_builds),
#ifdef SC64_RANGE_WORK
    /* Range-work jobs, queue, snapshot and copy state. */
    RP_REGION(sc64_range_route_work),
    RP_REGION(sc64_range_route_active),
    RP_REGION(sc64_range_route_pending),
    RP_REGION(sc64_range_route_complete_frame),
    RP_REGION(sc64_range_route_queue_head),
    RP_REGION(sc64_range_route_queue_count),
    RP_REGION(sc64_range_route_snapshot_slot),
    RP_REGION(sc64_range_route_copy_slot),
    RP_REGION(sc64_range_route_copy_cursor),
    RP_REGION(sc64_range_route_snapshot_key),
#ifdef GBA_HOST
    RP_REGION(sc64_range_route_last_units),
#endif
#endif
    /* Path grid, its valid flag and the BFS scratch. */
    RP_REGION(path_walkable),
    RP_REGION(path_grid_valid),
#ifdef SC64_PATH_GRID_INCREMENTAL
    /* The incremental update's record of the grid it last built: it must
     * travel with path_walkable or a restored grid would disagree with it.
     * Any new route, route-cache or grid state must be listed in this table. */
    RP_REGION(path_fp),
    RP_REGION(path_fp_key),
    RP_REGION(path_fp_valid),
#ifdef SC64_PATH_GRID_VERIFY
    RP_REGION(path_grid_verify_prev),
    RP_REGION(path_grid_verify_have),
#endif
#endif
    RP_REGION(bfs_queue),
#ifndef SC64_IMPORTED_MISSION
    RP_REGION(bfs_prev),
    RP_REGION(bfs_seen),
    RP_REGION(bfs_generation),
#endif
};
#undef RP_REGION
#define ROUTE_PURE_REGION_COUNT (sizeof(route_pure_regions)/sizeof(route_pure_regions[0]))

static uint64_t route_purity_hash(void)
{
    uint64_t h = 1469598103934665603ull;
    for (size_t i = 0; i < ROUTE_PURE_REGION_COUNT; ++i) {
        const unsigned char *p = (const unsigned char *)route_pure_regions[i].addr;
        for (size_t n = 0; n < route_pure_regions[i].size; ++n) {
            h ^= p[n];
            h *= 1099511628211ull;
        }
    }
    return h;
}

static unsigned char *route_pure_store;
static size_t route_pure_store_size;
static int route_pure_depth;
static unsigned long route_pure_scopes, route_pure_changed_scopes;
#ifdef ROUTE_PURITY_DEBUG
static uint64_t route_pure_scope_hash;
#endif

static void route_pure_begin(void)
{
    if (route_pure_depth++) return;
    if (!route_pure_store) {
        for (size_t i = 0; i < ROUTE_PURE_REGION_COUNT; ++i)
            route_pure_store_size += route_pure_regions[i].size;
        route_pure_store = (unsigned char *)malloc(route_pure_store_size);
    }
    unsigned char *q = route_pure_store;
    for (size_t i = 0; i < ROUTE_PURE_REGION_COUNT; ++i) {
        memcpy(q, route_pure_regions[i].addr, route_pure_regions[i].size);
        q += route_pure_regions[i].size;
    }
#ifdef ROUTE_PURITY_DEBUG
    route_pure_scope_hash = route_purity_hash();
#endif
}

static void route_pure_end(void)
{
    if (--route_pure_depth) return;
    unsigned char *q = route_pure_store;
    int changed = 0;
    for (size_t i = 0; i < ROUTE_PURE_REGION_COUNT; ++i) {
        if (memcmp(route_pure_regions[i].addr, q, route_pure_regions[i].size)) {
            changed = 1;
            memcpy(route_pure_regions[i].addr, q, route_pure_regions[i].size);
        }
        q += route_pure_regions[i].size;
    }
    ++route_pure_scopes;
    route_pure_changed_scopes += (unsigned long)changed;
#ifdef ROUTE_PURITY_DEBUG
    if (route_purity_hash() != route_pure_scope_hash) {
        fputs("route purity: state not restored after a wrapped query\n", stderr);
        abort();
    }
#endif
}

#define ROUTE_PURE_BEGIN() route_pure_begin()
#define ROUTE_PURE_END() route_pure_end()

static int route_pure_path_next_tile(int x, int y, int tx, int ty, int *nx, int *ny)
{
    route_pure_begin();
    int r = path_next_tile(x, y, tx, ty, nx, ny);
    route_pure_end();
    return r;
}
/* Every controller query goes through the pure wrapper. */
#ifndef ROUTE_PURE_DISABLE
#define path_next_tile route_pure_path_next_tile
#endif
#endif
