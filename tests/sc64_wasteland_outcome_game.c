/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Exercises the actual Wasteland outcome entry points with constructed worlds. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

static void reset_world(void)
{
    memset(u, 0, sizeof(u)); memset(b, 0, sizeof(b));
    won = lost = 0; frame = 0; minerals = 400; mission_objective_stage = 1;
#ifdef SC64_RACES
    race_mode = 0; race_selected_mission = SC64_RACE_MISSION_T1;
    memset(&race_campaign, 0, sizeof(race_campaign));
#endif
    u[0].alive = 1; u[0].owner = NEUTRAL; u[0].type = U_RAYNOR;
    u[0].x = 1000; u[0].y = 1000;
}
static void tick_outcome(void)
{
#ifdef SC64_RACES
    race_terran_tick();
#else
    check_imported_mission_outcome();
#endif
}
static void owned_unit(int slot, int type)
{ u[slot].alive = 1; u[slot].owner = SIDE; u[slot].type = type; u[slot].x = 1; u[slot].y = 1; }
static void owned_building(int slot, int type, int unfinished)
{ b[slot].alive = 1; b[slot].owner = SIDE; b[slot].type = type; b[slot].under_construction = unfinished; }
int main(void)
{
    /* Ten Marines and rescued Raynor cannot win on a foundation. */
    reset_world(); u[0].owner = SIDE;
    for (int i = 1; i <= 10; ++i) owned_unit(i, U_MARINE);
    owned_building(0, B_BARRACKS, 1); tick_outcome(); assert(!won && !lost);
    b[0].under_construction = 0; u[10].alive = 0; tick_outcome(); assert(!won && !lost);
    u[10].alive = 1; tick_outcome(); assert(won && !lost);

    /* The native mission stays active after all five SCVs die in combat. */
    reset_world(); u[0].owner = SIDE; minerals = 40;
    for (int i = 1; i <= 5; ++i) owned_unit(i, U_MARINE);
    for (int i = 6; i <= 10; ++i) owned_unit(i, U_SCV);
    owned_building(0, B_COMMAND, 0); owned_building(1, B_DEPOT, 0);
    for (int i = 6; i <= 10; ++i) {
        u[i].alive = 0;
        tick_outcome();
        assert(!lost && !won);
    }

    /* A live owned building counts as Any Unit, even after a mobile wipe. */
    reset_world(); owned_building(0, B_DEPOT, 1); tick_outcome(); assert(!won && !lost);
    b[0].owner = ENEMY; tick_outcome(); assert(lost && !won);

    /* Resource defeat includes a foundation and has inclusive bounds. */
    reset_world(); owned_unit(1, U_MARINE); minerals = 399;
    owned_building(0, B_BARRACKS, 1); tick_outcome(); assert(!lost && !won);
    b[0].type = B_COMMAND; tick_outcome(); assert(!lost && !won);
    b[0].alive = 0; minerals = 400; tick_outcome(); assert(!lost && !won);
    minerals = 399; tick_outcome(); assert(lost && !won);
    reset_world(); for (int i = 1; i <= 10; ++i) owned_unit(i, U_MARINE);
    minerals = 0; tick_outcome(); assert(!lost && !won);

    /* Raynor survives globally before rescue, but his death always fails. */
    reset_world(); owned_building(0, B_COMMAND, 0); tick_outcome(); assert(!lost && !won);
    u[0].alive = 0; tick_outcome(); assert(lost && !won);
    puts("Wasteland real outcome cases passed"); return 0;
}
