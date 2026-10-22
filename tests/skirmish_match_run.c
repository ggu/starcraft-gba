/* Full autonomous skirmish regression: only human input is the initial start.
 * This fixture includes game.c to inspect owner banks, fixed-pool entities and
 * the actual simulation outcome without using a debugger or emulator. */
#include <stdio.h>
#include "../starcraft/game.c"

static int
alive_units(int owner, int type)
{
    int i, n = 0;
    for (i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == owner && (type < 0 || u[i].type == type)) ++n;
    return n;
}

static int
owned_buildings(int owner, int type)
{
    int i, n = 0;
    for (i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == owner && (type < 0 || b[i].type == type)) ++n;
    return n;
}

static int
owned_queue(int owner)
{
    int i, n = 0;
    for (i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == owner) n += b[i].queue_count;
    return n;
}

static int
owned_attacking(int owner)
{
    int i, n = 0;
    for (i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == owner && u[i].order == ORDER_ATTACK_MOVE) ++n;
    return n;
}

static int
barracks_near_own_cc(int owner)
{
    int cc = owner_command_center(owner), i;
    if (cc < 0) return 0;
    for (i = 0; i < MAX_BUILDINGS; ++i)
        if (b[i].alive && b[i].owner == owner && b[i].type == B_BARRACKS)
            return dist2(b[cc].x, b[cc].y, b[i].x, b[i].y) <= 96 * 96;
    return 0;
}

static void
eliminate_owner(int owner)
{
    int i;
    for (i = 0; i < MAX_UNITS; ++i) if (u[i].owner == owner) u[i].alive = 0;
    for (i = 0; i < MAX_BUILDINGS; ++i) if (b[i].owner == owner) b[i].alive = 0;
}

int
main(void)
{
    int f, match_end_frame = 0, saw_mined_bank_difference = 0, saw_barracks = 0;
    int saw_all_home_barracks = 0;
    int saw_army = 0, saw_attack = 0, saved_bank[2], saved_queue[2];
    int human_distance = -1, bot2_distance = -1;
    Sc64AiWorld target_world;
    gba_init();
    game_init();
    game_update(KEY_UP, KEY_UP);
    game_update(0, 0);
    game_update(KEY_A, KEY_A);
    skirmish_make_world(0, &target_world);
    for (int i = 0; i < target_world.count; ++i) {
        if (target_world.targets[i].player == SIDE) human_distance = target_world.targets[i].distance;
        if (target_world.targets[i].player == RESCUABLE) bot2_distance = target_world.targets[i].distance;
    }
    if (human_distance <= bot2_distance || bot2_distance < 0) {
        fprintf(stderr, "target routing: first AI distances human=%d other bot=%d\n",
            human_distance, bot2_distance);
        return 9;
    }

    /* A single actual game_update loop drives both bots' workers, construction,
     * production, targeting, combat and the human defeat condition. Save at
     * the first long-match checkpoint while both production queues are busy. */
    for (f = 0; f < 30000 && !won && !lost; ++f) {
        game_update(0, 0);
        if (owner_minerals(ENEMY) != owner_minerals(RESCUABLE))
            saw_mined_bank_difference = 1;
        if (owned_buildings(ENEMY, B_BARRACKS) && owned_buildings(RESCUABLE, B_BARRACKS) &&
            owned_buildings(4, B_BARRACKS))
            saw_barracks = 1;
        if (barracks_near_own_cc(ENEMY) && barracks_near_own_cc(RESCUABLE) &&
            barracks_near_own_cc(4)) saw_all_home_barracks = 1;
        if (alive_units(ENEMY, U_MARINE) + alive_units(ENEMY, U_FIREBAT) > 0 &&
            alive_units(RESCUABLE, U_MARINE) + alive_units(RESCUABLE, U_FIREBAT) > 0)
            saw_army = 1;
        if (owned_attacking(ENEMY) || owned_attacking(RESCUABLE)) saw_attack = 1;

        if (frame == 3000) {
            saved_bank[0] = owner_minerals(ENEMY);
            saved_bank[1] = owner_minerals(RESCUABLE);
            saved_queue[0] = owned_queue(ENEMY);
            saved_queue[1] = owned_queue(RESCUABLE);
            if (!saved_queue[0] && !saved_queue[1]) return 1;
            if (!save_game()) return 2;
            skirmish_ai[0].minerals = 1;
            skirmish_ai[1].minerals = 2;
            for (int i = 0; i < MAX_BUILDINGS; ++i)
                if (b[i].alive && (b[i].owner == ENEMY || b[i].owner == RESCUABLE))
                    b[i].queue_count = 0;
            if (!load_game()) return 3;
            if (owner_minerals(ENEMY) != saved_bank[0] ||
                owner_minerals(RESCUABLE) != saved_bank[1] ||
                owned_queue(ENEMY) != saved_queue[0] ||
                owned_queue(RESCUABLE) != saved_queue[1]) return 4;
        }
    }
    if (!saw_mined_bank_difference || !saw_barracks || !saw_all_home_barracks || !saw_army || !saw_attack) {
        fprintf(stderr, "autonomous progress: banks=%d barracks=%d home=%d army=%d attack=%d frame=%d\n",
            saw_mined_bank_difference, saw_barracks, saw_all_home_barracks, saw_army, saw_attack, frame);
        return 5;
    }
    if (!lost || won) {
        fprintf(stderr, "long match outcome: frame=%d won=%d lost=%d\n", frame, won, lost);
        return 6;
    }
    match_end_frame = frame;

    /* Isolated outcome cases use direct fixture state setup: removing every
     * human unit/building must lose; removing every computer must win. */
    init_state();
    eliminate_owner(SIDE);
    game_update(0, 0);
    if (!lost || won) return 7;
    init_state();
    for (int i = 0; i < skirmish_bots; ++i) eliminate_owner(skirmish_owner_ids[i]);
    game_update(0, 0);
    if (!won || lost) return 8;

    printf("skirmish match: banks, builds, queues, attack waves, save/load and outcomes pass (match ended at frame %d)\n", match_end_frame);
    return 0;
}
