#ifndef SC64_ROUTES_H
#define SC64_ROUTES_H

/* Shared shortest-path fields on the same four-neighbour collision grid.
 * A mineral, base or group destination costs one reverse flood, rather than
 * one flood per moving unit per tile. Dynamic actors still use local steering.
 * Derived caches are discarded after load and whenever a structure changes. */
/* The race game needs fields for four economies and several armies. */
#ifdef SC64_RACES
#define SC64_ROUTE_SLOTS 16
#define SC64_ROUTE_BYTES ((MAP_CELLS + 1) / 2)
#elif defined(SC64_SKIRMISH)
#define SC64_ROUTE_SLOTS 16
#else
#define SC64_ROUTE_SLOTS 8
#endif
#ifdef SC64_RACES
/* Two cells per byte: 0 is the goal, 1..4 point toward a shortest-path
 * neighbour, and 15 means unreachable. 16 × 96 × 96 cells = 72 KiB in the production profile. */
static u8 sc64_route_field[SC64_ROUTE_SLOTS][SC64_ROUTE_BYTES];
#else
/* Legacy fields retain distances for the original blocked-start behavior. */
static u16 sc64_route_field[SC64_ROUTE_SLOTS][MAP_CELLS];
#endif
static u8 sc64_route_valid[SC64_ROUTE_SLOTS];
static u16 sc64_route_goal[SC64_ROUTE_SLOTS];
static u32 sc64_route_age[SC64_ROUTE_SLOTS], sc64_route_clock;
volatile u32 sc64_route_hits, sc64_route_builds;

#ifdef SC64_RACES
static int GBA_HOT sc64_route_get(int slot, int cell)
{
    u8 v = sc64_route_field[slot][cell >> 1];
    return (cell & 1) ? v >> 4 : v & 15;
}

static void GBA_HOT sc64_route_put(int slot, int cell, int direction)
{
    u8 *p = &sc64_route_field[slot][cell >> 1];
    if (cell & 1) *p = (u8)((*p & 15) | (direction << 4));
    else *p = (u8)((*p & 240) | direction);
}
#endif

static void
sc64_routes_reset(void)
{
    memset(sc64_route_valid, 0, sizeof(sc64_route_valid));
}

static int GBA_HOT
sc64_route_slot(int goal)
{
    int i, slot = -1, head = 0, tail = 0;
#ifndef SC64_RACES
    u16 *field;
#endif
    for (i = 0; i < SC64_ROUTE_SLOTS; i++) {
        if (sc64_route_valid[i] && sc64_route_goal[i] == goal) {
            sc64_route_age[i] = ++sc64_route_clock;
            sc64_route_hits++;
            return i;
        }
        if (slot < 0 || !sc64_route_valid[i] ||
            (sc64_route_valid[slot] && sc64_route_age[i] < sc64_route_age[slot])) slot = i;
    }
#ifdef SC64_RACES
    memset(sc64_route_field[slot], 255, SC64_ROUTE_BYTES);
#else
    field = sc64_route_field[slot];
    memset(field, 255, MAP_CELLS * sizeof(*field));
#endif
    if (path_walkable[goal]) {
#ifdef SC64_RACES
        sc64_route_put(slot, goal, 0);
#else
        field[goal] = 0;
#endif
        bfs_queue[tail++] = (u16)goal;
    } else {
#ifdef SC64_RACES
        int x = sc64_cell_col(goal), y = sc64_cell_row(goal), dx, dy;
#else
        int x = goal % MAP_COLS, y = goal / MAP_COLS, dx, dy;
#endif
        for (dy = -1; dy <= 1; dy++) for (dx = -1; dx <= 1; dx++) {
            int col = x + dx, row = y + dy, cell;
            if (col < 0 || row < 0 || col >= MAP_COLS || row >= MAP_ROWS) continue;
            cell = col + row * MAP_COLS;
            if (path_walkable[cell]) {
#ifdef SC64_RACES
                if (sc64_route_get(slot, cell) == 15) {
                    sc64_route_put(slot, cell, 0); bfs_queue[tail++] = (u16)cell;
                }
#else
                field[cell] = 0; bfs_queue[tail++] = (u16)cell;
#endif
            }
        }
    }
    while (head < tail) {
#ifdef SC64_RACES
        int cell = bfs_queue[head++], col = sc64_cell_col(cell), row = sc64_cell_row(cell);
#else
        int cell = bfs_queue[head++], col = cell % MAP_COLS, row = cell / MAP_COLS;
#endif
        int next;
        /* The stored direction takes the neighbour back toward this cell. */
#ifdef SC64_RACES
#define SC64_FLOOD(condition, neighbour, direction) \
        if (condition) { next = (neighbour); \
            if (path_walkable[next] && sc64_route_get(slot, next) == 15) { \
                sc64_route_put(slot, next, (direction)); bfs_queue[tail++] = (u16)next; } }
#else
#define SC64_FLOOD(condition, neighbour, direction) \
        if (condition) { next = (neighbour); \
            if (path_walkable[next] && field[next] == 0xFFFF) { \
                field[next] = (u16)(((field[cell] & ~7u) + 8u) | (direction)); \
                bfs_queue[tail++] = (u16)next; } }
#endif
        SC64_FLOOD(col > 0, cell - 1, 1);
        SC64_FLOOD(col + 1 < MAP_COLS, cell + 1, 2);
        SC64_FLOOD(row > 0, cell - MAP_COLS, 3);
        SC64_FLOOD(row + 1 < MAP_ROWS, cell + MAP_COLS, 4);
#undef SC64_FLOOD
    }
    sc64_route_goal[slot] = (u16)goal;
    sc64_route_age[slot] = ++sc64_route_clock;
    sc64_route_valid[slot] = 1;
    sc64_route_builds++;
    return slot;
}

static int GBA_HOT
sc64_route_next(int x, int y, int tx, int ty, int *next_x, int *next_y)
{
    int col = limit(x / 16, 0, MAP_COLS - 1), row = limit(y / 16, 0, MAP_ROWS - 1);
    int goal = limit(tx / 16, 0, MAP_COLS - 1) + limit(ty / 16, 0, MAP_ROWS - 1) * MAP_COLS;
    int cell = col + row * MAP_COLS, slot, direction;
#ifndef SC64_RACES
    u16 *field;
#endif
    if (!path_grid_valid) build_path_grid();
    slot = sc64_route_slot(goal);
#ifdef SC64_RACES
    direction = sc64_route_get(slot, cell);
    if (direction == 15) direction = -1;
#else
    field = sc64_route_field[slot];
    direction = field[cell] == 0xFFFF ? -1 : field[cell] & 7;
#endif
    if (direction < 0 && !path_walkable[cell]) {
        static const s8 dx[4] = {1,-1,0,0}, dy[4] = {0,0,1,-1};
        int i, best = -1, cost = 0x7fffffff;
        /* Imported/produced units may start in an inflated building cell. */
        for (i = 0; i < 4; i++) {
            int cx = col + dx[i], cy = row + dy[i], distance;
            int candidate;
            if (cx < 0 || cy < 0 || cx >= MAP_COLS || cy >= MAP_ROWS) continue;
#ifdef SC64_RACES
            candidate = cx + cy * MAP_COLS;
            if (sc64_route_get(slot, candidate) == 15) continue;
            /* Rare blocked starts have no distance nibble. Follow each
             * adjacent gradient to compare true path lengths. */
            distance = 0;
            while (candidate != goal && distance < MAP_CELLS) {
                int d = sc64_route_get(slot, candidate);
                if (d < 1 || d > 4) break;
                if (d == 1) candidate++;
                else if (d == 2) candidate--;
                else if (d == 3) candidate += MAP_COLS;
                else candidate -= MAP_COLS;
                ++distance;
            }
#else
            if (field[cx + cy * MAP_COLS] == 0xFFFF) continue;
            distance = field[cx + cy * MAP_COLS] >> 3;
#endif
            if (distance < cost) { cost = distance; best = i; }
        }
        if (best < 0) return 0;
        col += dx[best]; row += dy[best];
    } else if (direction < 0) return 0;
    else if (direction == 0) { *next_x = tx; *next_y = ty; return 1; }
    else if (direction == 1) col++;
    else if (direction == 2) col--;
    else if (direction == 3) row++;
    else row--;
    *next_x = col * 16 + 8; *next_y = row * 16 + 8;
    return 1;
}
#endif
