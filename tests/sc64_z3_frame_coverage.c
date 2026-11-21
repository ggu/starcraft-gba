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
#ifdef SC_FIELD_FEEDBACK
    field_ui_refresh(); /* as game_render does before drawing the scene */
#endif
    draw_map();
    draw_hud();
    if (screen == SCREEN_PAUSE) draw_pause();
    else if (screen == SCREEN_RESULT) draw_result_screen(); /* production dispatch, including the outro */
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
#if defined(SC64_Z7_RUNTIME)
    race_human = R_ZERG;
    race_selected_mission = SC64_Z7_GLOBAL_ID;
#elif defined(SC64_Z6_RUNTIME)
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
#elif defined(SC64_TERRAN_FULL_CAMPAIGN_ENTRY)
    race_human = R_TER;
    race_selected_mission = 0;
#elif defined(SC64_P3_RUNTIME)
    race_human = R_PROTOSS;
    race_selected_mission = SC64_P3_GLOBAL_ID;
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

#if defined(SC64_WORLD_DEPTH) && defined(SC64_TERRAN_FULL_CAMPAIGN_ENTRY)
typedef struct {
    int type, y, building, selected, airborne;
} DepthActor;

static void
depth_spawn(DepthActor actor)
{
    if (actor.type < 0) return;
    int slot = actor.building ? spawn_build(SIDE, actor.type, 96, actor.y, 0)
                              : spawn_unit(SIDE, actor.type, 96, actor.y);
    assert(slot >= 0);
    if (!actor.building) u[slot].selected = actor.selected;
#ifdef SC64_BATTLECRUISER_FLIGHT_EXPERIMENT
    if (actor.airborne) {
        assert(actor.building);
        assert(race_bc_facility_lift(slot));
    }
#else
    assert(!actor.airborne);
#endif
}

static void
depth_capture_world(u8 *out)
{
    reset_render_caches();
    gfx_begin(C_BLACK);
    gfx_set_clip_y(12, 120);
    draw_map();
    assert(camx == 0 && camy == 0);
    capture(out);
    gfx_end();
}

static void
depth_capture(DepthActor first, DepthActor second, u8 *out)
{
    memset(u, 0, sizeof(u));
    memset(b, 0, sizeof(b));
    depth_spawn(first);
    depth_spawn(second);
    depth_capture_world(out);
}

static void
setup_depth_profile(int human_race)
{
    game_init();
    race_mode = 1;
    race_human = human_race;
    race_bots = 1;
    race_skirmish_map = 0;
    race_skirmish_init();
    memset(m, 0, sizeof(m));
    geyser_count = 0;
    cx = 120;
    cy = 54;
    screen = SCREEN_PLAY;
}

static int
test_actor_depth(void)
{
    static u8 pixels[5][FB_SIZE];
    const DepthActor empty = { -1, 0, 0, 0, 0 };
    const DepthActor tank = { U_TANK, 50, 0, 0, 0 };
    const struct {
        const char *name;
        DepthActor back, front;
    } cases[] = {
        { "air-over-ground", tank, { U_WRAITH, 50, 0, 0, 0 } },
        { "ground-by-y", tank, { U_GOLIATH, 56, 0, 0, 0 } },
        { "building-over-unit", tank, { B_BARRACKS, 58, 1, 0, 0 } },
        { "unit-over-building", { B_BARRACKS, 43, 1, 0, 0 }, tank },
        { "high-tier-north", { U_OVERLORD, 54, 0, 0, 0 }, { U_WRAITH, 48, 0, 0, 0 } },
        { "high-tier-south", { U_OVERLORD, 48, 0, 0, 0 }, { U_WRAITH, 54, 0, 0, 0 } },
#ifdef SC64_BATTLECRUISER_FLIGHT_EXPERIMENT
        { "landed-facility-below-tank", { B_SCIENCE_FACILITY, 43, 1, 0, 0 }, tank },
        { "lifted-facility-above-tank", tank, { B_SCIENCE_FACILITY, 43, 1, 0, 1 } },
        { "lifted-facility-below-wraith", { B_SCIENCE_FACILITY, 50, 1, 0, 1 },
          { U_WRAITH, 50, 0, 0, 0 } },
#endif
    };
    int failures = 0;

    setup_depth_profile(R_TER);
    depth_capture(empty, empty, pixels[0]);
    for (unsigned n = 0; n < sizeof(cases) / sizeof(cases[0]); ++n) {
        DepthActor back = cases[n].back, front = cases[n].front;
        depth_capture(back, empty, pixels[1]);
        depth_capture(front, empty, pixels[2]);
        depth_capture(back, front, pixels[3]);
        depth_capture(front, back, pixels[4]);
        unsigned overlap = 0, incorrect = 0;
        for (unsigned i = 0; i < FB_SIZE; ++i) {
            u8 base = pixels[0][i], a = pixels[1][i], c = pixels[2][i];
            if (a == base || c == base || a == c) continue;
            ++overlap;
            incorrect += pixels[3][i] != c || pixels[4][i] != c;
        }
        if (!overlap || incorrect) {
            fprintf(stderr, "Actor depth %s: %u incorrect of %u overlap pixels\n",
                    cases[n].name, incorrect, overlap);
            ++failures;
        } else {
            printf("Actor depth %s: %u overlap pixels match\n", cases[n].name, overlap);
        }
    }

    /* In the same air layer, reversing the slots reverses the overlap
     * without moving either body. The world Y positions stay distinct. */
    const DepthActor same_layer[] = {
        { U_WRAITH, 48, 0, 0, 0 }, { U_MUTALISK, 54, 0, 0, 0 },
    };
    for (unsigned first = 0; first < 2; ++first) {
        memset(u, 0, sizeof(u));
        memset(b, 0, sizeof(b));
        depth_spawn(same_layer[first]);
        depth_spawn(same_layer[1 - first]);
        assert(u[0].type == same_layer[first].type && u[1].type == same_layer[1 - first].type);
        assert(u[0].y == same_layer[first].y && u[1].y == same_layer[1 - first].y);
        assert(sc64_unit_world_layer(&u[0]) == 18 && sc64_unit_world_layer(&u[1]) == 18);

        /* Compose the actual sprite draw calls so translucent sprite pixels
         * read the same destination colors as the shared painter. */
        for (unsigned reverse = 0; reverse < 2; ++reverse) {
            gfx_begin(C_BLACK);
            gfx_set_clip_y(12, 120);
            draw_unit(&u[reverse]);
            draw_unit(&u[1 - reverse]);
            capture(pixels[reverse]);
            gfx_end();
        }
        unsigned overlap = 0;
        for (unsigned i = 0; i < FB_SIZE; ++i)
            overlap += pixels[0][i] != pixels[1][i];
        assert(overlap);

        unsigned pages = 0;
        for (unsigned page = 0; page < 2; ++page) {
            gfx_begin(C_BLACK);
            gfx_set_clip_y(12, 120);
            pages |= 1u << gfx_backbuffer_index();
            sc64_draw_depth_ordered_actors();
            capture(pixels[2]);
            gfx_end();
            if (memcmp(pixels[0], pixels[2], FB_SIZE)) {
                fprintf(stderr, "Actor depth same-air-layer slot tie failed (order %u, page %u)\n",
                        first, page);
                ++failures;
            } else {
                printf("Actor depth same-air-layer slot tie (order %u, page %u): %u overlap pixels match\n",
                       first, page, overlap);
            }
        }
        assert(pages == 3);
        if (!first) memcpy(pixels[3], pixels[2], FB_SIZE);
        else assert(memcmp(pixels[3], pixels[2], FB_SIZE) != 0);
    }
    return failures;
}

#if defined(SC64_HAS_SPRITES) && defined(GBA_SCENE_CLIP)
static void
test_completed_building_edge_depth(void)
{
    static u8 expected[FB_SIZE], reversed[FB_SIZE], flyer_only[FB_SIZE], actual[FB_SIZE];
    static const struct { int x, y, flyer_x, flyer_y; } cuts[] = {
        { -21, 66, 4, 66 }, { 261, 66, 235, 66 },
        { 120, 1, 120, 17 }, { 120, 131, 120, 111 },
    };
    setup_depth_profile(R_TER);
    camx = camy = 256;
    SC64_GRID_CLEAR(visible);
    for (unsigned i = 0; i < sizeof(cuts) / sizeof(cuts[0]); ++i) {
        int x = cuts[i].x, y = cuts[i].y;
        memset(u, 0, sizeof(u));
        memset(b, 0, sizeof(b));
        int building = spawn_build(SIDE, B_COMMAND, camx + x, camy + y - 12, 0);
        int flyer = spawn_unit(SIDE, U_WRAITH, camx + cuts[i].flyer_x, camy + cuts[i].flyer_y - 12);
        assert(building >= 0 && flyer >= 0);
        assert(sc64_building_world_layer((unsigned)building) < sc64_unit_world_layer(&u[flyer]));
        const Sc64SpriteFrame *body = building_art_sprite(&b[building]);
        assert(body && !building_art_in_view(&b[building], x, y));

        u8 *references[] = { expected, reversed, flyer_only };
        for (unsigned order = 0; order < 3; ++order) {
            gfx_begin(C_BLACK);
            gfx_set_clip_y(12, 120);
            if (!order) draw_completed_building_sprite(body, x, y);
            draw_unit(&u[flyer]);
            if (order == 1) draw_completed_building_sprite(body, x, y);
            capture(references[order]);
            gfx_end();
        }
        unsigned overlap = 0, visible_body = 0;
        for (unsigned pixel = 0; pixel < FB_SIZE; ++pixel) {
            overlap += expected[pixel] != reversed[pixel];
            visible_body += expected[pixel] != flyer_only[pixel];
        }
        assert(overlap && visible_body);

        unsigned pages = 0;
        for (unsigned page = 0; page < 2; ++page) {
            gfx_begin(C_BLACK);
            gfx_set_clip_y(12, 120);
            pages |= 1u << gfx_backbuffer_index();
            sc64_draw_depth_ordered_actors();
            capture(actual);
            gfx_end();
            assert(!memcmp(expected, actual, FB_SIZE));
        }
        assert(pages == 3);
        b[building].owner = ENEMY;
        assert(!currently_visible(b[building].x, b[building].y));
        gfx_begin(C_BLACK);
        gfx_set_clip_y(12, 120);
        sc64_draw_depth_ordered_actors();
        capture(actual);
        gfx_end();
        assert(!memcmp(flyer_only, actual, FB_SIZE));
        printf("Completed building crop (%d,%d): %u visible body pixels and %u overlap pixels match\n",
               x, y, visible_body, overlap);
    }
}
#endif

#ifdef SC64_UNIT_FACING
static int
test_tank_turret_depth(void)
{
    static u8 expected[FB_SIZE], wrong_order[FB_SIZE], actual[FB_SIZE];
    int failures = 0;

    for (int sieged = 0; sieged <= 1; ++sieged) {
        setup_depth_profile(R_TER);
        memset(u, 0, sizeof(u));
        memset(b, 0, sizeof(b));
        camx = camy = 0;
        int tank = spawn_unit(SIDE, sieged ? U_SIEGE_TANK : U_TANK, 96, 54);
        int goliath = spawn_unit(SIDE, U_GOLIATH, 99, 54);
        assert(tank >= 0 && goliath > tank);
        assert(sc64_unit_world_layer(&u[tank]) == 4);
        assert(sc64_unit_world_layer(&u[goliath]) == 4);

        for (int south = 0; south <= 1; ++south) {
            u[goliath].y = 54 + south;
            for (int reversed = 0; reversed <= 1; ++reversed) {
                u8 *out = reversed ? wrong_order : expected;
                gfx_begin(C_BLACK);
                gfx_set_clip_y(12, 120);
                assert(sc64_facing_blit(sieged ? 30u : 5u, 0, 96, 66));
                if (south == reversed) draw_unit(&u[goliath]);
                assert(sc64_facing_blit(sieged ? 31u : 6u, 0, 96, 66));
                if (south != reversed) draw_unit(&u[goliath]);
                capture(out);
                gfx_end();
            }

            for (unsigned page = 0; page < 2; ++page) {
                gfx_begin(C_BLACK);
                gfx_set_clip_y(12, 120);
                sc64_draw_depth_ordered_actors();
                capture(actual);
                gfx_end();
                unsigned distinct = 0, incorrect = 0;
                for (unsigned i = 0; i < FB_SIZE; ++i) {
                    distinct += expected[i] != wrong_order[i];
                    incorrect += expected[i] != actual[i];
                }
                if (!distinct || incorrect) {
                    fprintf(stderr, "Tank turret depth (sieged %d, south %d, page %u): "
                            "%u wrong pixels, %u distinguishing\n",
                            sieged, south, page, incorrect, distinct);
                    ++failures;
                } else {
                    printf("Tank turret depth (sieged %d, south %d, page %u): "
                           "%u distinguishing pixels match\n", sieged, south, page, distinct);
                }
            }
        }
    }
    return failures;
}
#endif

static int
test_scv_foundation_depth(void)
{
    static u8 background[FB_SIZE], scv[FB_SIZE], scaffold[FB_SIZE];
    static u8 working[FB_SIZE], idle[FB_SIZE];
    setup_depth_profile(R_TER);
    memset(u, 0, sizeof(u));
    memset(b, 0, sizeof(b));
    depth_capture_world(background);
    int worker = spawn_unit(SIDE, U_SCV, 96, 48);
    int building = spawn_build(SIDE, B_BARRACKS, 96, 54, 1);
    assert(worker >= 0 && building >= 0);
    Unit *q = &u[worker];
    Building *foundation = &b[building];
    int duration = building_build_time(foundation->type);
    foundation->build_progress = duration * 3 / 4;

    foundation->alive = 0;
    depth_capture_world(scv);
    foundation->alive = 1;
    q->alive = 0;
    depth_capture_world(scaffold);
    q->alive = 1;
    depth_capture_world(idle);
    q->order = ORDER_BUILD_PENDING;
    q->target = B_BARRACKS;
    assert(sc64_unit_world_layer(q) == 4);
    q->order = ORDER_BUILD;
    q->target = building;
    q->y = foundation->y + BUILD_WORK_RANGE + 1;
    assert(sc64_unit_world_layer(q) == 4);
    q->y = foundation->y - BUILD_WORK_RANGE;
    assert(sc64_unit_world_layer(q) == 5);
    q->y = 48;
    depth_capture_world(working);

    foundation->build_progress = duration - 1;
    update_orders();
    assert(!foundation->under_construction && sc64_unit_world_layer(q) == 4);

    unsigned overlap = 0, incorrect = 0;
    for (unsigned i = 0; i < FB_SIZE; ++i) {
        if (scv[i] == background[i] || scaffold[i] == background[i] ||
            scv[i] == scaffold[i]) continue;
        ++overlap;
        incorrect += idle[i] != scaffold[i] || working[i] != scv[i];
    }
    printf("SCV foundation depth: %u incorrect of %u overlap pixels\n", incorrect, overlap);
    return !overlap || incorrect;
}

static void
test_building_flight_layer(void)
{
    setup_depth_profile(R_TER);
    memset(u, 0, sizeof(u));
    memset(b, 0, sizeof(b));
    int x = -1, y = -1;
    for (int yy = 32; yy < WORLD_H - 32 && x < 0; yy += 16)
        for (int xx = 32; xx < WORLD_W - 32; xx += 16)
            if (can_place(B_SCIENCE_FACILITY, xx, yy) &&
                !terrain_blocked((xx - 8) / 16, (yy - 8) / 16) &&
                !terrain_blocked((xx + 8) / 16, (yy - 8) / 16) &&
                !terrain_blocked((xx - 8) / 16, (yy + 8) / 16) &&
                !terrain_blocked((xx + 8) / 16, (yy + 8) / 16)) {
                x = xx; y = yy; break;
            }
    assert(x >= 0);
#ifdef SC64_BATTLECRUISER_FLIGHT_EXPERIMENT
    int facility = spawn_build(SIDE, B_SCIENCE_FACILITY, x, y, 0);
    assert(facility >= 0 && sc64_building_world_layer((unsigned)facility) == 4);
    assert(race_bc_facility_lift(facility));
    assert(race_bc_facility_land(facility, x, y));
    assert(sc64_building_world_layer((unsigned)facility) == 12);
    race_bc_facility_flight_tick();
    assert(sc64_building_world_layer((unsigned)facility) == 4);
#ifdef SC64_DEATH_WORLD_DEPTH
    assert(race_bc_facility_lift(facility));
    death_vfx_reset();
    death_vfx_scan();
    b[facility].hp = 0;
    destroy_building(facility, ENEMY);
    assert(!race_bc_facility_airborne(facility));
    death_vfx_scan();
    assert(death_vfx[0].source && death_vfx[0].actor_layer_plus1 == 13);
    frame += 5;
    memset(visible, 0xff, sizeof(visible));
    camx = x - 96;
    camy = y - 52;
    u32 keys[SC64_DEATH_SOURCE_DEPTH_LAYERS];
    unsigned size = 0;
    sc64_depth_push_deaths(keys, &size);
    assert(size == 2);
    assert((sc64_depth_pop(keys, &size) >> DEPTH_LAYER_BITS) == 1);
    assert((sc64_depth_pop(keys, &size) >> DEPTH_LAYER_BITS) == 12);
    puts("PASS destroyed lifted Facility: rubble layer 1, attached burst layer 12");
    death_vfx_reset();
#endif
    b[facility].alive = 0;
    invalidate_path_grid();
#endif
#ifdef SC64_TERRAN_LATE_RUNTIME
    race_mode = 0;
    race_selected_mission = 11;
    int command = spawn_build(SIDE, B_COMMAND, x, y, 0);
    assert(command >= 0 && sc64_building_world_layer((unsigned)command) == 4);
    assert(race_late_lift(command));
    int lx = -1, ly = -1;
    for (int yy = 64; yy < WORLD_H - 64 && lx < 0; yy += 32)
        for (int xx = 64; xx < WORLD_W - 64; xx += 32)
            if (race_late_landing_site(command, xx, yy)) {
                lx = xx; ly = yy; break;
            }
    assert(lx >= 0 && race_late_flight_order(command, lx, ly, 1));
    assert(sc64_building_world_layer((unsigned)command) == 12);
    for (int i = 0; i < WORLD_W * 3 && race_late_airborne(command); ++i) {
        ++frame;
        race_late_flight_tick();
    }
    assert(b[command].x == lx && b[command].y == ly &&
           sc64_building_world_layer((unsigned)command) == 4);
#endif
}

static int
test_selection_depth(void)
{
    static u8 ring[FB_SIZE], background[FB_SIZE], unit[FB_SIZE], building[FB_SIZE], pair[FB_SIZE];
    const DepthActor empty = { -1, 0, 0, 0, 0 };
    const struct {
        const char *name;
        int race, ring_in_front;
        DepthActor unit, building;
    } cases[] = {
        { "Overlord over Hatchery", R_ZERG, 1, { U_OVERLORD, 50, 0, 0, 0 }, { B_HATCHERY, 50, 1, 0, 0 } },
        { "Tank behind Barracks", R_TER, 0, { U_TANK, 50, 0, 0, 0 }, { B_BARRACKS, 58, 1, 0, 0 } },
    };
    int failures = 0;

    for (unsigned n = 0; n < sizeof(cases) / sizeof(cases[0]); ++n) {
        setup_depth_profile(cases[n].race);
        DepthActor actor = cases[n].unit;
        depth_capture(empty, empty, background);
        depth_capture(actor, empty, unit);
        depth_capture(cases[n].building, empty, building);

        /* Isolate the actual ring so body, vitals and team colours cannot
         * satisfy the overlap assertion by accident. */
        memset(u, 0, sizeof(u));
        actor.selected = 1;
        depth_spawn(actor);
        gfx_begin(C_BLACK);
        gfx_set_clip_y(12, 120);
        draw_unit_pass(&u[0], 1);
        capture(ring);
        gfx_end();

        depth_capture(actor, cases[n].building, pair);
        unsigned overlap = 0, body_overlap = 0, incorrect = 0;
        for (unsigned i = 0; i < FB_SIZE; ++i) {
            if (ring[i] != C_GREEN) continue;
            if (cases[n].ring_in_front && unit[i] != background[i] && unit[i] != C_GREEN) {
                ++body_overlap;
                incorrect += pair[i] != unit[i];
            }
            if (unit[i] != background[i] || building[i] == background[i] || building[i] == C_GREEN)
                continue;
            ++overlap;
            u8 expected = cases[n].ring_in_front ? C_GREEN : building[i];
            incorrect += pair[i] != expected;
        }
        if (!overlap || (cases[n].ring_in_front && !body_overlap) || incorrect) {
            fprintf(stderr, "Selection depth %s: %u incorrect, %u building pixels, %u own-body pixels\n",
                    cases[n].name, incorrect, overlap, body_overlap);
            ++failures;
        } else {
            printf("Selection depth %s: %u building pixels and %u own-body pixels match\n",
                   cases[n].name, overlap, body_overlap);
        }
    }
    return failures;
}
#endif

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
#if defined(SC64_WORLD_DEPTH) && defined(SC64_TERRAN_FULL_CAMPAIGN_ENTRY)
    failures += test_actor_depth();
#if defined(SC64_HAS_SPRITES) && defined(GBA_SCENE_CLIP)
    test_completed_building_edge_depth();
#endif
#ifdef SC64_UNIT_FACING
    failures += test_tank_turret_depth();
#endif
    failures += test_scv_foundation_depth();
    test_building_flight_layer();
    failures += test_selection_depth();
#endif
    if (failures) fprintf(stderr, "Race no-clear coverage failures: %d\n", failures);
    else puts("Race framebuffer coverage: all full-clear comparisons match");
    return failures != 0;
}
