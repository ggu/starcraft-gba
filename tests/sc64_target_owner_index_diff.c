/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Compare full-pool and already-validated owner-list index construction. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32;
#define GBA_HOST 1
#define GBA_HOT
#define MAX_UNITS 80
#define MAX_BUILDINGS 40
static int world_w=1024, world_h=1024;
#define WORLD_W world_w
#define WORLD_H world_h
#define SC64_RACES 1
#define SC64_SCARAB_RUNTIME 1
#define U_SCARAB 1
#define SC64_TRANSPORT_LOADED(q) ((q)->loaded)
typedef struct { int alive, owner, x, y, loaded, type, carry; } Unit;
typedef struct { int alive, owner, x, y; } Building;
static Unit u[MAX_UNITS]; static Building b[MAX_BUILDINGS];
static int target_unit_head[5], target_building_head[5];
static int target_unit_next[MAX_UNITS], target_building_next[MAX_BUILDINGS];
#include "../starcraft/sc64_targets.h"
static uint32_t rng=0x826341u;
static uint32_t rnd(void) {
    rng^=rng<<13; rng^=rng>>17; rng^=rng<<5;
    return rng;
}
static void lists(void) {
    for(int p=0;p<5;p++) target_unit_head[p]=target_building_head[p]=-1;
    for(int i=MAX_UNITS-1;i>=0;i--) if(u[i].alive && !u[i].loaded && !SC64_SCARAB_HIDDEN(&u[i])) {
        int p=u[i].owner; assert(p>=0 && p<5); target_unit_next[i]=target_unit_head[p]; target_unit_head[p]=i;
    }
    for(int i=MAX_BUILDINGS-1;i>=0;i--) if(b[i].alive) {
        int p=b[i].owner; assert(p>=0 && p<5); target_building_next[i]=target_building_head[p]; target_building_head[p]=i;
    }
}
static Sc64TargetIndex expected_heads[5][SC64_TARGET_CELL_COUNT];
static Sc64TargetIndex expected_next[SC64_TARGET_ENTITY_COUNT];
static Sc64TargetIndex expected_prev[SC64_TARGET_ENTITY_COUNT];
static u16 expected_cell[SC64_TARGET_ENTITY_COUNT];
static u16 expected_dirty[SC64_TARGET_DIRTY_CAPACITY], expected_dirty_count;
static int expected_active, expected_overflow;
#ifdef SC64_TARGET_OCCUPANCY
static u32 expected_columns[5], expected_rows[5];
#endif
static void snapshot(void) {
    memcpy(expected_heads, sc64_target_head, sizeof(expected_heads));
    memcpy(expected_next, sc64_target_next, sizeof(expected_next));
    memcpy(expected_prev, sc64_target_prev, sizeof(expected_prev));
    memcpy(expected_cell, sc64_target_cell, sizeof(expected_cell));
    expected_active=sc64_target_index_active;
    expected_overflow=sc64_target_dirty_overflowed;
    expected_dirty_count=sc64_target_dirty_count;
    memcpy(expected_dirty,sc64_target_dirty,expected_dirty_count*sizeof(*expected_dirty));
#ifdef SC64_TARGET_OCCUPANCY
    memcpy(expected_columns, sc64_target_owner_columns, sizeof(expected_columns));
    memcpy(expected_rows, sc64_target_owner_rows, sizeof(expected_rows));
#endif
}
static void compare_snapshot(int exact_dirty_order) {
    assert(memcmp(expected_heads, sc64_target_head, sizeof(expected_heads)) == 0);
    assert(expected_active==sc64_target_index_active);
    assert(expected_overflow==sc64_target_dirty_overflowed);
    assert(expected_dirty_count==sc64_target_dirty_count);
    if(exact_dirty_order)
        assert(memcmp(expected_dirty,sc64_target_dirty,expected_dirty_count*sizeof(*expected_dirty))==0);
#ifdef SC64_TARGET_OCCUPANCY
    assert(memcmp(expected_columns, sc64_target_owner_columns, sizeof(expected_columns)) == 0);
    assert(memcmp(expected_rows, sc64_target_owner_rows, sizeof(expected_rows)) == 0);
#endif
    for(int p=0;p<5;p++) for(int c=0;c<SC64_TARGET_CELL_COUNT;c++) {
        int guard=0; Sc64TargetIndex at=sc64_target_head[p][c];
        while(at!=SC64_TARGET_NONE) {
            assert(at<SC64_TARGET_ENTITY_COUNT && ++guard<=SC64_TARGET_ENTITY_COUNT);
            assert(expected_next[at]==sc64_target_next[at]);
            assert(expected_prev[at]==sc64_target_prev[at]);
            assert(expected_cell[at]==sc64_target_cell[at]);
            at=sc64_target_next[at];
        }
    }
}

/* The checked per-entry owner-list path is the reference for dirty order and
 * partial rebuilds. A full pool scan can visit owners in a different order. */
static void compare_checked_rebuild(void) {
    lists();
    sc64_target_index_clear();
    for(int p=0;p<5;p++) {
        for(int i=target_unit_head[p];i>=0;i=target_unit_next[i])
            if(!sc64_target_insert(i,p,u[i].x,u[i].y)) goto compare;
        for(int i=target_building_head[p];i>=0;i=target_building_next[i])
            if(!sc64_target_insert(MAX_UNITS+i,p,b[i].x,b[i].y)) goto compare;
    }
    sc64_target_index_active=1;
compare:
    snapshot();
    sc64_target_index_begin_owner_lists();
    compare_snapshot(1);
}

static void check_geometry_and_membership(void) {
    memset(u,0,sizeof(u)); memset(b,0,sizeof(b));
    world_w=127; world_h=65;
    u[0]=(Unit){.alive=1,.owner=0,.x=126,.y=64};
    u[1]=(Unit){.alive=1,.owner=0,.x=127,.y=64,.loaded=1};
    u[2]=(Unit){.alive=1,.owner=1,.x=-1,.y=-1,.type=U_SCARAB,.carry=256};
    u[3]=(Unit){.alive=1,.owner=1,.x=126,.y=64,.type=U_SCARAB,.carry=512};
    b[0]=(Building){.alive=1,.owner=0,.x=126,.y=64};
    compare_checked_rebuild();
    int cell=SC64_TARGET_GRID_COLS+1;
    assert(sc64_target_index_active && sc64_target_dirty_count==2);
    assert(sc64_target_head[0][cell]==MAX_UNITS && sc64_target_next[MAX_UNITS]==0);
    assert(sc64_target_head[1][cell]==3 && sc64_target_next[3]==SC64_TARGET_NONE);
    b[0].x=127;
    compare_checked_rebuild();
    assert(!sc64_target_index_active && sc64_target_dirty_count==1);
    assert(sc64_target_head[0][cell]==0 && sc64_target_head[1][cell]==SC64_TARGET_NONE);
    b[0].x=126; b[0].y=65;
    compare_checked_rebuild();
    assert(!sc64_target_index_active && sc64_target_dirty_count==1);
    b[0].y=-1;
    compare_checked_rebuild();
    assert(!sc64_target_index_active && sc64_target_dirty_count==1);

    world_w=SC64_TARGET_GRID_COLS*SC64_TARGET_CELL_SIZE;
    world_h=SC64_TARGET_GRID_ROWS*SC64_TARGET_CELL_SIZE;
    b[0].x=world_w-1; b[0].y=world_h-1;
    compare_checked_rebuild();
    assert(sc64_target_index_active && sc64_target_head[0][SC64_TARGET_CELL_COUNT-1]==MAX_UNITS);
    ++world_h;
    compare_checked_rebuild();
    assert(!sc64_target_index_active && sc64_target_dirty_count==0);
    --world_h; ++world_w;
    compare_checked_rebuild();
    assert(!sc64_target_index_active && sc64_target_dirty_count==0);
    memset(u,0,sizeof(u)); memset(b,0,sizeof(b));
    compare_checked_rebuild();
    assert(sc64_target_index_active && sc64_target_dirty_count==0);
    world_w=0; world_h=64;
    compare_checked_rebuild();
    assert(sc64_target_index_active && sc64_target_dirty_count==0);
    b[0]=(Building){.alive=1,.owner=0,.x=0,.y=0};
    compare_checked_rebuild();
    assert(!sc64_target_index_active && sc64_target_dirty_count==0);
    world_w=64; world_h=-16;
    compare_checked_rebuild();
    assert(!sc64_target_index_active && sc64_target_dirty_count==0);
    world_w=world_h=1024;
}

static void check_move_and_overflow(void) {
    memset(u,0,sizeof(u)); memset(b,0,sizeof(b));
    for(int i=0;i<3;i++) u[i]=(Unit){.alive=1,.owner=2,.x=10+i,.y=10};
    b[0]=(Building){.alive=1,.owner=2,.x=10,.y=10};
    compare_checked_rebuild();
    assert(sc64_target_index_active && sc64_target_head[2][0]==MAX_UNITS);
    assert(sc64_target_next[MAX_UNITS]==2 && sc64_target_next[2]==1 && sc64_target_next[1]==0);
    assert(sc64_target_dirty_count==1);
    u[0].x=63; sc64_target_index_move(&u[0]);
    assert(sc64_target_head[2][0]==MAX_UNITS && sc64_target_next[1]==0 && sc64_target_dirty_count==1);
    u[0].x=64; sc64_target_index_move(&u[0]);
    assert(sc64_target_head[2][1]==0 && sc64_target_next[1]==SC64_TARGET_NONE && sc64_target_dirty_count==2);
    u[0].x=63; sc64_target_index_move(&u[0]);
    assert(sc64_target_head[2][0]==0 && sc64_target_prev[MAX_UNITS]==0 && sc64_target_dirty_count==2);
    assert(sc64_target_head[2][1]==SC64_TARGET_NONE);
#ifdef SC64_TARGET_OCCUPANCY
    assert(sc64_target_owner_columns[2]==3u);
#endif
    u[0].x=64; sc64_target_index_move(&u[0]);
    assert(sc64_target_head[2][1]==0 && sc64_target_dirty_count==3);
    assert(sc64_target_dirty[1]==sc64_target_dirty[2]);
    sc64_target_dirty_count=SC64_TARGET_DIRTY_CAPACITY;
    u[0].x=128; sc64_target_index_move(&u[0]);
    assert(!sc64_target_index_active && sc64_target_dirty_overflowed);
    lists(); sc64_target_index_begin_owner_lists();
    assert(sc64_target_index_active && !sc64_target_dirty_overflowed);
    assert(sc64_target_head[2][1]==SC64_TARGET_NONE && sc64_target_head[2][2]==0 && sc64_target_dirty_count==2);
#ifdef SC64_TARGET_OCCUPANCY
    assert(sc64_target_owner_columns[2]==5u);
#endif
    compare_checked_rebuild();
}

int main(void) {
    for(int round=0;round<5000;round++) {
        for(int i=0;i<MAX_UNITS;i++) {
            u[i].alive=(rnd()%4)!=0; u[i].owner=rnd()%5;
            u[i].x=rnd()%WORLD_W; u[i].y=rnd()%WORLD_H; u[i].loaded=(rnd()%8)==0;
            u[i].type=(rnd()%16)==0?U_SCARAB:0; u[i].carry=rnd()%1024;
        }
        for(int i=0;i<MAX_BUILDINGS;i++) {b[i].alive=(rnd()%3)!=0;b[i].owner=rnd()%5;b[i].x=rnd()%WORLD_W;b[i].y=rnd()%WORLD_H;}
        /* Dense ties and mixed owners exercise insertion order. */
        if(round%2==0) for(int i=0;i<12;i++){u[i].x=u[i].y=256;b[i].x=b[i].y=256;}
        lists(); sc64_target_index_begin(); assert(sc64_target_index_active); snapshot();
        sc64_target_index_begin_owner_lists(); assert(sc64_target_index_active); compare_snapshot(0);
        /* Move, remove/reinsert, and overflow state must remain usable next frame. */
        for(int i=0;i<MAX_UNITS;i++) if(u[i].alive && !u[i].loaded && !SC64_SCARAB_HIDDEN(&u[i])) {
            u[i].x=(u[i].x+65)%WORLD_W;
            sc64_target_index_move(&u[i]);
        }
        sc64_target_dirty_overflowed=1;
        lists(); sc64_target_index_begin(); assert(sc64_target_index_active); snapshot();
        sc64_target_index_begin_owner_lists(); assert(sc64_target_index_active); compare_snapshot(0);
    }
    check_geometry_and_membership();
    check_move_and_overflow();
    sc64_target_index_end();
    puts("owner-list index differential: 10000 randomized builds, geometry, crossings and overflow passed");
    return 0;
}
