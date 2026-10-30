/* Differential coverage for the temporary SC64 race target broadphase.
 * Compile with the private race import headers using the test-races flags. */
#include <assert.h>
#include "../starcraft/game.c"

static int reference_can_attack(int attacker, int target, int owner)
{
#if defined(SC64_P3_ENABLED) && defined(SC64_UNIFIED)
    /* The P3 Scourge override includes hostility/air-only constraints that
     * race_can_attack alone does not model. */
    return sc64_p3_can_attack_from(attacker, target, owner);
#elif defined(SC64_T4_RUNTIME)
    int expected = race_can_attack(attacker, target);
#ifdef SC64_T6_RUNTIME
    if (target >= 0 && target < MAX_UNITS && race_t6_unit_burrowed(target))
        expected = 0;
#endif
    /* T4's wrapper must preserve race_can_attack eligibility exactly; T6's
     * extra burrow rejection above is its only additional condition. */
    assert(sc64_t4_can_attack_from(attacker, target, owner) == expected);
    return expected;
#else
    (void)owner;
    return race_can_attack(attacker, target);
#endif
}

static int reference_target(int x, int y, int owner, int range, int attacker)
{
    int best = -1, bestd = range * range;
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive &&
        FAST_ABS(x - u[i].x) < range && FAST_ABS(y - u[i].y) < range &&
        (owner != SIDE || (u[i].x >= 0 && u[i].y >= 0 &&
         u[i].x < WORLD_W && u[i].y < WORLD_H &&
         SC64_GRID_GET(visible, u[i].x / 16 + (u[i].y / 16) * MAP_COLS))) &&
        owners_hostile(owner, u[i].owner) && reference_can_attack(attacker, i, owner)) {
        int d = dist2(x, y, u[i].x, u[i].y);
        if (d < bestd || (best >= 0 && d == bestd && i < best)) {
            best = i; bestd = d;
        }
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive &&
        FAST_ABS(x - b[i].x) < range && FAST_ABS(y - b[i].y) < range &&
        (owner != SIDE || (b[i].x >= 0 && b[i].y >= 0 &&
         b[i].x < WORLD_W && b[i].y < WORLD_H &&
         SC64_GRID_GET(visible, b[i].x / 16 + (b[i].y / 16) * MAP_COLS))) &&
        owners_hostile(owner, b[i].owner) &&
        reference_can_attack(attacker, MAX_UNITS + i, owner)) {
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

static void compare_attacker_matrix(void)
{
    static const int queries[][4] = {
        { 200, 160, SIDE, 128 },
        { 780, 800, SIDE, 64 },
        { 520, 520, ENEMY, 112 },
        { 1000, 900, SIDE, 112 }
    };
    /* Include -1's wildcard contract and every compact unit ID, especially
     * IDs with no ground/air weapon. */
    for (int q = 0; q < (int)(sizeof(queries) / sizeof(queries[0])); ++q)
        for (int attacker = -1; attacker < SC_RU_COUNT; ++attacker)
            compare_query(queries[q][0], queries[q][1], queries[q][2],
                          queries[q][3], attacker);
}

static void test_player_auto_acquisition_fog(void)
{
    int before_hidden, before_visible;
    memset(u, 0, sizeof(u)); memset(b, 0, sizeof(b));
    SC64_GRID_CLEAR(visible);
    target_lists_valid = 0;
    sc64_target_index_end();

    /* A closer unseen unit must not mask the farther visible unit. */
    u[0].alive = u[1].alive = 1;
    u[0].owner = u[1].owner = ENEMY;
    u[0].type = u[1].type = U_ZERGLING;
    u[0].x = 416; u[0].y = 400;
    u[1].x = 448; u[1].y = 400;
    SC64_GRID_SET(visible, 448 / 16 + (400 / 16) * MAP_COLS, 1);
    assert(nearest_enemy_typed(400, 400, SIDE, 96, U_MARINE) == 1);
    /* The player cannot auto-acquire when every candidate is hidden. */
    SC64_GRID_CLEAR(visible);
    assert(nearest_enemy_typed(400, 400, SIDE, 96, U_MARINE) == -1);
    /* In FFA, AI owner 4 is hostile to owner 1 and acquires AI-vs-AI targets
       independently of the player's fog. */
    assert(owners_hostile(4, ENEMY));
    assert(nearest_enemy_typed(400, 400, 4, 96, U_MARINE) == 0);
    u[2].alive = 1; u[2].owner = SIDE; u[2].type = U_ZERGLING;
    u[2].x = 416; u[2].y = 400;
    assert(nearest_enemy_typed(400, 400, ENEMY, 96, U_MARINE) == 2);

    /* Revealing a unit enables acquisition; visibility loss removes it again. */
    SC64_GRID_SET(visible, 416 / 16 + (400 / 16) * MAP_COLS, 1);
    assert(nearest_enemy_typed(400, 400, SIDE, 96, U_MARINE) == 0);
    SC64_GRID_SET(visible, 416 / 16 + (400 / 16) * MAP_COLS, 0);
    SC64_GRID_SET(visible, 448 / 16 + (400 / 16) * MAP_COLS, 1);
    assert(nearest_enemy_typed(400, 400, SIDE, 96, U_MARINE) == 1);

    /* Hidden buildings are filtered by the same shared candidate predicate. */
    b[0].alive = 1; b[0].owner = ENEMY; b[0].type = B_HATCHERY;
    b[0].x = 408; b[0].y = 400;
    assert(nearest_enemy_typed(400, 400, SIDE, 96, U_MARINE) == 1);
    SC64_GRID_SET(visible, 408 / 16 + (400 / 16) * MAP_COLS, 1);
    assert(nearest_enemy_typed(400, 400, SIDE, 96, U_MARINE) == MAX_UNITS);
    SC64_GRID_CLEAR(visible);
    assert(nearest_enemy_typed(400, 400, 4, 96, U_MARINE) == MAX_UNITS);

    /* Auto-acquisition does not rewrite an explicit last-seen target order;
     * already-released attacks still enter the ordinary hit resolver. */
    u[3].alive = 1; u[3].owner = SIDE; u[3].type = U_MARINE;
    u[3].x = 400; u[3].y = 400; u[3].cd = 0;
    u[3].order = ORDER_ATTACK_TARGET; u[3].target = 0;
    u[0].hp = u[0].maxhp = 40;
    before_hidden = u[0].hp;
    SC64_GRID_SET(visible, 448 / 16 + (400 / 16) * MAP_COLS, 1);
    assert(nearest_enemy_typed(400, 400, SIDE, 96, U_MARINE) == 1);
    assert(u[3].order == ORDER_ATTACK_TARGET && u[3].target == 0);
    hit_target(0, 1, U_MARINE, SIDE);
    assert(u[0].hp < before_hidden);

    /* Actual idle controller path: the visible target takes a hit while the
     * closer hidden candidate is ignored. Available in imported campaign
     * target fixtures, whose idle defense calls nearest_enemy_typed(). */
#ifdef SC64_IMPORTED_MISSION
    memset(u, 0, sizeof(u)); memset(b, 0, sizeof(b));
    SC64_GRID_CLEAR(visible);
    u[0].alive = u[1].alive = u[2].alive = 1;
    u[0].owner = u[1].owner = ENEMY; u[2].owner = SIDE;
    u[0].type = u[1].type = u[2].type = U_MARINE;
    u[0].x = 416; u[0].y = 400;
    u[1].x = 448; u[1].y = 400;
    u[2].x = 400; u[2].y = 400;
    u[0].hp = u[0].maxhp = 40;
    u[1].hp = u[1].maxhp = 40;
    u[2].hp = u[2].maxhp = 40;
    u[0].cd = u[1].cd = u[2].cd = 0;
    u[0].order = u[1].order = u[2].order = ORDER_IDLE;
    u[0].target = u[1].target = u[2].target = -1;
    SC64_GRID_SET(visible, 448 / 16 + (400 / 16) * MAP_COLS, 1);
    assert(!currently_visible(u[0].x, u[0].y));
    assert(currently_visible(u[1].x, u[1].y));
    before_hidden = u[0].hp; before_visible = u[1].hp;
    game_update(0, 0);
    assert(u[0].hp == before_hidden);
    assert(u[1].hp < before_visible);
#else
    (void)before_visible;
#endif
    for (int cell = 0; cell < MAP_CELLS; ++cell) SC64_GRID_SET(visible, cell, 1);
}

int main(void)
{
    int moving;
    gba_init(); game_init();
    race_mode = 1; race_human = R_TER; race_bots = 2;
    race_skirmish_init(); screen = SCREEN_PLAY;
    memset(u, 0, sizeof(u)); memset(b, 0, sizeof(b));
    /* Preserve the historical target oracle's all-visible setup; dedicated
     * fog cases below toggle individual cells independently. */
    for (int cell = 0; cell < MAP_CELLS; ++cell) SC64_GRID_SET(visible, cell, 1);

    /* Crowd several neighboring cells with mixed owners and attack classes;
     * deterministic positions include strict range boundaries and ties. */
    for (int i = 0; i < 72; ++i) {
        int owner = i % 5;
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
        int owner = i % 5;
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
#if defined(SC64_LATE_TERRAN_STORAGE) || defined(SC64_TARGET_OCCUPANCY)
    sc64_target_hostility_snapshot();
    assert(target_hostile_masks_valid);
    for (int owner = 0; owner < 5; ++owner)
        for (int other = 0; other < 5; ++other)
            assert(!!(target_hostile_mask[owner] & (1u << other)) ==
                   !!owners_hostile(owner, other));
#endif
    compare_attacker_matrix();
#ifdef SC64_TARGET_OCCUPANCY
    /* Cross every owner slot with every compact attacker, covering both air
     * and ground weapon predicates through the indexed query and scan oracle. */
    for (int owner = 0; owner < 5; ++owner)
        for (int attacker = -1; attacker < SC_RU_COUNT; ++attacker)
            compare_query(780, 800, owner, 128, attacker);
#endif
    for (int range_index = 0; range_index < 5; ++range_index) {
        int range = (int[]){ 1, 52, 64, 112, 128 }[range_index];
        for (int owner = 0; owner < 5; ++owner)
            for (int attacker_index = 0; attacker_index < 3; ++attacker_index) {
                int attacker = (int[]){ U_MARINE, U_ZERGLING, U_DRAGOON }[attacker_index];
                compare_query(200 + range_index * 64, 160 + owner * 32,
                              owner, range, attacker);
            }
    }
    assert(nearest_enemy_typed(800, 820, SIDE, 64, U_MARINE) == 72);
    assert(nearest_enemy_typed(1000, 900, SIDE, 112, U_MARINE) == -1);

    /* Exercise the same attacker matrix through the owner-list fallback and
     * the original full-array scan, not just the indexed broadphase. */
    sc64_target_index_end();
    compare_attacker_matrix();
    target_lists_valid = 0;
    compare_attacker_matrix();
    build_owner_lists();
    sc64_target_index_begin();
    assert(sc64_target_index_active);
#ifdef SC64_UNARMED_QUERY_SKIP
    {
        int unarmed = -1;
        for (int attacker = 0; attacker < SC_RU_COUNT; ++attacker)
            if (sc64_race_unit_damage_vs(attacker, 0) <= 0 &&
                sc64_race_unit_damage_vs(attacker, 1) <= 0) {
                unarmed = attacker;
                break;
            }
        assert(unarmed >= 0);
        compare_query(780, 800, SIDE, 128, unarmed);
        assert(nearest_enemy_typed(780, 800, SIDE, 128, unarmed) == -1);
        compare_query(780, 800, SIDE, 128, -1);
        assert(nearest_enemy_typed(780, 800, SIDE, 128, -1) == 72);
    }
#endif
#if defined(SC64_P3_ENABLED) && defined(SC64_UNIFIED)
    /* The air-only Scourge must survive the no-weapon early-return check and
     * find an enemy Mutalisk, while the wildcard remains unchanged. */
    assert(sc64_race_unit_damage_vs(U_SCOURGE, 0) == 0);
    assert(sc64_race_unit_damage_vs(U_SCOURGE, 1) > 0);
    u[10].owner = ENEMY;
    build_owner_lists();
    sc64_target_index_begin();
    compare_query(820, 120, SIDE, 64, U_SCOURGE);
    assert(nearest_enemy_typed(820, 120, SIDE, 64, U_SCOURGE) == 10);
#endif

#if defined(SC64_LATE_TERRAN_STORAGE) || defined(SC64_TARGET_OCCUPANCY)
    /* The entity moves across bucket boundaries during the indexed pass; its
     * old cell must stop returning it and its new cell must return it. */
    moving = 0;
    u[moving].flags = 0;
    u[moving].x = WORLD_W - 16; u[moving].y = WORLD_H - 16;
    sc64_target_index_move(&u[moving]);
    assert(sc64_target_owner_columns[SIDE] & (1u << ((WORLD_W - 16) >> 6)));
    assert(sc64_target_owner_rows[SIDE] & (1u << ((WORLD_H - 16) >> 6)));
    compare_query(WORLD_W - 16, WORLD_H - 16, ENEMY, 10, U_MARINE);
    compare_query(180, 120, ENEMY, 10, U_MARINE);

    /* Death remains an immediate filter without rebuilding the index. */
    u[moving].alive = 0;
    compare_query(WORLD_W - 16, WORLD_H - 16, ENEMY, 10, U_MARINE);
    assert(sc64_target_owner_columns[SIDE] & (1u << ((WORLD_W - 16) >> 6)));

    /* Exercise masks at both bit endpoints. Moves add occupancy immediately;
     * dead-entry bits remain conservative until the next begin reset. */
    race_cols = SC64_TARGET_GRID_COLS * 4;
    race_rows = SC64_TARGET_GRID_ROWS * 4;
    memset(u, 0, sizeof(u)); memset(b, 0, sizeof(b));
    u[0].alive = 1; u[0].owner = ENEMY; u[0].type = U_MARINE;
    u[0].x = WORLD_W - 32; u[0].y = WORLD_H - 32;
    build_owner_lists(); sc64_target_index_begin();
    assert(sc64_target_index_active);
    assert(sc64_target_owner_columns[ENEMY] == (1u << (SC64_TARGET_GRID_COLS - 1)));
    assert(sc64_target_owner_rows[ENEMY] == (1u << (SC64_TARGET_GRID_ROWS - 1)));
    compare_query(u[0].x, u[0].y, SIDE, 1, U_MARINE);
    u[0].x = u[0].y = 32;
    sc64_target_index_move(&u[0]);
    assert(sc64_target_owner_columns[ENEMY] == ((1u << (SC64_TARGET_GRID_COLS - 1)) | 1u));
    assert(sc64_target_owner_rows[ENEMY] == ((1u << (SC64_TARGET_GRID_ROWS - 1)) | 1u));
    compare_query(32, 32, SIDE, 1, U_MARINE);
    u[0].alive = 0;
    compare_query(32, 32, SIDE, 1, U_MARINE);
    assert(sc64_target_owner_columns[ENEMY] & 1u);
    sc64_target_index_begin();
    assert(sc64_target_index_active);
    assert(sc64_target_owner_columns[ENEMY] == 0);
    assert(sc64_target_owner_rows[ENEMY] == 0);
#endif

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

#if defined(SC64_LATE_TERRAN_STORAGE) || defined(SC64_TARGET_OCCUPANCY)
    /* Generation-17 expands the unified index past 255. Verify the exact
     * transition values at 254/255 and the final target 287, including
     * lowest-global-index ties through both the indexed and oracle paths. */
    for (int i = 0; i < MAX_UNITS; ++i) u[i].alive = 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i) b[i].alive = 0;
    for (int i = 0; i < MAX_UNITS; ++i) {
        u[i].owner = ENEMY; u[i].type = U_MARINE;
        u[i].x = 500; u[i].y = 500; u[i].flags = 0;
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i) {
        b[i].owner = ENEMY; b[i].type = B_HATCHERY;
        b[i].x = 500; b[i].y = 500;
    }
    u[MAX_UNITS - 1].alive = 1;
    b[0].alive = b[1].alive = b[MAX_BUILDINGS - 1].alive = 1;
    build_owner_lists();
    sc64_target_index_begin();
    assert(sc64_target_index_active);
    compare_query(500, 500, SIDE, 64, U_MARINE);
    assert(nearest_enemy_typed(500, 500, SIDE, 64, U_MARINE) == MAX_UNITS - 1);
    u[MAX_UNITS - 1].alive = 0;
    compare_query(500, 500, SIDE, 64, U_MARINE);
    assert(nearest_enemy_typed(500, 500, SIDE, 64, U_MARINE) == MAX_UNITS);
    b[0].alive = 0;
    compare_query(500, 500, SIDE, 64, U_MARINE);
    assert(nearest_enemy_typed(500, 500, SIDE, 64, U_MARINE) == MAX_UNITS + 1);
    b[1].alive = 0;
    compare_query(500, 500, SIDE, 64, U_MARINE);
    assert(nearest_enemy_typed(500, 500, SIDE, 64, U_MARINE) ==
           MAX_UNITS + MAX_BUILDINGS - 1);
    sc64_target_index_end();
#endif

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
    race_cols = SC64_TARGET_GRID_COLS * 4;
    race_rows = SC64_TARGET_GRID_ROWS * 4;
    u[0].alive = 1; u[0].owner = SIDE; u[0].type = U_MARINE; u[0].flags = 0;
    u[1].alive = 1; u[1].owner = ENEMY; u[1].type = U_ZERGLING; u[1].flags = 0;
    u[0].x = 32; u[0].y = 32; u[1].x = 96; u[1].y = 32;
    build_owner_lists();
    sc64_target_index_begin();
    assert(sc64_target_index_active);
    for (int entity = 0; entity < 2 && sc64_target_index_active; ++entity)
        for (int row = 0; row < SC64_TARGET_GRID_ROWS && sc64_target_index_active; ++row)
            for (int col = 0; col < SC64_TARGET_GRID_COLS && sc64_target_index_active; ++col) {
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
    test_player_auto_acquisition_fog();
#if defined(SC64_TARGET_OCCUPANCY) && \
    (defined(SC64_Z4_RUNTIME) || defined(SC64_P4_RUNTIME))
    /* Rebuild hostility masks after an alliance/rescue ownership transition.
     * The Z/P mission treats the rescued owner as allied to SIDE; generic
     * skirmish hostility treats distinct non-neutral owners as hostile. */
    memset(u, 0, sizeof(u)); memset(b, 0, sizeof(b));
    race_mode = 0;
#ifdef SC64_Z4_RUNTIME
    race_selected_mission = SC64_Z4_GLOBAL_MISSION_ID;
#else
    race_selected_mission = SC64_P4_GLOBAL_ID;
#endif
    u[0].alive = u[1].alive = 1;
    u[0].owner = SIDE; u[1].owner = RESCUABLE;
    u[0].type = u[1].type = U_MARINE;
    u[0].x = u[1].x = 512; u[0].y = u[1].y = 512;
    build_owner_lists(); sc64_target_index_begin(); sc64_target_hostility_snapshot();
    compare_query(512, 512, SIDE, 64, U_MARINE);
    assert(nearest_enemy_typed(512, 512, SIDE, 64, U_MARINE) == -1);
    u[1].owner = ENEMY; /* Simulate ownership changing at a source event. */
    build_owner_lists(); sc64_target_index_begin(); sc64_target_hostility_snapshot();
    compare_query(512, 512, SIDE, 64, U_MARINE);
    assert(nearest_enemy_typed(512, 512, SIDE, 64, U_MARINE) == 1);
    u[1].owner = RESCUABLE;
    race_mode = 1;
    build_owner_lists(); sc64_target_index_begin(); sc64_target_hostility_snapshot();
    compare_query(512, 512, SIDE, 64, U_MARINE);
    assert(nearest_enemy_typed(512, 512, SIDE, 64, U_MARINE) == 1);
#endif
    return 0;
}
