/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef FRONTIER_ROUTE_CACHE_H
#define FRONTIER_ROUTE_CACHE_H

/* Direct-mapped transient point-query memo. Results -1 and -2 mean failure
 * and return the caller's original target pixels, respectively. */
typedef struct {
    u16 start, goal, generation;
    short result;
} FrontierRouteCacheEntry;
static FrontierRouteCacheEntry frontier_route_cache[64];
static u16 frontier_route_cache_generation = 1;

static void frontier_route_cache_invalidate(void)
{
    if (++frontier_route_cache_generation == 0) {
        memset(frontier_route_cache, 0, sizeof(frontier_route_cache));
        frontier_route_cache_generation = 1;
    }
}

static FrontierRouteCacheEntry *frontier_route_cache_slot(int start, int goal)
{
    unsigned hash = (unsigned)start * 37u + (unsigned)goal * 101u;
    hash ^= hash >> 6;
    return &frontier_route_cache[hash & 63u];
}

/* -1 means miss, 0 means cached failure, 1 means cached success. */
static int frontier_route_cache_get(int start, int goal, int tx, int ty,
                                    int *next_x, int *next_y)
{
    FrontierRouteCacheEntry *entry = frontier_route_cache_slot(start, goal);
    int result;
    if (entry->generation != frontier_route_cache_generation ||
        entry->start != start || entry->goal != goal) return -1;
    result = entry->result;
    if (result == -1) return 0;
    if (result == -2) { *next_x = tx; *next_y = ty; }
    else { *next_x = (result & 255) * 16 + 8; *next_y = (result >> 8) * 16 + 8; }
    return 1;
}

static void frontier_route_cache_put(int start, int goal, int result)
{
    FrontierRouteCacheEntry *entry = frontier_route_cache_slot(start, goal);
    entry->start = (u16)start;
    entry->goal = (u16)goal;
    entry->result = (short)result;
    entry->generation = frontier_route_cache_generation;
}
#endif
