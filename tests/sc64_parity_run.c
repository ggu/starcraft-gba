/* SPDX-License-Identifier: GPL-3.0-or-later */
/*
 * Parity demonstration runner: launches a mission or a skirmish through the
 * ordinary menus, injects a short scripted scenario through
 * tests/sc64_parity_inject.h and, when built with SC64_PARITY_TRACE, writes a
 * trace with tests/sc64_parity_trace.h. tools/sc64_parity_selfgate.py runs it
 * for the four harness self-gates.
 *
 *   sc64_parity_run --scenario t1|skirmish [--steps N] [--trace PATH]
 *                   [--no-render] [--no-chain] [--no-route-checks] [--profile NAME]
 *   sc64_parity_run --scenario move --unit X,Y [--unit X,Y ...] --target X,Y --at STEP ...
 *   sc64_parity_run --scenario income ...
 *
 * move and income are the N64 parity scenarios (tools/sc64_parity.py passes
 * their parameters from a golden file). move launches T1 and gives the
 * human Marines standing exactly at the --unit points one group move order
 * to --target at step --at. income launches Blood Bath, human Zerg against
 * one Terran computer, and gives no command: the skirmish starts the human's
 * Drones mining, as the cartridge does.
 *
 * It prints one JSON line: steps taken, host updates, the world hash and the
 * per-step world-hash chain, the route-purity checks made and the setup
 * failures (any failure exits 3). A step is an update in which the simulation
 * advanced: a text box or a menu takes none. Host only.
 */
#if defined(__linux__) && !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200809L /* clock_gettime under -std=c11 on glibc */
#endif
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef SC64_PARITY_TRACE
static void sc64_pt_hit(int target, int damage, int source_type, int source_owner);
static void sc64_pt_sound(int kind, int raw, int slot, int x, int y);
static int sc64_pt_text_box(unsigned trigger, unsigned action, unsigned long string_id, int opened);
#define SC64_TEST_HIT_OBSERVER(t, d, st, so) sc64_pt_hit((t), (d), (st), (so))
#define GAME_AUDIO_REQUEST(kind, raw, slot, x, y) sc64_pt_sound((kind), (raw), (slot), (x), (y))
#define SC64_TEXT_BOX_POSTED(row, pushed) \
    sc64_pt_text_box((row)->trigger, (row)->action, (row)->string_id, (pushed))
#endif

#include "../starcraft/game.c"
#define ROUTE_PURE_DISABLE 1
#include "sc64_route_purity.h"
#include "sc64_parity_inject.h"
#ifdef SC64_PARITY_TRACE
#include "sc64_parity_trace.h"
#endif

#ifndef SC64_RACES
#error "the parity runner scenarios need a race profile"
#endif

/* ---- World hash ------------------------------------------------------- */

static uint64_t pr_mix(uint64_t h, const void *ptr, size_t n)
{
    const unsigned char *p = (const unsigned char *)ptr;
    while (n--) { h ^= *p++; h *= 1099511628211ull; }
    return h;
}
#define PR_FIELD(v) do { long long x_ = (long long)(v); h = pr_mix(h, &x_, sizeof(x_)); } while (0)

/* Route state as route_purity_hash() covers it, but comparable across
 * processes: route statics hold pointers into route state (the field table,
 * the spare point plane, facility frontiers, and the pointer FIFO the linear
 * flood keeps in bfs_queue), and their values change with each process load.
 * Every 8-byte-aligned word that points into a route region is hashed as that
 * region's index and the offset instead. No other value can fall in that
 * address range by accident on a 64-bit host. */
static int pr_route_pointer(uint64_t value, long long where[2])
{
    for (size_t r = 0; r < ROUTE_PURE_REGION_COUNT; ++r) {
        uintptr_t a = (uintptr_t)route_pure_regions[r].addr;
        if (value >= a && value <= a + route_pure_regions[r].size) {
            where[0] = (long long)r;
            where[1] = (long long)(value - a);
            return 1;
        }
    }
    return 0;
}

static uint64_t pr_route_hash(void)
{
    uint64_t h = 1469598103934665603ull;
    _Static_assert(sizeof(void *) == sizeof(uint64_t), "the route hash scans 64-bit pointers");
    for (size_t r = 0; r < ROUTE_PURE_REGION_COUNT; ++r) {
        const unsigned char *p = (const unsigned char *)route_pure_regions[r].addr;
        size_t n = route_pure_regions[r].size, i = 0;
        while (i < n) {
            uint64_t word;
            long long where[2];
            if (((uintptr_t)(p + i) & 7u) == 0 && i + 8 <= n) {
                memcpy(&word, p + i, 8);
                if (pr_route_pointer(word, where)) h = pr_mix(h, where, sizeof(where));
                else h = pr_mix(h, &word, 8);
                i += 8;
            } else {
                h = pr_mix(h, p + i, 1);
                ++i;
            }
        }
    }
    return h;
}

/* Simulation state only, by named field so struct padding never counts.
 * Route state is included: residency changes game results. Presentation state
 * (facing, walk bits, the death pool, audio snapshot) is not. */
static uint64_t pr_world_hash(void)
{
    uint64_t h = 1469598103934665603ull;
    for (int i = 0; i < MAX_UNITS; ++i) {
        const Unit *q = &u[i];
        PR_FIELD(q->alive); PR_FIELD(q->x); PR_FIELD(q->y); PR_FIELD(q->hp); PR_FIELD(q->maxhp);
        PR_FIELD(q->owner); PR_FIELD(q->type); PR_FIELD(q->selected); PR_FIELD(q->cd); PR_FIELD(q->order);
        PR_FIELD(q->tx); PR_FIELD(q->ty); PR_FIELD(q->carry); PR_FIELD(q->build); PR_FIELD(q->work_timer);
        PR_FIELD(q->armor); PR_FIELD(q->weapon); PR_FIELD(q->target); PR_FIELD(q->path_timer);
        PR_FIELD(q->path_retry); PR_FIELD(q->path_x); PR_FIELD(q->path_y);
        PR_FIELD(q->shield); PR_FIELD(q->raw_type); PR_FIELD(q->raw_owner); PR_FIELD(q->flags);
        PR_FIELD(q->shield_remainder);
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i) {
        const Building *q = &b[i];
        PR_FIELD(q->alive); PR_FIELD(q->x); PR_FIELD(q->y); PR_FIELD(q->hp); PR_FIELD(q->maxhp);
        PR_FIELD(q->owner); PR_FIELD(q->type);
        for (int k = 0; k < 5; ++k) PR_FIELD(q->queue[k]);
        PR_FIELD(q->queue_head); PR_FIELD(q->queue_count); PR_FIELD(q->queue_progress);
        PR_FIELD(q->build_progress); PR_FIELD(q->under_construction); PR_FIELD(q->level);
        PR_FIELD(q->shield); PR_FIELD(q->raw_type); PR_FIELD(q->raw_owner); PR_FIELD(q->flags);
        PR_FIELD(q->larva); PR_FIELD(q->larva_timer); PR_FIELD(q->shield_remainder);
    }
    for (int i = 0; i < MAX_MINERALS; ++i) { PR_FIELD(m[i].x); PR_FIELD(m[i].y); PR_FIELD(m[i].amount); }
#ifdef SC64_IMPACT_EVENTS
    /* Delayed impacts: live rows only, in queue order, so a projectile whose
     * target or timer differs shows up before it lands. (RACE_IMPACT_CAPACITY
     * is an enum constant, so it cannot guard this.) */
    PR_FIELD(race_impact_count); PR_FIELD(race_impact_deferred);
    for (int i = 0; i < race_impact_count && i < RACE_IMPACT_CAPACITY; ++i)
        h = pr_mix(h, &race_impact_rows[i], sizeof(race_impact_rows[i]));
#endif
#if defined(SC64_BACKWATER) || defined(SC64_UNIFIED)
    h = pr_mix(h, invincible_units, sizeof(invincible_units));
    h = pr_mix(h, invincible_buildings, sizeof(invincible_buildings));
    PR_FIELD(backwater.executed); PR_FIELD(backwater.switches);
    for (int i = 0; i < 15; ++i) { PR_FIELD(backwater.timer[i]); PR_FIELD(backwater.phase[i]); }
    PR_FIELD(backwater.outcome); PR_FIELD(backwater.objective);
    PR_FIELD(backwater.spawn_trigger); PR_FIELD(backwater.spawn_remaining);
#endif
    h = pr_mix(h, visible, sizeof(visible));
    h = pr_mix(h, explored, sizeof(explored));
    for (int o = 0; o < 5; ++o) { PR_FIELD(race_player[o]); PR_FIELD(race_minerals[o]); PR_FIELD(race_gas[o]); }
    PR_FIELD(weapons_level); PR_FIELD(armor_level);
    PR_FIELD(weapons_research); PR_FIELD(armor_research);
    PR_FIELD(minerals); PR_FIELD(gas); PR_FIELD(supply_used); PR_FIELD(supply_cap);
    PR_FIELD(frame); PR_FIELD(screen); PR_FIELD(won); PR_FIELD(lost); PR_FIELD(seed);
    PR_FIELD(selected_building);
    PR_FIELD(race_campaign.elapsed_frames); PR_FIELD(race_campaign.objective_flags);
    PR_FIELD(race_campaign.phase); PR_FIELD(race_campaign.mission);
    PR_FIELD(race_campaign.objective_stage); PR_FIELD(race_campaign.rescue_unit);
    PR_FIELD(race_campaign.chrysalis_unit); PR_FIELD(race_campaign.carrier_unit);
    PR_FIELD(race_campaign.enemy_units_sent); PR_FIELD(race_campaign.enemy_wave);
    PR_FIELD(race_selected_mission); PR_FIELD(race_mode);
    PR_FIELD(pr_route_hash());
    return h;
}

/* A regression run in every gate process: both world and immediate chain
 * must change for either invincibility array, and restoring the bit restores
 * the hash. The test runs after launch, before any scripted command. */
static int pr_hash_selftest(void)
{
#if defined(SC64_BACKWATER) || defined(SC64_UNIFIED)
    uint64_t before = pr_world_hash(), old_chain = pr_mix(1469598103934665603ull, &before, sizeof(before));
    u8 *flags[] = { &invincible_units[0], &invincible_buildings[0] };
    for (unsigned i = 0; i < sizeof(flags) / sizeof(flags[0]); ++i) {
        *flags[i] ^= 1;
        uint64_t changed = pr_world_hash();
        uint64_t chain = pr_mix(1469598103934665603ull, &changed, sizeof(changed));
        *flags[i] ^= 1;
        if (changed == before || chain == old_chain || pr_world_hash() != before) return 0;
    }
#endif
    return 1;
}

/* ---- Runner state ----------------------------------------------------- */

/* pr_checks_on: route-purity hashing around every update and dispatch. The
 * gates always run with it; --no-route-checks is for throughput only, since
 * the hashing (byte-wise over every route region) costs more than a step. */
static int pr_render = 1, pr_chain_on = 1, pr_checks_on = 1;
static unsigned long pr_host_ticks, pr_steps;
static u16 pr_prev_held;
static uint64_t pr_chain = 1469598103934665603ull, pr_route_after;
static int pr_route_valid;
static unsigned long pr_route_checks;
#ifdef SC64_PARITY_TRACE
static FILE *pr_trace;
#endif

static void pr_route_check(const char *where)
{
    if (!pr_route_valid || !pr_checks_on) return;
    ++pr_route_checks;
    if (route_purity_hash() != pr_route_after) {
        fprintf(stderr, "parity runner: route state changed %s (host tick %lu)\n", where, pr_host_ticks);
        abort();
    }
}

/* One game_update() as common/main.c runs it, then one draw. */
static void pr_update(u16 held)
{
    int gate_open = 1, playing = screen == SCREEN_PLAY, frame_before = (int)frame;
#ifdef SC64_TEXT_BOXES
    gate_open = !sc64_text_box_count() && !sc64_text_box_outcome.result;
#endif
    pr_route_check("between updates");
    game_audio_host_held = held; /* the observer's input edges, as main.c gives them */
    game_update(held, (u16)(held & (u16)~pr_prev_held));
    if (pr_render) game_render();
    pr_prev_held = held;
    ++pr_host_ticks;
    if (gate_open && playing && (int)frame != frame_before) {
        ++pr_steps;
        if (pr_chain_on) {
            uint64_t w = pr_world_hash();
            pr_chain = pr_mix(pr_chain, &w, sizeof(w));
        }
        if (pr_checks_on) pr_route_after = route_purity_hash();
        pr_route_valid = 1;
#ifdef SC64_PARITY_TRACE
        sc64_pt_step(pr_steps, pr_host_ticks);
#endif
    } else {
        if (pr_checks_on) pr_route_after = route_purity_hash();
        pr_route_valid = 1;
#ifdef SC64_PARITY_TRACE
        sc64_pt_wait(pr_host_ticks);
#endif
    }
    pr_route_check("after the trace");
}

static void pr_press(u16 key) { pr_update(key); pr_update(0); }

/* ---- Queries scenarios use (read-only) -------------------------------- */

static int pr_nth_unit(int owner, int type, int n)
{
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == owner && u[i].type == type && !n--) return i;
    return -1;
}

static int pr_first_building(int owner, int type)
{
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == owner && b[i].type == type) return i;
    return -1;
}

static int pr_nearest_unit(int x, int y, int owner_not, int type)
{
    int best = -1, best_d = 0x7fffffff;
    for (int i = 0; i < MAX_UNITS; ++i) {
        int d;
        if (!u[i].alive || u[i].owner == owner_not || (type >= 0 && u[i].type != type)) continue;
        d = dist2(x, y, u[i].x, u[i].y);
        if (d < best_d) { best = i; best_d = d; }
    }
    return best;
}

static int pr_nearest_patch(int x, int y)
{
    int best = -1, best_d = 0x7fffffff;
    for (int i = 0; i < MAX_MINERALS; ++i) {
        int d;
        if (m[i].amount <= 0) continue;
        d = dist2(x, y, m[i].x, m[i].y);
        if (d < best_d) { best = i; best_d = d; }
    }
    return best;
}

static int pr_clamp_x(int x) { return x < 8 ? 8 : x >= WORLD_W - 8 ? WORLD_W - 8 : x; }
static int pr_clamp_y(int y) { return y < 8 ? 8 : y >= WORLD_H - 8 ? WORLD_H - 8 : y; }

/* A site the product's own admission accepts, on rings around a point. Pure:
 * race_build_request_valid() only checks; the route gate holds it to that. */
static int pr_find_site(int owner, int worker, int type, int cx0, int cy0, int *sx, int *sy)
{
    uint64_t route = route_purity_hash();
    for (int ring = 2; ring <= 12; ++ring)
        for (int k = 0; k < 8 * ring; ++k) {
            int side = k / (2 * ring), t = k % (2 * ring) - ring;
            int dx = side == 0 ? t : side == 1 ? ring : side == 2 ? -t : -ring;
            int dy = side == 0 ? -ring : side == 1 ? t : side == 2 ? ring : -t;
            int x = pr_clamp_x(cx0 + dx * 16), y = pr_clamp_y(cy0 + dy * 16);
            if (race_build_request_valid(owner, worker, type, &x, &y, 0)) {
                if (route_purity_hash() != route) { fputs("parity runner: site search changed routes\n", stderr); abort(); }
                *sx = x; *sy = y;
                return 1;
            }
        }
    if (route_purity_hash() != route) { fputs("parity runner: site search changed routes\n", stderr); abort(); }
    return 0;
}

/* A queued action that waits for the bank, with a deadline. */
typedef struct { int armed, done; unsigned long from, until; int minerals; } PrPending;

static int pr_due(PrPending *p, unsigned long step, int owner)
{
    if (!p->armed || p->done || step < p->from) return 0;
    if (race_owner_minerals(owner) >= p->minerals) return 1;
    if (step >= p->until) {
        sc64_pi_fail("scenario", "bank never reached %d minerals by step %lu", p->minerals, p->until);
        p->done = 1;
    }
    return 0;
}

/* ---- Scenarios -------------------------------------------------------- */

enum { PR_GROUP_CAP = 12 };
static int pr_group_n, pr_group_x[PR_GROUP_CAP], pr_group_y[PR_GROUP_CAP], pr_group_slot[PR_GROUP_CAP];
static int pr_target_x = -1, pr_target_y = -1;
static unsigned long pr_at_step = 5, pr_command_step;

static struct {
    int a, b2, spawned, enemy, worker0, worker1, worker2, producer;
    PrPending train, build;
    int gas_builder, gas_worker, gas_started, gas_assigned, gas_picked_up, gas_delivered, gas_bank;
} sc;

static int pr_launch_t1(void)
{
    game_init();
    pr_press(KEY_A);
#ifndef SC64_CAMPAIGN_RACE
    if (race_front.page != SC_FRONT_CAMPAIGN) return 0;
    pr_press(KEY_A);
#endif
    if (race_front.page != SC_FRONT_MISSION) return 0;
    pr_press(KEY_A);
    if (race_front.page != SC_FRONT_BRIEFING || race_front.mission != 4) return 0;
    pr_press(KEY_START);
    return screen == SCREEN_PLAY && race_selected_mission == 4;
}

static int pr_launch_skirmish(int human, int bot_race)
{
    game_init();
    pr_press(KEY_DOWN);
    pr_press(KEY_A);
    if (screen != SCREEN_TITLE || race_front.page != SC_FRONT_SKIRMISH) return 0;
    while (race_front.row > 0) pr_press(KEY_UP);
    for (int n = 0; race_front.map != 1 && n <= SC64_SKIRMISH_IMPORTED_COUNT; ++n) pr_press(KEY_RIGHT);
    if (race_front.map != 1) return 0;
    pr_press(KEY_DOWN);
    for (int n = 0; race_front.race != human && n < 3; ++n) pr_press(KEY_RIGHT);
    if (race_front.race != human) return 0;
    pr_press(KEY_DOWN);
    for (int n = 0; race_front.bots != 1 && n < 3; ++n) pr_press(KEY_RIGHT);
    if (race_front.bots != 1) return 0;
    pr_press(KEY_DOWN);
    for (int n = 0; race_front.bot_races[0] != bot_race && n < 5; ++n) pr_press(KEY_RIGHT);
    if (race_front.bot_races[0] != bot_race) return 0;
    while (race_front.row < SC_FRONT_SKIRMISH_LAUNCH_ROW) pr_press(KEY_DOWN);
    pr_press(KEY_A);
    return screen == SCREEN_PLAY && race_mode && race_human == human && race_bots == 1;
}

/* T1: Marines, SCVs and the Command Center the mission hands the player. */
static int pr_setup_t1(void)
{
    sc.a = pr_nth_unit(SIDE, U_MARINE, 0);
    sc.b2 = pr_nth_unit(SIDE, U_MARINE, 1);
    sc.producer = pr_first_building(SIDE, B_COMMAND);
    if (sc.a < 0 || sc.b2 < 0 || pr_nth_unit(SIDE, U_SCV, 0) < 0 || sc.producer < 0) {
        fprintf(stderr, "t1: opening lacks the expected Marines, SCVs or Command Center\n");
        return 0;
    }
    /* T1 starts the SCVs far from the Command Center and grants 40 minerals,
     * so the scenario spawns its own miners at the base (step 2). */
    sc.worker0 = sc.worker1 = sc.worker2 = -1;
    sc.train = (PrPending){ 1, 0, 45, 2800, 50 };
    sc.build = (PrPending){ 1, 0, 60, 2800, 100 };
    return 1;
}

/* Scenarios run before each step; the first they see is step 2, because the
 * update that releases the launch key is already step 1. */
static void pr_scenario_t1(unsigned long step)
{
    int z;
    switch (step) {
    case 2: {
        const Building *cc = &b[sc.producer];
        sc.worker0 = sc64_pi_spawn(SIDE, U_SCV, pr_clamp_x(cc->x - 16), pr_clamp_y(cc->y + 40));
        sc.worker1 = sc64_pi_spawn(SIDE, U_SCV, cc->x, pr_clamp_y(cc->y + 40));
        sc.worker2 = sc64_pi_spawn(SIDE, U_SCV, pr_clamp_x(cc->x + 16), pr_clamp_y(cc->y + 40));
        break;
    }
    case 5:
        sc.spawned = sc64_pi_spawn(SIDE, U_MARINE, pr_clamp_x(u[sc.a].x + 12), u[sc.a].y);
        break;
    case 15:
        z = pr_nearest_unit(u[sc.a].x, u[sc.a].y, SIDE, U_ZERGLING);
        sc64_pi_move(sc.a, pr_clamp_x(u[sc.a].x + (z >= 0 && u[z].x < u[sc.a].x ? -32 : 32)), u[sc.a].y);
        break;
    case 20: sc64_pi_move(sc.b2, pr_clamp_x(u[sc.b2].x + 16), pr_clamp_y(u[sc.b2].y + 16)); break;
    case 30:
        sc64_pi_harvest_minerals(sc.worker0, pr_nearest_patch(u[sc.worker0].x, u[sc.worker0].y));
        sc64_pi_harvest_minerals(sc.worker1, pr_nearest_patch(u[sc.worker1].x, u[sc.worker1].y));
        sc64_pi_harvest_minerals(sc.worker2, pr_nearest_patch(u[sc.worker2].x, u[sc.worker2].y));
        break;
    case 40: sc64_pi_stop(sc.b2); break;
    case 120:
        z = pr_nearest_unit(u[sc.spawned].x, u[sc.spawned].y, SIDE, U_ZERGLING);
        if (z < 0) sc64_pi_fail("scenario", "no Zergling to attack");
        else sc64_pi_attack(sc.spawned, z);
        break;
    case 150:
        z = pr_nearest_unit(u[sc.a].x, u[sc.a].y, SIDE, U_ZERGLING);
        if (z < 0) sc64_pi_fail("scenario", "no Zergling to attack-move toward");
        else sc64_pi_attack_move(sc.a, u[z].x, u[z].y);
        break;
    default: break;
    }
    if (pr_due(&sc.train, step, SIDE)) { sc64_pi_train(SIDE, sc.producer, U_SCV); sc.train.done = 1; }
    if (pr_due(&sc.build, step, SIDE)) {
        int x, y;
        if (pr_find_site(SIDE, sc.worker2, B_DEPOT, b[sc.producer].x, b[sc.producer].y, &x, &y))
            sc64_pi_build(SIDE, sc.worker2, B_DEPOT, x, y);
        else sc64_pi_fail("scenario", "no Supply Depot site near the Command Center");
        sc.build.done = 1;
    }
}

/* Blood Bath, human Zerg against one Terran computer. */
static int pr_setup_skirmish(void)
{
    int hall = pr_first_building(SIDE, B_HATCHERY);
    sc.worker0 = pr_nth_unit(SIDE, U_DRONE, 0);
    sc.worker1 = pr_nth_unit(SIDE, U_DRONE, 1);
    sc.worker2 = pr_nth_unit(SIDE, U_DRONE, 2);
    sc.producer = hall;
    if (hall < 0 || sc.worker2 < 0) {
        fprintf(stderr, "skirmish: opening lacks the Hatchery or three Drones\n");
        return 0;
    }
    sc.train = (PrPending){ 1, 0, 30, 2400, 50 };
    sc.build = (PrPending){ 1, 0, 60, 2800, 200 };
    return 1;
}

static void pr_scenario_skirmish(unsigned long step)
{
    const Building *hall = &b[sc.producer];
    switch (step) {
    case 2: /* step 1 is the launch's own key release */
        sc.a = sc64_pi_spawn(SIDE, U_ZERGLING, pr_clamp_x(hall->x + 40), pr_clamp_y(hall->y + 40));
        sc.b2 = sc64_pi_spawn(SIDE, U_ZERGLING, pr_clamp_x(hall->x + 48), pr_clamp_y(hall->y + 40));
        sc.enemy = sc64_pi_spawn(race_bot_owner_ids[0], U_MARINE, pr_clamp_x(hall->x + 120), pr_clamp_y(hall->y + 40));
        sc.gas_builder = sc64_pi_spawn(SIDE, U_DRONE, pr_clamp_x(hall->x - 32), pr_clamp_y(hall->y + 40));
        sc.gas_worker = sc64_pi_spawn(SIDE, U_DRONE, pr_clamp_x(hall->x - 48), pr_clamp_y(hall->y + 40));
        break;
    case 10: sc64_pi_move(sc.a, pr_clamp_x(u[sc.a].x + 24), u[sc.a].y); break;
    case 20: sc64_pi_harvest_minerals(sc.worker0, pr_nearest_patch(u[sc.worker0].x, u[sc.worker0].y)); break;
    case 40: sc64_pi_attack(sc.b2, sc.enemy); break;
    case 45: sc64_pi_move(sc.worker1, pr_clamp_x(u[sc.worker1].x + 16), u[sc.worker1].y); break;
    case 50: sc64_pi_stop(sc.worker1); break;
    case 60:
        if (u[sc.enemy].alive) sc64_pi_attack_move(sc.a, u[sc.enemy].x, u[sc.enemy].y);
        else sc64_pi_attack_move(sc.a, pr_clamp_x(hall->x + 120), pr_clamp_y(hall->y + 40));
        break;
    default: break;
    }
    if (step >= 70 && !sc.gas_started && race_owner_minerals(SIDE) >= 50) {
        int best = -1, distance = 0x7fffffff;
        for (int i = 0; i < geyser_count; ++i) {
            int x = geysers[i].x, y = geysers[i].y;
            int d = dist2(hall->x, hall->y, x, y);
            if (d < distance && race_build_request_valid(SIDE, sc.gas_builder, B_EXTRACTOR, &x, &y, 0)) {
                best = i; distance = d;
            }
        }
        if (best < 0) sc64_pi_fail("gas", "no usable geyser near the Hatchery");
        else sc64_pi_build(SIDE, sc.gas_builder, B_EXTRACTOR, geysers[best].x, geysers[best].y);
        sc.gas_started = 1;
    }
    if (sc.gas_started && !sc.gas_assigned) {
        int refinery = pr_first_building(SIDE, B_EXTRACTOR);
        if (refinery >= 0 && building_complete(refinery)) {
            sc.gas_bank = race_owner_gas(SIDE);
            if (sc64_pi_harvest_gas(sc.gas_worker, refinery) == 0) sc.gas_assigned = 1;
        }
    }
    if (sc.gas_assigned && u[sc.gas_worker].alive && u[sc.gas_worker].order == ORDER_GAS) {
        if (u[sc.gas_worker].carry > 0) sc.gas_picked_up = 1;
        if (sc.gas_picked_up && !u[sc.gas_worker].carry && race_owner_gas(SIDE) > sc.gas_bank)
            sc.gas_delivered = 1;
    }
    if (pr_due(&sc.train, step, SIDE)) { sc64_pi_train(SIDE, sc.producer, U_DRONE); sc.train.done = 1; }
    if (pr_due(&sc.build, step, SIDE)) {
        int x, y;
        if (pr_find_site(SIDE, sc.worker2, B_SPAWNING_POOL, hall->x, hall->y, &x, &y))
            sc64_pi_build(SIDE, sc.worker2, B_SPAWNING_POOL, x, y);
        else sc64_pi_fail("scenario", "no Spawning Pool site near the Hatchery");
        sc.build.done = 1;
    }
}

/* move: join each --unit point to the one human Marine standing exactly there
 * (the N64 position halved), then one group order at --at. */
static int pr_setup_move(void)
{
    for (int k = 0; k < pr_group_n; ++k) {
        int found = -1;
        if (pr_group_x[k] >= WORLD_W || pr_group_y[k] >= WORLD_H) {
            fprintf(stderr, "move: point %d,%d is off the map\n", pr_group_x[k], pr_group_y[k]);
            return 0;
        }
        for (int i = 0; i < MAX_UNITS; ++i)
            if (u[i].alive && u[i].owner == SIDE && u[i].type == U_MARINE &&
                u[i].x == pr_group_x[k] && u[i].y == pr_group_y[k]) {
                if (found >= 0) { fprintf(stderr, "move: two Marines at %d,%d\n", pr_group_x[k], pr_group_y[k]); return 0; }
                found = i;
            }
        if (found < 0) {
            int near = pr_nearest_unit(pr_group_x[k], pr_group_y[k], -1, U_MARINE);
            fprintf(stderr, "move: no human Marine at %d,%d (nearest: slot %d at %d,%d)\n", pr_group_x[k], pr_group_y[k],
                    near, near >= 0 ? u[near].x : -1, near >= 0 ? u[near].y : -1);
            return 0;
        }
        pr_group_slot[k] = found;
    }
    return pr_group_n > 0 && pr_target_x >= 0;
}

static void pr_scenario_move(unsigned long step)
{
    if (step != pr_at_step) return;
    pr_command_step = step;
    sc64_pi_group_move(pr_group_slot, pr_group_n, pr_target_x, pr_target_y);
}

/* income: the human's Drones as the skirmish starts them. */
static int pr_setup_income(void)
{
    pr_group_n = 0;
    for (int i = 0; i < MAX_UNITS && pr_group_n < PR_GROUP_CAP; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].type == U_DRONE) pr_group_slot[pr_group_n++] = i;
    if (pr_group_n != 4) { fprintf(stderr, "income: the human starts with %d Drones, not 4\n", pr_group_n); return 0; }
    return 1;
}

static void pr_scenario_income(unsigned long step)
{
    (void)step; /* the human stays passive */
}

static int pr_parse_point(const char *text, int *x, int *y)
{
    char *end;
    const char *second;
    long a = strtol(text, &end, 10), c;
    if (end == text || *end != ',') return 0;
    second = end + 1;
    c = strtol(second, &end, 10);
    if (end == second || *end || a < 0 || c < 0 || a >= 65536 || c >= 65536) return 0;
    *x = (int)a; *y = (int)c;
    return 1;
}

static double pr_clock(clockid_t id)
{
    struct timespec t;
    clock_gettime(id, &t);
    return (double)t.tv_sec + (double)t.tv_nsec * 1e-9;
}
/* Throughput is reported against process CPU time: wall time on a shared,
 * loaded host mostly measures the other processes. */
static double pr_now(void) { return pr_clock(CLOCK_MONOTONIC); }
static double pr_cpu(void) { return pr_clock(CLOCK_PROCESS_CPUTIME_ID); }

int main(int argc, char **argv)
{
    const char *scenario = 0, *trace_path = 0, *profile = "unnamed";
    unsigned long limit = 3000;
    int skirmish, kind;
    double t0, t1, c0, c1;
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--scenario") && i + 1 < argc) scenario = argv[++i];
        else if (!strcmp(argv[i], "--steps") && i + 1 < argc) limit = strtoul(argv[++i], 0, 10);
        else if (!strcmp(argv[i], "--trace") && i + 1 < argc) trace_path = argv[++i];
        else if (!strcmp(argv[i], "--profile") && i + 1 < argc) profile = argv[++i];
        else if (!strcmp(argv[i], "--no-render")) pr_render = 0;
        else if (!strcmp(argv[i], "--no-chain")) pr_chain_on = 0;
        else if (!strcmp(argv[i], "--no-route-checks")) pr_checks_on = 0;
        else if (!strcmp(argv[i], "--unit") && i + 1 < argc && pr_group_n < PR_GROUP_CAP &&
                 pr_parse_point(argv[i + 1], &pr_group_x[pr_group_n], &pr_group_y[pr_group_n])) { ++pr_group_n; ++i; }
        else if (!strcmp(argv[i], "--target") && i + 1 < argc && pr_parse_point(argv[i + 1], &pr_target_x, &pr_target_y)) ++i;
        else if (!strcmp(argv[i], "--at") && i + 1 < argc && (pr_at_step = strtoul(argv[++i], 0, 10)) >= 2) {}
        else { scenario = 0; break; }
    }
    for (const char *c = profile; *c; ++c)
        if (!((*c >= 'a' && *c <= 'z') || (*c >= '0' && *c <= '9') || (*c == '-' && c != profile))) {
            scenario = 0; /* the name goes into JSON and the trace header unescaped */
            break;
        }
    /* kind: 0 t1, 1 skirmish, 2 move, 3 income */
    kind = !scenario ? -1 : !strcmp(scenario, "t1") ? 0 : !strcmp(scenario, "skirmish") ? 1 :
           !strcmp(scenario, "move") ? 2 : !strcmp(scenario, "income") ? 3 : -1;
    if (kind < 0 || !limit || (kind == 2 && (!pr_group_n || pr_target_x < 0))) {
        fprintf(stderr, "usage: %s --scenario t1|skirmish [--steps N] [--trace PATH] [--no-render] [--no-chain]\n"
                "       [--no-route-checks] [--profile NAME]\n"
                "       %s --scenario move --unit X,Y [--unit X,Y ...] --target X,Y [--at STEP] ...\n"
                "       %s --scenario income [--at STEP] ...\n", argv[0], argv[0], argv[0]);
        return 64;
    }
    skirmish = kind == 1 || kind == 3;
#ifdef SC64_PARITY_TRACE
    sc64_pt_route_checks = pr_checks_on;
    if (trace_path) {
        pr_trace = fopen(trace_path, "w");
        if (!pr_trace) { perror(trace_path); return 66; }
        sc64_pt_open(pr_trace, profile, scenario, sc64_pi_attack_mode());
        sc64_pi_on_verb = sc64_pt_verb;
    }
#else
    if (trace_path) { fprintf(stderr, "this build has no trace emitter (SC64_PARITY_TRACE)\n"); return 64; }
#endif
    t0 = pr_now(); c0 = pr_cpu();
    if (!(skirmish ? pr_launch_skirmish(R_ZERG, R_TER) : pr_launch_t1())) {
        fprintf(stderr, "%s: menu launch failed (screen=%d page=%d)\n", scenario, screen, race_front.page);
        return 66;
    }
    if (!pr_hash_selftest()) { fputs("parity world-hash mutation regression failed\n", stderr); return 67; }
    {
        uint64_t route = route_purity_hash();
        if (!(kind == 0 ? pr_setup_t1() : kind == 1 ? pr_setup_skirmish() : kind == 2 ? pr_setup_move() : pr_setup_income()))
            return 66;
        ++pr_route_checks;
        if (route_purity_hash() != route) { fputs("parity runner: scenario setup changed route state\n", stderr); abort(); }
    }
    unsigned long dispatched = 0;
    /* A modal text box (SC64_TEXT_BOX_HOST_MODAL builds) takes no step and the
     * runner presses nothing, so bound host updates as well as steps. */
    while (pr_steps < limit && screen == SCREEN_PLAY && !sc64_pi_status.failures) {
        if (pr_host_ticks > 4ul * limit + 10000ul) {
            fprintf(stderr, "%s: no simulation step for too long (host tick %lu, step %lu)\n",
                    scenario, pr_host_ticks, pr_steps);
            return 4;
        }
        /* Once per step: an update that takes no step must not repeat it.
         * Route state must be exactly what the dispatch's verbs left:
         * unchanged, or advanced by a verified retirement. */
        if (dispatched != pr_steps + 1) {
            dispatched = pr_steps + 1;
            if (pr_checks_on) sc64_pi_route_expect_begin();
            if (kind == 0) pr_scenario_t1(dispatched);
            else if (kind == 1) pr_scenario_skirmish(dispatched);
            else if (kind == 2) pr_scenario_move(dispatched);
            else pr_scenario_income(dispatched);
            if (pr_checks_on) {
                ++pr_route_checks;
                sc64_pi_route_expect_end("during the scenario");
                pr_route_after = route_purity_hash();
            }
        }
        pr_route_valid = 1;
        pr_update(0);
    }
    t1 = pr_now(); c1 = pr_cpu();
    if (getenv("SC64_PARITY_REGION_HASHES"))
        for (size_t r = 0; r < ROUTE_PURE_REGION_COUNT; ++r)
            fprintf(stderr, "route region %zu size %zu hash %016llx\n", r, route_pure_regions[r].size,
                    (unsigned long long)pr_mix(1469598103934665603ull, route_pure_regions[r].addr,
                                               route_pure_regions[r].size));
    if (kind == 1 && !sc64_pi_status.failures && !sc.gas_delivered)
        sc64_pi_fail("gas", "no gas delivery by step %lu (started=%d assigned=%d)", pr_steps, sc.gas_started, sc.gas_assigned);
    /* Every scripted command must have been issued, not left waiting. */
    if (kind == 2 && !sc64_pi_status.failures && !pr_command_step)
        sc64_pi_fail("scenario", "run ended at step %lu before the command step %lu", pr_steps, pr_at_step);
    if (!sc64_pi_status.failures && ((sc.train.armed && !sc.train.done) || (sc.build.armed && !sc.build.done)))
        sc64_pi_fail("scenario", "run ended at step %lu before the train or build command was issued", pr_steps);
    {
        uint64_t world = pr_world_hash();
        char movers[PR_GROUP_CAP * 8 + 1] = "";
        /* The command goes in before step pr_command_step, so the last state
         * without it is that of the step before. */
        unsigned long command_sf = pr_command_step ? (pr_command_step - 1) * 4ul / 5ul : 0;
        for (int k = 0; kind >= 2 && k < pr_group_n; ++k)
            snprintf(movers + strlen(movers), sizeof(movers) - strlen(movers), "%s%d", k ? "," : "", pr_group_slot[k]);
#ifdef SC64_PARITY_TRACE
        if (pr_trace) {
            int bad = sc64_pt_close(pr_steps, pr_host_ticks, world, pr_chain) != 0;
            if (fclose(pr_trace) || bad) { fprintf(stderr, "%s: trace write failed\n", trace_path); return 74; }
        }
#endif
        printf("{\"profile\":\"%s\",\"scenario\":\"%s\",\"trace_compiled\":%d,\"render\":%d,"
               "\"steps\":%lu,\"source_frames\":%lu,\"host_ticks\":%lu,\"frame\":%d,\"screen\":%d,"
               "\"won\":%d,\"lost\":%d,\"world\":\"%016llx\",\"world_chain\":\"%016llx\","
               "\"route_checks_on\":%d,\"route_checks\":%lu,\"verb_route_checks\":%u,\"dispatch_route_checks\":%u,"
               "\"spawn_route_retirements\":%u,"
               "\"gas_assigned\":%d,\"gas_pickup\":%d,\"gas_delivered\":%d,"
               "\"verbs\":%u,\"setup_failures\":%u,\"command_step\":%lu,\"command_sf\":%lu,\"human_owner\":%d,\"movers\":[%s],"
               "\"first_failure\":\"%s\",\"attack_mode\":\"%s\",\"wall_seconds\":%.6f,\"cpu_seconds\":%.6f,"
               "\"updates_per_cpu_second\":%.1f}\n",
               profile, scenario,
#ifdef SC64_PARITY_TRACE
               1,
#else
               0,
#endif
               pr_render, pr_steps, pr_steps * 4ul / 5ul, pr_host_ticks, (int)frame, screen, won, lost,
               (unsigned long long)world, (unsigned long long)pr_chain,
               pr_checks_on, pr_route_checks, sc64_pi_status.route_checks, sc64_pi_status.dispatch_checks,
               sc64_pi_status.route_retirements,
               sc.gas_assigned, sc.gas_picked_up, sc.gas_delivered,
               sc64_pi_status.issued,
               sc64_pi_status.failures, pr_command_step, command_sf, SIDE, movers,
               sc64_pi_status.first_failure, sc64_pi_attack_mode(),
               t1 - t0, c1 - c0, c1 > c0 ? (double)pr_host_ticks / (c1 - c0) : 0.0);
    }
    return sc64_pi_status.failures ? 3 : 0;
}
