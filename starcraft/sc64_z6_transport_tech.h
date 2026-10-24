#ifndef SC64_Z6_TRANSPORT_TECH_H
#define SC64_Z6_TRANSPORT_TECH_H

#include "../common/gba.h"
#include "sc64_z6_transport_generated.h"

enum {
    SC64_Z6_TRANSPORT_TECH_UPGRADE_ID = 24,
    SC64_Z6_TRANSPORT_TECH_SOURCE_OWNER = 3,
    SC64_Z6_TRANSPORT_TECH_RAW_OVERLORD = 42,
    SC64_Z6_TRANSPORT_TECH_RAW_HATCHERY = 131,
    SC64_Z6_TRANSPORT_TECH_RAW_LAIR = 132,
    SC64_Z6_TRANSPORT_TECH_RAW_HIVE = 133,
    SC64_Z6_TRANSPORT_TECH_SOURCE_HZ = 24,
    SC64_Z6_TRANSPORT_TECH_GAME_HZ = 30,
    SC64_Z6_TRANSPORT_TECH_MAX_UNITS = 240,
    SC64_Z6_TRANSPORT_TECH_DESTROY_REFUND_PERCENT = 75,
    SC64_Z6_TRANSPORT_TECH_CANCEL_REFUND_PERCENT = 100,
    SC64_Z6_TRANSPORT_TECH_IDLE = 0,
    SC64_Z6_TRANSPORT_TECH_RESEARCHING = 1,
    SC64_Z6_TRANSPORT_TECH_RESEARCHED = 2,
    SC64_Z6_TRANSPORT_TECH_CANCELLED = 3,
    SC64_Z6_TRANSPORT_TECH_ERROR = -1
};

/* Progress is authored at 30Hz from the pinned 24Hz scenario timer. A
 * nonzero producer_slot_plus1 binds it to the exact completed Lair/Hive that
 * started the work; the world adapter validates that identity every tick. */
typedef struct {
    u16 ticks_remaining;
    u8 producer_slot_plus1;
    u8 researched;
} Sc64Z6TransportTechState;

_Static_assert(sizeof(Sc64Z6TransportTechState) == 4,
               "Z6 transport-tech state must remain four bytes");
_Static_assert(SC64_Z6_TRANSPORT_FACTS_COMPLETE == 1u &&
               SC64_Z6_TRANSPORT_SOURCE_GLOBAL_ID == 17u,
               "Z6 transport facts must match the authenticated source map");
_Static_assert(SC64_Z6_TRANSPORT_UPGRADE_ID == SC64_Z6_TRANSPORT_TECH_UPGRADE_ID &&
               SC64_Z6_TRANSPORT_SOURCE_OWNER == SC64_Z6_TRANSPORT_TECH_SOURCE_OWNER &&
               SC64_Z6_TRANSPORT_OVERLORD_RAW_TYPE == SC64_Z6_TRANSPORT_TECH_RAW_OVERLORD &&
               SC64_Z6_TRANSPORT_LAIR_RAW_TYPE == SC64_Z6_TRANSPORT_TECH_RAW_LAIR &&
               SC64_Z6_TRANSPORT_HIVE_RAW_TYPE == SC64_Z6_TRANSPORT_TECH_RAW_HIVE &&
               SC64_Z6_TRANSPORT_SOURCE_MAX_LEVEL == 1u &&
               SC64_Z6_TRANSPORT_SOURCE_INITIAL_LEVEL == 0u &&
               SC64_Z6_TRANSPORT_SOURCE_UPGRADE_USES_GLOBAL_DEFAULT == 1u &&
               SC64_Z6_TRANSPORT_USES_DEFAULT_COSTS == 1u &&
               SC64_Z6_TRANSPORT_MINERAL_COST == 200u &&
               SC64_Z6_TRANSPORT_MINERAL_FACTOR == 0u &&
               SC64_Z6_TRANSPORT_GAS_COST == 200u &&
               SC64_Z6_TRANSPORT_GAS_FACTOR == 0u &&
               SC64_Z6_TRANSPORT_TIME_COST_SOURCE_FRAMES == 2400u &&
               SC64_Z6_TRANSPORT_TIME_FACTOR == 0u,
               "authenticated Ventral Sacs facts changed");

#define SC64_Z6_TRANSPORT_RESEARCH_TICKS \
    ((SC64_Z6_TRANSPORT_TIME_COST_SOURCE_FRAMES * SC64_Z6_TRANSPORT_TECH_GAME_HZ + \
      SC64_Z6_TRANSPORT_TECH_SOURCE_HZ - 1u) / SC64_Z6_TRANSPORT_TECH_SOURCE_HZ)

static inline int sc64_z6_transport_tech_facts_valid(void)
{
    return SC64_Z6_TRANSPORT_FACTS_COMPLETE == 1u &&
        SC64_Z6_TRANSPORT_SOURCE_MAX_LEVEL == 1u &&
        SC64_Z6_TRANSPORT_SOURCE_INITIAL_LEVEL == 0u &&
        SC64_Z6_TRANSPORT_SOURCE_UPGRADE_USES_GLOBAL_DEFAULT == 1u &&
        SC64_Z6_TRANSPORT_USES_DEFAULT_COSTS == 1u &&
        SC64_Z6_TRANSPORT_MINERAL_COST == 200u &&
        SC64_Z6_TRANSPORT_MINERAL_FACTOR == 0u &&
        SC64_Z6_TRANSPORT_GAS_COST == 200u &&
        SC64_Z6_TRANSPORT_GAS_FACTOR == 0u &&
        SC64_Z6_TRANSPORT_TIME_COST_SOURCE_FRAMES == 2400u &&
        SC64_Z6_TRANSPORT_TIME_FACTOR == 0u &&
        SC64_Z6_TRANSPORT_SOURCE_OVERLORD_ALLOWED == 1u &&
        SC64_Z6_TRANSPORT_SOURCE_LAIR_ALLOWED == 1u &&
        SC64_Z6_TRANSPORT_SOURCE_HIVE_ALLOWED == 1u &&
        SC64_Z6_TRANSPORT_STARTING_OVERLORD_COUNT == 3u &&
        SC64_Z6_TRANSPORT_STARTING_HATCHERY_COUNT == 1u &&
        SC64_Z6_TRANSPORT_STARTING_LAIR_COUNT == 0u &&
        SC64_Z6_TRANSPORT_STARTING_HIVE_COUNT == 0u &&
        SC64_Z6_TRANSPORT_RESEARCH_TICKS == 3000u;
}

static inline int sc64_z6_transport_tech_state_valid(
    const Sc64Z6TransportTechState *state)
{
    if (!state || state->researched > 1u ||
        state->ticks_remaining > SC64_Z6_TRANSPORT_RESEARCH_TICKS)
        return 0;
    if (state->researched)
        return state->ticks_remaining == 0u && state->producer_slot_plus1 == 0u;
    if (!state->ticks_remaining)
        return state->producer_slot_plus1 == 0u;
    return state->producer_slot_plus1 > 0u &&
        state->producer_slot_plus1 <= SC64_Z6_TRANSPORT_TECH_MAX_UNITS;
}

static inline int sc64_z6_transport_tech_init(Sc64Z6TransportTechState *state)
{
    if (!state || !sc64_z6_transport_tech_facts_valid()) return 0;
    state->ticks_remaining = 0;
    state->producer_slot_plus1 = 0;
    state->researched = (u8)SC64_Z6_TRANSPORT_SOURCE_INITIAL_LEVEL;
    return sc64_z6_transport_tech_state_valid(state);
}

/* A valid source researcher is a completed, idle raw132/133 building with
 * original owner3 provenance, currently owned by the player. The caller
 * supplies live-array facts so this pure check is also easy to test. */
static inline int sc64_z6_transport_researcher_valid(unsigned slot,
    unsigned raw_type, unsigned raw_owner, int live_owner_is_player,
    int alive, int complete, int idle)
{
    return slot < SC64_Z6_TRANSPORT_TECH_MAX_UNITS &&
        (raw_type == SC64_Z6_TRANSPORT_TECH_RAW_LAIR ||
         raw_type == SC64_Z6_TRANSPORT_TECH_RAW_HIVE) &&
        raw_owner == SC64_Z6_TRANSPORT_TECH_SOURCE_OWNER &&
        live_owner_is_player == 1 && alive == 1 && complete == 1 && idle == 1;
}

/* Pay once, then bind the full timer to the source producer slot. Returns
 * RESEARCHING on success and ERROR without changing any input on rejection. */
static inline int sc64_z6_transport_tech_start(
    Sc64Z6TransportTechState *state, unsigned slot, unsigned raw_type,
    unsigned raw_owner, int live_owner_is_player, int alive, int complete,
    int idle, int *minerals, int *gas)
{
    if (!state || !minerals || !gas || !sc64_z6_transport_tech_state_valid(state) ||
        !sc64_z6_transport_tech_facts_valid() || state->researched ||
        state->ticks_remaining || !sc64_z6_transport_researcher_valid(
            slot, raw_type, raw_owner, live_owner_is_player, alive, complete, idle) ||
        *minerals < (int)SC64_Z6_TRANSPORT_MINERAL_COST ||
        *gas < (int)SC64_Z6_TRANSPORT_GAS_COST) return SC64_Z6_TRANSPORT_TECH_ERROR;
    *minerals -= (int)SC64_Z6_TRANSPORT_MINERAL_COST;
    *gas -= (int)SC64_Z6_TRANSPORT_GAS_COST;
    state->ticks_remaining = (u16)SC64_Z6_TRANSPORT_RESEARCH_TICKS;
    state->producer_slot_plus1 = (u8)(slot + 1u);
    return sc64_z6_transport_tech_state_valid(state) ?
        SC64_Z6_TRANSPORT_TECH_RESEARCHING : SC64_Z6_TRANSPORT_TECH_ERROR;
}

/* Destroyed producers return the source's 75% cost refund; canceled or
 * ownership-lost upgrades return their full cost. Refunds are bounded to the
 * runtime's 16-bit resource bank. No unlock occurs on cancellation. */
static inline int sc64_z6_transport_tech_cancel(
    Sc64Z6TransportTechState *state, int destroyed, int *minerals, int *gas)
{
    unsigned percent = destroyed ? SC64_Z6_TRANSPORT_TECH_DESTROY_REFUND_PERCENT :
        SC64_Z6_TRANSPORT_TECH_CANCEL_REFUND_PERCENT;
    if (!state || !minerals || !gas || !sc64_z6_transport_tech_state_valid(state) ||
        !state->ticks_remaining || !state->producer_slot_plus1 ||
        (destroyed != 0 && destroyed != 1)) return SC64_Z6_TRANSPORT_TECH_ERROR;
    *minerals += (int)(SC64_Z6_TRANSPORT_MINERAL_COST * percent / 100u);
    *gas += (int)(SC64_Z6_TRANSPORT_GAS_COST * percent / 100u);
    if (*minerals > 65535) *minerals = 65535;
    if (*gas > 65535) *gas = 65535;
    state->ticks_remaining = 0;
    state->producer_slot_plus1 = 0;
    return SC64_Z6_TRANSPORT_TECH_CANCELLED;
}

/* Advance only while the producer identity remains valid and the game is
 * active. Producer destruction must be reported separately so its 75% refund
 * can be applied exactly once by the death hook. */
static inline int sc64_z6_transport_tech_tick(
    Sc64Z6TransportTechState *state, int paused, int producer_valid)
{
    if (!state || !sc64_z6_transport_tech_state_valid(state) ||
        (paused != 0 && paused != 1) || (producer_valid != 0 && producer_valid != 1))
        return SC64_Z6_TRANSPORT_TECH_ERROR;
    if (paused || !state->ticks_remaining) return SC64_Z6_TRANSPORT_TECH_IDLE;
    if (!producer_valid) return SC64_Z6_TRANSPORT_TECH_ERROR;
    --state->ticks_remaining;
    if (state->ticks_remaining) return SC64_Z6_TRANSPORT_TECH_RESEARCHING;
    state->producer_slot_plus1 = 0;
    state->researched = 1;
    return SC64_Z6_TRANSPORT_TECH_RESEARCHED;
}

#endif
