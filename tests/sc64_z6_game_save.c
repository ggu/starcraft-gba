/* Z6 actual-engine save integration. The fixture launches from the ordinary
 * menu, advances the live mission, round-trips the shared Z5/Z6 payload, and
 * verifies malformed records are rejected before the live world changes. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

#ifndef SC64_Z6_RUNTIME
#error "Compile this fixture with the complete Z6 runtime profile flags"
#endif

static unsigned held_keys;
static int players_snapshot[5], minerals_snapshot[5], gas_snapshot[5];
static Geyser geysers_snapshot[MAX_RACE_GEYSERS];
static u8 fog_snapshot[(MAP_CELLS + 7) / 8];
static u8 invincible_units_snapshot[MAX_UNITS];
static u8 invincible_buildings_snapshot[MAX_BUILDINGS];

static void input(unsigned held)
{
    game_update((u16)held, (u16)(held & ~held_keys));
    held_keys = held;
}

static void press(unsigned key)
{
    input(key);
    input(0);
}

static void launch_z6_from_normal_menu(void)
{
    int ordinal;
    game_init();
    held_keys = 0;
    assert(race_front.page == SC_FRONT_HOME);
    assert(sc64_catalog_playable_count(SC64_FACTION_ZERG) >= 6);
    ordinal = sc64_catalog_playable_ordinal(SC64_FACTION_ZERG,
                                            SC64_Z6_GLOBAL_ID);
    assert(ordinal == 5);
    press(KEY_A);
    assert(race_front.page == SC_FRONT_MISSION);
    assert(race_front.faction == SC_FRONT_ZERG);
    for (int page = 0; page < ordinal / 2; ++page) press(KEY_RIGHT);
    if (ordinal & 1) press(KEY_DOWN);
    press(KEY_A);
    assert(race_front.page == SC_FRONT_BRIEFING &&
           race_front.mission == SC64_Z6_GLOBAL_ID);
    press(KEY_A);
    assert(screen == SCREEN_PLAY && !race_mode && !won && !lost);
    assert(race_selected_mission == SC64_Z6_GLOBAL_ID && race_z6_active());
    assert(race_campaign.mission == SC64_Z6_GLOBAL_ID);
    assert(sc64_z6_persistent_state_valid(&race_z6));
    assert(SC64_RACE_SAVE_GENERATION == 32u);
}

static void issue_a_normal_group_move(void)
{
    unsigned selected = 0, moving = 0;
    press(KEY_R);          /* Ordinary combat-group selection. */
    press(KEY_B);          /* Open the normal field command menu. */
    assert(menu_open && menu_cursor == 0);
    press(KEY_A);          /* First field command is Move. */
    assert(mode == MODE_MOVE && !menu_open);
    for (unsigned i = 0; i < 8; ++i) input(KEY_RIGHT | KEY_DOWN);
    press(KEY_A);          /* Issue the order at the ordinary cursor. */
    for (unsigned i = 0; i < MAX_UNITS; ++i) {
        if (u[i].owner == SIDE && u[i].selected) {
            ++selected;
            moving += u[i].order == ORDER_MOVE;
        }
    }
    assert(selected > 0 && moving > 0);
}

static void remember_world(Unit *units, Building *buildings, Mineral *minerals,
                           Sc64Z6PersistentState *state,
                           Sc64RaceCampaignState *campaign, int *clock)
{
    memcpy(units, u, sizeof(u));
    memcpy(buildings, b, sizeof(b));
    memcpy(minerals, m, sizeof(m));
    *state = race_z6;
    *campaign = race_campaign;
    *clock = frame;
    memcpy(players_snapshot, race_player, sizeof(players_snapshot));
    memcpy(minerals_snapshot, race_minerals, sizeof(minerals_snapshot));
    memcpy(gas_snapshot, race_gas, sizeof(gas_snapshot));
    memcpy(geysers_snapshot, geysers, sizeof(geysers_snapshot));
    memcpy(fog_snapshot, explored, sizeof(fog_snapshot));
    memcpy(invincible_units_snapshot, invincible_units,
           sizeof(invincible_units_snapshot));
    memcpy(invincible_buildings_snapshot, invincible_buildings,
           sizeof(invincible_buildings_snapshot));
}

static void assert_world_unchanged(const Unit *units, const Building *buildings,
                                   const Mineral *minerals,
                                   const Sc64Z6PersistentState *state,
                                   const Sc64RaceCampaignState *campaign,
                                   int clock)
{
    assert(!memcmp(units, u, sizeof(u)));
    assert(!memcmp(buildings, b, sizeof(b)));
    assert(!memcmp(minerals, m, sizeof(m)));
    assert(!memcmp(state, &race_z6, sizeof(race_z6)));
    assert(!memcmp(campaign, &race_campaign, sizeof(race_campaign)));
    assert(!memcmp(players_snapshot, race_player, sizeof(players_snapshot)));
    assert(!memcmp(minerals_snapshot, race_minerals, sizeof(minerals_snapshot)));
    assert(!memcmp(gas_snapshot, race_gas, sizeof(gas_snapshot)));
    assert(!memcmp(geysers_snapshot, geysers, sizeof(geysers_snapshot)));
    assert(!memcmp(fog_snapshot, explored, sizeof(fog_snapshot)));
    assert(!memcmp(invincible_units_snapshot, invincible_units,
                   sizeof(invincible_units_snapshot)));
    assert(!memcmp(invincible_buildings_snapshot, invincible_buildings,
                   sizeof(invincible_buildings_snapshot)));
    assert(frame == clock);
}

static void reject_without_mutation(const RaceSave *bad, u32 generation)
{
    Unit units[MAX_UNITS];
    Building buildings[MAX_BUILDINGS];
    Mineral minerals_before[MAX_MINERALS];
    Sc64Z6PersistentState state;
    Sc64RaceCampaignState campaign;
    int clock;
    remember_world(units, buildings, minerals_before, &state, &campaign, &clock);
    save_write(bad, sizeof(*bad), generation);
    assert(!race_load_game());
    assert_world_unchanged(units, buildings, minerals_before, &state,
                           &campaign, clock);
}

static void test_live_roundtrip_and_transactional_rejection(void)
{
    Unit units[MAX_UNITS];
    Building buildings[MAX_BUILDINGS];
    Mineral minerals_before[MAX_MINERALS];
    Sc64Z6PersistentState state;
    Sc64RaceCampaignState campaign;
    RaceSave good, bad;
    int clock;

    launch_z6_from_normal_menu();
    issue_a_normal_group_move();
    for (unsigned i = 0; i < 96; ++i) {
        game_update(0, 0);
        assert(screen == SCREEN_PLAY && !won && !lost);
        assert(sc64_z6_runtime_state_valid(&race_z6.runtime));
    }
    assert(race_campaign.elapsed_frames == (u32)frame);
    assert(race_save_game());
    good = race_data;
    assert(race_z6_save_candidate_valid(&good, &good.z6,
                                         SC64_Z6_MAP_COLS,
                                         SC64_Z6_MAP_ROWS));
    assert(sizeof(RaceSave) <= 16368u);
    for (unsigned i = sizeof(good.z6); i < sizeof(good.z5); ++i)
        assert(((const u8 *)&good.z6)[i] == 0);

    remember_world(units, buildings, minerals_before, &state, &campaign, &clock);
    assert(race_load_game());
    assert(!memcmp(units, u, sizeof(u)));
    assert(!memcmp(buildings, b, sizeof(b)));
    assert(!memcmp(minerals_before, m, sizeof(m)));
    assert(!memcmp(&state, &race_z6, sizeof(race_z6)));
    assert(!memcmp(&campaign, &race_campaign, sizeof(race_campaign)));
    assert(frame == clock && race_z6_map == &sc64_race_maps[SC64_Z6_GLOBAL_ID]);
    assert(race_z6_world.source_slot_active[
        sc64_race_maps[SC64_Z6_GLOBAL_ID].human_slot]);

    /* Normal simulation movement must remain saveable; movement of source
     * actors is not treated as an immutable placement identity. */
    for (unsigned i = 0; i < 30; ++i) game_update(0, 0);
    assert(race_save_game());
    good = race_data;

    bad = good;
    bad.z6.ai_plan_ticks = 241;
    reject_without_mutation(&bad, SAVE_VERSION);

    bad = good;
    bad.state[16] = good.state[16] + 1;
    reject_without_mutation(&bad, SAVE_VERSION);

    bad = good;
    ++bad.geysers[0].x;
    reject_without_mutation(&bad, SAVE_VERSION);

    assert(good.state[16] == 3);
    bad = good;
    bad.geysers[1] = bad.geysers[0];
    reject_without_mutation(&bad, SAVE_VERSION);

    bad = good;
    {
        int changed = 0;
        for (unsigned i = 0; i < MAX_UNITS; ++i) {
            Unit decoded;
            assert(race_save_decode_unit(&bad, i, SC64_Z6_MAP_COLS * 16u,
                                         SC64_Z6_MAP_ROWS * 16u, &decoded));
            if (!decoded.alive || decoded.raw_owner < 0) continue;
            decoded.raw_owner = (s16)((decoded.raw_owner + 1) & 7);
            assert(sc64_zp_unit_save_encode(&bad.units[i], &decoded,
                                             SC64_Z6_MAP_COLS * 16u,
                                             SC64_Z6_MAP_ROWS * 16u));
            changed = 1;
            break;
        }
        assert(changed);
    }
    reject_without_mutation(&bad, SAVE_VERSION);
    reject_without_mutation(&good, SAVE_VERSION - 1u);

    save_write(&good, sizeof(good), SAVE_VERSION);
    assert(race_load_game());
    assert(sc64_z6_runtime_state_valid(&race_z6.runtime));
    assert(race_z6_world.source_slot_active[
        sc64_race_maps[SC64_Z6_GLOBAL_ID].human_slot]);

    /* A cold HOME load must not require any previous live map. Reset the
     * resource globals to their C-startup values: game_init itself does not
     * import a map or reset a previous game's resource count. */
    save_write(&good, sizeof(good), SAVE_VERSION);
    game_init();
    geyser_count = 0;
    memset(geysers, 0, sizeof(geysers));
    held_keys = 0;
    assert(screen == SCREEN_TITLE && race_front.page == SC_FRONT_HOME);
    assert(geyser_count == 0);
    assert(race_z6_save_candidate_valid(&good, &good.z6,
                                         SC64_Z6_MAP_COLS,
                                         SC64_Z6_MAP_ROWS));
    input(KEY_L);
    assert(screen == SCREEN_PLAY && race_selected_mission == SC64_Z6_GLOBAL_ID);
    assert(geyser_count == good.state[16] && frame == good.state[7]);
    assert(!memcmp(geysers, good.geysers, sizeof(geysers)));
    assert(!memcmp(&race_z6, &good.z6, sizeof(race_z6)));
    puts("Z6 normal-menu save/load and transactional rejection passed");
}

int main(void)
{
    test_live_roundtrip_and_transactional_rejection();
    return 0;
}
