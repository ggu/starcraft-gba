/* Real-engine Z4 integration checks. The full countdown and source trigger
 * sequence run through game_update, but the protected-world fixture and the
 * forced actor losses below are synthetic checks, not controller-win proof. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

#ifndef SC64_Z4_RUNTIME
#error "Compile with the canonical Z4 runtime flags"
#endif

static u16 prior_keys;
static Unit saved_units[MAX_UNITS];
static Building saved_buildings[MAX_BUILDINGS];
static Mineral saved_minerals[MAX_MINERALS];

static void input(u16 held)
{
    game_update(held, held & (u16)~prior_keys);
    prior_keys = held;
}

static void press(u16 key)
{
    input(key);
    input(0);
}

static void protect_live_world(void)
{
    /* Synthetic protection keeps unrelated battles from changing authored
     * trigger predicates during the long timer test. Source create/kill
     * actions still execute normally through the campaign adapter. */
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive) invincible_units[i] = 1;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive) invincible_buildings[i] = 1;
}

static void safe_tick(void)
{
    protect_live_world();
    input(0);
    protect_live_world();
}

static int source_unit(int raw, int owner)
{
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].raw_type == raw &&
            (owner < 0 || u[i].raw_owner == owner)) return i;
    return -1;
}

static int source_building(int raw, int owner)
{
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].raw_type == raw &&
            (owner < 0 || b[i].raw_owner == owner)) return i;
    return -1;
}

static void launch_from_normal_menu(void)
{
    assert(sc64_catalog_playable_count(SC64_FACTION_ZERG) == 4);
    assert(sc64_catalog_playable_ordinal(SC64_FACTION_ZERG, 15) == 3);
    game_init();
    prior_keys = 0;
    assert(race_front.page == SC_FRONT_HOME);
    press(KEY_A);
    assert(race_front.page == SC_FRONT_MISSION);
    assert(race_front.faction == SC_FRONT_ZERG);
    press(KEY_RIGHT); /* Fourth Zerg entry, mission id 15. */
    press(KEY_DOWN);
    assert(race_front.mission_page == 1);
    assert(race_front.row == 1);
    assert(sc64_catalog_playable_at(SC64_FACTION_ZERG, 3) == 15);
    press(KEY_A);
    assert(race_front.page == SC_FRONT_BRIEFING && race_front.mission == 15);
    input(KEY_A);
    assert(screen == SCREEN_PLAY && !race_mode && !won && !lost);
    assert(race_selected_mission == 15 && race_campaign.mission == 15);
    assert(race_human == R_ZERG && race_z4_active());
}

static void check_source_world(void)
{
    int units = 0, buildings = 0, minerals_count = 0;
    int raw_owners[8] = {0};
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive) {
        ++units;
        if (u[i].raw_owner >= 0 && u[i].raw_owner < 8)
            raw_owners[u[i].raw_owner]++;
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive) {
        ++buildings;
        if (b[i].raw_owner >= 0 && b[i].raw_owner < 8)
            raw_owners[b[i].raw_owner]++;
    }
    for (int i = 0; i < MAX_MINERALS; ++i)
        if (m[i].amount > 0) ++minerals_count;
    assert(units == 73 && buildings == 67);
    assert(minerals_count == 39 && geyser_count == 2);
    assert(raw_owners[1] && raw_owners[2] && raw_owners[3]);
    assert(source_unit(51, 3) < 0); /* Kerrigan is created by authored trigger. */
    int zergling = source_unit(37, 3);
    assert(zergling >= 0 && u[zergling].owner == SIDE);
    assert(source_unit(150, 3) >= 0 || source_building(150, 3) >= 0);
    assert(source_building(106, 2) >= 0); /* Force 3 Command Center. */
}

static void assert_runtime_live_roundtrip(void)
{
    assert(race_save_game());
    RaceSave expected = race_data;
    memcpy(saved_units, u, sizeof(u));
    memcpy(saved_buildings, b, sizeof(b));
    memcpy(saved_minerals, m, sizeof(m));
    Sc64Z4RuntimeState expected_z4 = race_z4;
    Sc64RaceCampaignState expected_campaign = race_campaign;
    int expected_frame = frame;
    assert(race_load_game());
    assert(!memcmp(saved_units, u, sizeof(u)));
    assert(!memcmp(saved_buildings, b, sizeof(b)));
    assert(!memcmp(saved_minerals, m, sizeof(m)));
    assert(!memcmp(&expected_z4, &race_z4, sizeof(race_z4)));
    assert(!memcmp(&expected_campaign, &race_campaign, sizeof(race_campaign)));
    assert(frame == expected_frame && screen == SCREEN_PLAY);
    assert(race_save_game());
    assert(!memcmp(&expected, &race_data, sizeof(expected)));
}

static void assert_corrupt_save_preserves_live_world(void)
{
    assert(race_save_game());
    RaceSave corrupt = race_data;
    corrupt.z4.countdown_ticks = 0xffffu;
    save_write(&corrupt, sizeof(corrupt), SAVE_VERSION);
    memcpy(saved_units, u, sizeof(u));
    memcpy(saved_buildings, b, sizeof(b));
    memcpy(saved_minerals, m, sizeof(m));
    Sc64Z4RuntimeState expected_z4 = race_z4;
    Sc64RaceCampaignState expected_campaign = race_campaign;
    int expected_frame = frame, expected_screen = screen;
    assert(!race_load_game());
    assert(!memcmp(saved_units, u, sizeof(u)));
    assert(!memcmp(saved_buildings, b, sizeof(b)));
    assert(!memcmp(saved_minerals, m, sizeof(m)));
    assert(!memcmp(&expected_z4, &race_z4, sizeof(race_z4)));
    assert(!memcmp(&expected_campaign, &race_campaign, sizeof(race_campaign)));
    assert(frame == expected_frame && screen == expected_screen);
}

static void full_countdown_and_hatch(void)
{
    launch_from_normal_menu();
    check_source_world();
    /* Let startup, force initialization, waits, and the countdown activation
     * progress through ordinary simulation frames. */
    unsigned startup_ticks = 0;
    while (!race_z4.countdown_active && startup_ticks < 5000) {
        safe_tick();
        ++startup_ticks;
    }
    assert(race_z4.countdown_active && race_z4.countdown_ticks == 18000);
    assert(race_z4.ai_script_mask == 3);
    assert_runtime_live_roundtrip(); /* Exact pre-hatch RaceSave checkpoint. */

    for (unsigned tick = 0; tick < 18000; ++tick) {
        safe_tick();
        if (tick < 17999) assert(race_z4.countdown_ticks != 0);
    }
    assert(race_z4.model.fired & (1u << SC64_Z4_TRIGGER_HATCH_WARNING));
    assert(race_z4.model.fired & (1u << SC64_Z4_TRIGGER_HATCH));

    unsigned hatch_ticks = 0;
    int saved_mid_hatch = 0;
    int saw_invincible_cutscene_actor = 0;
    while (race_z4.model.action_cursor[SC64_Z4_TRIGGER_HATCH] &&
           hatch_ticks < 5000) {
        safe_tick();
        ++hatch_ticks;
        if ((race_z4.hatch_effects & 1u) && !(race_z4.hatch_effects & 2u) &&
            !saved_mid_hatch) {
            assert(source_unit(150, 3) < 0 && source_building(150, 3) < 0);
            assert_runtime_live_roundtrip();
            saved_mid_hatch = 1;
        }
        int vulture = source_unit(19, -1);
        if (vulture >= 0 && race_z4.model.action_cursor[SC64_Z4_TRIGGER_HATCH] >= 14) {
            assert(invincible_units[vulture]);
            saw_invincible_cutscene_actor = 1;
        }
    }
    assert(hatch_ticks < 5000 && saved_mid_hatch);
    assert(saw_invincible_cutscene_actor);
    assert(race_z4.hatch_effects == 3);
    assert(source_unit(51, 3) >= 0);
    assert_runtime_live_roundtrip(); /* Exact post-hatch RaceSave checkpoint. */
    assert_corrupt_save_preserves_live_world();
    printf("Z4 synthetic protected-world countdown/hatch: startup=%u countdown=18000 hatch=%u ticks; not a controller win proof\n",
           startup_ticks, hatch_ticks);
    int kerrigan = source_unit(51, 3);
    assert(kerrigan >= 0);
    u[kerrigan].alive = 0;
    for (int tick = 0; tick < 1800 && !lost; ++tick) safe_tick();
    assert(lost && !won && race_z4.model.outcome == SC64_Z4_LOST);
}

static void test_source_loss_and_victory_predicates(void)
{
    launch_from_normal_menu();
    int chrysalis = source_unit(150, 3);
    if (chrysalis >= 0) u[chrysalis].alive = 0;
    else {
        chrysalis = source_building(150, 3);
        assert(chrysalis >= 0);
        b[chrysalis].alive = 0;
    }
    for (int i = 0; i < 1800 && !lost; ++i) safe_tick();
    if (!lost) fprintf(stderr, "Z4 pre-hatch loss diagnostic: source150=%d unit=%d building=%d elapsed=%u outcome=%u fired=%x faults=%u screen=%d\n",
                      race_z4_count_owner_type(13, 150, 0),
                      source_unit(150, 3), source_building(150, 3),
                      (unsigned)race_campaign.elapsed_frames,
                      race_z4.model.outcome, race_z4.model.fired,
                      (unsigned)race_z4.faults, screen);
    assert(lost && !won && race_z4.model.outcome == SC64_Z4_LOST);

    launch_from_normal_menu();
    for (int i = 0; i < 1200 && !race_z4.countdown_active; ++i) safe_tick();
    assert(race_z4.countdown_active);
    assert(race_z4_count_owner_type(20, 106, 1) > 0);
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].raw_type == 106 && b[i].raw_owner == 2)
            b[i].alive = 0;
    assert(race_z4_count_owner_type(20, 106, 1) == 0);
    for (int i = 0; i < 1800 && !won; ++i) safe_tick();
    assert(won && !lost && race_z4.model.outcome == SC64_Z4_WON);
}

int main(void)
{
    gba_init();
    full_countdown_and_hatch();
    test_source_loss_and_victory_predicates();
    puts("Z4 real-engine runtime checks passed; forced losses/victory are synthetic predicate tests, not controller completion");
    return 0;
}
