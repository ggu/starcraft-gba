/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"
/* Independent per-source reference. Unlike the optimized grouped visibility
 * pass, this checks each source slot separately and scans actors/buildings
 * linearly. It models the current generic/P7/P8 owner bindings and raw-ID
 * fallback, and includes source-supported Zerg detectors. */
static int reference_binding(void)
{
    if (race_protoss_binding_override >= 0) return race_protoss_binding_override;
    if (race_mode) return SC64_PROTOSS_BIND_GENERIC;
    if (race_selected_mission == SC64_P7_GLOBAL_ID) return SC64_PROTOSS_BIND_P7;
#ifdef SC64_PROTOSS_LATER_BINDINGS
    if (race_selected_mission == SC64_P8_GLOBAL_ID) return SC64_PROTOSS_BIND_LATER;
#endif
    return SC64_PROTOSS_BIND_NONE;
}
static int reference_owner(int binding,unsigned slot)
{
    const Sc64RaceMap *map;
    if (slot>=8) return -1;
    if (binding==SC64_PROTOSS_BIND_GENERIC)
        return slot==SC64_PROTOSS_HUMAN_SLOT ? SIDE : slot>0 && slot<5 ? (int)slot : -1;
    if (binding==SC64_PROTOSS_BIND_P7) {
        if (SC64_P7_GLOBAL_ID>=SC64_RACE_MAP_COUNT || !sc64_p7_active_slots[slot]) return -1;
        map=&sc64_race_maps[SC64_P7_GLOBAL_ID];
        if (map->human_slot!=7 || !map->owner_modes) return -1;
        return slot==7 ? SIDE : map->owner_modes[slot];
    }
#ifdef SC64_PROTOSS_LATER_BINDINGS
    if (binding==SC64_PROTOSS_BIND_LATER) {
        if (race_protoss_binding_override>=0 && race_protoss_override_mission!=SC64_P8_GLOBAL_ID) return -1;
        if (SC64_P8_GLOBAL_ID>=SC64_RACE_MAP_COUNT) return -1;
        map=&sc64_race_maps[SC64_P8_GLOBAL_ID];
        if (map->human_slot!=SC64_P8_HUMAN_SLOT || !map->owner_modes) return -1;
        return slot==SC64_P8_HUMAN_SLOT ? SIDE : map->owner_modes[slot];
    }
#endif
    return -1;
}
static int reference_slot(int binding,int live,int raw)
{
    if (binding!=SC64_PROTOSS_BIND_GENERIC) return raw;
    return live==SIDE ? SC64_PROTOSS_HUMAN_SLOT : live>0 && live<5 ? live : -1;
}
static int reference_matches(int binding,unsigned source,int live,int raw,int unknown_ok)
{
    int slot=reference_slot(binding,live,raw), mapped=reference_owner(binding,source);
    if (mapped<0 || mapped!=live) return 0;
    if (slot>=0 && slot<8) return slot==(int)source;
    return unknown_ok && source<7;
}
static int reference_unit_raw(const Unit *q)
{ return q->raw_type>=0 ? q->raw_type : sc64_race_raw_unit(q->type); }
static int reference_building_raw(const Building *q)
{ return q->raw_type>=0 ? q->raw_type : sc64_race_raw_building(q->type); }

static unsigned sc64_p7_arbiter_detected_mask_reference(int target,
                                                  const u8 powered_cannons[MAX_BUILDINGS])
{
    if (target<0 || target>=MAX_UNITS || !u[target].alive) return 0;
    int x=u[target].x,y=u[target].y,col=x/16,row=y/16;
    if (col<0 || row<0 || col>=MAP_COLS || row>=MAP_ROWS) return 0;
    int target_raw=reference_unit_raw(&u[target]);
#ifdef SC64_P7_GEOMETRY_RECT_HOIST_ENABLED
    int tl,tt,tr,tb;
    if (!sc64_p7_actor_rect_source((unsigned)target_raw,x,y,0,&tl,&tt,&tr,&tb)) return 0;
#endif
    int cx=col*16+8,cy=row*16+8,binding=reference_binding();
    unsigned detected=0;
    for (unsigned source=0;source<8;++source) {
        int seen=(source==7 && SC64_GRID_GET(visible,col+row*MAP_COLS));
        for (int i=0;i<MAX_UNITS && !seen;++i) if (u[i].alive &&
#ifdef SC64_P7_CARRIER_TRAINING_ENABLED
            !SC64_CARRIER_HANGAR_HIDDEN(&u[i]) &&
#endif
            !SC64_TRANSPORT_LOADED(&u[i]) &&
            reference_matches(binding,source,u[i].owner,u[i].raw_owner,1)) {
            int dx=cx-u[i].x,dy=cy-u[i].y,r=sc64_p7_observer_sight_radius_runtime(u[i].type,0);
            if (dx*dx+dy*dy<=r*r) seen=1;
        }
        for (int i=0;i<MAX_BUILDINGS && !seen;++i) if (b[i].alive &&
            reference_matches(binding,source,b[i].owner,b[i].raw_owner,1)) {
            int dx=cx-b[i].x,dy=cy-b[i].y,r=sc64_p7_observer_sight_radius_runtime(b[i].type,1);
            if (dx*dx+dy*dy<=r*r) seen=1;
        }
        if (!seen) continue;
        int found=0;
        for (int i=0;i<MAX_UNITS && !found;++i) if (u[i].alive &&
#ifdef SC64_P7_CARRIER_TRAINING_ENABLED
            !SC64_CARRIER_HANGAR_HIDDEN(&u[i]) &&
#endif
            !SC64_TRANSPORT_LOADED(&u[i]) && reference_unit_raw(&u[i])==SC64_P7_OBSERVER_RAW &&
            reference_matches(binding,source,u[i].owner,u[i].raw_owner,1)) {
            int d=sc64_p7_observer_rect_distance(SC64_P7_OBSERVER_RAW,u[i].x,u[i].y,0,
                                                  (unsigned)target_raw,x,y,0);
            if (d>=0 && d<=SC64_P7_OBSERVER_BASE_SIGHT_SOURCE_PX) found=1;
        }
        for (int i=0;i<MAX_BUILDINGS && !found;++i) if (b[i].alive && building_complete(i) &&
            reference_building_raw(&b[i])==SC64_P7_CANNON_RAW && powered_cannons[i] &&
            reference_matches(binding,source,b[i].owner,b[i].raw_owner,1)) {
            int d=sc64_p7_observer_rect_distance(SC64_P7_CANNON_RAW,b[i].x,b[i].y,1,
                                                  (unsigned)target_raw,x,y,0);
            if (d>=0 && d<=SC64_P7_CANNON_DETECTION_SOURCE_PX) found=1;
        }
#ifdef SC64_P7_GEOMETRY_RECT_HOIST_ENABLED
        for (int i=0;i<MAX_UNITS && !found;++i) if (u[i].alive && !SC64_TRANSPORT_LOADED(&u[i]) &&
            reference_unit_raw(&u[i])==SC64_OVERLORD_RAW &&
            reference_matches(binding,source,u[i].owner,u[i].raw_owner,0)) {
            int d=sc64_p7_observer_rect_distance(SC64_OVERLORD_RAW,u[i].x,u[i].y,0,
                                                  (unsigned)target_raw,x,y,0);
            if (d>=0 && d<=SC64_OVERLORD_DETECTION_SOURCE_PX) found=1;
        }
        for (int i=0;i<MAX_BUILDINGS && !found;++i) if (b[i].alive && building_complete(i) &&
            reference_building_raw(&b[i])==SC64_SPORE_COLONY_RAW &&
            reference_matches(binding,source,b[i].owner,b[i].raw_owner,0)) {
            int d=sc64_p7_observer_rect_distance(SC64_SPORE_COLONY_RAW,b[i].x,b[i].y,1,
                                                  (unsigned)target_raw,x,y,0);
            if (d>=0 && d<=SC64_SPORE_DETECTION_SOURCE_PX) found=1;
        }
#endif
        if (found) detected|=1u<<source;
    }
    return detected;
}

static unsigned seed = 0x18bd0123u;
static unsigned rnd(void) { seed = seed * 1664525u + 1013904223u; return seed; }

static void reset_case(void)
{
    memset(u, 0, sizeof(u));
    memset(b, 0, sizeof(b));
    memset(visible, 0, sizeof(visible));
    u[0].alive = 1;
    u[0].raw_type = 0;
    u[0].type = 0;
    u[0].owner = SIDE;
    u[0].raw_owner = 7;
    u[0].x = 256;
    u[0].y = 256;
}

static void observer(int slot, int source, int owner)
{
    u[slot].alive = 1;
    u[slot].raw_type = SC64_P7_OBSERVER_RAW;
    u[slot].type = U_P7_OBSERVER;
    u[slot].raw_owner = source;
    u[slot].owner = owner;
    u[slot].x = u[0].x;
    u[slot].y = u[0].y;
}

static void expect_mask(unsigned expected, const u8 *powered)
{
    unsigned reference = sc64_p7_arbiter_detected_mask_reference(0, powered);
    if (reference != expected) fprintf(stderr, "expected %u got %u\n", expected, reference);
    assert(reference == expected);
    assert(sc64_p7_arbiter_detected_mask(0, powered) == expected);
}

static void explicit_cases(void)
{
    u8 powered[MAX_BUILDINGS] = {0};
    int enemy = race_p7_source_owner(0);
    assert(enemy >= 0 && race_p7_source_owner(4) == enemy);
    assert(race_p7_source_owner(7) == SIDE);

    reset_case();
    observer(1, 7, SIDE);
    u[0].owner = enemy;
    u[0].raw_owner = 0;
    u[1].type = U_SCV;
    u[1].x = 192; /* Detector reaches the target, but its 56px sight does not. */
    SC64_GRID_SET(visible, (256 / 16) * MAP_COLS + 256 / 16, 1);
    expect_mask(1u << 7, powered);
    SC64_GRID_SET(visible, (256 / 16) * MAP_COLS + 256 / 16, 0);
    expect_mask(0, powered);

    reset_case();
    observer(1, 0, enemy);
    observer(2, 4, enemy);
    observer(3, 0, enemy); /* Duplicate covered owner. */
    expect_mask((1u << 0) | (1u << 4), powered);
    u[2].owner = SIDE; /* A valid raw slot with mismatching live owner. */
    expect_mask(1u << 0, powered);
    u[1].raw_owner = -1; /* Invalid raw slot aliases both live-owner slots. */
    expect_mask((1u << 0) | (1u << 4), powered);

    reset_case();
    observer(1, 0, enemy);
    u[1].order = ORDER_TRANSPORTED;
    expect_mask(0, powered);
#ifdef SC64_P7_CARRIER_TRAINING_ENABLED
    u[1].order = 0;
    u[1].type = U_P7_INTERCEPTOR;
    u[1].flags = 1u << 8; /* A hangar-hidden child cannot supply sight or detection. */
    expect_mask(0, powered);
#else
    puts("SKIP interceptor-specific owner-mask case: profile lacks carrier training");
#endif

    reset_case();
    b[0].alive = 1;
    b[0].raw_type = SC64_P7_CANNON_RAW;
    b[0].raw_owner = 0;
    b[0].owner = enemy;
    b[0].x = u[0].x;
    b[0].y = u[0].y;
    powered[0] = 1;
    expect_mask(1u << 0, powered);
    powered[0] = 0;
    expect_mask(0, powered);
    powered[0] = 1;
    b[0].owner = SIDE;
    expect_mask(0, powered);

#ifdef SC64_P7_GEOMETRY_RECT_HOIST_ENABLED
    /* Later service bindings also include mobile and static Zerg detectors. */
    reset_case();
    u[1].alive=1; u[1].type=U_OVERLORD; u[1].raw_type=SC64_OVERLORD_RAW;
    u[1].owner=enemy; u[1].raw_owner=0; u[1].x=u[0].x; u[1].y=u[0].y;
    expect_mask(1u<<0,powered);
    reset_case();
    b[0].alive=1; b[0].type=B_SPORE_COLONY; b[0].raw_type=SC64_SPORE_COLONY_RAW;
    b[0].owner=enemy; b[0].raw_owner=0; b[0].x=u[0].x; b[0].y=u[0].y;
    expect_mask(1u<<0,powered);
#endif
    /* Generic binding derives slots from live owner, not raw mission owner. */
    race_mode=1;
    reset_case(); observer(1,0,2);
    expect_mask(1u<<2,powered);
    race_mode=0;
    reset_case();
    observer(1, 0, enemy);
    u[0].x = WORLD_W;
    expect_mask(0, powered);
}

int main(void)
{
    memset(&race_p7, 0, sizeof(race_p7));
    race_mode = 0;
    race_selected_mission = SC64_P7_GLOBAL_ID;
    explicit_cases();
    unsigned positive = 0;
    for (int run = 0; run < 2000; ++run) {
#ifdef SC64_PROTOSS_LATER_BINDINGS
        race_mode = (run%5)==1;
        race_selected_mission = (run%5)==2 ? SC64_P8_GLOBAL_ID : SC64_P7_GLOBAL_ID;
        race_protoss_binding_override = (run%5)==3 ? SC64_PROTOSS_BIND_GENERIC :
                                        (run%5)==4 ? SC64_PROTOSS_BIND_LATER : -1;
        race_protoss_override_mission=SC64_P8_GLOBAL_ID;
#else
        race_mode = run & 1;
#endif
        reset_case();
        for (int i = 1; i < MAX_UNITS; ++i) {
            if (rnd() % 5 != 0) continue;
            u[i].alive = 1;
            u[i].owner = rnd() % 5;
            u[i].raw_owner = (int)(rnd() % 12) - 2;
            u[i].raw_type = (rnd() % 3 == 0) ? SC64_P7_OBSERVER_RAW : rnd() % 90;
#ifdef SC64_P7_CARRIER_TRAINING_ENABLED
            u[i].type = (rnd() % 7 == 0) ? U_P7_INTERCEPTOR : rnd() % 10;
#else
            (void)rnd();
            u[i].type = rnd() % 10;
#endif
            u[i].flags = (rnd() % 5 == 0) ? 1u << 8 : 0;
            u[i].order = (rnd() % 9 == 0) ? ORDER_TRANSPORTED : 0;
            u[i].x = rnd() % WORLD_W;
            u[i].y = rnd() % WORLD_H;
        }
        for (int i = 1; i < MAX_BUILDINGS; ++i) {
            if (rnd() % 10 != 0) continue;
            b[i].alive = 1;
            b[i].owner = rnd() % 5;
            b[i].raw_owner = (int)(rnd() % 12) - 2;
            b[i].x = rnd() % WORLD_W;
            b[i].y = rnd() % WORLD_H;
        }
        u[0].x = (run % 4 == 0) ? 0 : (run % 4 == 1) ? WORLD_W - 1 : rnd() % WORLD_W;
        u[0].y = (run % 4 == 0) ? 0 : (run % 4 == 1) ? WORLD_H - 1 : rnd() % WORLD_H;
        int enemy = race_p7_source_owner(0);
        observer(1, 0, enemy); /* Seed a valid detector at every target position. */
        b[0].alive = 1;
        b[0].raw_type = SC64_P7_CANNON_RAW;
        b[0].raw_owner = 4;
        b[0].owner = race_p7_source_owner(4);
        b[0].x = u[0].x;
        b[0].y = u[0].y;
        u8 powered[MAX_BUILDINGS] = {0};
        powered[0] = 1;
        sc64_target_index_begin();
        Unit units_before[MAX_UNITS];
        Building buildings_before[MAX_BUILDINGS];
        memcpy(units_before, u, sizeof(u));
        memcpy(buildings_before, b, sizeof(b));
        Sc64P7PersistentState state_before = race_p7;
        unsigned want = sc64_p7_arbiter_detected_mask_reference(0, powered);
        unsigned got = sc64_p7_arbiter_detected_mask(0, powered);
        assert(got == want);
        positive += want != 0;
        assert(!memcmp(units_before, u, sizeof(u)) &&
               !memcmp(buildings_before, b, sizeof(b)) &&
               !memcmp(&state_before, &race_p7, sizeof(race_p7)));
        int moved = 2;
        while (moved < MAX_UNITS && !u[moved].alive) ++moved;
        if (moved < MAX_UNITS) {
            u[moved].x = rnd() % WORLD_W;
            u[moved].y = rnd() % WORLD_H;
            sc64_target_index_move(&u[moved]);
            want = sc64_p7_arbiter_detected_mask_reference(0, powered);
            assert(sc64_p7_arbiter_detected_mask(0, powered) == want);
            u[moved].x = -1;
            sc64_target_index_move(&u[moved]);
            assert(!sc64_target_index_active);
            want = sc64_p7_arbiter_detected_mask_reference(0, powered);
            assert(sc64_p7_arbiter_detected_mask(0, powered) == want);
        }
        sc64_target_index_end();
        assert(sc64_p7_arbiter_detected_mask(0, powered) == want);
    }
    assert(positive > 0);
    printf("owner mask oracle: 2000 cases, %u positive masks\n", positive);
    return 0;
}
