#ifndef SC64_P7_RUNTIME_ADAPTER_H
#define SC64_P7_RUNTIME_ADAPTER_H

#if defined(SC64_P7_RUNTIME)

#if !defined(SC64_P7_DATA_ENABLED) || !defined(SC64_ZP_FULL_STORAGE) || \
    !defined(SC64_UNIFIED) || !defined(SC64_CAMPAIGN_RACE) || \
    SC64_CAMPAIGN_RACE != 2
#error "P7 runtime requires authenticated P7 data and full Protoss campaign storage"
#endif

#include "sc64_p7_world_adapter.h"

static Sc64P7PersistentState race_p7;
static Sc64P7WorldContext race_p7_world;
static Sc64P7WorldOps race_p7_ops;
static const Sc64RaceMap *race_p7_map;

static int race_p7_active(void)
{ return !race_mode && race_selected_mission == SC64_P7_GLOBAL_ID; }

/* Resolve from the canonical descriptor before transient bindings exist. The
 * owner table is not rewritten: two computer source forces may share ENEMY. */
static int race_p7_source_owner(unsigned slot)
{
    const Sc64RaceMap *map;
    if (SC64_P7_GLOBAL_ID >= SC64_RACE_MAP_COUNT) return -1;
    map=&sc64_race_maps[SC64_P7_GLOBAL_ID];
    if (slot>=8u || map->human_slot!=7u || !map->owner_modes) return -1;
    if (!sc64_p7_active_slots[slot]) return -1;
    return slot==map->human_slot ? SIDE : map->owner_modes[slot];
}

static int race_p7_map_shape_valid(const Sc64RaceMap *map)
{
    if (!map || SC64_P7_GLOBAL_ID>=SC64_RACE_MAP_COUNT ||
        map!=&sc64_race_maps[SC64_P7_GLOBAL_ID] ||
        map->width!=SC64_P7_MAP_COLS || map->height!=SC64_P7_MAP_ROWS ||
        map->human_slot!=7u || map->unit_count!=357u ||
        !map->owner_modes || !map->owner_races || !map->units ||
        !map->blocked || !map->tile_lookup || !map->tile_pixels ||
        !map->mission_id || map->mission_id[0]!='P' ||
        map->mission_id[1]!='7' || map->mission_id[2]!='\0' ||
        !sc64_p7_runtime_metadata_valid()) return 0;
    for (unsigned i=0;i<8;++i)
        if (map->owner_modes[i]!=sc64_p7_owner_modes_expected[i] ||
            map->owner_races[i]!=sc64_p7_owner_races_expected[i]) return 0;
    for (unsigned i=0;i<8;++i)
        if (sc64_p7_source_forces[i]!=sc64_p7_force_for_slot[i]) return 0;
    return 1;
}

static int race_p7_world_ai_callback(void *context,
                                     const Sc64P7SourceAction *action);

/* A true source roster census, including fail-closed actor lookup. */
static int race_p7_source_row_counts(const Sc64RaceMap *map)
{
    unsigned units=0,buildings=0,minerals_count=0,geysers_count=0;
    if (!race_p7_map_shape_valid(map)) return 0;
    for (unsigned i=0;i<map->unit_count;++i) {
        const Sc64ImportedUnit *src=&map->units[i];
        unsigned raw=src->source_type;
        if (raw>=176u && raw<=178u) {
            if (src->owner!=11u) return 0;
            ++minerals_count; continue;
        }
        if (raw==188u) {
            if (src->owner!=11u) return 0;
            ++geysers_count; continue;
        }
        if (raw==214u || src->owner>=8u || !sc64_p7_active_slots[src->owner])
            return 0;
        if (sc64_race_unit_from_raw((int)raw)>=0) ++units;
        else if (sc64_race_building_from_raw((int)raw)>=0) ++buildings;
        else return 0;
    }
    return units==SC64_P7_MAP_UNITS && buildings==SC64_P7_MAP_BUILDINGS &&
           minerals_count==SC64_P7_MAP_MINERALS &&
           geysers_count==SC64_P7_MAP_GEYSERS &&
           units+SC64_P7_CREATION_COUNT<=MAX_UNITS &&
           buildings<=MAX_BUILDINGS && minerals_count<=MAX_MINERALS &&
           geysers_count<=MAX_RACE_GEYSERS;
}

static int race_p7_starting_world_matches(const Sc64RaceMap *map)
{
    u8 seen_units[MAX_UNITS]={0},seen_buildings[MAX_BUILDINGS]={0};
    u8 seen_minerals[MAX_MINERALS]={0},seen_geysers[MAX_RACE_GEYSERS]={0};
    unsigned units=0,buildings=0,mineral_count=0,geysers_count=0;
    if (!race_p7_source_row_counts(map) || geyser_count<0 ||
        geyser_count>MAX_RACE_GEYSERS) return 0;
    for (unsigned row=0;row<map->unit_count;++row) {
        const Sc64ImportedUnit *src=&map->units[row];
        unsigned raw=src->source_type;
        int found=-1;
        if (raw>=176u && raw<=178u) {
            for (int i=0;i<MAX_MINERALS;++i) if (!seen_minerals[i] &&
                m[i].amount>0 && m[i].x==src->x_px && m[i].y==src->y_px &&
                m[i].style==(int)raw &&
                m[i].amount==(src->resources?src->resources:1u)) { found=i; break; }
            if (found<0) return 0;
            seen_minerals[found]=1; ++mineral_count; continue;
        }
        if (raw==188u) {
            for (int i=0;i<geyser_count;++i) if (!seen_geysers[i] &&
                geysers[i].x==src->x_px && geysers[i].y==src->y_px) { found=i; break; }
            if (found<0) return 0;
            seen_geysers[found]=1; ++geysers_count; continue;
        }
        int owner=race_p7_source_owner(src->owner);
        int type=sc64_race_unit_from_raw((int)raw);
        if (owner<0) return 0;
        if (type>=0) {
            for (int i=0;i<MAX_UNITS;++i) if (!seen_units[i] && u[i].alive &&
                u[i].raw_type==(int)raw && u[i].raw_owner==(int)src->owner &&
                u[i].owner==owner && u[i].type==type &&
                u[i].x==src->x_px && u[i].y==src->y_px) { found=i; break; }
            if (found<0) return 0;
            seen_units[found]=1; ++units;
        } else {
            type=sc64_race_building_from_raw((int)raw);
            if (type<0) return 0;
            for (int i=0;i<MAX_BUILDINGS;++i) if (!seen_buildings[i] && b[i].alive &&
                b[i].raw_type==(int)raw && b[i].raw_owner==(int)src->owner &&
                b[i].owner==owner && b[i].type==type &&
                b[i].x==src->x_px && b[i].y==src->y_px) { found=i; break; }
            if (found<0) return 0;
            seen_buildings[found]=1; ++buildings;
        }
    }
    for (int i=0;i<MAX_UNITS;++i) if (u[i].alive && !seen_units[i]) return 0;
    for (int i=0;i<MAX_BUILDINGS;++i) if (b[i].alive && !seen_buildings[i]) return 0;
    for (int i=0;i<MAX_MINERALS;++i)
        if ((m[i].amount>0)!=(seen_minerals[i]!=0)) return 0;
    if ((unsigned)geyser_count!=SC64_P7_MAP_GEYSERS ||
        units!=SC64_P7_MAP_UNITS || buildings!=SC64_P7_MAP_BUILDINGS ||
        mineral_count!=SC64_P7_MAP_MINERALS || geysers_count!=SC64_P7_MAP_GEYSERS)
        return 0;
    for (int i=0;i<geyser_count;++i) if (!seen_geysers[i]) return 0;
    return 1;
}

static int race_p7_bind_world(const Sc64RaceMap *map)
{
    if (!race_p7_source_row_counts(map)) return 0;
    memset(&race_p7_world,0,sizeof(race_p7_world));
    race_p7_world.map=map; race_p7_world.persistent=&race_p7;
    race_p7_world.source_current_player=map->human_slot;
    for (unsigned slot=0;slot<8;++slot) {
        int owner=race_p7_source_owner(slot);
        if (sc64_p7_active_slots[slot] && (owner<SIDE || owner>4)) return 0;
        race_p7_world.source_slot_active[slot]=sc64_p7_active_slots[slot];
        race_p7_world.live_owner_for_source[slot]=(s8)owner;
        race_p7_world.source_force_for_slot[slot]=sc64_p7_force_for_slot[slot];
    }
    for (unsigned slot=8;slot<12;++slot) race_p7_world.live_owner_for_source[slot]=-1;
    race_p7_ops.context=&race_p7_world;
    race_p7_ops.condition=sc64_p7_world_condition;
    race_p7_ops.action=sc64_p7_world_action;
    race_p7_ops.opaque_ai=race_p7_world_ai_callback;
    return 1;
}

static int race_p7_runtime_state_valid(void)
{ return sc64_p7_persistent_state_valid(&race_p7); }

static int race_p7_world_ai_callback(void *context,
                                     const Sc64P7SourceAction *action)
{ return sc64_p7_world_run_ai((Sc64P7WorldContext *)context,action); }

static int race_p7_runtime_init(const Sc64RaceMap *map)
{
    memset(&race_p7,0,sizeof(race_p7));
    sc64_p7_runtime_init(&race_p7.runtime);
    race_p7_map=map;
    if (!race_p7_active()) return 1;
    if (!race_p7_bind_world(map)) {
        race_p7.runtime.fault=SC64_P7_RUNTIME_FAULT_MAP;
        return 0;
    }
    if (!race_p7_starting_world_matches(map)) {
        race_p7.runtime.fault=SC64_P7_RUNTIME_FAULT_ACTOR;
        return 0;
    }
    return race_p7_runtime_state_valid();
}

static int race_p7_rebind_loaded_world(const Sc64RaceMap *map)
{
    if (!race_p7_active()) return 1;
    if (!race_p7_runtime_state_valid() || !race_p7_bind_world(map)) return 0;
#ifdef SC64_P7_BATTERY_ENABLED
    race_p7_map=map;
#endif
    return 1;
}

/* The engine owns when this runs. The P7 model has exact one-shot trigger
 * timing; text/audio presentation assets remain a separate host concern. */
static int race_p7_runtime_tick(u32 elapsed_frames)
{
    if (!race_p7_active()) return 1;
    if (!race_p7_runtime_state_valid() || !race_p7_map ||
        !race_p7_bind_world(race_p7_map)) return 0;
    race_p7_world.elapsed_frames=elapsed_frames;
    race_p7_world.victory_requested=0;
    race_p7_world.defeat_requested=0;
    race_p7_world.objective_refresh_requested=0;
    race_p7_ops.elapsed_frames=elapsed_frames;
    int result=sc64_p7_adapter_tick(&race_p7.runtime,&race_p7_ops,0);
    return result>=0 && race_p7_runtime_state_valid();
}

#endif /* SC64_P7_RUNTIME */
#endif
