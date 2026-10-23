#ifndef SC64_COLLISION_H
#define SC64_COLLISION_H

/* Persistent reconciliation saves a full per-frame rebuild for the largest
 * late Terran roster. Other profiles keep the proven original lifecycle. */
#ifdef SC64_LATE_TERRAN_STORAGE
#define SC64_PERSISTENT_COLLISION 1
#include "sc64_persistent_collision.h"
#else
#include "sc64_collision_legacy.h"
#endif

#endif
