/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Z7 collision steering-list oracle, capacity fallback, cache reuse, and SRAM restore. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

#if !defined(SC64_Z7_RUNTIME) || !defined(SC64_COLLISION_STEER_LIST) || \
    !defined(SC64_COLLISION_CLASS_CACHE)
#error "Compile the Z7 steering-list fixture with its two collision gates"
#endif

static u16 held_keys;
static void input(u16 held)
{
    game_update(held, (u16)(held & (u16)~held_keys));
    held_keys=held;
}
static void press(u16 key) { input(key); input(0); }

static void launch_z7(void)
{
    const int ordinal=sc64_catalog_playable_ordinal(SC64_FACTION_ZERG,
                                                    SC64_Z7_GLOBAL_ID);
    game_init(); held_keys=0;
    assert(ordinal==6);
    press(KEY_A);
    for(int page=0;page<ordinal/2;++page) press(KEY_RIGHT);
    if(ordinal&1) press(KEY_DOWN);
    press(KEY_A); press(KEY_A);
    assert(screen==SCREEN_PLAY && race_z7_active() && !race_z7.runtime.fault);
    assert(race_z7_map==&sc64_race_maps[SC64_Z7_GLOBAL_ID]);
}

static const s8 dx8[8]={1,1,0,-1,-1,-1,0,1};
static const s8 dy8[8]={0,1,1,1,0,-1,-1,-1};
static unsigned oracle_queries, oracle_blocked, oracle_builds, oracle_fallbacks;

static void compare_live_world(void)
{
    unsigned source_ground=0, source_drone=0;
    sc64_unit_buckets_begin();
    assert(sc64_unit_bucket_active);
    for(int i=0;i<MAX_UNITS;++i) {
        Unit *q=&u[i];
        if(!q->alive || q->raw_owner!=3) continue;
        ++source_ground;
        if(q->type==U_DRONE) ++source_drone;
        if(sc64_race_unit_flying(q->type) || SC64_TRANSPORT_LOADED(q)) continue;
        for(int speed=1;speed<=2;++speed) {
            Sc64SteerList list;
            if(!sc64_steer_list_build(&list,q,speed)) { ++oracle_fallbacks; continue; }
            ++oracle_builds;
            for(int k=0;k<8;++k) {
                int x=q->x+dx8[k]*speed, y=q->y+dy8[k]*speed;
                int expected;
                if(x<0 || y<0 || x>=WORLD_W || y>=WORLD_H) continue;
                expected=unit_step_clear(q,x,y);
                assert(sc64_steer_list_step_clear(&list,q,x,y)==expected);
                ++oracle_queries;
                oracle_blocked+=(unsigned)!expected;
            }
        }
    }
    sc64_unit_buckets_end();
    assert(source_ground>0 && source_drone>0);
}

static void test_dense_z7_actor_oracle(void)
{
    static Unit saved[MAX_UNITS];
    memcpy(saved,u,sizeof(u));
    memset(u,0,sizeof(u));
    for(int i=0;i<12;++i) {
        u[i].alive=1; u[i].type=(i&1)?U_HYDRALISK:U_ZERGLING;
        u[i].raw_type=(i&1)?38:37; u[i].raw_owner=3;
        u[i].x=(s16)(160+(i%3)); u[i].y=(s16)(160+(i%4));
        u[i].order=ORDER_IDLE;
    }
    sc64_unit_buckets_begin();
    assert(sc64_unit_bucket_active);
    Sc64SteerList list;
    assert(sc64_steer_list_build(&list,&u[0],1));
    for(int k=0;k<8;++k) {
        int x=u[0].x+dx8[k], y=u[0].y+dy8[k];
        int expected=unit_step_clear(&u[0],x,y);
        assert(sc64_steer_list_step_clear(&list,&u[0],x,y)==expected);
        ++oracle_queries;
        oracle_blocked+=(unsigned)!expected;
    }
    ++oracle_builds;
    sc64_unit_buckets_end();
    memcpy(u,saved,sizeof(u));
}

static int original_best_direction(Unit *q, int speed, int target_x, int target_y)
{
    int from_cell = q->x / 16 + (q->y / 16) * MAP_COLS;
    int best = -1, best_cost = 0x7fffffff;
    for (int d = 0; d < 8; ++d) {
        int nx = q->x + (int)dx8[d] * speed;
        int ny = q->y + (int)dy8[d] * speed;
        int cost = dist2(nx, ny, target_x, target_y);
        if (cost < best_cost && is_traversable(nx / 16, ny / 16, from_cell) &&
            unit_step_clear(q, nx, ny)) {
            best = d;
            best_cost = cost;
        }
    }
    return best;
}

static void assert_fallback_direction(Unit *q, int speed)
{
    q->path_x = q->x + 24;
    q->path_y = q->y + 8;
    assert(sc64_steer_best_direction(q, speed,
        q->x / 16 + (q->y / 16) * MAP_COLS, q->path_x, q->path_y) ==
        original_best_direction(q, speed, q->path_x, q->path_y));
}

static void test_raw_density_threshold_and_filter_fallback(void)
{
    static Unit saved[MAX_UNITS];
    memcpy(saved, u, sizeof(u));
    memset(u, 0, sizeof(u));
    u[0].alive = 1; u[0].type = U_ZERGLING; u[0].raw_type = 37;
    u[0].raw_owner = 3; u[0].x = 512; u[0].y = 512;
    u[0].order = ORDER_IDLE;
    /* Exactly 48 raw nodes (including the mover) remain eligible for a list. */
    for (int i = 1; i < 48; ++i) {
        u[i].alive = 1; u[i].type = U_OVERLORD; u[i].raw_type = 42;
        u[i].raw_owner = 3; u[i].x = 513; u[i].y = 513;
        u[i].order = ORDER_IDLE;
    }
    sc64_unit_buckets_begin();
    assert(sc64_unit_bucket_active);
    Sc64SteerList list;
    assert(sc64_steer_list_build(&list, &u[0], 1));
    for (int d = 0; d < 8; ++d) {
        int nx = u[0].x + dx8[d], ny = u[0].y + dy8[d];
        assert(sc64_steer_list_step_clear(&list, &u[0], nx, ny) ==
               unit_step_clear(&u[0], nx, ny));
    }
    sc64_unit_buckets_end();

    /* The 49th raw node forces the original path even though all 48 other
     * actors are flying and would have been filtered from the candidate list. */
    u[48].alive = 1; u[48].type = U_OVERLORD; u[48].raw_type = 42;
    u[48].raw_owner = 3; u[48].x = 513; u[48].y = 513;
    u[48].order = ORDER_IDLE;
    sc64_unit_buckets_begin();
    assert(sc64_unit_bucket_active);
    assert(!sc64_steer_list_build(&list, &u[0], 1));
    assert_fallback_direction(&u[0], 1);
    ++oracle_fallbacks;
    sc64_unit_buckets_end();
    memcpy(u, saved, sizeof(u));
}

static void test_dense_edge_fallbacks(void)
{
    static Unit saved[MAX_UNITS];
    const int cx[4] = {2, WORLD_W - 3, 2, WORLD_W - 3};
    const int cy[4] = {2, 2, WORLD_H - 3, WORLD_H - 3};
    memcpy(saved, u, sizeof(u));
    for (int edge = 0; edge < 4; ++edge) {
        memset(u, 0, sizeof(u));
        for (int i = 0; i < 56; ++i) {
            u[i].alive = 1; u[i].type = U_ZERGLING; u[i].raw_type = 37;
            u[i].raw_owner = 3;
            u[i].x = (s16)limit(cx[edge] + (i % 3) - 1, 0, WORLD_W - 1);
            u[i].y = (s16)limit(cy[edge] + (i % 4) - 1, 0, WORLD_H - 1);
            u[i].order = ORDER_IDLE;
        }
        sc64_unit_buckets_begin();
        assert(sc64_unit_bucket_active);
        Sc64SteerList list;
        assert(!sc64_steer_list_build(&list, &u[0], 2));
        assert_fallback_direction(&u[0], 2);
        sc64_unit_buckets_end();
        ++oracle_fallbacks;
    }
    memcpy(u, saved, sizeof(u));
}

static void test_worker_fallback(void)
{
    int worker=-1, old_order;
    for(int i=0;i<MAX_UNITS;++i)
        if(u[i].alive && u[i].raw_owner==3 && u[i].type==U_DRONE) { worker=i; break; }
    assert(worker>=0);
    old_order=u[worker].order;
    u[worker].order=ORDER_MINE;
    sc64_unit_buckets_begin();
    assert(sc64_unit_bucket_active);
    Sc64SteerList list;
    assert(!sc64_steer_list_build(&list,&u[worker],1));
    ++oracle_fallbacks;
    /* The original checked path remains available to the movement fallback. */
    (void)unit_step_clear(&u[worker],u[worker].x+1,u[worker].y);
    sc64_unit_buckets_end();
    u[worker].order=old_order;
}

static void test_overflow_fallback(void)
{
    static Unit saved[MAX_UNITS];
    memcpy(saved,u,sizeof(u));
    memset(u,0,sizeof(u));
    for(int i=0;i<51;++i) {
        u[i].alive=1; u[i].type=U_ZERGLING; u[i].raw_type=37; u[i].raw_owner=3;
        u[i].x=(s16)(160+(i%7)); u[i].y=(s16)(160+(i%7));
        u[i].order=ORDER_IDLE;
    }
    sc64_unit_buckets_begin();
    assert(sc64_unit_bucket_active);
    Sc64SteerList list;
    assert(!sc64_steer_list_build(&list,&u[0],1));
    ++oracle_fallbacks;
    /* Overflow means the caller must use the exact original query. */
    (void)unit_step_clear(&u[0],u[0].x+1,u[0].y);
    sc64_unit_buckets_end();
    memcpy(u,saved,sizeof(u));
}

static void test_class_slot_reuse(void)
{
    int slot=-1, radius, flying, collidable, old_type;
    for(int i=0;i<MAX_UNITS;++i)
        if(u[i].alive && u[i].raw_owner==3) { slot=i; break; }
    assert(slot>=0);
    sc64_unit_buckets_begin();
    assert(sc64_unit_bucket_active);
    old_type=u[slot].type;
    (void)sc64_unit_collision_class(slot,old_type,&radius,&flying,&collidable);
    assert(sc64_unit_collision_class_cache[slot]!=0);
    u[slot].type=U_HYDRALISK;
    sc64_unit_collision_class_invalidate(slot); /* same invalidation used when a slot is reused */
    (void)sc64_unit_collision_class(slot,u[slot].type,&radius,&flying,&collidable);
    assert(radius==sc64_game_unit_collision_radius(U_HYDRALISK));
    assert(flying==sc64_race_unit_flying(U_HYDRALISK));
    { unsigned flags=sc64_game_unit_actor_flags(U_HYDRALISK);
      assert(collidable==(!flags || (flags&SC64_GAME_ACTOR_COLLIDABLE))); }
    sc64_unit_buckets_end();
    u[slot].type=old_type;
    sc64_unit_collision_class_invalidate(slot);
}

static void test_save_load_rebuilds_cache(void)
{
    RaceSave saved;
    assert(race_save_game());
    saved=race_data;
    save_write(&saved,sizeof(saved),SAVE_VERSION);
    game_init(); held_keys=0;
    assert(race_load_game());
    assert(screen==SCREEN_PLAY && race_z7_active() && !race_z7.runtime.fault);
    sc64_unit_buckets_begin();
    assert(sc64_unit_bucket_active);
    for(int i=0;i<MAX_UNITS;++i) assert(sc64_unit_collision_class_cache[i]==0);
    sc64_unit_buckets_end();
    compare_live_world();
}

int main(void)
{
    launch_z7();
    compare_live_world();
    test_dense_z7_actor_oracle();
    test_raw_density_threshold_and_filter_fallback();
    test_dense_edge_fallbacks();
    test_worker_fallback();
    test_overflow_fallback();
    test_class_slot_reuse();
    test_save_load_rebuilds_cache();
    assert(oracle_builds>0 && oracle_fallbacks>0 && oracle_queries>0 &&
           oracle_blocked>0);
    printf("Z7 steer oracle: %u queries (%u blocked), %u builds, %u fallbacks; SRAM restore passed\n",
           oracle_queries,oracle_blocked,oracle_builds,oracle_fallbacks);
    return 0;
}
