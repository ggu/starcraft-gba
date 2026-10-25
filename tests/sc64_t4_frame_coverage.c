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

#ifdef SC64_STATIC_DIRTY_ROWS
static void
test_static_dirty_row_partial_and_clip_contract(void)
{
    start_scene();
    unsigned row = prop_row(205);
    unsigned slot = race_t4.doodad_slot[row];
    assert(slot < MAX_BUILDINGS && b[slot].alive);

    /* Put a second static prop over the same source row, then warm the
     * cache. Repainting any dirty band must replay the complete ordered
     * static pass so transparent overlaps keep their painter order. */
    unsigned other = prop_row(206);
    unsigned other_slot = race_t4.doodad_slot[other];
    assert(other_slot < MAX_BUILDINGS && b[other_slot].alive);
    b[other_slot].x = b[slot].x + 8;
    b[other_slot].y = b[slot].y;
    focus_prop(row);
    assert_cached_equals_fresh();

    int center_cell = (b[slot].x / 16) + 1 + (b[slot].y / 16) * MAP_COLS;
    int other_cell = center_cell + 3 * MAP_COLS;
    assert(center_cell >= 0 && center_cell < MAP_CELLS);
    assert(other_cell >= 0 && other_cell < MAP_CELLS);
    SC64_GRID_SET(visible, center_cell, 0);
    SC64_GRID_SET(explored, center_cell, 0);
    SC64_GRID_SET(visible, other_cell, 0);
    SC64_GRID_SET(explored, other_cell, 0);
    unsigned partial_before = sc64_terrain_partial;
    unsigned planned_before = sc64_static_dirty_rows_host_bands;
    poison_both_backbuffers();
    game_render();
    capture_frame(cached_frame);
    assert(sc64_terrain_partial > partial_before);
    assert(sc64_static_dirty_rows_host_bands > planned_before);
    sc64_terrain_cache_valid = 0;
    sc64_hud_valid = 0;
    poison_both_backbuffers();
    gfx_begin(C_BLACK);
    game_render();
    capture_frame(fresh_frame);
    assert(memcmp(cached_frame, fresh_frame, sizeof(cached_frame)) == 0);

    /* The normal game path owns [12,120), while direct draw_map callers
     * retain their full-screen clip and must take the conservative pass. */
    SC64_GRID_SET(visible, center_cell, 1);
    SC64_GRID_SET(explored, center_cell, 1);
    gfx_begin(C_BLACK);
    gfx_set_clip_y(0, 160);
    partial_before = sc64_terrain_partial;
    planned_before = sc64_static_dirty_rows_host_bands;
    draw_map();
    assert(sc64_terrain_partial > partial_before);
    assert(sc64_static_dirty_rows_host_bands == planned_before);
    assert(gfx_get_clip_y() == (u16)(0u | (160u << 8)));

    /* A large camera jump misses reuse entirely and cannot claim a dirty-row
     * span; it takes the existing full static pass. */
    cx = 1900;
    cy = 1950;
    gfx_set_clip_y(12, 120);
    planned_before = sc64_static_dirty_rows_host_bands;
    draw_map();
    assert(sc64_static_dirty_rows_host_bands == planned_before);
}

static void
test_small_camera_scrolls_use_dirty_rows(void)
{
    static const int deltas[] = {1, 2, 3, 15, 16, 17};

    for (unsigned step = 0; step < sizeof(deltas) / sizeof(deltas[0]); ++step) {
        start_scene();
        unsigned row = prop_row(205);
        unsigned slot = race_t4.doodad_slot[row];
        assert(slot < MAX_BUILDINGS && b[slot].alive);
        for (int i = 0; i < MAX_BUILDINGS; ++i)
            if ((unsigned)i != slot) b[i].alive = 0;
        for (int i = 0; i < MAX_MINERALS; ++i) m[i].amount = 0;
        geyser_count = 0;
        b[slot].owner = SIDE;
        memset(visible, 0xff, sizeof(visible));
        memset(explored, 0xff, sizeof(explored));

        focus_prop(row);
        int base_cy = ((cy - 54) / 16) * 16 + 54;
        if (cy < 54) base_cy = 54;
        cy = base_cy;
        sc64_terrain_cache_valid = 0;
        sc64_hud_valid = 0;
        gfx_begin(C_BLACK);
        game_render();

        cy = base_cy + deltas[step];
        unsigned bands_before = sc64_static_dirty_rows_host_bands;
        gfx_begin(C_BLACK);
        game_render();
        capture_frame(cached_frame);
        assert(sc64_static_dirty_rows_host_bands > bands_before);

        sc64_terrain_cache_valid = 0;
        sc64_hud_valid = 0;
        gfx_begin(C_BLACK);
        game_render();
        capture_frame(fresh_frame);
        assert(memcmp(cached_frame, fresh_frame, sizeof(cached_frame)) == 0);
    }
}
#endif

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
#ifdef SC64_STATIC_DIRTY_ROWS
    test_static_dirty_row_partial_and_clip_contract();
    test_small_camera_scrolls_use_dirty_rows();
#endif
    test_restart_and_load_rebuild_prop_keys();
    puts("T4 cached and fresh full-frame output matches");
    return 0;
}
