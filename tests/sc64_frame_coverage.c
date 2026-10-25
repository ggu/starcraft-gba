/* Exploratory full-frame coverage oracle for imported race scenes.
 * Directly invokes the production draw routines after gfx_begin() supplies
 * either black or a poison color. This is a render fixture, not a gameplay
 * or controller-completion test. */
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

enum { FB_WIDTH = 240, FB_HEIGHT = 160, FB_SIZE = FB_WIDTH * FB_HEIGHT };
static u8 reference_pixels[FB_SIZE];
static u8 candidate_pixels[FB_SIZE];
static void setup_campaign(int mission);
static u8 transition_references[8][FB_SIZE];

static void
capture(u8 *out)
{
    for (int y = 0; y < FB_HEIGHT; ++y)
        for (int x = 0; x < FB_WIDTH; ++x)
            out[y * FB_WIDTH + x] = gfx_test_pixel(x, y);
}

static void
poison_framebuffer(void)
{
    gfx_begin(C_PURPLE);
    for (int y = 0; y < FB_HEIGHT; ++y)
        for (int x = 0; x < FB_WIDTH; ++x)
            gfx_pixel(x, y, ((x ^ y) & 1) ? C_ORANGE : C_BLUE);
}

static void
reset_render_caches(void)
{
#ifdef SC64_IMPORTED_MISSION
    sc64_terrain_cache_valid = 0;
    sc64_terrain_cache_reused_base = 0;
#endif
    sc64_hud_valid = 0;
}

static void
draw_active_scene(void)
{
    draw_map();
    draw_hud();
    if (screen == SCREEN_PAUSE) draw_pause();
    else if (screen == SCREEN_RESULT) draw_result_screen(); /* production dispatch, including the outro */
}

static void
draw_scene_contents(void)
{
    if (screen == SCREEN_TITLE) draw_title();
    else draw_active_scene();
}

static int
render_clear_reference(u8 *out)
{
    reset_render_caches();
    gfx_begin(C_BLACK);
    draw_scene_contents();
    gfx_end();
    capture(out);
    return 1;
}

static u8
poison_page(int phase)
{
    u8 first = phase ? C_CYAN : C_ORANGE;
    u8 second = phase ? C_RED : C_BLUE;
    gfx_begin(C_BLACK);
    for (int y = 0; y < FB_HEIGHT; ++y)
        for (int x = 0; x < FB_WIDTH; ++x)
            gfx_pixel(x, y, ((x ^ y) & 1) ? first : second);
    gfx_end();
    return second;
}

static int
poison_both_pages_and_check(void)
{
    u8 back_page = poison_page(0);
    u8 front_page = poison_page(1);
    /* gfx_begin_preserve selects the page opposite the current front. Flip
       without writing so each distinct page can be checked through the same
       public host pixel reader. */
    gfx_begin_preserve();
    int ok = gfx_test_pixel(0, 0) == back_page;
    gfx_end();
    gfx_begin_preserve();
    ok &= gfx_test_pixel(0, 0) == front_page;
    gfx_end();
    return ok;
}

static int
compare_game_render_after_poison(const char *name)
{
    int mismatches = 0;
    render_clear_reference(reference_pixels);
    if (!poison_both_pages_and_check()) {
        fprintf(stderr, "framebuffer page alternation failed before %s\n", name);
        return 1;
    }
    /* The reference render has warmed production caches. Poisoning the two
       pages does not alter their cache keys, so this checks the actual warm
       game_render path rather than only drawing isolated routines. */
    game_render();
    capture(candidate_pixels);
    for (int i = 0; i < FB_SIZE; ++i)
        if (reference_pixels[i] != candidate_pixels[i]) ++mismatches;
    if (mismatches)
        printf("game_render %-24s stale_pixels=%d\n", name, mismatches);
    else
        printf("game_render %-24s matches clear reference\n", name);
    return mismatches != 0;
}

static void
set_result_outro(int playing)
{
    result_outro_end = playing ? 0 : SC64_RESULT_OUTRO_MAX;
    result_outro = playing ? 10 : SC64_RESULT_OUTRO_MAX + SC64_RESULT_PANEL_OPEN;
}

static int
run_game_render_transitions(void)
{
    int failures = 0;
    /* outro=1 renders mid-outro: battlefield only, panel not yet open. */
    static const struct { int screen, won, lost, outro; const char *name; } cases[] = {
        {SCREEN_PLAY, 0, 0, 0, "live"},
        {SCREEN_PAUSE, 0, 0, 0, "pause"},
        {SCREEN_RESULT, 1, 0, 1, "win outro"},
        {SCREEN_RESULT, 1, 0, 0, "win result"},
        {SCREEN_RESULT, 0, 1, 0, "loss result"},
        {SCREEN_TITLE, 0, 0, 0, "title"},
        {SCREEN_PLAY, 0, 0, 0, "live again"},
        {SCREEN_PAUSE, 0, 0, 0, "pause again"}
    };
    setup_campaign(SC64_P3_GLOBAL_ID);
    /* Store independently cleared references first, then run the real
       game_render sequence without clearing between transitions. The two
       buffers therefore contain stale output from preceding scenes. */
    for (unsigned i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        screen = cases[i].screen; won = cases[i].won; lost = cases[i].lost;
        set_result_outro(cases[i].outro);
        render_clear_reference(transition_references[i]);
    }
    if (!poison_both_pages_and_check()) {
        fprintf(stderr, "framebuffer page alternation failed before transitions\n");
        return 1;
    }
    reset_render_caches();
    for (unsigned i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        screen = cases[i].screen; won = cases[i].won; lost = cases[i].lost;
        set_result_outro(cases[i].outro);
        game_render();
        capture(candidate_pixels);
        int stale = memcmp(transition_references[i], candidate_pixels,
                           FB_SIZE) != 0;
        if (stale)
            printf("game_render transition %-18s stale pixels\n", cases[i].name);
        else
            printf("game_render transition %-18s matches clear reference\n",
                   cases[i].name);
        failures += stale;
    }
    return failures;
}

static int
compare_scene(const char *name, int warm_cache)
{
    int mismatches = 0;
    int first_x = -1, first_y = -1;

    reset_render_caches();
    if (warm_cache) {
        gfx_begin(C_BLACK);
        draw_active_scene();
        capture(reference_pixels);
    } else {
        gfx_begin(C_BLACK);
        draw_active_scene();
        capture(reference_pixels);
    }

    /* Match the terrain/HUD cache state while starting the target framebuffer
       with a nonblack pattern. A second warm draw exercises cache hits. */
    reset_render_caches();
    if (warm_cache) {
        gfx_begin(C_BLACK);
        draw_active_scene();
    }
    poison_framebuffer();
    draw_active_scene();
    capture(candidate_pixels);

    for (int y = 0; y < FB_HEIGHT; ++y) {
        for (int x = 0; x < FB_WIDTH; ++x) {
            int index = y * FB_WIDTH + x;
            if (reference_pixels[index] != candidate_pixels[index]) {
                if (first_x < 0) { first_x = x; first_y = y; }
                ++mismatches;
            }
        }
    }
    if (mismatches)
        printf("coverage %-30s cache=%s uncovered=%d first=(%d,%d) ref=%u poison=%u\n",
               name, warm_cache ? "warm" : "cold", mismatches, first_x,
               first_y, reference_pixels[first_y * FB_WIDTH + first_x],
               candidate_pixels[first_y * FB_WIDTH + first_x]);
    else
        printf("coverage %-30s cache=%s complete\n", name,
               warm_cache ? "warm" : "cold");
    return mismatches;
}

static void
setup_campaign(int mission)
{
    game_init();
    race_mode = 0;
    race_human = R_PROTOSS;
    race_selected_mission = mission;
    init_state();
    screen = SCREEN_PLAY;
    mode = MODE_NORMAL;
    menu_open = 0;
    box_state = 0;
    selected_building = -1;
    race_campaign.phase = SC64_RACE_PHASE_ACTIVE;
    race_campaign.message = 0;
    race_campaign.message_ticks = 0;
    toast_timer = 0;
    cx = cy = 512;
    camx = camy = 0;
}

static int
run_campaign_scene_set(int mission, const char *label)
{
    char name[64];
    int mismatches = 0;
    setup_campaign(mission);
    for (int i = 0; i < MAX_MINERALS; ++i)
        if (m[i].amount > 0) SC64_GRID_SET(explored,
            (m[i].x / 16) + (m[i].y / 16) * MAP_COLS, 1);
    update_exploration();

    /* Corners plus sub-tile offsets exercise clipping and camera alignment. */
    const int cameras[][2] = {
        {120, 54}, {WORLD_W - 120, 54}, {120, WORLD_H - 54},
        {WORLD_W - 120, WORLD_H - 54}, {263, 187}
    };
    for (unsigned c = 0; c < sizeof(cameras) / sizeof(cameras[0]); ++c) {
        cx = cameras[c][0]; cy = cameras[c][1];
        snprintf(name, sizeof(name), "%s camera%u", label, c);
        mismatches += compare_scene(name, 0) != 0;
        mismatches += compare_scene(name, 1) != 0;
    }

    /* Fog-state changes and interactive overlays are direct render fixtures. */
    cx = 512; cy = 512;
    SC64_GRID_CLEAR(visible);
    SC64_GRID_CLEAR(explored);
    for (int y = 20; y < 36; ++y)
        for (int x = 18; x < 42; ++x)
            SC64_GRID_SET(explored, x + y * MAP_COLS, 1);
    snprintf(name, sizeof(name), "%s fog", label);
    mismatches += compare_scene(name, 0) != 0;

    SC64_GRID_CLEAR(visible);
    update_exploration();
    u[0].selected = 1;
    mode = MODE_BUILD;
    box_state = 2;
    box_anchor_x = cx - 24; box_anchor_y = cy - 18;
    box_end_x = cx + 35; box_end_y = cy + 29;
    menu_open = 1;
    menu_cursor = 3;
    toast_timer = 20; toast_id = 3;
    race_campaign.message = 1;
    race_campaign.message_ticks = 45;
    snprintf(name, sizeof(name), "%s overlays", label);
    mismatches += compare_scene(name, 1) != 0;

    screen = SCREEN_PAUSE;
    snprintf(name, sizeof(name), "%s pause", label);
    mismatches += compare_scene(name, 1) != 0;
    screen = SCREEN_RESULT;
    won = 1; lost = 0;
    snprintf(name, sizeof(name), "%s won result", label);
    mismatches += compare_scene(name, 1) != 0;
    won = 0; lost = 1;
    snprintf(name, sizeof(name), "%s lost result", label);
    mismatches += compare_scene(name, 1) != 0;
    return mismatches;
}

static int
run_skirmish_scene(void)
{
    int mismatches = 0;
    game_init();
    race_mode = 1;
    race_human = R_PROTOSS;
    race_skirmish_map = 0;
    init_state();
    screen = SCREEN_PLAY;
    cx = 512; cy = 512;
    update_exploration();
    mismatches += compare_scene("skirmish compact", 0) != 0;
    mismatches += compare_scene("skirmish compact", 1) != 0;
    return mismatches;
}

int
main(void)
{
    int scenes = 0;
    gba_init();
#if defined(SC64_P3_RUNTIME) && defined(SC64_P3_ENABLED) && defined(SC64_UNIFIED)
    scenes += run_campaign_scene_set(2, "P1") + run_campaign_scene_set(3, "P2") +
              run_campaign_scene_set(SC64_P3_GLOBAL_ID, "P3") + run_skirmish_scene();
    scenes += run_game_render_transitions();
#else
    scenes += run_campaign_scene_set(2, "P1") + run_campaign_scene_set(3, "P2") +
              run_skirmish_scene();
#endif
    printf("frame coverage oracle: %d scene groups had uncovered pixels; see per-scene counts above\n",
           scenes);
    return scenes != 0;
}
