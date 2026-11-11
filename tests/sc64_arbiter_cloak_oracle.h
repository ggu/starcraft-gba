/* SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Frozen copy of the Arbiter cloak/detection bookkeeping as it was before the
 * lazy-snapshot change (base source 769d84d3): the detector snapshot, the
 * detected-mask computation, the source tick and the cache validity checks.
 * tests/sc64_arbiter_cloak_diff.c runs it next to the shipping code and
 * compares every output after every tick. The helpers it calls (aura
 * candidate, rectangles, optical masks, pylon power, packing) were not
 * changed. Included after starcraft/game.c; only symbol names differ from
 * the original text, and the diagnostic timing hooks were dropped. */
#ifndef SC64_ARBITER_CLOAK_ORACLE_H
#define SC64_ARBITER_CLOAK_ORACLE_H

typedef struct {
    u8 units[MAX_UNITS], buildings[MAX_BUILDINGS];
    u8 optical_units[MAX_UNITS], optical_unit_masks[MAX_UNITS];
    u8 optical_buildings[MAX_BUILDINGS], optical_building_masks[MAX_BUILDINGS];
    unsigned unit_count, building_count, optical_unit_count, optical_building_count;
} OracleDetectorLists;

static void oracle_detector_lists_build(OracleDetectorLists *lists)
{
    int map[8], human_slot=sc64_p7_optical_human_slot();
#ifdef SC64_P7_DETECTOR_BINDING_HOIST
    int binding=race_protoss_service_binding();
    for(unsigned i=0;i<8;++i) map[i]=race_protoss_binding_source_owner(binding,i);
#else
    for(unsigned i=0;i<8;++i) map[i]=race_protoss_source_owner(i);
#endif
    lists->unit_count=lists->building_count=0;
    lists->optical_unit_count=lists->optical_building_count=0;
    for(int i=0;i<MAX_UNITS;++i) if(u[i].alive &&
#ifdef SC64_P7_CARRIER_TRAINING_ENABLED
        !SC64_CARRIER_HANGAR_HIDDEN(&u[i]) &&
#endif
        !SC64_TRANSPORT_LOADED(&u[i])) {
#ifdef SC64_P7_DETECTOR_BINDING_HOIST
        int source=sc64_p7_detector_bound_slot(binding,human_slot,u[i].owner,u[i].raw_owner);
#else
        int source=sc64_p7_detector_current_slot(u[i].owner,RACE_PROTOSS_SLOT(&u[i]));
#endif
        unsigned mask=sc64_p7_optical_source_mask(source,u[i].owner,map,human_slot);
        if(mask) {
            unsigned n=lists->optical_unit_count++;
            lists->optical_units[n]=(u8)i; lists->optical_unit_masks[n]=(u8)mask;
        }
    }
    for(int i=0;i<MAX_BUILDINGS;++i) if(b[i].alive) {
#ifdef SC64_P7_DETECTOR_BINDING_HOIST
        int source=sc64_p7_detector_bound_slot(binding,human_slot,b[i].owner,b[i].raw_owner);
#else
        int source=sc64_p7_detector_current_slot(b[i].owner,RACE_PROTOSS_SLOT(&b[i]));
#endif
        unsigned mask=sc64_p7_optical_source_mask(source,b[i].owner,map,human_slot);
        if(mask) {
            unsigned n=lists->optical_building_count++;
            lists->optical_buildings[n]=(u8)i; lists->optical_building_masks[n]=(u8)mask;
        }
    }
    for(int i=0;i<MAX_UNITS;++i)
        if(u[i].alive && (RACE_PROTOSS_RAW(&u[i])==SC64_P7_OBSERVER_RAW
#ifdef SC64_P7_GEOMETRY_RECT_HOIST_ENABLED
                          || RACE_PROTOSS_RAW(&u[i])==SC64_OVERLORD_RAW
#endif
                          )) lists->units[lists->unit_count++]=(u8)i;
    for(int i=0;i<MAX_BUILDINGS;++i)
        if(b[i].alive && (RACE_PROTOSS_RAW(&b[i])==SC64_P7_CANNON_RAW
#ifdef SC64_P7_GEOMETRY_RECT_HOIST_ENABLED
                          || RACE_PROTOSS_RAW(&b[i])==SC64_SPORE_COLONY_RAW
#endif
                          )) lists->buildings[lists->building_count++]=(u8)i;
}


static unsigned oracle_detected_mask_lists(int target,
        const u8 powered_cannons[MAX_BUILDINGS], const OracleDetectorLists *lists)
{
    unsigned visible_owners=0,detected=0;
    int x,y,col,row,cx,cy, human_slot=sc64_p7_optical_human_slot();
    if(target<0||target>=MAX_UNITS||!u[target].alive) return 0;
    x=u[target].x; y=u[target].y; col=x/16; row=y/16;
    if(col<0||row<0||col>=MAP_COLS||row>=MAP_ROWS) return 0;
    cx=col*16+8; cy=row*16+8;
#ifdef SC64_P7_GEOMETRY_RECT_HOIST_ENABLED
    int target_l,target_t,target_r,target_b;
    if(!sc64_p7_actor_rect_source((unsigned)RACE_PROTOSS_RAW(&u[target]),x,y,0,
                                  &target_l,&target_t,&target_r,&target_b))
        return 0;
#endif
    /* Calculate non-human source visibility in one actor/building pass;
       the authenticated human slot uses the SIDE visibility grid. */
    {
        int cell=col+row*MAP_COLS;
        if(human_slot>=0 && SC64_GRID_GET(visible,cell)) visible_owners|=1u<<(unsigned)human_slot;
    }
    for(int k=0;k<(lists?(int)lists->optical_unit_count:MAX_UNITS);++k) {
        int i=lists?lists->optical_units[k]:k;
#ifdef SC64_P7_DETECTOR_SNAPSHOT_FAST
        /* List entries were checked when this tick's snapshot was built. */
        if (!lists && !(u[i].alive &&
#else
        if (!(u[i].alive &&
#endif
#ifdef SC64_P7_CARRIER_TRAINING_ENABLED
            !SC64_CARRIER_HANGAR_HIDDEN(&u[i]) &&
#endif
            !SC64_TRANSPORT_LOADED(&u[i]))) continue;
#ifndef SC64_P7_DETECTOR_SNAPSHOT_FAST
        int source=sc64_p7_detector_current_slot(u[i].owner,RACE_PROTOSS_SLOT(&u[i]));
#endif
#ifdef SC64_P7_SIGHT_AXIS_CULL
        int dx=cx-u[i].x,dy=cy-u[i].y;
        if(dx < -SC64_P7_SIGHT_UNIT_AXIS_BOUND || dx > SC64_P7_SIGHT_UNIT_AXIS_BOUND ||
            dy < -SC64_P7_SIGHT_UNIT_AXIS_BOUND || dy > SC64_P7_SIGHT_UNIT_AXIS_BOUND) continue;
#else
        int dx=cx-u[i].x,dy=cy-u[i].y;
#endif
        unsigned candidate=lists?lists->optical_unit_masks[k]:0;
        if(!lists) {
#ifdef SC64_P7_DETECTOR_SNAPSHOT_FAST
            int source=sc64_p7_detector_current_slot(u[i].owner,RACE_PROTOSS_SLOT(&u[i]));
#endif
            if(source>=0&&source<8) {
                int live=race_protoss_source_owner((unsigned)source);
                if(live>=0&&live==u[i].owner) candidate=1u<<(unsigned)source;
            } else for(unsigned owner=0;owner<8;++owner)
                if((int)owner!=human_slot && race_protoss_source_owner(owner)==u[i].owner) candidate|=1u<<owner;
        }
#ifdef SC64_SOURCE_SIGHT_ALL_RACES
        if (human_slot>=0) candidate &= ~(1u << (unsigned)human_slot); /* SIDE grid is authoritative. */
#endif
        if(!(candidate & ~visible_owners)) continue;
        int radius=sc64_p7_observer_sight_radius_runtime(u[i].type,0);
        if(dx*dx+dy*dy>radius*radius) continue;
        visible_owners|=candidate;
    }
    for(int k=0;k<(lists?(int)lists->optical_building_count:MAX_BUILDINGS);++k) {
        int i=lists?lists->optical_buildings[k]:k;
#ifdef SC64_P7_DETECTOR_SNAPSHOT_FAST
        if (!lists && !b[i].alive) continue;
#else
        if (!b[i].alive) continue;
        int source=sc64_p7_detector_current_slot(b[i].owner,RACE_PROTOSS_SLOT(&b[i]));
#endif
#ifdef SC64_P7_SIGHT_AXIS_CULL
        int dx=cx-b[i].x,dy=cy-b[i].y;
        if(dx < -SC64_P7_SIGHT_BUILDING_AXIS_BOUND || dx > SC64_P7_SIGHT_BUILDING_AXIS_BOUND ||
            dy < -SC64_P7_SIGHT_BUILDING_AXIS_BOUND || dy > SC64_P7_SIGHT_BUILDING_AXIS_BOUND) continue;
#else
        int dx=cx-b[i].x,dy=cy-b[i].y;
#endif
        unsigned candidate=lists?lists->optical_building_masks[k]:0;
        if(!lists) {
#ifdef SC64_P7_DETECTOR_SNAPSHOT_FAST
            int source=sc64_p7_detector_current_slot(b[i].owner,RACE_PROTOSS_SLOT(&b[i]));
#endif
            if(source>=0&&source<8) {
                int live=race_protoss_source_owner((unsigned)source);
                if(live>=0&&live==b[i].owner) candidate=1u<<(unsigned)source;
            } else for(unsigned owner=0;owner<8;++owner)
                if((int)owner!=human_slot && race_protoss_source_owner(owner)==b[i].owner) candidate|=1u<<owner;
        }
#ifdef SC64_SOURCE_SIGHT_ALL_RACES
        if (human_slot>=0) candidate &= ~(1u << (unsigned)human_slot); /* SIDE grid is authoritative. */
#endif
        if(!(candidate & ~visible_owners)) continue;
        int radius=sc64_p7_observer_sight_radius_runtime(
#ifdef SC64_SOURCE_SIGHT_ALL_RACES
            sc64_shared_building_visibility_key(&b[i]),
#else
            b[i].type,
#endif
            1);
        if(dx*dx+dy*dy>radius*radius) continue;
        visible_owners|=candidate;
    }
    for(int k=0;k<(lists?(int)lists->unit_count:MAX_UNITS);++k) {
        int i=lists?lists->units[k]:k;
        if(!(u[i].alive &&
#ifdef SC64_P7_CARRIER_TRAINING_ENABLED
        !SC64_CARRIER_HANGAR_HIDDEN(&u[i]) &&
#endif
        !SC64_TRANSPORT_LOADED(&u[i]) &&
        RACE_PROTOSS_RAW(&u[i])==SC64_P7_OBSERVER_RAW)) continue;
        int source=sc64_p7_detector_current_slot(u[i].owner,RACE_PROTOSS_SLOT(&u[i]));
#ifdef SC64_P7_GEOMETRY_RECT_HOIST_ENABLED
        int ol,ot,or_,ob;
        int distance=-1;
        if(sc64_p7_actor_rect_source(SC64_P7_OBSERVER_RAW,u[i].x,u[i].y,0,
                                     &ol,&ot,&or_,&ob))
            distance=sc64_p7_observer_rect_distance_rects(
                ol,ot,or_,ob,target_l,target_t,target_r,target_b);
#else
        int distance=sc64_p7_observer_rect_distance(SC64_P7_OBSERVER_RAW,
            u[i].x,u[i].y,0,(unsigned)RACE_PROTOSS_RAW(&u[target]),u[target].x,u[target].y,0);
#endif
        if(distance<0||distance>SC64_P7_OBSERVER_BASE_SIGHT_SOURCE_PX) continue;
        if(source>=0&&source<8 && race_protoss_source_owner((unsigned)source)==u[i].owner &&
           (visible_owners&(1u<<(unsigned)source))) detected|=1u<<(unsigned)source;
        else if(source<0||source>=8) for(unsigned owner=0;owner<8;++owner)
            if((int)owner!=human_slot && race_protoss_source_owner(owner)==u[i].owner &&
               (visible_owners&(1u<<owner))) detected|=1u<<owner;
    }
    for(int k=0;k<(lists?(int)lists->building_count:MAX_BUILDINGS);++k) {
        int i=lists?lists->buildings[k]:k;
        if(!(b[i].alive && RACE_PROTOSS_RAW(&b[i])==SC64_P7_CANNON_RAW &&
             building_complete(i) && powered_cannons[i])) continue;
        int source=sc64_p7_detector_current_slot(b[i].owner,RACE_PROTOSS_SLOT(&b[i]));
#ifdef SC64_P7_GEOMETRY_RECT_HOIST_ENABLED
        int cl,ct,cr,cb;
        int distance=-1;
        if(sc64_p7_actor_rect_source(SC64_P7_CANNON_RAW,b[i].x,b[i].y,1,
                                     &cl,&ct,&cr,&cb))
            distance=sc64_p7_observer_rect_distance_rects(
                cl,ct,cr,cb,target_l,target_t,target_r,target_b);
#else
        int distance=sc64_p7_observer_rect_distance(SC64_P7_CANNON_RAW,
            b[i].x,b[i].y,1,(unsigned)RACE_PROTOSS_RAW(&u[target]),u[target].x,u[target].y,0);
#endif
        if(distance<0||distance>SC64_P7_CANNON_DETECTION_SOURCE_PX) continue;
        if(source>=0&&source<8 && race_protoss_source_owner((unsigned)source)==b[i].owner &&
           (visible_owners&(1u<<(unsigned)source))) detected|=1u<<(unsigned)source;
        else if(source<0||source>=8) for(unsigned owner=0;owner<8;++owner)
            if((int)owner!=human_slot && race_protoss_source_owner(owner)==b[i].owner &&
               (visible_owners&(1u<<owner))) detected|=1u<<owner;
    }
#ifdef SC64_P7_GEOMETRY_RECT_HOIST_ENABLED
    /* Zerg detectors. As for the Observer, a mobile detector detects at its
     * sight range (Overlord, 11 tiles, no Antennae); a completed structure
     * at 7 tiles (Spore Colony, as the Photon Cannon). */
    for(int k=0;k<(lists?(int)lists->unit_count:MAX_UNITS);++k) {
        int i=lists?lists->units[k]:k;
        if(!(u[i].alive && !SC64_TRANSPORT_LOADED(&u[i]) &&
             RACE_PROTOSS_RAW(&u[i])==SC64_OVERLORD_RAW)) continue;
        int source=sc64_p7_detector_current_slot(u[i].owner,RACE_PROTOSS_SLOT(&u[i]));
        int ol,ot,or_,ob,distance=-1;
        if(sc64_p7_actor_rect_source(SC64_OVERLORD_RAW,u[i].x,u[i].y,0,
                                     &ol,&ot,&or_,&ob))
            distance=sc64_p7_observer_rect_distance_rects(
                ol,ot,or_,ob,target_l,target_t,target_r,target_b);
        if(distance<0||distance>SC64_OVERLORD_DETECTION_SOURCE_PX) continue;
        if(source>=0&&source<8 && race_protoss_source_owner((unsigned)source)==u[i].owner &&
           (visible_owners&(1u<<(unsigned)source))) detected|=1u<<(unsigned)source;
    }
    for(int k=0;k<(lists?(int)lists->building_count:MAX_BUILDINGS);++k) {
        int i=lists?lists->buildings[k]:k;
        if(!(b[i].alive && RACE_PROTOSS_RAW(&b[i])==SC64_SPORE_COLONY_RAW &&
             building_complete(i))) continue;
        int source=sc64_p7_detector_current_slot(b[i].owner,RACE_PROTOSS_SLOT(&b[i]));
        int sl,st,sr,sb,distance=-1;
        if(sc64_p7_actor_rect_source(SC64_SPORE_COLONY_RAW,b[i].x,b[i].y,1,
                                     &sl,&st,&sr,&sb))
            distance=sc64_p7_observer_rect_distance_rects(
                sl,st,sr,sb,target_l,target_t,target_r,target_b);
        if(distance<0||distance>SC64_SPORE_DETECTION_SOURCE_PX) continue;
        if(source>=0&&source<8 && race_protoss_source_owner((unsigned)source)==b[i].owner &&
           (visible_owners&(1u<<(unsigned)source))) detected|=1u<<(unsigned)source;
    }
#endif
    return detected&255u;
}


static void oracle_arbiter_cache_source_tick(void)
{
    unsigned i,j;
    u8 powered_cannons[MAX_BUILDINGS];
    int powered_cannons_ready=0;
    OracleDetectorLists detector_lists;
    unsigned global=(unsigned)RACE_PROTOSS_SERVICE_CLOCK[1] |
                    ((unsigned)RACE_PROTOSS_SERVICE_CLOCK[2]<<8);
    int stagger_reset=0;
    unsigned visible_ordinal=0;
    /* OpenBW decrements the global secondary-order timer and assigns timer
       offsets to every visible unit before movement, sprite detection, and
       unit orders. The GBA approximation seeds the source's 150-frame first
       interval, then applies each 300-source-frame reset before target work. */
    if (!global && !sc64_p7_arbiter_cache_initialized) global=150;
    if (global) --global;
    if (!global) { global=300; stagger_reset=1; }
    if (stagger_reset) for (i=0;i<MAX_UNITS;++i)
        if (u[i].alive && RACE_PROTOSS_RAW(&u[i])!=101u &&
            !SC64_TRANSPORT_LOADED(&u[i])) {
            unsigned old=(unsigned)u[i].weapon;
            if (!SC64_P7_ARBITER_CACHE_VALID(old)) old=sc64_p7_arbiter_pack(0,0,0,0);
            u[i].weapon=(u16)sc64_p7_arbiter_pack(
                SC64_P7_ARBITER_DETECTED_MASK(old),visible_ordinal%30u,
                (old&SC64_P7_ARBITER_AURA)!=0,
                (old&SC64_P7_ARBITER_TRACKED)!=0);
            ++visible_ordinal;
        }
    u8 aura[MAX_UNITS];
    for (i=0;i<MAX_UNITS;++i) aura[i]=0;
    for (i=0;i<MAX_UNITS;++i) if (u[i].alive && RACE_PROTOSS_RAW(&u[i])==71u &&
        RACE_PROTOSS_SLOT(&u[i])>=0 && RACE_PROTOSS_SLOT(&u[i])<8 && !(u[i].flags&8u) &&
        !SC64_TRANSPORT_LOADED(&u[i]) &&
        race_protoss_source_owner((unsigned)RACE_PROTOSS_SLOT(&u[i]))==u[i].owner) {
#ifdef SC64_AURA_SOURCE_HOIST_PROBE
        int aura_source=RACE_PROTOSS_SLOT(&u[i]), aura_owner=u[i].owner;
#endif
#ifdef SC64_P7_GEOMETRY_RECT_HOIST_ENABLED
        int al,at,ar,ab;
        if(!sc64_p7_actor_rect_source(71u,u[i].x,u[i].y,0,
                                      &al,&at,&ar,&ab)) continue;
#ifdef SC64_AURA_SQUARE_HOIST_PROBE
        Sc64AuraSquare square=sc64_aura_square_make(u[i].x,u[i].y,
                                  SC64_P7_MAP_COLS*32,SC64_P7_MAP_ROWS*32);
#endif
#endif
        if (sc64_target_index_active) {
            /* Target index cells are 64 GBA pixels. 160 source pixels is
             * 80 GBA pixels; 32 additional GBA pixels cover the maximum
             * authenticated P7 unit half-extent before exact broadphase. */
            int x0=(u[i].x-112)/64, x1=(u[i].x+112)/64;
            int y0=(u[i].y-112)/64, y1=(u[i].y+112)/64;
            int cols=(WORLD_W+63)/64, rows=(WORLD_H+63)/64;
            if (x0<0)x0=0; if(y0<0)y0=0;
            if(x1>=cols)x1=cols-1; if(y1>=rows)y1=rows-1;
            /* Every aura target must have the Arbiter's compact owner. An
             * active index was built from validated owner lists; retain the
             * full traversal for malformed owner values. The raw-source
             * checks below still distinguish collapsed source identities. */
            int owner_first = (unsigned)u[i].owner < 5u ? u[i].owner : 0;
            int owner_end = (unsigned)u[i].owner < 5u ? u[i].owner + 1 : 5;
            for (int cy=y0;cy<=y1;++cy) for(int cx=x0;cx<=x1;++cx)
                for(int owner=owner_first;owner<owner_end;++owner) {
                    int cell=cx+cy*SC64_TARGET_GRID_COLS;
                    Sc64TargetIndex ent=sc64_target_head[owner][cell];
                    while(ent!=SC64_TARGET_NONE) {
                        int slot=(int)ent;
                        if(slot<MAX_UNITS && slot!=(int)i &&
#ifdef SC64_P7_ARBITER_AURA_ONCE
                           !aura[slot] &&
#endif
                           sc64_p7_observer_arbiter_aura_candidate(slot) &&
#ifdef SC64_AURA_SOURCE_HOIST_PROBE
                           sc64_p7_arbiter_same_validated_source(slot,aura_source,aura_owner) &&
#else
                           sc64_p7_arbiter_same_source((int)i,slot) &&
#endif
#ifdef SC64_P7_GEOMETRY_RECT_HOIST_ENABLED
                           1) {
                            int tl,tt,tr,tb;
                            if(sc64_p7_actor_rect_source((unsigned)RACE_PROTOSS_RAW(&u[slot]),
                                    u[slot].x,u[slot].y,0,&tl,&tt,&tr,&tb) &&
#ifdef SC64_AURA_SQUARE_HOIST_PROBE
                               sc64_aura_square_contains(&square,tl,tt,tr,tb)
#else
                               sc64_p7_arbiter_finder_square_rect((int)i,tl,tt,tr,tb)
#endif
                               ) {
                                int d=sc64_p7_observer_rect_distance_rects(
                                    al,at,ar,ab,tl,tt,tr,tb);
                                if(d>=0&&d<=160)aura[slot]=1;
                            }
                        }
#else
                           sc64_p7_arbiter_finder_square((int)i,slot)) {
                            int d=sc64_p7_observer_rect_distance(71u,u[i].x,u[i].y,0,
                                (unsigned)RACE_PROTOSS_RAW(&u[slot]),u[slot].x,u[slot].y,0);
                            if(d>=0&&d<=160)aura[slot]=1;
                        }
#endif
                        ent=sc64_target_next[slot];
                    }
                }
        } else {
            for (j=0;j<MAX_UNITS;++j) if (j!=i &&
#ifdef SC64_P7_ARBITER_AURA_ONCE
                !aura[j] &&
#endif
                sc64_p7_observer_arbiter_aura_candidate((int)j) &&
#ifdef SC64_AURA_SOURCE_HOIST_PROBE
                sc64_p7_arbiter_same_validated_source((int)j,aura_source,aura_owner) &&
#else
                sc64_p7_arbiter_same_source((int)i,(int)j) &&
#endif
#ifdef SC64_P7_GEOMETRY_RECT_HOIST_ENABLED
                1) {
                int tl,tt,tr,tb;
                if(sc64_p7_actor_rect_source((unsigned)RACE_PROTOSS_RAW(&u[j]),
                        u[j].x,u[j].y,0,&tl,&tt,&tr,&tb) &&
#ifdef SC64_AURA_SQUARE_HOIST_PROBE
                   sc64_aura_square_contains(&square,tl,tt,tr,tb)
#else
                   sc64_p7_arbiter_finder_square_rect((int)i,tl,tt,tr,tb)
#endif
                               ) {
                    int d=sc64_p7_observer_rect_distance_rects(
                        al,at,ar,ab,tl,tt,tr,tb);
                    if(d>=0&&d<=160)aura[j]=1;
                }
            }
#else
                sc64_p7_arbiter_finder_square((int)i,(int)j)) {
                int d=sc64_p7_observer_rect_distance(71u,u[i].x,u[i].y,0,
                    (unsigned)RACE_PROTOSS_RAW(&u[j]),u[j].x,u[j].y,0);
                if (d>=0 && d<=160) aura[j]=1;
            }
#endif
        }
    }
    for (i=0;i<MAX_UNITS;++i) {
        unsigned old=(unsigned)u[i].weapon;
        unsigned mask=0,timer=0;
        if (!u[i].alive) {
            /* The old value is already the exact dead-row result. Zero rows
             * still need the complete representation check below. */
            if (old != sc64_p7_arbiter_pack(0,0,0,0) &&
                !sc64_p7_arbiter_cache_empty_slot(i))
                u[i].weapon=(u16)sc64_p7_arbiter_pack(0,0,0,0);
            continue;
        }
        int tracked=aura[i] || sc64_p7_observer_is_cloaked((unsigned)RACE_PROTOSS_RAW(&u[i]));
        int was_tracked=SC64_P7_ARBITER_CACHE_VALID(old) &&
                        (old&SC64_P7_ARBITER_TRACKED)!=0;
        if (!SC64_P7_ARBITER_CACHE_VALID(old)) {
            /* Fail closed on malformed in-memory rows; spawn initializes it. */
            u[i].weapon=(u16)sc64_p7_arbiter_pack(0,0,0,0);
            old=(unsigned)u[i].weapon;
        }
        timer=SC64_P7_ARBITER_COUNTDOWN(old);
        if (tracked) {
            if (timer==0 || (tracked && !was_tracked)) {
                /* Building and owner state are unchanged within this tick.
                 * Compute the shared snapshot only if a refresh needs it. */
                if (!powered_cannons_ready) {
                    memset(powered_cannons,0,sizeof(powered_cannons));
                    for(j=0;j<MAX_BUILDINGS;++j) if(b[j].alive &&
                        RACE_PROTOSS_RAW(&b[j])==SC64_P7_CANNON_RAW && building_complete((int)j)) {
                        int source=sc64_p7_detector_current_slot(b[j].owner,RACE_PROTOSS_SLOT(&b[j]));
                        if(source>=0 && source<8) {
                            int live=race_protoss_source_owner((unsigned)source);
                            if(live==b[j].owner)
                                powered_cannons[j]=(u8)sc64_p7_observer_pylon_powers((int)j,source,live);
                        }
                    }
                    oracle_detector_lists_build(&detector_lists);
                    powered_cannons_ready=1;
                }
                mask=oracle_detected_mask_lists((int)i,powered_cannons,
                    &detector_lists);
                timer=30;
            } else {
                mask=SC64_P7_ARBITER_DETECTED_MASK(old);
                --timer;
            }
        } else {
            /* Native reset assigns a countdown even to currently uncloaked
               visible units; retain it in case cloak begins before next reset. */
            mask=0;
        }
        u[i].weapon=(u16)sc64_p7_arbiter_pack(mask,timer,(int)aura[i],tracked);
    }
    RACE_PROTOSS_SERVICE_CLOCK[1]=(u8)global;
    RACE_PROTOSS_SERVICE_CLOCK[2]=(u8)(global>>8);
}


static int oracle_cache_unit_valid_with_mask(const Unit *actor,
                                                      unsigned allowed_sources)
{
    unsigned w,mask;
    if (!actor) return 0;
    if (sc64_p7_arbiter_cache_empty_unit(actor)) return 1;
    w=(unsigned)actor->weapon;
    if (!SC64_P7_ARBITER_CACHE_VALID(w)) return 0;
    mask=SC64_P7_ARBITER_DETECTED_MASK(w);
    if (mask & ~allowed_sources) return 0;
    if (actor->alive && !(w&SC64_P7_ARBITER_TRACKED) && mask) return 0;
    if (actor->alive && (w&SC64_P7_ARBITER_AURA) &&
        !(w&SC64_P7_ARBITER_TRACKED)) return 0;
    if (actor->alive && (w&SC64_P7_ARBITER_TRACKED) &&
        !(w&SC64_P7_ARBITER_AURA) &&
        !sc64_p7_observer_is_cloaked((unsigned)RACE_PROTOSS_RAW(actor))) return 0;
    return 1;
}


static int oracle_cache_world_valid(void)
{
    unsigned allowed_sources=sc64_p7_valid_source_mask();
    for (int i=0;i<MAX_UNITS;++i)
        if (!oracle_cache_unit_valid_with_mask(&u[i],
                                                        allowed_sources)) return 0;
    return 1;
}

#endif
