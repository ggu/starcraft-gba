/* Real-engine Z5 Consume UI/order fixture. Positions are adjusted only to
 * isolate the order and its eligibility/range boundaries; this is not a
 * controller trace or a native timing claim. */
#define main sc64_z5_base_runtime_checks
#include "sc64_z5_runtime.c"
#undef main

#ifndef SC64_Z5_CONSUME_ENABLED
#error "Compile with the opt-in Z5 Consume profile flags"
#endif

static void tap(u16 key) { input(key); input(0); }

static int hero_slot(void)
{
    int hero = race_z5_cloak_hero_slot();
    assert(hero >= 0 && u[hero].raw_type == 51 && u[hero].raw_owner == 3);
    return hero;
}

static int sourced_zergling(int hero)
{
    for (int i = 0; i < MAX_UNITS; ++i)
        if (i != hero && u[i].alive && u[i].owner == SIDE &&
            u[i].raw_type == 37 && u[i].raw_owner == 3 &&
            u[i].type == U_ZERGLING) return i;
    return -1;
}

static void select_slot(int slot)
{
    clear_selection();
    u[slot].selected = 1;
}

static void open_consume(void)
{
    tap(KEY_B);
    assert(menu_open && menu_cursor == 0);
    for (int i = 0; i < MENU_STOP + 2; ++i) tap(KEY_DOWN);
    assert(menu_cursor == MENU_STOP + 2);
    tap(KEY_A);
    assert(!menu_open && mode == MODE_Z5_CONSUME);
}

static void issue_consume_at(int x, int y)
{
    cx = x;
    cy = y;
    /* Isolate target-boundary checks without advancing simulation. A separate
     * case below exercises the full A-input and same-frame order resolution. */
    assert(mode == MODE_Z5_CONSUME);
    interact();
    assert(mode == MODE_NORMAL);
}

/* Preserve a valid inert source-program snapshot for a one-frame real-input
 * fixture. All nonterminal one-shots are already finished; the victory gate
 * remains unset and the defeat predicate remains false while Kerrigan lives. */
static void freeze_script_for_input_fixture(void)
{
    Sc64Z5RuntimeState *runtime = &race_z5.runtime;
    sc64_z5_runtime_init(runtime);
    for (unsigned i = 0; i < SC64_Z5_TRIGGER_COUNT; ++i) {
        if (i == SC64_Z5_TRIGGER_VICTORY || i == SC64_Z5_TRIGGER_DEFEAT ||
            i == 37 || i == 39) continue;
        sc64_z5_mask_set(&runtime->model.started_lo,
                         &runtime->model.started_hi, (u8)i);
    }
    runtime->source_paused = 1;
    race_z5.pause_acknowledged = 1;
    assert(sc64_z5_runtime_state_valid(runtime));
    assert(sc64_z5_persistent_state_valid(&race_z5));
}

static void prepare(int *hero_out, int *food_out)
{
    launch_z5_from_normal_menu();
    int hero = hero_slot();
    int food = sourced_zergling(hero);
    assert(food >= 0);
    clear_selection();
    u[hero].carry = 0;
    /* Exercise the normal menu while isolated in the map's quiet corner, so
     * the authored script's combat elsewhere cannot preempt the click. */
    u[hero].x = 1850; u[hero].y = 1850;
    u[food].x = 1859; u[food].y = 1850;
    u[food].order = ORDER_IDLE; u[food].target = -1;
    u[food].path_timer = u[food].path_retry = 0;
    u[food].selected = 0;
    *hero_out = hero; *food_out = food;
}

static void reject_consume_save_without_world_changes(const RaceSave *good,
                                                       RaceSave bad)
{
    Unit units_before[MAX_UNITS];
    Building buildings_before[MAX_BUILDINGS];
    Sc64Z5PersistentState state_before = race_z5;
    memcpy(units_before, u, sizeof(u));
    memcpy(buildings_before, b, sizeof(b));
    save_write(&bad, sizeof(bad), SAVE_VERSION);
    assert(!race_load_game());
    assert(!memcmp(units_before, u, sizeof(u)));
    assert(!memcmp(buildings_before, b, sizeof(b)));
    assert(!memcmp(&state_before, &race_z5, sizeof(race_z5)));
    save_write(good, sizeof(*good), SAVE_VERSION);
    assert(race_load_game());
}

static void zero_cost_gain_cap_and_stop(void)
{
    int hero, food;
    prepare(&hero, &food);
    select_slot(hero);
    assert(race_z5_cloak_menu_available());
    freeze_script_for_input_fixture();
    open_consume();
    u[hero].carry = 0;
    u[food].x = u[hero].x + 16; /* Native range boundary after sprite boxes. */
    cx = u[food].x; cy = u[food].y;
    assert(u[hero].alive && u[hero].hp > 0 && u[hero].raw_type == 51 &&
           u[hero].raw_owner == 3 && u[hero].owner == SIDE);
    assert(u[food].alive && u[food].hp > 0 && u[food].raw_type == 37 &&
           u[food].raw_owner == 3 && u[food].type == U_ZERGLING &&
           u[food].owner == u[hero].owner && !invincible_units[food]);
    unsigned regen_before = race_z5.cloak.regen_remainder;
    input(KEY_A); /* Actual game_update -> interact -> update_orders path. */
    prior_keys = KEY_A;
    assert(mode == MODE_NORMAL);
    assert(!u[food].alive);
    assert(u[hero].carry == 50 * 256 + (int)((regen_before + 32u) / 5u));
    assert(u[hero].order == ORDER_IDLE && u[hero].target == -1);

    /* A second sourced target proves the energy cap and no cast cost. */
    int food2 = sourced_zergling(hero);
    assert(food2 >= 0);
    u[food2].x = u[hero].x + 17; u[food2].y = u[hero].y;
    select_slot(hero);
    open_consume(); issue_consume_at(u[food2].x, u[food2].y);
    u[hero].carry = 240 * 256;
    assert(u[hero].order == ORDER_Z5_CONSUME);
    assert(race_z5_consume_tick_unit(hero));
    assert(u[food2].alive && u[hero].order == ORDER_Z5_CONSUME);
    u[food2].x = u[hero].x + 16;
    assert(race_z5_consume_tick_unit(hero));
    assert(!u[food2].alive && u[hero].carry == 250 * 256);

    /* STOP cancels a pending Consume without spending or changing energy. */
    int food3 = sourced_zergling(hero);
    assert(food3 >= 0);
    u[food3].x = u[hero].x + 100; u[food3].y = u[hero].y;
    select_slot(hero); open_consume(); issue_consume_at(u[food3].x, u[food3].y);
    assert(u[hero].order == ORDER_Z5_CONSUME);
    assert(race_save_game() && SC64_RACE_SAVE_GENERATION == 27u);
    RaceSave active = race_data;
    assert(race_z5_consume_save_candidate_valid(&active, WORLD_W / 16,
                                                WORLD_H / 16));
    assert(race_load_game() && u[hero].order == ORDER_Z5_CONSUME &&
           u[hero].target == food3);
    /* Reject a malformed live pending order before replacing good SRAM. */
    u[food3].raw_owner = 4;
    assert(!race_save_game());
    RaceSave preserved;
    assert(save_read(&preserved, sizeof(preserved), SAVE_VERSION));
    assert(!memcmp(&preserved, &active, sizeof(active)));
    u[food3].raw_owner = 3;
    assert(race_load_game());
    RaceSave corrupt = active;
    Unit decoded;
    assert(race_save_decode_unit(&corrupt, (unsigned)food3, WORLD_W,
                                 WORLD_H, &decoded));
    decoded.raw_owner = 4;
    assert(sc64_zp_unit_save_encode(&corrupt.units[food3], &decoded,
                                   WORLD_W, WORLD_H));
    reject_consume_save_without_world_changes(&active, corrupt);
    corrupt = active;
    unsigned target_bit = (unsigned)food3;
    corrupt.invincible_bits[target_bit >> 3] |= (u8)(1u << (target_bit & 7u));
    reject_consume_save_without_world_changes(&active, corrupt);
    corrupt = active;
    assert(race_save_decode_unit(&corrupt, (unsigned)hero, WORLD_W,
                                 WORLD_H, &decoded));
    decoded.target = MAX_UNITS;
    assert(sc64_zp_unit_save_encode(&corrupt.units[hero], &decoded,
                                   WORLD_W, WORLD_H));
    reject_consume_save_without_world_changes(&active, corrupt);
    corrupt = active;
    assert(race_save_decode_unit(&corrupt, (unsigned)hero, WORLD_W,
                                 WORLD_H, &decoded));
    decoded.raw_type = 52;
    assert(sc64_zp_unit_save_encode(&corrupt.units[hero], &decoded,
                                   WORLD_W, WORLD_H));
    reject_consume_save_without_world_changes(&active, corrupt);
    corrupt = active;
    assert(race_save_decode_unit(&corrupt, (unsigned)hero, WORLD_W,
                                 WORLD_H, &decoded));
    decoded.raw_owner = 4;
    assert(sc64_zp_unit_save_encode(&corrupt.units[hero], &decoded,
                                   WORLD_W, WORLD_H));
    reject_consume_save_without_world_changes(&active, corrupt);
    corrupt = active;
    corrupt.version = 0x53435a30u + 25u;
    reject_consume_save_without_world_changes(&active, corrupt);
    tap(KEY_B); assert(menu_open);
    for (int i = 0; i < MENU_STOP; ++i) tap(KEY_DOWN);
    tap(KEY_A);
    assert(u[hero].order == ORDER_IDLE && u[hero].carry == 250 * 256);
}

static void approach_and_target_cancellation(void)
{
    int hero, food;
    prepare(&hero, &food);
    u[food].x = u[hero].x + 100;
    select_slot(hero); open_consume(); issue_consume_at(u[food].x, u[food].y);
    u[hero].carry = 0;
    assert(u[hero].order == ORDER_Z5_CONSUME);
    assert(race_z5_consume_tick_unit(hero));
    assert(u[food].alive && u[hero].order == ORDER_Z5_CONSUME);
    /* Once the synthetic target is placed at the source range boundary, the
     * ordinary actor update resolves the same outstanding player order. */
    u[hero].x = 1850; u[hero].y = 1850;
    u[food].x = 1859; u[food].y = 1850;
    assert(race_z5_consume_tick_unit(hero));

    int food2 = sourced_zergling(hero);
    assert(food2 >= 0);
    u[food2].x = u[hero].x + 9;
    select_slot(hero); open_consume(); issue_consume_at(u[food2].x, u[food2].y);
    int energy_before_cancel = u[hero].carry;
    u[food2].alive = 0; u[food2].hp = 0;
    assert(race_z5_consume_tick_unit(hero));
    assert(u[hero].order == ORDER_IDLE && u[hero].target == -1);
    assert(u[hero].carry == energy_before_cancel);
}

static void target_eligibility_and_pause(void)
{
    int hero, food;
    prepare(&hero, &food);
    u[food].x = u[hero].x + 9;
    select_slot(hero);
    /* The UI scan skips foreign-owner, wrong-source, invincible and dead
     * candidates, and does not arm an order when no valid target is tapped. */
    int raw_owner = u[food].raw_owner, owner = u[food].owner;
    u[food].raw_owner = 4;
    open_consume(); issue_consume_at(u[food].x, u[food].y);
    assert(u[hero].order != ORDER_Z5_CONSUME);
    u[food].raw_owner = raw_owner; u[food].owner = (owner + 1) % 5;
    select_slot(hero); open_consume(); issue_consume_at(u[food].x, u[food].y);
    assert(u[hero].order != ORDER_Z5_CONSUME);
    u[food].owner = owner; invincible_units[food] = 1;
    select_slot(hero); open_consume(); issue_consume_at(u[food].x, u[food].y);
    assert(u[hero].order != ORDER_Z5_CONSUME);
    invincible_units[food] = 0;

    select_slot(hero); open_consume(); issue_consume_at(u[food].x, u[food].y);
    assert(u[hero].order == ORDER_Z5_CONSUME);
    u[hero].carry = 100 * 256;
    unsigned energy = (unsigned)u[hero].carry;
    tap(KEY_START); assert(screen == SCREEN_PAUSE);
    for (int i = 0; i < 20; ++i) input(0);
    assert(u[hero].order == ORDER_Z5_CONSUME &&
           (unsigned)u[hero].carry == energy && u[food].alive);
    /* Restore the screen for this isolated adapter case; the separate real
     * A-input case above covers normal same-frame order execution. */
    screen = SCREEN_PLAY;
    assert(race_z5_consume_tick_unit(hero));
    assert(!u[food].alive && u[hero].carry == (int)energy + 50 * 256);
}

static void hallucinated_food_and_caster_denial(void)
{
    int hero, food;
    prepare(&hero, &food);
    u[hero].carry = 100 * 256;
    u[food].flags |= 8u; /* Native hallucination marker. */
    select_slot(hero); open_consume(); issue_consume_at(u[food].x, u[food].y);
    assert(u[hero].order == ORDER_Z5_CONSUME);
    int energy = u[hero].carry;
    assert(race_z5_consume_tick_unit(hero));
    assert(!u[food].alive && u[hero].carry == energy);

    int hero2, food2;
    prepare(&hero2, &food2);
    u[hero2].flags |= 8u;
    Sc64Z5ConsumeActor source = {51, u[hero2].owner, u[hero2].hp,
        u[hero2].carry, 1,1,1,0,0,0,0,0,0};
    Sc64Z5ConsumeActor target = {37, u[food2].owner, u[food2].hp,
        0,1,1,0,0,0,0,0,0,0};
    assert(sc64_z5_consume_cast(&source, &target, 1, 0, 0) ==
           SC64_Z5_CONSUME_DENIED);
    assert(target.alive && source.energy_q8 == u[hero2].carry);
    assert(!race_z5_consume_target_valid(&u[hero2], &u[food2]));
}

int main(void)
{
    assert(sc64_z5_base_runtime_checks() == 0);
    zero_cost_gain_cap_and_stop();
    approach_and_target_cancellation();
    target_eligibility_and_pause();
    hallucinated_food_and_caster_denial();
    puts("Z5 Consume UI/order, eligibility, energy and pause checks passed");
    return 0;
}
