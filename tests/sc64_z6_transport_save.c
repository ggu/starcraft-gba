/* Focused, asset-free candidate-save checks for Z6 Overlord cargo links. */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define SC64_Z6_TRANSPORT_ENABLED 1
#define SC64_Z6_RUNTIME 1
#define SC64_TRANSPORT_RUNTIME 1
#define SC64_ZP_FULL_STORAGE 1
#define MAX_UNITS 10
#define SIDE 0
#define ENEMY 1
#define R_ZERG 3
#define SC64_Z6_GLOBAL_ID 17
#define ORDER_IDLE 0
#define ORDER_MOVE 1
#define ORDER_BOARD 8
#define ORDER_TRANSPORTED 9
#define ORDER_UNLOAD 10
#define SC64_TRANSPORT_UNLOAD_TICKS 19

#include "../common/gba.h"
typedef struct {
    int x, y, tx, ty, work_timer, path_timer, path_retry, target;
    int raw_type, raw_owner;
    u16 flags;
    u8 alive, owner, type, order, selected;
} Unit;
enum { U_OVERLORD = 1, U_DRONE, U_ZERGLING, U_HYDRALISK, U_ULTRALISK, U_KERRIGAN,
       U_MUTALISK };
typedef struct {
    int state[8];
    Unit units[MAX_UNITS];
} RaceSave;

#include "../starcraft/sc64_z6_transport_tech.h"

static int sc64_race_unit_from_raw(int raw)
{
    switch (raw) {
    case 37: return U_ZERGLING;
    case 38: return U_HYDRALISK;
    case 39: return U_ULTRALISK;
    case 41: return U_DRONE;
    case 42: return U_OVERLORD;
    case 43: return U_MUTALISK;
    case 51: return U_KERRIGAN;
    default: return -1;
    }
}
static int sc64_race_unit_flying(int type)
{ return type == U_OVERLORD || type == U_MUTALISK; }
static int race_save_decode_unit(const RaceSave *save, unsigned slot,
                                 unsigned width, unsigned height, Unit *out)
{
    if (!save || !out || slot >= MAX_UNITS) return 0;
    *out = save->units[slot];
    return !out->alive || (out->x >= 0 && out->y >= 0 &&
                           (unsigned)out->x < width && (unsigned)out->y < height);
}

#include "../starcraft/sc64_z6_transport_save.h"

static RaceSave fresh(void)
{
    RaceSave s;
    memset(&s, 0, sizeof(s));
    s.state[0] = 0; s.state[1] = R_ZERG; s.state[2] = SC64_Z6_GLOBAL_ID;
    return s;
}
static void set_actor(RaceSave *s, int slot, int raw, int type, int x, int y)
{
    Unit *q = &s->units[slot];
    memset(q, 0, sizeof(*q));
    q->alive = 1; q->owner = SIDE; q->raw_owner = 3;
    q->raw_type = raw; q->type = type; q->x = x; q->y = y;
    q->target = -1; q->order = ORDER_IDLE;
}
static Sc64Z6TransportTechState researched(void)
{
    Sc64Z6TransportTechState t = {0, 0, 1};
    return t;
}

static void test_valid_board_transported_and_unload(void)
{
    RaceSave s = fresh();
    Sc64Z6TransportTechState tech = researched();
    set_actor(&s, 0, 42, U_OVERLORD, 80, 96);
    set_actor(&s, 1, 51, U_KERRIGAN, 80, 96);
    s.units[1].order = ORDER_TRANSPORTED;
    s.units[1].target = 0;
    assert(race_z6_transport_save_candidate_valid(&s, &tech, 128, 128));

    s.units[1].order = ORDER_BOARD;
    assert(race_z6_transport_save_candidate_valid(&s, &tech, 128, 128));
    s.units[1].path_timer = 25;
    assert(!race_z6_transport_save_candidate_valid(&s, &tech, 128, 128));
    s.units[1].path_timer = 0;
    s.units[1].order = ORDER_TRANSPORTED;
    s.units[0].order = ORDER_UNLOAD;
    s.units[0].target = -1;
    s.units[0].tx = 128; s.units[0].ty = 144;
    s.units[0].work_timer = SC64_TRANSPORT_UNLOAD_TICKS;
    assert(race_z6_transport_save_candidate_valid(&s, &tech, 128, 128));
    s.units[1].order = ORDER_BOARD;
    assert(race_z6_transport_save_candidate_valid(&s, &tech, 128, 128));
}

static void test_research_mission_identity_and_links(void)
{
    RaceSave s = fresh();
    Sc64Z6TransportTechState tech = researched();
    set_actor(&s, 0, 42, U_OVERLORD, 80, 96);
    set_actor(&s, 1, 51, U_KERRIGAN, 80, 96);
    s.units[1].order = ORDER_TRANSPORTED;
    s.units[1].target = 0;
    tech.researched = 0;
    assert(!race_z6_transport_save_candidate_valid(&s, &tech, 128, 128));
    tech = researched();
    s.state[2] = 16;
    assert(!race_z6_transport_save_candidate_valid(&s, &tech, 128, 128));
    s = fresh(); set_actor(&s, 0, 42, U_OVERLORD, 80, 96);
    set_actor(&s, 1, 51, U_KERRIGAN, 80, 96);
    s.units[1].order = ORDER_TRANSPORTED; s.units[1].target = MAX_UNITS;
    assert(!race_z6_transport_save_candidate_valid(&s, &tech, 128, 128));
    s.units[1].target = 1;
    assert(!race_z6_transport_save_candidate_valid(&s, &tech, 128, 128));

    /* P5 and Z5 share the mission-state union: do not interpret its bytes as
     * Z6 research state when there are no Z6 transport orders. */
    s = fresh(); s.state[2] = 16;
    tech.researched = 2; tech.ticks_remaining = 65535;
    assert(race_z6_transport_save_candidate_valid(&s, &tech, 128, 128));
    set_actor(&s, 0, 42, U_OVERLORD, 80, 96);
    s.units[0].order = ORDER_UNLOAD; s.units[0].target = -1;
    s.units[0].tx = 128; s.units[0].ty = 128;
    assert(!race_z6_transport_save_candidate_valid(&s, &tech, 128, 128));
}

static void test_capacity_type_owner_burrow_and_bounds(void)
{
    RaceSave s = fresh();
    Sc64Z6TransportTechState tech = researched();
    set_actor(&s, 0, 42, U_OVERLORD, 80, 96);
    for (int i = 1; i <= 3; ++i) {
        set_actor(&s, i, 39, U_ULTRALISK, 80, 96);
        s.units[i].order = ORDER_TRANSPORTED;
        s.units[i].target = 0;
    }
    assert(!race_z6_transport_save_candidate_valid(&s, &tech, 128, 128));
    s.units[1].alive = 0;
    assert(race_z6_transport_save_candidate_valid(&s, &tech, 128, 128));
    s.units[2].flags |= 2;
    assert(!race_z6_transport_save_candidate_valid(&s, &tech, 128, 128));
    s.units[2].flags = 0; s.units[2].raw_owner = 2;
    assert(!race_z6_transport_save_candidate_valid(&s, &tech, 128, 128));
    s.units[2].raw_owner = 3; s.units[2].type = U_MUTALISK;
    assert(!race_z6_transport_save_candidate_valid(&s, &tech, 128, 128));
    s.units[2].type = U_ULTRALISK; s.units[2].x = 2048;
    assert(!race_z6_transport_save_candidate_valid(&s, &tech, 128, 128));

    s = fresh(); set_actor(&s, 0, 42, U_OVERLORD, 80, 96);
    for (int i = 1; i <= 3; ++i) {
        set_actor(&s, i, 39, U_ULTRALISK, 80, 96);
        s.units[i].order = ORDER_BOARD; s.units[i].target = 0;
    }
    assert(race_z6_transport_save_candidate_valid(&s, &tech, 128, 128));
}

static void test_unload_reference_and_timer(void)
{
    RaceSave s = fresh();
    Sc64Z6TransportTechState tech = researched();
    set_actor(&s, 0, 42, U_OVERLORD, 80, 96);
    set_actor(&s, 1, 41, U_DRONE, 80, 96);
    s.units[1].order = ORDER_TRANSPORTED; s.units[1].target = 0;
    s.units[0].order = ORDER_UNLOAD; s.units[0].tx = 128; s.units[0].ty = 144;
    s.units[0].target = -1;
    assert(race_z6_transport_save_candidate_valid(&s, &tech, 128, 128));
    s.units[0].work_timer = SC64_TRANSPORT_UNLOAD_TICKS + 1;
    assert(!race_z6_transport_save_candidate_valid(&s, &tech, 128, 128));
    s.units[0].work_timer = 0; s.units[0].tx = 2048;
    assert(!race_z6_transport_save_candidate_valid(&s, &tech, 128, 128));
    s.units[0].alive = 0;
    assert(!race_z6_transport_save_candidate_valid(&s, &tech, 128, 128));

    s = fresh(); set_actor(&s, 0, 42, U_OVERLORD, 80, 96);
    s.units[0].order = ORDER_UNLOAD; s.units[0].target = -1;
    s.units[0].tx = 128; s.units[0].ty = 144;
    assert(race_z6_transport_save_candidate_valid(&s, &tech, 128, 128));
    set_actor(&s, 1, 41, U_DRONE, 80, 96);
    s.units[1].order = ORDER_BOARD; s.units[1].target = 0;
    s.units[1].path_timer = -1;
    assert(!race_z6_transport_save_candidate_valid(&s, &tech, 128, 128));
    s.units[1].path_timer = 0; s.units[1].path_retry = -1;
    assert(!race_z6_transport_save_candidate_valid(&s, &tech, 128, 128));
}

int main(void)
{
    test_valid_board_transported_and_unload();
    test_research_mission_identity_and_links();
    test_capacity_type_owner_burrow_and_bounds();
    test_unload_reference_and_timer();
    puts("Z6 transport candidate save: ok");
    return 0;
}
