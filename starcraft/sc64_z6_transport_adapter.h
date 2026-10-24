#ifndef SC64_Z6_TRANSPORT_ADAPTER_H
#define SC64_Z6_TRANSPORT_ADAPTER_H

#if defined(SC64_Z6_TRANSPORT_ENABLED)
#if !defined(SC64_Z6_RUNTIME) || !defined(SC64_TRANSPORT_RUNTIME) || \
    !defined(SC64_Z6_ACTORS)
#error "Z6 Ventral Sacs requires the Z6 actor/runtime and shared transport runtime"
#endif

#include "sc64_z6_transport_tech.h"

_Static_assert(SC64_Z6_TRANSPORT_FACTS_COMPLETE == 1u &&
               SC64_Z6_TRANSPORT_SOURCE_GLOBAL_ID == SC64_Z6_GLOBAL_ID,
               "Z6 transport technology is not authenticated for this campaign");

/* The source adapter has one human-controlled Zerg slot. Resolve the research
 * producer by its preserved source identity; never accept a generic Hatchery
 * or a building merely because it shares the runtime Lair enum. */
static int race_z6_transport_research_producer_valid(int slot)
{
    const Building *q;
    if (!race_z6_active() || race_z6_source_owner(SC64_Z6_TRANSPORT_TECH_SOURCE_OWNER) != SIDE ||
        slot < 0 || slot >= MAX_BUILDINGS) return 0;
    q = &b[slot];
    return sc64_z6_transport_researcher_valid((unsigned)slot,
        (unsigned)q->raw_type, (unsigned)q->raw_owner,
        q->owner == SIDE, q->alive && q->hp > 0,
        q->alive && !q->under_construction,
        q->alive && !q->under_construction);
}

static int race_z6_transport_metadata_valid(void)
{
    return race_z6_active() && sc64_z6_transport_tech_facts_valid() &&
        SC64_Z6_TRANSPORT_FACTS_COMPLETE == 1u &&
        SC64_Z6_TRANSPORT_SOURCE_OVERLORD_ALLOWED == 1u &&
        SC64_Z6_TRANSPORT_SOURCE_LAIR_ALLOWED == 1u &&
        SC64_Z6_TRANSPORT_SOURCE_HIVE_ALLOWED == 1u;
}

static int race_z6_transport_state_valid(void)
{
    if (!race_z6_transport_metadata_valid() ||
        !sc64_z6_transport_tech_state_valid(&race_z6.transport)) return 0;
    if (!race_z6.transport.ticks_remaining) return 1;
    {
        int slot = (int)race_z6.transport.producer_slot_plus1 - 1;
        return slot >= 0 && slot < MAX_BUILDINGS &&
            race_z6_transport_research_producer_valid(slot);
    }
}

static int race_z6_transport_initialize_source(void)
{
    if (!race_z6_transport_metadata_valid()) return 0;
    /* The authenticated source starts level 0. This initializer does not
     * grant the upgrade or cargo; only a paid Lair/Hive research completes. */
    return sc64_z6_transport_tech_init(&race_z6.transport);
}

static int race_z6_transport_research_selected(void)
{
    return race_z6_transport_metadata_valid() && selected_building >= 0 &&
        selected_building < MAX_BUILDINGS &&
        race_z6_transport_research_producer_valid(selected_building) &&
        !race_z6.transport.researched;
}

static int race_z6_transport_research_menu_available(void)
{
    return race_z6_transport_metadata_valid() &&
        sc64_z6_transport_tech_state_valid(&race_z6.transport) &&
        !race_z6.transport.researched && !race_z6.transport.ticks_remaining &&
        selected_building >= 0 && selected_building < MAX_BUILDINGS &&
        race_z6_transport_research_producer_valid(selected_building) &&
        minerals >= (int)SC64_Z6_TRANSPORT_MINERAL_COST &&
        gas >= (int)SC64_Z6_TRANSPORT_GAS_COST;
}

static int race_z6_transport_start_research_selected(void)
{
    int result;
    if (!race_z6_transport_research_menu_available())
        return SC64_Z6_TRANSPORT_TECH_ERROR;
    result = sc64_z6_transport_tech_start(&race_z6.transport,
        (unsigned)selected_building, (unsigned)b[selected_building].raw_type,
        (unsigned)b[selected_building].raw_owner,
        b[selected_building].owner == SIDE,
        b[selected_building].alive && b[selected_building].hp > 0,
        b[selected_building].alive && !b[selected_building].under_construction,
        b[selected_building].alive && !b[selected_building].under_construction,
        &minerals, &gas);
    return result;
}

/* Pinned OpenBW cancels an upgrade on producer destruction and refunds 75%
 * of its mineral/gas cost. Call before the building slot can be reused. */
static void race_z6_transport_note_building_death(int slot)
{
    if (!race_z6_active() || slot < 0 ||
        !race_z6.transport.ticks_remaining ||
        (int)race_z6.transport.producer_slot_plus1 - 1 != slot) return;
    (void)sc64_z6_transport_tech_cancel(&race_z6.transport, 1,
        &minerals, &gas);
}

static int race_z6_transport_research_tick(int paused)
{
    int slot, valid, result;
    if ((paused != 0 && paused != 1) || !race_z6_transport_metadata_valid() ||
        !sc64_z6_transport_tech_state_valid(&race_z6.transport)) return 0;
    if (paused) return 1;
    if (!race_z6.transport.ticks_remaining) return 1;
    slot = (int)race_z6.transport.producer_slot_plus1 - 1;
    if (slot < 0 || slot >= MAX_BUILDINGS) {
        (void)sc64_z6_transport_tech_cancel(&race_z6.transport, 1,
            &minerals, &gas);
        return 1;
    }
    if (!b[slot].alive || b[slot].hp <= 0) {
        race_z6_transport_note_building_death(slot);
        return 1;
    }
    if (b[slot].owner != SIDE || b[slot].raw_owner != SC64_Z6_TRANSPORT_TECH_SOURCE_OWNER ||
        (b[slot].raw_type != SC64_Z6_TRANSPORT_TECH_RAW_LAIR &&
         b[slot].raw_type != SC64_Z6_TRANSPORT_TECH_RAW_HIVE)) {
        /* A live ownership transfer cancels with the native full refund. */
        (void)sc64_z6_transport_tech_cancel(&race_z6.transport, 0,
            &minerals, &gas);
        return 1;
    }
    valid = race_z6_transport_research_producer_valid(slot);
    if (!valid) return 0;
    result = sc64_z6_transport_tech_tick(&race_z6.transport, 0, 1);
    return result != SC64_Z6_TRANSPORT_TECH_ERROR;
}

static int race_z6_transport_overlord_ready(void)
{
    return race_z6_transport_metadata_valid() &&
        sc64_z6_transport_tech_state_valid(&race_z6.transport) &&
        race_z6.transport.researched == 1u;
}

#endif /* SC64_Z6_TRANSPORT_ENABLED */

#endif
