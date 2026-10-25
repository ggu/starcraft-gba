#ifndef SC64_RACE_CAMPAIGN_ADAPTER_H
#define SC64_RACE_CAMPAIGN_ADAPTER_H

/* Mission-specific port rules for the first two Zerg and Protoss maps,
 * plus the existing Terran rules when SC64_UNIFIED is enabled. Source map/trigger evidence is the user-supplied verified
 * ROM, extracted with references/sc64-maps/extract_sc64_maps.py @
 * 09b57f787a247107d1de82b8c5eab2734667ea1d. Unit names use the pinned BWAPI
 * UnitType.h @ d727fed68558c506163048ea889131d8cbb33915.
 *
 * This is an authored subset of those maps' triggers, not a CHK interpreter:
 * it keeps the controller-facing mission goals (ordered Z1 tech, Z2 Chrysalis
 * escort, P1 Fenix rescue/survival plus Zerg-base objective, P2 timed Fenix
 * arrival/survival and Cerebrate target) and omits tutorial dialogue/AI scripts.
 */

enum {
    SC64_RACE_MISSION_Z1,
    SC64_RACE_MISSION_Z2,
    SC64_RACE_MISSION_P1,
    SC64_RACE_MISSION_P2
#ifdef SC64_UNIFIED
    ,SC64_RACE_MISSION_T1, SC64_RACE_MISSION_T2
#ifdef SC64_T3_ENABLED
    ,SC64_RACE_MISSION_T3
#endif
#ifdef SC64_Z3_ENABLED
    ,SC64_RACE_MISSION_Z3 = 14
#endif
#ifdef SC64_Z4_RUNTIME
    ,SC64_RACE_MISSION_Z4 = 15
#endif
#ifdef SC64_Z6_RUNTIME
    ,SC64_RACE_MISSION_Z6 = 17
#endif
#ifdef SC64_Z7_RUNTIME
    ,SC64_RACE_MISSION_Z7 = 18
#endif
#ifdef SC64_P3_RUNTIME
    ,SC64_RACE_MISSION_P3 = 22
#endif
#ifdef SC64_P4_RUNTIME
    ,SC64_RACE_MISSION_P4 = 23
#endif
#ifdef SC64_P5_RUNTIME
    ,SC64_RACE_MISSION_P5 = 24
#endif
#ifdef SC64_P6_RUNTIME
    ,SC64_RACE_MISSION_P6 = 25
#endif
#endif
};

#ifdef SC64_Z4_RUNTIME
static int race_z4_source_owner(unsigned source_slot);
#endif
#ifdef SC64_P4_RUNTIME
static int race_p4_source_owner(unsigned source_slot);
#endif
#ifdef SC64_P5_RUNTIME
static int race_p5_source_owner(unsigned source_slot);
#endif
#ifdef SC64_Z5_RUNTIME
static int race_z5_source_owner(unsigned source_slot);
#endif

static int
race_campaign_owner(const Sc64RaceMap *map, int raw_owner)
{
#ifdef SC64_Z7_RUNTIME
    if (race_z7_active()) {
        int owner = raw_owner < 0 ? -1 : race_z7_source_owner((unsigned)raw_owner);
        return owner < 0 ? NEUTRAL : owner;
    }
#endif
#ifdef SC64_Z6_RUNTIME
    if (race_z6_active()) {
        int owner = raw_owner < 0 ? -1 : race_z6_source_owner((unsigned)raw_owner);
        return owner < 0 ? NEUTRAL : owner;
    }
#endif
#ifdef SC64_P6_RUNTIME
    if (!race_mode && race_selected_mission == SC64_P6_GLOBAL_ID) {
        int owner = raw_owner < 0 ? -1 : race_p6_source_owner((unsigned)raw_owner);
        return owner < 0 ? NEUTRAL : owner;
    }
#endif
#ifdef SC64_P5_RUNTIME
    if (!race_mode && race_selected_mission == SC64_RACE_MISSION_P5) {
        int owner = raw_owner < 0 ? -1 : race_p5_source_owner((unsigned)raw_owner);
        return owner < 0 ? NEUTRAL : owner;
    }
#endif
#ifdef SC64_P4_RUNTIME
    if (!race_mode && race_selected_mission == SC64_RACE_MISSION_P4) {
        int owner = raw_owner < 0 ? -1 : race_p4_source_owner((unsigned)raw_owner);
        return owner < 0 ? NEUTRAL : owner;
    }
#endif
#ifdef SC64_Z4_RUNTIME
    if (!race_mode && race_selected_mission == SC64_RACE_MISSION_Z4) {
        int owner = raw_owner < 0 ? -1 : race_z4_source_owner((unsigned)raw_owner);
        return owner < 0 ? NEUTRAL : owner;
    }
#endif
#ifdef SC64_TERRAN_LATE_RUNTIME
    if (race_late_active()) return sc64_terran_late_runtime_owner((unsigned)raw_owner);
#endif
#ifdef SC64_T6_RUNTIME
    if (race_t6_active()) return sc64_t6_runtime_owner((unsigned)raw_owner);
#endif
#ifdef SC64_T5_RUNTIME
    if (race_t5_active()) {
        if (raw_owner == SC64_T5_SOURCE_HUMAN) return SIDE;
        if (raw_owner == SC64_T5_SOURCE_ENEMY) return ENEMY;
        if (raw_owner == SC64_T5_SOURCE_RESCUE) return RESCUABLE;
        return NEUTRAL;
    }
#endif
#ifdef SC64_T4_RUNTIME
    if (!race_mode && race_selected_mission == SC64_T4_GLOBAL_ID)
        return sc64_t4_runtime_owner(raw_owner);
#endif
    if (raw_owner == 11 || raw_owner >= 8) return NEUTRAL;
    if (raw_owner < 0) return NEUTRAL;
#ifdef SC64_Z5_RUNTIME
    if (!race_mode && race_selected_mission == SC64_Z5_GLOBAL_ID &&
        raw_owner < 8) {
        if (raw_owner == map->human_slot) return SIDE;
        return map->owner_modes[raw_owner];
    }
#endif
#ifdef SC64_P3_RUNTIME
    if (!race_mode && race_selected_mission == SC64_P3_GLOBAL_ID) {
        if (raw_owner == SC64_P3_HUMAN_SLOT) return SIDE;
        if (raw_owner == SC64_P3_FORCE2_SLOT) return ENEMY;
        if (raw_owner == SC64_P3_FORCE3_SLOT) return 4;
        return NEUTRAL;
    }
#endif
    return map->owner_modes[raw_owner];
}

static int
race_campaign_count_buildings(int owner)
{
    int i, n = 0;
    for (i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == owner && !b[i].under_construction) ++n;
    return n;
}

static int
race_campaign_has_building(int owner, int type)
{
    int i;
    for (i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == owner && b[i].type == type &&
            !b[i].under_construction) return 1;
    return 0;
}

static int
race_campaign_find_unit(int type)
{
    int i;
    for (i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].type == type) return i;
    return -1;
}

static int
race_campaign_unit_alive(int slot)
{
    return slot >= 0 && slot < MAX_UNITS && u[slot].alive;
}

static void
race_campaign_set_message(int id)
{
    if (race_campaign.message == id) return;
    race_campaign.message = (unsigned char)id;
    race_campaign.message_ticks = 180;
    toast(0);
}

#if defined(SC64_T3_ENABLED) && defined(SC64_UNIFIED)
#include "sc64_t3_adapter.h"
#endif
#if defined(SC64_Z3_ENABLED) && defined(SC64_UNIFIED)
#include "sc64_z3_adapter.h"
#endif
#ifdef SC64_Z4_RUNTIME
#include "sc64_z4_adapter.h"
static unsigned race_z4_hud_seconds(void)
{ return (race_z4.countdown_ticks + 29u) / 30u; }
#endif
#ifdef SC64_Z5_RUNTIME
#include "sc64_z5_runtime_adapter.h"
#ifdef SC64_Z5_CLOAK_ENABLED
#include "sc64_z5_cloak_adapter.h"
#ifdef SC64_Z5_CONSUME_ENABLED
#include "sc64_z5_consume_adapter.h"
#endif
#endif
#endif
#ifdef SC64_P3_RUNTIME
#include "sc64_p3_adapter.h"
#endif
#ifdef SC64_P4_RUNTIME
#include "sc64_p4_adapter.h"
#ifdef SC64_P4_STORM_ENABLED
#include "sc64_p4_storm_adapter.h"
#endif
#ifdef SC64_P5_RUNTIME
#include "sc64_p5_runtime_adapter.h"
#endif
static int race_p4_world_paused(void)
{ return race_p4_active() && race_p4.source_paused; }

/* Bounded contact rescue keeps source-slot provenance while transferring
 * control. The 32-pixel GBA radius is an authored compatibility policy. */
static void race_p4_rescue_tick(void)
{
    if (race_p4.source_paused) return;
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == RESCUABLE) {
        for (int j = 0; j < MAX_UNITS; ++j) if (u[j].alive && u[j].owner == SIDE &&
            u[j].raw_type != 101 && u[j].raw_type != 150) {
            int dx = u[i].x - u[j].x, dy = u[i].y - u[j].y;
            if (dx * dx + dy * dy <= 32 * 32) {
                u[i].owner = SIDE; u[i].selected = 0;
                u[i].order = ORDER_IDLE; u[i].target = -1;
                u[i].tx = u[i].x; u[i].ty = u[i].y;
                break;
            }
        }
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive && b[i].owner == RESCUABLE) {
        for (int j = 0; j < MAX_UNITS; ++j) if (u[j].alive && u[j].owner == SIDE &&
            u[j].raw_type != 101 && u[j].raw_type != 150) {
            int dx = b[i].x - u[j].x, dy = b[i].y - u[j].y;
            if (dx * dx + dy * dy <= 32 * 32) { b[i].owner = SIDE; break; }
        }
    }
}
#endif

#ifdef SC64_P6_RUNTIME
#include "sc64_p6_doodads_generated.h"
#ifdef SC64_P6_COMBAT_ENABLED
#include "sc64_p6_combat_adapter.h"
#endif
#include "sc64_p6_runtime_adapter.h"
#include "sc64_p6_render_adapter.h"
#ifdef SC64_P6_STORM_ENABLED
#include "sc64_p6_storm_adapter.h"
#endif
#endif

#ifdef SC64_Z6_RUNTIME
#include "sc64_z6_runtime_adapter.h"
#include "sc64_z6_hallucination.h"
#ifdef SC64_Z6_TRANSPORT_ENABLED
#include "sc64_z6_transport_adapter.h"
#endif
#ifdef SC64_Z6_CLOAK_ENABLED
#include "sc64_z6_cloak_adapter.h"
#endif
#ifdef SC64_Z6_STORM_ENABLED
#include "sc64_z6_storm_adapter.h"
#endif
#endif

#ifdef SC64_Z7_RUNTIME
#include "sc64_z7_runtime_adapter.h"
#endif

static void
race_campaign_assign_mining(void)
{
    int i, j;
    for (i = 0; i < MAX_UNITS; ++i) {
        int best = -1, best_d = 0x7fffffff;
        if (!u[i].alive || u[i].owner != ENEMY || !sc64_race_is_worker(u[i].type) ||
            u[i].order != ORDER_IDLE) continue;
        for (j = 0; j < MAX_MINERALS; ++j) if (m[j].amount > 0) {
            int d = dist2(u[i].x, u[i].y, m[j].x, m[j].y);
            if (d < best_d) { best = j; best_d = d; }
        }
        if (best >= 0) {
            u[i].order = ORDER_MINE;
            u[i].target = best;
        }
    }
}

static int
race_campaign_build_site(int owner, int type, int base_x, int base_y, int *x, int *y)
{
    static const int dx[8] = { 48, -48, 0, 0, 48, -48, 48, -48 };
    static const int dy[8] = { 0, 0, 48, -48, 48, 48, -48, -48 };
    int i;
    for (i = 0; i < 8; ++i) {
        int px = limit(base_x + dx[i], 24, WORLD_W - 24);
        int py = limit(base_y + dy[i], 24, WORLD_H - 24);
        if (race_can_site(owner, type, px, py) && can_place(type, px, py)) {
            *x = px; *y = py; return 1;
        }
    }
    return 0;
}

/* P2's 900-second trigger creates thirteen escort units and Fenix in one
 * event. Keep the campaign AI's own production backlog from consuming those
 * slots before the trigger. This is only a queue budget: player production
 * and imported actors remain untouched, so the scripted event retries if
 * they leave fewer than fourteen free slots. */
static int
race_campaign_p2_ai_queue_room(void)
{
    int i, j, free_slots = 0, queued_slots = 0;
    if (race_mode || race_campaign.mission != SC64_RACE_MISSION_P2 ||
        (race_campaign.objective_flags & SC64_RACE_OBJECTIVE_FENIX_READY) ||
        race_campaign.phase == SC64_RACE_PHASE_COMPLETE ||
        race_campaign.phase == SC64_RACE_PHASE_FAILED) return MAX_UNITS;
    for (i = 0; i < MAX_UNITS; ++i) free_slots += !u[i].alive;
    /* Count every owner's pending products because any of them can occupy
     * the slots needed by the source trigger. Zerglings hatch as pairs. */
    for (i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive)
        for (j = 0; j < b[i].queue_count; ++j) {
            int type = b[i].queue[(b[i].queue_head + j) % 5];
            if (sc64_race_unit_rule(type)) queued_slots += type == U_ZERGLING ? 2 : 1;
        }
    queued_slots += 14; /* five Dragoons, five Zealots, three Reavers, Fenix */
    return free_slots > queued_slots ? free_slots - queued_slots : 0;
}

static void
race_campaign_ai_tick(void)
{
    int i, workers = 0, army = 0, producer = -1;
    int enemy_race = race_player[ENEMY], worker_type, producer_type;
    int next_build = -1, train_type = -1, x = 0, y = 0, base_x = -1, base_y = -1;
    if (race_campaign.elapsed_frames % 60u) return;

    race_campaign_assign_mining();
    for (i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == ENEMY) {
        if (sc64_race_is_worker(u[i].type)) {
            ++workers;
            if (base_x < 0) { base_x = u[i].x; base_y = u[i].y; }
        } else ++army;
    }
    for (i = 0; i < MAX_BUILDINGS; ++i) if (building_complete(i) && b[i].owner == ENEMY) {
        if (base_x < 0) { base_x = b[i].x; base_y = b[i].y; }
        if (enemy_race == R_ZERG && sc64_race_is_zerg_townhall(b[i].type)) producer = i;
        else if (enemy_race == R_PROTOSS && b[i].type == B_NEXUS) producer = i;
        else if (enemy_race == R_TER && b[i].type == B_COMMAND) producer = i;
    }
    if (base_x < 0) { base_x = race_cols * 8; base_y = race_rows * 8; }
    worker_type = enemy_race == R_ZERG ? U_DRONE :
                  enemy_race == R_PROTOSS ? U_PROBE : U_SCV;
    train_type = enemy_race == R_ZERG ? U_ZERGLING :
                 enemy_race == R_PROTOSS ? U_ZEALOT : U_MARINE;
    producer_type = enemy_race == R_ZERG ? B_HATCHERY :
                     enemy_race == R_PROTOSS ? B_NEXUS : B_COMMAND;
    if (producer >= 0 && workers < 7 && race_campaign_p2_ai_queue_room() >= 1)
        (void)race_queue_unit(ENEMY, producer, worker_type);

    if (enemy_race == R_ZERG) {
        if (!race_owner_has_type(ENEMY, B_SPAWNING_POOL)) next_build = B_SPAWNING_POOL;
        else if (!race_owner_has_type(ENEMY, B_HYDRALISK_DEN)) next_build = B_HYDRALISK_DEN;
    } else if (enemy_race == R_PROTOSS) {
        if (!race_owner_has_type(ENEMY, B_PYLON)) next_build = B_PYLON;
        else if (!race_owner_has_type(ENEMY, B_GATEWAY)) next_build = B_GATEWAY;
    } else if (!race_owner_has_type(ENEMY, B_BARRACKS)) {
        next_build = B_BARRACKS;
    }
    if (next_build >= 0 && race_campaign_build_site(ENEMY, next_build, base_x, base_y, &x, &y)) {
        int builder = race_owner_worker(ENEMY, worker_type, x, y);
        if (builder >= 0 && race_start_build(ENEMY, builder, next_build, x, y)) return;
    }

    if (producer >= 0 && b[producer].queue_count < 2 &&
        race_campaign_p2_ai_queue_room() >= (train_type == U_ZERGLING ? 2 : 1))
        (void)race_queue_unit(ENEMY, producer, train_type);
    /* Imported enemy squads start as map guards. Release only a small group
     * on a long cadence so the original bases do not turn into an immediate
     * map-wide rush. This is a deliberately small authored AI, not SC's full
     * trigger/AI-script interpreter. */
    if (army >= 4 && race_campaign.elapsed_frames % (180u * 30u) == 0) {
        int target_x = cx, target_y = cy, sent = 0;
        for (i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == SIDE) {
            target_x = u[i].x; target_y = u[i].y; break;
        }
        for (i = 0; i < MAX_UNITS && sent < 2; ++i) if (u[i].alive &&
            u[i].owner == ENEMY && u[i].order == ORDER_IDLE &&
            !sc64_race_is_worker(u[i].type) && u[i].type != U_FENIX &&
            u[i].type != U_CHRYSALIS) {
            u[i].order = ORDER_ATTACK_MOVE;
            u[i].tx = target_x; u[i].ty = target_y;
            ++sent;
            ++race_campaign.enemy_units_sent;
        }
        if (sent) ++race_campaign.enemy_wave;
    }
    (void)producer_type;
}

static void
race_campaign_init(void)
{
    const Sc64RaceMap *map;
    int i, player_x = 0, player_y = 0, player_n = 0;
    int human_owner;
#ifdef SC64_P7_PROJECTILES_ENABLED
    race_p7_projectile_reset();
#endif

#ifdef SC64_UNIFIED
    memset(&backwater,0,sizeof(backwater));
    memset(invincible_units,0,sizeof(invincible_units));memset(invincible_buildings,0,sizeof(invincible_buildings));
    backwater_message_trigger=backwater_message_action=backwater_message_ticks=0;
#endif
#if defined(SC64_Z3_ENABLED) && defined(SC64_UNIFIED)
    memset(&race_z3,0,sizeof(race_z3));race_z3_action_ticks=0;
#endif
#ifdef SC64_P3_RUNTIME
    memset(&race_p3,0,sizeof(race_p3));race_p3_action_ticks=0;
#endif
#ifdef SC64_T4_SCENARIO_RUNTIME
    memset(&race_t4, 0, sizeof(race_t4));
#endif
#ifdef SC64_T5_RUNTIME
    memset(&race_t5, 0, sizeof(race_t5));
#endif
#ifdef SC64_T6_RUNTIME
    memset(&race_t6, 0, sizeof(race_t6));
#endif
#ifdef SC64_TERRAN_LATE_RUNTIME
    memset(&race_late, 0, sizeof(race_late));
#endif
#ifdef SC64_Z5_RUNTIME
    memset(&race_z5, 0, sizeof(race_z5));
    memset(&race_z5_world, 0, sizeof(race_z5_world));
    memset(race_z5_doodads, 0, sizeof(race_z5_doodads));
#endif
#ifdef SC64_P5_RUNTIME
    memset(&race_p5, 0, sizeof(race_p5));
    memset(&race_p5_world, 0, sizeof(race_p5_world));
#endif
#ifdef SC64_P6_RUNTIME
    memset(&race_p6, 0, sizeof(race_p6));
    memset(&race_p6_world, 0, sizeof(race_p6_world));
    memset(race_p6_doodad_bindings, 0, sizeof(race_p6_doodad_bindings));
#endif
#ifdef SC64_P7_BATTERY_ENABLED
    memset(&race_p7, 0, sizeof(race_p7));
    memset(&race_p7_world, 0, sizeof(race_p7_world));
    memset(&race_p7_ops, 0, sizeof(race_p7_ops));
    race_p7_map = 0;
#endif
#ifdef SC64_Z6_RUNTIME
    memset(&race_z6, 0, sizeof(race_z6));
    memset(&race_z6_world, 0, sizeof(race_z6_world));
    race_z6_map = 0;
#endif
#ifdef SC64_Z7_RUNTIME
    memset(&race_z7, 0, sizeof(race_z7));
    memset(&race_z7_world, 0, sizeof(race_z7_world));
    memset(&race_z7_ops, 0, sizeof(race_z7_ops));
    race_z7_map = 0;
#endif
    if (race_mode) {
        race_skirmish_init();
        return;
    }
    race_selected_mission = limit(race_selected_mission, 0, SC64_RACE_MAP_COUNT - 1);
#ifdef SC64_UNIFIED
    if ((!sc64_catalog_mission_allowed(race_selected_mission,
#ifdef SC64_CAMPAIGN_RACE
                                     SC64_CAMPAIGN_RACE
#else
                                     -1
#endif
                                     )
#ifdef SC64_T4_RUNTIME
         /* Internal world-loading probe. The frontend catalog remains gated
          * until the scenario has a complete ordinary-controller proof. */
         && race_selected_mission != SC64_T4_GLOBAL_ID
#endif
#ifdef SC64_T5_RUNTIME
         && race_selected_mission != SC64_T5_GLOBAL_ID
#endif
#ifdef SC64_T6_RUNTIME
         && race_selected_mission != SC64_T6_GLOBAL_ID
#endif
#ifdef SC64_TERRAN_LATE_RUNTIME
         && !race_late_active()
#endif
#ifdef SC64_P6_RUNTIME
         && race_selected_mission != SC64_P6_GLOBAL_ID
#endif
#ifdef SC64_P7_BATTERY_ENABLED
         && race_selected_mission != SC64_P7_GLOBAL_ID
#endif
#ifdef SC64_Z7_RUNTIME
         && race_selected_mission != SC64_Z7_GLOBAL_ID
#endif
         ) || !sc64_race_maps[race_selected_mission].width)
        race_selected_mission = sc64_frontend_first_mission(
#ifdef SC64_CAMPAIGN_RACE
            SC64_CAMPAIGN_RACE
#else
            SC_FRONT_TERRAN
#endif
        );
#endif
    map = &sc64_race_maps[race_selected_mission];
#ifdef SC64_Z7_RUNTIME
    if (race_z7_active() && !race_z7_source_row_counts(map)) {
        ++mission_unsupported_count; screen = SCREEN_PAUSE; toast(14); return;
    }
#endif
#ifdef SC64_Z6_RUNTIME
    if (race_z6_active() && !race_z6_map_shape_valid(map)) {
        ++mission_unsupported_count; screen = SCREEN_PAUSE; toast(14); return;
    }
#endif
#ifdef SC64_Z5_RUNTIME
    if (race_selected_mission == SC64_Z5_GLOBAL_ID && !race_z5_preflight(map)) {
        ++mission_unsupported_count; screen = SCREEN_PAUSE; toast(14); return;
    }
#endif
#ifdef SC64_P5_RUNTIME
    if (race_selected_mission == SC64_P5_GLOBAL_ID && !race_p5_preflight(map)) {
        ++mission_unsupported_count; screen = SCREEN_PAUSE; toast(14); return;
    }
#endif
#ifdef SC64_P6_RUNTIME
    if (race_selected_mission == SC64_P6_GLOBAL_ID && !race_p6_preflight(map)) {
        ++mission_unsupported_count; screen = SCREEN_PAUSE; toast(14); return;
    }
#endif
#ifdef SC64_P7_BATTERY_ENABLED
    if (race_p7_active() && !race_p7_map_shape_valid(map)) {
        ++mission_unsupported_count; screen = SCREEN_PAUSE; toast(14); return;
    }
#endif
#ifdef SC64_ZP_CAMPAIGN_STORAGE
    /* Validate every retained starting row before touching the live world.
     * These staged pools are deliberately smaller than later campaign maps. */
    {
        unsigned units = 0, buildings = 0, minerals_count = 0, gas_count = 0;
        int invalid = map->width > 128 || map->height > 128;
        for (unsigned row = 0; row < map->unit_count; ++row) {
            const Sc64ImportedUnit *src = &map->units[row];
            int raw = src->source_type;
            if (src->x_px >= map->width * 16 || src->y_px >= map->height * 16) invalid = 1;
#ifdef SC64_Z5_RUNTIME
            if (race_selected_mission == SC64_Z5_GLOBAL_ID && raw == 214) continue;
#endif
#ifdef SC64_P6_RUNTIME
            if (race_selected_mission == SC64_P6_GLOBAL_ID && raw == 214) continue;
#endif
#ifdef SC64_Z6_RUNTIME
            if (race_selected_mission == SC64_Z6_GLOBAL_ID && raw == 214) continue;
#endif
#ifdef SC64_Z7_RUNTIME
            if (race_selected_mission == SC64_Z7_GLOBAL_ID && raw == 214) continue;
#endif
            if (raw >= 176 && raw <= 178) ++minerals_count;
            else if (raw == 188) ++gas_count;
            else if (sc64_race_unit_from_raw(raw) >= 0) ++units;
            else if (sc64_race_building_from_raw(raw) >= 0) ++buildings;
            else invalid = 1;
        }
#ifdef SC64_Z5_RUNTIME
        if (race_selected_mission == SC64_Z5_GLOBAL_ID)
            buildings += SC64_Z5_UNIT_BACKED_DOODADS;
#endif
#ifdef SC64_P6_RUNTIME
        if (race_selected_mission == SC64_P6_GLOBAL_ID)
            buildings += SC64_P6_DOODAD_UNIT_PROP_COUNT;
#endif
        if (invalid || units > MAX_UNITS || buildings > MAX_BUILDINGS ||
            minerals_count > MAX_MINERALS || gas_count > MAX_RACE_GEYSERS) {
            ++mission_unsupported_count; screen = SCREEN_PAUSE; toast(14); return;
        }
    }
#endif
    race_cols = map->width;
    race_rows = map->height;
    race_human = map->owner_races[map->human_slot];
    memset(u, 0, sizeof(u));
    memset(b, 0, sizeof(b));
    memset(m, 0, sizeof(m));
    memset(&race_campaign, 0, sizeof(race_campaign));
    memset(race_player, 0, sizeof(race_player));
    memset(race_minerals, 0, sizeof(race_minerals));
    memset(race_gas, 0, sizeof(race_gas));
    memset(explored, 0, sizeof(explored));
    memset(visible, 0, sizeof(visible));
    memset(tracers, 0, sizeof(tracers));
    death_vfx_reset();
    reset_selection_gestures();
    geyser_count = 0;
    supply_used = 0;
    supply_cap = 0;
    frame = 0;
    mode = MODE_NORMAL;
    menu = 0; menu_open = 0; menu_cursor = 0; help = 0;
    won = lost = 0;
    result_timer = 0;
    toast_timer = toast_id = 0;
    enemy_timer = 0;
    wave = 0;
    weapons_level = armor_level = 0;
    weapons_research = armor_research = 0;
    selected_building = -1;
    train_type = race_human == R_ZERG ? U_DRONE :
                 race_human == R_PROTOSS ? U_PROBE : U_SCV;
    seed = 0x51a7c0deu;
    screen = SCREEN_PLAY;
    race_campaign.mission = (unsigned char)race_selected_mission;
    race_campaign.phase = SC64_RACE_PHASE_ACTIVE;
    race_campaign.rescue_unit = 0xffffu;
    race_campaign.chrysalis_unit = 0xffffu;
    race_campaign.carrier_unit = 0xffffu;
    race_campaign.message = 0;
    minerals = gas = 0;
    invalidate_path_grid();
    sc64_visibility_reset();

    for (i = 0; i < 8; ++i) {
        int owner = map->owner_modes[i];
#ifdef SC64_Z7_RUNTIME
        if (race_z7_active()) owner = race_campaign_owner(map, i);
#endif
#ifdef SC64_Z4_RUNTIME
        if (race_selected_mission == SC64_RACE_MISSION_Z4) owner = race_campaign_owner(map, i);
#endif
#ifdef SC64_Z5_RUNTIME
        if (race_selected_mission == SC64_Z5_GLOBAL_ID)
            owner = race_campaign_owner(map, i);
#endif
#ifdef SC64_P4_RUNTIME
        if (race_selected_mission == SC64_RACE_MISSION_P4) owner = race_campaign_owner(map, i);
#endif
#ifdef SC64_P5_RUNTIME
        if (race_selected_mission == SC64_P5_GLOBAL_ID)
            owner = race_campaign_owner(map, i);
#endif
#ifdef SC64_P6_RUNTIME
        if (race_selected_mission == SC64_P6_GLOBAL_ID)
            owner = race_campaign_owner(map, i);
#endif
#ifdef SC64_Z6_RUNTIME
        if (race_z6_active()) owner = race_campaign_owner(map, i);
#endif
#ifdef SC64_T4_RUNTIME
        if (race_selected_mission == SC64_T4_GLOBAL_ID)
            owner = race_campaign_owner(map, i);
#endif
#ifdef SC64_T5_RUNTIME
        if (race_t5_active()) owner = race_campaign_owner(map, i);
#endif
#ifdef SC64_T6_RUNTIME
        if (race_t6_active()) owner = race_campaign_owner(map, i);
#endif
#ifdef SC64_TERRAN_LATE_RUNTIME
        if (race_late_active()) owner = race_campaign_owner(map, i);
#endif
#ifdef SC64_P3_RUNTIME
        if (race_selected_mission == SC64_P3_GLOBAL_ID)
            owner = race_campaign_owner(map, i);
#endif
        if (owner >= 0 && owner < 5 && map->owner_races[i] < 3)
            race_player[owner] = map->owner_races[i];
    }
#ifdef SC64_Z5_RUNTIME
    /* Multiple source slots can map onto the live SIDE owner; make the
     * authenticated human slot the final authority for the save identity. */
    if (race_selected_mission == SC64_Z5_GLOBAL_ID)
        race_player[SIDE] = race_human;
#endif
#ifdef SC64_P5_RUNTIME
    if (race_selected_mission == SC64_P5_GLOBAL_ID) {
        race_player[SIDE] = race_human;
        race_player[NEUTRAL] = map->owner_races[6];
    }
#endif
#ifdef SC64_P6_RUNTIME
    if (race_selected_mission == SC64_P6_GLOBAL_ID) race_player[SIDE] = race_human;
#endif
    human_owner = map->human_slot;

    for (i = 0; i < map->unit_count; ++i) {
        const Sc64ImportedUnit *src = &map->units[i];
        int raw = src->source_type, owner, type, slot;
        int x = src->x_px, y = src->y_px;
        if (x >= map->width * 16 || y >= map->height * 16) continue;
#ifdef SC64_Z5_RUNTIME
        if (race_selected_mission == SC64_Z5_GLOBAL_ID && raw == 214) continue;
#endif
#ifdef SC64_P6_RUNTIME
        if (race_selected_mission == SC64_P6_GLOBAL_ID && raw == 214) continue;
#endif
#ifdef SC64_Z6_RUNTIME
            if (race_selected_mission == SC64_Z6_GLOBAL_ID && raw == 214) continue;
#endif
#ifdef SC64_Z7_RUNTIME
            if (race_selected_mission == SC64_Z7_GLOBAL_ID && raw == 214) continue;
#endif
        if (raw == 176 || raw == 177 || raw == 178) {
            int j;
            for (j = 0; j < MAX_MINERALS; ++j) if (m[j].amount <= 0) break;
            if (j < MAX_MINERALS) {
                m[j].x = x; m[j].y = y;
                m[j].amount = src->resources ? src->resources : 1500;
                m[j].style = raw;
            }
            continue;
        }
        if (raw == 188) {
            if (geyser_count < MAX_RACE_GEYSERS) {
                geysers[geyser_count].x = x;
                geysers[geyser_count++].y = y;
            }
            continue;
        }
        owner = race_campaign_owner(map, src->owner);
        if (src->owner == human_owner) owner = SIDE;
#ifdef SC64_UNIFIED
        /* Terran Raynor starts neutral and joins on contact, as in the existing adapter. */
        if(race_selected_mission>=SC64_RACE_MISSION_T1&&raw==19
#ifdef SC64_ZP_CAMPAIGN_ACTORS
           && race_selected_mission<=13
#endif
#ifdef SC64_T5_RUNTIME
           && !race_t5_active()
#endif
#ifdef SC64_T6_RUNTIME
           && !race_t6_active()
#endif
#ifdef SC64_TERRAN_LATE_RUNTIME
           && !race_late_active()
#endif
           )owner=NEUTRAL;
#endif
        type = sc64_race_unit_from_raw(raw);
#ifdef SC64_T5_RUNTIME
        /* Earlier adapters use the established Raynor actor. Raw 19 gains
         * its distinct Vulture rules only in the T5 scenario. */
        if (raw == 19 && !race_t5_active()
#ifdef SC64_T6_RUNTIME
            && !race_t6_active()
#endif
#ifdef SC64_TERRAN_LATE_RUNTIME
            && !race_late_active()
#endif
            ) type = U_RAYNOR;
#endif
        if (type >= 0) {
            slot = spawn_unit(owner, type, x, y);
            if (slot < 0) {
#ifdef SC64_Z5_RUNTIME
                if (race_selected_mission == SC64_Z5_GLOBAL_ID) {
                    ++mission_unsupported_count; screen = SCREEN_PAUSE; toast(14); return;
                }
#endif
                continue;
            }
            u[slot].raw_type = raw;
            u[slot].raw_owner = src->owner;
            u[slot].flags = src->flags;
            /* Mission identity aliases (notably early Raynor) must retain
             * their objective enum while using the actual source unit's
             * stock capacities before applying scenario percentages. */
            const Sc64StockUnitProfile *stock=sc64_stock_unit_profile(raw);
            if (stock) {
                u[slot].hp=u[slot].maxhp=(int)stock->hp;
                u[slot].shield=(int)stock->shields;
                u[slot].armor=(u8)stock->armor;
            }
            if (src->hp_percent && src->hp_percent <= 100)
                u[slot].hp = u[slot].maxhp * src->hp_percent / 100;
            if (type == U_FENIX) race_campaign.rescue_unit = (unsigned short)slot;
#ifdef SC64_UNIFIED
            if(type==U_RAYNOR&&race_selected_mission>=SC64_RACE_MISSION_T1)race_campaign.rescue_unit=(unsigned short)slot;
#endif
            if (type == U_CHRYSALIS) race_campaign.chrysalis_unit = (unsigned short)slot;
            if (owner == SIDE) {
                player_x += x; player_y += y; ++player_n;
            }
            continue;
        }
        type = sc64_race_building_from_raw(raw);
        if (type >= 0) {
            slot = spawn_build(owner, type, x, y, 0);
            if (slot < 0) {
#ifdef SC64_Z5_RUNTIME
                if (race_selected_mission == SC64_Z5_GLOBAL_ID) {
                    ++mission_unsupported_count; screen = SCREEN_PAUSE; toast(14); return;
                }
#endif
                continue;
            }
            b[slot].raw_type = raw;
            b[slot].raw_owner = src->owner;
            b[slot].flags = src->flags;
            if (src->hp_percent && src->hp_percent <= 100)
                b[slot].hp = b[slot].maxhp * src->hp_percent / 100;
            if (type == B_PYLON || type == B_NEXUS || type == B_COMMAND)
                supply_cap += type == B_NEXUS ? 18 : (type == B_COMMAND ? 20 : 16);
        }
#ifdef SC64_UNIFIED
        else if(race_selected_mission>=SC64_RACE_MISSION_T1)++mission_unsupported_count;
#endif
    }

#ifdef SC64_Z5_RUNTIME
    if (race_selected_mission == SC64_Z5_GLOBAL_ID && !race_z5_runtime_init(map)) {
        ++mission_unsupported_count; screen = SCREEN_PAUSE; toast(14); return;
    }
#ifdef SC64_Z5_CLOAK_ENABLED
    if (race_z5_active() && !race_z5_cloak_initialize_source_units()) {
        ++mission_unsupported_count; screen = SCREEN_PAUSE; toast(14); return;
    }
#endif
#endif
#ifdef SC64_P5_RUNTIME
    if (race_selected_mission == SC64_P5_GLOBAL_ID && !race_p5_runtime_init(map)) {
        ++mission_unsupported_count; screen = SCREEN_PAUSE; toast(14); return;
    }
#endif

#ifdef SC64_P6_RUNTIME
    if (race_selected_mission == SC64_P6_GLOBAL_ID && !race_p6_runtime_init(map)) {
        ++mission_unsupported_count; screen = SCREEN_PAUSE; toast(14); return;
    }
#ifdef SC64_P6_STORM_ENABLED
    if (race_p6_active()) race_p6_storm_initialize_source_units();
#endif
#endif

#ifdef SC64_Z6_RUNTIME
    if (race_z6_active() && !race_z6_runtime_init(map)) {
        ++mission_unsupported_count; screen = SCREEN_PAUSE; toast(14); return;
    }
#ifdef SC64_Z6_TRANSPORT_ENABLED
    if (race_z6_active() && !race_z6_transport_initialize_source()) {
        ++mission_unsupported_count; screen = SCREEN_PAUSE; toast(14); return;
    }
#endif
#ifdef SC64_Z6_CLOAK_ENABLED
    if (race_z6_active() && !race_z6_cloak_initialize_source_units()) {
        ++mission_unsupported_count; screen = SCREEN_PAUSE; toast(14); return;
    }
#endif
#endif
#ifdef SC64_Z7_RUNTIME
    if (race_z7_active() && !race_z7_runtime_init(map)) {
        ++mission_unsupported_count; screen = SCREEN_PAUSE; toast(14); return;
    }
#endif
#ifdef SC64_P7_BATTERY_ENABLED
    if (race_p7_active() &&
        (!race_p7_runtime_init(map) || !race_p7_battery_initialize_source(map))) {
        ++mission_unsupported_count; screen = SCREEN_PAUSE; toast(14); return;
    }
#endif
    switch (race_selected_mission) {
#ifdef SC64_Z7_RUNTIME
    case SC64_Z7_GLOBAL_ID: minerals = gas = 0; break;
#endif
#ifdef SC64_Z6_RUNTIME
    case SC64_Z6_GLOBAL_ID: minerals = gas = 0; break;
#endif
#ifdef SC64_P6_RUNTIME
    case SC64_P6_GLOBAL_ID: minerals = gas = 0; break;
#endif
#ifdef SC64_P7_BATTERY_ENABLED
    case SC64_P7_GLOBAL_ID: minerals = gas = 0; break;
#endif
#ifdef SC64_TERRAN_LATE_RUNTIME
    case 10: case 11: case 12: case 13: minerals = gas = 0; break;
#endif
#ifdef SC64_T6_RUNTIME
    case SC64_T6_GLOBAL_ID: minerals = gas = 0; break;
#endif
#ifdef SC64_T5_RUNTIME
    case SC64_T5_GLOBAL_ID: minerals = gas = 0; break;
#endif
#ifdef SC64_T4_RUNTIME
    case SC64_T4_GLOBAL_ID: minerals = gas = 0; break;
#endif
    case SC64_RACE_MISSION_Z1: minerals = 200; break;
    case SC64_RACE_MISSION_Z2: minerals = 300; break;
    case SC64_RACE_MISSION_P1: minerals = 400; gas = 100; break;
#ifdef SC64_UNIFIED
    case SC64_RACE_MISSION_T1: minerals=0;break;
    case SC64_RACE_MISSION_T2: backwater_init();break;
#ifdef SC64_T3_ENABLED
    case SC64_RACE_MISSION_T3: minerals=100;gas=0;break;
#endif
#ifdef SC64_Z3_ENABLED
    case SC64_RACE_MISSION_Z3: minerals=SC64_Z3_MISSION_ORE;gas=SC64_Z3_MISSION_GAS;break;
#endif
#ifdef SC64_Z4_RUNTIME
    case SC64_RACE_MISSION_Z4: minerals=gas=0;break;
#endif
#ifdef SC64_P3_RUNTIME
    case SC64_RACE_MISSION_P3: minerals=SC64_P3_HUMAN_MINERALS;gas=SC64_P3_HUMAN_GAS;break;
#endif
#ifdef SC64_P4_RUNTIME
    case SC64_RACE_MISSION_P4: minerals=gas=0;break;
#endif
#endif
    default: minerals = 170; break;
    }
    race_minerals[SIDE] = minerals;
    race_gas[SIDE] = gas;
    if (player_n) { cx = player_x / player_n; cy = player_y / player_n; }
    else { cx = map->width * 8; cy = map->height * 8; }
    cx = limit(cx, 120, map->width * 16 - 120);
    cy = limit(cy, 54, map->height * 16 - 54);
    camx = camy = 0;
#if defined(SC64_T3_ENABLED) && defined(SC64_UNIFIED)
    if (race_selected_mission == SC64_RACE_MISSION_T3) {
        /* T3 TRIG 0 centers on MRGN 14 (the damaged Vulture area), at half
         * the CHK pixel coordinates used by imported map records. */
        cx = 544;
        cy = 1384;
        race_t3_init();
    }
#endif
#if defined(SC64_Z3_ENABLED) && defined(SC64_UNIFIED)
    if (race_selected_mission == SC64_RACE_MISSION_Z3) race_z3_init();
#endif
#ifdef SC64_Z4_RUNTIME
    if (race_selected_mission == SC64_RACE_MISSION_Z4 && !race_z4_init()) {
        screen = SCREEN_PAUSE; toast(14);
    }
#endif
#ifdef SC64_P3_RUNTIME
    if (race_selected_mission == SC64_RACE_MISSION_P3) race_p3_init();
#endif
#ifdef SC64_P4_RUNTIME
    if (race_selected_mission == SC64_RACE_MISSION_P4 && !race_p4_init()) {
        screen = SCREEN_PAUSE; toast(14);
    }
#ifdef SC64_P4_STORM_ENABLED
    else if (race_selected_mission == SC64_RACE_MISSION_P4)
        race_p4_storm_initialize_source_units();
#endif
#endif
    race_campaign_assign_mining();
#ifdef SC64_T4_SCENARIO_RUNTIME
    if (race_t4_active() && !race_t4_init()) screen = SCREEN_PAUSE;
#endif
#ifdef SC64_T5_RUNTIME
    if (race_t5_active() && !race_t5_init()) screen = SCREEN_PAUSE;
#endif
#ifdef SC64_T6_RUNTIME
    if (race_t6_active() && !race_t6_init()) screen = SCREEN_PAUSE;
#endif
#ifdef SC64_TERRAN_LATE_RUNTIME
    if (race_late_active() && !race_late_init()) screen = SCREEN_PAUSE;
#endif
    if (race_selected_mission == SC64_RACE_MISSION_Z1)
        race_campaign_set_message(1);
    else if (race_selected_mission == SC64_RACE_MISSION_Z2)
        race_campaign_set_message(3);
    else if (race_selected_mission == SC64_RACE_MISSION_P1)
        race_campaign_set_message(4);
    else if(race_selected_mission==SC64_RACE_MISSION_P2)
        race_campaign_set_message(5);
#ifdef SC64_UNIFIED
    else if (race_selected_mission==SC64_RACE_MISSION_T1) race_campaign_set_message(10);
    else if (race_selected_mission==SC64_RACE_MISSION_T2) race_campaign_set_message(11);
#ifdef SC64_T3_ENABLED
    else if (race_selected_mission==SC64_RACE_MISSION_T3) race_campaign_set_message(14);
#endif
#ifdef SC64_Z3_ENABLED
    else if (race_selected_mission==SC64_RACE_MISSION_Z3) race_campaign_set_message(20);
#endif
#ifdef SC64_P3_RUNTIME
    else if (race_selected_mission==SC64_RACE_MISSION_P3) race_campaign_set_message(0);
#endif
#endif
    update_exploration();
}

static int
race_campaign_spawn_p2_reinforcements(void)
{
    static const int raw_types[14] = {
        66, 66, 66, 66, 66, 65, 65, 65, 65, 65, 83, 83, 83, 77
    };
    int i, type, slot, free_slots = 0, created[14], old_supply_used = supply_used;
    /* P2 TRIG record 5: CountdownTimer(900), MRGN 1/3/4/5. */
    for (i = 0; i < MAX_UNITS; ++i) free_slots += !u[i].alive;
    if (free_slots < 14) return 0;
    for (i = 0; i < 14; ++i)
        if (sc64_race_unit_from_raw(raw_types[i]) < 0) return 0;

    /* The update loop is single-threaded. Preflight all slots, then create
     * the complete event before publishing FENIX_READY. On an unexpected
     * failure, roll back only units created by this call and retry later. */
    for (i = 0; i < 14; ++i) {
        int x, y, owner = i == 13 ? RESCUABLE : SIDE;
        if (i < 5) { x = 1400 + (i % 2) * 8; y = 64 + (i / 2) * 8; }
        else if (i < 10) { int n = i - 5; x = 1344 + (n % 2) * 8; y = 96 + (n / 2) * 8; }
        else if (i < 13) { int n = i - 10; x = 1400 + (n % 2) * 8; y = 128 + (n / 2) * 8; }
        else { x = 1312; y = 96; }
        type = sc64_race_unit_from_raw(raw_types[i]);
        slot = spawn_unit(owner, type, x, y);
        if (slot < 0) {
            while (i > 0) u[created[--i]].alive = 0;
            supply_used = old_supply_used;
            return 0;
        }
        created[i] = slot;
    }
    for (i = 0; i < 14; ++i) {
        u[created[i]].raw_type = raw_types[i];
        u[created[i]].raw_owner = i == 13 ? 2 : 13;
    }
    race_campaign.rescue_unit = (unsigned short)created[13];
    race_campaign.objective_flags |= SC64_RACE_OBJECTIVE_FENIX_READY;
    race_campaign.phase = SC64_RACE_PHASE_REINFORCEMENTS;
    race_campaign_set_message(6);
    return 1;
}

static int
race_campaign_enemy_buildings_remain(void)
{
    return race_campaign_count_buildings(ENEMY) != 0;
}

#ifdef SC64_UNIFIED
/* Reuse the already-ported Terran scenario rules rather than substituting
 * the Zerg/Protoss mission AI. T2's callbacks retain its CHK region/timer rules. */
static void race_terran_tick(void)
{
    if(race_selected_mission==SC64_RACE_MISSION_T2){backwater_update();}
    else {
        int i,j,raynor_alive=0,raynor_found=0,marines=0,barracks=0,command=0,men=0;
        if(!mission_objective_stage&&frame>=30){minerals=40;race_minerals[SIDE]=40;mission_objective_stage=1;race_campaign_set_message(12);}
        for(i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].type==U_RAYNOR&&u[i].owner==NEUTRAL)
            for(j=0;j<MAX_UNITS;j++)if(u[j].alive&&u[j].owner==SIDE&&dist2(u[i].x,u[i].y,u[j].x,u[j].y)<=20*20){
                u[i].owner=SIDE;u[i].selected=0;race_campaign_set_message(13);break;}
        for(i=0;i<MAX_UNITS;i++)if(u[i].alive){
            if(u[i].owner==SIDE)++men;
            if(u[i].type==U_RAYNOR){raynor_alive=1;raynor_found=u[i].owner==SIDE;}
            else if(u[i].owner==SIDE&&u[i].type==U_MARINE)++marines;}
        for(i=0;i<MAX_BUILDINGS;i++)if(b[i].alive&&b[i].owner==SIDE){
            if(b[i].type==B_BARRACKS)++barracks;else if(b[i].type==B_COMMAND)++command;}
        if(!raynor_alive||!men||(!command&&!barracks&&marines<=9&&minerals<=399))lost=1;
        if(raynor_found&&marines>=10&&barracks>=1)won=1;
    }
    if(lost){won=0;race_campaign.phase=SC64_RACE_PHASE_FAILED;}
    else if(won){race_campaign.phase=SC64_RACE_PHASE_COMPLETE;race_campaign.objective_flags|=SC64_RACE_OBJECTIVE_PRIMARY;race_completed|=1<<race_selected_mission;}
    supply_used=race_owner_supply(SIDE);supply_cap=race_owner_supply_cap(SIDE);
}
#endif

static void
race_campaign_tick(void)
{
    int i, j;
    const int mission = race_campaign.mission;
    if (race_campaign.phase == SC64_RACE_PHASE_COMPLETE ||
        race_campaign.phase == SC64_RACE_PHASE_FAILED) return;
    if (race_campaign.message_ticks) --race_campaign.message_ticks;
    ++race_campaign.elapsed_frames;
#ifdef SC64_TERRAN_LATE_RUNTIME
    if (race_late_active()) {
        if (!race_late_tick()) screen = SCREEN_PAUSE;
        return;
    }
#endif
#ifdef SC64_T6_RUNTIME
    if (mission == SC64_T6_GLOBAL_ID) {
        if (!race_t6_tick()) screen = SCREEN_PAUSE;
        return;
    }
#endif
#ifdef SC64_T5_RUNTIME
    if (mission == SC64_T5_GLOBAL_ID) {
#if defined(SC64_DIAGNOSTIC_UPDATE)
        u32 t5_stage_start = gba_clock();
#endif
        race_t5_world_tick();
#if defined(SC64_DIAGNOSTIC_UPDATE)
        sc64_t5_rescue_cycles = gba_clock() - t5_stage_start;
        t5_stage_start = gba_clock();
#endif
        race_t5_mines_tick();
#if defined(SC64_DIAGNOSTIC_UPDATE)
        sc64_t5_mine_cycles = gba_clock() - t5_stage_start;
        t5_stage_start = gba_clock();
#endif
        if (!race_t5_tick()) screen = SCREEN_PAUSE;
#if defined(SC64_DIAGNOSTIC_UPDATE)
        sc64_t5_trigger_ai_cycles = gba_clock() - t5_stage_start;
#endif
        return;
    }
#endif
#ifdef SC64_T4_RUNTIME
    if (mission == SC64_T4_GLOBAL_ID) {
#if defined(SC64_DIAGNOSTIC_UPDATE) && defined(SC64_T4_SCENARIO_RUNTIME)
        u32 t4_rescue_start = gba_clock();
#endif
        race_t4_world_tick();
#if defined(SC64_DIAGNOSTIC_UPDATE) && defined(SC64_T4_SCENARIO_RUNTIME)
        sc64_t4_rescue_cycles = gba_clock() - t4_rescue_start;
#endif
#ifdef SC64_T4_SCENARIO_RUNTIME
        if (!race_t4_tick()) screen = SCREEN_PAUSE;
#else
        /* Door/teleport/quest actions are not implemented by this roster
         * stage. Do not run another mission's AI or infer a victory. */
#endif
        return;
    }
#endif
#if defined(SC64_T3_ENABLED) && defined(SC64_UNIFIED)
    if (mission == SC64_RACE_MISSION_T3) {
        race_t3_campaign_tick();
        return;
    }
#endif
#ifdef SC64_UNIFIED
#ifdef SC64_Z3_ENABLED
    if(mission==SC64_RACE_MISSION_Z3){race_z3_campaign_tick();return;}
#endif
#ifdef SC64_Z4_RUNTIME
    if (mission == SC64_RACE_MISSION_Z4) {
        if (!race_z4_campaign_tick()) { screen = SCREEN_PAUSE; toast(14); }
        return;
    }
#endif
#ifdef SC64_Z5_RUNTIME
    if (mission == SC64_Z5_GLOBAL_ID) {
#ifdef SC64_Z5_CLOAK_ENABLED
        if (!race_z5_cloak_tick()) { screen = SCREEN_PAUSE; toast(14); return; }
#endif
        if (!race_z5_campaign_tick()) { screen = SCREEN_PAUSE; toast(14); }
        return;
    }
#endif
#ifdef SC64_P3_RUNTIME
    if(mission==SC64_RACE_MISSION_P3){race_p3_campaign_tick();return;}
#endif
#ifdef SC64_P4_RUNTIME
    if (mission == SC64_RACE_MISSION_P4) {
        race_p4_rescue_tick();
#ifdef SC64_P4_STORM_ENABLED
        if (!race_p4_storm_tick()) { screen = SCREEN_PAUSE; toast(14); return; }
#endif
        if (!race_p4_campaign_tick()) { screen = SCREEN_PAUSE; toast(14); }
        return;
    }
#endif
#ifdef SC64_P5_RUNTIME
    if (mission == SC64_P5_GLOBAL_ID) {
        if (!race_p5_campaign_tick()) { screen = SCREEN_PAUSE; toast(14); }
        return;
    }
#endif
#ifdef SC64_P6_RUNTIME
    if (mission == SC64_P6_GLOBAL_ID) {
        race_p6_rescue_tick();
#ifdef SC64_P6_STORM_ENABLED
        if (!race_p6_storm_tick(0)) { screen = SCREEN_PAUSE; toast(14); return; }
#endif
        if (!race_p6_campaign_tick()) { screen = SCREEN_PAUSE; toast(14); }
        return;
    }
#endif
#ifdef SC64_P7_BATTERY_ENABLED
    if (mission == SC64_P7_GLOBAL_ID) {
        if (!race_p7_battery_live_world_valid() ||
            !race_p7_runtime_tick(race_campaign.elapsed_frames)) {
            screen=SCREEN_PAUSE; toast(14); return;
        }
        if (race_p7.runtime.model.outcome==SC64_P7_WON) {
            won=1; lost=0; race_campaign.phase=SC64_RACE_PHASE_COMPLETE;
            race_campaign.objective_flags|=SC64_RACE_OBJECTIVE_PRIMARY;
            race_completed |= 1 << SC64_P7_GLOBAL_ID;
        } else if (race_p7.runtime.model.outcome==SC64_P7_LOST) {
            won=0; lost=1; race_campaign.phase=SC64_RACE_PHASE_FAILED;
        }
        return;
    }
#endif
#ifdef SC64_Z7_RUNTIME
    if (mission == SC64_Z7_GLOBAL_ID) {
        if (!race_z7_runtime_tick(0, race_campaign.elapsed_frames)) {
            screen=SCREEN_PAUSE; toast(14); return;
        }
        if (race_z7.runtime.model.outcome==SC64_Z7_WON) {
            won=1; lost=0; race_campaign.phase=SC64_RACE_PHASE_COMPLETE;
            race_campaign.objective_flags|=SC64_RACE_OBJECTIVE_PRIMARY;
            race_completed |= 1 << SC64_Z7_GLOBAL_ID;
        } else if (race_z7.runtime.model.outcome==SC64_Z7_LOST) {
            won=0; lost=1; race_campaign.phase=SC64_RACE_PHASE_FAILED;
        }
        return;
    }
#endif
#ifdef SC64_Z6_RUNTIME
    if (mission == SC64_Z6_GLOBAL_ID) {
#ifdef SC64_Z6_TRANSPORT_ENABLED
        if (!race_z6_transport_research_tick(0)) { screen=SCREEN_PAUSE; toast(14); return; }
#endif
#ifdef SC64_Z6_CLOAK_ENABLED
        if (!race_z6_cloak_tick(0)) { screen=SCREEN_PAUSE; toast(14); return; }
#endif
#ifdef SC64_Z6_STORM_ENABLED
        if (!race_z6_storm_tick()) { screen=SCREEN_PAUSE; toast(14); return; }
#endif
        if (!race_z6_hallucination_world_state_valid() ||
            !race_z6_hallucination_tick(0) ||
            !race_z6_runtime_tick(race_campaign.elapsed_frames, 0)) {
            screen = SCREEN_PAUSE; toast(14); return;
        }
        if (race_z6.runtime.model.outcome == SC64_Z6_WON) {
            won=1; lost=0; race_campaign.phase=SC64_RACE_PHASE_COMPLETE;
            race_campaign.objective_flags|=SC64_RACE_OBJECTIVE_PRIMARY;
            race_completed |= 1 << SC64_Z6_GLOBAL_ID;
        } else if (race_z6.runtime.model.outcome == SC64_Z6_LOST) {
            won=0; lost=1; race_campaign.phase=SC64_RACE_PHASE_FAILED;
        }
        return;
    }
#endif
    if(mission==SC64_RACE_MISSION_T1||mission==SC64_RACE_MISSION_T2){race_terran_tick();return;}
#endif
    race_campaign_ai_tick();

    if (mission == SC64_RACE_MISSION_Z1) {
        int chrysalis = race_campaign_find_unit(U_CHRYSALIS);
        int pool = race_campaign_has_building(SIDE, B_SPAWNING_POOL);
        int den = race_campaign_has_building(SIDE, B_HYDRALISK_DEN);
        if (chrysalis < 0) {
            race_campaign.objective_flags |= SC64_RACE_OBJECTIVE_CHRYSALIS_LOST;
            race_campaign.phase = SC64_RACE_PHASE_FAILED;
        } else if (pool && den) {
            race_campaign.objective_flags |= SC64_RACE_OBJECTIVE_PRIMARY;
            if (race_campaign.objective_stage == 0) {
                race_campaign.objective_stage = 1;
                race_campaign_set_message(2);
            }
            if (!race_campaign_enemy_buildings_remain()) {
                race_campaign.objective_flags |= SC64_RACE_OBJECTIVE_SECONDARY;
                race_campaign.phase = SC64_RACE_PHASE_COMPLETE;
            }
        }
    } else if (mission == SC64_RACE_MISSION_Z2) {
        int c = race_campaign.chrysalis_unit;
        /* The source OWNR marks six Hunter Killers as Rescue Passive
         * (raw owner 0). Once a human unit reaches that group, take control
         * of the surviving special units for the original escort objective. */
        {
            int contact = 0, joined = 0;
            for (i = 0; i < MAX_UNITS && !contact; ++i) if (u[i].alive &&
                u[i].owner == RESCUABLE && u[i].raw_type == 53)
                for (j = 0; j < MAX_UNITS; ++j) if (u[j].alive && u[j].owner == SIDE &&
                    dist2(u[j].x,u[j].y,u[i].x,u[i].y) <= 28*28) { contact = 1; break; }
            if (contact) for (i = 0; i < MAX_UNITS; ++i) if (u[i].alive &&
                u[i].owner == RESCUABLE && u[i].raw_type == 53) {
                u[i].owner = SIDE; joined = 1;
            }
            if (joined) race_campaign_set_message(9);
        }
        if (!race_campaign_unit_alive(c)) {
            race_campaign.objective_flags |= SC64_RACE_OBJECTIVE_CHRYSALIS_LOST;
            race_campaign.phase = SC64_RACE_PHASE_FAILED;
        } else {
            Unit *q = &u[c];
            if (race_campaign.carrier_unit < MAX_UNITS) {
                int carrier = race_campaign.carrier_unit;
                if (!u[carrier].alive || u[carrier].owner != SIDE || u[carrier].type != U_DRONE) {
                    race_campaign.carrier_unit = 0xffffu;
                    q->owner = NEUTRAL;
                    race_campaign.objective_flags &= ~SC64_RACE_OBJECTIVE_CHRYSALIS_RECOVERED;
                } else {
                    q->x = u[carrier].x;
                    q->y = u[carrier].y;
                }
            }
            if (q->owner != SIDE) {
                /* Z2's powerup is retrieved by a drone. It remains a mission
                 * object while carried and follows that controller-movable
                 * unit to the Warp Point. */
                for (i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == SIDE &&
                    u[i].type == U_DRONE && dist2(u[i].x, u[i].y, q->x, q->y) <= 16 * 16) {
                    q->owner = SIDE;
                    race_campaign.carrier_unit = (unsigned short)i;
                    q->selected = 0;
                    race_campaign.objective_flags |= SC64_RACE_OBJECTIVE_CHRYSALIS_RECOVERED;
                    race_campaign_set_message(7);
                    break;
                }
            }
            /* Source MRGN 1 in 008/015 is "Warp Point": 1184..1232 by
             * 1200..1232 source px, converted to the GBA's 16px scale. */
            if (q->owner == SIDE && q->x >= 1184 && q->x <= 1232 &&
                q->y >= 1200 && q->y <= 1232) {
                race_campaign.objective_flags |= SC64_RACE_OBJECTIVE_PRIMARY;
                race_campaign.phase = SC64_RACE_PHASE_COMPLETE;
            }
        }
    } else if (mission == SC64_RACE_MISSION_P1) {
        int fenix = race_campaign.rescue_unit;
        if (!race_campaign_unit_alive(fenix)) {
            race_campaign.objective_flags |= SC64_RACE_OBJECTIVE_FENIX_LOST;
            race_campaign.phase = SC64_RACE_PHASE_FAILED;
        } else {
            Unit *f = &u[fenix];
            /* P1 source OWNR marks the Antioch force as Rescue Passive.
             * MRGN 6 "Protoss Base" is [448,1696,1568,2400] source pixels;
             * coordinates here use the SC64 half-scale. Entering the region
             * brings its surviving rescuable units and buildings under the
             * human side, matching the original passive-rescue behavior. */
            for (i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == SIDE &&
                u[i].x >= 224 && u[i].x <= 784 && u[i].y >= 848 && u[i].y <= 1200) break;
            if (i < MAX_UNITS) {
                int joined = 0;
                for (i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == RESCUABLE &&
                    u[i].x >= 224 && u[i].x <= 784 && u[i].y >= 848 && u[i].y <= 1200) {
                    if ((int)i == fenix) race_campaign.objective_flags |= SC64_RACE_OBJECTIVE_FENIX_MET;
                    u[i].owner = SIDE; joined = 1;
                }
                for (i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive && b[i].owner == RESCUABLE &&
                    b[i].x >= 224 && b[i].x <= 784 && b[i].y >= 848 && b[i].y <= 1200) {
                    b[i].owner = SIDE; joined = 1;
                }
                if (joined) race_campaign_set_message(8);
            }
            if (f->owner == RESCUABLE) {
                /* Source 008/01E MRGN 1 is "rescue Fenix". */
                for (i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == SIDE &&
                    dist2(u[i].x, u[i].y, f->x, f->y) <= 28 * 28) {
                    f->owner = SIDE;
                    race_campaign.objective_flags |= SC64_RACE_OBJECTIVE_FENIX_MET;
                    race_campaign_set_message(4);
                    break;
                }
            }
            if (f->owner == SIDE && !race_campaign_enemy_buildings_remain()) {
                race_campaign.objective_flags |= SC64_RACE_OBJECTIVE_PRIMARY |
                                                  SC64_RACE_OBJECTIVE_SECONDARY;
                race_campaign.phase = SC64_RACE_PHASE_COMPLETE;
            }
        }
    } else if (mission == SC64_RACE_MISSION_P2) {
        int fenix;
        if (race_campaign.elapsed_frames >= 900u * 30u &&
            !(race_campaign.objective_flags & SC64_RACE_OBJECTIVE_FENIX_READY))
            race_campaign_spawn_p2_reinforcements();
        fenix = race_campaign.rescue_unit;
        if (race_campaign.objective_flags & SC64_RACE_OBJECTIVE_FENIX_READY) {
            if (!race_campaign_unit_alive(fenix)) {
                race_campaign.objective_flags |= SC64_RACE_OBJECTIVE_FENIX_LOST;
                race_campaign.phase = SC64_RACE_PHASE_FAILED;
            } else if (u[fenix].owner == RESCUABLE) {
                for (i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == SIDE &&
                    dist2(u[i].x, u[i].y, u[fenix].x, u[fenix].y) <= 28 * 28) {
                    u[fenix].owner = SIDE;
                    race_campaign.objective_flags |= SC64_RACE_OBJECTIVE_FENIX_MET;
                    break;
                }
            }
            for (i = 0; i < MAX_UNITS; ++i)
                if (u[i].alive && u[i].owner == ENEMY && u[i].raw_type == 151) break;
            for (j = 0; j < MAX_BUILDINGS; ++j)
                if (b[j].alive && b[j].owner == ENEMY && b[j].raw_type == 151) break;
            if (i == MAX_UNITS && j == MAX_BUILDINGS) {
                race_campaign.objective_flags |= SC64_RACE_OBJECTIVE_CEREBRATE_DEAD;
                race_campaign.phase = SC64_RACE_PHASE_COMPLETE;
            }
        }
    }
    if (race_campaign.phase == SC64_RACE_PHASE_COMPLETE) {
        race_completed |= 1 << race_campaign.mission;
        won = 1;
    } else if (race_campaign.phase == SC64_RACE_PHASE_FAILED) {
        lost = 1;
    }
}

static int race_campaign_won(void)
{ return race_campaign.phase == SC64_RACE_PHASE_COMPLETE; }

static int race_campaign_lost(void)
{ return race_campaign.phase == SC64_RACE_PHASE_FAILED; }

static const char *
race_campaign_objective_text(void)
{
#ifdef SC64_P7_BATTERY_ENABLED
    if (race_p7_active()) return "Destroy the Conclave heart. Keep Fenix, Zeratul, and Tassadar alive.";
#endif
#ifdef SC64_Z7_RUNTIME
    if (race_z7_active()) return "Destroy all enemy buildings.";
#endif
#ifdef SC64_Z6_RUNTIME
    if (race_z6_active()) return "Hunt Tassadar; keep Kerrigan alive.";
#endif
#ifdef SC64_P5_RUNTIME
    if (race_p5_active()) return race_p5_objective_text();
#endif
#ifdef SC64_P6_RUNTIME
    if (race_p6_active()) return race_p6_objective_text();
#endif
#ifdef SC64_P4_RUNTIME
    if (race_p4_active()) return race_p4_objective_text();
#endif
#ifdef SC64_Z4_RUNTIME
    if (race_z4_active()) return race_z4_objective_text();
#endif
#ifdef SC64_TERRAN_LATE_RUNTIME
    if (race_late_active()) {
        switch (race_campaign.mission) {
        case 10: return "Deliver the PSI Emitter. Kerrigan must survive.";
        case 11: return "Clear the opposing bases. Protect Norad II.";
        case 12: return "Destroy the remaining buildings.";
        case 13: return "Destroy the Ion Cannon. Raynor must survive.";
        default: break;
        }
    }
#endif
#ifdef SC64_T6_RUNTIME
    if (race_t6_active())
        return "Bring Raynor and two Dropships to Norad II. Raynor and Norad II must survive.";
#endif
#ifdef SC64_T5_RUNTIME
    if (race_t5_active())
        return "Reach the rebel base and destroy Confederate buildings. Kerrigan and Raynor must survive.";
#endif
#ifdef SC64_T4_SCENARIO_RUNTIME
    if (race_t4_active()) return "Find the data disks. Raynor must survive.";
#endif
#ifdef SC64_P3_RUNTIME
    if (race_campaign.mission == SC64_RACE_MISSION_P3)
        return race_p3_objective_text();
#endif
#if defined(SC64_Z3_ENABLED) && defined(SC64_UNIFIED)
    if (race_campaign.mission == SC64_RACE_MISSION_Z3)
        return race_z3_objective_text();
#endif
#if defined(SC64_T3_ENABLED) && defined(SC64_UNIFIED)
    if (race_campaign.mission == SC64_RACE_MISSION_T3)
        return race_t3_objective_text();
#endif
    static const char *const objectives[] = {
        "Build a Spawning Pool and Hydralisk Den; protect the Chrysalis, then clear Terran buildings.",
        "Recover the Chrysalis and move it to the Warp Point.",
        "Meet Fenix and destroy the Zerg base. Fenix must survive.",
        "Hold out until Fenix arrives, protect him, and destroy the Cerebrate."
#ifdef SC64_UNIFIED
        ,"Find Raynor; build a Barracks and train ten Marines.",
        "Destroy the Infested Command Center. Raynor must survive."
#ifdef SC64_T3_ENABLED
        ,"Survive the countdown, or unlock and complete the Zerg Hatchery objective."
#endif
#endif
    };
    int mission = limit(race_campaign.mission, 0, (int)(sizeof(objectives)/sizeof(objectives[0])) - 1);
    return objectives[mission];
}

static const char *
race_campaign_victory_text(void)
{
#ifdef SC64_Z7_RUNTIME
    if (race_z7_active()) return "THE CULLING IS COMPLETE";
#endif
#ifdef SC64_Z6_RUNTIME
    if (race_z6_active()) return "THE HUNT IS OVER";
#endif
#ifdef SC64_P5_RUNTIME
    if (race_p5_active()) return race_p5_victory_text();
#endif
#ifdef SC64_P6_RUNTIME
    if (race_p6_active()) return race_p6_victory_text();
#endif
#ifdef SC64_P4_RUNTIME
    if (race_p4_active()) return race_p4_victory_text();
#endif
#ifdef SC64_Z4_RUNTIME
    if (race_z4_active()) return race_z4_victory_text();
#endif
#ifdef SC64_TERRAN_LATE_RUNTIME
    if (race_late_active()) {
        switch (race_campaign.mission) {
        case 10: return "PSI EMITTER DELIVERED";
        case 11: return "THE BIG PUSH COMPLETE";
        case 12: return "NEW GETTYSBURG CLEARED";
        case 13: return "THE ION CANNON DESTROYED";
        default: break;
        }
    }
#endif
#ifdef SC64_T6_RUNTIME
    if (race_t6_active()) return "NORAD II RESCUED";
#endif
#ifdef SC64_T5_RUNTIME
    if (race_t5_active()) return "ANTIGAN REVOLT COMPLETE";
#endif
#ifdef SC64_T4_SCENARIO_RUNTIME
    if (race_t4_active()) return "JACOBS DATA RECOVERED";
#endif
#ifdef SC64_P3_RUNTIME
    if (race_campaign.mission == SC64_RACE_MISSION_P3)
        return race_p3_victory_text();
#endif
#if defined(SC64_Z3_ENABLED) && defined(SC64_UNIFIED)
    if (race_campaign.mission == SC64_RACE_MISSION_Z3)
        return race_z3_victory_text();
#endif
#if defined(SC64_T3_ENABLED) && defined(SC64_UNIFIED)
    if (race_campaign.mission == SC64_RACE_MISSION_T3)
        return race_t3_victory_text();
#endif
    static const char *const titles[] = {
        "AMONG THE RUINS CLEARED", "CHRYSALIS DELIVERED",
        "FIRST STRIKE COMPLETE", "CEREBRATE DESTROYED"
#ifdef SC64_UNIFIED
        ,"WASTELAND SECURED", "BACKWATER SECURED"
#ifdef SC64_T3_ENABLED
        ,"THE DESPERATE ALLIANCE SURVIVED"
#endif
#endif
    };
    return titles[limit(race_campaign.mission, 0, (int)(sizeof(titles)/sizeof(titles[0])) - 1)];
}

/* Used by the result screen when the late Terran profile is wired to its
 * mission-specific defeat label. Keep the player-facing text independent of
 * source dialogue and describe only each map's loss-critical actor/objective. */
static const char *
race_campaign_failure_text(void)
{
#ifdef SC64_Z7_RUNTIME
    if (race_z7_active()) return "THE BROOD HAS FALLEN";
#endif
#ifdef SC64_Z6_RUNTIME
    if (race_z6_active()) return "KERRIGAN HAS FALLEN";
#endif
#ifdef SC64_P5_RUNTIME
    if (race_p5_active()) return race_p5_failure_text();
#endif
#ifdef SC64_P6_RUNTIME
    if (race_p6_active()) return race_p6_failure_text();
#endif
#ifdef SC64_P4_RUNTIME
    if (race_p4_active()) return race_p4_failure_text();
#endif
#ifdef SC64_Z4_RUNTIME
    if (race_z4_active()) return race_z4_failure_text();
#endif
#ifdef SC64_TERRAN_LATE_RUNTIME
    if (race_late_active()) {
        switch (race_campaign.mission) {
        case 10: return "KERRIGAN OR THE FORCE WAS LOST";
        case 11: return "NORAD II OR THE FORCE WAS LOST";
        case 12: return "THE ASSAULT FORCE WAS LOST";
        case 13: return "RAYNOR OR THE ASSAULT FORCE WAS LOST";
        default: break;
        }
    }
#endif
    return "MISSION FAILED";
}

static const char *
race_campaign_message_text(void)
{
#ifdef SC64_TERRAN_LATE_RUNTIME
    if (race_late_active())
        return race_campaign_lost() ? race_campaign_failure_text() :
               race_campaign_objective_text();
#endif
#ifdef SC64_T6_RUNTIME
    if (race_t6_active()) return race_campaign_objective_text();
#endif
#ifdef SC64_T5_RUNTIME
    if (race_t5_active()) return race_campaign_objective_text();
#endif
#ifdef SC64_T4_SCENARIO_RUNTIME
    if (race_t4_active()) return (race_t4.switches.bits[0] & 16) ?
        "Data located. Keep Raynor alive until the mission report." :
        "Explore the installation and find the data disks. Protect Raynor.";
#endif
#ifdef SC64_P3_RUNTIME
    if (race_campaign.mission == SC64_RACE_MISSION_P3) {
        if (race_p3.transmission) return "Zerg colonies cleared. Mission report pending.";
        return race_p3_objective_text();
    }
#endif
    static const char *const messages[] = {
        "", "Protect the Chrysalis. Build a Spawning Pool and Hydralisk Den.",
        "The required Zerg structures are ready. Destroy the Terran encampment.",
        "Chrysalis recovered. Bring it to the Warp Point.",
        "Fenix is with you. Destroy the Zerg base.",
        "Fenix will move into position in 15 minutes.",
        "Fenix is in position. Destroy the Cerebrate and keep him alive.",
        "Move a Drone next to the Chrysalis, then guide it to the Warp Point.",
        "You have reached the Protoss Base. The allied forces join you.",
        "The Hunter Killers join you. Escort the Drone and Chrysalis to the Warp Point."
#ifdef SC64_UNIFIED
        ,"Build your outpost and find Jim Raynor.",
        "Rescue the outpost and destroy the Infested Command Center.",
        "Opening supplies received. Find Jim Raynor.",
        "Raynor has joined. Build a Barracks and train ten Marines.",
        "Hold the Terran base. Destroy a Zerg building before the final two minutes to reveal another objective.",
        "The enemy has begun its assault. Hold your buildings until the countdown ends.",
        "Ten minutes remain. Keep the Terran buildings alive.",
        "Five minutes remain. Keep the Terran buildings alive.",
        "One minute remains. Hold the line.",
        "The Hatchery objective is open. Destroy the Zerg Hatchery or survive the countdown.",
        "Protect the Chrysalis and eliminate the Terran presence."
#endif
    };
    return race_campaign.message < sizeof(messages) / sizeof(messages[0]) ?
        messages[race_campaign.message] : "";
}

#endif
