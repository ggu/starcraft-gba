/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Asset-free broad-phase regression against an independent full scan. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef int8_t s8;
typedef int16_t s16;
#define MAX_UNITS 240
#define MAP_COLS 64
#define MAP_ROWS 64
#define MAP_CELLS (MAP_COLS * MAP_ROWS)
#define WORLD_W (MAP_COLS * 16)
#define WORLD_H (MAP_ROWS * 16)
#define GBA_HOT
#define ORDER_IDLE 0
#define ORDER_MINE 1
#define ORDER_GAS 2
#define ORDER_TRANSPORTED 3
#define IS_WORKER(t) ((t) == U_SCV || (t) == U_DRONE || (t) == U_PROBE)

typedef struct Unit {
    int x, y, type, alive, order, loaded, burrowed;
} Unit;
static Unit u[MAX_UNITS];
#define SC64_TRANSPORT_LOADED(p) ((p)->loaded)

#include "race_rules.h"
#ifdef SC64_COLLISION_PACKED_HIT
/* Keep synthetic out-of-range facts local to this fixture. Real actor cases
 * still use the public providers, and product headers remain unchanged. */
#define sc64_game_unit_actor_flags collision_bounds_source_flags
#define sc64_game_unit_collision_radius collision_bounds_source_radius
#endif
#include "sc64_game_actor_facts.h"
#ifdef SC64_COLLISION_PACKED_HIT
#undef sc64_game_unit_actor_flags
#undef sc64_game_unit_collision_radius
static unsigned sc64_game_unit_actor_flags(int type)
{
    if (type == 253) return SC64_GAME_ACTOR_MOVABLE;
    return collision_bounds_source_flags(type);
}
static int sc64_game_unit_collision_radius(int type)
{
    if (type == 254) return 64;
    if (type == 253) return 0;
    return collision_bounds_source_radius(type);
}
#endif
static int race_p6_combat_unit_burrowed(int slot)
{ return slot >= 0 && slot < MAX_UNITS && u[slot].burrowed; }
#include "sc64_collision_legacy.h"

static const int dirs[8][2] = {
    {1,0},{1,1},{0,1},{-1,1},{-1,0},{-1,-1},{0,-1},{1,-1}
};
static unsigned long cases_run;

static void clear_world(void)
{
    memset(u, 0, sizeof(u));
    for (int i = 0; i < MAX_UNITS; ++i) {
        u[i].x = u[i].y = 512;
        u[i].type = U_MARINE;
    }
}

/* Independent full scan: uses public actor facts, never bucket/list state. */
static int oracle(const Unit *q, int nx, int ny, int strict)
{
    if (nx < 0 || ny < 0 || nx >= WORLD_W || ny >= WORLD_H) return 0;
    if (!strict && IS_WORKER(q->type) &&
        (q->order == ORDER_MINE || q->order == ORDER_GAS)) return 1;
    const int mr = sc64_game_unit_collision_radius(q->type);
    const int mf = sc64_race_unit_flying(q->type);
    const unsigned mflags = sc64_game_unit_actor_flags(q->type);
    const int mc = !mflags || (mflags & SC64_GAME_ACTOR_COLLIDABLE);
    for (int i = 0; i < MAX_UNITS; ++i) {
        const Unit *p = &u[i];
        if (p == q || !p->alive || SC64_TRANSPORT_LOADED(p) || p->burrowed ||
            p->type == U_CHRYSALIS) continue;
        if (sc64_race_unit_flying(p->type) != mf) continue;
        const unsigned flags = sc64_game_unit_actor_flags(p->type);
        const int collidable = !flags || (flags & SC64_GAME_ACTOR_COLLIDABLE);
        if (!mc || !collidable) continue;
        const int cr = sc64_game_unit_collision_radius(p->type);
        const int radius = mr > cr ? mr : cr;
        const int dx = p->x - nx, dy = p->y - ny;
        if (dx <= -radius || dx >= radius || dy <= -radius || dy >= radius)
            continue;
        const int nd2 = dx * dx + dy * dy;
        const int ox = p->x - q->x, oy = p->y - q->y;
        if (nd2 < radius * radius &&
            (strict || nd2 <= ox * ox + oy * oy)) return 0;
    }
    return 1;
}

static void check(int mover, int nx, int ny, int strict, const char *label)
{
    const int want = oracle(&u[mover], nx, ny, strict);
    const int got = strict ? sc64_unit_bucket_placement_clear(&u[mover],nx,ny)
                           : sc64_unit_bucket_step_clear(&u[mover],nx,ny);
    if (got != want) {
        fprintf(stderr, "%s mismatch mover=%s/raw%d at(%d,%d), query(%d,%d), strict=%d want=%d got=%d\n",
                label, sc64_race_unit_name(u[mover].type),
                sc64_race_raw_unit(u[mover].type), u[mover].x,u[mover].y,
                nx,ny,strict,want,got);
        abort();
    }
    ++cases_run;
}

static int actor_with_raw(int raw)
{
    for (int t = 0; t < SC_RU_COUNT; ++t)
        if (sc64_race_raw_unit(t) == raw) return t;
    return -1;
}

static void exhaustive_public_radius_bounds(void)
{
    int max_ground = 0, max_air = 0;
    int guardian = actor_with_raw(44), battlecruiser = U_P5_BATTLECRUISER;
    assert(guardian >= 0 && battlecruiser >= 0);
    assert(strcmp(sc64_race_unit_name(guardian), "GUARDIAN") == 0);
    assert(sc64_game_unit_collision_radius(guardian) == 11);
    assert(sc64_game_unit_collision_radius(battlecruiser) == 19);
    assert(sc64_race_raw_unit(U_P5_BATTLECRUISER) == 12);
    assert(sc64_game_unit_collision_radius(U_P5_BATTLECRUISER) == 19);
    for (int t = 0; t < SC_RU_COUNT; ++t) {
        unsigned f = sc64_game_unit_actor_flags(t);
        if (f && !(f & SC64_GAME_ACTOR_COLLIDABLE)) continue;
        int r = sc64_game_unit_collision_radius(t);
        assert(r >= 0 && r < 64);
        if (sc64_race_unit_flying(t)) { if (r > max_air) max_air = r; }
        else if (r > max_ground) max_ground = r;
    }
    assert(max_ground == 10 && max_air == 19);
    printf("bounds: ground=%d air=%d; Guardian raw44 r11; Battlecruiser raw12 r19\n",
           max_ground,max_air);
}

static void exact_radius_regressions(void)
{
    const int types[2] = { actor_with_raw(44), U_P5_BATTLECRUISER };
    const int blockers[2] = {16,24};
    clear_world();
    for (int k=0;k<2;k++) {
        clear_world();
        u[0]=(Unit){.x=5,.y=512,.type=types[k],.alive=1};
        u[1]=(Unit){.x=blockers[k],.y=512,.type=types[k],.alive=1};
        sc64_unit_buckets_begin();
        assert(oracle(&u[0],6,512,0)==0);
        check(0,6,512,0,"exact radius regression");
        assert(sc64_unit_bucket_step_clear(&u[0],6,512)==0);
        sc64_unit_buckets_end();
    }
}

static void grid_phase_direction_speed_matrix(void)
{
    const int reps[] = { U_ZP_ULTRALISK, U_Z6_GUARDIAN, U_P5_BATTLECRUISER,
                         U_P7_ARBITER, U_P7_CARRIER };
    const int offsets[][2] = {{0,0},{1,0},{-1,0},{0,1},{0,-1},{1,1},{-1,-1}};
    for (unsigned a=0; a<sizeof(reps)/sizeof(reps[0]); ++a) {
        const int mover_type = reps[a];
        for (int px=0; px<16; ++px) for (int py=0; py<16; ++py)
        for (int d=0; d<8; ++d) for (int speed=0; speed<=3; ++speed)
        for (int gap=0; gap<3; ++gap)
        for (unsigned oi=0; oi<sizeof(offsets)/sizeof(offsets[0]); ++oi) {
            clear_world();
            const int qx=400+px, qy=400+py;
            const int nx=qx+dirs[d][0]*speed, ny=qy+dirs[d][1]*speed;
            const int radius=sc64_game_unit_collision_radius(mover_type);
            u[0]=(Unit){.x=qx,.y=qy,.type=mover_type,.alive=1};
            const int extent=radius-1+gap; /* inside, tangent, just separated */
            u[1]=(Unit){.x=nx+offsets[oi][0]*extent,
                        .y=ny+offsets[oi][1]*extent,
                        .type=mover_type,.alive=1};
            sc64_unit_buckets_begin();
            check(0,nx,ny,0,"phase/direction/speed");
            check(0,nx,ny,1,"strict placement phase matrix");
            sc64_unit_buckets_end();
        }
    }
}

static void filter_cache_and_edge_cases(void)
{
    int marine=U_MARINE, guardian=actor_with_raw(44), bc=U_P5_BATTLECRUISER;
    clear_world();
    u[0]=(Unit){.x=512,.y=512,.type=marine,.alive=1};
    u[1]=(Unit){.x=518,.y=512,.type=marine,.alive=1};
    sc64_unit_buckets_begin();
    check(0,513,512,0,"cold class cache");
    check(0,513,512,0,"warm class cache");
    assert(!sc64_unit_bucket_step_clear(&u[0],513,512));
    u[1].alive=0;
#ifdef SC64_COLLISION_CLASS_CACHE
    sc64_unit_collision_class_invalidate(1); /* stale slot becomes Guardian */
#endif
    u[1]=(Unit){.x=524,.y=512,.type=guardian,.alive=1};
#ifdef SC64_COLLISION_CLASS_CACHE
    sc64_unit_collision_class_invalidate(1);
#endif
    sc64_unit_bucket_move(&u[1]);
    check(0,513,512,0,"slot reuse/type invalidation");
    sc64_unit_buckets_end();

    /* Flight class, death, transport, burrow, and worker's mining bypass. */
    clear_world();
    u[0]=(Unit){.x=512,.y=512,.type=marine,.alive=1};
    u[1]=(Unit){.x=513,.y=512,.type=marine,.alive=1};
    u[2]=(Unit){.x=513,.y=512,.type=marine,.alive=0};
    u[3]=(Unit){.x=513,.y=512,.type=marine,.alive=1,.burrowed=1};
    u[4]=(Unit){.x=513,.y=512,.type=bc,.alive=1};
    sc64_unit_buckets_begin();
    u[1].loaded=1; /* Transition after bucket creation exercises live filter. */
    check(0,512,512,0,"loaded/dead/burrowed/flight filters");
    sc64_unit_buckets_end();

    clear_world();
    u[0]=(Unit){.x=512,.y=512,.type=U_SCV,.alive=1,.order=ORDER_MINE};
    u[1]=(Unit){.x=512,.y=512,.type=marine,.alive=1};
    sc64_unit_buckets_begin();
    check(0,512,512,0,"worker mining bypass");
    check(0,512,512,1,"strict placement ignores worker bypass");
    sc64_unit_buckets_end();

    clear_world();
    u[0]=(Unit){.x=0,.y=0,.type=guardian,.alive=1};
    u[1]=(Unit){.x=15,.y=0,.type=guardian,.alive=1};
    sc64_unit_buckets_begin();
    check(0,0,0,0,"world origin");
    check(0,1,0,0,"edge bucket reach");
    check(0,WORLD_W-1,WORLD_H-1,0,"far world edge query");
    sc64_unit_buckets_end();
}

#ifdef SC64_COLLISION_STEER_LIST
static void steer_list_matches_oracle_and_falls_back(void)
{
    Sc64SteerList list;
    clear_world();
    u[0]=(Unit){.x=512,.y=512,.type=U_P5_BATTLECRUISER,.alive=1};
    u[1]=(Unit){.x=520,.y=512,.type=U_P5_BATTLECRUISER,.alive=1};
    sc64_unit_buckets_begin();
    assert(!sc64_steer_list_build(&list,&u[0],1));
    check(0,513,512,0,"air mover list fallback");
    sc64_unit_buckets_end();

    clear_world();
    u[0]=(Unit){.x=512,.y=512,.type=U_MARINE,.alive=1};
    u[1]=(Unit){.x=520,.y=512,.type=U_MARINE,.alive=1};
    sc64_unit_buckets_begin();
    assert(sc64_steer_list_build(&list,&u[0],1));
    for (int d=0;d<8;d++) {
        int x=u[0].x+dirs[d][0], y=u[0].y+dirs[d][1];
        assert(sc64_steer_list_step_clear(&list,&u[0],x,y)==oracle(&u[0],x,y,0));
        ++cases_run;
    }
    sc64_unit_buckets_end();

    /* Dense bucket preflight must return to the original exact query. */
    clear_world();
    u[0]=(Unit){.x=512,.y=512,.type=U_MARINE,.alive=1};
    for(int i=1;i<60;i++) u[i]=(Unit){.x=512+(i%3),.y=512,.type=U_OVERLORD,.alive=1};
    sc64_unit_buckets_begin();
    assert(!sc64_steer_list_build(&list,&u[0],1));
    check(0,513,512,0,"dense filtered-false-positive fallback");
    sc64_unit_buckets_end();

    clear_world();
    u[0]=(Unit){.x=512,.y=512,.type=U_MARINE,.alive=1};
    sc64_unit_buckets_begin();
    assert(!sc64_steer_list_build(&list,&u[0],3));
    sc64_unit_buckets_end();
}
#endif

#ifdef SC64_COLLISION_PACKED_HIT
static void packed_hit_wrapper_contract(void)
{
    int r, f, c, cold_r, cold_f, cold_c;
    u8 packed;
    clear_world();
    u[1] = (Unit){.x=512,.y=512,.type=U_MARINE,.alive=1};
    assert(sc64_unit_collision_class_cache[1] == 0);
    packed = sc64_unit_collision_class_packed_hit(1, U_MARINE, &r, &f, &c);
    assert(packed == sc64_unit_collision_class_cache[1]);
    assert(r == 6 && f == 0 && c == 1);
    assert(sc64_unit_collision_class_packed_hit(1, -1, &r, &f, &c) == packed);
    assert(r == 6 && f == 0 && c == 1);
    /* A nonzero valid cache byte decodes locally without consulting the type. */
    sc64_unit_collision_class_cache[2] = sc64_unit_collision_class_pack(63,1,1);
    assert(sc64_unit_collision_class_packed_hit(2, -1, &r, &f, &c) == 255);
    assert(r == 63 && f == 1 && c == 1);
    /* Miss/zero sentinel calls the original helper and preserves its outputs. */
    assert(sc64_unit_collision_class_cache[3] == 0);
    packed = sc64_unit_collision_class_packed_hit(3, SC_RU_COUNT, &r, &f, &c);
    assert(packed == sc64_unit_collision_class(3, SC_RU_COUNT,
                                               &cold_r, &cold_f, &cold_c));
    assert(r == cold_r && f == cold_f && c == cold_c);
    assert(sc64_unit_collision_class_cache[3] == packed);
    /* Out-of-range slots use the original helper and never alter any slot. */
    u8 cache_before[MAX_UNITS];
    memcpy(cache_before, sc64_unit_collision_class_cache, sizeof(cache_before));
    packed = sc64_unit_collision_class_packed_hit(-1, U_MARINE, &r, &f, &c);
    assert(packed == sc64_unit_collision_class(-1, U_MARINE,
                                               &cold_r, &cold_f, &cold_c));
    assert(r == cold_r && f == cold_f && c == cold_c);
    packed = sc64_unit_collision_class_packed_hit(MAX_UNITS, U_MARINE,
                                                  &r, &f, &c);
    assert(packed == sc64_unit_collision_class(MAX_UNITS, U_MARINE,
                                               &cold_r, &cold_f, &cold_c));
    assert(r == cold_r && f == cold_f && c == cold_c);
    assert(memcmp(cache_before, sc64_unit_collision_class_cache,
                  sizeof(cache_before)) == 0);
    assert(sc64_unit_collision_class_pack(64,1,1) == 0);
    assert(sc64_unit_collision_class_pack(-1,1,1) == 0);
    assert(sc64_unit_collision_class_pack(0,0,0) == 0);
    /* Synthetic source facts force the helper's nonrepresentable-radius and
     * all-zero tuple cases. The wrapper must delegate on a zero cache byte. */
    sc64_unit_collision_class_invalidate(6);
    packed = sc64_unit_collision_class_packed_hit(6, 254, &r, &f, &c);
    assert(packed == 0 && r == 64 && f == 0 && c == 1);
    assert(sc64_unit_collision_class_cache[6] == 0);
    sc64_unit_collision_class_invalidate(7);
    packed = sc64_unit_collision_class_packed_hit(7, 253, &r, &f, &c);
    assert(packed == 0 && r == 0 && f == 0 && c == 0);
    assert(sc64_unit_collision_class_cache[7] == 0);
    puts("packed hit/miss/slot/unrepresentable/zero-tuple contract: PASS");
}
#endif

int main(void)
{
#ifdef SC64_COLLISION_PACKED_HIT
    packed_hit_wrapper_contract();
#endif
    exhaustive_public_radius_bounds();
    exact_radius_regressions();
    grid_phase_direction_speed_matrix();
    filter_cache_and_edge_cases();
#ifdef SC64_COLLISION_STEER_LIST
    steer_list_matches_oracle_and_falls_back();
#endif
    printf("PUBLIC COLLISION BOUNDS PASS cases=%lu actors=%d\n",cases_run,SC_RU_COUNT);
    return 0;
}
