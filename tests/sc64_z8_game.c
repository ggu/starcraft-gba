/* Actual-engine Z8 campaign binding checks. The mission is launched through
 * the ordinary campaign menu, its authenticated trigger program runs on the
 * live pools, and an ordinary pause-menu save reloads into a fresh game that
 * then continues identically to the uninterrupted one. This is a host
 * fixture, not a mission win. */
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"
#include "sc64_save_candidate_fixture.h"

#ifndef SC64_Z8_RUNTIME
#error "Compile this fixture with the zerg-z8 profile flags"
#endif

static unsigned held_keys;
static RaceSave pause_save; /* the ordinary pause-menu record */

static void input(unsigned held)
{
    game_update((u16)held, (u16)(held & ~held_keys));
    held_keys = held;
}

static void press(unsigned key) { input(key); input(0); }

static void launch_z8_from_normal_menu(void)
{
    int ordinal;
    game_init();
    held_keys = 0;
    assert(race_front.page == SC_FRONT_HOME);
    ordinal = sc64_catalog_playable_ordinal(SC64_FACTION_ZERG, SC64_Z8_GLOBAL_ID);
    assert(ordinal == 7);
    assert(sc64_catalog_playable_count(SC64_FACTION_ZERG) == 8);
    press(KEY_A);
    assert(race_front.page == SC_FRONT_MISSION && race_front.faction == SC_FRONT_ZERG);
    for (int page = 0; page < ordinal / 2; ++page) press(KEY_RIGHT);
    if (ordinal & 1) press(KEY_DOWN);
    press(KEY_A);
    assert(race_front.page == SC_FRONT_BRIEFING && race_front.mission == SC64_Z8_GLOBAL_ID);
    press(KEY_START);
    assert(screen == SCREEN_PLAY && !race_mode && !won && !lost);
    assert(race_selected_mission == SC64_Z8_GLOBAL_ID && race_z8_active());
    assert(mission_unsupported_count == 0);
}

/* Order-sensitive digest of the simulation and scheduler. */
static u32 world_digest(void)
{
    u32 h = 2166136261u;
#define MIX(v) do { u32 x_ = (u32)(v); for (int k_ = 0; k_ < 4; ++k_) { h ^= (x_ >> (8 * k_)) & 255u; h *= 16777619u; } } while (0)
    for (int i = 0; i < MAX_UNITS; ++i) {
        MIX(u[i].alive);
        if (!u[i].alive) continue;
        MIX(u[i].x); MIX(u[i].y); MIX(u[i].hp); MIX(u[i].type); MIX(u[i].owner);
        MIX(u[i].order); MIX(u[i].target); MIX(u[i].raw_owner); MIX(u[i].raw_type);
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i) {
        MIX(b[i].alive);
        if (!b[i].alive) continue;
        MIX(b[i].hp); MIX(b[i].type); MIX(b[i].owner); MIX(b[i].queue_count);
        MIX(b[i].raw_owner);
    }
    MIX(frame); MIX(minerals); MIX(gas);
    for (unsigned i = 0; i < sizeof(race_z8_sched); ++i) MIX(((const u8 *)&race_z8_sched)[i]);
    for (unsigned i = 0; i < sizeof(race_z8_ai); ++i) MIX(((const u8 *)&race_z8_ai)[i]);
    for (unsigned t = 0; t < SC64_Z8_AI_TOWNS; ++t) { MIX(race_z8_ai_minerals[t]); MIX(race_z8_ai_gas[t]); }
#undef MIX
    return h;
}

static void test_binding(void)
{
    unsigned nydus = 0, kerrigan = 0, overlords = 0;
    assert(race_z8_sched.instance_count == 39u && !race_z8_sched.fault);
    assert(race_z8_sched.active_mask == 0x0eu);
    assert(race_z8_sched.force[1] == 2u && race_z8_sched.force[2] == 3u &&
           race_z8_sched.force[3] == 1u);
    assert(race_nydus.count == 2u);
    for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive && b[i].raw_type == 134)
        ++nydus;
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == SIDE) {
        assert(u[i].raw_owner == SC64_Z8_HUMAN_SLOT);
        kerrigan += u[i].raw_type == 51;
        overlords += u[i].raw_type == 42;
    }
    assert(nydus == 4u && kerrigan == 1u && overlords == 5u);
    for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive && b[i].raw_type == 194)
        assert(b[i].owner == NEUTRAL && b[i].raw_owner == 5);
    assert(race_z8_live_world_valid());
    assert(race_z8_start_nexus == 2u);
}

/* The first passes run T2 (human resources), T8 and T9 (AI wallets, towns). */
static void test_opening_triggers(void)
{
    for (int i = 0; i < 150; ++i) input(0);
    assert(!race_z8_sched.fault && race_z8_outcome() == SC64_Z8_OUTCOME_NONE);
    assert(minerals == 350 && gas == 350);
    assert(race_z8_ai_minerals[0] >= 7000u && race_z8_ai_minerals[1] >= 7000u);
    assert(race_z8_ai.started == 3u);
    assert(sc64_z8_switch_get(&race_z8_sched, 0));
}

static void test_save_continuation(void)
{
    u32 reference[6], resumed[6];
    static RaceSave saved;
    unsigned saved_frame;
    int reference_lost;
    for (int i = 0; i < 2000; ++i) input(0);
    assert(!race_z8_sched.fault && !won && !lost && screen == SCREEN_PLAY);
    press(KEY_START);
    assert(screen == SCREEN_PAUSE);
    press(KEY_B);
#ifndef SC64_IMPACT_EVENTS
    assert(save_read(&race_data, sizeof(race_data), SAVE_VERSION));
#endif
    fixture_capture_save(&saved);
    pause_save = saved;
    assert(race_z8_save_candidate_world_valid(&saved, race_cols, race_rows));
    assert(race_z8_ai.produce_index[0] && race_z8_ai.produce_index[1]);
    saved_frame = (unsigned)frame;
    press(KEY_START);
    assert(screen == SCREEN_PLAY);
    for (int k = 0; k < 6; ++k) {
        for (int i = 0; i < 500; ++i) input(0);
        reference[k] = world_digest();
    }
    /* An idle human loses to the first Dark Templar raid during the
     * reference run, and that defeat autosaves. Reload the pause save. */
    reference_lost = lost;
    write_candidate_record(&saved);
    game_init();
    held_keys = 0;
    press(KEY_L);
    assert(screen == SCREEN_PLAY && race_z8_active() && (unsigned)frame == saved_frame + 1u);
    assert(race_nydus.count == 2u && race_z8_map && !race_z8_sched.fault);
    for (int k = 0; k < 6; ++k) {
        for (int i = 0; i < 500; ++i) input(0);
        resumed[k] = world_digest();
        assert(resumed[k] == reference[k]);
    }
    assert(lost == reference_lost);
    printf("Z8 save continuation: frame %u, 3000 updates identical, digest %08x, idle outcome %s\n",
           saved_frame, (unsigned)resumed[5], lost ? "defeat" : "running");
}

static void test_rejects_corrupt_candidate(void)
{
    static RaceSave good, bad;
    u32 before;
    good = pause_save;
    before = world_digest();
    bad = good;
    bad.z8.instance[0] = 0xffu; /* invalid flags */
    write_candidate_record(&bad);
    assert(!load_game() && world_digest() == before);
    bad = good;
    bad.z8.reserved = 1;
    write_candidate_record(&bad);
    assert(!load_game() && world_digest() == before);
    bad = good;
    bad.z8.ai[offsetof(Sc64Z8AiState, reserved)] = 1;
    write_candidate_record(&bad);
    assert(!load_game() && world_digest() == before);
    write_candidate_record(&good);
}

/* Each record is serialized through the production envelope and read back
 * before the selected Z8 world validator and atomic load are checked. */
static void assert_bad_z8_world(RaceSave *bad, u32 before)
{
    write_candidate_record(bad);
    assert(!race_z8_save_candidate_world_valid(&race_data, race_cols, race_rows));
    assert(!load_game() && world_digest() == before);
}

static int encode_z8_candidate_unit(RaceSave *save, unsigned slot,
                                    unsigned ww, unsigned wh, const Unit *unit)
{
#ifdef SC64_P7_TYPED_COMPACT_ENABLED
    return sc64_p7_typed_unit_encode(&save->units[slot], unit, ww, wh);
#elif defined(SC64_Z7_DERIVED_SAVE)
    return sc64_derived_unit_encode(&save->units[slot], unit, ww, wh);
#else
    return sc64_zp_unit_save_encode(&save->units[slot], unit, ww, wh);
#endif
}

static int encode_z8_candidate_building(RaceSave *save, unsigned slot,
                                        unsigned ww, unsigned wh, const Building *building)
{
#ifdef SC64_P7_TYPED_COMPACT_ENABLED
    return sc64_p7_typed_building_encode(&save->buildings[slot], building, ww, wh);
#elif defined(SC64_Z7_DERIVED_SAVE)
    return sc64_derived_building_encode(&save->buildings[slot], building, ww, wh);
#else
    return sc64_building_save_encode(&save->buildings[slot], building, ww, wh);
#endif
}

static void test_rejects_invalid_source_world(void)
{
    static RaceSave bad;
    Unit actor; Building building;
    int unit = -1, structure = -1;
    const unsigned ww = (unsigned)race_cols * 16u, wh = (unsigned)race_rows * 16u;
    u32 before = world_digest();
    for (unsigned i=0; i<MAX_UNITS; ++i) {
        assert(race_save_decode_unit(&pause_save, i, ww, wh, &actor));
        if (actor.alive && actor.owner == SIDE && actor.raw_owner == SC64_Z8_HUMAN_SLOT) {unit=(int)i;break;}
    }
    for (unsigned i=0; i<MAX_BUILDINGS; ++i) {
        assert(race_save_decode_building(&pause_save, i, &building));
        if (building.alive && building.owner == SIDE && building.raw_owner == SC64_Z8_HUMAN_SLOT) {structure=(int)i;break;}
    }
    assert(unit>=0 && structure>=0);
    assert(race_save_decode_unit(&pause_save, (unsigned)unit, ww, wh, &actor));
    bad=pause_save; actor.raw_owner=1;
    assert(encode_z8_candidate_unit(&bad, (unsigned)unit, ww, wh, &actor));
    assert_bad_z8_world(&bad,before);
    assert(race_save_decode_unit(&pause_save, (unsigned)unit, ww, wh, &actor));
    bad=pause_save; actor.raw_type = sc64_race_raw_unit(actor.type)==0 ? 7 : 0;
    assert(encode_z8_candidate_unit(&bad, (unsigned)unit, ww, wh, &actor));
    assert_bad_z8_world(&bad,before);
    assert(race_save_decode_building(&pause_save, (unsigned)structure, &building));
    bad=pause_save; building.raw_owner=1;
    assert(encode_z8_candidate_building(&bad, (unsigned)structure, ww, wh, &building));
    assert_bad_z8_world(&bad,before);
    assert(race_save_decode_building(&pause_save, (unsigned)structure, &building));
    bad=pause_save; building.raw_type=254;
    assert(encode_z8_candidate_building(&bad, (unsigned)structure, ww, wh, &building));
    assert_bad_z8_world(&bad,before);
    write_candidate_record(&pause_save);
}

/* Fixture-only placement (not gameplay evidence): a human Overlord in
 * range reveals a hero Dark Templar to targeting and drawing. */
static void test_dark_templar_cloak(void)
{
    int dt = -1, overlord = -1, hydra = -1;
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive) {
        if (u[i].raw_type == 74 && u[i].owner == ENEMY && dt < 0) dt = i;
        if (u[i].raw_type == 42 && u[i].owner == SIDE && overlord < 0) overlord = i;
        if (u[i].raw_type == 38 && u[i].owner == SIDE && hydra < 0) hydra = i;
    }
    assert(dt >= 0 && overlord >= 0 && hydra >= 0);
    race_z8_update_cloak();
    if (dist2(u[overlord].x, u[overlord].y, u[dt].x, u[dt].y) >
        race_z8_overlord_detect_px() * race_z8_overlord_detect_px()) {
        assert(race_z8_unit_hidden_from_side(dt));
        assert(!SC64_CAN_ATTACK_FROM(u[hydra].type, dt, SIDE));
    }
    u[overlord].x = u[dt].x + race_z8_overlord_detect_px() - 1;
    u[overlord].y = u[dt].y;
    race_z8_update_cloak();
    assert(!race_z8_unit_hidden_from_side(dt));
    assert(SC64_CAN_ATTACK_FROM(u[hydra].type, dt, SIDE));
    u[overlord].x = u[dt].x + race_z8_overlord_detect_px() + 1;
    race_z8_update_cloak();
    assert(race_z8_unit_hidden_from_side(dt));
    puts("Z8 hero Dark Templar cloak: shared source-fact Overlord range");
}

int main(void)
{
    assert(SC64_RACE_SAVE_GENERATION == 46u);
    assert(sizeof(RaceSave) <= 16368u);
    launch_z8_from_normal_menu();
    test_binding();
    test_opening_triggers();
    test_save_continuation();
    test_rejects_corrupt_candidate();
    test_rejects_invalid_source_world();
    /* Return to the pause save and await the authored Dark Templar. */
    write_candidate_record(&pause_save);
    game_init();
    held_keys = 0;
    press(KEY_L);
    assert(screen == SCREEN_PLAY && race_z8_active());
    for (int tick = 0; tick < 2000; ++tick) {
        int found = 0;
        for (int i = 0; i < MAX_UNITS; ++i)
            if (u[i].alive && u[i].raw_type == 74 && u[i].owner == ENEMY) found = 1;
        if (found) break;
        assert(screen == SCREEN_PLAY);
        input(0);
    }
    test_dark_templar_cloak();
    printf("Z8 campaign binding: %u instances, %u Nydus links, RaceSave %u bytes, generation %u\n",
           (unsigned)race_z8_sched.instance_count, (unsigned)race_nydus.count,
           (unsigned)sizeof(RaceSave), (unsigned)SC64_RACE_SAVE_GENERATION);
    return 0;
}
