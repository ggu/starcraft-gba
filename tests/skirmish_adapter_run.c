/* Host integration fixture for the actual owner-aware skirmish adapter.
 * game.c is included so this test can inspect the same fixed pools and step
 * the real construction, production and combat routines. */
#include <stdio.h>
#include "../starcraft/game.c"

static int
owner_units(int owner, int type)
{
    int i, count = 0;
    for (i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == owner && (type < 0 || u[i].type == type)) ++count;
    return count;
}

int
main(void)
{
    int x, y, worker, barracks = -1, i, zerg;
    Sc64AiIntent build = { SC64_AI_INTENT_BUILD, SC64_AI_BUILD_BARRACKS, 0, 0, 1, 150, 0 };
    Sc64AiIntent train = { SC64_AI_INTENT_TRAIN, SC64_AI_TRAIN_SOLDIER, 0, 0, 1, 50, 0 };
    gba_init();
    game_init();
    game_update(KEY_A, KEY_A);

    if (owner_units(SIDE, U_SCV) != 4 || owner_units(ENEMY, U_SCV) != 4 ||
        owner_units(RESCUABLE, U_SCV) != 4 || owner_minerals(ENEMY) != 200 ||
        owner_minerals(RESCUABLE) != 200) {
        fprintf(stderr, "setup: workers=%d/%d/%d banks=%d/%d\n",
            owner_units(SIDE, U_SCV), owner_units(ENEMY, U_SCV), owner_units(RESCUABLE, U_SCV),
            owner_minerals(ENEMY), owner_minerals(RESCUABLE));
        return 1;
    }

    /* The first bot pays the real SCV construction order and completion time;
     * the second bot's independently held bank is untouched. */
    if (!skirmish_barracks_site(ENEMY, &x, &y)) return 2;
    worker = skirmish_find_worker(ENEMY, x, y);
    if (worker < 0 || !owner_start_build(ENEMY, worker, B_BARRACKS, x, y) ||
        !sc64_ai_commit(&skirmish_ai[0], &build)) return 3;
    if (owner_minerals(ENEMY) != 50 || owner_minerals(RESCUABLE) != 200) return 4;
    for (i = 0; i < building_build_time(B_BARRACKS) + 120; ++i) update_orders();
    for (i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == ENEMY && b[i].type == B_BARRACKS && !b[i].under_construction)
            barracks = i;
    if (barracks < 0) return 5;

    /* Queue and complete a Marine through the actual production timer. */
    if (!owner_queue_unit(ENEMY, barracks, U_MARINE) ||
        !sc64_ai_commit(&skirmish_ai[0], &train)) return 6;
    if (owner_minerals(ENEMY) != 0 || owner_minerals(RESCUABLE) != 200) return 7;
    for (i = 0; i < unit_build_time(U_MARINE) + 1; ++i) update_production();
    if (owner_units(ENEMY, U_MARINE) != 1 || owner_units(RESCUABLE, U_MARINE) != 0) return 8;

    /* Actual combat target selection must treat owner 3 as hostile to owner 1. */
    zerg = spawn_unit(RESCUABLE, U_ZERGLING, 320, 320);
    if (zerg < 0) return 9;
    for (i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == ENEMY && u[i].type == U_MARINE) {
            u[i].x = 312; u[i].y = 320;
        }
    owner_issue_order(ENEMY, ORDER_ATTACK_MOVE, 320, 320);
    for (i = 0; i < 100; ++i) update_orders();
    if (u[zerg].alive && u[zerg].hp >= u[zerg].maxhp) return 10;

    puts("skirmish adapter: independent banks, SCV construction, timed production and hostile combat pass");
    return 0;
}
