#ifndef SC64_T5_AVAILABILITY_ADAPTER_H
#define SC64_T5_AVAILABILITY_ADAPTER_H

#ifdef SC64_T5_RUNTIME
#include "sc64_unit_availability.h"
#include "sc64_t5_availability_generated.h"

static int race_t5_active(void);

/* T5 source player selection follows the current controller. Rescued groups
 * retain raw owners 2/3, which are the only supported rescue-source banks. */
static int race_t5_product_source_slot(int owner, int raw_owner)
{
    if (owner == SIDE) return 0;
    if (owner == ENEMY) return 6;
    if (owner == RESCUABLE && (raw_owner == 2 || raw_owner == 3))
        return raw_owner;
    return -1;
}

static int race_t5_source_product_available(int source_slot, int raw_id)
{
    unsigned available = 0;
    if (source_slot < 0 || source_slot >= (int)SC64_PUNI_PLAYER_COUNT ||
        raw_id < 0 || raw_id >= (int)SC64_PUNI_UNIT_COUNT) return 0;
    if (!sc64_availability_bitmap_get(sc64_t5_availability_bits,
            SC64_T5_AVAILABILITY_BIT_BYTES, (unsigned)source_slot,
            (unsigned)raw_id, &available)) return 0;
    return available != 0;
}

static int race_t5_unit_product_available(int owner, int raw_owner, int type)
{
    int raw;
    if (!race_t5_active()) return 1;
    raw = sc64_race_raw_unit(type);
    return race_t5_source_product_available(
        race_t5_product_source_slot(owner, raw_owner), raw);
}

static int race_t5_building_product_available(int owner, int raw_owner, int type)
{
    int raw;
    if (!race_t5_active()) return 1;
    raw = sc64_race_raw_building(type);
    return race_t5_source_product_available(
        race_t5_product_source_slot(owner, raw_owner), raw);
}

/* Check the serialized producer's current owner and preserved source owner.
 * This deliberately keys off the candidate save, not the live mission state,
 * so corrupt saves are rejected before any live world field is overwritten. */
static int race_t5_candidate_queues_available(const RaceSave *candidate)
{
    if (!candidate) return 0;
    if (candidate->state[0] != 0 || candidate->state[2] != SC64_T5_GLOBAL_ID)
        return 1;
    for (int i = 0; i < MAX_BUILDINGS; ++i) {
        const s16 *row = candidate->buildings[i];
        int alive = row[offsetof(Building, alive) / sizeof(int)];
        int owner = row[offsetof(Building, owner) / sizeof(int)];
        int raw_owner = row[offsetof(Building, raw_owner) / sizeof(int)];
        int count = row[offsetof(Building, queue_count) / sizeof(int)];
        int head = row[offsetof(Building, queue_head) / sizeof(int)];
        if (!alive) continue;
        if (count < 0 || count > 5 || head < 0 || head > 4) return 0;
        for (int q = 0; q < count; ++q) {
            int type = row[offsetof(Building, queue) / sizeof(int) +
                           (head + q) % 5];
            int source_slot, raw;
            if (type < 0 || type >= SC_RU_COUNT) return 0;
            source_slot = race_t5_product_source_slot(owner, raw_owner);
            raw = sc64_race_raw_unit(type);
            if (!race_t5_source_product_available(source_slot, raw))
                return 0;
        }
    }
    return 1;
}
#endif /* SC64_T5_RUNTIME */

#endif /* SC64_T5_AVAILABILITY_ADAPTER_H */
