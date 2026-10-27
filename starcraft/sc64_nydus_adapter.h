/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * Optional Nydus Canal runtime (SC64_NYDUS_ENABLED, default off).
 *
 * Endpoints are ordinary B_NYDUS_CANAL buildings. A mission adapter supplies
 * its validated source link table with race_nydus_set_source() and binds the
 * spawned buildings with race_nydus_bind_world(1). Passengers stay in the
 * unit pool: ORDER_NYDUS_ENTER with target = entrance building slot. The
 * transfer completes atomically inside that passenger's order step, as in
 * the pinned reference, so no frame, save or query ever observes a unit in
 * transit and no cargo storage exists.
 *
 * The link table is derived, not saved: after a load it is rebuilt from the
 * same source records and the restored buildings' stable identities.
 */
#ifndef SC64_NYDUS_ADAPTER_H
#define SC64_NYDUS_ADAPTER_H

#ifdef SC64_NYDUS_ENABLED
#if !defined(SC64_RACES) || !defined(SC64_ZP_CAMPAIGN_ACTORS) || \
    !defined(SC64_ZP_CAMPAIGN_STORAGE) || !defined(SC64_UNIFIED)
#error "Nydus Canal requires the unified race runtime with ZP actors and storage"
#endif
#include "sc64_nydus_rules.h"

_Static_assert(MAX_BUILDINGS < SC64_NYDUS_NO_SLOT,
               "Nydus links store building slots in one byte");

static const Sc64NydusSourceRecord *race_nydus_source;
static Sc64NydusSourcePair race_nydus_pairs[SC64_NYDUS_MAX_LINKS];
static unsigned char race_nydus_pair_count;
static int race_nydus_source_mission = -1;
static Sc64NydusState race_nydus;

static void race_nydus_clear(void)
{
    race_nydus_source = 0;
    race_nydus_pair_count = 0;
    race_nydus_source_mission = -1;
    memset(&race_nydus, 0, sizeof(race_nydus));
}

/* Integration hook for a mission adapter (Z8 will pass its four raw-134
 * rows). Records must outlive the mission; x/y are the spawn centres. */
static int race_nydus_set_source(int mission, const Sc64NydusSourceRecord *records,
                                 unsigned count)
{
    unsigned pairs = 0;
    race_nydus_clear();
    if (mission < 0 ||
        !sc64_nydus_source_pairs(records, count, race_nydus_pairs, &pairs))
        return 0;
    race_nydus_source = records;
    race_nydus_pair_count = (unsigned char)pairs;
    race_nydus_source_mission = mission;
    return 1;
}

static int race_nydus_source_current(void)
{
    return race_nydus_source && !race_mode &&
        race_nydus_source_mission == race_selected_mission;
}

static Sc64NydusEndpointView race_nydus_view_of(const Building *q)
{
    Sc64NydusEndpointView v;
    memset(&v, 0, sizeof(v));
    if (!q->alive || q->type != B_NYDUS_CANAL || q->raw_type != SC64_NYDUS_RAW)
        return v;
    v.x = (short)q->x; v.y = (short)q->y;
    v.raw_type = SC64_NYDUS_RAW;
    v.alive = 1;
    v.complete = !q->under_construction;
    v.owner = (unsigned char)q->owner;
    return v;
}

static int race_nydus_bind_buildings(const Building *buildings, int require_all,
                                     Sc64NydusState *out)
{
    static Sc64NydusEndpointView views[MAX_BUILDINGS];
    if (!race_nydus_source_current()) return 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i) views[i] = race_nydus_view_of(&buildings[i]);
    return sc64_nydus_bind(race_nydus_source, race_nydus_pairs, race_nydus_pair_count,
                           views, MAX_BUILDINGS, require_all, out);
}

/* Mission start: both endpoints of every source pair must be live. */
static int race_nydus_bind_world(int require_all)
{
    Sc64NydusState next;
    if (!race_nydus_bind_buildings(b, require_all, &next)) return 0;
    race_nydus = next;
    return 1;
}

static int race_nydus_endpoint(int slot)
{
    return slot >= 0 && slot < MAX_BUILDINGS &&
        sc64_nydus_partner(&race_nydus, (unsigned)slot) >= 0;
}

/* slot < 0 describes a decoded save candidate outside u[]. */
static Sc64NydusPassengerView race_nydus_passenger_view(const Unit *q, int slot)
{
    Sc64NydusPassengerView p;
    const Sc64RaceUnitRule *rule = sc64_race_unit_rule(q->type);
    memset(&p, 0, sizeof(p));
    p.alive = q->alive != 0;
    p.owner = (unsigned char)q->owner;
    p.flying = sc64_race_unit_flying(q->type) != 0;
    p.zerg = rule && rule->race == R_ZERG;
    p.movable = !(sc64_game_unit_actor_flags(q->type) &&
                  !(sc64_game_unit_actor_flags(q->type) & SC64_GAME_ACTOR_MOVABLE)) &&
                q->type != U_CHRYSALIS;
#ifdef SC64_Z7_EGG_PRODUCTION
    if (q->type == U_Z7_EGG) p.movable = 0;
#endif
    p.loaded = SC64_TRANSPORT_LOADED(q) != 0;
    p.burrowed = (q->flags & 2) != 0;
#ifdef SC64_P6_COMBAT_ENABLED
    if (slot >= 0 && race_p6_combat_unit_burrowed(slot)) p.burrowed = 1;
#endif
#ifdef SC64_T6_RUNTIME
    if (slot >= 0 && race_t6_unit_burrowed(slot)) p.burrowed = 1;
#endif
    (void)slot;
    return p;
}

static int race_nydus_can_enter(int unit, int entrance)
{
    Sc64NydusEndpointView in, out;
    Sc64NydusPassengerView p;
    int exit;
    if (unit < 0 || unit >= MAX_UNITS || entrance < 0 || entrance >= MAX_BUILDINGS)
        return 0;
    exit = sc64_nydus_partner(&race_nydus, (unsigned)entrance);
    if (exit < 0 || exit >= MAX_BUILDINGS) return 0;
    in = race_nydus_view_of(&b[entrance]);
    out = race_nydus_view_of(&b[exit]);
    p = race_nydus_passenger_view(&u[unit], unit);
    return sc64_nydus_can_enter(&p, &in, &out);
}

static void race_nydus_cancel(Unit *q)
{
    q->order = ORDER_IDLE; q->target = -1;
    q->tx = q->x; q->ty = q->y;
    q->path_timer = q->path_retry = 0;
}

/* A slot is not an identity: release every order on a building row before
 * that row is reused or once its link is gone. */
static void race_nydus_release_orders(int slot)
{
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].order == ORDER_NYDUS_ENTER && u[i].target == slot)
            race_nydus_cancel(&u[i]);
}

/* OpenBW kill_unit(exit): unconditional, so source invincibility does not
 * keep a severed partner alive. Credit no killer. */
static void race_nydus_kill_partner(int slot)
{
    if (slot < 0 || slot >= MAX_BUILDINGS || !b[slot].alive) return;
    b[slot].flags &= ~16;
    invincible_buildings[slot] = 0;
    hit_target_impl_ex(MAX_UNITS + slot, b[slot].hp > 0 ? b[slot].hp : 1,
                       -1, -1, -1, SC64_HIT_PRECOMPUTED_HP);
}

/* Called from the building-death path and before a building row is reused.
 * Unlinking first makes the partner's own death a no-op here. */
static void race_nydus_note_building_lost(int slot)
{
    int partner;
    if (slot < 0 || slot >= MAX_BUILDINGS) return;
    partner = sc64_nydus_unlink(&race_nydus, (unsigned)slot);
    if (partner < 0) return;
    race_nydus_release_orders(slot);
    race_nydus_release_orders(partner);
    race_nydus_kill_partner(partner);
}

/* Per-frame reconciliation, before orders. Covers removals that bypass the
 * damage path (scenario effects) and source ownership transfers. Bounded by
 * SC64_NYDUS_MAX_LINKS. */
static void __attribute__((noinline)) race_nydus_tick_world(void)
{
    for (unsigned i = 0; i < race_nydus.count && i < SC64_NYDUS_MAX_LINKS; ++i) {
        Sc64NydusLink *l = &race_nydus.link[i];
        int s0, s1, from;
        if (!l->active) continue;
        s0 = l->slot[0]; s1 = l->slot[1];
        if (!race_nydus_view_of(&b[s0]).alive) { race_nydus_note_building_lost(s0); continue; }
        if (!race_nydus_view_of(&b[s1]).alive) { race_nydus_note_building_lost(s1); continue; }
        from = sc64_nydus_owner_source(l, (unsigned)b[s0].owner, (unsigned)b[s1].owner);
        if (from >= 0) {
            int src = from ? s1 : s0, dst = from ? s0 : s1;
            b[dst].owner = b[src].owner;
            b[dst].raw_owner = b[src].raw_owner;
            l->owner = (unsigned char)b[src].owner;
            race_nydus_release_orders(s0);
            race_nydus_release_orders(s1);
        } else if (l->owner != (unsigned char)b[s0].owner) {
            l->owner = (unsigned char)b[s0].owner;
            race_nydus_release_orders(s0);
            race_nydus_release_orders(s1);
        }
    }
}

/* OpenBW hide_unit() removes every visible unit's reference to the
 * passenger. Unit-target orders here are attack and Consume. */
static void race_nydus_clear_unit_references(int passenger)
{
    for (int i = 0; i < MAX_UNITS; ++i) {
        Unit *q = &u[i];
        if (i == passenger || !q->alive || q->target != passenger) continue;
        if (q->order == ORDER_ATTACK_TARGET
#ifdef SC64_Z5_CONSUME_ENABLED
            || q->order == ORDER_Z5_CONSUME
#endif
            ) race_nydus_cancel(q);
        else if (q->order == ORDER_ATTACK_MOVE || q->order == ORDER_IDLE ||
                 q->order == ORDER_MOVE)
            q->target = -1;
    }
#ifdef SC64_DIRECT_ATTACK
    sc64_direct_attack_clear_references(passenger);
#endif
}

static int race_nydus_place(int passenger, int exit, int *px, int *py)
{
    Unit *q = &u[passenger];
    for (unsigned n = 0; n < SC64_NYDUS_EXIT_CANDIDATES; ++n) {
        int x, y;
        if (!sc64_nydus_exit_candidate(n, b[exit].x, b[exit].y, &x, &y) ||
            x < 0 || y < 0 || x >= WORLD_W || y >= WORLD_H ||
            !is_traversable(x / 16, y / 16, -1) || !unit_step_clear(q, x, y))
            continue;
        *px = x; *py = y;
        return 1;
    }
    return 0;
}

/* GBA policy: imported-map routes cannot target the blocked canal centre,
 * so a passenger walks to the walkable contact-ring point nearest to it
 * (stored in tx/ty). Rings 1..4 are the first 80 search candidates. */
static int race_nydus_approach_point(int passenger, int entrance, int *px, int *py)
{
    int best = -1, best_x = 0, best_y = 0;
    for (unsigned n = 0; n < 80u; ++n) {
        int x, y, d;
        if (!sc64_nydus_exit_candidate(n, b[entrance].x, b[entrance].y, &x, &y) ||
            x < 0 || y < 0 || x >= WORLD_W || y >= WORLD_H ||
            !sc64_nydus_contact(x, y, b[entrance].x, b[entrance].y) ||
            !is_traversable(x / 16, y / 16, -1)) continue;
        d = dist2(x, y, u[passenger].x, u[passenger].y);
        if (best < 0 || d < best) { best = d; best_x = x; best_y = y; }
    }
    if (best < 0) return 0;
    *px = best_x; *py = best_y;
    return 1;
}

/* ORDER_NYDUS_ENTER service. Slot order makes simultaneous arrivals
 * deterministic: each placement sees every earlier passenger's new spot. */
static int __attribute__((noinline)) race_nydus_tick_unit(int slot)
{
    Unit *q = &u[slot];
    int entrance = q->target, exit, x, y;
    if (q->order != ORDER_NYDUS_ENTER) return 0;
    if (!race_nydus_can_enter(slot, entrance)) { race_nydus_cancel(q); return 1; }
    exit = sc64_nydus_partner(&race_nydus, (unsigned)entrance);
    if (!sc64_nydus_contact(q->x, q->y, b[entrance].x, b[entrance].y)) {
        if (!sc64_nydus_contact(q->tx, q->ty, b[entrance].x, b[entrance].y) ||
            !is_traversable(q->tx / 16, q->ty / 16, -1)) {
            int ax, ay;
            if (!race_nydus_approach_point(slot, entrance, &ax, &ay)) {
                race_nydus_cancel(q); return 1;
            }
            q->tx = ax; q->ty = ay;
        }
        move_toward(q, q->tx, q->ty);
        return 1;
    }
    /* Blocked exit: hold at the entrance and retry next order step. */
    if (!race_nydus_place(slot, exit, &x, &y)) return 1;
    race_nydus_clear_unit_references(slot);
    q->x = x; q->y = y;
    sc64_unit_bucket_move(q);
    sc64_target_index_move(q);
    race_nydus_cancel(q);
    return 1;
}

/* A press on an owned, linked endpoint with passengers selected (the GBA
 * equivalent of right-click Follow -> EnterNydusCanal). Returns orders
 * issued; zero leaves ordinary selection handling untouched. */
static int race_nydus_issue_selected(int x, int y)
{
    int entrance = -1, issued = 0;
    for (unsigned i = 0; i < race_nydus.count && i < SC64_NYDUS_MAX_LINKS; ++i)
        for (unsigned e = 0; e < 2 && race_nydus.link[i].active; ++e) {
            int s = race_nydus.link[i].slot[e];
            int dx = x - b[s].x, dy = y - b[s].y;
            if (b[s].alive && b[s].owner == SIDE &&
                dx >= -SC64_NYDUS_HALF_FOOTPRINT_PX && dx <= SC64_NYDUS_HALF_FOOTPRINT_PX &&
                dy >= -SC64_NYDUS_HALF_FOOTPRINT_PX && dy <= SC64_NYDUS_HALF_FOOTPRINT_PX)
                entrance = s;
        }
    if (entrance < 0) return 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].selected && u[i].owner == SIDE &&
            race_nydus_can_enter(i, entrance)) {
            int ax, ay;
            if (!race_nydus_approach_point(i, entrance, &ax, &ay)) continue;
            u[i].order = ORDER_NYDUS_ENTER;
            u[i].target = entrance;
            u[i].tx = ax; u[i].ty = ay;
            u[i].path_timer = u[i].path_retry = 0;
            ++issued;
        }
    return issued;
}

/* Save preflight against the decoded candidate. Binding the candidate's own
 * buildings proves endpoint identity without trusting live state; every
 * ORDER_NYDUS_ENTER must name an active endpoint the passenger may enter. */
static int race_nydus_save_candidate_valid(const RaceSave *candidate, int width,
                                           int height)
{
    static Building buildings[MAX_BUILDINGS];
    Sc64NydusState s;
    memset(&s, 0, sizeof(s));
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (!race_save_decode_building(candidate, (unsigned)i, &buildings[i])) return 0;
    if (race_nydus_source_current() &&
        !race_nydus_bind_buildings(buildings, 0, &s)) return 0;
    for (int i = 0; i < MAX_UNITS; ++i) {
        Unit q;
        Sc64NydusEndpointView in, out;
        Sc64NydusPassengerView p;
        int partner;
        if (!race_save_decode_unit(candidate, (unsigned)i, (unsigned)width * 16u,
                                   (unsigned)height * 16u, &q)) return 0;
        if (!q.alive || q.order != ORDER_NYDUS_ENTER) continue;
        if (!race_nydus_source_current() || q.target < 0 || q.target >= MAX_BUILDINGS)
            return 0;
        partner = sc64_nydus_partner(&s, (unsigned)q.target);
        if (partner < 0) return 0;
        in = race_nydus_view_of(&buildings[q.target]);
        out = race_nydus_view_of(&buildings[partner]);
        p = race_nydus_passenger_view(&q, -1);
        if (!sc64_nydus_can_enter(&p, &in, &out)) return 0;
    }
    return 1;
}

/* After a committed load: rebuild links from the restored buildings. A
 * mission without a current source simply has no Nydus links. */
static int race_nydus_rebind_loaded_world(void)
{
    if (!race_nydus_source_current()) {
        memset(&race_nydus, 0, sizeof(race_nydus));
        for (int i = 0; i < MAX_UNITS; ++i)
            if (u[i].alive && u[i].order == ORDER_NYDUS_ENTER) return 0;
        return 1;
    }
    return race_nydus_bind_world(0);
}

#endif /* SC64_NYDUS_ENABLED */
#endif
