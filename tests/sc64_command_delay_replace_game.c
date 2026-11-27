/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Command latency checks in the Terran product, on missions launched from
 * the ordinary menu with every update through game_update().
 *
 * A player command that replaces a pending (delayed) Move keeps its order
 * when the Move falls due.
 * The Dropship takes unit slot 22: with the earlier 8-bit order hash, an
 * idle Marine's boarding order for that slot hashed like its idle order,
 * so the pending Move landed and cancelled the boarding. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../starcraft/game.c"

#if !defined(SC64_TRANSPORT_RUNTIME) || !defined(SC64_IMPORTED_MISSION)
#error "Compile this fixture with the Terran product flags"
#endif

volatile u16 demo_keys;
static unsigned held_keys;

static void input(unsigned held)
{
    game_update((u16)held, (u16)(held & ~held_keys));
    held_keys = held;
}

static void press(unsigned key) { input(key); input(0); }

static void launch_t1(void)
{
    game_init();
    held_keys = 0;
    press(KEY_A);
    press(KEY_A);
    press(KEY_START);
    assert(screen == SCREEN_PLAY && race_selected_mission == SC64_RACE_MISSION_T1);
    for (int k = 0; k < 30; ++k) input(0);
}

static int player_marine(int skip)
{
    for (int i = 0; i < MAX_UNITS; ++i)
        if (u[i].alive && u[i].owner == SIDE && u[i].raw_type == 0 &&
            u[i].order == ORDER_IDLE && skip-- == 0) return i;
    assert(!"no idle player Marine");
    return -1;
}

static void select_only(int a, int b)
{
    for (int i = 0; i < MAX_UNITS; ++i) u[i].selected = i == a || i == b;
    selected_building = -1;
}

/* The player's Move through the field menu's Move mode, as a press of A. */
static void player_move(int x, int y)
{
    menu_open = 0;
    mode = MODE_MOVE;
    cx = x; cy = y;
    press(KEY_A);
    assert(mode == MODE_NORMAL);
}

/* The formation's direct building and door test matches a freshly built
 * walk grid on every cell. */
static void check_footprints(const char *where)
{
    int cells = 0, built = 0;
    build_path_grid();
    assert(path_grid_valid);
    for (int row = 0; row < MAP_ROWS; ++row)
        for (int col = 0; col < MAP_COLS; ++col) {
            int open = !terrain_blocked(col, row), covered = sc64_formation_cell_built(col, row);
            if (SC64_GRID_GET(path_walkable, col + row * MAP_COLS) != (open && !covered)) {
                fprintf(stderr, "%s: cell %d,%d grid %d terrain open %d covered %d\n", where, col, row,
                        (int)SC64_GRID_GET(path_walkable, col + row * MAP_COLS), open, covered);
                abort();
            }
            ++cells; built += open && covered;
        }
    printf("%s: %d cells match the walk grid, %d open cells under buildings or doors\n",
           where, cells, built);
    assert(built > 0);
}

/* With the walk grid stale, a group Move's translated slot under a
 * building falls back to an open slot connected to the ordered point, a
 * slot on open ground beside it is kept, and nothing rebuilds the grid. */
static void check_stale_grid_formation(int building)
{
    static const int near[4][2] = {{48, 0}, {-48, 0}, {0, 48}, {0, -48}};
    int a = player_marine(0), c = player_marine(1), ox = -1, oy = -1;
    select_only(a, c);
    if (path_grid_valid) invalidate_path_grid();
    assert(!path_grid_valid);
    for (int k = 0; k < 4 && ox < 0; ++k) {
        int x = b[building].x + near[k][0], y = b[building].y + near[k][1];
        if (!terrain_blocked(x / 16, y / 16) && !sc64_formation_cell_built(x / 16, y / 16)) { ox = x; oy = y; }
    }
    assert(ox >= 0);
    Sc64GroupFormation f = sc64_group_formation_begin(ORDER_MOVE, 1, ox, oy);
    assert(f.keep && f.ground && !path_grid_valid);
    int sx, sy;
    /* Translate so the first Marine's slot is the building's centre. */
    f.dx = b[building].x - u[a].x; f.dy = b[building].y - u[a].y;
    sc64_group_formation_slot(&f, &u[a], 0, ox, oy, 1, &sx, &sy);
    assert(sx != b[building].x || sy != b[building].y);
    assert(!sc64_formation_cell_built(sx / 16, sy / 16) && sc64_group_formation_reaches(&f, sx, sy));
    /* A slot on the open ordered point itself is kept. */
    f.dx = ox - u[a].x; f.dy = oy - u[a].y;
    sc64_group_formation_slot(&f, &u[a], 0, ox, oy, 1, &sx, &sy);
    assert(sx == ox && sy == oy && !path_grid_valid);
    /* Ordered onto the building itself, ground units gather on the grid. */
    f = sc64_group_formation_begin(ORDER_MOVE, 1, b[building].x, b[building].y);
    assert(f.keep && !f.ground && !path_grid_valid);
    puts("command delay: stale-grid formation slot under a building takes its grid slot: ok");
}

/* Four-neighbour components of the current walk grid, for the cliff test. */
static short cliff_component[256 * 256]; /* MAP_COLS is a variable in large-map builds */
static void label_components(void)
{
    static int queue[256 * 256];
    int label = 0;
    assert(MAP_COLS * MAP_ROWS <= 256 * 256);
    build_path_grid();
    for (int i = 0; i < MAP_COLS * MAP_ROWS; ++i) cliff_component[i] = -1;
    for (int s0 = 0; s0 < MAP_COLS * MAP_ROWS; ++s0) {
        if (cliff_component[s0] >= 0 || !SC64_GRID_GET(path_walkable, s0)) continue;
        int head = 0, tail = 0;
        queue[tail++] = s0; cliff_component[s0] = (short)label;
        while (head < tail) {
            int c = queue[head++], x = c % MAP_COLS, y = c / MAP_COLS;
            static const int dx[4] = {1, -1, 0, 0}, dy[4] = {0, 0, 1, -1};
            for (int d = 0; d < 4; ++d) {
                int nx = x + dx[d], ny = y + dy[d], n;
                if (nx < 0 || ny < 0 || nx >= MAP_COLS || ny >= MAP_ROWS) continue;
                n = nx + ny * MAP_COLS;
                if (cliff_component[n] >= 0 || !SC64_GRID_GET(path_walkable, n)) continue;
                cliff_component[n] = (short)label; queue[tail++] = n;
            }
        }
        ++label;
    }
}
static int component_at(int x, int y) { return cliff_component[x / 16 + (y / 16) * MAP_COLS]; }

/* A group beside a cliff: the ordered point is reachable, but one Marine's
 * translated slot is open ground the point does not connect to. Every unit
 * gets a destination in its own component and arrives there. */
static void check_cliff_formation(void)
{
    label_components();
    for (int c = 0; c < MAP_COLS * MAP_ROWS; ++c) {
        if (cliff_component[c] < 0) continue;
        int cxp = (c % MAP_COLS) * 16 + 8, cyp = (c / MAP_COLS) * 16 + 8, k = cliff_component[c];
        for (int vy = -3; vy <= 3; ++vy) for (int vx = -3; vx <= 3; ++vx) {
            if (FAST_ABS(vx) + FAST_ABS(vy) < 2) continue;
            int sx = cxp + vx * 16, sy = cyp + vy * 16;
            if (sx < 8 || sy < 8 || sx >= WORLD_W - 8 || sy >= WORLD_H - 8) continue;
            int ks = component_at(sx, sy);
            if (ks < 0 || ks == k) continue;
            /* Marine A stands where its slot is the far point; B mirrors it
             * (slot_A = point + v, slot_B = point - v, centre = A - v). Both
             * stand in the point's component, away from the point. */
            for (int off = 3; off <= 12; ++off) for (int sgn = -1; sgn <= 1; sgn += 2) {
                int ax = cxp + sgn * off * 16, ay = cyp + vy * 16 + 4 * 16;
                int bx = ax - 2 * vx * 16, by = ay - 2 * vy * 16;
                if (ax < 8 || ay < 8 || bx < 8 || by < 8 || ax >= WORLD_W - 8 || bx >= WORLD_W - 8 ||
                    ay >= WORLD_H - 8 || by >= WORLD_H - 8) continue;
                if (component_at(ax, ay) != k || component_at(bx, by) != k) continue;
                int left = ax < bx ? ax : bx, right = ax < bx ? bx : ax;
                int top = ay < by ? ay : by, bottom = ay < by ? by : ay;
                if (cxp >= left && cxp <= right && cyp >= top && cyp <= bottom) continue;
                if (right - left > SC64_GROUP_FORMATION_MAX_SPAN || bottom - top > SC64_GROUP_FORMATION_MAX_SPAN) continue;
                int a = spawn_unit(SIDE, sc64_race_unit_from_raw(0), ax, ay);
                int b2 = spawn_unit(SIDE, sc64_race_unit_from_raw(0), bx, by);
                assert(a >= 0 && b2 >= 0);
                select_only(a, b2);
                /* The translation the formation would apply puts A on the far side. */
                Sc64GroupFormation f = sc64_group_formation_begin(ORDER_MOVE, 1, cxp, cyp);
                assert(f.keep && component_at(u[a].x + f.dx, u[a].y + f.dy) == ks);
                assert(issue_order(ORDER_MOVE, cxp, cyp) == 2);
                printf("cliff formation: point %d,%d, slot %d,%d across terrain; "
                       "A %d,%d -> %d,%d, B %d,%d -> %d,%d\n", cxp, cyp, u[a].x + f.dx, u[a].y + f.dy,
                       u[a].x, u[a].y, u[a].tx, u[a].ty, u[b2].x, u[b2].y, u[b2].tx, u[b2].ty);
                assert(component_at(u[a].tx, u[a].ty) == k && component_at(u[b2].tx, u[b2].ty) == k);
                for (int t = 0; t < 600 && (dist2(u[a].x, u[a].y, u[a].tx, u[a].ty) > 6 * 6 ||
                                            dist2(u[b2].x, u[b2].y, u[b2].tx, u[b2].ty) > 6 * 6); ++t)
                    input(0);
                assert(dist2(u[a].x, u[a].y, u[a].tx, u[a].ty) <= 6 * 6);
                assert(dist2(u[b2].x, u[b2].y, u[b2].tx, u[b2].ty) <= 6 * 6);
                puts("command delay: a group beside a cliff keeps every destination reachable: ok");
                u[a].selected = u[b2].selected = 0;
                return;
            }
        }
    }
    assert(!"no cliff geometry found in T1");
}

/* Ordered onto a blocked tile: a Dropship keeps its translated slot and a
 * Marine whose slot is blocked reaches an open tile; a pair whose slots
 * are open keeps them and arrives. */
static void check_flyer_and_edge_formation(void)
{
    label_components();
    int blocked = -1;
    for (int c = 0; c < MAP_COLS * MAP_ROWS && blocked < 0; ++c) {
        int x = c % MAP_COLS, y = c / MAP_COLS;
        if (x > 8 && y > 8 && x < MAP_COLS - 8 && y < MAP_ROWS - 8 && terrain_blocked(x, y)) {
            int ok = 1;
            for (int dx = -3; dx <= -1 && ok; ++dx) ok = cliff_component[c + dx] >= 0;
            if (ok && cliff_component[c - 3] == cliff_component[c - 1]) blocked = c;
        }
    }
    assert(blocked >= 0);
    int bxp = (blocked % MAP_COLS) * 16 + 8, byp = (blocked / MAP_COLS) * 16 + 8;
    int ship = spawn_unit(SIDE, sc64_race_unit_from_raw(11), bxp - 48, byp);
    int walker = spawn_unit(SIDE, sc64_race_unit_from_raw(0), bxp - 40, byp);
    assert(ship >= 0 && walker >= 0);
    select_only(ship, walker);
    Sc64GroupFormation f = sc64_group_formation_begin(ORDER_MOVE, 1, bxp, byp);
    assert(f.keep && !f.ground);
    int sx, sy;
    sc64_group_formation_slot(&f, &u[ship], 0, bxp, byp, 1, &sx, &sy);
    assert(sx == u[ship].x + f.dx && sy == u[ship].y + f.dy);
    /* The Marine's own slot is on the blocked tile: it takes an open tile
     * near its grid slot, never a point inside the wall. */
    sc64_group_formation_slot(&f, &u[walker], 1, bxp, byp, 1, &sx, &sy);
    assert(!terrain_blocked(sx / 16, sy / 16) && SC64_GRID_GET(path_walkable, sx / 16 + (sy / 16) * MAP_COLS));
    {
        /* Exactly the open tile centre nearest the grid slot in pixels,
         * the first in scan order on a tie. */
        int gx = limit(bxp - 8, 8, WORLD_W - 8), gy = limit(byp - 8, 8, WORLD_H - 8);
        int best = -1, ex = gx, ey = gy;
        for (int dy = -2; dy <= 2; ++dy) for (int dx = -2; dx <= 2; ++dx) {
            int c = gx / 16 + dx, r = gy / 16 + dy, d;
            if ((!dx && !dy) || c < 0 || r < 0 || c >= MAP_COLS || r >= MAP_ROWS ||
                !SC64_GRID_GET(path_walkable, c + r * MAP_COLS)) continue;
            d = dist2(gx, gy, c * 16 + 8, r * 16 + 8);
            if (best < 0 || d < best) { best = d; ex = c * 16 + 8; ey = r * 16 + 8; }
        }
        assert(!SC64_GRID_GET(path_walkable, gx / 16 + (gy / 16) * MAP_COLS) && best >= 0);
        assert(sx == ex && sy == ey);
    }
    assert(issue_order(ORDER_MOVE, bxp, byp) == 2);
    assert(u[walker].tx == sx && u[walker].ty == sy);
    for (int t = 0; t < 400 && u[walker].order == ORDER_MOVE; ++t) input(0);
    assert(u[walker].order == ORDER_IDLE && dist2(u[walker].x, u[walker].y, sx, sy) <= 5 * 5);
    u[ship].selected = u[walker].selected = 0;
    puts("command delay: over blocked ground a flyer keeps its slot and a Marine reaches an open one: ok");

    /* A pair ordered onto a wall tile whose translated slots, just above
     * and below it, are open: both slots are kept and both Moves end. */
    int wall = -1;
    for (int c = 0; c < MAP_COLS * MAP_ROWS && wall < 0; ++c) {
        int x = c % MAP_COLS, yy = c / MAP_COLS;
        if (x < 6 || yy < 3 || x >= MAP_COLS - 3 || yy >= MAP_ROWS - 3 || !terrain_blocked(x, yy)) continue;
        int k = cliff_component[c - 3];
        if (k < 0 || cliff_component[c - MAP_COLS] != k || cliff_component[c + MAP_COLS] != k ||
            cliff_component[c - 3 - MAP_COLS] != k || cliff_component[c - 3 + MAP_COLS] != k) continue;
        wall = c;
    }
    assert(wall >= 0);
    int wx = (wall % MAP_COLS) * 16 + 8, wy = (wall / MAP_COLS) * 16 + 8;
    int up = spawn_unit(SIDE, sc64_race_unit_from_raw(0), wx - 48, wy - 16);
    int down = spawn_unit(SIDE, sc64_race_unit_from_raw(0), wx - 48, wy + 16);
    assert(up >= 0 && down >= 0);
    select_only(up, down);
    assert(issue_order(ORDER_MOVE, wx, wy) == 2);
    assert(u[up].tx == wx && u[up].ty == wy - 16 && u[down].tx == wx && u[down].ty == wy + 16);
    for (int t = 0; t < 400 && (u[up].order == ORDER_MOVE || u[down].order == ORDER_MOVE); ++t) input(0);
    assert(u[up].order == ORDER_IDLE && u[down].order == ORDER_IDLE);
    u[up].selected = u[down].selected = 0;
    puts("command delay: a pair ordered onto a wall keeps its open slots and arrives: ok");

}

/* A wide group ordered to open ground at the map's left edge keeps its
 * formation after the clamp (every slot inside the flood window). */
static void check_edge_formation(void)
{
    int open_row = -1, sx, sy;
    Sc64GroupFormation f;
    label_components();
    /* A row whose first nine tiles and a 128 px span further in are one
     * open component. */
    for (int y = 2; y < MAP_ROWS - 2 && open_row < 0; ++y) {
        int k = cliff_component[y * MAP_COLS], ok = k >= 0;
        for (int x = 0; x < 20 && ok; ++x) ok = cliff_component[x + y * MAP_COLS] == k;
        if (ok) open_row = y;
    }
    assert(open_row >= 0);
    int y = open_row * 16 + 8;
    int left = spawn_unit(SIDE, sc64_race_unit_from_raw(0), 8 * 16 + 8, y);
    int right = spawn_unit(SIDE, sc64_race_unit_from_raw(0), 8 * 16 + 8 + 120, y);
    assert(left >= 0 && right >= 0);
    select_only(left, right);
    f = sc64_group_formation_begin(ORDER_MOVE, 1, 4, y);
    assert(f.keep && f.ground);
    sc64_group_formation_slot(&f, &u[left], 0, 4, y, 1, &sx, &sy);
    assert(sx == u[left].x + f.dx && sx == 8);
    sc64_group_formation_slot(&f, &u[right], 1, 4, y, 1, &sx, &sy);
    assert(sx == u[right].x + f.dx && sx == 128);
    u[left].selected = u[right].selected = 0;
    puts("command delay: a wide group keeps its formation at the map edge: ok");
}

static void launch_t4(void)
{
    game_init();
    held_keys = 0;
    press(KEY_A);
    press(KEY_RIGHT);
    press(KEY_DOWN);
    press(KEY_A);
    press(KEY_START);
    assert(screen == SCREEN_PLAY && race_t4_active());
    for (int k = 0; k < 30; ++k) input(0);
}

int main(void)
{
    launch_t4();
    int doors = 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i)
        doors += b[i].alive && sc64_race_t4_is_door(b[i].type) && sc64_t4_building_collidable(i);
    assert(doors > 0);
    check_footprints("T4 closed doors");

    launch_t1();
    int command_center = spawn_build(SIDE, B_COMMAND, u[2].x + 120, u[2].y + 60, 0);
    assert(command_center >= 0);
    assert(spawn_build(SIDE, B_DEPOT, u[2].x + 40, u[2].y + 120, 0) >= 0);
    check_footprints("T1 with buildings");
    /* A building placed after the grid was built: the stale grid lacks it,
     * so the formation must see it from b[]. */
    int depot = spawn_build(SIDE, B_DEPOT, u[2].x + 150, u[2].y - 40, 0);
    assert(depot >= 0 && !path_grid_valid);
    /* Open in the old grid, covered only by the new building. */
    assert(SC64_GRID_GET(path_walkable, b[depot].x / 16 + (b[depot].y / 16) * MAP_COLS) &&
           !terrain_blocked(b[depot].x / 16, b[depot].y / 16) &&
           sc64_formation_cell_built(b[depot].x / 16, b[depot].y / 16));
    check_stale_grid_formation(depot);
    int marine = player_marine(0), witness = player_marine(1);

    /* Put a player Dropship in slot 22 (an idle Zergling far from the
     * player stands there in T1; spawn_unit reuses a dead slot). */
    assert(u[22].alive && u[22].owner != SIDE && !u[22].selected);
    u[22].alive = 0;
    int dropship = spawn_unit(SIDE, sc64_race_unit_from_raw(11), u[marine].x + 24, u[marine].y);
    assert(dropship == 22 && race_transport_carrier_valid(dropship));
    for (int k = 0; k < 4; ++k) input(0);

    /* Control: a pending Move with no newer command lands after the delay. */
    select_only(witness, -1);
    player_move(u[witness].x + 40, u[witness].y + 30);
    assert(u[witness].order == ORDER_IDLE);
    for (int k = 0; k < SC64_COMMAND_DELAY_UPDATES + 1 && u[witness].order == ORDER_IDLE; ++k) input(0);
    assert(u[witness].order == ORDER_MOVE);

    /* The Marine's Move is pending when the player boards it on the
     * Dropship: press A on the carrier in the field. */
    int move_x = u[marine].x + 60, move_y = u[marine].y + 50;
    select_only(marine, -1);
    player_move(move_x, move_y);
    assert(u[marine].order == ORDER_IDLE);
    assert(sc64_delayed_commands[0].active || sc64_delayed_commands[1].active);
    mode = MODE_NORMAL; menu_open = 0;
    cx = u[dropship].x; cy = u[dropship].y;
    press(KEY_A);
    assert(u[marine].order == ORDER_BOARD && u[marine].target == dropship);

    /* The Move was given two updates ago and falls due within the next
     * few. Run well past that update: the boarding stands. */
    for (int k = 0; k < 3 * SC64_COMMAND_DELAY_UPDATES; ++k) {
        input(0);
        assert(u[marine].order == ORDER_BOARD || u[marine].order == ORDER_TRANSPORTED);
        assert(u[marine].target == dropship);
        assert(u[marine].tx != move_x || u[marine].ty != move_y);
    }
    assert(!sc64_delayed_commands[0].active && !sc64_delayed_commands[1].active);
    puts("command delay: boarding replaces a pending Move: ok");

    /* A Move given while a Marine walks into a Bunker is still pending when
     * it boards. Unloading it before the Move falls due is the newer
     * command: the Marine stays at its landing point. */
    int bunker_marine = player_marine(1);
    int bx = u[bunker_marine].x + 36, by = u[bunker_marine].y;
    int bunker = spawn_build(SIDE, sc64_race_building_from_raw(125), bx, by, 0);
    assert(bunker >= 0 && race_bunker_carrier_valid(bunker));
    select_only(bunker_marine, -1);
    assert(race_bunker_request_board(bunker_marine, bunker));
    move_x = u[bunker_marine].x - 60; move_y = u[bunker_marine].y + 40;
    player_move(move_x, move_y);
    assert(u[bunker_marine].order == ORDER_BOARD);
    for (int k = 0; k < SC64_COMMAND_DELAY_UPDATES && u[bunker_marine].order != ORDER_TRANSPORTED; ++k) input(0);
    assert(u[bunker_marine].order == ORDER_TRANSPORTED);
    assert(sc64_delayed_commands[0].active || sc64_delayed_commands[1].active);
    assert(race_bunker_request_unload(bunker) == 1);
    for (int k = 0; k < 3 * SC64_COMMAND_DELAY_UPDATES; ++k) {
        input(0);
        assert(u[bunker_marine].order == ORDER_IDLE);
    }
    assert(!sc64_delayed_commands[0].active && !sc64_delayed_commands[1].active);
    puts("command delay: unloading a Bunker replaces a pending Move: ok");

    /* The same through a Dropship: a Move given while the Marine walks to
     * the carrier is pending when it boards; unloading is newer. */
    int drop_marine = player_marine(2);
    int ship = spawn_unit(SIDE, sc64_race_unit_from_raw(11), u[drop_marine].x + 12, u[drop_marine].y);
    assert(ship >= 0 && race_transport_carrier_valid(ship));
    select_only(drop_marine, -1);
    assert(race_transport_request_board(drop_marine, ship));
    move_x = u[drop_marine].x - 50; move_y = u[drop_marine].y + 50;
    player_move(move_x, move_y);
    for (int k = 0; k < SC64_COMMAND_DELAY_UPDATES && u[drop_marine].order != ORDER_TRANSPORTED; ++k) input(0);
    assert(u[drop_marine].order == ORDER_TRANSPORTED);
    assert(sc64_delayed_commands[0].active || sc64_delayed_commands[1].active);
    assert(race_transport_request_unload(ship, u[ship].x, u[ship].y));
    for (int k = 0; k < 3 * SC64_COMMAND_DELAY_UPDATES; ++k) {
        input(0);
        assert(u[drop_marine].order == ORDER_TRANSPORTED || u[drop_marine].order == ORDER_IDLE);
    }
    assert(u[drop_marine].order == ORDER_IDLE);
    assert(!sc64_delayed_commands[0].active && !sc64_delayed_commands[1].active);
    puts("command delay: unloading a Dropship replaces a pending Move: ok");

    /* Three Moves in quick succession: the third applies the oldest at once
     * (during input; the host audit in this build must not report it), and
     * the unit ends up following the last. */
    int runner = player_marine(0);
    select_only(runner, -1);
    player_move(u[runner].x + 20, u[runner].y + 10);
    player_move(u[runner].x + 30, u[runner].y + 20);
    int last_x = u[runner].x + 40, last_y = u[runner].y + 30;
    assert(sc64_delayed_commands[0].active && sc64_delayed_commands[1].active);
    player_move(last_x, last_y);
    for (int k = 0; k < 2 * SC64_COMMAND_DELAY_UPDATES; ++k) input(0);
    assert(!sc64_delayed_commands[0].active && !sc64_delayed_commands[1].active);
    assert(u[runner].tx == last_x && u[runner].ty == last_y);
    puts("command delay: three quick Moves land in order: ok");

    check_cliff_formation();
    check_flyer_and_edge_formation();
    check_edge_formation();
    return 0;
}
