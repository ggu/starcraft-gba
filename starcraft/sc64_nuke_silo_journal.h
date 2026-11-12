/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_NUKE_SILO_JOURNAL_H
#define SC64_NUKE_SILO_JOURNAL_H

/* Only raw108 reuses progress/level. A candidate plan caches each validated
 * receipt in 14 bits (11 progress, 3 payer), and is published after the
 * ordinary journal has selected and preflighted one complete world. */
#define SC64_NUKE_SILO_TRANSPORT_TAG \
    ((SC64_TRANSPORT_SAVE_VERSION & ~SC64_SAVE_TAG_NAMESPACE_MASK) | SC64_NUKE_SAVE_NAMESPACE)
_Static_assert(MAX_BUILDINGS <= SC64_NUKE_SAVE_BUILDING_LIMIT &&
               MAX_UNITS <= SC64_NUKE_SAVE_UNIT_LIMIT,
               "Silo journal census capacity");
_Static_assert(SC64_NUKE_SAVE_ARMED_PROGRESS < 2048, "Silo receipt progress");

static Sc64NukeSaveWorld race_nuke_save_world(int mode, int bots, unsigned width, unsigned height)
{
    static const unsigned owners[] = {SIDE, ENEMY, RESCUABLE, 4};
    unsigned owner_mask = 0;
    int count = mode == 0 ? 4 : mode == 1 && bots >= 1 && bots <= 3 ? bots + 1 : 0;
    for (int i = 0; i < count; ++i) owner_mask |= 1u << owners[i];
    return (Sc64NukeSaveWorld){width, height, MAX_UNITS, MAX_BUILDINGS, NEUTRAL, owner_mask,
#ifdef SC64_NUKE_ACTIVE_JOURNAL
                              U_NUKE_WIRE_MISSILE
#else
                              63 /* the missile actor format is not admitted */
#endif
                             };
}

static int race_nuke_new_unit_type(int type)
{
#ifdef SC64_NUKE_FALLBACK_GHOST
    return type == U_SHARED_GHOST;
#else
    (void)type; return 0;
#endif
}

static int race_nuke_new_building_type(int type)
{
#ifdef SC64_NUKE_FALLBACK_ADDONS
    if (type == B_SHARED_NUCLEAR_SILO || type == B_SHARED_COVERT_OPS) return 1;
#endif
#ifdef SC64_NUKE_FALLBACK_SCIENCE_FACILITY
    if (type == B_NUKE_SCIENCE_FACILITY) return 1;
#endif
    (void)type; return 0;
}

static int race_nuke_save_unit_valid(int type, int raw_type, int has_tag)
{
#ifdef SC64_NUKE_ACTIVE_JOURNAL
    /* Wire rows are consumed by detached preflight. The ordinary actor
     * layer must never admit a raw or synthetic missile row. */
    if (type == U_NUKE_WIRE_MISSILE) return 0;
#endif
    if (raw_type == SC64_NUKE_MISSILE_RAW) return 0;
    if (!race_nuke_new_unit_type(type)) return 1;
    return has_tag && (raw_type == -1 || raw_type == SC64_NUKE_GHOST_RAW);
}

static int race_nuke_save_building_identity(const Building *q, int has_tag)
{
    int raw = sc64_race_raw_building(q->type);
    if (q->raw_type == SC64_NUKE_SILO_RAW && raw != SC64_NUKE_SILO_RAW) return 0;
    if (raw == SC64_NUKE_SILO_RAW && q->raw_type != -1 && q->raw_type != raw) return 0;
    return !race_nuke_new_building_type(q->type) ||
        (has_tag && (q->raw_type == -1 || q->raw_type == raw));
}

/* Decide the outer namespace before serializing any bytes. A pending
 * approach has no wire representation and cannot be dropped. An already
 * resolved blast's short visual effect is transient. */
static int race_nuke_save_live_scope(int *has_tag)
{
    *has_tag = 0;
    for (unsigned i = 0; i < SC64_NUKE_FLIGHT_LIMIT; ++i) {
        if (race_nuke_flights[i].approaching) return 0;
        if (!race_nuke_flights[i].strike.phase) continue;
#ifdef SC64_NUKE_ACTIVE_JOURNAL
        *has_tag = 1;
#else
        return 0;
#endif
    }
    for (unsigned i = 0; i < MAX_UNITS; ++i) {
        if (!race_nuke_save_unit_valid(u[i].type, u[i].raw_type, 1)) return 0;
        if (race_nuke_new_unit_type(u[i].type)) *has_tag = 1;
    }
    for (unsigned i = 0; i < MAX_BUILDINGS; ++i) {
        if (!race_nuke_save_building_identity(&b[i], 1)) return 0;
        if (race_nuke_new_building_type(b[i].type) ||
            sc64_nuke_silo_supply(&race_nuke_silos[i].ammo)) *has_tag = 1;
        for (unsigned q = 0; q < 5; ++q)
            if (race_nuke_new_unit_type(b[i].queue[q])) *has_tag = 1;
#ifdef SC64_NUKE_ACTIVE_JOURNAL
            else if (b[i].queue[q] == U_NUKE_WIRE_MISSILE) return 0;
#endif
    }
    return 1;
}

static int race_nuke_save_train_allowed(int mode, int mission, const Building *silo)
{
#ifdef SC64_TERRAN_LATE_RUNTIME
    if (!mode && mission >= SC64_TERRAN_LATE_GLOBAL_T7 &&
        mission <= SC64_TERRAN_LATE_GLOBAL_T10) {
        const Sc64T4SourceSetup *setup = sc64_terran_late_setup_for_mission(mission);
        int source = race_terran_late_save_queue_source(setup, silo->owner, silo->raw_owner);
        return race_terran_late_save_product_available(mission, (unsigned)source, SC64_NUKE_MISSILE_RAW);
    }
#else
    (void)mode; (void)mission; (void)silo;
#endif
    return 1;
}

static Sc64NukeSaveSiloFacts race_nuke_save_silo_facts(const Building *q, int parent, int allowed)
{
    return (Sc64NukeSaveSiloFacts){
        sc64_race_raw_building(q->type), q->owner, q->alive,
        q->alive && !q->under_construction, parent, allowed, q->queue_count, q->queue_head
    };
}

/* Read either the detached candidate or live fields, never both at once.
 * Reject duplicate complete matching parents and any overlapping duplicate
 * Silo at a paid coordinate. Refuse a mismatched raw identity there. */
static int race_nuke_save_parent(const RaceSave *candidate, unsigned slot, const Building *silo)
{
    int count = 0;
    for (unsigned i = 0; i < MAX_BUILDINGS; ++i) {
        Building q;
        if (i == slot) continue;
        if (candidate) {
            if (!race_save_decode_building(candidate, i, &q)) return -1;
        } else q = b[i];
        if (!q.alive) continue;
        int raw = sc64_race_raw_building(q.type);
        if (raw == SC64_NUKE_SILO_RAW && q.x == silo->x && q.y == silo->y) return -1;
        if (raw != 106 || silo->owner == NEUTRAL || q.owner != silo->owner ||
            q.x + 48 != silo->x || q.y + 8 != silo->y ||
            q.under_construction || (q.flags & 4)) continue;
        if ((q.raw_type != -1 && q.raw_type != 106) || ++count > 1) return -1;
    }
    return count;
}

static int race_nuke_save_replace_building(RaceSave *candidate, unsigned slot,
    const Building *row, const Sc64NukeSaveWorld *world)
{
#if defined(SC64_T6_STORAGE) || defined(SC64_ZP_CAMPAIGN_STORAGE)
#ifdef SC64_P7_TYPED_COMPACT_ENABLED
    return sc64_p7_typed_building_encode(&candidate->buildings[slot], row,
                                         world->width, world->height);
#elif defined(SC64_Z7_DERIVED_SAVE)
    return sc64_derived_building_encode(&candidate->buildings[slot], row,
                                        world->width, world->height);
#else
    return sc64_building_save_encode(&candidate->buildings[slot], row,
                                      world->width, world->height);
#endif
#else
    (void)world;
    candidate->buildings[slot][offsetof(Building,queue_progress)/sizeof(int)] = (s16)row->queue_progress;
    candidate->buildings[slot][offsetof(Building,level)/sizeof(int)] = (s16)row->level;
    return 1;
#endif
}

static int race_nuke_save_write_silos(const Sc64NukeSaveAuth *auth, int has_tag)
{
    Sc64NukeSaveWorld world = race_nuke_save_world(race_mode, race_bots, WORLD_W, WORLD_H);
    Sc64NukeSaveCensus census = {0};
    unsigned paid = 0;
    for (unsigned i = 0; i < MAX_BUILDINGS; ++i) {
        const Building *q = &b[i];
        const Sc64NukeWorldSilo *s = &race_nuke_silos[i];
        if (sc64_race_raw_building(q->type) == SC64_NUKE_SILO_RAW) {
            /* The live sidecar, never the Building, owns the receipt. */
            if (q->queue_count || q->queue_head || q->queue_progress || q->level) return 0;
        } else if (sc64_nuke_silo_supply(&s->ammo)) return 0;
        if (!sc64_nuke_silo_supply(&s->ammo)) continue;
        int parent = race_nuke_save_parent(0, i, q);
        Sc64NukeSaveSiloFacts facts = race_nuke_save_silo_facts(q, parent,
            race_nuke_save_train_allowed(race_mode, race_selected_mission, q));
        Sc64NukeSaveSilo state = {s->ammo, s->payer};
        Sc64NukeSaveSiloWire fields;
        if (!has_tag || !sc64_nuke_save_silo_encode(&fields, &state, &facts, &world,
                auth, &race_data, race_data.version) ||
            !sc64_nuke_save_count_silo(&census, &world, i, &state)) return 0;
        Building wire = *q;
        wire.queue_progress = fields.queue_progress; wire.level = fields.level;
        if (!race_nuke_save_replace_building(&race_data, i, &wire, &world)) return 0;
        ++paid;
    }
    return paid == race_nuke_paid_silos;
}

static int race_nuke_save_candidate_building(const RaceSave *candidate, unsigned slot,
    const Building *q, const Sc64NukeSaveWorld *world, const Sc64NukeSaveAuth *auth,
    Sc64NukeSaveCensus *census, u16 *receipt)
{
    int has_tag = sc64_nuke_save_auth_matches(auth, candidate, candidate->version);
    if (!race_nuke_save_building_identity(q, has_tag)) return 0;
    for (unsigned j = 0; j < 5; ++j) {
        if (race_nuke_new_unit_type(q->queue[j]) && !has_tag) return 0;
#ifdef SC64_NUKE_ACTIVE_JOURNAL
        if (q->queue[j] == U_NUKE_WIRE_MISSILE) return 0;
#endif
    }
    *receipt = 0;
    if (sc64_race_raw_building(q->type) != SC64_NUKE_SILO_RAW) return 1;
    if (q->queue_count || q->queue_head) return 0;
    const Sc64NukeSaveSiloWire wire = {q->queue_progress, q->level};
    if (sc64_nuke_save_legacy_silo_clear(&wire)) return 1;
    int parent = race_nuke_save_parent(candidate, slot, q);
    Sc64NukeSaveSiloFacts facts = race_nuke_save_silo_facts(q, parent,
        race_nuke_save_train_allowed(candidate->state[0], candidate->state[2], q));
    Sc64NukeSaveSilo state;
    if (!sc64_nuke_save_silo_decode(&state, &wire, &facts, world, auth,
                                    candidate, candidate->version) ||
        !sc64_nuke_save_count_silo(census, world, slot, &state)) return 0;
    *receipt = (u16)(wire.queue_progress | ((unsigned)state.payer << 11));
    return 1;
}

static void race_nuke_save_restore(const u16 *receipts)
{
    race_nuke_reset();
    for (unsigned i = 0; i < MAX_BUILDINGS; ++i) {
        if (!receipts[i]) continue;
        unsigned progress = receipts[i] & 2047u;
        race_nuke_silos[i].payer = (u8)(receipts[i] >> 11);
        if (progress == SC64_NUKE_SAVE_ARMED_PROGRESS) race_nuke_silos[i].ammo.armed = 1;
        else race_nuke_silos[i].ammo.remaining = (u16)progress;
        b[i].queue_progress = 0; b[i].level = 0;
        ++race_nuke_paid_silos;
    }
}
#endif
