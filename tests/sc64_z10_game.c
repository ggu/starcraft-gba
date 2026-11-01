/* Actual-engine Z10 campaign binding checks. The mission is launched through
 * the ordinary campaign menu, its authenticated trigger program runs on the
 * live pools, and ordinary pause-menu saves reload into a fresh game that
 * then continues identically to the uninterrupted one. The kills and carrier
 * move below are fixture-only state writes standing in for a controller;
 * this is a host fixture, not a mission win. */
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"
#include "sc64_save_candidate_fixture.h"

#ifndef SC64_Z10_RUNTIME
#error "Compile this fixture with the zerg-z10 profile flags"
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

static void launch_z10_from_normal_menu(void)
{
    int ordinal;
    game_init();
    held_keys = 0;
    assert(race_front.page == SC_FRONT_HOME);
    ordinal = sc64_catalog_playable_ordinal(SC64_FACTION_ZERG, SC64_Z10_GLOBAL_ID);
    assert(ordinal == 9);
    assert(sc64_catalog_playable_count(SC64_FACTION_ZERG) == 10);
    press(KEY_A);
    assert(race_front.page == SC_FRONT_MISSION && race_front.faction == SC_FRONT_ZERG);
    for (int page = 0; page < ordinal / 2; ++page) press(KEY_RIGHT);
    if (ordinal & 1) press(KEY_DOWN);
    press(KEY_A);
    assert(race_front.page == SC_FRONT_BRIEFING && race_front.mission == SC64_Z10_GLOBAL_ID);
    press(KEY_START);
    assert(screen == SCREEN_PLAY && !race_mode && !won && !lost);
    assert(race_selected_mission == SC64_Z10_GLOBAL_ID && race_zl_active());
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

static int find_temple(void)
{
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].raw_type == 174) return i;
    return -1;
}

static void location_centre(unsigned location, int *x, int *y)
{
    const Sc64Z8Rect *r = &race_zl_sched.core.locations[location - 1];
    *x = ((int)r->left + r->right) / 4;
    *y = ((int)r->top + r->bottom) / 4;
}

static void test_binding(void)
{
    int temple = find_temple();
    unsigned slot0 = 0;
    assert(race_zl_sched.core.instance_count == 10u && !race_zl_sched.core.fault);
    assert(race_zl_sched.core.active_mask == 0x8du);
    assert(race_zl_sched.core.force[0] == 2u && race_zl_sched.core.force[2] == 3u &&
           race_zl_sched.core.force[3] == 1u && race_zl_sched.core.force[7] == 4u);
    for (unsigned i = 0; i < SC64_Z10_PROGRAM_ACTION_COUNT; ++i)
        if (sc64_z10_program_actions[i].opcode == 16u)
            assert(sc64_zl_ai_script_known(&race_z10_program, sc64_z10_program_actions[i].group2));
    assert(count_unit_raw(SIDE, 37) == 6 && count_unit_raw(SIDE, 38) == 3 &&
           count_unit_raw(SIDE, 39) == 1 && count_unit_raw(SIDE, 41) == 3 &&
           count_unit_raw(SIDE, 42) == 4);
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive && u[i].owner == SIDE)
        assert(u[i].raw_owner == SC64_ZL_HUMAN_SLOT);
    assert(temple >= 0 && b[temple].type == B_ZL_PROTOSS_TEMPLE);
    assert(b[temple].owner == ENEMY && b[temple].raw_owner == 0);
    assert(!invincible_buildings[temple] && b[temple].maxhp == 1500);
    for (int i = 0; i < MAX_UNITS; ++i) slot0 += u[i].alive && u[i].raw_owner == 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i) slot0 += b[i].alive && b[i].raw_owner == 0;
    assert(slot0 == race_zl_map_count(0, 0) && slot0 == 60u);
    assert(race_zl_live_world_valid());
}

/* T1 sets 1000/1000, T4 and T5 fund and start slots 2 and 7, T6 funds the
 * Temple guard (slot 0), whose town waits for T7 or T8. */
static void test_opening_triggers(void)
{
    for (int i = 0; i < 150; ++i) input(0);
    assert(!race_zl_sched.core.fault && race_zl_outcome() == SC64_Z8_OUTCOME_NONE);
    assert(minerals == 1000 && gas == 1000);
    assert(race_zl_ai.started == 3u);
    assert(race_zl_ai_minerals[2] == 20000u);
    assert(!sc64_z8_switch_get(&race_zl_sched.core, 10));
}

/* Fixture-only removal of one guard unit: the Deaths census sees it, T7 sets
 * switch 10 and T9 starts the guard's Z10e town. */
static void test_guard_death_starts_town(void)
{
    int guard = -1;
    for (int i = 0; i < MAX_UNITS && guard < 0; ++i)
        if (u[i].alive && u[i].raw_owner == 0 && u[i].raw_type == 65) guard = i;
    assert(guard >= 0);
    race_zl_clear_unit(guard);
    for (int i = 0; i < 200 && !(race_zl_ai.started & 4u); ++i) input(0);
    assert(!race_zl_sched.core.fault && sc64_z8_switch_get(&race_zl_sched.core, 10));
    assert(race_zl_ai.started == 7u);
    puts("Z10 guard death: switch 10 set, Temple guard town started");
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
        race_selected_mission = SC64_Z10_GLOBAL_ID;
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
    printf("Z10 save continuation (%s): frame %u, %u updates identical, digest %08x\n",
           label, saved_frame, blocks * 500u, (unsigned)resumed[blocks - 1]);
}

static void test_rejects_corrupt_candidate(void)
{
    static RaceSave good, bad;
    u32 before;
    good = pause_save;
    before = world_digest();
    bad = good;
    bad.zl.instance[0] = 0xffu;
    write_candidate_record(&bad);
    assert(!load_game() && world_digest() == before);
    bad = good;
    ((u8 *)&bad.z5)[sizeof(Sc64ZlPersistentState)] = 1;
    write_candidate_record(&bad);
    assert(!load_game() && world_digest() == before);
    bad = good;
    bad.zl.moved[0].left = 1; /* Z10 moves no location */
    write_candidate_record(&bad);
    assert(!load_game() && world_digest() == before);
    write_candidate_record(&good);
}

/* Fixture-only Temple destruction stands in for the human's attack. The
 * Kill census counts it, and T2 creates the Drone and crystal at location 1. */
static void test_temple_kill(void)
{
    int temple = find_temple(), crystal, carrier;
    assert(temple >= 0);
    b[temple].alive = 0;
    if (selected_building == temple) selected_building = -1;
    invalidate_path_grid();
    for (int i = 0; i < 400 && find_unit_raw(SIDE, SC64_ZL_RAW_CRYSTAL) < 0; ++i) input(0);
    crystal = find_unit_raw(SIDE, SC64_ZL_RAW_CRYSTAL);
    assert(crystal >= 0 && !race_zl_sched.core.fault);
    input(0);
    carrier = race_zl_crystal_carrier;
    assert(carrier != SC64_ZL_NO_CARRIER && u[carrier].raw_type == SC64_ZL_RAW_DRONE);
    printf("Z10 Temple destroyed: crystal at (%d,%d), carried by unit %d\n",
           u[crystal].x, u[crystal].y, carrier);
}

/* Fixture-only placement: the carrier reaches location 4, the Temple's
 * site, and T3's presentation ends in Victory. */
static void test_crystal_delivery(void)
{
    int carrier = race_zl_crystal_carrier, x, y;
    assert(carrier != SC64_ZL_NO_CARRIER);
    location_centre(4, &x, &y);
    u[carrier].x = (s16)x; u[carrier].y = (s16)y; u[carrier].order = ORDER_IDLE;
    for (int i = 0; i < 1200 && !won && !lost; ++i) input(0);
    assert(won && !lost && race_zl_outcome() == SC64_Z8_OUTCOME_VICTORY);
    assert(race_completed & (1u << SC64_Z10_GLOBAL_ID));
    puts("Z10 victory: crystal delivered to location 4");
}

int main(void)
{
    assert(SC64_RACE_SAVE_GENERATION == 49u);
    assert(sizeof(RaceSave) <= 16368u);
    launch_z10_from_normal_menu();
    test_binding();
    test_opening_triggers();
    test_guard_death_starts_town();
    check_continuation("towns running", 6);
    test_rejects_corrupt_candidate();
    reload(&pause_save, pause_frame);
    test_temple_kill();
    check_continuation("crystal carried", 2);
    test_crystal_delivery();
    printf("Z10 campaign binding: %u instances, RaceSave %u bytes, generation %u\n",
           (unsigned)race_zl_sched.core.instance_count, (unsigned)sizeof(RaceSave),
           (unsigned)SC64_RACE_SAVE_GENERATION);
    return 0;
}
