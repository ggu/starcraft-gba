/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Asset-free exercise of the actual packed mineral queue runtime. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define SC64_PACKED_MINERAL_FIFO 1
#define SC64_NEAREST_RESOURCE_DEPOT 1
#define SC64_P6_COMBAT_ENABLED 1
#define SC64_T6_RUNTIME 1
#define MAX_UNITS 8
#define MAX_MINERALS 21
#define WORLD_W 256
#define WORLD_H 256
#define MINERAL_WORK_RANGE 12
#define ORDER_MINE 1
#define ORDER_GAS 2
#define ORDER_TRANSPORTED 3
#define IS_WORKER(t) ((t) == 1)
#define SC64_TRANSPORT_LOADED(q) ((q)->order == ORDER_TRANSPORTED)
typedef uint8_t u8;
typedef struct { int alive, type, order, target, x, y, carry, gas_cargo, pending, build, work_timer; } Unit;
typedef struct { int x, y, amount; } Mineral;
static Unit u[MAX_UNITS];
static Mineral m[MAX_MINERALS];
static int burrow_p[MAX_UNITS], burrow_t[MAX_UNITS];
static int race_p6_combat_unit_burrowed(int i) { return burrow_p[i]; }
static int race_t6_unit_burrowed(int i) { return burrow_t[i]; }
static int sc64_worker_cargo_matches_order(const Unit *q) {
    return q->carry > 0 && ((q->order == ORDER_GAS) == q->gas_cargo);
}
static int sc64_worker_gather_pending(const Unit *q) { return q->pending; }
static int dist2(int x, int y, int a, int b) { return (x-a)*(x-a)+(y-b)*(y-b); }
#include "../starcraft/sc64_mineral_fifo.h"
static Sc64MineralFifo mineral_fifo;
#include "../starcraft/sc64_mineral_fifo_runtime.h"

static void reset_scene(void) {
    memset(u, 0, sizeof(u)); memset(m, 0, sizeof(m));
    memset(&mineral_fifo, 0, sizeof(mineral_fifo));
    memset(burrow_p, 0, sizeof(burrow_p)); memset(burrow_t, 0, sizeof(burrow_t));
    m[0] = (Mineral){100,100,100}; m[1] = (Mineral){110,100,100};
    for (int i=0; i<MAX_UNITS; ++i)
        u[i] = (Unit){.alive=1,.type=1,.order=ORDER_MINE,.target=0,.x=100,.y=100,.build=-1};
    worker_mineral_fifo_begin();
}
static void test_fifo_and_foreign_cargo(void) {
    reset_scene();
    u[2].carry=8; u[2].gas_cargo=1;
    assert(worker_mineral_fifo_live_slot_at_patch(2, &u[2], &m[0], 0));
    assert(!worker_mineral_fifo_ready(&u[2]));
    assert(!worker_mineral_fifo_ready(&u[1]));
    assert(mineral_fifo.head[0]==3 && mineral_fifo.tail[0]==2);
    assert(u[2].work_timer==44 && u[1].work_timer==0);
    for (int tick=0; tick<43; ++tick) {
        worker_mineral_fifo_begin();
        assert(!worker_mineral_fifo_ready(&u[1]));
        assert(!worker_mineral_fifo_ready(&u[2]));
    }
    worker_mineral_fifo_begin();
    assert(worker_mineral_fifo_ready(&u[2]));
    assert(!worker_mineral_fifo_ready(&u[1]) && u[1].work_timer==0);
    worker_mineral_fifo_begin();
    assert(!worker_mineral_fifo_ready(&u[1]) && u[1].work_timer==44);
}
static void test_prune_and_retarget(void) {
    reset_scene();
    assert(!worker_mineral_fifo_ready(&u[0]));
    assert(!worker_mineral_fifo_ready(&u[1]));
    u[0].target=1; worker_mineral_fifo_begin();
    assert(mineral_fifo.head[0]==2 && !mineral_fifo.patch[0] && !u[0].work_timer);
    assert(!worker_mineral_fifo_ready(&u[0]) && u[0].work_timer==44);
    u[0].carry=8; u[0].gas_cargo=0; worker_mineral_fifo_begin();
    assert(!mineral_fifo.patch[0] && !u[0].work_timer);
    u[1].order=ORDER_GAS; u[1].work_timer=37; worker_mineral_fifo_begin();
    assert(!mineral_fifo.patch[1] && u[1].work_timer==37);
    assert(!mineral_fifo.head[0] && !mineral_fifo.head[1]);
}
static void test_eligibility_and_reset(void) {
    reset_scene();
    u[0].build=-2; assert(!worker_mineral_fifo_live_slot_at_patch(0, &u[0], &m[0], 0));
    u[0].pending=1; assert(worker_mineral_fifo_live_slot_at_patch(0, &u[0], &m[0], 0));
    u[0].pending=0; u[0].build=-1; u[0].carry=8;
    assert(!worker_mineral_fifo_live_slot_at_patch(0, &u[0], &m[0], 0));
    u[0].gas_cargo=1; assert(worker_mineral_fifo_live_slot_at_patch(0, &u[0], &m[0], 0));
    u[0].alive=0; assert(!worker_mineral_fifo_live_slot_at_patch(0, &u[0], &m[0], 0));
    u[0].alive=1; u[0].x=200; assert(!worker_mineral_fifo_live_slot_at_patch(0, &u[0], &m[0], 0));
    u[0].x=100; m[0].amount=0; assert(!worker_mineral_fifo_live_slot_at_patch(0, &u[0], &m[0], 0));
    m[0].amount=100; m[0].x=-1; assert(!worker_mineral_fifo_live_slot_at_patch(0, &u[0], &m[0], 0));
    m[0].x=100; assert(!worker_mineral_fifo_ready(&u[0]));
    burrow_p[0]=1; worker_mineral_fifo_begin(); assert(!mineral_fifo.patch[0]);
    burrow_p[0]=0; assert(!worker_mineral_fifo_ready(&u[0]));
    burrow_t[0]=1; worker_mineral_fifo_begin(); assert(!mineral_fifo.patch[0]);
    reset_scene(); assert(!mineral_fifo.head[0] && !mineral_fifo_serviced[0]);
}

static void test_prune_spans(void)
{
    const unsigned patches[MAX_UNITS] = {0, 3, 4, 6, 7, 15, 20, 20};
    for (unsigned mask = 0; mask < (1u << MAX_UNITS); ++mask) {
        reset_scene();
        for (unsigned slot = 0; slot < MAX_UNITS; ++slot) {
            if (!(mask & (1u << slot))) continue;
            unsigned patch = patches[slot];
            m[patch] = (Mineral){100, 100, 100};
            u[slot].target = (int)patch;
            u[slot].work_timer = 19;
            assert(sc64_mineral_fifo_append(&mineral_fifo, patch, slot));
        }
        m[3].amount = 0;
        u[3].target = 20;
        u[6].alive = 0;
        burrow_t[4] = 1;

        Sc64MineralFifo scalar = mineral_fifo;
        Unit scalar_units[MAX_UNITS];
        memcpy(scalar_units, u, sizeof(u));
        for (unsigned patch = 0; patch < MAX_MINERALS; ++patch)
            worker_mineral_fifo_prune_patch(&scalar, scalar_units, m, patch,
                                            scalar.head[patch]);

        worker_mineral_fifo_begin();
        assert(!memcmp(mineral_fifo.head, scalar.head, sizeof(scalar.head)));
        assert(!memcmp(mineral_fifo.tail, scalar.tail, sizeof(scalar.tail)));
        assert(!memcmp(mineral_fifo.next, scalar.next, sizeof(scalar.next)));
        assert(!memcmp(mineral_fifo.patch, scalar.patch, sizeof(scalar.patch)));
        assert(!memcmp(u, scalar_units, sizeof(u)));
    }
}

int main(void) {
    test_fifo_and_foreign_cargo(); test_prune_and_retarget(); test_eligibility_and_reset();
    test_prune_spans();
    puts("packed mineral FIFO runtime: ok"); return 0;
}
