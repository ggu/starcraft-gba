#ifndef SC64_Z5_RUNTIME_ADAPTER_H
#define SC64_Z5_RUNTIME_ADAPTER_H

#if defined(SC64_Z5_RUNTIME)
#include "sc64_z5_state.h"
#include "sc64_z5_world_adapter.h"
#include "sc64_z5_doodads_generated.h"

_Static_assert(SC64_Z5_DOODAD_COUNT == 71u &&
               SC64_Z5_UNIT_BACKED_DOODAD_COUNT == SC64_Z5_UNIT_BACKED_DOODADS,
               "Z5 authenticated prop table shape mismatch");

static int race_z5_active(void)
{ return !race_mode && race_selected_mission == SC64_Z5_GLOBAL_ID; }

static Sc64Z5DoodadState race_z5_doodads[SC64_Z5_DOODAD_CAPACITY];
static Sc64Z5WorldContext race_z5_world;

static unsigned race_z5_prop_cache_field(unsigned row, int enabled)
{
    if (row >= SC64_Z5_DOODAD_CAPACITY) return 0;
    return enabled ? race_z5_doodads[row].enabled :
                     race_z5_doodads[row].building_slot;
}

static int race_z5_source_owner(unsigned source)
{
    if (source >= 12 || !race_z5_world.source_slot_active[source]) return -1;
    return race_z5_world.live_owner_for_source[source];
}

static int race_z5_is_door_type(int type)
{
    return type == B_Z5_UPPER_DOOR || type == B_Z5_RIGHT_UPPER_DOOR ||
           type == B_Z5_PIT_DOOR || type == B_Z5_RIGHT_PIT_DOOR;
}

static int race_z5_building_collidable(int slot)
{
    if (slot < 0 || slot >= MAX_BUILDINGS || !b[slot].alive) return 0;
    if (race_z5_active() && race_z5_is_door_type(b[slot].type)) {
        for (unsigned i = 0; i < SC64_Z5_DOODAD_CAPACITY; ++i)
            if (race_z5_doodads[i].building_slot == slot)
                return race_z5_doodads[i].enabled != 0;
        return 1; /* Unknown bindings stay closed. */
    }
    return 1;
}

static int race_z5_world_paused(void)
{
    return race_z5_active() && race_z5.runtime.source_paused &&
           !race_z5.pause_acknowledged;
}

#ifdef SC64_HAS_SPRITES
static int race_z5_draw_prop(Building *q, int x, int y)
{
    int door, trap, mirror;
    const Sc64SpriteFrame *sprite;
    if (!race_z5_active() || !q || !q->alive || q->raw_type < 0) return 0;
    door = race_z5_is_door_type(q->type);
    trap = q->type == B_Z5_FLOOR_MISSILE_TRAP || q->type == B_Z5_FLOOR_GUN_TRAP ||
           q->type == B_Z5_WALL_FLAME_TRAP || q->type == B_Z5_RIGHT_WALL_FLAME_TRAP;
    if (!door && !trap) return 0;
    unsigned row;
    for (row = 0; row < SC64_Z5_DOODAD_CAPACITY; ++row)
        if (race_z5_doodads[row].building_slot == (unsigned)(q - b)) break;
    if (row == SC64_Z5_DOODAD_CAPACITY ||
        race_z5_doodads[row].raw_type != (unsigned)q->raw_type) return 0;
    if (trap && !race_z5_doodads[row].enabled) return 1;
    sprite = sc64_race_building_sprite((unsigned short)q->raw_type,
                                       (unsigned short)(door &&
                                           !race_z5_doodads[row].enabled ? 9 : 0));
    if (!sprite || !sprite->pixels || !sprite->w || !sprite->h) return 1;
    mirror = q->raw_type == 206 || q->raw_type == 208;
    if (!mirror) {
        draw_mission_sprite(sprite, x, y);
        return 1;
    }
    {
        int left = x - sprite->canvas_w / 2 +
                   (int)sprite->canvas_w - (sprite->offset_x + sprite->w);
        int top = y - sprite->canvas_h / 2 + sprite->offset_y;
        for (unsigned sy = 0; sy < sprite->h; ++sy)
            for (unsigned sx = 0; sx < sprite->w; ++sx) {
                u8 color = sprite->pixels[sy * sprite->w + sx];
                if (color) gfx_pixel(left + (int)(sprite->w - 1 - sx),
                                     top + (int)sy, color);
            }
    }
    return 1;
}
#else
static int race_z5_draw_prop(Building *q, int x, int y)
{ (void)q; (void)x; (void)y; return 0; }
#endif

static int race_z5_preflight(const Sc64RaceMap *map)
{
    unsigned units = 0, buildings = 0, minerals_count = 0, geysers_count = 0;
    unsigned doodads = 0;
    if (!map || map->width != SC64_Z5_MAP_COLS ||
        map->height != SC64_Z5_MAP_ROWS || map->human_slot >= 8 ||
        map->unit_count > MAX_UNITS + MAX_BUILDINGS + MAX_MINERALS +
                          MAX_RACE_GEYSERS + 12) return 0;
    for (unsigned i = 0; i < map->unit_count; ++i) {
        const Sc64ImportedUnit *src = &map->units[i];
        int raw = src->source_type;
        if (src->x_px >= map->width * 16u || src->y_px >= map->height * 16u)
            return 0;
        if (raw == 214) continue; /* Authenticated start-location marker. */
        if (raw >= 176 && raw <= 178) ++minerals_count;
        else if (raw == 188) ++geysers_count;
        else if (sc64_race_unit_from_raw(raw) >= 0) ++units;
        else if (sc64_race_building_from_raw(raw) >= 0) ++buildings;
        else return 0;
    }
    for (unsigned i = 0; i < SC64_Z5_DOODAD_COUNT; ++i) {
        const Sc64Z5SourceDoodad *src = &sc64_z5_source_doodads[i];
        if (!src->unit_backed) continue;
        if (src->owner >= 8 || src->x >= map->width * 32u ||
            src->y >= map->height * 32u ||
            sc64_race_building_from_raw(src->type) < 0) return 0;
        ++doodads;
    }
    return doodads == SC64_Z5_DOODAD_CAPACITY &&
           units <= MAX_UNITS && buildings + doodads <= MAX_BUILDINGS &&
           minerals_count <= MAX_MINERALS && geysers_count <= MAX_RACE_GEYSERS;
}

static int race_z5_rebind_doodads(const Sc64RaceMap *map, int spawn)
{
    unsigned row = 0;
    if (!race_z5_active()) return 1;
    if (!race_z5_preflight(map) || SC64_Z5_DOODAD_COUNT != 71u) return 0;
    if (spawn) {
        sc64_z5_runtime_init(&race_z5.runtime);
        memset(race_z5.doodad_enabled, 0, sizeof(race_z5.doodad_enabled));
        race_z5.pause_acknowledged = 0;
    } else if (!sc64_z5_persistent_state_valid(&race_z5)) return 0;
    memset(&race_z5_world, 0, sizeof(race_z5_world));
    for (unsigned slot = 0; slot < 12; ++slot) {
        int owner;
        if (slot < 8 && map->owner_races[slot] < 3) {
            owner = race_campaign_owner(map, (int)slot);
            if (slot == map->human_slot) owner = SIDE;
            race_z5_world.source_slot_active[slot] = 1;
            race_z5_world.live_owner_for_source[slot] = (s8)owner;
        } else {
            race_z5_world.live_owner_for_source[slot] = -1;
        }
    }
    if (map->human_slot >= 12) return 0;
    race_z5_world.source_current_player = map->human_slot;
    race_z5_world.doodads = race_z5_doodads;
    race_z5_world.doodad_count = 0;
    race_z5_world.pause_acknowledged = &race_z5.pause_acknowledged;
    for (unsigned i = 0; i < SC64_Z5_DOODAD_COUNT; ++i) {
        const Sc64Z5SourceDoodad *src = &sc64_z5_source_doodads[i];
        Sc64Z5DoodadState *dst;
        int type, owner, slot;
        if (!src->unit_backed) continue;
        if (row >= SC64_Z5_DOODAD_CAPACITY || src->owner >= 12 ||
            src->x >= SC64_Z5_MAP_COLS * 32u ||
            src->y >= SC64_Z5_MAP_ROWS * 32u) return 0;
        type = sc64_race_building_from_raw(src->type);
        owner = race_z5_source_owner(src->owner);
        if (type < 0 || owner < 0) return 0;
        if (spawn) {
            slot = spawn_build(owner, type, src->x / 2, src->y / 2, 0);
            if (slot < 0) return 0;
            b[slot].raw_type = src->type;
            b[slot].raw_owner = src->owner;
            b[slot].flags = src->flags;
        } else {
            slot = -1;
            for (int j = 0; j < MAX_BUILDINGS; ++j)
                if (b[j].alive && b[j].raw_type == src->type &&
                    b[j].raw_owner == src->owner && b[j].x == src->x / 2 &&
                    b[j].y == src->y / 2) {
                    if (slot >= 0) return 0;
                    slot = j;
                }
            if (slot < 0 || b[slot].type != type || b[slot].owner != owner ||
                b[slot].flags != src->flags) return 0;
        }
        dst = &race_z5_doodads[row];
        dst->raw_type = src->type;
        dst->x = src->x; dst->y = src->y; dst->raw_flags = src->flags;
        dst->source_owner = src->owner;
        dst->enabled = spawn ? (u8)(src->initial_enabled != 0) :
            (u8)((race_z5.doodad_enabled[row >> 3] >> (row & 7u)) & 1u);
        dst->building_slot = (u8)slot;
        if (spawn && dst->enabled)
            race_z5.doodad_enabled[row >> 3] |= (u8)(1u << (row & 7u));
        ++row;
    }
    if (row != SC64_Z5_DOODAD_CAPACITY) return 0;
    race_z5_world.doodad_count = (u8)row;
    return sc64_z5_persistent_state_valid(&race_z5);
}

static int race_z5_runtime_init(const Sc64RaceMap *map)
{ return race_z5_preflight(map) && race_z5_rebind_doodads(map, 1); }

static int race_z5_rebind_loaded_world(const Sc64RaceMap *map)
{ return race_z5_rebind_doodads(map, 0); }

static int race_z5_restore_doodads(void)
{
    unsigned row = 0;
    if (!race_z5_active()) return 1;
    if (race_z5_world.doodad_count != SC64_Z5_DOODAD_CAPACITY) return 0;
    for (unsigned i = 0; i < SC64_Z5_DOODAD_COUNT; ++i) {
        const Sc64Z5SourceDoodad *src = &sc64_z5_source_doodads[i];
        Sc64Z5DoodadState *dst;
        if (!src->unit_backed) continue;
        if (row >= SC64_Z5_DOODAD_CAPACITY) return 0;
        dst = &race_z5_doodads[row];
        if (dst->raw_type != src->type || dst->source_owner != src->owner ||
            dst->x != src->x || dst->y != src->y ||
            dst->raw_flags != src->flags || dst->building_slot >= MAX_BUILDINGS)
            return 0;
        dst->enabled = (u8)((race_z5.doodad_enabled[row >> 3] >> (row & 7u)) & 1u);
        if (!b[dst->building_slot].alive ||
            b[dst->building_slot].raw_type != src->type ||
            b[dst->building_slot].raw_owner != src->owner ||
            b[dst->building_slot].x != src->x / 2 ||
            b[dst->building_slot].y != src->y / 2) return 0;
        ++row;
    }
    return row == SC64_Z5_DOODAD_CAPACITY;
}

static int race_z5_campaign_tick(void)
{
    Sc64Z5WorldOps ops;
    int result;
    if (!race_z5_active()) return 1;
    if (!sc64_z5_persistent_state_valid(&race_z5) || !race_z5_restore_doodads())
        return 0;
    race_z5_world.victory_requested = 0;
    race_z5_world.defeat_requested = 0;
    ops = sc64_z5_world_ops(&race_z5_world);
    result = sc64_z5_adapter_tick(&race_z5.runtime, &ops, 0);
    if (result < 0 || !sc64_z5_persistent_state_valid(&race_z5)) return 0;
    for (unsigned i = 0; i < SC64_Z5_DOODAD_CAPACITY; ++i) {
        if (race_z5_doodads[i].enabled)
            race_z5.doodad_enabled[i >> 3] |= (u8)(1u << (i & 7u));
        else
            race_z5.doodad_enabled[i >> 3] &= (u8)~(1u << (i & 7u));
    }
    if (race_z5.runtime.model.outcome == SC64_Z5_WON) {
        won = 1; lost = 0;
        race_campaign.phase = SC64_RACE_PHASE_COMPLETE;
        race_campaign.objective_flags |= SC64_RACE_OBJECTIVE_PRIMARY;
        race_completed |= 1 << race_selected_mission;
    } else if (race_z5.runtime.model.outcome == SC64_Z5_LOST) {
        won = 0; lost = 1;
        race_campaign.phase = SC64_RACE_PHASE_FAILED;
    }
    return 1;
}
#endif

#endif
