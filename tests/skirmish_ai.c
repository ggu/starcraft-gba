#include <assert.h>
#include <stdio.h>
#include "../starcraft/skirmish_ai.h"

static int commit_kind(Sc64AiState *ai, Sc64AiWorld *world, unsigned char kind,
                       unsigned char type)
{
    Sc64AiIntent intents[SC64_AI_MAX_INTENTS];
    unsigned char n = sc64_ai_plan(ai, world, intents), i;
    for (i = 0; i < n; ++i) {
        if (intents[i].kind == kind && (!type || intents[i].type == type))
            return sc64_ai_commit(ai, &intents[i]);
    }
    return 0;
}

static void test_independent_economies_and_production(void)
{
    Sc64AiState a, b;
    Sc64AiWorld empty = {0};
    unsigned short a_before, b_before;
    sc64_ai_init(&a, 0, 1, 360, 30, 4, 20);
    sc64_ai_init(&b, 3, 2, 80, 0, 4, 20);
    a.desired_gas_workers = 2;

    assert(commit_kind(&a, &empty, SC64_AI_INTENT_MINE, 0));
    assert(a.workers_mining == 2 && a.workers_gas == 2);
    sc64_ai_economy_tick(&a);
    sc64_ai_economy_tick(&b);
    assert(a.minerals == 362 && a.gas == 32);
    assert(b.minerals == 84 && b.gas == 0);

    a_before = a.minerals;
    b_before = b.minerals;
    assert(commit_kind(&a, &empty, SC64_AI_INTENT_BUILD, SC64_AI_BUILD_BARRACKS));
    assert(a.minerals == a_before - 150 && b.minerals == b_before);
    assert(a.queued_buildings == 1 && !a.barracks);
    sc64_ai_building_completed(&a, SC64_AI_BUILD_BARRACKS);
    assert(a.barracks && !a.queued_buildings);

    assert(commit_kind(&a, &empty, SC64_AI_INTENT_TRAIN, SC64_AI_TRAIN_WORKER));
    assert(a.minerals == a_before - 200 && a.queued_workers == 1);
    sc64_ai_unit_completed(&a, SC64_AI_TRAIN_WORKER);
    assert(a.workers == 5 && a.supply_used == 5 && a.queued_workers == 0);
    assert(commit_kind(&a, &empty, SC64_AI_INTENT_TRAIN, SC64_AI_TRAIN_HEAVY));
    assert(a.queued_army == 1 && a.minerals == a_before - 250 && a.gas == 7);
    sc64_ai_unit_completed(&a, SC64_AI_TRAIN_HEAVY);
    assert(a.army == 1 && a.supply_used == 6 && a.queued_army == 0);

    sc64_ai_economy_tick(&b);
    assert(b.minerals == 88 && b.army == 0 && b.workers == 4);
}

static void test_teams_neutrals_and_explicit_diplomacy(void)
{
    Sc64AiState a;
    Sc64AiWorld w = {5, {
        {1, 1, 1, 0, 12, 1},  /* same team: excluded by default */
        {2, 2, 1, 0, 4, 5},   /* different team */
        {4, 3, 1, 1, 99, 0},  /* neutral: always excluded */
        {5, 1, 1, 0, 2, 9},   /* same-team intruder: can be explicitly hostile */
        {6, 4, 0, 0, 5, 2}    /* dead */
    }};
    Sc64AiIntent intents[SC64_AI_MAX_INTENTS];
    unsigned char n, i;
    sc64_ai_init(&a, 0, 1, 0, 0, 4, 20);
    a.barracks = 1;
    a.army = 3;
    a.supply_used = 7;
    a.desired_workers = 4;
    assert(!sc64_ai_hostile(&a, &w.targets[0]));
    assert(sc64_ai_hostile(&a, &w.targets[1]));
    assert(!sc64_ai_hostile(&a, &w.targets[2]));
    assert(!sc64_ai_hostile(&a, &w.targets[4]));

    sc64_ai_set_relation(&a, 1, SC64_AI_REL_HOSTILE);
    sc64_ai_set_relation(&a, 2, SC64_AI_REL_ALLY);
    assert(sc64_ai_hostile(&a, &w.targets[0]));
    assert(!sc64_ai_hostile(&a, &w.targets[1]));
    sc64_ai_set_relation(&a, 2, SC64_AI_REL_DEFAULT);
    n = sc64_ai_plan(&a, &w, intents);
    for (i = 0; i < n; ++i) if (intents[i].kind == SC64_AI_INTENT_ATTACK) break;
    assert(i < n && intents[i].target_player == 1); /* nearest hostile by distance */

    /* Opposing AI gets its own team perspective and independently routes to 0. */
    {
        Sc64AiState b;
        sc64_ai_init(&b, 3, 2, 0, 0, 4, 20);
        b.army = 3;
        b.desired_workers = 4;
        n = sc64_ai_plan(&b, &w, intents);
        for (i = 0; i < n; ++i) if (intents[i].kind == SC64_AI_INTENT_ATTACK) break;
        assert(i < n && intents[i].target_player == 1);
    }
}

int main(void)
{
    test_independent_economies_and_production();
    test_teams_neutrals_and_explicit_diplomacy();
    puts("skirmish AI: independent banks, worker income, costs, completion and diplomacy pass");
    return 0;
}
