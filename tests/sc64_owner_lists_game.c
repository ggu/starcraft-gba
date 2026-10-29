/* SPDX-License-Identifier: GPL-3.0-or-later
 * Exercise the real owner-list rebuild and its invalid-owner fallback. */
#define SC64_OWNER_LIST_TEST 1
#define SC64_OWNER_LIST_TEST_ONLY 1
static void sc64_owner_lists_test_probe(void);
#include "../starcraft/game.c"
#include <stdio.h>
#include <assert.h>
#include <string.h>

static void sc64_owner_lists_test_probe(void)
{
    int bad=0;
    for(int j=0;j<MAX_UNITS;j++)
        if(u[j].alive && !SC64_TRANSPORT_LOADED(&u[j]) &&
           (u[j].owner<0 || u[j].owner>=5)) bad=1;
    for(int j=0;j<MAX_BUILDINGS;j++)
        if(b[j].alive && (b[j].owner<0 || b[j].owner>=5)) bad=1;
    assert(!!target_lists_valid != !!bad);
    if(!target_lists_valid) return;
    for(int owner=0;owner<5;owner++) {
        int at=target_unit_head[owner];
        for(int j=0;j<MAX_UNITS;j++)
            if(u[j].alive && !SC64_TRANSPORT_LOADED(&u[j]) && u[j].owner==owner) {
                assert(at==j); at=target_unit_next[at];
            }
        assert(at==-1);
        at=target_building_head[owner];
        for(int j=0;j<MAX_BUILDINGS;j++)
            if(b[j].alive && b[j].owner==owner) {
                assert(at==j); at=target_building_next[at];
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
    game_init();
    memset(u, 0, sizeof(u));
    memset(b, 0, sizeof(b));
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
        int owner = b[0].owner, alive = b[0].alive;
        if (invalid) { b[0].alive = 1; b[0].owner = 9; }
        update_orders(); /* Test-only hook checks complete ordered membership. */
        if (invalid) { b[0].owner = owner; b[0].alive = alive; }
    }
    puts("owner lists: 10000 actual rebuilds passed, including invalid owners");
    return 0;
}
