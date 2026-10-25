/* P7 movement differential and steering-list oracle.
 *
 * Every build enters P7 through the menu, runs the neutral crowding opening,
 * then issues one ordinary attack-move with controller input only. The
 * per-tick FNV trace covers Unit, Building, P7 runtime and wallet state; the
 * reference and SC64_COLLISION_STEER_LIST builds must print the same trace.
 * The candidate also compares every list answer with unit_step_clear() on
 * live states and on randomized dense crowds. */
#include "../starcraft/game.c"
#include <assert.h>
#include <stdio.h>

static void tap(u16 key)
{
    game_update(key, key);
    game_update(0, 0);
}

static unsigned fnv(unsigned h, const void *data, size_t length)
{
    const unsigned char *p = data;
    for (size_t i = 0; i < length; ++i) { h ^= p[i]; h *= 16777619u; }
    return h;
}

static unsigned state_hash(void)
{
    unsigned h = 2166136261u;
    h = fnv(h, &race_p7, sizeof(race_p7));
    h = fnv(h, u, sizeof(u));
    h = fnv(h, b, sizeof(b));
    h = fnv(h, &frame, sizeof(frame));
    h = fnv(h, race_minerals, sizeof(race_minerals));
    h = fnv(h, race_gas, sizeof(race_gas));
    return h;
}

static unsigned live_units(void)
{
    unsigned n = 0;
    for (int i = 0; i < MAX_UNITS; ++i) n += u[i].alive != 0;
    return n;
}

#ifdef SC64_COLLISION_STEER_LIST
static const s8 oracle_dx[8] = {1,1,0,-1,-1,-1,0,1};
static const s8 oracle_dy[8] = {0,1,1,1,0,-1,-1,-1};
static unsigned long oracle_queries, oracle_builds, oracle_fallbacks, oracle_blocked;

/* Compare the list with the unchanged query for every ground mover. */
static void steer_oracle_pass(void)
{
    sc64_unit_buckets_begin();
    assert(sc64_unit_bucket_active);
    for (int i = 0; i < MAX_UNITS; ++i) {
        const Unit *q = &u[i];
        if (!q->alive || SC64_TRANSPORT_LOADED(q) || sc64_race_unit_flying(q->type))
            continue;
        for (int speed = 1; speed <= 2; ++speed) {
            if (!sc64_steer_list_build(q, speed)) { ++oracle_fallbacks; continue; }
            ++oracle_builds;
            for (int k = 0; k < 8; ++k) {
                int sx = q->x + oracle_dx[k] * speed, sy = q->y + oracle_dy[k] * speed;
                int expected = unit_step_clear(q, sx, sy);
                assert(sc64_steer_list_step_clear(q, sx, sy) == expected);
                ++oracle_queries;
                oracle_blocked += !expected;
            }
        }
    }
    sc64_unit_buckets_end();
}

static unsigned rng_state = 0x5eed1234u;
static unsigned rng(void)
{
    rng_state = rng_state * 1103515245u + 12345u;
    return rng_state >> 8;
}

/* Pack live units into small clusters, including map edges, and exercise
 * worker gathering orders; restore the world afterwards. */
static void steer_oracle_random_worlds(int rounds)
{
    static Unit saved[MAX_UNITS];
    memcpy(saved, u, sizeof(u));
    for (int r = 0; r < rounds; ++r) {
        int cx = (int)(rng() % WORLD_W), cy = (int)(rng() % WORLD_H);
        int spread = 4 + (int)(rng() % 40);
        if (r % 7 == 0) cx = (r & 8) ? WORLD_W - 1 - (int)(rng() % 6) : (int)(rng() % 6);
        if (r % 11 == 0) cy = (r & 16) ? WORLD_H - 1 - (int)(rng() % 6) : (int)(rng() % 6);
        for (int i = 0; i < MAX_UNITS; ++i) {
            if (!u[i].alive || (rng() & 3) == 0) continue;
            u[i].x = (s16)limit(cx + (int)(rng() % (2 * spread + 1)) - spread, 0, WORLD_W - 1);
            u[i].y = (s16)limit(cy + (int)(rng() % (2 * spread + 1)) - spread, 0, WORLD_H - 1);
            if (IS_WORKER(u[i].type) && (rng() & 1))
                u[i].order = (rng() & 1) ? ORDER_MINE : ORDER_GAS;
        }
        steer_oracle_pass();
        memcpy(u, saved, sizeof(u));
    }
}
#endif

int main(void)
{
    unsigned trace = 2166136261u, h;
    game_init();
    tap(KEY_A);
    tap(KEY_RIGHT); tap(KEY_RIGHT); tap(KEY_RIGHT);
    tap(KEY_A);
    game_update(KEY_A, KEY_A);
    assert(screen == SCREEN_PLAY && race_selected_mission == SC64_P7_GLOBAL_ID);
    assert(race_p7_active() && race_p7_runtime_state_valid());
    for (int t = 0; t < 1200; ++t) {
        game_update(0, 0);
        assert(screen == SCREEN_PLAY && !race_p7.runtime.fault);
        h = state_hash();
        trace = fnv(trace, &h, sizeof(h));
#ifdef SC64_COLLISION_STEER_LIST
        if (t % 50 == 49) steer_oracle_pass();
#endif
    }
    printf("P7 neutral trace %08x frame=%d units=%u\n", trace, frame, live_units());

    /* Controller-only attack-move: R army, B menu, DOWN to Attack, A. */
    tap(KEY_R);
    tap(KEY_B);
    tap(KEY_DOWN);
    tap(KEY_A);
    assert(mode == MODE_ATTACK);
    {
        int target = -1, best = 0x7fffffff, ax = 0, ay = 0, n = 0;
        for (int i = 0; i < MAX_UNITS; ++i)
            if (u[i].alive && u[i].selected) { ax += u[i].x; ay += u[i].y; ++n; }
        assert(n > 0);
        ax /= n; ay /= n;
        for (int i = 0; i < MAX_BUILDINGS; ++i)
            if (b[i].alive && b[i].owner == 1 && dist2(ax, ay, b[i].x, b[i].y) < best) {
                best = dist2(ax, ay, b[i].x, b[i].y);
                target = i;
            }
        assert(target >= 0);
        for (int guard = 0; guard < 2000; ++guard) {
            u16 keys = 0;
            if (cx < b[target].x - 2) keys |= KEY_RIGHT;
            else if (cx > b[target].x + 2) keys |= KEY_LEFT;
            if (cy < b[target].y - 2) keys |= KEY_DOWN;
            else if (cy > b[target].y + 2) keys |= KEY_UP;
            if (!keys) break;
            game_update(keys, 0);
        }
        game_update(0, 0);
        tap(KEY_A);
        assert(mode == MODE_NORMAL);
    }
    for (int t = 0; t < 1500 && screen == SCREEN_PLAY; ++t) {
        game_update(0, 0);
        assert(!race_p7.runtime.fault);
        h = state_hash();
        trace = fnv(trace, &h, sizeof(h));
#ifdef SC64_COLLISION_STEER_LIST
        if (t % 50 == 49 && screen == SCREEN_PLAY) steer_oracle_pass();
#endif
    }
    printf("P7 attack-move trace %08x frame=%d screen=%d won=%d lost=%d units=%u\n",
           trace, frame, screen, won, lost, live_units());
#ifdef SC64_COLLISION_STEER_LIST
    steer_oracle_random_worlds(400);
    fprintf(stderr, "steer oracle: %lu queries (%lu blocked), %lu builds, %lu fallbacks\n",
            oracle_queries, oracle_blocked, oracle_builds, oracle_fallbacks);
    assert(oracle_blocked > 0 && oracle_fallbacks > 0 && oracle_queries > oracle_blocked);
#endif
    return 0;
}
