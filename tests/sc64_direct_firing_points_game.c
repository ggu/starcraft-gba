/* SPDX-License-Identifier: GPL-3.0-or-later
 * Actual-engine tests for the bounded direct firing-corridor fallback. */
#include "../starcraft/game.c"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int attacker, victim;
static void setup_combat_scene(void);

static void clear_walk_grid(void)
{
    memset(path_walkable, 0xff, sizeof(path_walkable));
    path_grid_valid = 1;
#ifdef SC64_DIRECT_FIRING_POINTS
    sc64_direct_firing_cache_invalidate();
#endif
#ifdef SC64_RANGE_WORK
    sc64_range_route_reset();
#endif
}

static void block_center_ring(int tx, int ty)
{
    int col = tx / 16, row = ty / 16;
    for (int y = row - 1; y <= row + 1; ++y)
        for (int x = col - 1; x <= col + 1; ++x)
            SC64_GRID_SET(path_walkable, x + y * MAP_COLS, 0);
    SC64_GRID_SET(path_walkable, col + row * MAP_COLS, 1);
}

#ifdef SC64_DIRECT_FIRING_POINTS
static void assert_band(int x, int y, int tx, int ty, int minimum, int maximum)
{
    int d = dist2(x, y, tx, ty);
    assert(d >= minimum * minimum && d <= maximum * maximum);
    assert(x >= 8 && y >= 8 && x < MAP_COLS * 16 && y < MAP_ROWS * 16);
    assert(SC64_GRID_GET(path_walkable, x / 16 + (y / 16) * MAP_COLS));
}

#ifdef SC64_RANGE_WORK
static void range_route_pump_one_tick(void)
{
    ++frame;
    sc64_range_route_tick();
    assert(sc64_range_route_last_units <= SC64_RANGE_WORK_BUDGET_PER_TICK);
}

static void range_route_game_update_tick(void)
{
    game_update(0, 0);
    assert(sc64_range_route_last_units <= SC64_RANGE_WORK_BUDGET_PER_TICK);
}

static void range_route_pump_to_done(unsigned max_ticks, int target,
                                     int tx, int ty, int col, int row)
{
    for (unsigned tick = 0; tick < max_ticks; ++tick) {
        int next_col = -1, next_row = -1;
        if (sc64_range_route_active.valid &&
            sc64_range_route_active.target == (u16)target &&
            sc64_range_route_work.target_x == (u16)tx &&
            sc64_range_route_work.target_y == (u16)ty &&
            sc64_range_route_work.min_range == 0 &&
            sc64_range_route_work.max_range == 64 &&
            sc64_range_route_work.phase == SC64_RANGE_WORK_DONE) return;
        /* A pending combat order needs 12 retry ticks plus one timer-drain
         * tick before its next plan. Refresh at that 14-frame cadence. */
        if (tick % 14u == 0u) {
            (void)sc64_range_route_next(target, tx, ty, 0, 64, col, row,
                                        &next_col, &next_row);
            for (int i = 0; i < SC64_RANGE_ROUTE_QUEUE_CAPACITY; ++i) {
                Sc64RangeRouteJob *queued = &sc64_range_route_pending[i];
                if (queued->valid) {
                    next_col = next_row = -1;
                    (void)sc64_range_route_next(queued->target, queued->tx,
                        queued->ty, queued->minimum, queued->maximum, 0, 0,
                        &next_col, &next_row);
                }
            }
        }
        range_route_pump_one_tick();
    }
    assert(!"range work failed to complete within bounded fixture limit");
}

static int range_route_queued_target(int target)
{
    for (int i = 0; i < SC64_RANGE_ROUTE_QUEUE_CAPACITY; ++i)
        if (sc64_range_route_pending[i].valid &&
            sc64_range_route_pending[i].target == target) return 1;
    return 0;
}

static int range_route_request_present(int target, int tx, int ty)
{
    if (sc64_range_route_active.valid &&
        sc64_range_route_active.target == (u16)target &&
        sc64_range_route_work.target_x == (u16)tx &&
        sc64_range_route_work.target_y == (u16)ty &&
        sc64_range_route_work.min_range == 0 &&
        sc64_range_route_work.max_range == 64) return 1;
    for (int i = 0; i < SC64_RANGE_ROUTE_QUEUE_CAPACITY; ++i) {
        const Sc64RangeRouteJob *job = &sc64_range_route_pending[i];
        if (job->valid && job->target == (u16)target &&
            job->tx == (u16)tx && job->ty == (u16)ty &&
            job->minimum == 0 && job->maximum == 64) return 1;
    }
    return 0;
}

static int range_route_active_position_was_requested(int target,
                                                      int first_tx,
                                                      int latest_tx,
                                                      int first_ty)
{
    Sc64RangeRouteJob current = sc64_range_route_active_key();
    const Sc64RangeRouteJob *job = &current;
    if (!job->valid || job->target != (u16)target ||
        job->minimum != 0 || job->maximum != 64 ||
        job->tx < (u16)first_tx || job->tx > (u16)latest_tx) return 0;
    if (job->ty == (u16)first_ty && job->tx == (u16)first_tx) return 1;
    return job->ty == (u16)(first_ty + 16) &&
        ((job->tx - (u16)first_tx) % 16u) == 0u;
}

static Sc64RangeRouteJob range_route_stale_guidance_source(int target)
{
    if (sc64_range_route_active.valid &&
        sc64_range_route_active.target == (u16)target &&
        sc64_range_route_work.min_range == 0 &&
        sc64_range_route_work.max_range == 64 &&
        (sc64_range_route_work.phase == SC64_RANGE_WORK_FLOOD ||
         sc64_range_route_work.phase == SC64_RANGE_WORK_DONE))
        return sc64_range_route_active_key();
    assert(sc64_range_route_snapshot_slot >= 0 &&
           sc64_range_route_snapshot_slot < SC64_ROUTE_ACTIVE_SLOTS &&
           sc64_route_valid[sc64_range_route_snapshot_slot] == 3 &&
           sc64_range_route_snapshot_key.valid &&
           sc64_range_route_snapshot_key.target == (u16)target &&
           sc64_range_route_snapshot_key.minimum == 0 &&
           sc64_range_route_snapshot_key.maximum == 64);
    {
        Sc64RangeRouteJob key = {0};
        key.target = sc64_range_route_snapshot_key.target;
        key.tx = sc64_range_route_snapshot_key.tx; key.ty = sc64_range_route_snapshot_key.ty;
        key.minimum = sc64_range_route_snapshot_key.minimum;
        key.maximum = sc64_range_route_snapshot_key.maximum; key.valid = 1;
        return key;
    }
}

static void test_range_scheduler_stays_lazy_and_releases_abandoned_copy(void)
{
    const int tx = 10 * 16 + 8, ty = 10 * 16 + 8;
    int nc = -1, nr = -1;
    sc64_range_route_reset();
    path_grid_valid = 0;
    sc64_range_route_tick();
    assert(!path_grid_valid); /* no queued work: no eager whole-grid rebuild */

#if SC64_RANGE_WORK_BUDGET_PER_TICK < 96u
    /* A one-cell component completes quickly while the 6144-byte maximum-map
     * snapshot takes multiple bounded copy ticks at this test quota. */
    sc64_range_route_reset();
    memset(path_walkable, 0, sizeof(path_walkable));
    SC64_GRID_SET(path_walkable, 10 + 10 * MAP_COLS, 1);
    path_grid_valid = 1;
    assert(sc64_range_route_next(100, tx, ty, 0, 0, 10, 10,
                                 &nc, &nr) == SC64_RANGE_ROUTE_PENDING);
    for (unsigned tick = 0; tick < 1000u &&
         sc64_range_route_work.phase != SC64_RANGE_WORK_DONE; ++tick)
        range_route_pump_one_tick();
    assert(sc64_range_route_active.valid &&
           sc64_range_route_work.phase == SC64_RANGE_WORK_DONE);
    assert(sc64_range_route_next(101, 20 * 16 + 8, ty, 0, 0, 10, 10,
                                 &nc, &nr) == SC64_RANGE_ROUTE_PENDING);
    frame = (int)(sc64_range_route_complete_frame +
                  SC64_RANGE_ROUTE_RESULT_LEASE);
    range_route_pump_one_tick();
    assert(sc64_range_route_copy_slot >= 0);
    assert(sc64_range_route_copy_cursor > 0 &&
           sc64_range_route_copy_cursor <
               sc64_range_work_ceil3((unsigned)sc64_range_route_work.cols * sc64_range_route_work.rows));
    frame = (int)(sc64_range_route_pending[
        sc64_range_route_queue_head].last_request +
        SC64_RANGE_ROUTE_IDLE_LIMIT + 1u);
    range_route_pump_one_tick();
    assert(sc64_range_route_queue_count == 0);
    assert(sc64_range_route_copy_slot < 0);
    assert(sc64_range_route_snapshot_slot < 0);
#endif
    sc64_range_route_reset();
    clear_walk_grid();
}

#ifdef SC64_ROUTE_ACTIVE_STRIDE
static void test_range_query_reconfigures_shape_before_snapshot_lookup(void)
{
    int old_cols = race_cols, old_rows = race_rows;
    int nc = -1, nr = -1;
    race_cols = 64; race_rows = 64;
    sc64_routes_reset();
    clear_walk_grid();
    assert(sc64_range_route_next(102, 40 * 16 + 8, 40 * 16 + 8,
        0, 64, 10, 10, &nc, &nr) == SC64_RANGE_ROUTE_PENDING);
    range_route_pump_one_tick();
    assert(sc64_range_route_active.valid);

    /* Model a map-shape update before the end-of-frame worker tick. Reconfigure
     * must discard the old active field before the new query can inspect it. */
    race_cols = 96; race_rows = 64;
    path_grid_valid = 0;
    assert(sc64_range_route_next(102, 50 * 16 + 8, 40 * 16 + 8,
        0, 64, 10, 10, &nc, &nr) == SC64_RANGE_ROUTE_PENDING);
    assert(!sc64_range_route_active.valid);
    assert(sc64_range_route_queue_count == 1);

    race_cols = old_cols; race_rows = old_rows;
    sc64_routes_reset();
    path_grid_valid = 0;
}
#endif

/* Scheduler-only cases use explicit synthetic target IDs. */
static void seed_range_synthetic_targets(void)
{
    for (int i=70;i<=102;++i) { u[i].alive=1;u[i].type=U_MARINE;u[i].owner=ENEMY; }
}

static void test_range_work_scheduler_contracts(void)
{
    const int tx = 56 * 16 + 8, ty = 40 * 16 + 8;
    int nc = -1, nr = -1, status;
    u32 route_builds_before;
    game_init();
    init_state();
    seed_range_synthetic_targets();
    test_range_scheduler_stays_lazy_and_releases_abandoned_copy();
#ifdef SC64_ROUTE_ACTIVE_STRIDE
    test_range_query_reconfigures_shape_before_snapshot_lookup();
#endif
    memset(u, 0, sizeof(u));
    memset(b, 0, sizeof(b));
    seed_range_synthetic_targets();
    memset(race_player, 0xff, sizeof(race_player));
    race_player[SIDE] = race_player[ENEMY] = R_ZERG;
    race_mode = 1;
    race_human = R_ZERG;
    assert(spawn_build(SIDE, B_COMMAND, 1000, 1000, 0) >= 0);
    assert(spawn_build(ENEMY, B_COMMAND, 1500, 1000, 0) >= 0);
    won = lost = 0;
    screen = SCREEN_PLAY;
    clear_walk_grid();
    for (int row = 0; row < MAP_ROWS; ++row)
        if (row != 20) SC64_GRID_SET(path_walkable, 24 + row * MAP_COLS, 0);

    /* First query is admitted but cannot expose an uncomputed gradient. */
    status = sc64_range_route_next(70, tx, ty, 0, 64, 5, 40, &nc, &nr);
    assert(status == SC64_RANGE_ROUTE_PENDING);
    assert(!sc64_range_route_active.valid);
    range_route_pump_one_tick();
    assert(sc64_range_route_active.valid &&
           sc64_range_route_active.target == 70);
    status = sc64_range_route_next(70, tx, ty, 0, 64, 5, 40, &nc, &nr);
    assert(status == SC64_RANGE_ROUTE_PENDING);

    /* A second target queues behind the active query. Completion is retained
       for a finite lease, then FIFO service advances to the waiting target. */
    assert(sc64_range_route_next(71, tx - 128, ty, 0, 64, 4, 40,
                                 &nc, &nr) == SC64_RANGE_ROUTE_PENDING);
    assert(range_route_queued_target(71));
    range_route_pump_to_done((MAP_COLS * MAP_ROWS) + 1000u,
                             70, tx, ty, 5, 40);
    status = sc64_range_route_next(70, tx, ty, 0, 64, 5, 40, &nc, &nr);
    assert(status == SC64_RANGE_ROUTE_FOUND);
    {
        int col = 5, row = 40;
        for (int step = 0; step < MAP_COLS * MAP_ROWS; ++step) {
            int next_col = -1, next_row = -1;
            status = sc64_range_route_next(70, tx, ty, 0, 64,
                                           col, row, &next_col, &next_row);
            assert(status == SC64_RANGE_ROUTE_FOUND);
            assert(SC64_GRID_GET(path_walkable,
                                 next_col + next_row * MAP_COLS));
            if (next_col == col && next_row == row) {
                int dx = next_col * 16 + 8 - tx;
                int dy = next_row * 16 + 8 - ty;
                assert(dx * dx + dy * dy <= 64 * 64);
                break;
            }
            col = next_col; row = next_row;
            assert(step + 1 < MAP_COLS * MAP_ROWS);
        }
    }
    for (int tick = 0; tick + 1 < SC64_RANGE_ROUTE_RESULT_LEASE; ++tick) {
        if (tick % 14 == 0) {
            assert(sc64_range_route_next(70, tx, ty, 0, 64, 5, 40,
                &nc, &nr) == SC64_RANGE_ROUTE_FOUND);
            assert(sc64_range_route_next(71, tx - 128, ty, 0, 64, 4, 40,
                &nc, &nr) == SC64_RANGE_ROUTE_PENDING);
        }
        range_route_pump_one_tick();
    }
    assert(sc64_range_route_active.valid &&
           sc64_range_route_active.target == 70);
    for (int tick = 0; tick < 14; ++tick) {
        if (tick == 0)
            assert(sc64_range_route_next(71, tx - 128, ty, 0, 64, 4, 40,
                &nc, &nr) == SC64_RANGE_ROUTE_PENDING);
        range_route_pump_one_tick();
    }
    assert(sc64_range_route_active.valid &&
           sc64_range_route_active.target == 71);

    /* Move the target at each 14-frame replan cadence while game_update pumps
       bounded work. Completed jobs may roll forward at the default quota, so
       validate the active key against requested positions and require the
       newest request to remain active or queued. */
    {
        const int first_tx = tx - 128;
        int latest_tx = first_tx, latest_ty = ty + 16;
        int stale_guidance_count = 0;
        int have_guidance_key = 0;
        Sc64RangeRouteJob guidance_key = {0};
        for (int move = 0; move < 8; ++move) {
            for (int tick = 0; tick < 14; ++tick)
                range_route_game_update_tick();
            latest_tx += 16;
            status = sc64_range_route_next(71, latest_tx, latest_ty,
                                           0, 64, 4, 40, &nc, &nr);
            if (status == SC64_RANGE_ROUTE_STALE_GUIDANCE) {
                ++stale_guidance_count;
                if (!have_guidance_key) {
                    guidance_key = range_route_stale_guidance_source(71);
                    have_guidance_key = 1;
                }
            } else assert(status == SC64_RANGE_ROUTE_CANCELLED ||
                          status == SC64_RANGE_ROUTE_PENDING ||
                          status == SC64_RANGE_ROUTE_FOUND);
            assert(range_route_request_present(71, latest_tx, latest_ty));
            if (sc64_range_route_active.valid)
                assert(range_route_active_position_was_requested(
                    71, first_tx, latest_tx, ty));
        }
        for (int tick = 0; stale_guidance_count == 0 &&
             tick < MAP_COLS * MAP_ROWS * 2; ++tick) {
            if (tick % 14 == 0) {
                status = sc64_range_route_next(71, latest_tx, latest_ty,
                    0, 64, 4, 40, &nc, &nr);
                if (status == SC64_RANGE_ROUTE_STALE_GUIDANCE) {
                    ++stale_guidance_count;
                    guidance_key = range_route_stale_guidance_source(71);
                    have_guidance_key = 1;
                } else assert(status == SC64_RANGE_ROUTE_CANCELLED ||
                              status == SC64_RANGE_ROUTE_PENDING ||
                              status == SC64_RANGE_ROUTE_FOUND);
                assert(range_route_request_present(71, latest_tx, latest_ty));
            }
            if (!stale_guidance_count) range_route_game_update_tick();
        }
        assert(stale_guidance_count > 0 && have_guidance_key);
        assert(guidance_key.target == 71 && guidance_key.minimum == 0 &&
               guidance_key.maximum == 64 &&
               guidance_key.tx >= (u16)first_tx &&
               guidance_key.tx <= (u16)latest_tx);
        /* Make the current position far enough from this witnessed field's
           seed that following it demonstrates useful stale guidance. */
        latest_tx = (int)guidance_key.tx + 128;
        latest_ty = (int)guidance_key.ty + 16;
        status = sc64_range_route_next(71, latest_tx, latest_ty,
            0, 64, 4, 40, &nc, &nr);
        assert(status == SC64_RANGE_ROUTE_STALE_GUIDANCE);
        assert(range_route_request_present(71, latest_tx, latest_ty));
        /* Follow the actual stale field across the wall to its firing ring,
           then verify that ring is outside the moved target's current range. */
        {
            int col = 4, row = 40, reached_seed = 0;
            for (int step = 0; step < MAP_COLS * MAP_ROWS; ++step) {
                int next_col = -1, next_row = -1;
                status = sc64_range_route_next(71, latest_tx, latest_ty,
                    0, 64, col, row, &next_col, &next_row);
                assert(status == SC64_RANGE_ROUTE_STALE_GUIDANCE);
                assert(SC64_GRID_GET(path_walkable,
                    next_col + next_row * MAP_COLS));
                if (next_col == col && next_row == row) {
                    int dx = col * 16 + 8 - guidance_key.tx;
                    int dy = row * 16 + 8 - guidance_key.ty;
                    assert(dx * dx + dy * dy <= 64 * 64);
                    assert(dist2(col * 16 + 8, row * 16 + 8,
                                 latest_tx, latest_ty) > 64 * 64);
                    reached_seed = 1;
                    break;
                }
                col = next_col; row = next_row;
            }
            assert(reached_seed);
        }
        range_route_pump_to_done((MAP_COLS * MAP_ROWS) + 1000u,
                                 71, latest_tx, latest_ty, 4, 40);
        assert(sc64_range_route_active.valid &&
               sc64_range_route_active.target == 71 &&
               sc64_range_route_work.target_x == latest_tx &&
               sc64_range_route_work.target_y == latest_ty &&
               sc64_range_route_work.phase == SC64_RANGE_WORK_DONE);
        status = sc64_range_route_next(71, latest_tx + 16, latest_ty, 0, 64,
                                       4, 40, &nc, &nr);
        assert(status == SC64_RANGE_ROUTE_STALE_GUIDANCE);
        for (unsigned tick = 0; tick < 1000u &&
             (sc64_range_route_snapshot_slot < 0 ||
              sc64_range_route_snapshot_key.tx != (u16)latest_tx ||
              sc64_range_route_snapshot_key.ty != (u16)latest_ty); ++tick)
            range_route_pump_one_tick();
        assert(sc64_range_route_snapshot_slot >= 0 &&
               sc64_route_valid[sc64_range_route_snapshot_slot] == 3 &&
               sc64_range_route_snapshot_key.target == 71 &&
               sc64_range_route_snapshot_key.tx == (u16)latest_tx &&
               sc64_range_route_snapshot_key.ty == (u16)latest_ty);
    }

    /* A topology reset discards active and queued derived work. */
    assert(sc64_range_route_next(72, tx, ty, 0, 64, 4, 40,
                                 &nc, &nr) == SC64_RANGE_ROUTE_PENDING);
    invalidate_path_grid();
    assert(!sc64_range_route_active.valid);
    assert(!range_route_queued_target(72));
    clear_walk_grid();

    /* A sealed component finishes as NO_PATH, rather than exposing a stale
       seed or invoking the synchronous route flood. Count only this operation:
       earlier game updates can route unrelated live synthetic targets. */
    route_builds_before = sc64_route_builds;
    for (int row = 0; row < MAP_ROWS; ++row)
        SC64_GRID_SET(path_walkable, 12 + row * MAP_COLS, 0);
    assert(sc64_range_route_next(73, tx, ty, 0, 64, 4, 40,
                                 &nc, &nr) == SC64_RANGE_ROUTE_PENDING);
    range_route_pump_to_done((MAP_COLS * MAP_ROWS) + 1000u,
                             73, tx, ty, 4, 40);
    assert(sc64_range_route_next(73, tx, ty, 0, 64, 4, 40,
                                 &nc, &nr) == SC64_RANGE_ROUTE_NO_PATH);
    assert(sc64_route_builds == route_builds_before);
    sc64_range_route_reset();

    /* Sixteen-bit request stamps remain wrap-safe for queue and active idle
       expiry, including the exact 48/49-frame threshold. */
    sc64_range_route_reset();
    frame = 65530;
    assert(sc64_range_route_next(74, tx, ty, 0, 64, 4, 40,
                                 &nc, &nr) == SC64_RANGE_ROUTE_PENDING);
    frame += SC64_RANGE_ROUTE_IDLE_LIMIT - 1;
    sc64_range_route_tick();
    assert(sc64_range_route_active.valid &&
           sc64_range_route_active.target == 74);
    ++frame;
    sc64_range_route_tick();
    assert(sc64_range_route_active.valid);
    ++frame;
    sc64_range_route_tick();
    assert(!sc64_range_route_active.valid);

    assert(sc64_range_route_next(75, tx, ty, 0, 64, 4, 40,
                                 &nc, &nr) == SC64_RANGE_ROUTE_PENDING);
    sc64_range_route_tick();
    assert(sc64_range_route_active.valid);
    frame += SC64_RANGE_ROUTE_IDLE_LIMIT;
    sc64_range_route_tick();
    assert(sc64_range_route_active.valid);
    ++frame;
    sc64_range_route_tick();
    assert(!sc64_range_route_active.valid);

    /* Drive a live Unit's native attack-target combat path every frame while
       game_update pumps the scheduler. Target motion follows the 14-frame
       retry cadence for the whole wall-crossing run. */
    sc64_range_route_reset();
    /* Synthetic scheduler targets must not participate in the live scene. */
    for (int i = 70; i <= 102; ++i) memset(&u[i], 0, sizeof(u[i]));
    clear_walk_grid();
    for (int row = 0; row < MAP_ROWS; ++row)
        if (row != 20) SC64_GRID_SET(path_walkable, 24 + row * MAP_COLS, 0);
    {
        const int ty = 40 * 16 + 8;
        int mover_slot = spawn_unit(SIDE, U_TANK, 22 * 16 + 8, ty);
        int target_slot = spawn_unit(ENEMY, U_HYDRALISK, 32 * 16 + 8, ty);
        int crossed_gap = 0;
        u32 builds = sc64_route_builds;
        assert(mover_slot >= 0 && target_slot >= 0);
        assert(u[mover_slot].alive && u[mover_slot].owner == SIDE &&
               u[mover_slot].type == U_TANK);
        assert(u[target_slot].alive && u[target_slot].owner == ENEMY &&
               u[target_slot].type == U_HYDRALISK);
        u[mover_slot].order = ORDER_ATTACK_TARGET;
        u[mover_slot].target = target_slot;
        u[target_slot].hp = u[target_slot].maxhp = 65535;
#ifdef SC64_DIRECT_ATTACK
        memset(visible, 0xff, sizeof(visible));
#endif
        assert(!direct_path_clear(u[mover_slot].x, u[mover_slot].y,
                                  u[target_slot].x, u[target_slot].y));
        {
            int next_x = -1, next_y = -1;
            assert(!sc64_direct_firing_waypoint(u[mover_slot].x,
                u[mover_slot].y, u[target_slot].x, u[target_slot].y,
                0, 112, &next_x, &next_y));
        }
        for (int tick = 0; tick < MAP_COLS * MAP_ROWS * 2; ++tick) {
            range_route_game_update_tick();
            if (tick % 14 == 0)
                u[target_slot].x = ((tick / 14) & 1 ? 31 : 32) * 16 + 8;
#ifndef SC64_DIRECT_ATTACK
            tick_combat(&u[mover_slot], target_slot);
#endif
            if (u[mover_slot].x >= 24 * 16 &&
                u[mover_slot].y >= 18 * 16 && u[mover_slot].y <= 22 * 16)
                crossed_gap = 1;
            if (crossed_gap) break;
        }
        assert(crossed_gap);
        assert(u[mover_slot].alive && u[target_slot].alive);
        assert(sc64_route_builds == builds);
    }
    sc64_range_route_reset();
}
#endif

static void test_bounded_search_key_and_origin_revalidation(void)
{
    const int tx = 512, ty = 400, sx = 32, sy = 400;
    int nx, ny, oldx;
    clear_walk_grid();
    block_center_ring(tx, ty);
    assert(!path_next_tile(sx, sy, tx, ty, &nx, &ny));
    assert(sc64_direct_firing_waypoint(sx, sy, tx, ty, 0, 210, &nx, &ny));
    assert_band(nx, ny, tx, ty, 0, 210);
    assert(direct_path_clear(sx, sy, nx, ny));
    assert(sc64_direct_firing_cache.valid);
    oldx = sc64_direct_firing_cache.x;
    assert(sc64_direct_firing_waypoint(sx + 16, sy, tx, ty, 0, 210, &nx, &ny));
    assert(nx == oldx && direct_path_clear(sx + 16, sy, nx, ny));

    /* All key terms are exact and min/max use inclusive squared distance. */
    assert(sc64_direct_firing_waypoint(sx, sy, tx + 1, ty, 0, 210, &nx, &ny));
    assert(sc64_direct_firing_cache.tx == tx + 1);
    assert(sc64_direct_firing_waypoint(sx, sy, tx + 1, ty, 50, 210, &nx, &ny));
    assert(sc64_direct_firing_cache.minimum == 50 &&
           sc64_direct_firing_cache.maximum == 210);
    assert_band(nx, ny, tx + 1, ty, 50, 210);
    assert(sc64_direct_firing_waypoint(sx, sy, tx + 1, ty, 50, 200, &nx, &ny));
    assert(sc64_direct_firing_cache.maximum == 200);
    assert_band(nx, ny, tx + 1, ty, 50, 200);
    clear_walk_grid();
    {
        const int bx = 50 * 16 + 8, by = 25 * 16 + 8;
        assert(sc64_direct_firing_waypoint(bx, by, bx, by, 16, 64, &nx, &ny));
        assert(dist2(nx, ny, bx, by) == 16 * 16);
        assert(sc64_direct_firing_waypoint(bx, by, bx, by, 64, 64, &nx, &ny));
        assert(dist2(nx, ny, bx, by) == 64 * 64);
    }

    /* A cached waypoint across a wall is rejected from this origin and the
     * bounded candidate scan finds a reachable point on the other side. */
    clear_walk_grid();
    for (int row = 0; row < MAP_ROWS; ++row)
        SC64_GRID_SET(path_walkable, 32 + row * MAP_COLS, 0);
    assert(sc64_direct_firing_waypoint(32, ty, tx, ty, 0, 210, &nx, &ny));
    assert(nx < tx);
    assert(sc64_direct_firing_waypoint(992, ty, tx, ty, 0, 210, &nx, &ny));
    assert(nx > tx && direct_path_clear(992, ty, nx, ny));
    assert_band(nx, ny, tx, ty, 0, 210);

    invalidate_path_grid();
    assert(!sc64_direct_firing_cache.valid);
    assert(sizeof(sc64_direct_firing_cache) == 14);
    assert(!sc64_direct_firing_waypoint(sx, sy, tx, ty, 0, 257, &nx, &ny));
}

static void test_one_corner_paths_cache_final_firing_tile(void)
{
    const int sx = 400, sy = 400, tx = 520, ty = 400;
    int nx, ny, final_x, final_y;

    /* A local vertical wall blocks every direct final-point corridor. Its
     * opening is north of the actor; a second wall leaves the target center
     * disconnected while the in-band firing tile stays on the actor's side. */
    clear_walk_grid();
    block_center_ring(tx, ty);
    for (int row = 0; row < MAP_ROWS; ++row)
        if (row != 22) SC64_GRID_SET(path_walkable, 26 + row * MAP_COLS, 0);
    for (int row = 0; row < MAP_ROWS; ++row)
        SC64_GRID_SET(path_walkable, 30 + row * MAP_COLS, 0);
    assert(!direct_path_clear(sx, sy, tx, ty));
    assert(!path_next_tile(sx, sy, tx, ty, &nx, &ny));
    assert(!direct_path_clear(sx, sy, 472, 360));
    assert(sc64_direct_firing_waypoint(sx, sy, tx, ty, 0, 64, &nx, &ny));
    final_x = sc64_direct_firing_cache.x;
    final_y = sc64_direct_firing_cache.y;
    assert_band(final_x, final_y, tx, ty, 0, 64);
    assert(nx == sx && ny != sy); /* vertical-first elbow, not final tile */
    assert(dist2(nx, ny, tx, ty) > 64 * 64);
    assert(direct_path_clear(sx, sy, nx, ny));
    assert(direct_path_clear(nx, ny, final_x, final_y));
    assert(!direct_path_clear(sx, sy, final_x, final_y));
    assert(sc64_direct_firing_waypoint(sx + 8, sy, tx, ty, 0, 64, &nx, &ny));
    assert(sc64_direct_firing_cache.x == final_x &&
           sc64_direct_firing_cache.y == final_y);
    assert(direct_path_clear(sx + 8, sy, nx, ny));
    assert(direct_path_clear(nx, ny, final_x, final_y));

    /* A horizontal local wall requires the horizontal-first elbow. A second
     * wall keeps the target center unreachable, like the vertical case. */
    clear_walk_grid();
    block_center_ring(520, 520);
    for (int col = 0; col < MAP_COLS; ++col)
        if (col != 30) SC64_GRID_SET(path_walkable, col + 26 * MAP_COLS, 0);
    for (int col = 0; col < MAP_COLS; ++col)
        SC64_GRID_SET(path_walkable, col + 30 * MAP_COLS, 0);
    assert(!path_next_tile(sx, sy, 520, 520, &nx, &ny));
    assert(sc64_direct_firing_waypoint(sx, sy, 520, 520, 0, 64, &nx, &ny));
    final_x = sc64_direct_firing_cache.x;
    final_y = sc64_direct_firing_cache.y;
    assert_band(final_x, final_y, 520, 520, 0, 64);
    assert(nx != sx && ny == sy); /* horizontal-first elbow, not final tile */
    assert(dist2(nx, ny, 520, 520) > 64 * 64);
    assert(direct_path_clear(sx, sy, nx, ny));
    assert(direct_path_clear(nx, ny, final_x, final_y));
    assert(!direct_path_clear(sx, sy, final_x, final_y));
    assert(sc64_direct_firing_waypoint(sx + 8, sy, 520, 520, 0, 64, &nx, &ny));
    assert(sc64_direct_firing_cache.x == final_x &&
           sc64_direct_firing_cache.y == final_y);
    assert(direct_path_clear(sx + 8, sy, nx, ny));
    assert(direct_path_clear(nx, ny, final_x, final_y));
}

static void test_obstructed_candidates_keep_baseline_retry(void)
{
    int nx, ny;
    setup_combat_scene();
    u[attacker].x = 32; u[attacker].y = 400;
    u[victim].x = 512; u[victim].y = 400;
    clear_walk_grid();
    for (int row = 0; row < MAP_ROWS; ++row)
        SC64_GRID_SET(path_walkable, 12 + row * MAP_COLS, 0);
    assert(!path_next_tile(u[attacker].x, u[attacker].y,
                           u[victim].x, u[victim].y, &nx, &ny));
    assert(!sc64_direct_firing_waypoint(u[attacker].x, u[attacker].y,
                                        u[victim].x, u[victim].y,
                                        0, 64, &nx, &ny));
    assert(!sc64_direct_firing_cache.valid);
    tick_combat(&u[attacker], victim);
    assert(u[attacker].path_retry == 12 && u[attacker].path_timer == 0);
}

static void test_save_load_invalidates_cache(void)
{
    int nx, ny;
#ifdef SC64_RANGE_WORK
    int target = -1;
#endif
    game_init();
    race_mode = 1; race_human = R_ZERG; race_bots = 1; race_skirmish_map = 0;
    race_skirmish_init();
    clear_walk_grid();
    assert(sc64_direct_firing_waypoint(200, 200, 300, 200, 0, 64, &nx, &ny));
    assert(race_save_game());
    assert(sc64_direct_firing_cache.valid);
#ifdef SC64_RANGE_WORK
    for (int i = 0; i < MAX_UNITS; ++i)
        if (target_alive(i)) { target = i; break; }
    assert(target >= 0);
    assert(sc64_range_route_next(target, 300, 200, 0, 64, 10, 10,
                                 &nx, &ny) == SC64_RANGE_ROUTE_PENDING);
    range_route_pump_one_tick();
    assert(sc64_range_route_active.valid);
#endif
    assert(race_load_game());
    assert(!sc64_direct_firing_cache.valid);
#ifdef SC64_RANGE_WORK
    assert(!sc64_range_route_active.valid);
    assert(!range_route_queued_target(target));
#endif
}
#endif

static void setup_combat_scene_type(int type)
{
    game_init();
    init_state();
    memset(u, 0, sizeof(u));
    memset(b, 0, sizeof(b));
    memset(race_player, 0xff, sizeof(race_player));
    race_player[SIDE] = race_player[ENEMY] = R_ZERG;
    race_mode = 1;
    race_human = R_ZERG;
    screen = SCREEN_PLAY;
    attacker = spawn_unit(SIDE, type, 400, 400);
    victim = spawn_unit(ENEMY, U_HYDRALISK, 400, 240);
    assert(attacker >= 0 && victim >= 0);
    u[attacker].raw_type = type == U_ZERGLING ? 37 : 38; u[attacker].raw_owner = 0;
    u[victim].raw_type = 38; u[victim].raw_owner = 1;
    u[attacker].hp = u[attacker].maxhp = 500;
    u[victim].hp = u[victim].maxhp = 500;
    clear_walk_grid();
    block_center_ring(400, 240);
}

static void setup_combat_scene(void)
{
    setup_combat_scene_type(U_HYDRALISK);
}

static void test_combat_hook_or_baseline(void)
{
    int nx, ny, fired = 0;
    setup_combat_scene();
    assert(!direct_path_clear(400, 400, 400, 240));
    assert(!path_next_tile(400, 400, 400, 240, &nx, &ny));
    tick_combat(&u[attacker], victim);
#ifdef SC64_DIRECT_FIRING_POINTS
    assert(sc64_direct_firing_cache.valid);
#ifdef SC64_RANGE_WORK
    assert(u[attacker].path_retry == 0 && u[attacker].path_timer == 23);
#else
    assert(u[attacker].path_retry == 0 && u[attacker].path_timer == 24);
#endif
    assert(u[attacker].path_x != u[victim].x || u[attacker].path_y != u[victim].y);
    assert_band(u[attacker].path_x, u[attacker].path_y,
                u[victim].x, u[victim].y, 0, 64);
    assert(direct_path_clear(400, 400, u[attacker].path_x, u[attacker].path_y));
#else
    assert(u[attacker].path_retry == 12 && u[attacker].path_timer == 0);
#endif
    for (int tick = 0; tick < 500; ++tick) {
#ifdef SC64_RANGE_WORK
        sc64_range_route_tick();
#endif
        int x = u[attacker].x, y = u[attacker].y;
        int hp = u[victim].hp;
        int distance = dist2(x, y, u[victim].x, u[victim].y);
        tick_combat(&u[attacker], victim);
        assert(abs(u[attacker].x - x) <= 4 && abs(u[attacker].y - y) <= 4);
        if (u[victim].hp != hp) {
            assert(distance <= 64 * 64);
            fired = 1; break;
        }
        if (distance > 64 * 64) assert(u[victim].hp == hp);
    }
#ifdef SC64_DIRECT_FIRING_POINTS
    assert(fired);
    assert(u[victim].hp == 493);
#else
    assert(!fired && u[attacker].x == 400 && u[attacker].y == 400);
#endif
}

static void test_one_corner_combat_moves_and_only_fires_in_range(void)
{
#ifdef SC64_DIRECT_FIRING_POINTS
    int fired = 0;
    int weapon_range = (sc64_race_unit_range_vs(U_HYDRALISK, 0) + 1) / 2;
    setup_combat_scene();
    u[attacker].x = 400; u[attacker].y = 400;
    u[victim].x = 520; u[victim].y = 400;
    clear_walk_grid();
    block_center_ring(u[victim].x, u[victim].y);
    for (int row = 0; row < MAP_ROWS; ++row)
        SC64_GRID_SET(path_walkable, 30 + row * MAP_COLS, 0);
    for (int row = 0; row < MAP_ROWS; ++row)
        if (row != 22) SC64_GRID_SET(path_walkable, 26 + row * MAP_COLS, 0);
    {
        int nx, ny;
        assert(!path_next_tile(u[attacker].x, u[attacker].y,
                               u[victim].x, u[victim].y, &nx, &ny));
        tick_combat(&u[attacker], victim);
        assert(sc64_direct_firing_cache.valid);
        assert(u[attacker].path_x == u[attacker].x &&
               u[attacker].path_y != u[attacker].y);
        assert(dist2(sc64_direct_firing_cache.x,
                     sc64_direct_firing_cache.y,
                     u[victim].x, u[victim].y) <= weapon_range * weapon_range);
    }
    for (int tick = 0; tick < 500; ++tick) {
        int x = u[attacker].x, y = u[attacker].y;
        int hp = u[victim].hp;
        int distance = dist2(x, y, u[victim].x, u[victim].y);
        tick_combat(&u[attacker], victim);
        assert(abs(u[attacker].x - x) <= 4 && abs(u[attacker].y - y) <= 4);
        if (u[victim].hp != hp) {
            assert(distance <= weapon_range * weapon_range);
            fired = 1;
            break;
        }
        if (distance > weapon_range * weapon_range) assert(u[victim].hp == hp);
    }
    assert(fired && u[victim].hp < 500);
#endif
}

static void test_melee_and_explicit_move_are_unchanged(void)
{
    setup_combat_scene_type(U_ZERGLING);
    for (int tick = 0; tick < 100; ++tick) tick_combat(&u[attacker], victim);
    assert(u[attacker].x == 400 && u[attacker].y == 400 && u[victim].hp == 500);
#ifdef SC64_DIRECT_FIRING_POINTS
    assert(!sc64_direct_firing_cache.valid);
#endif
    setup_combat_scene();
    for (int tick = 0; tick < 100; ++tick) move_toward(&u[attacker], 400, 240);
    assert(u[attacker].x == 400 && u[attacker].y == 400 && u[victim].hp == 500);
#ifdef SC64_DIRECT_FIRING_POINTS
    assert(!sc64_direct_firing_cache.valid);
#endif
}

#ifdef SC64_RANGE_WORK
/* Exercise real slot reuse with an active copy, a snapshot and a separate
 * pending target. Reuse must not discard the unrelated pending request. */
static void test_range_target_slot_reuse(void)
{
    sc64_range_route_reset();
    memset(u,0,sizeof(u));
    sc64_range_route_active=(Sc64RangeRouteActive){.target=0,.valid=1};
    sc64_range_route_snapshot_key=(Sc64RangeRouteSnapshotKey){.target=0,.valid=1};
    sc64_range_route_snapshot_slot=0;sc64_route_valid[0]=3;
    sc64_range_route_copy_slot=1;sc64_route_valid[1]=3;
    sc64_range_route_pending[0]=(Sc64RangeRouteJob){.target=0,.valid=1};
    sc64_range_route_pending[1]=(Sc64RangeRouteJob){.target=1,.valid=1};
    sc64_range_route_queue_head=0;sc64_range_route_queue_count=2;
    assert(spawn_unit(ENEMY,U_MARINE,128,128)==0);
    assert(!sc64_range_route_active.valid && sc64_range_route_copy_slot==-1);
    assert(sc64_range_route_snapshot_slot==-1 && !sc64_range_route_snapshot_key.valid);
    assert(sc64_route_valid[0]==0 && sc64_route_valid[1]==0);
    assert(sc64_range_route_queue_count==1 && sc64_range_route_pending[0].target==1);
    sc64_range_route_reset();
}
/* A real lethal hit invalidates pending work at the next bounded worker tick. */
static void test_range_dead_target_cleanup(void)
{
    setup_combat_scene();
    sc64_range_route_active=(Sc64RangeRouteActive){.target=(u16)victim,.valid=1};
    sc64_range_route_snapshot_key=(Sc64RangeRouteSnapshotKey){.target=(u16)victim,.valid=1};
    sc64_range_route_snapshot_slot=0;sc64_route_valid[0]=3;
    sc64_range_route_copy_slot=1;sc64_route_valid[1]=3;
    sc64_range_route_pending[0]=(Sc64RangeRouteJob){.target=(u16)victim,.valid=1};
    sc64_range_route_queue_head=0;sc64_range_route_queue_count=1;
    hit_target(victim,1000,U_MARINE,SIDE);assert(!u[victim].alive);
    sc64_range_route_tick();
    assert(!sc64_range_route_active.valid && !sc64_range_route_queue_count);
    assert(sc64_range_route_snapshot_slot==-1 && sc64_range_route_copy_slot==-1);
    assert(!sc64_route_valid[0] && !sc64_route_valid[1]);
}

/* A depot flood must not treat a tagged range copy as a matching depot or
 * evict its storage while the bounded copy is in progress. */
static void test_range_copy_depot_isolation(void)
{
    int slot;
    clear_walk_grid();sc64_routes_reset();
    sc64_range_route_copy_slot=0;sc64_route_valid[0]=3;
    sc64_route_goal[0]=(u16)(SC64_ROUTE_DEPOT_TAG | 0u);
    slot=sc64_route_depot_slot_for_origin(0,160,160,-1);
    assert(slot>0 && sc64_range_route_copy_slot==0 && sc64_route_valid[0]==3);
    assert(sc64_route_valid[slot]==1 && sc64_route_goal[slot]==SC64_ROUTE_DEPOT_TAG);
    sc64_range_route_reset();
}
#endif

int main(void)
{
    gba_init();
#ifdef SC64_DIRECT_FIRING_POINTS
    test_bounded_search_key_and_origin_revalidation();
    test_one_corner_paths_cache_final_firing_tile();
    test_obstructed_candidates_keep_baseline_retry();
#endif
    test_combat_hook_or_baseline();
    test_one_corner_combat_moves_and_only_fires_in_range();
    test_melee_and_explicit_move_are_unchanged();
#ifdef SC64_RANGE_WORK
    test_range_work_scheduler_contracts();
    test_range_target_slot_reuse();
    test_range_dead_target_cleanup();
    test_range_copy_depot_isolation();
#endif
#ifdef SC64_DIRECT_FIRING_POINTS
    setup_combat_scene();
    {
        int nx, ny;
        assert(sc64_direct_firing_waypoint(400, 400, 400, 240, 0, 64,
                                           &nx, &ny));
        u[attacker].path_x = nx; u[attacker].path_y = ny;
    }
    test_save_load_invalidates_cache();
#endif
    puts("Direct firing corridors actual-engine fixture passed");
    return 0;
}
