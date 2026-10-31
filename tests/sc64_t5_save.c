/* T5 save schema/restore fixture. Direct field setup in a few cases exercises
 * persisted states; it is not evidence of a controller-completed campaign. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"
#include "sc64_save_candidate_fixture.h"

static RaceSave valid_record;
static Unit saved_units[MAX_UNITS];
static Building saved_buildings[MAX_BUILDINGS];
static Mineral saved_minerals[MAX_MINERALS];
static Sc64T5RuntimeState saved_runtime;
static Sc64RaceCampaignState saved_campaign;
static u8 saved_explored[(MAP_CELLS + 7) / 8];
static int saved_frame, saved_cx, saved_cy, saved_minerals_count, saved_gas_count;
static int saved_players[5], saved_ore[5], saved_vespene[5];

#ifdef SC64_LATE_TERRAN_STORAGE
static int saved_unit_field(const RaceSave *record, int slot, size_t offset)
{
    Unit decoded;
    assert(sc64_unit_save_decode(&decoded, &record->units[slot],
                                 (unsigned)(race_cols * 16),
                                 (unsigned)(race_rows * 16)));
    if (offset == offsetof(Unit, alive)) return decoded.alive;
    if (offset == offsetof(Unit, type)) return decoded.type;
    if (offset == offsetof(Unit, carry)) return decoded.carry;
    assert(0 && "unsupported packed Unit field in save fixture");
    return 0;
}

static void corrupt_saved_unit_field(RaceSave *record, int slot,
                                     size_t offset, int value)
{
    Unit decoded;
    assert(sc64_unit_save_decode(&decoded, &record->units[slot],
                                 (unsigned)(race_cols * 16),
                                 (unsigned)(race_rows * 16)));
    if (offset == offsetof(Unit, carry)) decoded.carry = value;
    else assert(0 && "unsupported packed Unit corruption field");
    assert(sc64_unit_save_encode(&record->units[slot], &decoded,
                                (unsigned)(race_cols * 16),
                                (unsigned)(race_rows * 16)));
}
#else
static int saved_unit_field(const RaceSave *record, int slot, size_t offset)
{
    return record->units[slot][offset / sizeof(int)];
}

static void corrupt_saved_unit_field(RaceSave *record, int slot,
                                     size_t offset, int value)
{
    record->units[slot][offset / sizeof(int)] = (s16)value;
}
#endif

#ifdef SC64_TERRAN_LATE_RUNTIME
#define SC64_TEST_OPTIONAL_SAVE_BYTES sizeof(race_data.terran_late)
#elif defined(SC64_T6_RUNTIME)
#define SC64_TEST_OPTIONAL_SAVE_BYTES sizeof(race_data.t6)
#else
#define SC64_TEST_OPTIONAL_SAVE_BYTES sizeof(race_data.t4)
#endif

static void start_t5(void)
{
    game_init();
    race_mode = 0;
    race_selected_mission = SC64_T5_GLOBAL_ID;
    init_state();
    assert(race_t5_active() && screen == SCREEN_PLAY);
    assert(race_cols == 96 && race_rows == 96);
    assert(!won && !lost && race_campaign.phase == SC64_RACE_PHASE_ACTIVE);
}

static void remember_live(void)
{
    memcpy(saved_units, u, sizeof(u));
    memcpy(saved_buildings, b, sizeof(b));
    memcpy(saved_minerals, m, sizeof(m));
    saved_runtime = race_t5;
    saved_campaign = race_campaign;
    memcpy(saved_explored, explored, sizeof(saved_explored));
    saved_frame = frame; saved_cx = cx; saved_cy = cy;
    saved_minerals_count = minerals; saved_gas_count = gas;
    memcpy(saved_players, race_player, sizeof(saved_players));
    memcpy(saved_ore, race_minerals, sizeof(saved_ore));
    memcpy(saved_vespene, race_gas, sizeof(saved_vespene));
}

static void assert_live_unchanged(void)
{
    assert(!memcmp(saved_units, u, sizeof(u)));
    assert(!memcmp(saved_buildings, b, sizeof(b)));
    assert(!memcmp(saved_minerals, m, sizeof(m)));
    assert(!memcmp(&saved_runtime, &race_t5, sizeof(race_t5)));
    assert(!memcmp(&saved_campaign, &race_campaign, sizeof(race_campaign)));
    assert(!memcmp(saved_explored, explored, sizeof(saved_explored)));
    assert(frame == saved_frame && cx == saved_cx && cy == saved_cy);
    assert(minerals == saved_minerals_count && gas == saved_gas_count);
    assert(!memcmp(saved_players, race_player, sizeof(saved_players)));
    assert(!memcmp(saved_ore, race_minerals, sizeof(saved_ore)));
    assert(!memcmp(saved_vespene, race_gas, sizeof(saved_vespene)));
}

static void reset_to_good_sram(void)
{
    write_candidate_record(&valid_record);
}

static void reject_record_atomically(const RaceSave *bad)
{
    remember_live();
    write_candidate_record(bad);
    assert(!race_load_game());
    assert_live_unchanged();
    reset_to_good_sram();
}

static void active_runtime_roundtrip(void)
{
    /* Let normal campaign ticks establish the initial resource grant and
     * advance each active source-player clock before persisting. */
    for (int i = 0; i < 35; ++i) game_update(0, 0);
    assert(screen == SCREEN_PLAY && !won && !lost);
    assert(race_t5.active_mask == SC64_T5_ACTIVE_SOURCE_MASK);
    assert(race_t5.player[0].elapsed_ticks == (u32)race_campaign.elapsed_frames);
    assert(race_t5.player[2].elapsed_ticks == (u32)race_campaign.elapsed_frames);
    assert(race_t5.resources[0].total_minerals_gathered == 300);
    assert(race_t5.resources[0].total_gas_gathered == 300);

    assert(race_save_game());
    fixture_capture_save(&valid_record);
    assert(race_data.t5.active_mask == SC64_T5_ACTIVE_SOURCE_MASK);
    assert(race_t5_save_candidate_valid(&race_data, race_cols, race_rows));
    {
        RaceSave erased_grant = valid_record;
        memset(&erased_grant.t5.resources[0], 0,
               sizeof(erased_grant.t5.resources[0]));
        reject_record_atomically(&erased_grant);
    }
    for (unsigned i = sizeof(race_data.t5); i < SC64_TEST_OPTIONAL_SAVE_BYTES; ++i)
        assert(((const u8 *)&race_data.t4)[i] == 0);

    remember_live();
    /* Damage/alter the live state, then require a successful SRAM load to
     * restore both the world and the compact trigger/runtime union. */
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive) { u[i].hp = 1; break; }
    race_t5.player[0].elapsed_ticks += 7;
    race_t5.resources[0].current_minerals += 7;
    minerals += 7;
    assert(race_load_game());
    assert(!memcmp(saved_units, u, sizeof(u)));
    assert(!memcmp(saved_buildings, b, sizeof(b)));
    assert(!memcmp(saved_minerals, m, sizeof(m)));
    assert(!memcmp(&saved_runtime, &race_t5, sizeof(race_t5)));
    assert(!memcmp(&saved_campaign, &race_campaign, sizeof(race_campaign)));
    assert(saved_frame == frame && saved_cx == cx && saved_cy == cy);
    assert(saved_minerals_count == minerals && saved_gas_count == gas);
    assert(!memcmp(saved_players, race_player, sizeof(saved_players)));
    assert(!memcmp(saved_ore, race_minerals, sizeof(saved_ore)));
    assert(!memcmp(saved_vespene, race_gas, sizeof(saved_vespene)));
}

static void malformed_records_are_atomic(void)
{
    for (unsigned n = 0; n < 13; ++n) {
        RaceSave bad = valid_record;
        Sc64T4TriggerState *player = &bad.t5.player[0];
        switch (n) {
        case 0: player->cursor[0] = 64; break;
        case 1: player->wait_ticks = 751; player->active_wait = 0; break;
        case 2: bad.t5.ai.reserved[0] = 1; break;
        case 3: bad.t5.active_mask ^= 1; break;
        case 4: bad.campaign.phase = 0xff; break;
        case 5: bad.t5.resources[0].current_minerals = 65536u; break;
        case 6: bad.t5.player[2].owner = 7; break;
        case 7: bad.t5.message_ticks = 751; break;
        case 8: bad.t5.ping_ticks = 751; break;
        case 9: bad.t5.player[1].outcome = SC64_T4_TR_WON; break;
        case 10: ((u8 *)&bad.t5)[sizeof(bad.t5)] = 1; break;
        case 11: {
            int mine = -1;
            for (int i = 0; i < MAX_UNITS; ++i)
                if (saved_unit_field(&bad, i, offsetof(Unit, alive)) &&
                    saved_unit_field(&bad, i, offsetof(Unit, type)) == U_SPIDER_MINE) {
                    mine = i; break;
                }
            assert(mine >= 0);
            corrupt_saved_unit_field(&bad, mine, offsetof(Unit, carry), 99);
            break;
        }
        /* An in-range mission bit from another race remains invalid. */
        case 12: bad.state[3] |= 1u << 0; break;
        }
        if (n <= 9 && n != 4)
            assert(!race_t5_save_candidate_valid(&bad, race_cols, race_rows));
        else if (n == 4) assert(bad.campaign.phase == 0xff);
        else if (n == 10) assert(((const u8 *)&bad.t5)[sizeof(bad.t5)] == 1);
        else if (n == 11) {
            int found = 0;
            for (int i = 0; i < MAX_UNITS; ++i)
                if (saved_unit_field(&bad, i, offsetof(Unit, alive)) &&
                    saved_unit_field(&bad, i, offsetof(Unit, type)) == U_SPIDER_MINE &&
                    saved_unit_field(&bad, i, offsetof(Unit, carry)) == 99) found = 1;
            assert(found);
        } else assert(bad.state[3] & 1u);
        reject_record_atomically(&bad);
    }
}

static void inactive_context_must_be_canonical(void)
{
    Sc64T5RuntimeState saved;
    start_t5();
    for (int i = 0; i < 3; ++i) game_update(0, 0);
    assert(race_save_game());
    fixture_capture_save(&valid_record);
    saved = race_t5;
    ++race_t5.player[1].elapsed_ticks; /* Source slot 1 is omitted from save. */
    assert(!race_save_game());
    race_t5 = saved;
    assert(race_load_game());
    assert(!memcmp(&saved, &race_t5, sizeof(saved)));
}

static void early_provenance_is_rejected(void)
{
    start_t5();
    assert(race_save_game());
    fixture_capture_save(&valid_record);
    assert(!valid_record.t5.ai.requested_scripts[2]);
    RaceSave bad = valid_record;
    bad.t5.ai.requested_scripts[2] |= SC64_T5_SCRIPT_BIT_RSCU;
    reject_record_atomically(&bad);

    bad = valid_record;
    bad.t5.resources[6].current_minerals = 6000;
    bad.t5.resources[6].current_gas = 6000;
    bad.t5.resources[6].total_minerals_gathered = 6000;
    bad.t5.resources[6].total_gas_gathered = 6000;
    reject_record_atomically(&bad);
}

static void gated_grant_roundtrip(void)
{
    start_t5();
    int marine = -1;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].type == U_MARINE) {
            marine = i; break;
        }
    assert(marine >= 0);
    /* Positioning is fixture setup for the imported Bring condition, not a
     * normal-controller objective proof. The actual trigger/action runs in
     * ordinary game_update ticks. */
    const Sc64T4Location *south = &sc64_t5_scenario.locations[17];
    u[marine].x = (int)(south->left + south->right) / 4;
    u[marine].y = (int)(south->top + south->bottom) / 4;
    for (int i = 0; i < 40; ++i) game_update(0, 0);
    assert(screen == SCREEN_PLAY && !won && !lost);
    assert(race_t5.switches.bits[0] & (1u << 6));
    assert(race_t5.resources[6].current_minerals == 6000);
    assert(race_t5.resources[6].current_gas == 6000);
    assert(race_t5.resources[6].total_minerals_gathered == 6000);
    assert(race_save_game());
    fixture_capture_save(&valid_record);
    {
        RaceSave erased_grant = valid_record;
        memset(&erased_grant.t5.resources[6], 0,
               sizeof(erased_grant.t5.resources[6]));
        reject_record_atomically(&erased_grant);
    }
    remember_live();
    race_t5.switches.bits[0] = 0;
    race_t5.resources[6].current_minerals = 1;
    assert(race_load_game());
    assert(!memcmp(saved_units, u, sizeof(u)));
    assert(!memcmp(saved_buildings, b, sizeof(b)));
    assert(!memcmp(&saved_runtime, &race_t5, sizeof(race_t5)));
    assert(!memcmp(&saved_campaign, &race_campaign, sizeof(race_campaign)));
}

static void terminal_outcome_roundtrip(int victory)
{
    start_t5();
    /* Controlled source-predicate fixture: clear the relevant actor class,
     * then let the ordinary trigger runner choose and persist the outcome. */
    if (victory) {
        for (int i = 0; i < MAX_BUILDINGS; ++i)
            if (b[i].alive && b[i].owner == ENEMY) b[i].alive = 0;
    } else {
        for (int i = 0; i < MAX_UNITS; ++i)
            if (u[i].alive && u[i].owner == SIDE) u[i].alive = 0;
        for (int i = 0; i < MAX_BUILDINGS; ++i)
            if (b[i].alive && b[i].owner == SIDE) b[i].alive = 0;
    }
    for (int i = 0; i < 90 && screen == SCREEN_PLAY; ++i)
        game_update(0, 0);
    assert(screen == SCREEN_RESULT);
    assert(won == !!victory && lost == !victory);
    assert(race_t5.player[0].outcome ==
           (victory ? SC64_T4_TR_WON : SC64_T4_TR_LOST));
    assert(race_campaign.phase ==
           (victory ? SC64_RACE_PHASE_COMPLETE : SC64_RACE_PHASE_FAILED));
    assert(race_save_game());
    fixture_capture_save(&valid_record);
    Sc64T5RuntimeState expected_runtime = race_t5;
    Sc64RaceCampaignState expected_campaign = race_campaign;

    /* Reload from SRAM after clearing the live process state. */
    game_init();
    assert(screen == SCREEN_TITLE);
    assert(race_load_game());
    assert(screen == SCREEN_RESULT && won == !!victory && lost == !victory);
    assert(!memcmp(&expected_runtime, &race_t5, sizeof(expected_runtime)));
    assert(!memcmp(&expected_campaign, &race_campaign, sizeof(expected_campaign)));
}

static int progress_to_t10_wait(void)
{
    int kerrigan = -1, escort = -1;
    const Sc64T4Location *beacon = &sc64_t5_scenario.locations[0];
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive) {
        if (u[i].type == U_KERRIGAN) kerrigan = i;
        if (escort < 0 && u[i].owner == SIDE && u[i].type == U_MARINE) escort = i;
        if (u[i].type == U_SPIDER_MINE) u[i].alive = 0;
    }
    assert(kerrigan >= 0 && escort >= 0);
    /* Place a live player unit beside Kerrigan so the normal world-tick rescue
     * rule can convert her. This is an authored trigger/save fixture. */
    u[escort].x = u[kerrigan].x;
    u[escort].y = u[kerrigan].y;
    game_update(0, 0);
    assert(u[kerrigan].owner == SIDE);
    u[kerrigan].x = (int)(beacon->left + beacon->right) / 4;
    u[kerrigan].y = (int)(beacon->top + beacon->bottom) / 4;
    for (int i = 0; i < 2400; ++i) {
        game_update(0, 0);
        if (screen != SCREEN_PLAY || won || lost) return 0;
        for (int p = 0; p < 8; ++p) if (race_t5.player[p].fault) return 0;
        if ((race_t5.player[0].flags[10] & SC64_T4_TR_WAIT) &&
            race_t5.player[0].wait_ticks) return 1;
    }
    return 0;
}

static void trigger_wait_resume_roundtrip(void)
{
    start_t5();
    assert(progress_to_t10_wait());
    assert(race_t5.player[0].active_wait == 10);
    assert(race_t5.player[0].wait_ticks > 0);
    assert(race_save_game());
    fixture_capture_save(&valid_record);

    /* Compare uninterrupted continuation with SRAM expansion/resumption from
     * the exact same active Wait state, including AI and actor/world state. */
    for (int i = 0; i < 12; ++i) game_update(0, 0);
    remember_live();
    write_candidate_record(&valid_record);
    assert(race_load_game());
    for (int i = 0; i < 12; ++i) game_update(0, 0);
    assert(!memcmp(saved_units, u, sizeof(u)));
    assert(!memcmp(saved_buildings, b, sizeof(b)));
    assert(!memcmp(saved_minerals, m, sizeof(m)));
    assert(!memcmp(&saved_runtime, &race_t5, sizeof(race_t5)));
    assert(!memcmp(&saved_campaign, &race_campaign, sizeof(race_campaign)));
    assert(saved_frame == frame && saved_minerals_count == minerals &&
           saved_gas_count == gas);
}

static void mine_state_roundtrip(unsigned mine_state)
{
    start_t5();
    int mine = -1;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].type == U_SPIDER_MINE) { mine = i; break; }
    assert(mine >= 0);
    /* Focused save-schema fixture: establish the documented adapter state
     * directly, then verify that normal save/load preserves its continuation. */
    u[mine].carry = (int)mine_state;
    u[mine].work_timer = mine_state == SC64_T5_MINE_ARMING ? 31 : 0;
    u[mine].target = mine_state == SC64_T5_MINE_CHASING ? 3 : -1;
    for (int i = 0; i < 3; ++i) game_update(0, 0);
    assert(race_save_game());
    fixture_capture_save(&valid_record);
    remember_live();
    u[mine].carry = 0; u[mine].target = -1; u[mine].work_timer = 0;
    assert(race_load_game());
    assert(!memcmp(saved_units, u, sizeof(u)));
    assert(!memcmp(&saved_runtime, &race_t5, sizeof(race_t5)));
}

static void normal_pause_save_and_load(void)
{
    start_t5();
    for (int i = 0; i < 4; ++i) game_update(0, 0);
    game_update(KEY_START, KEY_START);
    assert(screen == SCREEN_PAUSE);
    game_update(0, 0);
    game_update(KEY_B, KEY_B); /* Pause menu save command. */
    assert(race_save_game()); /* Also assert the adapter can save this pause state. */
    fixture_capture_save(&valid_record);
    remember_live();
    game_update(0, 0);
    game_update(KEY_A, KEY_A); /* Resume through the ordinary pause menu. */
    assert(screen == SCREEN_PLAY);
    for (int i = 0; i < 3; ++i) game_update(0, 0);
    game_init();
    assert(screen == SCREEN_TITLE);
    game_update(KEY_L, KEY_L); /* Fresh process-style load from HOME. */
    assert(screen == SCREEN_PLAY && race_t5_active());
    assert(!memcmp(saved_units, u, sizeof(u)));
    assert(!memcmp(saved_buildings, b, sizeof(b)));
    assert(!memcmp(&saved_runtime, &race_t5, sizeof(race_t5)));
}

static void other_modes_clear_union(void)
{
    game_init(); race_mode = 0; race_selected_mission = 6; init_state(); /* T1 */
    assert(!race_t5_active() && race_save_game());
    for (unsigned j = 0; j < SC64_TEST_OPTIONAL_SAVE_BYTES; ++j)
        assert(((const u8 *)&race_data.t4)[j] == 0);

    game_init(); race_mode = 0; race_selected_mission = 7; init_state(); /* T4 */
    {
        assert(!race_t5_active());
        assert(race_save_game());
        assert(race_t4_active());
        assert(race_data.t4.active_mask == 0xdf);
    }
    game_init(); race_mode = 1; race_skirmish_map = 0; init_state();
    assert(!race_t5_active() && race_save_game());
    for (unsigned j = 0; j < SC64_TEST_OPTIONAL_SAVE_BYTES; ++j)
        assert(((const u8 *)&race_data.t5)[j] == 0);
}

int main(void)
{
#ifdef SC64_TERRAN_LATE_RUNTIME
    _Static_assert(SC64_RACE_SAVE_GENERATION == 18u,
                   "full late Terran saves require generation 18");
#elif defined(SC64_T6_STORAGE)
    _Static_assert(SC64_RACE_SAVE_GENERATION == 15u,
                   "expanded T5 storage uses its own save generation");
#else
    _Static_assert(SC64_RACE_SAVE_GENERATION == 14u,
                   "T5 persistence uses its own save generation");
#endif
    _Static_assert(sizeof(Sc64T5SaveState) <= 1124,
                   "T5 state must fit the compact scenario union");
    start_t5(); active_runtime_roundtrip(); malformed_records_are_atomic();
    inactive_context_must_be_canonical(); early_provenance_is_rejected();
    gated_grant_roundtrip();
    terminal_outcome_roundtrip(1); terminal_outcome_roundtrip(0);
    trigger_wait_resume_roundtrip();
    mine_state_roundtrip(SC64_T5_MINE_ARMING);
    mine_state_roundtrip(SC64_T5_MINE_CHASING);
    normal_pause_save_and_load(); other_modes_clear_union();
    puts("T5 compact save/load, atomic validation, mine continuation and mode clearing pass");
    return 0;
}
