/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_SUPPLY_H
#define SC64_SUPPLY_H

/* One displayed supply point is two engine units (one Zergling uses one). */
enum { SC64_SUPPLY_LIMIT = 400 };

static inline int sc64_supply_limit(int capacity)
{
    return capacity < SC64_SUPPLY_LIMIT ? capacity : SC64_SUPPLY_LIMIT;
}

/* Zero-supply production can restore supply even when over the cap. */
static inline int sc64_supply_allows(int reserved, int needed, int capacity)
{
    return needed == 0 || needed <= sc64_supply_limit(capacity) - reserved;
}
#endif
