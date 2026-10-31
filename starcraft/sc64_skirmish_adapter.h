/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_SKIRMISH_ADAPTER_H
#define SC64_SKIRMISH_ADAPTER_H

/* Custom-game admission uses ordinary stock raw IDs rather than mission
 * actor permissions or the small legacy prerequisite bitmask. No new saved
 * state is allocated. Campaign admission continues through its own adapter. */
static const Sc64SkirmishRecord *race_skirmish_build_record(int owner, int raw)
{
    return sc64_skirmish_menu_record((unsigned)race_owner_identity(owner), (unsigned)raw);
}

static int race_skirmish_has_raw(int owner, unsigned raw)
{
    if (raw == SC64_SK_NONE) return 1;
    for (int i = 0; i < MAX_BUILDINGS; ++i) {
        int have;
        if (!b[i].alive || b[i].owner != owner) continue;
#ifdef SC64_BATTLECRUISER_FLIGHT_EXPERIMENT
        if (race_bc_facility_airborne(i)) continue;
#endif
        have = sc64_race_raw_building(b[i].type);
        if (b[i].under_construction) {
            /* While upgrading, the existing source tier still exists. */
            if (have == 132) have = 131;
            else if (have == 133) have = 132;
            else if (have == 137) have = 141;
            else continue;
        }
        if (have == (int)raw ||
            (raw == 131 && (have == 132 || have == 133)) ||
            (raw == 132 && have == 133) || (raw == 141 && have == 137)) return 1;
    }
    return 0;
}

static int race_skirmish_requirements(int owner, const Sc64SkirmishRecord *r)
{
    return r && r->race == race_owner_identity(owner) &&
        r->tech_key == SC64_SK_NONE &&
        race_skirmish_has_raw(owner, r->prereq1_raw) &&
        race_skirmish_has_raw(owner, r->prereq2_raw);
}

static int race_skirmish_power_required(int type)
{
    const Sc64RaceBuildingRule *r = sc64_race_building_rule(type);
    int raw = sc64_race_raw_building(type);
    return r && r->race == R_PROTOSS && raw != 154 && raw != 156 && raw != 157;
}

/* The attachment belongs to a particular producer. An add-on at another
 * Factory/Starport does not unlock this producer's products. Geometry also
 * avoids storing a pool index in the seven-bit Building.level save field. */
static int race_skirmish_attached(int producer, unsigned raw, int completed)
{
    for (int i = 0; i < MAX_BUILDINGS; ++i) {
        const Sc64SkirmishRecord *r;
        if (!b[i].alive || b[i].owner != b[producer].owner ||
            (completed && !building_complete(i))) continue;
        r = race_skirmish_build_record(b[i].owner, sc64_race_raw_building(b[i].type));
        if (!r || r->kind != SC64_SK_ADDON ||
            r->parent_raw != sc64_race_raw_building(b[producer].type)) continue;
        if (b[i].x == b[producer].x + 48 && b[i].y == b[producer].y + 8 &&
            (raw == SC64_SK_NONE || r->raw_id == raw)) return 1;
    }
    return 0;
}

/* Scenario rules keep their impossible producer sentinel. A custom-game
 * product may override it only when its complete actor service is present;
 * admission and the paid queue must resolve the same ordinary producer. */
static int race_skirmish_product_producer(int type)
{
    const Sc64RaceUnitRule *r = sc64_race_unit_rule(type);
    if (!r) return 255;
#ifdef SC64_SHARED_TRANSPORT
    /* The producer is a source fact; per-mission product availability is
     * checked independently by race_queue_unit. */
    if (sc64_race_raw_unit(type) == 69 && sc64_race_unit_from_raw(69) == type)
        return B_ROBOTICS_FACILITY;
#endif
    if (race_mode) {
        int raw = sc64_race_raw_unit(type);
#ifdef SC64_BATTLECRUISER_PRODUCTION_EXPERIMENT
        if (raw == 12) return B_STARPORT;
#endif
#if defined(SC64_P7_INTERCEPTOR_FLIGHT_ENABLED) && \
    defined(SC64_P7_CARRIER_TRAINING_ENABLED) && \
    defined(SC64_P7_PROJECTILES_ENABLED) && defined(SC64_P7_ACTORS)
        if (raw == 72 && type == U_P7_CARRIER && race_protoss_services_active())
            return B_STARGATE;
#endif
        (void)raw;
    }
    return r->producer;
}

static int race_skirmish_product_admitted(int owner, int producer,
                                          const Sc64SkirmishRecord *r)
{
    const Sc64RaceUnitRule *ur;
    int type, producer_raw;
    if (!r || r->kind != SC64_SK_TRAIN || producer < 0 || producer >= MAX_BUILDINGS ||
        (!building_complete(producer) && !(b[producer].alive &&
            (b[producer].type == B_LAIR || b[producer].type == B_HIVE))) || b[producer].owner != owner ||
        !race_skirmish_requirements(owner, r)) return 0;
    type = sc64_race_unit_from_raw(r->raw_id);
    ur = sc64_race_unit_rule(type);
    /* Campaign-only actors with no ordinary producer are not silently made
     * into working skirmish units by exposing their names in a menu. */
    /* Reaver requires a Scarab ammunition lifecycle; the generic weapon
     * path alone is not an implementation of its ordinary product. */
#ifndef SC64_BATTLECRUISER_PRODUCTION_EXPERIMENT
    /* Production remains unavailable until its service is complete. */
    if (r->raw_id == 12) return 0;
#endif
    if (r->raw_id == 83 || !ur || race_skirmish_product_producer(type) == 255) return 0;
    producer_raw = sc64_race_raw_building(b[producer].type);
    if (r->producer_raw == 35) {
        if (!sc64_race_is_zerg_townhall(b[producer].type)) return 0;
    } else if (producer_raw != r->producer_raw) return 0;
    for (unsigned k = 0; k < 2; ++k) {
        unsigned raw = k ? r->prereq2_raw : r->prereq1_raw;
        const Sc64SkirmishRecord *addon = race_skirmish_build_record(owner, (int)raw);
        if (addon && addon->kind == SC64_SK_ADDON && addon->parent_raw == producer_raw &&
            !race_skirmish_attached(producer, raw, 1)) return 0;
    }
    return 1;
}

static int race_skirmish_product_available(const Sc64SkirmishRecord *r, void *context)
{
    return race_mode && context &&
        race_skirmish_product_admitted(SIDE, *(const int *)context, r);
}

static int race_skirmish_worker(int type)
{
    const Sc64SkirmishRecord *r = race_skirmish_build_record(SIDE, sc64_race_raw_building(type));
    int best = -1, best_d = 0x7fffffff;
    if (!r || r->kind != SC64_SK_WORKER_BUILD) return -1;
    for (int i = 0; i < MAX_UNITS; ++i) {
        int d;
        if (!build_worker_available(i, SIDE, -1) ||
            sc64_race_raw_unit(u[i].type) != r->producer_raw) continue;
        if (u[i].selected) return i;
        d = dist2(cx, cy, u[i].x, u[i].y);
        if (d < best_d) { best = i; best_d = d; }
    }
    return best;
}

/* Owner-generic admission for a producer-attached addon or an in-place
 * building morph. The human command row and computer opponents share it. */
static int race_skirmish_record_ready(int owner, int producer,
                                      const Sc64SkirmishRecord *r)
{
    /* The catalog retains the Silo row, but production is unavailable until
     * nuclear ammunition and launch are implemented. */
    if (r && r->raw_id == 108) return 0;
    if (!race_mode || !race_skirmish_requirements(owner, r) ||
        sc64_race_building_from_raw(r->raw_id) < 0) return 0;
    if (r->kind != SC64_SK_ADDON && r->kind != SC64_SK_BUILDING_MORPH) return 0;
#ifdef SC64_SHARED_TRANSPORT
    if (race_sk_transport_job_uses(producer)) return 0;
#endif
    /* Infestation is an ability targeting a damaged enemy Command Center,
     * not an ordinary in-place upgrade of an owned structure. */
    if (r->raw_id == 130 || producer < 0 || producer >= MAX_BUILDINGS ||
        !building_complete(producer) || b[producer].owner != owner ||
        sc64_race_raw_building(b[producer].type) != r->parent_raw ||
        b[producer].queue_count) return 0;
#ifdef SC64_TERRAN_LATE_RUNTIME
    if (race_late_airborne(producer)) return 0;
#endif
    if (r->kind == SC64_SK_ADDON &&
        race_skirmish_attached(producer, SC64_SK_NONE, 0)) return 0;
    return 1;
}

/* Pay from the owner's own bank and start the addon foundation or morph
 * clock. No resources are granted and nothing completes immediately. */
static int race_skirmish_start_record(int owner, int producer,
                                      const Sc64SkirmishRecord *r)
{
    const Sc64RaceBuildingRule *rule;
    Sc64RaceBank bank;
    int type, slot, x, y;
    if (!race_skirmish_record_ready(owner, producer, r)) return 0;
    type = sc64_race_building_from_raw(r->raw_id);
    if (type==B_SUNKEN_COLONY || type==B_SPORE_COLONY)
        return race_colony_begin(owner,producer,type);
    rule = sc64_race_building_rule(type);
    if (!rule) return 0;
    bank.minerals = (unsigned short)race_owner_minerals(owner);
    bank.gas = (unsigned short)race_owner_gas(owner);
    if (!sc64_race_pay_building(&bank, type)) return 0;
    if (r->kind == SC64_SK_ADDON) {
        x = b[producer].x + 48; y = b[producer].y + 8;
        if (!can_place(type, x, y) || find_build_slot() < 0) return 0;
        slot = spawn_build(owner, type, x, y, 1);
        if (slot < 0) return 0;
        b[slot].build_progress = 0;
    } else {
        slot = producer;
        b[slot].raw_type = r->raw_id;
        b[slot].under_construction = 1; b[slot].build_progress = 0;
        /* BW applies a morph's maximum-HP difference when it completes.
         * Saves derive the maximum from the type and reject more hit
         * points than that, so a morph to a lower maximum keeps its source
         * type and maximum until race_complete_building switches it. */
        if (race_construction_deferred_morph(&b[slot]) < 0) {
            b[slot].type = type;
            b[slot].maxhp = sc64_race_building_max_hp(type);
        }
        invalidate_path_grid();
    }
    if (owner == SIDE) { minerals = bank.minerals; gas = bank.gas; }
    else { race_minerals[owner] = bank.minerals; race_gas[owner] = bank.gas; }
    return 1;
}

static int race_skirmish_record_available(const Sc64SkirmishRecord *r)
{
    int type;
    if (r && r->raw_id == 108) return 0;
    if (!race_mode || !race_skirmish_requirements(SIDE, r) ||
        (type = sc64_race_building_from_raw(r->raw_id)) < 0) return 0;
    if (r->kind == SC64_SK_WORKER_BUILD) {
        return race_skirmish_worker(type) >= 0;
    }
    return race_skirmish_record_ready(SIDE, selected_building, r);
}

static int race_skirmish_choose_record(const Sc64SkirmishRecord *r)
{
    if (!race_skirmish_record_available(r)) return 0;
    if (r->kind == SC64_SK_WORKER_BUILD) {
        mode = MODE_BUILD; menu = sc64_race_building_from_raw(r->raw_id); return 1;
    }
    mode = MODE_NORMAL;
    return race_skirmish_start_record(SIDE, selected_building, r);
}

#endif
