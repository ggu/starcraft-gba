/* SPDX-License-Identifier: GPL-3.0-or-later
 * Exercise the real owner-list rebuild and its invalid-owner fallback. */
#define SC64_OWNER_LIST_TEST 1
#define SC64_OWNER_LIST_TEST_ONLY 1
static void sc64_owner_lists_test_probe(void);
#include "../starcraft/game.c"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

static unsigned long long trace = 1469598103934665603ULL;
static int exact_trace;
static void record(int v) {
    trace = (trace ^ (unsigned)v) * 1099511628211ULL;
    if (exact_trace && fwrite(&v, sizeof(v), 1, stdout) != 1) abort();
}

static void sc64_owner_lists_test_probe(void)
{
    int bad=0;
    for(int j=0;j<MAX_UNITS;j++)
        if(u[j].alive && !SC64_TRANSPORT_LOADED(&u[j]) && !SC64_SCARAB_HIDDEN(&u[j]) &&
           (u[j].owner<0 || u[j].owner>=5)) bad=1;
    for(int j=0;j<MAX_BUILDINGS;j++)
        if(b[j].alive && (b[j].owner<0 || b[j].owner>=5)) bad=1;
    assert(!!target_lists_valid != !!bad);
    record(target_lists_valid);
    for (int owner = 0; owner < 5; ++owner)
        record(nearest_enemy_scan(128 + owner * 13, 128, owner, 512, -1
                                  SC64_P7_SOURCE_OWNER_ARG(-1)));
    if(!target_lists_valid) return;
    for(int owner=0;owner<5;owner++) {
        int at=target_unit_head[owner];
        for(int j=0;j<MAX_UNITS;j++)
            if(u[j].alive && !SC64_TRANSPORT_LOADED(&u[j]) && !SC64_SCARAB_HIDDEN(&u[j]) && u[j].owner==owner) {
                assert(at==j); record(at); at=target_unit_next[at];
            }
        assert(at==-1);
        at=target_building_head[owner];
        for(int j=0;j<MAX_BUILDINGS;j++)
            if(b[j].alive && b[j].owner==owner) {
                assert(at==j); record(at); at=target_building_next[at];
            }
        assert(at==-1);
    }
}

static unsigned seed = 12345;
static unsigned random_value(void)
{
    seed = seed * 1664525u + 1013904223u;
    return seed >> 8;
}

int main(void)
{
    exact_trace = getenv("SC64_OWNER_EXACT_TRACE") != NULL;
    game_init();
    memset(u, 0, sizeof(u));
    memset(b, 0, sizeof(b));
    /* Both final slots must link correctly, including the -1 tail. */
    u[MAX_UNITS - 1].alive = 1; u[MAX_UNITS - 1].owner = 4;
    b[MAX_BUILDINGS - 1].alive = 1; b[MAX_BUILDINGS - 1].owner = 4;
    update_orders();
    memset(u, 0, sizeof(u)); memset(b, 0, sizeof(b));
#ifdef SC64_SCARAB_RUNTIME
    /* Hidden units must not make an invalid owner poison the index. */
    u[MAX_UNITS - 1].alive = 1; u[MAX_UNITS - 1].owner = 9;
    u[MAX_UNITS - 1].type = U_SCARAB; u[MAX_UNITS - 1].carry = 0;
    update_orders();
    memset(u, 0, sizeof(u));
#endif
    for (int tick = 0; tick < 10000; ++tick) {
        for (int j = 0; j < 4; ++j) {
            int i = random_value() % MAX_UNITS;
            u[i].alive = random_value() % 2;
            u[i].owner = random_value() % 5;
            u[i].order = random_value() % 7 == 0 ? ORDER_TRANSPORTED : ORDER_IDLE;
            i = random_value() % MAX_BUILDINGS;
            b[i].alive = random_value() % 2;
            b[i].owner = random_value() % 5;
        }
        int invalid = tick % 7 == 0;
        int badunit = tick % 11 == 0;
        int uowner = u[0].owner, ualive = u[0].alive, uorder = u[0].order, utype = u[0].type;
        if (badunit) { u[0].alive = 1; u[0].owner = 9; u[0].order = ORDER_IDLE; u[0].type = 0; }
        int owner = b[0].owner, alive = b[0].alive;
        if (invalid) { b[0].alive = 1; b[0].owner = 9; }
        update_orders(); /* Test-only hook checks complete ordered membership. */
        if (invalid) { b[0].owner = owner; b[0].alive = alive; }
        if (badunit) { u[0].owner = uowner; u[0].alive = ualive; u[0].order = uorder; u[0].type = utype; }
    }
    if (!exact_trace) printf("owner lists: 10000 rebuilds and queries; trace=%016llx\n", trace);
    return 0;
}
