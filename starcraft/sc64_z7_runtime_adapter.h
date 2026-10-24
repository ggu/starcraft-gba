#ifndef SC64_Z7_RUNTIME_ADAPTER_H
#define SC64_Z7_RUNTIME_ADAPTER_H

#if defined(SC64_Z7_RUNTIME)

#if !defined(SC64_Z7_DATA_ENABLED) || !defined(SC64_Z6_ACTORS) || \
    !defined(SC64_ZP_CAMPAIGN_ACTORS) || !defined(SC64_ZP_FULL_STORAGE) || \
    !defined(SC64_ZP_CAMPAIGN_STORAGE) || !defined(SC64_UNIFIED) || \
    !defined(SC64_CAMPAIGN_RACE) || SC64_CAMPAIGN_RACE != 1
#error "Z7 runtime requires authenticated Z7 data and full Zerg campaign actor/storage gates"
#endif

#include "sc64_z7_state.h"
#include "sc64_z7_world_adapter.h"

enum {
    SC64_Z7_RUNTIME_FAULT_MAP = 0x50,
    SC64_Z7_RUNTIME_FAULT_BINDING = 0x51,
    SC64_Z7_RUNTIME_FAULT_ACTOR = 0x52,
    SC64_Z7_RUNTIME_FAULT_AI = 0x53
};

/* Persisted state is declared beside the engine's other mission states. */
static Sc64Z7WorldContext race_z7_world;
static Sc64Z7WorldOps race_z7_ops;
static const Sc64RaceMap *race_z7_map;

static int race_z7_active(void)
{ return !race_mode && race_selected_mission==SC64_Z7_GLOBAL_ID; }

/* The staged map records slot 3 as the human Force 0 member and slot 4 as
 * the hostile Zerg Force 1 member. Resolve from the canonical descriptor even
 * before the runtime context has been bound by map import. */
static int race_z7_source_owner(unsigned slot)
{
    const Sc64RaceMap *map=race_z7_map;
    if (!map && SC64_Z7_GLOBAL_ID<SC64_RACE_MAP_COUNT)
        map=&sc64_race_maps[SC64_Z7_GLOBAL_ID];
    if (!map || slot>=8u || !map->owner_modes || map->human_slot>=8u) return -1;
    return slot==map->human_slot ? SIDE : map->owner_modes[slot];
}

static int race_z7_product_source_owner(int owner)
{
    /* Authenticated Z7 has exactly one live human and one hostile source
       owner; the strict map-shape preflight enforces these bindings. */
    return owner==SIDE ? 3 : owner==ENEMY ? 4 : -1;
}

static int race_z7_map_shape_valid(const Sc64RaceMap *map);

/* Native StarCraft turns a destroyed Extractor into a neutral Vespene
 * Geyser at the same map coordinate.  Z7 keeps authenticated raw 149 rows as
 * buildings until this hook runs; only those five immutable locations can
 * become additional nodes.  This restores availability only: the shared
 * engine's gas reservoir remains non-depleting, and no resource is refunded. */
static int race_z7_source_gas_node(const Sc64RaceMap *map,int x,int y)
{
    if (!map || !race_z7_map_shape_valid(map)) return 0;
    for (unsigned row=0;row<map->unit_count;++row) {
        const Sc64ImportedUnit *src=&map->units[row];
        if (src->source_type==149u && src->owner==4u &&
            src->x_px==(unsigned)x && src->y_px==(unsigned)y) {
            return 1;
        }
    }
    return 0;
}

static void race_z7_note_building_death(const Building *q)
{
    const Sc64RaceMap *map=race_z7_map;
    if (!race_z7_active() || !q || q->type!=B_EXTRACTOR) return;
    if (!map && SC64_Z7_GLOBAL_ID<SC64_RACE_MAP_COUNT)
        map=&sc64_race_maps[SC64_Z7_GLOBAL_ID];
    if (!race_z7_source_gas_node(map,q->x,q->y)) return;
    if (geyser_count<0 || geyser_count>MAX_RACE_GEYSERS) {
        race_z7.runtime.fault=SC64_Z7_RUNTIME_FAULT_ACTOR;
        return;
    }
    for (int i=0;i<geyser_count;++i)
        if (geysers[i].x==q->x && geysers[i].y==q->y) return;
    if (geyser_count>=MAX_RACE_GEYSERS) {
        race_z7.runtime.fault=SC64_Z7_RUNTIME_FAULT_ACTOR;
        return;
    }
    geysers[geyser_count].x=q->x;
    geysers[geyser_count].y=q->y;
    ++geyser_count;
}

static int race_z7_map_shape_valid(const Sc64RaceMap *map)
{
    static const u8 modes[8]={2,2,2,0,1,2,2,2};
    static const u8 races[8]={R_PROTOSS,R_PROTOSS,R_PROTOSS,R_ZERG,R_ZERG,
                              R_PROTOSS,R_PROTOSS,R_PROTOSS};
    if (!map || map!=&sc64_race_maps[SC64_Z7_GLOBAL_ID] ||
        map->width!=SC64_Z7_MAP_COLS || map->height!=SC64_Z7_MAP_ROWS ||
        map->human_slot!=3u || !map->owner_modes || !map->owner_races ||
        !map->units || !map->blocked || !map->tile_lookup || !map->tile_pixels ||
        !map->mission_id || map->unit_count!=231u ||
        !sc64_z7_runtime_metadata_valid()) return 0;
    for (unsigned i=0;i<8;++i)
        if (map->owner_modes[i]!=modes[i] || map->owner_races[i]!=races[i] ||
            sc64_z7_source_forces[i]!=(i==3?0u:i==4?1u:3u)) return 0;
    return 1;
}

static int race_z7_source_row_counts(const Sc64RaceMap *map)
{
    unsigned units=0,buildings=0,minerals_count=0,geysers_count=0,
             extractors=0,markers=0;
    if (!race_z7_map_shape_valid(map)) return 0;
    for (unsigned i=0;i<map->unit_count;++i) {
        const Sc64ImportedUnit *src=&map->units[i]; unsigned raw=src->source_type;
        if (src->owner>=8u && raw!=188u && !(raw>=176u && raw<=178u) && raw!=214u) return 0;
        if (raw==214u) { ++markers; continue; }
        if (raw>=176u && raw<=178u) { ++minerals_count; continue; }
        if (raw==188u) { ++geysers_count; continue; }
        if (raw==149u) {
            if (src->owner!=4u) return 0;
            ++extractors;
        }
        if (src->owner>=8u) return 0;
        if (sc64_race_unit_from_raw((int)raw)>=0) ++units;
        else if (sc64_race_building_from_raw((int)raw)>=0) ++buildings;
        else return 0;
    }
    return units==113u && buildings==57u && minerals_count==60u &&
           geysers_count==1u && extractors==5u && markers==0u && units<=MAX_UNITS &&
           buildings<=MAX_BUILDINGS && minerals_count<=MAX_MINERALS &&
           geysers_count<=MAX_RACE_GEYSERS && SC64_Z7_SOURCE_ACTION_COUNT==15u;
}

static int race_z7_owner_for_map_row(const Sc64RaceMap *map,unsigned slot)
{ return slot==map->human_slot ? SIDE : map->owner_modes[slot]; }

/* A one-to-one census prevents a map import from silently aliasing, dropping,
 * or duplicating any authenticated source actor or resource placement. */
static int race_z7_starting_world_matches(const Sc64RaceMap *map)
{
    u8 seen_units[MAX_UNITS]={0},seen_buildings[MAX_BUILDINGS]={0};
    u8 seen_minerals[MAX_MINERALS]={0},seen_geysers[MAX_RACE_GEYSERS]={0};
    unsigned units=0,buildings=0,mineral_count=0,geyser_rows=0;
    if (!race_z7_source_row_counts(map) || geyser_count<0 || geyser_count>MAX_RACE_GEYSERS) return 0;
    for (unsigned row=0;row<map->unit_count;++row) {
        const Sc64ImportedUnit *src=&map->units[row]; unsigned raw=src->source_type;
        if (raw==214u) continue;
        if (raw>=176u && raw<=178u) {
            int found=-1;
            for (int i=0;i<MAX_MINERALS;++i) if (!seen_minerals[i] &&
                m[i].x==src->x_px && m[i].y==src->y_px &&
                m[i].amount==(src->resources?src->resources:1u) && m[i].style==(int)raw) {
                found=i; break;
            }
            if (found<0) return 0;
            seen_minerals[found]=1; ++mineral_count; continue;
        }
        if (raw==188u) {
            int found=-1;
            for (int i=0;i<geyser_count;++i) if (!seen_geysers[i] &&
                geysers[i].x==src->x_px && geysers[i].y==src->y_px) { found=i; break; }
            if (found<0) return 0;
            seen_geysers[found]=1; ++geyser_rows; continue;
        }
        {
            int owner=race_z7_owner_for_map_row(map,src->owner);
            int type=sc64_race_unit_from_raw((int)raw);
            if (type>=0) {
                int found=-1;
                for (int i=0;i<MAX_UNITS;++i) if (!seen_units[i] && u[i].alive &&
                    u[i].raw_type==(int)raw && u[i].raw_owner==src->owner &&
                    u[i].owner==owner && u[i].type==type && u[i].x==src->x_px && u[i].y==src->y_px) {
                    found=i; break;
                }
                if (found<0) return 0;
                seen_units[found]=1; ++units;
            } else {
                type=sc64_race_building_from_raw((int)raw);
                if (type<0) return 0;
                int found=-1;
                for (int i=0;i<MAX_BUILDINGS;++i) if (!seen_buildings[i] && b[i].alive &&
                    b[i].raw_type==(int)raw && b[i].raw_owner==src->owner &&
                    b[i].owner==owner && b[i].type==type && b[i].x==src->x_px && b[i].y==src->y_px) {
                    found=i; break;
                }
                if (found<0) return 0;
                seen_buildings[found]=1; ++buildings;
            }
        }
    }
    for (int i=0;i<MAX_UNITS;++i) if (u[i].alive && !seen_units[i]) return 0;
    for (int i=0;i<MAX_BUILDINGS;++i) if (b[i].alive && !seen_buildings[i]) return 0;
    for (int i=0;i<MAX_MINERALS;++i) if (m[i].amount>0 && !seen_minerals[i]) return 0;
    return units==113u && buildings==57u && mineral_count==60u &&
           geyser_rows==1u && geyser_count==1;
}

static int race_z7_bind_world(const Sc64RaceMap *map)
{
    if (!race_z7_source_row_counts(map)) return 0;
    memset(&race_z7_world,0,sizeof(race_z7_world));
    race_z7_world.source_current_player=map->human_slot;
    for (unsigned slot=0;slot<8;++slot) {
        int owner=race_z7_owner_for_map_row(map,slot);
        if (owner<SIDE || owner>4 || sc64_z7_source_forces[slot]>3u) return 0;
        race_z7_world.source_slot_active[slot]=1;
        race_z7_world.live_owner_for_source[slot]=(s8)owner;
        race_z7_world.source_force_for_slot[slot]=sc64_z7_source_forces[slot];
    }
    for (unsigned slot=8;slot<12;++slot) race_z7_world.live_owner_for_source[slot]=-1;
    race_z7_ops.context=&race_z7_world;
    race_z7_ops.condition=sc64_z7_world_condition;
    race_z7_ops.action=(int (*)(void *,const Sc64Z7SourceAction *))0;
    return 1;
}

static int race_z7_action_callback(void *context,const Sc64Z7SourceAction *a)
{ return sc64_z7_world_action((Sc64Z7WorldContext *)context,&race_z7.runtime,a); }

static int race_z7_world_owner_slot(const Sc64Z7WorldContext *ctx,int owner,unsigned *slot)
{
    unsigned found=12;
    if (!ctx || !slot) return 0;
    for (unsigned i=0;i<8;++i) if (ctx->source_slot_active[i] &&
        ctx->live_owner_for_source[i]==owner) {
        if (found!=12) return 0;
        found=i;
    }
    if (found>=12) return 0;
    *slot=found; return 1;
}

static unsigned race_z7_ai_committed_units(void)
{
    unsigned used=0;
    for (int i=0;i<MAX_UNITS;++i) used+=u[i].alive!=0;
    for (int i=0;i<MAX_BUILDINGS;++i) if (b[i].alive)
        for (int j=0;j<b[i].queue_count;++j)
            used+=b[i].queue[(b[i].queue_head+j)%5]==U_ZERGLING?2u:1u;
    return used;
}

/* Replacement for three opaque Zr7* scripts. It only issues ordinary
 * attack-move orders to existing slot-4 units and normally-paid Zergling
 * queue orders from imported slot-4 town halls. Its cadence and wave size are
 * authored engine policy, not original AI-bytecode equivalence. */
static int race_z7_authored_ai_pulse(void)
{
    int tx=cx,ty=cy,sent=0,producer=-1;
    unsigned slot;
    if (!race_z7_map || !race_z7_active() ||
        race_z7_map->owner_races[4]!=R_ZERG ||
        !race_z7_world_owner_slot(&race_z7_world,ENEMY,&slot) || slot!=4u ||
        !race_z7.runtime.ai_script_mask) return 0;
    for (int i=0;i<MAX_UNITS;++i) if (u[i].alive && u[i].owner==SIDE) {
        tx=u[i].x; ty=u[i].y; break;
    }
    for (int i=0;i<MAX_UNITS && sent<SC64_Z7_AI_WAVE_SIZE;++i) if (u[i].alive &&
        u[i].owner==ENEMY && u[i].raw_owner==4 && u[i].order==ORDER_IDLE &&
        !sc64_race_is_worker(u[i].type) && sc64_race_unit_damage(u[i].type)>0) {
        u[i].order=ORDER_ATTACK_MOVE; u[i].target=-1;
        u[i].tx=(s16)tx; u[i].ty=(s16)ty; u[i].path_x=u[i].path_y=0;
        ++sent; ++race_campaign.enemy_units_sent;
    }
    if (race_z7_ai_committed_units()<MAX_UNITS-SC64_Z7_AI_RESERVE_UNITS) {
        for (int i=0;i<MAX_BUILDINGS;++i) if (building_complete(i) && b[i].alive &&
            b[i].owner==ENEMY && b[i].raw_owner==4 &&
            (b[i].type==B_HATCHERY || b[i].type==B_LAIR || b[i].type==B_HIVE) &&
            b[i].queue_count==0) { producer=i; break; }
        if (producer>=0 && race_queue_unit(ENEMY,producer,U_ZERGLING))
            race_z7.runtime.ai_plan_phase=(u8)((race_z7.runtime.ai_plan_phase+1u)&3u);
    }
    return 1;
}

static void race_z7_reconcile_paid_products(void)
{
    if (!race_z7_active() || !race_z7_map) return;
    /* Production/build placement may create runtime actors with no CHK slot.
       The descriptor gives a unique live owner for SIDE and ENEMY; bind those
       products to their authored source slot without inventing provenance for
       neutral/ambiguous actors. */
    for (int i=0;i<MAX_UNITS;++i) if (u[i].alive && u[i].raw_owner==-1) {
        unsigned slot; int raw=sc64_race_raw_unit(u[i].type);
        if (raw>=0 && sc64_race_unit_from_raw(raw)==u[i].type &&
            race_z7_world_owner_slot(&race_z7_world,u[i].owner,&slot)) {
            u[i].raw_owner=(s16)slot;
            u[i].raw_type=(s16)raw;
        }
    }
    for (int i=0;i<MAX_BUILDINGS;++i) if (b[i].alive && b[i].raw_owner==-1) {
        unsigned slot; int raw=sc64_race_raw_building(b[i].type);
        if (raw>=0 && sc64_race_building_from_raw(raw)>=0 &&
            race_z7_world_owner_slot(&race_z7_world,b[i].owner,&slot)) {
            b[i].raw_owner=(s16)slot;
            b[i].raw_type=(s16)raw;
        }
    }
}

static int race_z7_live_world_valid(void)
{
    if (!race_z7_map) return 0;
    for (int i=0;i<MAX_UNITS;++i) if (u[i].alive) {
        int raw=u[i].raw_type; unsigned slot=(unsigned)u[i].raw_owner;
        if (u[i].raw_owner<0 || slot>=8u ||
            !race_z7_world.source_slot_active[slot] ||
            race_z7_world.live_owner_for_source[slot]!=u[i].owner || raw<0 ||
            sc64_race_unit_from_raw(raw)!=u[i].type) return 0;
    }
    for (int i=0;i<MAX_BUILDINGS;++i) if (b[i].alive) {
        unsigned slot=(unsigned)b[i].raw_owner;
        if (b[i].raw_owner<0 || slot>=8u ||
            !race_z7_world.source_slot_active[slot] ||
            race_z7_world.live_owner_for_source[slot]!=b[i].owner ||
            b[i].raw_type<0 || sc64_race_building_from_raw(b[i].raw_type)<0) return 0;
    }
    return 1;
}

static int race_z7_runtime_init(const Sc64RaceMap *map)
{
    if (!race_z7_active() || !race_z7_map_shape_valid(map) ||
        !race_z7_starting_world_matches(map) || !race_z7_bind_world(map)) return 0;
    memset(&race_z7,0,sizeof(race_z7));
    race_z7_map=map;
    race_z7_ops.context=&race_z7_world;
    race_z7_ops.condition=sc64_z7_world_condition;
    race_z7_ops.action=race_z7_action_callback;
    return 1;
}

static int race_z7_rebind_loaded_world(const Sc64RaceMap *map)
{
    if (!race_z7_active() || !sc64_z7_persistent_state_valid(&race_z7) ||
        !race_z7_map_shape_valid(map) || !race_z7_bind_world(map)) return 0;
    race_z7_map=map;
    race_z7_ops.context=&race_z7_world;
    race_z7_ops.condition=sc64_z7_world_condition;
    race_z7_ops.action=race_z7_action_callback;
    race_z7_reconcile_paid_products();
    return race_z7_live_world_valid();
}

static int race_z7_runtime_tick(int user_paused,u32 elapsed_frames)
{
    int result;
    if (!race_z7_active()) return 1;
    if (!race_z7_map || !sc64_z7_persistent_state_valid(&race_z7)) {
        race_z7.runtime.fault=SC64_Z7_RUNTIME_FAULT_BINDING; return 0;
    }
    race_z7_world.elapsed_frames=elapsed_frames;
    if (user_paused || race_z7.runtime.model.outcome!=SC64_Z7_RUNNING) return 1;
    race_z7_reconcile_paid_products();
    if (!race_z7_live_world_valid()) {
        race_z7.runtime.fault=SC64_Z7_RUNTIME_FAULT_ACTOR; return 0;
    }
    result=sc64_z7_runtime_tick(&race_z7.runtime,&race_z7_ops,0);
    if (result<0) return 0;
    /* A terminal action ends mission processing immediately. Other source
     * programs may have been active concurrently; discard their now-irrelevant
     * cursors/timers so no finished trigger remains half-advanced in a save. */
    if (race_z7.runtime.model.outcome!=SC64_Z7_RUNNING) {
        for (unsigned i=0;i<SC64_Z7_TRIGGER_COUNT;++i) {
            race_z7.runtime.model.action_cursor[i]=0;
            race_z7.runtime.action_ticks[i]=0;
        }
        return sc64_z7_persistent_state_valid(&race_z7);
    }
    if (race_z7.runtime.ai_script_mask) {
        if (race_z7.runtime.ai_plan_ticks) --race_z7.runtime.ai_plan_ticks;
        if (!race_z7.runtime.ai_plan_ticks) {
            if (!race_z7_authored_ai_pulse()) {
                race_z7.runtime.fault=SC64_Z7_RUNTIME_FAULT_AI; return 0;
            }
            race_z7.runtime.ai_plan_ticks=SC64_Z7_AI_PERIOD_TICKS;
        }
    }
    return sc64_z7_persistent_state_valid(&race_z7);
}

#endif /* SC64_Z7_RUNTIME */
#endif
