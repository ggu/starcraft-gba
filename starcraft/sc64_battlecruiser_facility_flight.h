/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_BATTLECRUISER_FACILITY_FLIGHT_H
#define SC64_BATTLECRUISER_FACILITY_FLIGHT_H
/* Experimental common Facility flight. Existing compact Building fields
 * encode the destination: build_progress=x, queue_progress=y, level=1 while
 * landing. No mission-state union or new actor pool is used. */
static int race_bc_facility_fields_valid(const Building *q,unsigned ww,unsigned wh)
{
    if (!q || !ww || !wh) return 0;
    if (!q->alive || sc64_race_raw_building(q->type)!=116 || !(q->flags&4)) return 1;
    if (q->under_construction || q->owner<0 || q->owner>=5 || q->queue_count ||
        q->level<0 || q->level>1 || (q->raw_type!=-1 && q->raw_type!=116)) return 0;
    if (q->level==0) return q->build_progress==0 && q->queue_progress==0;
    return q->build_progress>=24 && q->queue_progress>=24 &&
        (unsigned)q->build_progress<ww-24 && (unsigned)q->queue_progress<wh-24;
}
static int race_bc_facility_airborne(int slot)
{
    return slot>=0 && slot<MAX_BUILDINGS && b[slot].alive &&
        (b[slot].flags&4)!=0 && sc64_race_raw_building(b[slot].type)==116;
}
static int race_bc_facility_landing_site(int slot,int x,int y)
{
    if (!race_bc_facility_airborne(slot) || x<24 || y<24 ||
        x>=WORLD_W-24 || y>=WORLD_H-24 || !can_place(b[slot].type,x,y)) return 0;
    for (int yy=y-8;yy<=y+8;yy+=16)
        for (int xx=x-8;xx<=x+8;xx+=16)
            if (terrain_blocked(xx/16,yy/16)) return 0;
    for (int i=0;i<MAX_BUILDINGS;++i)
        if (i!=slot && race_bc_facility_airborne(i) && b[i].level==1 &&
            dist2(x,y,b[i].build_progress,b[i].queue_progress)<40*40) return 0;
    for (int i=0;i<MAX_UNITS;++i)
        if (u[i].alive && !SC64_TRANSPORT_LOADED(&u[i]) &&
            !sc64_race_unit_flying(u[i].type) && dist2(x,y,u[i].x,u[i].y)<20*20) return 0;
    return 1;
}
static int race_bc_facility_lift(int slot)
{
    if (slot<0 || slot>=MAX_BUILDINGS || !b[slot].alive ||
        b[slot].owner!=SIDE || !building_complete(slot) ||
        sc64_race_raw_building(b[slot].type)!=116 || (b[slot].flags&4) ||
        b[slot].queue_count || b[slot].level || race_bc_parent_addon_constructing(slot)) return 0;
    race_bc_parent_detach(slot);
    b[slot].flags|=4; b[slot].level=0;
    b[slot].build_progress=0; b[slot].queue_progress=0;
    invalidate_path_grid(); target_lists_valid=0;
    return 1;
}
static int race_bc_facility_land(int slot,int x,int y)
{
    if (!race_bc_facility_airborne(slot) || b[slot].owner!=SIDE ||
        !race_bc_facility_landing_site(slot,x,y)) return 0;
    b[slot].build_progress=x; b[slot].queue_progress=y; b[slot].level=1;
    return 1;
}
static void race_bc_facility_flight_tick(void)
{
#ifdef SC64_TERRAN_LATE_RUNTIME
    if (race_late_active()) return; /* legacy campaign retains its saved union */
#endif
    for (int i=0;i<MAX_BUILDINGS;++i) {
        int x,y;
        if (b[i].level!=1 || !race_bc_facility_airborne(i)) continue;
        x=b[i].build_progress; y=b[i].queue_progress;
        if (frame%5<2) {
            b[i].x += x>b[i].x ? 1 : x<b[i].x ? -1 : 0;
            b[i].y += y>b[i].y ? 1 : y<b[i].y ? -1 : 0;
        }
        if (b[i].x!=x || b[i].y!=y || !race_bc_facility_landing_site(i,x,y)) continue;
        b[i].flags &= ~4; b[i].level=0; b[i].build_progress=0; b[i].queue_progress=0;
        invalidate_path_grid(); target_lists_valid=0; race_bc_parent_relink(i);
    }
}
#endif
