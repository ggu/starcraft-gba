#ifndef SC64_T4_COUNT_CACHE_H
#define SC64_T4_COUNT_CACHE_H

#include "sc64_t4_triggers.h"

#ifdef SC64_T4_RUNTIME
/* Per-tick memoization for immutable generated condition records. This cache
 * assumes the wrapped count callback varies by owner only for selector 13
 * (Current Player); all other selector results are shared across contexts.
 * The caller must initialize a fresh cache for each game tick. Actions clear
 * the cache before delegation because they may change entities or locations.
 * Failures and a valid full cache fall through to the original callback;
 * malformed cache state is rejected without touching the output. */
enum { SC64_T4_COUNT_CACHE_LIMIT = 64 };

typedef struct {
    const Sc64T4Condition *condition;
    u32 count;
    u8 owner;
} Sc64T4CountCacheEntry;

typedef struct {
    const Sc64T4TriggerOps *source_ops;
    void *source_context;
    Sc64T4CountCacheEntry entries[SC64_T4_COUNT_CACHE_LIMIT];
    u8 used;
} Sc64T4CountCache;
_Static_assert(sizeof(Sc64T4CountCache) <= 1200,
               "T4 per-tick count cache stack budget");

static int
sc64_t4_count_cache_count(void *context, const Sc64T4Condition *condition,
                          unsigned owner, u32 *count)
{
    Sc64T4CountCache *cache = context;
    if (!cache || !cache->source_ops || !cache->source_ops->count ||
        !condition || !count || owner >= 8 ||
        cache->used > SC64_T4_COUNT_CACHE_LIMIT)
        return 0;

    /* Only Current Player (13) resolves relative to the trigger instance. */
    unsigned key_owner = condition->player == 13 ? owner : 0;

    for (unsigned i = 0; i < cache->used; ++i) {
        Sc64T4CountCacheEntry *entry = &cache->entries[i];
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

    if (cache->used < SC64_T4_COUNT_CACHE_LIMIT) {
        Sc64T4CountCacheEntry *entry = &cache->entries[cache->used++];
        entry->condition = condition;
        entry->count = result;
        entry->owner = (u8)key_owner;
    }
    return 1;
}

static int
sc64_t4_count_cache_action(void *context, const Sc64T4Action *action,
                           unsigned owner)
{
    Sc64T4CountCache *cache = context;
    if (!cache || !cache->source_ops || !cache->source_ops->action) return -1;
    /* Even a retry or unsupported action may have implementation-specific
     * side effects, so invalidate before every delegation. */
    cache->used = 0;
    return cache->source_ops->action(cache->source_context, action, owner);
}

static const Sc64T4TriggerOps sc64_t4_count_cache_ops = {
    sc64_t4_count_cache_count,
    sc64_t4_count_cache_action
};

static void
sc64_t4_count_cache_init(Sc64T4CountCache *cache,
                         const Sc64T4TriggerOps *source_ops,
                         void *source_context)
{
    if (!cache) return;
    cache->source_ops = source_ops;
    cache->source_context = source_context;
    cache->used = 0;
}
#endif

#endif
