#ifndef SC64_COLLISION_H
#define SC64_COLLISION_H

#if (defined(SC64_LATE_TERRAN_STORAGE) || defined(SC64_PERSISTENT_COLLISION_BUCKETS)) && \
    defined(SC64_NYDUS_ENABLED) && !defined(SC64_CONTACT_CACHE)
#error "persistent collision buckets with Nydus require the strict contact cache"
#endif

#if defined(SC64_COLLISION_CLASS_CACHE) && \
    (defined(SC64_LATE_TERRAN_STORAGE) || defined(SC64_PERSISTENT_COLLISION_BUCKETS))
#error "SC64_COLLISION_CLASS_CACHE requires the per-frame legacy bucket lifecycle"
#endif

/* Retain collision cells across updates and reconcile changed actor slots. */
#if defined(SC64_LATE_TERRAN_STORAGE) || defined(SC64_PERSISTENT_COLLISION_BUCKETS)
#define SC64_PERSISTENT_COLLISION 1
#include "sc64_persistent_collision.h"
#else
#include "sc64_collision_legacy.h"
#endif

#endif
