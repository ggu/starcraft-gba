/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_P7_SIGHT_POLICY_H
#define SC64_P7_SIGHT_POLICY_H

/* Shared reference policy for P7 optical visibility and its offline table
 * generator. `source_pixels` comes from the authored Z/P sight provider. */
static inline int sc64_p7_sight_radius_policy(int type, int building,
                                               int source_pixels)
{
    if (source_pixels > 0) return source_pixels / 2;
    if (building)
        return (type == B_COMMAND || sc64_race_is_zerg_townhall(type) ||
                type == B_NEXUS) ? 112 : 64;
    return sc64_race_is_worker(type) ? 56 : 72;
}

#endif
