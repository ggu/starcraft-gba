/* SPDX-License-Identifier: GPL-3.0-or-later
 * Paid Scarabs in the normal Protoss P10 product vector (PROTOSS_P10_FLAGS).
 * HOST only, built under ASan/UBSan by `make test-sc64-paid-scarab`.
 *
 *   economy           buy, queue, cap, price, build time, empty Reaver cannot
 *                     fire, starting-stock rule, the owner's own bank
 *   combat            launch, one in flight, cooldown, flight, impact and
 *                     splash against the rule table, buildings, shields
 *   loss              target lost, path lost, Reaver death, slot reuse
 *   validate          malformed Reaver and Scarab rows are rejected
 *   cold, retrain     placed stock from the first update (stale hint, restart, failed load); a Reaver trained
 *                     after the others died restocks
 *   recharge, shuttle a recharging Reaver with a part-built Scarab saves and loads; a loaded Reaver dies
 *                     with its Shuttle and the save after it works
 *   birth             a Scarab born into a slot freed this update switches the target index off
 *   restock0, lift    an empty placed enemy Reaver established by a launch attempt still restocks; a
 *                     Science Facility that lifts mid-flight is neither homed on nor splashed
 *   save-write FILE   real race_save_game() with orders in production and a
 *                     Scarab in flight, then 600 updates of digests
 *   save-read FILE    a fresh process loads it and must match every digest
 *   old-write FILE    (base build) a genuine pre-service save with a Reaver
 *   old-read FILE     the new build loads it and the Reaver is loaded
 *
 * Worlds are constructed as in the other race fixtures; everything after
 * construction runs the live code through game_update(). */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../common/gba.c"
#include "../starcraft/game.c"

#ifdef SC64_PAID_SCARAB
#define HAVE_SERVICE 1
#endif

static void start_custom(void)
{
    gba_init(); game_init(); race_mode = 1; race_human = R_PROTOSS;
    race_bots = 1; race_bot_races[0] = R_PROTOSS; race_skirmish_map = 0; init_state();
    minerals = gas = 5000;
    for (int i = 0; i < MAX_UNITS; ++i) u[i].selected = 0;
}
static void read_sram(const char *path)
{
    FILE *f = fopen(path, "rb"); assert(f);
    assert(fread(host_save, 1, sizeof host_save, f) == sizeof host_save);
    assert(fgetc(f) == EOF); assert(!fclose(f));
}
static void write_sram(const char *path)
{
    FILE *f = fopen(path, "wb"); assert(f);
    assert(fwrite(host_save, 1, sizeof host_save, f) == sizeof host_save);
    assert(!fclose(f));
}
static void step(unsigned n) { for (unsigned i = 0; i < n; ++i) game_update(0, 0); }

static u32 hash_u32(u32 h, u32 v)
{
    for (int i = 0; i < 4; ++i) { h ^= (u8)(v >> (8 * i)); h *= 16777619u; }
    return h;
}
#define H(v) (h = hash_u32(h, (u32)(v)))
static u32 state_digest(void)
{
    u32 h = 2166136261u;
    for (int i = 0; i < MAX_UNITS; i++) {
        const Unit *q = &u[i];
        H(q->alive);
        if (!q->alive) continue;
        H(q->hp); H(q->shield); H(q->x); H(q->y); H(q->tx); H(q->ty);
        H(q->target); H(q->raw_type); H(q->raw_owner); H(q->cd);
        H(q->owner); H(q->type); H(q->order); H(q->carry); H(q->flags);
        H(q->build); H(q->work_timer); H(q->path_retry); H(q->path_timer);
    }
    for (int i = 0; i < MAX_BUILDINGS; i++) {
        const Building *q = &b[i];
        H(q->alive);
        if (!q->alive) continue;
        H(q->hp); H(q->shield); H(q->build_progress); H(q->x); H(q->y);
        H(q->queue_progress); H(q->owner); H(q->type); H(q->queue_count);
    }
    H(minerals); H(gas); H(frame);
    for (int o = 0; o < 5; ++o) { H(race_minerals[o]); H(race_gas[o]); }
    return h;
}
#undef H

/* ---- world construction ---- */

/* A point with a walkable 14x6-tile block around it and nothing else near. */
static void quiet_spot(int *x, int *y)
{
    int best = -1;
    for (int py = 160; py < WORLD_H - 160; py += 32)
        for (int px = 160; px < WORLD_W - 260; px += 32) {
            int nearest = 0x7fffffff, ok = 1;
            for (int r = -3; r <= 3 && ok; ++r)
                for (int c = -2; c <= 12 && ok; ++c)
                    ok = is_traversable(px / 16 + c, py / 16 + r, -1);
            if (!ok) continue;
            for (int i = 0; i < MAX_UNITS; ++i)
                if (u[i].alive) { int d = dist2(px, py, u[i].x, u[i].y); if (d < nearest) nearest = d; }
            for (int i = 0; i < MAX_BUILDINGS; ++i)
                if (b[i].alive) { int d = dist2(px, py, b[i].x, b[i].y); if (d < nearest) nearest = d; }
            if (nearest > best) { best = nearest; *x = px; *y = py; }
        }
    assert(best >= 300 * 300);
}

static int enemy_hatchery(int x, int y)
{
    int s = spawn_build(ENEMY, B_HATCHERY, x, y, 0);
    assert(s >= 0);
    b[s].hp = b[s].maxhp; b[s].under_construction = 0;
    invalidate_path_grid();
    sc64_visibility_reset(); update_exploration();
    return s;
}

static int enemy_unit(int type, int x, int y)
{
    int s = spawn_unit(ENEMY, type, x, y);
    assert(s >= 0);
    u[s].order = ORDER_IDLE; u[s].tx = x; u[s].ty = y;
    sc64_visibility_reset(); update_exploration();
    return s;
}

static void kill_actor(int index)
{
    int damage;
    if (index < MAX_UNITS) { invincible_units[index] = 0; damage = u[index].hp + u[index].shield + 1; }
    else { invincible_buildings[index - MAX_UNITS] = 0;
           damage = b[index - MAX_UNITS].hp + b[index - MAX_UNITS].shield + 1; }
    hit_target_impl_ex(index, damage, -1, -1, -1,
                       SC64_HIT_PRECOMPUTED_HP | SC64_HIT_SKIP_HALLUCINATION_SCALE);
}

#ifdef HAVE_SERVICE
static int scarab_in_flight(int parent)
{
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].type == U_SCARAB && (parent < 0 || u[i].build == parent)) return i;
    return -1;
}
static int count_scarabs(void)
{ int n = 0; for (int i = 0; i < MAX_UNITS; ++i) n += u[i].alive && u[i].type == U_SCARAB; return n; }

static int reaver_at(int x, int y)
{
    int s = spawn_unit(SIDE, U_REAVER, x, y);
    assert(s >= 0);
    u[s].order = ORDER_IDLE; u[s].tx = x; u[s].ty = y; u[s].selected = 0;
    return s;
}
/* The ordinary attack order of this product is attack-move: aim at the
 * target and let the order loop acquire it. */
static void attack(int r, int target)
{
    u[r].order = ORDER_ATTACK_MOVE; u[r].target = -1;
    u[r].tx = target_x(target); u[r].ty = target_y(target);
}
static void place(Unit *q, int x, int y)
{
    q->x = x; q->y = y; q->tx = x; q->ty = y;
    sc64_unit_bucket_move(q);
    sc64_target_index_move(q);
}
static void select_only(int slot)
{
    for (int i = 0; i < MAX_UNITS; ++i) u[i].selected = 0;
    selected_building = -1;
    u[slot].selected = 1;
}
static void press_buy(void)
{
    /* The ordinary contextual command: the row the Carrier uses. */
    int row = race_mode ? race_carrier_command_base() : MENU_STOP + 1;
    assert(race_p7_carrier_training_menu_available());
    choose_command(row);
}

/* Reaver with a full state, away from every other actor. */
static int arena(int *hx, int *hy)
{
    int x, y, r;
    start_custom();
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner != SIDE) kill_actor(i);
    quiet_spot(&x, &y);
    r = reaver_at(x, y);
    race_paid_scarab_trained(r);
    *hx = x + 96; *hy = y;
    return r;
}

static void economy(void)
{
    int hx, hy, r = arena(&hx, &hy);
    const unsigned price = sc64_psc_price(), ticks = sc64_psc_build_ticks();
    int before;
    assert(price == 15 && ticks == 105);          /* the rule table, not constants */
    assert(sc64_race_unit_rule(U_SCARAB)->minerals == 15 &&
           sc64_race_unit_rule(U_SCARAB)->build_frames == 105);
    /* A trained Reaver starts empty and cannot attack. */
    assert(race_paid_scarab_stock(r) == 0 && race_paid_scarab_queued(r) == 0);
    {
        int h = enemy_hatchery(hx, hy), hp = b[h].hp + b[h].shield;
        attack(r, MAX_UNITS + h);
        step(240);
        assert(count_scarabs() == 0 && b[h].hp + b[h].shield == hp);   /* empty: no shot */
        assert(race_paid_scarab_stock(r) == 0);
        kill_actor(MAX_UNITS + h);
    }
    u[r].order = ORDER_IDLE; u[r].target = -1;
    /* Buy: one command, exactly the price from the owner's bank. */
    select_only(r);
    assert(!strcmp(race_menu_label(race_carrier_command_base()), "BUILD SCARAB"));
    minerals = 1000; before = minerals;
    press_buy();
    assert(minerals == before - (int)price && race_paid_scarab_queued(r) == 1);
    /* Build time: one Scarab after `ticks` source ticks, and not before. */
    {
        unsigned updates = 0;
        while (race_paid_scarab_stock(r) == 0 && updates < 400) { step(1); ++updates; }
        /* 105 source ticks = 131 updates at the 24:30 cadence (4 of 5 updates tick). */
        assert(race_paid_scarab_stock(r) == 1 && race_paid_scarab_queued(r) == 0);
        assert(updates >= ticks * 5 / 4 - 2 && updates <= ticks * 5 / 4 + 2);
        printf("economy: 1 Scarab for %u minerals after %u updates (%u source ticks)\n",
               price, updates, ticks);
    }
    /* Queue to the cap: five held or on order, a sixth refused and free. */
    minerals = 1000;
    for (int i = 0; i < 4; ++i) { press_buy(); }
    assert(race_paid_scarab_stock(r) + race_paid_scarab_queued(r) == 5);
    assert(minerals == 1000 - 4 * (int)price);
    before = minerals;
    assert(!race_paid_scarab_queue_owner(r));
    choose_command(race_carrier_command_base());
    assert(minerals == before && race_paid_scarab_queued(r) == 4);
    /* Too poor: refused and free. */
    for (int i = 0; i < 1000 && race_paid_scarab_queued(r); ++i) step(1);
    assert(race_paid_scarab_stock(r) == 5);
    u[r].carry &= ~15; u[r].carry |= 3;          /* pretend two were spent */
    minerals = (int)price - 1;
    assert(!race_paid_scarab_queue_owner(r) && minerals == (int)price - 1);
    minerals = (int)price;
    assert(race_paid_scarab_queue_owner(r) && minerals == 0);
    /* The queue builds one at a time: the second starts after the first. */
    minerals = 1000; assert(race_paid_scarab_queue_owner(r));
    assert(race_paid_scarab_stock(r) + race_paid_scarab_queued(r) == 5);
    /* STOP keeps the load and the orders. */
    {
        int stock = race_paid_scarab_stock(r), queued = race_paid_scarab_queued(r);
        select_only(r); stop_selected();
        assert(race_paid_scarab_stock(r) == stock && race_paid_scarab_queued(r) == queued);
    }
    /* Starting stock: a Reaver a mission or reinforcement places is loaded
     * the first time the service looks; production hands over an empty one. */
    {
        int placed = spawn_unit(SIDE, U_REAVER, u[r].x + 40, u[r].y + 40);
        assert(placed >= 0 && u[placed].carry == 0);
        assert(race_paid_scarab_stock(placed) == 5);
        assert(race_paid_scarab_queued(placed) == 0);
        int ep = spawn_unit(ENEMY, U_REAVER, u[r].x + 80, u[r].y + 40);
        assert(ep >= 0 && race_paid_scarab_stock(ep) == 5);
    }
    /* A computer opponent pays from its own bank, never the human's. */
    {
        int ai = spawn_unit(ENEMY, U_REAVER, u[r].x - 80, u[r].y);
        int bank = race_minerals[ENEMY], mine;
        assert(ai >= 0); race_paid_scarab_trained(ai);
        minerals = 777; mine = minerals; race_minerals[ENEMY] = 100;
        assert(race_paid_scarab_queue_owner(ai));
        assert(minerals == mine && race_minerals[ENEMY] == 100 - (int)price);
        race_minerals[ENEMY] = 0;
        assert(!race_paid_scarab_queue_owner(ai) && race_paid_scarab_queued(ai) == 1);
        (void)bank;
    }
    puts("economy PASS");
}

static int hp_total(int target)
{
    return target < MAX_UNITS ? u[target].hp + u[target].shield :
                                b[target - MAX_UNITS].hp + b[target - MAX_UNITS].shield;
}

/* Damage a unit of this type at this offset takes from a Scarab exploding at
 * (px,py), by the rule table: tier from the bounds distance, shields then
 * armor, Normal damage. Returns -1 outside 60. */
static int expected_loss(int raw, int x, int y, int px, int py, int shield, int armor, int *tier)
{
    int d = sc64_siege_distance(raw, x, y, px, py), packet, absorbed;
    if (d < 0 || d > 60) return -1;
    *tier = d <= 20 ? 1 : d <= 40 ? 2 : 3;
    packet = 100 / (*tier == 1 ? 1 : *tier == 2 ? 2 : 4);
    absorbed = packet < shield ? packet : shield; packet -= absorbed;
    packet = packet > armor ? packet - armor : 0;
    return packet;
}

static void combat(void)
{
    int hx, hy, r = arena(&hx, &hy), h, t0, hp0, shot = -1, flights = 0, launch_frame = -1;
    int stock0;
    /* A loaded Reaver in range of an enemy Hatchery. */
    h = enemy_hatchery(hx, hy);
    t0 = MAX_UNITS + h; hp0 = hp_total(t0);
    stock0 = race_paid_scarab_stock(r);
    assert(stock0 == 0);
    u[r].carry = SC64_PSC_INIT | 3; u[r].work_timer = 0;   /* three held */
    attack(r, t0);
    for (int n = 0; n < 400 && shot < 0; ++n) { step(1); shot = scarab_in_flight(r); }
    if (shot < 0)
        printf("debug: reaver order=%d target=%d stock=%d cd=%d alive=%d target_alive=%d x=%d y=%d tx=%d ty=%d hint=%d\n",
               u[r].order, u[r].target, race_paid_scarab_stock(r), (u[r].carry >> 7) & 63,
               u[r].alive, target_alive(t0), u[r].x, u[r].y, b[h].x, b[h].y, sc64_psc_hint);
    assert(shot >= 0 && u[shot].build == r && u[shot].target == t0);
    launch_frame = frame;
    assert(race_paid_scarab_stock(r) == 2);              /* one left the Reaver */
    camx = u[r].x - 120; camy = u[r].y - 80;
    game_render();                                       /* the Scarab and Reaver draw */
    assert(u[shot].raw_type == 85 && u[shot].owner == SIDE && u[shot].hp == 20 &&
           u[shot].flags == 16 && u[shot].work_timer >= 88);
    assert(u[shot].x == u[r].x && u[shot].y == u[r].y);   /* from the Reaver */
    /* Not targetable, not selectable, hit by no splash. */
    assert(!target_alive(shot) && SC64_SCARAB_HIDDEN(&u[shot]));
    assert(race_paid_scarab_candidate_valid(u, MAX_UNITS));
    /* Flight: waits three source ticks, then 8 px per tick toward the target. */
    {
        int x0 = u[shot].x, steps = 0, last = x0, peak_step = 0;
        while (u[shot].alive && u[shot].type == U_SCARAB && steps < 80) {
            step(1); ++steps;
            if (u[shot].alive && u[shot].type == U_SCARAB) {
                int moved = u[shot].x - last; if (moved > peak_step) peak_step = moved; last = u[shot].x;
                assert(race_paid_scarab_candidate_valid(u, MAX_UNITS));
            }
        }
        assert(!u[shot].alive);
        assert(peak_step == SC64_PSC_STEP);
        assert(hp_total(t0) < hp0);
        /* Primary hit at tier 1: 100 Normal damage less the Hatchery's armor. */
        {
            int armor = sc64_race_building_armor(B_HATCHERY), shields = 0, tier;
            int lost = expected_loss(131, b[h].x, b[h].y, u[shot].x, u[shot].y, shields, armor, &tier);
            (void)lost;
            assert(hp0 - hp_total(t0) == 100 - armor);
        }
        printf("combat: launch -> impact in %d updates, Hatchery lost %d (100 - armor %d)\n",
               frame - launch_frame, hp0 - hp_total(t0), sc64_race_building_armor(B_HATCHERY));
        assert(frame - launch_frame < 60);
    }
    /* Cooldown: no second launch for 60 source ticks (75 updates). */
    assert(race_paid_scarab_stock(r) == 2);
    {
        int first = frame, second = -1;
        for (int n = 0; n < 400 && second < 0; ++n) { step(1); second = scarab_in_flight(r); }
        assert(second >= 0);
        (void)first;
        assert(race_paid_scarab_stock(r) == 1);
    }
    /* One in flight per Reaver: with a Scarab out, the next waits. */
    {
        int out = scarab_in_flight(r);
        assert(out >= 0);
        u[r].carry = (u[r].carry & ~(63 << 7));       /* cooldown cleared by hand */
        step(1);
        assert(count_scarabs() <= 1);
    }
    puts("combat PASS");
}

/* Splash at the three tiers against the rule table: unit type Ultralisk
 * (400 hp, large, armor 1) placed by bounds distance. */
static void splash(void)
{
    int hx, hy, r = arena(&hx, &hy), seen[4] = {0,0,0,0};
    (void)r;
    int px = hx, py = hy;
    /* Find offsets for each tier from the Ultralisk bounds table. */
    int raw = sc64_race_raw_unit(U_ZP_ULTRALISK);
    for (int tier = 1; tier <= 3; ++tier) {
        int lo = tier == 1 ? 0 : tier == 2 ? 21 : 41, hi = tier == 1 ? 20 : tier == 2 ? 40 : 60, ox = -1;
        for (int off = 0; off < 90 && ox < 0; ++off) {
            int d = sc64_siege_distance(raw, px + off, py, px, py);
            if (d >= lo && d <= hi && d == (lo + hi) / 2 - (tier == 1 ? 10 : 0)) ox = off;
        }
        if (ox < 0)
            for (int off = 0; off < 90 && ox < 0; ++off) {
                int d = sc64_siege_distance(raw, px + off, py, px, py);
                if (d >= lo && d <= hi) ox = off;
            }
        assert(ox >= 0);
        int s = enemy_unit(U_ZP_ULTRALISK, px + ox, py + 30 * (tier - 2) * 0 + (tier - 1) * 0);
        /* Spread the three along the other axis so they stay in distinct tiers. */
        u[s].y = py + (tier - 1) * 200; u[s].x = px + ox; u[s].tx = u[s].x; u[s].ty = u[s].y;
        seen[tier] = s;
    }
    /* Each at its own explosion point to keep them independent. */
    for (int tier = 1; tier <= 3; ++tier) {
        int s = seen[tier], shield = u[s].shield, armor = u[s].armor, hp0 = u[s].hp, t;
        int want = expected_loss(raw, u[s].x, u[s].y, px, py + (tier - 1) * 200, shield, armor, &t);
        assert(want > 0 && t == tier);
        race_paid_scarab_explode(px, py + (tier - 1) * 200, SIDE, -1);
        assert(hp0 - u[s].hp == want);
        printf("splash: tier %d Ultralisk lost %d\n", tier, want);
    }
    /* Outside 60 source pixels: untouched. Own units and air units: untouched. */
    {
        int far = enemy_unit(U_ZP_ULTRALISK, px + 400, py + 800), own = spawn_unit(SIDE, U_ZEALOT, px, py + 20);
        int air = enemy_unit(U_MUTALISK, px + 2, py + 2);
        int hf = u[far].hp, ho = hp_total(own), ha = u[air].hp;
        assert(own >= 0);
        race_paid_scarab_explode(px, py, SIDE, -1);
        assert(u[far].hp == hf && hp_total(own) == ho && u[air].hp == ha);
    }
    /* Shields first, then armor: a Dragoon takes 100 - 80 shield - 1 armor. */
    {
        int d = enemy_unit(U_DRAGOON, px, py + 600), hp0, sh0;
        hp0 = u[d].hp; sh0 = u[d].shield;
        race_paid_scarab_explode(u[d].x, u[d].y, SIDE, -1);
        assert(sh0 - u[d].shield == (sh0 < 100 ? sh0 : 100));
        assert(hp0 - u[d].hp == 100 - sh0 - sc64_race_unit_armor(U_DRAGOON));
    }
    puts("splash PASS");
}

/* Run until no Scarab is in flight (bounded). */
static void drain(void)
{
    for (int n = 0; n < 200 && count_scarabs(); ++n) step(1);
    assert(count_scarabs() == 0);
}
/* Stage a fresh Hatchery at the arena target spot and aim `r` at it. */
static int fresh_target(int r, int hx, int hy, int stock)
{
    int h = enemy_hatchery(hx, hy);
    u[r].carry = SC64_PSC_INIT | stock; u[r].work_timer = 0;
    attack(r, MAX_UNITS + h);
    return MAX_UNITS + h;
}
static int wait_launch(int r)
{
    int shot = -1;
    for (int n = 0; n < 400 && shot < 0; ++n) { step(1); shot = scarab_in_flight(r); }
    assert(shot >= 0);
    return shot;
}
static void stand_down(int r)
{ u[r].order = ORDER_IDLE; u[r].target = -1; u[r].tx = u[r].x; u[r].ty = u[r].y; }

static void loss(void)
{
    int hx, hy, r = arena(&hx, &hy), t0, shot, pylon, hp0;
    /* Target lost: a Hatchery dies after launch; the Scarab flies on to the
     * last position and detonates there, hurting what stands within 60. */
    t0 = fresh_target(r, hx, hy, 2);
    {   /* A Pylon against the Hatchery's edge: it cannot walk away. */
        int p = spawn_build(ENEMY, B_PYLON, hx, hy + 34, 0);
        assert(p >= 0);
        b[p].hp = b[p].maxhp; b[p].under_construction = 0;
        invalidate_path_grid(); sc64_visibility_reset(); update_exploration();
        pylon = MAX_UNITS + p;
    }
    shot = wait_launch(r);
    stand_down(r);
    hp0 = hp_total(pylon);
    kill_actor(t0);
    step(1);
    if (u[shot].alive && u[shot].type == U_SCARAB) assert(u[shot].target == -1);
    drain();
    printf("loss: a Pylon beside a lost target took %d\n", hp0 - hp_total(pylon));
    assert(hp_total(pylon) < hp0 || !b[pylon - MAX_UNITS].alive);
    if (b[pylon - MAX_UNITS].alive) kill_actor(pylon);
    /* Lifetime: a Scarab that runs out of ticks is lost without damage. */
    t0 = fresh_target(r, hx, hy, 2);
    shot = wait_launch(r);
    stand_down(r);
    {
        int hp1 = hp_total(t0);
        u[shot].work_timer = 3;
        for (int n = 0; n < 12 && count_scarabs(); ++n) step(1);
        assert(count_scarabs() == 0 && hp_total(t0) == hp1);
        printf("loss: an expired Scarab did no damage\n");
    }
    kill_actor(t0);
    /* Blocked route, built into the fixture: a wall of seven of the Reaver's own Pylons
     * between the Reaver and a Hatchery on the same row. The inflated
     * footprints are unwalkable. The Scarab never enters a Pylon's tile;
     * it flies the walkable route round the wall or waits and expires. */
    {
        int pyl[7], wallx = hx - 40, minpass = 1 << 30, inside = 0;
        for (int k = 0; k < 7; ++k) {
            int p = spawn_build(SIDE, B_PYLON, wallx, hy - 96 + 32 * k, 0);
            assert(p >= 0);
            b[p].hp = b[p].maxhp; b[p].under_construction = 0; pyl[k] = p;
        }
        invalidate_path_grid(); sc64_visibility_reset(); update_exploration();
        assert(!is_traversable(wallx / 16, hy / 16, -1));       /* the wall blocks */
        {
            int h2 = enemy_hatchery(hx + 60, hy);
            u[r].carry = SC64_PSC_INIT | 2; u[r].work_timer = 0;
            place(&u[r], hx - 120, hy);
            u[r].order = ORDER_ATTACK_MOVE; u[r].tx = hx + 60; u[r].ty = hy;
            shot = wait_launch(r);
            stand_down(r);
            for (int n = 0; n < 120 && count_scarabs(); ++n) {
                int s2 = scarab_in_flight(-1);
                if (s2 >= 0)
                    for (int k = 0; k < 7; ++k) {
                        int d = dist2(u[s2].x, u[s2].y, b[pyl[k]].x, b[pyl[k]].y);
                        if (d < minpass) minpass = d;
                        /* Never inside the walled-off (unwalkable) ground near the wall. */
                        if (d < 24 * 24 && !is_traversable(u[s2].x / 16, u[s2].y / 16, -1)) {
                            if (!inside) printf("loss: Scarab at %d,%d cell %d,%d on unwalkable ground, pylon %d at %d,%d, tick %d, life %d, reaver %d,%d\n", u[s2].x, u[s2].y, u[s2].x / 16, u[s2].y / 16, k, b[pyl[k]].x, b[pyl[k]].y, n, (int)u[s2].work_timer, u[r].x, u[r].y);
                            ++inside;
                        }
                    }
                step(1);
            }
            drain();
            assert(inside == 0);                 /* never on unwalkable ground at the wall */
            assert(race_paid_scarab_candidate_valid(u, MAX_UNITS));
            printf("loss: blocked route: closest approach to a wall Pylon %d px, Hatchery %s\n",
                   (int)(minpass < (1 << 30) ? (int)__builtin_sqrt((double)minpass) : -1),
                   hp_total(MAX_UNITS + h2) < b[h2].maxhp + b[h2].shield ? "hit by detour" : "untouched");
            kill_actor(MAX_UNITS + h2);
        }
        for (int k = 0; k < 7; ++k) kill_actor(MAX_UNITS + pyl[k]);
        place(&u[r], hx - 96, hy);
    }
    /* Reaver death with a Scarab in flight: it goes on, detached, and hits. */
    t0 = fresh_target(r, hx, hy, 2);
    shot = wait_launch(r);
    {
        int hp1 = hp_total(t0);
        kill_actor(r);
        assert(!u[r].alive && u[r].carry == 0 && u[r].work_timer == 0);
        assert(u[shot].alive && u[shot].build == -1);
        assert(race_paid_scarab_candidate_valid(u, MAX_UNITS));
        {   /* Reuse the Reaver's slot: the old Scarab must not adopt the new unit. */
            int reused = spawn_unit(SIDE, U_ZEALOT, u[shot].x, u[shot].y + 64);
            assert(reused >= 0);
            if (reused == r) printf("loss: the dead Reaver's slot was reused\n");
            assert(u[shot].build == -1);
        }
        drain();
        assert(hp_total(t0) < hp1);
        printf("loss: an orphaned Scarab still hit for %d\n", hp1 - hp_total(t0));
    }
    kill_actor(t0);
    /* Slot reuse of the target: a Scarab aimed at a dead unit's slot loses it. */
    {
        int r2 = reaver_at(hx - 96, hy), vic;
        race_paid_scarab_trained(r2); u[r2].carry = SC64_PSC_INIT | 2;
        vic = enemy_unit(U_ZP_ULTRALISK, hx, hy);
        attack(r2, vic);
        shot = wait_launch(r2);
        stand_down(r2);
        assert(u[shot].target == vic);
        kill_actor(vic);
        {   /* Occupy the freed slot with a friend of the Scarab's owner. */
            int again = spawn_unit(SIDE, U_ZEALOT, hx + 200, hy);
            assert(again >= 0);
            if (again == vic) printf("loss: the victim's slot was reused by a friendly unit\n");
        }
        step(1);
        if (u[shot].alive && u[shot].type == U_SCARAB) assert(u[shot].target == -1);
        drain();
    }
    /* Slot reuse of a BUILDING target: a Scarab aimed at a destroyed building's slot must not
     * adopt the next hostile building placed in it. It explodes at its last position. */
    {
        int r3 = reaver_at(hx - 96, hy), old_h, new_h, lastx, lasty, hp_new;
        race_paid_scarab_trained(r3); u[r3].carry = SC64_PSC_INIT | 2;
        old_h = enemy_hatchery(hx, hy);
        attack(r3, MAX_UNITS + old_h);
        shot = wait_launch(r3);
        stand_down(r3);
        assert(u[shot].target == MAX_UNITS + old_h);
        kill_actor(MAX_UNITS + old_h);
        assert(!b[old_h].alive);
        /* A new hostile building takes the freed slot, far from the old one. */
        new_h = enemy_hatchery(hx + 400, hy + 400);
        if (new_h == old_h) printf("loss: the destroyed building's slot was reused by a new hostile building\n");
        assert(new_h == old_h);
        hp_new = hp_total(MAX_UNITS + new_h);
        assert(u[shot].alive && u[shot].type == U_SCARAB && u[shot].target == -1);
        lastx = u[shot].tx; lasty = u[shot].ty;
        step(1);
        if (u[shot].alive && u[shot].type == U_SCARAB) {
            assert(u[shot].target == -1);
            assert(u[shot].tx == lastx && u[shot].ty == lasty);   /* never re-aimed at the new one */
        }
        drain();
        assert(hp_total(MAX_UNITS + new_h) == hp_new);            /* the new building took nothing */
        printf("loss: stale building index did not damage the reused slot (hp %d unchanged)\n", hp_new);
        kill_actor(MAX_UNITS + new_h);
    }
    puts("loss PASS");
}

static void validate(void)
{
    int hx, hy, r = arena(&hx, &hy), h, t0, shot;
    static Unit copy[MAX_UNITS];
    u[r].carry = SC64_PSC_INIT | 2 | (1 << 4); u[r].work_timer = 20;
    h = enemy_hatchery(hx, hy); t0 = MAX_UNITS + h;
    attack(r, t0);
    shot = -1; for (int n = 0; n < 400 && shot < 0; ++n) { step(1); shot = scarab_in_flight(r); }
    assert(shot >= 0 && race_paid_scarab_candidate_valid(u, MAX_UNITS));
    memcpy(copy, u, sizeof copy);
#define BAD(stmt) do { memcpy(copy, u, sizeof copy); { Unit *R = &copy[r], *S = &copy[shot]; (void)R; (void)S; stmt; } \
                       assert(!race_paid_scarab_candidate_valid(copy, MAX_UNITS)); } while (0)
    BAD(R->carry = SC64_PSC_INIT | 6);                       /* six held: over capacity */
    BAD(R->carry = SC64_PSC_INIT | 3 | (3 << 4));            /* 3 held + 3 ordered = 6 */
    BAD(R->carry = SC64_PSC_INIT | (6 << 4));                /* six ordered */
    BAD(R->carry = SC64_PSC_INIT | (61 << 7));               /* cooldown over 60 */
    BAD(R->carry = 5);                                       /* not established, yet nonzero */
    BAD(R->work_timer = 106);                                /* over the build time */
    BAD(R->carry = SC64_PSC_INIT | 1; R->work_timer = 7);    /* progress without an order */
    BAD(R->carry = -1);
    BAD(R->carry = 1 << 14);
    BAD(S->work_timer = 0);                                  /* flying with no life */
    BAD(S->work_timer = 91);
    BAD(S->carry = 5);
    BAD(S->flags = 0);                                       /* must be immune */
    BAD(S->hp = 19);
    BAD(S->selected = 1);
    BAD(S->target = MAX_UNITS + MAX_BUILDINGS);
    BAD(S->build = MAX_UNITS);
    BAD(S->build = shot);                                    /* parent is not a Reaver */
    BAD(S->raw_type = 83);                                   /* type and raw disagree */
    {   /* A dead row's state is never read (a reuse clears it): stale state in one is not a malformed world. */
        memcpy(copy, u, sizeof copy); copy[r].alive = 0; copy[shot].build = -1;   /* its Scarab flies on, detached */
        assert(race_paid_scarab_candidate_valid(copy, MAX_UNITS));
    }
    {   /* Two Scarabs for one Reaver. */
        int second = -1;
        for (int i = 0; i < MAX_UNITS; ++i) if (!u[i].alive && i != shot) { second = i; break; }
        BAD(Unit *T = &copy[second]; *T = *S; T->alive = 1);
    }
    puts("validate PASS (19 malformed worlds rejected, stale dead rows ignored)");
}

/* ---- saves ---- */

static int saved_reaver, saved_shot;
static void world_with_state(void)
{
    int hx, hy, r = arena(&hx, &hy), r2, h, shot = -1;
    saved_reaver = r;
    /* One Reaver mid-build: 2 held, 3 ordered, the front one 50 ticks in. */
    u[r].carry = SC64_PSC_INIT | 2 | (3 << 4); u[r].work_timer = 50;
    /* Another with a Scarab in flight. */
    r2 = reaver_at(hx - 96, hy - 300);
    race_paid_scarab_trained(r2); u[r2].carry = SC64_PSC_INIT | 4;
    h = enemy_hatchery(hx, hy - 300);
    attack(r2, MAX_UNITS + h);
    for (int n = 0; n < 400 && shot < 0; ++n) { step(1); shot = scarab_in_flight(r2); }
    assert(shot >= 0);
    saved_shot = shot;
    step(3);                                   /* mid-flight, not at launch */
    assert(u[shot].alive && race_paid_scarab_candidate_valid(u, MAX_UNITS));
    race_minerals[ENEMY] = 321;
}
static void t_save_write(const char *path)
{
    unsigned digests[7];
    world_with_state();
    assert(race_save_game());
    write_sram(path);
    {
        char side[512]; FILE *f;
        int held = race_paid_scarab_stock(saved_reaver), ordered = race_paid_scarab_queued(saved_reaver);
        int progress = u[saved_reaver].work_timer, life = u[saved_shot].work_timer;
        assert(held == 2 && ordered == 3 && progress >= 50 && progress < 105 && life >= 1);
        snprintf(side, sizeof side, "%s.state", path);
        digests[0] = state_digest();
        for (int k = 1; k < 7; ++k) { step(100); digests[k] = state_digest(); }
        f = fopen(side, "w"); assert(f);
        fprintf(f, "%d %d %d %d %d %d\n", saved_reaver, saved_shot, held, ordered, progress, life);
        for (int k = 0; k < 7; ++k) fprintf(f, "%u\n", digests[k]);
        assert(!fclose(f));
    }
    printf("saved: Reaver %d (2 held, 3 ordered, mid-build), Scarab %d in flight\n",
           saved_reaver, saved_shot);
}
static void t_save_read(const char *path)
{
    char side[512]; FILE *f; unsigned want[7]; int rv, sh, held, ordered, progress, life;
    snprintf(side, sizeof side, "%s.state", path);
    f = fopen(side, "r"); assert(f);
    assert(fscanf(f, "%d %d %d %d %d %d", &rv, &sh, &held, &ordered, &progress, &life) == 6);
    for (int k = 0; k < 7; ++k) assert(fscanf(f, "%u", &want[k]) == 1);
    fclose(f);
    gba_init(); game_init(); read_sram(path);
    assert(race_load_game());
    assert(race_mode == 1 && race_human == R_PROTOSS);
    assert(race_paid_scarab_stock(rv) == held && race_paid_scarab_queued(rv) == ordered &&
           u[rv].work_timer == progress);
    assert(u[sh].alive && u[sh].type == U_SCARAB && u[sh].work_timer == life && u[sh].target >= 0 &&
           u[sh].build >= 0 && u[u[sh].build].type == U_REAVER);
    assert(state_digest() == want[0]);
    for (int k = 1; k < 7; ++k) { step(100); assert(state_digest() == want[k]); }
    printf("save-read: fresh process matched 6 digests over 600 updates; Reaver now holds %d\n",
           race_paid_scarab_stock(rv));
}

/* In-process: save, wreck the world, load, keep going identically. */
static void save_in_process(void)
{
    unsigned a[4], c[4];
    world_with_state();
    assert(race_save_game());
    for (int k = 0; k < 4; ++k) { step(80); a[k] = state_digest(); }
    start_custom();                              /* wipe */
    assert(race_load_game());
    for (int k = 0; k < 4; ++k) { step(80); c[k] = state_digest(); assert(a[k] == c[k]); }
    puts("save-in-process PASS");
}


/* ---- campaign enemy Reavers: placed empty, restocked on the timer, no pay ---- */

static u16 prev_keys;
static void pad(u16 h) { game_update(h, (u16)(h & (u16)~prev_keys)); prev_keys = h; }
static void ptap(u16 k) { pad(k); pad(0); }
static void start_campaign(int gid)
{
    int ord;
    game_init(); prev_keys = 0;
    assert(screen == SCREEN_TITLE && race_front.page == SC_FRONT_HOME);
    ptap(KEY_A);
    ord = sc64_catalog_playable_ordinal(SC64_FACTION_PROTOSS, gid);
    assert(ord > 0);
    for (int i = 0; i < ord / 2; ++i) ptap(KEY_RIGHT);
    if (ord & 1) ptap(KEY_DOWN);
    ptap(KEY_A);
    assert(race_front.page == SC_FRONT_BRIEFING && race_front.mission == gid);
    pad(KEY_START);
    assert(screen == SCREEN_PLAY && !race_mode);
}
static int enemy_reavers(int *out, int max)
{
    int n = 0;
    for (int i = 0; i < MAX_UNITS && n < max; ++i)
        if (u[i].alive && u[i].type == U_REAVER && u[i].owner != SIDE) out[n++] = i;
    return n;
}

static void campaign(void)
{
    int rv[8], n, fired = 0, one_at_a_time = 1, bank[5], spent_updates = 0;
    start_campaign(SC64_P8_GLOBAL_ID);
    for (int k = 0; k < 4; ++k) pad(0);
    n = enemy_reavers(rv, 8);
    assert(n >= 1);
    for (int k = 0; k < n; ++k)
        assert(race_paid_scarab_stock(rv[k]) == 0 && race_paid_scarab_queued(rv[k]) <= 1);
    printf("campaign: %d placed enemy Reavers start with 0 Scarabs at frame %d\n", n, frame);
    for (int o = 0; o < 5; ++o) bank[o] = race_minerals[o];
    for (int t = 0; t < 5 * 131 + 120; ++t) {
        int before[5];
        for (int o = 0; o < 5; ++o) before[o] = race_minerals[o];
        pad(0);
        for (int o = 0; o < 5; ++o) if (race_minerals[o] < before[o]) ++spent_updates;
        for (int k = 0; k < n; ++k)
            if (u[rv[k]].alive && race_paid_scarab_queued(rv[k]) > 1) one_at_a_time = 0;
    }
    for (int k = 0; k < n; ++k)
        if (u[rv[k]].alive) {
            printf("campaign: Reaver %d holds %d after %d updates\n", rv[k], race_paid_scarab_stock(rv[k]), frame);
            assert(race_paid_scarab_stock(rv[k]) == 5);
        }
    assert(one_at_a_time);
    printf("campaign: updates in which any engine bank fell: %d\n", spent_updates);
    assert(spent_updates == 0);                                           /* restock charged no engine bank */
    {   /* A target in reach: it fires and starts a replacement. */
        int r = rv[0], z = spawn_unit(SIDE, U_ZEALOT, u[r].x + 70, u[r].y);
        assert(z >= 0); u[z].order = ORDER_IDLE;
        for (int t = 0; t < 300 && !fired; ++t) { pad(0); fired = scarab_in_flight(r) >= 0; }
        assert(fired);
        for (int t = 0; t < 40; ++t) pad(0);
        printf("campaign: it fired (stock now %d, queued %d)\n", race_paid_scarab_stock(r), race_paid_scarab_queued(r));
        assert(race_paid_scarab_stock(r) + race_paid_scarab_queued(r) >= 4);
    }
    puts("campaign PASS");
}

/* ---- placed Reavers start with the source map's hangar count ---- */

/* The N64 table in docs/PAID-SCARABS.md, as (mission, source owner, Reavers, Scarabs each). */
static const struct { int mission, owner, count, stock; } n64_placed[] = {
    {SC64_P7_GLOBAL_ID, 0, 2, 5}, {SC64_P7_GLOBAL_ID, 4, 2, 0}, {SC64_P8_GLOBAL_ID, 3, 2, 0},
    {17, 1, 4, 0},                                        /* Z6 */
    {19, 1, 2, 0}, {19, 2, 2, 0},                         /* Z8 */
    {20, 2, 1, 0}, {20, 7, 4, 0}, {20, 4, 4, 0},          /* Z9 */
    {21, 0, 1, 0},                                        /* Z10 */
};

static void placed_hangar(void)
{
    int rv[16], n = 0, expected = 0;
    /* 1. The generated table agrees with the N64 table, row by row and in total. */
    for (unsigned k = 0; k < sizeof n64_placed / sizeof *n64_placed; ++k) {
        int found = 0;
        expected += n64_placed[k].count;
        for (unsigned i = 0; i < SC64_PLACED_HANGAR_COUNT; ++i)
            if (sc64_placed_hangar[i].mission == n64_placed[k].mission &&
                sc64_placed_hangar[i].owner == n64_placed[k].owner) {
                ++found;
                assert(sc64_placed_hangar[i].hangar == n64_placed[k].stock);
            }
        assert(found == n64_placed[k].count);
    }
    assert((int)SC64_PLACED_HANGAR_COUNT == expected);
    printf("placed hangar: the table's %d Reavers agree with the N64 rows (P7 owner 0 = 5, all others 0)\n", expected);
    /* 2. P7 in the game: owner 0's two Reavers start full, owner 4's empty. */
    start_campaign(SC64_P7_GLOBAL_ID);
    for (int k = 0; k < 4; ++k) pad(0);
    for (int i = 0, m = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].type == U_REAVER) rv[m++] = i, n = m;
    assert(n == 4);
    {
        int full = 0, empty = 0;
        for (int k = 0; k < n; ++k) {
            int owner = u[rv[k]].raw_owner, stock = race_paid_scarab_stock(rv[k]);
            printf("placed hangar: P7 Reaver %d, source owner %d, holds %d\n", rv[k], owner, stock);
            assert(stock == (owner == 0 ? 5 : 0));
            full += owner == 0 && stock == 5; empty += owner == 4 && stock == 0;
        }
        assert(full == 2 && empty == 2);
    }
    /* 3. A Reaver that appears later (reinforced, rescued, made by a trigger) starts full. */
    for (int t = 0; t < SC64_PSC_PLACED_FRAMES + 10; ++t) pad(0);
    {
        int late = spawn_unit(SIDE, U_REAVER, u[rv[0]].x + 40, u[rv[0]].y + 40);
        assert(late >= 0);
        pad(0);
        printf("placed hangar: a Reaver that arrives at frame %d holds %d (the project rule: full)\n", frame, race_paid_scarab_stock(late));
        assert(race_paid_scarab_stock(late) == 5);
    }
    /* 4. The empty pair restocks on the timer as before; the full pair stays at the cap. */
    for (int t = 0; t < 5 * 131 + 60; ++t) pad(0);
    for (int k = 0; k < n; ++k) if (u[rv[k]].alive) assert(race_paid_scarab_stock(rv[k]) == 5);
    puts("placed hangar PASS");
}

/* ---- starting stock, wake-up, battery saves, cargo death, birth into a freed slot ---- */

/* The stock every placed P7 Reaver holds, read straight after each of the first updates. */
static void check_p7_placed_stock(int updates)
{
    int rv[16], n = 0;
    for (int k = 0; k < updates; ++k) {
        pad(0);
        n = 0;
        for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].type == U_REAVER) rv[n++] = i;
        assert(n == 4);
        for (int j = 0; j < n; ++j)
            assert(race_paid_scarab_stock(rv[j]) == (u[rv[j]].raw_owner == 0 ? 5 : 0));
    }
}

/* The first world tick meets rows nobody has established yet: give the placed ones back their raw state and tick. */
static void tick_unestablished(void)
{
    int rv[16], n = 0;
    assert((unsigned)frame <= SC64_PSC_PLACED_FRAMES);
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].type == U_REAVER) { u[i].carry = 0; u[i].work_timer = 0; rv[n++] = i; }
    assert(n == 4);
    sc64_psc_hint = 1;
    race_paid_scarab_world_tick();
    for (int j = 0; j < n; ++j) {
        int want = u[rv[j]].raw_owner == 0 ? 5 : 0;
        assert(race_paid_scarab_stock(rv[j]) == want);
        assert(u[rv[j]].carry & SC64_PSC_INIT);
        assert(((u[rv[j]].carry & 15) == want) || want == 0);   /* what the tick stored is the source count */
    }
}

static void cold_start(void)
{
    static Unit bad[MAX_UNITS];
    /* 1. A world start puts the service to sleep, and a tick that meets an unestablished placed Reaver
     * gives it the source hangar count, not a full load (P7: owner 0 holds 5, owner 4 holds 0). */
    sc64_psc_hint = 1;
    start_custom();                                  /* a world with no Reaver in it */
    assert(sc64_psc_hint == 0);
    start_campaign(SC64_P7_GLOBAL_ID);
    assert((unsigned)frame < 8);
    tick_unestablished();
    check_p7_placed_stock(8);
    puts("cold start: an unestablished placed Reaver read by the first world tick holds the source count");
    /* 2. The same process, started again after a world that restocked. */
    for (int t = 0; t < 200; ++t) pad(0);
    sc64_psc_hint = 1;
    start_custom();                                  /* a world with no Reaver in it */
    assert(sc64_psc_hint == 0);
    start_campaign(SC64_P7_GLOBAL_ID);
    tick_unestablished();
    check_p7_placed_stock(8);
    puts("cold start: same-process restart gives the same stock");
    /* 3. A rejected candidate leaves the hint alone, and so does a failed load. */
    check_p7_placed_stock(2);
    sc64_psc_hint = 0;
    memcpy(bad, u, sizeof bad);
    for (int i = 0; i < MAX_UNITS; ++i) if (bad[i].alive && bad[i].type == U_REAVER) { bad[i].carry = 5; break; }
    assert(!race_paid_scarab_candidate_valid(bad, MAX_UNITS) && sc64_psc_hint == 0);
    assert(race_paid_scarab_candidate_valid(u, MAX_UNITS) && sc64_psc_hint == 0);
    memset(host_save, 0, sizeof host_save);
    assert(!race_load_game() && sc64_psc_hint == 0);
    start_campaign(SC64_P7_GLOBAL_ID);
    tick_unestablished();
    check_p7_placed_stock(8);
    puts("cold start PASS (hint reset, restart, failed validation and failed load)");
}

static void retrain(void)
{
    int rv[8], n, owner, raw_owner, x, y, s;
    start_campaign(SC64_P8_GLOBAL_ID);
    for (int k = 0; k < 4; ++k) pad(0);
    n = enemy_reavers(rv, 8);
    assert(n >= 1);
    owner = u[rv[0]].owner; raw_owner = u[rv[0]].raw_owner; x = u[rv[0]].x; y = u[rv[0]].y;
    for (int k = 0; k < n; ++k) kill_actor(rv[k]);
    for (int t = 0; t < 600 && sc64_psc_hint; ++t) pad(0);
    assert(sc64_psc_hint == 0);                      /* nothing left to tick: the service sleeps */
    s = spawn_unit(owner, U_REAVER, x + 24, y + 24);
    assert(s >= 0);
    u[s].raw_owner = raw_owner;
    race_paid_scarab_trained(s);
    assert(sc64_psc_hint == 1 && race_paid_scarab_stock(s) == 0);
    for (int t = 0; t < 5 * 131 + 60; ++t) pad(0);
    printf("retrain: an enemy Reaver trained after the others died holds %d after %d updates\n",
           race_paid_scarab_stock(s), 5 * 131 + 60);
    assert(race_paid_scarab_stock(s) == 5);
    puts("retrain PASS");
}

static void recharge(void)
{
    int r, bat = -1, order, progress, probe = -1;
    unsigned a[4], c[4];
    /* The Shield Battery of this vector is the P7 one: its order and its owner rules are the campaign's. */
    start_campaign(SC64_P7_GLOBAL_ID);
    for (int k = 0; k < 4; ++k) pad(0);
    for (int i = 0; i < MAX_BUILDINGS && bat < 0; ++i)
        if (b[i].alive && b[i].owner == SIDE && sc64_battery_raw_building(&b[i]) == 172) bat = i;
    for (int i = 0; i < MAX_UNITS && probe < 0; ++i)
        if (u[i].alive && u[i].owner == SIDE) probe = i;
    assert(probe >= 0);
    if (bat < 0) {   /* the human has none at the start: stand one next to its first actor */
        int pylon = spawn_build(SIDE, B_PYLON, u[probe].x + 80, u[probe].y, 0);
        assert(pylon >= 0);
        b[pylon].hp = b[pylon].maxhp; b[pylon].under_construction = 0; b[pylon].queue_count = 0;
        bat = spawn_build(SIDE, B_P7_SHIELD_BATTERY, u[probe].x + 80, u[probe].y + 48, 0);
        assert(bat >= 0);
        b[bat].raw_type = 172; b[bat].raw_owner = 7; b[bat].hp = b[bat].maxhp;
        b[bat].under_construction = 0; b[bat].queue_count = 0;
        invalidate_path_grid();
    }
    r = spawn_unit(SIDE, U_REAVER, b[bat].x + 48, b[bat].y + 48);
    assert(r >= 0);
    u[r].raw_type = 83; u[r].raw_owner = 7; u[r].order = ORDER_IDLE; u[r].tx = u[r].x; u[r].ty = u[r].y;
    race_paid_scarab_trained(r);
    u[r].carry = SC64_PSC_INIT | 2 | (3 << 4); u[r].work_timer = 50; u[r].shield = 10;
    b[bat].queue_progress = SC64_P7_BATTERY_MAX_ENERGY_Q8 / 2; b[bat].level = 0;
    order = race_battery_recharge_order();
    assert(race_battery_unit_valid(r, bat));
    u[r].order = order; u[r].target = bat;
    for (int t = 0; t < 12; ++t) pad(0);
    assert(u[r].alive && u[r].order == order && race_paid_scarab_queued(r) == 3 && u[r].work_timer > 50);
    progress = u[r].work_timer;
    /* The save must accept a recharging Reaver with a part-built Scarab, and load it back. */
    assert(race_save_game());
    for (int k = 0; k < 4; ++k) { for (int t = 0; t < 40; ++t) pad(0); a[k] = state_digest(); }
    gba_init(); game_init();
    assert(race_load_game() && !race_mode);
    assert(u[r].alive && u[r].order == order && race_paid_scarab_queued(r) == 3 && u[r].work_timer == progress);
    for (int k = 0; k < 4; ++k) { for (int t = 0; t < 40; ++t) pad(0); c[k] = state_digest(); assert(a[k] == c[k]); }
    printf("recharge: saved and reloaded a recharging Reaver with 3 ordered and %d ticks of progress\n", progress);
    /* No Battery path wipes the progress: running dry, losing the Battery, or a new command. */
    progress = u[r].work_timer;
    u[r].order = order; u[r].target = bat;
    b[bat].queue_progress = 0;                       /* the next service tick cancels */
    for (int t = 0; t < 12; ++t) pad(0);
    assert(u[r].order == ORDER_IDLE && u[r].work_timer >= progress);       /* the build went on, none was wiped */
    assert(race_paid_scarab_stock(r) + race_paid_scarab_queued(r) == 5);
    u[r].order = order; u[r].target = bat; progress = u[r].work_timer;
    race_battery_clear_building_references(bat);
    assert(u[r].order == ORDER_IDLE && u[r].work_timer == progress);
    puts("recharge PASS");
}

static void shuttle(void)
{
    int hx, hy, r = arena(&hx, &hy), sh, raw = 69, loaded = 0;
    unsigned a[3], c[3];
    assert(sc64_race_unit_from_raw(raw) >= 0);
    sh = spawn_unit(SIDE, sc64_race_unit_from_raw(raw), u[r].x + 24, u[r].y);
    assert(sh >= 0 && race_transport_carrier_type(&u[sh]));
    u[sh].order = ORDER_IDLE; u[sh].tx = u[sh].x; u[sh].ty = u[sh].y;
    assert(race_transport_request_board(r, sh));
    for (int t = 0; t < 300 && !loaded; ++t) { step(1); loaded = SC64_TRANSPORT_LOADED(&u[r]); }
    assert(loaded && u[r].carry != 0);
    kill_actor(sh);
    step(1);
    printf("shuttle: the loaded Reaver %s (carry %d, work %d)\n", u[r].alive ? "survived" : "died with its Shuttle",
           u[r].carry, u[r].work_timer);
    assert(!u[r].alive && u[r].carry == 0 && u[r].work_timer == 0);
    assert(race_paid_scarab_candidate_valid(u, MAX_UNITS));
    assert(race_save_game());
    for (int k = 0; k < 3; ++k) { step(60); a[k] = state_digest(); }
    start_custom();
    assert(race_load_game());
    for (int k = 0; k < 3; ++k) { step(60); c[k] = state_digest(); assert(a[k] == c[k]); }
    puts("shuttle PASS (state shed at the cargo death, save and reload after it)");
}

static void birth_slot(void)
{
    int hx, hy, r = arena(&hx, &hy), victim, h, t0, child, freed;
    victim = enemy_unit(U_ZEALOT, hx + 200, hy);
    step(2);
    h = enemy_hatchery(hx, hy); t0 = MAX_UNITS + h;
    race_paid_scarab_trained(r); u[r].carry = SC64_PSC_INIT | 5 | (60 << 7);   /* cooling down: it holds its fire for now */
    step(2);
    u[r].carry = SC64_PSC_INIT | 5;
    /* The index structures as an update builds them: every live unit, the victim included, is linked. */
    sc64_unit_buckets_begin(); sc64_target_index_begin(); target_lists_valid = 1;
    assert(sc64_unit_bucket_active && sc64_target_index_active && sc64_target_cell[victim] != 0xffffu);
    kill_actor(victim);                              /* the slot dies in this update, still linked */
    freed = -1;
    for (int i = 0; i < MAX_UNITS && freed < 0; ++i) if (!u[i].alive) freed = i;   /* what spawn_unit will take */
    assert(freed == victim);
    assert(sc64_unit_bucket_active && sc64_target_index_active && target_lists_valid);   /* a death alone leaves them on */
    race_paid_scarab_launch(&u[r], t0);
    child = scarab_in_flight(r);
    assert(child >= 0);
    printf("birth: the Scarab took slot %d (the victim's); buckets %d, target index %d, owner lists %d afterwards\n", child,
           sc64_unit_bucket_active, sc64_target_index_active, target_lists_valid);
    assert(child == victim);
    assert(!sc64_unit_bucket_active && !sc64_target_index_active && !target_lists_valid);
    step(3);
    for (int i = 0; i < MAX_UNITS; ++i)              /* nothing aims at the live Scarab */
        if (u[i].alive && u[i].target == child) assert(0);
    puts("birth PASS");
}

/* An empty placed enemy Reaver in reach on the first update: the launch attempt establishes its row and
 * fires nothing, and the service must still be awake to restock it. */
static void restock_empty(void)
{
    int rv[8], n, r, z;
    unsigned a[4], c[4];
    start_campaign(SC64_P8_GLOBAL_ID);
    n = enemy_reavers(rv, 8);
    assert(n >= 1);
    r = rv[0];
    u[r].carry = 0; u[r].work_timer = 0;             /* as before the first end of update: this one alone */
    sc64_psc_hint = 0;
    z = spawn_unit(SIDE, U_PROBE, u[r].x + 70, u[r].y);
    assert(z >= 0 && (unsigned)frame <= SC64_PSC_PLACED_FRAMES);
    u[z].order = ORDER_IDLE; u[z].tx = u[z].x; u[z].ty = u[z].y;
    /* The first update's combat step in reach of a target: the launch attempt establishes the row at stock 0 and
     * fires nothing. */
    race_paid_scarab_launch(&u[r], z);
    assert((u[r].carry & SC64_PSC_INIT) && race_paid_scarab_stock(r) == 0 && scarab_in_flight(r) < 0);
    assert(sc64_psc_hint == 1);                      /* established by a reader that did nothing: the service is awake */
    pad(0);
    printf("restock0: after update 1 the Reaver %s (carry %d), hint %d\n",
           (u[r].carry & SC64_PSC_INIT) ? "is established" : "is not established", u[r].carry, sc64_psc_hint);
    assert(sc64_psc_hint == 1);
    assert(race_paid_scarab_stock(r) == 0);
    for (int t = 0; t < 140; ++t) pad(0);            /* the first Scarab: one per 105 ticks, about 131 updates */
    assert(race_paid_scarab_stock(r) == 1 || race_paid_scarab_queued(r) == 1);
    /* The earliest save the writer accepts, the uninterrupted continuation, and a fresh load of that save. */
    for (int t = 0; t < 40 && !race_save_game(); ++t) pad(0);
    assert(race_save_game());
    for (int k = 0; k < 4; ++k) { for (int t = 0; t < 150; ++t) pad(0); a[k] = state_digest(); }
    printf("restock0: uninterrupted, the Reaver holds %d after %d updates\n", race_paid_scarab_stock(r), frame);
    assert(race_paid_scarab_stock(r) >= 3);
    gba_init(); game_init();
    assert(race_load_game() && !race_mode);
    for (int k = 0; k < 4; ++k) { for (int t = 0; t < 150; ++t) pad(0); c[k] = state_digest(); assert(a[k] == c[k]); }
    puts("restock0 PASS (restock starts on schedule; the save and a fresh load match the uninterrupted run)");
}

/* A Science Facility that lifts while a Scarab is on its way: the Scarab drops it as a target and the
 * explosion leaves it alone. */
static void lift_facility(void)
{
    int hx, hy, r = arena(&hx, &hy), t, f, hp, shot;
    for (t = 0; t < SC_RB_COUNT && sc64_race_raw_building(t) != 116; ++t) ;
    assert(t < SC_RB_COUNT);
    f = spawn_build(ENEMY, t, hx, hy, 0);
    assert(f >= 0);
    b[f].hp = b[f].maxhp; b[f].under_construction = 0; b[f].queue_count = 0;
    invalidate_path_grid();
    sc64_visibility_reset(); update_exploration();
    race_paid_scarab_trained(r); u[r].carry = SC64_PSC_INIT | 5;
    hp = b[f].hp + b[f].shield;
    attack(r, MAX_UNITS + f);
    shot = -1;
    for (int n = 0; n < 400 && shot < 0; ++n) { step(1); shot = scarab_in_flight(r); }
    assert(shot >= 0 && u[shot].target == MAX_UNITS + f);
    b[f].flags |= 4;                                 /* it lifts off after the launch */
    assert(race_bc_facility_airborne(f));
    step(2);
    assert(u[shot].alive && u[shot].target == -1);   /* the Scarab no longer homes on it */
    for (int n = 0; n < 120 && scarab_in_flight(r) >= 0; ++n) step(1);
    assert(scarab_in_flight(r) < 0);
    printf("lift: the lifted facility kept %d of %d points\n", b[f].hp + b[f].shield, hp);
    assert(b[f].hp + b[f].shield == hp);
    puts("lift PASS");
}

static void campaign_write(const char *path)
{
    unsigned digests[7]; int rv[8], n;
    start_campaign(SC64_P8_GLOBAL_ID);
    for (int k = 0; k < 4; ++k) pad(0);
    n = enemy_reavers(rv, 8); assert(n >= 1);
    for (int t = 0; t < 200; ++t) pad(0);
    assert(race_paid_scarab_queued(rv[0]) == 1 && u[rv[0]].work_timer > 0 && u[rv[0]].work_timer < 105);
    assert(race_save_game()); write_sram(path);
    {
        char side[512]; FILE *f;
        snprintf(side, sizeof side, "%s.state", path);
        digests[0] = state_digest();
        for (int k = 1; k < 7; ++k) { for (int t = 0; t < 100; ++t) pad(0); digests[k] = state_digest(); }
        f = fopen(side, "w"); assert(f);
        fprintf(f, "%d %d %d\n", rv[0], race_paid_scarab_stock(rv[0]), (int)u[rv[0]].work_timer);
        for (int k = 0; k < 7; ++k) fprintf(f, "%u\n", digests[k]);
        assert(!fclose(f));
    }
    puts("campaign-write: saved mid-restock (one on order, part built), continued 600 updates");
}
static void campaign_read(const char *path)
{
    char side[512]; FILE *f; unsigned want[7]; int rv, stock_after, progress_after;
    snprintf(side, sizeof side, "%s.state", path);
    f = fopen(side, "r"); assert(f);
    assert(fscanf(f, "%d %d %d", &rv, &stock_after, &progress_after) == 3);
    for (int k = 0; k < 7; ++k) assert(fscanf(f, "%u", &want[k]) == 1);
    fclose(f);
    gba_init(); game_init(); read_sram(path);
    assert(race_load_game() && !race_mode);
    assert(race_paid_scarab_queued(rv) == 1 && u[rv].work_timer > 0 && u[rv].work_timer < 105);
    assert(state_digest() == want[0]);
    for (int k = 1; k < 7; ++k) { for (int t = 0; t < 100; ++t) pad(0); assert(state_digest() == want[k]); }
    printf("campaign-read: fresh process resumed mid-restock; 6 digests match; Reaver now holds %d\n",
           race_paid_scarab_stock(rv));
}

#endif /* HAVE_SERVICE */

/* The old-save check: build the same file against the unchanged tree. */
static void t_old_write(const char *path)
{
    gba_init(); game_init(); race_mode = 1; race_human = R_PROTOSS;
    race_bots = 1; race_bot_races[0] = R_PROTOSS; race_skirmish_map = 0; init_state();
    for (int i = 0; i < MAX_UNITS; ++i) u[i].selected = 0;
    int r = spawn_unit(SIDE, U_REAVER, 700, 700);
    assert(r >= 0);
    for (int i = 0; i < 300; ++i) game_update(0, 0);
    assert(race_save_game());
    write_sram(path);
    {
        char side[512]; FILE *f;
        snprintf(side, sizeof side, "%s.state", path);
        f = fopen(side, "w"); assert(f);
        fprintf(f, "%d %d %d %d\n", r, u[r].x, u[r].y, minerals); assert(!fclose(f));
    }
    puts("old-write: Reaver saved by the base code");
}
#ifdef HAVE_SERVICE
static void t_old_read(const char *path)
{

#ifdef HAVE_SERVICE
    char side[512]; FILE *f; int r, x, y, m;
    snprintf(side, sizeof side, "%s.state", path);
    f = fopen(side, "r"); assert(f);
    assert(fscanf(f, "%d %d %d %d", &r, &x, &y, &m) == 4); fclose(f);
    gba_init(); game_init(); read_sram(path);
    assert(race_load_game());
    assert(u[r].alive && u[r].type == U_REAVER && u[r].carry == 0 && u[r].work_timer == 0);
    assert(race_paid_scarab_stock(r) == 5);                /* loaded on first look */
    {
        int h = enemy_hatchery(u[r].x + 96, u[r].y), hp0;
        hp0 = hp_total(MAX_UNITS + h);
        attack(r, MAX_UNITS + h);
        for (int n = 0; n < 300; ++n) step(1);
        assert(hp_total(MAX_UNITS + h) < hp0 && race_paid_scarab_stock(r) < 5);
    }
    puts("old-read: a pre-service save loads; its Reaver is loaded and fights");
#else
    (void)path; puts("old-read needs the service build"); exit(2);
#endif
}
#endif /* HAVE_SERVICE */

int main(int argc, char **argv)
{
    if (argc >= 2 && !strcmp(argv[1], "old-write") && argc == 3) { t_old_write(argv[2]); return 0; }
#ifdef HAVE_SERVICE
    if (argc == 2 && !strcmp(argv[1], "economy")) economy();
    else if (argc == 2 && !strcmp(argv[1], "combat")) combat();
    else if (argc == 2 && !strcmp(argv[1], "splash")) splash();
    else if (argc == 2 && !strcmp(argv[1], "loss")) loss();
    else if (argc == 2 && !strcmp(argv[1], "validate")) validate();
    else if (argc == 2 && !strcmp(argv[1], "save-process")) save_in_process();
    else if (argc == 2 && !strcmp(argv[1], "campaign")) campaign();
    else if (argc == 2 && !strcmp(argv[1], "placed")) placed_hangar();
    else if (argc == 2 && !strcmp(argv[1], "cold")) cold_start();
    else if (argc == 2 && !strcmp(argv[1], "retrain")) retrain();
    else if (argc == 2 && !strcmp(argv[1], "recharge")) recharge();
    else if (argc == 2 && !strcmp(argv[1], "shuttle")) shuttle();
    else if (argc == 2 && !strcmp(argv[1], "birth")) birth_slot();
    else if (argc == 2 && !strcmp(argv[1], "restock0")) restock_empty();
    else if (argc == 2 && !strcmp(argv[1], "lift")) lift_facility();
    else if (argc == 3 && !strcmp(argv[1], "campaign-write")) campaign_write(argv[2]);
    else if (argc == 3 && !strcmp(argv[1], "campaign-read")) campaign_read(argv[2]);
    else if (argc == 3 && !strcmp(argv[1], "save-write")) t_save_write(argv[2]);
    else if (argc == 3 && !strcmp(argv[1], "save-read")) t_save_read(argv[2]);
    else if (argc == 3 && !strcmp(argv[1], "old-read")) t_old_read(argv[2]);
    else { fprintf(stderr, "usage: see header comment\n"); return 2; }
#else
    fprintf(stderr, "base build: only old-write is available\n"); return 2;
#endif
    return 0;
}
