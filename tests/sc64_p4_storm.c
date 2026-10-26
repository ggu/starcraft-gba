/* Actual-engine P4 Storm input, damage and persistence checks. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

#ifndef SC64_P4_STORM_ENABLED
#error "Compile the P4 Storm fixture with SC64_P4_STORM_ENABLED"
#endif

enum { STORM_FB_WIDTH = 240, STORM_FB_HEIGHT = 160,
       STORM_FB_SIZE = STORM_FB_WIDTH * STORM_FB_HEIGHT };
static u8 storm_frame_a[STORM_FB_SIZE];
static u8 storm_frame_b[STORM_FB_SIZE];
static u16 prior_keys;

static void input(u16 held)
{
    game_update(held, held & (u16)~prior_keys);
    prior_keys = held;
}

static void press(u16 key) { input(key); input(0); }

static void launch_from_normal_menu(void)
{
    game_init();
    prior_keys = 0;
    press(KEY_A);
    press(KEY_RIGHT);
    press(KEY_DOWN);
    press(KEY_A);
    input(KEY_A);
    assert(screen == SCREEN_PLAY && race_p4_active());
#ifdef SC64_P5_SHUTTLE_ENABLED
    /* P4 remains the selected mission, but the combined P5 ROM owns the
     * shared save identity and generation. */
    assert(SAVE_VERSION == 0x53435030u + 26u);
#else
    assert(SAVE_VERSION == 0x53435030u + 21u);
#endif
}

static int source_unit(int raw)
{
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].raw_type == raw) return i;
    return -1;
}

static void test_escort_group_chord(void)
{
    launch_from_normal_menu();
    int tassadar = source_unit(SC64_P4_RAW_TASSADAR);
    int raynor = spawn_unit(SIDE, U_ZP_RAYNOR_VULTURE, 1640, 1900);
    int escort = spawn_unit(SIDE, U_ZEALOT, 1630, 1900);
    assert(tassadar >= 0 && raynor >= 0 && escort >= 0);
    u[tassadar].owner = SIDE;
    u[raynor].raw_type = SC64_P4_RAW_RAYNOR_VULTURE;
    press(KEY_R);
    assert(u[tassadar].selected && u[raynor].selected && u[escort].selected);
    input(KEY_B);
    assert(menu_open);
    input(KEY_B | KEY_R);
    assert(!menu_open && mode == MODE_NORMAL);
    assert(!u[tassadar].selected && !u[raynor].selected && u[escort].selected);
    input(0);
    /* Both synthetic heroes becoming human-owned starts the real rescue
     * presentation. Ordinary commands correctly wait for its source pause. */
    for (int wait = 0; race_p4.source_paused && wait < 5000; ++wait) input(0);
    assert(!race_p4.source_paused && screen == SCREEN_PLAY);
    press(KEY_R);
    assert(u[tassadar].selected && u[raynor].selected && u[escort].selected);
}

static int rescue_high_templar(void)
{
    int caster = source_unit(SC64_P4_STORM_HIGH_TEMPLAR_RAW);
    if (caster < 0) return -1;
    if (u[caster].owner == SIDE) return caster;
    assert(u[caster].owner == RESCUABLE);
    int friend = -1;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (i != caster && u[i].alive && u[i].owner == SIDE) { friend = i; break; }
    assert(friend >= 0);
    u[friend].x = u[caster].x; u[friend].y = u[caster].y;
    input(0); /* The P4 contact-rescue adapter transfers ownership. */
    assert(u[caster].owner == SIDE && u[caster].raw_owner == 1);
    return caster;
}

static void select_caster_and_enter_storm(int caster)
{
    assert(caster >= 0 && u[caster].alive && u[caster].owner == SIDE);
    cx = u[caster].x; cy = u[caster].y;
    press(KEY_A);
    assert(u[caster].selected);
    assert(race_p4_storm_menu_available());
    assert(field_menu_count() == MENU_STOP + 2);
    press(KEY_B);
    assert(menu_open);
    for (int i = 0; i < MENU_STOP + 1; ++i) press(KEY_DOWN);
    assert(menu_cursor == MENU_STOP + 1);
    press(KEY_A);
    assert(!menu_open && mode == MODE_P4_STORM);
}

static void cast_from_storm_at(int caster, int target_x, int target_y)
{
    int expected_energy = u[caster].carry - SC64_STORM_ENERGY_COST * SC64_STORM_ENERGY_ONE;
    u8 expected_remainder = (u8)u[caster].work_timer;
    for (int i = 0; i < 2; ++i) {
        expected_energy = sc64_psionic_storm_regen_q8(expected_energy,
            race_p4_storm_max_energy(u[caster].raw_type), &expected_remainder);
        assert(expected_energy >= 0);
    }
    cx = target_x; cy = target_y;
    press(KEY_A);
    assert(mode == MODE_NORMAL);
    assert(u[caster].carry == expected_energy);
    assert(u[caster].work_timer == expected_remainder);
}

static void select_caster_and_open_storm(int caster, int target_x, int target_y)
{
    select_caster_and_enter_storm(caster);
    cast_from_storm_at(caster, target_x, target_y);
}

static void test_inactive_runtime_and_save(void)
{
    game_init();
    prior_keys = 0;
    assert(!race_p4_active());
    assert(!race_p4_storm_menu_available());
    sc64_psionic_storm_init(&race_p4.storm);
    race_p4.storm.storm[0].active = 2;
    Sc64PsionicStormState before = race_p4.storm;
    assert(race_p4_storm_tick());
    assert(!memcmp(&before, &race_p4.storm, sizeof(before)));
    assert(race_save_game());
    Sc64PsionicStormState empty;
    sc64_psionic_storm_init(&empty);
    assert(!memcmp(&empty, &race_data.p4.storm, sizeof(empty)));
}

static void test_cast_range_pool_and_cancel(void)
{
    launch_from_normal_menu();
    for (int i = 0; i < 180 && race_p4.source_paused; ++i) input(0);
    assert(!race_p4.source_paused);
    int ordinary = -1;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE) { ordinary = i; break; }
    assert(ordinary >= 0);
    u[ordinary].selected = 1;
    assert(!race_p4_storm_menu_available());
    assert(field_menu_count() == MENU_STOP + 1);
    u[ordinary].selected = 0;

    int caster = rescue_high_templar();
    assert(caster >= 0);
    u[caster].x = 1000; u[caster].y = 1000;
    cx = u[caster].x; cy = u[caster].y;
    press(KEY_A);
    assert(u[caster].selected && race_p4_storm_menu_available());
    int energy;

    /* The contextual row opens only for one eligible caster, and B cancels
     * it through the same field-menu input path. */
    press(KEY_B);
    assert(menu_open && field_menu_count() == MENU_STOP + 2);
    press(KEY_B);
    assert(!menu_open && mode == MODE_NORMAL);

    select_caster_and_enter_storm(caster);
    energy = u[caster].carry;
    u8 expected_remainder = (u8)u[caster].work_timer;
    for (int i = 0; i < 2; ++i)
        energy = sc64_psionic_storm_regen_q8(energy,
            race_p4_storm_max_energy(u[caster].raw_type), &expected_remainder);
    cx = 1200; cy = 1000; /* farther than the audited 144-pixel range */
    press(KEY_A);
    assert(mode == MODE_NORMAL && !race_p4.storm.storm[0].active);
    assert(u[caster].carry == energy);
    assert(u[caster].work_timer == expected_remainder);

    /* A full four-effect pool rejects another cast without spending energy. */
    for (int i = 0; i < SC64_STORM_MAX_ACTIVE; ++i) {
        race_p4.storm.storm[i].x = (u16)(1000 + i);
        race_p4.storm.storm[i].y = 1000;
        race_p4.storm.storm[i].pulses_left = SC64_STORM_PULSE_COUNT;
        race_p4.storm.storm[i].active = 1;
    }
    assert(race_p4_storm_world_state_valid());
    assert(!race_p4_storm_cast_selected(1000, 1000));
    assert(u[caster].carry == energy);
}

static void test_menu_cast_damage_and_friendly_fire(void)
{
    launch_from_normal_menu();
    int source_caster = source_unit(SC64_P4_STORM_HIGH_TEMPLAR_RAW);
    assert(source_caster >= 0);
    assert(u[source_caster].carry == SC64_STORM_MAX_ENERGY * SC64_STORM_ENERGY_ONE);
    assert(u[source_caster].work_timer == 0);
    int caster = rescue_high_templar();
    assert(caster >= 0 && u[caster].owner == SIDE);
    assert(u[caster].carry == SC64_STORM_MAX_ENERGY * SC64_STORM_ENERGY_ONE);
    assert(u[caster].work_timer <= 4);

    /* Isolate a real caster and ordinary engine units in a clear patch. */
    u[caster].x = 1000; u[caster].y = 1000;
    int ground = spawn_unit(ENEMY, U_ZERGLING, 1010, 1000);
    int air = spawn_unit(ENEMY, U_MUTALISK, 1000, 1010);
    int protected_unit = spawn_unit(ENEMY, U_HYDRALISK, 1000, 1000);
    int building = spawn_build(ENEMY, B_PYLON, 1000, 1000, 0);
    assert(ground >= 0 && air >= 0 && protected_unit >= 0 && building >= 0);
    u[ground].hp = u[ground].maxhp = 100; u[ground].shield = 0;
    u[air].hp = u[air].maxhp = 100; u[air].shield = 0;
    u[protected_unit].hp = u[protected_unit].maxhp = 100;
    u[protected_unit].shield = 0; invincible_units[protected_unit] = 1;
    b[building].hp = b[building].maxhp = 300;

    int caster_hp_before = u[caster].hp;
    int caster_shield_before = u[caster].shield;
    select_caster_and_open_storm(caster, 1000, 1000);
    assert(race_p4.storm.storm[0].active);
    assert(race_p4.storm.storm[0].age_ticks == 2);

    for (int i = 0; i < 17; ++i) assert(race_p4_storm_tick());
    assert(race_p4.storm.storm[0].age_ticks == 19);
    assert(u[ground].hp == 100 && u[air].hp == 100);

    /* Two storms pulsing together still damage an actor only once per tick. */
    race_p4.storm.storm[1].x = 1000;
    race_p4.storm.storm[1].y = 1000;
    race_p4.storm.storm[1].age_ticks = 19;
    race_p4.storm.storm[1].pulses_left = SC64_STORM_PULSE_COUNT;
    race_p4.storm.storm[1].active = 1;
    assert(race_p4_storm_tick());
    assert(u[ground].hp == 86 && u[air].hp == 86);
    assert(u[protected_unit].hp == 100);
    assert(b[building].hp == 300);
    /* P4 Storm is friendly fire; the selected caster takes the pulse too. */
    assert(u[caster].hp < caster_hp_before || u[caster].shield < caster_shield_before);
    assert(race_p4_storm_world_state_valid());

    /* Exercise the actual world adapter's eighth pulse, including removal of
     * the effect slot after applying that final event. */
    sc64_psionic_storm_init(&race_p4.storm);
    race_p4.storm.storm[0].x = 1000;
    race_p4.storm.storm[0].y = 1000;
    race_p4.storm.storm[0].age_ticks = SC64_STORM_FIRST_PULSE_TICKS +
        (SC64_STORM_PULSE_COUNT - 1) * SC64_STORM_PULSE_INTERVAL_TICKS - 1;
    race_p4.storm.storm[0].pulses_left = 1;
    race_p4.storm.storm[0].active = 1;
    int final_target = spawn_unit(ENEMY, U_ZERGLING, 1000, 1000);
    assert(final_target >= 0);
    u[final_target].hp = u[final_target].maxhp = 100;
    u[final_target].shield = 0;
    assert(race_p4_storm_world_state_valid());
    assert(race_p4_storm_tick());
    assert(!race_p4.storm.storm[0].active && u[final_target].hp == 86);
}

static int encode_storm_test_unit(RaceSave *save, unsigned slot, const Unit *unit)
{
#ifdef SC64_ZP_FULL_STORAGE
    return sc64_zp_unit_save_encode(&save->units[slot], unit, WORLD_W, WORLD_H);
#else
    return sc64_unit_save_encode(&save->units[slot], unit, WORLD_W, WORLD_H);
#endif
}

static void test_pause_and_save_validation(void)
{
    launch_from_normal_menu();
    int caster = rescue_high_templar();
    assert(caster >= 0);
    u[caster].x = 1000; u[caster].y = 1000;
    select_caster_and_open_storm(caster, 1000, 1000);
    int energy = u[caster].carry;
    u16 age = race_p4.storm.storm[0].age_ticks;
    race_p4.source_paused = 1;
    assert(race_p4_storm_tick());
    assert(u[caster].carry == energy && race_p4.storm.storm[0].age_ticks == age);
    race_p4.source_paused = 0;

    assert(race_save_game());
    RaceSave good = race_data;
    race_p4.storm.storm[0].age_ticks = 0xffffu;
    assert(!race_save_game());
    race_p4 = good.p4;
    assert(race_load_game());
    assert(u[caster].carry == energy);
    assert(!memcmp(&race_p4.storm, &good.p4.storm, sizeof(race_p4.storm)));
    assert(mode == MODE_NORMAL && !menu_open);

    RaceSave corrupt = good;
    corrupt.p4.storm.storm[0].age_ticks = 0xffffu;
    save_write(&corrupt, sizeof(corrupt), SAVE_VERSION);
    Unit before_units[MAX_UNITS];
    memcpy(before_units, u, sizeof(u));
    Sc64P4RuntimeState before_p4 = race_p4;
    assert(!race_load_game());
    assert(!memcmp(before_units, u, sizeof(u)));
    assert(!memcmp(&before_p4, &race_p4, sizeof(race_p4)));

    corrupt = good;
    Unit candidate;
    assert(race_save_decode_unit(&corrupt, (unsigned)caster, WORLD_W, WORLD_H, &candidate));
    candidate.carry = SC64_STORM_MAX_ENERGY * SC64_STORM_ENERGY_ONE + 1;
    assert(encode_storm_test_unit(&corrupt, (unsigned)caster, &candidate));
    save_write(&corrupt, sizeof(corrupt), SAVE_VERSION);
    assert(!race_load_game());
    assert(!memcmp(before_units, u, sizeof(u)));
    assert(!memcmp(&before_p4, &race_p4, sizeof(race_p4)));
}

static void capture_storm_frame(u8 out[STORM_FB_SIZE])
{
    sc64_terrain_cache_valid = 0;
    sc64_terrain_cache_reused_base = 0;
    sc64_hud_valid = 0;
    gfx_begin(C_BLACK);
    draw_map();
    draw_hud();
    gfx_end();
    for (int y = 0; y < STORM_FB_HEIGHT; ++y)
        for (int x = 0; x < STORM_FB_WIDTH; ++x)
            out[y * STORM_FB_WIDTH + x] = gfx_test_pixel(x, y);
}

static void compare_consecutive_storm_frames(void)
{
    /* Exercise the real preserve-mode renderer on both warm framebuffer
     * pages. Clearing for every comparison would hide stale edge pixels. */
    for (int frame_index = 0; frame_index < 8; ++frame_index) {
        game_render();
        for (int y = 0; y < STORM_FB_HEIGHT; ++y)
            for (int x = 0; x < STORM_FB_WIDTH; ++x)
                storm_frame_b[y * STORM_FB_WIDTH + x] = gfx_test_pixel(x, y);
        assert(!memcmp(storm_frame_a, storm_frame_b, sizeof(storm_frame_a)));
    }
}

static void test_storm_vfx_and_target_edges(void)
{
    launch_from_normal_menu();
    sc64_psionic_storm_init(&race_p4.storm);
    race_p4.storm.storm[0].x = 20;
    race_p4.storm.storm[0].y = 20;
    race_p4.storm.storm[0].pulses_left = SC64_STORM_PULSE_COUNT;
    race_p4.storm.storm[0].active = 1;
    mode = MODE_P4_STORM;
    cx = 0; cy = 28;
    capture_storm_frame(storm_frame_a);
    capture_storm_frame(storm_frame_b);
    assert(!memcmp(storm_frame_a, storm_frame_b, sizeof(storm_frame_a)));
    assert(storm_frame_a[32 * STORM_FB_WIDTH + 20] == C_WHITE);
    assert(storm_frame_a[36 * STORM_FB_WIDTH + 4] == C_CYAN);
    compare_consecutive_storm_frames();

    cx = WORLD_W - 1; cy = WORLD_H - 1;
    race_p4.storm.storm[0].x = WORLD_W - 10;
    race_p4.storm.storm[0].y = WORLD_H - 38;
    capture_storm_frame(storm_frame_a);
    capture_storm_frame(storm_frame_b);
    assert(!memcmp(storm_frame_a, storm_frame_b, sizeof(storm_frame_a)));
    assert(storm_frame_a[82 * STORM_FB_WIDTH + 230] == C_WHITE);
    assert(storm_frame_a[115 * STORM_FB_WIDTH + 235] == C_CYAN);
    compare_consecutive_storm_frames();
}

int main(void)
{
    gba_init();
    test_inactive_runtime_and_save();
    test_escort_group_chord();
    test_cast_range_pool_and_cancel();
    test_menu_cast_damage_and_friendly_fire();
    test_pause_and_save_validation();
    test_storm_vfx_and_target_edges();
    puts("P4 Storm actual-engine checks passed");
    return 0;
}
