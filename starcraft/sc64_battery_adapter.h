/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_BATTERY_ADAPTER_H
#define SC64_BATTERY_ADAPTER_H

#ifdef SC64_BATTERY_SERVICE_ENABLED
#include "sc64_p7_battery_rules.h"
#include "sc64_p7_power_geometry.h"
#include <limits.h>
#ifdef SC64_P7_ACTORS
#include "sc64_p7_geometry.h"
#endif

static int race_battery_p7_context(void)
{
#ifdef SC64_P7_RUNTIME
    return race_p7_active();
#else
    return 0;
#endif
}

static int race_battery_active(void)
{
#ifdef SC64_SHARED_BATTERY
    return 1;
#else
    return race_battery_p7_context();
#endif
}

static int race_battery_recharge_order(void)
{
#ifdef SC64_P7_BATTERY_ENABLED
    if (race_battery_p7_context()) return ORDER_P7_RECHARGE;
#endif
#ifdef SC64_SHARED_BATTERY
    return ORDER_RECHARGE;
#else
    return -1;
#endif
}

static int race_battery_order_is_recharge(int order)
{
#ifdef SC64_SHARED_BATTERY
    if (order == ORDER_RECHARGE) return 1;
#endif
#ifdef SC64_P7_BATTERY_ENABLED
    if (order == ORDER_P7_RECHARGE && race_battery_p7_context()) return 1;
#endif
    return 0;
}

/* Explicit raw identities must agree with the canonical type. */
static int sc64_battery_raw_unit(const Unit *q)
{
    int raw=sc64_race_raw_unit(q->type);
    return raw>=0 && (q->raw_type==-1 || q->raw_type==raw) ? raw : -1;
}
static int sc64_battery_raw_building(const Building *q)
{
    int raw=sc64_race_raw_building(q->type);
    return raw>=0 && (q->raw_type==-1 || q->raw_type==raw) ? raw : -1;
}
static int sc64_battery_owner_valid(int live,int raw)
{ return live>=0 && live<5 && raw>=-1 && raw<8; }
static int sc64_battery_owner_matches(int a_live,int a_raw,int b_live,int b_raw)
{
    if (!sc64_battery_owner_valid(a_live,a_raw) ||
        !sc64_battery_owner_valid(b_live,b_raw) || a_live!=b_live) return 0;
    /* Custom players use live ownership; rescued SIDE actors in later
     * campaigns may retain a source slot. P7 keeps its exact source identity. */
    if (race_battery_p7_context()) return a_raw>=0 && a_raw==b_raw;
    if (!race_mode && a_live!=SIDE) return a_raw==b_raw;
    return 1;
}

/* Pinned BWAPI d727fed source-pixel bounds. P7 builds use their fact table;
 * the fallback covers native and mapped hero Protoss actors in other builds. */
static int sc64_battery_rect(unsigned raw,int x,int y,int building,
                              int *left,int *top,int *right,int *bottom)
{
    int l,t,r,d;
    if (!left || !top || !right || !bottom || x<0 || y<0 ||
        x>(INT_MAX-256)/2 || y>(INT_MAX-256)/2) return 0;
#ifdef SC64_P7_ACTORS
    if (sc64_p7_actor_rect_source(raw,x,y,building,left,top,right,bottom)) return 1;
#endif
    if (building) {
        if (raw!=172) return 0;
        l=32;t=16;r=32;d=16;
    } else switch(raw) {
    case 64:l=11;t=11;r=11;d=11;break;
    case 65:case 77:l=11;t=5;r=11;d=13;break;
    case 66:case 78:l=15;t=15;r=16;d=16;break;
    case 67:case 79:l=12;t=10;r=11;d=13;break;
    case 68:l=16;t=16;r=15;d=15;break;
    case 69:l=20;t=16;r=19;d=15;break;
    case 70:l=18;t=16;r=17;d=15;break;
    case 71:l=22;t=22;r=21;d=21;break;
    case 72:l=32;t=32;r=31;d=31;break;
    case 74:case 75:l=12;t=6;r=11;d=19;break;
    case 83:case 84:l=16;t=16;r=15;d=15;break;
    default:return 0;
    }
    *left=x*2-l;*top=y*2-t;*right=x*2+r+1;*bottom=y*2+d+1;
    return 1;
}

static unsigned sc64_battery_xy_length(unsigned x,unsigned y)
{
    unsigned major=x>y?x:y,minor=x>y?y:x;
    if(major/4u<minor) major=major-major/16u+minor*3u/8u-major/64u+minor*3u/256u;
    return major;
}
static int race_battery_fields_valid(int battery)
{
    if(battery<0 || battery>=MAX_BUILDINGS) return 0;
    const Building *q=&b[battery];
    return q->alive && sc64_battery_raw_building(q)==172 &&
        sc64_battery_owner_valid(q->owner,q->raw_owner) &&
        (!race_battery_p7_context() || (q->raw_type==172 && q->raw_owner>=0)) &&
        q->queue_count==0 && q->queue_head==0 &&
        q->queue_progress>=0 && q->queue_progress<=SC64_P7_BATTERY_MAX_ENERGY_Q8 &&
        q->level>=0 && q->level<SC64_P7_BATTERY_REGEN_DEN &&
        sc64_p7_battery_energy_valid((unsigned)q->queue_progress,(unsigned)q->level);
}
/* Construction uses the same pinned psi mask as a live Battery. */
static int race_battery_site_powered(int owner,int x,int y)
{
    if(owner<0 || owner>=5 || x<0 || y<0 || x>=WORLD_W || y>=WORLD_H) return 0;
    for(int i=0;i<MAX_BUILDINGS;++i) if(building_complete(i) &&
       sc64_battery_raw_building(&b[i])==156 && b[i].owner==owner &&
       sc64_battery_owner_valid(b[i].owner,b[i].raw_owner) &&
       (!race_battery_p7_context() || (owner==SIDE && b[i].raw_owner==7 && b[i].raw_type==156)) &&
       sc64_p7_in_psi_range(x-b[i].x,y-b[i].y)) return 1;
    return 0;
}
static int race_battery_powered_live(int battery)
{
    if(battery<0 || battery>=MAX_BUILDINGS || !b[battery].alive ||
       sc64_battery_raw_building(&b[battery])!=172) return 0;
    const Building *q=&b[battery];
    for(int i=0;i<MAX_BUILDINGS;++i) if(building_complete(i) &&
       sc64_battery_raw_building(&b[i])==156 &&
       (!race_battery_p7_context() || b[i].raw_type==156) &&
       sc64_battery_owner_matches(q->owner,q->raw_owner,b[i].owner,b[i].raw_owner) &&
       sc64_p7_in_psi_range(q->x-b[i].x,q->y-b[i].y)) return 1;
    return 0;
}
static int race_battery_target_valid(int battery)
{ return race_battery_fields_valid(battery) && !b[battery].under_construction && race_battery_powered_live(battery); }
#if defined(SC64_T4_RUNTIME) || defined(SC64_ZP_CAMPAIGN_ACTORS)
#define SC64_BATTERY_UNIT_IMMOVABLE(type) \
    (sc64_game_unit_actor_flags(type) && \
     !(sc64_game_unit_actor_flags(type)&SC64_GAME_ACTOR_MOVABLE))
#else
/* Native shielded Protoss identities are movable; immovable scenario actors
 * require the extended actor facts. Keep their API out of plain profiles. */
#define SC64_BATTERY_UNIT_IMMOVABLE(type) 0
#endif
static int race_battery_unit_valid(int unit,int battery)
{
    if(unit<0 || unit>=MAX_UNITS || battery<0 || battery>=MAX_BUILDINGS) return 0;
    const Unit *q=&u[unit];const Building *source=&b[battery];
    int raw=sc64_battery_raw_unit(q);const Sc64RaceUnitRule *rule=sc64_race_unit_rule(q->type);
    if(!q->alive || !source->alive || sc64_battery_raw_building(source)!=172 ||
       source->under_construction || source->queue_count ||
       !sc64_battery_owner_matches(q->owner,q->raw_owner,source->owner,source->raw_owner) ||
       SC64_TRANSPORT_LOADED(q) || SC64_CARRIER_HANGAR_HIDDEN(q) || raw<0 || raw==73 || raw==85 ||
       (race_battery_p7_context() && q->raw_type<0) ||
       !rule || rule->race!=R_PROTOSS || !(rule->flags&SC_RACE_F_SHIELDS) ||
       q->shield>=rule->shields || q->shield<0 || q->shield_remainder<0 || q->shield_remainder>=1280 ||
       SC64_BATTERY_UNIT_IMMOVABLE(q->type)) return 0;
    return 1;
}
static int race_battery_distance_unit(int unit,int battery)
{
    int al,at,ar,ab,bl,bt,br,bb;
    if(unit<0 || unit>=MAX_UNITS || battery<0 || battery>=MAX_BUILDINGS) return -1;
    int raw=sc64_battery_raw_unit(&u[unit]);
    if(raw<0 || sc64_battery_raw_building(&b[battery])!=172 ||
       !sc64_battery_rect((unsigned)raw,u[unit].x,u[unit].y,0,&al,&at,&ar,&ab) ||
       !sc64_battery_rect(172,b[battery].x,b[battery].y,1,&bl,&bt,&br,&bb)) return -1;
    int dx=al>=br?al-(br-1):bl>=ar?bl-(ar-1):0;
    int dy=at>=bb?at-(bb-1):bt>=ab?bt-(ab-1):0;
    return (int)sc64_battery_xy_length((unsigned)dx,(unsigned)dy);
}
static void race_battery_clear_building_references(int slot)
{
    if(!race_battery_active() || slot<0 || slot>=MAX_BUILDINGS) return;
    for(int i=0;i<MAX_UNITS;++i) if(u[i].alive && race_battery_order_is_recharge(u[i].order) && u[i].target==slot) {
        u[i].order=ORDER_IDLE;u[i].target=-1;u[i].work_timer=0;u[i].tx=u[i].x;u[i].ty=u[i].y;
        u[i].path_timer=0;u[i].path_retry=0;u[i].path_x=u[i].x;u[i].path_y=u[i].y;
    }
}
static int __attribute__((noinline)) race_battery_tick_unit(int slot)
{
    if(!race_battery_active() || slot<0 || slot>=MAX_UNITS) return 0;
    Unit *q=&u[slot];if(!q->alive || !race_battery_order_is_recharge(q->order)) return 0;
    int battery=q->target;
    if(!race_battery_target_valid(battery) || !race_battery_unit_valid(slot,battery) || b[battery].queue_progress<=0) goto cancel;
    int distance=race_battery_distance_unit(slot,battery);if(distance<0) goto cancel;
    if(distance>128){move_toward(q,b[battery].x,b[battery].y);return 1;}
    const Sc64RaceUnitRule *rule=sc64_race_unit_rule(q->type);
    unsigned energy=(unsigned)b[battery].queue_progress,shield=(unsigned)q->shield,rem=(unsigned)q->shield_remainder;
    if(!sc64_p7_battery_transfer(&energy,&shield,rule->shields,&rem)) goto cancel;
    b[battery].queue_progress=(int)energy;q->shield=(int)shield;q->shield_remainder=(int)rem;
    if(shield>=rule->shields || energy==0) goto cancel;
    return 1;
cancel:q->order=ORDER_IDLE;q->target=-1;q->work_timer=0;return 1;
}
static int race_battery_command_available(void)
{
    if(!race_battery_active() || selected_building<0 || selected_building>=MAX_BUILDINGS) return 0;
    for(int i=0;i<MAX_UNITS;++i) if(u[i].alive && u[i].selected) return 0;
    const Building *q=&b[selected_building];
    return q->owner==SIDE && (!race_battery_p7_context() || (race_human==R_PROTOSS && q->raw_owner==7)) &&
        race_battery_target_valid(selected_building) && q->queue_progress>0;
}
static int race_battery_build_available(void)
{
    int found=0;
    if(!race_battery_p7_context() || race_human!=R_PROTOSS || selected_building>=0 ||
       !race_owner_has_prereq(SIDE,SC_PR_GATEWAY)) return 0;
    for(int i=0;i<MAX_UNITS;++i) if(u[i].alive && u[i].selected){
        if(u[i].owner!=SIDE || u[i].type!=U_PROBE || u[i].raw_owner!=7 || SC64_TRANSPORT_LOADED(&u[i])) return 0;
        found=1;
    }
    return found;
}
static int race_battery_issue_command(int battery,int x,int y)
{
    int count=0,unit=selection_target(),order=race_battery_recharge_order();
    if(!race_battery_command_available() || battery!=selected_building || order<0) return 0;
    if(unit>=0 && race_battery_unit_valid(unit,battery)){
        u[unit].order=order;u[unit].target=battery;u[unit].work_timer=0;return 1;
    }
    if(x<0 || y<0 || x>(INT_MAX-128)/2 || y>(INT_MAX-128)/2) return 0;
    int px=x*2,py=y*2;
    for(int i=0;i<MAX_UNITS;++i) if(race_battery_unit_valid(i,battery)){
        int l,t,r,d,raw=sc64_battery_raw_unit(&u[i]);
        if(raw<0 || !sc64_battery_rect((unsigned)raw,u[i].x,u[i].y,0,&l,&t,&r,&d)) continue;
        if(r<=px-128 || l>=px+128 || d<=py-128 || t>=py+128) continue;
        u[i].order=order;u[i].target=battery;u[i].work_timer=0;++count;
    }
    return count;
}
#endif /* SC64_BATTERY_SERVICE_ENABLED */
#endif
