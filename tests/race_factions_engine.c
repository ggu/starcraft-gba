/* Live multi-race simulation coverage for the authored GBA race adapter.
 * Build with SC64_RACES and private generated headers; see build command in
 * the test's companion run script/Makefile target. */
#include <assert.h>
#include "../starcraft/game.c"

static int find_building(int owner, int type)
{
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == owner && b[i].type == type) return i;
    return -1;
}

static int find_worker(int owner, int type)
{
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == owner && u[i].type == type) return i;
    return -1;
}

/* Independent brute-force reference for the defense tower's strict weapon
 * range, owner, scenario-flag, flight-class, and slot-order rules. */
static int defense_reference_target(int building)
{
    Building *q = &b[building];
    int ground = 0, air = 0, range = 0, best = -1, bestd;
    switch (q->type) {
    case B_SUNKEN_COLONY: ground = 1; range = 112; break;
    case B_SPORE_COLONY: air = 1; range = 112; break;
    default: assert(0); break;
    }
    bestd = range * range;
    for (int j = 0; j < MAX_UNITS; ++j) if (u[j].alive &&
        owners_hostile(q->owner, u[j].owner) && !(u[j].flags & 16)) {
        int flying = sc64_race_unit_flying(u[j].type);
        int dx = q->x - u[j].x, dy = q->y - u[j].y;
        if ((flying && !air) || (!flying && !ground) || !race_can_attack(-1, j)) continue;
        if (dx * dx + dy * dy < bestd) { bestd = dx * dx + dy * dy; best = j; }
    }
    return best;
}

static void check_defense_targeting(int tower_type, const int *types,
                                    const int *xs, const int *ys,
                                    const int *flags, int count)
{
    int tower, expected, hp[MAX_UNITS];
    race_mode = 1; race_human = R_ZERG; race_bots = 1;
    race_skirmish_init(); screen = SCREEN_PLAY;
    memset(u, 0, sizeof(u));
    memset(b, 0, sizeof(b));
    tower = spawn_build(SIDE, tower_type, 200, 160, 0);
    assert(tower >= 0);
    for (int i = 0; i < count; ++i) {
        int slot = spawn_unit(ENEMY, types[i], xs[i], ys[i]);
        assert(slot == i);
        u[slot].flags = (u8)flags[i];
    }
    expected = defense_reference_target(tower);
    for (int i = 0; i < MAX_UNITS; ++i) hp[i] = u[i].hp;
    race_tick_defenses();
    for (int i = 0; i < count; ++i)
        /* These Sunken fixtures select Small, zero-armor Zerglings: its
         * stock 40 Explosive damage is halved against Small HP. Spore's
         * 15 Normal damage is unscaled against these zero-armor flyers. */
        assert(u[i].hp == hp[i] - (i == expected ?
            (tower_type == B_SUNKEN_COLONY ? 20 : 15) : 0));
}

static void test_defense_target_query(void)
{
    /* Sunken: nearest ground target wins; flagged and air targets are skipped.
     * The nearer air target also checks that flight rejection stays intact. */
    {
        const int types[] = { U_ZERGLING, U_MARINE, U_MUTALISK, U_ZERGLING };
        const int xs[] = { 240, 250, 201, 202 }, ys[] = { 160, 160, 160, 160 };
        const int flags[] = { 0, 0, 0, 16 };
        check_defense_targeting(B_SUNKEN_COLONY, types, xs, ys, flags, 4);
    }
    /* Spore: ground is ignored, exact range boundary is excluded, equal-range
     * air targets resolve to the lowest unit slot. */
    {
        const int types[] = { U_ZERGLING, U_MUTALISK, U_WRAITH, U_MUTALISK };
        const int xs[] = { 201, 312, 240, 240 }, ys[] = { 160, 160, 200, 120 };
        const int flags[] = { 0, 0, 0, 0 };
        check_defense_targeting(B_SPORE_COLONY, types, xs, ys, flags, 4);
    }
    /* Exact radius is not in range. */
    {
        const int types[] = { U_WRAITH };
        const int xs[] = { 312 }, ys[] = { 160 };
        const int flags[] = { 0 };
        check_defense_targeting(B_SPORE_COLONY, types, xs, ys, flags, 1);
    }
    /* The closest airborne candidate is invulnerable; the next eligible air
     * candidate must be selected instead. */
    {
        const int types[] = { U_MUTALISK, U_WRAITH, U_ZERGLING };
        const int xs[] = { 204, 240, 205 }, ys[] = { 160, 160, 160 };
        const int flags[] = { 16, 0, 0 };
        check_defense_targeting(B_SPORE_COLONY, types, xs, ys, flags, 3);
    }
}

int main(void)
{
    gba_init();
    game_init();
    test_defense_target_query();

    /* Mixed-race starts give each owner its actual worker, town hall, and
     * race-specific supply. The extra owners cycle through Z/T/P. */
    race_mode = 1; race_human = R_ZERG; race_bots = 2;
    race_skirmish_init(); screen = SCREEN_PLAY;
    assert(race_player[SIDE] == R_ZERG);
    assert(race_player[ENEMY] == R_PROTOSS);
    assert(race_player[RESCUABLE] == R_TER);
    assert(find_worker(SIDE, U_DRONE) >= 0);
    assert(find_worker(ENEMY, U_PROBE) >= 0);
    assert(find_worker(RESCUABLE, U_SCV) >= 0);
    assert(race_owner_supply_cap(SIDE) >= 18); /* Hatchery + Overlord. */
    assert(race_owner_supply_cap(ENEMY) >= 34); /* Nexus + Pylon. */
    assert(sc64_race_unit_flying(U_OVERLORD) && sc64_race_unit_flying(U_QUEEN));
    assert(sc64_race_unit_range(U_DRAGOON) == 128);

    /* Keep units from fighting so this integration check can follow each
     * economy and construction path without a test-only combat advantage. */
    for (int f = 0; f < 12000; ++f) {
        for (int i = 0; i < MAX_UNITS; ++i)
            if (u[i].alive && u[i].owner != SIDE && !sc64_race_is_worker(u[i].type) &&
                u[i].type != U_OVERLORD) u[i].alive = 0;
        game_update(0, 0);
    }
    assert(m[4].amount < 1500 || m[8].amount < 1500 || m[12].amount < 1500);
    assert(find_building(ENEMY, B_GATEWAY) >= 0);
    assert(find_building(ENEMY, B_ASSIMILATOR) >= 0);
    assert(find_building(RESCUABLE, B_BARRACKS) >= 0);
    assert(find_building(RESCUABLE, B_REFINERY) >= 0);
    assert(race_gas[ENEMY] > 0 || race_gas[RESCUABLE] > 0);

    /* Protoss structure power, worker return, unit production, shields and
     * supply are checked through the live queues and per-frame transitions. */
    race_human = R_PROTOSS; race_bots = 0;
    race_skirmish_init(); screen = SCREEN_PLAY;
    {
        int gateway = spawn_build(SIDE, B_GATEWAY, 208, 160, 0);
        int zealot, probe;
        assert(gateway >= 0 && race_site_powered(SIDE, b[gateway].x, b[gateway].y));
        assert(race_queue_unit(SIDE, gateway, U_ZEALOT));
        for (int f = 0; f < 800; ++f) race_tick_buildings();
        zealot = -1;
        for (int i = 0; i < MAX_UNITS; ++i)
            if (u[i].alive && u[i].owner == SIDE && u[i].type == U_ZEALOT) { zealot = i; break; }
        assert(zealot >= 0);
        u[zealot].shield = 0;
        for (int f = 0; f < 4000; ++f) race_tick_state();
        assert(u[zealot].shield > 0);
        for (int i = 0; i < MAX_BUILDINGS; ++i)
            if (b[i].alive && b[i].owner == SIDE && b[i].type == B_PYLON) b[i].alive = 0;
        assert(!race_site_powered(SIDE, b[gateway].x, b[gateway].y));
        assert(!race_queue_unit(SIDE, gateway, U_ZEALOT));

        /* A Probe returns after reaching the Gateway warp-in site, and that
         * building advances only after arrival. */
        assert(race_start_build(SIDE, find_worker(SIDE, U_PROBE), B_PYLON, 240, 240));
        probe = find_worker(SIDE, U_PROBE);
        assert(probe >= 0);
        {
            int pylon = u[probe].target;
            assert(pylon >= 0 && b[pylon].under_construction);
            u[probe].x = b[pylon].x; u[probe].y = b[pylon].y;
            assert(race_builder_arrived(probe));
            assert(u[probe].alive && u[probe].order == ORDER_IDLE);
            for (int f = 0; f < 600; ++f) race_tick_buildings();
            assert(building_complete(pylon));
        }
    }

    /* Zerg Drones are consumed at arrival; the Pool then progresses on the
     * race building clock and removes the worker's two supply. */
    race_human = R_ZERG; race_bots = 0;
    race_skirmish_init(); screen = SCREEN_PLAY;
    {
        int drone = find_worker(SIDE, U_DRONE), pool;
        int before = race_owner_supply(SIDE);
        assert(drone >= 0);
        assert(race_start_build(SIDE, drone, B_SPAWNING_POOL, 208, 160));
        pool = u[drone].target;
        assert(pool >= 0 && b[pool].under_construction);
        u[drone].x = b[pool].x; u[drone].y = b[pool].y;
        assert(race_builder_arrived(drone));
        assert(!u[drone].alive && race_owner_supply(SIDE) == before - 2);
        for (int f = 0; f < 1600; ++f) race_tick_buildings();
        assert(building_complete(pool));
    }

    /* Supply providers must remain queueable after a cap loss. The Zerg can
     * lose its Overlord while retaining more supply than the Hatchery cap;
     * producing a replacement Overlord is the recovery action. */
    race_human = R_ZERG; race_bots = 0;
    race_skirmish_init(); screen = SCREEN_PLAY;
    {
        int hatch = find_building(SIDE, B_HATCHERY), overlords = 0;
        assert(hatch >= 0);
        race_add_owner_resources(SIDE, 500, 0);
        for (int i = 0; i < MAX_UNITS; ++i)
            if (u[i].alive && u[i].owner == SIDE && u[i].type == U_OVERLORD) {
                u[i].alive = 0;
                ++overlords;
            }
        assert(overlords > 0);
        assert(race_owner_reserved_supply(SIDE) > race_owner_supply_cap(SIDE));
        assert(race_queue_unit(SIDE, hatch, U_OVERLORD));
        assert(b[hatch].queue_count == 1);
    }

    /* A short unsuppressed FFA exercises the actual 30 Hz command path and
     * verifies Zerg and Protoss do not count noncombat supply units as army. */
    race_human = R_TER; race_bots = 2;
    race_skirmish_init(); screen = SCREEN_PLAY;
    for (int f = 0; f < 6000; ++f) game_update(0, 0);
    {
        int owners[3] = { SIDE, ENEMY, RESCUABLE };
        for (int k = 0; k < 3; ++k) {
            int combat = 0;
            for (int i = 0; i < MAX_UNITS; ++i)
                if (u[i].alive && u[i].owner == owners[k] && sc64_race_unit_damage(u[i].type) > 0) ++combat;
            assert(combat > 0);
        }
    }

    /* Elimination is based on all buildings, not just the race's town hall. */
    race_human = R_ZERG; race_bots = 1;
    race_skirmish_init(); screen = SCREEN_PLAY;
    {
        int hall = find_building(ENEMY, B_NEXUS);
        assert(hall >= 0);
        b[hall].alive = 0;
        race_skirmish_outcome();
        assert(!won && !lost); /* the bot still has its Pylon */
        for (int i = 0; i < MAX_BUILDINGS; ++i)
            if (b[i].alive && b[i].owner == ENEMY) b[i].alive = 0;
        race_skirmish_outcome();
        assert(won && !lost);
    }
    return 0;
}
