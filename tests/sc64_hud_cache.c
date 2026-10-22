/* Pixel differential coverage for the imported HUD's split-band cache. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

enum { HUD_TOP = 1, HUD_BOTTOM = 2, FB_PIXELS = 240 * 160 };
static u8 cached_pixels[FB_PIXELS];
static u8 full_pixels[FB_PIXELS];

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
    int previous[24];
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
    assert_cached_matches_full();
    sc64_minimap_visible[0] = 0;
    assert_cached_matches_full();
}

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
#if defined(SC64_P3_RUNTIME) && defined(SC64_P3_ENABLED) && defined(SC64_UNIFIED)
    test_p3_sequence_invalidates_bottom_and_matches_redraw();
#endif
    return 0;
}
