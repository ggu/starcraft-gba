#ifndef SC64_COLLISION_H
#define SC64_COLLISION_H

#if defined(SC64_COLLISION_CLASS_CACHE) && defined(SC64_LATE_TERRAN_STORAGE)
#error "SC64_COLLISION_CLASS_CACHE requires the per-frame legacy bucket lifecycle"
#endif

/* Persistent reconciliation saves a full per-frame rebuild for the largest
 * late Terran roster. Other profiles keep the proven original lifecycle. */
#ifdef SC64_LATE_TERRAN_STORAGE
#define SC64_PERSISTENT_COLLISION 1
#include "sc64_persistent_collision.h"
#else
#include "sc64_collision_legacy.h"
#endif

#endif
