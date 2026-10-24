/* Actual-engine P4 campaign integration checks. World actors are protected
 * during long action-list tests; rescue/contact and win/loss transitions use
 * normal game_update hooks, with actor positions/liveness changed synthetically. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

#ifndef SC64_P4_RUNTIME
#error "Compile with the P4 runtime profile"
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

static void press(u16 key) { input(key); input(0); }

static void protect_live_world(void)
{
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

static int source_unit(int raw, int raw_owner)
{
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].raw_type == raw &&
            (raw_owner < 0 || u[i].raw_owner == raw_owner)) return i;
    return -1;
}

static int source_building(int raw, int raw_owner)
{
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].raw_type == raw &&
            (raw_owner < 0 || b[i].raw_owner == raw_owner)) return i;
    return -1;
}

static int wait_for_source_unit(int raw, int raw_owner, unsigned max_ticks)
{
    int slot = source_unit(raw, raw_owner);
    for (unsigned i = 0; slot < 0 && i < max_ticks; ++i) {
        safe_tick();
        slot = source_unit(raw, raw_owner);
    }
    return slot;
}

static void launch_from_normal_menu(void)
{
    assert(sc64_catalog_playable_count(SC64_FACTION_PROTOSS) == 4);
    assert(sc64_catalog_playable_ordinal(SC64_FACTION_PROTOSS, 23) == 3);
    game_init(); prior_keys = 0;
    assert(race_front.page == SC_FRONT_HOME);
    press(KEY_A);
    assert(race_front.page == SC_FRONT_MISSION &&
           race_front.faction == SC_FRONT_PROTOSS);
    press(KEY_RIGHT); /* Fourth Protoss mission, source mission 23. */
    press(KEY_DOWN);
    assert(race_front.mission_page == 1 && race_front.row == 1);
    assert(sc64_catalog_playable_at(SC64_FACTION_PROTOSS, 3) == 23);
    press(KEY_A);
    assert(race_front.page == SC_FRONT_BRIEFING && race_front.mission == 23);
    input(KEY_A);
    assert(screen == SCREEN_PLAY && !race_mode && !won && !lost);
    assert(race_selected_mission == 23 && race_campaign.mission == 23);
    assert(race_human == R_PROTOSS && race_p4_active());
}

static void check_source_world(void)
{
    unsigned units = 0, buildings = 0, minerals_count = 0;
    unsigned unit_owners[8] = {0}, building_owners[8] = {0};
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive) {
        ++units;
        if (u[i].raw_owner >= 0 && u[i].raw_owner < 8) ++unit_owners[u[i].raw_owner];
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive) {
        ++buildings;
        if (b[i].raw_owner >= 0 && b[i].raw_owner < 8) ++building_owners[b[i].raw_owner];
    }
    for (int i = 0; i < MAX_MINERALS; ++i) minerals_count += m[i].amount > 0;
    /* The staged source has 32 authored building rows. The raw-196 Protoss
     * beacon is reclassified from the unit table into the building pool. */
    assert(units == 137 && buildings == 33 && minerals_count == 26);
    assert(unit_owners[0] == 49 && unit_owners[1] == 7 && unit_owners[2] == 1 &&
           unit_owners[3] == 71 && unit_owners[7] == 9);
    assert(building_owners[0] == 19 && building_owners[1] == 10 &&
           building_owners[3] == 3 && building_owners[7] == 1);
    int rescuable_pylon = source_building(156, 1);
    assert(rescuable_pylon >= 0 && b[rescuable_pylon].owner == RESCUABLE);
    assert(minerals == 50 && gas == 0 && geyser_count == 0 &&
           race_gas[ENEMY] == 8000);
    assert(source_unit(SC64_P4_RAW_TASSADAR, 2) >= 0);
    assert(source_unit(SC64_P4_RAW_RAYNOR_VULTURE, -1) < 0);
}

static void assert_runtime_roundtrip(void)
{
    assert(race_save_game());
    RaceSave expected = race_data;
    memcpy(saved_units, u, sizeof(u));
    memcpy(saved_buildings, b, sizeof(b));
    memcpy(saved_minerals, m, sizeof(m));
    Sc64P4RuntimeState expected_p4 = race_p4;
    Sc64RaceCampaignState expected_campaign = race_campaign;
    int expected_frame = frame;
    assert(race_load_game());
    assert(!memcmp(saved_units, u, sizeof(u)));
    assert(!memcmp(saved_buildings, b, sizeof(b)));
    assert(!memcmp(saved_minerals, m, sizeof(m)));
    assert(!memcmp(&expected_p4, &race_p4, sizeof(race_p4)));
    assert(!memcmp(&expected_campaign, &race_campaign, sizeof(race_campaign)));
    assert(frame == expected_frame && screen == SCREEN_PLAY);
    assert(race_save_game());
    assert(!memcmp(&expected, &race_data, sizeof(expected)));
}

static void assert_bad_save_preserves_world(void)
{
    assert(race_save_game());
    RaceSave corrupt = race_data;
    corrupt.p4.action_ticks[0] = 0xffffu;
    save_write(&corrupt, sizeof(corrupt), SAVE_VERSION);
    memcpy(saved_units, u, sizeof(u));
    memcpy(saved_buildings, b, sizeof(b));
    memcpy(saved_minerals, m, sizeof(m));
    Sc64P4RuntimeState expected_p4 = race_p4;
    Sc64RaceCampaignState expected_campaign = race_campaign;
    int expected_frame = frame, expected_screen = screen;
    assert(!race_load_game());
    assert(!memcmp(saved_units, u, sizeof(u)));
    assert(!memcmp(saved_buildings, b, sizeof(b)));
    assert(!memcmp(saved_minerals, m, sizeof(m)));
    assert(!memcmp(&expected_p4, &race_p4, sizeof(race_p4)));
    assert(!memcmp(&expected_campaign, &race_campaign, sizeof(race_campaign)));
    assert(frame == expected_frame && screen == expected_screen);
}

static void test_contact_rescue_and_source_pause(void)
{
    launch_from_normal_menu();
    check_source_world();
    int tassadar = source_unit(SC64_P4_RAW_TASSADAR, 2);
    int side = -1;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE) { side = i; break; }
    assert(tassadar >= 0 && side >= 0 && u[tassadar].owner == RESCUABLE);
    u[side].x = u[tassadar].x; u[side].y = u[tassadar].y;
    safe_tick();
    assert(u[tassadar].owner == SIDE && u[tassadar].raw_owner == 2);

    int saw_source_pause = 0;
    for (unsigned i = 0; i < 5000 && !saw_source_pause; ++i) {
        safe_tick();
        if (race_p4.source_paused) saw_source_pause = 1;
    }
    assert(saw_source_pause);
    assert_runtime_roundtrip(); /* Exact intro/pause cursor checkpoint. */
    memcpy(saved_units, u, sizeof(u));
    memcpy(saved_buildings, b, sizeof(b));
    Sc64P4RuntimeState before = race_p4;
    int before_frame = frame;
    safe_tick();
    assert(!memcmp(saved_units, u, sizeof(u)));
    assert(!memcmp(saved_buildings, b, sizeof(b)));
    assert(frame == before_frame + 1);
    assert(race_p4.action_cursor[3] != before.action_cursor[3] ||
           race_p4.action_ticks[3] != before.action_ticks[3]);

    /* User pause returns before the campaign scheduler; unlike source pause,
     * it advances neither the action timer nor its cursor. */
    press(KEY_START);
    assert(screen == SCREEN_PAUSE);
    before = race_p4;
    for (int i = 0; i < 5; ++i) input(0);
    assert(!memcmp(&before, &race_p4, sizeof(before)));
    press(KEY_A);
    assert(screen == SCREEN_PLAY);
}

static void test_creation_queue_reserve_and_saves(void)
{
    launch_from_normal_menu();
    check_source_world();
    int tassadar = source_unit(SC64_P4_RAW_TASSADAR, 2), side = -1;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE) { side = i; break; }
    assert(tassadar >= 0 && side >= 0);
    u[side].x = u[tassadar].x; u[side].y = u[tassadar].y;

    int saved_mid_creation = 0;
    int checked_queue_boundary = 0;
    for (unsigned tick = 0; tick < 30000 && race_p4.created_count < 87; ++tick) {
        safe_tick();
        assert(race_p4.faults == 0);
        if (!saved_mid_creation && race_p4.created_count >= 10 &&
            race_p4.created_count < 87) {
            assert_runtime_roundtrip();
            saved_mid_creation = 1;
        }
        if (!checked_queue_boundary && race_p4.created_count >= 10 &&
            race_p4.created_count < 87) {
            int producer = -1;
            for (int i = 0; i < MAX_BUILDINGS; ++i)
                if (b[i].alive && b[i].owner == SIDE && !b[i].under_construction &&
                    sc64_race_producer_compatible(b[i].type, B_NEXUS)) {
                    producer = i; break;
                }
            /* The source human starts with a Pylon but no producer. Add a
             * synthetic Nexus solely to exercise the normal queue guard. */
            if (producer < 0) {
                producer = spawn_build(SIDE, B_NEXUS, 12, 12, 0);
                assert(producer >= 0);
            }
            if (producer >= 0) {
                unsigned reserve = race_p4_reserved_units();
                unsigned free_slots = 0;
                for (int i = 0; i < MAX_UNITS; ++i) free_slots += !u[i].alive;
                int fillers[MAX_UNITS], filler_count = 0;
                while (free_slots > reserve) {
                    int slot = spawn_unit(NEUTRAL, U_PROBE, 8, 8);
                    assert(slot >= 0);
                    fillers[filler_count++] = slot;
                    --free_slots;
                }
                assert(free_slots == reserve);
                int old_queue = b[producer].queue_count, old_minerals = minerals;
                assert(!race_queue_unit(SIDE, producer, U_PROBE));
                assert(b[producer].queue_count == old_queue && minerals == old_minerals);
                while (filler_count) u[fillers[--filler_count]].alive = 0;
                checked_queue_boundary = 1;
            }
        }
    }
    if (race_p4.created_count != 87)
        fprintf(stderr, "P4 create diagnostic: count=%u fired=%x cursors=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u faults=%u frame=%u screen=%d\n",
            race_p4.created_count, race_p4.model.fired,
            race_p4.action_cursor[0], race_p4.action_cursor[1], race_p4.action_cursor[2],
            race_p4.action_cursor[3], race_p4.action_cursor[4], race_p4.action_cursor[5],
            race_p4.action_cursor[6], race_p4.action_cursor[7], race_p4.action_cursor[8],
            race_p4.action_cursor[9], race_p4.action_cursor[10], race_p4.action_cursor[11],
            race_p4.action_cursor[12], race_p4.faults, frame, screen);
    assert(race_p4.created_count == 87 && race_p4_reserved_units() == 0);
    assert(saved_mid_creation && checked_queue_boundary);
    assert_runtime_roundtrip(); /* Exact post-create checkpoint. */
    assert_bad_save_preserves_world();
}

static void test_synthetic_loss_and_escape_win(void)
{
    launch_from_normal_menu();
    int tassadar = source_unit(SC64_P4_RAW_TASSADAR, 2), side = -1;
    int raynor = wait_for_source_unit(SC64_P4_RAW_RAYNOR_VULTURE, -1, 5000);
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE) { side = i; break; }
    assert(tassadar >= 0 && side >= 0 && raynor >= 0);
    u[side].x = u[tassadar].x; u[side].y = u[tassadar].y;
    safe_tick();
    assert(u[tassadar].owner == SIDE);
    u[raynor].alive = 0; /* Synthetic loss; normal trigger/action path follows. */
    for (int i = 0; i < 1000 && !lost; ++i) safe_tick();
    assert(lost && !won && race_p4.model.outcome == SC64_P4_LOST);

    launch_from_normal_menu();
    tassadar = source_unit(SC64_P4_RAW_TASSADAR, 2);
    raynor = wait_for_source_unit(SC64_P4_RAW_RAYNOR_VULTURE, -1, 5000);
    side = -1;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE) { side = i; break; }
    assert(tassadar >= 0 && raynor >= 0 && side >= 0);
    u[side].x = u[tassadar].x; u[side].y = u[tassadar].y;
    safe_tick();
    assert(u[tassadar].owner == SIDE);
    int l, t, r, bottom;
    assert(race_p4_location(SC64_P4_LOCATION_ESCAPE, &l, &t, &r, &bottom));
    u[raynor].x = u[tassadar].x = (l + r) / 2;
    u[raynor].y = u[tassadar].y = (t + bottom) / 2;
    for (int i = 0; i < 1000 && !won; ++i) safe_tick();
    assert(won && !lost && race_p4.model.outcome == SC64_P4_WON);
}

int main(void)
{
    gba_init();
    test_contact_rescue_and_source_pause();
    test_creation_queue_reserve_and_saves();
    test_synthetic_loss_and_escape_win();
    puts("P4 actual-engine runtime integration checks passed (synthetic protection/actor outcomes are not controller-playthrough proof)");
    return 0;
}
