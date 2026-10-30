/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Compare full-pool and already-validated owner-list index construction. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32;
#define GBA_HOT
#define MAX_UNITS 80
#define MAX_BUILDINGS 40
#define WORLD_W 1024
#define WORLD_H 1024
#define SC64_TARGET_OCCUPANCY
#define SC64_TRANSPORT_LOADED(q) ((q)->loaded)
typedef struct { int alive, owner, x, y, loaded; } Unit;
typedef struct { int alive, owner, x, y; } Building;
static Unit u[MAX_UNITS]; static Building b[MAX_BUILDINGS];
static int target_unit_head[5], target_building_head[5];
static int target_unit_next[MAX_UNITS], target_building_next[MAX_BUILDINGS];
#include "../starcraft/sc64_targets.h"
static uint32_t rng=0x826341u;
static uint32_t rnd(void) { rng=rng*1664525u+1013904223u; return rng; }
static void lists(void) {
    for(int p=0;p<5;p++) target_unit_head[p]=target_building_head[p]=-1;
    for(int i=MAX_UNITS-1;i>=0;i--) if(u[i].alive && !u[i].loaded) {
        int p=u[i].owner; assert(p>=0 && p<5); target_unit_next[i]=target_unit_head[p]; target_unit_head[p]=i;
    }
    for(int i=MAX_BUILDINGS-1;i>=0;i--) if(b[i].alive) {
        int p=b[i].owner; assert(p>=0 && p<5); target_building_next[i]=target_building_head[p]; target_building_head[p]=i;
    }
}
static uint32_t snapshot(void) {
    uint32_t h=2166136261u;
    for(int p=0;p<5;p++) for(int c=0;c<SC64_TARGET_CELL_COUNT;c++) {
        int guard=0; Sc64TargetIndex at=sc64_target_head[p][c];
        while(at!=SC64_TARGET_NONE) { assert(at<SC64_TARGET_ENTITY_COUNT && ++guard<=SC64_TARGET_ENTITY_COUNT); h=(h^at)*16777619u; at=sc64_target_next[at]; }
        h=(h^0xffffu)*16777619u;
    }
    for(int p=0;p<5;p++) {h=(h^sc64_target_owner_columns[p])*16777619u;h=(h^sc64_target_owner_rows[p])*16777619u;}
    return h;
}
int main(void) {
    for(int round=0;round<5000;round++) {
        for(int i=0;i<MAX_UNITS;i++) {u[i].alive=(rnd()%4)!=0;u[i].owner=rnd()%5;u[i].x=rnd()%WORLD_W;u[i].y=rnd()%WORLD_H;u[i].loaded=(rnd()%8)==0;}
        for(int i=0;i<MAX_BUILDINGS;i++) {b[i].alive=(rnd()%3)!=0;b[i].owner=rnd()%5;b[i].x=rnd()%WORLD_W;b[i].y=rnd()%WORLD_H;}
        /* Dense ties and mixed owners exercise insertion order. */
        if(round%2==0) for(int i=0;i<12;i++){u[i].x=u[i].y=256;b[i].x=b[i].y=256;}
        lists(); sc64_target_index_begin(); assert(sc64_target_index_active); uint32_t expected=snapshot();
        sc64_target_index_begin_owner_lists(); assert(sc64_target_index_active); assert(snapshot()==expected);
        /* Move, remove/reinsert, and overflow state must remain usable next frame. */
        for(int i=0;i<MAX_UNITS;i++) if(u[i].alive && !u[i].loaded) {u[i].x=(u[i].x+65)%WORLD_W;sc64_target_index_move(&u[i]);}
        sc64_target_dirty_overflowed=1;
        lists(); sc64_target_index_begin(); assert(sc64_target_index_active); expected=snapshot();
        sc64_target_index_begin_owner_lists(); assert(sc64_target_index_active); assert(snapshot()==expected);
    }
    memset(u, 0, sizeof(u)); memset(b, 0, sizeof(b));
    u[0].alive=1; u[0].owner=0; u[0].x=-1; u[0].y=0;
    lists(); sc64_target_index_begin(); assert(!sc64_target_index_active);
    sc64_target_index_begin_owner_lists(); assert(!sc64_target_index_active);
    u[0].x=0; lists(); sc64_target_index_begin(); assert(sc64_target_index_active);
    uint32_t recovered=snapshot(); sc64_target_index_begin_owner_lists();
    assert(sc64_target_index_active && snapshot()==recovered);
    sc64_target_index_end();
    puts("owner-list index differential: 10000 randomized builds passed");
    return 0;
}
