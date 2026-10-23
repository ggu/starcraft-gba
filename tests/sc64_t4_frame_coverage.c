/* Full-frame T4 renderer oracle for cached versus forced-fresh terrain. */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../starcraft/game.c"

static u8 cached_frame[240 * 160];
static u8 fresh_frame[240 * 160];

static void
capture_frame(u8 *out)
{
    for (int y = 0; y < 160; ++y)
        for (int x = 0; x < 240; ++x)
            out[y * 240 + x] = gfx_test_pixel(x, y);
}

static void
poison_both_backbuffers(void)
{
    /* Two clears hit both alternating host/GBA backbuffers and restore the
     * original front/back parity. This catches preserve-mode coverage holes. */
    for (int i = 0; i < 2; ++i) {
        gfx_begin(0xe7);
        gfx_end();
    }
}

static void
assert_cached_equals_fresh(void)
{
    poison_both_backbuffers();
    game_render();
    capture_frame(cached_frame);

    /* A forced miss is the independent full redraw oracle. Reset the HUD
     * cache too so the reference reconstructs every screen band. */
    sc64_terrain_cache_valid = 0;
    sc64_hud_valid = 0;
    poison_both_backbuffers();
    /* Make the oracle independent of preserve-mode coverage: clear the same
     * selected backbuffer that game_render will use, without flipping it. */
    gfx_begin(C_BLACK);
    game_render();
    capture_frame(fresh_frame);
    assert(memcmp(cached_frame, fresh_frame, sizeof(cached_frame)) == 0);
}

static unsigned
prop_row(unsigned raw)
{
    for (unsigned row = 0; row < sc64_t4_scenario.doodad_count; ++row)
        if (!(sc64_t4_scenario.doodads[row].flags & SC64_T4_THG2_SPRITE_FLAG) &&
            sc64_t4_scenario.doodads[row].type == raw &&
            race_t4.doodad_slot[row] < MAX_BUILDINGS)
            return row;
    assert(!"requested T4 prop is absent");
    return 32;
}

static void
focus_prop(unsigned row)
{
    unsigned slot = race_t4.doodad_slot[row];
    assert(slot < MAX_BUILDINGS && b[slot].alive);
    cx = b[slot].x;
    cy = b[slot].y;
}

static void
start_scene(void)
{
    game_init();
    race_mode = 0;
    race_selected_mission = SC64_T4_GLOBAL_ID;
    init_state();
    assert(race_t4_active());
    assert(screen == SCREEN_PLAY);
    memset(visible, 0xff, sizeof(visible));
    memset(explored, 0xff, sizeof(explored));
    sc64_visibility_reset();
    /* Rebuild visibility after reset, then reveal the whole fixture so every
     * source prop is eligible for the full-frame rendering oracle. */
    SC64_GRID_CLEAR(visible);
    SC64_GRID_CLEAR(explored);
    for (int cell = 0; cell < MAP_CELLS; ++cell) {
        SC64_GRID_SET(visible, cell, 1);
        SC64_GRID_SET(explored, cell, 1);
    }
    sc64_terrain_cache_valid = 0;
    sc64_hud_valid = 0;
    screen = SCREEN_PLAY;
    mode = MODE_NORMAL;
    won = lost = 0;
}

static void
test_doodad_state_invalidates_art(unsigned raw, unsigned first_state,
                                  unsigned second_state)
{
    u8 prior_frame[sizeof(fresh_frame)];
    start_scene();
    unsigned row = prop_row(raw);
    focus_prop(row);
    race_t4.doodad_enabled[row] = (u8)first_state;
    assert_cached_equals_fresh();
    memcpy(prior_frame, fresh_frame, sizeof(prior_frame));

    unsigned old_key = sc64_static_objects_key();
    race_t4.doodad_enabled[row] = (u8)second_state;
    unsigned new_key = sc64_static_objects_key();
    assert(old_key != new_key);
    assert_cached_equals_fresh();
    assert(memcmp(prior_frame, fresh_frame, sizeof(prior_frame)) != 0);
}

static void
test_prop_raw_identity_invalidates_key(void)
{
    start_scene();
    unsigned row = prop_row(205);
    unsigned slot = race_t4.doodad_slot[row];
    assert(slot < MAX_BUILDINGS);
    focus_prop(row);
    assert_cached_equals_fresh(); /* Set the rendered viewport before keying. */
    unsigned old_key = sc64_static_objects_key();
    int raw_type = b[slot].raw_type;
    b[slot].raw_type = 206;
    assert(sc64_static_objects_key() != old_key);
    b[slot].raw_type = raw_type;
    assert(sc64_static_objects_key() != old_key);
}

static void
test_binding_change_invalidates_art(void)
{
    start_scene();
    unsigned row = prop_row(205);
    unsigned slot = race_t4.doodad_slot[row];
    assert(slot < MAX_BUILDINGS);
    focus_prop(row);
    race_t4.doodad_enabled[row] = 0;
    assert_cached_equals_fresh();
    u8 prior_frame[sizeof(fresh_frame)];
    memcpy(prior_frame, fresh_frame, sizeof(prior_frame));

    unsigned old_key = sc64_static_objects_key();
    race_t4.doodad_slot[row] = 255;
    assert(sc64_static_objects_key() != old_key);
    assert_cached_equals_fresh();
    assert(memcmp(prior_frame, fresh_frame, sizeof(prior_frame)) != 0);

    old_key = sc64_static_objects_key();
    race_t4.doodad_slot[row] = (u8)slot;
    assert(sc64_static_objects_key() != old_key);
    assert_cached_equals_fresh();
}

static void
test_camera_fog_and_scene_bands(void)
{
    start_scene();
    assert_cached_equals_fresh();
    static const int camera[][2] = {
        {120, 54}, {357, 211}, {800, 900}, {1900, 1950}, {2047, 2047},
        {121, 55}, {1790, 1800}
    };
    for (unsigned i = 0; i < sizeof(camera) / sizeof(camera[0]); ++i) {
        cx = camera[i][0];
        cy = camera[i][1];
        assert_cached_equals_fresh();
    }

    /* Fog changes must dirty the exact affected tiles after a cached view. */
    cx = 800; cy = 900;
    assert_cached_equals_fresh();
    int cell = 50 + 56 * MAP_COLS;
    SC64_GRID_SET(visible, cell, 0);
    SC64_GRID_SET(explored, cell, 0);
    assert_cached_equals_fresh();
    SC64_GRID_SET(visible, cell, 1);
    SC64_GRID_SET(explored, cell, 1);
    assert_cached_equals_fresh();

    screen = SCREEN_PAUSE;
    assert_cached_equals_fresh();
    screen = SCREEN_RESULT;
    won = 1; lost = 0;
    assert_cached_equals_fresh();
}

static void
test_restart_and_load_rebuild_prop_keys(void)
{
    start_scene();
    unsigned row = prop_row(205);
    focus_prop(row);
    assert_cached_equals_fresh();
    race_t4.doodad_enabled[row] ^= 1;
    assert_cached_equals_fresh();

    /* Restart restores canonical map bindings/state while retaining the same
     * global mission id, so the object key itself must invalidate the cache. */
    init_state();
    assert(race_t4_active());
    screen = SCREEN_PLAY;
    assert_cached_equals_fresh();

    assert(save_game());
    race_t4.doodad_enabled[row] ^= 1;
    assert_cached_equals_fresh();
    assert(load_game());
    assert_cached_equals_fresh();
}

int
main(void)
{
    gba_init();
    static const unsigned props[] = {203, 205, 206, 207, 208, 209, 211, 212};
    for (unsigned i = 0; i < sizeof(props) / sizeof(props[0]); ++i) {
        test_doodad_state_invalidates_art(props[i], 0, 1);
        test_doodad_state_invalidates_art(props[i], 1, 0);
    }
    test_prop_raw_identity_invalidates_key();
    test_binding_change_invalidates_art();
    test_camera_fog_and_scene_bands();
    test_restart_and_load_rebuild_prop_keys();
    puts("T4 cached and fresh full-frame output matches");
    return 0;
}
