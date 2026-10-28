/* Actual-engine P5 integration checks. The fixture uses the normal frontend,
 * imports the staged map, and lets game_update drive the authored runtime.
 * Protected actors keep this a startup/source-action test, not a campaign-win
 * or native-AI equivalence claim. */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/game.c"
#include "../starcraft/sc64_p5_runtime_adapter.h"

#ifndef SC64_P5_RUNTIME
#error "Compile this fixture with the complete opt-in P5 runtime flags"
#endif
#if !defined(SC64_P5_ACTORS) || !defined(SC64_Z6_ACTORS) || \
    !defined(SC64_ZP_CAMPAIGN_STORAGE)
#error "P5 requires its actor mappings, Guardian mapping, and expanded storage"
#endif

static u16 prior_keys;

static void input(u16 held)
{
    game_update(held, held & (u16)~prior_keys);
    prior_keys = held;
}

static void press(u16 key) { input(key); input(0); }

static void select_protoss_mission(int mission)
{
    int faction = SC64_FACTION_PROTOSS;
    if (race_front.page == SC_FRONT_HOME) press(KEY_A);
    if (race_front.page == SC_FRONT_CAMPAIGN) {
        unsigned guard = 0;
        while (sc64_front_campaign_faction_at(race_front.row) != faction &&
               guard++ < 4) press(KEY_RIGHT);
        assert(sc64_front_campaign_faction_at(race_front.row) == faction);
        press(KEY_A);
    }
    assert(race_front.page == SC_FRONT_MISSION && race_front.faction == faction);
    int ordinal = sc64_catalog_playable_ordinal(faction, mission);
    int count = sc64_catalog_playable_count(faction);
    assert(ordinal >= 0 && ordinal < count);
    int pages = (count + 1) / 2;
    unsigned guard = 0;
    while (race_front.mission_page != ordinal / 2 && guard++ < (unsigned)pages)
        press(KEY_RIGHT);
    assert(race_front.mission_page == ordinal / 2);
    while (race_front.row != ordinal % 2 && guard++ < (unsigned)(pages + 2))
        press(ordinal % 2 ? KEY_DOWN : KEY_UP);
    assert(race_front.row == ordinal % 2);
    press(KEY_A);
    assert(race_front.page == SC_FRONT_BRIEFING && race_front.mission == mission);
    input(KEY_START);
    if (!(screen == SCREEN_PLAY && !race_mode && !won && !lost))
        fprintf(stderr, "P5 launch state screen=%d mission=%d unsupported=%d fault=%u\n",
                screen, race_selected_mission, mission_unsupported_count,
                race_p5.runtime.fault);
    assert(screen == SCREEN_PLAY && !race_mode && !won && !lost);
    assert(race_selected_mission == mission && race_campaign.mission == mission);
}

static void protect_world(void)
{
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive) invincible_units[i] = 1;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive) invincible_buildings[i] = 1;
}

static void safe_tick(void)
{
    protect_world();
    input(0);
    protect_world();
}

static unsigned count_raw_unit(unsigned raw, int source_owner)
{
    unsigned count = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].raw_type == (int)raw &&
            (source_owner < 0 || u[i].raw_owner == source_owner)) ++count;
    return count;
}

static void check_source_census(void)
{
    unsigned units = 0, buildings = 0, minerals_count = 0;
    for (int i = 0; i < MAX_UNITS; ++i) units += u[i].alive != 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i) buildings += b[i].alive != 0;
    for (int i = 0; i < MAX_MINERALS; ++i) minerals_count += m[i].amount > 0;
    assert(units == 110 && buildings == 69 && minerals_count == 57);
    assert((unsigned)geyser_count == 3);
    assert(race_p5_preflight(&sc64_race_maps[SC64_P5_GLOBAL_ID]));
    assert(race_p5_starting_world_matches_source(
               &sc64_race_maps[SC64_P5_GLOBAL_ID]));
    assert(race_p5_runtime_state_valid());
    assert(race_p5_reserved_units() == SC64_P5_SOURCE_CREATE_COUNT);
    assert(race_p5_remaining_creations(&race_p5) == SC64_P5_SOURCE_CREATE_COUNT);
    assert(race_p5_saved_remaining_creations(&race_p5) ==
           SC64_P5_SOURCE_CREATE_COUNT);
    /* Source OWNR/SIDE metadata stays authentic; live ownership gets distinct
     * force buckets for target queries and is checked against every map row. */
    assert(race_p5_live_owner_for_source(7) == SIDE);
    assert(race_p5_live_owner_for_source(2) == RESCUABLE);
    assert(race_p5_live_owner_for_source(3) == ENEMY);
    assert(race_p5_live_owner_for_source(4) == 4);
    assert(race_p5_live_owner_for_source(6) == NEUTRAL);
    assert(race_p5_live_owner_for_source(0) == NEUTRAL);
    assert(race_p5_source_owner(7) == SIDE);
    assert(race_p5_source_owner(2) == RESCUABLE);
    assert(race_p5_source_owner(3) == ENEMY);
    assert(race_p5_source_owner(4) == 4);
    assert(race_p5_source_owner(6) == NEUTRAL);
    assert(race_player[NEUTRAL] == race_p5_owner_races_expected[6]);
    Sc64P5PersistentState invalid = race_p5;
    invalid.reserved[0] = 1;
    assert(!sc64_p5_persistent_state_valid(&invalid));
    invalid = race_p5;
    invalid.pressure_dispatched_mask = 0x80;
    assert(!sc64_p5_persistent_state_valid(&invalid));
}

static void check_selectors_rescue_and_source_deaths(void)
{
    int rescuable = -1, human = -1, death_actor = -1, death_source = -1;
    unsigned before;
    assert(!race_p5_sources_hostile(7, 2)); /* Same Force 1. */
    assert(race_p5_sources_hostile(7, 6));
    assert(race_p5_sources_hostile(3, 6)); /* Separate hostile forces. */
    assert(race_p5_sources_hostile(4, 6));
    assert(!race_p5_sources_hostile(3, 3));
    assert(!race_p5_sources_hostile(7, 0)); /* Neutral map slots do not fight. */
    assert(!race_p5_sources_hostile(0, 5));
    assert(!race_p5_owners_hostile(SIDE, RESCUABLE));
    assert(race_p5_owners_hostile(SIDE, ENEMY));
    assert(race_p5_owners_hostile(ENEMY, NEUTRAL));
    assert(race_p5_owners_hostile(4, NEUTRAL));
    assert(!race_p5_owners_hostile(NEUTRAL, NEUTRAL));
    assert(!race_p5_owners_hostile(RESCUABLE, RESCUABLE));
    assert(owners_hostile(ENEMY, NEUTRAL));
    assert(owners_hostile(4, NEUTRAL));
    assert(!owners_hostile(SIDE, RESCUABLE));
    assert(!owners_hostile(NEUTRAL, NEUTRAL));
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive) {
        if (u[i].owner == RESCUABLE && rescuable < 0) rescuable = i;
        if (u[i].owner == SIDE && human < 0) human = i;
    }
    /* Trigger 5's authenticated gate is exactly one Deaths for source slot 6.
     * That source has buildings rather than units, so exercise the real
     * building hit/death hook. */
    for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive && b[i].raw_owner == 6) {
        death_actor = MAX_UNITS + i;
        death_source = b[i].raw_owner;
        break;
    }
    assert(rescuable >= 0 && human >= 0 && death_actor >= 0);
    u[rescuable].x = u[human].x;
    u[rescuable].y = u[human].y;
    int original_source = u[rescuable].raw_owner;
    race_p5_rescue_tick();
    assert(u[rescuable].owner == SIDE && u[rescuable].raw_owner == original_source);
    before = race_p5.deaths_by_source[death_source];
    if (death_actor < MAX_UNITS) {
        u[death_actor].shield = 0;
        u[death_actor].hp = 1;
    } else {
        b[death_actor - MAX_UNITS].hp = 1;
    }
    hit_target(death_actor, 10, U_ZEALOT, SIDE);
    if (death_actor < MAX_UNITS) assert(!u[death_actor].alive);
    else assert(!b[death_actor - MAX_UNITS].alive);
    assert(race_p5.deaths_by_source[death_source] == before + 1);
    assert(race_p5_runtime_state_valid());
}

static void check_save_roundtrip_and_rejection(void)
{
    RaceSave good;
    Sc64P5PersistentState saved_p5;
    u32 saved_frame;
    assert(race_save_game());
    memcpy(&good, &race_data, sizeof(good));
    saved_p5 = race_p5;
    saved_frame = (u32)frame;
    assert(good.version == SAVE_VERSION);
    assert(good.state[2] == SC64_P5_GLOBAL_ID && good.p5.runtime.model.started == 0);

    safe_tick();
    assert((u32)frame != saved_frame);
    assert(race_load_game());
    assert((u32)frame == saved_frame);
    assert(memcmp(&race_p5, &saved_p5, sizeof(saved_p5)) == 0);
    assert(race_p5_runtime_state_valid());

    /* A semantically corrupt but checksum-valid newest record is rejected
     * before any live world or mission state is committed. */
    Unit units_before[MAX_UNITS];
    Building buildings_before[MAX_BUILDINGS];
    Sc64P5PersistentState state_before = race_p5;
    Sc64RaceCampaignState campaign_before = race_campaign;
    int frame_before = frame;
    memcpy(units_before, u, sizeof(units_before));
    memcpy(buildings_before, b, sizeof(buildings_before));
    RaceSave bad = good;
    bad.p5.reserved[0] = 1;
    save_write(&bad, sizeof(bad), SAVE_VERSION);
    assert(!race_load_game());
    assert(frame == frame_before && memcmp(&race_p5, &state_before, sizeof(state_before)) == 0);
    assert(memcmp(&race_campaign, &campaign_before, sizeof(campaign_before)) == 0);
    assert(memcmp(units_before, u, sizeof(units_before)) == 0);
    assert(memcmp(buildings_before, b, sizeof(buildings_before)) == 0);

    save_write(&good, sizeof(good), SAVE_VERSION - 1u);
    assert(!race_load_game());
    assert(frame == frame_before && memcmp(units_before, u, sizeof(units_before)) == 0);
    save_write(&good, sizeof(good), SAVE_VERSION);
    assert(race_load_game());
    assert((u32)frame == saved_frame &&
           memcmp(&race_p5, &saved_p5, sizeof(saved_p5)) == 0);
}

static void check_source_create_program(void)
{
    const unsigned wraiths_before = count_raw_unit(8, 6);
    const unsigned battlecruisers_before = count_raw_unit(12, 6);
    const unsigned revealers_before = count_raw_unit(101, 7);
    unsigned ticks;
    for (ticks = 0; ticks < 600; ++ticks) {
        if (count_raw_unit(8, 6) >= wraiths_before + 9 &&
            count_raw_unit(12, 6) >= battlecruisers_before + 1 &&
            count_raw_unit(101, 7) >= revealers_before + 1) break;
        safe_tick();
        assert(race_p5.runtime.fault == SC64_P5_FAULT_NONE);
    }
    if (ticks == 600)
        fprintf(stderr,"P5 create progress frame=%d started=%04x trig3cursor=%u ticks=%u wraiths=%u bc=%u revealer=%u\n",
                frame,race_p5.runtime.model.started,race_p5.runtime.model.action_cursor[3],
                race_p5.runtime.action_ticks[3],count_raw_unit(8,6),count_raw_unit(12,6),
                count_raw_unit(101,7));
    assert(ticks < 600);
    assert(count_raw_unit(8, 6) == wraiths_before + 9);
    assert(count_raw_unit(12, 6) == battlecruisers_before + 1);
    assert(count_raw_unit(101, 7) == revealers_before + 1);
    /* Cursor is current action index + 1; creation indices 0..10 end at 11. */
    assert(race_p5.runtime.model.action_cursor[3] >= 11);
    assert(race_p5_reserved_units() == 0);

    /* A started trigger with cursor zero is complete, unlike the fresh
     * unstarted state (11 reserved) and an in-flight cursor (10 remain after
     * action 0 has already dispatched). Keep runtime and save validation in
     * lockstep on that distinction. */
    Sc64P5PersistentState progress = race_p5;
    progress.runtime.model.action_cursor[3] = 1;
    progress.runtime.action_ticks[3] = 1;
    assert(sc64_p5_persistent_state_valid(&progress));
    assert(race_p5_remaining_creations(&progress) == 10);
    assert(race_p5_saved_remaining_creations(&progress) == 10);
    progress.runtime.model.action_cursor[3] = 0;
    progress.runtime.action_ticks[3] = 0;
    assert(sc64_p5_persistent_state_valid(&progress));
    assert(race_p5_remaining_creations(&progress) == 0);
    assert(race_p5_saved_remaining_creations(&progress) == 0);
    Sc64P5PersistentState original = race_p5;
    race_p5 = progress;
    assert(race_p5_reserved_units() == 0);
    race_p5 = original;
    assert(race_p5_runtime_state_valid());
}

static void check_bad_descriptor_fails_closed(void)
{
    Sc64RaceMap bad = sc64_race_maps[SC64_P5_GLOBAL_ID];
    unsigned char owner_modes[8];
    unsigned char owner_races[8];
    for (unsigned i = 0; i < 8; ++i) {
        owner_modes[i] = bad.owner_modes[i];
        owner_races[i] = bad.owner_races[i];
    }
    bad.owner_modes = owner_modes;
    bad.owner_races = owner_races;
    owner_modes[6] ^= 1;
    assert(!race_p5_preflight(&bad));
    owner_modes[6] ^= 1;
    bad.unit_count--;
    assert(!race_p5_preflight(&bad));
}

int main(void)
{
    gba_init();
    assert(sc64_p5_source_model_valid());
    assert(race_p5_source_model_preflight());
    assert(race_p5_preflight(&sc64_race_maps[SC64_P5_GLOBAL_ID]));
    assert(sc64_catalog_playable_ordinal(SC64_FACTION_PROTOSS,
                                         SC64_P5_GLOBAL_ID) >= 0);
    assert(sc64_catalog_playable_ordinal(SC64_FACTION_PROTOSS,
                                         SC64_P5_GLOBAL_ID) == 4);
    game_init(); prior_keys = 0;
    select_protoss_mission(SC64_P5_GLOBAL_ID);
    assert(race_p5_active());
    check_source_census();
    check_bad_descriptor_fails_closed();
    check_save_roundtrip_and_rejection();
    check_selectors_rescue_and_source_deaths();
    check_source_create_program();
    puts("P5 normal-menu startup, source census, force hostility, save validation, and creation checks passed");
    return 0;
}
