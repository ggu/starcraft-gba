#ifndef SC64_TERRAIN_CACHE_INCLUDED
#define SC64_TERRAIN_CACHE_INCLUDED

#ifndef SC64_GRID_GET
#define SC64_GRID_GET(grid, cell) ((grid)[(cell)] != 0)
#define SC64_TERRAIN_LOCAL_GRID_GET
#endif

/* Cache only the static terrain and fog pixels. Dynamic objects are drawn
 * after this band is restored each frame. Keep the buffer word-aligned for
 * the Mode 4 DMA helpers. */
#define SC64_TERRAIN_CACHE_Y 12
#define SC64_TERRAIN_CACHE_HEIGHT 108
#define SC64_TERRAIN_CACHE_CELLS (17 * 9)
#ifndef SC64_TERRAIN_STATIC_KEY
#define SC64_TERRAIN_STATIC_KEY 0u
#endif

static u8 sc64_terrain_cache_pixels[240 * SC64_TERRAIN_CACHE_HEIGHT]
    __attribute__((aligned(4)));
static u8 sc64_terrain_cache_mask[SC64_TERRAIN_CACHE_CELLS];
static u8 sc64_terrain_cache_dirty[SC64_TERRAIN_CACHE_CELLS];
static int sc64_terrain_cache_valid;
static volatile unsigned sc64_terrain_requests, sc64_terrain_hits, sc64_terrain_partial;
static int sc64_terrain_cache_reused_base;
static int sc64_terrain_cache_camx, sc64_terrain_cache_camy;
static int sc64_terrain_cache_mission;
static unsigned int sc64_terrain_cache_static_key;
static int sc64_terrain_cache_mask_count;
#ifdef SC64_TERRAIN_SCROLL
/* dirty[] is indexed for the camera currently being rendered; mask[] and
 * camx/camy continue to describe the last captured framebuffer until store. */
static int sc64_terrain_cache_view_camx, sc64_terrain_cache_view_camy;
#endif

static int
sc64_terrain_cache_mission_key(void)
{
#ifdef SC64_RACES
    return race_mode?
#ifdef SC64_IMPORTED_SKIRMISH
        100+race_skirmish_map
#else
        100
#endif
        :race_selected_mission;
#elif defined(SC64_TERRAN_CAMPAIGN)
    return campaign_mission;
#else
    return 0;
#endif
}

static int
sc64_terrain_cache_view(int x, int y, int *col0, int *row0,
                        int *col1, int *row1)
{
    if (x < 0 || y < 0 || x > WORLD_W - 240 || y > WORLD_H - SC64_TERRAIN_CACHE_HEIGHT)
        return 0;
    *col0 = x / 16;
    *col1 = (x + 240) / 16 + 1;
    *row0 = y / 16;
    *row1 = (y + SC64_TERRAIN_CACHE_HEIGHT) / 16 + 1;
    if (*col1 > MAP_COLS) *col1 = MAP_COLS;
    if (*row1 > MAP_ROWS) *row1 = MAP_ROWS;
    return (*col1 > *col0 && *row1 > *row0 &&
            (*col1 - *col0) * (*row1 - *row0) <= SC64_TERRAIN_CACHE_CELLS);
}

static int
sc64_terrain_cached(int x, int y)
{
#ifdef SC64_TERRAIN_SCROLL
    int old_col0, old_row0, old_col1, old_row1;
    int col0, row0, col1, row1, col, row, i = 0, unchanged = 1;
    int dx, dy;
    sc64_terrain_requests++;
    sc64_terrain_cache_reused_base = 0;
    if (!sc64_terrain_cache_valid ||
        sc64_terrain_cache_mission != sc64_terrain_cache_mission_key() ||
        sc64_terrain_cache_static_key != (unsigned int)(SC64_TERRAIN_STATIC_KEY))
        return 0;
    dx = x - sc64_terrain_cache_camx;
    dy = y - sc64_terrain_cache_camy;
    if ((dx != 0 || dy != 0) &&
        ((dx < 0 ? -dx : dx) >= 240 ||
         (dy < 0 ? -dy : dy) >= SC64_TERRAIN_CACHE_HEIGHT))
        return 0;
    if (!sc64_terrain_cache_view(sc64_terrain_cache_camx, sc64_terrain_cache_camy,
                                 &old_col0, &old_row0, &old_col1, &old_row1))
        return 0;
    if (dx == 0 && dy == 0) {
        if ((old_col1 - old_col0) * (old_row1 - old_row0) !=
            sc64_terrain_cache_mask_count) {
            sc64_terrain_cache_valid = 0;
            return 0;
        }
        if (!gfx_blit_band(SC64_TERRAIN_CACHE_Y, SC64_TERRAIN_CACHE_HEIGHT,
                           sc64_terrain_cache_pixels)) {
            sc64_terrain_cache_valid = 0;
            return 0;
        }
        sc64_terrain_cache_view_camx = x;
        sc64_terrain_cache_view_camy = y;
        sc64_terrain_cache_reused_base = 1;
        for (row = old_row0; row < old_row1; row++)
            for (col = old_col0; col < old_col1; col++) {
                int cell = col + row * MAP_COLS;
                u8 state = (SC64_GRID_GET(visible, cell) ? 1 : 0) |
                           (SC64_GRID_GET(explored, cell) ? 2 : 0);
                sc64_terrain_cache_dirty[i] = sc64_terrain_cache_mask[i] != state;
                if (sc64_terrain_cache_dirty[i]) unchanged = 0;
                i++;
            }
        if (unchanged) sc64_terrain_hits++;
        else sc64_terrain_partial++;
        return unchanged;
    }
    if (!sc64_terrain_cache_view(x, y, &col0, &row0, &col1, &row1))
        return 0;
    if ((old_col1 - old_col0) * (old_row1 - old_row0) !=
        sc64_terrain_cache_mask_count) {
        sc64_terrain_cache_valid = 0;
        return 0;
    }
    if (!gfx_blit_window(SC64_TERRAIN_CACHE_Y, SC64_TERRAIN_CACHE_HEIGHT,
                                sc64_terrain_cache_pixels, 240,
                                SC64_TERRAIN_CACHE_HEIGHT, dx, dy)) {
        sc64_terrain_cache_valid = 0;
        return 0;
    }
    sc64_terrain_cache_view_camx = x;
    sc64_terrain_cache_view_camy = y;
    sc64_terrain_cache_reused_base = 1;
    for (row = row0; row < row1; row++)
        for (col = col0; col < col1; col++) {
            int cell = col + row * MAP_COLS;
            int left = col * 16, top = row * 16;
            int right = left + 16, bottom = top + 16;
            int old_index = -1;
            u8 state = (SC64_GRID_GET(visible, cell) ? 1 : 0) |
                       (SC64_GRID_GET(explored, cell) ? 2 : 0);
            if (left < x) left = x;
            if (top < y) top = y;
            if (right > x + 240) right = x + 240;
            if (bottom > y + SC64_TERRAIN_CACHE_HEIGHT)
                bottom = y + SC64_TERRAIN_CACHE_HEIGHT;
            if (col >= old_col0 && col < old_col1 &&
                row >= old_row0 && row < old_row1)
                old_index = (row - old_row0) * (old_col1 - old_col0) + col - old_col0;
            sc64_terrain_cache_dirty[i] =
                old_index < 0 ||
                left < sc64_terrain_cache_camx ||
                right > sc64_terrain_cache_camx + 240 ||
                top < sc64_terrain_cache_camy ||
                bottom > sc64_terrain_cache_camy + SC64_TERRAIN_CACHE_HEIGHT ||
                sc64_terrain_cache_mask[old_index < 0 ? 0 : old_index] != state;
            if (sc64_terrain_cache_dirty[i]) unchanged = 0;
            i++;
        }
    if (unchanged) sc64_terrain_hits++;
    else sc64_terrain_partial++;
    return unchanged;
#else
    int col0, row0, col1, row1, col, row, i = 0, unchanged = 1;
    sc64_terrain_requests++;
    sc64_terrain_cache_reused_base = 0;
    if (!sc64_terrain_cache_valid ||
        x != sc64_terrain_cache_camx || y != sc64_terrain_cache_camy ||
        sc64_terrain_cache_mission != sc64_terrain_cache_mission_key() ||
        sc64_terrain_cache_static_key != (unsigned int)(SC64_TERRAIN_STATIC_KEY))
        return 0;
    if (!gfx_blit_band(SC64_TERRAIN_CACHE_Y, SC64_TERRAIN_CACHE_HEIGHT,
                       sc64_terrain_cache_pixels)) {
        sc64_terrain_cache_valid = 0;
        return 0;
    }
    if (!sc64_terrain_cache_view(x, y, &col0, &row0, &col1, &row1) ||
        (col1 - col0) * (row1 - row0) != sc64_terrain_cache_mask_count) {
        sc64_terrain_cache_valid = 0;
        return 0;
    }
    sc64_terrain_cache_reused_base = 1;
    for (row = row0; row < row1; row++)
        for (col = col0; col < col1; col++) {
            int cell = col + row * MAP_COLS;
            u8 state = (SC64_GRID_GET(visible, cell) ? 1 : 0) |
                       (SC64_GRID_GET(explored, cell) ? 2 : 0);
            sc64_terrain_cache_dirty[i] = sc64_terrain_cache_mask[i] != state;
            if (sc64_terrain_cache_dirty[i]) unchanged = 0;
            i++;
        }
    if (unchanged) sc64_terrain_hits++;
    else sc64_terrain_partial++;
    return unchanged;
#endif
}

static int
sc64_terrain_tile_clean(int col, int row)
{
    int col0, row0, col1, row1, i;
#ifdef SC64_TERRAIN_SCROLL
    int x = sc64_terrain_cache_view_camx, y = sc64_terrain_cache_view_camy;
    if (!sc64_terrain_cache_reused_base ||
        !sc64_terrain_cache_view(x, y, &col0, &row0, &col1, &row1) ||
#else
    if (!sc64_terrain_cache_reused_base ||
        !sc64_terrain_cache_view(sc64_terrain_cache_camx, sc64_terrain_cache_camy,
                                 &col0, &row0, &col1, &row1) ||
#endif
        col < col0 || col >= col1 || row < row0 || row >= row1)
        return 0;
    i = (row - row0) * (col1 - col0) + col - col0;
    return !sc64_terrain_cache_dirty[i];
}

static int
sc64_terrain_store(int x, int y)
{
    int col0, row0, col1, row1, col, row, i = 0;
    if (!sc64_terrain_cache_view(x, y, &col0, &row0, &col1, &row1) ||
        !gfx_read_band(SC64_TERRAIN_CACHE_Y, SC64_TERRAIN_CACHE_HEIGHT,
                       sc64_terrain_cache_pixels)) {
        sc64_terrain_cache_valid = 0;
        sc64_terrain_cache_reused_base = 0;
        return 0;
    }
    for (row = row0; row < row1; row++)
        for (col = col0; col < col1; col++) {
            int cell = col + row * MAP_COLS;
            sc64_terrain_cache_mask[i++] =
                (SC64_GRID_GET(visible, cell) ? 1 : 0) |
                (SC64_GRID_GET(explored, cell) ? 2 : 0);
        }
    sc64_terrain_cache_mask_count = i;
    sc64_terrain_cache_camx = x;
    sc64_terrain_cache_camy = y;
    sc64_terrain_cache_mission = sc64_terrain_cache_mission_key();
    sc64_terrain_cache_static_key = (unsigned int)(SC64_TERRAIN_STATIC_KEY);
    sc64_terrain_cache_valid = 1;
    sc64_terrain_cache_reused_base = 0;
    return 1;
}

#ifdef SC64_TERRAIN_LOCAL_GRID_GET
#undef SC64_GRID_GET
#undef SC64_TERRAIN_LOCAL_GRID_GET
#endif
#endif
