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
    memset(tracers, 0, sizeof(tracers));
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
            if (!sc64_source_emit(p->actor_raw, 0, p->weapon_id, p->mode,
                                  x0, y0, x1, y1)) {
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

static int sprite_frame_equal(const Sc64SpriteFrame *a, const Sc64SpriteFrame *b)
{
    return a == b || (a && b && a->offset_x == b->offset_x &&
        a->offset_y == b->offset_y && a->w == b->w && a->h == b->h &&
        a->canvas_w == b->canvas_w && a->canvas_h == b->canvas_h &&
        memcmp(a->pixels, b->pixels, (size_t)a->w * a->h) == 0);
}

static void test_scout_air_combat(void)
{
    reset_world();
    int scout = make_unit(SIDE, U_SCOUT, 70, 100, 100);
    int target = make_unit(ENEMY, U_MUTALISK, 43, 124, 100);
    int before = u[target].hp;
    tick_combat(&u[scout], target);
    assert(u[target].hp < before); /* Native damage remains immediate. */
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
    assert(!u[mine].alive && u[victim].hp < hp);
    assert(has_source_tracer(13, mine_attack.weapon_id, (unsigned)mine + 1u));
#endif
#if defined(SC64_P6_COMBAT_ENABLED) && defined(SC64_WEAPON_SOURCE_BANK_VERSION)
    reset_world();
    int infested = make_unit(SIDE, U_P6_INFESTED_TERRAN,
                             SC64_P6_RAW_INFESTED_TERRAN, 120, 100);
    Sc64StockAttack attack;
    assert(sc64_stock_attack_select(SC64_P6_RAW_INFESTED_TERRAN, 0, &attack));
    assert(sc64_source_unit_vfx(&u[infested], 121, 100, 0));
    assert(has_source_tracer(SC64_P6_RAW_INFESTED_TERRAN, attack.weapon_id,
                             (unsigned)infested + 1u));
#endif
#if defined(SC64_T4_SCENARIO_RUNTIME) && defined(SC64_WEAPON_SOURCE_BANK_VERSION)
    reset_world();
    int trap = spawn_build(SIDE, B_WALL_FLAME_TRAP, 120, 100, 0);
    assert(trap >= 0);
    b[trap].raw_type = 211;
    b[trap].raw_owner = 6;
    assert(sc64_source_building_vfx(&b[trap], 130, 100, 0));
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
    puts("PASS native weapon source engine integration");
    return 0;
}
