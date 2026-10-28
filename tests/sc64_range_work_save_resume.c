/* SPDX-License-Identifier: GPL-3.0-or-later
 * Actual-engine save-boundary continuity for opt-in range routing. */
#ifndef SC64_RANGE_WORK
#error "Build this fixture with SC64_RANGE_WORK"
#endif
#ifndef SC64_DIRECT_ATTACK
#define SC64_DIRECT_ATTACK
#endif
#include "../starcraft/game.c"
#include <assert.h>
#include <stdio.h>
#include <string.h>

enum { RANGE_SAVE_CONTINUE_TICKS = 500 };

typedef struct {
    uint32_t world;
    int frame, attacker_x, attacker_y, attacker_order, attacker_target;
    int attacker_cd, attacker_path_x, attacker_path_y;
    int attacker_path_timer, attacker_path_retry;
    int target_x, target_y, target_order, target_hp;
    unsigned worker_active, worker_phase, worker_cursor, worker_queue;
} RangeSaveLiveMark;

static RangeSaveLiveMark range_save_reference[RANGE_SAVE_CONTINUE_TICKS];
static u16 range_save_previous_input;
static int range_save_attacker, range_save_target;

static void range_save_input(u16 held)
{
    game_update(held, held & (u16)~range_save_previous_input);
    range_save_previous_input = held;
}

static void range_save_tap(u16 key)
{
    range_save_input(key);
    range_save_input(0);
}

static void range_save_to_row(int row)
{
    for (int step = 0; race_front.row != row && step < 16; ++step)
        range_save_tap(race_front.row < row ? KEY_DOWN : KEY_UP);
    assert(race_front.row == row);
}

static void range_save_set_field(int *field, int value)
{
    for (int step = 0; *field != value && step < 3; ++step)
        range_save_tap(KEY_RIGHT);
    assert(*field == value);
}

static void range_save_launch_skirmish(void)
{
    game_init(); range_save_previous_input = 0;
    range_save_tap(KEY_DOWN); range_save_tap(KEY_A);
    assert(screen == SCREEN_TITLE && race_front.page == SC_FRONT_SKIRMISH);
    range_save_to_row(0); range_save_set_field(&race_front.map, 0);
    range_save_to_row(1); range_save_set_field(&race_front.race, R_TER);
    range_save_to_row(2); range_save_set_field(&race_front.bots, 1);
    range_save_to_row(3); range_save_set_field(&race_front.bot_races[0], R_ZERG);
    range_save_to_row(4); range_save_tap(KEY_A);
    assert(screen == SCREEN_PLAY && race_mode && race_skirmish_map == 0);
}

static u32 range_save_hash_word(u32 hash, u32 value)
{
    for (unsigned byte = 0; byte < 4; ++byte) {
        hash ^= (u8)value;
        hash *= 16777619u;
        value >>= 8;
    }
    return hash;
}

static RangeSaveLiveMark range_save_live_mark(void)
{
    RangeSaveLiveMark mark = {0};
    u32 hash = 2166136261u;
    const Unit *attacker = &u[range_save_attacker];
    const Unit *target = &u[range_save_target];
#define RANGE_SAVE_MIX(value) (hash = range_save_hash_word(hash, (u32)(value)))
    mark.frame = frame;
    RANGE_SAVE_MIX(frame); RANGE_SAVE_MIX(seed); RANGE_SAVE_MIX(minerals);
    RANGE_SAVE_MIX(gas); RANGE_SAVE_MIX(supply_used); RANGE_SAVE_MIX(supply_cap);
    for (int i = 0; i < MAX_UNITS; ++i) {
        const Unit *q = &u[i];
        RANGE_SAVE_MIX(q->alive);
        if (!q->alive) continue;
        RANGE_SAVE_MIX(q->owner); RANGE_SAVE_MIX(q->type);
        RANGE_SAVE_MIX(q->x); RANGE_SAVE_MIX(q->y); RANGE_SAVE_MIX(q->hp);
        RANGE_SAVE_MIX(q->maxhp); RANGE_SAVE_MIX(q->order); RANGE_SAVE_MIX(q->target);
        RANGE_SAVE_MIX(q->tx); RANGE_SAVE_MIX(q->ty); RANGE_SAVE_MIX(q->cd);
        RANGE_SAVE_MIX(q->path_x); RANGE_SAVE_MIX(q->path_y);
        RANGE_SAVE_MIX(q->path_timer); RANGE_SAVE_MIX(q->path_retry);
#ifdef SC64_RACES
        RANGE_SAVE_MIX(q->shield); RANGE_SAVE_MIX(q->raw_type);
        RANGE_SAVE_MIX(q->raw_owner); RANGE_SAVE_MIX(q->flags);
#endif
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i) {
        const Building *q = &b[i];
        RANGE_SAVE_MIX(q->alive);
        if (!q->alive) continue;
        RANGE_SAVE_MIX(q->owner); RANGE_SAVE_MIX(q->type);
        RANGE_SAVE_MIX(q->x); RANGE_SAVE_MIX(q->y); RANGE_SAVE_MIX(q->hp);
        RANGE_SAVE_MIX(q->maxhp); RANGE_SAVE_MIX(q->under_construction);
        RANGE_SAVE_MIX(q->build_progress); RANGE_SAVE_MIX(q->queue_count);
        RANGE_SAVE_MIX(q->queue_progress);
    }
    for (int i = 0; i < MAX_MINERALS; ++i) {
        RANGE_SAVE_MIX(m[i].x); RANGE_SAVE_MIX(m[i].y); RANGE_SAVE_MIX(m[i].amount);
    }
    for (int cell = 0; cell < MAP_CELLS; ++cell)
        RANGE_SAVE_MIX(SC64_GRID_GET(explored, cell));
    mark.world = hash;
    mark.attacker_x = attacker->x; mark.attacker_y = attacker->y;
    mark.attacker_order = attacker->order; mark.attacker_target = attacker->target;
    mark.attacker_cd = attacker->cd; mark.attacker_path_x = attacker->path_x;
    mark.attacker_path_y = attacker->path_y; mark.attacker_path_timer = attacker->path_timer;
    mark.attacker_path_retry = attacker->path_retry;
    mark.target_x = target->x; mark.target_y = target->y;
    mark.target_order = target->order; mark.target_hp = target->hp;
    mark.worker_active = sc64_range_route_active.valid;
    mark.worker_phase = sc64_range_route_work.phase;
    mark.worker_cursor = sc64_range_route_work.cursor;
    mark.worker_queue = sc64_range_route_queue_count;
#undef RANGE_SAVE_MIX
    return mark;
}

static int range_save_same_game(RangeSaveLiveMark a, RangeSaveLiveMark b)
{
    return a.world == b.world && a.frame == b.frame &&
        a.attacker_x == b.attacker_x && a.attacker_y == b.attacker_y &&
        a.attacker_order == b.attacker_order && a.attacker_target == b.attacker_target &&
        a.attacker_cd == b.attacker_cd && a.attacker_path_x == b.attacker_path_x &&
        a.attacker_path_y == b.attacker_path_y &&
        a.attacker_path_timer == b.attacker_path_timer &&
        a.attacker_path_retry == b.attacker_path_retry &&
        a.target_x == b.target_x && a.target_y == b.target_y &&
        a.target_order == b.target_order && a.target_hp == b.target_hp;
}

static int range_save_same_live(RangeSaveLiveMark a, RangeSaveLiveMark b)
{
    return range_save_same_game(a, b) &&
        a.worker_active == b.worker_active && a.worker_phase == b.worker_phase &&
        a.worker_cursor == b.worker_cursor && a.worker_queue == b.worker_queue;
}

static void test_live_save_boundary_restarts_range_worker(void)
{
    const int attacker_x = 20 * 16 + 8, target_x = 42 * 16 + 8;
    const int y = 30 * 16 + 8;
    int next_x = -1, next_y = -1;
    range_save_launch_skirmish();
    memset(u, 0, sizeof(u));
    supply_used = 0;
    sc64_visibility_reset(); sc64_walk_reset_all();
#ifdef SC64_UNIT_FACING
    sc64_facing_reset_all();
#endif
    sc64_range_route_reset(); sc64_direct_firing_cache_invalidate();
    range_save_attacker = spawn_unit(SIDE, U_TANK, attacker_x, y);
    range_save_target = spawn_unit(ENEMY, U_HYDRALISK, target_x, y);
    assert(range_save_attacker >= 0 && range_save_target >= 0);
    u[range_save_attacker].order = ORDER_ATTACK_TARGET;
    u[range_save_attacker].target = range_save_target;
    u[range_save_attacker].tx = target_x; u[range_save_attacker].ty = y;
    u[range_save_attacker].cd = u[range_save_attacker].path_timer = 0;
    u[range_save_attacker].path_retry = 0;
    u[range_save_target].order = ORDER_ATTACK_MOVE;
    u[range_save_target].tx = target_x; u[range_save_target].ty = y;
    u[range_save_target].hp = u[range_save_target].maxhp = 65535;
    assert(spawn_build(SIDE, B_COMMAND, 48 * 16 + 8, y, 0) >= 0);
    menu_open = 0; help = 0;
    update_exploration();
    assert(currently_visible(target_x, y));
    path_grid_valid = 0; build_path_grid();
    /* Map 0 retains its Barricade divider at column 31 and its original gaps. */
    assert(!SC64_GRID_GET(path_walkable, 31 + 30 * MAP_COLS));
    assert(SC64_GRID_GET(path_walkable, 31 + 15 * MAP_COLS));
    assert(SC64_GRID_GET(path_walkable, 31 + 47 * MAP_COLS));
    assert(!direct_path_clear(attacker_x, y, target_x, y));
    assert(!sc64_direct_firing_waypoint(attacker_x, y, target_x, y,
        0, unit_range(U_TANK), &next_x, &next_y));

    game_update(0, 0);
    assert(sc64_range_route_queue_count == 1 &&
           sc64_range_route_pending[sc64_range_route_queue_head].target == range_save_target);
    game_update(0, 0);
    assert(u[range_save_attacker].order == ORDER_ATTACK_TARGET &&
           u[range_save_attacker].target == range_save_target);
    assert(sc64_range_route_active.valid &&
           sc64_range_route_active.target == range_save_target &&
           sc64_range_route_work.phase != SC64_RANGE_WORK_IDLE &&
           sc64_range_route_work.cursor > 0);
#if SC64_RANGE_WORK_BUDGET_PER_TICK >= 96u
    assert(sc64_range_route_work.phase == SC64_RANGE_WORK_SEED);
#endif

#ifdef SC64_T4_RUNTIME
    {
        int invalid_slot = -1;
        Sc64RangeRouteJob previous_job = sc64_range_route_active;
        Sc64RangeWork previous_work = sc64_range_route_work;
        unsigned previous_queue = sc64_range_route_queue_count;
        Building previous_building;
        for (int i = 0; i < MAX_BUILDINGS; ++i)
            if (b[i].alive) { invalid_slot = i; break; }
        assert(invalid_slot >= 0);
        previous_building = b[invalid_slot];
        b[invalid_slot].type = B_TERRAN_BEACON;
        assert(!race_save_game());
        b[invalid_slot] = previous_building;
        assert(sc64_range_route_active.valid == previous_job.valid &&
               sc64_range_route_active.target == previous_job.target &&
               sc64_range_route_active.tx == previous_job.tx &&
               sc64_range_route_work.phase == previous_work.phase &&
               sc64_range_route_work.cursor == previous_work.cursor &&
               sc64_range_route_queue_count == previous_queue);
    }
#endif

    RangeSaveLiveMark saved = range_save_live_mark();
    assert(race_save_game());
    assert(!sc64_range_route_active.valid &&
           sc64_range_route_work.phase == SC64_RANGE_WORK_IDLE &&
           !sc64_range_route_queue_count && sc64_range_route_snapshot_slot < 0 &&
           sc64_range_route_copy_slot < 0);
    for (int tick = 0; tick < RANGE_SAVE_CONTINUE_TICKS; ++tick) {
        game_update(0, 0);
        range_save_reference[tick] = range_save_live_mark();
    }
    game_init(); range_save_previous_input = 0;
    range_save_input(KEY_L);
    assert(screen == SCREEN_PLAY && race_mode);
    assert(range_save_same_game(saved, range_save_live_mark()));
    assert(!sc64_range_route_active.valid);
    for (int tick = 0; tick < RANGE_SAVE_CONTINUE_TICKS; ++tick) {
        RangeSaveLiveMark resumed;
        game_update(0, 0);
        resumed = range_save_live_mark();
        if (!range_save_same_live(range_save_reference[tick], resumed)) {
            printf("range save divergence tick=%d frame=%d ref=%08x load=%08x tank=(%d,%d path=%d,%d timer=%d retry=%d) load=(%d,%d path=%d,%d timer=%d retry=%d) worker=%u/%u/%u queue=%u\n",
                tick + 1, resumed.frame, range_save_reference[tick].world, resumed.world,
                range_save_reference[tick].attacker_x, range_save_reference[tick].attacker_y,
                range_save_reference[tick].attacker_path_x, range_save_reference[tick].attacker_path_y,
                range_save_reference[tick].attacker_path_timer, range_save_reference[tick].attacker_path_retry,
                resumed.attacker_x, resumed.attacker_y, resumed.attacker_path_x,
                resumed.attacker_path_y, resumed.attacker_path_timer, resumed.attacker_path_retry,
                range_save_reference[tick].worker_phase, range_save_reference[tick].worker_cursor,
                resumed.worker_phase, resumed.worker_queue);
            assert(!"successful save must canonicalize transient range work");
        }
    }
}

#if SC64_RANGE_WORK_BUDGET_PER_TICK < 96u
static void range_save_pump_worker_tick(void)
{
    ++frame;
    sc64_range_route_tick();
    assert(sc64_range_route_last_units <= SC64_RANGE_WORK_BUDGET_PER_TICK);
}

static void test_save_restarts_partial_snapshot_copy(void)
{
    const int target_x = 10 * 16 + 8, target_y = 10 * 16 + 8;
    int next_x = -1, next_y = -1;
    sc64_range_route_reset();
    memset(path_walkable, 0, sizeof(path_walkable));
    SC64_GRID_SET(path_walkable, 10 + 10 * MAP_COLS, 1);
    path_grid_valid = 1;
    assert(sc64_range_route_next(80, target_x, target_y, 0, 0, 10, 10,
        &next_x, &next_y) == SC64_RANGE_ROUTE_PENDING);
    for (unsigned tick = 0; tick < 1000u; ++tick) {
        if (sc64_range_route_active.valid &&
            sc64_range_route_work.phase == SC64_RANGE_WORK_DONE) break;
        (void)sc64_range_route_next(80, target_x, target_y, 0, 0, 10, 10,
                                    &next_x, &next_y);
        range_save_pump_worker_tick();
    }
    assert(sc64_range_route_active.valid &&
           sc64_range_route_work.phase == SC64_RANGE_WORK_DONE);
    assert(sc64_range_route_next(81, target_x + 16, target_y, 0, 0, 10, 10,
        &next_x, &next_y) == SC64_RANGE_ROUTE_PENDING);
    frame = (int)(sc64_range_route_complete_frame +
                  SC64_RANGE_ROUTE_RESULT_LEASE);
    range_save_pump_worker_tick();
    assert(sc64_range_route_copy_slot >= 0 &&
           sc64_range_route_copy_cursor > 0 &&
           sc64_range_route_copy_cursor <
               sc64_range_route_work.gradient_bytes);
    assert(race_save_game());
    assert(!sc64_range_route_active.valid &&
           sc64_range_route_work.phase == SC64_RANGE_WORK_IDLE &&
           !sc64_range_route_queue_count && sc64_range_route_snapshot_slot < 0 &&
           sc64_range_route_copy_slot < 0 && !sc64_range_route_copy_cursor);
}
#endif

int main(void)
{
    gba_init();
    test_live_save_boundary_restarts_range_worker();
#if SC64_RANGE_WORK_BUDGET_PER_TICK < 96u
    test_save_restarts_partial_snapshot_copy();
#endif
    puts("range-work live save-boundary continuation matched 500 game updates");
    return 0;
}
