/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Ordinary-menu Z8 clock continuation and camera checks. */
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

#ifndef SC64_Z8_RUNTIME
#error "Compile this fixture with the zerg-z8 profile flags"
#endif

static unsigned held_keys;

static void input(unsigned held)
{
    game_update((u16)held, (u16)(held & ~held_keys));
    held_keys = held;
}

static void press(unsigned key) { input(key); input(0); }

static void launch_z8_from_normal_menu(void)
{
    int ordinal;
    game_init();
    held_keys = 0;
    assert(race_front.page == SC_FRONT_HOME);
    ordinal = sc64_catalog_playable_ordinal(SC64_FACTION_ZERG, SC64_Z8_GLOBAL_ID);
    assert(ordinal == 7);
    assert(sc64_catalog_playable_count(SC64_FACTION_ZERG) == 8);
    press(KEY_A);
    assert(race_front.page == SC_FRONT_MISSION && race_front.faction == SC_FRONT_ZERG);
    for (int page = 0; page < ordinal / 2; ++page) press(KEY_RIGHT);
    if (ordinal & 1) press(KEY_DOWN);
    press(KEY_A);
    assert(race_front.page == SC_FRONT_BRIEFING && race_front.mission == SC64_Z8_GLOBAL_ID);
    press(KEY_START);
    assert(screen == SCREEN_PLAY && !race_mode && !won && !lost);
    assert(race_selected_mission == SC64_Z8_GLOBAL_ID && race_z8_active());
    assert(mission_unsupported_count == 0);
}

/* Order-sensitive digest of the simulation and scheduler. */
static u32 world_digest(void)
{
    u32 h = 2166136261u;
#define MIX(v) do { u32 x_ = (u32)(v); for (int k_ = 0; k_ < 4; ++k_) { h ^= (x_ >> (8 * k_)) & 255u; h *= 16777619u; } } while (0)
    for (int i = 0; i < MAX_UNITS; ++i) {
        MIX(u[i].alive);
        if (!u[i].alive) continue;
        MIX(u[i].x); MIX(u[i].y); MIX(u[i].hp); MIX(u[i].type); MIX(u[i].owner);
        MIX(u[i].order); MIX(u[i].target); MIX(u[i].raw_owner); MIX(u[i].raw_type);
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i) {
        MIX(b[i].alive);
        if (!b[i].alive) continue;
        MIX(b[i].hp); MIX(b[i].type); MIX(b[i].owner); MIX(b[i].queue_count);
        MIX(b[i].raw_owner);
    }
    MIX(frame); MIX(minerals); MIX(gas);
    for (unsigned i = 0; i < sizeof(race_z8_sched); ++i) MIX(((const u8 *)&race_z8_sched)[i]);
    for (unsigned i = 0; i < sizeof(race_z8_ai); ++i) MIX(((const u8 *)&race_z8_ai)[i]);
    for (unsigned t = 0; t < SC64_Z8_AI_TOWNS; ++t) { MIX(race_z8_ai_minerals[t]); MIX(race_z8_ai_gas[t]); }
#undef MIX
    return h;
}

int main(void)
{
    /* Observe the authored camera action rather than fabricating its wait. */
    launch_z8_from_normal_menu();
    for (int i = 0; i < 2000 && !race_z8_camera_release; ++i) {
        assert(screen == SCREEN_PLAY);
        input(0);
        assert(race_z8_sched.frames == (u32)(4u * (unsigned)frame / 5u));
    }
    assert(race_z8_camera_release);
    {
        unsigned prior = (unsigned)frame;
        u32 native = race_z8_sched.frames;
        input(0);
        if (!sc64_z8_native_step(prior + 1u)) {
            assert(race_z8_camera_release && race_z8_sched.frames == native);
            input(0);
        }
        assert(!race_z8_camera_release && race_z8_sched.frames == native + 1u);
    }
    for (unsigned phase = 0; phase < 5; ++phase) {
        u32 expected;
        launch_z8_from_normal_menu();
        for (int i = 0; i < 150; ++i) {
            assert(screen == SCREEN_PLAY);
            input(0);
            assert(race_z8_sched.frames == (u32)(4u * (unsigned)frame / 5u));
        }
        assert(minerals == 350 && gas == 350);
        assert(race_z8_ai_minerals[0] >= 7000u && race_z8_ai_minerals[1] >= 7000u);
        assert(race_z8_ai.started == 3u && sc64_z8_switch_get(&race_z8_sched, 0));
        for (int i = 0; i < 20 &&
             ((unsigned)frame % 5u != phase || race_z8_camera_release); ++i) {
            assert(screen == SCREEN_PLAY);
            input(0);
        }
        assert(screen == SCREEN_PLAY && (unsigned)frame % 5u == phase &&
               !race_z8_camera_release);
        press(KEY_START);
        assert(screen == SCREEN_PAUSE);
        unsigned paused_frame = (unsigned)frame;
        u32 paused_native = race_z8_sched.frames;
        for (int i = 0; i < 10; ++i) input(0);
        assert((unsigned)frame == paused_frame && race_z8_sched.frames == paused_native);
        press(KEY_B); /* ordinary pause-menu save */
        press(KEY_START);
        for (int i = 0; i < 20; ++i) input(0);
        expected = world_digest();
        game_init();
        held_keys = 0;
        press(KEY_L); /* ordinary title load */
        assert(screen == SCREEN_PLAY && race_z8_active());
        for (int i = 0; i < 20; ++i) input(0);
        assert(world_digest() == expected);
    }
    puts("Z8 ordinary save/load continuation at all five clock phases");
    return 0;
}
