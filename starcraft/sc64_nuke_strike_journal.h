/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_NUKE_STRIKE_JOURNAL_H
#define SC64_NUKE_STRIKE_JOURNAL_H

typedef struct {
    Sc64NukeSaveMissile missile[SC64_NUKE_FLIGHT_LIMIT];
    /* Preserve simulation order when simultaneous impacts share a tick.
     * A zero reservation means the corresponding flight slot is empty. */
    u8 actor_plus1[SC64_NUKE_FLIGHT_LIMIT];
} Sc64NukeStrikeLoadPlan;

typedef struct {
    const RaceSave *candidate;
    const Sc64NukeSaveWorld *world;
} Sc64NukeStrikeContext;

static int race_nuke_save_read_ghost(void *opaque, unsigned slot, Sc64NukeSaveGhostFacts *out)
{
    const Sc64NukeStrikeContext *ctx = opaque;
    Unit q;
    if (ctx->candidate) {
        if (!race_save_decode_unit(ctx->candidate, slot, ctx->world->width,
                                    ctx->world->height, &q)) return 0;
    } else q = u[slot];
    *out = (Sc64NukeSaveGhostFacts){
        q.alive, sc64_race_ordinary_ghost(q.type) &&
            (q.raw_type < 0 || q.raw_type == SC64_NUKE_GHOST_RAW),
        q.owner, q.order == ORDER_IDLE && q.target == -1 && !SC64_TRANSPORT_LOADED(&q)
    };
    return 1;
}

static int race_nuke_save_derive_cadence(void *context, unsigned ox, unsigned oy,
                                        unsigned x, unsigned y, Sc64NukeCadence *out)
{
    (void)context; (void)ox; (void)x;
    u8 ascent;
    *out = race_nuke_cadence_at(oy, y, &ascent);
    return 1;
}

static int race_nuke_save_encode_unit_row(RaceSave *candidate, unsigned slot,
    const Unit *row, const Sc64NukeSaveWorld *world)
{
#if defined(SC64_LATE_TERRAN_STORAGE) || defined(SC64_ZP_CAMPAIGN_STORAGE)
#ifdef SC64_ZP_FULL_STORAGE
#ifdef SC64_P7_TYPED_COMPACT_ENABLED
    return sc64_p7_typed_unit_encode(&candidate->units[slot], row, world->width, world->height);
#elif defined(SC64_Z7_DERIVED_SAVE)
    return sc64_derived_unit_encode(&candidate->units[slot], row, world->width, world->height);
#else
    return sc64_zp_unit_save_encode(&candidate->units[slot], row, world->width, world->height);
#endif
#else
    return sc64_unit_save_encode(&candidate->units[slot], row, world->width, world->height);
#endif
#else
    (void)world;
    for (unsigned j = 0; j < RACE_UNIT_WORDS; ++j) {
        int n;
        memcpy(&n, (const u8 *)row + j * sizeof n, sizeof n);
        candidate->units[slot][j] = (s16)n;
    }
    return 1;
#endif
}

static Sc64NukeSaveMissileWire race_nuke_save_read_wire(const Unit *q)
{
    return (Sc64NukeSaveMissileWire){
        .type = q->type, .raw_type = q->raw_type, .owner = q->owner,
        .alive = q->alive, .selected = q->selected, .idle = q->order == ORDER_IDLE,
        .x = q->x, .y = q->y, .tx = q->tx, .ty = q->ty, .work_timer = q->work_timer,
        .target = q->target, .build = q->build
    };
}

static Unit race_nuke_save_write_wire(const Sc64NukeSaveMissileWire *fields, unsigned track)
{
    Unit q = {0};
    q.x = fields->x; q.y = fields->y;
    q.tx = fields->tx; q.ty = fields->ty;
    q.type = fields->type; q.raw_type = fields->raw_type; q.raw_owner = -1;
    q.owner = fields->owner; q.alive = fields->alive;
    q.order = ORDER_IDLE; q.build = fields->build; q.target = fields->target;
    q.work_timer = fields->work_timer;
    q.path_timer = (u16)(track + 1);
    q.hp = q.maxhp = 100;
    return q;
}

/* The spare actor fields cannot carry ordinary unit or secondary subsystem
 * state. Avoid comparing any compiler struct padding. */
static int race_nuke_save_wire_extras_clear(const Unit *q)
{
    return q->hp == 100 && q->maxhp == 100 && q->raw_owner == -1 &&
        q->cd == 0 && q->carry == 0 && q->armor == 0 && q->weapon == 0 &&
        q->path_timer > 0 && q->path_timer <= SC64_NUKE_FLIGHT_LIMIT &&
        q->path_retry == 0 && q->path_x == 0 && q->path_y == 0 &&
        q->shield == 0 && q->flags == 0 && q->shield_remainder == 0;
}

static int race_nuke_save_write_strikes(const Sc64NukeSaveAuth *auth)
{
    Sc64NukeSaveWorld world = race_nuke_save_world(race_mode, race_bots, WORLD_W, WORLD_H);
    Sc64NukeStrikeContext ctx = {0, &world};
    Sc64NukeSaveCensus census = {0};
    u8 slots[sizeof race_nuke_unit_reserved] = {0};
    for (unsigned i = 0; i < SC64_NUKE_FLIGHT_LIMIT; ++i) {
        const Sc64NukeWorldFlight *f = &race_nuke_flights[i];
        if (!f->strike.phase) {
            if (f->actor_slot_plus1) return 0;
            continue;
        }
        unsigned slot = (unsigned)f->actor_slot_plus1 - 1u;
        if (slot >= MAX_UNITS || u[slot].alive || sc64_unit_slot_available((int)slot)) return 0;
        Sc64NukeSaveMissile missile = {f->strike, f->origin_x, f->origin_y};
        Sc64NukeSaveMissileWire fields;
        if (!sc64_nuke_save_missile_encode(&fields, slot, &missile, &world, auth,
                &race_data, race_data.version, race_nuke_save_derive_cadence,
                race_nuke_save_read_ghost, &ctx) ||
            !sc64_nuke_save_count_missile(&census, &world, slot, &missile)) return 0;
        Unit wire = race_nuke_save_write_wire(&fields, i);
        if (!race_nuke_save_encode_unit_row(&race_data, slot, &wire, &world)) return 0;
        slots[slot >> 3] |= (u8)(1u << (slot & 7u));
    }
    return memcmp(slots, race_nuke_unit_reserved, sizeof slots) == 0;
}

static int race_nuke_save_prepare_strikes(RaceSave *candidate, const Sc64NukeSaveWorld *world,
    const Sc64NukeSaveAuth *auth, Sc64NukeStrikeLoadPlan *out)
{
    /* Predecessor rows reach ordinary validation unchanged. It rejects
     * untagged raw14 and the new reserved enum even when the slot is dead. */
    if (!sc64_nuke_save_auth_matches(auth, candidate, candidate->version)) return 1;
    Sc64NukeStrikeContext ctx = {candidate, world};
    Sc64NukeSaveCensus census = {0};
    for (unsigned i = 0; i < MAX_UNITS; ++i) {
        Unit q;
        if (!race_save_decode_unit(candidate, i, world->width, world->height, &q)) return 0;
        Sc64NukeSaveMissileWire fields = race_nuke_save_read_wire(&q);
        if (!sc64_nuke_save_missile_claimed(&fields, world)) continue;
        Sc64NukeSaveMissile decoded;
        if (!race_nuke_save_wire_extras_clear(&q) ||
#ifdef SC64_UNIFIED
            RACE_SAVE_INV_U(candidate, i) ||
#endif
            !sc64_nuke_save_missile_decode(&decoded, i, &fields, world, auth, candidate,
                candidate->version, race_nuke_save_derive_cadence, race_nuke_save_read_ghost, &ctx) ||
            !sc64_nuke_save_count_missile(&census, world, i, &decoded)) return 0;
        unsigned track = (unsigned)q.path_timer - 1u;
        if (out->actor_plus1[track]) return 0;
        out->missile[track] = decoded;
        out->actor_plus1[track] = (u8)(i + 1);
    }
    /* Do not change the candidate until all spotter links and duplicate
     * reservations have been checked against the same set of original rows. */
    for (unsigned i = 0; i < SC64_NUKE_FLIGHT_LIMIT; ++i) {
        if (!out->actor_plus1[i]) continue;
        unsigned slot = (unsigned)out->actor_plus1[i] - 1u;
        memset(&candidate->units[slot], 0, sizeof candidate->units[slot]);
    }
    return 1;
}

static void race_nuke_save_restore_strikes(const Sc64NukeStrikeLoadPlan *plan)
{
    for (unsigned i = 0; i < SC64_NUKE_FLIGHT_LIMIT; ++i) {
        if (!plan->actor_plus1[i]) continue;
        const Sc64NukeSaveMissile *m = &plan->missile[i];
        Sc64NukeWorldFlight *f = &race_nuke_flights[i];
        f->strike = m->strike;
        f->origin_x = m->origin_x; f->origin_y = m->origin_y;
        (void)race_nuke_cadence_at(m->origin_y, m->strike.y, &f->ascent_ticks);
        race_nuke_reserve_actor(f, (unsigned)plan->actor_plus1[i] - 1u);
        ++race_nuke_active_flights;
    }
}
#endif
