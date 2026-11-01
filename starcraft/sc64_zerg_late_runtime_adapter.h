/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_ZERG_LATE_RUNTIME_ADAPTER_H
#define SC64_ZERG_LATE_RUNTIME_ADAPTER_H

#if defined(SC64_Z9_RUNTIME)

#if !defined(SC64_Z9_DATA_ENABLED) || !defined(SC64_Z8_RUNTIME) || \
    !defined(SC64_Z8_GAME_POOL_BRIDGE) || !defined(SC64_CAMPAIGN_RACE) || \
    SC64_CAMPAIGN_RACE != 1
#error "Z9 runtime requires authenticated Z9 data on the Z1-Z8 Zerg profile"
#endif
#if defined(SC64_Z10_RUNTIME) && !defined(SC64_Z10_DATA_ENABLED)
#error "Z10 runtime requires authenticated Z10 data"
#endif

/* Z9 "The Invasion of Aiur" and Z10 "Full Circle" campaign binding. Each
 * authenticated program runs through the late-Zerg scheduler
 * (sc64_zerg_late_triggers.h) with the Z8 native cadence. Census, owner,
 * location and resource actions reuse the Z8 world adapter over the live
 * pools. CreateUnit, RemoveUnitAtLocation, the AI scripts, the Kill/Deaths
 * censuses and the Khaydarin Crystal carry are authored here and documented
 * at each function. Mission strings and sounds stay in the private CHK and
 * are not shown or played. Only one of the two missions is active at once,
 * so they share the live state below. */
#include "sc64_zerg_late_ai.h"
#include "sc64_z9_program_generated.h"
#ifdef SC64_Z10_RUNTIME
#include "sc64_z10_program_generated.h"
#endif

/* SC64_Z9_GLOBAL_ID (20) and SC64_Z10_GLOBAL_ID (21) are defined with the
 * forward declarations in game.c. */
enum {
    SC64_ZL_HUMAN_SLOT = 3,
    /* Native Fastest speed-table entry, as Z8; applied once per native step. */
    SC64_ZL_FRAME_MS = 42,
    /* Authored stand-in for the unimported Transmission sound length. */
    SC64_ZL_TRANSMISSION_MS = 4000,
    SC64_ZL_RAW_DRONE = 41,
    SC64_ZL_RAW_CRYSTAL = 219,
    SC64_ZL_RAW_HATCHERY = 131,
    SC64_ZL_RAW_LAIR = 132,
    SC64_ZL_RAW_HIVE = 133,
    /* Pinned BWAPI raw-219 dimensions, in source pixels. */
    SC64_ZL_CRYSTAL_HALF = 16,
    /* A human Drone this close (engine pixels) picks up a loose crystal. */
    SC64_ZL_CRYSTAL_PICKUP_PX = 16
};

#define SC64_ZL_FOURCC(a, b, c, d) \
    ((u32)(u8)(a) | ((u32)(u8)(b) << 8) | ((u32)(u8)(c) << 16) | ((u32)(u8)(d) << 24))

typedef struct {
    u8 global_id;
    const char *mission_id;
    u16 map_units, map_cols, map_rows;
    u8 owner_modes[8];
    /* Source slot to live owner; -1 for inactive slots. */
    s8 live_owner[SC64_Z8_SOURCE_OWNER_LIMIT];
    const Sc64ZlProgram *program;
    const u8 *forces;
    u8 active_mask;
    const Sc64Z8PermissionTable *permissions;
    u8 town_slot[SC64_ZL_AI_TOWNS];
    const Sc64ZlAiScript *scripts;
    u8 script_count;
    /* Kill census raw type (0 for none) and Deaths census slot mask. */
    u8 kill_raw;
    u8 deaths_slots;
} Sc64ZlMission;

static const u32 race_z9_ai_scripts[] = {
    SC64_ZL_FOURCC('V','l','u','A'), SC64_ZL_FOURCC('Z','9','l','o'),
    SC64_ZL_FOURCC('Z','9','l','y'), SC64_ZL_FOURCC('Z','9','r','o'),
    SC64_ZL_FOURCC('Z','9','r','y'), SC64_ZL_FOURCC('Z','e','r','9')
};
static const Sc64ZlProgram race_z9_program = {
    { sc64_z9_program_triggers, sc64_z9_program_conditions,
      sc64_z9_program_actions, sc64_z9_program_locations,
      SC64_Z9_PROGRAM_TRIGGER_COUNT, SC64_Z9_PROGRAM_CONDITION_COUNT,
      SC64_Z9_PROGRAM_ACTION_COUNT, SC64_Z9_PROGRAM_LOCATION_COUNT,
      SC64_Z9_PROGRAM_MAP_PIXEL_WIDTH, SC64_Z9_PROGRAM_MAP_PIXEL_HEIGHT },
    race_z9_ai_scripts, 6
};
/* Z9 towns: slot 2 (force 4, Zer9) starts at once; slot 7 (force 2) when the
 * human enters its approaches; slot 4 (force 3) when the human enters the
 * southern region. VluA's native meaning is not established. */
static const Sc64ZlAiScript race_z9_ai_policy[] = {
    { SC64_ZL_FOURCC('Z','e','r','9'), 2, 0 },
    { SC64_ZL_FOURCC('Z','9','l','y'), 7, 1 },
    { SC64_ZL_FOURCC('Z','9','r','y'), 7, 1 },
    { SC64_ZL_FOURCC('Z','9','l','o'), 4, 2 },
    { SC64_ZL_FOURCC('Z','9','r','o'), 4, 2 },
    { SC64_ZL_FOURCC('V','l','u','A'), 7, SC64_ZL_AI_NO_TOWN },
    { SC64_ZL_FOURCC('V','l','u','A'), 4, SC64_ZL_AI_NO_TOWN }
};
static const Sc64ZlMission race_z9_mission = {
    SC64_Z9_GLOBAL_ID, "Z9", 371, 128, 128,
    { 3, 2, 1, 0, 1, 2, 1, 1 },
    /* Slot 0 holds a rescuable-owner Zerg beacon, slot 6 the Khaydarin
     * formation and slot 11 the resources; all stay neutral. */
    { NEUTRAL, -1, ENEMY, SIDE, ENEMY, -1, NEUTRAL, ENEMY, -1, -1, -1, NEUTRAL },
    &race_z9_program, sc64_z9_program_forces, SC64_Z9_PROGRAM_ACTIVE_MASK,
    &sc64_z9_program_permissions,
    { 2, 7, 4 }, race_z9_ai_policy,
    sizeof(race_z9_ai_policy) / sizeof(race_z9_ai_policy[0]),
    0, 0
};

#ifdef SC64_Z10_RUNTIME
static const u32 race_z10_ai_scripts[] = {
    SC64_ZL_FOURCC('Z','1','0','a'), SC64_ZL_FOURCC('Z','1','0','b'),
    SC64_ZL_FOURCC('Z','1','0','c'), SC64_ZL_FOURCC('Z','1','0','d'),
    SC64_ZL_FOURCC('Z','1','0','e')
};
static const Sc64ZlProgram race_z10_program = {
    { sc64_z10_program_triggers, sc64_z10_program_conditions,
      sc64_z10_program_actions, sc64_z10_program_locations,
      SC64_Z10_PROGRAM_TRIGGER_COUNT, SC64_Z10_PROGRAM_CONDITION_COUNT,
      SC64_Z10_PROGRAM_ACTION_COUNT, SC64_Z10_PROGRAM_LOCATION_COUNT,
      SC64_Z10_PROGRAM_MAP_PIXEL_WIDTH, SC64_Z10_PROGRAM_MAP_PIXEL_HEIGHT },
    race_z10_ai_scripts, 5
};
/* Z10 towns: slot 2 (force 3, Z10a/Z10b) and slot 7 (force 4, Z10c/Z10d)
 * start at once; slot 0 (force 2, the Temple's guard, Z10e) starts after
 * its first death or once no Temple remains. */
static const Sc64ZlAiScript race_z10_ai_policy[] = {
    { SC64_ZL_FOURCC('Z','1','0','a'), 2, 0 },
    { SC64_ZL_FOURCC('Z','1','0','b'), 2, 0 },
    { SC64_ZL_FOURCC('Z','1','0','c'), 7, 1 },
    { SC64_ZL_FOURCC('Z','1','0','d'), 7, 1 },
    { SC64_ZL_FOURCC('Z','1','0','e'), 0, 2 }
};
static const Sc64ZlMission race_z10_mission = {
    SC64_Z10_GLOBAL_ID, "Z10", 337, 96, 128,
    { 1, 2, 1, 0, 2, 2, 2, 1 },
    /* Slot 11 holds the resources; slots 1 and 4-6 are inactive and own no
     * map rows. */
    { ENEMY, -1, ENEMY, SIDE, -1, -1, -1, ENEMY, -1, -1, -1, NEUTRAL },
    &race_z10_program, sc64_z10_program_forces, SC64_Z10_PROGRAM_ACTIVE_MASK,
    &sc64_z10_program_permissions,
    { 2, 7, 0 }, race_z10_ai_policy,
    sizeof(race_z10_ai_policy) / sizeof(race_z10_ai_policy[0]),
    174, 1u << 0
};
#endif

static Sc64ZlSchedulerState race_zl_sched;
static Sc64ZlAiState race_zl_ai;
static u32 race_zl_ai_minerals[SC64_ZL_AI_TOWNS], race_zl_ai_gas[SC64_ZL_AI_TOWNS];
/* Z8 is never active together with Z9 or Z10, and only the active mission
 * binds its view, so they share the Z8 world-view object (EWRAM is tight). */
#define race_zl_view race_z8_view
static u32 race_zl_human_minerals, race_zl_human_gas;
static u8 race_zl_camera_release;
static u8 race_zl_crystal_carrier = SC64_ZL_NO_CARRIER;
static const Sc64RaceMap *race_zl_map;
static const Sc64ZlMission *race_zl_mission;
/* Source slot for products of the building currently completing a queue. */
static s8 race_zl_product_slot = -1;

static const Sc64ZlMission *race_zl_mission_for(int mission)
{
    if (mission == SC64_Z9_GLOBAL_ID) return &race_z9_mission;
#ifdef SC64_Z10_RUNTIME
    if (mission == SC64_Z10_GLOBAL_ID) return &race_z10_mission;
#endif
    return 0;
}

static int race_zl_active(void)
{ return !race_mode && race_zl_mission_for(race_selected_mission) != 0; }

static int race_zl_text_equal(const char *a, const char *b)
{
    if (!a || !b) return 0;
    while (*a && *a == *b) { ++a; ++b; }
    return *a == *b;
}

static int race_zl_map_shape_valid(const Sc64RaceMap *map)
{
    const Sc64ZlMission *m = race_zl_mission_for(race_selected_mission);
    if (!m || !map || m->global_id >= SC64_RACE_MAP_COUNT ||
        map != &sc64_race_maps[m->global_id] ||
        map->width != m->map_cols || map->height != m->map_rows ||
        map->human_slot != SC64_ZL_HUMAN_SLOT || map->unit_count != m->map_units ||
        !map->owner_modes || !map->units ||
        !race_zl_text_equal(map->mission_id, m->mission_id)) return 0;
    for (unsigned i = 0; i < 8; ++i)
        if (map->owner_modes[i] != m->owner_modes[i]) return 0;
    return 1;
}

static int race_zl_source_owner(unsigned slot)
{
    const Sc64ZlMission *m = race_zl_mission_for(race_selected_mission);
    return m && slot < SC64_Z8_SOURCE_OWNER_LIMIT ? m->live_owner[slot] : -1;
}

/* PUNI is source-slot scoped, as in Z8; only colony products use it. */
static int race_zl_colony_product_available(int owner, int raw_owner, int type)
{
    const Sc64ZlMission *m = race_zl_mission_for(race_selected_mission);
    int raw = sc64_race_raw_building(type);
    if (!m || race_mode || owner != SIDE || raw_owner < 0 ||
        race_zl_source_owner((unsigned)raw_owner) != owner || raw < 0) return 0;
    return sc64_z8_permission_available(m->permissions, (unsigned)raw_owner,
                                        (unsigned)raw);
}

static void race_zl_note_unit_spawn(int slot, int owner, int type)
{
    if (!race_zl_active() || slot < 0 || slot >= MAX_UNITS) return;
    u[slot].raw_type = (s16)sc64_race_raw_unit(type);
    u[slot].raw_owner = owner == SIDE ? SC64_ZL_HUMAN_SLOT : race_zl_product_slot;
}

static void race_zl_note_building_spawn(int slot, int owner, int type)
{
    if (!race_zl_active() || slot < 0 || slot >= MAX_BUILDINGS) return;
    b[slot].raw_type = (s16)sc64_race_raw_building(type);
    b[slot].raw_owner = owner == SIDE ? SC64_ZL_HUMAN_SLOT : -1;
}

static void race_zl_note_product_building(int building)
{
    race_zl_product_slot = race_zl_active() && building >= 0 &&
        building < MAX_BUILDINGS ? (s8)b[building].raw_owner : -1;
}

static int race_zl_timed(void *context, int phase, const Sc64Z8Action *action,
                         unsigned owner, const Sc64Z8Rect *rect, u32 *duration_ms)
{
    (void)context;
    if (!action || !duration_ms || owner != SC64_ZL_HUMAN_SLOT) return -1;
    if (phase == SC64_Z8_TIMED_ELIGIBLE || phase == SC64_Z8_TIMED_FINISH) return 1;
    if (phase != SC64_Z8_TIMED_START) return -1;
    if (action->opcode == 7u) {
        *duration_ms = SC64_ZL_TRANSMISSION_MS;
        race_campaign_set_message(0);
        return 1;
    }
    if (action->opcode == 10u && rect) {
        /* The camera snaps, so its pan-completion release follows on the
         * next native step instead of after an animated pan. */
        cx = limit(((int)rect->left + (int)rect->right) / 4, 120, race_cols * 16 - 120);
        cy = limit(((int)rect->top + (int)rect->bottom) / 4, 54, race_rows * 16 - 54);
        *duration_ms = SC64_Z8_WAIT_INDEFINITE;
        race_zl_camera_release = 1;
        return 1;
    }
    return -1;
}

/* PlayWAV, DisplayTextMessage, SetMissionObjectives and MinimapPing have no
 * world effect here; their private strings and sounds are not imported. */
static int race_zl_present(void *context, const Sc64Z8Action *action,
                           unsigned owner, const Sc64Z8Rect *rect)
{
    (void)context; (void)rect;
    if (!action || owner >= SC64_Z8_OWNER_LIMIT ||
        (action->opcode != 8u && action->opcode != 9u && action->opcode != 12u &&
         action->opcode != 28u)) return -1;
    return 1;
}

/* Starting map census of non-human actors of one raw type or source slot;
 * the Kill and Deaths stand-ins compare it with the live pools. */
static unsigned race_zl_map_count(unsigned raw, int slot)
{
    unsigned n = 0;
    for (unsigned i = 0; race_zl_map && i < race_zl_map->unit_count; ++i) {
        const Sc64ImportedUnit *src = &race_zl_map->units[i];
        if (src->owner == SC64_ZL_HUMAN_SLOT) continue;
        if (raw && src->source_type == raw) ++n;
        else if (!raw && slot >= 0 && src->owner == slot &&
                 (sc64_race_unit_from_raw(src->source_type) >= 0 ||
                  sc64_race_building_from_raw(src->source_type) >= 0)) ++n;
    }
    return n;
}

static unsigned race_zl_live_count(unsigned raw, int slot)
{
    unsigned n = 0;
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].raw_owner != SC64_ZL_HUMAN_SLOT)
        n += raw ? u[i].raw_type == (s16)raw : u[i].raw_owner == slot;
    for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive && b[i].raw_owner != SC64_ZL_HUMAN_SLOT)
        n += raw ? b[i].raw_type == (s16)raw : b[i].raw_owner == slot;
    return n;
}

/* The engine keeps no per-player kill ledger. Only the human fights the
 * mission's Kill target, so a destroyed starting target counts as a human
 * kill: the same authored stand-in as Z8's Nexus census. */
static int race_zl_kill_count(void *context, unsigned source_owner,
                              unsigned raw_type, u32 *count)
{
    unsigned start, alive;
    (void)context;
    if (!count || !race_zl_mission || !race_zl_mission->kill_raw ||
        source_owner != SC64_ZL_HUMAN_SLOT || raw_type != race_zl_mission->kill_raw)
        return -1;
    start = race_zl_map_count(raw_type, -1);
    alive = race_zl_live_count(raw_type, -1);
    *count = alive < start ? start - alive : 0u;
    return 1;
}

/* Deaths stand-in: starting actors of the slot minus those alive. The only
 * Deaths row (Z10) reads a slot that produces nothing until it fires. */
static int race_zl_deaths(void *context, const Sc64Z8Condition *c,
                          unsigned owner, u32 *count)
{
    unsigned slot, start, alive;
    (void)context;
    if (!c || !count || !race_zl_mission ||
        !sc64_z8_world_resolve_one(&race_zl_view, c->player, owner, &slot) ||
        !(race_zl_mission->deaths_slots & (1u << slot)) || c->unit != 229u) return -1;
    start = race_zl_map_count(0, (int)slot);
    alive = race_zl_live_count(0, (int)slot);
    *count = alive < start ? start - alive : 0u;
    return 1;
}

static int race_zl_run_ai(void *context, const Sc64Z8Action *action,
                          unsigned owner, const Sc64Z8Rect *rect)
{
    const Sc64ZlMission *m = race_zl_mission;
    (void)context;
    if (!m || !action || !rect) return -1;
    for (unsigned i = 0; i < m->script_count; ++i) {
        const Sc64ZlAiScript *s = &m->scripts[i];
        if (s->fourcc != action->group2 || s->owner != owner) continue;
        if (s->town != SC64_ZL_AI_NO_TOWN) sc64_zl_ai_start_town(&race_zl_ai, s->town);
        return 1;
    }
    return -1;
}

/* Clear one live unit without a death, as the P5/T5 RemoveUnit actions do. */
static void race_zl_clear_unit(int i)
{
    u[i].alive = 0; u[i].selected = 0; u[i].target = -1;
    u[i].order = ORDER_IDLE; invincible_units[i] = 0;
    if (race_zl_crystal_carrier == i) race_zl_crystal_carrier = SC64_ZL_NO_CARRIER;
    for (int j = 0; j < MAX_UNITS; ++j)
        if (u[j].alive && u[j].order == ORDER_ATTACK_TARGET && u[j].target == i) {
            u[j].order = ORDER_IDLE; u[j].target = -1;
        }
}

/* RemoveUnitAtLocation: every matching unit of the resolved owner whose
 * footprint overlaps the location. Modifier 0 means all; only unit rows are
 * authored (Z9 removes its entering Drone). Matches are validated before the
 * first removal so a bad view row cannot leave a partial action. */
static int race_zl_remove(void *context, const Sc64Z8Action *a,
                          unsigned owner, const Sc64Z8Rect *rect)
{
    unsigned target;
    u8 remove[MAX_UNITS];
    int changed = 0;
    (void)context;
    if (!a || !rect || a->modifier || a->unit >= 228u ||
        sc64_race_unit_from_raw(a->unit) < 0 ||
        !sc64_z8_world_resolve_one(&race_zl_view, a->group1, owner, &target)) return -1;
    memset(remove, 0, sizeof(remove));
    for (int i = 0; i < MAX_UNITS; ++i) {
        Sc64Z8ActorView view;
        int overlap;
        if (!u[i].alive || u[i].raw_owner != (s16)target || u[i].raw_type != (s16)a->unit)
            continue;
        if (sc64_z8_world_actor_at(&race_zl_view, (unsigned)i, &view) != 1) return -1;
        overlap = sc64_z8_world_overlap(&view, rect);
        if (overlap < 0) return -1;
        remove[i] = overlap && !SC64_TRANSPORT_LOADED(&u[i]);
    }
    for (int i = 0; i < MAX_UNITS; ++i) if (remove[i]) { race_zl_clear_unit(i); changed = 1; }
    if (changed) {
        supply_used = race_owner_supply(SIDE);
        supply_cap = race_owner_supply_cap(SIDE);
        target_lists_valid = 0; sc64_unit_bucket_active = 0;
        sc64_target_index_end();
    }
    return 1;
}

/* The crystal has no Z7/P7 raw rectangle; give it its pinned dimensions and
 * read every other actor through the Z8 pool bridge. */
static int race_zl_actor_at(void *context, unsigned index, Sc64Z8ActorView *out)
{
    if (out && index < MAX_UNITS && u[index].alive &&
        u[index].raw_type == SC64_ZL_RAW_CRYSTAL) {
        const Unit *q = &u[index];
        if (q->raw_owner < 0 || q->raw_owner >= SC64_Z8_SOURCE_OWNER_LIMIT ||
            q->type != U_CHRYSALIS) return -1;
        *out = (Sc64Z8ActorView){0};
        out->alive = 1; out->completed = 1; out->type_known = 1;
        out->raw_type = SC64_ZL_RAW_CRYSTAL;
        out->source_owner = q->raw_owner;
        out->x = (s16)q->x; out->y = (s16)q->y;
        out->left = out->top = SC64_ZL_CRYSTAL_HALF;
        out->right = out->bottom = SC64_ZL_CRYSTAL_HALF - 1;
        out->invincible = &invincible_units[index];
        return 1;
    }
    return sc64_z8_game_actor_at(context, index, out);
}

/* CreateUnitWithProperties: Z9 and Z10 create one human Drone and one
 * Khaydarin Crystal per row, with default properties. The centre and
 * footprint clamp follow the Z8 bridge; the crystal is an invincible
 * powerup actor carried by a human Drone. */
static int race_zl_create(void *opaque, const Sc64Z8Action *action,
                          unsigned source_owner, unsigned count,
                          unsigned property_id, unsigned x1, unsigned y1,
                          unsigned x2, unsigned y2)
{
    const Sc64ZlMission *m = race_zl_mission;
    unsigned width, height;
    int raw, type, live_owner, left, top, right, bottom, x, y, slot;
    (void)opaque;
    if (!m || !action || action->opcode != 11u || action->flags != 0x10u ||
        action->modifier != 0u || action->group2 != 0u || property_id != 0u ||
        count != 1u || !action->location || action->string_id || action->wav_id ||
        action->time || source_owner != SC64_ZL_HUMAN_SLOT || x2 < x1 || y2 < y1)
        return -1;
    width = m->program->rows.map_pixel_width;
    height = m->program->rows.map_pixel_height;
    if (x2 > width || y2 > height) return -1;
    raw = action->unit;
    if (raw == SC64_ZL_RAW_DRONE) {
        type = U_DRONE;
        if (!sc64_z8_game_source_extents((unsigned)raw, 0, 0, 0,
                                         &left, &top, &right, &bottom)) return -1;
    } else if (raw == SC64_ZL_RAW_CRYSTAL) {
        type = U_CHRYSALIS;
        left = top = SC64_ZL_CRYSTAL_HALF;
        right = bottom = SC64_ZL_CRYSTAL_HALF - 1;
    } else {
        return -1;
    }
    x = (int)((x1 + x2) / 2u);
    y = (int)((y1 + y2) / 2u);
    if (x < left) x = left;
    if (y < top) y = top;
    if (x + right >= (int)width) x = (int)width - right - 1;
    if (y + bottom >= (int)height) y = (int)height - bottom - 1;
    if (x < left || y < top) return -1;
    live_owner = m->live_owner[source_owner];
    if (live_owner != SIDE || find_unit_slot() < 0) return -1;
    slot = spawn_unit(live_owner, type, x / 2, y / 2);
    if (slot < 0) return -1;
    u[slot].raw_type = (s16)raw;
    u[slot].raw_owner = (s16)source_owner;
    if (raw == SC64_ZL_RAW_CRYSTAL) {
        invincible_units[slot] = 1;
        u[slot].selected = 0;
        race_zl_crystal_carrier = SC64_ZL_NO_CARRIER;
    }
    return 1;
}

static const Sc64ZlWorldOps race_zl_world_ops = {
    { sc64_z8_world_count, sc64_z8_world_action, sc64_z8_world_timed,
      sc64_z8_world_find_unit },
    race_zl_run_ai, race_zl_remove, race_zl_deaths
};

static void race_zl_setup(const Sc64ZlMission *m, Sc64Z8Setup *setup)
{
    setup->active_mask = m->active_mask;
    for (unsigned o = 0; o < SC64_Z8_OWNER_LIMIT; ++o) setup->force[o] = m->forces[o];
    setup->frame_ms = SC64_ZL_FRAME_MS;
}

/* Rebuild the transient view and wallet bindings around the live state. */
static int race_zl_bind(const Sc64RaceMap *map)
{
    const Sc64ZlMission *m = race_zl_mission_for(race_selected_mission);
    if (!m || !race_zl_map_shape_valid(map)) return 0;
    race_zl_mission = m;
    race_zl_map = map;
    memset(&race_zl_view, 0, sizeof(race_zl_view));
    race_zl_view.actors = 0;
    race_zl_view.actor_count = (u16)(MAX_UNITS + MAX_BUILDINGS);
    race_zl_view.actor_at = race_zl_actor_at;
    race_zl_view.active_mask = m->active_mask;
    for (unsigned o = 0; o < SC64_Z8_OWNER_LIMIT; ++o) race_zl_view.force[o] = m->forces[o];
    race_zl_view.source_minerals[SC64_ZL_HUMAN_SLOT] = &race_zl_human_minerals;
    race_zl_view.source_gas[SC64_ZL_HUMAN_SLOT] = &race_zl_human_gas;
    for (unsigned t = 0; t < SC64_ZL_AI_TOWNS; ++t) {
        race_zl_view.source_minerals[m->town_slot[t]] = &race_zl_ai_minerals[t];
        race_zl_view.source_gas[m->town_slot[t]] = &race_zl_ai_gas[t];
    }
    race_zl_view.services.present = race_zl_present;
    race_zl_view.services.kill_count = race_zl_kill_count;
    race_zl_view.services.timed = race_zl_timed;
    race_zl_view.services.create = race_zl_create;
    return 1;
}

/* Every live actor must carry a source identity the world view can read. */
static int race_zl_live_world_valid(void)
{
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive) {
        int raw = u[i].raw_type;
        if (u[i].raw_owner < 0 || u[i].raw_owner >= SC64_Z8_OWNER_LIMIT ||
            race_zl_source_owner((unsigned)u[i].raw_owner) != u[i].owner ||
            raw < 0 || sc64_race_unit_from_raw(raw) != u[i].type) return 0;
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive) {
        if (b[i].raw_owner < 0 || b[i].raw_owner >= SC64_Z8_OWNER_LIMIT ||
            race_zl_source_owner((unsigned)b[i].raw_owner) != b[i].owner ||
            b[i].raw_type < 0 || sc64_race_building_from_raw(b[i].raw_type) < 0) return 0;
    }
    if (race_zl_crystal_carrier != SC64_ZL_NO_CARRIER) {
        int c = race_zl_crystal_carrier;
        if (c >= MAX_UNITS || !u[c].alive || u[c].owner != SIDE ||
            u[c].raw_type != SC64_ZL_RAW_DRONE) return 0;
    }
    return 1;
}

static int race_zl_runtime_init(const Sc64RaceMap *map)
{
    Sc64Z8Setup setup;
    const Sc64ZlMission *m = race_zl_mission_for(race_selected_mission);
    memset(&race_zl_sched, 0, sizeof(race_zl_sched));
    memset(&race_zl_ai, 0, sizeof(race_zl_ai));
    memset(race_zl_ai_minerals, 0, sizeof(race_zl_ai_minerals));
    memset(race_zl_ai_gas, 0, sizeof(race_zl_ai_gas));
    race_zl_map = 0;
    race_zl_mission = 0;
    race_zl_product_slot = -1;
    race_zl_camera_release = 0;
    race_zl_crystal_carrier = SC64_ZL_NO_CARRIER;
    if (!race_zl_active()) return 1;
    race_zl_setup(m, &setup);
    return sc64_zl_program_valid(m->program) &&
           sc64_zl_init(&race_zl_sched, m->program, &setup) && race_zl_bind(map);
}

static int race_zl_save_pack(Sc64ZlPersistentState *out)
{
    if (!race_zl_mission || !sc64_zl_ai_state_valid(&race_zl_ai) ||
        race_zl_camera_release || !race_zl_live_world_valid()) return 0;
    memset(out, 0, sizeof(*out));
    if (!sc64_zl_state_pack(out, &race_zl_sched, race_zl_mission->program)) return 0;
    for (unsigned t = 0; t < SC64_ZL_AI_TOWNS; ++t) {
        if (race_zl_ai_minerals[t] > 65535u || race_zl_ai_gas[t] > 65535u) return 0;
        out->ai_minerals[t] = (u16)race_zl_ai_minerals[t];
        out->ai_gas[t] = (u16)race_zl_ai_gas[t];
    }
    out->crystal_carrier = race_zl_crystal_carrier;
    memcpy(out->ai, &race_zl_ai, sizeof(race_zl_ai));
    return 1;
}

/* Validate a candidate without touching live state; `scratch` is a
 * transient engine buffer. On success with `apply`, install it. */
static int race_zl_save_unpack(const Sc64ZlPersistentState *in,
                               Sc64ZlSchedulerState *scratch, int apply)
{
    Sc64Z8Setup setup;
    Sc64ZlAiState ai;
    const Sc64ZlMission *m = race_zl_mission_for(race_selected_mission);
    if (!m) return 0;
    race_zl_setup(m, &setup);
    memcpy(&ai, in->ai, sizeof(ai));
    if (!sc64_zl_ai_state_valid(&ai) ||
        (in->crystal_carrier != SC64_ZL_NO_CARRIER && in->crystal_carrier >= MAX_UNITS))
        return 0;
    if (!sc64_zl_state_unpack(apply ? &race_zl_sched : 0, in, m->program,
                              &setup, scratch)) return 0;
    if (!apply) return 1;
    race_zl_ai = ai;
    for (unsigned t = 0; t < SC64_ZL_AI_TOWNS; ++t) {
        race_zl_ai_minerals[t] = in->ai_minerals[t];
        race_zl_ai_gas[t] = in->ai_gas[t];
    }
    race_zl_crystal_carrier = in->crystal_carrier;
    return 1;
}

static int race_zl_rebind_loaded_world(const Sc64RaceMap *map)
{
    race_zl_product_slot = -1;
    race_zl_camera_release = 0;
    if (!race_zl_active()) return 1;
    return race_zl_bind(map) && race_zl_live_world_valid();
}

/* Run one wallet-bounded engine action as if the live ENEMY bank were the
 * town's source wallet. */
static int race_zl_ai_queue(unsigned town, int producer, int type)
{
    int saved_minerals = race_minerals[ENEMY], saved_gas = race_gas[ENEMY], ok;
    race_minerals[ENEMY] = (int)race_zl_ai_minerals[town];
    race_gas[ENEMY] = (int)race_zl_ai_gas[town];
    ok = race_queue_unit(ENEMY, producer, type);
    race_zl_ai_minerals[town] = (u32)race_minerals[ENEMY];
    race_zl_ai_gas[town] = (u32)race_gas[ENEMY];
    race_minerals[ENEMY] = saved_minerals;
    race_gas[ENEMY] = saved_gas;
    return ok;
}

static void race_zl_ai_produce(unsigned town)
{
    unsigned used = 0;
    s16 slot = (s16)race_zl_mission->town_slot[town];
    for (int i = 0; i < MAX_UNITS; ++i) used += u[i].alive != 0;
    if (used + SC64_ZL_AI_RESERVE_UNITS >= MAX_UNITS) return;
    for (int i = 0; i < MAX_BUILDINGS; ++i) {
        int type = -1;
        if (!b[i].alive || b[i].owner != ENEMY || b[i].raw_owner != slot ||
            !building_complete(i) || b[i].queue_count) continue;
        if (b[i].type == B_GATEWAY)
            type = race_zl_ai.produce_index[town] & 1u ? U_DRAGOON : U_ZEALOT;
        else if (b[i].type == B_STARGATE) type = U_SCOUT;
        else if (b[i].type == B_ROBOTICS_FACILITY) type = U_REAVER;
        if (type >= 0 && race_zl_ai_queue(town, i, type))
            race_zl_ai.produce_index[town] = (u8)(race_zl_ai.produce_index[town] + 1u);
    }
}

static void race_zl_ai_wave(unsigned town)
{
    int tx = -1, ty = -1, sent = 0, guard = 0, want;
    s16 slot = (s16)race_zl_mission->town_slot[town];
    /* Aim at a human hive cluster, else any destructible human structure,
     * else the first human unit, so waves keep pressing a beaten human. */
    for (int i = 0; i < MAX_BUILDINGS && tx < 0; ++i) if (b[i].alive &&
        b[i].owner == SIDE && (b[i].raw_type == SC64_ZL_RAW_HATCHERY ||
        b[i].raw_type == SC64_ZL_RAW_LAIR || b[i].raw_type == SC64_ZL_RAW_HIVE)) {
        tx = b[i].x; ty = b[i].y;
    }
    for (int i = 0; i < MAX_BUILDINGS && tx < 0; ++i)
        if (b[i].alive && b[i].owner == SIDE && !invincible_buildings[i]) {
            tx = b[i].x; ty = b[i].y;
        }
    for (int i = 0; i < MAX_UNITS && tx < 0; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].type != U_CHRYSALIS) {
            tx = u[i].x; ty = u[i].y;
        }
    if (tx < 0) return;
    want = SC64_ZL_AI_WAVE_BASE + 2 * (int)race_zl_ai.wave[town];
    if (want > SC64_ZL_AI_WAVE_MAX) want = SC64_ZL_AI_WAVE_MAX;
    for (int i = 0; i < MAX_UNITS && sent < want; ++i) if (u[i].alive &&
        u[i].owner == ENEMY && u[i].raw_owner == slot && u[i].order == ORDER_IDLE &&
        !sc64_race_is_worker(u[i].type) && sc64_race_unit_damage(u[i].type) > 0) {
        if (guard < SC64_ZL_AI_HOME_GUARD) { ++guard; continue; }
        u[i].order = ORDER_ATTACK_MOVE; u[i].target = -1;
        u[i].tx = (s16)tx; u[i].ty = (s16)ty; u[i].path_x = u[i].path_y = 0;
        ++sent; ++race_campaign.enemy_units_sent;
    }
    if (race_zl_ai.wave[town] < 255u) ++race_zl_ai.wave[town];
}

static void race_zl_ai_tick(void)
{
    for (unsigned t = 0; t < SC64_ZL_AI_TOWNS; ++t) {
        if (!(race_zl_ai.started & (1u << t))) continue;
        if (race_zl_ai.produce_timer[t]) --race_zl_ai.produce_timer[t];
        if (!race_zl_ai.produce_timer[t]) {
            race_zl_ai.produce_timer[t] = SC64_ZL_AI_PRODUCE_UPDATES;
            race_zl_ai_produce(t);
        }
        if (race_zl_ai.wave_timer[t]) --race_zl_ai.wave_timer[t];
        if (!race_zl_ai.wave_timer[t]) {
            race_zl_ai.wave_timer[t] = SC64_ZL_AI_WAVE_UPDATES;
            race_zl_ai_wave(t);
        }
    }
}

/* The Khaydarin Crystal follows its carrier Drone, as Z2's powerup does. A
 * loose crystal is picked up by the nearest human Drone in range; a carrier
 * that dies or is removed drops it where it stood. */
static void race_zl_crystal_tick(void)
{
    int crystal = -1, best = -1, best_d = SC64_ZL_CRYSTAL_PICKUP_PX * SC64_ZL_CRYSTAL_PICKUP_PX + 1;
    for (int i = 0; i < MAX_UNITS && crystal < 0; ++i)
        if (u[i].alive && u[i].raw_type == SC64_ZL_RAW_CRYSTAL) crystal = i;
    if (crystal < 0) { race_zl_crystal_carrier = SC64_ZL_NO_CARRIER; return; }
    if (race_zl_crystal_carrier != SC64_ZL_NO_CARRIER) {
        int c = race_zl_crystal_carrier;
        if (!u[c].alive || u[c].owner != SIDE || u[c].raw_type != SC64_ZL_RAW_DRONE ||
            SC64_TRANSPORT_LOADED(&u[c])) {
            race_zl_crystal_carrier = SC64_ZL_NO_CARRIER;
        } else {
            u[crystal].x = u[c].x;
            u[crystal].y = u[c].y;
            return;
        }
    }
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == SIDE &&
        u[i].raw_type == SC64_ZL_RAW_DRONE && !SC64_TRANSPORT_LOADED(&u[i])) {
        int d = dist2(u[i].x, u[i].y, u[crystal].x, u[crystal].y);
        if (d < best_d) { best = i; best_d = d; }
    }
    if (best >= 0) {
        race_zl_crystal_carrier = (u8)best;
        u[crystal].x = u[best].x;
        u[crystal].y = u[best].y;
        u[crystal].selected = 0;
    }
}

/* The scheduler advances four native steps per five active engine updates;
 * AI and the crystal stay on the engine clock. A fault pauses the mission. */
static int race_zl_runtime_tick(void)
{
    int native_due;
    const Sc64ZlMission *m = race_zl_mission;
    if (!race_zl_active()) return 1;
    if (!race_zl_map || !m) return 0;
    native_due = sc64_z8_native_step((unsigned)frame);
    if (native_due && race_zl_camera_release) {
        race_zl_camera_release = 0;
        if (!sc64_z8_release_wait(&race_zl_sched.core, SC64_ZL_HUMAN_SLOT)) return 0;
    }
    race_zl_human_minerals = (u32)minerals;
    race_zl_human_gas = (u32)gas;
    if (native_due &&
        !sc64_zl_tick_prevalidated(&race_zl_sched, m->program, &race_zl_world_ops,
                                   &race_zl_view, 0)) return 0;
    if (race_zl_human_minerals > 65535u || race_zl_human_gas > 65535u) return 0;
    for (unsigned t = 0; t < SC64_ZL_AI_TOWNS; ++t)
        if (race_zl_ai_minerals[t] > 65535u || race_zl_ai_gas[t] > 65535u) return 0;
    minerals = race_minerals[SIDE] = (int)race_zl_human_minerals;
    gas = race_gas[SIDE] = (int)race_zl_human_gas;
    race_zl_ai_tick();
    race_zl_crystal_tick();
    return 1;
}

static int race_zl_outcome(void)
{ return race_zl_sched.core.outcome[SC64_ZL_HUMAN_SLOT]; }

#endif /* SC64_Z9_RUNTIME */
#endif /* SC64_ZERG_LATE_RUNTIME_ADAPTER_H */
