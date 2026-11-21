/* SPDX-License-Identifier: GPL-3.0-or-later
 * Integration checks for source-authored weapon events in the real game code. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

static const int heading_dx[8] = { 0, 17, 24, 17, 0, -17, -24, -17 };
static const int heading_dy[8] = { -24, -17, 0, 17, 24, 17, 0, -17 };

static void reset_world(void)
{
    gba_init();
    game_init();
    memset(u, 0, sizeof(u));
    memset(b, 0, sizeof(b));
    clear_tracers(); /* Also clears deferred impacts between fixture cases. */
    memset(visible, 0xff, sizeof(visible));
    memset(explored, 0xff, sizeof(explored));
    camx = camy = 0;
    race_mode = 0;
    race_human = R_PROTOSS;
    race_bots = 1;
    weapons_level = armor_level = 0;
    won = lost = 0;
    screen = SCREEN_PLAY;
    gfx_begin(C_PANEL);
}

static unsigned live_tracers(void)
{
    unsigned n = 0;
    for (int i = 0; i < MAX_TRACERS; ++i) n += tracers[i].life > 0;
    return n;
}

static int has_source_tracer(unsigned raw, unsigned weapon, unsigned slot_plus1)
{
    for (int i = 0; i < MAX_TRACERS; ++i) {
        Sc64WeaponSourceVfxKey key;
        if (tracers[i].life > 0 &&
            sc64_weapon_source_vfx_unpack((uint32_t)tracers[i].color, &key) &&
            key.actor_raw == raw && key.weapon_id == weapon &&
            key.actor_slot_plus1 == slot_plus1) return 1;
    }
    return 0;
}

static int framebuffer_has_battlefield_pixels(void)
{
    for (int y = 12; y < 120; ++y)
        for (int x = 0; x < 240; ++x)
            if (gfx_test_pixel(x, y) != C_PANEL) return 1;
    return 0;
}

static void assert_hud_untouched(void)
{
    for (int y = 0; y < 12; ++y)
        for (int x = 0; x < 240; ++x)
            assert(gfx_test_pixel(x, y) == C_PANEL);
    for (int y = 120; y < 160; ++y)
        for (int x = 0; x < 240; ++x)
            assert(gfx_test_pixel(x, y) == C_PANEL);
}

static void test_all_source_profiles(void)
{
    unsigned emitted_profiles = 0, projectile_profiles = 0;
    assert(SC64_WEAPON_SOURCE_PROFILE_COUNT == 66u);
    for (unsigned n = 0; n < SC64_WEAPON_SOURCE_PROFILE_COUNT; ++n) {
        const Sc64WeaponSourceAttackProfile *p = &sc64_weapon_source_profiles[n];
        const int has_projectile = p->projectile_image != 0 &&
            (p->flags & SC64_WEAPON_SOURCE_PROFILE_HAS_PROJECTILE_IMAGE) != 0;
        if (has_projectile) ++projectile_profiles;
        ++emitted_profiles;

        for (unsigned heading = 0; heading < 8; ++heading) {
            memset(tracers, 0, sizeof(tracers));
            int x0 = 112, y0 = 58;
            int x1 = x0 + heading_dx[heading], y1 = y0 + heading_dy[heading];
            if (!sc64_source_emit(p->actor_raw, -1, p->weapon_id, p->mode,
                                  x0, y0, x1, y1, -1)) {
                fprintf(stderr, "source profile %u not emitted: raw=%u weapon=%u mode=%u variant=%u\n",
                        n, p->actor_raw, p->weapon_id, p->mode, p->variant);
                assert(0 && "every generated source profile must be reachable from the engine emitter");
            }
            unsigned expected = has_projectile ?
                (p->weapon_count == 2 ? 2u : 1u) : 0u;
            assert(live_tracers() == expected);
            for (int i = 0; i < MAX_TRACERS; ++i) {
                if (tracers[i].life <= 0) continue;
                Sc64WeaponSourceVfxKey key;
                assert(sc64_weapon_source_vfx_unpack((uint32_t)tracers[i].color, &key));
                assert(key.actor_raw == p->actor_raw);
                assert(key.weapon_id == p->weapon_id);
                assert(key.target_air == p->mode);
                assert(key.actor_slot_plus1 == 0);
                assert(key.direction == sc64_source_effect_direction(x1 - x0, y1 - y0));
            }
            if (!has_projectile) continue;
            const Sc64WeaponSourceAttackProfile *resolved =
                sc64_weapon_source_profile_lookup(p->actor_raw, p->weapon_id,
                                                   p->mode,
                                                   p->variant);
            assert(resolved && resolved->projectile_image == p->projectile_image);
            int duration = sc64_weapon_source_trace_duration_heading(
                resolved, sc64_source_effect_direction(x1 - x0, y1 - y0),
                x0, y0, x1, y1);
            assert(duration > 0);
            int any_pixels = 0;
            for (int age = 0; age < duration; ++age) {
                /* Test every simulation-age of the source event. Clearing only
                 * the battlefield preserves sentinel pixels in both HUD bands. */
                gfx_rect(0, 12, 240, 108, C_PANEL);
                for (int i = 0; i < MAX_TRACERS; ++i) {
                    if (tracers[i].life <= 0) continue;
                    tracers[i].life = duration - age;
                }
                draw_tracers();
                assert_hud_untouched();
                any_pixels |= framebuffer_has_battlefield_pixels();
            }
            if (!any_pixels) {
                fprintf(stderr, "projectile source profile %u drew no pixels: raw=%u weapon=%u mode=%u image=%u\n",
                        n, p->actor_raw, p->weapon_id, p->mode, p->projectile_image);
                assert(any_pixels);
            }
        }
    }
    assert(emitted_profiles == SC64_WEAPON_SOURCE_PROFILE_COUNT);
    assert(projectile_profiles > 0);
    printf("PASS %u source weapon profiles (%u with projectile art), 8 headings and full tracer lifetimes\n",
           emitted_profiles, projectile_profiles);
}

static int make_unit(int owner, int type, unsigned raw, int x, int y)
{
    int slot = spawn_unit(owner, type, x, y);
    assert(slot >= 0);
    u[slot].raw_type = (int)raw;
    u[slot].raw_owner = owner == SIDE ? 0 : 1;
    u[slot].hp = 500;
    u[slot].shield = 0;
    return slot;
}

#ifdef SC64_PROJECTILE_DEPTH
enum { BATTLEFIELD_BYTES = 240 * 108 };

static void begin_depth_frame(void)
{
    gfx_begin(C_PANEL);
    gfx_set_clip_y(12, 120);
}

static void capture_depth_frame(u8 *out)
{
    assert(gfx_read_band(12, 108, out));
    assert_hud_untouched();
}

/* The same ground-target projectile covers a ground actor and passes behind
 * a flyer. Isolate each sprite and require real distinguishable overlap. */
static void test_moving_projectile_overlap(void)
{
    static _Alignas(4) u8 actor[BATTLEFIELD_BYTES];
    static _Alignas(4) u8 projectile[BATTLEFIELD_BYTES];
    static _Alignas(4) u8 pair[BATTLEFIELD_BYTES];
    static const struct {
        const char *name;
        int type, raw, y, target_air, bullet_in_front;
    } cases[] = {
        { "Tank", U_TANK, 5, 54, 0, 1 },
        { "Wraith", U_WRAITH, 8, 54, 0, 0 },
        { "Wraith with an air target", U_WRAITH, 8, 54, 1, 1 },
        { "Wraith at the upper HUD", U_WRAITH, 8, 0, 0, 0 },
        { "Tank at the lower HUD", U_TANK, 5, 107, 0, 1 },
    };
    for (unsigned n = 0; n < sizeof(cases) / sizeof(cases[0]); ++n) {
        reset_world();
        memset(m, 0, sizeof(m));
        geyser_count = 0;
        int y = cases[n].y;
        int blocker = make_unit(SIDE, cases[n].type, (unsigned)cases[n].raw, 112, y);
        assert(sc64_unit_world_layer(&u[blocker]) == (cases[n].type == U_WRAITH ? 18u : 4u));
        begin_depth_frame();
        sc64_draw_depth_ordered_actors();
        capture_depth_frame(actor);

        int shooter = make_unit(SIDE, U_DRAGOON, 66, 32, y);
        int air = cases[n].target_air;
        int target = make_unit(ENEMY, air ? U_WRAITH : U_SCV, air ? 8 : 7, 208, y);
        assert(sc64_source_unit_vfx(&u[shooter], u[target].x, u[target].y, air, target));
        assert(live_tracers() == 1 && sc64_trace_slots[0].release_life);
        unsigned overlap = 0;
        while (tracers[0].life > 1) {
            update_tracers();
            if (sc64_trace_slots[0].release_life) continue;
            assert(sc64_trace_slots[0].bullet_layer == (air ? 19 : 5));
            begin_depth_frame();
            draw_tracer(0, SC64_TRACE_PROJECTILE);
            capture_depth_frame(projectile);
            for (unsigned i = 0; i < BATTLEFIELD_BYTES; ++i)
                overlap += actor[i] != C_PANEL && projectile[i] != C_PANEL &&
                           actor[i] != projectile[i];
            if (overlap) break;
        }
        assert(overlap && "the test shot must cross both isolated sprite masks");
        begin_depth_frame();
        sc64_draw_depth_ordered_actors();
        draw_tracers_pass(1);
        capture_depth_frame(pair);
        for (unsigned i = 0; i < BATTLEFIELD_BYTES; ++i) {
            if (actor[i] == C_PANEL || projectile[i] == C_PANEL ||
                actor[i] == projectile[i]) continue;
            assert(pair[i] == (cases[n].bullet_in_front ? projectile[i] : actor[i]));
        }
        printf("PASS projectile depth against %s, %u overlap pixels\n", cases[n].name, overlap);
    }
}

static void test_ground_projectile_world_y(void)
{
    static _Alignas(4) u8 actor[BATTLEFIELD_BYTES];
    static _Alignas(4) u8 projectile[BATTLEFIELD_BYTES];
    static _Alignas(4) u8 pair[BATTLEFIELD_BYTES];
    reset_world();
    memset(m, 0, sizeof(m));
    geyser_count = 0;
    int shooter = make_unit(SIDE, U_DRAGOON, 66, 32, 24);
    int bunker = spawn_build(ENEMY, B_BUNKER, 208, 100, 0);
    assert(bunker >= 0 && sc64_building_world_layer((unsigned)bunker) == 3);
    assert(sc64_source_unit_vfx(&u[shooter], b[bunker].x, b[bunker].y,
                                0, MAX_UNITS + bunker));
    assert(live_tracers() == 1 && sc64_trace_slots[0].bullet_layer == 4);

    const Tracer *t = &tracers[0];
    Sc64WeaponSourceVfxKey key;
    assert(sc64_weapon_source_vfx_unpack((u32)t->color, &key));
    const Sc64WeaponSourceAttackProfile *profile = sc64_weapon_source_trace_profile((u32)t->color);
    assert(profile);
    unsigned windup = sc64_weapon_source_trace_release_ticks(profile) +
                      sc64_weapon_source_trace_startup_ticks(profile);
    int launch_x, launch_y, px = 0, py = 0;
    sc64_weapon_source_trace_launch_origin(profile, key.direction, t->x0, t->y0,
                                          &launch_x, &launch_y);
    Sc64WeaponSourceMotion motion = sc64_weapon_source_motion_prepare(
        profile, launch_x, launch_y, t->x1, t->y1);
    while (t->life > 1) {
        update_tracers();
        unsigned age = sc64_weapon_source_vfx_source_tick((unsigned)(sc64_tracer_duration(0) - t->life));
        if (age < windup || age - windup >= motion.flight) continue;
        assert(sc64_weapon_source_trace_position_motion(profile, launch_x, launch_y,
            t->x1, t->y1, age - windup, &px, &py, &motion));
        if (px >= 96 && px <= 136) break;
    }
    assert(t->life > 1 && !sc64_trace_slots[0].release_life);
    assert(px >= 96 && px <= 136 && py - 3 > t->y0 && py + 3 < t->y1);
    begin_depth_frame();
    draw_tracer(0, SC64_TRACE_PROJECTILE);
    capture_depth_frame(projectile);

    /* Both Tank anchors are south of the shooter, but only one is south of
     * the live projectile. A launch-Y key wrongly puts both Tanks in front. */
    int tank = make_unit(SIDE, U_TANK, 5, px, py - 3);
    assert(sc64_unit_world_layer(&u[tank]) == 4);
    for (int south = 0; south <= 1; ++south) {
        u[tank].y = py + (south ? 3 : -3);
        begin_depth_frame();
        draw_unit(&u[tank]);
        capture_depth_frame(actor);
        begin_depth_frame();
        sc64_draw_depth_ordered_actors();
        draw_tracers_pass(1);
        capture_depth_frame(pair);
        unsigned overlap = 0;
        for (unsigned i = 0; i < BATTLEFIELD_BYTES; ++i) {
            if (actor[i] == C_PANEL || projectile[i] == C_PANEL ||
                actor[i] == projectile[i]) continue;
            ++overlap;
            assert(pair[i] == (south ? actor[i] : projectile[i]));
        }
        assert(overlap);
        printf("PASS ground projectile Y against Tank to the %s, %u overlap pixels\n",
               south ? "south" : "north", overlap);
    }
}

static void test_stationary_projectile_depth(void)
{
    static _Alignas(4) u8 effect_in_front[BATTLEFIELD_BYTES];
    static _Alignas(4) u8 actor_in_front[BATTLEFIELD_BYTES];
    static _Alignas(4) u8 pair[BATTLEFIELD_BYTES];
    static const struct {
        int type, raw, y, in_front;
    } cases[] = {
        { U_TANK, 5, 51, 0 },
        { U_TANK, 5, 57, 1 },
        { U_WRAITH, 8, 54, 1 },
    };
    reset_world();
    memset(m, 0, sizeof(m));
    geyser_count = 0;
    int shooter = make_unit(SIDE, U_SIEGE_TANK, 30, 32, 24);
    int target = spawn_build(ENEMY, B_BUNKER, 112, 54, 0);
    assert(target >= 0 && sc64_building_world_layer((unsigned)target) == 3);
    assert(sc64_source_unit_vfx(&u[shooter], 112, 54, 0, MAX_UNITS + target));
    assert(live_tracers() == 1 && sc64_trace_slots[0].release_life);
    const Sc64WeaponSourceAttackProfile *profile =
        sc64_weapon_source_trace_profile((u32)tracers[0].color);
    assert(profile && profile->weapon_bullet_type == SC64_SOURCE_BULLET_APPEAR_TARGET_UNIT);
    while (sc64_trace_slots[0].release_life && tracers[0].life > 1) update_tracers();
    assert(tracers[0].life > 1 && sc64_trace_slots[0].bullet_layer == 4);
    assert(sc64_trace_world_y(0) == b[target].y);

    /* A released effect keeps its depth and the captured impact point
     * when its original target disappears. */
    b[target].alive = 0;
    for (unsigned n = 0; n < sizeof(cases) / sizeof(cases[0]); ++n) {
        int blocker = make_unit(SIDE, cases[n].type, (unsigned)cases[n].raw, 112, cases[n].y);
        unsigned overlap = 0;
        while (tracers[0].life > 1) {
            /* Siege splash remaps the underlying palette. Compare both
             * compositions directly instead of using isolated flat colors. */
            begin_depth_frame();
            draw_unit(&u[blocker]);
            draw_tracer(0, SC64_TRACE_PROJECTILE);
            capture_depth_frame(effect_in_front);
            begin_depth_frame();
            draw_tracer(0, SC64_TRACE_PROJECTILE);
            draw_unit(&u[blocker]);
            capture_depth_frame(actor_in_front);
            for (unsigned i = 0; i < BATTLEFIELD_BYTES; ++i)
                overlap += effect_in_front[i] != actor_in_front[i];
            if (overlap) break;
            update_tracers();
        }
        assert(overlap && "the stationary effect must overlap the blocker");
        begin_depth_frame();
        sc64_draw_depth_ordered_actors();
        draw_tracers_pass(1);
        capture_depth_frame(pair);
        for (unsigned i = 0; i < BATTLEFIELD_BYTES; ++i) {
            if (effect_in_front[i] == actor_in_front[i]) continue;
            assert(pair[i] == (cases[n].in_front ? actor_in_front[i] : effect_in_front[i]));
        }
        printf("PASS stationary projectile depth against raw %d at Y%d, %u overlap pixels\n",
               cases[n].raw, cases[n].y, overlap);
        u[blocker].alive = 0;
    }
}

#ifdef SC64_P4_STORM_ENABLED
static void test_flight_trail_depth(void)
{
    static _Alignas(4) u8 trail[BATTLEFIELD_BYTES];
    static _Alignas(4) u8 expected[BATTLEFIELD_BYTES];
    static _Alignas(4) u8 pair[BATTLEFIELD_BYTES];
    static const struct { unsigned raw, weapon, air, image; } cases[] = {
        { 2, 4, 0, 532 }, { 8, 15, 1, 528 }, { 3, 8, 1, 529 },
    };
    const unsigned parent = SC64_TRACE_PROJECTILE | SC64_TRACE_WITHOUT_FLIGHT_TRAILS;
    const unsigned child = SC64_TRACE_PROJECTILE | SC64_TRACE_ONLY_FLIGHT_TRAILS;
    for (unsigned n = 0; n < sizeof(cases) / sizeof(cases[0]); ++n) {
        reset_world();
        memset(m, 0, sizeof(m));
        geyser_count = 0;
        race_selected_mission = SC64_P4_GLOBAL_ID;
        memset(&race_p4.storm, 0, sizeof(race_p4.storm));
        Sc64PsionicStorm *storm = &race_p4.storm.storm[0];

        /* Use the native observed layer-5 missile plane and isolate one
         * emitter even when its weapon normally launches a pair. */
        int target = make_unit(ENEMY, U_SCV, 7, 208, 54);
        assert(sc64_source_emit(cases[n].raw, -1, cases[n].weapon, cases[n].air,
                                32, 54, 208, 54, target));
        for (unsigned i = 1; i < MAX_TRACERS; ++i) tracers[i].life = 0;
        const Sc64TraceSlot *slot = &sc64_trace_slots[0];
        const Sc64WeaponSourceAttackProfile *profile =
            sc64_weapon_source_trace_profile((u32)tracers[0].color);
        assert(profile && profile->projectile_image == cases[n].image);
        while (slot->release_life && tracers[0].life > 1) update_tracers();
        assert(slot->bullet_layer == 5);
        u[target].alive = 0;
        unsigned pixel = BATTLEFIELD_BYTES;
        while (tracers[0].life > 1 && pixel == BATTLEFIELD_BYTES) {
            begin_depth_frame();
            draw_tracer(0, child);
            capture_depth_frame(trail);
            for (unsigned i = 0; i < BATTLEFIELD_BYTES; ++i) {
                if (trail[i] != C_PANEL && trail[i] != C_WHITE) {
                    pixel = i;
                    break;
                }
            }
            if (pixel == BATTLEFIELD_BYTES) update_tracers();
        }
        assert(pixel < BATTLEFIELD_BYTES);

        begin_depth_frame();
        draw_tracer(0, SC64_TRACE_PROJECTILE);
        capture_depth_frame(expected);
        begin_depth_frame();
        sc64_draw_depth_ordered_actors();
        draw_tracers_pass(1);
        capture_depth_frame(pair);
        assert(!memcmp(expected, pair, BATTLEFIELD_BYTES));

        storm->x = (int)(pixel % 240u);
        storm->y = (int)(pixel / 240u);
        storm->active = 1;
        begin_depth_frame();
        draw_tracer(0, parent);
        draw_storm_marker(storm);
        draw_tracer(0, child);
        capture_depth_frame(expected);
        begin_depth_frame();
        draw_tracer(0, SC64_TRACE_PROJECTILE);
        draw_storm_marker(storm);
        capture_depth_frame(pair);
        assert(expected[pixel] != pair[pixel]);
        begin_depth_frame();
        sc64_draw_depth_ordered_actors();
        draw_tracers_pass(1);
        capture_depth_frame(pair);
        assert(!memcmp(expected, pair, BATTLEFIELD_BYTES));
        unsigned source_age = sc64_weapon_source_vfx_source_tick(
            (unsigned)(sc64_tracer_duration(0) - tracers[0].life));
        unsigned flight_start = sc64_weapon_source_trace_release_ticks(profile) +
            sc64_weapon_source_trace_startup_ticks(profile);
        printf("PASS source flight trail image %u over a layer-5 Storm at tick %u (flight [%u,%u))\n",
               cases[n].image, source_age, flight_start, flight_start + slot->flight);

        if (n == 0) {
            int blocker = make_unit(SIDE, U_WRAITH, 8, storm->x, storm->y);
            int overlap = 0;
            for (int dx = -8; dx <= 8 && !overlap; ++dx) {
                u[blocker].x = storm->x + dx;
                begin_depth_frame();
                draw_tracer(0, parent);
                draw_storm_marker(storm);
                draw_tracer(0, child);
                draw_unit(&u[blocker]);
                capture_depth_frame(expected);
                begin_depth_frame();
                draw_tracer(0, parent);
                draw_storm_marker(storm);
                draw_unit(&u[blocker]);
                draw_tracer(0, child);
                capture_depth_frame(pair);
                overlap = memcmp(expected, pair, BATTLEFIELD_BYTES) != 0;
            }
            assert(overlap);
            begin_depth_frame();
            sc64_draw_depth_ordered_actors();
            draw_tracers_pass(1);
            capture_depth_frame(pair);
            assert(!memcmp(expected, pair, BATTLEFIELD_BYTES));
            u[blocker].alive = 0;
        }

        /* A layer-4 child needs its individual world-Y key. Keep the
         * unsplit path until lower children have individual queue entries. */
        sc64_trace_slots[0].bullet_layer = 3;
        camy = storm->y;
        begin_depth_frame();
        draw_tracer(0, SC64_TRACE_PROJECTILE);
        draw_storm_marker(storm);
        capture_depth_frame(expected);
        begin_depth_frame();
        sc64_draw_depth_ordered_actors();
        draw_tracers_pass(1);
        capture_depth_frame(pair);
        assert(!memcmp(expected, pair, BATTLEFIELD_BYTES));
        storm->active = 0;
    }
}
#endif

static void test_attached_attacker_depth(void)
{
    static _Alignas(4) u8 shooter_without_muzzle[BATTLEFIELD_BYTES];
    static _Alignas(4) u8 muzzle_in_front[BATTLEFIELD_BYTES];
    static _Alignas(4) u8 blocker_in_front[BATTLEFIELD_BYTES];
    static _Alignas(4) u8 pair[BATTLEFIELD_BYTES];
    static const struct {
        int type, raw, shooter_y, blocker_dy, blocker_front;
    } cases[] = {
        { U_TANK, 5, 54, -3, 0 },
        { U_TANK, 5, 54, 3, 1 },
#ifdef SC64_UNIT_FACING
        { U_VULTURE, 2, 54, 0, 0 },
#endif
        { U_WRAITH, 8, 54, 0, 1 },
        { U_WRAITH, 8, 2, 0, 1 },
        { U_WRAITH, 8, 106, 0, 1 },
    };

    reset_world();
    memset(m, 0, sizeof(m));
    geyser_count = 0;
    int shooter = make_unit(SIDE, U_SIEGE_TANK, 30, 40, 30);
    assert(sc64_unit_world_layer(&u[shooter]) == 4);
    assert(sc64_source_unit_vfx(&u[shooter], 208, 30, 0, -1));
    assert(live_tracers() == 1 && sc64_trace_slots[0].release_life);
    const Sc64WeaponSourceAttackProfile *profile =
        sc64_weapon_source_trace_profile((u32)tracers[0].color);
    assert(profile && profile->attack_image == 254);
    update_tracers(); /* The new trace starts one update before age zero. */
    while (tracers[0].life > 1 && sc64_weapon_source_vfx_source_tick(
               (unsigned)(sc64_tracer_duration(0) - tracers[0].life)) < 2)
        update_tracers();
    assert(tracers[0].life && sc64_trace_slots[0].release_life);

    /* This siege muzzle is a palette remap. Compare both paint orders at
     * pixels changed by the attached image, excluding body-only overlap.
     * The shooter moved after firing began, so the muzzle must follow it. */
    u[shooter].x = 112;
    int blocker = make_unit(SIDE, U_TANK, 5, u[shooter].x, 54);
    for (unsigned n = 0; n < sizeof(cases) / sizeof(cases[0]); ++n) {
        u[shooter].y = cases[n].shooter_y;
        u[blocker].type = cases[n].type;
        u[blocker].raw_type = cases[n].raw;
        u[blocker].y = u[shooter].y + cases[n].blocker_dy;
        unsigned overlap = 0;
        for (int dx = -24; dx <= 24 && !overlap; ++dx) {
            u[blocker].x = u[shooter].x + dx;
            begin_depth_frame();
#ifdef SC64_UNIT_FACING
            if (cases[n].raw == 2) {
                assert(sc64_facing_blit(30, 0, u[shooter].x, u[shooter].y + 12));
                draw_unit(&u[blocker]);
                sc64_facing_draw_turret((unsigned)shooter, 0);
            } else
#endif
            {
                draw_unit(&u[blocker]);
                draw_unit(&u[shooter]);
            }
            capture_depth_frame(shooter_without_muzzle);
            draw_tracer(0, SC64_TRACE_ATTACKER_IMAGE);
            capture_depth_frame(muzzle_in_front);
            begin_depth_frame();
            draw_unit(&u[shooter]);
            draw_tracer(0, SC64_TRACE_ATTACKER_IMAGE);
            draw_unit(&u[blocker]);
            capture_depth_frame(blocker_in_front);
            for (unsigned i = 0; i < BATTLEFIELD_BYTES; ++i)
                overlap += muzzle_in_front[i] != shooter_without_muzzle[i] &&
                           muzzle_in_front[i] != blocker_in_front[i];
        }
        assert(overlap && "the attached muzzle must overlap the blocker at its live anchor");
        begin_depth_frame();
        sc64_draw_depth_ordered_actors();
        draw_tracers_pass(1);
        capture_depth_frame(pair);
        const u8 *expected = cases[n].blocker_front ? blocker_in_front : muzzle_in_front;
        assert(!memcmp(pair, expected, BATTLEFIELD_BYTES));
        printf("PASS attached siege muzzle at Y%d against raw %d at Y%d, %u overlap pixels\n",
               u[shooter].y, cases[n].raw, u[blocker].y, overlap);
    }

    u[blocker].alive = 0;
    u[shooter].y = 54;
    u[shooter].owner = ENEMY;
    memset(visible, 0, sizeof(visible));
    assert(!currently_visible(u[shooter].x, u[shooter].y));
    begin_depth_frame();
    sc64_draw_depth_ordered_actors();
    draw_tracers_pass(1);
    assert(!framebuffer_has_battlefield_pixels());
    assert_hud_untouched();

    /* Reuse the very same slot, raw type and anchor during wind-up. */
    u[shooter].alive = 0;
    assert(make_unit(SIDE, U_SIEGE_TANK, 30, 112, 54) == shooter);
    assert(tracers[0].life && sc64_trace_slots[0].release_life);
    begin_depth_frame();
    sc64_draw_depth_ordered_actors();
    draw_tracers_pass(1);
    capture_depth_frame(pair);
    clear_tracers();
    begin_depth_frame();
    sc64_draw_depth_ordered_actors();
    draw_tracers_pass(1);
    capture_depth_frame(shooter_without_muzzle);
    assert(!memcmp(pair, shooter_without_muzzle, BATTLEFIELD_BYTES));
}

static void test_detached_attack_sprite_depth(void)
{
    static _Alignas(4) u8 child_in_front[BATTLEFIELD_BYTES];
    static _Alignas(4) u8 blocker_in_front[BATTLEFIELD_BYTES];
    static _Alignas(4) u8 pair[BATTLEFIELD_BYTES];
    reset_world();
    memset(m, 0, sizeof(m));
    geyser_count = 0;
    int shooter = make_unit(SIDE, U_HYDRALISK, 38, 64, 54);
    assert(sc64_source_unit_vfx(&u[shooter], 208, 54, 0, -1));
    Sc64TraceSlot *slot = &sc64_trace_slots[0];
    assert(slot->attack_sprite_birth_life && !slot->attack_sprite_layer);
    u[shooter].x = 112;
    u[shooter].y = 60;
    while (slot->attack_sprite_birth_life && tracers[0].life > 1) update_tracers();
    assert(slot->attack_sprite_layer == 5);
    assert(slot->attack_sprite_x == 112 && slot->attack_sprite_y == 60);
    Sc64AttackSpriteDraw child;
    assert(sc64_attack_sprite_draw_state(0, &child));
    assert(child.image == 35 && child.age == 0);
    assert(child.x != tracers[0].x0 && child.y > 44 && child.y < 76);

    u[shooter].alive = 0;
    int blocker = make_unit(SIDE, U_TANK, 5, child.x, child.y);
    assert(blocker == shooter && slot->attacker_plus1 < 0);
    /* Keep the main projectile out of the isolated attacker-child fixture. */
    assert(slot->bullet_layer);
    slot->release_life = 1;
    int init_x = tracers[0].x0, init_y = tracers[0].y0;
    for (int air = 0; air <= 1; ++air) {
        u[blocker].type = air ? U_WRAITH : U_TANK;
        u[blocker].raw_type = air ? 8 : 5;
        unsigned overlap = 0;
        for (int dx = -10; dx <= 10 && !overlap; ++dx) {
            u[blocker].x = child.x + dx;
            /* Use the previous standalone compositor as the isolated art
             * reference, anchored at the captured spawn instead of emit. */
            tracers[0].x0 = slot->attack_sprite_x;
            tracers[0].y0 = slot->attack_sprite_y;
            begin_depth_frame();
            draw_unit(&u[blocker]);
            draw_tracer(0, SC64_TRACE_ATTACKER_SPRITE);
            capture_depth_frame(child_in_front);
            begin_depth_frame();
            draw_tracer(0, SC64_TRACE_ATTACKER_SPRITE);
            draw_unit(&u[blocker]);
            capture_depth_frame(blocker_in_front);
            tracers[0].x0 = init_x;
            tracers[0].y0 = init_y;
            for (unsigned i = 0; i < BATTLEFIELD_BYTES; ++i)
                overlap += child_in_front[i] != blocker_in_front[i];
        }
        assert(overlap);
        begin_depth_frame();
        sc64_draw_depth_ordered_actors();
        draw_tracers_pass(1);
        capture_depth_frame(pair);
        assert(!memcmp(pair, air ? blocker_in_front : child_in_front, BATTLEFIELD_BYTES));
        printf("PASS detached Hydralisk sprite versus %s, %u overlap pixels\n",
               air ? "Wraith" : "Tank", overlap);
    }

    clear_tracers();
    u[blocker].alive = 0;
    shooter = make_unit(SIDE, U_HYDRALISK, 38, 112, 60);
    assert(sc64_source_unit_vfx(&u[shooter], 208, 60, 0, -1));
    u[shooter].alive = 0;
    assert(make_unit(SIDE, U_HYDRALISK, 38, 112, 60) == shooter);
    while (slot->attack_sprite_birth_life && tracers[0].life > 1) update_tracers();
    assert(tracers[0].life && !slot->attack_sprite_layer);
    assert(!sc64_attack_sprite_draw_state(0, &child));

    clear_tracers();
    u[shooter].alive = 0;
    shooter = make_unit(SIDE, U_SCOURGE, 47, 112, 60);
    assert(sc64_source_unit_vfx(&u[shooter], 116, 60, 1, -1));
    assert(slot->attack_sprite_layer == 19 && !slot->attack_sprite_birth_life);
    u[shooter].alive = 0;
    update_tracers();
    assert(sc64_attack_sprite_draw_state(0, &child));
    assert(child.image == 4 && child.x == 112 && child.y == 60);
}

static void test_projectile_release_layer(void)
{
    reset_world();
    int shooter = make_unit(SIDE, U_DRAGOON, 66, 32, 54);
    int target = make_unit(ENEMY, U_SCV, 7, 208, 54);
    int foundation = spawn_build(ENEMY, B_BARRACKS, 208, 60, 1);
    assert(foundation >= 0);
    assert(sc64_source_unit_vfx(&u[shooter], u[target].x, u[target].y, 0, target));
    Sc64TraceSlot *trace = &sc64_trace_slots[0];
    assert(trace->bullet_layer == 5 && trace->depth_ref == target && trace->release_life);
    while (tracers[0].life > trace->release_life + 1) update_tracers();
    assert(trace->release_life && trace->bullet_layer == 5);
    u[target].order = ORDER_BUILD;
    u[target].target = foundation;
    assert(sc64_unit_world_layer(&u[target]) == 5);
    update_tracers();
    assert(!trace->release_life && trace->bullet_layer == 6);
    u[target].order = ORDER_IDLE;
    update_tracers();
    assert(sc64_unit_world_layer(&u[target]) == 4 && trace->bullet_layer == 6);

    clear_tracers();
    assert(sc64_source_unit_vfx(&u[shooter], 208, 54, 0, -1));
    assert(trace->depth_ref == shooter && trace->release_life);
    while (trace->release_life && tracers[0].life > 1) update_tracers();
    assert(tracers[0].life && !trace->release_life && trace->bullet_layer == 5);

    clear_tracers();
    assert(sc64_source_unit_vfx(&u[shooter], u[target].x, u[target].y, 0, target));
    assert(trace->depth_ref == target && trace->release_life);
    u[target].alive = 0;
    int replacement = make_unit(ENEMY, U_WRAITH, 8, 208, 54);
    assert(replacement == target && trace->depth_ref == -1);
    while (trace->release_life && tracers[0].life > 1) update_tracers();
    assert(tracers[0].life && !trace->release_life && trace->bullet_layer == 5);
}
#endif

static int sprite_frame_equal(const Sc64SpriteFrame *a, const Sc64SpriteFrame *b)
{
    return a == b || (a && b && a->offset_x == b->offset_x &&
        a->offset_y == b->offset_y && a->w == b->w && a->h == b->h &&
        a->canvas_w == b->canvas_w && a->canvas_h == b->canvas_h &&
        memcmp(a->pixels, b->pixels, (size_t)a->w * a->h) == 0);
}

/* Canonical profiles may deliver weapon damage through the impact queue. */
static void wait_for_native_impact(int target, int before)
{
#ifdef SC64_IMPACT_EVENTS
    for (int tick = 0; tick < 256 && u[target].hp == before; ++tick)
        race_impact_tick();
#else
    (void)target; (void)before;
#endif
}

static void test_scout_air_combat(void)
{
    reset_world();
    int scout = make_unit(SIDE, U_SCOUT, 70, 100, 100);
    int target = make_unit(ENEMY, U_MUTALISK, 43, 124, 100);
    int before = u[target].hp;
    tick_combat(&u[scout], target);
    wait_for_native_impact(target, before);
    assert(u[target].hp < before);
    assert(live_tracers() == 2);
    unsigned found = 0;
    for (int i = 0; i < MAX_TRACERS; ++i) {
        Sc64WeaponSourceVfxKey key;
        if (tracers[i].life <= 0) continue;
        assert(sc64_weapon_source_vfx_unpack((uint32_t)tracers[i].color, &key));
        assert(key.actor_raw == 70 && key.weapon_id == 74 && key.target_air == 1);
        assert(key.actor_slot_plus1 == (unsigned)scout + 1u);
        ++found;
    }
    assert(found == 2);
}

static void test_zealot_pose_lifecycle(void)
{
    reset_world();
    unsigned int full_ticks = sc64_unit_attack_source_full_duration(65, 0);
    unsigned char release_tick = sc64_unit_attack_source_release_tick(65, 0);
    assert(full_ticks > release_tick && release_tick > 0);
    const Sc64SpriteFrame *first = sc64_unit_attack_source_full_frame(65, 0, 8, 0);
    const Sc64SpriteFrame *last = 0;
    int changed_full_pose = 0;
    assert(first && first->pixels && first->w && first->h);
    for (unsigned int tick = 1; tick < full_ticks; ++tick) {
        const Sc64SpriteFrame *frame =
            sc64_unit_attack_source_full_frame(65, 0, 8, tick);
        assert(frame && frame->pixels);
        if (!sprite_frame_equal(first, frame)) changed_full_pose = 1;
        last = frame;
    }
    assert(changed_full_pose); /* Source windup/release frames visibly change. */
    assert(sc64_unit_attack_source_full_frame(65, 0, 8, full_ticks) == 0);
    assert(sprite_frame_equal(last,
        sc64_unit_attack_source_frame(65, 0, 8, full_ticks - 1u - release_tick)));

    int zealot = make_unit(SIDE, U_ZEALOT, 65, 100, 100);
    int target = make_unit(ENEMY, U_ZERGLING, 37, 106, 100);
    int before = u[target].hp;
    tick_combat(&u[zealot], target);
    wait_for_native_impact(target, before);
    assert(u[target].hp < before);
    assert(u[zealot].cd > 0);
    update_tracers();
    const Sc64SpriteFrame *pose = sc64_source_unit_attack_pose(&u[zealot]);
    assert(pose && pose->pixels && pose->w && pose->h);
    u[zealot].x++;
    assert(sc64_source_unit_attack_pose(&u[zealot]) == 0);

    /* Reusing the same physical slot with a new identity cannot inherit the
     * outgoing unit's source pose, even while its old tracer is still alive. */
    int slot = zealot;
    u[slot].alive = 0;
    int replacement = make_unit(SIDE, U_MARINE, 0, 180, 140);
    assert(replacement == slot);
    assert(sc64_source_unit_attack_pose(&u[replacement]) == 0);
}

static void test_photon_cannon_defense_emission(void)
{
    reset_world();
    race_human = R_PROTOSS;
    int pylon = spawn_build(SIDE, B_PYLON, 160, 160, 0);
    int cannon = spawn_build(SIDE, B_PHOTON_CANNON, 160, 160, 0);
    assert(pylon >= 0 && cannon >= 0);
    b[pylon].raw_type = 156;
    b[cannon].raw_type = 162;
    assert(race_site_powered(SIDE, b[cannon].x, b[cannon].y));
    int target = make_unit(ENEMY, U_ZEALOT, 65, 190, 160);
    int before = u[target].hp;
    b[cannon].queue_progress = 0;
    race_tick_defenses();
    wait_for_native_impact(target, before);
    assert(u[target].hp < before);
    unsigned tagged = 0;
    for (int i = 0; i < MAX_TRACERS; ++i) {
        Sc64WeaponSourceVfxKey key;
        if (!tracers[i].life || !sc64_weapon_source_vfx_unpack((uint32_t)tracers[i].color, &key)) continue;
        if (key.actor_raw == 162 && key.actor_slot_plus1 == 0 &&
            (key.weapon_id == 80 || key.weapon_id == 81)) ++tagged;
    }
    assert(tagged > 0);
}

static void test_archon_splash_emission(void)
{
#ifdef SC64_P7_ARCHON_SPLASH
    reset_world();
    int archon = make_unit(SIDE, U_P7_ARCHON, 68, 100, 100);
    int target = make_unit(ENEMY, U_ZEALOT, 65, 116, 100);
    int before = u[target].hp;
    tick_combat(&u[archon], target);
    wait_for_native_impact(target, before);
    assert(u[target].hp < before);
    assert(live_tracers() > 0);
    Sc64WeaponSourceVfxKey key;
    assert(sc64_weapon_source_vfx_unpack((uint32_t)tracers[0].color, &key));
    assert(key.actor_raw == 68 && key.weapon_id == 70);
    assert(key.actor_slot_plus1 == (unsigned)archon + 1u);
#endif
}

static void test_special_event_source_vfx(void)
{
#if defined(SC64_T5_RUNTIME) && defined(SC64_WEAPON_SOURCE_BANK_VERSION)
    reset_world();
    int mine = make_unit(SIDE, U_SPIDER_MINE, 13, 120, 100);
    int victim = make_unit(ENEMY, U_ZEALOT, 65, 122, 100);
    int hp = u[victim].hp;
    Sc64StockAttack mine_attack;
    assert(sc64_stock_attack_select(13, 0, &mine_attack));
    assert(mine_attack.weapon_id == 6); /* Stock Spider Mine suicide weapon. */
    sc64_t5_mine_explode(&u[mine]);
    wait_for_native_impact(victim, hp);
    assert(!u[mine].alive && u[victim].hp < hp);
    assert(has_source_tracer(13, mine_attack.weapon_id, (unsigned)mine + 1u));
#endif
#if defined(SC64_P6_COMBAT_ENABLED) && defined(SC64_WEAPON_SOURCE_BANK_VERSION)
    reset_world();
    int infested = make_unit(SIDE, U_P6_INFESTED_TERRAN,
                             SC64_P6_RAW_INFESTED_TERRAN, 120, 100);
    Sc64StockAttack attack;
    assert(sc64_stock_attack_select(SC64_P6_RAW_INFESTED_TERRAN, 0, &attack));
    assert(sc64_source_unit_vfx(&u[infested], 121, 100, 0, -1));
    assert(has_source_tracer(SC64_P6_RAW_INFESTED_TERRAN, attack.weapon_id,
                             (unsigned)infested + 1u));
#endif
#if defined(SC64_T4_SCENARIO_RUNTIME) && defined(SC64_WEAPON_SOURCE_BANK_VERSION)
    reset_world();
    int trap = spawn_build(SIDE, B_WALL_FLAME_TRAP, 120, 100, 0);
    assert(trap >= 0);
    b[trap].raw_type = 211;
    b[trap].raw_owner = 6;
    assert(sc64_source_building_vfx(&b[trap], 130, 100, 0, -1));
    assert(has_source_tracer(211, 98, 0));
#endif
}

int main(void)
{
    reset_world();
    test_all_source_profiles();
    test_scout_air_combat();
    test_zealot_pose_lifecycle();
    test_photon_cannon_defense_emission();
    test_archon_splash_emission();
    test_special_event_source_vfx();
#ifdef SC64_PROJECTILE_DEPTH
    test_moving_projectile_overlap();
    test_ground_projectile_world_y();
    test_stationary_projectile_depth();
#ifdef SC64_P4_STORM_ENABLED
    test_flight_trail_depth();
#endif
    test_attached_attacker_depth();
    test_detached_attack_sprite_depth();
    test_projectile_release_layer();
#endif
    puts("PASS native weapon source engine integration");
    return 0;
}
