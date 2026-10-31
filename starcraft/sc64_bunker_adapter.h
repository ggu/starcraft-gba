/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_BUNKER_ADAPTER_H
#define SC64_BUNKER_ADAPTER_H
#include "sc64_bunker_rules.h"

/* Cargo shares the transport order and unit pool. A building target is
 * distinguished by MAX_UNITS; no save fields or persistent pool are added. */
static int __attribute__((noinline)) race_bunker_carrier_valid(int slot)
{
    if (slot<0 || slot>=MAX_BUILDINGS) return 0;
    const Building *q=&b[slot];
    return q->alive && !q->under_construction && q->owner>=0 && q->owner<5 &&
        sc64_race_raw_building(q->type)==SC64_BUNKER_RAW &&
        (q->raw_type==-1 || q->raw_type==SC64_BUNKER_RAW);
}
static int __attribute__((noinline)) race_bunker_passenger_valid(const Unit *q, int owner)
{
    if (!q || !q->alive || q->raw_type < -1 || owner<0 || owner>=5 || q->owner!=owner ||
        (q->flags&2) || sc64_race_unit_flying(q->type)) return 0;
    int raw=q->raw_type>=0?q->raw_type:sc64_race_raw_unit(q->type);
    if (!sc64_bunker_passenger_raw(raw) || sc64_race_unit_from_raw(raw)!=q->type) return 0;
    return 1;
}
/* Return used capacity; malformed occupancy is rejected. */
static int __attribute__((noinline)) race_bunker_cargo_space(int slot)
{
    if (!race_bunker_carrier_valid(slot)) return -1;
    int target=sc64_bunker_target_encode(MAX_UNITS,MAX_BUILDINGS,slot),used=0;
    if (target<0) return -1;
    for (int i=0;i<MAX_UNITS;++i) if (u[i].alive && u[i].order==ORDER_TRANSPORTED && u[i].target==target) {
        if (!race_bunker_passenger_valid(&u[i],b[slot].owner) ||
            !sc64_bunker_owner_matches(u[i].owner,u[i].raw_owner,b[slot].owner,b[slot].raw_owner,race_mode,SIDE) || ++used>SC64_BUNKER_CAPACITY) return -1;
    }
    return used;
}
static int __attribute__((noinline)) race_bunker_request_board(int unit, int slot)
{
    if (unit<0 || unit>=MAX_UNITS || !race_bunker_carrier_valid(slot) ||
        SC64_TRANSPORT_LOADED(&u[unit]) || !race_bunker_passenger_valid(&u[unit],b[slot].owner) ||
        !sc64_bunker_owner_matches(u[unit].owner,u[unit].raw_owner,b[slot].owner,b[slot].raw_owner,race_mode,SIDE)) return 0;
    int used=race_bunker_cargo_space(slot),target=sc64_bunker_target_encode(MAX_UNITS,MAX_BUILDINGS,slot);
    if (used<0 || used>=SC64_BUNKER_CAPACITY || target<0) return 0;
    u[unit].order=ORDER_BOARD;u[unit].target=target;u[unit].build=-1;
    u[unit].path_timer=u[unit].path_retry=0;return 1;
}
static int __attribute__((noinline)) race_bunker_tick_board(int unit)
{
    Unit *q=&u[unit];
    if (q->order!=ORDER_BOARD || q->target<MAX_UNITS) return 0;
    int slot=sc64_bunker_target_decode(MAX_UNITS,MAX_BUILDINGS,q->target);
    int used=race_bunker_cargo_space(slot);
    if (used<0 || used>=SC64_BUNKER_CAPACITY || !race_bunker_passenger_valid(q,b[slot].owner) ||
        !sc64_bunker_owner_matches(q->owner,q->raw_owner,b[slot].owner,b[slot].raw_owner,race_mode,SIDE)) {
        q->order=ORDER_IDLE;q->target=-1;q->build=-1;return 1;
    }
    /* The center is blocked by the building footprint: board from its
     * surrounding approach ring, not the transport's 16-pixel center. */
    if (dist2(q->x,q->y,b[slot].x,b[slot].y)>32*32) {
        int best=-1,tx=0,ty=0;
        for (int dy=-32;dy<=32;dy+=8) for (int dx=-32;dx<=32;dx+=8) {
            int d=dx*dx+dy*dy,x=b[slot].x+dx,y=b[slot].y+dy;
            if (d<24*24 || d>32*32 || x<0 || y<0 || x>=WORLD_W || y>=WORLD_H ||
                !is_traversable(x/16,y/16,-1) || !unit_step_clear(q,x,y)) continue;
            int cost=dist2(q->x,q->y,x,y);
            if (best<0 || cost<best) {best=cost;tx=x;ty=y;}
        }
        if (best>=0) move_toward(q,tx,ty);
    } else {q->order=ORDER_TRANSPORTED;q->selected=0;q->x=b[slot].x;q->y=b[slot].y;
        q->path_timer=q->path_retry=0;race_transport_changed();}
    return 1;
}
static int __attribute__((noinline)) race_bunker_land_point(int unit,int slot,int *px,int *py)
{
    for (int radius=24;radius<=64;radius+=8)
        for (int dy=-radius;dy<=radius;dy+=8)
            for (int dx=-radius;dx<=radius;dx+=8) {
                if (FAST_ABS(dx)!=radius && FAST_ABS(dy)!=radius) continue;
                int x=b[slot].x+dx,y=b[slot].y+dy;
                if (x<0 || y<0 || x>=WORLD_W || y>=WORLD_H ||
                    !is_traversable(x/16,y/16,-1) || !unit_step_clear(&u[unit],x,y)) continue;
                *px=x;*py=y;return 1;
            }
    return 0;
}
static int __attribute__((noinline)) race_bunker_unload_one(int unit,int slot)
{
    int x,y;if (!race_bunker_land_point(unit,slot,&x,&y)) return 0;
    Unit *q=&u[unit];q->x=x;q->y=y;q->order=ORDER_IDLE;q->target=-1;q->build=-1;
    q->path_timer=q->path_retry=0;
    race_transport_changed();return 1;
}
static int __attribute__((noinline)) race_bunker_request_unload(int slot)
{
    int count=race_bunker_cargo_space(slot),done=0;
    if (count<=0) return 0;
    int target=MAX_UNITS+slot;
    for (int i=0;i<MAX_UNITS;++i)
        if (u[i].alive && u[i].order==ORDER_TRANSPORTED && u[i].target==target)
            done+=race_bunker_unload_one(i,slot);
    return done;
}
static void __attribute__((noinline)) race_bunker_kill_passenger(int unit,int killer_owner)
{
    Unit *q=&u[unit];
#ifdef SC64_TERRAN_LATE_RUNTIME
    if (race_late_active()) race_late_record_actor_death(killer_owner,-1,q->owner,q->raw_owner,q->raw_type,0);
#else
    (void)killer_owner;
#endif
#ifdef SC64_P5_RUNTIME
    if (race_p5_active()) race_p5_note_unit_death(q);
#endif
#ifdef SC64_P6_RUNTIME
    if (race_p6_active()) race_p6_note_unit_death(q);
#endif
#ifdef SC64_Z6_RUNTIME
    if (race_z6_active()) race_z6_note_unit_death(q);
#endif
    if (q->owner==SIDE) supply_used-=unit_slot_supply(q);
    /* Keep the loaded order on the dead slot until reuse so death_vfx_scan
     * recognizes an off-map casualty and does not draw an infantry corpse. */
    q->hp=0;q->alive=q->selected=0;q->order=ORDER_TRANSPORTED;q->target=-1;q->build=-1;
}
static void __attribute__((noinline)) race_bunker_destroy_cargo(int slot,int killer_owner)
{
    if (slot<0 || slot>=MAX_BUILDINGS) return;
    int target=MAX_UNITS+slot,changed=0;
    for (int i=0;i<MAX_UNITS;++i) if (u[i].alive && u[i].order==ORDER_TRANSPORTED && u[i].target==target) {
        if (!race_bunker_unload_one(i,slot)) race_bunker_kill_passenger(i,killer_owner);
        changed=1;
    }
    if (changed) race_transport_changed();
}
static void __attribute__((noinline)) race_bunker_clear_slot(int slot)
{
    if (slot<0 || slot>=MAX_BUILDINGS) return;
    int target=MAX_UNITS+slot,changed=0;
    for (int i=0;i<MAX_UNITS;++i) if (u[i].alive && u[i].target==target) {
        if (u[i].order==ORDER_BOARD) {u[i].order=ORDER_IDLE;u[i].target=-1;u[i].build=-1;changed=1;}
        else if (u[i].order==ORDER_TRANSPORTED) {race_bunker_kill_passenger(i,-1);changed=1;}
    }
    if (changed) race_transport_changed();
}
static int __attribute__((noinline)) race_bunker_tick_loaded(int unit)
{
    Unit *q=&u[unit];
    if (q->order!=ORDER_TRANSPORTED || q->target<MAX_UNITS) return 0;
    int slot=sc64_bunker_target_decode(MAX_UNITS,MAX_BUILDINGS,q->target);
    if (slot<0 || !race_bunker_carrier_valid(slot) || !race_bunker_passenger_valid(q,b[slot].owner) ||
        !sc64_bunker_owner_matches(q->owner,q->raw_owner,b[slot].owner,b[slot].raw_owner,race_mode,SIDE)) {
        race_bunker_kill_passenger(unit,-1);race_transport_changed();return 1;
    }
    q->x=b[slot].x;q->y=b[slot].y;
    int raw=q->raw_type>=0?q->raw_type:sc64_race_raw_unit(q->type);
    if (sc64_bunker_firing_raw(raw)) race_bunker_fire_unit(unit,slot);
    return 1;
}
#endif
