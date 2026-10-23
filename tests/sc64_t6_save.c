/* T6 persistence validation fixture. Direct mutations below deliberately
 * exercise malformed snapshots; they are not mission-completion evidence. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

#ifndef SC64_T6_RUNTIME
#error "Compile with the internal T6 runtime profile"
#endif

static void init_t6_state(Sc64T6MissionState *state)
{
    memset(state, 0, sizeof(*state));
    assert(sc64_t6_scenario_runtime_init(&state->triggers));
    assert(race_t6_save_state_valid(state));
}

static void test_full_state_roundtrip_and_atomic_rejection(void)
{
    Sc64T6MissionState source, encoded, decoded, sentinel;
    init_t6_state(&source);
    source.deaths[1] = 3;
    source.deaths[4] = 9;
    source.rescue_mask = (u8)(1u << 2);
    sc64_t6_burrowed_set(&source, 159, 1);
    assert(race_t6_save_pack(&encoded, &source));
    memset(&decoded, 0xa5, sizeof(decoded));
    assert(race_t6_save_expand(&decoded, &encoded));
    assert(!memcmp(&source, &decoded, sizeof(source)));

    memset(&sentinel, 0x5a, sizeof(sentinel));
    decoded = sentinel;
    encoded.triggers.player[7].owner = 0;
    assert(!race_t6_save_expand(&decoded, &encoded));
    assert(!memcmp(&decoded, &sentinel, sizeof(decoded)));
}

static void test_bounds_and_canonical_fields(void)
{
    Sc64T6MissionState state;
    init_t6_state(&state);
    state.resources[3].current_gas = 65536u;
    assert(!race_t6_save_state_valid(&state));
    init_t6_state(&state);
    state.ai_requests[5] = 0x80u;
    assert(!race_t6_save_state_valid(&state));
    init_t6_state(&state);
    state.rescue_mask = 1u << 0;
    assert(!race_t6_save_state_valid(&state));
    init_t6_state(&state);
    state.deaths[2] = 1;
    /* A building-death ledger is allowed for any source slot and stores a
       saturating u32; it is not derivable from trigger actions. */
    assert(race_t6_save_state_valid(&state));
    init_t6_state(&state);
    state.ai_requests[0] = SC64_T6_AI_T6A;
    assert(!race_t6_save_state_valid(&state));
    init_t6_state(&state);
    state.resources[0].current_minerals = 75;
    assert(!race_t6_save_state_valid(&state));
    init_t6_state(&state);
    state.reserved[1] = 1;
    assert(!race_t6_save_state_valid(&state));
    init_t6_state(&state);
    state.message = 7;
    assert(!race_t6_save_state_valid(&state));
    state.message_ticks = 90;
    assert(race_t6_save_state_valid(&state));
    state.ping_x = 1;
    assert(!race_t6_save_state_valid(&state));
}

static void test_candidate_identity_clock_and_outcome(void)
{
    RaceSave candidate;
    memset(&candidate, 0, sizeof(candidate));
    candidate.state[0] = 0;
    candidate.state[2] = SC64_T6_GLOBAL_ID;
    candidate.state[3] = 0;
    candidate.state[7] = 0;
    candidate.campaign.mission = SC64_T6_GLOBAL_ID;
    candidate.campaign.phase = SC64_RACE_PHASE_ACTIVE;
    assert(sc64_t6_scenario_runtime_init(&candidate.t6.triggers));
    assert(race_t6_save_candidate_valid(&candidate, 96, 96));
    assert(!race_t6_save_candidate_valid(&candidate, 128, 128));

    candidate.t6.triggers.player[4].elapsed_ticks = 1;
    assert(!race_t6_save_candidate_valid(&candidate, 96, 96));
    candidate.t6.triggers.player[4].elapsed_ticks = 0;
    candidate.campaign.phase = SC64_RACE_PHASE_COMPLETE;
    assert(!race_t6_save_candidate_valid(&candidate, 96, 96));
}

static RaceSave good_sram;
static Unit live_units[MAX_UNITS];
static Building live_buildings[MAX_BUILDINGS];
static Mineral live_minerals[MAX_MINERALS];
static Sc64T6MissionState live_t6;
static Sc64RaceCampaignState live_campaign;
static u8 live_fog[(MAP_CELLS + 7) / 8];
static int live_frame, live_cx, live_cy;

static void t6_start_actual_game(void)
{
    game_init();
    race_mode = 0;
    race_selected_mission = SC64_T6_GLOBAL_ID;
    init_state();
    assert(race_t6_active() && screen == SCREEN_PLAY);
    assert(race_cols == 96 && race_rows == 96);
    assert(SC64_RACE_SAVE_GENERATION == 16u);
}

static void remember_live_world(void)
{
    memcpy(live_units, u, sizeof(u));
    memcpy(live_buildings, b, sizeof(b));
    memcpy(live_minerals, m, sizeof(m));
    live_t6 = race_t6;
    live_campaign = race_campaign;
    memcpy(live_fog, explored, sizeof(live_fog));
    live_frame = frame;
    live_cx = cx;
    live_cy = cy;
}

static void assert_live_world_unchanged(void)
{
    assert(!memcmp(live_units, u, sizeof(u)));
    assert(!memcmp(live_buildings, b, sizeof(b)));
    assert(!memcmp(live_minerals, m, sizeof(m)));
    assert(!memcmp(&live_t6, &race_t6, sizeof(race_t6)));
    assert(!memcmp(&live_campaign, &race_campaign, sizeof(race_campaign)));
    assert(!memcmp(live_fog, explored, sizeof(live_fog)));
    assert(frame == live_frame && cx == live_cx && cy == live_cy);
}

static void reject_sram_atomically(const RaceSave *bad, u32 version)
{
    remember_live_world();
    save_write(bad, sizeof(*bad), version);
    assert(!race_load_game());
    assert_live_world_unchanged();
    save_write(&good_sram, sizeof(good_sram), SAVE_VERSION);
}

static void test_actual_game_save_load_and_atomic_validation(void)
{
    t6_start_actual_game();
    for (int i = 0; i < 35; ++i) game_update(0, 0);
    assert(screen == SCREEN_PLAY && !won && !lost);
    assert(race_campaign.elapsed_frames == (u32)frame);
    for (unsigned p = 0; p < 8; ++p) {
        assert(!race_t6.triggers.player[p].fault);
        assert(race_t6.triggers.player[p].elapsed_ticks == (u32)frame);
    }
    assert(race_save_game());
    good_sram = race_data;
    assert(good_sram.version == SAVE_VERSION);
    assert(race_t6_save_candidate_valid(&good_sram, 96, 96));
    assert(sizeof(RaceSave) == 15644);
    for (unsigned i = sizeof(good_sram.t6); i < sizeof(good_sram.t4); ++i)
        assert(((const u8 *)&good_sram.t4)[i] == 0);

    remember_live_world();
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive) {
        u[i].hp = 1;
        break;
    }
    race_t6.triggers.player[0].elapsed_ticks++;
    ++frame;
    ++cx;
    assert(race_load_game());
    assert_live_world_unchanged();

    {
        RaceSave bad = good_sram;
        bad.t6.triggers.player[7].owner = 0;
        reject_sram_atomically(&bad, SAVE_VERSION);
    }
    reject_sram_atomically(&good_sram, SAVE_VERSION - 1u);
    assert(race_load_game());
    assert(frame == (int)good_sram.state[7]);
}

int main(void)
{
    test_full_state_roundtrip_and_atomic_rejection();
    test_bounds_and_canonical_fields();
    test_candidate_identity_clock_and_outcome();
    test_actual_game_save_load_and_atomic_validation();
    puts("T6 save adapter tests passed");
    return 0;
}
