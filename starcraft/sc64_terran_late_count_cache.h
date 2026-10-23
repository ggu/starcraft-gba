#ifndef SC64_TERRAN_LATE_COUNT_CACHE_H
#define SC64_TERRAN_LATE_COUNT_CACHE_H

#include "sc64_terran_late_runtime.h"

#if defined(SC64_TERRAN_LATE_RUNTIME) && defined(SC64_T4_RUNTIME)
/* Late trigger counts are immutable only until a delegated world action runs.
 * Keep this small memo table scoped to one late-world tick and clear it before
 * every action delegation. Overflow simply falls through to the source query. */
enum { SC64_TERRAN_LATE_COUNT_CACHE_LIMIT = 8 };

typedef struct {
    const Sc64T4Condition *condition;
    u32 count;
    u8 owner;
} Sc64TerranLateCountCacheEntry;

typedef struct {
    const Sc64TerranLateRuntimeOps *source_ops;
    void *source_context;
    Sc64TerranLateCountCacheEntry entries[SC64_TERRAN_LATE_COUNT_CACHE_LIMIT];
    u8 used;
} Sc64TerranLateCountCache;

_Static_assert(sizeof(Sc64TerranLateCountCache) <= 256,
               "late trigger count cache storage budget");

static int
sc64_terran_late_count_cache_count(void *context,
                                   const Sc64T4Condition *condition,
                                   unsigned owner, u32 *count)
{
    Sc64TerranLateCountCache *cache = context;
    if (!cache || !cache->source_ops || !cache->source_ops->count ||
        !condition || !count || owner >= 8 ||
        cache->used > SC64_TERRAN_LATE_COUNT_CACHE_LIMIT)
        return 0;

    /* Current Player and selector 26's opposing-force relation depend on the
     * trigger source. All remaining supported selectors are source-invariant. */
    unsigned key_owner = condition->player == 13 || condition->player == 26 ?
                         owner : 0;
    for (unsigned i = 0; i < cache->used; ++i) {
        Sc64TerranLateCountCacheEntry *entry = &cache->entries[i];
        if (entry->condition == condition && entry->owner == key_owner) {
            *count = entry->count;
            return 1;
        }
    }

    u32 result;
    int status = cache->source_ops->count(cache->source_context, condition,
                                          owner, &result);
    if (status != 1) return status;
    *count = result;
    if (cache->used < SC64_TERRAN_LATE_COUNT_CACHE_LIMIT) {
        Sc64TerranLateCountCacheEntry *entry = &cache->entries[cache->used++];
        entry->condition = condition;
        entry->count = result;
        entry->owner = (u8)key_owner;
    }
    return 1;
}

static int
sc64_terran_late_count_cache_action(void *context,
                                    const Sc64T4Action *action,
                                    unsigned owner)
{
    Sc64TerranLateCountCache *cache = context;
    if (!cache || !cache->source_ops || !cache->source_ops->action) return -1;
    cache->used = 0;
    return cache->source_ops->action(cache->source_context, action, owner);
}

static const Sc64TerranLateRuntimeOps sc64_terran_late_count_cache_ops = {
    sc64_terran_late_count_cache_count,
    sc64_terran_late_count_cache_action
};

static void
sc64_terran_late_count_cache_init(Sc64TerranLateCountCache *cache,
                                  const Sc64TerranLateRuntimeOps *source_ops,
                                  void *source_context)
{
    if (!cache) return;
    cache->source_ops = source_ops;
    cache->source_context = source_context;
    cache->used = 0;
}
#endif

#endif
