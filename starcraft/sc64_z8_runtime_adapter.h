/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_Z8_RUNTIME_ADAPTER_H
#define SC64_Z8_RUNTIME_ADAPTER_H

#if defined(SC64_Z8_RUNTIME)

#if !defined(SC64_Z8_DATA_ENABLED) || !defined(SC64_Z7_RUNTIME) || \
    !defined(SC64_Z8_GAME_POOL_BRIDGE) || !defined(SC64_NYDUS_ENABLED) || \
    !defined(SC64_CAMPAIGN_RACE) || SC64_CAMPAIGN_RACE != 1
#error "Z8 runtime requires authenticated Z8 data on the Z1-Z7 Zerg profile"
#endif

/* Z8 "The Dark Templar" campaign binding. The authenticated 39-trigger
 * program runs through the native-cadence scheduler (sc64_z8_triggers.h).
 * Census, owner, location, invincibility and resource actions use the
 * shared Z8 world adapter over the live pools; CreateUnit uses the bounded
 * default-property bridge service. Mission strings and sounds stay in the
 * private CHK and are not shown or played. Transmission uses an authored
 * duration, and CenterView snaps the camera. The three AI scripts run the
 * authored backend in sc64_z8_ai.h. */
#include "sc64_z8_state.h"
#include "sc64_z8_program_generated.h"
#include "sc64_z8_ai.h"

_Static_assert(SC64_Z8_PROGRAM_TRIGGER_COUNT == SC64_Z8_SOURCE_TRIGGER_COUNT &&
               SC64_Z8_PROGRAM_CONDITION_COUNT == SC64_Z8_SOURCE_CONDITION_COUNT &&
               SC64_Z8_PROGRAM_ACTION_COUNT == SC64_Z8_SOURCE_ACTION_COUNT &&
               SC64_Z8_PROGRAM_LOCATION_COUNT == SC64_Z8_SOURCE_LOCATION_COUNT &&
               SC64_Z8_PROGRAM_NYDUS_COUNT == SC64_Z8_SOURCE_NYDUS_RECORD_COUNT,
               "Z8 program tables must match the authenticated source census");

/* SC64_Z8_GLOBAL_ID (19) is defined with the forward declarations in game.c. */
enum {
    SC64_Z8_MAP_UNITS = 194,
    SC64_Z8_HUMAN_SLOT = 3,
    /* Native Fastest speed-table entry; applied once per native step. */
    SC64_Z8_FRAME_MS = 42,
    /* Every Z8 Transmission subtracts its time from a sound length that is
     * not imported; this bounded authored value stands in for it. */
    SC64_Z8_TRANSMISSION_MS = 4000,
    SC64_Z8_RAW_NEXUS = 154,
    /* Pinned BWAPI Overlord sight (11 tiles) is its detection range; the
     * pinned OpenBW cap for a grounded building detector is seven tiles.
     * Engine pixels are source pixels / 2. */
    SC64_Z8_RAW_OVERLORD = 42,
    SC64_Z8_RAW_SPORE_COLONY = 144,
    SC64_Z8_OVERLORD_DETECT_PX = 11 * 16,
    SC64_Z8_SPORE_DETECT_PX = 7 * 16,
    SC64_Z8_RAW_HATCHERY = 131,
    SC64_Z8_RAW_LAIR = 132,
    SC64_Z8_RAW_HIVE = 133
};

static const Sc64Z8Program race_z8_program = {
    sc64_z8_program_triggers, sc64_z8_program_conditions,
    sc64_z8_program_actions, sc64_z8_program_locations,
    SC64_Z8_PROGRAM_TRIGGER_COUNT, SC64_Z8_PROGRAM_CONDITION_COUNT,
    SC64_Z8_PROGRAM_ACTION_COUNT, SC64_Z8_PROGRAM_LOCATION_COUNT,
    SC64_Z8_PROGRAM_MAP_PIXEL_WIDTH, SC64_Z8_PROGRAM_MAP_PIXEL_HEIGHT
};

#include "sc64_z8_clock.h"

static Sc64Z8SchedulerState race_z8_sched;
static Sc64Z8AiState race_z8_ai;
static u32 race_z8_ai_minerals[SC64_Z8_AI_TOWNS], race_z8_ai_gas[SC64_Z8_AI_TOWNS];
static Sc64Z8WorldView race_z8_view;
static Sc64Z8GameContext race_z8_game;
static u32 race_z8_human_minerals, race_z8_human_gas;
static u8 race_z8_start_nexus;
static u8 race_z8_camera_release;
static const Sc64RaceMap *race_z8_map;
/* Source slot for products of the building currently completing a queue. */
static s8 race_z8_product_slot = -1;
/* Derived each update (and on load): enemy hero Dark Templars that no human
 * detector reveals. They are not drawn and cannot be targeted by the human. */
static u8 race_z8_hidden[(MAX_UNITS + 7) / 8];
static u8 race_z8_hidden_any;

static const u8 sc64_z8_owner_modes_expected[8] = { 2, 1, 1, 0, 2, 1, 2, 2 };

static int race_z8_active(void)
{ return !race_mode && race_selected_mission == SC64_Z8_GLOBAL_ID; }

static int race_z8_map_shape_valid(const Sc64RaceMap *map)
{
    if (!map || SC64_Z8_GLOBAL_ID >= SC64_RACE_MAP_COUNT ||
        map != &sc64_race_maps[SC64_Z8_GLOBAL_ID] ||
        map->width != SC64_Z8_SOURCE_MAP_COLS ||
        map->height != SC64_Z8_SOURCE_MAP_ROWS ||
        map->human_slot != SC64_Z8_HUMAN_SLOT ||
        map->unit_count != SC64_Z8_MAP_UNITS || !map->owner_modes ||
        !map->units || !map->mission_id || map->mission_id[0] != 'Z' ||
        map->mission_id[1] != '8' || map->mission_id[2] != '\0') return 0;
    for (unsigned i = 0; i < 8; ++i)
        if (map->owner_modes[i] != sc64_z8_owner_modes_expected[i]) return 0;
    return 1;
}

/* Source slot to live owner. Slot 5 holds the neutral Zerg beacons and slot
 * 11 the resources; both stay neutral rather than following OWNR mode 1. */
static int race_z8_source_owner(unsigned slot)
{
    if (slot == SC64_Z8_HUMAN_SLOT) return SIDE;
    if (slot == 1u || slot == 2u) return ENEMY;
    if (slot == 5u || slot == 11u) return NEUTRAL;
    return -1;
}

/* PUNI is source-slot scoped. Only colony products use this first shared
 * route; other production retains its existing profile behavior. */
static int race_z8_colony_product_available(int owner,int raw_owner,int type)
{
    int raw=sc64_race_raw_building(type);
    if (!race_z8_active() || owner!=SIDE || raw_owner<0 ||
        race_z8_source_owner((unsigned)raw_owner)!=owner || raw<0) return 0;
    return sc64_z8_permission_available(&sc64_z8_permission_table,
                                         (unsigned)raw_owner,(unsigned)raw);
}

static void race_z8_note_unit_spawn(int slot, int owner, int type)
{
    if (!race_z8_active() || slot < 0 || slot >= MAX_UNITS) return;
    u[slot].raw_type = (s16)sc64_race_raw_unit(type);
    u[slot].raw_owner = owner == SIDE ? SC64_Z8_HUMAN_SLOT : race_z8_product_slot;
}

static void race_z8_note_building_spawn(int slot, int owner, int type)
{
    if (!race_z8_active() || slot < 0 || slot >= MAX_BUILDINGS) return;
    b[slot].raw_type = (s16)sc64_race_raw_building(type);
    b[slot].raw_owner = owner == SIDE ? SC64_Z8_HUMAN_SLOT : -1;
}

static void race_z8_note_product_building(int building)
{
    race_z8_product_slot = race_z8_active() && building >= 0 &&
        building < MAX_BUILDINGS ? (s8)b[building].raw_owner : -1;
}

static int __attribute__((noinline)) race_z8_unit_hidden_from_side(int unit)
{
    return race_z8_hidden_any && unit >= 0 && unit < MAX_UNITS &&
           ((race_z8_hidden[unit >> 3] >> (unit & 7)) & 1u);
}

/* Native hero Dark Templars are permanently cloaked. Overlords and complete
 * Spore Colonies are the human's detectors in this mission. */
static void race_z8_update_cloak(void)
{
    memset(race_z8_hidden, 0, sizeof(race_z8_hidden));
    race_z8_hidden_any = 0;
    if (!race_z8_active()) return;
    for (int i = 0; i < MAX_UNITS; ++i) {
        int seen = 0;
        if (!u[i].alive || u[i].owner == SIDE ||
            u[i].raw_type != SC64_Z8_RAW_HERO_DARK_TEMPLAR) continue;
        for (int j = 0; j < MAX_UNITS && !seen; ++j)
            seen = u[j].alive && u[j].owner == SIDE &&
                   u[j].raw_type == SC64_Z8_RAW_OVERLORD &&
                   !SC64_TRANSPORT_LOADED(&u[j]) &&
                   dist2(u[j].x, u[j].y, u[i].x, u[i].y) <=
                       SC64_Z8_OVERLORD_DETECT_PX * SC64_Z8_OVERLORD_DETECT_PX;
        for (int j = 0; j < MAX_BUILDINGS && !seen; ++j)
            seen = b[j].alive && b[j].owner == SIDE &&
                   b[j].raw_type == SC64_Z8_RAW_SPORE_COLONY && building_complete(j) &&
                   dist2(b[j].x, b[j].y, u[i].x, u[i].y) <=
                       SC64_Z8_SPORE_DETECT_PX * SC64_Z8_SPORE_DETECT_PX;
        if (seen) continue;
        race_z8_hidden[i >> 3] |= (u8)(1u << (i & 7));
        race_z8_hidden_any = 1;
    }
}

static int race_z8_timed(void *context, int phase, const Sc64Z8Action *action,
                         unsigned owner, const Sc64Z8Rect *rect, u32 *duration_ms)
{
    (void)context;
    if (!action || !duration_ms || owner != SC64_Z8_HUMAN_SLOT) return -1;
    if (phase == SC64_Z8_TIMED_ELIGIBLE || phase == SC64_Z8_TIMED_FINISH) return 1;
    if (phase != SC64_Z8_TIMED_START) return -1;
    if (action->opcode == 7u) {
        *duration_ms = SC64_Z8_TRANSMISSION_MS;
        race_campaign_set_message(0);
        return 1;
    }
    if (action->opcode == 10u && rect) {
        /* The camera snaps, so its pan-completion release follows on the
         * next update instead of after an animated pan. */
        cx = limit(((int)rect->left + (int)rect->right) / 4, 120, race_cols * 16 - 120);
        cy = limit(((int)rect->top + (int)rect->bottom) / 4, 54, race_rows * 16 - 54);
        *duration_ms = SC64_Z8_WAIT_INDEFINITE;
        race_z8_camera_release = 1;
        return 1;
    }
    return -1;
}

/* PlayWAV, DisplayTextMessage, SetMissionObjectives and MinimapPing have no
 * world effect here; their private strings and sounds are not imported. */
static int race_z8_present(void *context, const Sc64Z8Action *action,
                           unsigned owner, const Sc64Z8Rect *rect)
{
    (void)context; (void)rect;
    if (!action || owner >= SC64_Z8_OWNER_LIMIT ||
        (action->opcode != 8u && action->opcode != 9u && action->opcode != 12u &&
         action->opcode != 28u)) return -1;
    return 1;
}

/* The only Kill row counts the human's kills of Nexus (raw 154). The engine
 * keeps no per-player kill ledger; only the human fights the Nexus owners,
 * so a destroyed starting Nexus counts as a human kill. This is an authored
 * stand-in for the native census. */
static int race_z8_kill_count(void *context, unsigned source_owner,
                              unsigned raw_type, u32 *count)
{
    unsigned alive = 0;
    (void)context;
    if (!count || source_owner != SC64_Z8_HUMAN_SLOT || raw_type != SC64_Z8_RAW_NEXUS)
        return -1;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        alive += b[i].alive && b[i].raw_type == SC64_Z8_RAW_NEXUS &&
                 b[i].raw_owner != SC64_Z8_HUMAN_SLOT;
    *count = alive < race_z8_start_nexus ? race_z8_start_nexus - alive : 0u;
    return 1;
}

static void race_z8_move_dark_templars(unsigned owner, const Sc64Z8Rect *rect)
{
    int x = ((int)rect->left + (int)rect->right) / 4;
    int y = ((int)rect->top + (int)rect->bottom) / 4;
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive &&
        u[i].raw_type == SC64_Z8_RAW_HERO_DARK_TEMPLAR &&
        u[i].raw_owner == (s16)owner && u[i].order == ORDER_IDLE) {
        u[i].order = ORDER_MOVE; u[i].target = -1;
        u[i].tx = (s16)x; u[i].ty = (s16)y; u[i].path_x = u[i].path_y = 0;
    }
}

static int race_z8_run_ai(void *context, u32 fourcc, unsigned owner,
                          unsigned location, const Sc64Z8Rect *rect)
{
    (void)context; (void)location;
    if (!rect) return -1;
    if (fourcc == SC64_Z8_AI_ZE8T && owner == 1u) sc64_z8_ai_start_town(&race_z8_ai, 0);
    else if (fourcc == SC64_Z8_AI_ZER8 && owner == 2u) sc64_z8_ai_start_town(&race_z8_ai, 1);
    else if (fourcc == SC64_Z8_AI_MVTE && owner == 1u) race_z8_move_dark_templars(owner, rect);
    else return -1;
    return 1;
}

static int race_z8_nydus_source(int mission, const Sc64NydusSourceRecord **records,
                                unsigned *count)
{
    if (mission != SC64_Z8_GLOBAL_ID || !records || !count) return 0;
    *records = sc64_z8_program_nydus;
    *count = SC64_Z8_PROGRAM_NYDUS_COUNT;
    return 1;
}

static void race_z8_setup(Sc64Z8Setup *setup)
{
    setup->active_mask = SC64_Z8_PROGRAM_ACTIVE_MASK;
    for (unsigned o = 0; o < SC64_Z8_OWNER_LIMIT; ++o)
        setup->force[o] = sc64_z8_program_forces[o];
    setup->frame_ms = SC64_Z8_FRAME_MS;
}

/* Rebuild the transient view and wallet bindings around the live state. */
static int race_z8_bind(const Sc64RaceMap *map)
{
    s8 live[SC64_Z8_SOURCE_OWNER_LIMIT];
    if (!race_z8_map_shape_valid(map)) return 0;
    race_z8_map = map;
    for (unsigned slot = 0; slot < SC64_Z8_SOURCE_OWNER_LIMIT; ++slot)
        live[slot] = (s8)(slot < SC64_Z8_OWNER_LIMIT ? race_z8_source_owner(slot) : -1);
    memset(&race_z8_view, 0, sizeof(race_z8_view));
    if (!sc64_z8_game_context_init(&race_z8_game, live,
                                   SC64_Z8_SOURCE_MAP_COLS * 32u,
                                   SC64_Z8_SOURCE_MAP_ROWS * 32u)) return 0;
    sc64_z8_game_bind_actor_pool(&race_z8_view);
    race_z8_view.active_mask = SC64_Z8_PROGRAM_ACTIVE_MASK;
    for (unsigned o = 0; o < SC64_Z8_OWNER_LIMIT; ++o)
        race_z8_view.force[o] = sc64_z8_program_forces[o];
    race_z8_view.source_minerals[SC64_Z8_HUMAN_SLOT] = &race_z8_human_minerals;
    race_z8_view.source_gas[SC64_Z8_HUMAN_SLOT] = &race_z8_human_gas;
    for (unsigned t = 0; t < SC64_Z8_AI_TOWNS; ++t) {
        race_z8_view.source_minerals[sc64_z8_ai_town_slot(t)] = &race_z8_ai_minerals[t];
        race_z8_view.source_gas[sc64_z8_ai_town_slot(t)] = &race_z8_ai_gas[t];
    }
    race_z8_view.services.run_ai = race_z8_run_ai;
    race_z8_view.services.present = race_z8_present;
    race_z8_view.services.kill_count = race_z8_kill_count;
    race_z8_view.services.timed = race_z8_timed;
    sc64_z8_game_bind_create_service(&race_z8_view, &race_z8_game);
    return race_nydus_set_source(SC64_Z8_GLOBAL_ID, sc64_z8_program_nydus,
                                 SC64_Z8_PROGRAM_NYDUS_COUNT);
}

static int race_z8_starting_nexus(void)
{
    unsigned n = 0;
    for (unsigned i = 0; i < race_z8_map->unit_count; ++i)
        n += race_z8_map->units[i].source_type == SC64_Z8_RAW_NEXUS &&
             race_z8_map->units[i].owner != SC64_Z8_HUMAN_SLOT;
    return (int)n;
}

static int race_z8_runtime_init(const Sc64RaceMap *map)
{
    Sc64Z8Setup setup;
    memset(&race_z8_sched, 0, sizeof(race_z8_sched));
    memset(&race_z8_ai, 0, sizeof(race_z8_ai));
    memset(race_z8_ai_minerals, 0, sizeof(race_z8_ai_minerals));
    memset(race_z8_ai_gas, 0, sizeof(race_z8_ai_gas));
    race_z8_map = 0;
    race_z8_product_slot = -1;
    race_z8_camera_release = 0;
    memset(race_z8_hidden, 0, sizeof(race_z8_hidden));
    race_z8_hidden_any = 0;
    if (!race_z8_active()) return 1;
    race_z8_setup(&setup);
    if (!sc64_z8_program_valid(&race_z8_program) ||
        !sc64_z8_init(&race_z8_sched, &race_z8_program, &setup) ||
        !race_z8_bind(map) || !race_nydus_bind_world(1)) return 0;
    race_z8_start_nexus = (u8)race_z8_starting_nexus();
    race_z8_update_cloak();
    return race_z8_start_nexus == 2u;
}

/* Every live actor must carry a source identity the world view can read. */
static int race_z8_live_world_valid(void)
{
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive) {
        int raw = u[i].raw_type;
        if (u[i].raw_owner < 0 || u[i].raw_owner >= SC64_Z8_OWNER_LIMIT ||
            race_z8_source_owner((unsigned)u[i].raw_owner) != u[i].owner ||
            raw < 0 || sc64_race_unit_from_raw(raw) != u[i].type) return 0;
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive) {
        if (b[i].raw_owner < 0 || b[i].raw_owner >= SC64_Z8_OWNER_LIMIT ||
            race_z8_source_owner((unsigned)b[i].raw_owner) != b[i].owner ||
            b[i].raw_type < 0 || sc64_race_building_from_raw(b[i].raw_type) < 0) return 0;
    }
    return 1;
}

static int race_z8_save_pack(Sc64Z8PersistentState *out)
{
    if (!race_z8_map || !sc64_z8_ai_state_valid(&race_z8_ai) ||
        race_z8_camera_release || !race_z8_live_world_valid()) return 0;
    memset(out, 0, sizeof(*out));
    if (!sc64_z8_state_pack(out, &race_z8_sched, &race_z8_program)) return 0;
    for (unsigned t = 0; t < SC64_Z8_AI_TOWNS; ++t) {
        out->ai_minerals[t] = race_z8_ai_minerals[t];
        out->ai_gas[t] = race_z8_ai_gas[t];
    }
    memcpy(out->ai, &race_z8_ai, sizeof(race_z8_ai));
    return 1;
}

/* Validate a candidate without touching live state; `scratch` is a
 * transient engine buffer. On success with `apply`, install it. */
static int race_z8_save_unpack(const Sc64Z8PersistentState *in,
                               Sc64Z8SchedulerState *scratch, int apply)
{
    Sc64Z8Setup setup;
    Sc64Z8AiState ai;
    race_z8_setup(&setup);
    memcpy(&ai, in->ai, sizeof(ai));
    if (!sc64_z8_ai_state_valid(&ai)) return 0;
    for (unsigned t = 0; t < SC64_Z8_AI_TOWNS; ++t)
        if (in->ai_minerals[t] > 65535u || in->ai_gas[t] > 65535u) return 0;
    if (!sc64_z8_state_unpack(apply ? &race_z8_sched : 0, in, &race_z8_program,
                              &setup, scratch)) return 0;
    if (!apply) return 1;
    race_z8_ai = ai;
    for (unsigned t = 0; t < SC64_Z8_AI_TOWNS; ++t) {
        race_z8_ai_minerals[t] = in->ai_minerals[t];
        race_z8_ai_gas[t] = in->ai_gas[t];
    }
    return 1;
}

static int race_z8_rebind_loaded_world(const Sc64RaceMap *map)
{
    race_z8_product_slot = -1;
    race_z8_camera_release = 0;
    if (!race_z8_active()) return 1;
    if (!race_z8_bind(map)) return 0;
    race_z8_start_nexus = (u8)race_z8_starting_nexus();
    race_z8_update_cloak();
    return race_z8_start_nexus == 2u && race_z8_live_world_valid();
}

/* Run one wallet-bounded engine action as if the live ENEMY bank were the
 * town's source wallet. */
static int race_z8_ai_queue(unsigned town, int producer, int type)
{
    int saved_minerals = race_minerals[ENEMY], saved_gas = race_gas[ENEMY], ok;
    race_minerals[ENEMY] = (int)race_z8_ai_minerals[town];
    race_gas[ENEMY] = (int)race_z8_ai_gas[town];
    ok = race_queue_unit(ENEMY, producer, type);
    race_z8_ai_minerals[town] = (u32)race_minerals[ENEMY];
    race_z8_ai_gas[town] = (u32)race_gas[ENEMY];
    race_minerals[ENEMY] = saved_minerals;
    race_gas[ENEMY] = saved_gas;
    return ok;
}

static void race_z8_ai_produce(unsigned town)
{
    unsigned used = 0;
    s16 slot = (s16)sc64_z8_ai_town_slot(town);
    for (int i = 0; i < MAX_UNITS; ++i) used += u[i].alive != 0;
    if (used + SC64_Z8_AI_RESERVE_UNITS >= MAX_UNITS) return;
    for (int i = 0; i < MAX_BUILDINGS; ++i) {
        int type = -1;
        if (!b[i].alive || b[i].owner != ENEMY || b[i].raw_owner != slot ||
            !building_complete(i) || b[i].queue_count) continue;
        if (b[i].type == B_GATEWAY)
            type = race_z8_ai.produce_index[town] & 1u ? U_DRAGOON : U_ZEALOT;
        else if (b[i].type == B_STARGATE) type = U_SCOUT;
        else if (b[i].type == B_ROBOTICS_FACILITY) type = U_REAVER;
        if (type >= 0 && race_z8_ai_queue(town, i, type))
            race_z8_ai.produce_index[town] = (u8)(race_z8_ai.produce_index[town] + 1u);
    }
}

static void race_z8_ai_wave(unsigned town)
{
    int tx = -1, ty = -1, sent = 0, guard = 0, want;
    s16 slot = (s16)sc64_z8_ai_town_slot(town);
    /* Aim at a human hive cluster, else the first human unit. */
    for (int i = 0; i < MAX_BUILDINGS && tx < 0; ++i) if (b[i].alive &&
        b[i].owner == SIDE && (b[i].raw_type == SC64_Z8_RAW_HATCHERY ||
        b[i].raw_type == SC64_Z8_RAW_LAIR || b[i].raw_type == SC64_Z8_RAW_HIVE)) {
        tx = b[i].x; ty = b[i].y;
    }
    for (int i = 0; i < MAX_UNITS && tx < 0; ++i) if (u[i].alive && u[i].owner == SIDE) {
        tx = u[i].x; ty = u[i].y;
    }
    if (tx < 0) return;
    want = SC64_Z8_AI_WAVE_BASE + 2 * (int)race_z8_ai.wave[town];
    if (want > SC64_Z8_AI_WAVE_MAX) want = SC64_Z8_AI_WAVE_MAX;
    for (int i = 0; i < MAX_UNITS && sent < want; ++i) if (u[i].alive &&
        u[i].owner == ENEMY && u[i].raw_owner == slot && u[i].order == ORDER_IDLE &&
        u[i].raw_type != SC64_Z8_RAW_HERO_DARK_TEMPLAR &&
        !sc64_race_is_worker(u[i].type) && sc64_race_unit_damage(u[i].type) > 0) {
        if (guard < SC64_Z8_AI_HOME_GUARD) { ++guard; continue; }
        u[i].order = ORDER_ATTACK_MOVE; u[i].target = -1;
        u[i].tx = (s16)tx; u[i].ty = (s16)ty; u[i].path_x = u[i].path_y = 0;
        ++sent; ++race_campaign.enemy_units_sent;
    }
    if (race_z8_ai.wave[town] < 255u) ++race_z8_ai.wave[town];
}

static void race_z8_ai_tick(void)
{
    for (unsigned t = 0; t < SC64_Z8_AI_TOWNS; ++t) {
        if (!(race_z8_ai.started & (1u << t))) continue;
        if (race_z8_ai.produce_timer[t]) --race_z8_ai.produce_timer[t];
        if (!race_z8_ai.produce_timer[t]) {
            race_z8_ai.produce_timer[t] = SC64_Z8_AI_PRODUCE_UPDATES;
            race_z8_ai_produce(t);
        }
        if (race_z8_ai.wave_timer[t]) --race_z8_ai.wave_timer[t];
        if (!race_z8_ai.wave_timer[t]) {
            race_z8_ai.wave_timer[t] = t ? SC64_Z8_AI_ZER8_WAVE_UPDATES :
                                           SC64_Z8_AI_ZE8T_WAVE_UPDATES;
            race_z8_ai_wave(t);
        }
    }
}

/* The scheduler advances four native steps per five active engine updates.
 * AI and cloak remain on the engine clock. A fault pauses the mission. */
static int race_z8_runtime_tick(void)
{
    int native_due;
    if (!race_z8_active()) return 1;
    if (!race_z8_map) return 0;
    native_due = sc64_z8_native_step((unsigned)frame);
    if (native_due && race_z8_camera_release) {
        race_z8_camera_release = 0;
        if (!sc64_z8_release_wait(&race_z8_sched, SC64_Z8_HUMAN_SLOT)) return 0;
    }
    race_z8_human_minerals = (u32)minerals;
    race_z8_human_gas = (u32)gas;
    if (native_due &&
        !sc64_z8_tick_prevalidated(&race_z8_sched, &race_z8_program,
                                   &sc64_z8_world_ops, &race_z8_view, 0)) return 0;
    if (race_z8_human_minerals > 65535u || race_z8_human_gas > 65535u) return 0;
    for (unsigned t = 0; t < SC64_Z8_AI_TOWNS; ++t)
        if (race_z8_ai_minerals[t] > 65535u || race_z8_ai_gas[t] > 65535u) return 0;
    minerals = race_minerals[SIDE] = (int)race_z8_human_minerals;
    gas = race_gas[SIDE] = (int)race_z8_human_gas;
    race_z8_ai_tick();
    race_z8_update_cloak();
    return 1;
}

static int race_z8_outcome(void)
{ return race_z8_sched.outcome[SC64_Z8_HUMAN_SLOT]; }

#endif /* SC64_Z8_RUNTIME */
#endif /* SC64_Z8_RUNTIME_ADAPTER_H */
