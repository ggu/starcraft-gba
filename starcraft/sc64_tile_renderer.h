#ifndef SC64_TILE_RENDERER_H
#define SC64_TILE_RENDERER_H

/* Experimental mode-0 world layers for imported race-campaign maps.
 *
 * BG3 shows terrain from deduplicated 8x8 tiles held in a VRAM cache, BG2 the
 * opaque fog/unexplored cells, and BG1 static art (minerals, geysers and
 * building art). All three are world-anchored 32x32 rings scrolled by the
 * hardware. Static art is drawn by the unchanged software code into the
 * back buffer, then lifted into BG1 tiles; dynamic objects and the HUD stay
 * in the software layer. Layer order matches the mode-4 draw order: terrain,
 * fog, static art, then everything drawn afterwards. */

#if !defined(SC64_TILE8_DATA)
#error "SC64_TILE_RENDERER requires the derived tile8 race-map header"
#endif
#if !defined(SC64_RACES) || !defined(SC64_IMPORTED_MISSION) || !defined(SC64_NO_FRAME_CLEAR)
#error "SC64_TILE_RENDERER requires imported race missions with persistent frames"
#endif
#if SC64_TILE8_WORST_WINDOW_TILES > GFX_TILE_TERRAIN_SLOTS
#error "Terrain tile cache cannot hold the worst 32x16 visible window"
#endif

/* VRAM tables (u16 only): pool-id hash, slot pool ids, slot references. */
#define SC64_TILE_HASH_SIZE 1024
#define SC64_TILE_HASH ((volatile u16 *)GFX_VRAM16 + GFX_TILE_TABLES / 2)
#define SC64_TILE_SLOT_ID (SC64_TILE_HASH + SC64_TILE_HASH_SIZE)
#define SC64_TILE_SLOT_REF (SC64_TILE_SLOT_ID + GFX_TILE_TERRAIN_SLOTS)
#define SC64_TILE_REF_PENDING 0x8000u
#define SC64_TILE_NO_ID 0xffffu
_Static_assert(GFX_TILE_TABLES + 2 * (SC64_TILE_HASH_SIZE + 2 * GFX_TILE_TERRAIN_SLOTS) <= 0x10000,
               "tile renderer tables exceed background VRAM");

typedef struct { int x0, y0, x1, y1; } Sc64TileWindow;

static const Sc64RaceMap *sc64_tile_map;
static Sc64TileWindow sc64_tile_held, sc64_tile_shown;
static int sc64_tile_held_valid, sc64_tile_shown_valid;
static int sc64_tile_camx, sc64_tile_camy, sc64_tile_slot_cursor;
static u32 sc64_tile_fog_key;
static int sc64_tile_fog_valid, sc64_tile_fog_pending;
static unsigned int sc64_tile_art_static_key;
static int sc64_tile_art_camx, sc64_tile_art_camy, sc64_tile_art_valid;
static int sc64_tile_art_pending, sc64_tile_art_back, sc64_tile_art_overflow;
static u8 sc64_tile_art_front_used[GFX_TILE_ART_SLOTS], sc64_tile_art_back_used[GFX_TILE_ART_SLOTS];
static int sc64_tile_hud_invalid, sc64_tile_released;
static volatile u32 sc64_tile_uploads, sc64_tile_jump_steals, sc64_tile_art_lifts,
    sc64_tile_art_overflows;
#ifdef SC64_DIAGNOSTIC_TILES
/* Cumulative cycles per renderer stage, read by the private profilers. */
volatile u32 sc64_tile_diag_clear, sc64_tile_diag_world, sc64_tile_diag_art_draw,
    sc64_tile_diag_lift, sc64_tile_diag_present, sc64_tile_diag_tiles_out, sc64_tile_diag_frames,
    sc64_tile_diag_hud_top, sc64_tile_diag_hud_bottom, sc64_tile_diag_hud_bands, sc64_tile_diag_minimap;
#define SC64_TILE_DIAG(counter, start) ((counter) += gba_clock() - (start))
#endif

static int
sc64_tile_map_supported(void)
{
    const Sc64RaceMap *map;
    if (race_mode || race_selected_mission < 0 || race_selected_mission >= SC64_RACE_MAP_COUNT)
        return 0;
    map = &sc64_race_maps[race_selected_mission];
    return map->tile8_meta && map->tile8_pool && map->tile_lookup &&
           map->width == MAP_COLS && map->height == MAP_ROWS &&
           MAP_COLS * 16 >= 256 && MAP_ROWS * 16 >= 128;
}

static u16
sc64_tile_meta(int wx, int wy)
{
    unsigned lookup = sc64_tile_map->tile_lookup[(wy >> 1) * MAP_COLS + (wx >> 1)];
    return sc64_tile_map->tile8_meta[lookup][((wy & 1) << 1) | (wx & 1)];
}

static unsigned
sc64_tile_hash_home(unsigned id)
{
    return (id * 40503u) & (SC64_TILE_HASH_SIZE - 1);
}

/* Returns the slot holding pool tile id, or -1. */
static int
sc64_tile_find(unsigned id)
{
    unsigned h = sc64_tile_hash_home(id);
    for (;;) {
        unsigned entry = SC64_TILE_HASH[h];
        if (!entry) return -1;
        if (SC64_TILE_SLOT_ID[entry - 1] == id) return (int)entry - 1;
        h = (h + 1) & (SC64_TILE_HASH_SIZE - 1);
    }
}

static void
sc64_tile_hash_remove(unsigned id)
{
    unsigned h = sc64_tile_hash_home(id), next;
    while (SC64_TILE_HASH[h] && SC64_TILE_SLOT_ID[SC64_TILE_HASH[h] - 1] != id)
        h = (h + 1) & (SC64_TILE_HASH_SIZE - 1);
    if (!SC64_TILE_HASH[h]) return;
    /* Backward-shift deletion keeps every remaining probe chain intact. */
    for (next = (h + 1) & (SC64_TILE_HASH_SIZE - 1); SC64_TILE_HASH[next];
         next = (next + 1) & (SC64_TILE_HASH_SIZE - 1)) {
        unsigned home = sc64_tile_hash_home(SC64_TILE_SLOT_ID[SC64_TILE_HASH[next] - 1]);
        if (((next - home) & (SC64_TILE_HASH_SIZE - 1)) >=
            ((next - h) & (SC64_TILE_HASH_SIZE - 1))) {
            SC64_TILE_HASH[h] = SC64_TILE_HASH[next];
            h = next;
        }
    }
    SC64_TILE_HASH[h] = 0;
}

static void
sc64_tile_upload(volatile u16 *dst, const u8 *src)
{
    const u32 *s = (const u32 *)src;
    volatile u32 *d = (volatile u32 *)dst;
    for (int i = 0; i < 16; ++i) d[i] = s[i];
}

/* Reference one terrain tile; reuse a cached copy or upload into a slot that
 * no displayed cell uses. Slots released this frame stay pending until the
 * next present, unless a camera jump leaves no other choice. */
static void
sc64_tile_acquire(unsigned id)
{
    int slot = sc64_tile_find(id), pass, n;
    if (slot >= 0) {
        SC64_TILE_SLOT_REF[slot] = (u16)((SC64_TILE_SLOT_REF[slot] & ~SC64_TILE_REF_PENDING) + 1);
        return;
    }
    for (pass = 0; pass < 2 && slot < 0; ++pass)
        for (n = 0; n < GFX_TILE_TERRAIN_SLOTS; ++n) {
            int candidate = (sc64_tile_slot_cursor + n) % GFX_TILE_TERRAIN_SLOTS;
            u16 ref = SC64_TILE_SLOT_REF[candidate];
            if ((ref & ~SC64_TILE_REF_PENDING) == 0 &&
                (pass || !(ref & SC64_TILE_REF_PENDING))) {
                slot = candidate;
                if (pass) ++sc64_tile_jump_steals;
                break;
            }
        }
    if (slot < 0) return; /* unreachable: the window bound is checked at build */
    sc64_tile_slot_cursor = (slot + 1) % GFX_TILE_TERRAIN_SLOTS;
    if (SC64_TILE_SLOT_ID[slot] != SC64_TILE_NO_ID) sc64_tile_hash_remove(SC64_TILE_SLOT_ID[slot]);
    SC64_TILE_SLOT_ID[slot] = (u16)id;
    SC64_TILE_SLOT_REF[slot] = 1;
    {
        unsigned h = sc64_tile_hash_home(id);
        while (SC64_TILE_HASH[h]) h = (h + 1) & (SC64_TILE_HASH_SIZE - 1);
        SC64_TILE_HASH[h] = (u16)(slot + 1);
    }
    sc64_tile_upload(GFX_VRAM16 + (GFX_TILE_TERRAIN_FIRST + slot) * 32,
                     sc64_tile_map->tile8_pool[id]);
    ++sc64_tile_uploads;
}

static void
sc64_tile_release(unsigned id)
{
    int slot = sc64_tile_find(id);
    u16 ref;
    if (slot < 0) return;
    ref = (u16)(SC64_TILE_SLOT_REF[slot] & ~SC64_TILE_REF_PENDING);
    if (ref > 1) SC64_TILE_SLOT_REF[slot] = (u16)(ref - 1);
    else { SC64_TILE_SLOT_REF[slot] = SC64_TILE_REF_PENDING; sc64_tile_released = 1; }
}

static int
sc64_tile_in(const Sc64TileWindow *w, int x, int y)
{
    return x >= w->x0 && x <= w->x1 && y >= w->y0 && y <= w->y1;
}

static void
sc64_tile_reset(void)
{
    volatile u16 *vram = GFX_VRAM16;
    for (int i = 0; i < SC64_TILE_HASH_SIZE; ++i) SC64_TILE_HASH[i] = 0;
    for (int i = 0; i < GFX_TILE_TERRAIN_SLOTS; ++i) {
        SC64_TILE_SLOT_ID[i] = SC64_TILE_NO_ID;
        SC64_TILE_SLOT_REF[i] = 0;
    }
    for (int i = 0; i < 1024; ++i) {
        vram[GFX_TILE_SBB_TERRAIN * 1024 + i] = 0;
        vram[GFX_TILE_SBB_FOG * 1024 + i] = 0;
        vram[GFX_TILE_SBB_ART * 1024 + i] = 0;
        vram[(GFX_TILE_SBB_ART + 1) * 1024 + i] = 0;
    }
    for (int i = 0; i < GFX_TILE_ART_SLOTS; ++i)
        sc64_tile_art_front_used[i] = sc64_tile_art_back_used[i] = 0;
    sc64_tile_map = &sc64_race_maps[race_selected_mission];
    sc64_tile_held_valid = sc64_tile_shown_valid = 0;
    sc64_tile_fog_valid = sc64_tile_art_valid = 0;
    sc64_tile_fog_pending = sc64_tile_art_pending = 0;
    sc64_tile_art_back = 1;
    sc64_tile_art_overflow = 0;
    sc64_tile_slot_cursor = 0;
    sc64_tile_hud_invalid = 1;
}

static u8
sc64_tile_fog_entry(int wx, int wy)
{
    int cell = (wx >> 1) + (wy >> 1) * MAP_COLS;
    if (SC64_GRID_GET(visible, cell)) return 0;
    return SC64_GRID_GET(explored, cell) ? GFX_TILE_FOG : GFX_TILE_BLACK;
}

/* Window bookkeeping, terrain references and the fog change key. */
static void
sc64_tile_world_update(int camx, int camy)
{
    Sc64TileWindow w;
    u32 key = 2166136261u;
    w.x0 = camx >> 3; w.x1 = (camx + 239) >> 3;
    w.y0 = camy >> 3; w.y1 = (camy + GFX_TILE_WORLD_BOTTOM - GFX_TILE_WORLD_TOP - 1) >> 3;
    if (sc64_tile_held_valid && w.x0 == sc64_tile_held.x0 && w.y0 == sc64_tile_held.y0 &&
        w.x1 == sc64_tile_held.x1 && w.y1 == sc64_tile_held.y1)
        goto fog_key; /* same cells: no references change */
    /* Release departing cells first: their slots stay pending until present,
     * so the displayed frame is untouched, and overlapping cells keep refs. */
    if (sc64_tile_held_valid)
        for (int y = sc64_tile_held.y0; y <= sc64_tile_held.y1; ++y)
            for (int x = sc64_tile_held.x0; x <= sc64_tile_held.x1; ++x)
                if (!sc64_tile_in(&w, x, y)) sc64_tile_release(sc64_tile_meta(x, y) & 0x3fffu);
    for (int y = w.y0; y <= w.y1; ++y)
        for (int x = w.x0; x <= w.x1; ++x)
            if (!sc64_tile_held_valid || !sc64_tile_in(&sc64_tile_held, x, y))
                sc64_tile_acquire(sc64_tile_meta(x, y) & 0x3fffu);
    sc64_tile_held = w;
    sc64_tile_held_valid = 1;
fog_key:
    sc64_tile_camx = camx;
    sc64_tile_camy = camy;
    for (int y = w.y0 >> 1; y <= w.y1 >> 1; ++y)
        for (int x = w.x0 >> 1; x <= w.x1 >> 1; ++x) {
            int cell = x + y * MAP_COLS;
            key = (key ^ (u32)((SC64_GRID_GET(visible, cell) ? 1 : 0) |
                               (SC64_GRID_GET(explored, cell) ? 2 : 0))) * 16777619u;
        }
    /* Any window change exposes new 8x8 fog entries, even inside one cell. */
    key = (key ^ (u32)(w.x0 | (w.y0 << 16))) * 16777619u;
    key = (key ^ (u32)(w.x1 | (w.y1 << 16))) * 16777619u;
    if (!sc64_tile_fog_valid || key != sc64_tile_fog_key) {
        sc64_tile_fog_key = key;
        sc64_tile_fog_valid = 1;
        sc64_tile_fog_pending = 1;
    }
}

static int
sc64_tile_art_needs_refresh(int camx, int camy, unsigned int static_key)
{
    return !sc64_tile_art_valid || sc64_tile_art_camx != camx || sc64_tile_art_camy != camy ||
           sc64_tile_art_static_key != static_key || sc64_tile_fog_pending;
}

/* Lift the static-art pixels just drawn into the back buffer into BG1 tiles
 * of the hidden art map. Returns 0 when slots run out; the caller then keeps
 * the pixels as ordinary overlay content for this frame. */
static int GBA_HOT __attribute__((noinline))
sc64_tile_art_lift(int camx, int camy, unsigned int static_key)
{
    const u8 *buffer = gfx_tile_buffer_pixels();
    volatile u16 *vram = GFX_VRAM16;
    volatile u16 *back = vram + (GFX_TILE_SBB_ART + sc64_tile_art_back) * 1024;
    volatile u16 *front = vram + (GFX_TILE_SBB_ART + (sc64_tile_art_back ^ 1)) * 1024;
    const Sc64TileWindow *w = &sc64_tile_held;
    u32 tile[16];
    int cursor = 0;
    ++sc64_tile_art_lifts;
    for (int i = 0; i < GFX_TILE_ART_SLOTS; ++i) sc64_tile_art_back_used[i] = 0;
    for (int i = 0; i < 1024; ++i) back[i] = 0;
    for (int wy = w->y0; wy <= w->y1; ++wy)
        for (int wx = w->x0; wx <= w->x1; ++wx) {
            int sx0 = wx * 8 - camx, sy0 = wy * 8 - camy + GFX_TILE_WORLD_TOP;
            int tx0 = sx0 < 0 ? 0 : sx0 >> 3, tx1 = (sx0 + 7) >> 3;
            int ty0 = sy0 >> 3, ty1 = (sy0 + 7) >> 3, marked = 0, any = 0;
            int ring = (wy & 31) * 32 + (wx & 31);
            u8 *bytes = (u8 *)tile;
            if (tx1 > 29) tx1 = 29;
            for (int ty = ty0; ty <= ty1; ++ty)
                if (ty >= 0 && ty < 20) for (int tx = tx0; tx <= tx1; ++tx)
                    if (gfx_tile_art_marks[ty] & (1u << tx)) marked = 1;
            if (!marked) continue;
            if (sx0 >= 0 && sx0 <= 232 && !(sx0 & 3) &&
                sy0 >= GFX_TILE_WORLD_TOP && sy0 <= GFX_TILE_WORLD_BOTTOM - 8) {
                /* camx is word aligned, so whole visible tiles copy by words. */
                const u32 *src = (const u32 *)(buffer + sy0 * 240 + sx0);
                for (int r = 0; r < 8; ++r, src += 60) {
                    tile[r * 2] = src[0];
                    tile[r * 2 + 1] = src[1];
                    any |= (src[0] | src[1]) != 0;
                }
            } else for (int r = 0; r < 8; ++r) {
                int sy = sy0 + r;
                for (int c = 0; c < 8; ++c) {
                    int sx = sx0 + c;
                    u8 v = (sy >= GFX_TILE_WORLD_TOP && sy < GFX_TILE_WORLD_BOTTOM &&
                            sx >= 0 && sx < 240) ? buffer[sy * 240 + sx] : 0;
                    bytes[r * 8 + c] = v;
                    any |= v;
                }
            }
            if (!any) continue;
            {
                unsigned old = front[ring] & 1023u;
                int slot = -1;
                if (old >= GFX_TILE_ART_FIRST && old < GFX_TILE_ART_FIRST + GFX_TILE_ART_SLOTS) {
                    volatile const u32 *have = (volatile const u32 *)(vram + old * 32);
                    int same = 1;
                    for (int i = 0; i < 16 && same; ++i) same = have[i] == tile[i];
                    if (same) slot = (int)old - GFX_TILE_ART_FIRST;
                }
                if (slot < 0) {
                    while (cursor < GFX_TILE_ART_SLOTS &&
                           (sc64_tile_art_front_used[cursor] || sc64_tile_art_back_used[cursor]))
                        ++cursor;
                    if (cursor >= GFX_TILE_ART_SLOTS) {
                        for (int i = 0; i < 1024; ++i) back[i] = 0;
                        for (int i = 0; i < GFX_TILE_ART_SLOTS; ++i) sc64_tile_art_back_used[i] = 0;
                        sc64_tile_art_overflow = 1;
                        sc64_tile_art_pending = 1;
                        sc64_tile_art_valid = 0;
                        ++sc64_tile_art_overflows;
                        return 0;
                    }
                    slot = cursor++;
                    sc64_tile_upload(vram + (GFX_TILE_ART_FIRST + slot) * 32, (const u8 *)tile);
                }
                sc64_tile_art_back_used[slot] = 1;
                back[ring] = (u16)(GFX_TILE_ART_FIRST + slot);
            }
        }
    sc64_tile_art_overflow = 0;
    sc64_tile_art_pending = 1;
    sc64_tile_art_valid = 1;
    sc64_tile_art_camx = camx;
    sc64_tile_art_camy = camy;
    sc64_tile_art_static_key = static_key;
    return 1;
}

/* After a successful lift, remove the art pixels from the software layer. */
static void
sc64_tile_art_clear_pixels(void)
{
    u8 *buffer = gfx_tile_buffer_pixels();
    for (int ty = 0; ty < 20; ++ty) {
        u32 m = gfx_tile_art_marks[ty];
        int y0 = ty * 8 < GFX_TILE_WORLD_TOP ? GFX_TILE_WORLD_TOP : ty * 8;
        int y1 = ty * 8 + 8 > GFX_TILE_WORLD_BOTTOM ? GFX_TILE_WORLD_BOTTOM : ty * 8 + 8;
        if (!m || y0 >= y1) continue;
        for (int tx = 0; tx < 30; ++tx)
            if (m & (1u << tx))
                for (int y = y0; y < y1; ++y) {
                    u32 *row = (u32 *)(buffer + y * 240 + tx * 8);
                    row[0] = 0; row[1] = 0;
                }
    }
}

/* Art kept in the software layer becomes ordinary changed overlay tiles. */
static void
sc64_tile_art_keep_pixels(void)
{
    for (int ty = 0; ty < 20; ++ty) {
        gfx_tile_dirty[ty] |= gfx_tile_art_marks[ty];
        gfx_tile_drawn[ty] |= gfx_tile_art_marks[ty];
    }
}

/* VBlank work, before the software tiles: scroll, new terrain cells, fog
 * and the art map flip. */
static void
sc64_tile_present(void)
{
    volatile u16 *vram = GFX_VRAM16;
    const Sc64TileWindow *w = &sc64_tile_held;
    u16 hofs = (u16)(sc64_tile_camx & 0x1ff);
    u16 vofs = (u16)((sc64_tile_camy - GFX_TILE_WORLD_TOP) & 0x1ff);
    if (!sc64_tile_held_valid) return;
    for (int bg = 1; bg < 4; ++bg) {
        GFX_IO16(0x10 + bg * 4) = hofs;
        GFX_IO16(0x12 + bg * 4) = vofs;
    }
    if (!sc64_tile_shown_valid || w->x0 != sc64_tile_shown.x0 || w->y0 != sc64_tile_shown.y0 ||
        w->x1 != sc64_tile_shown.x1 || w->y1 != sc64_tile_shown.y1)
    for (int y = w->y0; y <= w->y1; ++y)
        for (int x = w->x0; x <= w->x1; ++x)
            if (!sc64_tile_shown_valid || !sc64_tile_in(&sc64_tile_shown, x, y)) {
                u16 meta = sc64_tile_meta(x, y);
                int slot = sc64_tile_find(meta & 0x3fffu);
                vram[GFX_TILE_SBB_TERRAIN * 1024 + (y & 31) * 32 + (x & 31)] =
                    (u16)((slot < 0 ? 0 : GFX_TILE_TERRAIN_FIRST + slot) |
                          ((meta >> 14) & 1u) << 10 | ((meta >> 15) & 1u) << 11);
            }
    sc64_tile_shown = *w;
    sc64_tile_shown_valid = 1;
    if (sc64_tile_fog_pending) {
        /* One visibility read per 16px cell feeds its four 8x8 entries. */
        volatile u16 *fog = vram + GFX_TILE_SBB_FOG * 1024;
        for (int y = w->y0 & ~1; y <= w->y1; y += 2)
            for (int x = w->x0 & ~1; x <= w->x1; x += 2) {
                u16 e = sc64_tile_fog_entry(x, y);
                int row = (y & 31) * 32, next = ((y + 1) & 31) * 32;
                fog[row + (x & 31)] = e; fog[row + ((x + 1) & 31)] = e;
                fog[next + (x & 31)] = e; fog[next + ((x + 1) & 31)] = e;
            }
        sc64_tile_fog_pending = 0;
    }
    if (sc64_tile_art_pending) {
        GFX_IO16(0x0a) = (u16)(0x0081 | ((GFX_TILE_SBB_ART + sc64_tile_art_back) << 8));
        for (int i = 0; i < GFX_TILE_ART_SLOTS; ++i)
            sc64_tile_art_front_used[i] = sc64_tile_art_back_used[i];
        sc64_tile_art_back ^= 1;
        sc64_tile_art_pending = 0;
    }
    if (sc64_tile_released) {
        for (int i = 0; i < GFX_TILE_TERRAIN_SLOTS; ++i)
            if (SC64_TILE_SLOT_REF[i] & SC64_TILE_REF_PENDING) SC64_TILE_SLOT_REF[i] = 0;
        sc64_tile_released = 0;
    }
}

#endif
