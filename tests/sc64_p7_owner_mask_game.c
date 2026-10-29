/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"
/* Frozen linear reference from commit 472645a; keep independent of the candidate. */
static unsigned sc64_p7_arbiter_detected_mask_reference(int target,
                                                  const u8 powered_cannons[MAX_BUILDINGS])
{
    unsigned visible_owners=0,detected=0;
    int x,y,col,row,cx,cy;
    if(target<0||target>=MAX_UNITS||!u[target].alive) return 0;
    x=u[target].x; y=u[target].y; col=x/16; row=y/16;
    if(col<0||row<0||col>=MAP_COLS||row>=MAP_ROWS) return 0;
    cx=col*16+8; cy=row*16+8;
#ifdef SC64_P7_GEOMETRY_RECT_HOIST_ENABLED
    int target_l,target_t,target_r,target_b;
    if(!sc64_p7_actor_rect_source((unsigned)u[target].raw_type,x,y,0,
                                  &target_l,&target_t,&target_r,&target_b))
        return 0;
#endif
    /* Calculate all non-human source visibility in one actor/building pass;
       slot 7 uses the engine's authenticated SIDE visibility grid. */
    {
        int cell=col+row*MAP_COLS;
        if(SC64_GRID_GET(visible,cell)) visible_owners|=1u<<7;
    }
    for(int i=0;i<MAX_UNITS;++i) if(u[i].alive &&
#ifdef SC64_P7_CARRIER_TRAINING_ENABLED
        !SC64_CARRIER_HANGAR_HIDDEN(&u[i]) &&
#endif
        !SC64_TRANSPORT_LOADED(&u[i])) {
        int source=u[i].raw_owner;
#ifdef SC64_P7_SIGHT_AXIS_CULL
        int dx=cx-u[i].x,dy=cy-u[i].y;
        if(dx < -SC64_P7_SIGHT_UNIT_AXIS_BOUND || dx > SC64_P7_SIGHT_UNIT_AXIS_BOUND ||
            dy < -SC64_P7_SIGHT_UNIT_AXIS_BOUND || dy > SC64_P7_SIGHT_UNIT_AXIS_BOUND) continue;
        int radius=sc64_p7_observer_sight_radius_runtime(u[i].type,0);
#else
        int radius=sc64_p7_observer_sight_radius_runtime(u[i].type,0);
        int dx=cx-u[i].x,dy=cy-u[i].y;
#endif
        if(dx*dx+dy*dy>radius*radius) continue;
        if(source>=0&&source<8) {
            int live=race_p7_source_owner((unsigned)source);
            if(live>=0&&live==u[i].owner) visible_owners|=1u<<(unsigned)source;
        } else for(unsigned owner=0;owner<7;++owner)
            if(race_p7_source_owner(owner)==u[i].owner)
                visible_owners|=1u<<owner;
    }
    for(int i=0;i<MAX_BUILDINGS;++i) if(b[i].alive) {
        int source=b[i].raw_owner;
#ifdef SC64_P7_SIGHT_AXIS_CULL
        int dx=cx-b[i].x,dy=cy-b[i].y;
        if(dx < -SC64_P7_SIGHT_BUILDING_AXIS_BOUND || dx > SC64_P7_SIGHT_BUILDING_AXIS_BOUND ||
            dy < -SC64_P7_SIGHT_BUILDING_AXIS_BOUND || dy > SC64_P7_SIGHT_BUILDING_AXIS_BOUND) continue;
        int radius=sc64_p7_observer_sight_radius_runtime(b[i].type,1);
#else
        int radius=sc64_p7_observer_sight_radius_runtime(b[i].type,1);
        int dx=cx-b[i].x,dy=cy-b[i].y;
#endif
        if(dx*dx+dy*dy>radius*radius) continue;
        if(source>=0&&source<8) {
            int live=race_p7_source_owner((unsigned)source);
            if(live>=0&&live==b[i].owner) visible_owners|=1u<<(unsigned)source;
        } else for(unsigned owner=0;owner<7;++owner)
            if(race_p7_source_owner(owner)==b[i].owner)
                visible_owners|=1u<<owner;
    }
    for(int i=0;i<MAX_UNITS;++i) if(u[i].alive &&
#ifdef SC64_P7_CARRIER_TRAINING_ENABLED
        !SC64_CARRIER_HANGAR_HIDDEN(&u[i]) &&
#endif
        !SC64_TRANSPORT_LOADED(&u[i]) &&
        u[i].raw_type==SC64_P7_OBSERVER_RAW) {
        int source=u[i].raw_owner;
#ifdef SC64_P7_GEOMETRY_RECT_HOIST_ENABLED
        int ol,ot,or_,ob;
        int distance=-1;
        if(sc64_p7_actor_rect_source(SC64_P7_OBSERVER_RAW,u[i].x,u[i].y,0,
                                     &ol,&ot,&or_,&ob))
            distance=sc64_p7_observer_rect_distance_rects(
                ol,ot,or_,ob,target_l,target_t,target_r,target_b);
#else
        int distance=sc64_p7_observer_rect_distance(SC64_P7_OBSERVER_RAW,
            u[i].x,u[i].y,0,(unsigned)u[target].raw_type,u[target].x,u[target].y,0);
#endif
        if(distance<0||distance>SC64_P7_OBSERVER_BASE_SIGHT_SOURCE_PX) continue;
        if(source>=0&&source<8 && race_p7_source_owner((unsigned)source)==u[i].owner &&
           (visible_owners&(1u<<(unsigned)source))) detected|=1u<<(unsigned)source;
        else if(source<0||source>=8) for(unsigned owner=0;owner<7;++owner)
            if(race_p7_source_owner(owner)==u[i].owner &&
               (visible_owners&(1u<<owner))) detected|=1u<<owner;
    }
    for(int i=0;i<MAX_BUILDINGS;++i) if(b[i].alive &&
        b[i].raw_type==SC64_P7_CANNON_RAW && building_complete(i) &&
        powered_cannons[i]) {
        int source=b[i].raw_owner;
#ifdef SC64_P7_GEOMETRY_RECT_HOIST_ENABLED
        int cl,ct,cr,cb;
        int distance=-1;
        if(sc64_p7_actor_rect_source(SC64_P7_CANNON_RAW,b[i].x,b[i].y,1,
                                     &cl,&ct,&cr,&cb))
            distance=sc64_p7_observer_rect_distance_rects(
                cl,ct,cr,cb,target_l,target_t,target_r,target_b);
#else
        int distance=sc64_p7_observer_rect_distance(SC64_P7_CANNON_RAW,
            b[i].x,b[i].y,1,(unsigned)u[target].raw_type,u[target].x,u[target].y,0);
#endif
        if(distance<0||distance>SC64_P7_CANNON_DETECTION_SOURCE_PX) continue;
        if(source>=0&&source<8 && race_p7_source_owner((unsigned)source)==b[i].owner &&
           (visible_owners&(1u<<(unsigned)source))) detected|=1u<<(unsigned)source;
        else if(source<0||source>=8) for(unsigned owner=0;owner<7;++owner)
            if(race_p7_source_owner(owner)==b[i].owner &&
               (visible_owners&(1u<<owner))) detected|=1u<<owner;
    }
    return detected&255u;
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
    u[1].order = 0;
    u[1].type = U_P7_INTERCEPTOR;
    u[1].flags = 1u << 8; /* A hangar-hidden child cannot supply sight or detection. */
    expect_mask(0, powered);

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
        race_mode = run & 1;
        reset_case();
        for (int i = 1; i < MAX_UNITS; ++i) {
            if (rnd() % 5 != 0) continue;
            u[i].alive = 1;
            u[i].owner = rnd() % 5;
            u[i].raw_owner = (int)(rnd() % 12) - 2;
            u[i].raw_type = (rnd() % 3 == 0) ? SC64_P7_OBSERVER_RAW : rnd() % 90;
            u[i].type = (rnd() % 7 == 0) ? U_P7_INTERCEPTOR : rnd() % 10;
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
