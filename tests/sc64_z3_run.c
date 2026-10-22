/* Controller-driven Zerg 3 integration test plus explicitly labeled adapter
 * fixtures. Built with the opt-in unified Zerg campaign profile. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

static u16 previous_keys;
static Unit saved_units[MAX_UNITS];
static Building saved_buildings[MAX_BUILDINGS];

static void controller_frame(u16 held)
{
    game_update(held, held & (u16)~previous_keys);
    previous_keys = held;
}

static void controller_press(u16 key)
{
    controller_frame(key);
    controller_frame(0);
}

static int owner_units(int owner)
{
    int i, n = 0;
    for (i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == owner) ++n;
    return n;
}

static int owner_buildings(int owner)
{
    int i, n = 0;
    for (i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == owner) ++n;
    return n;
}

static int live_units(void)
{
    int i, n = 0;
    for (i = 0; i < MAX_UNITS; ++i) n += u[i].alive != 0;
    return n;
}

static int live_buildings(void)
{
    int i, n = 0;
    for (i = 0; i < MAX_BUILDINGS; ++i) n += b[i].alive != 0;
    return n;
}

static int live_minerals(void)
{
    int i, n = 0;
    for (i = 0; i < MAX_MINERALS; ++i) n += m[i].amount > 0;
    return n;
}

static void launch_z3_through_campaign_menu(void)
{
    game_init();
    previous_keys = 0;
    controller_press(KEY_A); /* HOME -> the profile's Zerg mission list. */
    assert(race_front.page == SC_FRONT_MISSION);
    assert(race_front.faction == SC_FRONT_ZERG);
    controller_press(KEY_RIGHT); /* Z3 is ordinal 2, first row of page 2. */
    assert(race_front.mission_page == 1);
    controller_press(KEY_A);
    assert(race_front.page == SC_FRONT_BRIEFING && race_front.mission == 14);
    controller_frame(KEY_A); /* Deploy; do not release into a live simulation tick. */
    assert(screen == SCREEN_PLAY && !race_mode);
    assert(race_selected_mission == 14 && race_campaign.mission == 14);
}

static void test_controller_opening_and_pause_save_resume(void)
{
    Sc64Z3State before_z3;
    u16 before_action_ticks;
    int before_frame, before_elapsed, before_ore, before_gas;
    int before_enemy_ore, before_enemy_gas;

    launch_z3_through_campaign_menu();
    assert(mission_unsupported_count == 0);
    assert(owner_units(SIDE) == 13 && owner_units(ENEMY) == 45);
    assert(live_units() == 58);
    assert(owner_buildings(SIDE) == 7 && owner_buildings(ENEMY) == 26);
    assert(live_buildings() == 33);
    assert(live_minerals() == 28 && geyser_count == 3);
    assert(minerals == SC64_Z3_MISSION_ORE && gas == SC64_Z3_MISSION_GAS);
    assert(sc64_z3_state_valid(&race_z3));
    assert(race_z3.fired & SC64_Z3_F_MISSION_START);
    assert(!(race_z3.fired & SC64_Z3_F_FORCE2_START));

    /* Controller-neutral opening: source Force 2 startup grants 8000 ore and
     * 8000 gas once, while the authored two-minute production hasn't fired. */
    controller_frame(0);
    assert(race_minerals[ENEMY] == SC64_Z3_FORCE2_RESOURCES);
    assert(race_gas[ENEMY] == SC64_Z3_FORCE2_RESOURCES);
    assert(race_z3.fired & SC64_Z3_F_FORCE2_START);
    for (int i = 1; i < 1000; ++i) controller_frame(0);
    assert(race_campaign.elapsed_frames == 1000);
    assert(race_z3.outcome == SC64_Z3_RUNNING);
    assert(live_units() == 58 && live_buildings() == 33);
    /* The trigger is one-shot. Runtime training/income can alter the stock
     * after the initial 8000/8000 grant. */
    assert(race_z3.fired & SC64_Z3_F_FORCE2_START);

    controller_press(KEY_START);
    assert(screen == SCREEN_PAUSE);
    before_z3 = race_z3;
    before_action_ticks = race_z3_action_ticks;
    before_frame = frame;
    before_elapsed = race_campaign.elapsed_frames;
    before_ore = minerals; before_gas = gas;
    before_enemy_ore = race_minerals[ENEMY];
    before_enemy_gas = race_gas[ENEMY];
    memcpy(saved_units, u, sizeof(saved_units));
    memcpy(saved_buildings, b, sizeof(saved_buildings));
    for (int i = 0; i < 120; ++i) controller_frame(0);
    assert(frame == before_frame && race_campaign.elapsed_frames == before_elapsed);
    assert(memcmp(&race_z3, &before_z3, sizeof(before_z3)) == 0);
    assert(race_z3_action_ticks == before_action_ticks);
    assert(minerals == before_ore && gas == before_gas);
    assert(race_minerals[ENEMY] == before_enemy_ore &&
           race_gas[ENEMY] == before_enemy_gas);
    assert(memcmp(saved_units, u, sizeof(saved_units)) == 0);
    assert(memcmp(saved_buildings, b, sizeof(saved_buildings)) == 0);

    controller_press(KEY_B); /* Pause B writes SRAM without advancing time. */
    assert(screen == SCREEN_PAUSE);
    game_init();
    previous_keys = 0;
    controller_frame(KEY_B); /* Load on the title input edge, before another live tick. */
    assert(screen == SCREEN_PLAY && race_selected_mission == 14);
    assert(race_campaign.mission == 14);
    assert(frame == before_frame && race_campaign.elapsed_frames == before_elapsed);
    assert(memcmp(&race_z3, &before_z3, sizeof(before_z3)) == 0);
    assert(race_z3_action_ticks == before_action_ticks);
    assert(minerals == before_ore && gas == before_gas);
    assert(race_minerals[ENEMY] == before_enemy_ore &&
           race_gas[ENEMY] == before_enemy_gas);
    assert(memcmp(saved_units, u, sizeof(saved_units)) == 0);
    assert(memcmp(saved_buildings, b, sizeof(saved_buildings)) == 0);
    controller_frame(0);
    assert(frame == before_frame + 1 && race_campaign.elapsed_frames == before_elapsed + 1);
    assert(sc64_z3_state_valid(&race_z3));
}

/* The remaining cases deliberately mutate live slots and call the adapter
 * directly. These are rule/adapter fixtures, not controller-only gameplay. */
static void reset_adapter_fixture(void)
{
    race_z3_init();
    race_z3_action_ticks = 0;
    race_campaign.mission = SC64_Z3_GLOBAL_MISSION_ID;
    race_campaign.elapsed_frames = 0;
    race_campaign.objective_stage = 0;
    race_campaign.objective_flags = 0;
    race_campaign.phase = SC64_RACE_PHASE_ACTIVE;
    race_completed &= ~(1 << SC64_Z3_GLOBAL_MISSION_ID);
    won = lost = 0;
}

static int enemy_unit_slots(int out[MAX_UNITS])
{
    int i, n = 0;
    for (i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == ENEMY) out[n++] = i;
    return n;
}

static int enemy_building_slots(int out[MAX_BUILDINGS])
{
    int i, n = 0;
    for (i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == ENEMY) out[n++] = i;
    return n;
}

static void drive_action(u16 ticks, u8 expected_sequence)
{
    for (u16 i = 0; i < ticks; ++i) race_z3_campaign_tick();
    assert(race_z3.sequence == expected_sequence);
    assert(sc64_z3_state_valid(&race_z3));
}

static void test_both_distinct_source_victory_predicates_and_sequence(void)
{
    int units[MAX_UNITS], buildings[MAX_BUILDINGS];
    int nu = enemy_unit_slots(units), nb = enemy_building_slots(buildings);
    assert(nu == 45 && nb == 26);

    /* Fixture A: exactly five enemy units plus one building satisfies Any Unit
     * <= 6 without satisfying Buildings <= 0. */
    reset_adapter_fixture();
    for (int i = 5; i < nu; ++i) u[units[i]].alive = 0;
    for (int i = 1; i < nb; ++i) b[buildings[i]].alive = 0;
    race_z3_campaign_tick();
    assert(race_z3.switch7 && race_z3.sequence == SC64_Z3_SEQ_WIN_WAIT_1000);
    assert(race_campaign.objective_stage == 1);
    assert(race_z3_action_ticks == race_z3_action_duration(&race_z3));
    drive_action(30, SC64_Z3_SEQ_WIN_TX_1);
    drive_action(SC64_Z3_TX_HOLD_TICKS, SC64_Z3_SEQ_WIN_TX_2);
    drive_action(SC64_Z3_TX_HOLD_TICKS, SC64_Z3_SEQ_WIN_TX_3);
    drive_action(SC64_Z3_TX_HOLD_TICKS, SC64_Z3_SEQ_WIN_WAIT_500);
    drive_action(15, SC64_Z3_SEQ_DONE_WIN);
    assert(race_z3.outcome == SC64_Z3_WON && won && !lost);
    assert(race_campaign.phase == SC64_RACE_PHASE_COMPLETE);
    assert(race_campaign.objective_flags & SC64_RACE_OBJECTIVE_PRIMARY);
    assert(race_completed & (1 << 14));

    /* Fixture B: seven enemy units and no enemy buildings fails the Any Unit
     * threshold but satisfies the independent Buildings <= 0 trigger. */
    launch_z3_through_campaign_menu();
    reset_adapter_fixture();
    nu = enemy_unit_slots(units); nb = enemy_building_slots(buildings);
    for (int i = 7; i < nu; ++i) u[units[i]].alive = 0;
    for (int i = 0; i < nb; ++i) b[buildings[i]].alive = 0;
    race_z3_campaign_tick();
    assert(race_z3.switch7 && race_z3.sequence == SC64_Z3_SEQ_WIN_WAIT_1000);
    assert(owner_units(ENEMY) == 7 && owner_buildings(ENEMY) == 0);
}

static void test_only_raw150_chrysalis_starts_ordered_loss_sequence(void)
{
    int chrysalis[MAX_BUILDINGS], n = 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == SIDE && b[i].raw_type == 150) {
            chrysalis[n++] = i;
        }
    assert(n > 0 && b[chrysalis[0]].type == B_MATURE_CHRYSALIS);
    assert(b[chrysalis[0]].hp == 250 && b[chrysalis[0]].maxhp == 250);
    assert(sc64_race_building_armor(b[chrysalis[0]].type) == 1);
    assert(sc64_race_building_rules[b[chrysalis[0]].type].flags & SC_RACE_F_BUILDING);
    reset_adapter_fixture();

    /* Source Z3 has no raw-216 placement. Add this explicitly labeled fixture
     * to prove the Young Chrysalis powerup is not the raw-150 objective. */
    int young_powerup = spawn_unit(SIDE, U_CHRYSALIS, 760, 1320);
    assert(young_powerup >= 0);
    u[young_powerup].raw_type = 216;
    race_z3_campaign_tick();
    assert(race_z3.sequence == SC64_Z3_SEQ_NONE && race_z3.outcome == SC64_Z3_RUNNING);
    for (int i = 0; i < n; ++i)
        b[chrysalis[i]].raw_type = 0;
    race_z3_campaign_tick();
    /* Keeping the raw-216 Young Chrysalis powerup alive does not satisfy the
     * raw-150 Mature Chrysalis trigger once its building identity is gone. */
    assert(race_z3.sequence == SC64_Z3_SEQ_LOSS_WAIT_1000 && !lost);
    drive_action(30, SC64_Z3_SEQ_LOSS_WAV);
    drive_action(13, SC64_Z3_SEQ_LOSS_MESSAGE);
    drive_action(SC64_Z3_LOSS_TEXT_HOLD_TICKS, SC64_Z3_SEQ_LOSS_WAIT_500);
    drive_action(15, SC64_Z3_SEQ_DONE_LOSS);
    assert(race_z3.outcome == SC64_Z3_LOST && lost && !won);
    assert(race_campaign.phase == SC64_RACE_PHASE_FAILED);
    assert(!(race_campaign.objective_flags & SC64_RACE_OBJECTIVE_PRIMARY));
}

static void test_paid_enemy_reinforcement_respects_player_reserve(void)
{
    int starting = live_units(), queued = 0;
    assert(starting == 58);
    race_minerals[ENEMY] = 8000;
    race_gas[ENEMY] = 8000;
    race_z3_paid_reinforcement();
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == ENEMY) queued += b[i].queue_count;
    assert(queued > 0 && race_minerals[ENEMY] < 8000);
    int target_live = MAX_UNITS - SC64_Z3_PLAYER_UNIT_RESERVE - queued;
    for (int i = starting; i < target_live; ++i) {
        int x = 16 + (i % 10) * 8;
        int y = 16 + ((i - starting) / 10) * 8;
        assert(spawn_unit(SIDE, U_MARINE, x, y) >= 0);
    }
    assert(live_units() + queued == MAX_UNITS - SC64_Z3_PLAYER_UNIT_RESERVE);
    int queued_before = 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == ENEMY) queued_before += b[i].queue_count;
    race_minerals[ENEMY] = 8000;
    race_gas[ENEMY] = 8000;
    race_z3_paid_reinforcement(); /* direct cap fixture, no gameplay tick */
    int queued_after = 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == ENEMY) queued_after += b[i].queue_count;
    assert(queued_after == queued_before);
    assert(race_z3_total_units_and_queued() <= MAX_UNITS - SC64_Z3_PLAYER_UNIT_RESERVE);
}

int main(void)
{
    gba_init();
    test_controller_opening_and_pause_save_resume();
    launch_z3_through_campaign_menu();
    test_both_distinct_source_victory_predicates_and_sequence();
    launch_z3_through_campaign_menu();
    test_only_raw150_chrysalis_starts_ordered_loss_sequence();
    launch_z3_through_campaign_menu();
    test_paid_enemy_reinforcement_respects_player_reserve();
    puts("Z3 controller opening, raw trigger gates, sequenced results, and reserve cap: ok");
    return 0;
}
