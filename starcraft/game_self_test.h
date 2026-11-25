/* The game's built-in self test. This is part of game.c and is included there
 * once; it lives in its own file to keep game.c under the source-tree size
 * guard.
 * SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef GAME_SELF_TEST_H
#define GAME_SELF_TEST_H

int
game_self_test(void)
{
    int i;
#ifndef SC64_IMPORTED_MISSION
    int t, enemy, before, last;
#endif
    init_state();
#ifdef SC64_RACES
    return MAP_COLS!=64 || MAP_ROWS!=64 || race_owner_supply(SIDE)<=0 ||
        race_owner_supply_cap(SIDE)<=0 || !owners_hostile(SIDE,ENEMY) ||
        owners_hostile(SIDE,RESCUABLE);
#elif defined(SC64_BACKWATER)
    return mission_unsupported_count || bw_count(0, 17, 19, 64) != 1 || bw_count(0, 19, 130, 64) != 1;
#elif defined(SC64_SKIRMISH)
    return skirmish_bots < 1 || skirmish_bots > 3 || owner_command_center(SIDE) < 0;
#endif
#ifdef SC64_IMPORTED_MISSION
    {
	int scv = 0, marine = 0, raynor = 0, zergling = 0;
	int command = 0, depot = 0, barracks = 0, mineral = 0, raynor_slot = -1, side_slot = -1;
	if (MAP_COLS != 64 || MAP_ROWS != 64 || SC64_IMPORTED_UNIT_COUNT != 46)
	    return 1;
	for (i = 0; i < MAX_UNITS; i++) if (u[i].alive) {
	    if (u[i].owner == SIDE && side_slot < 0) side_slot = i;
	    if (IS_WORKER(u[i].type) && u[i].owner == SIDE) scv++;
	    else if (u[i].type == U_MARINE && u[i].owner == SIDE) marine++;
	    else if (u[i].type == U_RAYNOR) { raynor++; raynor_slot = i; }
	    else if (u[i].type == U_ZERGLING && u[i].owner == ENEMY) zergling++;
	}
	for (i = 0; i < MAX_BUILDINGS; i++) if (b[i].alive) {
	    if (b[i].type == B_COMMAND && b[i].owner == SIDE) command++;
	    else if (b[i].type == B_DEPOT && b[i].owner == SIDE) depot++;
	    else if (b[i].type == B_BARRACKS && b[i].owner == SIDE) barracks++;
	}
	for (i = 0; i < MAX_MINERALS; i++) if (m[i].amount > 0) mineral++;
	if (scv != 5 || marine != 5 || raynor != 1 || zergling != 23 ||
	    u[raynor_slot].owner != NEUTRAL || u[raynor_slot].maxhp != 200 ||
	    command != 1 || depot != 1 || barracks != 0 || mineral != 10 ||
	    supply_cap != 36 || supply_used != 20 || mission_unsupported_count)
	    return 2;
	check_imported_mission_outcome();
	if (won || lost) return 3;
	/* Touching the neutral hero recruits him and reserves his two supply. */
	u[side_slot].x = u[raynor_slot].x; u[side_slot].y = u[raynor_slot].y;
	update_imported_mission();
	if (u[raynor_slot].owner != SIDE || supply_used != 22) return 4;
	frame = 29; minerals = 0; update_imported_mission();
	if (mission_objective_stage || minerals) return 5;
	frame = 30; update_imported_mission();
	if (!mission_objective_stage || minerals != 40) return 6;
	for (i = 0; i < 5; i++)
	    if (spawn_unit(SIDE, U_MARINE, 1100 + i * 8, 1100) < 0) return 7;
	if (spawn_build(SIDE, B_BARRACKS, 1200, 1200, 0) < 0) return 8;
	check_imported_mission_outcome();
	if (!won || lost) return 9;
	init_state();
	for (i = 0; i < MAX_UNITS; i++)
	    if (u[i].alive && u[i].type == U_RAYNOR) u[i].alive = 0;
	check_imported_mission_outcome();
	if (!lost || won) return 10;
	init_state();
	/* The other source defeat trigger needs no CC/Barracks, at most nine
	 * Marines, and at most 399 minerals; losing only the CC is not enough. */
	b[0].alive = 0;
	check_imported_mission_outcome();
	if (lost) return 11;
	for (i = 0; i < MAX_BUILDINGS; i++) b[i].alive = 0;
	check_imported_mission_outcome();
	if (!lost) return 12;
	/* A sampled straight-line shortcut used to accept a different route
	 * from the movement rule and stick at this real cliff corner. */
	init_state();
	if (direct_path_clear(175, 658, 240, 456)) return 13;
	/* Complete a real outbound/mining/return cycle from an original SCV
	 * placement using the imported terrain, resource field and base. Other
	 * actors are removed to isolate routing from combat/crowd interference. */
	{
	    int worker = -1, patch = -1, best = 0x7fffffff;
	    for (i = 0; i < MAX_UNITS; i++) if (u[i].alive && u[i].owner == SIDE && IS_WORKER(u[i].type)) {
		int candidate = nearest_mineral(u[i].x, u[i].y);
		int distance = candidate < 0 ? 0x7fffffff : dist2(u[i].x, u[i].y, m[candidate].x, m[candidate].y);
		if (distance < best) { worker = i; patch = candidate; best = distance; }
	    }
	    if (worker < 0) return 14;
	    for (i = 0; i < MAX_UNITS; i++) if (i != worker) u[i].alive = 0;
	    u[worker].order = ORDER_MINE; u[worker].target = patch; minerals = 0;
	    for (i = 0; i < 5000 && minerals < 8; i++) update_orders();
	    if (minerals < 8 || u[worker].carry) return 15;
	}
	/* Loading in a fresh game must not repeat the one-time mineral grant. */
	init_state();
	frame = 30; update_imported_mission(); minerals = 321;
	if (!save_game()) return 16;
	init_state();
	if (!load_game() || mission_objective_stage != 1 || minerals != 321) return 17;
	update_imported_mission();
	if (minerals != 321) return 18;
	/* A save before the trigger still receives exactly one grant later. */
	init_state(); frame = 20;
	if (!save_game()) return 19;
	init_state();
	if (!load_game() || mission_objective_stage) return 20;
	frame = 30; update_imported_mission();
	if (minerals != 40 || mission_objective_stage != 1) return 21;
	/* Retain the whole friendly starting formation to catch workers
	 * deadlocking behind one another. Remove enemies only to isolate the
	 * economy from the player's escort and defense decisions. */
	init_state();
	for (i = 0; i < MAX_UNITS; i++) if (u[i].owner == ENEMY) u[i].alive = 0;
	for (i = 0; i < 4000; i++) game_update(0, 0);
	if (minerals <= 40 || lost || won) return 22;
        /* Cell-boundary jumps must visit exactly the same collision cells
         * as the old pixel-by-pixel trajectory in every direction. */
        {
            u32 check_seed = 0x9182bfu;
            for (i = 0; i < 2000; i++) {
                int x = (int)(rng_next(&check_seed) % WORLD_W);
                int y = (int)(rng_next(&check_seed) % WORLD_H);
                int tx = (int)(rng_next(&check_seed) % WORLD_W);
                int ty = (int)(rng_next(&check_seed) % WORLD_H);
                int px = x, py = y, previous = x / 16 + y / 16 * MAP_COLS, clear = 1;
                while (px != tx || py != ty) {
                    int cell;
                    if (px < tx) px++; else if (px > tx) px--;
                    if (py < ty) py++; else if (py > ty) py--;
                    cell = px / 16 + py / 16 * MAP_COLS;
                    if (cell != previous && !is_traversable(px / 16, py / 16, -1)) { clear = 0; break; }
                    previous = cell;
                }
                if (direct_path_clear(x, y, tx, ty) != clear) return 23;
            }
        }
        /* Reaching a waypoint must not leave an armed escort defenseless. */
        init_state();
        memset(u, 0, sizeof(u)); supply_used = 0;
        {
            int guard = spawn_unit(SIDE, U_MARINE, 80, 80);
            int foe = spawn_unit(ENEMY, U_ZERGLING, 100, 80);
            update_orders();
            if (u[guard].order != ORDER_IDLE || u[foe].hp >= u[foe].maxhp) return 24;
        }
#ifdef GBA_HOST
        /* Reusing the two HUD bands must be pixel-identical to drawing the
         * text again, including a resource change that invalidates the cache. */
        init_state();
        for (int state = 0; state < 2; state++) {
            u32 expected = 2166136261u, actual = 2166136261u;
            minerals = state ? 1234 : 40;
            sc64_hud_valid = 0;
            game_render();
            for (int y = 0; y < 160; y++) for (int x = 0; x < 240; x++)
                expected = (expected ^ gfx_test_pixel(x, y)) * 16777619u;
            game_render();
            for (int y = 0; y < 160; y++) for (int x = 0; x < 240; x++)
                actual = (actual ^ gfx_test_pixel(x, y)) * 16777619u;
            if (actual != expected) return 25;
        }
#endif
    }
    return 0;
#else
    if (minerals != 500 || supply_used != 26 || supply_cap != 36)
        return 1;
    /* Starting Marines overlap an inflated Command Center tile. They must
     * escape it while movement into a different blocked tile stays illegal. */
    if (is_traversable(5, 4, -1) || !is_traversable(5, 4, 5 + 4 * MAP_COLS) ||
        is_traversable(4, 5, 5 + 4 * MAP_COLS))
        return 42;
    for (i = 0; i < MAX_UNITS; i++) if (i != 5) u[i].alive = 0;
    for (i = 0; i < 30; i++) move_toward(&u[5], 104, 72);
    if (u[5].x < 96) return 43;
    init_state();
    if (unit_cost(U_MARINE) != 50 || unit_supply(U_MARINE) != 2 ||
        unit_supply(U_ZERGLING) != 1 || unit_build_time(U_SCV) != 375)
        return 2;
    if (!has_tech_for(B_BARRACKS) || !can_place(B_REFINERY, geysers[0].x, geysers[0].y) ||
        can_place(B_REFINERY, 200, 180))
        return 3;
    b[2].under_construction = 1;
    if (has_tech_for(B_BARRACKS))
        return 4;
    b[2].under_construction = 0;

    t = spawn_unit(SIDE, U_MARINE, 110, 105);
    if (t < 0 || supply_used != 28)
        return 5;
    hit_target(t, 6, U_ZERGLING, ENEMY);
    if (u[t].hp != 34)
        return 6;
    hit_target(t, 40, U_ZERGLING, ENEMY);
    if (u[t].alive || supply_used != 26)
        return 7;
    if (can_place(B_DEPOT, 80, 88) || !can_place(B_DEPOT, 200, 180))
        return 8;

    /* Queue slots reserve supply, reject overflow, and refund cancellation. */
    minerals = 1000;
    supply_cap = 28;
    queue_unit(3, U_MARINE);
    if (b[3].queue_count != 1 || reserved_supply() != 28 || minerals != 950)
        return 9;
    queue_unit(3, U_MARINE);
    if (b[3].queue_count != 1 || minerals != 950)
        return 10;
    cancel_queue(3);
    if (b[3].queue_count || minerals != 1000)
        return 11;
    supply_cap = 36;
    queue_unit(3, U_MARINE);
    for (i = 0; i < unit_build_time(U_MARINE); i++)
        update_production();
    if (b[3].queue_count || supply_used != 28)
        return 12;

    /* Attack-move keeps advancing to its destination once nearby foes are gone. */
    memset(u, 0, sizeof(u));
    memset(b, 0, sizeof(b));
    invalidate_path_grid();
    supply_used = 0;
    t = spawn_unit(SIDE, U_MARINE, 80, 120);
    u[t].selected = 1;
    issue_order(ORDER_ATTACK_MOVE, 300, 200);
    before = dist2(u[t].x, u[t].y, 300, 200);
    for (i = 0; i < 100; i++)
        update_orders();
    if (dist2(u[t].x, u[t].y, 300, 200) >= before)
        return 13;

    /* BFS routing skirts static terrain and a structure blocking the corridor. */
    memset(u, 0, sizeof(u));
    memset(b, 0, sizeof(b));
    invalidate_path_grid();
    supply_used = 0;
    t = spawn_unit(SIDE, U_MARINE, 112, 120);
    spawn_build(ENEMY, B_BARRACKS, 160, 120, 0);
    if (is_traversable(10, 7, -1))
	return 40;
    hit_target(MAX_UNITS, 2000, U_MARINE, SIDE);
    if (!is_traversable(10, 7, -1))
	return 41;
    spawn_build(ENEMY, B_BARRACKS, 160, 120, 0);
    for (i = 0; i < 1200; i++)
        move_toward(&u[t], 232, 120);
    if (u[t].x < 205 || dist2(u[t].x, u[t].y, 160, 120) < 18 * 18)
	return 14;

    /* Enemy units choose the Command Center and the Hatchery schedules waves. */
    init_state();
    for (i = 0; i < MAX_UNITS; i++)
        if (u[i].owner == SIDE)
            u[i].alive = 0;
    enemy = 13;
    before = dist2(u[enemy].x, u[enemy].y, b[0].x, b[0].y);
    for (i = 0; i < 100; i++)
        update_orders();
    if (dist2(u[enemy].x, u[enemy].y, b[0].x, b[0].y) >= before)
        return 15;
    init_state();
    for (i = 0; i < MAX_UNITS; i++)
        if (u[i].owner == ENEMY || u[i].owner == SIDE)
            u[i].alive = 0;
    supply_used = 0;
    frame = enemy_timer;
    enemy_wave();
    last = 0;
    for (i = 0; i < MAX_UNITS; i++)
        if (u[i].alive && u[i].owner == ENEMY)
            last++;
    if (last != 2)
        return 16;

    /* Construction travels to the site, completes, and adds Depot supply. */
    init_state();
    for (i = 0; i < MAX_UNITS; i++)
        if (u[i].owner == ENEMY || u[i].owner == SIDE)
            u[i].alive = 0;
    supply_used = 0;
    b[2].alive = 0;
    invalidate_path_grid();
    supply_cap = 20;
    minerals = 1000;
    t = spawn_unit(SIDE, U_SCV, 92, 98);
    u[t].selected = 1;
    cx = 216;
    cy = 176;
    start_build(B_DEPOT);
    if (!b[2].alive || b[2].type != B_DEPOT || supply_cap != 20)
        return 17;
    for (i = 0; i < 1200; i++)
        update_orders();
    if (!building_complete(2))
        return 10000 + u[t].x * 10000 + u[t].y * 10 + b[2].build_progress;
    if (supply_cap != 36)
        return 181;
    if (!has_tech_for(B_BARRACKS))
        return 182;
    hit_target(MAX_UNITS + 2, 999, U_ZERGLING, ENEMY);
    if (supply_cap != 20)
        return 19;

    /* Save/load restores resources, queue state, cursor, and explored map. */
    init_state();
    minerals = 321;
    gas = 17;
    m[0].amount = 1234;
    cx = 245;
    cy = 191;
    SC64_GRID_SET(explored, MAP_CELLS - 1, 1);
    if (!save_game())
        return 20;
    minerals = gas = m[0].amount = 0;
    cx = cy = 10;
    SC64_GRID_SET(explored, MAP_CELLS - 1, 0);
    if (!load_game() || minerals != 321 || gas != 17 || m[0].amount != 1234 ||
        cx != 245 || cy != 191 || !SC64_GRID_GET(explored, MAP_CELLS - 1))
        return 21;

    /* Victory requires clearing the Zerg base; defeat follows CC destruction. */
    init_state();
    for (i = 0; i < MAX_UNITS; i++)
        if (u[i].owner == ENEMY)
            u[i].alive = 0;
    hit_target(MAX_UNITS + 1, 1400, U_MARINE, SIDE);
    game_update(0, 0);
    if (!won || screen != SCREEN_RESULT)
        return 22;
    init_state();
    hit_target(MAX_UNITS, 2000, U_ZERGLING, ENEMY);
    game_update(0, 0);
    if (!lost || screen != SCREEN_RESULT)
        return 23;

    game_init();
    if (screen != SCREEN_TITLE)
        return 24;
    game_update(KEY_A, KEY_A);
    if (screen != SCREEN_PLAY)
        return 25;
    game_update(KEY_START, KEY_START);
    if (screen != SCREEN_PAUSE)
        return 26;
    game_update(KEY_A, KEY_A);
    if (screen != SCREEN_PLAY)
        return 27;

    /* Play a short legal end-to-end run: spend the starter budget through the
     * Barracks queue, let SCVs mine, attack-move the produced force, and clear
     * the Hatchery with the actual combat/economy/update systems. */
    init_state();
    for (i = 0; i < 5; i++)
	queue_unit(3, U_MARINE);
    for (i = 0; i < 1900; i++)
	game_update(0, 0);
    for (i = 0; i < 5; i++)
	queue_unit(3, U_MARINE);
    for (i = 0; i < MAX_UNITS; i++)
	if (u[i].alive && u[i].owner == SIDE && u[i].type != U_SCV)
	    u[i].selected = 1;
    issue_order(ORDER_ATTACK_MOVE, b[1].x, b[1].y);
    for (i = 0; i < 7000 && !won && !lost; i++)
	game_update(0, 0);
    if (!won)
	return 1000 + (u[5].order * 1000) + u[5].x / 10 + (u[5].y / 10) * 100;
    init_state();
    return 0;
#endif
}

#endif
