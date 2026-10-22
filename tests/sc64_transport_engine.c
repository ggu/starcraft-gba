/* Actual-engine T5 transport integration fixtures. Every world edit here is
 * explicit test setup; these checks are not controller/gameplay proof. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

#ifndef SC64_T5_RUNTIME
#error "transport engine fixture requires the experimental T5 runtime"
#endif

static void
start_t5(void)
{
    game_init();
    race_mode = 0;
    race_selected_mission = SC64_T5_GLOBAL_ID;
    init_state();
    assert(race_t5_active() && race_cols == 96 && race_rows == 96);
    screen = SCREEN_PLAY;
}

static Unit *
unit_at(int slot, int raw_type, int owner, int raw_owner, int x, int y, int hp)
{
    int type = sc64_race_unit_from_raw(raw_type);
    assert(type >= 0);
    Unit *q = &u[slot];
    memset(q, 0, sizeof(*q));
    q->alive = 1;
    q->type = type;
    q->raw_type = raw_type;
    q->owner = owner;
    q->raw_owner = raw_owner;
    q->x = x; q->y = y;
    q->hp = q->maxhp = hp;
    q->target = -1;
    return q;
}

static int
free_unit_slot(void)
{
    for (int i = 0; i < MAX_UNITS; ++i) if (!u[i].alive) return i;
    assert(!"fixture needs a free unit slot");
    return -1;
}

static int
fixture_unit(int raw_type, int owner, int raw_owner, int x, int y, int hp)
{
    int slot = free_unit_slot();
    assert(unit_at(slot, raw_type, owner, raw_owner, x, y, hp));
    return slot;
}

static int
count_owner_units(int owner)
{
    int count = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == owner) ++count;
    return count;
}

static int
find_open_horizontal_run(int run, int *x, int *y)
{
    for (int row = 3; row < MAP_ROWS - 3; ++row)
        for (int col = 3; col < MAP_COLS - run - 3; ++col) {
            int open = 1;
            for (int step = 0; step < run; ++step)
                if (terrain_blocked(col + step, row)) { open = 0; break; }
            int px = col * 16 + 8, py = row * 16 + 8;
            int right = px + run * 16;
            for (int i = 0; i < MAX_UNITS && open; ++i)
                if (u[i].alive && !SC64_TRANSPORT_LOADED(&u[i]) &&
                    u[i].x >= px - 32 && u[i].x <= right + 32 &&
                    u[i].y >= py - 32 && u[i].y <= py + 32) open = 0;
            for (int i = 0; i < MAX_BUILDINGS && open; ++i)
                if (b[i].alive && b[i].x >= px - 48 && b[i].x <= right + 48 &&
                    b[i].y >= py - 48 && b[i].y <= py + 48) open = 0;
            if (open) {
                *x = px;
                *y = py;
                return 1;
            }
        }
    return 0;
}

static int
find_fully_blocked_patch(int radius, int *x, int *y)
{
    for (int row = radius + 1; row < MAP_ROWS - radius - 1; ++row)
        for (int col = radius + 1; col < MAP_COLS - radius - 1; ++col) {
            int blocked = 1;
            for (int dy = -radius; dy <= radius && blocked; ++dy)
                for (int dx = -radius; dx <= radius; ++dx)
                    if (!terrain_blocked(col + dx, row + dy)) {
                        blocked = 0; break;
                    }
            if (blocked) {
                *x = col * 16 + 8;
                *y = row * 16 + 8;
                return 1;
            }
        }
    return 0;
}

static void
request_board_and_advance(int passenger, int ship)
{
    assert(race_transport_request_board(passenger, ship));
    for (int i = 0; i < 600 && !SC64_TRANSPORT_LOADED(&u[passenger]); ++i) {
        game_update(0, 0);
        if (screen != SCREEN_PLAY) {
            fprintf(stderr, "paused frame=%d fault=%u trigger=%u slot=%u outcome=%u cursor0=%u wait=%u active=%02x elapsed=%u\n",
                    frame, race_t5.player[0].fault,
                    race_t5.player[0].fault_trigger, race_t5.player[0].fault_slot,
                    race_t5.player[0].outcome, race_t5.player[0].cursor[0],
                    race_t5.player[0].wait_ticks, race_t5.active_mask,
                    race_t5.player[0].elapsed_ticks);
            break;
        }
    }
    if (!SC64_TRANSPORT_LOADED(&u[passenger]))
        fprintf(stderr, "board timeout slot=%d order=%d target=%d xy=%d,%d ship=%d,%d screen=%d won=%d lost=%d frame=%d\n",
                passenger, u[passenger].order, u[passenger].target, u[passenger].x,
                u[passenger].y, u[ship].x, u[ship].y, screen, won, lost, frame);
    assert(SC64_TRANSPORT_LOADED(&u[passenger]));
    assert(u[passenger].order == ORDER_TRANSPORTED);
    assert(u[passenger].target == ship);
}

static void
test_boarding_capacity_and_unobservable_cargo(void)
{
    start_t5();
    int x, y;
    assert(find_open_horizontal_run(12, &x, &y));
    int ship_slot = fixture_unit(11, SIDE, 0, x, y, 750);
    int marine_slot = fixture_unit(0, SIDE, 0, x + 48, y, 40);
    int raynor_slot = fixture_unit(19, SIDE, 0, x + 80, y, 300);
    int vulture2_slot = fixture_unit(19, SIDE, 0, x + 112, y, 300);
    int vulture3_slot = fixture_unit(19, SIDE, 0, x + 144, y, 300);
    int enemy_slot = fixture_unit(32, ENEMY, 6, x + 16, y + 16, 35);
    int extra_slot = fixture_unit(0, SIDE, 0, x + 176, y, 40);
    int capacity_slot = fixture_unit(0, SIDE, 0, x + 184, y, 40);
    Unit *ship = &u[ship_slot], *marine = &u[marine_slot];
    Unit *raynor = &u[raynor_slot], *vulture2 = &u[vulture2_slot];
    Unit *vulture3 = &u[vulture3_slot], *enemy = &u[enemy_slot];
    Unit *extra = &u[extra_slot];
    assert(race_transport_cargo_space(ship_slot) == 0);

    int initial_supply = race_owner_supply(SIDE);
    supply_used = initial_supply;
    request_board_and_advance(marine_slot, ship_slot);
    request_board_and_advance(raynor_slot, ship_slot);
    request_board_and_advance(vulture2_slot, ship_slot);
    request_board_and_advance(vulture3_slot, ship_slot);
    request_board_and_advance(extra_slot, ship_slot);
    assert(race_transport_cargo_space(ship_slot) == 8);
    assert(supply_used == initial_supply && race_owner_supply(SIDE) == initial_supply);
    assert(!race_transport_request_board(capacity_slot, ship_slot)); /* full 8-point capacity */
    assert(!race_transport_request_board(enemy_slot, ship_slot)); /* hostile unit */

    /* Cargo remains alive and counted, but it is no longer targetable,
       selectable, or a vision source. */
    assert(marine->alive && count_owner_units(SIDE) >= 6);
    assert(!target_alive(marine_slot));
    assert(nearest_enemy_typed(marine->x, marine->y, ENEMY, 96, U_ZERGLING) != 0);
    enemy->x = marine->x; enemy->y = marine->y;
    int saved_hp = marine->hp;
    tick_combat(enemy, marine_slot);
    assert(marine->hp == saved_hp);

    box_anchor_x = marine->x - 2; box_end_x = marine->x + 2;
    box_anchor_y = marine->y - 2; box_end_y = marine->y + 2;
    select_box_units();
    assert(!marine->selected);

    /* Move only the carrier in this isolated sight fixture. On an ordinary
       update the passenger is synchronized to it after orders. */
    ship->x = x + 400; ship->y = y + 200;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && !SC64_TRANSPORT_LOADED(&u[i]) &&
            &u[i] != ship) u[i].alive = 0;
    memset(visible, 0, sizeof(visible));
    memset(explored, 0, sizeof(explored));
    sc64_visibility_reset();
    update_exploration();
    unsigned passenger_cell = (unsigned)(marine->x / 16) +
                              (unsigned)(marine->y / 16) * MAP_COLS;
    assert(!SC64_GRID_GET(visible, passenger_cell));
    assert(!SC64_GRID_GET(explored, passenger_cell));
    (void)ship;
}

static void
test_blocked_then_successful_unload(void)
{
    start_t5();
    int x, y, blocked_x, blocked_y;
    assert(find_open_horizontal_run(4, &x, &y));
    assert(find_fully_blocked_patch(2, &blocked_x, &blocked_y));
    int ship_slot = fixture_unit(11, SIDE, 0, x, y, 750);
    int passenger_slot = fixture_unit(0, SIDE, 0, x + 8, y, 40);
    Unit *ship = &u[ship_slot];
    request_board_and_advance(passenger_slot, ship_slot);

    assert(race_transport_request_unload(ship_slot, blocked_x, blocked_y));
    for (int i = 0; i < 600; ++i) game_update(0, 0);
    assert(SC64_TRANSPORT_LOADED(&u[passenger_slot]));
    assert(u[passenger_slot].alive && u[passenger_slot].order == ORDER_TRANSPORTED);

    /* Directly retargeting the ship is test setup; it proves a blocked
       landing did not discard cargo before retrying at a clear patch. */
    int clear_x = x + 32, clear_y = y;
    assert(!terrain_blocked(clear_x / 16, clear_y / 16));
    assert(race_transport_request_unload(ship_slot, clear_x, clear_y));
    for (int i = 0; i < 800 && SC64_TRANSPORT_LOADED(&u[passenger_slot]); ++i)
        game_update(0, 0);
    assert(!SC64_TRANSPORT_LOADED(&u[passenger_slot]));
    assert(u[passenger_slot].alive && u[passenger_slot].owner == SIDE &&
           u[passenger_slot].raw_owner == 0);
    assert(u[passenger_slot].order != ORDER_TRANSPORTED &&
           u[passenger_slot].cd == 19);
    (void)ship;
}

static void
test_save_load_preserves_cargo_and_rejects_bad_reference_atomically(void)
{
    start_t5();
    int x, y;
    assert(find_open_horizontal_run(3, &x, &y));
    int ship_slot = fixture_unit(11, SIDE, 0, x, y, 750);
    int passenger_slot = fixture_unit(16, SIDE, 0, x + 8, y, 250);
    Unit *ship = &u[ship_slot];
    request_board_and_advance(passenger_slot, ship_slot);
    assert(race_save_game());
    RaceSave saved = race_data;
    memset(&u[passenger_slot], 0, sizeof(u[passenger_slot]));
    assert(race_load_game());
    assert(SC64_TRANSPORT_LOADED(&u[passenger_slot]) &&
           u[passenger_slot].target == ship_slot);

    /* A checksummed but malformed cargo pointer must fail before applying
       any candidate state to the live world. */
    RaceSave bad = saved;
    bad.units[passenger_slot][offsetof(Unit, target) / sizeof(int)] = MAX_UNITS;
    save_write(&bad, sizeof(bad), SAVE_VERSION);
    int before_hp = u[passenger_slot].hp, before_x = u[passenger_slot].x;
    assert(!race_load_game());
    assert(u[passenger_slot].alive && u[passenger_slot].hp == before_hp &&
           u[passenger_slot].x == before_x &&
           SC64_TRANSPORT_LOADED(&u[passenger_slot]) &&
           u[passenger_slot].target == ship_slot);
    save_write(&saved, sizeof(saved), SAVE_VERSION);
    assert(race_load_game());
    (void)ship;
}

static void
test_unload_with_carrier_after_passenger_slot(void)
{
    start_t5();
    int x, y;
    assert(find_open_horizontal_run(4, &x, &y));
    int passenger_slot = fixture_unit(0, SIDE, 0, x + 8, y, 40);
    int ship_slot = fixture_unit(11, SIDE, 0, x, y, 750);
    assert(passenger_slot < ship_slot);
    request_board_and_advance(passenger_slot, ship_slot);
    assert(race_transport_request_unload(ship_slot, x + 32, y));
    for (int i = 0; i < 160 && SC64_TRANSPORT_LOADED(&u[passenger_slot]); ++i)
        game_update(0, 0);
    assert(!SC64_TRANSPORT_LOADED(&u[passenger_slot]));
    assert(u[passenger_slot].alive && u[passenger_slot].cd == 19);
    assert(u[ship_slot].work_timer == 19);
}

static void
test_transport_death_kills_cargo_before_t5_query(void)
{
    start_t5();
    int x, y;
    assert(find_open_horizontal_run(3, &x, &y));
    int ship_slot = fixture_unit(11, SIDE, 0, x, y, 1);
    /* Fixture setup isolates the human-owned hero-loss predicate while
       leaving the rest of the imported scenario intact. */
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].raw_type == 16) u[i].alive = 0;
    int passenger_slot = fixture_unit(16, SIDE, 0, x + 8, y, 250);
    Unit *ship = &u[ship_slot], *hero = &u[passenger_slot];
    hero->x = x + 8; hero->y = y;
    hero->tx = hero->x; hero->ty = hero->y; hero->target = -1;
    hero->order = ORDER_IDLE; hero->path_timer = hero->path_retry = 0;
    invincible_units[passenger_slot] = 1; /* Cargo still dies if carrier dies. */
    request_board_and_advance(passenger_slot, ship_slot);
    hit_target(ship_slot, 1000, U_MARINE, ENEMY);
    assert(!ship->alive && !hero->alive);
    Sc64T4Condition hero_count = {0};
    u32 count = 99;
    hero_count.opcode = 2; hero_count.player = 0; hero_count.unit = 16;
    assert(sc64_t5_world_count(&race_t5, &hero_count, 0, &count));
    assert(count == 0); /* The following trigger query sees cargo already dead. */
    game_update(0, 0);
    assert(screen == SCREEN_PLAY && !race_t5.player[0].fault);
}

int
main(void)
{
    test_boarding_capacity_and_unobservable_cargo();
    test_blocked_then_successful_unload();
    test_unload_with_carrier_after_passenger_slot();
    test_save_load_preserves_cargo_and_rejects_bad_reference_atomically();
    test_transport_death_kills_cargo_before_t5_query();
    puts("T5 transport actual-engine fixtures passed (not gameplay proof)");
    return 0;
}
