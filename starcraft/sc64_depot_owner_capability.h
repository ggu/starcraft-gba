/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_DEPOT_OWNER_CAPABILITY_H
#define SC64_DEPOT_OWNER_CAPABILITY_H
/* Scalar override wins even when an old experimental flag is also passed. */
#ifdef SC64_DEPOT_OWNER_LIST_DISABLED
#undef SC64_DEPOT_OWNER_LIST_PROBE
#elif defined(SC64_NEAREST_RESOURCE_DEPOT)
#ifndef SC64_DEPOT_OWNER_LIST_PROBE
#define SC64_DEPOT_OWNER_LIST_PROBE 1
#endif
#endif
#endif
