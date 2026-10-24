/* P6 save payload and precommit validator smoke checks. The source world is
 * intentionally incomplete here, so an otherwise coherent P6 candidate must
 * fail its immutable UNIT/THG identity audit. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"
#include "../starcraft/race_save_adapter.h"

#ifndef SC64_P6_RUNTIME
#error "Compile this fixture with the gated P6 runtime flags"
#endif

static RaceSave candidate_with_valid_p6_header(void)
{
    RaceSave s;
    memset(&s, 0, sizeof(s));
    s.version = SAVE_VERSION;
    s.state[0] = 0;
    s.state[1] = R_PROTOSS;
    s.state[2] = SC64_P6_GLOBAL_ID;
    s.state[7] = 0;
    s.state[18] = 0;
    s.state[19] = 0;
    s.players[SIDE] = R_PROTOSS;
    s.players[RESCUABLE] = R_PROTOSS;
    s.players[ENEMY] = R_ZERG;
    s.players[4] = R_ZERG;
    s.players[NEUTRAL] = R_TER;
    s.campaign.mission = SC64_P6_GLOBAL_ID;
    s.campaign.phase = SC64_RACE_PHASE_ACTIVE;
    sc64_p6_runtime_init(&s.p6.runtime);
    return s;
}

static void test_union_and_candidate_identity(void)
{
    RaceSave s = candidate_with_valid_p6_header();
    assert(sizeof(Sc64P6PersistentState) == 148);
    assert(sizeof(RaceSave) <= 16368);
#if defined(SC64_P5_RUNTIME) && defined(SC64_P6_RUNTIME)
    assert((void *)&s.p5 == (void *)&s.p6);
#endif
    assert(sc64_p6_persistent_state_valid(&s.p6));
    assert(!race_p6_save_candidate_valid(&s, SC64_P6_MAP_COLS,
                                          SC64_P6_MAP_ROWS));
    s.p6.runtime.fault = SC64_P6_FAULT_STATE;
    assert(!race_p6_save_candidate_valid(&s, SC64_P6_MAP_COLS,
                                          SC64_P6_MAP_ROWS));
    s = candidate_with_valid_p6_header();
    s.p6.doodad_enabled[4] = 0x20;
    assert(!race_p6_save_candidate_valid(&s, SC64_P6_MAP_COLS,
                                          SC64_P6_MAP_ROWS));
}

static void test_invalid_record_does_not_commit(void)
{
    RaceSave bad;
    Unit units_before[MAX_UNITS];
    Building buildings_before[MAX_BUILDINGS];
    Sc64RaceCampaignState campaign_before = race_campaign;
    int frame_before = frame, selected_before = race_selected_mission;
    memset(&bad, 0, sizeof(bad));
    bad.version = SAVE_VERSION;
    bad.p6.runtime.fault = SC64_P6_FAULT_STATE;
    memcpy(units_before, u, sizeof(units_before));
    memcpy(buildings_before, b, sizeof(buildings_before));
    save_write(&bad, sizeof(bad), SAVE_VERSION);
    assert(!race_load_game());
    assert(frame == frame_before && race_selected_mission == selected_before);
    assert(!memcmp(&campaign_before, &race_campaign, sizeof(campaign_before)));
    assert(!memcmp(units_before, u, sizeof(units_before)));
    assert(!memcmp(buildings_before, b, sizeof(buildings_before)));
}

static void test_initialized_world_save_after_unit_motion(void)
{
    int actor = -1;
    int trap = -1;
    int start_x, start_y;

    game_init();
    race_mode = 0;
    race_selected_mission = SC64_P6_GLOBAL_ID;
    init_state();
    assert(screen == SCREEN_PLAY && race_p6_active());
    assert(race_p6_doodad_bindings_valid);
    if (!race_save_game()) {
        fprintf(stderr, "P6 initial save rejected: candidate=%d runtime=%d bindings=%d row=%d state=%d/%d/%d/%d campaign=%u/%u/%u players=%u,%u,%u,%u,%u\n",
                race_p6_save_candidate_valid(&race_data, SC64_P6_MAP_COLS,
                                              SC64_P6_MAP_ROWS),
                race_p6_runtime_state_valid(), race_p6_doodad_bindings_still_valid(),
                race_p6_source_row_valid(&sc64_race_maps[SC64_P6_GLOBAL_ID]),
                race_data.state[0], race_data.state[1], race_data.state[2], race_data.state[7],
                race_data.campaign.mission, race_data.campaign.phase,
                (unsigned)race_data.campaign.elapsed_frames,
                race_data.players[SIDE], race_data.players[RESCUABLE],
                race_data.players[ENEMY], race_data.players[4], race_data.players[NEUTRAL]);
        assert(0);
    }

    /* Exercise the ordinary order/movement tick on a real imported actor,
     * then ensure candidate validation does not mistake movement for loss of
     * the immutable source identity. */
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].raw_owner == 7 &&
            u[i].raw_type == 65) { actor = i; break; }
    assert(actor >= 0);
    start_x = u[actor].x;
    start_y = u[actor].y;
    u[actor].selected = 1;
    issue_order(ORDER_MOVE, start_x + 48, start_y + 32);
    for (int i = 0; i < 90; ++i) game_update(0, 0);
    assert(u[actor].alive);
    assert(u[actor].x != start_x || u[actor].y != start_y);

    /* Exercise a real trap death through hit_target. Overkill leaves signed
     * negative HP in the compact building record; save validation must accept
     * that authentic dead state rather than requiring normalized zero. */
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].raw_type == SC64_P6_RAW_WALL_MISSILE_TRAP) {
            trap = i; break;
        }
    assert(trap >= 0);
    hit_target(MAX_UNITS + trap, 1000, U_MARINE, SIDE);
    assert(!b[trap].alive && b[trap].hp < 0);
    assert(race_save_game());
    assert(race_p6_save_candidate_valid(&race_data, SC64_P6_MAP_COLS,
                                         SC64_P6_MAP_ROWS));
    assert(race_p5_transport_save_candidate_valid(&race_data, race_cols, race_rows));
    assert(race_save_invincible_padding_valid(&race_data));
    memset(u, 0, sizeof(u));
    if (!race_load_game()) {
        fprintf(stderr, "P6 load rejected after valid save: unit=%u campaign=%u p6=%u bind=%u\n",
                u[actor].alive, race_campaign.mission, race_p6.runtime.fault,
                race_p6_doodad_bindings_valid);
        assert(0);
    }
    assert(u[actor].alive && (u[actor].x != start_x || u[actor].y != start_y));
    assert(race_p6_runtime_state_valid());
}

int main(void)
{
    test_union_and_candidate_identity();
    test_invalid_record_does_not_commit();
    test_initialized_world_save_after_unit_motion();
    return 0;
}
