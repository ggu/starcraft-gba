/* SPDX-License-Identifier: GPL-3.0-or-later */
/* SC64_CONTACT_CACHE differential checks: the cached-fact bucket query, the
 * one-pass blocked-steering mask and the stock-rate cache must give the same
 * answers as the original query, eight unit_step_clear() calls and
 * sc64_stock_motion_step(), across slot reuse, morphs, loads and bucket moves. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "../starcraft/game.c"

#ifndef SC64_CONTACT_CACHE
#error "Compile with -DSC64_CONTACT_CACHE"
#endif

static unsigned int rng_state = 0x6a09e667u;

static unsigned int
next_random(void)
{
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state << 5;
    return rng_state;
}

static unsigned long step_checks, strict_checks, mask_checks, inactive_mask_checks;
static unsigned long stock_checks, mutations, passes, p6_passes, failures;
static unsigned long step_fail_inmap, step_fail_offmap, mask_fail, stock_fail;
static unsigned long fail_flying_mover, fail_ground_mover;
static u8 linked[MAX_UNITS];

#define REPORT_LIMIT 12

static void
describe_unit(const char *label, int i)
{
    fprintf(stderr, "  %s slot=%d type=%d(%s) raw_type=%d raw=%d x=%d y=%d alive=%d "
            "order=%d flags=%d flying=%d\n", label, i, (int)u[i].type,
            sc64_race_unit_name(u[i].type), (int)u[i].raw_type,
            sc64_race_raw_unit(u[i].type), (int)u[i].x, (int)u[i].y,
            (int)u[i].alive, (int)u[i].order, (int)u[i].flags,
            sc64_race_unit_flying(u[i].type));
}

static int
fact_radius(int type)
{
#ifdef SC64_ZP_CAMPAIGN_ACTORS
    return sc64_game_unit_collision_radius(type);
#else
    (void)type;
    return 6;
#endif
}

/* Lists only candidates that the window-free reference predicate says block
 * the queried point, with their own collision radius. */
static void
describe_blockers(int q, int nx, int ny, int strict)
{
    int i, flying = sc64_race_unit_flying(u[q].type) != 0;
    for (i = 0; i < MAX_UNITS; ++i)
        if (i != q && u[i].alive && !SC64_TRANSPORT_LOADED(&u[i]) &&
            sc64_contact_candidate_reference(&u[q], i, nx, ny, fact_radius(u[q].type),
                                             flying, strict)) {
            fprintf(stderr, "    blocker radius=%d (mover radius=%d):\n",
                    fact_radius(u[i].type), fact_radius(u[q].type));
            describe_unit("blocker", i);
        }
}

static void
note_failure_mover(int qi)
{
    if (sc64_race_unit_flying(u[qi].type)) ++fail_flying_mover;
    else ++fail_ground_mover;
}

static void
report_step(const Unit *q, int nx, int ny, int strict, int want, int got)
{
    int qi = (int)(q - u);
    if (nx < 0 || ny < 0 || nx >= WORLD_W || ny >= WORLD_H) ++step_fail_offmap;
    else ++step_fail_inmap;
    note_failure_mover(qi);
    if (++failures > REPORT_LIMIT) return;
    fprintf(stderr, "step mismatch: strict=%d query(%d,%d) reference=%d cached=%d p6=%d\n",
            strict, nx, ny, want, got, sc64_contact_p6_active());
    describe_unit("mover", qi);
    describe_blockers(qi, nx, ny, strict);
}

static void
report_mask(const Unit *q, int speed, unsigned want, unsigned got, int inactive)
{
    int qi = (int)(q - u);
    int k;
    ++mask_fail;
    note_failure_mover(qi);
    if (++failures > REPORT_LIMIT) return;
    fprintf(stderr, "mask mismatch: speed=%d bucket_active=%d unit_step_clear=0x%02x "
            "mask=0x%02x p6=%d\n", speed, !inactive, want, got,
            sc64_contact_p6_active());
    describe_unit("mover", qi);
    for (k = 0; k < 8; ++k)
        if ((want ^ got) & (1u << k))
            describe_blockers(qi, q->x + sc64_contact_dx[k] * speed,
                              q->y + sc64_contact_dy[k] * speed, 0);
}

static int
movable(const Unit *q)
{
    return q->alive && !SC64_TRANSPORT_LOADED(q);
}

static void
check_step(const Unit *q, int nx, int ny)
{
    int want = sc64_unit_bucket_step_clear_reference(q, nx, ny, 0);
    int got = sc64_unit_bucket_step_clear(q, nx, ny);
    if (want != got) report_step(q, nx, ny, 0, want, got);
    ++step_checks;
#ifdef SC64_NYDUS_ENABLED
    want = sc64_unit_bucket_step_clear_reference(q, nx, ny, 1);
    got = sc64_unit_bucket_placement_clear(q, nx, ny);
    if (want != got) report_step(q, nx, ny, 1, want, got);
    ++strict_checks;
#endif
}

static void
check_mask(const Unit *q, int speed)
{
    unsigned want = 0, got;
    int k;
    for (k = 0; k < 8; ++k)
        if (!unit_step_clear(q, q->x + sc64_contact_dx[k] * speed,
                             q->y + sc64_contact_dy[k] * speed))
            want |= 1u << k;
    got = sc64_contact_blocked_mask(q, speed);
    if (want != got) report_mask(q, speed, want, got, !sc64_unit_bucket_active);
    if (sc64_unit_bucket_active) ++mask_checks;
    else ++inactive_mask_checks;
}

static void
check_unit(int i)
{
    static const int delta[3] = {-2, 0, 2};
    const Unit *q = &u[i];
    int step;
    if (!movable(q)) return;
    if (sc64_unit_bucket_active) {
        for (step = 0; step < 9; ++step)
            check_step(q, q->x + delta[step % 3], q->y + delta[step / 3]);
        for (step = 0; step < 4; ++step)
            check_step(q, q->x + (int)(next_random() % 41u) - 20,
                       q->y + (int)(next_random() % 41u) - 20);
    }
    check_mask(q, 1);
    check_mask(q, 2);
}

static int
type_with_raw(int raw)
{
    int t;
    for (t = 0; t < SC_RU_COUNT; ++t)
        if (sc64_race_raw_unit(t) == raw) return t;
    return -1;
}

static int
random_type(void)
{
    static const int preferred[] = {U_MARINE, U_ZERGLING, U_SCV, U_DRONE, U_PROBE,
                                    U_WRAITH, U_MUTALISK, U_CHRYSALIS, U_DROPSHIP,
                                    U_ZEALOT, U_OVERLORD};
    unsigned pick = next_random() % 8u;
    if (pick < 4u)
        return preferred[next_random() % (sizeof(preferred) / sizeof(preferred[0]))];
    if (pick < 6u) {
        static const int burrow_raw[] = {37, 38, 50};
        int t = type_with_raw(burrow_raw[next_random() % 3u]);
        if (t >= 0) return t;
    }
    return (int)(next_random() % (unsigned)SC_RU_COUNT);
}

static int
random_raw(int type)
{
    switch (next_random() % 8u) {
    case 0: return sc64_race_raw_unit(type);
    case 1: return 37;
    case 2: return 38;
    case 3: return 50;
    case 4: return (int)(next_random() % 230u);
    default: return -1;
    }
}

static int
clamp_coord(int v, int limit)
{
    return v < 0 ? 0 : v >= limit ? limit - 1 : v;
}

static void
place_unit(int i, int cx, int cy, int spread)
{
    u[i].x = clamp_coord(cx + (int)(next_random() % (unsigned)(2 * spread + 1)) - spread, WORLD_W);
    u[i].y = clamp_coord(cy + (int)(next_random() % (unsigned)(2 * spread + 1)) - spread, WORLD_H);
}

static void
randomize_unit(int i)
{
    int type = random_type();
    u[i].type = type;
    u[i].raw_type = random_raw(type);
    u[i].flags = (next_random() % 3u) == 0 ? 2 : 0;
    u[i].order = ORDER_IDLE;
    if (IS_WORKER(type) && (next_random() & 3u) == 0)
        u[i].order = (next_random() & 1u) ? ORDER_MINE : ORDER_GAS;
#ifdef SC64_TRANSPORT_RUNTIME
    if ((next_random() % 16u) == 0) u[i].order = ORDER_TRANSPORTED;
#endif
}

static void
build_world(int state)
{
    const int corner[][2] = {{0, 0}, {WORLD_W - 1, 0}, {0, WORLD_H - 1},
                                    {WORLD_W - 1, WORLD_H - 1}, {WORLD_W / 2, 0},
                                    {0, WORLD_H / 2}};
    int clusters = 3 + (int)(next_random() % 5u), i;
    int cx[8], cy[8];
    memset(u, 0, sizeof(u));
    for (i = 0; i < clusters; ++i) {
        if (i < 2 || (next_random() & 1u)) {
            int c = (int)(next_random() % (sizeof(corner) / sizeof(corner[0])));
            cx[i] = corner[c][0]; cy[i] = corner[c][1];
        } else {
            cx[i] = (int)(next_random() % WORLD_W); cy[i] = (int)(next_random() % WORLD_H);
        }
    }
    for (i = 0; i < MAX_UNITS; ++i) {
        u[i].alive = (next_random() % 6u) != 0;
        randomize_unit(i);
        if ((next_random() % 5u) == 0) place_unit(i, (int)(next_random() % WORLD_W),
                                                  (int)(next_random() % WORLD_H), 0);
        else {
            int c = (int)(next_random() % (unsigned)clusters);
            place_unit(i, cx[c], cy[c], (state & 1) ? 10 : 24);
        }
    }
}

static void
note_linked(void)
{
    int i;
    for (i = 0; i < MAX_UNITS; ++i) linked[i] = movable(&u[i]);
}

/* Keep the per-pass index consistent the way the runtime does. The legacy
 * index only holds slots linked at the start of the pass; the persistent one
 * reconciles lifecycle changes through sc64_unit_bucket_move(). */
static void
touch(int i)
{
#ifdef SC64_PERSISTENT_COLLISION
    sc64_unit_bucket_move(&u[i]);
#else
    if (linked[i]) sc64_unit_bucket_move(&u[i]);
#endif
}

static int
may_relink(int i)
{
#ifdef SC64_PERSISTENT_COLLISION
    (void)i;
    return 1;
#else
    return linked[i];
#endif
}

static void
mutate(void)
{
    int j = (int)(next_random() % MAX_UNITS);
    ++mutations;
    switch (next_random() % 7u) {
    case 0: /* morph in place */
        u[j].type = random_type();
        if ((next_random() & 1u)) u[j].raw_type = -1;
        break;
    case 1: /* death */
        u[j].alive = 0;
        touch(j);
        break;
    case 2: /* slot reuse with another type at a new position */
        if (!may_relink(j) || u[j].alive) break;
        u[j].alive = 1;
        randomize_unit(j);
#ifdef SC64_TRANSPORT_RUNTIME
        u[j].order = ORDER_IDLE;
#endif
        place_unit(j, u[j].x, u[j].y, 20);
        touch(j);
        break;
    case 3: /* raw identity pinned or released */
        u[j].raw_type = u[j].raw_type >= 0 ? -1 : random_raw(u[j].type);
        break;
    case 4: /* burrow toggle */
        u[j].flags ^= 2;
        break;
    case 5: /* load or unload */
#ifdef SC64_TRANSPORT_RUNTIME
        if (u[j].order == ORDER_TRANSPORTED) {
            if (!may_relink(j)) break;
            u[j].order = ORDER_IDLE;
        } else u[j].order = ORDER_TRANSPORTED;
        touch(j);
#endif
        break;
    default: /* cross bucket cells */
        if (!may_relink(j)) break;
        place_unit(j, u[j].x, u[j].y, 24);
        touch(j);
        break;
    }
}

static void
set_p6(int on)
{
#ifdef SC64_P6_COMBAT_ENABLED
    race_mode = 0;
    race_selected_mission = on ? SC64_P6_GLOBAL_ID : SC64_P6_GLOBAL_ID + 1;
    if (sc64_contact_p6_active()) ++p6_passes;
#else
    (void)on;
#endif
}

static void
world_passes(void)
{
    int state, pass, i;
    for (state = 0; state < 400; ++state) {
        build_world(state);
        set_p6(state & 1);
        for (pass = 0; pass < 3; ++pass) {
            sc64_unit_buckets_begin();
            assert(sc64_unit_bucket_active);
            note_linked();
            ++passes;
            for (i = 0; i < MAX_UNITS; ++i) {
                check_unit(i);
                if ((next_random() & 3u) == 0) {
                    mutate();
                    check_unit(i);
                }
            }
            /* Bucket-inactive fallback: the mask must match the full scan. */
            sc64_unit_bucket_active = 0;
            for (i = 0; i < MAX_UNITS; i += 3) check_unit(i);
            sc64_unit_buckets_end();
            /* Between passes: morphs and slot reuse without cache resets. */
            for (i = 0; i < 24; ++i) {
                int j = (int)(next_random() % MAX_UNITS);
                u[j].alive = (next_random() & 3u) != 0;
                randomize_unit(j);
            }
        }
    }
}

#ifndef SC64_PERSISTENT_COLLISION
/* Legacy index deactivates itself when its dirty-cell list would overflow. */
static void
dirty_overflow(void)
{
    int i, moves = 0;
    memset(u, 0, sizeof(u));
    for (i = 0; i < MAX_UNITS; ++i) {
        u[i].alive = 1;
        randomize_unit(i);
        u[i].order = ORDER_IDLE;
        u[i].x = (i % MAP_COLS) * 16 + 8;
        u[i].y = (i / MAP_COLS) * 16 + 8;
    }
    /* Crowd neighbors next to unit 0 so the fallback mask has blockers. */
    for (i = 1; i < 6; ++i) { u[i].x = u[0].x + i; u[i].y = u[0].y + (i & 1); }
    sc64_unit_buckets_begin();
    note_linked();
    assert(sc64_unit_bucket_active);
    while (sc64_unit_bucket_active && moves < MAP_CELLS) {
        int j = 6 + moves % (MAX_UNITS - 6);
        int cell = MAX_UNITS + moves;
        u[j].x = (cell % MAP_COLS) * 16 + 8;
        u[j].y = (cell / MAP_COLS) * 16 + 8;
        if (u[j].y >= WORLD_H) break;
        sc64_unit_bucket_move(&u[j]);
        ++moves;
    }
    assert(!sc64_unit_bucket_active);
    for (i = 0; i < 6; ++i) check_unit(i);
    sc64_unit_buckets_end();
}
#endif

static void
stock_one(int raw, unsigned tick, unsigned slot, int diagonal)
{
    unsigned want = sc64_stock_motion_step(raw, tick, slot, diagonal);
    unsigned got = sc64_contact_stock_step(raw, tick, slot, diagonal);
    if (want != got) ++stock_fail;
    if (want != got && ++failures <= REPORT_LIMIT)
        fprintf(stderr, "stock mismatch raw=%d tick=%u slot=%u diagonal=%d want=%u got=%u\n",
                raw, tick, slot, diagonal, want, got);
    ++stock_checks;
}

static void
stock_rates(void)
{
    static const int extra[] = {-32769, -32768, -1000, 32767, 32768, 70000, 0x7fffffff,
                                -0x7fffffff - 1};
    static const unsigned ticks[] = {0, 1, 2, 66, 67, 511, 1022, 1023, 1024, 4095,
                                     65535, 0xfffffffeu, 0xffffffffu};
    int raw, d, k;
    unsigned t, s;
    for (raw = -48; raw < 320; ++raw)
        for (t = 0; t < sizeof(ticks) / sizeof(ticks[0]); ++t)
            for (s = 0; s < MAX_UNITS; s += 17)
                for (d = 0; d < 2; ++d) stock_one(raw, ticks[t], s, d);
    for (k = 0; k < (int)(sizeof(extra) / sizeof(extra[0])); ++k)
        for (t = 0; t < 1100; t += 13)
            for (d = 0; d < 2; ++d) stock_one(extra[k], t, (unsigned)k, d);
    /* Direct-mapped slot collisions: raw, raw+16 and raw+32 share a slot. */
    for (k = 0; k < 20000; ++k) {
        int base = (int)(next_random() % 240u) - 16;
        int r = base + 16 * (int)(next_random() % 3u);
        stock_one(r, next_random(), next_random() % MAX_UNITS, (int)(next_random() & 1u));
    }
    for (t = 0; t < 2048; ++t)
        for (k = 0; k < 3; ++k) stock_one(5 + 16 * k, t, t % MAX_UNITS, (int)(t & 1u));
}

int
main(void)
{
    stock_rates();
    world_passes();
#ifndef SC64_PERSISTENT_COLLISION
    dirty_overflow();
#endif
    printf("sc64 contact cache: %lu step, %lu strict, %lu mask, %lu inactive-mask, "
           "%lu stock checks; %lu passes (%lu P6-active), %lu mutations; "
           "%lu failures\n", step_checks, strict_checks, mask_checks, inactive_mask_checks,
           stock_checks, passes, p6_passes, mutations, failures);
    if (failures)
        printf("failures: step in-map %lu, step off-map %lu, mask %lu, stock %lu; "
               "flying movers %lu, ground movers %lu\n", step_fail_inmap,
               step_fail_offmap, mask_fail, stock_fail, fail_flying_mover,
               fail_ground_mover);
    assert(failures == 0);
    return 0;
}
