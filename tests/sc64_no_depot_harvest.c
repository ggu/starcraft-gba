/* Base-less native mineral-trip behavior over the real current engine path. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

typedef struct {
    int race;
    int worker_type;
    int depot_type;
    int gas_type;
} WorkerProfile;

static void reset_fixture(int race)
{
    game_init();
    memset(u, 0, sizeof(u));
    memset(b, 0, sizeof(b));
    memset(m, 0, sizeof(m));
#ifdef SC64_RACES
    race_mode = 1;
    race_human = race;
    race_bots = 0;
    memset(race_player, 0xff, sizeof(race_player));
    race_player[SIDE] = race;
    race_player[ENEMY] = race;
#else
    (void)race;
#endif
    minerals = gas = 0;
    supply_used = 0;
    supply_cap = 200;
    frame = 1;
    won = lost = 0;
    path_grid_valid = 0;
    invalidate_path_grid();
}

static void set_depot(int slot, int owner, int type, int incomplete)
{
    memset(&b[slot], 0, sizeof(b[slot]));
    b[slot].alive = 1;
    b[slot].owner = owner;
    b[slot].type = type;
    b[slot].x = 400;
    b[slot].y = 408;
    b[slot].maxhp = b[slot].hp = 1000;
    b[slot].under_construction = incomplete;
}

static void check_base_less_mineral_trip(const WorkerProfile *p)
{
    reset_fixture(p->race);
    int worker = spawn_unit(SIDE, p->worker_type, 400, 400);
    assert(worker >= 0);
    m[0].x = 404; m[0].y = 400; m[0].amount = 24;
    u[worker].order = ORDER_MINE;
    u[worker].target = 0;
    u[worker].build = -1;

    for (int tick = 0; tick < 240 && !u[worker].carry; ++tick) {
        update_orders();
        ++frame;
    }
    assert(owner_command_center(SIDE) == -1);
    assert(u[worker].order == ORDER_MINE);
    assert(u[worker].carry > 0);
    assert(minerals == 0);

    int cargo = u[worker].carry;
    m[0].amount = 0; /* The completed mineral trip must outlive its patch. */
    for (int tick = 0; tick < 20; ++tick) {
        update_orders();
        ++frame;
    }
    assert(u[worker].order == ORDER_MINE);
    assert(u[worker].carry == cargo);
    assert(minerals == 0);

    /* Neither an enemy depot nor an unfinished friendly depot can receive it. */
    set_depot(0, ENEMY, p->depot_type, 0);
    set_depot(1, SIDE, p->depot_type, 1);
    for (int tick = 0; tick < 20; ++tick) {
        update_orders();
        ++frame;
    }
    assert(owner_command_center(SIDE) == -1);
    assert(u[worker].carry == cargo);
    assert(minerals == 0);

    set_depot(1, SIDE, p->depot_type, 0);
    assert(owner_command_center(SIDE) == 1);
    for (int tick = 0; tick < 100 && u[worker].carry; ++tick) {
        update_orders();
        ++frame;
    }
    assert(u[worker].carry == 0);
    assert(minerals == cargo);
    assert(u[worker].order == ORDER_MINE);
}

static void check_gas_order_unchanged(const WorkerProfile *p)
{
    reset_fixture(p->race);
    int worker = spawn_unit(SIDE, p->worker_type, 400, 400);
    assert(worker >= 0);
    set_depot(0, SIDE, p->gas_type, 0);
    u[worker].order = ORDER_GAS;
    u[worker].target = 0;
    u[worker].build = -1;
    update_orders();
    assert(owner_command_center(SIDE) == -1);
    assert(u[worker].order == ORDER_IDLE);
    assert(u[worker].carry == 0);
    assert(gas == 0);
}

int main(void)
{
#ifdef SC64_RACES
    const WorkerProfile profiles[] = {
        {R_TER, U_SCV, B_COMMAND, B_REFINERY},
        {R_PROTOSS, U_PROBE, B_NEXUS, B_ASSIMILATOR},
        {R_ZERG, U_DRONE, B_HATCHERY, B_EXTRACTOR},
    };
#else
    const WorkerProfile profiles[] = {
        {0, U_SCV, B_COMMAND, B_REFINERY},
    };
#endif
    for (unsigned i = 0; i < sizeof(profiles) / sizeof(profiles[0]); ++i) {
        check_base_less_mineral_trip(&profiles[i]);
        check_gas_order_unchanged(&profiles[i]);
    }
    puts("base-less mineral harvest/return checks passed");
    return 0;
}
