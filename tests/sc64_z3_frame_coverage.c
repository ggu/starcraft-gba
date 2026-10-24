/* Pixel-coverage oracle for race renderers with and without frame clearing.
 * Private generated map/art headers are inputs at build time, never embedded
 * in this fixture. It tests rendering only, not controller completion. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

enum { FB_WIDTH = 240, FB_HEIGHT = 160, FB_SIZE = FB_WIDTH * FB_HEIGHT };
static u8 reference_pixels[FB_SIZE];
static u8 candidate_pixels[FB_SIZE];
static u8 transition_references[7][FB_SIZE];

static void
capture(u8 *out)
{
    for (int y = 0; y < FB_HEIGHT; ++y)
        for (int x = 0; x < FB_WIDTH; ++x)
            out[y * FB_WIDTH + x] = gfx_test_pixel(x, y);
}

static void
reset_render_caches(void)
{
    sc64_terrain_cache_valid = 0;
    sc64_terrain_cache_reused_base = 0;
    sc64_hud_valid = 0;
}

static void
draw_scene_contents(void)
{
    if (screen == SCREEN_TITLE) {
        draw_title();
        return;
    }
    draw_map();
    draw_hud();
    if (screen == SCREEN_PAUSE) draw_pause();
    else if (screen == SCREEN_RESULT) draw_result();
}

static void
draw_full_clear_reference(u8 *out)
{
    reset_render_caches();
    gfx_begin(C_BLACK);
    draw_scene_contents();
    gfx_end();
    capture(out);
}

static void
poison_framebuffer_page(int phase)
{
    u8 first = phase ? C_CYAN : C_ORANGE;
    u8 second = phase ? C_RED : C_BLUE;
    gfx_begin(C_BLACK);
    for (int y = 0; y < FB_HEIGHT; ++y)
        for (int x = 0; x < FB_WIDTH; ++x)
            gfx_pixel(x, y, ((x ^ y) & 1) ? first : second);
    gfx_end();
}

static void
poison_both_pages(void)
{
    poison_framebuffer_page(0);
    poison_framebuffer_page(1);
}

static int
compare_game_render(const char *name, int warm_cache)
{
    draw_full_clear_reference(reference_pixels);
    if (!warm_cache) reset_render_caches();
    poison_both_pages();
    game_render();
    capture(candidate_pixels);
    if (memcmp(reference_pixels, candidate_pixels, FB_SIZE) == 0) {
        printf("%s: pixel-identical (%s cache)\n", name,
               warm_cache ? "warm" : "cold");
        return 0;
    }
    for (int i = 0; i < FB_SIZE; ++i) {
        if (reference_pixels[i] != candidate_pixels[i]) {
            fprintf(stderr, "%s: first mismatch at (%d,%d): %u vs %u\n",
                    name, i % FB_WIDTH, i / FB_WIDTH,
                    reference_pixels[i], candidate_pixels[i]);
            break;
        }
    }
    return 1;
}

static void
setup_profile(void)
{
    game_init();
    race_mode = 0;
#if defined(SC64_Z6_RUNTIME)
    race_human = R_ZERG;
    race_selected_mission = SC64_Z6_GLOBAL_ID;
#elif defined(SC64_P6_RUNTIME)
    race_human = R_PROTOSS;
    race_selected_mission = SC64_P6_GLOBAL_ID;
#elif defined(SC64_P5_RUNTIME)
    race_human = R_PROTOSS;
    race_selected_mission = SC64_P5_GLOBAL_ID;
#elif defined(SC64_P4_RUNTIME)
    race_human = R_PROTOSS;
    race_selected_mission = SC64_P4_GLOBAL_ID;
#else
    race_human = R_ZERG;
#ifdef SC64_Z5_RUNTIME
    race_selected_mission = SC64_Z5_GLOBAL_ID;
#elif defined(SC64_Z4_RUNTIME)
    race_selected_mission = 15;
#else
    race_selected_mission = 14;
#endif
#endif
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
test_construction_cache_transitions(void)
{
    const int types[]={B_BARRACKS,B_HATCHERY,B_NEXUS};
    int failures=0;
    for (unsigned race=0;race<3;++race) {
        setup_profile();
        int slot=spawn_build(SIDE,types[race],512,512,1);
        assert(slot>=0);
        int duration=building_build_time(types[race]);
        for (unsigned pass=0;pass<2;++pass) {
            if (pass) { reset_render_caches(); poison_both_pages(); }
            for (unsigned state=0;state<6;++state) {
                Building *q=&b[slot];
                q->alive=state!=5; q->under_construction=state<4;
                q->build_progress=state==0?-1:(int)(state-1u)*duration/3;
                frame=state*4u;
                if (!pass) draw_full_clear_reference(transition_references[state]);
                else {
                    game_render(); capture(candidate_pixels);
                    if (memcmp(transition_references[state],candidate_pixels,FB_SIZE)) {
                        fprintf(stderr,"construction race%u state%u differs with reused terrain\n",
                                race,state);
                        ++failures;
                    }
                }
            }
        }
    }
    return failures;
}

static int
test_camera_fog_and_overlays(void)
{
    int failures = 0;
    char name[64];
    const int cameras[][2] = {
        {120, 54}, {WORLD_W - 120, 54}, {120, WORLD_H - 54},
        {WORLD_W - 120, WORLD_H - 54}, {263, 187}
    };
    setup_profile();
    for (int i = 0; i < MAX_MINERALS; ++i)
        if (m[i].amount > 0)
            SC64_GRID_SET(explored, (m[i].x / 16) + (m[i].y / 16) * MAP_COLS, 1);
    update_exploration();

    for (unsigned i = 0; i < sizeof(cameras) / sizeof(cameras[0]); ++i) {
        cx = cameras[i][0];
        cy = cameras[i][1];
        snprintf(name, sizeof(name), "camera %u", i);
        failures += compare_game_render(name, 0);
        failures += compare_game_render(name, 1);
    }

    cx = cy = 512;
    SC64_GRID_CLEAR(visible);
    SC64_GRID_CLEAR(explored);
    for (int y = 20; y < 36; ++y)
        for (int x = 18; x < 42; ++x)
            SC64_GRID_SET(explored, x + y * MAP_COLS, 1);
    failures += compare_game_render("fog", 0);

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
    failures += compare_game_render("build, selection, overlays", 1);
    return failures;
}

static int
test_scene_transitions(void)
{
    int failures = 0;
    static const struct { int screen, won, lost; const char *name; } cases[] = {
        {SCREEN_PLAY, 0, 0, "live"},
        {SCREEN_PAUSE, 0, 0, "pause"},
        {SCREEN_RESULT, 1, 0, "win result"},
        {SCREEN_RESULT, 0, 1, "loss result"},
        {SCREEN_TITLE, 0, 0, "home/title"},
        {SCREEN_PLAY, 0, 0, "live again"},
        {SCREEN_PAUSE, 0, 0, "pause again"}
    };
    setup_profile();
    for (unsigned i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        screen = cases[i].screen; won = cases[i].won; lost = cases[i].lost;
        draw_full_clear_reference(transition_references[i]);
    }
    poison_both_pages();
    reset_render_caches();
    for (unsigned i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        screen = cases[i].screen; won = cases[i].won; lost = cases[i].lost;
        game_render();
        capture(candidate_pixels);
        if (memcmp(transition_references[i], candidate_pixels, FB_SIZE) == 0)
            printf("transition %s: pixel-identical\n", cases[i].name);
        else {
            fprintf(stderr, "transition %s: stale pixels\n", cases[i].name);
            ++failures;
        }
    }
    return failures;
}

static int
test_mutation_save_load_and_restart(void)
{
    int mineral_slot = -1, building_slot = -1;
    int failures = 0;
    Mineral saved_mineral;
    Building saved_building;
    int has_mineral;
    setup_profile();
    update_exploration();
    for (int i = 0; i < MAX_MINERALS; ++i)
        if (m[i].amount > 0) { mineral_slot = i; break; }
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive) { building_slot = i; break; }
    assert(building_slot >= 0);
    has_mineral = mineral_slot >= 0;
    saved_building = b[building_slot];
    if (has_mineral) saved_mineral = m[mineral_slot];

    /* Mutate map actors after the initial frame/cache has been drawn. */
    game_render();
    if (has_mineral) m[mineral_slot].amount = 0;
    b[building_slot].alive = 0;
    failures += compare_game_render(has_mineral ? "resource/building mutation" :
                                    "building mutation", 0);

    /* Restore the complete source actor before saving. This matters for P6
     * beacons and unit-backed THG2 props whose identities are immutable. */
    b[building_slot] = saved_building;
    if (has_mineral) m[mineral_slot] = saved_mineral;
    assert(race_save_game());
    if (has_mineral) m[mineral_slot].amount = 0;
    b[building_slot].alive = 0;
    assert(race_load_game());
    failures += compare_game_render("save/load", 0);

    setup_profile();
    failures += compare_game_render("restart", 0);
    return failures;
}

#if defined(SC64_P6_RUNTIME)
static int
test_p6_doodad_action_cold_warm(void)
{
    u8 enabled[sizeof(race_p6.doodad_enabled)];
    Sc64P6DoodadBinding saved_bindings[SC64_P6_DOODAD_SOURCE_COUNT];
    int applied = 0, failures = 0;
    setup_profile();
    memcpy(enabled, race_p6.doodad_enabled, sizeof(enabled));
    memcpy(saved_bindings, race_p6_doodad_bindings, sizeof(saved_bindings));
    for (unsigned i = 0; i < SC64_P6_SOURCE_ACTION_COUNT; ++i) {
        const Sc64P6SourceAction *action = &sc64_p6_source_actions[i];
        if (action->opcode != SC64_P6_ACTION_SET_DOODAD) continue;
        if (sc64_p6_world_action(&race_p6_world, action) == 1) {
            applied = 1;
            break;
        }
    }
    assert(applied);
    failures += compare_game_render("P6 source doodad action cold", 0);
    failures += compare_game_render("P6 source doodad action warm", 1);
    memcpy(race_p6.doodad_enabled, enabled, sizeof(enabled));
    memcpy(race_p6_doodad_bindings, saved_bindings, sizeof(saved_bindings));
    invalidate_path_grid();
    target_lists_valid = 0;
    sc64_unit_bucket_active = 0;
    sc64_target_index_end();
    reset_render_caches();
    return failures;
}
#endif

static int
test_consecutive_viewport_edge_frames(void)
{
    int failures = 0;
    setup_profile();
    frame = 600;
    camx = viewport_left(); camy = viewport_top();
    u[0].alive = 1; u[0].owner = SIDE; u[0].type = U_ZERGLING;
    u[0].raw_type = 37; u[0].hp = 8; u[0].maxhp = 35; u[0].selected = 1;
    u[0].x = camx + 174; u[0].y = camy - 2;
    u[1].alive = 1; u[1].owner = SIDE; u[1].type = U_ZERGLING;
    u[1].raw_type = 37; u[1].hp = 32; u[1].maxhp = 35;
    u[1].x = camx + 198; u[1].y = camy + 113;
    tracers[0].life = 2; tracers[0].color = C_GOLD;
    tracers[0].x0 = camx + 165; tracers[0].y0 = camy - 2;
    tracers[0].x1 = camx + 205; tracers[0].y1 = camy + 112;
    draw_full_clear_reference(reference_pixels);
    poison_both_pages();
    reset_render_caches();
    /* Keep both pages warm without a new clear or reference render between
     * captures. Otherwise clear-generation invalidation masks stale bands. */
    for (int i = 0; i < 8; ++i) {
        game_render(); capture(candidate_pixels);
        if (memcmp(reference_pixels, candidate_pixels, FB_SIZE) != 0) {
            fprintf(stderr, "consecutive viewport-edge frame %d differs\n", i);
            ++failures;
        }
    }
    return failures;
}

int
main(void)
{
    int failures;
    gba_init();
    failures = test_camera_fog_and_overlays();
    failures += test_scene_transitions();
    failures += test_mutation_save_load_and_restart();
    failures += test_construction_cache_transitions();
    failures += test_consecutive_viewport_edge_frames();
#if defined(SC64_P6_RUNTIME)
    failures += test_p6_doodad_action_cold_warm();
#endif
    if (failures) fprintf(stderr, "Race no-clear coverage failures: %d\n", failures);
    else puts("Race framebuffer coverage: all full-clear comparisons match");
    return failures != 0;
}
