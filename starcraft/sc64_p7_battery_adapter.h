#ifndef SC64_P7_BATTERY_ADAPTER_H
#define SC64_P7_BATTERY_ADAPTER_H

#ifdef SC64_P7_BATTERY_ENABLED
#include "sc64_p7_battery_properties_generated.h"
/* Legacy fixture names and authenticated P7 property hooks. The actual
 * recharge/energy mechanics live in the shared engine adapter. */
#define sc64_p7_battery_xy_length sc64_battery_xy_length
#define sc64_p7_battery_unit_valid race_battery_unit_valid
#define sc64_p7_battery_fields_valid race_battery_fields_valid
#define sc64_p7_battery_powered_live race_battery_powered_live
#define sc64_p7_battery_target_valid race_battery_target_valid
#define sc64_p7_battery_distance_unit race_battery_distance_unit
#define race_p7_battery_clear_building_references race_battery_clear_building_references
#define race_p7_battery_tick_unit race_battery_tick_unit
#define race_p7_battery_command_available race_battery_command_available
#define race_p7_battery_build_available race_battery_build_available
#define race_p7_battery_issue_command race_battery_issue_command

static int race_protoss_generic_battery_candidate_valid(const Building *q)
{
    return q && RACE_PROTOSS_RAW(q)==172 && q->queue_count==0 && q->queue_head==0 &&
        q->queue_progress>=0 && q->queue_progress<=SC64_P7_BATTERY_MAX_ENERGY_Q8 &&
        q->level>=0 && q->level<SC64_P7_BATTERY_REGEN_DEN &&
        sc64_p7_battery_energy_valid((unsigned)q->queue_progress,(unsigned)q->level) &&
        q->shield<=sc64_race_building_shields(B_P7_SHIELD_BATTERY);
}

static int sc64_p7_battery_click_overlap(int left,int top,int right,int bottom,int px,int py)
{ return right>px-128 && left<px+128 && bottom>py-128 && top<py+128; }

static int race_p7_battery_initialize_source(const Sc64RaceMap *map)
{
    unsigned seen=0;
    if (race_protoss_service_binding()==SC64_PROTOSS_BIND_GENERIC) return 1;
#ifdef SC64_PROTOSS_LATER_BINDINGS
    if (race_protoss_service_binding()==SC64_PROTOSS_BIND_LATER)
        return race_protoss_later_battery_initialize(map);
#endif
    if (!race_p7_active() || !map || map!=&sc64_race_maps[SC64_P7_GLOBAL_ID] ||
        map->unit_count!=357u) return 0;
    for (unsigned row=0;row<map->unit_count;++row) {
        const Sc64ImportedUnit *src=&map->units[row];
        if (src->source_type!=172) continue;
        int property=-1;
        for (unsigned p=0;p<2;++p) {
            const Sc64P7BatterySourceProperty *want=&sc64_p7_battery_source_properties[p];
            if (src->owner==want->owner && src->x_px==want->x_source/2u &&
                src->y_px==want->y_source/2u) property=(int)p;
        }
        if (property<0 || (seen&(1u<<(unsigned)property))) return 0;
        const Sc64P7BatterySourceProperty *want=&sc64_p7_battery_source_properties[property];
        if (want->valid!=0x0fu || want->hp!=100 || want->shields!=100 ||
            want->energy!=100) return 0;
        int owner=race_p7_source_owner(want->owner), found=-1;
        for (int i=0;i<MAX_BUILDINGS;++i) if (b[i].alive &&
            b[i].type==B_P7_SHIELD_BATTERY && b[i].raw_type==172 &&
            b[i].raw_owner==want->owner && b[i].owner==owner &&
            b[i].x==src->x_px && b[i].y==src->y_px) { found=i; break; }
        if (owner<0 || found<0) return 0;
        b[found].shield=(unsigned short)(sc64_race_building_shields(b[found].type)*want->shields/100u);
        b[found].queue_progress=(int)(SC64_P7_BATTERY_MAX_ENERGY_Q8*want->energy/100u);
        b[found].level=0;
        seen|=1u<<(unsigned)property;
    }
    /* Startup must bind both authenticated map rows. Later destruction is
       accepted by the separate live-world validator. */
    return seen==3u;
}

static int race_p7_battery_live_world_valid(void)
{
    unsigned seen=0;
#ifdef SC64_P7_ARBITER_CLOAK_CACHE_ENABLED
    if (!sc64_p7_arbiter_cache_world_valid()) return 0;
#endif
    if (race_protoss_service_binding()==SC64_PROTOSS_BIND_GENERIC) {
        for (int i=0;i<MAX_BUILDINGS;++i) if (b[i].alive &&
            b[i].type==B_P7_SHIELD_BATTERY &&
            (RACE_PROTOSS_RAW(&b[i])!=172 || !sc64_p7_battery_fields_valid(i))) return 0;
        return 1;
    }
#ifdef SC64_PROTOSS_LATER_BINDINGS
    if (race_protoss_service_binding()==SC64_PROTOSS_BIND_LATER)
        return race_protoss_later_battery_world_valid();
#endif
    for (int i=0;i<MAX_BUILDINGS;++i) if (b[i].alive &&
        b[i].type==B_P7_SHIELD_BATTERY) {
        if (b[i].raw_type!=172 || !sc64_p7_battery_fields_valid(i)) return 0;
        if (b[i].raw_owner==0 || b[i].raw_owner==4) {
            int property=b[i].raw_owner==0?0:1;
            const Sc64P7BatterySourceProperty *want=&sc64_p7_battery_source_properties[property];
            if (b[i].x!=(int)(want->x_source/2u) ||
                b[i].y!=(int)(want->y_source/2u) ||
                b[i].owner!=race_p7_source_owner(want->owner) ||
                (seen&(1u<<(unsigned)property))) return 0;
            seen|=1u<<(unsigned)property;
        } else if (b[i].raw_owner!=7 || b[i].owner!=SIDE) return 0;
    }
    /* Starting map Batteries may be destroyed; any survivor must still
       match its authenticated source slot and coordinates. */
    return 1;
}
#endif

#endif
