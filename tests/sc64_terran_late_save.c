/* Generation-18 late Terran save fixture. Direct state edits below are
 * corruption/serialization fixtures, not gameplay or mission-completion proof. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"
#include "sc64_save_candidate_fixture.h"

#ifndef SC64_TERRAN_LATE_RUNTIME
#error "Compile with the generation-18 late Terran runtime profile"
#endif

static RaceSave valid_record;
static Unit live_units[MAX_UNITS];
static Building live_buildings[MAX_BUILDINGS];
static Sc64TerranLateMissionState live_late;
static Sc64RaceCampaignState live_campaign;
static int live_frame, live_mission, live_cx, live_cy;

static void start_late_mission(int mission)
{
    game_init();
    race_mode = 0;
    race_selected_mission = mission;
    init_state();
    assert(race_late_active());
    assert(race_cols == sc64_race_maps[mission].width);
    assert(race_rows == sc64_race_maps[mission].height);
    assert(SC64_RACE_SAVE_GENERATION == 18u);
    assert(screen == SCREEN_PLAY);
    assert(race_campaign.phase == SC64_RACE_PHASE_ACTIVE);
    assert(race_campaign.elapsed_frames == 0 && frame == 0);
    for (unsigned p = 0; p < 8; ++p) {
        assert(race_late.triggers.player[p].elapsed_ticks == 0);
        assert(!race_late.triggers.player[p].fault);
    }
}

static void remember_live(void)
{
    memcpy(live_units, u, sizeof(u));
    memcpy(live_buildings, b, sizeof(b));
    live_late = race_late;
    live_campaign = race_campaign;
    live_frame = frame;
    live_mission = race_selected_mission;
    live_cx = cx;
    live_cy = cy;
}

static void assert_live_unchanged(void)
{
    assert(!memcmp(live_units, u, sizeof(u)));
    assert(!memcmp(live_buildings, b, sizeof(b)));
    assert(!memcmp(&live_late, &race_late, sizeof(race_late)));
    assert(!memcmp(&live_campaign, &race_campaign, sizeof(race_campaign)));
    assert(frame == live_frame && race_selected_mission == live_mission);
    assert(cx == live_cx && cy == live_cy);
}

static void reject_atomically(const RaceSave *bad)
{
    remember_live();
    write_candidate_record(bad);
    assert(!race_terran_late_save_candidate_valid(&race_data, race_cols, race_rows));
    assert(!race_load_game());
    assert_live_unchanged();
    write_candidate_record(&valid_record);
}

static void test_all_map_geometries_roundtrip(void)
{
    static const int missions[4] = {
        SC64_TERRAN_LATE_GLOBAL_T7, SC64_TERRAN_LATE_GLOBAL_T8,
        SC64_TERRAN_LATE_GLOBAL_T9, SC64_TERRAN_LATE_GLOBAL_T10
    };
    for (unsigned n = 0; n < 4; ++n) {
        start_late_mission(missions[n]);
        assert(race_campaign.elapsed_frames == (u32)frame);
        for (unsigned p = 0; p < 8; ++p) {
            assert(race_late.triggers.player[p].elapsed_ticks == (u32)frame);
            assert(!race_late.triggers.player[p].fault);
        }
        assert(race_save_game());
        fixture_capture_save(&valid_record);
        assert(valid_record.version == SAVE_VERSION);
        assert(race_terran_late_save_candidate_valid(
            &valid_record, race_cols, race_rows));

        /* Faction IDs are derived from this mission's source setup during
         * init. A checksum-valid save with any corrupted runtime owner slot
         * must be rejected before it can mutate the live mission. */
        for (unsigned owner = 0; owner < 5; ++owner) {
            RaceSave bad = valid_record;
            bad.players[owner] = (bad.players[owner] + 1) % 3;
            assert(!race_terran_late_save_candidate_valid(
                &bad, race_cols, race_rows));
            reject_atomically(&bad);
        }

        /* Loading must restore the exact trigger clock and mission payload. */
        race_late.triggers.player[0].elapsed_ticks++;
        ++frame;
        ++cx;
        assert(race_load_game());
        assert(frame == valid_record.state[7]);
        assert(!memcmp(&race_late, &valid_record.terran_late,
                       sizeof(race_late)));
        assert(race_campaign.elapsed_frames == (u32)frame);

        /* Wrong source identity and stale union-tail bytes fail atomically. */
        {
            RaceSave bad = valid_record;
            bad.terran_late.triggers.player[7].owner = 0;
            assert(!race_terran_late_save_candidate_valid(
                &bad, race_cols, race_rows));
            reject_atomically(&bad);
        }
        {
            RaceSave bad = valid_record;
            ((u8 *)&bad.terran_late)[sizeof(bad.terran_late) - 1] ^= 0x80;
            /* The final byte is reserved and must remain canonical zero. */
            assert(!race_terran_late_save_candidate_valid(
                &bad, race_cols, race_rows));
            reject_atomically(&bad);
        }
    }
}

static void test_t7_emitter_and_mutable_region(void)
{
    start_late_mission(SC64_TERRAN_LATE_GLOBAL_T7);
    /* The source creates its raw-217 item through TRIG after mission start;
     * before that action, the sentinel is the valid saved state. */
    assert(race_late.emitter_unit == SC64_TERRAN_LATE_NO_SLOT);
    assert(race_save_game());
    fixture_capture_save(&valid_record);
    assert(race_terran_late_save_candidate_valid(&valid_record, 128, 96));

    /* Direct creation fixture isolates post-create/link persistence. */
    int fixture_carrier = -1;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].type == U_SCV) { fixture_carrier = i; break; }
    assert(fixture_carrier >= 0);
    int item_slot = spawn_unit(SIDE, U_PSI_EMITTER,
                               u[fixture_carrier].x, u[fixture_carrier].y);
    assert(item_slot >= 0);
    u[item_slot].raw_type = 217;
    u[item_slot].raw_owner = sc64_terran_late_source_slot_for_controller(
        &sc64_t7_source_setup, SC64_TERRAN_LATE_SOURCE_HUMAN_CONTROLLER);
    u[item_slot].selected = 0;
    u[item_slot].order = ORDER_IDLE;
    u[item_slot].target = -1;
    invincible_units[item_slot] = 1;
    race_late.emitter_unit = (u16)item_slot;
    assert(race_late.emitter_unit < MAX_UNITS);
    Unit *item = &u[race_late.emitter_unit];
    assert(item->alive && item->raw_type == 217 && item->owner == SIDE);

    /* Direct fixture: a carried emitter is linked to one valid live SIDE unit. */
    int carrier = -1;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (i != race_late.emitter_unit && u[i].alive && u[i].owner == SIDE && u[i].type == U_SCV) {
            carrier = i;
            break;
        }
    assert(carrier >= 0);
    item->raw_owner = sc64_terran_late_source_slot_for_controller(
        &sc64_t7_source_setup, SC64_TERRAN_LATE_SOURCE_HUMAN_CONTROLLER);
    item->selected = 0;
    item->order = ORDER_IDLE;
    item->target = -1;
    item->x = u[carrier].x;
    item->y = u[carrier].y;
    race_late.emitter_carrier = (u16)carrier;

    /* The source MoveLocation targets MRGN4 (location field 4, destination
     * argument 4), preserving its rectangle dimensions. This direct fixture
     * exercises a translated but same-sized rectangle, not gameplay. */
    const Sc64T4Location *original = &sc64_t7_scenario.locations[3];
    assert(original->right > original->left &&
           original->bottom > original->top);
    race_late.mutable_location_id = 4;
    race_late.mutable_location[0] = original->left + 32;
    race_late.mutable_location[1] = original->top;
    race_late.mutable_location[2] = original->right + 32;
    race_late.mutable_location[3] = original->bottom;
    assert(race_save_game());
    fixture_capture_save(&valid_record);
    assert(race_terran_late_save_candidate_valid(&valid_record, 128, 96));

    {
        RaceSave bad = valid_record;
        bad.terran_late.mutable_location[2]++;
        reject_atomically(&bad);
    }
    {
        RaceSave bad = valid_record;
        bad.terran_late.emitter_carrier = bad.terran_late.emitter_unit;
        reject_atomically(&bad);
    }
    assert(race_load_game());
    assert(race_late.emitter_carrier == (u16)carrier);
}

static void test_wait_state_survives_save(void)
{
    start_late_mission(SC64_TERRAN_LATE_GLOBAL_T8);
    int human = sc64_terran_late_source_slot_for_controller(
        &sc64_t8_source_setup, SC64_TERRAN_LATE_SOURCE_HUMAN_CONTROLLER);
    int found = 0;
    for (unsigned t = 0; t < sc64_t8_scenario.trigger_count && !found; ++t) {
        const Sc64T4Trigger *trigger = &sc64_t8_scenario.triggers[t];
        for (unsigned a = 0; a < 64; ++a) {
            if (trigger->actions[a].opcode == 0) break;
            if (trigger->actions[a].opcode != 4 ||
                (trigger->actions[a].flags & 2) ||
                !sc64_terran_late_enabled(trigger, (unsigned)human,
                    race_late.triggers.player[human].force)) continue;
            race_late.triggers.player[human].flags[t] =
                SC64_T4_TR_RUNNING | SC64_T4_TR_WAIT;
            race_late.triggers.player[human].cursor[t] = (u8)a;
            race_late.triggers.player[human].active_wait = (u8)t;
            race_late.triggers.player[human].wait_ticks = 1;
            found = 1;
            break;
        }
    }
    /* This directly constructed wait state tests save encoding only; it does
     * not claim the source mission reaches that state during gameplay. */
    assert(found);
    assert(race_save_game());
    fixture_capture_save(&valid_record);
    assert(race_terran_late_save_candidate_valid(&valid_record,
                                                 race_cols, race_rows));
    u16 saved_wait = 0;
    unsigned saved_owner = 0;
    for (unsigned p = 0; p < 8; ++p)
        if (valid_record.terran_late.triggers.player[p].wait_ticks) {
            saved_wait = valid_record.terran_late.triggers.player[p].wait_ticks;
            saved_owner = p;
            break;
        }
    assert(saved_wait);
    race_late.triggers.player[saved_owner].wait_ticks++;
    ++frame;
    assert(race_load_game());
    assert(race_late.triggers.player[saved_owner].wait_ticks == saved_wait);
}

static void test_flight_goal_roundtrip_and_candidate_checks(void)
{
    start_late_mission(SC64_TERRAN_LATE_GLOBAL_T8);
    int slot = -1;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == SIDE && !b[i].under_construction &&
            (b[i].flags & 4) && race_late_can_lift(b[i].type)) {
            slot = i;
            break;
        }
    assert(slot >= 0);
    int x = b[slot].x + 32, y = b[slot].y + 24;
    assert(race_late_airborne(slot));
    assert(race_late_flight_order(slot, x, y, 0));
    assert(race_late.flight_goals[slot] & SC64_FLIGHT_ACTIVE);
    assert(race_save_game());
    fixture_capture_save(&valid_record);
    assert(race_terran_late_save_candidate_valid(&valid_record, race_cols, race_rows));

    /* Save/load preserves the in-flight command and its exact destination. */
    u32 saved_goal = race_late.flight_goals[slot];
    race_late.flight_goals[slot] = 0;
    assert(race_load_game());
    assert(race_late.flight_goals[slot] == saved_goal);

    /* A goal cannot survive after its building is grounded. */
    {
        RaceSave bad = valid_record;
        Building grounded;
        assert(race_save_decode_building(&bad, (unsigned)slot, &grounded));
        grounded.flags &= ~4;
        assert(sc64_building_save_encode(&bad.buildings[slot], &grounded,
                                         (unsigned)race_cols * 16u,
                                         (unsigned)race_rows * 16u));
        assert(!race_terran_late_save_candidate_valid(&bad, race_cols, race_rows));
        reject_atomically(&bad);
    }
    /* An active goal at an empty building slot is also rejected. */
    {
        int empty = -1;
        for (int i = 0; i < MAX_BUILDINGS; ++i)
            if (!b[i].alive) { empty = i; break; }
        assert(empty >= 0);
        RaceSave bad = valid_record;
        bad.terran_late.flight_goals[empty] = SC64_FLIGHT_ACTIVE | 16u | (16u << 11);
        assert(!race_terran_late_save_candidate_valid(&bad, race_cols, race_rows));
        reject_atomically(&bad);
    }
}

int main(void)
{
    test_all_map_geometries_roundtrip();
    test_t7_emitter_and_mutable_region();
    test_wait_state_survives_save();
    test_flight_goal_roundtrip_and_candidate_checks();
    puts("late Terran generation-18 save tests passed");
    return 0;
}
