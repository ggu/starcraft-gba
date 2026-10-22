#ifndef SC64_TERRAIN_CACHE_INCLUDED
#define SC64_TERRAIN_CACHE_INCLUDED

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

static int
sc64_terrain_cache_mission_key(void)
{
#ifdef SC64_RACES
    return race_mode?100:race_selected_mission;
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
            u8 state = (visible[cell] ? 1 : 0) | (explored[cell] ? 2 : 0);
            sc64_terrain_cache_dirty[i] = sc64_terrain_cache_mask[i] != state;
            if (sc64_terrain_cache_dirty[i]) unchanged = 0;
            i++;
        }
    if (unchanged) sc64_terrain_hits++;
    else sc64_terrain_partial++;
    return unchanged;
}

static int
sc64_terrain_tile_clean(int col, int row)
{
    int col0, row0, col1, row1, i;
    if (!sc64_terrain_cache_reused_base ||
        !sc64_terrain_cache_view(sc64_terrain_cache_camx, sc64_terrain_cache_camy,
                                 &col0, &row0, &col1, &row1) ||
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
                (visible[cell] ? 1 : 0) | (explored[cell] ? 2 : 0);
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

#endif
