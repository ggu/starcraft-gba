/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_RACE_SKIRMISH_AI_H
#define SC64_RACE_SKIRMISH_AI_H

/* Computer opponents for canonical custom games (race_mode).
 *
 * This is an authored planner, not a decoded StarCraft 64 AI script. The
 * private SC64 script catalog does list the stock melee entries (TMCu, ZMCu,
 * PMCu and the Brood War TMCx, ZMCx, PMCx), but their program semantics are
 * unaudited, so no build order, wave timing or composition is taken from
 * them. Costs, build times, producers, prerequisites, addons and morphs come
 * from the engine's rule tables and the raw-ID skirmish catalog, and every
 * action goes through the same paid services as the human:
 * race_start_build (pays on arrival), race_queue_unit (pays on queue),
 * race_skirmish_start_record (addon/morph), and ordinary unit orders.
 * No resources, units, or construction progress are granted.
 *
 * The planner keeps no persistent state of its own. Every decision is derived
 * from saved actor storage, owner banks and the frame counter, so a saved
 * match continues deterministically without a new save field. The one reused
 * field is a pending builder's work_timer, which counts this owner's planning
 * passes while it walks (pending builders otherwise leave it unused); the
 * existing codecs already store work_timer for every unit.
 *
 * Work is bounded per update: at most one opponent plans on a given frame
 * (bots are staggered by RACE_AI_PHASE), each pass is a fixed number of linear
 * scans over the unit, building and mineral pools, and placement evaluates
 * at most RACE_AI_SITE_CHECKS expensive candidates.
 *
 * Knowledge: the engine has no per-owner fog, so opponents read hostile actor
 * positions and composition directly (an omniscient approximation). The
 * early worker scout is cosmetic and is not used as a knowledge source.
 */

enum {
    RACE_AI_PHASE = 16,                 /* frames between two bots' passes */
    RACE_AI_PERIOD = 48,                /* frames between one bot's passes */
    RACE_AI_SITE_CHECKS = 8,
    RACE_AI_MAX_DEPOTS = 4,
    RACE_AI_MAX_REFINERIES = 4,
    RACE_AI_WORKER_CAP = 36,
    RACE_AI_UNIT_LIMIT = 56,
    RACE_AI_STALL_PASSES = 40,          /* ~64 s for a builder to arrive */
    RACE_AI_THREAT_RANGE = 320,
    RACE_AI_DEPLOYED_RANGE = 576
};

/* Tracked raw IDs. 0 means untracked; otherwise key + 1. Unit and building
 * raw IDs do not overlap in the ordinary roster. */
enum {
    AK_MARINE, AK_FIREBAT, AK_VULTURE, AK_GOLIATH, AK_TANK, AK_WRAITH, AK_SCV,
    AK_ZERGLING, AK_HYDRALISK, AK_MUTALISK, AK_SCOURGE, AK_OVERLORD, AK_DRONE,
    AK_ZEALOT, AK_DRAGOON, AK_SCOUT, AK_PROBE,
    AK_COMMAND, AK_DEPOT, AK_REFINERY, AK_BARRACKS, AK_ACADEMY, AK_FACTORY,
    AK_STARPORT, AK_MACHINE_SHOP, AK_ENG_BAY, AK_ARMORY, AK_TURRET,
    AK_HATCHERY, AK_LAIR, AK_HIVE, AK_POOL, AK_DEN, AK_EVO, AK_SPIRE,
    AK_CREEP, AK_SUNKEN, AK_SPORE, AK_EXTRACTOR,
    AK_NEXUS, AK_PYLON, AK_ASSIMILATOR, AK_GATEWAY, AK_CORE, AK_FORGE,
    AK_CANNON, AK_STARGATE,
    AK_COUNT
};

static const unsigned char race_ai_raw_key[256] = {
    [0] = AK_MARINE + 1, [32] = AK_FIREBAT + 1, [2] = AK_VULTURE + 1,
    [3] = AK_GOLIATH + 1, [5] = AK_TANK + 1, [8] = AK_WRAITH + 1, [7] = AK_SCV + 1,
    [37] = AK_ZERGLING + 1, [38] = AK_HYDRALISK + 1, [43] = AK_MUTALISK + 1,
    [47] = AK_SCOURGE + 1, [42] = AK_OVERLORD + 1, [41] = AK_DRONE + 1,
    [65] = AK_ZEALOT + 1, [66] = AK_DRAGOON + 1, [70] = AK_SCOUT + 1, [64] = AK_PROBE + 1,
    [106] = AK_COMMAND + 1, [109] = AK_DEPOT + 1, [110] = AK_REFINERY + 1,
    [111] = AK_BARRACKS + 1, [112] = AK_ACADEMY + 1, [113] = AK_FACTORY + 1,
    [114] = AK_STARPORT + 1, [120] = AK_MACHINE_SHOP + 1, [122] = AK_ENG_BAY + 1,
    [123] = AK_ARMORY + 1, [124] = AK_TURRET + 1,
    [131] = AK_HATCHERY + 1, [132] = AK_LAIR + 1, [133] = AK_HIVE + 1,
    [142] = AK_POOL + 1, [135] = AK_DEN + 1, [139] = AK_EVO + 1, [141] = AK_SPIRE + 1,
    [143] = AK_CREEP + 1, [146] = AK_SUNKEN + 1, [144] = AK_SPORE + 1,
    [149] = AK_EXTRACTOR + 1,
    [154] = AK_NEXUS + 1, [156] = AK_PYLON + 1, [157] = AK_ASSIMILATOR + 1,
    [160] = AK_GATEWAY + 1, [164] = AK_CORE + 1, [166] = AK_FORGE + 1,
    [162] = AK_CANNON + 1, [167] = AK_STARGATE + 1,
};

typedef struct {
    int owner, race, base, worker_type, worker_raw;
    int home, hx, hy;
    int depot[RACE_AI_MAX_DEPOTS], depots;
    int refinery[RACE_AI_MAX_REFINERIES], refinery_load[RACE_AI_MAX_REFINERIES], refineries;
    int workers, miners, gas_workers, idle_workers, pending_builders;
    int army, army_supply, home_army, deployed, deployed_supply, units, buildings;
    int producers, larva, pending_supply, patches, queued_units, free_supply, site_searches;
    int supply_used, supply_cap, supply_queued;
    unsigned char lane_patch[16], lane_patches, lane_geyser[2], lane_geysers;
    int enemy_air, enemy_army, enemy_air_tech, enemy_defense;
    int threat, threat_x, threat_y, threat_d, threat_count;
    int reserve_minerals, reserve_gas;
    unsigned char have[AK_COUNT], pend[AK_COUNT];
} RaceAiCensus;

static int race_ai_key(int raw)
{
    return raw >= 0 && raw < 256 ? (int)race_ai_raw_key[raw] - 1 : -1;
}

static int race_ai_count(const RaceAiCensus *c, int key)
{
    return c->have[key] + c->pend[key];
}

static int race_ai_owner_base(int owner)
{
    return owner == SIDE ? 0 : owner == ENEMY ? 1 : owner == RESCUABLE ? 2 : 3;
}

static int race_ai_is_depot(int type)
{
    return type == B_COMMAND || type == B_NEXUS || sc64_race_is_zerg_townhall(type);
}

static int race_ai_armed(int type)
{
    return sc64_race_unit_damage(type) > 0 || sc64_race_unit_can_attack_air(type);
}

/* Unit pool share. The human keeps the same share as each opponent, and
 * no opponent exceeds RACE_AI_UNIT_LIMIT, bounding combat and route work. */
static int race_ai_unit_cap(void)
{
    int cap = (MAX_UNITS - 16) / (race_bots + 1);
    return cap > RACE_AI_UNIT_LIMIT ? RACE_AI_UNIT_LIMIT : cap;
}

static int race_ai_building_cap(void)
{
    return (MAX_BUILDINGS - 8) / (race_bots + 1);
}

static void __attribute__((noinline)) race_ai_census(int owner, RaceAiCensus *c)
{
    int i, best_threat = RACE_AI_THREAT_RANGE * RACE_AI_THREAT_RANGE;
    memset(c, 0, sizeof *c);
    c->owner = owner;
    c->race = race_owner_identity(owner);
    c->base = race_ai_owner_base(owner);
    c->worker_type = c->race == R_ZERG ? U_DRONE : c->race == R_PROTOSS ? U_PROBE : U_SCV;
    c->worker_raw = sc64_race_raw_unit(c->worker_type);
    c->threat = -1;
    for (i = 0; i < MAX_BUILDINGS; ++i) {
        const Building *q = &b[i];
        int key;
        if (!q->alive) continue;
        if (q->owner != owner) {
            int raw;
            if (!owners_hostile(owner, q->owner)) continue;
            raw = sc64_race_raw_building(q->type);
            if (raw == 124 || raw == 125 || raw == 144 || raw == 146 || raw == 162)
                ++c->enemy_defense;
            else if (raw == 114 || raw == 141 || raw == 137 || raw == 167)
                ++c->enemy_air_tech;
            continue;
        }
        ++c->buildings;
        /* A Creep Colony morphing into a Sunken Colony counts as the
         * pending Sunken (race_construction_deferred_morph). */
        key = race_ai_key(sc64_race_raw_building(race_construction_deferred_morph(q) >= 0 ?
                                                 race_construction_deferred_morph(q) : q->type));
        if (q->under_construction &&
            !(race_mode && (q->type == B_LAIR || q->type == B_HIVE))) {
            if (key >= 0 && c->pend[key] < 255) ++c->pend[key];
            if (race_ai_is_depot(q->type) || q->type == B_DEPOT || q->type == B_PYLON)
                ++c->pending_supply;
            continue;
        }
        if (key >= 0 && c->have[key] < 255) ++c->have[key];
        {
            const Sc64RaceBuildingRule *br = sc64_race_building_rule(q->type);
            if (br) c->supply_cap += br->supply_provided;
        }
        if (race_ai_is_depot(q->type) && c->depots < RACE_AI_MAX_DEPOTS)
            c->depot[c->depots++] = i;
        if ((q->type == B_REFINERY || q->type == B_EXTRACTOR || q->type == B_ASSIMILATOR) &&
            c->refineries < RACE_AI_MAX_REFINERIES)
            c->refinery[c->refineries++] = i;
        if (sc64_race_is_zerg_townhall(q->type)) c->larva += q->larva;
        if (q->type == B_BARRACKS || q->type == B_FACTORY || q->type == B_STARPORT ||
            q->type == B_GATEWAY || q->type == B_STARGATE || race_ai_is_depot(q->type))
            ++c->producers;
        for (int j = 0; j < q->queue_count; ++j) {
            int queued = q->queue[(q->queue_head + j) % 5];
            int qk = race_ai_key(sc64_race_raw_unit(queued));
            if (qk >= 0 && c->pend[qk] < 255) ++c->pend[qk];
            if (queued == U_OVERLORD) ++c->pending_supply;
            c->queued_units += sc64_race_product_count(queued);
            {
                const Sc64RaceUnitRule *qr = sc64_race_unit_rule(queued);
                if (qr) c->supply_queued += qr->supply * sc64_race_product_count(queued);
            }
        }
    }
    /* The first completed depot in slot order receives cargo (the engine's
     * owner_command_center rule). It is not called here so that its
     * inlining into the hot order loop is unaffected. */
    c->home = c->depots ? c->depot[0] : -1;
    c->hx = c->home >= 0 ? b[c->home].x : race_base_x[c->base];
    c->hy = c->home >= 0 ? b[c->home].y : race_base_y[c->base];
    for (i = 0; i < MAX_MINERALS; ++i) {
        if (m[i].amount <= 0) continue;
        if (c->lane_patches < 16 && dist2(m[i].x, m[i].y, c->hx, c->hy) <= 288 * 288)
            c->lane_patch[c->lane_patches++] = (unsigned char)i;
        for (int k = 0; k < c->depots; ++k)
            if (dist2(m[i].x, m[i].y, b[c->depot[k]].x, b[c->depot[k]].y) < 288 * 288) {
                ++c->patches; break;
            }
    }
    for (i = 0; i < geyser_count && c->lane_geysers < 2; ++i)
        if (dist2(geysers[i].x, geysers[i].y, c->hx, c->hy) <= 288 * 288)
            c->lane_geyser[c->lane_geysers++] = (unsigned char)i;
    for (i = 0; i < MAX_UNITS; ++i) {
        const Unit *q = &u[i];
        if (!q->alive) continue;
        if (SC64_TRANSPORT_LOADED(q)) {
            /* Loaded passengers still count against supply. */
            if (q->owner == owner) {
                const Sc64RaceUnitRule *ur = sc64_race_unit_rule(q->type);
                if (ur) c->supply_used += ur->supply;
                c->supply_cap += sc64_race_unit_supply_provided(q->type);
            }
            continue;
        }
        if (q->owner != owner) {
            int flying, d;
            if (!owners_hostile(owner, q->owner) || !race_ai_armed(q->type)) continue;
            flying = sc64_race_unit_flying(q->type);
            if (!sc64_race_is_worker(q->type)) {
                ++c->enemy_army;
                if (flying) ++c->enemy_air;
            }
            /* Threat: the hostile armed actor nearest any owned depot. */
            for (int k = 0; k < c->depots || (k == 0 && !c->depots); ++k) {
                int ax = c->depots ? b[c->depot[k]].x : c->hx;
                int ay = c->depots ? b[c->depot[k]].y : c->hy;
                d = dist2(q->x, q->y, ax, ay);
                if (d < RACE_AI_THREAT_RANGE * RACE_AI_THREAT_RANGE &&
                    !sc64_race_is_worker(q->type)) ++c->threat_count;
                if (d < best_threat) {
                    best_threat = d; c->threat = i;
                    c->threat_x = q->x; c->threat_y = q->y;
                }
            }
            continue;
        }
        ++c->units;
        {
            int key = race_ai_key(sc64_race_raw_unit(q->type));
            const Sc64RaceUnitRule *ur = sc64_race_unit_rule(q->type);
            if (key >= 0 && c->have[key] < 255) ++c->have[key];
#ifdef SC64_Z7_EGG_PRODUCTION
            if (ur) c->supply_used += unit_slot_supply(q);
#else
            if (ur) c->supply_used += ur->supply;
#endif
            c->supply_cap += sc64_race_unit_supply_provided(q->type);
        }
        if (sc64_race_is_worker(q->type)) {
            ++c->workers;
            if (q->order == ORDER_MINE) ++c->miners;
            else if (q->order == ORDER_GAS) {
                ++c->gas_workers;
                for (int k = 0; k < c->refineries; ++k)
                    if (q->target == c->refinery[k]) ++c->refinery_load[k];
            } else if (q->order == ORDER_IDLE) ++c->idle_workers;
            else if (q->order == ORDER_BUILD_PENDING) {
                const Sc64RaceBuildingRule *r = sc64_race_building_rule(q->target);
                int key = race_ai_key(sc64_race_raw_building(q->target));
                ++c->pending_builders;
                if (key >= 0 && c->pend[key] < 255) ++c->pend[key];
                if (r) { c->reserve_minerals += r->minerals; c->reserve_gas += r->gas; }
                if (q->target == B_DEPOT || q->target == B_PYLON || race_ai_is_depot(q->target))
                    ++c->pending_supply;
            }
        } else if (race_ai_armed(q->type)) {
            const Sc64RaceUnitRule *r = sc64_race_unit_rule(q->type);
            int supply = r ? r->supply : 0;
            ++c->army; c->army_supply += supply;
            if (dist2(q->x, q->y, c->hx, c->hy) > RACE_AI_DEPLOYED_RANGE * RACE_AI_DEPLOYED_RANGE) {
                ++c->deployed; c->deployed_supply += supply;
            } else ++c->home_army;
        }
    }
    c->threat_d = best_threat;
    if (c->threat_count == 0) c->threat = -1;
}

/* ---------------------------------------------------------------- economy */

static int race_ai_affordable(const RaceAiCensus *c, int minerals_cost, int gas_cost)
{
    return race_owner_minerals(c->owner) - c->reserve_minerals >= minerals_cost &&
           race_owner_gas(c->owner) - c->reserve_gas >= gas_cost;
}

static int race_ai_worker_target(const RaceAiCensus *c)
{
    /* Two per local mineral patch, three per refinery, plus a builder. */
    int target = c->patches * 2 + c->refineries * 3 + 1;
    int cap = race_ai_unit_cap() / 2;
    if (target < 8) target = 8;
    if (target > cap) target = cap;
    return target > RACE_AI_WORKER_CAP ? RACE_AI_WORKER_CAP : target;
}

/* Send an idle worker back to the least-loaded patch near its nearest owned
 * depot, or to the nearest patch anywhere if that base is mined out. The
 * engine returns cargo to the owner's first completed depot. */
static void __attribute__((noinline)) race_ai_recover_workers(RaceAiCensus *c)
{
    unsigned char load[MAX_MINERALS];
    int moved = 0;
    if (!c->idle_workers || !c->depots) return;
    memset(load, 0, sizeof load);
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == c->owner && u[i].order == ORDER_MINE &&
            u[i].target >= 0 && u[i].target < MAX_MINERALS && load[u[i].target] < 255)
            ++load[u[i].target];
    for (int i = 0; i < MAX_UNITS && moved < 6; ++i) {
        Unit *q = &u[i];
        int depot = -1, depot_d = 0x7fffffff, best = -1, best_score = 0x7fffffff;
        if (!q->alive || q->owner != c->owner || q->order != ORDER_IDLE ||
            !sc64_race_is_worker(q->type) || SC64_TRANSPORT_LOADED(q)) continue;
        for (int k = 0; k < c->depots; ++k) {
            int d = dist2(q->x, q->y, b[c->depot[k]].x, b[c->depot[k]].y);
            if (d < depot_d) { depot_d = d; depot = c->depot[k]; }
        }
        for (int p = 0; p < MAX_MINERALS; ++p) {
            int d, score;
            if (m[p].amount <= 0) continue;
            d = dist2(m[p].x, m[p].y, b[depot].x, b[depot].y);
            /* Local patches first, least loaded first; then distance. */
            score = (d > 288 * 288 ? 1 << 26 : 0) + load[p] * (1 << 18) + (d >> 6);
            if (score < best_score) { best_score = score; best = p; }
        }
        if (best < 0) return;
        q->order = ORDER_MINE; q->target = best; q->work_timer = 0; q->carry = 0;
        if (load[best] < 255) ++load[best];
        --c->idle_workers; ++c->miners; ++moved;
    }
}

static int race_ai_nearest_worker(const RaceAiCensus *c, int order, int x, int y)
{
    int best = -1, best_d = 0x7fffffff;
    for (int i = 0; i < MAX_UNITS; ++i) {
        int d;
        if (!u[i].alive || u[i].owner != c->owner || !sc64_race_is_worker(u[i].type) ||
            u[i].order != order || SC64_TRANSPORT_LOADED(&u[i])) continue;
        d = dist2(x, y, u[i].x, u[i].y);
        if (d < best_d) { best_d = d; best = i; }
    }
    return best;
}

static void race_ai_balance_gas(RaceAiCensus *c)
{
    int want = 0, light = -1, light_load = 99;
    if (!c->refineries) return;
    if (c->workers >= 9) want = c->refineries * 3;
    if (want > c->workers - 7) want = c->workers - 7;
    /* Rebalance a large gas surplus toward minerals. */
    if (race_owner_gas(c->owner) > 600 &&
        race_owner_gas(c->owner) > 3 * race_owner_minerals(c->owner)) want -= c->refineries;
    if (want < 0) want = 0;
    for (int k = 0; k < c->refineries; ++k)
        if (c->refinery_load[k] < light_load) { light_load = c->refinery_load[k]; light = k; }
    if (c->gas_workers < want && light >= 0 && light_load < 3) {
        int ref = c->refinery[light];
        int w = race_ai_nearest_worker(c, ORDER_MINE, b[ref].x, b[ref].y);
        if (w < 0) w = race_ai_nearest_worker(c, ORDER_IDLE, b[ref].x, b[ref].y);
        if (w >= 0) {
            u[w].order = ORDER_GAS; u[w].target = ref; u[w].carry = 0; u[w].work_timer = 0;
            ++c->gas_workers; ++c->refinery_load[light];
        }
    } else if (c->gas_workers > want + 1) {
        int w = race_ai_nearest_worker(c, ORDER_GAS, c->hx, c->hy);
        if (w >= 0) { u[w].order = ORDER_IDLE; u[w].target = -1; u[w].carry = 0; }
    }
}

/* Recovery of construction work: count planning passes for walking
 * builders and release one that cannot arrive; give an orphaned Terran
 * foundation (its SCV died or was redirected) a new SCV. */
static void __attribute__((noinline)) race_ai_recover_construction(RaceAiCensus *c)
{
    for (int i = 0; i < MAX_UNITS; ++i) {
        Unit *q = &u[i];
        if (!q->alive || q->owner != c->owner || q->order != ORDER_BUILD_PENDING) continue;
        if (q->work_timer < RACE_AI_STALL_PASSES) ++q->work_timer;
        else { q->order = ORDER_IDLE; q->target = -1; q->work_timer = 0; ++c->idle_workers; }
    }
    if (c->race != R_TER) return;
    for (int i = 0; i < MAX_BUILDINGS; ++i) {
        const Sc64SkirmishRecord *record;
        int builder = 0, w;
        if (!b[i].alive || b[i].owner != c->owner || !b[i].under_construction) continue;
        record = race_skirmish_build_record(c->owner, sc64_race_raw_building(b[i].type));
        if (!record || record->kind != SC64_SK_WORKER_BUILD) continue;
        for (int j = 0; j < MAX_UNITS && !builder; ++j)
            if (u[j].alive && u[j].owner == c->owner && u[j].order == ORDER_BUILD && u[j].target == i)
                builder = 1;
        if (builder) continue;
        w = race_ai_nearest_worker(c, ORDER_MINE, b[i].x, b[i].y);
        if (w < 0) w = race_ai_nearest_worker(c, ORDER_IDLE, b[i].x, b[i].y);
        if (w < 0) return;
        u[w].order = ORDER_BUILD; u[w].target = i; u[w].carry = 0;
        u[w].path_timer = u[w].path_retry = 0;
        return; /* one per pass */
    }
}

/* Whether a structure of `type` at (bx,by) would block path tile (col,row),
 * mirroring build_path_grid's footprint for this profile. */
static int race_ai_would_block(int type, int bx, int by, int col, int row)
{
#ifdef SC64_ZP_CAMPAIGN_ACTORS
    int left, up, right, down;
    if (sc64_game_building_bounds_source(type, &left, &up, &right, &down)) {
        int x0 = bx * 2 - left, x1 = bx * 2 + right, y0 = by * 2 - up, y1 = by * 2 + down;
        return col * 32 <= x1 && col * 32 + 32 >= x0 && row * 32 <= y1 && row * 32 + 32 >= y0;
    }
#else
    (void)type;
#endif
    return dist2(col * 16 + 8, row * 16 + 8, bx, by) < 18 * 18;
}

/* Products appear at the producer's fixed exit (race_tick_buildings). An
 * addon and neighbors can enclose that tile on the path grid, and a ground
 * unit trained there could never leave. A bounded flood over the 9x9 tiles
 * around the exit must reach the window's edge, optionally treating a
 * planned structure (extra_type at ex,ey) as already built. */
static int __attribute__((noinline))
race_ai_exit_escapes(int producer, int extra_type, int extra_x, int extra_y)
{
    unsigned char queue[81], seen[81];
    int head = 0, tail = 0, x = b[producer].x + 20, y = b[producer].y + 4, c0, r0;
    if (x >= WORLD_W - 8) x = b[producer].x - 20;
    c0 = x / 16 - 4; r0 = y / 16 - 4;
    memset(seen, 0, sizeof seen);
    queue[tail++] = 40; seen[40] = 1;
    while (head < tail) {
        int cell = queue[head++], cc = cell % 9, rr = cell / 9;
        int from = (c0 + cc) + (r0 + rr) * MAP_COLS;
        if (cc == 0 || rr == 0 || cc == 8 || rr == 8) return 1;
        for (int k = 0; k < 4; ++k) {
            int nc = cc + (k == 0) - (k == 1), nr = rr + (k == 2) - (k == 3), n = nr * 9 + nc;
            if (seen[n]) continue;
            seen[n] = 1;
            if (!is_traversable(c0 + nc, r0 + nr, from)) continue;
            if (extra_type >= 0 && race_ai_would_block(extra_type, extra_x, extra_y, c0 + nc, r0 + nr))
                continue;
            queue[tail++] = (unsigned char)n;
        }
    }
    return 0;
}

/* -------------------------------------------------------------- placement */

static const signed char race_ai_dir[16][2] = {
    {64,0},{59,24},{45,45},{24,59},{0,64},{-24,59},{-45,45},{-59,24},
    {-64,0},{-59,-24},{-45,-45},{-24,-59},{0,-64},{24,-59},{45,-45},{59,-24}
};

/* Squared distance from (px,py) to segment (ax,ay)-(bx,by), in pixels. */
static int race_ai_segment_d2(int px, int py, int ax, int ay, int bx, int by)
{
    int vx = bx - ax, vy = by - ay, wx = px - ax, wy = py - ay;
    int len = vx * vx + vy * vy, t = vx * wx + vy * wy;
    if (len <= 0 || t <= 0) return wx * wx + wy * wy;
    if (t >= len) return dist2(px, py, bx, by);
    {
        int cx0 = ax + vx * t / len, cy0 = ay + vy * t / len;
        return dist2(px, py, cx0, cy0);
    }
}

/* Cheap rejection: keep harvesting lanes, map edges and producer addon
 * spots clear before asking the placement and admission services. */
static int race_ai_site_lane_clear(const RaceAiCensus *c, int x, int y)
{
    if (x < 24 || y < 24 || x >= WORLD_W - 24 || y >= WORLD_H - 24) return 0;
    for (int k = 0; k < c->lane_patches; ++k) {
        const Mineral *p = &m[c->lane_patch[k]];
        if (race_ai_segment_d2(x, y, c->hx, c->hy, p->x, p->y) < 30 * 30) return 0;
    }
    for (int k = 0; k < c->lane_geysers; ++k) {
        int g = c->lane_geyser[k];
        if (race_ai_segment_d2(x, y, c->hx, c->hy, geysers[g].x, geysers[g].y) < 40 * 40)
            return 0;
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i) {
        if (!b[i].alive) continue;
        /* Ring neighbors can be ~38 px apart; packed structures wall in
         * workers and products. Keep a walkable gap between structures. */
        if (dist2(x, y, b[i].x, b[i].y) < 52 * 52) return 0;
        if (b[i].owner != c->owner) continue;
        if ((b[i].type == B_FACTORY || b[i].type == B_STARPORT) &&
            dist2(x, y, b[i].x + 48, b[i].y + 8) < 40 * 40) return 0;
        if ((b[i].type == B_BARRACKS || b[i].type == B_FACTORY || b[i].type == B_STARPORT ||
             b[i].type == B_GATEWAY || b[i].type == B_STARGATE || race_ai_is_depot(b[i].type)) &&
            (dist2(x, y, b[i].x + 20, b[i].y - 24) < 40 * 40 ||
             dist2(x, y, b[i].x + 20, b[i].y + 32) < 40 * 40)) return 0;
    }
    return 1;
}

/* Products appear at the producer's fixed exit (x + 20, y + 4); a Terran
 * addon later occupies x + 48. Keep a vertical lane through the exit open
 * so units can leave a producer boxed in by its addon and neighbors. */
static int race_ai_exit_clear(int type, int x, int y)
{
    static const signed char lane[4] = { -28, -44, 32, 48 };
    int ex = x + 20, ey = y + 4;
    if (!(type == B_BARRACKS || type == B_FACTORY || type == B_STARPORT ||
          type == B_GATEWAY || type == B_STARGATE || race_ai_is_depot(type))) return 1;
    /* Both lanes, two points each, inside the map margin, on open terrain
     * and clear of every structure. */
    if (ex < 32 || ex >= WORLD_W - 32) return 0;
    for (int k = 0; k < 4; ++k) {
        int py = ey + lane[k];
        if (py < 32 || py >= WORLD_H - 32 || terrain_blocked(ex / 16, py / 16)) return 0;
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i) {
        int dx = b[i].x - ex;
        if (!b[i].alive || dx >= 36 || dx <= -36) continue;
        for (int k = 0; k < 4; ++k)
            if (dist2(ex, ey + lane[k], b[i].x, b[i].y) < 36 * 36) return 0;
    }
    return 1;
}

/* Lattice rings around (ax,ay). Only one pair of rings is examined per pass,
 * rotating with the frame counter, and at most RACE_AI_SITE_CHECKS
 * candidates reach the expensive checks. front_first orders each ring from
 * the direction of the map center. Only one search runs per pass and it
 * examines a single ring, rotating with the pass number. */
static int __attribute__((noinline))
race_ai_find_site(RaceAiCensus *c, int type, int ax, int ay, int min_ring,
                  int max_ring, int front_first, int *out_x, int *out_y)
{
    static const unsigned char radius[6] = { 64, 96, 128, 160, 192, 224 };
    int rings = max_ring - min_ring + 1, pass = frame / RACE_AI_PERIOD, checks = 0;
    int start_ring, front = 0, best_dot = -0x7fffffff;
    int mx = WORLD_W / 2 - ax, my = WORLD_H / 2 - ay;
    /* One placement search per pass bounds the per-update cost. */
    if (rings <= 0 || c->site_searches++) return 0;
    for (int k = 0; k < 16; ++k) {
        int dot = race_ai_dir[k][0] * mx + race_ai_dir[k][1] * my;
        if (dot > best_dot) { best_dot = dot; front = k; }
    }
    if (!front_first) front = (front + 8) & 15;
    start_ring = min_ring + pass % rings;
    for (int ring = start_ring; ring <= start_ring; ++ring) {
        for (int step = 0; step < 16; ++step) {
            /* front, front+1, front-1, front+2, ... */
            int k = (front + ((step & 1) ? (step + 1) / 2 : -(step / 2)) + 16) & 15;
            int x = ax + race_ai_dir[k][0] * radius[ring] / 64;
            int y = ay + race_ai_dir[k][1] * radius[ring] / 64;
            if (!race_ai_site_lane_clear(c, x, y) || !race_ai_exit_clear(type, x, y)) continue;
            if ((type == B_FACTORY || type == B_STARPORT) &&
                !can_place(B_DEPOT, x + 48, y + 8)) continue;
            if (++checks > RACE_AI_SITE_CHECKS) return 0;
            if (race_can_site(c->owner, type, x, y) && can_place(type, x, y)) {
                *out_x = x; *out_y = y; return 1;
            }
        }
    }
    return 0;
}

static int race_ai_start_build(RaceAiCensus *c, int type, int x, int y)
{
    int worker = race_owner_worker(c->owner, c->worker_type, x, y);
    if (worker < 0 || !race_start_build(c->owner, worker, type, x, y)) return 0;
    u[worker].work_timer = 0;
    {
        const Sc64RaceBuildingRule *r = sc64_race_building_rule(type);
        int key = race_ai_key(sc64_race_raw_building(type));
        if (r) { c->reserve_minerals += r->minerals; c->reserve_gas += r->gas; }
        if (key >= 0) ++c->pend[key];
    }
    return 1;
}

static int race_ai_build_gas(RaceAiCensus *c, int dry)
{
    int type = c->race == R_ZERG ? B_EXTRACTOR : c->race == R_PROTOSS ? B_ASSIMILATOR : B_REFINERY;
    for (int k = 0; k < c->depots; ++k) {
        int depot = c->depot[k], best = -1, best_d = 224 * 224;
        for (int g = 0; g < geyser_count; ++g) {
            int d = dist2(geysers[g].x, geysers[g].y, b[depot].x, b[depot].y);
            if (d < best_d && race_can_site(c->owner, type, geysers[g].x, geysers[g].y) &&
                can_place(type, geysers[g].x, geysers[g].y)) { best_d = d; best = g; }
        }
        if (best >= 0 && (dry || race_ai_start_build(c, type, geysers[best].x, geysers[best].y)))
            return 1;
    }
    return 0;
}

/* A free start location with minerals left, nearest the owner's base. */
static int race_ai_expansion_site(const RaceAiCensus *c, int *x, int *y)
{
    int best = -1, best_d = 0x7fffffff;
    for (int s = 0; s < 4; ++s) {
        int sx = race_base_x[s], sy = race_base_y[s], occupied = 0, ore = 0, d;
        if (sx <= 0 || sy <= 0) continue;
        for (int i = 0; i < MAX_BUILDINGS && !occupied; ++i)
            if (b[i].alive && dist2(b[i].x, b[i].y, sx, sy) < 192 * 192) occupied = 1;
        if (occupied) continue;
        for (int p = 0; p < MAX_MINERALS; ++p)
            if (m[p].amount > 0 && dist2(m[p].x, m[p].y, sx, sy) < 256 * 256) ore += m[p].amount;
        if (ore < 1500) continue;
        d = dist2(sx, sy, c->hx, c->hy);
        if (d < best_d) { best_d = d; best = s; }
    }
    if (best < 0) return 0;
    *x = race_base_x[best]; *y = race_base_y[best];
    return 1;
}

static int race_ai_home_ore(const RaceAiCensus *c)
{
    int ore = 0;
    for (int k = 0; k < c->depots; ++k)
        for (int p = 0; p < MAX_MINERALS; ++p)
            if (m[p].amount > 0 && dist2(m[p].x, m[p].y, b[c->depot[k]].x, b[c->depot[k]].y) < 288 * 288)
                ore += m[p].amount;
    return ore;
}

/* --------------------------------------------------------- supply, tech */

static int race_ai_supply(RaceAiCensus *c)
{
    /* Census totals follow race_owner_supply_cap and
     * race_owner_reserved_supply; race_queue_unit still checks admission. */
    int cap = c->supply_cap;
    int free_supply = cap - c->supply_used - c->supply_queued;
    c->free_supply = free_supply;
    int margin = 4 + 4 * c->producers, x, y;
    if (margin > 24) margin = 24;
    if (cap >= 400 || free_supply >= margin) return 0;
    if (c->pending_supply >= (cap >= 64 ? 2 : 1)) return 0;
    if (c->race == R_ZERG) {
        for (int k = 0; k < c->depots; ++k)
            if (b[c->depot[k]].larva > 0 && race_queue_unit(c->owner, c->depot[k], U_OVERLORD)) {
                ++c->pending_supply; return 1;
            }
        /* Reserve the Overlord price until a larva is available. */
        c->reserve_minerals += sc64_race_unit_rule(U_OVERLORD)->minerals;
        return 0;
    }
    {
        int type = c->race == R_PROTOSS ? B_PYLON : B_DEPOT;
        const Sc64RaceBuildingRule *r = sc64_race_building_rule(type);
        if (!race_ai_affordable(c, r->minerals, r->gas)) {
            c->reserve_minerals += r->minerals; return 0;
        }
        if (race_ai_find_site(c, type, c->hx, c->hy, c->race == R_PROTOSS ? 1 : 0, 5, 0, &x, &y) &&
            race_ai_start_build(c, type, x, y)) { ++c->pending_supply; return 1; }
    }
    return 0;
}

enum {
    RACE_AI_STEP_BUILD = 1, RACE_AI_STEP_ADDON = 2, RACE_AI_STEP_MORPH = 3,
    RACE_AI_STEP_GAS = 4, RACE_AI_STEP_HALL = 5, RACE_AI_STEP_DEFENSE = 6,
    RACE_AI_IF_AIR = 1, RACE_AI_IF_LATE = 2, RACE_AI_IF_RICH = 4
};

/* Ordered development plans. raw is the stock raw ID; count is the wanted
 * total for this owner; workers is the minimum worker count. These are
 * authored opening/tech orders over working engine products, not decoded
 * script programs. Missing engine capabilities are omitted (see
 * docs/SKIRMISH-AI.md). */
typedef struct { unsigned char kind, raw, count, workers, flags; } RaceAiStep;

static const RaceAiStep race_ai_terran_plan[] = {
    { RACE_AI_STEP_BUILD, 111, 1, 9, 0 },        /* Barracks */
    { RACE_AI_STEP_GAS,   110, 1, 11, 0 },       /* Refinery */
    { RACE_AI_STEP_BUILD, 111, 2, 13, 0 },
    { RACE_AI_STEP_BUILD, 113, 1, 14, 0 },       /* Factory */
    { RACE_AI_STEP_ADDON, 120, 1, 14, 0 },       /* Machine Shop */
    { RACE_AI_STEP_BUILD, 112, 1, 15, 0 },       /* Academy: Firebats */
    { RACE_AI_STEP_BUILD, 122, 1, 15, RACE_AI_IF_AIR }, /* Engineering Bay */
    { RACE_AI_STEP_DEFENSE, 124, 2, 15, RACE_AI_IF_AIR }, /* Missile Turrets */
    { RACE_AI_STEP_BUILD, 123, 1, 16, RACE_AI_IF_AIR }, /* Armory: Goliaths */
    { RACE_AI_STEP_HALL,  106, 2, 14, 0 },       /* expansion when ore runs low */
    { RACE_AI_STEP_BUILD, 114, 1, 15, 0 },       /* Starport: Wraiths */
    { RACE_AI_STEP_BUILD, 123, 1, 16, RACE_AI_IF_LATE },
    { RACE_AI_STEP_BUILD, 113, 2, 16, RACE_AI_IF_RICH },
    { RACE_AI_STEP_ADDON, 120, 2, 16, RACE_AI_IF_RICH },
    { RACE_AI_STEP_GAS,   110, 2, 16, 0 },
    { RACE_AI_STEP_BUILD, 111, 3, 16, RACE_AI_IF_RICH },
    { RACE_AI_STEP_BUILD, 122, 1, 16, RACE_AI_IF_LATE },
    { RACE_AI_STEP_DEFENSE, 124, 1, 16, RACE_AI_IF_LATE },
};

/* A Hatchery's larva queue is serial in these products, so Zerg production
 * capacity comes from additional Hatcheries rather than from bank size. */
static const RaceAiStep race_ai_zerg_plan[] = {
    { RACE_AI_STEP_BUILD, 142, 1, 9, 0 },        /* Spawning Pool */
    { RACE_AI_STEP_HALL,  131, 2, 9, 0 },        /* macro Hatchery / expansion */
    { RACE_AI_STEP_GAS,   149, 1, 10, 0 },       /* Extractor */
    { RACE_AI_STEP_HALL,  131, 3, 10, 0 },
    { RACE_AI_STEP_BUILD, 135, 1, 11, 0 },       /* Hydralisk Den */
    { RACE_AI_STEP_MORPH, 132, 1, 12, 0 },       /* Lair */
    { RACE_AI_STEP_BUILD, 139, 1, 12, RACE_AI_IF_AIR }, /* Evolution Chamber */
    { RACE_AI_STEP_DEFENSE, 144, 2, 12, RACE_AI_IF_AIR }, /* Spore Colonies */
    { RACE_AI_STEP_HALL,  131, 4, 12, RACE_AI_IF_RICH },
    { RACE_AI_STEP_BUILD, 141, 1, 12, 0 },       /* Spire */
    { RACE_AI_STEP_DEFENSE, 146, 1, 12, RACE_AI_IF_LATE }, /* Sunken Colony */
    { RACE_AI_STEP_GAS,   149, 2, 14, 0 },
    { RACE_AI_STEP_HALL,  131, 5, 14, RACE_AI_IF_RICH },
    { RACE_AI_STEP_HALL,  131, 6, 14, RACE_AI_IF_RICH | RACE_AI_IF_LATE },
    { RACE_AI_STEP_BUILD, 139, 1, 14, RACE_AI_IF_LATE },
    { RACE_AI_STEP_DEFENSE, 146, 2, 14, RACE_AI_IF_LATE },
};

static const RaceAiStep race_ai_protoss_plan[] = {
    { RACE_AI_STEP_BUILD, 160, 1, 9, 0 },        /* Gateway */
    { RACE_AI_STEP_GAS,   157, 1, 11, 0 },       /* Assimilator */
    { RACE_AI_STEP_BUILD, 164, 1, 12, 0 },       /* Cybernetics Core */
    { RACE_AI_STEP_BUILD, 160, 2, 13, 0 },
    { RACE_AI_STEP_BUILD, 166, 1, 15, RACE_AI_IF_AIR }, /* Forge */
    { RACE_AI_STEP_DEFENSE, 162, 2, 15, RACE_AI_IF_AIR }, /* Photon Cannons */
    { RACE_AI_STEP_HALL,  154, 2, 14, 0 },
    { RACE_AI_STEP_BUILD, 167, 1, 15, 0 },       /* Stargate: Scouts */
    { RACE_AI_STEP_BUILD, 160, 3, 15, RACE_AI_IF_RICH },
    { RACE_AI_STEP_GAS,   157, 2, 16, 0 },
    { RACE_AI_STEP_BUILD, 166, 1, 16, RACE_AI_IF_LATE },
    { RACE_AI_STEP_DEFENSE, 162, 1, 16, RACE_AI_IF_LATE },
    { RACE_AI_STEP_BUILD, 160, 4, 16, RACE_AI_IF_RICH },
};

static const Sc64SkirmishRecord *race_ai_record(const RaceAiCensus *c, int raw)
{
    return race_skirmish_build_record(c->owner, raw);
}

/* The first producer that can take this addon or morph now: ready for the
 * record, and for an addon a placeable footprint, a free building slot and
 * a unit exit the addon would not seal. Checked before any reservation, so
 * an infeasible addon never holds the owner's money. */
static int race_ai_attach_parent(const RaceAiCensus *c, const Sc64SkirmishRecord *r)
{
    int addon = r->kind == SC64_SK_ADDON ? sc64_race_building_from_raw(r->raw_id) : -1;
    if (r->kind == SC64_SK_ADDON && (addon < 0 || find_build_slot() < 0)) return -1;
    for (int i = 0; i < MAX_BUILDINGS; ++i) {
        if (!b[i].alive || b[i].owner != c->owner ||
            sc64_race_raw_building(b[i].type) != r->parent_raw ||
            !race_skirmish_record_ready(c->owner, i, r)) continue;
        if (addon >= 0 && (!can_place(addon, b[i].x + 48, b[i].y + 8) ||
                           !race_ai_exit_escapes(i, addon, b[i].x + 48, b[i].y + 8))) continue;
        return i;
    }
    return -1;
}

/* Start the addon or morph on a parent from race_ai_attach_parent. */
static int race_ai_start_attached(RaceAiCensus *c, const Sc64SkirmishRecord *r, int parent)
{
    if (parent < 0 || !race_skirmish_start_record(c->owner, parent, r)) return 0;
    {
        int key = race_ai_key(r->raw_id);
        if (key >= 0) ++c->pend[key];
    }
    return 1;
}

/* Defense structures face the map center. Zerg morph a Creep Colony. */
static int race_ai_start_defense(RaceAiCensus *c, int raw)
{
    int x, y, type = sc64_race_building_from_raw(raw);
    int ax = c->hx + (WORLD_W / 2 - c->hx) / 8, ay = c->hy + (WORLD_H / 2 - c->hy) / 8;
    if (type < 0) return 0;
    if (c->race == R_ZERG) {
        const Sc64SkirmishRecord *r = race_ai_record(c, raw);
        if (r && r->kind == SC64_SK_BUILDING_MORPH &&
            race_ai_start_attached(c, r, race_ai_attach_parent(c, r))) return 1;
        if (c->have[AK_CREEP] + c->pend[AK_CREEP]) return 0; /* wait for the colony */
        type = B_CREEP_COLONY;
        ax = c->hx; ay = c->hy;
    }
    if (!race_ai_find_site(c, type, ax, ay, 0, c->race == R_ZERG ? 3 : 2, 1, &x, &y)) return 0;
    return race_ai_start_build(c, type, x, y);
}

static int race_ai_hall(RaceAiCensus *c, int count)
{
    int x, y, type = c->race == R_ZERG ? B_HATCHERY : c->race == R_PROTOSS ? B_NEXUS : B_COMMAND;
    int halls = c->have[AK_COMMAND] + c->have[AK_NEXUS] + c->have[AK_HATCHERY] +
                c->pend[AK_COMMAND] + c->pend[AK_NEXUS] + c->pend[AK_HATCHERY] +
                c->have[AK_LAIR] + c->have[AK_HIVE];
    int ore = race_ai_home_ore(c);
    if (halls >= count && c->depots) return -1;
    /* Lost every depot: rebuild one at the owner's own start location, or
     * a free one, and wait for it rather than starting more. */
    if (!c->depots) {
        if (c->pend[AK_COMMAND] + c->pend[AK_NEXUS] + c->pend[AK_HATCHERY]) return 0;
        x = race_base_x[c->base]; y = race_base_y[c->base];
        if (can_place(type, x, y) || race_ai_expansion_site(c, &x, &y))
            return race_ai_start_build(c, type, x, y);
        return 0;
    }
    if (ore < 3000 && race_ai_expansion_site(c, &x, &y))
        return race_ai_start_build(c, type, x, y);
    /* Zerg: an additional Hatchery at home is a larva source. */
    if (c->race == R_ZERG && race_ai_find_site(c, type, c->hx, c->hy, 1, 3, 0, &x, &y))
        return race_ai_start_build(c, type, x, y);
    return -1;
}

/* One structure start per pass. An unaffordable eligible step reserves its
 * price so army production cannot starve the next tech step. */
static void __attribute__((noinline)) race_ai_develop(RaceAiCensus *c)
{
    const RaceAiStep *plan;
    int steps, worker_target = race_ai_worker_target(c);
    if (c->race == R_ZERG) { plan = race_ai_zerg_plan; steps = (int)(sizeof race_ai_zerg_plan / sizeof *plan); }
    else if (c->race == R_PROTOSS) { plan = race_ai_protoss_plan; steps = (int)(sizeof race_ai_protoss_plan / sizeof *plan); }
    else { plan = race_ai_terran_plan; steps = (int)(sizeof race_ai_terran_plan / sizeof *plan); }
    if (c->buildings >= race_ai_building_cap()) return;
    if (!c->depots) { (void)race_ai_hall(c, 1); return; }
    for (int s = 0; s < steps; ++s) {
        const RaceAiStep *step = &plan[s];
        const Sc64SkirmishRecord *r;
        const Sc64RaceBuildingRule *rule;
        int key = race_ai_key(step->raw), type, have, x, y, started = 0, parent = -1;
        /* Small mineral fields lower the worker gates with the target. */
        if (c->workers < step->workers && c->workers < worker_target - 1) continue;
        if ((step->flags & RACE_AI_IF_AIR) && !c->enemy_air && !c->enemy_air_tech) continue;
        if ((step->flags & RACE_AI_IF_LATE) && frame < 18000) continue;
        if ((step->flags & RACE_AI_IF_RICH) && race_owner_minerals(c->owner) < 600) continue;
        type = sc64_race_building_from_raw(step->raw);
        rule = type >= 0 ? sc64_race_building_rule(type) : 0;
        r = race_ai_record(c, step->raw);
        if (!rule || !r || key < 0) continue;
        if (step->kind == RACE_AI_STEP_HALL) {
            int result;
            if (!race_ai_affordable(c, rule->minerals, rule->gas)) {
                int halls = c->have[key] + c->pend[key] + c->have[AK_LAIR] + c->have[AK_HIVE];
                if (halls < step->count && (race_ai_home_ore(c) < 3000 || c->race == R_ZERG)) {
                    c->reserve_minerals += rule->minerals; return;
                }
                continue;
            }
            result = race_ai_hall(c, step->count);
            if (result > 0) return;
            continue;
        }
        have = race_ai_count(c, key);
        if (step->raw == 132) have += c->have[AK_HIVE];
        if (have >= step->count) continue;
        if (!race_skirmish_requirements(c->owner, r)) continue;
        if ((step->kind == RACE_AI_STEP_ADDON || step->kind == RACE_AI_STEP_MORPH) &&
            (parent = race_ai_attach_parent(c, r)) < 0) continue;
        if (step->kind == RACE_AI_STEP_GAS && !race_ai_build_gas(c, 1)) continue;
        if (!race_ai_affordable(c, rule->minerals, rule->gas)) {
            /* Reserve only when the gas side can eventually be met. */
            if (!rule->gas || c->refineries) {
                c->reserve_minerals += rule->minerals; c->reserve_gas += rule->gas;
            }
            return;
        }
        switch (step->kind) {
        case RACE_AI_STEP_BUILD:
            if (race_ai_find_site(c, type, c->hx, c->hy, 0, c->race == R_ZERG ? 3 : 5, 0, &x, &y))
                started = race_ai_start_build(c, type, x, y);
            break;
        case RACE_AI_STEP_GAS: started = race_ai_build_gas(c, 0); break;
        case RACE_AI_STEP_ADDON:
        case RACE_AI_STEP_MORPH: started = race_ai_start_attached(c, r, parent); break;
        case RACE_AI_STEP_DEFENSE: started = race_ai_start_defense(c, step->raw); break;
        }
        if (started) return;
    }
}

/* ------------------------------------------------------------ production */

static int race_ai_try_queue(RaceAiCensus *c, int producer, int raw, int honor_reserve)
{
    int type = sc64_race_unit_from_raw(raw), key;
    const Sc64RaceUnitRule *r = type >= 0 ? sc64_race_unit_rule(type) : 0;
    if (!r) return 0;
    if (!sc64_race_unit_flying(type) && !race_ai_exit_escapes(producer, -1, 0, 0)) return 0;
    /* Skip the queue service when the cached supply cannot admit it. */
    if (r->supply && r->supply * sc64_race_product_count(type) > c->free_supply) return 0;
    if (honor_reserve ? !race_ai_affordable(c, r->minerals, r->gas) :
        (race_owner_minerals(c->owner) < r->minerals || race_owner_gas(c->owner) < r->gas)) return 0;
    if (!race_queue_unit(c->owner, producer, type)) return 0;
    c->free_supply -= r->supply * sc64_race_product_count(type);
    key = race_ai_key(raw);
    if (key >= 0) ++c->pend[key];
    if (sc64_race_is_worker(type)) ++c->workers;
    return 1;
}

static int race_ai_pick_terran(RaceAiCensus *c, int producer)
{
    int marines = race_ai_count(c, AK_MARINE), firebats = race_ai_count(c, AK_FIREBAT);
    int tanks = race_ai_count(c, AK_TANK), goliaths = race_ai_count(c, AK_GOLIATH);
    int vultures = race_ai_count(c, AK_VULTURE);
    switch (b[producer].type) {
    case B_BARRACKS:
        if (c->have[AK_ACADEMY] && firebats * 4 < marines && c->enemy_air * 2 < c->enemy_army + 1)
            return 32;
        return 0;
    case B_FACTORY:
        if (c->have[AK_ARMORY] && (c->enemy_air > goliaths || goliaths * 2 < tanks)) return 3;
        if (race_skirmish_attached(producer, 120, 1)) return 5;
        if (c->have[AK_ARMORY]) return 3;
        return vultures < 4 ? 2 : -1;
    case B_STARPORT:
        return 8;
    }
    return -1;
}

static int race_ai_pick_zerg(RaceAiCensus *c)
{
    int lings = race_ai_count(c, AK_ZERGLING), hydras = race_ai_count(c, AK_HYDRALISK);
    int mutas = race_ai_count(c, AK_MUTALISK), scourge = race_ai_count(c, AK_SCOURGE);
    if (c->have[AK_SPIRE]) {
        if (c->enemy_air >= 2 && scourge < c->enemy_air && scourge < 8) return 47;
        if (mutas * 2 < hydras + 2) return 43;
    }
    if (c->have[AK_DEN] && (c->enemy_air || lings >= 8 || hydras * 2 < lings)) return 38;
    if (c->have[AK_POOL]) return 37;
    return -1;
}

static int race_ai_pick_protoss(RaceAiCensus *c, int producer)
{
    int zealots = race_ai_count(c, AK_ZEALOT), dragoons = race_ai_count(c, AK_DRAGOON);
    if (b[producer].type == B_STARGATE) return 70;
    if (b[producer].type != B_GATEWAY) return -1;
    if (c->have[AK_CORE] && (c->enemy_air || dragoons <= zealots) &&
        race_owner_gas(c->owner) - c->reserve_gas >= 50) return 66;
    return 65;
}

static void __attribute__((noinline)) race_ai_produce(RaceAiCensus *c)
{
    int worker_target = race_ai_worker_target(c), unit_cap = race_ai_unit_cap();
    int committed = c->units + c->queued_units;
    /* Workers first, from every depot; only pending builds are reserved. */
    for (int k = 0; k < c->depots && c->workers < worker_target && committed < unit_cap; ++k) {
        int depot = c->depot[k];
        if (c->race == R_ZERG) {
            /* A Hatchery's queue is serial in these products. Keep it short
             * and stop droning while the army is small or under threat. */
            if (b[depot].larva < 1 || b[depot].queue_count >= 2) continue;
            if (c->have[AK_POOL] && c->workers >= 9 &&
                (c->threat >= 0 || c->army_supply < c->workers)) continue;
        } else if (b[depot].queue_count >= 1) continue;
        {
            int saved_m = c->reserve_minerals, saved_g = c->reserve_gas;
            int pending_m = 0, pending_g = 0;
            race_pending_build_cost(c->owner, &pending_m, &pending_g);
            c->reserve_minerals = pending_m; c->reserve_gas = pending_g;
            if (race_ai_try_queue(c, depot, c->worker_raw, 1)) ++committed;
            c->reserve_minerals = saved_m; c->reserve_gas = saved_g;
        }
    }
    if (committed >= unit_cap) return;
    for (int i = 0; i < MAX_BUILDINGS; ++i) {
        int raw;
        if (!b[i].alive || b[i].owner != c->owner) continue;
        if (c->race == R_ZERG) {
            if (!sc64_race_is_zerg_townhall(b[i].type) || b[i].larva < 1 ||
                b[i].queue_count >= 2) continue;
            if (b[i].under_construction && b[i].type == B_HATCHERY) continue;
            raw = race_ai_pick_zerg(c);
        } else {
            if (!building_complete(i) || b[i].queue_count >= 1 || race_ai_is_depot(b[i].type)) continue;
            raw = c->race == R_TER ? race_ai_pick_terran(c, i) : race_ai_pick_protoss(c, i);
        }
        if (raw < 0) continue;
        if (!race_ai_try_queue(c, i, raw, 1) &&
            !(c->race == R_PROTOSS && raw == 66 && race_ai_try_queue(c, i, 65, 1))) continue;
        if (++committed >= unit_cap) return;
    }
}

/* -------------------------------------------------------------- military */

/* Spread a group over a 5x3 block at 16 px pitch: converging on one point
 * jams units, and each jammed unit repeats a route search every few frames. */
static void race_ai_order(Unit *q, int order, int x, int y, int n)
{
    int tx = limit(x + ((n % 5) - 2) * 16, 8, WORLD_W - 8);
    int ty = limit(y + ((n / 5 % 3) - 1) * 16, 8, WORLD_H - 8);
    /* Do not restart paths for a unit already carrying out this order. */
    if (q->order == order && dist2(q->tx, q->ty, tx, ty) < 48 * 48) return;
    q->order = order; q->tx = tx; q->ty = ty;
    /* The engine's retry wait (path_retry, at most 12 like its own) delays
     * each unit's first route search, so a group order spreads its searches
     * over 12 updates instead of running them all on one. */
    q->target = -1; q->path_timer = 0; q->path_retry = n % 12;
}

static int race_ai_movable_army(const Unit *q, int owner)
{
    if (!q->alive || q->owner != owner || SC64_TRANSPORT_LOADED(q) ||
        sc64_race_is_worker(q->type) || !race_ai_armed(q->type)) return 0;
#if defined(SC64_T4_RUNTIME) || defined(SC64_ZP_CAMPAIGN_ACTORS)
    if (sc64_game_unit_actor_flags(q->type) &&
        !(sc64_game_unit_actor_flags(q->type) & SC64_GAME_ACTOR_MOVABLE)) return 0;
#endif
    return 1;
}

/* The existing opponent rule, unchanged in this stage: once four armed units
 * exist, every armed unit attack-moves to the hostile structure nearest the
 * owner's base. */
static void __attribute__((noinline)) race_ai_military(RaceAiCensus *c)
{
    int target = -1, best = 0x7fffffff, n = 0;
    if (c->army < 4) return;
    for (int i = 0; i < MAX_BUILDINGS; ++i) {
        int d;
        if (!b[i].alive || !owners_hostile(c->owner, b[i].owner)) continue;
        d = dist2(race_base_x[c->base], race_base_y[c->base], b[i].x, b[i].y);
        if (d < best) { best = d; target = i; }
    }
    if (target < 0) return;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (race_ai_movable_army(&u[i], c->owner))
            race_ai_order(&u[i], ORDER_ATTACK_MOVE, b[target].x, b[target].y, n++);
}

static void __attribute__((noinline)) race_ai_plan(int owner)
{
    RaceAiCensus c;
    race_ai_census(owner, &c);
    if (!c.units && !c.buildings) return;
    race_ai_recover_construction(&c);
    race_ai_recover_workers(&c);
    race_ai_balance_gas(&c);
    if (!race_ai_supply(&c)) race_ai_develop(&c);
    race_ai_produce(&c);
    race_ai_military(&c);
}

/* Bots plan on distinct frames, each once per RACE_AI_PERIOD. */
static void __attribute__((noinline)) race_ai_tick(void)
{
    int slot;
    if (frame % RACE_AI_PHASE) return;
    slot = (frame / RACE_AI_PHASE) % (RACE_AI_PERIOD / RACE_AI_PHASE);
    if (slot >= race_bots || slot >= 3) return;
    race_ai_plan(race_bot_owner_ids[slot]);
}

#endif
