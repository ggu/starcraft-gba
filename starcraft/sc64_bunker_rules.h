/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_BUNKER_RULES_H
#define SC64_BUNKER_RULES_H
/* BWAPI pinned UnitType race/spaceRequired facts; these are all Terran
 * identities requiring one cargo point. Zero-space IDs are sentinels with
 * None/Unknown race, not Terran actors. Unknown identities fail closed. */
enum { SC64_BUNKER_CAPACITY = 4, SC64_BUNKER_RAW = 125 };
static inline int sc64_bunker_passenger_raw(int raw)
{
    switch (raw) {
    case 0: case 1: case 7: case 10: case 15: case 16:
    case 20: case 32: case 34: case 99: case 100: return 1;
    default: return 0;
    }
}
/* Human campaign allies can retain rescued source identities. Nonhuman
 * campaign owner slots can collapse multiple source players, so require
 * authenticated exact source ownership there. */
static inline int sc64_bunker_owner_matches(int owner, int raw, int carrier_owner,
                                             int carrier_raw, int custom, int human)
{
    if (owner<0 || owner>=5 || owner!=carrier_owner || raw < -1 || raw>=8 ||
        carrier_raw < -1 || carrier_raw>=8) return 0;
    if (custom || owner==human) return 1;
    return raw>=0 && carrier_raw>=0 && raw==carrier_raw;
}
/* OpenBW unit_is_marine/firebat/ghost; boardability remains separate. */
static inline int sc64_bunker_firing_raw(int raw)
{
    switch (raw) {
    case 0: case 1: case 10: case 16: case 20: case 32: case 99: case 100: return 1;
    default: return 0;
    }
}
static inline int sc64_bunker_target_encode(int units, int buildings, int slot)
{
    if (units<=0 || buildings<=0 || units+buildings>511 || slot<0 || slot>=buildings) return -1;
    return units+slot;
}
static inline int sc64_bunker_target_decode(int units, int buildings, int target)
{
    if (units<=0 || buildings<=0 || units+buildings>511 || target<units || target>=units+buildings) return -1;
    return target-units;
}
#endif
