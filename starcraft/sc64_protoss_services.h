/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_PROTOSS_SERVICES_H
#define SC64_PROTOSS_SERVICES_H

/* Shared Protoss unit services: Carrier Interceptor training, flight and
 * projectiles; Arbiter cloak; intrinsic cloak (Observer, Dark Templar,
 * Zeratul); detection (Observer, Photon Cannon, Overlord, Spore Colony); and
 * Shield Battery energy and recharge. They are engine behavior of those
 * actors (docs/SHARED-ENGINE.md), not mission state, so they run in every
 * mode that supplies a binding: the owner identity of each service slot,
 * storage for the saved service clock, and the data that authenticates
 * placed actors.
 *
 * - Custom games and Showcase scenes (race_mode) use the generic binding.
 *   Slot 7 is the human side and slots 1..4 are the other live owners; the
 *   clock is saved in race save state word 26.
 * - P7, and later missions that carry their own source map, use mission
 *   bindings. Slots are the actors' source (CHK) slots; each mission saves
 *   the clock in its own payload and authenticates its map's actors.
 * - Campaign missions without a binding run none of these services. P1-P5
 *   field none of the served actors except Zerg detectors with nothing to
 *   detect, and P6 keeps its own cloak and detection adapter.
 *
 * Mission objectives, triggers, map authentication and source-owner tables
 * stay with each mission's adapter. */

#include "sc64_p7.h"

enum {
    SC64_PROTOSS_BIND_NONE = 0,
    SC64_PROTOSS_BIND_GENERIC,
    SC64_PROTOSS_BIND_P7,
    SC64_PROTOSS_BIND_LATER
};

enum {
    SC64_PROTOSS_HUMAN_SLOT = 7,
    /* Generic clock word: tag in the high half, stagger << 4 | phase. Zero
     * (older saves) is the not-yet-started clock. */
    SC64_PROTOSS_CLOCK_STATE_INDEX = 30,
    SC64_PROTOSS_CLOCK_STATE_TAG = 0x50530000
};

#ifdef SC64_P7_RUNTIME
static int race_p7_active(void);
static int race_p7_source_owner(unsigned slot);
#else
static inline int race_p7_active(void) { return 0; }
static inline int race_p7_source_owner(unsigned slot) { (void)slot; return -1; }
#endif
#ifdef SC64_PROTOSS_LATER_BINDINGS
static int race_protoss_later_services_active(void);
static int race_protoss_later_source_owner(unsigned slot);
static u8 *race_protoss_later_service_clock(void);
static int race_protoss_later_service_mission(int mission);
static int race_protoss_later_mission_source_owner(int mission, unsigned slot);
static int race_protoss_later_battery_initialize(const Sc64RaceMap *map);
static int race_protoss_later_battery_world_valid(void);
static int race_protoss_later_battery_candidate_valid(int mission, const Building *q);
#endif

/* Generic clock; the mission clocks live in their payloads. */
static u8 race_protoss_generic_clock[3];

/* While a save candidate is validated, its mode and mission stand in for
 * the live ones (see race_load_game). */
static signed char race_protoss_binding_override = -1;
static int race_protoss_override_mission;

static inline int race_protoss_service_binding(void)
{
    if (race_protoss_binding_override >= 0) return race_protoss_binding_override;
    if (race_mode) return SC64_PROTOSS_BIND_GENERIC;
    if (race_p7_active()) return SC64_PROTOSS_BIND_P7;
#ifdef SC64_PROTOSS_LATER_BINDINGS
    if (race_protoss_later_services_active()) return SC64_PROTOSS_BIND_LATER;
#endif
    return SC64_PROTOSS_BIND_NONE;
}

#define race_protoss_services_active() \
    (race_protoss_service_binding() != SC64_PROTOSS_BIND_NONE)

/* Temporary qualification fence for the experimental cross-profile Carrier
 * adapter. Existing P7/P8 flight keeps its mission bindings; the new adapter
 * reaches campaign worlds only after their save and runtime qualification. */
#if defined(SC64_SHARED_CARRIER_EXPERIMENT) && !defined(SC64_P7_PROJECTILES_ENABLED)
#define race_carrier_services_active() (race_mode && race_protoss_services_active())
#else
#define race_carrier_services_active() race_protoss_services_active()
#endif

/* Generic slots follow the live owner: SIDE is the human slot. */
static inline int race_protoss_generic_slot(int owner)
{
    return owner == SIDE ? SC64_PROTOSS_HUMAN_SLOT :
        owner > 0 && owner < 5 ? owner : -1;
}

static inline int race_protoss_generic_source_owner(unsigned slot)
{
    return slot == SC64_PROTOSS_HUMAN_SLOT ? SIDE :
        slot > 0 && slot < 5 ? (int)slot : -1;
}

static inline int race_protoss_binding_source_owner(int binding, unsigned slot)
{
    if (slot >= 8u) return -1;
    if (binding == SC64_PROTOSS_BIND_GENERIC)
        return race_protoss_generic_source_owner(slot);
    if (binding == SC64_PROTOSS_BIND_P7) return race_p7_source_owner(slot);
#ifdef SC64_PROTOSS_LATER_BINDINGS
    if (binding == SC64_PROTOSS_BIND_LATER)
        return race_protoss_binding_override >= 0 ?
            race_protoss_later_mission_source_owner(race_protoss_override_mission, slot) :
            race_protoss_later_source_owner(slot);
#endif
    return -1;
}

/* Live owner of a service slot in the current mode. */
#define race_protoss_source_owner(slot) \
    race_protoss_binding_source_owner(race_protoss_service_binding(), \
                                      (unsigned)(slot))

/* A service actor's slot: its source slot under a mission binding, or the
 * slot of its live owner under the generic one. */
static inline int race_protoss_binding_slot(int binding, int owner, int raw_owner)
{
    return binding == SC64_PROTOSS_BIND_GENERIC ?
        race_protoss_generic_slot(owner) : raw_owner;
}
#define RACE_PROTOSS_SLOT(q) \
    race_protoss_binding_slot(race_protoss_service_binding(), \
                              (q)->owner, (q)->raw_owner)

/* A served actor's source type. Mission actors carry it; custom-game actors
 * fall back to their engine type's source id, as elsewhere in the engine. */
static inline int race_protoss_unit_raw(const Unit *q)
{ return q->raw_type >= 0 ? q->raw_type : sc64_race_raw_unit(q->type); }
static inline int race_protoss_building_raw(const Building *q)
{ return q->raw_type >= 0 ? q->raw_type : sc64_race_raw_building(q->type); }
#define RACE_PROTOSS_RAW(q) _Generic((q), \
    Unit *: race_protoss_unit_raw, const Unit *: race_protoss_unit_raw, \
    Building *: race_protoss_building_raw, \
    const Building *: race_protoss_building_raw)(q)

/* A query names its asker by source slot. Under the generic binding the
 * asker's slot follows its live owner, whatever slot the caller passed. */
static inline int race_protoss_query_slot(int source_owner, int live_owner)
{
    return race_protoss_service_binding() == SC64_PROTOSS_BIND_GENERIC ?
        race_protoss_generic_slot(live_owner) : source_owner;
}

/* Save candidates are checked before their mode is live, so these name the
 * candidate's mode and mission explicitly. */
static inline int race_protoss_candidate_binding(int candidate_race_mode, int mission)
{
    if (candidate_race_mode) return SC64_PROTOSS_BIND_GENERIC;
    if (mission == SC64_P7_GLOBAL_ID) return SC64_PROTOSS_BIND_P7;
#ifdef SC64_PROTOSS_LATER_BINDINGS
    if (race_protoss_later_service_mission(mission)) return SC64_PROTOSS_BIND_LATER;
#endif
    return SC64_PROTOSS_BIND_NONE;
}

static inline void race_protoss_bind_candidate(int candidate_race_mode, int mission)
{
    race_protoss_binding_override =
        (signed char)race_protoss_candidate_binding(candidate_race_mode, mission);
    race_protoss_override_mission = mission;
}

static inline void race_protoss_unbind_candidate(void)
{ race_protoss_binding_override = -1; }

#ifdef SC64_P7_RUNTIME
static u8 *race_protoss_service_clock(void);
#else
static u8 *race_protoss_service_clock(void) { return race_protoss_generic_clock; }
#endif
#define RACE_PROTOSS_SERVICE_CLOCK (race_protoss_service_clock())

static inline int race_protoss_clock_word_valid(int word)
{
    unsigned phase, stagger;
    if (!word) return 1;
    if (((unsigned)word & 0xffff0000u) != SC64_PROTOSS_CLOCK_STATE_TAG) return 0;
    phase = (unsigned)word & 15u;
    stagger = ((unsigned)word >> 4) & 0xfffu;
    return phase <= 4u && stagger <= 300u;
}

static inline int race_protoss_clock_word(const u8 clock[3])
{
    unsigned stagger = (unsigned)clock[1] | ((unsigned)clock[2] << 8);
    if (!clock[0] && !stagger) return 0;
    return (int)(SC64_PROTOSS_CLOCK_STATE_TAG | (stagger << 4) | clock[0]);
}

static inline void race_protoss_clock_from_word(u8 clock[3], int word)
{
    unsigned stagger = ((unsigned)word >> 4) & 0xfffu;
    clock[0] = word ? (u8)((unsigned)word & 15u) : 0;
    clock[1] = word ? (u8)stagger : 0;
    clock[2] = word ? (u8)(stagger >> 8) : 0;
}

#endif
