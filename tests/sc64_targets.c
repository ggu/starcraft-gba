/* Differential coverage for the temporary SC64 race target broadphase.
 * Compile with the private race import headers using the test-races flags. */
#include <assert.h>
#include "../starcraft/game.c"

static int reference_target(int x, int y, int owner, int range, int attacker)
{
    int best = -1, bestd = range * range;
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive &&
        FAST_ABS(x - u[i].x) < range && FAST_ABS(y - u[i].y) < range &&
        owners_hostile(owner, u[i].owner) && race_can_attack(attacker, i)) {
        int d = dist2(x, y, u[i].x, u[i].y);
        if (d < bestd || (best >= 0 && d == bestd && i < best)) {
            best = i; bestd = d;
        }
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive &&
        FAST_ABS(x - b[i].x) < range && FAST_ABS(y - b[i].y) < range &&
        owners_hostile(owner, b[i].owner) &&
        race_can_attack(attacker, MAX_UNITS + i)) {
        int candidate = MAX_UNITS + i, d = dist2(x, y, b[i].x, b[i].y);
        if (d < bestd || (best >= 0 && d == bestd && candidate < best)) {
            best = candidate; bestd = d;
        }
    }
    return best;
}

static void build_owner_lists(void)
{
    for (int i = 0; i < 5; ++i) target_unit_head[i] = target_building_head[i] = -1;
    for (int i = 0; i < MAX_UNITS; ++i) target_unit_next[i] = -1;
    for (int i = 0; i < MAX_BUILDINGS; ++i) target_building_next[i] = -1;
    for (int i = MAX_UNITS - 1; i >= 0; --i) if (u[i].alive) {
        int owner = u[i].owner;
        assert(owner >= 0 && owner < 5);
        target_unit_next[i] = target_unit_head[owner]; target_unit_head[owner] = i;
    }
    for (int i = MAX_BUILDINGS - 1; i >= 0; --i) if (b[i].alive) {
        int owner = b[i].owner;
        assert(owner >= 0 && owner < 5);
        target_building_next[i] = target_building_head[owner]; target_building_head[owner] = i;
    }
    target_lists_valid = 1;
}

static void compare_query(int x, int y, int owner, int range, int attacker)
{
    int expected = reference_target(x, y, owner, range, attacker);
    int indexed = nearest_enemy_typed(x, y, owner, range, attacker);
    assert(indexed == expected);
}

static unsigned random_state = 0x64c0ffeeu;
static unsigned next_random(void)
{
    random_state = random_state * 1664525u + 1013904223u;
    return random_state;
}

static void test_profiled_attack_queries(void)
{
    /* Populate mixed target classes at deterministic pseudo-random locations,
       then compare every attacker rule against the original per-target scan. */
    for (int i = 0; i < MAX_UNITS; ++i) {
        u[i].alive = (next_random() & 3u) != 0;
        u[i].owner = next_random() % 5u;
        u[i].type = next_random() % SC_RU_COUNT;
        u[i].flags = (next_random() & 7u) == 0 ? 16 : 0;
        u[i].x = next_random() % WORLD_W;
        u[i].y = next_random() % WORLD_H;
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i) {
        b[i].alive = (next_random() & 3u) != 0;
        b[i].owner = next_random() % 5u;
        b[i].type = next_random() % SC_RB_COUNT;
        b[i].flags = (next_random() & 7u) == 0 ? 16 : 0;
        b[i].x = next_random() % WORLD_W;
        b[i].y = next_random() % WORLD_H;
    }
    build_owner_lists();
    sc64_target_index_begin();
    assert(sc64_target_index_active);
    for (int attacker = -1; attacker < SC_RU_COUNT; ++attacker) {
        int ground, air;
        race_attack_profile(attacker, &ground, &air);
        for (int target = 0; target < MAX_UNITS + MAX_BUILDINGS; ++target)
            assert(race_can_attack_profiled(target, ground, air) ==
                   race_can_attack(attacker, target));
        for (int query = 0; query < 12; ++query) {
            int x = next_random() % WORLD_W;
            int y = next_random() % WORLD_H;
            int owner = next_random() % 5u;
            int range = 1 + next_random() % 300u;
            compare_query(x, y, owner, range, attacker);
        }
    }
    /* Mission 2's carried Chrysalis exception is independent of the cached
       attacker profile and must continue to match the original predicate. */
    u[0].alive = 1; u[0].owner = ENEMY; u[0].type = U_CHRYSALIS;
    u[0].flags = 0; u[0].x = 384; u[0].y = 384;
    sc64_target_index_begin();
    assert(sc64_target_index_active);
    race_mode = 0; race_selected_mission = 1;
    race_campaign.carrier_unit = 1;
    for (int attacker = -1; attacker < SC_RU_COUNT; ++attacker)
        compare_query(384, 384, SIDE, 64, attacker);
    race_mode = 1;
    sc64_target_index_begin();
    assert(sc64_target_index_active);
}

int main(void)
{
    int moving;
    gba_init(); game_init();
    race_mode = 1; race_human = R_TER; race_bots = 2;
    race_skirmish_init(); screen = SCREEN_PLAY;
    memset(u, 0, sizeof(u)); memset(b, 0, sizeof(b));

    /* Crowd several neighboring cells with mixed owners and attack classes;
     * deterministic positions include strict range boundaries and ties. */
    for (int i = 0; i < 72; ++i) {
        int owner = i % 3 == 0 ? SIDE : i % 3 == 1 ? ENEMY : RESCUABLE;
        int type = (i % 5 == 0) ? U_MUTALISK : (i % 2 ? U_ZERGLING : U_MARINE);
        int x = 180 + (i % 12) * 64, y = 120 + (i / 12) * 64;
        int slot = spawn_unit(owner, type, x, y);
        assert(slot == i);
        if (i % 17 == 0) u[slot].flags |= 16;
    }
    for (int i = 0; i < 18; ++i) {
        int owner = i % 2 ? ENEMY : RESCUABLE;
        int type = i % 2 ? B_HATCHERY : B_BARRACKS;
        assert(spawn_build(owner, type, 200 + (i % 6) * 128,
                           420 + (i / 6) * 96, 0) >= 0);
    }
    /* Deliberate unit/unit/building tie: global target ordering prefers the
     * lower unit slot, even though bucket list insertion is reverse order. */
    assert(spawn_unit(ENEMY, U_ZERGLING, 780, 800) == 72);
    assert(spawn_unit(RESCUABLE, U_MARINE, 820, 800) == 73);
    assert(spawn_build(ENEMY, B_HATCHERY, 820, 840, 0) >= 0);
    /* A lone exact-boundary target must remain out of strict range. */
    assert(spawn_unit(ENEMY, U_WRAITH, 888, 900) == 74);
    /* Fill the remaining unit and building slots densely in one 64px cell,
     * including T3's combined target index 223 (unit 159 / building 63). */
    for (int i = 75; i < MAX_UNITS; ++i) {
        int owner = i % 3 == 0 ? SIDE : i % 3 == 1 ? ENEMY : RESCUABLE;
        int type = i % 2 ? U_ZERGLING : U_MARINE;
        assert(spawn_unit(owner, type, 500 + (i % 4), 500 + (i % 4)) == i);
    }
    for (int i = 19; i < MAX_BUILDINGS; ++i) {
        int owner = i % 2 ? ENEMY : RESCUABLE;
        int type = i % 2 ? B_HATCHERY : B_BARRACKS;
        assert(spawn_build(owner, type, 500 + (i % 4), 500 + (i % 4), 0) == i);
    }
    build_owner_lists();
    sc64_target_index_begin();
    assert(sc64_target_index_active);
    for (int range_index = 0; range_index < 5; ++range_index) {
        int range = (int[]){ 1, 52, 64, 112, 128 }[range_index];
        for (int owner = SIDE; owner <= RESCUABLE; ++owner)
            for (int attacker_index = 0; attacker_index < 3; ++attacker_index) {
                int attacker = (int[]){ U_MARINE, U_ZERGLING, U_DRAGOON }[attacker_index];
                compare_query(200 + range_index * 64, 160 + owner * 32,
                              owner, range, attacker);
            }
    }
    assert(nearest_enemy_typed(800, 820, SIDE, 64, U_MARINE) == 72);
    assert(nearest_enemy_typed(1000, 900, SIDE, 112, U_MARINE) == -1);

    test_profiled_attack_queries();

    /* The entity moves across bucket boundaries during the indexed pass; its
     * old cell must stop returning it and its new cell must return it. */
    moving = 0;
    u[moving].flags = 0;
    u[moving].x = 900; u[moving].y = 900;
    sc64_target_index_move(&u[moving]);
    compare_query(900, 900, ENEMY, 10, U_MARINE);
    compare_query(180, 120, ENEMY, 10, U_MARINE);

    /* Death remains an immediate filter without rebuilding the index. */
    u[moving].alive = 0;
    compare_query(900, 900, ENEMY, 10, U_MARINE);

    /* Keep only the highest unit and building slots alive to cover the
     * uint8 sentinel edge and unified global-index ordering. */
    for (int i = 0; i < MAX_UNITS; ++i) u[i].alive = i == MAX_UNITS - 1;
    for (int i = 0; i < MAX_BUILDINGS; ++i) b[i].alive = i == MAX_BUILDINGS - 1;
    u[MAX_UNITS - 1].owner = ENEMY; u[MAX_UNITS - 1].type = U_MARINE;
    u[MAX_UNITS - 1].x = 500; u[MAX_UNITS - 1].y = 500; u[MAX_UNITS - 1].flags = 0;
    b[MAX_BUILDINGS - 1].owner = ENEMY; b[MAX_BUILDINGS - 1].type = B_HATCHERY;
    b[MAX_BUILDINGS - 1].x = 500; b[MAX_BUILDINGS - 1].y = 500;
    sc64_target_index_begin();
    assert(sc64_target_index_active);
    compare_query(500, 500, SIDE, 64, U_MARINE);
    assert(nearest_enemy_typed(500, 500, SIDE, 64, U_MARINE) == MAX_UNITS - 1);
    u[MAX_UNITS - 1].alive = 0;
    compare_query(500, 500, SIDE, 64, U_MARINE);
    assert(nearest_enemy_typed(500, 500, SIDE, 64, U_MARINE) ==
           MAX_UNITS + MAX_BUILDINGS - 1);

    /* An out-of-bounds entity disables the broadphase and preserves the
     * original owner-list scan as a safe fallback. */
    u[MAX_UNITS - 1].alive = 1; u[MAX_UNITS - 1].x = -1;
    sc64_target_index_begin();
    assert(!sc64_target_index_active);
    compare_query(200, 160, ENEMY, 128, U_MARINE);
    sc64_target_index_end();

    /* Excessive repeated cross-cell moves exceed the bounded dirty-head
     * list. The broadphase disables itself and the next begin fully clears
     * every head before rebuilding, rather than retaining stale links. */
    for (int i = 0; i < MAX_UNITS; ++i) u[i].alive = 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i) b[i].alive = 0;
    u[0].alive = 1; u[0].owner = SIDE; u[0].type = U_MARINE; u[0].flags = 0;
    u[1].alive = 1; u[1].owner = ENEMY; u[1].type = U_ZERGLING; u[1].flags = 0;
    u[0].x = 32; u[0].y = 32; u[1].x = 96; u[1].y = 32;
    build_owner_lists();
    sc64_target_index_begin();
    assert(sc64_target_index_active);
    for (int entity = 0; entity < 2 && sc64_target_index_active; ++entity)
        for (int row = 0; row < 16 && sc64_target_index_active; ++row)
            for (int col = 0; col < 16 && sc64_target_index_active; ++col) {
                u[entity].x = col * 64 + 32;
                u[entity].y = row * 64 + 32;
                sc64_target_index_move(&u[entity]);
            }
    assert(!sc64_target_index_active);
    sc64_target_index_begin();
    assert(sc64_target_index_active);
    u[0].x = 32; u[0].y = 32; sc64_target_index_move(&u[0]);
    u[1].x = 96; u[1].y = 32; sc64_target_index_move(&u[1]);
    compare_query(32, 32, ENEMY, 64, U_MARINE);
    assert(nearest_enemy_typed(32, 32, ENEMY, 64, U_MARINE) == 0);
    sc64_target_index_end();
    target_lists_valid = 0;
    return 0;
}
