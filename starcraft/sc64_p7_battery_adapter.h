#ifndef SC64_P7_BATTERY_ADAPTER_H
#define SC64_P7_BATTERY_ADAPTER_H

#ifdef SC64_P7_BATTERY_ENABLED
#include "sc64_p7_battery_rules.h"
#include "sc64_p7_geometry.h"
#include "sc64_p7_battery_properties_generated.h"

static unsigned sc64_p7_battery_xy_length(unsigned x, unsigned y)
{
    unsigned major=x>y?x:y, minor=x>y?y:x;
    if (major/4u<minor)
        major=major-major/16u+minor*3u/8u-major/64u+minor*3u/256u;
    return major;
}

static int sc64_p7_battery_unit_valid(int slot, int battery)
{
    const Sc64RaceUnitRule *rule;
    const Unit *unit;
    const Building *source;
    if (slot<0 || slot>=MAX_UNITS || battery<0 || battery>=MAX_BUILDINGS)
        return 0;
    unit=&u[slot]; source=&b[battery];
    if (!unit->alive || !source->alive || source->type!=B_P7_SHIELD_BATTERY ||
        source->raw_type!=172 || source->under_construction ||
        source->queue_count || source->raw_owner<0 || source->raw_owner>=8 ||
        unit->raw_owner!=source->raw_owner || unit->owner!=source->owner ||
        SC64_TRANSPORT_LOADED(unit) ||
        unit->raw_type==73 || unit->raw_type==85) return 0;
    rule=sc64_race_unit_rule(unit->type);
    if (!rule || rule->race!=R_PROTOSS || !(rule->flags&SC_RACE_F_SHIELDS) ||
        unit->shield>=rule->shields || unit->shield<0 ||
        unit->shield_remainder<0 || unit->shield_remainder>=1280 ||
        (sc64_game_unit_actor_flags(unit->type) &&
         !(sc64_game_unit_actor_flags(unit->type)&SC64_GAME_ACTOR_MOVABLE)))
        return 0;
    return 1;
}

static int sc64_p7_battery_fields_valid(int battery)
{
    const Building *q;
    if (battery<0 || battery>=MAX_BUILDINGS) return 0;
    q=&b[battery];
    return q->alive && q->type==B_P7_SHIELD_BATTERY && q->raw_type==172 &&
        q->queue_count==0 && q->queue_head==0 &&
        q->raw_owner>=0 && q->raw_owner<8 &&
        q->queue_progress>=0 && q->queue_progress<=SC64_P7_BATTERY_MAX_ENERGY_Q8 &&
        q->level>=0 && q->level<SC64_P7_BATTERY_REGEN_DEN &&
        sc64_p7_battery_energy_valid((unsigned)q->queue_progress,
                                     (unsigned)q->level);
}

static int sc64_p7_battery_powered_live(int battery)
{
    const Building *target;
    if (battery<0 || battery>=MAX_BUILDINGS || !b[battery].alive) return 0;
    target=&b[battery];
    if (target->type!=B_P7_SHIELD_BATTERY || target->raw_type!=172 ||
        target->raw_owner<0 || target->raw_owner>=8) return 0;
    for (int i=0;i<MAX_BUILDINGS;++i) if (building_complete(i) &&
        b[i].type==B_PYLON && b[i].raw_type==156 &&
        b[i].owner==target->owner && b[i].raw_owner==target->raw_owner &&
        sc64_p7_in_psi_range(target->x-b[i].x,target->y-b[i].y)) return 1;
    return 0;
}

static int sc64_p7_battery_target_valid(int battery)
{ return sc64_p7_battery_fields_valid(battery) &&
         !b[battery].under_construction && sc64_p7_battery_powered_live(battery); }

static int sc64_p7_battery_distance_unit(int unit, int battery)
{
    int al,at,ar,ab,bl,bt,br,bb;
    if (!sc64_p7_actor_rect_source((unsigned)u[unit].raw_type,u[unit].x,
            u[unit].y,0,&al,&at,&ar,&ab) ||
        !sc64_p7_actor_rect_source((unsigned)b[battery].raw_type,b[battery].x,
            b[battery].y,1,&bl,&bt,&br,&bb)) return -1;
    int dx=al>=br?al-(br-1):bl>=ar?bl-(ar-1):0;
    int dy=at>=bb?at-(bb-1):bt>=ab?bt-(ab-1):0;
    return (int)sc64_p7_battery_xy_length((unsigned)dx,(unsigned)dy);
}

/* OpenBW square_at() creates [center-half, center+half), and the unit finder
 * intersects that half-open rectangle with the actor's half-open bounds. */
static int sc64_p7_battery_click_overlap(int left,int top,int right,int bottom,
                                         int px,int py)
{
    return right>px-128 && left<px+128 &&
           bottom>py-128 && top<py+128;
}

/* A slot is not a persistent building identity. Clear old orders before the
 * allocator can reuse a dead Battery's row, including source-trigger deaths. */
static void race_p7_battery_clear_building_references(int slot)
{
    if (!race_p7_active() || slot<0 || slot>=MAX_BUILDINGS) return;
    for (int i=0;i<MAX_UNITS;++i) if (u[i].alive &&
        u[i].order==ORDER_P7_RECHARGE && u[i].target==slot) {
        u[i].order=ORDER_IDLE; u[i].target=-1; u[i].work_timer=0;
        u[i].tx=u[i].x; u[i].ty=u[i].y;
        u[i].path_timer=0; u[i].path_retry=0;
        u[i].path_x=u[i].x; u[i].path_y=u[i].y;
    }
}

/* Keep this bounded optional service in ROM rather than expanding the main
 * IWRAM order loop with geometry, energy and shield-transfer code. */
static int __attribute__((noinline)) race_p7_battery_tick_unit(int slot)
{
    Unit *q;
    int battery, distance;
    unsigned energy, shield, remainder;
    const Sc64RaceUnitRule *rule;
    if (!race_p7_active() || slot<0 || slot>=MAX_UNITS) return 0;
    q=&u[slot];
    if (!q->alive || q->order!=ORDER_P7_RECHARGE) return 0;
    battery=q->target;
    if (!sc64_p7_battery_target_valid(battery) ||
        !sc64_p7_battery_unit_valid(slot,battery)) goto cancel;
    if (b[battery].queue_progress<=0) goto cancel;
    distance=sc64_p7_battery_distance_unit(slot,battery);
    if (distance<0) goto cancel;
    if (distance>128) { move_toward(q,b[battery].x,b[battery].y); return 1; }
    rule=sc64_race_unit_rule(q->type);
    energy=(unsigned)b[battery].queue_progress;
    shield=(unsigned)q->shield;
    remainder=(unsigned)q->shield_remainder;
    if (!sc64_p7_battery_transfer(&energy,&shield,rule->shields,&remainder))
        goto cancel;
    b[battery].queue_progress=(int)energy;
    q->shield=(int)shield;
    q->shield_remainder=(int)remainder;
    if (shield>=rule->shields || energy==0) goto cancel;
    return 1;
cancel:
    q->order=ORDER_IDLE; q->target=-1; q->work_timer=0;
    return 1;
}

static int race_p7_battery_command_available(void)
{
    if (!race_p7_active() || race_human!=R_PROTOSS || selected_building<0 ||
        selected_building>=MAX_BUILDINGS) return 0;
    for (int i=0;i<MAX_UNITS;++i) if (u[i].alive && u[i].selected) return 0;
    const Building *q=&b[selected_building];
    return q->owner==SIDE && q->raw_owner==7 &&
        sc64_p7_battery_target_valid(selected_building) && q->queue_progress>0;
}

static int race_p7_battery_build_available(void)
{
    int found=0;
    if (!race_p7_active() || race_human!=R_PROTOSS || selected_building>=0 ||
        !race_owner_has_prereq(SIDE,SC_PR_GATEWAY)) return 0;
    for (int i=0;i<MAX_UNITS;++i) if (u[i].alive && u[i].selected) {
        if (u[i].owner!=SIDE || u[i].type!=U_PROBE || u[i].raw_owner!=7 ||
            SC64_TRANSPORT_LOADED(&u[i]))
            return 0;
        found=1;
    }
    return found;
}

static int race_p7_battery_issue_command(int battery, int x, int y)
{
    int count=0, unit=selection_target();
    if (!race_p7_battery_command_available() || battery!=selected_building ||
        !sc64_p7_battery_target_valid(battery)) return 0;
    /* A tap on an actor gives the native single-unit order. Field clicks
       reproduce find_units_noexpand(square_at(pos,128)) rectangle overlap. */
    if (unit>=0 && sc64_p7_battery_unit_valid(unit,battery)) {
        u[unit].order=ORDER_P7_RECHARGE; u[unit].target=battery;
        u[unit].work_timer=0; ++count;
    } else {
        int px=x*2, py=y*2;
        for (int i=0;i<MAX_UNITS;++i) if (sc64_p7_battery_unit_valid(i,battery)) {
            int l,t,r,bottom;
            if (!sc64_p7_actor_rect_source((unsigned)u[i].raw_type,u[i].x,
                    u[i].y,0,&l,&t,&r,&bottom)) continue;
            if (!sc64_p7_battery_click_overlap(l,t,r,bottom,px,py)) continue;
            u[i].order=ORDER_P7_RECHARGE; u[i].target=battery;
            u[i].work_timer=0; ++count;
        }
    }
    return count;
}

static int race_p7_battery_initialize_source(const Sc64RaceMap *map)
{
    unsigned seen=0;
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
