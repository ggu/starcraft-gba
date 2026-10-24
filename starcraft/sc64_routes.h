#ifndef SC64_ROUTES_H
#define SC64_ROUTES_H

/* Standalone route fixtures still use byte grids; game.c supplies the packed
 * accessor for SC64_LARGE_MAPS builds. */
#ifndef SC64_GRID_GET
#define SC64_GRID_GET(grid, cell) ((grid)[(cell)] != 0)
#define SC64_ROUTES_LOCAL_GRID_GET
#endif

#ifdef SC64_ROUTE_VISIT_GRID
#if !defined(SC64_RACES) || !defined(SC64_ROUTE_PACK3)
#error "SC64_ROUTE_VISIT_GRID requires SC64_RACES and SC64_ROUTE_PACK3"
#endif
#ifndef SC64_GRID_SET
#ifdef SC64_ROUTES_LOCAL_GRID_GET
#define SC64_GRID_SET(grid, cell, value) ((grid)[(cell)] = (u8)((value) != 0))
#define SC64_ROUTES_LOCAL_GRID_SET
#else
#error "SC64_ROUTE_VISIT_GRID requires SC64_GRID_SET for a custom grid accessor"
#endif
#endif
#endif

/* Shared shortest-path fields on the same four-neighbour collision grid.
 * A mineral, base or group destination costs one reverse flood, rather than
 * one flood per moving unit per tile. Dynamic actors still use local steering.
 * Derived caches are discarded after load and whenever a structure changes. */
/* The race game needs fields for four economies and several armies. */
#if defined(SC64_ROUTE_PACK3) && !defined(SC64_RACES)
#error "SC64_ROUTE_PACK3 is only supported by race route fields"
#endif
#if defined(SC64_T4_RUNTIME) && (!defined(SC64_RACES) || !defined(SC64_LARGE_MAPS) || \
                                !defined(SC64_ROUTE_PACK3))
#error "SC64_T4_RUNTIME requires large packed race routes"
#endif
#ifdef SC64_RACES
#if defined(SC64_T4_RUNTIME) || defined(SC64_ZP_CAMPAIGN_STORAGE)
#define SC64_ROUTE_SLOTS 14
#elif defined(SC64_LARGE_MAPS)
#ifdef SC64_ROUTE_PACK3
#define SC64_ROUTE_SLOTS 20
#else
#define SC64_ROUTE_SLOTS 15
#endif
#elif defined(SC64_T3_ENABLED)
#ifdef SC64_ROUTE_PACK3
#define SC64_ROUTE_SLOTS 22
#else
#define SC64_ROUTE_SLOTS 17
#endif
#else
#ifdef SC64_ROUTE_PACK3
#define SC64_ROUTE_SLOTS 24
#else
#define SC64_ROUTE_SLOTS 18
#endif
#endif
#ifdef SC64_ROUTE_PACK3
#define SC64_ROUTE_BYTES ((MAP_CELLS * 3 + 7) / 8)
#define SC64_ROUTE_UNREACHABLE 7
#else
#define SC64_ROUTE_BYTES ((MAP_CELLS + 1) / 2)
#define SC64_ROUTE_UNREACHABLE 15
#endif
#elif defined(SC64_SKIRMISH)
#define SC64_ROUTE_SLOTS 16
#else
#define SC64_ROUTE_SLOTS 8
#endif
#ifdef SC64_RACES
/* Default: two cells per byte. Optional PACK3: one 3-bit cell code. In either
 * layout 0 is the goal, 1..4 point toward a shortest-path neighbour, and the
 * all-ones code means unreachable. */
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
#ifdef SC64_ROUTE_PACK3
    u32 bit = (u32)cell * 3u;
    unsigned byte = bit >> 3, shift = bit & 7u;
    u16 pair = sc64_route_field[slot][byte];
    if (shift > 5u) pair |= (u16)sc64_route_field[slot][byte + 1] << 8;
    return (pair >> shift) & 7u;
#else
    u8 v = sc64_route_field[slot][cell >> 1];
    return (cell & 1) ? v >> 4 : v & 15;
#endif
}

static void GBA_HOT sc64_route_put(int slot, int cell, int direction)
{
#ifdef SC64_ROUTE_PACK3
    u32 bit = (u32)cell * 3u;
    unsigned byte = bit >> 3, shift = bit & 7u;
    u16 pair = sc64_route_field[slot][byte];
    u16 mask = (u16)(7u << shift);
    if (shift > 5u) pair |= (u16)sc64_route_field[slot][byte + 1] << 8;
    pair = (u16)((pair & ~mask) | (((u16)direction & 7u) << shift));
    sc64_route_field[slot][byte] = (u8)pair;
    if (shift > 5u) sc64_route_field[slot][byte + 1] = (u8)(pair >> 8);
#else
    u8 *p = &sc64_route_field[slot][cell >> 1];
    if (cell & 1) *p = (u8)((*p & 15) | (direction << 4));
    else *p = (u8)((*p & 240) | direction);
#endif
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
    if (SC64_GRID_GET(path_walkable, goal)) {
#ifdef SC64_RACES
        sc64_route_put(slot, goal, 0);
#else
        field[goal] = 0;
#endif
        bfs_queue[tail++] = (u16)goal;
#ifdef SC64_ROUTE_VISIT_GRID
        SC64_GRID_SET(path_walkable, goal, 0);
#endif
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
            if (SC64_GRID_GET(path_walkable, cell)) {
#ifdef SC64_RACES
                if (sc64_route_get(slot, cell) == SC64_ROUTE_UNREACHABLE) {
                    sc64_route_put(slot, cell, 0); bfs_queue[tail++] = (u16)cell;
#ifdef SC64_ROUTE_VISIT_GRID
                    SC64_GRID_SET(path_walkable, cell, 0);
#endif
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
#ifdef SC64_ROUTE_VISIT_GRID
#define SC64_FLOOD(condition, neighbour, direction) \
        if (condition) { next = (neighbour); \
            if (SC64_GRID_GET(path_walkable, next)) { \
                sc64_route_put(slot, next, (direction)); bfs_queue[tail++] = (u16)next; \
                SC64_GRID_SET(path_walkable, next, 0); } }
#else
#define SC64_FLOOD(condition, neighbour, direction) \
        if (condition) { next = (neighbour); \
            if (SC64_GRID_GET(path_walkable, next) && sc64_route_get(slot, next) == SC64_ROUTE_UNREACHABLE) { \
                sc64_route_put(slot, next, (direction)); bfs_queue[tail++] = (u16)next; } }
#endif
#else
#define SC64_FLOOD(condition, neighbour, direction) \
        if (condition) { next = (neighbour); \
            if (SC64_GRID_GET(path_walkable, next) && field[next] == 0xFFFF) { \
                field[next] = (u16)(((field[cell] & ~7u) + 8u) | (direction)); \
                bfs_queue[tail++] = (u16)next; } }
#endif
        SC64_FLOOD(col > 0, cell - 1, 1);
        SC64_FLOOD(col + 1 < MAP_COLS, cell + 1, 2);
        SC64_FLOOD(row > 0, cell - MAP_COLS, 3);
        SC64_FLOOD(row + 1 < MAP_ROWS, cell + MAP_COLS, 4);
#undef SC64_FLOOD
    }
#ifdef SC64_ROUTE_VISIT_GRID
    /* Each enqueued cell was cleared exactly once, so bfs_queue is a compact
     * restoration list for the temporarily borrowed walkability bits. */
    for (i = 0; i < tail; ++i) SC64_GRID_SET(path_walkable, bfs_queue[i], 1);
#endif
    sc64_route_goal[slot] = (u16)goal;
    sc64_route_age[slot] = ++sc64_route_clock;
    sc64_route_valid[slot] = 1;
    sc64_route_builds++;
    return slot;
}

#if defined(SC64_RACES) && defined(SC64_ROUTE_PACK3)
/* Inflated structure cells are rare actor origins. Keep their gradient walk in
 * ROM so the hot route query stays small; selection/ties remain identical. */
static int __attribute__((noinline))
sc64_route_blocked_start(int slot,int col,int row,int goal,int *out_col,int *out_row)
{
    static const s8 dx[4]={1,-1,0,0},dy[4]={0,0,1,-1};
    int i,best=-1,cost=0x7fffffff;
    for(i=0;i<4;i++) {
        int cx=col+dx[i],cy=row+dy[i],distance,candidate;
        if(cx<0||cy<0||cx>=MAP_COLS||cy>=MAP_ROWS)continue;
        candidate=cx+cy*MAP_COLS;
        if(sc64_route_get(slot,candidate)==SC64_ROUTE_UNREACHABLE)continue;
        /* Follow each adjacent packed gradient to compare actual path lengths.
         * Direction zero (including a blocked-goal seed) stops this walk. */
        distance=0;
        while(candidate!=goal&&distance<MAP_CELLS) {
            int d=sc64_route_get(slot,candidate);
            if(d<1||d>4)break;
            if(d==1)candidate++;
            else if(d==2)candidate--;
            else if(d==3)candidate+=MAP_COLS;
            else candidate-=MAP_COLS;
            ++distance;
        }
        if(distance<cost){cost=distance;best=i;}
    }
    if(best<0)return 0;
    *out_col=col+dx[best];*out_row=row+dy[best];
    return 1;
}
#endif

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
    if (direction == SC64_ROUTE_UNREACHABLE) direction = -1;
#else
    field = sc64_route_field[slot];
    direction = field[cell] == 0xFFFF ? -1 : field[cell] & 7;
#endif
#if defined(SC64_RACES) && defined(SC64_ROUTE_PACK3)
    if (direction < 0 && !SC64_GRID_GET(path_walkable, cell)) {
        if(!sc64_route_blocked_start(slot,col,row,goal,&col,&row))return 0;
    } else if(direction<0)return 0;
    else if(direction==0){*next_x=tx;*next_y=ty;return 1;}
    else if(direction==1)col++;
    else if(direction==2)col--;
    else if(direction==3)row++;
    else row--;
#else
    if (direction < 0 && !SC64_GRID_GET(path_walkable, cell)) {
        static const s8 dx[4] = {1,-1,0,0}, dy[4] = {0,0,1,-1};
        int i, best = -1, cost = 0x7fffffff;
        /* Imported/produced units may start in an inflated building cell. */
        for (i = 0; i < 4; i++) {
            int cx = col + dx[i], cy = row + dy[i], distance;
            int candidate;
            if (cx < 0 || cy < 0 || cx >= MAP_COLS || cy >= MAP_ROWS) continue;
#ifdef SC64_RACES
            candidate = cx + cy * MAP_COLS;
            if (sc64_route_get(slot, candidate) == SC64_ROUTE_UNREACHABLE) continue;
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
#endif
    *next_x = col * 16 + 8; *next_y = row * 16 + 8;
    return 1;
}
#ifdef SC64_ROUTES_LOCAL_GRID_GET
#undef SC64_GRID_GET
#undef SC64_ROUTES_LOCAL_GRID_GET
#endif
#ifdef SC64_ROUTES_LOCAL_GRID_SET
#undef SC64_GRID_SET
#undef SC64_ROUTES_LOCAL_GRID_SET
#endif
#endif
