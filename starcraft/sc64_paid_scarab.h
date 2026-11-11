/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_PAID_SCARAB_H
#define SC64_PAID_SCARAB_H

/* Paid Scarab ammunition for the Reaver (raw 83), enabled with
 * SC64_PAID_SCARAB in the Protoss P10 product vector.
 *
 *   Stock. A Reaver holds Scarabs as a count in its own `carry` and
 *   `work_timer` fields, which the Reaver does not otherwise use. Nothing
 *   waits in the actor pool: only a Scarab in flight is a Unit row, so the
 *   save record keeps its size and the 240-actor cap is unchanged.
 *
 *   carry bits 0-3   Scarabs held (0..10)
 *   carry bits 4-6   Scarabs paid for and not yet built (0..5)
 *   carry bits 7-12  launch cooldown, source ticks (0..60)
 *   carry bit 13     the state has been established (see "Starting stock")
 *   work_timer       source ticks of the front order already built (0..105)
 *
 *   Capacity is 5. The rule that raises it to 10 is sc64_psc_capacity();
 *   no Reaver Capacity research exists in the product, so it returns 5. The
 *   fields already hold 10.
 *
 *   Buying. One Scarab at a time, for the rule table's price (U_SCARAB
 *   minerals), taken from the owner's own bank when ordered. Each takes the
 *   rule table's build time (build_frames, in source ticks) after the one
 *   before it. A Reaver with none held cannot attack. There is no cancel
 *   command, as for the Carrier's Interceptors.
 *
 *   Attack. A Reaver in range of a ground target, with a Scarab held, no
 *   Scarab of its own still flying and its cooldown expired, launches one.
 *
 *   Flight. The Scarab is a U_SCARAB row (hidden from targeting, vision and
 *   collision; drawn). It waits three source ticks, then moves 16 source
 *   pixels (8 runtime pixels) each source tick toward the target, and
 *   explodes within 10 source pixels of the target's bounds. A target that
 *   dies (or whose slot is reused) is lost: the Scarab flies on to the last
 *   position and explodes there. A route blocked by terrain stalls it; one
 *   that stays stalled until its 90-tick lifetime ends is lost without
 *   damage. If the Reaver dies the Scarab goes on, detached.
 *
 *   Splash. On arrival, every hostile ground actor whose bounds lie within
 *   60 source pixels takes the rule table's 100 damage (Scarab weapon 82),
 *   halved beyond 20 and quartered beyond 40; burrowed actors only inside
 *   20. Normal damage: shields, then armor.
 *
 *   Starting stock. A Reaver made by production starts empty. A Reaver that
 *   a mission places, reinforces or rescues (any owner, including a campaign
 *   enemy) and a Reaver from a save written before this service existed
 *   starts with a full load, the first time the service looks at it.
 *   Campaign enemies do not buy more: their money is a scripted source bank,
 *   not an engine bank. Computer opponents in custom games buy through the
 *   same command as the human, from their own bank.
 *
 *   Save. Reaver state and the flying Scarab use fields the typed unit row
 *   already stores. RaceSave does not grow. A save from before this service
 *   loads (its Reavers carry zero in both fields, which means "not yet
 *   established"). */

#ifdef SC64_PAID_SCARAB
#if !defined(SC64_P7_CARRIER_TRAINING_ENABLED) || !defined(SC64_SIEGE_SPLASH) || \
    !defined(SC64_RACES)
#error "Paid Scarabs need the Protoss services and the source bounds table"
#endif

enum {
    SC64_PSC_QUEUE_LIMIT = 5,
    SC64_PSC_CAPACITY_BASE = 5,
    SC64_PSC_CAPACITY_UPGRADED = 10,
    SC64_PSC_COOLDOWN = 60,        /* source ticks between launches */
    SC64_PSC_LIFETIME = 90,        /* source ticks a Scarab may fly */
    SC64_PSC_LAUNCH_TICKS = 3,     /* ticks before it starts to move */
    SC64_PSC_STEP = 8,             /* runtime pixels per source tick */
    SC64_PSC_ARRIVE = 10,          /* source pixels from the target bounds */
    SC64_PSC_RAW = 85,
    SC64_PSC_PARENT_RAW = 83,
    SC64_PSC_INIT = 1 << 13,
    SC64_PSC_CARRY_MASK = (1 << 14) - 1
};

typedef struct { unsigned stock, queued, cooldown, progress; int init; } Sc64PscState;

/* The one place the capacity rule lives. */
static inline unsigned sc64_psc_capacity(int owner)
{ (void)owner; return SC64_PSC_CAPACITY_BASE; }

static inline unsigned sc64_psc_price(void)
{ return sc64_race_unit_rule(U_SCARAB)->minerals; }
static inline unsigned sc64_psc_build_ticks(void)
{ return sc64_race_unit_rule(U_SCARAB)->build_frames; }

static inline int sc64_psc_decode(int carry, int work_timer, unsigned capacity,
                                  Sc64PscState *s)
{
    unsigned c = (unsigned)carry;
    if (carry < 0 || c > SC64_PSC_CARRY_MASK || work_timer < 0) return 0;
    s->stock = c & 15u; s->queued = (c >> 4) & 7u;
    s->cooldown = (c >> 7) & 63u; s->init = !!(c & SC64_PSC_INIT);
    s->progress = (unsigned)work_timer;
    if (!s->init) return !carry && !work_timer;
    return s->stock + s->queued <= capacity && s->queued <= SC64_PSC_QUEUE_LIMIT &&
           s->cooldown <= SC64_PSC_COOLDOWN &&
           (unsigned)work_timer <= sc64_psc_build_ticks() &&
           (s->queued || !work_timer);
}

static inline void sc64_psc_store(Unit *q, const Sc64PscState *s)
{
    q->carry = (int)(s->stock | (s->queued << 4) | (s->cooldown << 7) | SC64_PSC_INIT);
    q->work_timer = (int)s->progress;
}

static int race_paid_scarab_parent(const Unit *q)
{ return q->alive && q->type == U_REAVER && (q->raw_type == -1 || q->raw_type == SC64_PSC_PARENT_RAW); }

/* Read a Reaver's state; a row that was never established is given its full
 * load here (a placed, reinforced, rescued or pre-service Reaver). */
static int sc64_psc_load(Unit *q, Sc64PscState *s)
{
    unsigned capacity = sc64_psc_capacity(q->owner);
    if (!race_paid_scarab_parent(q) || !sc64_psc_decode(q->carry, q->work_timer, capacity, s))
        return 0;
    if (!s->init) {
        s->init = 1; s->stock = capacity; s->queued = s->cooldown = s->progress = 0;
        sc64_psc_store(q, s);
    }
    return 1;
}

/* Production hands over an empty Reaver. */
static void race_paid_scarab_trained(int slot)
{
    if (slot >= 0 && slot < MAX_UNITS && u[slot].type == U_REAVER) {
        u[slot].carry = SC64_PSC_INIT; u[slot].work_timer = 0;
    }
}

/* sc64_psc_hint (declared in game.c) is the cheap "something to tick" hint:
 * set by an order, a launch and every save candidate; cleared by the first
 * source tick that finds no Scarab in flight and no Reaver with work or a
 * cooldown pending. Idle worlds pay one test per update. */

static int sc64_psc_flying_of(int parent)
{
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].type == U_SCARAB && u[i].build == parent) return i;
    return -1;
}

/* ---- buying ---- */

static int race_paid_scarab_can_queue(int slot)
{
    Sc64PscState s;
    Unit *q;
    if (slot < 0 || slot >= MAX_UNITS) return 0;
    q = &u[slot];
    if (!sc64_psc_load(q, &s) || SC64_TRANSPORT_LOADED(q)) return 0;
    return s.queued < SC64_PSC_QUEUE_LIMIT &&
           s.stock + s.queued < sc64_psc_capacity(q->owner) &&
           race_owner_minerals(q->owner) >= (int)sc64_psc_price();
}

/* Any owner's order; the price comes from that owner's bank. */
static int race_paid_scarab_queue_owner(int slot)
{
    Sc64PscState s;
    if (!race_paid_scarab_can_queue(slot) || !sc64_psc_load(&u[slot], &s)) return 0;
    race_add_owner_resources(u[slot].owner, -(int)sc64_psc_price(), 0);
    ++s.queued;
    sc64_psc_store(&u[slot], &s);
    sc64_psc_hint = 1;
    return 1;
}

static int race_paid_scarab_queue_selected(void)
{
    int queued = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].selected && u[i].owner == SIDE &&
            u[i].type == U_REAVER && race_paid_scarab_queue_owner(i)) ++queued;
    return queued;
}

static int race_paid_scarab_menu_available(void)
{
    if (selected_building >= 0) return 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].selected && u[i].owner == SIDE &&
            u[i].type == U_REAVER && !SC64_TRANSPORT_LOADED(&u[i])) return 1;
    return 0;
}

static int race_paid_scarab_stock(int slot)
{
    Sc64PscState s;
    return slot >= 0 && slot < MAX_UNITS && sc64_psc_load(&u[slot], &s) ? (int)s.stock : -1;
}
static int race_paid_scarab_queued(int slot)
{
    Sc64PscState s;
    return slot >= 0 && slot < MAX_UNITS && sc64_psc_load(&u[slot], &s) ? (int)s.queued : -1;
}

/* ---- source tick: production and cooldown, once per source tick ---- */

/* Returns whether the Reaver still has work or a cooldown pending. */
static int sc64_psc_parent_tick(int slot)
{
    Sc64PscState s;
    Unit *q = &u[slot];
    if (!sc64_psc_load(q, &s)) return 0;
    if (s.cooldown) --s.cooldown;
    if (s.queued && ++s.progress >= sc64_psc_build_ticks()) {
        ++s.stock; --s.queued; s.progress = 0;
    }
    sc64_psc_store(q, &s);
    return s.queued || s.cooldown;
}

/* ---- launch ---- */

static void race_paid_scarab_remove(int slot)
{
    Unit *q = &u[slot];
    race_p7_projectile_clear_references(slot);
    q->build = -1; q->target = -1; q->carry = 0; q->work_timer = 0; q->flags = 0;
    q->alive = 0;
}

/* Called by tick_combat once the Reaver is in reach of `target` with its
 * generic weapon ready: launch a Scarab if it may. Out of reach, the generic
 * chase moves it; with none held, nothing happens. */
static void race_paid_scarab_launch(Unit *q, int target)
{
    Sc64PscState s;
    int slot = (int)(q - u), child;
    if (!race_paid_scarab_parent(q) || target < 0 || target >= MAX_UNITS + MAX_BUILDINGS ||
        SC64_TRANSPORT_LOADED(q) || !sc64_psc_load(q, &s)) return;
    if (!s.stock || s.cooldown) return;
    if (target < MAX_UNITS && sc64_race_unit_flying(u[target].type)) return;
    if (sc64_psc_hint && sc64_psc_flying_of(slot) >= 0) return;
    child = spawn_unit(q->owner, U_SCARAB, q->x, q->y);
    if (child < 0) return; /* full actor pool: the Scarab stays in the Reaver */
    u[child].raw_type = SC64_PSC_RAW; u[child].raw_owner = q->raw_owner;
    u[child].build = slot; u[child].target = target;
    u[child].tx = target_x(target); u[child].ty = target_y(target);
    u[child].work_timer = SC64_PSC_LIFETIME; u[child].carry = 0;
    u[child].flags = 16; /* the source invincibility bit: no splash touches it */
    u[child].order = ORDER_IDLE; u[child].selected = 0; u[child].cd = 0;
#ifdef SC64_UNIT_FACING
    SC64_FACING_FIRE(q, target);
#endif
    --s.stock; s.cooldown = SC64_PSC_COOLDOWN;
    sc64_psc_store(q, &s);
    sc64_psc_hint = 1;
}

/* ---- impact ---- */

static int sc64_psc_resolve(int target, unsigned packet_q8, int *shield_loss)
{
    unsigned damage = packet_q8, armor_q8;
    int shields, armor, shield_contact = 0;
    *shield_loss = 0;
    if (damage < 128u) damage = 128u;
    if (target < MAX_UNITS) {
        shields = u[target].shield;
        armor = u[target].armor + sc64_unit_armor_upgrade_level(&u[target]);
    } else {
        shields = b[target - MAX_UNITS].shield;
        armor = sc64_race_building_armor(b[target - MAX_UNITS].type);
    }
    if (shields > 0) {
        unsigned absorbed;
#ifdef SC64_EXPERIMENTAL_RESEARCH_JOBS
        unsigned sa = (unsigned)sc64_pr_stage_shield_armor(target) * 256u;
        if (sa) damage = damage > sa + 128u ? damage - sa : 128u;
#endif
        absorbed = damage < (unsigned)shields * 256u ? damage : (unsigned)shields * 256u;
        shield_contact = absorbed != 0;
        *shield_loss = (int)(absorbed / 256u); damage -= absorbed;
    }
    armor_q8 = (unsigned)(armor < 0 ? 0 : armor) * 256u;
    damage = damage > armor_q8 ? damage - armor_q8 : 0;
    if (!shield_contact && damage < 128u) damage = 128u;
    return (int)(damage / 256u);
}

/* Splash around runtime pixel (px,py) for a Scarab of `owner`. */
static void race_paid_scarab_explode(int px, int py, int owner, int primary)
{
    int base = (int)sc64_race_unit_damage_vs(U_REAVER, 0);
    for (int target = 0; target < MAX_UNITS + MAX_BUILDINGS; ++target) {
        int raw, distance, tier, shield_loss = 0, hp_damage, tgt_owner, x, y;
        unsigned packet;
        int flags = SC64_HIT_PRECOMPUTED_HP | SC64_HIT_SKIP_HALLUCINATION_SCALE;
        if (!sc64_siege_target_alive(target) || sc64_siege_hidden_or_immune(target)) continue;
        if (target < MAX_UNITS) {
            if (SC64_TRANSPORT_LOADED(&u[target]) || SC64_SCARAB_HIDDEN(&u[target]) ||
                sc64_race_unit_flying(u[target].type)) continue;
            raw = u[target].raw_type >= 0 ? u[target].raw_type : sc64_race_raw_unit(u[target].type);
            x = u[target].x; y = u[target].y; tgt_owner = u[target].owner;
        } else {
            raw = b[target - MAX_UNITS].raw_type >= 0 ? b[target - MAX_UNITS].raw_type :
                  sc64_race_raw_building(b[target - MAX_UNITS].type);
            x = b[target - MAX_UNITS].x; y = b[target - MAX_UNITS].y;
            tgt_owner = b[target - MAX_UNITS].owner;
        }
        if (target != primary && !owners_hostile(owner, tgt_owner)) continue;
        distance = sc64_siege_distance(raw, x, y, px, py);
        if (distance < 0 || distance > 60) continue;
        tier = distance <= 20 ? 1 : distance <= 40 ? 2 : 3;
        if (target < MAX_UNITS && sc64_siege_burrowed(target)) {
            if (tier != 1) continue;
            flags |= SC64_HIT_ALLOW_SPLASH_BURROW;
        }
        packet = (unsigned)base * 256u / (tier == 1 ? 1u : tier == 2 ? 2u : 4u);
#ifdef SC64_EXPERIMENTAL_RESEARCH_Q8_COMBAT
        hit_target_impl_ex(target, (int)packet, U_REAVER, owner, SC64_STOCK_DMG_NORMAL,
            (flags & ~(SC64_HIT_PRECOMPUTED_HP | SC64_HIT_SKIP_HALLUCINATION_SCALE)) | SC64_HIT_PACKET_Q8);
#else
        hp_damage = sc64_psc_resolve(target, packet, &shield_loss);
        if (target < MAX_UNITS) u[target].shield -= shield_loss;
        else b[target - MAX_UNITS].shield -= shield_loss;
        hit_target_impl_ex(target, hp_damage, U_REAVER, owner, -1, flags);
#endif
    }
}

/* ---- flight, once per source tick ---- */

/* True if the runtime pixel is not walkable. Cells inside the inflated
 * footprint of the Scarab's own building target are open to it. */
static int sc64_psc_blocked(int x, int y, int target)
{
    if (x < 0 || y < 0 || x >= WORLD_W || y >= WORLD_H) return 1;
    if (is_traversable(x / 16, y / 16, -1)) return 0;
    if (target >= MAX_UNITS) {
        const Building *v = &b[target - MAX_UNITS];
        int raw = v->raw_type >= 0 ? v->raw_type : sc64_race_raw_building(v->type);
        int d = sc64_siege_distance(raw, v->x, v->y, x, y);
        if (d >= 0 && d <= 40) return 0;
    }
    return 1;
}

static int sc64_psc_target_ok(const Unit *q, int target)
{
    if (target < 0 || target >= MAX_UNITS + MAX_BUILDINGS || !target_alive(target)) return 0;
    if (target < MAX_UNITS)
        return owners_hostile(q->owner, u[target].owner) && !sc64_race_unit_flying(u[target].type);
    return owners_hostile(q->owner, b[target - MAX_UNITS].owner);
}

static void sc64_psc_flight_tick(int slot)
{
    Unit *q = &u[slot];
    int life = q->work_timer, target = q->target, raw = -1, aimx, aimy, dx, dy, len, nx, ny, arrived;
    if (!q->alive || q->type != U_SCARAB) return;
    if (life <= 1 || life > SC64_PSC_LIFETIME) { race_paid_scarab_remove(slot); return; }
    q->work_timer = --life;
    if (target >= 0) {
        if (sc64_psc_target_ok(q, target)) { q->tx = target_x(target); q->ty = target_y(target); }
        else q->target = target = -1;
    }
    if (SC64_PSC_LIFETIME - life < SC64_PSC_LAUNCH_TICKS) return;
    if (target >= 0)
        raw = target < MAX_UNITS ? (u[target].raw_type >= 0 ? u[target].raw_type :
                  sc64_race_raw_unit(u[target].type)) :
              (b[target - MAX_UNITS].raw_type >= 0 ? b[target - MAX_UNITS].raw_type :
                  sc64_race_raw_building(b[target - MAX_UNITS].type));
    if (target >= 0) {
        int d = sc64_siege_distance(raw, q->tx, q->ty, q->x, q->y);
        arrived = d >= 0 && d <= SC64_PSC_ARRIVE;
    } else arrived = q->x == q->tx && q->y == q->ty;
    if (arrived) {
        int owner = q->owner, px = q->x, py = q->y;
        race_paid_scarab_remove(slot);
        race_paid_scarab_explode(px, py, owner, target);
        return;
    }
    aimx = q->tx; aimy = q->ty;
    for (int attempt = 0; attempt < 2; ++attempt) {
        dx = aimx - q->x; dy = aimy - q->y;
        len = (int)sc64_siege_xy_length(dx, dy);
        if (len <= SC64_PSC_STEP) { nx = aimx; ny = aimy; }
        else { nx = q->x + dx * SC64_PSC_STEP / len; ny = q->y + dy * SC64_PSC_STEP / len; }
        if (!sc64_psc_blocked(nx, ny, target) &&
            !sc64_psc_blocked((q->x + nx) / 2, (q->y + ny) / 2, target)) {
            q->x = nx; q->y = ny;
            return;
        }
        /* Blocked straight ahead: take the next tile of the walkable route. */
        if (attempt || !path_next_tile(q->x, q->y, aimx, aimy, &aimx, &aimy)) return;
    }
}

/* Once per source tick, while the hint is set. */
static void race_paid_scarab_world_tick(void)
{
    int busy = 0;
    for (int i = 0; i < MAX_UNITS; ++i) {
        if (!u[i].alive) continue;
        if (u[i].type == U_SCARAB) { sc64_psc_flight_tick(i); busy |= u[i].alive && u[i].type == U_SCARAB; }
        else if (u[i].type == U_REAVER) busy |= sc64_psc_parent_tick(i);
    }
    if (!busy) sc64_psc_hint = 0;
}

/* ---- slot lifetime and the end of an update ---- */

/* A target slot is dying or being reused: any Scarab aimed at it flies on to
 * the last position. */
static void race_scarab_probe_target_invalidate(int ref)
{
    if (!sc64_psc_hint) return;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].type == U_SCARAB && u[i].target == ref) u[i].target = -1;
}

/* A unit slot dies or is reused. A dying Reaver sheds its state; Scarabs it
 * launched keep flying, detached. */
static void race_paid_scarab_clear_slot(int slot)
{
    if (slot < 0 || slot >= MAX_UNITS) return;
    if (u[slot].type == U_REAVER) {
        u[slot].carry = 0; u[slot].work_timer = 0;
        if (sc64_psc_hint)
            for (int i = 0; i < MAX_UNITS; ++i)
                if (u[i].alive && u[i].type == U_SCARAB && u[i].build == slot) u[i].build = -1;
    }
}

/* A scripted removal can take a Reaver without the death hook: detach
 * what it launched. */
static void race_paid_scarab_end_frame(void)
{
    if (!sc64_psc_hint) return;
    for (int i = 0; i < MAX_UNITS; ++i) {
        Unit *q = &u[i];
        if (q->alive && q->type == U_SCARAB && q->build >= 0 && !race_paid_scarab_parent(&u[q->build]))
            q->build = -1;
    }
}

/* ---- save candidates ---- */

static int race_paid_scarab_candidate_valid(const Unit *actors, int count)
{
    u8 claimed[MAX_UNITS];
    if (!actors || count != MAX_UNITS) return 0;
    sc64_psc_hint = 1; /* conservative: a loaded world may hold a flight */
    for (int i = 0; i < MAX_UNITS; ++i) claimed[i] = 0;
    for (int i = 0; i < count; ++i) {
        const Unit *q = &actors[i];
        Sc64PscState s;
        if ((q->type == U_SCARAB) != (q->raw_type == SC64_PSC_RAW)) return 0;
        if (q->type == U_REAVER) {
            if (!q->alive) { if (q->carry || q->work_timer) return 0; continue; }
            if (!sc64_psc_decode(q->carry, q->work_timer, sc64_psc_capacity(q->owner), &s)) return 0;
            continue;
        }
        if (q->type != U_SCARAB) continue;
        if (!q->alive) {
            if (q->build != -1 || q->carry || q->work_timer) return 0;
            continue;
        }
        if (q->work_timer < 1 || q->work_timer > SC64_PSC_LIFETIME || q->carry != 0 ||
            q->cd != 0 || q->selected || q->flags != 16 || q->order != ORDER_IDLE ||
            q->hp != 20 || q->shield < 0 || q->shield > 10 ||
            q->target < -1 || q->target >= MAX_UNITS + MAX_BUILDINGS ||
            q->build < -1 || q->build >= MAX_UNITS) return 0;
        if (q->build >= 0) {
            if (!race_paid_scarab_parent(&actors[q->build]) || claimed[q->build]) return 0;
            claimed[q->build] = 1;
        }
    }
    return 1;
}

static int race_paid_scarab_live_world_valid(void)
{ return race_paid_scarab_candidate_valid(u, MAX_UNITS); }

/* The shared contextual command: "BUILD INTERCEPTOR" for a Carrier-class
 * selection, "BUILD SCARAB" when only Reavers are selected. */
static const char *race_carrier_menu_label(void)
{ return race_p7_carrier_training_selected() ? "BUILD INTERCEPTOR" : "BUILD SCARAB"; }

#endif /* SC64_PAID_SCARAB */
#endif /* SC64_PAID_SCARAB_H */
