/*
 * Private-asset StarCraft 64 Wasteland completion replay.
 * Compile with SC64_IMPORTED_MISSION and the generated local include path.
 * This deliberately includes game.c so assertions can inspect state; every
 * player action after game_init is delivered only through game_update keys.
 */
#include <stdio.h>
#include <stdlib.h>
#include "../starcraft/game.c"

typedef struct { unsigned long tick; u16 held; } InputEvent;
typedef struct {
    unsigned long tick;
    int frame, minerals, marines, scv, zerg;
    int raynor_owner, raynor_alive;
} Checkpoint;
static InputEvent events[512];
static int event_count;
static Checkpoint checkpoints[64];
static int checkpoint_count;
static unsigned long input_tick;
static u16 previous_held;
static u16 movement_base;

static int raynor_slot(void);
static int alive_count(int owner, int type);
static void input_frame(u16 held);
static void press_key(u16 key);

static u32
mission_state_hash(void)
{
    const u8 *parts[] = {(const u8 *)u, (const u8 *)b, (const u8 *)m};
    const size_t sizes[] = {sizeof(u), sizeof(b), sizeof(m)};
    u32 hash = 2166136261u;
    size_t p, i;
    for (p = 0; p < sizeof(parts) / sizeof(parts[0]); p++)
        for (i = 0; i < sizes[p]; i++)
            hash = (hash ^ parts[p][i]) * 16777619u;
    hash = (hash ^ (u32)minerals) * 16777619u;
    hash = (hash ^ (u32)gas) * 16777619u;
    hash = (hash ^ (u32)frame) * 16777619u;
    hash = (hash ^ (u32)mission_objective_stage) * 16777619u;
    return hash;
}

static int
save_load_round_trip(void)
{
    int before_minerals = minerals;
    int before_frame = frame;
    u32 before_hash = mission_state_hash();
    /* Save, destroy the live run with a genuine restart, then load the slot. */
    movement_base &= (u16)~KEY_R;
    input_frame(KEY_START);
    input_frame(0);
    if (screen != SCREEN_PAUSE) { fprintf(stderr, "save/load: pause before save screen=%d\n", screen); return 0; }
    input_frame(KEY_B);
    input_frame(0);
    input_frame(KEY_R);
    input_frame(0);
    if (screen != SCREEN_PLAY || minerals == before_minerals) { fprintf(stderr, "save/load: restart screen=%d minerals=%d before=%d\n", screen, minerals, before_minerals); return 0; }
    input_frame(KEY_START);
    input_frame(0);
    if (screen != SCREEN_PAUSE) { fprintf(stderr, "save/load: pause before load screen=%d\n", screen); return 0; }
    input_frame(KEY_L);
    if (screen != SCREEN_PLAY || minerals != before_minerals || frame != before_frame ||
        mission_state_hash() != before_hash || raynor_slot() < 0 || !u[raynor_slot()].alive)
        { fprintf(stderr, "save/load: mismatch screen=%d mineral=%d/%d frame=%d/%d hash=%08x/%08x hero=%d alive=%d\n", screen, minerals, before_minerals, frame, before_frame, mission_state_hash(), before_hash, raynor_slot(), raynor_slot() >= 0 ? u[raynor_slot()].alive : 0); return 0; }
    /* Loading resumes play immediately; only now release L and advance. */
    input_frame(0);
    return screen == SCREEN_PLAY;
}

static int
abs_i(int x)
{
    return x < 0 ? -x : x;
}

static int
distance_squared(int x1, int y1, int x2, int y2)
{
    int dx = x1 - x2, dy = y1 - y2;
    return dx * dx + dy * dy;
}

static void
input_frame(u16 held)
{
    u16 pressed = held & (u16)~previous_held;
    if (held != previous_held && event_count < (int)(sizeof(events) / sizeof(events[0]))) {
        events[event_count].tick = input_tick;
        events[event_count].held = held;
        event_count++;
    }
    game_update(held, pressed);
    previous_held = held;
    input_tick++;
    if (input_tick % 1000 == 0 && checkpoint_count < (int)(sizeof(checkpoints) / sizeof(checkpoints[0]))) {
        Checkpoint *c = &checkpoints[checkpoint_count++];
        int hero = raynor_slot();
        c->tick = input_tick;
        c->frame = frame;
        c->minerals = minerals;
        c->marines = alive_count(SIDE, U_MARINE);
        c->scv = alive_count(SIDE, U_SCV);
        c->zerg = alive_count(ENEMY, U_ZERGLING);
        c->raynor_owner = hero >= 0 ? u[hero].owner : -1;
        c->raynor_alive = hero >= 0 && u[hero].alive;
    }
}

static void
press_key(u16 key)
{
    input_frame(movement_base | key);
    input_frame(movement_base);
}

/* Move the on-screen world cursor only via held D-pad inputs. With R held,
 * the game moves four world pixels per simulation update. */
static void
move_cursor_near(int target_x, int target_y)
{
    int guard = 0;
    while ((abs_i(cx - target_x) > 2 || abs_i(cy - target_y) > 2) && guard++ < 2000) {
        int dx = target_x - cx, dy = target_y - cy;
        u16 held = movement_base;
        if (dx > 2) held |= KEY_RIGHT;
        else if (dx < -2) held |= KEY_LEFT;
        if (dy > 2) held |= KEY_DOWN;
        else if (dy < -2) held |= KEY_UP;
        input_frame(held);
    }
}

/* Building placement needs a tile-exact cursor, so finish the final one or
 * two pixels at normal cursor speed after releasing R. */
static int
move_cursor_exact(int target_x, int target_y)
{
    int guard = 0;
    movement_base &= (u16)~KEY_R;
    while ((cx != target_x || cy != target_y) && guard++ < 2000) {
        int dx = target_x - cx, dy = target_y - cy;
        u16 held = movement_base;
        if (dx > 0) held |= KEY_RIGHT;
        else if (dx < 0) held |= KEY_LEFT;
        if (dy > 0) held |= KEY_DOWN;
        else if (dy < 0) held |= KEY_UP;
        input_frame(held);
    }
    return cx == target_x && cy == target_y;
}

static int
closest_enemy_indices(int x, int y, int out[2])
{
    int i, count = 0, best0 = -1, best1 = -1;
    int d0 = 0x7fffffff, d1 = 0x7fffffff;
    for (i = 0; i < MAX_UNITS; i++) if (u[i].alive && u[i].owner == ENEMY && u[i].type == U_ZERGLING) {
        int d = distance_squared(x, y, u[i].x, u[i].y);
        count++;
        if (d < d0) { d1 = d0; best1 = best0; d0 = d; best0 = i; }
        else if (d < d1) { d1 = d; best1 = i; }
    }
    if (count < 2 || best0 < 0 || best1 < 0)
        return 0;
    out[0] = best0;
    out[1] = best1;
    return 1;
}

static void
cluster_center(const int pair[2], int *x, int *y)
{
    *x = ((u[pair[0]].x + u[pair[1]].x + 2) / 4) * 2;
    *y = ((u[pair[0]].y + u[pair[1]].y + 2) / 4) * 2;
}

static int
both_dead(const int pair[2])
{
    return !u[pair[0]].alive && !u[pair[1]].alive;
}

static int
either_dead(const int pair[2])
{
    return !u[pair[0]].alive || !u[pair[1]].alive;
}

static int
closest_cluster_to_friendly_workers(int out[2])
{
    int i, j, n = 0, x = 0, y = 0, best_distance = 0x7fffffff;
    int best0 = -1, best1 = -1;
    for (i = 0; i < MAX_UNITS; i++)
        if (u[i].alive && u[i].owner == SIDE && u[i].type == U_SCV) {
            x += u[i].x;
            y += u[i].y;
            n++;
        }
    if (!n)
        return 0;
    x /= n;
    y /= n;
    for (i = 0; i < MAX_UNITS; i++)
        if (u[i].alive && u[i].owner == ENEMY && u[i].type == U_ZERGLING)
            for (j = i + 1; j < MAX_UNITS; j++)
                if (u[j].alive && u[j].owner == ENEMY && u[j].type == U_ZERGLING &&
                    distance_squared(u[i].x, u[i].y, u[j].x, u[j].y) <= 32 * 32) {
                    int gx = (u[i].x + u[j].x) / 2;
                    int gy = (u[i].y + u[j].y) / 2;
                    int d = distance_squared(x, y, gx, gy);
                    if (d < best_distance) {
                        best_distance = d;
                        best0 = i;
                        best1 = j;
                    }
                }
    if (best0 < 0)
        return 0;
    out[0] = best0;
    out[1] = best1;
    return 1;
}

static int
wait_until_either_dead(const int pair[2], int max_updates)
{
    int i;
    for (i = 0; i < max_updates && !either_dead(pair) && screen != SCREEN_RESULT; i++)
        input_frame(movement_base);
    return either_dead(pair);
}

static int
nearest_patch(int x, int y)
{
    int i, best = -1, best_distance = 0x7fffffff;
    for (i = 0; i < MAX_MINERALS; i++) if (m[i].amount > 0) {
        int d = distance_squared(x, y, m[i].x, m[i].y);
        if (d < best_distance) { best_distance = d; best = i; }
    }
    return best;
}

static int
command_center_slot(void)
{
    int i;
    for (i = 0; i < MAX_BUILDINGS; i++)
        if (b[i].alive && b[i].owner == SIDE && b[i].type == B_COMMAND) return i;
    return -1;
}

static int
raynor_slot(void)
{
    int i;
    for (i = 0; i < MAX_UNITS; i++)
        if (u[i].type == U_RAYNOR)
            return i;
    return -1;
}

static int
barracks_slot(void)
{
    int i;
    for (i = 0; i < MAX_BUILDINGS; i++)
        if (b[i].alive && b[i].owner == SIDE && b[i].type == B_BARRACKS)
            return i;
    return -1;
}

static int
alive_count(int owner, int type)
{
    int i, count = 0;
    for (i = 0; i < MAX_UNITS; i++)
        if (u[i].alive && u[i].owner == owner && u[i].type == type)
            count++;
    return count;
}

static int
wait_for_clear(const int pair[2], int max_updates)
{
    int i;
    for (i = 0; i < max_updates && !both_dead(pair) && screen != SCREEN_RESULT; i++)
        input_frame(movement_base);
    return both_dead(pair);
}

static int
open_attack_command(void)
{
    int guard = 0;
    press_key(KEY_B);
    while (menu_cursor != MENU_ATTACK && guard++ < MENU_COUNT) {
        if (menu_cursor < MENU_ATTACK)
            press_key(KEY_DOWN);
        else
            press_key(KEY_UP);
    }
    if (menu_cursor != MENU_ATTACK)
        return 0;
    press_key(KEY_A);
    return mode == MODE_ATTACK;
}

static int
move_combat_group_to(int x, int y)
{
    if (!open_attack_command())
        return 0;
    move_cursor_near(x, y);
    press_key(KEY_A);
    return mode == MODE_NORMAL;
}

static int
wait_for_minerals(int amount, int max_updates)
{
    int i;
    for (i = 0; i < max_updates && minerals < amount && screen != SCREEN_RESULT; i++)
        input_frame(movement_base);
    return minerals >= amount;
}

static int
select_nearest_worker(void)
{
    int attempt, i, worker;
    for (attempt = 0; attempt < 5; attempt++) {
        int best = 0x7fffffff;
        worker = -1;
        for (i = 0; i < MAX_UNITS; i++)
            if (u[i].alive && u[i].owner == SIDE && u[i].type == U_SCV) {
                int d = distance_squared(u[i].x, u[i].y, cx, cy);
                if (d < best) { best = d; worker = i; }
            }
        if (worker < 0)
            return 0;
        move_cursor_near(u[worker].x, u[worker].y);
        if (!move_cursor_exact(u[worker].x, u[worker].y))
            continue;
        press_key(KEY_A);
        for (i = 0; i < MAX_UNITS; i++)
            if (u[i].alive && u[i].owner == SIDE && u[i].type == U_SCV && u[i].selected)
                return 1;
    }
    return 0;
}

static int
start_barracks(void)
{
    int i, home = command_center_slot(), site_x, site_y, worker;
    if (home < 0 || minerals < 150) {
        fprintf(stderr, "Barracks precheck failed home=%d minerals=%d\n", home, minerals);
        return 0;
    }
    /* Choose a clear authored placement north-west of the imported CC using
     * structure-relative offsets, not an extracted asset coordinate. */
    site_x = b[home].x - 48;
    site_y = b[home].y - 56;
    if (!select_nearest_worker()) {
        fprintf(stderr, "Barracks worker selection failed at %d,%d\n", cx, cy);
        return 0;
    }
    worker = -1;
    for (i = 0; i < MAX_UNITS; i++)
        if (u[i].alive && u[i].owner == SIDE && u[i].type == U_SCV && u[i].selected)
            worker = i;
    if (worker < 0) {
        fprintf(stderr, "Barracks selected worker disappeared\n");
        return 0;
    }
    press_key(KEY_B);
    while (menu_cursor != MENU_BARRACKS) {
        if (menu_cursor < MENU_BARRACKS) press_key(KEY_DOWN);
        else press_key(KEY_UP);
    }
    press_key(KEY_A);
    if (mode != MODE_BUILD || !move_cursor_exact(site_x, site_y)) {
        fprintf(stderr, "Barracks placement mode/cursor failed mode=%d cursor=%d,%d target=%d,%d menu=%d\n",
                mode,cx,cy,site_x,site_y,menu_open);
        return 0;
    }
    press_key(KEY_A);
    if (barracks_slot() < 0 || u[worker].order != ORDER_BUILD) {
        fprintf(stderr, "Barracks issue failed slot=%d worker=%d order=%d target=%d,%d minerals=%d\n",
                barracks_slot(),worker,u[worker].order,site_x,site_y,minerals);
        return 0;
    }
    /* B exits build mode via its menu; a second B closes that menu. */
    press_key(KEY_B);
    press_key(KEY_B);
    if (mode != MODE_NORMAL || menu_open)
        fprintf(stderr, "Barracks menu close failed mode=%d menu=%d\n",mode,menu_open);
    return mode == MODE_NORMAL && !menu_open;
}

static int
queue_five_marines(int max_updates)
{
    int i, slot = barracks_slot();
    if (slot < 0)
        return 0;
    for (i = 0; i < max_updates && (b[slot].under_construction || minerals < 250) && screen != SCREEN_RESULT; i++)
        input_frame(movement_base);
    if (b[slot].under_construction || minerals < 250)
        return 0;
    /* Cursor is still at the new Barracks. First A selects it; later presses
     * use the selected building's ordinary queue action. */
    move_cursor_exact(b[slot].x, b[slot].y);
    press_key(KEY_A);
    if (selected_building != slot)
        return 0;
    for (i = 0; i < 5; i++)
        press_key(KEY_A);
    return b[slot].queue_count == 5;
}

int
main(int argc, char **argv)
{
    int pair[2], target_x, target_y, i, home, patch;
    int initial_zerg, initial_scv, initial_marine, initial_raynor;
    int result = 0;
    const char *trace_path = NULL;
    FILE *trace = NULL;
    if (argc == 3 && argv[1][0] == '-' && argv[1][1] == '-' && argv[1][2] == 't' &&
        argv[1][3] == 'r' && argv[1][4] == 'a' && argv[1][5] == 'c' && argv[1][6] == 'e' && argv[1][7] == '\0')
        trace_path = argv[2];
    else if (argc != 1) {
        fprintf(stderr, "usage: %s [--trace PATH]\n", argv[0]);
        return 64;
    }
    game_init();
#ifdef SC64_UNIFIED
    /* In the full Terran profile, enter through the real campaign and mission
     * menus rather than treating HOME confirmation as an immediate deploy. */
    input_frame(KEY_A); input_frame(0);
#ifndef SC64_CAMPAIGN_RACE
    if (race_front.page != SC_FRONT_CAMPAIGN) return 66;
    input_frame(KEY_A); input_frame(0); /* Terran campaign -> mission list. */
#endif
    if (race_front.page != SC_FRONT_MISSION) return 66;
    input_frame(KEY_A); input_frame(0); /* T1 briefing. */
    if (race_front.page != SC_FRONT_BRIEFING || race_front.mission != 4) return 66;
    input_frame(KEY_START); input_frame(0); /* Explicitly deploy. */
    if (screen != SCREEN_PLAY || race_selected_mission != 4) return 66;
#else
    input_frame(KEY_A);
    input_frame(0);
#endif
    initial_zerg = alive_count(ENEMY, U_ZERGLING);
    initial_scv = alive_count(SIDE, U_SCV);
    initial_marine = alive_count(SIDE, U_MARINE);
    initial_raynor = raynor_slot();
    if (initial_zerg != 23 || initial_scv != 5 || initial_marine != 5 || initial_raynor < 0)
        return 1;

    /* R selects the in-game combat group. The command wheel then issues an
     * attack-move toward the hostile pair closest to the nearest mineral patch. */
    /* A clean R tap selects the group; R then stays held as fast cursor. */
    input_frame(KEY_R);
    input_frame(0);
    movement_base = KEY_R;
    input_frame(movement_base);
    patch = nearest_patch(cx, cy);
    if (patch < 0 || !closest_enemy_indices(m[patch].x, m[patch].y, pair))
        return 2;
    cluster_center(pair, &target_x, &target_y);
    if (!move_combat_group_to(target_x, target_y))
        return 3;
    if (!wait_until_either_dead(pair, 9000))
        return 4;

    /* Once a worker is returning a mineral load, clear the closest hostile
     * pair along that actual return route, then let the auto-miners work. */
    if (!closest_cluster_to_friendly_workers(pair))
        return 5;
    cluster_center(pair, &target_x, &target_y);
    if (!move_combat_group_to(target_x, target_y))
        return 6;
    if (!wait_for_clear(pair, 9000))
        return 7;
    if (!wait_for_minerals(400, 30000))
        return 8;
    if (!save_load_round_trip())
        return 12;

    home = command_center_slot();
    if (home < 0 || !start_barracks()) {
        fprintf(stderr, "Wasteland Barracks command failed: home=%d frame=%d minerals=%d SCVs=%d Marines=%d Raynor=%d/%d mode=%d menu=%d selected=%d cursor=%d,%d\n",
                home, frame, minerals, alive_count(SIDE,U_SCV),
                alive_count(SIDE,U_MARINE), initial_raynor,
                initial_raynor >= 0 ? u[initial_raynor].alive : 0,
                mode, menu_open, selected_building, cx, cy);
        return 9;
    }
    if (!queue_five_marines(5000))
        return 10;
    for (i = 0; i < 6000 && screen != SCREEN_RESULT; i++)
        input_frame(movement_base);

    if (!won || lost || !u[initial_raynor].alive || u[initial_raynor].owner != SIDE ||
        alive_count(SIDE, U_MARINE) < 10 || barracks_slot() < 0 || alive_count(SIDE, U_SCV) < 1)
        result = 11;
    if (trace_path) {
        trace = fopen(trace_path, "w");
        if (!trace) {
            perror(trace_path);
            return 65;
        }
        fprintf(trace, "{\"total_ticks\":%lu,\"events\":[", input_tick);
        for (i = 0; i < event_count; i++)
            fprintf(trace, "%s{\"tick\":%lu,\"held\":%u}", i ? "," : "", events[i].tick, events[i].held);
        fprintf(trace, "],\"checkpoints\":[");
        for (i = 0; i < checkpoint_count; i++) {
            Checkpoint *c = &checkpoints[i];
            fprintf(trace, "%s{\"tick\":%lu,\"frame\":%d,\"minerals\":%d,\"marines\":%d,\"scv\":%d,\"zerg\":%d,\"raynor_owner\":%d,\"raynor_alive\":%d}",
                i ? "," : "", c->tick, c->frame, c->minerals, c->marines, c->scv, c->zerg, c->raynor_owner, c->raynor_alive);
        }
        fprintf(trace, "],\"expected\":{\"won\":%d,\"marines\":%d,\"scv\":%d,\"minerals\":%d,\"frame\":%d}}\n",
            won ? 1 : 0, alive_count(SIDE, U_MARINE), alive_count(SIDE, U_SCV), minerals, frame);
        if (fclose(trace) != 0) {
            perror(trace_path);
            return 65;
        }
    }
    if (result == 0)
        printf("PASS Wasteland: frame=%d ticks=%lu Marines=%d SCVs=%d minerals=%d Zerglings=%d Raynor=rescued\n",
            frame, input_tick, alive_count(SIDE, U_MARINE), alive_count(SIDE, U_SCV), minerals,
            alive_count(ENEMY, U_ZERGLING));
    else
        fprintf(stderr, "FAIL Wasteland: code=%d frame=%d won=%d lost=%d\n", result, frame, won, lost);
    return result;
}
