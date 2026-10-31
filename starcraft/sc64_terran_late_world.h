#ifndef SC64_TERRAN_LATE_WORLD_H
#define SC64_TERRAN_LATE_WORLD_H

#if defined(SC64_TERRAN_LATE_RUNTIME) && defined(SC64_T4_RUNTIME) && \
    defined(SC64_TERRAN_LATE_ACTORS)

#include "sc64_terran_late_mission_state.h"
#include "sc64_terran_late_count_cache.h"
#include "sc64_unit_availability.h"
#include "sc64_t7_scenario_generated.h"
#include "sc64_t8_scenario_generated.h"
#include "sc64_t9_scenario_generated.h"
#include "sc64_t10_scenario_generated.h"
#include "sc64_t7_availability_generated.h"
#include "sc64_t8_availability_generated.h"
#include "sc64_t9_availability_generated.h"
#include "sc64_t10_availability_generated.h"
#include "sc64_t4_queries.h"
#include "sc64_t4_actions.h"

#ifdef SC64_DIAGNOSTIC_UPDATE
volatile u32 sc64_late_flight_cycles, sc64_late_world_cycles;
volatile u32 sc64_late_emitter_cycles, sc64_late_trigger_cycles;
#endif

enum {
    SC64_TERRAN_LATE_SOURCE_HUMAN_CONTROLLER = 6,
    SC64_TERRAN_LATE_SOURCE_RESCUE_CONTROLLER = 3,
    SC64_TERRAN_LATE_SOURCE_COMPUTER_CONTROLLER = 5,
    SC64_TERRAN_LATE_SOURCE_NEUTRAL_CONTROLLER = 7,
    SC64_TERRAN_LATE_SOURCE_NEUTRAL = 11
};

typedef struct {
    s32 left, top, right, bottom;
} Sc64TerranLateRect;

static int race_late_active(void);

static const Sc64T4Scenario *
sc64_terran_late_scenario_for_mission(int mission)
{
    switch (mission) {
    case SC64_TERRAN_LATE_GLOBAL_T7: return &sc64_t7_scenario;
    case SC64_TERRAN_LATE_GLOBAL_T8: return &sc64_t8_scenario;
    case SC64_TERRAN_LATE_GLOBAL_T9: return &sc64_t9_scenario;
    case SC64_TERRAN_LATE_GLOBAL_T10: return &sc64_t10_scenario;
    default: return 0;
    }
}

static const Sc64T4SourceSetup *
sc64_terran_late_setup_for_mission(int mission)
{
    switch (mission) {
    case SC64_TERRAN_LATE_GLOBAL_T7: return &sc64_t7_source_setup;
    case SC64_TERRAN_LATE_GLOBAL_T8: return &sc64_t8_source_setup;
    case SC64_TERRAN_LATE_GLOBAL_T9: return &sc64_t9_source_setup;
    case SC64_TERRAN_LATE_GLOBAL_T10: return &sc64_t10_source_setup;
    default: return 0;
    }
}

static int
sc64_terran_late_source_slot_for_controller(
    const Sc64T4SourceSetup *setup, unsigned controller)
{
    if (!setup) return -1;
    for (unsigned slot = 0; slot < 8; ++slot)
        if (setup->controllers[slot] == controller) return (int)slot;
    return -1;
}

/* The source race byte uses 0=Zerg, 1=Terran, 2=Protoss; translate it to
 * this runtime's compact R_* identity. */
static int
sc64_terran_late_runtime_race(unsigned source_slot)
{
    const Sc64T4SourceSetup *setup =
        sc64_terran_late_setup_for_mission(race_selected_mission);
    if (!race_late_active() || !setup || source_slot >= 8) return -1;
    switch (setup->races[source_slot]) {
    case 0: return R_ZERG;
    case 1: return R_TER;
    case 2: return R_PROTOSS;
    default: return -1;
    }
}

/* Current computer forces are kept in separate runtime owners. Source maps
 * in this four-mission set contain at most two distinct computer forces;
 * unsupported extra groups fail closed instead of becoming allies by alias. */
static int
sc64_terran_late_runtime_owner_for_setup(const Sc64T4SourceSetup *setup,
                                         unsigned source_slot)
{
    if (!setup || source_slot >= 8) return -1;
    unsigned controller = setup->controllers[source_slot];
    if (controller == SC64_TERRAN_LATE_SOURCE_HUMAN_CONTROLLER) return SIDE;
    if (controller == SC64_TERRAN_LATE_SOURCE_RESCUE_CONTROLLER)
        return RESCUABLE;
    if (controller == SC64_TERRAN_LATE_SOURCE_NEUTRAL_CONTROLLER)
        return NEUTRAL;
    if (controller != SC64_TERRAN_LATE_SOURCE_COMPUTER_CONTROLLER) return -1;

    unsigned human_force = 0xffu;
    for (unsigned i = 0; i < 8; ++i)
        if (setup->controllers[i] == SC64_TERRAN_LATE_SOURCE_HUMAN_CONTROLLER) {
            human_force = setup->force_by_slot[i];
            break;
        }
    if (human_force == 0xffu || setup->force_by_slot[source_slot] == human_force)
        return -1;

    unsigned first_force = 0xffu;
    for (unsigned i = 0; i < 8; ++i) {
        if (setup->controllers[i] == SC64_TERRAN_LATE_SOURCE_COMPUTER_CONTROLLER &&
            setup->force_by_slot[i] != human_force) {
            first_force = setup->force_by_slot[i];
            break;
        }
    }
    if (first_force == 0xffu) return -1;
    if (setup->force_by_slot[source_slot] == first_force) return ENEMY;

    for (unsigned i = 0; i < source_slot; ++i)
        if (setup->controllers[i] == SC64_TERRAN_LATE_SOURCE_COMPUTER_CONTROLLER &&
            setup->force_by_slot[i] == setup->force_by_slot[source_slot])
            return 4;
    /* The first occurrence of the second force is distinguished from further
     * unsupported forces by checking every computer slot in source order. */
    unsigned second_force = 0xffu;
    for (unsigned i = 0; i < 8; ++i)
        if (setup->controllers[i] == SC64_TERRAN_LATE_SOURCE_COMPUTER_CONTROLLER &&
            setup->force_by_slot[i] != human_force &&
            setup->force_by_slot[i] != first_force) {
            second_force = setup->force_by_slot[i];
            break;
        }
    return second_force != 0xffu &&
           setup->force_by_slot[source_slot] == second_force ? 4 : -1;
}

/* Convenience API for the current internal late mission. Source slot 11 is
 * the map-neutral owner and has no entry in the eight trigger player slots. */
static int
sc64_terran_late_runtime_owner(unsigned source_slot)
{
    const Sc64T4SourceSetup *setup;
    if (source_slot == SC64_TERRAN_LATE_SOURCE_NEUTRAL) return NEUTRAL;
    if (!race_late_active()) return -1;
    setup = sc64_terran_late_setup_for_mission(race_selected_mission);
    return sc64_terran_late_runtime_owner_for_setup(setup, source_slot);
}

static int sc64_terran_late_source_force(unsigned source_slot);
static int sc64_terran_late_source_enumerated(unsigned source_slot);

static int
sc64_terran_late_source_race(unsigned source_slot)
{
    return sc64_terran_late_runtime_race(source_slot);
}

static int
sc64_terran_late_current_source_owner(int live_owner, int raw_owner)
{
    const Sc64T4SourceSetup *setup;
    if (raw_owner == SC64_TERRAN_LATE_SOURCE_NEUTRAL)
        return live_owner == NEUTRAL ? raw_owner : -1;
    if (!race_late_active()) return -1;
    setup = sc64_terran_late_setup_for_mission(race_selected_mission);
    if (!setup) return -1;
    if (live_owner == SIDE) {
        int human = sc64_terran_late_source_slot_for_controller(
            setup, SC64_TERRAN_LATE_SOURCE_HUMAN_CONTROLLER);
        return human;
    }
    if (raw_owner < 0) {
        int found = -1;
        for (unsigned slot = 0; slot < 8; ++slot)
            if (sc64_terran_late_source_enumerated(slot) &&
                sc64_terran_late_runtime_owner_for_setup(setup, slot) == live_owner) {
                if (found >= 0) return -1; /* Ambiguous force provenance. */
                found = (int)slot;
            }
        return found;
    }
    if (raw_owner >= 8) return -1;
    return sc64_terran_late_runtime_owner_for_setup(setup,
                                                    (unsigned)raw_owner) == live_owner ?
           raw_owner : -1;
}

/* Products created by the runtime have no imported raw-owner slot. Infer a
 * source slot only when this runtime owner maps to exactly one active source
 * player; multiple members of one force stay deliberately ambiguous. */
static int
sc64_terran_late_product_source_owner(int live_owner)
{
    const Sc64T4SourceSetup *setup =
        sc64_terran_late_setup_for_mission(race_selected_mission);
    int found = -1;
    if (!race_late_active() || !setup || live_owner == NEUTRAL ||
        live_owner == RESCUABLE) return -1;
    for (unsigned slot = 0; slot < 8; ++slot)
        if (sc64_terran_late_source_enumerated(slot) &&
            sc64_terran_late_runtime_owner_for_setup(setup, slot) == live_owner) {
            if (found >= 0) return -1;
            found = (int)slot;
        }
    return found;
}

/* The local diplomacy policy treats distinct source forces as hostile and
 * the same force as allied. Native relation-byte initialization has not yet
 * been fully verified for these four source scenarios. */
static u8 sc64_terran_late_hostile_masks[5];
static u8 sc64_terran_late_hostility_mission = 0xffu;

static void
sc64_terran_late_build_hostility_cache(void)
{
    const Sc64T4SourceSetup *setup =
        sc64_terran_late_setup_for_mission(race_selected_mission);
    s8 force_by_owner[5] = {-1, -1, -1, -1, -1};
    for (unsigned owner = 0; owner < 5; ++owner)
        sc64_terran_late_hostile_masks[owner] = 0;
    if (!setup) {
        sc64_terran_late_hostility_mission = (u8)race_selected_mission;
        return;
    }
    for (unsigned slot = 0; slot < 8; ++slot) {
        if (!sc64_terran_late_source_enumerated(slot)) continue;
        int runtime_owner = sc64_terran_late_runtime_owner_for_setup(setup, slot);
        if (runtime_owner >= 0 && runtime_owner < 5)
            force_by_owner[runtime_owner] = (s8)(setup->force_by_slot[slot] + 1);
    }
    for (unsigned owner = 0; owner < 5; ++owner) {
        if (owner == NEUTRAL || owner == RESCUABLE || force_by_owner[owner] <= 0)
            continue;
        for (unsigned other = 0; other < 5; ++other)
            if (other != owner && other != NEUTRAL && other != RESCUABLE &&
                force_by_owner[other] > 0 &&
                force_by_owner[owner] != force_by_owner[other])
                sc64_terran_late_hostile_masks[owner] |= (u8)(1u << other);
    }
    sc64_terran_late_hostility_mission = (u8)race_selected_mission;
}

static int
sc64_terran_late_owners_hostile(int owner, int other)
{
    if (!race_late_active() || owner < 0 || owner >= 5 || other < 0 ||
        other >= 5 || owner == other || owner == NEUTRAL ||
        owner == RESCUABLE || other == NEUTRAL || other == RESCUABLE)
        return 0;
    if (sc64_terran_late_hostility_mission != (u8)race_selected_mission)
        sc64_terran_late_build_hostility_cache();
    return !!(sc64_terran_late_hostile_masks[owner] & (1u << other));
}

static int
sc64_terran_late_source_force(unsigned source_slot)
{
    const Sc64T4SourceSetup *setup =
        sc64_terran_late_setup_for_mission(race_selected_mission);
    return setup && source_slot < 8 ?
        (int)setup->force_by_slot[source_slot] + 1 : -1;
}

static int
sc64_terran_late_source_enumerated(unsigned source_slot)
{
    const Sc64T4SourceSetup *setup =
        sc64_terran_late_setup_for_mission(race_selected_mission);
    if (!setup || source_slot >= 8) return 0;
    return setup->controllers[source_slot] != 0;
}

/* Return one when an actor belongs to a trigger selector, zero for a valid
 * nonmatch, and -1 for a selector this adapter cannot evaluate. Selector 26
 * uses the source force table as the local relation model: this is an authored
 * approximation until N64 relation-byte initialization is fully verified. */
static int
sc64_terran_late_selector_matches(unsigned selector, unsigned trigger_owner,
                                  int live_owner, int raw_owner)
{
    int source = sc64_terran_late_current_source_owner(live_owner, raw_owner);
    if (trigger_owner >= 8) return -1;
    if (selector <= 7) return source == (int)selector;
    if (selector == 12) return 0;
    if (selector == 13) return source == (int)trigger_owner;
    if (selector == 17)
        return source >= 0 && source < 8 &&
               sc64_terran_late_source_enumerated((unsigned)source);
    if (selector >= 18 && selector <= 21)
        return source >= 0 && source < 8 &&
               sc64_terran_late_source_force((unsigned)source) ==
                   (int)(selector - 17);
    if (selector == 26) {
        if (source < 0 || source >= 8 || (unsigned)source == trigger_owner ||
            !sc64_terran_late_source_enumerated((unsigned)source)) return 0;
        return sc64_terran_late_source_force((unsigned)source) !=
               sc64_terran_late_source_force(trigger_owner);
    }
    return -1;
}

static int
sc64_terran_late_location_rect(const Sc64TerranLateMissionState *state,
                               unsigned id, Sc64TerranLateRect *out)
{
    const Sc64T4Scenario *scenario =
        sc64_terran_late_scenario_for_mission(race_selected_mission);
    u32 left, top, right, bottom;
    if (!state || !out || !scenario || !id || id > scenario->location_count)
        return 0;
    if (id == state->mutable_location_id) {
        left=state->mutable_location[0]; top=state->mutable_location[1];
        right=state->mutable_location[2]; bottom=state->mutable_location[3];
    } else {
        const Sc64T4Location *loc = &scenario->locations[id - 1];
        /* T7's one filtered MRGN uses 0x38, the source mask for all three
         * ground elevation bands. Other nonzero masks remain unsupported. */
        if (loc->elevation != 0 && loc->elevation != 0x38) return 0;
        left=loc->left; top=loc->top; right=loc->right; bottom=loc->bottom;
    }
    if (left >= right || top >= bottom || right > (u32)race_cols * 32u ||
        bottom > (u32)race_rows * 32u) return 0;
    out->left=(s32)left; out->top=(s32)top;
    out->right=(s32)right; out->bottom=(s32)bottom;
    return 1;
}

static int
sc64_terran_late_location_ground_only(unsigned id)
{
    const Sc64T4Scenario *scenario =
        sc64_terran_late_scenario_for_mission(race_selected_mission);
    return scenario && id && id <= scenario->location_count &&
           id != race_late.mutable_location_id &&
           scenario->locations[id - 1].elevation == 0x38;
}

static int
sc64_terran_late_actor_rect(int raw_type, int building, int x, int y,
                            Sc64TerranLateRect *out)
{
    Sc64T4QueryRect r;
    int l, u, right, d;
    int runtime_type = building ? sc64_race_building_from_raw((unsigned)raw_type) :
                                  sc64_race_unit_from_raw((unsigned)raw_type);
    if (!out) return 0;
    if (runtime_type >= 0 &&
        sc64_terran_late_actor_bounds(runtime_type, building, &l, &u,
                                       &right, &d)) {
        out->left=x*2-l; out->top=y*2-u;
        out->right=x*2+right+1; out->bottom=y*2+d+1;
        return 1;
    }
    /* Kerrigan and Raynor's hero Vulture use the earlier T5 actor facts;
     * they must also support late source region/invincibility queries. */
    if (!building && runtime_type >= 0 &&
        sc64_t5_actor_bounds_source(runtime_type, &l, &u, &right, &d)) {
        out->left=x*2-l; out->top=y*2-u;
        out->right=x*2+right+1; out->bottom=y*2+d+1;
        return 1;
    }
    if (!building && sc64_t4_query_actor_rect((unsigned)raw_type, x*2, y*2, &r)) {
        out->left=r.left; out->top=r.top;
        out->right=r.right; out->bottom=r.bottom;
        return 1;
    }
    /* The late missions query standard Terran Wraiths inside source regions;
     * this BWAPI UnitType.cpp dimension row is not in the earlier T4 table. */
    if (!building && raw_type == 8) {
        out->left=x*2-19; out->top=y*2-15;
        out->right=x*2+18+1; out->bottom=y*2+14+1;
        return 1;
    }
    /* BWAPI UnitType.cpp dimensions for the Dropship are not in the
     * late-only actor table because it is a shared earlier unit type. */
    if (!building && raw_type == 11) {
        out->left=x*2-24; out->top=y*2-16;
        out->right=x*2+24+1; out->bottom=y*2+20+1;
        return 1;
    }
    if (building) {
        /* BWAPI UnitType.cpp dimensions for common Terran structures used by
         * these source rosters. The late-only fact table intentionally holds
         * only the newly appended structure types. */
        switch (raw_type) {
        case 106: l=58; u=41; right=58; d=41; break;
        case 109: l=38; u=22; right=38; d=26; break;
        case 110: l=56; u=32; right=56; d=31; break;
        case 111: l=48; u=40; right=56; d=32; break;
        case 112: l=40; u=32; right=44; d=24; break;
        case 113: l=56; u=40; right=56; d=40; break;
        case 114: l=48; u=40; right=48; d=38; break;
        case 115: l=47; u=24; right=28; d=22; break;
        default: l=u=right=d=-1; break;
        }
        if (l >= 0) {
            out->left=x*2-l; out->top=y*2-u;
            out->right=x*2+right+1; out->bottom=y*2+d+1;
            return 1;
        }
        int type = sc64_race_building_from_raw((unsigned)raw_type);
        if (type >= 0 && sc64_race_t4_building_bounds_source(type, &l, &u,
                                                              &right, &d)) {
            out->left=x*2-l; out->top=y*2-u;
            out->right=x*2+right+1; out->bottom=y*2+d+1;
            return 1;
        }
    }
    return 0;
}

static int
sc64_terran_late_rect_overlap(const Sc64TerranLateRect *a,
                               const Sc64TerranLateRect *b)
{
    return a && b && a->left < a->right && a->top < a->bottom &&
           b->left < b->right && b->top < b->bottom &&
           a->left < b->right && b->left < a->right &&
           a->top < b->bottom && b->top < a->bottom;
}

static int
sc64_terran_late_raw_unit(const Unit *unit)
{
    return unit->raw_type >= 0 ? unit->raw_type :
           sc64_race_raw_unit(unit->type);
}

static int
sc64_terran_late_raw_building(const Building *building)
{
    return building->raw_type >= 0 ? building->raw_type :
           sc64_race_raw_building(building->type);
}

static int
sc64_terran_late_type_matches(unsigned requested, int raw_type,
                               int building)
{
    if (requested == 229u) return 1; /* Any Unit includes buildings. */
    if (requested == 231u) return building;
    if (requested == 230u) {
        if (building) return 0;
        const Sc64TerranLateActorFact *fact =
            sc64_terran_late_unit_fact(sc64_race_unit_from_raw((unsigned)raw_type));
        if (fact) return (sc64_terran_late_unit_actor_flags(
                            sc64_race_unit_from_raw((unsigned)raw_type)) &
                         SC64_LATE_ACTOR_MEN) != 0;
        return sc64_race_unit_from_raw((unsigned)raw_type) >= 0 &&
               raw_type != 217 && raw_type != 101;
    }
    return raw_type >= 0 && raw_type == (int)requested;
}

static int
sc64_terran_late_actor_matches(const Sc64T4Condition *condition,
                               unsigned trigger_owner, int owner, int raw_owner,
                               int raw_type, int building,
                               int x, int y, const Sc64TerranLateRect *area,
                               u32 *count)
{
    int selected = sc64_terran_late_selector_matches(condition->player,
                    trigger_owner, owner, raw_owner);
    if (selected < 0) return -1;
    if (!selected || !sc64_terran_late_type_matches(condition->unit, raw_type,
                                                     building)) return 1;
    if (area) {
        if (!building && sc64_terran_late_location_ground_only(
                condition->location)) {
            const Sc64TerranLateActorFact *fact =
                sc64_terran_late_unit_fact(
                    sc64_race_unit_from_raw((unsigned)raw_type));
            if (fact && fact->flying) return 1;
        }
        Sc64TerranLateRect actor;
        if (!sc64_terran_late_actor_rect(raw_type, building, x, y, &actor))
            return -1;
        if (!sc64_terran_late_rect_overlap(area, &actor)) return 1;
    }
    ++*count;
    return 1;
}

static int
race_late_trigger_count(void *context, const Sc64T4Condition *condition,
                        unsigned trigger_owner, u32 *count)
{
    Sc64TerranLateMissionState *state = context;
    Sc64TerranLateRect area, *area_ptr = 0;
    u32 result = 0;
    if (!state || !condition || !count || trigger_owner >= 8 ||
        (condition->location && condition->opcode != 3)) return 0;
    if (condition->opcode == 5 || condition->opcode == 15) {
        if (condition->location || (condition->unit != 229u &&
            condition->unit != 231u)) return 0;
        for (unsigned source = 0; source < 8; ++source) {
            int live_owner = sc64_terran_late_runtime_owner(source);
            if (live_owner < 0) continue;
            int selected = sc64_terran_late_selector_matches(condition->player,
                            trigger_owner, live_owner, (int)source);
            if (selected < 0) return 0;
            if (selected) {
                if (condition->unit == 229u) {
                    result += condition->opcode == 5 ? state->kills[source] :
                                                       state->deaths[source];
                } else {
                    result += condition->opcode == 5 ?
                        state->building_kills[source] :
                        state->building_deaths[source];
                }
            }
        }
        *count = result;
        return 1;
    }
    if (condition->opcode != 2 && condition->opcode != 3) return 0;
    if (condition->opcode == 3) {
        if (!sc64_terran_late_location_rect(state, condition->location, &area))
            return 0;
        area_ptr = &area;
    } else if (condition->location) return 0;
    for (int i = 0; i < MAX_UNITS; ++i) {
        int r;
        if (!u[i].alive) continue;
        r = sc64_terran_late_actor_matches(condition, trigger_owner,
                u[i].owner, u[i].raw_owner, sc64_terran_late_raw_unit(&u[i]),
                0, u[i].x, u[i].y, area_ptr, &result);
        if (r < 1) return 0;
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i) {
        int r;
        if (!b[i].alive) continue;
        r = sc64_terran_late_actor_matches(condition, trigger_owner,
                b[i].owner, b[i].raw_owner, sc64_terran_late_raw_building(&b[i]),
                1, b[i].x, b[i].y, area_ptr, &result);
        if (r < 1) return 0;
    }
    *count = result;
    return 1;
}

static const Sc64T4UnitProperties *
sc64_terran_late_properties(int mission)
{
    switch (mission) {
    case SC64_TERRAN_LATE_GLOBAL_T7: return sc64_t7_properties;
    case SC64_TERRAN_LATE_GLOBAL_T8: return sc64_t8_properties;
    case SC64_TERRAN_LATE_GLOBAL_T9: return sc64_t9_properties;
    case SC64_TERRAN_LATE_GLOBAL_T10: return sc64_t10_properties;
    default: return 0;
    }
}

static int
sc64_terran_late_script_index(u32 script)
{
    switch (script) {
    case 0x38726554u: return SC64_TERRAN_LATE_SCRIPT_T7_TER8; /* Ter8 */
    case 0x6b426e45u: return SC64_TERRAN_LATE_SCRIPT_ENBK;   /* EnBk */
    case 0x48397254u: return SC64_TERRAN_LATE_SCRIPT_TR9H;   /* Tr9H */
    case 0x4c397254u: return SC64_TERRAN_LATE_SCRIPT_TR9L;   /* Tr9L */
    case 0x7a313154u: return SC64_TERRAN_LATE_SCRIPT_T11Z;   /* T11z */
    case 0x61313154u: return SC64_TERRAN_LATE_SCRIPT_T11A;   /* T11a */
    case 0x62313154u: return SC64_TERRAN_LATE_SCRIPT_T11B;   /* T11b */
    case 0x52697553u: return SC64_TERRAN_LATE_SCRIPT_SUIR;   /* SuiR */
    case 0x4e323154u: return SC64_TERRAN_LATE_SCRIPT_T12N;   /* T12N */
    case 0x50323154u: return SC64_TERRAN_LATE_SCRIPT_T12P;   /* T12P */
    default: return -1;
    }
}

static int
sc64_terran_late_wallet_owner(int source_slot)
{
    int owner;
    if (source_slot < 0 || source_slot >= 8) return -1;
    owner = sc64_terran_late_runtime_owner((unsigned)source_slot);
    return owner >= SIDE && owner < 5 ? owner : -1;
}

static int
sc64_terran_late_apply_resources(Sc64TerranLateMissionState *state,
                                 const Sc64T4Action *action,
                                 unsigned trigger_owner)
{
    Sc64ResourceActionState candidates[8];
    int wallets[8], selected[8] = {0};
    if (!state || !action || action->opcode != 26 || trigger_owner >= 8 ||
        action->unit > SC64_RESOURCE_BOTH ||
        (action->modifier != SC64_RESOURCE_MOD_SET &&
         action->modifier != SC64_RESOURCE_MOD_ADD &&
         action->modifier != SC64_RESOURCE_MOD_SUBTRACT)) return -1;
    for (unsigned source = 0; source < 8; ++source) {
        int owner = sc64_terran_late_runtime_owner(source);
        int match = sc64_terran_late_selector_matches(action->player,
                         trigger_owner, owner, (int)source);
        if (match < 0) return -1;
        if (!match) continue;
        int wallet = sc64_terran_late_wallet_owner((int)source);
        if (wallet < 0) return -1;
        for (unsigned p = 0; p < source; ++p)
            if (selected[p] && wallets[p] == wallet) return -1;
        selected[source] = 1;
        wallets[source] = wallet;
        candidates[source] = state->resources[source];
        candidates[source].current_minerals =
            wallet == SIDE ? (u32)minerals : (u32)race_minerals[wallet];
        candidates[source].current_gas =
            wallet == SIDE ? (u32)gas : (u32)race_gas[wallet];
        if (!sc64_resource_action_apply_one_player(&candidates[source],
                (u16)action->unit, action->modifier, action->argument)) return -1;
        if (candidates[source].current_minerals > 65535u ||
            candidates[source].current_gas > 65535u) return -1;
    }
    for (unsigned source = 0; source < 8; ++source) if (selected[source]) {
        int wallet = wallets[source];
        state->resources[source] = candidates[source];
        if (wallet == SIDE) {
            minerals = (int)candidates[source].current_minerals;
            gas = (int)candidates[source].current_gas;
        } else {
            race_minerals[wallet] = (int)candidates[source].current_minerals;
            race_gas[wallet] = (int)candidates[source].current_gas;
        }
    }
    return 1;
}

static int
sc64_terran_late_create(const Sc64T4Action *action, unsigned trigger_owner)
{
    const Sc64T4UnitProperties *all_properties;
    const Sc64T4UnitProperties *property = 0;
    Sc64TerranLateRect area;
    unsigned count, property_index = 0, source_owner = 8;
    int live_owner, type, raw, slots[255], made = 0, x, y;
    if (!action || action->opcode != 11 || trigger_owner >= 8 ||
        action->unit >= SC64_PUNI_UNIT_COUNT ||
        (action->flags != 0x10 &&
         !(action->unit == 217 && action->flags == 0x18)) ||
        action->string_id || action->wav_id || action->time ||
        action->trailing[0] || action->trailing[1] || action->trailing[2] ||
        !sc64_t4_creation_count(action->modifier, &count) || count > MAX_UNITS)
        return -1;
    for (unsigned source = 0; source < 8; ++source) {
        int owner = sc64_terran_late_runtime_owner(source);
        int match = sc64_terran_late_selector_matches(action->player,
                         trigger_owner, owner, (int)source);
        if (match < 0) return -1;
        if (match) { source_owner = source; break; }
    }
    if (source_owner >= 8) return -1;
    live_owner = sc64_terran_late_runtime_owner(source_owner);
    raw = (int)action->unit;
    type = sc64_race_unit_from_raw((unsigned)raw);
    if (type < 0 || sc64_race_building_from_raw((unsigned)raw) >= 0)
        return -1;
    if (raw == 217 && (race_selected_mission != SC64_TERRAN_LATE_GLOBAL_T7 ||
        count != 1 || race_late.emitter_unit != SC64_TERRAN_LATE_NO_SLOT))
        return -1;
    if (action->argument) {
        if (!sc64_t4_creation_property_index(action->argument, &property_index) ||
            property_index >= 64) return -1;
        all_properties = sc64_terran_late_properties(race_selected_mission);
        const Sc64T4SourceSetup *setup =
            sc64_terran_late_setup_for_mission(race_selected_mission);
        if (!all_properties || !setup || !setup->property_used[property_index])
            return -1;
        property = &all_properties[property_index];
        if (property->valid_special != 0x18 || property->valid_fields != 0x02 ||
            property->owner != 0xff || property->hp_percent != 100 ||
            property->shield_percent || property->energy_percent ||
            property->resources || property->hangar_count || property->special ||
            property->unused) return -1;
    }
    if (!sc64_terran_late_location_rect(&race_late, action->location, &area))
        return -1;
    x = ((area.left + area.right) / 2) / 2;
    y = ((area.top + area.bottom) / 2) / 2;
    if (x < 8 || y < 8 || x >= WORLD_W - 8 || y >= WORLD_H - 8) return -1;
    unsigned free_count = 0;
    for (int i = 0; i < MAX_UNITS && free_count < count; ++i)
        if (!u[i].alive) slots[free_count++] = i;
    if (free_count < count) return 0;
    /* All actor slots and authored placement inputs have been validated before
     * the first creation. The deterministic compact formation is a GBA policy. */
    for (unsigned i = 0; i < count; ++i) {
        int px = limit(x + ((int)(i % 5) - 2) * 4, 8, WORLD_W - 8);
        int py = limit(y + ((int)(i / 5) - 1) * 4, 8, WORLD_H - 8);
        int slot = spawn_unit(live_owner, type, px, py);
        if (slot < 0 || slot != slots[i]) return 0;
        u[slot].raw_type = raw;
        u[slot].raw_owner = (int)source_owner;
        if (property) {
            u[slot].hp = u[slot].maxhp * property->hp_percent / 100;
            if (u[slot].hp < 1) u[slot].hp = 1;
        }
        ++made;
    }
    if (made) {
        if (raw == 217) {
            race_late.emitter_unit = (u16)slots[0];
            race_late.emitter_carrier = SC64_TERRAN_LATE_NO_SLOT;
        }
        target_lists_valid = 0;
        sc64_target_index_end();
        update_exploration();
    }
    return 1;
}

static int
sc64_terran_late_set_invincibility(const Sc64T4Action *action,
                                   unsigned trigger_owner)
{
    Sc64TerranLateRect area, *area_ptr = 0;
    if (!action || action->opcode != 43 || trigger_owner >= 8 ||
        (action->modifier != 4 && action->modifier != 5) ||
        action->argument || action->string_id || action->wav_id || action->time ||
        action->trailing[0] || action->trailing[1] || action->trailing[2]) return -1;
    if (action->location) {
        if (!sc64_terran_late_location_rect(&race_late, action->location, &area))
            return -1;
        area_ptr = &area;
    }
    /* First validate every matching actor's bounds so an unsupported actor
     * cannot leave a partially changed world. */
    for (int pass = 0; pass < 2; ++pass) {
        int n = pass ? MAX_BUILDINGS : MAX_UNITS;
        for (int i = 0; i < n; ++i) {
            int owner = pass ? b[i].owner : u[i].owner;
            int raw_owner = pass ? b[i].raw_owner : u[i].raw_owner;
            int raw = pass ? sc64_terran_late_raw_building(&b[i]) :
                             sc64_terran_late_raw_unit(&u[i]);
            int alive = pass ? b[i].alive : u[i].alive;
            if (!alive) continue;
            int match = sc64_terran_late_selector_matches(action->player,
                         trigger_owner, owner, raw_owner);
            if (match < 0) return -1;
            if (!match || !sc64_terran_late_type_matches(action->unit, raw, pass))
                continue;
            if (area_ptr) {
                Sc64TerranLateRect actor;
                int x = pass ? b[i].x : u[i].x;
                int y = pass ? b[i].y : u[i].y;
                if (!sc64_terran_late_actor_rect(raw, pass, x, y, &actor))
                    return -1;
            }
        }
    }
    for (int pass = 0; pass < 2; ++pass) {
        int n = pass ? MAX_BUILDINGS : MAX_UNITS;
        for (int i = 0; i < n; ++i) {
            int owner = pass ? b[i].owner : u[i].owner;
            int raw_owner = pass ? b[i].raw_owner : u[i].raw_owner;
            int raw = pass ? sc64_terran_late_raw_building(&b[i]) :
                             sc64_terran_late_raw_unit(&u[i]);
            int alive = pass ? b[i].alive : u[i].alive;
            if (!alive) continue;
            if (!sc64_terran_late_selector_matches(action->player,
                    trigger_owner, owner, raw_owner) ||
                !sc64_terran_late_type_matches(action->unit, raw, pass)) continue;
            if (area_ptr) {
                Sc64TerranLateRect actor;
                int x = pass ? b[i].x : u[i].x;
                int y = pass ? b[i].y : u[i].y;
                if (!sc64_terran_late_actor_rect(raw, pass, x, y, &actor) ||
                    !sc64_terran_late_rect_overlap(area_ptr, &actor)) continue;
            }
            if (pass) invincible_buildings[i] = action->modifier == 4;
            else invincible_units[i] = action->modifier == 4;
        }
    }
    return 1;
}

static int
sc64_terran_late_is_human_context(unsigned source_slot)
{
    const Sc64T4SourceSetup *setup =
        sc64_terran_late_setup_for_mission(race_selected_mission);
    return setup && source_slot < 8 &&
           setup->controllers[source_slot] ==
               SC64_TERRAN_LATE_SOURCE_HUMAN_CONTROLLER;
}

static int
sc64_terran_late_center_location(const Sc64TerranLateMissionState *state,
                                 unsigned id, int *x, int *y)
{
    Sc64TerranLateRect r;
    if (!x || !y || !sc64_terran_late_location_rect(state, id, &r)) return 0;
    *x = ((r.left + r.right) / 2) / 2;
    *y = ((r.top + r.bottom) / 2) / 2;
    return 1;
}

/* MoveLocation accepts point-sized source/destination MRGNs. Keep ordinary
 * query locations on the stricter rectangle accessor; this variant is used
 * only by opcode 38 and still enforces map bounds/elevation validity. */
static int
sc64_terran_late_move_location_rect(const Sc64TerranLateMissionState *state,
                                    unsigned id, Sc64TerranLateRect *out)
{
    const Sc64T4Scenario *scenario =
        sc64_terran_late_scenario_for_mission(race_selected_mission);
    u32 left, top, right, bottom;
    if (sc64_terran_late_location_rect(state, id, out)) return 1;
    if (!state || !out || !scenario || !id || id > scenario->location_count)
        return 0;
    if (id == state->mutable_location_id) {
        left = state->mutable_location[0]; top = state->mutable_location[1];
        right = state->mutable_location[2]; bottom = state->mutable_location[3];
    } else {
        const Sc64T4Location *loc = &scenario->locations[id - 1];
        if (loc->elevation != 0 && loc->elevation != 0x38) return 0;
        left = loc->left; top = loc->top; right = loc->right; bottom = loc->bottom;
    }
    if (left > right || top > bottom || right > (u32)race_cols * 32u ||
        bottom > (u32)race_rows * 32u) return 0;
    out->left = (s32)left; out->top = (s32)top;
    out->right = (s32)right; out->bottom = (s32)bottom;
    return 1;
}

/* Source MoveLocation chooses the first matching actor in its source area.
 * The source binary's linked-list ordering is not reproduced here; slot order
 * is the deterministic bounded policy used by this port. */
static int
sc64_terran_late_move_location(Sc64TerranLateMissionState *state,
                               const Sc64T4Action *action,
                               unsigned trigger_owner)
{
    Sc64TerranLateRect source, destination;
    Sc64T4Condition selector;
    u32 found_x = 0, found_y = 0;
    int found = 0;
    if (!state || !action || action->opcode != 38 ||
        action->string_id || action->wav_id || action->time ||
        action->modifier || action->flags || action->trailing[0] ||
        action->trailing[1] || action->trailing[2] ||
        action->unit >= SC64_PUNI_UNIT_COUNT ||
        !sc64_terran_late_move_location_rect(state, action->location, &source) ||
        !sc64_terran_late_move_location_rect(state, action->argument,
                                              &destination))
        return -1;
    memset(&selector, 0, sizeof(selector));
    selector.player = action->player;
    selector.unit = action->unit;
    selector.opcode = 3;
    selector.location = action->location;
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive) {
        u32 count = 0;
        int r = sc64_terran_late_actor_matches(&selector, trigger_owner,
            u[i].owner, u[i].raw_owner, sc64_terran_late_raw_unit(&u[i]),
            0, u[i].x, u[i].y, &source, &count);
        if (r != 1) return -1;
        if (count) { found_x = (u32)u[i].x * 2u;
                     found_y = (u32)u[i].y * 2u; found = 1; break; }
    }
    /* Native MoveLocation falls back to the source location's center when
     * its unit query finds no actor. The source helper computes the midpoint
     * with signed integer division (truncating toward zero). */
    if (!found) {
        found_x = (u32)((source.left + source.right) / 2);
        found_y = (u32)((source.top + source.bottom) / 2);
    }
    u32 width = (u32)(destination.right - destination.left);
    u32 height = (u32)(destination.bottom - destination.top);
    u32 max_x = (u32)race_cols * 32u, max_y = (u32)race_rows * 32u;
    /* A span equal to the full coordinate bound cannot fit inside inclusive
     * [0,bound-1] coordinates without losing a cell; fail before any signed
     * conversion or state write rather than underflowing the far-edge clamp. */
    if (!width || !height || width >= max_x || height >= max_y) return -1;
    s32 left = (s32)found_x - (s32)(width / 2u);
    s32 top = (s32)found_y - (s32)(height / 2u);
    if (left < 0) left = 0;
    if (top < 0) top = 0;
    /* Native map bounds are inclusive at the far edge: coordinates at or
     * beyond tile_count*32 become bound-1, with the opposite edge shifted by
     * the same overflow. Keep the normal interior translation unchanged. */
    if ((u32)left + width >= max_x) left = (s32)(max_x - 1u - width);
    if ((u32)top + height >= max_y) top = (s32)(max_y - 1u - height);
    state->mutable_location_id = (u16)action->argument;
    state->mutable_location[0] = (u32)left;
    state->mutable_location[1] = (u32)top;
    state->mutable_location[2] = (u32)left + width;
    state->mutable_location[3] = (u32)top + height;
    return 1;
}

static int
race_late_trigger_action(void *context, const Sc64T4Action *action,
                         unsigned source_owner)
{
    Sc64TerranLateMissionState *state = context;
    int x, y;
    if (!state || !action || source_owner >= 8) return -1;
    if (action->flags & 2) return 1;
    switch (action->opcode) {
    case 7: case 8: case 9: case 12:
        if (!sc64_terran_late_is_human_context(source_owner)) return 1;
        state->message = action->opcode;
        state->message_ticks = 90; /* Authored display duration; dialogue omitted. */
        return 1;
    case 10:
        if (!sc64_terran_late_is_human_context(source_owner)) return 1;
        if (!sc64_terran_late_center_location(state, action->location, &x, &y))
            return -1;
        cx = x; cy = y;
        return 1;
    case 11:
        return sc64_terran_late_create(action, source_owner);
    case 15: case 16: {
        int script = sc64_terran_late_script_index(action->argument);
        if (script < 0 ||
            (action->opcode == 15 && action->location) ||
            (action->opcode == 16 && !action->location) ||
            action->unit || action->modifier || action->flags ||
            action->trailing[0] || action->trailing[1] || action->trailing[2])
            return -1;
        state->ai_requests[source_owner] |= (u32)1u << (unsigned)script;
        return 1;
    }
    case 26:
        return sc64_terran_late_apply_resources(state, action, source_owner);
    case 38:
        return sc64_terran_late_move_location(state, action, source_owner);
    case 43:
        return sc64_terran_late_set_invincibility(action, source_owner);
    default:
        return -1;
    }
}

static int
sc64_terran_late_product_source_slot(int owner, int raw_owner)
{
    const Sc64T4SourceSetup *setup;
    if (!race_late_active()) return -1;
    setup = sc64_terran_late_setup_for_mission(race_selected_mission);
    if (!setup) return -1;
    if (owner == SIDE)
        return sc64_terran_late_source_slot_for_controller(
            setup, SC64_TERRAN_LATE_SOURCE_HUMAN_CONTROLLER);
    if (raw_owner < 0) return sc64_terran_late_product_source_owner(owner);
    if (raw_owner < 0 || raw_owner >= 8 ||
        sc64_terran_late_runtime_owner_for_setup(setup,
                                                 (unsigned)raw_owner) != owner)
        return -1;
    return raw_owner;
}

static int
sc64_terran_late_source_product_available(int source_slot, int raw_type)
{
    const unsigned char *bits = 0;
    unsigned bytes = 0, available = 0;
    if (source_slot < 0 || source_slot >= (int)SC64_PUNI_PLAYER_COUNT ||
        raw_type < 0 || raw_type >= (int)SC64_PUNI_UNIT_COUNT) return 0;
    switch (race_selected_mission) {
    case SC64_TERRAN_LATE_GLOBAL_T7:
        bits = sc64_t7_availability_bits; bytes = SC64_T7_AVAILABILITY_BIT_BYTES; break;
    case SC64_TERRAN_LATE_GLOBAL_T8:
        bits = sc64_t8_availability_bits; bytes = SC64_T8_AVAILABILITY_BIT_BYTES; break;
    case SC64_TERRAN_LATE_GLOBAL_T9:
        bits = sc64_t9_availability_bits; bytes = SC64_T9_AVAILABILITY_BIT_BYTES; break;
    case SC64_TERRAN_LATE_GLOBAL_T10:
        bits = sc64_t10_availability_bits; bytes = SC64_T10_AVAILABILITY_BIT_BYTES; break;
    default: return 0;
    }
    return sc64_availability_bitmap_get(bits, bytes, (unsigned)source_slot,
                                         (unsigned)raw_type, &available) &&
           available != 0;
}

static int
race_late_unit_product_available(int owner, int raw_owner, int type)
{
    if (!race_late_active()) return 1;
    return sc64_terran_late_source_product_available(
        sc64_terran_late_product_source_slot(owner, raw_owner),
        sc64_race_raw_unit(type));
}

static int
race_late_building_product_available(int owner, int raw_owner, int type)
{
    if (!race_late_active()) return 1;
    return sc64_terran_late_source_product_available(
        sc64_terran_late_product_source_slot(owner, raw_owner),
        sc64_race_raw_building(type));
}

static const Sc64TerranLateRuntimeOps race_late_runtime_ops = {
    race_late_trigger_count, race_late_trigger_action
};
static Sc64TerranLateCountCache race_late_count_cache;

static int
race_late_init(void)
{
    const Sc64T4Scenario *scenario =
        sc64_terran_late_scenario_for_mission(race_selected_mission);
    const Sc64T4SourceSetup *setup =
        sc64_terran_late_setup_for_mission(race_selected_mission);
    if (!scenario || !setup || race_mode || race_cols <= 0 || race_rows <= 0 ||
        !sc64_terran_late_mission_init(&race_late, scenario, setup)) return 0;
    if (race_selected_mission == SC64_TERRAN_LATE_GLOBAL_T7)
        for (int i = 0; i < MAX_UNITS; ++i)
            if (u[i].alive && sc64_terran_late_raw_unit(&u[i]) == 217) {
                race_late.emitter_unit = (u16)i;
                break;
            }
    /* Imported add-ons retain their source placement. Bind exact adjacent
     * Starport/Tower pairs once; live construction uses the same parent ID. */
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].type == B_CONTROL_TOWER)
            for (int j = 0; j < MAX_BUILDINGS; ++j)
                if (b[j].alive && b[j].type == B_STARPORT &&
                    b[j].owner == b[i].owner && b[i].x == b[j].x + 48 &&
                    b[i].y == b[j].y + 8) { b[i].level = j + 1; break; }
    race_campaign.phase = SC64_RACE_PHASE_ACTIVE;
    race_campaign.objective_flags = 0;
    race_campaign.objective_stage = 0;
    race_campaign.elapsed_frames = 0;
    race_campaign.message = race_late.message = 0;
    race_campaign.message_ticks = race_late.message_ticks = 0;
    won = lost = 0;
    return 1;
}

static int
sc64_terran_late_human_near(int x, int y)
{
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == SIDE) {
        unsigned flags = sc64_race_t4_actor_flags(u[i].type);
        if (flags && !(flags & SC64_T4_ACTOR_DRAWABLE)) continue;
        int dx = (u[i].x - x) * 2, dy = (u[i].y - y) * 2;
        if (dx < 0) dx = -dx;
        if (dy < 0) dy = -dy;
        if (dx <= 64 && dy <= 64) return 1;
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive && b[i].owner == SIDE) {
        int dx = (b[i].x - x) * 2, dy = (b[i].y - y) * 2;
        if (dx < 0) dx = -dx;
        if (dy < 0) dy = -dy;
        if (dx <= 64 && dy <= 64) return 1;
    }
    return 0;
}

static void
sc64_terran_late_transfer_unit(Unit *q)
{
#ifdef SC64_BATTLECRUISER_ENERGY_EXPERIMENT
    if (!race_bc_before_unit_transfer(q,SIDE,-1)) return;
#endif
    q->owner = SIDE; q->selected = 0; q->order = ORDER_IDLE;
    q->target = -1; q->tx = q->x; q->ty = q->y;
    q->path_timer = q->path_retry = 0;
    q->weapon = weapons_level;
}

static void
sc64_terran_late_transfer_group(int source_owner)
{
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == RESCUABLE &&
            u[i].raw_owner == source_owner)
            sc64_terran_late_transfer_unit(&u[i]);
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == RESCUABLE &&
            b[i].raw_owner == source_owner && SC64_BC_ALLOW_BUILDING_TRANSFER(i,SIDE)) b[i].owner = SIDE;
}

static void
sc64_terran_late_world_tick(void)
{
    int changed = 0;
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == RESCUABLE) {
        int source = u[i].raw_owner;
        if (source < 0 || source >= 8 ||
            sc64_terran_late_runtime_owner((unsigned)source) != RESCUABLE ||
            !sc64_terran_late_human_near(u[i].x, u[i].y)) continue;
        sc64_terran_late_transfer_unit(&u[i]); changed = 1;
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == RESCUABLE) {
            int source = b[i].raw_owner;
            if (source < 0 || source >= 8 ||
                sc64_terran_late_runtime_owner((unsigned)source) != RESCUABLE ||
                !sc64_terran_late_human_near(b[i].x, b[i].y)) continue;
            int depot = b[i].type == B_COMMAND || b[i].type == B_NEXUS ||
                        sc64_race_is_zerg_townhall(b[i].type);
            if (depot && !b[i].under_construction)
                sc64_terran_late_transfer_group(source);
            else if (SC64_BC_ALLOW_BUILDING_TRANSFER(i,SIDE)) b[i].owner = SIDE;
            changed = 1;
        }
    if (!changed) return;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].order == ORDER_ATTACK_TARGET &&
            u[i].target >= 0 && u[i].target < MAX_UNITS + MAX_BUILDINGS) {
            int target = u[i].target;
            int target_owner = target < MAX_UNITS ? u[target].owner :
                b[target - MAX_UNITS].owner;
            if (!owners_hostile(u[i].owner, target_owner)) {
                u[i].order = ORDER_IDLE; u[i].target = -1;
            }
        }
    supply_used = race_owner_supply(SIDE);
    supply_cap = race_owner_supply_cap(SIDE);
    target_lists_valid = 0;
    sc64_unit_bucket_active = 0;
    sc64_target_index_end();
}

/* The source Psi Emitter remains an actor for Bring queries. Carrying is a
 * small authored link in mission state, not generic transport cargo. */
static void
sc64_terran_late_emitter_tick(void)
{
    if (race_selected_mission != SC64_TERRAN_LATE_GLOBAL_T7) return;
    if (race_late.emitter_unit >= MAX_UNITS ||
        !u[race_late.emitter_unit].alive ||
        sc64_terran_late_raw_unit(&u[race_late.emitter_unit]) != 217) {
        race_late.emitter_unit = SC64_TERRAN_LATE_NO_SLOT;
        race_late.emitter_carrier = SC64_TERRAN_LATE_NO_SLOT;
        return;
    }
    Unit *emitter = &u[race_late.emitter_unit];
    if (race_late.emitter_carrier != SC64_TERRAN_LATE_NO_SLOT) {
        if (race_late.emitter_carrier >= MAX_UNITS ||
            !u[race_late.emitter_carrier].alive ||
            u[race_late.emitter_carrier].owner != SIDE ||
            u[race_late.emitter_carrier].type != U_SCV) {
            race_late.emitter_carrier = SC64_TERRAN_LATE_NO_SLOT;
        } else {
            const Unit *carrier = &u[race_late.emitter_carrier];
            emitter->x = carrier->x; emitter->y = carrier->y;
            emitter->owner = SIDE;
            emitter->raw_owner = sc64_terran_late_source_slot_for_controller(
                sc64_terran_late_setup_for_mission(race_selected_mission),
                SC64_TERRAN_LATE_SOURCE_HUMAN_CONTROLLER);
            sc64_unit_bucket_move(emitter);
            sc64_target_index_move(emitter);
            return;
        }
    }
    /* OpenBW associates carried powerups with workers (worker.powerup).
     * The inclusive 8-pixel pickup box is this adaptation's boundary. */
    for (int i = 0; i < MAX_UNITS; ++i)
        if (i != race_late.emitter_unit && u[i].alive && u[i].owner == SIDE &&
            u[i].type == U_SCV && !SC64_TRANSPORT_LOADED(&u[i])) {
            int dx = u[i].x - emitter->x, dy = u[i].y - emitter->y;
            if (dx < 0) dx = -dx;
            if (dy < 0) dy = -dy;
            if (dx <= 8 && dy <= 8) {
                race_late.emitter_carrier = (u16)i;
                emitter->owner = SIDE;
                emitter->raw_owner = sc64_terran_late_source_slot_for_controller(
                    sc64_terran_late_setup_for_mission(race_selected_mission),
                    SC64_TERRAN_LATE_SOURCE_HUMAN_CONTROLLER);
                break;
            }
        }
}

static void
race_late_record_actor_death(int killer_live_owner, int killer_raw_owner,
                             int victim_live_owner, int victim_raw_owner,
                             int victim_raw_type, int is_building)
{
    (void)victim_raw_type;
    if (!race_late_active()) return;
    int killer = sc64_terran_late_current_source_owner(killer_live_owner,
                                                        killer_raw_owner);
    int victim = sc64_terran_late_current_source_owner(victim_live_owner,
                                                        victim_raw_owner);
    if (killer >= 0 && killer < 8 && race_late.kills[killer] != 0xffffffffu) {
        ++race_late.kills[killer];
        if (is_building && race_late.building_kills[killer] != 0xffffffffu)
            ++race_late.building_kills[killer];
    }
    if (victim >= 0 && victim < 8 && race_late.deaths[victim] != 0xffffffffu) {
        ++race_late.deaths[victim];
        if (is_building && race_late.building_deaths[victim] != 0xffffffffu)
            ++race_late.building_deaths[victim];
    }
}

static int
race_late_tick(void)
{
    const Sc64T4Scenario *scenario;
    const Sc64T4SourceSetup *setup;
    int trigger_ok;
#ifdef SC64_DIAGNOSTIC_UPDATE
    u32 late_stage_start;
    sc64_late_flight_cycles = sc64_late_world_cycles = 0;
    sc64_late_emitter_cycles = sc64_late_trigger_cycles = 0;
#endif
    if (!race_late_active()) return 1;
    scenario = sc64_terran_late_scenario_for_mission(race_selected_mission);
    setup = sc64_terran_late_setup_for_mission(race_selected_mission);
    if (!scenario || !setup || race_campaign.phase == SC64_RACE_PHASE_COMPLETE ||
        race_campaign.phase == SC64_RACE_PHASE_FAILED) return 1;
    if (screen != SCREEN_PLAY) return 1;
    if (race_late.message_ticks && !--race_late.message_ticks)
        race_late.message = 0;
    if (race_late.ping_ticks && !--race_late.ping_ticks)
        race_late.ping_x = race_late.ping_y = 0;
#ifdef SC64_DIAGNOSTIC_UPDATE
    late_stage_start = gba_clock();
#endif
    race_late_flight_tick();
#ifdef SC64_DIAGNOSTIC_UPDATE
    sc64_late_flight_cycles = gba_clock() - late_stage_start;
    late_stage_start = gba_clock();
#endif
    sc64_terran_late_world_tick();
#ifdef SC64_DIAGNOSTIC_UPDATE
    sc64_late_world_cycles = gba_clock() - late_stage_start;
    late_stage_start = gba_clock();
#endif
    sc64_terran_late_emitter_tick();
#ifdef SC64_DIAGNOSTIC_UPDATE
    sc64_late_emitter_cycles = gba_clock() - late_stage_start;
    late_stage_start = gba_clock();
#endif
    /* Actor counts stay stable during condition evaluation. Delegated actions
     * invalidate the memo before they can change units, buildings or regions. */
    sc64_terran_late_count_cache_init(&race_late_count_cache,
                                     &race_late_runtime_ops, &race_late);
    trigger_ok = sc64_terran_late_runtime_step_prevalidated(
            &race_late.triggers, scenario, setup,
            &sc64_terran_late_count_cache_ops,
            &race_late_count_cache, 0);
#ifdef SC64_DIAGNOSTIC_UPDATE
    sc64_late_trigger_cycles = gba_clock() - late_stage_start;
#endif
    if (!trigger_ok) return 0;
    race_campaign.message = race_late.message;
    race_campaign.message_ticks = race_late.message_ticks;
    race_campaign.objective_stage = race_late.emitter_carrier !=
        SC64_TERRAN_LATE_NO_SLOT;
    int human = sc64_terran_late_source_slot_for_controller(
        setup, SC64_TERRAN_LATE_SOURCE_HUMAN_CONTROLLER);
    if (human >= 0 && race_late.triggers.player[human].outcome ==
                          SC64_TERRAN_LATE_LOST) {
        lost = 1; won = 0; race_campaign.phase = SC64_RACE_PHASE_FAILED;
    } else if (human >= 0 && race_late.triggers.player[human].outcome ==
                                 SC64_TERRAN_LATE_WON) {
        won = 1; lost = 0; race_campaign.phase = SC64_RACE_PHASE_COMPLETE;
        race_campaign.objective_flags |= SC64_RACE_OBJECTIVE_PRIMARY;
        race_completed |= 1 << race_selected_mission;
    }
    return 1;
}

#endif /* late runtime */
#endif
