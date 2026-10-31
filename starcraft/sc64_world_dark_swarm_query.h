/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_WORLD_DARK_SWARM_QUERY_H
#define SC64_WORLD_DARK_SWARM_QUERY_H
/* The current engine has no Dark Swarm creator or active-effect storage.
 * Bind this shared query to that storage when the capability is added;
 * do not allow a newly enabled implementation to silently return empty. */
#ifdef SC64_SHARED_DARK_SWARM
#error "Bind sc64_world_dark_swarm_affects_target to the shared effect state"
#endif
static int sc64_world_dark_swarm_affects_target(int target)
{ (void)target; return 0; }
#endif
