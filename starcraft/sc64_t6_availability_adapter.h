#ifndef SC64_T6_AVAILABILITY_ADAPTER_H
#define SC64_T6_AVAILABILITY_ADAPTER_H

#if defined(SC64_T6_RUNTIME) && defined(SC64_T4_RUNTIME)
#include "sc64_unit_availability.h"
#include "sc64_t6_availability_generated.h"

static int race_t6_active(void);

/* Resolve the source PUNI bank for a product request. Imported scenario
 * actors retain their original slot in raw_owner even after a rescue changes
 * their live controller. Unsupported owner/provenance combinations fail
 * closed instead of aliasing an unrelated player's availability. */
static int
race_t6_product_source_slot(int owner, int raw_owner)
{
    if (owner == SIDE) return 0;
    if (owner == ENEMY) return 1;
    if (owner == 4) return 4;
    if (owner == RESCUABLE && (raw_owner == 2 || raw_owner == 6))
        return raw_owner;
    return -1;
}

static int
race_t6_source_product_available(int source_slot, int raw_type)
{
    unsigned available = 0;
    if (source_slot < 0 || source_slot >= (int)SC64_PUNI_PLAYER_COUNT ||
        raw_type < 0 || raw_type >= (int)SC64_PUNI_UNIT_COUNT) return 0;
    if (!sc64_availability_bitmap_get(sc64_t6_availability_bits,
            SC64_T6_AVAILABILITY_BIT_BYTES, (unsigned)source_slot,
            (unsigned)raw_type, &available)) return 0;
    return available != 0;
}

/* Candidate validation intentionally bypasses race_t6_active(): a saved
 * T6 record is checked before live state is replaced. */
static int
race_t6_candidate_product_available(int source_slot, int raw_type)
{
    return race_t6_source_product_available(source_slot, raw_type);
}

static int
race_t6_unit_product_available(int owner, int raw_owner, int type)
{
    if (!race_t6_active()) return 1;
    return race_t6_source_product_available(
        race_t6_product_source_slot(owner, raw_owner),
        sc64_race_raw_unit(type));
}

static int
race_t6_building_product_available(int owner, int raw_owner, int type)
{
    if (!race_t6_active()) return 1;
    return race_t6_source_product_available(
        race_t6_product_source_slot(owner, raw_owner),
        sc64_race_raw_building(type));
}

#endif /* SC64_T6_RUNTIME && SC64_T4_RUNTIME */
#endif /* SC64_T6_AVAILABILITY_ADAPTER_H */
