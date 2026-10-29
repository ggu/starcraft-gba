/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_P8_RUNTIME_ADAPTER_H
#define SC64_P8_RUNTIME_ADAPTER_H

#if defined(SC64_P8_RUNTIME)

#if !defined(SC64_P8_DATA_ENABLED) || !defined(SC64_P7_RUNTIME) || \
    !defined(SC64_ZP_FULL_STORAGE) || !defined(SC64_UNIFIED) || \
    !defined(SC64_CAMPAIGN_RACE) || SC64_CAMPAIGN_RACE != 2 || \
    !defined(SC64_Z8_GAME_POOL_BRIDGE) || !defined(SC64_P8_GAME_POOL_BRIDGE)
#error "P8 runtime requires authenticated P8 data on the full P1-P7 Protoss profile"
#endif

/* P8 "The Trial of Tassadar" campaign binding. The authenticated compact
 * source program runs through the shared P8 trigger runner for each of its
 * three executing source owners. Creates use the strict P8 property service;
 * census, owner, location and resource actions use the shared Z8 world
 * services over the live pools. Presentation strings stay in the private CHK
 * and are not displayed; Transmission uses an authored duration. */
#include "sc64_p8_state.h"
#include "sc64_p8_timed_service.h"
#include "sc64_p8_game_bridge.h"
#include "sc64_p8_ai.h"

/* SC64_P8_GLOBAL_ID (27) is defined with the forward declarations in game.c. */
enum {
    SC64_P8_MAP_UNITS = 256,
    SC64_P8_HUMAN_SLOT = 7,
    /* Engine updates are converted to source time at the native Fastest
     * speed. ElapsedTime, Wait and Transmission use the same clock. */
    SC64_P8_FRAME_MS = 42,
    /* Trigger passes are caller-scheduled; this cadence is authored. */
    SC64_P8_PASS_UPDATES = 8,
    /* Every P8 Transmission names blank.wav with modifier 9 (subtract) and
     * time 0, so its native duration is that sound's length. The sound
     * length is not imported; this bounded authored value stands in for it. */
    SC64_P8_TRANSMISSION_MS = 4000
};

static Sc64P8PersistentState race_p8;
static Sc64P8WorldContext race_p8_world;
static Sc64Z8WorldView race_p8_view;
static Sc64Z8GameContext race_p8_game;
static Sc64P8TimedService race_p8_timed;
static Sc64P8SourceWorldOps race_p8_source_ops;
static Sc64P8WorldOps race_p8_ops;
static Sc64Z8Rect race_p8_locations[SC64_P8_SOURCE_LOCATION_COUNT];
static u32 race_p8_elapsed_seconds;
static u32 race_p8_human_minerals, race_p8_human_gas;
static const Sc64RaceMap *race_p8_map;
/* Source slot for products of the building currently completing a queue. */
static s8 race_p8_product_slot = -1;

static const u8 sc64_p8_owner_modes_expected[8] = { 1, 2, 3, 1, 2, 2, 2, 0 };

static int race_p8_active(void)
{ return !race_mode && race_selected_mission == SC64_P8_GLOBAL_ID; }

static int race_p8_world_paused(void)
{ return race_p8_active() && race_p8.source_pause_active; }

static int race_p8_map_shape_valid(const Sc64RaceMap *map)
{
    if (!map || SC64_P8_GLOBAL_ID >= SC64_RACE_MAP_COUNT ||
        map != &sc64_race_maps[SC64_P8_GLOBAL_ID] ||
        map->width != SC64_P8_SOURCE_MAP_COLS ||
        map->height != SC64_P8_SOURCE_MAP_ROWS ||
        map->human_slot != SC64_P8_HUMAN_SLOT ||
        map->unit_count != SC64_P8_MAP_UNITS || !map->owner_modes ||
        !map->units || !map->mission_id || map->mission_id[0] != 'P' ||
        map->mission_id[1] != '8' || map->mission_id[2] != '\0' ||
        !sc64_p8_source_metadata_valid()) return 0;
    for (unsigned i = 0; i < 8; ++i)
        if (map->owner_modes[i] != sc64_p8_owner_modes_expected[i] ||
            sc64_p8_source_forces[i] > 3u) return 0;
    /* Runner forces are one-based; FORC stores zero-based force ids. */
    for (unsigned r = 0; r < SC64_P8_RUNNER_COUNT; ++r)
        if (sc64_p8_source_forces[sc64_p8_runner_slot[r]] + 1u !=
            sc64_p8_runner_force[r]) return 0;
    return 1;
}

static int race_p8_source_owner(unsigned slot)
{
    if (slot >= 8u || !race_p8_map) return -1;
    return slot == SC64_P8_HUMAN_SLOT ? SIDE : race_p8_map->owner_modes[slot];
}

/* Tag every engine-created actor with a source identity so the shared census
 * never sees an unowned row. Trigger creates overwrite these afterwards. */
static void race_p8_note_unit_spawn(int slot, int owner, int type)
{
    if (!race_p8_active() || slot < 0 || slot >= MAX_UNITS) return;
    u[slot].raw_type = (s16)sc64_race_raw_unit(type);
    u[slot].raw_owner = owner == SIDE ? SC64_P8_HUMAN_SLOT : race_p8_product_slot;
}

static void race_p8_note_building_spawn(int slot, int owner, int type)
{
    if (!race_p8_active() || slot < 0 || slot >= MAX_BUILDINGS) return;
    b[slot].raw_type = (s16)sc64_race_raw_building(type);
    b[slot].raw_owner = owner == SIDE ? SC64_P8_HUMAN_SLOT : -1;
}

static void race_p8_note_product_building(int building)
{
    race_p8_product_slot = race_p8_active() && building >= 0 &&
        building < MAX_BUILDINGS ? (s8)b[building].raw_owner : -1;
}

/* Transmission and CenterView. The human runner owns all such rows. */
static int race_p8_timed_presentation(void *context, int phase,
                                      const Sc64P8SourceAction *action,
                                      unsigned owner, const Sc64Z8Rect *rect,
                                      u32 *duration_ms)
{
    (void)context;
    if (!action || !rect || !duration_ms || owner != SC64_P8_HUMAN_SLOT)
        return -1;
    if (phase == SC64_P8_TIMED_ELIGIBLE || phase == SC64_P8_TIMED_FINISH)
        return 1;
    if (phase != SC64_P8_TIMED_START) return -1;
    if (action->opcode == 7u) {
        *duration_ms = SC64_P8_TRANSMISSION_MS;
        race_campaign_set_message(0);
        return 1;
    }
    if (action->opcode == 10u) {
        /* The camera snaps, so its pan-completion release follows on the
         * next update instead of after an animated pan. */
        cx = limit(((int)rect->left + (int)rect->right) / 4, 120,
                   race_cols * 16 - 120);
        cy = limit(((int)rect->top + (int)rect->bottom) / 4, 54,
                   race_rows * 16 - 54);
        race_p8.camera_release |= 1u << SC64_P8_RUNNER_HUMAN;
        return 1;
    }
    return -1;
}

/* PlayWAV, DisplayTextMessage and SetMissionObjectives have no world effect
 * here; their private strings and sounds are not imported. */
static int race_p8_present(void *context, const Sc64Z8Action *action,
                           unsigned owner, const Sc64Z8Rect *rect)
{
    (void)context; (void)rect;
    if (!action || owner >= 8u ||
        (action->opcode != 8u && action->opcode != 9u && action->opcode != 12u))
        return -1;
    return 1;
}

/* Native PauseGame freezes the game while trigger time continues; the game
 * update loop reads race_p8_world_paused() for that. */
static int race_p8_pause_backend(void *context, const Sc64P8SourceAction *action,
                                 unsigned owner, int paused)
{
    (void)context; (void)action;
    return owner == SC64_P8_HUMAN_SLOT && paused == 1 ? SC64_P8_PAUSE_APPLIED : -1;
}

static int race_p8_pause_end_backend(void *context, unsigned trigger,
                                     unsigned owner)
{
    (void)context;
    return owner == SC64_P8_HUMAN_SLOT && trigger < SC64_P8_TRIGGER_LIMIT ?
           SC64_P8_PAUSE_APPLIED : -1;
}

static int race_p8_run_ai(void *context, const Sc64P8SourceAction *action,
                          unsigned trigger_owner, const Sc64Z8Rect *rect)
{
    (void)context;
    return rect && sc64_p8_ai_start(&race_p8, action, trigger_owner) ? 1 : -1;
}

/* Rebuild the transient view, wallets, locations and timer bindings around
 * the persistent state. */
static int race_p8_bind(const Sc64RaceMap *map)
{
    s8 live[SC64_Z8_SOURCE_OWNER_LIMIT];
    if (!race_p8_map_shape_valid(map)) return 0;
    race_p8_map = map;
    for (unsigned slot = 0; slot < SC64_Z8_SOURCE_OWNER_LIMIT; ++slot)
        live[slot] = slot < 8u ? (s8)race_p8_source_owner(slot) : (s8)-1;
    memset(&race_p8_view, 0, sizeof(race_p8_view));
    memset(&race_p8_world, 0, sizeof(race_p8_world));
    if (!sc64_z8_game_context_init(&race_p8_game, live,
                                   SC64_P8_SOURCE_MAP_COLS * 32u,
                                   SC64_P8_SOURCE_MAP_ROWS * 32u)) return 0;
    sc64_z8_game_bind_actor_pool(&race_p8_view);
    /* All eight CHK players are census rows; the epilogue creates actors for
     * the otherwise unused slot 1. */
    race_p8_view.active_mask = 0xffu;
    for (unsigned slot = 0; slot < 8u; ++slot)
        race_p8_view.force[slot] = (u8)(sc64_p8_source_forces[slot] + 1u);
    race_p8_view.source_minerals[SC64_P8_HUMAN_SLOT] = &race_p8_human_minerals;
    race_p8_view.source_gas[SC64_P8_HUMAN_SLOT] = &race_p8_human_gas;
    race_p8_view.source_minerals[0] = &race_p8.ai_minerals[0];
    race_p8_view.source_gas[0] = &race_p8.ai_gas[0];
    race_p8_view.source_minerals[3] = &race_p8.ai_minerals[1];
    race_p8_view.source_gas[3] = &race_p8.ai_gas[1];
    race_p8_view.services.present = race_p8_present;
    sc64_p8_game_bind_default_create_service(&race_p8_view, &race_p8_game);

    if (!sc64_p8_world_locations_init(race_p8_locations,
                                      SC64_P8_SOURCE_LOCATION_COUNT)) return 0;
    if (race_p8.moved_location_valid) {
        Sc64Z8Rect *r = &race_p8_locations[SC64_P8_MOVED_LOCATION - 1];
        r->left = race_p8.moved_location[0]; r->top = race_p8.moved_location[1];
        r->right = race_p8.moved_location[2]; r->bottom = race_p8.moved_location[3];
    }
    race_p8_world.actors = &race_p8_view;
    race_p8_world.locations = race_p8_locations;
    race_p8_world.location_count = SC64_P8_SOURCE_LOCATION_COUNT;
    race_p8_world.map_pixel_width = SC64_P8_SOURCE_MAP_COLS * 32u;
    race_p8_world.map_pixel_height = SC64_P8_SOURCE_MAP_ROWS * 32u;
    race_p8_world.switches = race_p8.switches;
    race_p8_world.elapsed_seconds = &race_p8_elapsed_seconds;
    race_p8_world.run_ai = race_p8_run_ai;
    race_p8_world.pause_game = race_p8_pause_backend;
    race_p8_world.pause_trigger_end = race_p8_pause_end_backend;
    sc64_p8_game_bind_property_create_service(&race_p8_world, &race_p8_game);

    if (!sc64_p8_timed_service_init(&race_p8_timed, SC64_P8_FRAME_MS)) return 0;
    for (unsigned r = 0; r < SC64_P8_RUNNER_COUNT; ++r) {
        unsigned slot = sc64_p8_runner_slot[r];
        if (race_p8.waiting_mask & (1u << r)) {
            race_p8_timed.waiting_mask |= (u8)(1u << slot);
            race_p8_timed.wait_ms[slot] = race_p8.wait_ms[r];
        }
        race_p8_timed.pause_trigger_mask[slot] = race_p8.pause_trigger_mask[r];
    }
    race_p8_timed.source_pause_active = race_p8.source_pause_active;
    race_p8_timed.presentation = race_p8_timed_presentation;
    sc64_p8_timed_service_bind_world(&race_p8_world, &race_p8_timed);
    if (!sc64_p8_timed_service_valid(&race_p8_timed)) return 0;

    race_p8_source_ops = sc64_p8_world_ops(&race_p8_world);
    race_p8_ops = sc64_p8_source_world_ops(&race_p8_source_ops);
    return 1;
}

/* Copy the live timer service back into the persistent record. */
static int race_p8_capture_timed(void)
{
    u8 others = race_p8_timed.waiting_mask;
    race_p8.waiting_mask = 0;
    for (unsigned r = 0; r < SC64_P8_RUNNER_COUNT; ++r) {
        unsigned slot = sc64_p8_runner_slot[r];
        u8 bit = (u8)(1u << slot);
        race_p8.wait_ms[r] = (race_p8_timed.waiting_mask & bit) ?
                             race_p8_timed.wait_ms[slot] : 0u;
        if (race_p8_timed.waiting_mask & bit) race_p8.waiting_mask |= (u8)(1u << r);
        race_p8.pause_trigger_mask[r] = race_p8_timed.pause_trigger_mask[slot];
        others &= (u8)~bit;
    }
    race_p8.source_pause_active = race_p8_timed.source_pause_active;
    race_p8.camera_release &= race_p8.waiting_mask;
    return !others;
}

static int race_p8_runtime_state_valid(void)
{
    if (!sc64_p8_persistent_state_valid(&race_p8)) return 0;
    if (race_p8_map) {
        Sc64P8SourceWorldOps source = sc64_p8_world_ops(&race_p8_world);
        Sc64P8WorldOps ops = sc64_p8_source_world_ops(&source);
        for (unsigned r = 0; r < SC64_P8_RUNNER_COUNT; ++r)
            if (!sc64_p8_runtime_state_valid(&race_p8.runtime[r], &ops)) return 0;
    }
    return 1;
}

static int race_p8_runtime_init(const Sc64RaceMap *map)
{
    memset(&race_p8, 0, sizeof(race_p8));
    race_p8_map = 0;
    race_p8_product_slot = -1;
    if (!race_p8_active()) return 1;
    for (unsigned r = 0; r < SC64_P8_RUNNER_COUNT; ++r)
        if (!sc64_p8_source_runtime_init(&race_p8.runtime[r],
                                         sc64_p8_runner_slot[r],
                                         sc64_p8_runner_force[r])) return 0;
    if (!race_p8_bind(map)) return 0;
    return race_p8_runtime_state_valid();
}

static int race_p8_rebind_loaded_world(const Sc64RaceMap *map)
{
    race_p8_product_slot = -1;
    if (!race_p8_active()) return 1;
    return sc64_p8_persistent_state_valid(&race_p8) && race_p8_bind(map) &&
           race_p8_runtime_state_valid();
}

/* Bounded contact rescue (as in P4/P6): Fenix and the Hyperion join the
 * human side when a human unit comes within 32 engine pixels. They keep
 * their source slot, which the Fenix/Hyperion defeat census counts. */
static void race_p8_rescue_tick(void)
{
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == RESCUABLE) {
        for (int j = 0; j < MAX_UNITS; ++j) if (u[j].alive && u[j].owner == SIDE &&
            u[j].raw_owner == SC64_P8_HUMAN_SLOT && u[j].raw_type != 101) {
            int dx = u[i].x - u[j].x, dy = u[i].y - u[j].y;
            if (dx * dx + dy * dy <= 32 * 32) {
                u[i].owner = SIDE; u[i].selected = 0; u[i].target = -1;
                u[i].order = ORDER_IDLE; u[i].tx = u[i].x; u[i].ty = u[i].y;
                break;
            }
        }
    }
}

/* One engine update: timers every update, the source pass on its cadence,
 * then the town AI. Returns 0 on any fault; the caller pauses the mission. */
static int race_p8_runtime_tick(u32 elapsed_updates)
{
    if (!race_p8_active()) return 1;
    if (!race_p8_map || !sc64_p8_persistent_state_valid(&race_p8)) return 0;
    for (unsigned r = 0; r < SC64_P8_RUNNER_COUNT; ++r)
        if ((race_p8.camera_release & (1u << r)) &&
            !sc64_p8_timed_service_release_wait(&race_p8_timed,
                                               sc64_p8_runner_slot[r])) return 0;
    race_p8.camera_release = 0;
    if (!sc64_p8_timed_service_tick(&race_p8_timed, 0)) return 0;
    race_p8_elapsed_seconds = (u32)(((unsigned long long)elapsed_updates *
                                     SC64_P8_FRAME_MS) / 1000u);
    if (race_p8.pass_timer) --race_p8.pass_timer;
    if (!race_p8.pass_timer) {
        race_p8.pass_timer = SC64_P8_PASS_UPDATES;
        race_p8_human_minerals = (u32)minerals;
        race_p8_human_gas = (u32)gas;
        for (unsigned r = 0; r < SC64_P8_RUNNER_COUNT; ++r)
            if (!sc64_p8_runtime_step(&race_p8.runtime[r], &race_p8_ops, 0))
                return 0;
        if (race_p8_human_minerals > 65535u || race_p8_human_gas > 65535u)
            return 0;
        minerals = race_minerals[SIDE] = (int)race_p8_human_minerals;
        gas = race_gas[SIDE] = (int)race_p8_human_gas;
        {
            const Sc64Z8Rect *r = &race_p8_locations[SC64_P8_MOVED_LOCATION - 1];
            const Sc64P8SourceLocation *src =
                &sc64_p8_source_locations[SC64_P8_MOVED_LOCATION - 1];
            if (r->left != (s16)src->left || r->top != (s16)src->top ||
                r->right != (s16)src->right || r->bottom != (s16)src->bottom) {
                race_p8.moved_location_valid = 1;
                race_p8.moved_location[0] = r->left;
                race_p8.moved_location[1] = r->top;
                race_p8.moved_location[2] = r->right;
                race_p8.moved_location[3] = r->bottom;
            }
        }
    }
    if (!race_p8_capture_timed()) return 0;
    if (!race_p8.source_pause_active) {
        race_p8_rescue_tick();
        sc64_p8_ai_tick(&race_p8, elapsed_updates);
    }
    return sc64_p8_persistent_state_valid(&race_p8);
}

static int race_p8_outcome(void)
{ return race_p8.runtime[SC64_P8_RUNNER_HUMAN].outcome; }

#endif /* SC64_P8_RUNTIME */
#endif /* SC64_P8_RUNTIME_ADAPTER_H */
