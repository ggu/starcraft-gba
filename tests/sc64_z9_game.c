/* Actual-engine Z9 campaign binding checks. The mission is launched through
 * the ordinary campaign menu, its authenticated trigger program runs on the
 * live pools, and ordinary pause-menu saves reload into a fresh game that
 * then continues identically to the uninterrupted one. The Drone and crystal
 * moves below are fixture-only state writes standing in for a controller;
 * this is a host fixture, not a mission win. */
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"
#include "sc64_save_candidate_fixture.h"

#ifndef SC64_Z9_RUNTIME
#error "Compile this fixture with the zerg-z9 profile flags"
#endif

static unsigned held_keys;
static RaceSave pause_save; /* the ordinary pause-menu record */
static unsigned pause_frame;

static void input(unsigned held)
{
    game_update((u16)held, (u16)(held & ~held_keys));
    held_keys = held;
}

static void press(unsigned key) { input(key); input(0); }

static void launch_z9_from_normal_menu(void)
{
    int ordinal;
    game_init();
    held_keys = 0;
    assert(race_front.page == SC_FRONT_HOME);
    ordinal = sc64_catalog_playable_ordinal(SC64_FACTION_ZERG, SC64_Z9_GLOBAL_ID);
    assert(ordinal == 8);
#ifdef SC64_Z10_RUNTIME
    assert(sc64_catalog_playable_count(SC64_FACTION_ZERG) == 10);
#else
    assert(sc64_catalog_playable_count(SC64_FACTION_ZERG) == 9);
#endif
    press(KEY_A);
    assert(race_front.page == SC_FRONT_MISSION && race_front.faction == SC_FRONT_ZERG);
    for (int page = 0; page < ordinal / 2; ++page) press(KEY_RIGHT);
    if (ordinal & 1) press(KEY_DOWN);
    press(KEY_A);
    assert(race_front.page == SC_FRONT_BRIEFING && race_front.mission == SC64_Z9_GLOBAL_ID);
    press(KEY_START);
    assert(screen == SCREEN_PLAY && !race_mode && !won && !lost);
    assert(race_selected_mission == SC64_Z9_GLOBAL_ID && race_zl_active());
    assert(mission_unsupported_count == 0);
}

/* Order-sensitive digest of the simulation, scheduler, AI and crystal. */
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
    MIX(frame); MIX(minerals); MIX(gas); MIX(race_zl_crystal_carrier);
    for (unsigned i = 0; i < sizeof(race_zl_sched); ++i) MIX(((const u8 *)&race_zl_sched)[i]);
    for (unsigned i = 0; i < sizeof(race_zl_ai); ++i) MIX(((const u8 *)&race_zl_ai)[i]);
    for (unsigned t = 0; t < SC64_ZL_AI_TOWNS; ++t) { MIX(race_zl_ai_minerals[t]); MIX(race_zl_ai_gas[t]); }
#undef MIX
    return h;
}

static int find_unit_raw(int owner, int raw)
{
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == owner && u[i].raw_type == raw) return i;
    return -1;
}

static int count_unit_raw(int owner, int raw)
{
    int n = 0;
    for (int i = 0; i < MAX_UNITS; ++i)
        n += u[i].alive && u[i].owner == owner && u[i].raw_type == raw;
    return n;
}

/* Centre of a source location in engine pixels. */
static void location_centre(unsigned location, int *x, int *y)
{
    const Sc64Z8Rect *r = &race_zl_sched.core.locations[location - 1];
    *x = ((int)r->left + r->right) / 4;
    *y = ((int)r->top + r->bottom) / 4;
}

static void test_binding(void)
{
    unsigned formation = 0, nexus[8] = {0};
    const u32 *scripts = race_z9_program.ai_scripts;
    assert(race_zl_sched.core.instance_count == 14u && !race_zl_sched.core.fault);
    assert(race_zl_sched.core.active_mask == 0x9cu);
    assert(race_zl_sched.core.force[2] == 4u && race_zl_sched.core.force[3] == 1u &&
           race_zl_sched.core.force[4] == 3u && race_zl_sched.core.force[7] == 2u);
    /* The authored FourCC constants match the authenticated action rows. */
    for (unsigned i = 0; i < SC64_Z9_PROGRAM_ACTION_COUNT; ++i)
        if (sc64_z9_program_actions[i].opcode == 16u)
            assert(sc64_zl_ai_script_known(&race_z9_program, sc64_z9_program_actions[i].group2));
    for (unsigned i = 0; i < race_z9_program.ai_script_count; ++i) {
        int used = 0;
        for (unsigned a = 0; a < SC64_Z9_PROGRAM_ACTION_COUNT; ++a)
            used |= sc64_z9_program_actions[a].opcode == 16u &&
                    sc64_z9_program_actions[a].group2 == scripts[i];
        assert(used);
    }
    assert(count_unit_raw(SIDE, 37) == 6 && count_unit_raw(SIDE, 38) == 2 &&
           count_unit_raw(SIDE, 41) == 4 && count_unit_raw(SIDE, 42) == 3);
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == SIDE)
        assert(u[i].raw_owner == SC64_ZL_HUMAN_SLOT);
    for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive) {
        if (b[i].raw_type == 173) {
            ++formation;
            assert(b[i].owner == NEUTRAL && b[i].raw_owner == 6 && b[i].type == B_ZL_KHAYDARIN_FORMATION);
            assert(invincible_buildings[i]);
        }
        if (b[i].raw_type == 154) {
            assert(b[i].owner == ENEMY && b[i].raw_owner >= 0 && b[i].raw_owner < 8);
            ++nexus[b[i].raw_owner];
        }
    }
    assert(formation == 1u && nexus[2] == 1u && nexus[4] == 2u && nexus[7] == 2u);
    assert(race_zl_live_world_valid());
}

/* The first passes run T2 (human 750/750), T5 (slot 2's wallet and Zer9),
 * T8 and T9 (slot 7 and slot 4 wallets). Only slot 2's town starts. */
static void test_opening_triggers(void)
{
    for (int i = 0; i < 150; ++i) input(0);
    assert(!race_zl_sched.core.fault && race_zl_outcome() == SC64_Z8_OUTCOME_NONE);
    assert(minerals == 750 && gas == 750);
    assert(race_zl_ai.started == 1u);
    assert(race_zl_ai_minerals[0] <= 5000u && race_zl_ai_minerals[0] >= 4000u);
    assert(race_zl_ai_minerals[1] == 8000u && race_zl_ai_minerals[2] == 10000u);
    assert(!sc64_z8_switch_get(&race_zl_sched.core, 0) && race_zl_sched.countdown == 0u);
}

/* Fixture-only placement: a human Drone enters location 2 at the crystal
 * formation. T3 removes it, runs its presentation and sets a 600-second
 * countdown and switch 0. */
static void test_drone_enters_formation(void)
{
    int drone = find_unit_raw(SIDE, 41), x, y;
    int drones = count_unit_raw(SIDE, 41);
    assert(drone >= 0);
    location_centre(2, &x, &y);
    u[drone].x = (s16)x; u[drone].y = (s16)(y + 40);
    u[drone].order = ORDER_IDLE;
    for (int i = 0; i < 400 && !sc64_z8_switch_get(&race_zl_sched.core, 0); ++i) input(0);
    assert(!race_zl_sched.core.fault && sc64_z8_switch_get(&race_zl_sched.core, 0));
    assert(count_unit_raw(SIDE, 41) == drones - 1);
    assert(race_zl_sched.countdown > 590u && race_zl_sched.countdown <= 600u);
    printf("Z9 formation: Drone removed, countdown %u s\n", (unsigned)race_zl_sched.countdown);
}

static void save_from_pause(RaceSave *out, unsigned *saved_frame)
{
    press(KEY_START);
    assert(screen == SCREEN_PAUSE);
    press(KEY_B);
#ifndef SC64_IMPACT_EVENTS
    assert(save_read(&race_data, sizeof(race_data), SAVE_VERSION));
#endif
    fixture_capture_save(out);
    *saved_frame = (unsigned)frame;
    press(KEY_START);
    assert(screen == SCREEN_PLAY);
}

static void reload(const RaceSave *saved, unsigned saved_frame)
{
    write_candidate_record(saved);
    game_init();
    held_keys = 0;
    press(KEY_L);
    assert(screen == SCREEN_PLAY && race_zl_active() && (unsigned)frame == saved_frame + 1u);
    assert(race_zl_map && !race_zl_sched.core.fault);
}

static void check_continuation(const char *label, unsigned blocks)
{
    u32 reference[8], resumed[8];
    static RaceSave saved;
    unsigned saved_frame;
    int reference_won, reference_lost;
    assert(blocks <= 8);
    assert(!race_zl_sched.core.fault && !won && !lost && screen == SCREEN_PLAY);
    save_from_pause(&saved, &saved_frame);
    pause_save = saved;
    pause_frame = saved_frame;
    {
        int saved_mission = race_selected_mission;
        race_selected_mission = SC64_Z9_GLOBAL_ID;
        assert(race_zl_save_candidate_world_valid(&saved, race_cols, race_rows));
        race_selected_mission = saved_mission;
    }
    for (unsigned k = 0; k < blocks; ++k) {
        for (int i = 0; i < 500; ++i) input(0);
        reference[k] = world_digest();
    }
    reference_won = won; reference_lost = lost;
    reload(&saved, saved_frame);
    for (unsigned k = 0; k < blocks; ++k) {
        for (int i = 0; i < 500; ++i) input(0);
        resumed[k] = world_digest();
        assert(resumed[k] == reference[k]);
    }
    assert(won == reference_won && lost == reference_lost);
    printf("Z9 save continuation (%s): frame %u, %u updates identical, digest %08x\n",
           label, saved_frame, blocks * 500u, (unsigned)resumed[blocks - 1]);
}

static void test_rejects_corrupt_candidate(void)
{
    static RaceSave good, bad;
    u32 before;
    good = pause_save;
    before = world_digest();
    bad = good;
    bad.zl.instance[0] = 0xffu; /* invalid flags */
    write_candidate_record(&bad);
    assert(!load_game() && world_digest() == before);
    bad = good;
    ((u8 *)&bad.z5)[sizeof(Sc64ZlPersistentState)] = 1; /* non-canonical tail */
    write_candidate_record(&bad);
    assert(!load_game() && world_digest() == before);
    bad = good;
    bad.zl.ai[offsetof(Sc64ZlAiState, reserved)] = 1;
    write_candidate_record(&bad);
    assert(!load_game() && world_digest() == before);
    bad = good;
    bad.zl.countdown = SC64_ZL_COUNTDOWN_LIMIT + 1u;
    write_candidate_record(&bad);
    assert(!load_game() && world_digest() == before);
    bad = good;
    bad.zl.crystal_carrier = MAX_UNITS;
    write_candidate_record(&bad);
    assert(!load_game() && world_digest() == before);
    write_candidate_record(&good);
}

static int encode_candidate_unit(RaceSave *save, unsigned slot,
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

static void test_rejects_invalid_source_world(void)
{
    static RaceSave bad;
    Unit actor;
    int unit = -1;
    const unsigned ww = (unsigned)race_cols * 16u, wh = (unsigned)race_rows * 16u;
    u32 before = world_digest();
    for (unsigned i = 0; i < MAX_UNITS; ++i) {
        assert(race_save_decode_unit(&pause_save, i, ww, wh, &actor));
        if (actor.alive && actor.owner == SIDE && actor.raw_owner == SC64_ZL_HUMAN_SLOT) { unit = (int)i; break; }
    }
    assert(unit >= 0);
    bad = pause_save; actor.raw_owner = 2;
    assert(encode_candidate_unit(&bad, (unsigned)unit, ww, wh, &actor));
    write_candidate_record(&bad);
    assert(!load_game() && world_digest() == before);
    write_candidate_record(&pause_save);
}

/* Fixture-only countdown write (not gameplay evidence): expiring the timer
 * fires T4, which recreates the Drone, re-centres location 3 on it and
 * creates the crystal that the Drone then carries. */
static void test_crystal_returns(void)
{
    int crystal, carrier;
    race_zl_sched.countdown = 1u;
    for (int i = 0; i < 200 && find_unit_raw(SIDE, SC64_ZL_RAW_CRYSTAL) < 0; ++i) input(0);
    crystal = find_unit_raw(SIDE, SC64_ZL_RAW_CRYSTAL);
    assert(crystal >= 0 && !race_zl_sched.core.fault);
    assert(u[crystal].type == U_CHRYSALIS && u[crystal].raw_owner == SC64_ZL_HUMAN_SLOT);
    assert(invincible_units[crystal]);
    input(0);
    carrier = race_zl_crystal_carrier;
    assert(carrier != SC64_ZL_NO_CARRIER && u[carrier].raw_type == SC64_ZL_RAW_DRONE);
    assert(u[crystal].x == u[carrier].x && u[crystal].y == u[carrier].y);
    printf("Z9 crystal: created at (%d,%d), carried by unit %d\n",
           u[crystal].x, u[crystal].y, carrier);
}

/* Fixture-only placement: the carrier walks into location 1, so T1 brings
 * the crystal home and its Victory ends the mission. */
static void test_crystal_delivery(void)
{
    int carrier = race_zl_crystal_carrier, x, y;
    assert(carrier != SC64_ZL_NO_CARRIER);
    location_centre(1, &x, &y);
    u[carrier].x = (s16)x; u[carrier].y = (s16)y; u[carrier].order = ORDER_IDLE;
    for (int i = 0; i < 600 && !won && !lost; ++i) input(0);
    assert(won && !lost && race_zl_outcome() == SC64_Z8_OUTCOME_VICTORY);
    assert(race_completed & (1u << SC64_Z9_GLOBAL_ID));
    puts("Z9 victory: crystal delivered to location 1");
}

int main(void)
{
#ifdef SC64_Z10_RUNTIME
    assert(SC64_RACE_SAVE_GENERATION == 49u);
#else
    assert(SC64_RACE_SAVE_GENERATION == 48u);
#endif
    assert(sizeof(RaceSave) <= 16368u);
    launch_z9_from_normal_menu();
    test_binding();
    test_opening_triggers();
    test_drone_enters_formation();
    check_continuation("countdown running", 6);
    test_rejects_corrupt_candidate();
    test_rejects_invalid_source_world();
    reload(&pause_save, pause_frame);
    test_crystal_returns();
    check_continuation("crystal carried", 2);
    test_crystal_delivery();
    printf("Z9 campaign binding: %u instances, RaceSave %u bytes, generation %u\n",
           (unsigned)race_zl_sched.core.instance_count, (unsigned)sizeof(RaceSave),
           (unsigned)SC64_RACE_SAVE_GENERATION);
    return 0;
}
