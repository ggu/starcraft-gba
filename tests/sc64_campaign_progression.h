/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Shared tail of the campaign progression fixtures. A wrapper first includes one
 * ordinary-input mission controller with its main() renamed controller_main(),
 * then includes this file and defines PROGRESSION_* below. The wrapper's main()
 * calls progression_after_win() once the controller has returned:
 *
 *   win -> result screen -> A -> the next catalog mission starts in this same
 *   process, with the right mission id and a world identical to that mission's
 *   ordinary menu launch.
 *
 * Nothing is written to game state; every step is a game_update() call. */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#ifndef PROGRESSION_FACTION
#error "define PROGRESSION_FACTION, PROGRESSION_MISSION and PROGRESSION_NEXT"
#endif

static u16 progression_held;

static void progression_input(u16 held)
{
    game_update(held, (u16)(held & (u16)~progression_held));
    progression_held = held;
}

static void progression_press(u16 key) { progression_input(key); progression_input(0); }

/* Order-sensitive digest of the live world: actors, resources and the clock. */
static u32 progression_world_digest(void)
{
    u32 h = 2166136261u;
#define PMIX(v) do { u32 x_ = (u32)(v); for (int k_ = 0; k_ < 4; ++k_) { h ^= (x_ >> (8 * k_)) & 255u; h *= 16777619u; } } while (0)
    for (int i = 0; i < MAX_UNITS; ++i) {
        PMIX(u[i].alive);
        if (!u[i].alive) continue;
        PMIX(u[i].x); PMIX(u[i].y); PMIX(u[i].hp); PMIX(u[i].type); PMIX(u[i].owner);
        PMIX(u[i].order); PMIX(u[i].raw_type); PMIX(u[i].raw_owner);
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i) {
        PMIX(b[i].alive);
        if (!b[i].alive) continue;
        PMIX(b[i].x); PMIX(b[i].y); PMIX(b[i].hp); PMIX(b[i].type); PMIX(b[i].owner);
        PMIX(b[i].raw_type); PMIX(b[i].raw_owner);
    }
    PMIX(frame); PMIX(minerals); PMIX(gas); PMIX(won); PMIX(lost);
#undef PMIX
    return h;
}

static int progression_count(int owner, int buildings)
{
    int n = 0;
    if (buildings) { for (int i = 0; i < MAX_BUILDINGS; ++i) n += b[i].alive && b[i].owner == owner; }
    else { for (int i = 0; i < MAX_UNITS; ++i) n += u[i].alive && u[i].owner == owner; }
    return n;
}

/* The ordinary menu launch of one mission: HOME, mission page, row, briefing, START. */
static void progression_launch_from_menu(int mission)
{
    int ordinal;
    game_init();
    progression_held = 0;
    ordinal = sc64_catalog_playable_ordinal(PROGRESSION_FACTION, mission);
    assert(ordinal >= 0);
    assert(race_front.page == SC_FRONT_HOME);
    progression_press(KEY_A);
    assert(race_front.page == SC_FRONT_MISSION);
    for (int page = 0; page < ordinal / 2; ++page) progression_press(KEY_RIGHT);
    if (ordinal & 1) progression_press(KEY_DOWN);
    progression_press(KEY_A);
    assert(race_front.page == SC_FRONT_BRIEFING && race_front.mission == mission);
    progression_press(KEY_START);
    assert(screen == SCREEN_PLAY && !race_mode && !won && !lost);
    assert(race_selected_mission == mission);
}

/* Call after the controller has returned with the mission won. */
static int progression_after_win(void)
{
    int mission = PROGRESSION_MISSION, next = PROGRESSION_NEXT;
    u32 carried_completed, reference_digest, advanced_digest;
    int ref_units, ref_buildings, ref_frame;

    assert(sc64_catalog_available_next(PROGRESSION_FACTION, mission) == next);
    assert(won && !lost && race_selected_mission == mission);

    /* A text box may still stand between the win and the result screen. Dismiss
     * it with A, as a player would; the result screen must arrive. */
    progression_held = 0;
    for (int i = 0; i < 1200 && screen != SCREEN_RESULT; ++i) {
        progression_input((i & 1) ? 0 : KEY_A);
    }
    progression_input(0);
    assert(screen == SCREEN_RESULT && won && !lost);
    assert(race_completed & (1u << mission));
    assert(!(race_completed & (1u << next)));
    carried_completed = (u32)race_completed;
    printf("progression: mission %d won, result screen at frame %d, completed=%08x\n",
           mission, frame, (unsigned)carried_completed);

    /* A on the result screen goes to the next catalog mission. */
    progression_press(KEY_A);
    assert(screen == SCREEN_PLAY && !won && !lost && !race_mode);
    assert(race_selected_mission == next);
    assert(race_campaign.mission == next);
    assert(frame <= 2);
    assert(race_completed == (int)carried_completed); /* progress carries over */
    advanced_digest = progression_world_digest();
    ref_units = progression_count(SIDE, 0) + progression_count(ENEMY, 0);
    ref_buildings = progression_count(SIDE, 1) + progression_count(ENEMY, 1);
    ref_frame = frame;
    assert(ref_units > 0 && ref_buildings > 0);

    /* The same mission through its ordinary menu launch is the reference. */
    progression_launch_from_menu(next);
    reference_digest = progression_world_digest();
    assert(frame == ref_frame);
    assert(progression_count(SIDE, 0) + progression_count(ENEMY, 0) == ref_units);
    assert(progression_count(SIDE, 1) + progression_count(ENEMY, 1) == ref_buildings);
    if (advanced_digest != reference_digest) {
        fprintf(stderr, "progression: world after A-next %08x differs from menu launch %08x\n",
                (unsigned)advanced_digest, (unsigned)reference_digest);
        return 1;
    }
    printf("progression: A on the result screen started mission %d (units=%d buildings=%d, digest %08x equals its menu launch)\n",
           next, ref_units, ref_buildings, (unsigned)advanced_digest);
    return 0;
}
