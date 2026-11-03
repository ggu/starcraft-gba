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

#ifdef SC64_FIELD_CONTROLS
    /* The imported icon follows raw identity, not just compact engine type. */
    b[0].raw_type=131;
    assert_cached_matches_full();
    b[0].raw_type=132;
    expect_mask_and_match(HUD_BOTTOM);
#endif

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

#ifdef SC64_HUD_PANEL
static int
hud_count_color(int x, int y, int w, int h, u8 color)
{
    int n = 0;
    for (int py = y; py < y + h; py++)
        for (int px = x; px < x + w; px++)
            n += gfx_test_pixel(px, py) == color;
    return n;
}

static int
panel_bar_fill(void)
{
    return hud_count_color(HUD_COLUMN, HUD_BAR_Y, HUD_BAR_W, 1, C_GOLD);
}

static int
panel_slot_lit(int i)
{
    int x = HUD_COLUMN + i * HUD_SLOT_PITCH;
    return hud_count_color(x + 1, HUD_SLOT_Y + 1, CARD_CELL - 2, CARD_CELL - 2, C_GREEN) +
        hud_count_color(x + 1, HUD_SLOT_Y + 1, CARD_CELL - 2, CARD_CELL - 2, C_GOLD);
}

/* A selected producer shows its queue as icons with a bar; the cached band
 * follows what the panel draws, in steps and only for real changes. */
static void
test_panel_shows_building_queue(void)
{
    Building *q = &b[0];
    int time = unit_build_time(U_SCV);
    fixture();
    race_human = R_TER;
    train_type = U_SCV;
    q->alive = 1; q->owner = SIDE; q->type = B_COMMAND;
    q->raw_type = sc64_race_raw_building(B_COMMAND); q->raw_owner = 7;
    q->hp = q->maxhp = 1500; q->x = q->y = 100;
    selected_building = 0;
    sc64_hud_valid = 0;
    render_cached();
    assert(hud_count_color(0, 121, 25, 26, C_GREEN) > 0);
    for (int i = 0; i < HUD_QUEUE_SLOTS; i++) assert(!panel_slot_lit(i));
    assert(panel_bar_fill() == 0);
    assert(hud_next_product(0) == U_SCV);
    assert(hud_count_color(HUD_CHIP_X, HUD_SLOT_Y, CARD_CELL, CARD_CELL, C_GREEN) > 0);
    assert(hud_count_color(HUD_CHIP_X + CARD_CELL, HUD_SLOT_Y, 20, 10, C_GOLD) > 0);

    q->queue[0] = q->queue[1] = q->queue[2] = U_SCV;
    q->queue_head = 0;
    q->queue_count = 3;
    expect_mask_and_match(HUD_TOP | HUD_BOTTOM);
    render_cached();
    assert(panel_slot_lit(0) && panel_slot_lit(1) && panel_slot_lit(2));
    assert(!panel_slot_lit(3) && !panel_slot_lit(4));
    assert(gfx_test_pixel(HUD_COLUMN, HUD_SLOT_Y) == C_GOLD);
    assert(gfx_test_pixel(HUD_COLUMN + HUD_SLOT_PITCH, HUD_SLOT_Y) != C_GOLD);

    /* Progress is drawn over the cached band, so it never redraws the band
       and still shows the current value on a cached frame. */
    render_cached();
    q->queue_progress = time / 2;
    expect_mask_and_match(0);
    render_cached();
    assert(panel_bar_fill() == HUD_BAR_W * (time / 2) / time);
    q->queue_progress = time / 4;
    expect_mask_and_match(0);
    render_cached();
    assert(panel_bar_fill() == HUD_BAR_W * (time / 4) / time);
    q->queue_progress = time / 2;

    /* A swap of a waiting unit keeps count and progress but must still redraw. */
    q->queue[2] = U_MARINE;
    assert(probe_change_mask() & HUD_BOTTOM);
    assert_cached_matches_full();

    /* Hit points step: real damage redraws the panel, a scratch does not. */
    q->hp = 1200;
    expect_mask_and_match(HUD_BOTTOM);
    render_cached();
    q->hp = 1199;
    assert(probe_change_mask() == 0);
    q->hp = 1200;

    /* The upgrade building shows both research slots and their progress. */
    memset(q, 0, sizeof *q);
    q->alive = 1; q->owner = SIDE; q->type = B_ENG_BAY;
    q->raw_type = sc64_race_raw_building(B_ENG_BAY); q->raw_owner = 7;
    q->hp = q->maxhp = 850; q->x = q->y = 100;
    weapons_research = armor_research = 0;
    sc64_hud_valid = 0;
    render_cached();
    assert(panel_slot_lit(0) && panel_slot_lit(1));
    assert(gfx_test_pixel(HUD_COLUMN, HUD_SLOT_Y) != C_GOLD);
    weapons_research = 300;
    expect_mask_and_match(HUD_BOTTOM);
    render_cached();
    assert(gfx_test_pixel(HUD_COLUMN, HUD_SLOT_Y) == C_GOLD);
    assert(hud_count_color(HUD_COLUMN, HUD_BAR_Y, CARD_CELL, 1, C_GOLD) == CARD_CELL / 2);
    weapons_research = 450;
    expect_mask_and_match(0);
    render_cached();
    assert(hud_count_color(HUD_COLUMN, HUD_BAR_Y, CARD_CELL, 1, C_GOLD) == CARD_CELL * 3 / 4);
    weapons_research = 0;
    weapons_level = 1;
    expect_mask_and_match(HUD_BOTTOM);
    weapons_level = 0;

    /* Under construction there is one long bar and no queue. */
    memset(q, 0, sizeof *q);
    q->alive = 1; q->owner = SIDE; q->type = B_BARRACKS;
    q->raw_type = sc64_race_raw_building(B_BARRACKS); q->raw_owner = 7;
    q->hp = q->maxhp = 1000; q->x = q->y = 100;
    q->under_construction = 1;
    q->build_progress = building_build_time(B_BARRACKS) / 2;
    sc64_hud_valid = 0;
    render_cached();
    assert(hud_count_color(HUD_COLUMN, HUD_BAR_Y, HUD_FULL_BAR_W, 1, C_GOLD) >= HUD_FULL_BAR_W / 2 - 6);
    assert(hud_next_product(0) < 0);
    q->build_progress += building_build_time(B_BARRACKS) / 4;
    expect_mask_and_match(0);
    render_cached();
    assert(hud_count_color(HUD_COLUMN, HUD_BAR_Y, HUD_FULL_BAR_W, 1, C_GOLD) >= HUD_FULL_BAR_W * 3 / 4 - 6);

    /* The bars stay off the band while a menu covers the panel. */
    q->under_construction = 0;
    menu_open = 1;
    sc64_hud_valid = 0;
    render_cached();
    assert(hud_count_color(HUD_COLUMN, HUD_BAR_Y, HUD_FULL_BAR_W, HUD_BAR_H, C_GOLD) == 0);
    menu_open = 0;
    selected_building = -1;
    memset(b, 0, sizeof(b));
}

/* A trains exactly the unit the panel offers. */
static void
test_panel_product_matches_a(void)
{
    static const struct { int race, type, stale_train, expected; } cases[] = {
        {R_TER, B_COMMAND, U_MARINE, U_SCV},
        {R_TER, B_BARRACKS, U_SCV, U_MARINE},
        {R_PROTOSS, B_NEXUS, U_ZEALOT, U_PROBE},
    };
    for (unsigned n = 0; n < sizeof cases / sizeof cases[0]; n++) {
        Building *q = &b[0];
        fixture();
        race_human = cases[n].race;
        if (race_owner_identity(SIDE) != cases[n].race) continue;
        minerals = 500;
        supply_cap = 60; supply_used = 0;
        q->alive = 1; q->owner = SIDE; q->type = cases[n].type;
        q->raw_type = sc64_race_raw_building(cases[n].type); q->raw_owner = 7;
        q->hp = q->maxhp = 1000; q->x = q->y = 100;
        selected_building = 0;
        cx = cy = 100;
        train_type = cases[n].stale_train;
        assert(hud_next_product(0) == cases[n].expected);
        race_refusal = 0;
        interact();
        /* A resolves to the offered unit; a profile's supply rules may still refuse it. */
        assert(train_type == cases[n].expected);
        if (!race_refusal) assert(q->queue_count == 1 && q->queue[q->queue_head] == cases[n].expected);
    }
    selected_building = -1;
    memset(b, 0, sizeof(b));
}

#ifdef SC64_SHARED_BUNKER
/* A Bunker shows its passengers as icons in four slots. */
static void
test_panel_bunker_cargo(void)
{
    Building *q = &b[0];
    fixture();
    race_human = R_TER;
    q->alive = 1; q->owner = SIDE; q->type = B_BUNKER;
    q->raw_type = 125; q->raw_owner = 7;
    q->hp = q->maxhp = 350; q->x = q->y = 100;
    selected_building = 0;
    sc64_hud_valid = 0;
    render_cached();
    assert(!panel_slot_lit(0));
    assert(gfx_test_pixel(HUD_COLUMN, HUD_SLOT_Y) == C_EDGE);
    u[0].alive = 1; u[0].owner = SIDE; u[0].type = U_MARINE;
    u[0].raw_type = sc64_race_raw_unit(U_MARINE); u[0].raw_owner = 7;
    u[0].hp = u[0].maxhp = 40;
    u[0].order = ORDER_TRANSPORTED;
    u[0].target = sc64_bunker_target_encode(MAX_UNITS, MAX_BUILDINGS, 0);
    assert(race_bunker_cargo_space(0) == 1);
    assert(probe_change_mask() & HUD_BOTTOM);
    assert_cached_matches_full();
    render_cached();
    assert(panel_slot_lit(0) && !panel_slot_lit(1));
    selected_building = -1;
    memset(b, 0, sizeof(b));
    memset(u, 0, sizeof(u));
}
#endif

/* Armed orders and the box show key caps and marks instead of words. */
static void
test_panel_modes_use_marks(void)
{
    fixture();
    race_human = R_TER;
    u[0].alive = 1; u[0].owner = SIDE; u[0].selected = 1; u[0].type = U_MARINE;
    u[0].raw_type = sc64_race_raw_unit(U_MARINE); u[0].raw_owner = 7;
    u[0].hp = u[0].maxhp = 40;
    mode = MODE_MOVE;
    sc64_hud_valid = 0;
    render_cached();
    assert(hud_count_color(27, 136, 10, 11, C_SILVER) > 0);
    assert(hud_count_color(27, 136, 150, 9, C_GOLD) == 0);
    mode = MODE_ATTACK;
    sc64_hud_valid = 0;
    render_cached();
    assert(hud_count_color(27, 136, 60, 11, C_RED) > 0);
    assert(hud_count_color(27, 136, 150, 9, C_GOLD) == 0);
    mode = MODE_NORMAL;
    box_state = 2;
    sc64_hud_valid = 0;
    render_cached();
    assert(hud_count_color(5, 136, 150, 9, C_GOLD) == 0);
    box_state = 0;
    memset(u, 0, sizeof(u));
}

/* A timed mission keeps its clock under the portrait while a unit is selected. */
#ifdef SC64_T3_ENABLED
static void
test_panel_keeps_mission_clock(void)
{
    fixture();
    race_human = R_TER;
    race_selected_mission = 6;
    race_t3.remaining_ticks = 300;
    u[0].alive = 1; u[0].owner = SIDE; u[0].selected = 1; u[0].type = U_MARINE;
    u[0].raw_type = sc64_race_raw_unit(U_MARINE); u[0].raw_owner = 7;
    u[0].hp = u[0].maxhp = 40;
    sc64_hud_valid = 0;
    render_cached();
    assert(hud_count_color(0, HUD_CLOCK_Y, 30, 8, C_WHITE) > 0);
    memset(u, 0, sizeof(u));
}
#endif

/* A group is its unit types with counts; a selection hides the objective. */
static void
test_panel_group_and_objective(void)
{
    fixture();
    race_human = R_TER;
    sc64_hud_valid = 0;
    render_cached();
    assert(hud_count_color(5, 124, 170, 18, C_GOLD) > 0);
    for (int i = 0; i < 3; i++) {
        u[i].alive = 1; u[i].owner = SIDE; u[i].selected = 1;
        u[i].type = i < 2 ? U_MARINE : U_FIREBAT;
        u[i].raw_type = sc64_race_raw_unit(u[i].type); u[i].raw_owner = 7;
        u[i].hp = u[i].maxhp = 40;
    }
    expect_mask_and_match(HUD_BOTTOM);
    render_cached();
    assert(hud_count_color(5, 148, 175, 11, C_GOLD) == 0);
    assert(hud_count_color(HUD_COLUMN, HUD_PORTRAIT_Y, CARD_CELL, CARD_CELL, C_GREEN) > 0);
    assert(hud_count_color(HUD_COLUMN + HUD_SLOT_PITCH, HUD_PORTRAIT_Y, CARD_CELL, CARD_CELL, C_GREEN) > 0);
    assert(hud_count_color(HUD_COLUMN + 2 * HUD_SLOT_PITCH, HUD_PORTRAIT_Y, CARD_CELL, CARD_CELL, C_GREEN) == 0);
    /* The group's bar steps; a small hit stays within a step and costs no redraw. */
    u[0].hp = 38;
    expect_mask_and_match(HUD_BOTTOM);
    u[0].hp = 37;
    expect_mask_and_match(0);
    u[0].hp = 10;
    expect_mask_and_match(HUD_BOTTOM);
    u[2].selected = 0;
    expect_mask_and_match(HUD_BOTTOM);
    memset(u, 0, sizeof(u));
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

#ifdef SC64_HUD_ICONS
/* The top bar draws the N64 mineral and gas icons and each race's supply
   icon in place of the MIN, GAS and SUP labels. */
static void
test_top_bar_draws_resource_icons(void)
{
    static const int slots[3] = {2, 60, 126};
    int saved_race = race_human;
    for (int race = 0; race < 3; ++race) {
        race_human = race;
        gfx_begin(C_BLACK);
        draw_hud_resources();
        const int sprites[3] = {SC64_HUD_SPRITE_MINERAL, SC64_HUD_SPRITE_GAS, card_supply_sprite()};
        assert(sprites[2] == SC64_HUD_SPRITE_SUPPLY_TERRAN + race);
        for (int k = 0; k < 3; ++k) {
            int w = sc64_hud_sprite_size[sprites[k]][0], h = sc64_hud_sprite_size[sprites[k]][1];
            int x0 = slots[k] + (12 - w) / 2, y0 = (12 - h) / 2, opaque = 0;
            const unsigned char *px = sc64_hud_sprite_pixels + sc64_hud_sprite_offset[sprites[k]];
            for (int yy = 0; yy < h; ++yy)
                for (int xx = 0; xx < w; ++xx)
                    if (px[yy * w + xx]) {
                        ++opaque;
                        assert(gfx_test_pixel(x0 + xx, y0 + yy) == px[yy * w + xx]);
                    }
            assert(opaque > 0);
        }
        for (int y = 0; y < 12; ++y)
            for (int x = 0; x < 121; ++x)
                assert(gfx_test_pixel(x, y) != (x < 60 ? C_GOLD : x > 60 ? C_CYAN : C_BLACK));
    }
    race_human = saved_race;
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

#if defined(SC64_SHOWCASE_ENABLED) && defined(SC64_HUD_PAGE_BANDS)
static void test_showcase_hud_transitions_match_forced_frames(void)
{
    fixture();
    /* Keep minimap dots within the map so they do not independently
     * invalidate the bottom band and mask a stale-label failure. */
    memset(u, 0, sizeof(u));
    memset(b, 0, sizeof(b));
    screen = SCREEN_PLAY;
    gfx_begin(0xe7); gfx_end();
    gfx_begin(0xe7); gfx_end();
    assert_game_frame_matches_forced_hud();
    minerals += 37; gas += 11;
    assert_game_frame_matches_forced_hud();
    u[0].alive=1; u[0].owner=SIDE; u[0].type=U_MARINE;
    u[0].hp=u[0].maxhp=40; u[0].selected=1;
    assert_game_frame_matches_forced_hud();
    menu_open=1; menu_cursor=1;
    assert_game_frame_matches_forced_hud();
    menu_cursor=2;
    assert_game_frame_matches_forced_hud();
    menu_open=0;
    assert_game_frame_matches_forced_hud();
    race_campaign.message=11; race_campaign.message_ticks=90;
    assert_game_frame_matches_forced_hud();
    race_campaign.message_ticks=0;
    assert_game_frame_matches_forced_hud();
    sc64_minimap_visible[7]=1; sc64_minimap_raster_update(7);
    assert_game_frame_matches_forced_hud();
    sc64_minimap_visible[7]=0; sc64_minimap_explored[7]=1;
    sc64_minimap_raster_update(7);
    assert_game_frame_matches_forced_hud();
    sc64_minimap_explored[7]=0; sc64_minimap_raster_update(7);
    assert_game_frame_matches_forced_hud();
    screen=SCREEN_PAUSE;
    assert_game_frame_matches_forced_hud();
    screen=SCREEN_PLAY;
    assert_game_frame_matches_forced_hud();
    screen=SCREEN_RESULT;
    assert_game_frame_matches_forced_hud();
    screen=SCREEN_PLAY;
    assert_game_frame_matches_forced_hud();
    gfx_begin(0xe7); gfx_end();
    gfx_begin(0xe7); gfx_end();
    assert_game_frame_matches_forced_hud();
    assert_game_frame_matches_forced_hud();
}

static void test_showcase_labels_do_not_persist_in_cached_pages(void)
{
    fixture();
    /* Keep minimap dots within the map so they do not independently
     * invalidate the bottom band and mask a stale-label failure. */
    memset(u, 0, sizeof(u));
    memset(b, 0, sizeof(b));
    screen = SCREEN_PLAY;
    gfx_begin(0xe7); gfx_end();
    gfx_begin(0xe7); gfx_end();
    sc64_showcase_scene = 0;
    game_render();
    assert(!page_band_current(gfx_backbuffer_index(), 1));
    game_render();
    assert(!page_band_current(gfx_backbuffer_index(), 1));
    sc64_showcase_scene = -1;
    assert_game_frame_matches_forced_hud();
    assert_game_frame_matches_forced_hud();
    sc64_showcase_scene = 2;
    game_render();
    assert(!page_band_current(gfx_backbuffer_index(), 1));
    game_render();
    assert(!page_band_current(gfx_backbuffer_index(), 1));
    sc64_showcase_scene = -1;
    assert_game_frame_matches_forced_hud();
    assert_game_frame_matches_forced_hud();
}
#endif

int
main(void)
{
    gba_init();
    fixture();
#if defined(SC64_SHOWCASE_ENABLED) && defined(SC64_HUD_PAGE_BANDS)
    test_showcase_labels_do_not_persist_in_cached_pages();
    test_showcase_hud_transitions_match_forced_frames();
    fixture();
#endif
#ifdef SC64_HUD_ICONS
    test_top_bar_draws_resource_icons();
#endif
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
#ifdef SC64_HUD_PANEL
    test_panel_shows_building_queue();
    test_panel_product_matches_a();
    test_panel_group_and_objective();
    test_panel_modes_use_marks();
#ifdef SC64_SHARED_BUNKER
    test_panel_bunker_cargo();
#endif
#ifdef SC64_T3_ENABLED
    test_panel_keeps_mission_clock();
#endif
#endif
#ifdef SC64_RACES
    /* Keep the skirmish fixture last so it cannot change the campaign state
       expected by the older mission-message and mission-cache checks above. */
    test_skirmish_shield_only_changes_invalidate_cached_bottom();
#endif
    return 0;
}
