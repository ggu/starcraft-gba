/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Asset-free contract fixture for the exact contact-mask memo. The real
 * collision headers and geometry are under test; only unrelated engine facts
 * are supplied here. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../common/gba.h"

typedef struct { int x,y,type,raw_type,alive,order; unsigned flags; } Unit;
typedef struct { unsigned top_speed_q10; } Sc64StockUnitProfile;
#define MAX_UNITS 8
#define MAP_COLS 8
#define MAP_ROWS 8
#define MAP_CELLS (MAP_COLS * MAP_ROWS)
#define WORLD_W (MAP_COLS * 16)
#define WORLD_H (MAP_ROWS * 16)
#define U_CHRYSALIS 7
#define U_MARINE 1
#define U_WRAITH 2
#define U_SCV 3
#define U_BURROW 37
#define SC64_P6_COMBAT_ENABLED 1
#define SC64_P6_RAW_INFESTED_TERRAN 40
#define ORDER_MINE 1
#define ORDER_GAS 2
#define GBA_HOT
#define SC64_RACES 1
#define SC64_CONTACT_CACHE 1
#define SC64_CONTACT_MEMO_ENABLED 1
#define IS_WORKER(t) ((t) == U_SCV)
#define SC64_TRANSPORT_LOADED(q) (((q)->flags & 1u) != 0)
#define SC64_GAME_ACTOR_COLLIDABLE 4u
#define SC64_GAME_ACTOR_MOVABLE 2u
static Unit u[MAX_UNITS];
static int p6;
static void sc64_contact_memo_invalidate(void);
#ifdef SC64_CONTACT_LOCAL_MEMO_PROBE
static void sc64_contact_memo_move_cells(unsigned from, unsigned to);
#endif
#define SC64_CONTACT_MEMO_INVALIDATE() sc64_contact_memo_invalidate()
static int sc64_race_unit_flying(int t) { return t == U_WRAITH; }
static int sc64_race_raw_unit(int t) { return t; }
static unsigned sc64_game_unit_actor_flags(int t) { (void)t; return 0; }
static int sc64_game_unit_collision_radius(int t) { return t == 4 ? 20 : t == 5 ? 31 : 6; }
static int race_p6_active(void) { return p6; }
static int race_p6_combat_unit_burrowed(int slot)
{ return p6 && (u[slot].flags & 2u); }
static const Sc64StockUnitProfile *sc64_stock_unit_profile(int raw)
{ (void)raw; return 0; }
#ifdef SC64_PERSISTENT_COLLISION
#include "../starcraft/sc64_persistent_collision.h"
#else
#include "../starcraft/sc64_collision_legacy.h"
#endif

static int unit_step_clear(const Unit *q, int nx, int ny)
{ return sc64_unit_bucket_step_clear(q,nx,ny); }

static void clear_world(void)
{
    memset(u,0,sizeof(u));
    sc64_contact_memo_invalidate();
}
static void setunit(int i,int x,int y,int type,int raw)
{
    u[i].x=x; u[i].y=y; u[i].type=type; u[i].raw_type=raw; u[i].alive=1;
}
static unsigned uncached(const Unit *q,int speed)
{ return sc64_contact_blocked_mask_uncached(q,speed); }
static void same(unsigned got,unsigned want) { assert(got==want); }

static void test_geometry_and_key(void)
{
    clear_world(); setunit(0,80,80,U_MARINE,1); setunit(1,92,80,U_MARINE,1);
    sc64_unit_buckets_begin(); assert(sc64_unit_bucket_active);
    same(sc64_contact_blocked_mask(&u[0],2),uncached(&u[0],2));
    same(sc64_contact_blocked_mask(&u[0],2),uncached(&u[0],2));
    assert(sc64_contact_memo_count==1);
    /* Exact keys include speed/order/raw/type; changing each prevents reuse. */
    (void)sc64_contact_blocked_mask(&u[0],1); assert(sc64_contact_memo_count==2);
    ++u[0].order; (void)sc64_contact_blocked_mask(&u[0],2); assert(sc64_contact_memo_count==3);
    ++u[0].raw_type; (void)sc64_contact_blocked_mask(&u[0],2); assert(sc64_contact_memo_count==4);
    u[0].type=U_WRAITH; (void)sc64_contact_blocked_mask(&u[0],2); assert(sc64_contact_memo_count==4);
    sc64_contact_memo_invalidate(); assert(sc64_contact_memo_count==0);
}

static void test_self_exclusion_and_lru(void)
{
    clear_world();
    for(int i=0;i<5;i++) setunit(i,40+i*16,40,U_MARINE,i);
    sc64_unit_buckets_begin();
    for(int i=0;i<4;i++) (void)sc64_contact_blocked_mask(&u[i],1);
    /* Touch oldest entry 0, so entry 1 is evicted by inserting unit 4. */
    (void)sc64_contact_blocked_mask(&u[0],1);
    (void)sc64_contact_blocked_mask(&u[4],1);
    assert(sc64_contact_memo_lookup(&u[0],1,0,&(unsigned){0}));
    assert(!sc64_contact_memo_lookup(&u[1],1,0,&(unsigned){0}));
    /* Nonzero-step self position must not count as a blocker. */
    clear_world(); setunit(0,80,80,U_MARINE,1); sc64_unit_buckets_begin();
    same(sc64_contact_blocked_mask(&u[0],2),0);
}

static void test_eligibility_and_invalidation(void)
{
    clear_world(); setunit(0,80,80,U_MARINE,1); setunit(1,92,80,U_MARINE,2);
    sc64_unit_buckets_begin();
    (void)sc64_contact_blocked_mask(&u[0],2); assert(sc64_contact_memo_count==1);
    /* Real hook invalidates even a move within the same spatial cell. */
    u[1].x=95; sc64_unit_bucket_move(&u[1]); assert(sc64_contact_memo_count==0);
    same(sc64_contact_blocked_mask(&u[0],2),uncached(&u[0],2));
    sc64_unit_buckets_end(); assert(sc64_contact_memo_count==0);
    sc64_unit_buckets_begin(); (void)sc64_contact_blocked_mask(&u[0],2);
    /* Each direct mutation below bypasses runtime lifecycle callbacks; clear
     * explicitly to model those callbacks, rather than claiming hook coverage. */
    u[1].alive=0; sc64_contact_memo_invalidate();
    same(sc64_contact_blocked_mask(&u[0],2),uncached(&u[0],2));
    u[1].alive=1; u[1].type=U_WRAITH; sc64_contact_memo_invalidate();
    same(sc64_contact_blocked_mask(&u[0],2),uncached(&u[0],2));
    u[1].type=U_MARINE; u[1].flags=1; sc64_contact_memo_invalidate();
    same(sc64_contact_blocked_mask(&u[0],2),uncached(&u[0],2));
    assert(!sc64_contact_memo_eligible(&u[0],0));
    assert(!sc64_contact_memo_eligible(&u[0],256));
    u[0].raw_type=32768; assert(!sc64_contact_memo_eligible(&u[0],2));
    u[0].raw_type=1; u[0].x=-1; assert(!sc64_contact_memo_eligible(&u[0],2));
    u[0].x=80; u[0].order=256; assert(!sc64_contact_memo_eligible(&u[0],2));
    u[0].order=0;
    u[0].type=U_WRAITH; assert(sc64_contact_memo_eligible(&u[0],2));
    u[0].type=U_SCV; u[0].order=ORDER_MINE; assert(!sc64_contact_memo_eligible(&u[0],2));
    u[0].order=0; u[0].flags=1; assert(sc64_contact_memo_eligible(&u[0],2));
    u[0].flags=0; p6=1; assert(sc64_contact_memo_eligible(&u[0],2));
    u[0].flags=0; u[1].x=86; u[1].type=U_BURROW; u[1].raw_type=37; u[1].flags=2;
    sc64_contact_memo_invalidate();
    p6=0;
    unsigned ordinary = sc64_contact_blocked_mask(&u[0],2);
    same(ordinary,uncached(&u[0],2));
    p6=1;
    unsigned burrowed = sc64_contact_blocked_mask(&u[0],2);
    same(burrowed,uncached(&u[0],2));
    assert(ordinary != burrowed);
    p6=0;
}

/* Compare real cache and uncached masks with eight original point queries.
 * Type 4 exercises an out-of-table radius, type 5 the invalid-class
 * reference fallback in the actor-facts build. */
static void test_stacked_masks(void)
{
    static const int speeds[] = {0, 1, 2, 14, 15};
    static const int types[] = {U_MARINE, U_WRAITH, U_CHRYSALIS, 4, 5, U_BURROW};
    for (unsigned ti=0; ti<sizeof(types)/sizeof(types[0]); ++ti)
        for (unsigned flags=0; flags<4; ++flags) {
            clear_world(); p6 = 1;
            setunit(0, 64, 64, U_MARINE, 1);
            setunit(1, 64, 64, types[ti], types[ti]); u[1].flags=flags;
            setunit(2, 68, 64, U_MARINE, 1);
            sc64_unit_buckets_begin();
            for (unsigned si=0; si<sizeof(speeds)/sizeof(speeds[0]); ++si) {
                int speed=speeds[si]; unsigned want=0;
                for (int k=0; k<8; ++k)
                    if (!unit_step_clear(&u[0], u[0].x+sc64_contact_dx[k]*speed,
                                               u[0].y+sc64_contact_dy[k]*speed))
                        want |= 1u << k;
                sc64_contact_memo_invalidate();
                same(uncached(&u[0],speed),want);
                same(sc64_contact_blocked_mask(&u[0],speed),want);
                same(sc64_contact_blocked_mask(&u[0],speed),want);
            }
            sc64_unit_buckets_end();
        }
    p6=0;
}

#ifdef SC64_CONTACT_DEFER_STEERING_MEMO
static void test_deferred_steering_admission(void)
{
    int fresh;
    clear_world(); setunit(0,80,80,U_MARINE,1); setunit(1,86,80,U_MARINE,1);
    sc64_unit_buckets_begin();
    unsigned mask = SC64_STEER_MASK(&u[0],2,&fresh);
    same(mask,uncached(&u[0],2));
    assert(fresh && !sc64_contact_memo_count && (mask & 1u));
    SC64_STEER_REMEMBER(&u[0],2,mask,4,fresh);
    assert(!sc64_contact_memo_count);
    u[0].x -= 2; sc64_unit_bucket_move(&u[0]);
    mask = SC64_STEER_MASK(&u[0],2,&fresh);
    same(mask,uncached(&u[0],2));
    assert(fresh && !sc64_contact_memo_count);
    SC64_STEER_REMEMBER(&u[0],2,mask,-1,fresh);
    assert(sc64_contact_memo_count == 1);
    same(SC64_STEER_MASK(&u[0],2,&fresh),mask);
    assert(!fresh);
    sc64_unit_buckets_end();
    assert(!sc64_contact_memo_count);
}
#endif

#if defined(SC64_PERSISTENT_COLLISION) && defined(SC64_NYDUS_ENABLED)
static unsigned lifecycle_rng = 0x5364a71du;

static unsigned lifecycle_random(void)
{
    lifecycle_rng ^= lifecycle_rng << 13;
    lifecycle_rng ^= lifecycle_rng >> 17;
    lifecycle_rng ^= lifecycle_rng << 5;
    return lifecycle_rng;
}

static void verify_bucket_members(void)
{
    int seen[MAX_UNITS] = {0};
    for (int cell = 0; cell < MAP_CELLS; ++cell) {
        int previous = 0xff, count = 0;
        for (int slot = sc64_unit_bucket_head[cell]; slot != 0xff;
             slot = sc64_unit_bucket_next[slot]) {
            assert(slot >= 0 && slot < MAX_UNITS && ++count <= MAX_UNITS);
            assert(sc64_unit_bucket_prev[slot] == previous &&
                   sc64_unit_bucket_cell[slot] == cell && !seen[slot]);
            assert(u[slot].alive && !SC64_TRANSPORT_LOADED(&u[slot]));
            assert(cell == sc64_unit_bucket_cell_at(u[slot].x, u[slot].y));
            seen[slot] = 1;
            previous = slot;
        }
    }
    for (int slot = 0; slot < MAX_UNITS; ++slot) {
        int expected = u[slot].alive && !SC64_TRANSPORT_LOADED(&u[slot]);
        assert(seen[slot] == expected);
        if (!expected) assert(sc64_unit_bucket_cell[slot] == 0xffffu);
    }
}

static void test_persistent_lifecycle(void)
{
    clear_world();
    unsigned strict_checks = 0;
    for (int turn = 0; turn < 10000; ++turn) {
        int slot = lifecycle_random() % MAX_UNITS;
        Unit *actor = &u[slot];
        switch (lifecycle_random() % 6) {
        case 0:
            actor->alive = 0;
            break;
        case 1:
            actor->flags = 0;
            actor->alive = 1;
            actor->type = actor->raw_type = U_MARINE;
            actor->x = lifecycle_random() % WORLD_W;
            actor->y = lifecycle_random() % WORLD_H;
            break;
        case 2:
            actor->flags ^= 1u;
            break;
        case 3:
            if (!actor->alive) break;
            actor->x = lifecycle_random() % WORLD_W;
            actor->y = lifecycle_random() % WORLD_H;
            break;
        case 4:
            if (!actor->alive || SC64_TRANSPORT_LOADED(actor)) break;
            actor->x = -1;
            sc64_unit_buckets_begin();
            assert(!sc64_unit_bucket_active);
            actor->x = lifecycle_random() % WORLD_W;
            break;
        case 5:
            if (actor->alive) actor->type = (lifecycle_random() & 1u) ? U_MARINE : U_WRAITH;
            break;
        }
        sc64_unit_buckets_begin();
        assert(sc64_unit_bucket_active);
        verify_bucket_members();
        for (int i = 0; i < MAX_UNITS; ++i) {
            if (!u[i].alive || SC64_TRANSPORT_LOADED(&u[i])) continue;
            int x = (int)(lifecycle_random() % WORLD_W);
            int y = (int)(lifecycle_random() % WORLD_H);
            assert(sc64_unit_bucket_placement_clear(&u[i], x, y) ==
                   sc64_unit_bucket_step_clear_reference(&u[i], x, y, 1));
            ++strict_checks;
        }
        Unit *moving = &u[lifecycle_random() % MAX_UNITS];
        if (moving->alive && !SC64_TRANSPORT_LOADED(moving)) {
            moving->x = lifecycle_random() % WORLD_W;
            moving->y = lifecycle_random() % WORLD_H;
            sc64_unit_bucket_move(moving);
            assert(sc64_unit_bucket_active);
            verify_bucket_members();
            if (!(lifecycle_random() & 31u)) {
                moving->x = -1;
                sc64_unit_bucket_move(moving);
                assert(!sc64_unit_bucket_active);
                moving->x = lifecycle_random() % WORLD_W;
                sc64_unit_buckets_begin();
                assert(sc64_unit_bucket_active);
                verify_bucket_members();
            }
        }
        sc64_unit_buckets_end();
    }
    printf("persistent lifecycle: 10000 mutations, %u strict placement comparisons\n", strict_checks);
}
#endif

int main(void)
{
    test_geometry_and_key(); test_self_exclusion_and_lru(); test_eligibility_and_invalidation(); test_stacked_masks();
#ifdef SC64_CONTACT_DEFER_STEERING_MEMO
    test_deferred_steering_admission();
#endif
#if defined(SC64_PERSISTENT_COLLISION) && defined(SC64_NYDUS_ENABLED)
    test_persistent_lifecycle();
#endif
    puts("contact memo contract: ok"); return 0;
}
