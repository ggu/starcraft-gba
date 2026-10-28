/* Pixel differential coverage for the imported HUD's split-band cache. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

enum { HUD_TOP = 1, HUD_BOTTOM = 2, FB_PIXELS = 240 * 160 };
static _Alignas(4) u8 cached_pixels[FB_PIXELS];
static _Alignas(4) u8 full_pixels[FB_PIXELS];
static _Alignas(4) u8 cached_frame_pixels[FB_PIXELS];
static _Alignas(4) u8 forced_frame_pixels[FB_PIXELS];
#ifdef SC64_T5_RUNTIME
static u8 cached_game_frame[FB_PIXELS];
static u8 fresh_game_frame[FB_PIXELS];
static u8 t5_frame29_bottom[240 * 40];
#endif

static void
fixture(void)
{
    game_init();
    memset(u, 0, sizeof(u));
    memset(b, 0, sizeof(b));
    memset(m, 0, sizeof(m));
    race_mode = 0;
    race_human = R_ZERG;
#if defined(SC64_P3_RUNTIME) && defined(SC64_P3_ENABLED) && defined(SC64_UNIFIED)
    race_selected_mission = SC64_P3_GLOBAL_ID;
#elif defined(SC64_Z4_RUNTIME)
    race_selected_mission = 15;
#elif defined(SC64_Z3_ENABLED)
    race_selected_mission = 14;
#elif defined(SC64_T3_ENABLED)
    race_selected_mission = 6;
#else
    race_selected_mission = 4;
#endif
    race_campaign_init();
    race_campaign.mission = race_selected_mission;
    race_campaign.phase = SC64_RACE_PHASE_ACTIVE;
    race_campaign.message = 10;
    race_campaign.message_ticks = 90;
    race_campaign.elapsed_frames = 600;
    frame = 600;
    minerals = 500;
    gas = 125;
    supply_used = 10;
    supply_cap = 60;
    mode = MODE_NORMAL;
    box_state = 0;
    selected_building = -1;
    train_type = U_DRONE;
    race_cols = 64;
    race_rows = 64;
    cx = cy = 0;
    camx = camy = 0;
    sc64_hud_valid = 0;
}

static void
read_full(u8 *out)
{
    for (int y = 0; y < 160; ++y)
        for (int x = 0; x < 240; ++x)
            out[y * 240 + x] = gfx_test_pixel(x, y);
}

static void
assert_game_frame_matches_forced_hud(void)
{
    game_render();
    read_full(cached_frame_pixels);
    /* Invalidate both HUD copies while preserving the same world and render
     * state. The next game_render is the full-HUD pixel oracle. */
    sc64_hud_valid = 0;
    game_render();
    read_full(forced_frame_pixels);
    if (memcmp(cached_frame_pixels, forced_frame_pixels,
               sizeof(cached_frame_pixels)) != 0) {
        for (int i = 0; i < FB_PIXELS; ++i) {
            if (cached_frame_pixels[i] != forced_frame_pixels[i]) {
                fprintf(stderr, "full-frame HUD mismatch at (%d,%d): cached %u forced %u\n",
                        i % 240, i / 240, cached_frame_pixels[i], forced_frame_pixels[i]);
                break;
            }
        }
        assert(0);
    }
}

static void
render_cached(void)
{
    gfx_begin(C_BLACK);
    draw_hud();
    read_full(cached_pixels);
}

static void
render_forced_full(void)
{
    sc64_hud_valid = 0;
    gfx_begin(C_BLACK);
    draw_hud();
    read_full(full_pixels);
}

static void
assert_cached_matches_full(void)
{
    render_cached();
    render_forced_full();
    assert(memcmp(cached_pixels, full_pixels, sizeof(full_pixels)) == 0);
}

/* Probe the production key function without consuming its key update. */
static int
probe_change_mask(void)
{
    int previous[SC64_HUD_KEY_COUNT];
    int valid = sc64_hud_valid;
    memcpy(previous, sc64_hud_previous, sizeof(previous));
    int changed = sc64_hud_changed();
    memcpy(sc64_hud_previous, previous, sizeof(previous));
    sc64_hud_valid = valid;
    return changed;
}

static void
expect_mask_and_match_at(int expected, int line)
{
    int observed = probe_change_mask();
    if (observed != expected)
        fprintf(stderr, "HUD invalidation mask at line %d: expected %d, got %d\n",
                line, expected, observed);
    assert(observed == expected);
    assert_cached_matches_full();
}
#define expect_mask_and_match(expected) expect_mask_and_match_at((expected), __LINE__)

static void
test_wallet_and_queued_supply_invalidate_top_only(void)
{
    assert_cached_matches_full();
    minerals += 37;
    assert(probe_change_mask() == HUD_TOP);
    /* Probe restored the cache key; actual draw must update top and retain
       the bottom commands band, with the exact same pixels as a full redraw. */
    assert_cached_matches_full();

    gas += 9;
    expect_mask_and_match(HUD_TOP);
    supply_used += 2;
    expect_mask_and_match(HUD_TOP);
    supply_cap += 8;
    expect_mask_and_match(HUD_TOP);

    b[0].alive = 1;
    b[0].owner = SIDE;
    b[0].type = B_HATCHERY;
    b[0].queue_count = 1;
    b[0].queue_head = 0;
    b[0].queue[0] = U_ZERGLING;
    assert(race_owner_queued_supply(SIDE) > 0);
    expect_mask_and_match(HUD_TOP);
}

static void
test_selection_building_box_and_modes_invalidate_bottom(void)
{
    u[0].alive = 1;
    u[0].owner = SIDE;
    u[0].type = U_ZERGLING;
    u[0].hp = 31;
    u[0].maxhp = 35;
    u[0].selected = 1;
    expect_mask_and_match(HUD_BOTTOM);
    u[0].hp = 19;
    expect_mask_and_match(HUD_BOTTOM);

    u[0].selected = 0;
    selected_building = 0;
    b[0].type = B_HATCHERY;
    b[0].under_construction = 1;
    expect_mask_and_match(HUD_BOTTOM);
    b[0].under_construction = 0;
    train_type = U_HYDRALISK;
    expect_mask_and_match(HUD_BOTTOM);
    b[0].queue_count = 2;
    expect_mask_and_match(HUD_TOP | HUD_BOTTOM);

    selected_building = -1;
    box_state = 2;
    expect_mask_and_match(HUD_BOTTOM);
    box_state = 0;
    mode = MODE_ATTACK;
    expect_mask_and_match(HUD_BOTTOM);
    mode = MODE_BUILD;
    expect_mask_and_match(HUD_BOTTOM);
}

#ifdef SC64_RACES
static void
test_skirmish_shield_only_changes_invalidate_cached_bottom(void)
{
    game_init();
    race_mode=1; race_human=R_PROTOSS; race_bots=1; race_skirmish_map=0;
    race_skirmish_init();
    screen=SCREEN_PLAY;
    memset(u,0,sizeof(u));
    memset(b,0,sizeof(b));
    Unit *actor=&u[0];
#ifdef SC64_P7_ACTORS
    actor->type=U_P7_ARCHON;
    actor->hp=actor->maxhp=10;
#else
    actor->type=U_DRAGOON;
    actor->hp=actor->maxhp=sc64_race_unit_rules[U_DRAGOON].hp;
#endif
    actor->alive=1; actor->owner=SIDE; actor->raw_type=sc64_race_raw_unit(actor->type);
    actor->raw_owner=7; actor->selected=1;
    actor->shield=sc64_actor_max_shields(actor->type,0)/2;
    sc64_hud_valid=0;
    assert_cached_matches_full();
    actor->shield++;
    expect_mask_and_match(HUD_BOTTOM);

    actor->selected=0;
    selected_building=0;
    Building *building=&b[0];
    building->alive=1; building->owner=SIDE; building->type=B_PYLON;
    building->raw_type=sc64_race_raw_building(B_PYLON); building->raw_owner=7;
    building->hp=building->maxhp=300; building->shield=80;
    sc64_hud_valid=0;
    assert_cached_matches_full();
    building->shield++;
    expect_mask_and_match(HUD_BOTTOM);
}
#endif

static void
test_mission_message_timer_and_minimap_stay_pixel_exact(void)
{
    race_campaign.message = 11;
    race_campaign.message_ticks = 120;
    expect_mask_and_match(HUD_BOTTOM);
    race_campaign.message_ticks = 60;
    /* Message overlay remains dynamic; its countdown is outside the cached
       resource/command bands and should not change those band keys. */
    expect_mask_and_match(0);

    frame += 30;
    race_campaign.elapsed_frames += 30;
#ifdef SC64_T3_ENABLED
    race_t3.remaining_ticks = 300;
#endif
#ifdef SC64_Z3_ENABLED
    race_z3.sequence = 1;
#endif
    expect_mask_and_match(HUD_BOTTOM);

    /* Minimap is deliberately redrawn outside the cached HUD bands. */
    sc64_minimap_visible[0] = 1;
#ifdef SC64_MINIMAP_RASTER_ACTIVE
    sc64_minimap_raster_update(0);
#endif
    assert_cached_matches_full();
    sc64_minimap_visible[0] = 0;
#ifdef SC64_MINIMAP_RASTER_ACTIVE
    sc64_minimap_raster_update(0);
#endif
    assert_cached_matches_full();
}

#ifdef SC64_HUD_PAGE_BANDS
/* A page band is usable only while it holds the current content version and
 * the page has not been cleared since it was drawn or copied. */
static int
page_band_current(u8 buffer, int band)
{
    return sc64_hud_page_version[buffer][band] == sc64_hud_band_version[band] &&
           sc64_hud_page_clear_generation[buffer] == gfx_clear_generation(buffer);
}

static void
test_per_buffer_hud_reuse_across_changes_and_scenes(void)
{
    u8 buffer, paused_buffer, title_buffer, poisoned_buffer;
    u32 generation;
    fixture();
    screen = SCREEN_PLAY;
    /* The fixture declares a 64x64 world around a larger mission roster.
     * Actors beyond it would mark the minimap panel outside the 24x24 map,
     * which rightly re-stales the bottom band; keep this a band test. */
    for (int i = 0; i < MAX_UNITS; i++)
        if (u[i].x < 0 || u[i].x >= WORLD_W || u[i].y < 0 || u[i].y >= WORLD_H)
            u[i].alive = 0;
    for (int i = 0; i < MAX_BUILDINGS; i++)
        if (b[i].x < 0 || b[i].x >= WORLD_W || b[i].y < 0 || b[i].y >= WORLD_H)
            b[i].alive = 0;
    sc64_hud_page_version[0][0] = sc64_hud_page_version[0][1] = ~0u;
    sc64_hud_page_version[1][0] = sc64_hud_page_version[1][1] = ~0u;

    /* The first HUD draw fills one page; the other stays stale until it is
     * selected, then copies from the page on screen. */
    game_render();
    buffer = gfx_backbuffer_index();
    assert(page_band_current(buffer, 0) && page_band_current(buffer, 1));
    assert(!page_band_current((u8)(buffer ^ 1), 0) &&
           !page_band_current((u8)(buffer ^ 1), 1));
    game_render();
    assert(page_band_current(gfx_backbuffer_index(), 0) &&
           page_band_current(gfx_backbuffer_index(), 1));
    game_render();
    assert(page_band_current(gfx_backbuffer_index(), 0) &&
           page_band_current(gfx_backbuffer_index(), 1));

    minerals += 31;
    game_render();
    buffer = gfx_backbuffer_index();
    assert(page_band_current(buffer, 0));
    assert(!page_band_current((u8)(buffer ^ 1), 0));
    assert(page_band_current((u8)(buffer ^ 1), 1));
    game_render();
    assert(page_band_current(gfx_backbuffer_index(), 0));
    assert_cached_matches_full();

    /* Pause draws over the bottom HUD strip. That page must repair the band
     * when it returns to live play, even though the HUD key is unchanged. */
    screen = SCREEN_PAUSE;
    game_render();
    paused_buffer = gfx_backbuffer_index();
    assert(!page_band_current(paused_buffer, 1));
    screen = SCREEN_PLAY;
    game_render();
    game_render();
    assert(page_band_current(paused_buffer, 1));
    assert_cached_matches_full();

    /* Result and title transitions also exercise preserved and cleared
     * pages; title's gfx_begin clear generation must force a HUD copy. */
    screen = SCREEN_RESULT;
    game_render();
    screen = SCREEN_PLAY;
    game_render();
    game_render();
    screen = SCREEN_TITLE;
    game_render();
    title_buffer = gfx_backbuffer_index();
    generation = gfx_clear_generation(title_buffer);
    assert(generation != sc64_hud_page_clear_generation[title_buffer]);
    screen = SCREEN_PLAY;
    game_render();
    game_render();
    assert(sc64_hud_page_clear_generation[title_buffer] == generation);
    assert_cached_matches_full();

    /* An external poison/clear is observable through the per-page
     * generation even though game_render subsequently selects preserve mode. */
    gfx_begin(0xe7);
    poisoned_buffer = gfx_backbuffer_index();
    generation = gfx_clear_generation(poisoned_buffer);
    gfx_end();
    assert(generation != sc64_hud_page_clear_generation[poisoned_buffer]);
    game_render();
    game_render();
    assert(sc64_hud_page_clear_generation[poisoned_buffer] == generation);
    assert_cached_matches_full();
}
#endif
#if (defined(SC64_TERRAN_LATE_RUNTIME) || defined(SC64_HUD_BUFFER_REUSE)) && defined(SC64_NO_FRAME_CLEAR)
#ifndef SC64_HUD_PAGE_BANDS
static void
test_per_buffer_hud_reuse_across_changes_and_scenes(void)
{
    u8 buffer, paused_buffer, title_buffer, poisoned_buffer;
    u32 generation;
    fixture();
    screen = SCREEN_PLAY;
    sc64_hud_buffer_dirty[0] = sc64_hud_buffer_dirty[1] = 0;
    sc64_hud_buffer_clear_generation[0] = sc64_hud_buffer_clear_generation[1] = 0;

    /* Initial HUD draw dirties both copies; the next two frames consume each
     * backbuffer's independent cached bands. */
    game_render();
    buffer = gfx_backbuffer_index();
    assert(sc64_hud_buffer_dirty[buffer] == 0);
    assert(sc64_hud_buffer_dirty[buffer ^ 1] == 3);
    game_render();
    assert(sc64_hud_buffer_dirty[gfx_backbuffer_index()] == 0);
    game_render();
    assert(sc64_hud_buffer_dirty[gfx_backbuffer_index()] == 0);

    minerals += 31;
    game_render();
    buffer = gfx_backbuffer_index();
    assert(sc64_hud_buffer_dirty[buffer] == 0);
    assert(sc64_hud_buffer_dirty[buffer ^ 1] & HUD_TOP);
    game_render();
    assert(!(sc64_hud_buffer_dirty[gfx_backbuffer_index()] & HUD_TOP));
    assert_cached_matches_full();

    /* Pause draws over the bottom HUD strip. That framebuffer must repair the
     * band when it returns to live play, even though the HUD key is unchanged. */
    screen = SCREEN_PAUSE;
    game_render();
    paused_buffer = gfx_backbuffer_index();
    assert(sc64_hud_buffer_dirty[paused_buffer] & HUD_BOTTOM);
    screen = SCREEN_PLAY;
    game_render();
    game_render();
    assert(!(sc64_hud_buffer_dirty[paused_buffer] & HUD_BOTTOM));
    assert_cached_matches_full();

    /* Result and title transitions also exercise preserved and cleared
     * backbuffers; title's gfx_begin clear generation must force a HUD copy. */
    screen = SCREEN_RESULT;
    game_render();
    screen = SCREEN_PLAY;
    game_render();
    game_render();
    screen = SCREEN_TITLE;
    game_render();
    title_buffer = gfx_backbuffer_index();
    generation = gfx_clear_generation(title_buffer);
    assert(generation != sc64_hud_buffer_clear_generation[title_buffer]);
    screen = SCREEN_PLAY;
    game_render();
    game_render();
    assert(sc64_hud_buffer_clear_generation[title_buffer] == generation);
    assert_cached_matches_full();

    /* An external poison/clear is observable through the per-buffer
     * generation even though game_render subsequently selects preserve mode. */
    gfx_begin(0xe7);
    poisoned_buffer = gfx_backbuffer_index();
    generation = gfx_clear_generation(poisoned_buffer);
    gfx_end();
    assert(generation != sc64_hud_buffer_clear_generation[poisoned_buffer]);
    game_render();
    game_render();
    assert(sc64_hud_buffer_clear_generation[poisoned_buffer] == generation);
    assert_cached_matches_full();
}

#endif

static void
test_full_frame_cache_does_not_retain_map_pixels_in_hud_bands(void)
{
    fixture();
    screen = SCREEN_PLAY;
    mode = MODE_NORMAL;
    /* Anchor the view at the top edge and put the cursor, a tall hatchery,
     * and a unit sprite there. Mirror large sprites at the lower edge, where
     * their lower pixels can enter the cached command band. */
    cx = 120; cy = 54;
    u[0].alive = 1; u[0].owner = SIDE; u[0].type = U_DRONE;
    u[0].raw_type = sc64_race_raw_unit(U_DRONE);
    u[0].x = 120; u[0].y = 0;
    u[1].alive = 1; u[1].owner = SIDE; u[1].type = U_DRONE;
    u[1].raw_type = sc64_race_raw_unit(U_DRONE);
    u[1].x = 180; u[1].y = 1023;
    b[0].alive = 1; b[0].owner = SIDE; b[0].type = B_HATCHERY;
    b[0].raw_type = sc64_race_raw_building(B_HATCHERY);
    b[0].x = 48; b[0].y = 0;
    b[1].alive = 1; b[1].owner = SIDE; b[1].type = B_HATCHERY;
    b[1].raw_type = sc64_race_raw_building(B_HATCHERY);
    b[1].x = 192; b[1].y = 1023;

    /* Populate both backbuffers, then move the sprites without changing HUD
     * keys. Compare the complete cached render against a forced band redraw. */
    game_render(); game_render(); game_render();
    for (int step = 0; step < 3; ++step) {
        game_render(); game_render(); /* consume any oracle invalidation */
        u[0].x += 1; u[1].x -= 1;
        b[0].x += 1; b[1].x -= 1;
        frame++;
        assert_game_frame_matches_forced_hud();
    }
}
#endif

#ifdef SC64_T5_RUNTIME
static void
capture_game_frame(u8 *out)
{
    for (int y = 0; y < 160; ++y)
        for (int x = 0; x < 240; ++x)
            out[y * 240 + x] = gfx_test_pixel(x, y);
}

static void
assert_cached_game_matches_fresh(void)
{
    /* Poison both buffers for the cache candidate, then independently clear
       the oracle buffer so preserve-mode holes cannot match by accident. */
    gfx_begin(0xe7); gfx_end();
    gfx_begin(0xe7); gfx_end();
    game_render();
    capture_game_frame(cached_game_frame);

    sc64_terrain_cache_valid = 0;
    sc64_hud_valid = 0;
    gfx_begin(C_BLACK);
    game_render();
    capture_game_frame(fresh_game_frame);
    assert(memcmp(cached_game_frame, fresh_game_frame,
                  sizeof(cached_game_frame)) == 0);
}

static void
test_t5_static_cache_key_and_hud_timer_are_pixel_exact(void)
{
    game_init();
    race_mode = 0;
    race_selected_mission = SC64_T5_GLOBAL_ID;
    init_state();
    assert(race_t5_active());
    assert(screen == SCREEN_PLAY);
    mode = MODE_NORMAL;
    memset(u, 0, sizeof(u));
    memset(b, 0, sizeof(b));
    memset(m, 0, sizeof(m));
    cx = 120; cy = 54; /* Top-left map edge. */
    SC64_GRID_CLEAR(visible);
    SC64_GRID_CLEAR(explored);
    for (int row = 0; row < 12; ++row)
        for (int col = 0; col < 18; ++col) {
            int cell = col + row * MAP_COLS;
            SC64_GRID_SET(explored, cell, 1);
            if ((row + col) & 1) SC64_GRID_SET(visible, cell, 1);
        }

    /* Place actual drawable static objects in the visible viewport. This is
       render-fixture state only; no gameplay or campaign proof is implied. */
    /* Keep the test patch outside the Command Center's composed source
     * body/underlay so depletion has independently visible pixels. */
    m[0].amount = 50; m[0].x = 64; m[0].y = 64; m[0].style = 0;
    int mineral_cell = (m[0].x / 16) + (m[0].y / 16) * MAP_COLS;
    SC64_GRID_SET(visible, mineral_cell, 1);
    SC64_GRID_SET(explored, mineral_cell, 1);
    int building_slot = -1;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (!b[i].alive) { building_slot = i; break; }
    assert(building_slot >= 0);
    b[building_slot].alive = 1;
    b[building_slot].owner = ENEMY;
    b[building_slot].type = B_COMMAND;
    b[building_slot].raw_type = -1;
    b[building_slot].x = 136; b[building_slot].y = 80;
    b[building_slot].hp = b[building_slot].maxhp = 100;
    int building_cell = (b[building_slot].x / 16) +
                        (b[building_slot].y / 16) * MAP_COLS;
    SC64_GRID_SET(explored, building_cell, 1);
    SC64_GRID_SET(visible, building_cell, 0);

    frame = 29;
    sc64_terrain_cache_valid = 0;
    sc64_hud_valid = 0;
    assert_cached_game_matches_fresh();
    assert(sc64_hud_changed() == 0);
    memcpy(t5_frame29_bottom, &fresh_game_frame[120 * 240],
           sizeof(t5_frame29_bottom));
    frame = 30;
    assert(sc64_hud_changed() == 0);
    assert_cached_game_matches_fresh();
    assert(memcmp(t5_frame29_bottom, &fresh_game_frame[120 * 240],
                  sizeof(t5_frame29_bottom)) == 0);

    /* Fog, camera, resources, and visible static-object state still key their
       respective cached pixels correctly after the one-key-per-draw change. */
    u8 before_object_change[FB_PIXELS];
    memcpy(before_object_change, fresh_game_frame, sizeof(before_object_change));
    SC64_GRID_SET(visible, building_cell, 1);
    assert_cached_game_matches_fresh();
    assert(memcmp(before_object_change, fresh_game_frame,
                  sizeof(before_object_change)) != 0);

    memcpy(before_object_change, fresh_game_frame, sizeof(before_object_change));
    m[0].amount = 0; /* Deplete/remove the visible mineral patch. */
    assert_cached_game_matches_fresh();
    assert(memcmp(before_object_change, fresh_game_frame,
                  sizeof(before_object_change)) != 0);

    minerals += 73; gas += 19;
    assert_cached_game_matches_fresh();

    /* Switch the building from visible to explored-only, then pan inward from
       the map edge while checking the cached result against a cleared redraw. */
    SC64_GRID_SET(visible, building_cell, 0);
    assert_cached_game_matches_fresh();
    cx += 16; cy += 16;
    assert_cached_game_matches_fresh();

    /* The explored bit also affects the tile's fog art independently of the
       building's own visibility. */
    SC64_GRID_SET(explored, building_cell, 0);
    assert_cached_game_matches_fresh();
}
#endif

#if defined(SC64_P3_RUNTIME) && defined(SC64_P3_ENABLED) && defined(SC64_UNIFIED)
/* Direct model-state render fixture: mission 22's sequence is part of the
   bottom-band cache key because its pending transmission changes the footer. */
static void
test_p3_sequence_invalidates_bottom_and_matches_redraw(void)
{
    assert(race_campaign.mission == SC64_P3_GLOBAL_ID);
    assert(race_p3.sequence == SC64_P3_SEQ_NONE);
    assert_cached_matches_full();

    race_p3.sequence = SC64_P3_SEQ_WIN_WAIT_2000;
    race_campaign.objective_stage = race_p3.sequence;
    expect_mask_and_match(HUD_BOTTOM);

    race_p3.sequence = SC64_P3_SEQ_WIN_TX_1;
    race_p3.transmission = 1;
    race_campaign.objective_stage = race_p3.sequence;
    race_campaign.message_ticks = 90;
    expect_mask_and_match(HUD_BOTTOM);
}
#endif

int
main(void)
{
    gba_init();
    fixture();
    test_wallet_and_queued_supply_invalidate_top_only();
    test_selection_building_box_and_modes_invalidate_bottom();
    test_mission_message_timer_and_minimap_stay_pixel_exact();
#if (defined(SC64_TERRAN_LATE_RUNTIME) || defined(SC64_HUD_BUFFER_REUSE)) && defined(SC64_NO_FRAME_CLEAR)
    test_per_buffer_hud_reuse_across_changes_and_scenes();
    test_full_frame_cache_does_not_retain_map_pixels_in_hud_bands();
#endif
#ifdef SC64_T5_RUNTIME
    test_t5_static_cache_key_and_hud_timer_are_pixel_exact();
#endif
#if defined(SC64_P3_RUNTIME) && defined(SC64_P3_ENABLED) && defined(SC64_UNIFIED)
    test_p3_sequence_invalidates_bottom_and_matches_redraw();
#endif
#ifdef SC64_RACES
    /* Keep the skirmish fixture last so it cannot change the campaign state
       expected by the older mission-message and mission-cache checks above. */
    test_skirmish_shield_only_changes_invalidate_cached_bottom();
#endif
    return 0;
}
