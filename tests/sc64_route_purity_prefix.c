/* SPDX-License-Identifier: GPL-3.0-or-later */
/* The observer transaction must preserve every prefix frontier and hint,
 * including nested scopes. Built with the prepared Z10/P10 feature flags. */
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "../starcraft/game.c"
#include "sc64_route_purity.h"
#if !defined(SC64_ROUTE_POINT_PREFIX_PROBE) || !defined(SC64_ROUTE_SMALL_COMPONENT_PROBE)
#error "requires the current point-prefix and small-component flags"
#endif
int main(void)
{
    memset(sc64_point_prefixes, 0x35, sizeof(sc64_point_prefixes));
    memset(sc64_point_prefix_index, 0x17, sizeof(sc64_point_prefix_index));
    sc64_small_route_first = 11; sc64_small_route_last = 19;
    Sc64PointPrefix saved[SC64_POINT_PREFIX_STORAGE];
    u8 index[SC64_ROUTE_SLOTS];
    memcpy(saved, sc64_point_prefixes, sizeof saved);
    memcpy(index, sc64_point_prefix_index, sizeof index);
    uint64_t before = route_purity_hash();
    ROUTE_PURE_BEGIN();
    ROUTE_PURE_BEGIN();
    sc64_point_prefixes[SC64_POINT_PREFIX_STORAGE - 1].count ^= 1;
    sc64_point_prefix_index[SC64_ROUTE_SLOTS - 1] ^= 1;
    sc64_small_route_first ^= 1; sc64_small_route_last ^= 1;
    assert(route_purity_hash() != before);
    ROUTE_PURE_END();
    assert(route_purity_hash() != before);
    ROUTE_PURE_END();
    assert(route_purity_hash() == before);
    assert(!memcmp(saved, sc64_point_prefixes, sizeof saved));
    assert(!memcmp(index, sc64_point_prefix_index, sizeof index));
    assert(sc64_small_route_first == 11 && sc64_small_route_last == 19);
    puts("PASS observer restores complete point prefixes and component hints");
    return 0;
}
