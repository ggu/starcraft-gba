/* Internal T6 world-binding fixture. Direct mutations here isolate bounded
 * callbacks; this is not a controller-played mission-completion proof. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

static void
start_t6(void)
{
    game_init();
    race_mode = 0;
    race_selected_mission = SC64_T6_GLOBAL_ID;
    init_state();
    assert(race_t6_active() && screen == SCREEN_PLAY);
    assert(race_cols == 96 && race_rows == 96);
    assert(!mission_unsupported_count && !won && !lost);
}

static void
owners_and_initial_burrow_state(void)
{
    start_t6();
    assert(sc64_t6_runtime_owner(0) == SIDE);
    assert(sc64_t6_runtime_owner(1) == ENEMY);
    assert(sc64_t6_runtime_owner(2) == RESCUABLE);
    assert(sc64_t6_runtime_owner(4) == 4);
    assert(sc64_t6_runtime_owner(6) == RESCUABLE);
    assert(sc64_t6_runtime_owner(3) == NEUTRAL);
    assert(sc64_t6_runtime_owner(5) == NEUTRAL);
    assert(sc64_t6_runtime_owner(7) == NEUTRAL);
    assert(sc64_t6_runtime_owner(8) == -1);

    assert(!sc64_t6_owners_hostile(SIDE, RESCUABLE));
    assert(!sc64_t6_owners_hostile(ENEMY, 4));
    assert(sc64_t6_owners_hostile(SIDE, ENEMY));
    assert(sc64_t6_owners_hostile(RESCUABLE, 4));
    assert(!sc64_t6_owners_hostile(NEUTRAL, SIDE));
    assert(!sc64_t6_owners_hostile(NEUTRAL, ENEMY));

    int burrowed = 0, units = 0, buildings = 0;
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive) {
        ++units;
        assert(race_t6_unit_burrowed(i) ==
               (u[i].raw_owner == 1 && (u[i].raw_type == 37 || u[i].raw_type == 38) &&
                (u[i].flags & 2u)));
        burrowed += race_t6_unit_burrowed(i);
    }
    for (int i = 0; i < MAX_BUILDINGS; ++i) buildings += !!b[i].alive;
    assert(units == 104 && buildings == 74 && burrowed == 4);
    assert(race_t6.rescue_mask == 0);
}

static void
query_bounds_cover_start_and_producible_actors(void)
{
    const struct { unsigned raw; int building; } cases[] = {
        {0,0}, {1,0}, {2,0}, {3,0}, {7,0}, {8,0}, {11,0}, {19,0},
        {32,0}, {34,0}, {37,0}, {38,0}, {41,0}, {42,0}, {43,0}, {45,0},
        {47,0}, {106,1}, {109,1}, {110,1}, {111,1}, {112,1}, {113,1},
        {114,1}, {115,1}, {116,1}, {122,1}, {124,1}, {125,1}, {126,1},
        {132,1}, {133,1}, {135,1}, {138,1}, {139,1}, {141,1}, {142,1},
        {144,1}, {146,1}, {149,1}, {195,1}
    };
    Sc64T4QueryRect rect;
    for (unsigned i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i)
        assert(sc64_t6_actor_rect(cases[i].raw, cases[i].building, 256, 256, &rect));
    assert(rect.left == 512 - 48 && rect.top == 512 - 32 &&
           rect.right == 512 + 47 + 1 && rect.bottom == 512 + 31 + 1);
    assert(!sc64_t6_actor_rect(115, 0, 256, 256, &rect));
    assert(!sc64_t6_actor_rect(999, 1, 256, 256, &rect));
}

static void
queries_creation_and_resources(void)
{
    start_t6();
    u8 was_alive[MAX_UNITS] = {0};
    Sc64T4Condition command = {0};
    u32 count = 0;
    command.opcode = 2; command.player = 17; command.unit = 229;
    command.comparison = 0; command.flags = 16;
    assert(sc64_t6_world_count(&race_t6, &command, 0, &count));
    assert(count == 178); /* Every live source unit/building, resources excluded. */

    int before = 0;
    for (int i = 0; i < MAX_UNITS; ++i) {
        was_alive[i] = (u8)u[i].alive;
        before += u[i].alive && (u[i].raw_type == 37 || u[i].raw_type == 38);
    }
    unsigned created = 0;
    const Sc64T4Action *row = sc64_t6_scenario.triggers[10].actions;
    for (unsigned i = 0; i < 64 && row[i].opcode; ++i) {
        if (row[i].opcode != 11) continue;
        assert(sc64_t6_world_create(&race_t6, &row[i], 0) == 1);
        ++created;
    }
    assert(created == 5);
    int after = 0;
    for (int i = 0; i < MAX_UNITS; ++i) if (u[i].alive) {
        if (u[i].raw_type == 37 || u[i].raw_type == 38) ++after;
        if (!was_alive[i] && u[i].raw_owner == 4 &&
            (u[i].raw_type == 37 || u[i].raw_type == 38))
            assert(race_t6_unit_burrowed(i));
    }
    assert(after == before + 5);

    int saw_human = 0, saw_force3 = 0, saw_force4 = 0;
    for (unsigned t = 0; t < sc64_t6_scenario.trigger_count; ++t)
        for (unsigned i = 0; i < 64; ++i) {
            const Sc64T4Action *a = &sc64_t6_scenario.triggers[t].actions[i];
            if (!a->opcode) break;
            if (a->opcode != 26) continue;
            assert(sc64_t6_world_resources(&race_t6, a, 0) == 1);
            if (a->player == 0) saw_human = 1;
            if (a->player == 20) saw_force3 = 1;
            if (a->player == 21) saw_force4 = 1;
        }
    assert(saw_human && saw_force3 && saw_force4);
    assert(race_t6.resources[0].current_minerals == 75);
    assert(race_t6.resources[4].current_minerals == 8000);
    assert(race_t6.resources[1].current_gas == 8000);
}

static void
ai_deaths_rescue_and_tick(void)
{
    start_t6();
    unsigned seen_ai = 0;
    for (unsigned t = 0; t < sc64_t6_scenario.trigger_count; ++t)
        for (unsigned i = 0; i < 64; ++i) {
            const Sc64T4Action *a = &sc64_t6_scenario.triggers[t].actions[i];
            if (!a->opcode) break;
            if (a->opcode != 15 && a->opcode != 16) continue;
            unsigned owner = a->argument == SC64_T6_SCRIPT_RSCU ? 2u :
                             a->argument == SC64_T6_SCRIPT_TER6 ? 4u : 1u;
            assert(sc64_t6_world_ai(&race_t6, a, owner) == 1);
            seen_ai |= a->argument == SC64_T6_SCRIPT_RSCU ? 1u :
                       a->argument == SC64_T6_SCRIPT_TER6 ? 2u : 4u;
        }
    assert(seen_ai == 7);
    assert(race_t6.ai_requests[2] & SC64_T6_AI_RSCU);
    assert(race_t6.ai_requests[4] & SC64_T6_AI_T6A);
    assert(race_t6.ai_requests[1] & SC64_T6_AI_T6B);

    race_t6_building_died(ENEMY, 1);
    race_t6_building_died(4, 4);
    race_t6_building_died(SIDE, 0);
    assert(race_t6.deaths[1] == 1 && race_t6.deaths[4] == 1 &&
           race_t6.deaths[0] == 0);
    Sc64T4Condition deaths = {0};
    u32 count = 0;
    deaths.opcode = 15; deaths.unit = 231; deaths.player = 1;
    deaths.comparison = 0; deaths.amount = 1; deaths.flags = 16;
    assert(sc64_t6_world_count(&race_t6, &deaths, 0, &count) && count == 1);
    deaths.player = 4;
    assert(sc64_t6_world_count(&race_t6, &deaths, 0, &count) && count == 1);
    deaths.player = 0;
    assert(!sc64_t6_world_count(&race_t6, &deaths, 0, &count));

    int side = -1, rescue = -1;
    for (int i = 0; i < MAX_UNITS; ++i) {
        if (u[i].alive && u[i].owner == SIDE && side < 0) side = i;
        if (u[i].alive && u[i].owner == RESCUABLE && rescue < 0) rescue = i;
    }
    assert(side >= 0 && rescue >= 0);
    u[rescue].x = u[side].x; u[rescue].y = u[side].y;
    int old_source = u[rescue].raw_owner;
    sc64_t6_rescue_tick();
    assert(u[rescue].owner == SIDE && u[rescue].raw_owner == old_source);
    assert(race_t6.rescue_mask & (1u << old_source));

    /* Exercise the integrated callback scheduler for a few ordinary ticks;
     * this checks fault-free binding only, not campaign completion. */
    for (int i = 0; i < 4; ++i) assert(race_t6_tick());
    for (unsigned i = 0; i < 8; ++i) assert(!race_t6.triggers.player[i].fault);
    int burrowed = -1;
    for (int i = 0; i < MAX_UNITS; ++i)
        if (race_t6_unit_burrowed(i)) { burrowed = i; break; }
    assert(burrowed >= 0);
    u[burrowed].alive = 0;
    assert(race_t6_tick());
    assert(!sc64_t6_burrowed_get(&race_t6, (unsigned)burrowed));
}

int main(void)
{
    owners_and_initial_burrow_state();
    query_bounds_cover_start_and_producible_actors();
    queries_creation_and_resources();
    ai_deaths_rescue_and_tick();
    puts("sc64 T6 world binding: ok");
    return 0;
}
