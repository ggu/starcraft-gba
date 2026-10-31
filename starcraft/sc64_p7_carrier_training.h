/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_P7_CARRIER_TRAINING_H
#define SC64_P7_CARRIER_TRAINING_H

#if defined(SC64_P7_CARRIER_TRAINING_ENABLED) || defined(SC64_SHARED_CARRIER_EXPERIMENT)
#if !defined(SC64_SHARED_CARRIER_EXPERIMENT) && (!defined(SC64_P7_PROJECTILES_ENABLED) || !defined(SC64_P7_ACTORS))
#error "P7 Carrier training requires the typed P7 Interceptor actor profile"
#endif

/* Carrier Interceptors are real Unit rows, but remain hidden in the hangar
 * until a separate source-faithful flight/IScript adapter is available. */
#define SC64_P7_CARRIER_RAW 72
#define SC64_P7_INTERCEPTOR_RAW 73
#define SC64_P7_CARRIER_CAPACITY 4
#define SC64_P7_CARRIER_QUEUE_LIMIT 5
#define SC64_P7_INTERCEPTOR_COST 25
#define SC64_P7_INTERCEPTOR_BUILD_TICKS 300
#define SC64_P7_INTERCEPTOR_HP_RATE_Q8 30

#define SC64_P7_CARRIER_STATE_SHIFT 8
#define SC64_P7_CARRIER_STATE_MASK (7u << SC64_P7_CARRIER_STATE_SHIFT)
#define SC64_P7_CARRIER_RANK_SHIFT 11
#define SC64_P7_CARRIER_RANK_MASK (7u << SC64_P7_CARRIER_RANK_SHIFT)
#define SC64_P7_CARRIER_RUNTIME_MASK \
    (SC64_P7_CARRIER_STATE_MASK | SC64_P7_CARRIER_RANK_MASK)

enum {
    SC64_P7_INTERCEPTOR_NO_LINK = 0,
    SC64_P7_INTERCEPTOR_TRAINING = 1,
    SC64_P7_INTERCEPTOR_INSIDE = 2,
    /* Generation 43 (SC64_P7_INTERCEPTOR_FLIGHT_ENABLED) only. */
    SC64_P7_INTERCEPTOR_OUTSIDE = 3,
    SC64_P7_INTERCEPTOR_DETACHED = 4
};

#if defined(SC64_P7_INTERCEPTOR_FLIGHT_ENABLED) || defined(SC64_SHARED_CARRIER_EXPERIMENT)
static void race_p7_interceptor_flight_detach(int child);
static void race_p7_interceptor_flight_source_tick(void);
static void race_p7_interceptor_flight_frame_move(void);
static int race_p7_interceptor_flight_child_valid_in(const Unit *actors,
                                                     int count, int slot);
#endif

static unsigned sc64_p7_carrier_child_state(const Unit *child)
{
    return child ? ((unsigned)child->flags & SC64_P7_CARRIER_STATE_MASK) >>
                       SC64_P7_CARRIER_STATE_SHIFT : 0;
}

static unsigned sc64_p7_carrier_child_rank(const Unit *child)
{
    return child ? ((unsigned)child->flags & SC64_P7_CARRIER_RANK_MASK) >>
                       SC64_P7_CARRIER_RANK_SHIFT : 0;
}

static void sc64_p7_carrier_child_set_state(Unit *child, unsigned state)
{
    child->flags = (int)(((unsigned)child->flags &
                          ~SC64_P7_CARRIER_STATE_MASK) |
                         ((state << SC64_P7_CARRIER_STATE_SHIFT) &
                          SC64_P7_CARRIER_STATE_MASK));
}

static void sc64_p7_carrier_child_set_rank(Unit *child, unsigned rank)
{
    child->flags = (int)(((unsigned)child->flags &
                          ~SC64_P7_CARRIER_RANK_MASK) |
                         ((rank << SC64_P7_CARRIER_RANK_SHIFT) &
                          SC64_P7_CARRIER_RANK_MASK));
}

static int sc64_p7_carrier_parent_valid_in(const Unit *actors, int count,
                                           int slot)
{
    int mapped_owner, service_slot;
    if (!actors || count < 0 || count > MAX_UNITS || slot < 0 || slot >= count)
        return 0;
    service_slot = RACE_PROTOSS_SLOT(&actors[slot]);
    mapped_owner = service_slot >= 0 && service_slot < 8 ?
        race_protoss_source_owner((unsigned)service_slot) : -1;
    return actors[slot].alive && mapped_owner >= 0 &&
        actors[slot].type == SC64_CARRIER_TYPE &&
        RACE_PROTOSS_RAW(&actors[slot]) == SC64_P7_CARRIER_RAW &&
        actors[slot].owner == mapped_owner;
}

static int sc64_p7_carrier_parent_valid(int slot)
{ return sc64_p7_carrier_parent_valid_in(u, MAX_UNITS, slot); }

static int sc64_p7_carrier_child_valid_in(const Unit *actors, int count,
                                          int slot)
{
    int mapped_owner, service_slot;
    if (!actors || count < 0 || count > MAX_UNITS || slot < 0 || slot >= count)
        return 0;
    service_slot = RACE_PROTOSS_SLOT(&actors[slot]);
    mapped_owner = service_slot >= 0 && service_slot < 8 ?
        race_protoss_source_owner((unsigned)service_slot) : -1;
    return actors[slot].alive && mapped_owner >= 0 &&
        actors[slot].type == SC64_INTERCEPTOR_TYPE &&
        RACE_PROTOSS_RAW(&actors[slot]) == SC64_P7_INTERCEPTOR_RAW &&
        actors[slot].owner == mapped_owner;
}

static int sc64_p7_carrier_child_valid(int slot)
{ return sc64_p7_carrier_child_valid_in(u, MAX_UNITS, slot); }

static int sc64_p7_carrier_count_children_in(const Unit *actors, int count,
                                             int carrier, unsigned *inside,
                                             unsigned *training)
{
    unsigned in = 0, work = 0;
    if (!sc64_p7_carrier_parent_valid_in(actors, count, carrier)) return 0;
    for (int i = 0; i < count; ++i) {
        const Unit *child = &actors[i];
        unsigned state;
        if (!child->alive || child->type != SC64_INTERCEPTOR_TYPE ||
            RACE_PROTOSS_RAW(child) != SC64_P7_INTERCEPTOR_RAW ||
            child->build != carrier) continue;
        state = sc64_p7_carrier_child_state(child);
        if (state == SC64_P7_INTERCEPTOR_INSIDE) ++in;
        else if (state == SC64_P7_INTERCEPTOR_TRAINING) ++work;
#if defined(SC64_P7_INTERCEPTOR_FLIGHT_ENABLED) || defined(SC64_SHARED_CARRIER_EXPERIMENT)
        /* Launched children still occupy one of the Carrier's slots. */
        else if (state == SC64_P7_INTERCEPTOR_OUTSIDE) ++in;
#endif
        else return 0;
    }
    if (inside) *inside = in;
    if (training) *training = work;
    return 1;
}

static int sc64_p7_carrier_count_children(int carrier, unsigned *inside,
                                          unsigned *training)
{ return sc64_p7_carrier_count_children_in(u, MAX_UNITS, carrier, inside, training); }

static unsigned race_p7_carrier_training_child_count(int carrier)
{
    unsigned inside = 0, training = 0;
    if (!sc64_p7_carrier_count_children(carrier, &inside, &training)) return 0;
    return inside + training;
}

/* Count all slot commitments that can compete with P7 Carrier training:
 * remaining authenticated script creates, queued building products, and
 * already-paid Carrier orders. The current training child is an alive Unit,
 * so it is already deducted from free slots. `wanted` is the new commitment
 * being considered by either Carrier queueing or the ordinary race queue. */
static int race_p7_carrier_training_slots_available(unsigned wanted)
{
    unsigned free_slots = 0, commitments = wanted, created = 0;
    if (!race_protoss_services_active()) return 1;
    /* Only P7 reserves slots for its three scripted Carrier creates. */
#ifdef SC64_P7_RUNTIME
    if (race_p7_active()) {
    if (race_p7.created_mask & 0xf8u) return 0;
    for (unsigned mask = race_p7.created_mask & 7u; mask; mask &= mask - 1u)
        ++created;
    commitments += 3u - created;
    }
#else
    (void)created;
#endif
    for (int i = 0; i < MAX_UNITS; ++i) {
        if (!u[i].alive || u[i].type != SC64_CARRIER_TYPE ||
            RACE_PROTOSS_RAW(&u[i]) != SC64_P7_CARRIER_RAW) continue;
        if (u[i].carry < 0 || u[i].carry >= SC64_P7_CARRIER_QUEUE_LIMIT)
            return 0;
        commitments += (unsigned)u[i].carry;
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive) {
        if (b[i].queue_count < 0 || b[i].queue_count > 5 ||
            b[i].queue_head < 0 || b[i].queue_head >= 5) return 0;
        for (int q = 0; q < b[i].queue_count; ++q) {
            int type = b[i].queue[(b[i].queue_head + q) % 5];
            if (type < 0 || type >= SC_RU_COUNT) return 0;
            commitments += (unsigned)sc64_race_product_count(type);
        }
    }
    for (int i = 0; i < MAX_UNITS; ++i) free_slots += !u[i].alive;
    return commitments <= free_slots;
}

/* The save/load candidate check calls this before committing. The typed
 * codec already bounds Unit fields; this adds link and state
 * invariants without changing its 28-byte row. */
static int race_p7_carrier_training_candidate_valid(const Unit *actors,
                                                    int count)
{
    unsigned char rank_bits[MAX_UNITS];
    if (!actors || count < 0 || count > MAX_UNITS) return 0;
    for (int i = 0; i < MAX_UNITS; ++i) rank_bits[i] = 0;
    for (int i = 0; i < count; ++i) {
        const Unit *q = &actors[i];
        unsigned state;
        if ((RACE_PROTOSS_RAW(q) == SC64_P7_CARRIER_RAW) !=
            (q->type == SC64_CARRIER_TYPE)) return 0;
        if ((RACE_PROTOSS_RAW(q) == SC64_P7_INTERCEPTOR_RAW) !=
            (q->type == SC64_INTERCEPTOR_TYPE)) return 0;
        if (!q->alive) {
            if (q->type == SC64_INTERCEPTOR_TYPE &&
                (((unsigned)q->flags & SC64_P7_CARRIER_RUNTIME_MASK) ||
                 q->build >= 0)) return 0;
            if (q->type == SC64_CARRIER_TYPE && q->carry) return 0;
            continue;
        }
        if (q->type == SC64_CARRIER_TYPE) {
            unsigned inside = 0, training = 0;
            if (!sc64_p7_carrier_parent_valid_in(actors, count, i) ||
                q->carry < 0 ||
                q->carry >= SC64_P7_CARRIER_QUEUE_LIMIT ||
                ((unsigned)q->flags & SC64_P7_CARRIER_RUNTIME_MASK) ||
                !sc64_p7_carrier_count_children_in(actors, count, i,
                                                    &inside, &training) ||
                training > 1 || inside + training + (unsigned)q->carry >
                    SC64_P7_CARRIER_CAPACITY) return 0;
#if defined(SC64_P7_INTERCEPTOR_FLIGHT_ENABLED) || defined(SC64_SHARED_CARRIER_EXPERIMENT)
            /* Generation 43 keeps the 7-tick release timer in work_timer. */
            if (q->work_timer > 7) return 0;
#endif
        }
        if (q->type != SC64_INTERCEPTOR_TYPE) continue;
#if defined(SC64_P7_INTERCEPTOR_FLIGHT_ENABLED) || defined(SC64_SHARED_CARRIER_EXPERIMENT)
        state = sc64_p7_carrier_child_state(q);
        if (state == SC64_P7_INTERCEPTOR_OUTSIDE ||
            state == SC64_P7_INTERCEPTOR_DETACHED) {
            if (!race_p7_interceptor_flight_child_valid_in(actors, count, i))
                return 0;
            continue;
        }
#endif
        if (!sc64_p7_carrier_child_valid_in(actors, count, i) ||
            q->maxhp != 40 || q->hp < 1 || q->hp > 40 ||
            q->shield != 40 || q->carry < 0 || q->carry > 255 ||
            q->build < 0 || q->build >= MAX_UNITS ||
            !sc64_p7_carrier_parent_valid_in(actors, count, q->build) ||
            actors[q->build].owner != q->owner ||
            actors[q->build].raw_owner != q->raw_owner) return 0;
        state = sc64_p7_carrier_child_state(q);
        if (state == SC64_P7_INTERCEPTOR_TRAINING) {
            if (q->work_timer < 1 ||
                q->work_timer > SC64_P7_INTERCEPTOR_BUILD_TICKS) return 0;
            unsigned built = SC64_P7_INTERCEPTOR_BUILD_TICKS -
                             (unsigned)q->work_timer;
            unsigned hp_q8 = 4u * 256u + built * SC64_P7_INTERCEPTOR_HP_RATE_Q8;
            if (sc64_p7_carrier_child_rank(q) != 0 || q->target != -1 ||
                q->selected || q->cd || q->order != ORDER_IDLE ||
                q->shield_remainder ||
                (unsigned)q->hp * 256u + (unsigned)q->carry != hp_q8)
                return 0;
        } else if (state == SC64_P7_INTERCEPTOR_INSIDE) {
            unsigned rank = sc64_p7_carrier_child_rank(q);
            if (q->work_timer || q->hp != 40 || q->carry || rank >= 4 ||
                q->target != -1 || q->selected || q->cd ||
                q->order != ORDER_IDLE || q->shield_remainder ||
                (rank_bits[q->build] & (1u << rank))) return 0;
            rank_bits[q->build] |= 1u << rank;
        } else return 0; /* Flight, detached and other states are unsupported. */
    }
    for (int i = 0; i < MAX_UNITS; ++i) if (rank_bits[i]) {
        unsigned count = 0, bits = rank_bits[i];
        while (bits) { count += bits & 1u; bits >>= 1; }
        if (rank_bits[i] != ((1u << count) - 1u)) return 0;
    }
    return 1;
}

static int race_p7_carrier_training_live_world_valid(void)
{ return !race_protoss_services_active() || race_p7_carrier_training_candidate_valid(u, MAX_UNITS); }

static int race_p7_carrier_training_selected(void)
{
    if (!race_protoss_services_active()) return 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].selected && u[i].owner==SIDE &&
            RACE_PROTOSS_SLOT(&u[i])==SC64_PROTOSS_HUMAN_SLOT &&
            sc64_p7_carrier_parent_valid(i)) return 1;
    return 0;
}

static int race_p7_carrier_training_can_queue_owner(int carrier, int owner)
{
    unsigned inside = 0, training = 0;
    Unit *parent;
    if (!race_protoss_services_active() || !sc64_p7_carrier_parent_valid(carrier) ||
        u[carrier].owner != owner ||
        (!race_mode && (owner != SIDE ||
         RACE_PROTOSS_SLOT(&u[carrier]) != SC64_PROTOSS_HUMAN_SLOT))) return 0;
    parent = &u[carrier];
    return parent->carry >= 0 && parent->carry < SC64_P7_CARRIER_QUEUE_LIMIT &&
        race_owner_minerals(owner) >= SC64_P7_INTERCEPTOR_COST &&
        sc64_p7_carrier_count_children(carrier, &inside, &training) &&
        training <= 1 && training + (unsigned)parent->carry <
            SC64_P7_CARRIER_QUEUE_LIMIT &&
        inside + training + (unsigned)parent->carry < SC64_P7_CARRIER_CAPACITY &&
        race_p7_carrier_training_slots_available(1);
}

static int race_p7_carrier_training_menu_available(void)
{
    return race_p7_carrier_training_selected();
}

/* The user's command handler should call this only for a selected SIDE
 * Carrier in P7. It charges exactly the Interceptor price and stores the
 * already-paid order count on the parent. In authenticated P7 PUPx data, human
 * slot 7 has Carrier Capacity current/max level 0/0. Enemy slot 4 uses the
 * global default, whose current/max levels are also 0/0. Thus both active
 * Carrier owners use four slots in this mission. */
static int race_p7_carrier_training_queue(int carrier)
{
    return race_p7_carrier_training_queue_owner(carrier, SIDE);
}

static int race_p7_carrier_training_queue_owner(int carrier, int owner)
{
    if (!race_p7_carrier_training_can_queue_owner(carrier, owner)) return 0;
    race_add_owner_resources(owner, -SC64_P7_INTERCEPTOR_COST, 0);
    ++u[carrier].carry;
    return 1;
}

static int race_p7_carrier_training_queue_selected(void)
{
    int queued = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].selected && race_p7_carrier_training_queue(i)) ++queued;
    return queued;
}

/* Call once after the shared source-24 due event has been established. A
 * paid order remains queued until its slot fits all dynamic commitments.
 * spawn_unit performs canonical slot-reuse/cache cleanup. */
static int race_p7_carrier_training_start(int carrier)
{
    unsigned inside = 0, training = 0;
    Unit *parent;
    int slot;
    if (!race_protoss_services_active() || !sc64_p7_carrier_parent_valid(carrier)) return -1;
    parent = &u[carrier];
    if (parent->carry <= 0 ||
        !sc64_p7_carrier_count_children(carrier, &inside, &training) ||
        training || inside + (unsigned)parent->carry > SC64_P7_CARRIER_CAPACITY ||
        !race_p7_carrier_training_slots_available(0)) return -1;
    slot = spawn_unit(parent->owner, SC64_INTERCEPTOR_TYPE, parent->x, parent->y);
    if (slot < 0 || slot >= MAX_UNITS) return -1;
    u[slot].raw_owner = parent->raw_owner;
    if (!sc64_p7_carrier_child_valid(slot)) {
        race_p7_projectile_clear_references(slot);
        u[slot].alive = 0;
        return -1;
    }
    u[slot].x = parent->x; u[slot].y = parent->y;
    u[slot].tx = parent->x; u[slot].ty = parent->y;
    u[slot].maxhp = 40; u[slot].hp = 4; u[slot].shield = 40;
    u[slot].build = carrier; u[slot].target = -1;
    u[slot].carry = 0;
    u[slot].work_timer = SC64_P7_INTERCEPTOR_BUILD_TICKS;
    sc64_p7_carrier_child_set_state(&u[slot], SC64_P7_INTERCEPTOR_TRAINING);
    sc64_p7_carrier_child_set_rank(&u[slot], 0);
    --parent->carry;
    return slot;
}

static void sc64_p7_carrier_inside_insert_front(int child_slot)
{
    Unit *child;
    int parent;
    if (!sc64_p7_carrier_child_valid(child_slot)) return;
    child = &u[child_slot]; parent = child->build;
    if (!sc64_p7_carrier_parent_valid(parent)) return;
    for (int i = 0; i < MAX_UNITS; ++i) {
        Unit *q = &u[i];
        if (i == child_slot || !sc64_p7_carrier_child_valid(i) ||
            q->build != parent ||
            sc64_p7_carrier_child_state(q) != SC64_P7_INTERCEPTOR_INSIDE)
            continue;
        sc64_p7_carrier_child_set_rank(q,
            sc64_p7_carrier_child_rank(q) + 1u);
    }
    sc64_p7_carrier_child_set_state(child, SC64_P7_INTERCEPTOR_INSIDE);
    sc64_p7_carrier_child_set_rank(child, 0);
    child->work_timer = 0;
}

/* A Carrier that enters play with a stocked hangar (an authored scene or a
 * map placement with Interceptors aboard): add up to `count` docked,
 * fully built children. Returns how many were added. */
static int race_p7_carrier_training_stock(int carrier, unsigned count)
{
    unsigned inside = 0, training = 0, added = 0;
    if (!race_protoss_services_active() || !sc64_p7_carrier_parent_valid(carrier))
        return 0;
    while (added < count &&
           sc64_p7_carrier_count_children(carrier, &inside, &training) &&
           inside + training + (unsigned)u[carrier].carry < SC64_P7_CARRIER_CAPACITY &&
           race_p7_carrier_training_slots_available(1)) {
        int slot = spawn_unit(u[carrier].owner, SC64_INTERCEPTOR_TYPE,
                              u[carrier].x, u[carrier].y);
        if (slot < 0 || slot >= MAX_UNITS) break;
        u[slot].raw_owner = u[carrier].raw_owner;
        if (!sc64_p7_carrier_child_valid(slot)) {
            race_p7_projectile_clear_references(slot);
            u[slot].alive = 0;
            break;
        }
        u[slot].tx = u[carrier].x; u[slot].ty = u[carrier].y;
        u[slot].maxhp = 40; u[slot].hp = 40; u[slot].shield = 40;
        u[slot].build = carrier; u[slot].target = -1;
        u[slot].carry = 0; u[slot].work_timer = 0;
        sc64_p7_carrier_inside_insert_front(slot);
        ++added;
    }
    return (int)added;
}

/* Call only on the existing source-24 due event. Tick existing trainers
 * before starting new orders on that event so every child receives 300 full
 * source ticks before completion. */
static void race_p7_carrier_training_parent_died(
    int carrier, void (*clear_projectile_refs)(int));
static void sc64_p7_carrier_child_remove(int child_slot,
                                         void (*clear_projectile_refs)(int));

static void race_p7_carrier_training_clear_slot(int slot)
{
    if (slot < 0 || slot >= MAX_UNITS) return;
    if (u[slot].type == SC64_CARRIER_TYPE &&
        RACE_PROTOSS_RAW(&u[slot]) == SC64_P7_CARRIER_RAW) {
        race_p7_carrier_training_parent_died(
            slot, race_p7_projectile_clear_references);
    } else if (u[slot].type == SC64_INTERCEPTOR_TYPE &&
               RACE_PROTOSS_RAW(&u[slot]) == SC64_P7_INTERCEPTOR_RAW) {
        sc64_p7_carrier_child_remove(slot,
                                     race_p7_projectile_clear_references);
    }
}

static void race_p7_carrier_training_source_tick(void)
{
    for (int i = 0; i < MAX_UNITS; ++i) {
        Unit *child = &u[i];
        if (!child->alive || child->type != SC64_INTERCEPTOR_TYPE ||
            RACE_PROTOSS_RAW(child) != SC64_P7_INTERCEPTOR_RAW) continue;
#if defined(SC64_P7_INTERCEPTOR_FLIGHT_ENABLED) || defined(SC64_SHARED_CARRIER_EXPERIMENT)
        /* Flight owns launched and detached children. */
        if (sc64_p7_carrier_child_state(child) >= SC64_P7_INTERCEPTOR_OUTSIDE)
            continue;
#endif
        if (child->build < 0 || child->build >= MAX_UNITS ||
            !sc64_p7_carrier_parent_valid(child->build)) {
            race_p7_carrier_training_clear_slot(i);
            continue;
        }
        if (!sc64_p7_carrier_child_valid(i) ||
            sc64_p7_carrier_child_state(child) != SC64_P7_INTERCEPTOR_TRAINING)
            continue;
        if (child->work_timer <= 0 ||
            child->work_timer > SC64_P7_INTERCEPTOR_BUILD_TICKS) continue;
        --child->work_timer;
        child->carry += SC64_P7_INTERCEPTOR_HP_RATE_Q8;
        child->hp += child->carry / 256;
        child->carry %= 256;
        if (child->work_timer == 0) {
            child->hp = 40; child->shield = 40; child->carry = 0;
            sc64_p7_carrier_inside_insert_front(i);
        }
    }
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].carry>0 && sc64_p7_carrier_parent_valid(i))
            (void)race_p7_carrier_training_start(i);
}

/* game_update can call this before orders; race_p7 is fully defined by the
 * time this adapter header is included. Match the shared source-24 phase used
 * by the projectile and Observer systems, without introducing another clock. */
static void race_p7_carrier_training_frame_tick(void)
{
    if (race_protoss_services_active() && RACE_PROTOSS_SERVICE_CLOCK[0] != 4u) {
        race_p7_carrier_training_source_tick();
#if defined(SC64_P7_INTERCEPTOR_FLIGHT_ENABLED) || defined(SC64_SHARED_CARRIER_EXPERIMENT)
        race_p7_interceptor_flight_source_tick();
#endif
    }
#if defined(SC64_P7_INTERCEPTOR_FLIGHT_ENABLED) || defined(SC64_SHARED_CARRIER_EXPERIMENT)
    if (race_protoss_services_active()) race_p7_interceptor_flight_frame_move();
#endif
}

/* Compact the parent's visible hangar order after a child dies. */
static void sc64_p7_carrier_child_remove(int child_slot,
                                         void (*clear_projectile_refs)(int))
{
    Unit *child;
    int parent;
    unsigned old_state, old_rank;
    if (child_slot < 0 || child_slot >= MAX_UNITS || !clear_projectile_refs ||
        u[child_slot].type != SC64_INTERCEPTOR_TYPE ||
        RACE_PROTOSS_RAW(&u[child_slot]) != SC64_P7_INTERCEPTOR_RAW) return;
    child = &u[child_slot]; parent = child->build;
    old_state = sc64_p7_carrier_child_state(child);
    old_rank = sc64_p7_carrier_child_rank(child);
    clear_projectile_refs(child_slot);
    if (sc64_p7_carrier_parent_valid(parent) &&
        old_state == SC64_P7_INTERCEPTOR_INSIDE) {
        for (int i = 0; i < MAX_UNITS; ++i) {
            Unit *q = &u[i];
            if (i == child_slot || !sc64_p7_carrier_child_valid(i) ||
                q->build != parent ||
                sc64_p7_carrier_child_state(q) != SC64_P7_INTERCEPTOR_INSIDE ||
                sc64_p7_carrier_child_rank(q) <= old_rank) continue;
            sc64_p7_carrier_child_set_rank(q,
                sc64_p7_carrier_child_rank(q) - 1u);
        }
    }
    child->build = -1; child->target = -1; child->carry = 0;
    child->work_timer = 0;
    child->flags = (int)((unsigned)child->flags &
                         ~SC64_P7_CARRIER_RUNTIME_MASK);
    child->alive = 0;
}

/* Parent destruction synchronously removes all hidden children. Invoke
 * before the Carrier row is marked dead so links can be checked safely. */
static void race_p7_carrier_training_parent_died(
    int carrier, void (*clear_projectile_refs)(int))
{
    if (carrier < 0 || carrier >= MAX_UNITS || !clear_projectile_refs) return;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].type == SC64_INTERCEPTOR_TYPE &&
            RACE_PROTOSS_RAW(&u[i]) == SC64_P7_INTERCEPTOR_RAW && u[i].build == carrier) {
#if defined(SC64_P7_INTERCEPTOR_FLIGHT_ENABLED) || defined(SC64_SHARED_CARRIER_EXPERIMENT)
            /* Native: launched fighters outlive the Carrier for 15-45
             * source ticks; hangar children die with it. */
            if (u[i].alive && sc64_p7_carrier_child_state(&u[i]) ==
                    SC64_P7_INTERCEPTOR_OUTSIDE) {
                race_p7_interceptor_flight_detach(i);
                continue;
            }
#endif
            sc64_p7_carrier_child_remove(i, clear_projectile_refs);
        }
    u[carrier].carry = 0;
}

/* Scripts may remove a Carrier after the source-order tick. Run at the
 * completed-update boundary so its hidden children cannot remain orphaned. */
static void race_p7_carrier_training_end_frame(void)
{
    if (!race_protoss_services_active()) return;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (!u[i].alive && u[i].type == SC64_CARRIER_TYPE &&
            RACE_PROTOSS_RAW(&u[i]) == SC64_P7_CARRIER_RAW)
            race_p7_carrier_training_parent_died(
                i, race_p7_projectile_clear_references);
    for (int i = 0; i < MAX_UNITS; ++i) {
        Unit *child = &u[i];
        unsigned state;
        if (child->type != SC64_INTERCEPTOR_TYPE ||
            RACE_PROTOSS_RAW(child) != SC64_P7_INTERCEPTOR_RAW) continue;
        state = sc64_p7_carrier_child_state(child);
#if defined(SC64_P7_INTERCEPTOR_FLIGHT_ENABLED) || defined(SC64_SHARED_CARRIER_EXPERIMENT)
        if (state == SC64_P7_INTERCEPTOR_OUTSIDE && child->alive &&
            (child->build < 0 || child->build >= MAX_UNITS ||
             !sc64_p7_carrier_parent_valid(child->build) ||
             u[child->build].owner != child->owner ||
             u[child->build].raw_owner != child->raw_owner)) {
            race_p7_interceptor_flight_detach(i);
            continue;
        }
#endif
        if (state != SC64_P7_INTERCEPTOR_TRAINING &&
            state != SC64_P7_INTERCEPTOR_INSIDE) continue;
        if (!child->alive || child->build < 0 || child->build >= MAX_UNITS ||
            !sc64_p7_carrier_parent_valid(child->build) ||
            u[child->build].owner != child->owner ||
            u[child->build].raw_owner != child->raw_owner)
            sc64_p7_carrier_child_remove(i,
                                         race_p7_projectile_clear_references);
    }
}

/* Game hooks exclude TRAINING/INSIDE rows from ordinary visibility, movement,
 * selection, collision and target acquisition. No launch, outside flight or
 * combat policy is provided here. */
#endif /* SC64_P7_CARRIER_TRAINING_ENABLED */
#endif /* SC64_P7_CARRIER_TRAINING_H */
