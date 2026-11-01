/* P5 Shuttle lifecycle and save checks over the real engine and normal UI.
 * Fixture placements below isolate cargo mechanics; this is not a campaign
 * win replay or a claim of native BW transport-order timing. */
#define main sc64_p5_runtime_base_main
#include "sc64_p5_runtime.c"
#undef main

#ifndef SC64_P5_SHUTTLE_ENABLED
#error "Compile with the opt-in P5 Shuttle profile flags"
#endif

static void tap(u16 key) { input(key); input(0); }

static int snap4_local(int current, int target, int bound)
{
    int delta = target - current;
    int steps = delta >= 0 ? (delta + 2) / 4 : (delta - 2) / 4;
    int result = current + steps * 4;
    return result < 0 ? 0 : result >= bound ? bound - 1 : result;
}

static void move_cursor(int x, int y)
{
    x = snap4_local(cx, x, WORLD_W);
    y = snap4_local(cy, y, WORLD_H);
    int guard = 0;
    while ((cx != x || cy != y) && guard++ < 2000 &&
           screen == SCREEN_PLAY && !won && !lost) {
        u16 held = KEY_R;
        if (cx < x) held |= KEY_RIGHT;
        if (cx > x) held |= KEY_LEFT;
        if (cy < y) held |= KEY_DOWN;
        if (cy > y) held |= KEY_UP;
        input(held);
    }
    input(0);
}

static int shuttle_slot(void)
{
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].raw_type == 69 &&
            u[i].type == U_P5_SHUTTLE) return i;
    return -1;
}

static int make_passenger(int raw, int type, int x, int y)
{
    int slot = spawn_unit(SIDE, type, x, y);
    assert(slot >= 0);
    u[slot].raw_type = raw;
    u[slot].raw_owner = 7;
    u[slot].x = x;
    u[slot].y = y;
    u[slot].selected = 0;
    return slot;
}

static int select_unit_by_slot(int slot)
{
    if (slot < 0 || slot >= MAX_UNITS || !u[slot].alive || u[slot].owner != SIDE)
        return 0;
    if (selected_building >= 0) return 0;
    for (int i = 0; i <= MAX_UNITS; ++i) {
        if (u[slot].selected && selected_count() == 1) return 1;
        tap(KEY_L);
    }
    return u[slot].selected && selected_count() == 1;
}

static int issue_move(int slot, int x, int y)
{
    if (!select_unit_by_slot(slot)) return 0;
    tap(KEY_B); tap(KEY_A);
    if (mode != MODE_MOVE) return 0;
    move_cursor(x, y); tap(KEY_A);
    return u[slot].alive && u[slot].order == ORDER_MOVE;
}

static int issue_board_by_input(int passenger, int carrier)
{
    assert(select_unit_by_slot(passenger));
    move_cursor(u[carrier].x, u[carrier].y);
    tap(KEY_A);
    return (u[passenger].order == ORDER_BOARD || SC64_TRANSPORT_LOADED(&u[passenger])) &&
           u[passenger].target == carrier;
}

static void issue_unload_by_menu(int carrier, int x, int y)
{
    assert(select_unit_by_slot(carrier));
    assert(race_p5_shuttle_menu_available());
    tap(KEY_B);
    assert(menu_open);
    assert(field_menu_count() == MENU_STOP + 2);
    for (int i = 0; i < MENU_STOP + 1; ++i) tap(KEY_DOWN);
    tap(KEY_A);
    assert(mode == MODE_UNLOAD && !menu_open);
    move_cursor(x, y);
    tap(KEY_A);
    assert(u[carrier].order == ORDER_UNLOAD && u[carrier].target == -1);
}

static int point_inside_location2(int *out_x, int *out_y)
{
    int left, top, right, bottom;
    if (!sc64_p5_world_location(2, &left, &top, &right, &bottom)) return 0;
    int center_x = (left + right) / 2, center_y = (top + bottom) / 2;
    for (int radius = 0; radius < right - left + bottom - top; radius += 8)
        for (int oy = -radius; oy <= radius; oy += 8)
            for (int ox = -radius; ox <= radius; ox += 8) {
                int x = center_x + ox, y = center_y + oy;
                if (x < left || y < top || x >= right || y >= bottom) continue;
            if (is_traversable(x / 16, y / 16, -1)) {
                *out_x = x; *out_y = y; return 1;
            }
            }
    return 0;
}

static int count_list_actor(int owner, int slot)
{
    int guard = 0;
    for (int i = target_unit_head[owner]; i >= 0 && guard++ < MAX_UNITS;
         i = target_unit_next[i])
        if (i == slot) return 1;
    return 0;
}

static int collision_bucket_contains(int slot)
{
    for (int cell = 0; cell < MAP_CELLS; ++cell) {
        int guard = 0;
        for (int i = sc64_unit_bucket_head[cell]; i != 0xff && guard++ < MAX_UNITS;
             i = sc64_unit_bucket_next[i])
            if (i == slot) return 1;
    }
    return 0;
}

static void check_world_exclusions(int passenger)
{
    sc64_unit_buckets_begin();
    assert(sc64_unit_bucket_active);
    assert(!collision_bucket_contains(passenger));
    sc64_update_exploration();
    assert(!sc64_unit_visibility[passenger].active);
    update_orders();
    assert(!count_list_actor(SIDE, passenger));
}

static void encode_candidate_unit(RaceSave *candidate, int slot, Unit *actor)
{
    assert(sc64_zp_unit_save_encode(&candidate->units[slot], actor,
                                    WORLD_W, WORLD_H));
}

static void assert_load_rejects_without_live_mutation(const RaceSave *good,
                                                       const RaceSave *bad)
{
    Unit units_before[MAX_UNITS];
    Building buildings_before[MAX_BUILDINGS];
    Mineral minerals_before[MAX_MINERALS];
    Sc64P5PersistentState p5_before = race_p5;
    Sc64P5WorldContext world_before = race_p5_world;
    int frame_before = frame, minerals_resource_before = minerals;
    int gas_before = gas, screen_before = screen, selected_before = selected_building;

    memcpy(units_before, u, sizeof(units_before));
    memcpy(buildings_before, b, sizeof(buildings_before));
    memcpy(minerals_before, m, sizeof(minerals_before));
    /* Write twice so this test cannot accidentally pass by falling back to a
     * previous valid SRAM slot after rejecting the malformed generation-26
     * payload. */
    save_write(bad, sizeof(*bad), SAVE_VERSION);
    save_write(bad, sizeof(*bad), SAVE_VERSION);
    assert(!race_load_game());
    assert(frame == frame_before && minerals == minerals_resource_before &&
           gas == gas_before && screen == screen_before &&
           selected_building == selected_before);
    assert(memcmp(units_before, u, sizeof(units_before)) == 0);
    assert(memcmp(buildings_before, b, sizeof(buildings_before)) == 0);
    assert(memcmp(minerals_before, m, sizeof(minerals_before)) == 0);
    assert(memcmp(&p5_before, &race_p5, sizeof(p5_before)) == 0);
    assert(memcmp(&world_before, &race_p5_world, sizeof(world_before)) == 0);
    save_write(good, sizeof(*good), SAVE_VERSION);
    save_write(good, sizeof(*good), SAVE_VERSION);
}

static void check_candidate_rejections(const RaceSave *good, int carrier,
                                      int passenger)
{
    RaceSave bad = *good;
    Unit actor;
    assert(race_p5_transport_save_candidate_valid(good, race_cols, race_rows));

    assert(race_save_decode_unit(good, (unsigned)passenger, WORLD_W, WORLD_H, &actor));
    actor.target = MAX_UNITS;
    encode_candidate_unit(&bad, passenger, &actor);
#ifdef SC64_SHARED_BUNKER
    /* Building targets are owned by the candidate-wide Bunker validator. */
    assert(!race_bunker_save_world_valid(&bad, race_cols, race_rows));
#else
    assert(!race_p5_transport_save_candidate_valid(&bad, race_cols, race_rows));
#endif

    /* P5 Count/Bring and death attribution use source provenance. Both
     * impossible sentinel/out-of-range owners must fail candidate and load
     * validation transactionally. */
    for (int raw_owner_case = 0; raw_owner_case < 2; ++raw_owner_case) {
        bad = *good;
        assert(race_save_decode_unit(good, (unsigned)passenger, WORLD_W,
                                     WORLD_H, &actor));
        actor.raw_owner = raw_owner_case ? 255 : -1;
        encode_candidate_unit(&bad, passenger, &actor);
        assert(!race_p5_transport_save_candidate_valid(&bad, race_cols, race_rows));
        assert_load_rejects_without_live_mutation(good, &bad);
    }

    bad = *good;
    assert(race_save_decode_unit(good, (unsigned)carrier, WORLD_W, WORLD_H, &actor));
    actor.raw_owner = 2;
    encode_candidate_unit(&bad, carrier, &actor);
    assert(!race_p5_transport_save_candidate_valid(&bad, race_cols, race_rows));
    assert_load_rejects_without_live_mutation(good, &bad);

    /* Boarding/unloading is a player command. A self-consistent cargo link
     * must still reject when both actors are forged into the enemy owner. */
    bad = *good;
    assert(race_save_decode_unit(good, (unsigned)carrier, WORLD_W, WORLD_H, &actor));
    actor.owner = ENEMY;
    encode_candidate_unit(&bad, carrier, &actor);
    assert(race_save_decode_unit(good, (unsigned)passenger, WORLD_W, WORLD_H, &actor));
    actor.owner = ENEMY;
    encode_candidate_unit(&bad, passenger, &actor);
    assert(!race_p5_transport_save_candidate_valid(&bad, race_cols, race_rows));
    assert_load_rejects_without_live_mutation(good, &bad);

    /* Raw 79 is the source Tassadar owned by source slot 2. Claiming that
     * actor belongs to source slot 7 must be rejected. */
    bad = *good;
    assert(race_save_decode_unit(good, (unsigned)passenger, WORLD_W, WORLD_H, &actor));
    actor.raw_type = 79;
    actor.raw_owner = 7;
    actor.type = U_TASSADAR;
    encode_candidate_unit(&bad, passenger, &actor);
    assert(!race_p5_transport_save_candidate_valid(&bad, race_cols, race_rows));
    assert_load_rejects_without_live_mutation(good, &bad);

    /* A legitimately rescued source Tassadar keeps source slot 2 provenance
     * after its live owner becomes SIDE. Use the actual authored actor row,
     * and free one Zealot's two cargo points so the eight-space capacity stays
     * valid. */
    bad = *good;
    int tassadar = -1;
    for (int i = 0; i < MAX_UNITS; ++i) {
        Unit source;
        assert(race_save_decode_unit(good, (unsigned)i, WORLD_W, WORLD_H, &source));
        if (source.alive && source.raw_type == 79 && source.raw_owner == 2 &&
            source.type == U_TASSADAR) { tassadar = i; break; }
    }
    assert(tassadar >= 0);
    assert(race_save_decode_unit(good, (unsigned)tassadar, WORLD_W, WORLD_H, &actor));
    actor.owner = SIDE; actor.order = ORDER_TRANSPORTED; actor.target = carrier;
    actor.x = u[carrier].x; actor.y = u[carrier].y; actor.selected = 0;
    actor.path_timer = actor.path_retry = 0;
    encode_candidate_unit(&bad, tassadar, &actor);
    assert(race_save_decode_unit(good, (unsigned)passenger, WORLD_W, WORLD_H, &actor));
    actor.order = ORDER_IDLE; actor.target = -1;
    actor.x = u[carrier].x + 32 < WORLD_W ? u[carrier].x + 32 : u[carrier].x - 32;
    actor.y = u[carrier].y;
    actor.path_timer = actor.path_retry = 0;
    encode_candidate_unit(&bad, passenger, &actor);
    assert(race_p5_transport_save_candidate_valid(&bad, race_cols, race_rows));

    bad = *good;
    assert(race_save_decode_unit(good, (unsigned)carrier, WORLD_W, WORLD_H, &actor));
    actor.owner = ENEMY;
    encode_candidate_unit(&bad, carrier, &actor);
    assert(!race_p5_transport_save_candidate_valid(&bad, race_cols, race_rows));

    /* A valid extra transported Zealot that raises the carrier above its
     * eight-space capacity must reject before any save commit. */
    int extra = -1;
    for (int i = 0; i < MAX_UNITS; ++i) {
        Unit spare;
        if (race_save_decode_unit(good, (unsigned)i, WORLD_W, WORLD_H, &spare) &&
            !spare.alive) { extra = i; break; }
    }
    if (extra >= 0) {
        Unit cargo = u[passenger];
        cargo.alive = 1; cargo.selected = 0; cargo.raw_type = 65;
        cargo.raw_owner = 7; cargo.type = U_ZEALOT; cargo.owner = SIDE;
        cargo.order = ORDER_TRANSPORTED; cargo.target = carrier;
        cargo.x = u[carrier].x; cargo.y = u[carrier].y;
        cargo.path_timer = cargo.path_retry = 0;
        bad = *good;
        encode_candidate_unit(&bad, extra, &cargo);
        assert(!race_p5_transport_save_candidate_valid(&bad, race_cols, race_rows));
    }
}

static void check_board_save_and_capacity(int carrier, int *passenger_out)
{
    int passengers[5];
    int fresh = spawn_unit(SIDE, U_ZEALOT, u[carrier].x, u[carrier].y);
    assert(fresh >= 0 && u[fresh].raw_type == 65 && u[fresh].raw_owner == 7);
    u[fresh].alive = 0;
    supply_used -= unit_supply(U_ZEALOT);
    for (unsigned i = 0; i < 5; ++i)
        passengers[i] = make_passenger(65, U_ZEALOT, u[carrier].x + 24,
                                       u[carrier].y + (int)i * 24);

    /* Exercise the public field interaction for boarding. */
    issue_board_by_input(passengers[0], carrier);
    assert(race_save_game()); /* Persist an in-progress board order. */
    RaceSave boarding = race_data;
    assert(race_p5_transport_save_candidate_valid(&boarding, race_cols, race_rows));
    for (int tick = 0; tick < 300 && u[passengers[0]].order == ORDER_BOARD; ++tick)
        safe_tick();
    assert(SC64_TRANSPORT_LOADED(&u[passengers[0]]));
    assert(race_transport_cargo_space(carrier) == 2);

    /* Fill the remaining six points through the same UI path. */
    for (int i = 1; i < 4; ++i) {
        issue_board_by_input(passengers[i], carrier);
        for (int tick = 0; tick < 300 && u[passengers[i]].order == ORDER_BOARD; ++tick)
            safe_tick();
        assert(SC64_TRANSPORT_LOADED(&u[passengers[i]]));
    }
    assert(race_transport_cargo_space(carrier) == 8);
    assert(!issue_board_by_input(passengers[4], carrier));
    assert(u[passengers[4]].order != ORDER_BOARD);
    assert(!SC64_TRANSPORT_LOADED(&u[passengers[4]]));

    assert(race_save_game());
    RaceSave full = race_data;
    assert(race_p5_transport_save_candidate_valid(&full, race_cols, race_rows));
    check_candidate_rejections(&full, carrier, passengers[0]);
    *passenger_out = passengers[0];
}

static void check_loaded_runtime_and_unload(int carrier, int passenger)
{
    int gx, gy;
    assert(point_inside_location2(&gx, &gy));

    /* Ordinary Move order while cargo is aboard: the post-order sync must
     * keep the passenger at the moving carrier even when the carrier is idle
     * on subsequent ticks. */
    int old_x = u[carrier].x, old_y = u[carrier].y;
    assert(issue_move(carrier, old_x + 64 < WORLD_W ? old_x + 64 : old_x - 64, old_y));
    safe_tick();
    assert(u[carrier].x != old_x || u[carrier].y != old_y);
    assert(u[passenger].x == u[carrier].x && u[passenger].y == u[carrier].y);
    u[carrier].order = ORDER_IDLE; /* A fixture landing setup, not campaign input. */

    /* Place the carrier in the source extraction region as an explicit
     * fixture setup, then verify loaded units do not satisfy source Bring. */
    u[carrier].x = gx; u[carrier].y = gy;
    input(0);
    assert(u[passenger].x == gx && u[passenger].y == gy);
    u32 count = 0;
    assert(sc64_p5_world_count(&race_p5_world, 7, 65, 2, &count) && count == 0);
    check_world_exclusions(passenger);

    /* Exercise Shuttle row 10 as a normal command and save while it travels
     * toward a different point in the island. */
    int left, top, right, bottom;
    assert(sc64_p5_world_location(2, &left, &top, &right, &bottom));
    int dx = (left + right) / 2, dy = (top + bottom) / 2;
    if (!is_traversable(dx / 16, dy / 16, -1)) dx = gx;
    issue_unload_by_menu(carrier, dx, dy);
    assert(race_save_game());
    RaceSave unloading = race_data;
    assert(race_p5_transport_save_candidate_valid(&unloading, race_cols, race_rows));
    safe_tick();
    assert(race_load_game());
    assert(u[carrier].order == ORDER_UNLOAD);
    assert(race_p5_transport_save_candidate_valid(&race_data, race_cols, race_rows));

    for (int tick = 0; tick < 500 && race_transport_cargo_space(carrier) > 0; ++tick)
        safe_tick();
    assert(race_transport_cargo_space(carrier) == 0);
    assert(u[passenger].alive && !SC64_TRANSPORT_LOADED(&u[passenger]));
    if (!sc64_p5_world_count(&race_p5_world, 7, 65, 2, &count) || !count)
        fprintf(stderr,"landed count missing loc=2 carrier=%d,%d passenger=%d,%d target=%d,%d order=%d rawowner=%d cargo=%d\n",
                u[carrier].x,u[carrier].y,u[passenger].x,u[passenger].y,
                u[carrier].tx,u[carrier].ty,u[passenger].order,u[passenger].raw_owner,
                race_transport_cargo_space(carrier));
    assert(sc64_p5_world_count(&race_p5_world, 7, 65, 2, &count) && count > 0);
}

static void check_destroyed_carrier_counts_cargo_death(int carrier)
{
    int passenger = make_passenger(65, U_ZEALOT, u[carrier].x, u[carrier].y);
    assert(issue_board_by_input(passenger, carrier));
    for (int tick = 0; tick < 300 && u[passenger].order == ORDER_BOARD; ++tick)
        safe_tick();
    assert(SC64_TRANSPORT_LOADED(&u[passenger]));
    u[carrier].hp = 1;
    u[carrier].shield = 0;
    invincible_units[carrier] = 0;
    u32 before = race_p5.deaths_by_source[7];
    hit_target(carrier, 10000, U_ZEALOT, ENEMY);
    if (u[carrier].alive || u[passenger].alive)
        fprintf(stderr,"carrier death result carrier=%u hp=%d order=%d passenger=%u hp=%d order=%d target=%d deaths=%u/%u\n",
                u[carrier].alive,u[carrier].hp,u[carrier].order,
                u[passenger].alive,u[passenger].hp,u[passenger].order,
                u[passenger].target,before,race_p5.deaths_by_source[7]);
    assert(!u[carrier].alive && !u[passenger].alive);
    assert(race_p5.deaths_by_source[7] >= before + 2);
}

static void test_old_generation_is_rejected_transactionally(void)
{
    RaceSave good;
    Unit units_before[MAX_UNITS];
    Sc64P5PersistentState state_before = race_p5;
    int frame_before = frame;
    assert(race_save_game());
    good = race_data;
    memcpy(units_before, u, sizeof(units_before));
    save_write(&good, sizeof(good), SAVE_VERSION - 2u); /* P5 gen24 -> Shuttle gen26 */
    save_write(&good, sizeof(good), SAVE_VERSION - 2u);
    assert(!race_load_game());
    assert(frame == frame_before && memcmp(units_before, u, sizeof(units_before)) == 0);
    assert(memcmp(&race_p5, &state_before, sizeof(state_before)) == 0);
    save_write(&good, sizeof(good), SAVE_VERSION);
}

int main(void)
{
    gba_init();
    assert(sc64_p5_source_model_valid());
    game_init(); prior_keys = 0;
    select_protoss_mission(SC64_P5_GLOBAL_ID);
    assert(race_p5_active() && race_p5_runtime_state_valid());
    check_source_census();

    int carrier = shuttle_slot();
    assert(carrier >= 0 && race_transport_carrier_type(&u[carrier]));
    int passenger = -1;
    check_board_save_and_capacity(carrier, &passenger);
    check_loaded_runtime_and_unload(carrier, passenger);
    test_old_generation_is_rejected_transactionally();
    check_destroyed_carrier_counts_cargo_death(carrier);
    puts("P5 Shuttle UI, cargo, source Bring, collision/vision, and save checks passed");
    return 0;
}
