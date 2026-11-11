/* Asset-free fog raster and signature tests. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/sc64_fog.h"

static u8 terrain[256] __attribute__((aligned(4)));
static u8 shade[256];

static unsigned reference_state(unsigned sig, unsigned digit)
{
    while (digit--) sig /= 3u;
    return sig % 3u;
}

static unsigned reference_coverage(unsigned d2)
{
    if (d2 <= 4) return 16;
    if (d2 <= 9) return 14;
    if (d2 <= 16) return 11;
    if (d2 <= 25) return 8;
    if (d2 <= 36) return 5;
    if (d2 <= 49) return 2;
    return 0;
}

static unsigned reference_bayer(unsigned x, unsigned y)
{
    static const unsigned table[4][4] = {
        {0, 8, 2, 10}, {12, 4, 14, 6},
        {3, 11, 1, 9}, {15, 7, 13, 5}
    };
    return table[y & 3u][x & 3u];
}

/* Deliberately computes each pixel's neighbor distances from scratch rather
 * than using the implementation's per-neighbor maxima. */
static u8 reference_pixel(unsigned sig, unsigned px, unsigned py)
{
    unsigned center = reference_state(sig, 4), unknown = 0, explored = 0;
    if (center == SC64_FOG_UNKNOWN) return C_BLACK;
    for (unsigned digit = 0; digit < 9; ++digit) {
        int nx = (int)(digit % 3u) - 1;
        int ny = (int)(digit / 3u) - 1;
        unsigned state = reference_state(sig, digit), dx, dy, c;
        if (!nx && !ny) continue;
        dx = nx < 0 ? px + 1u : nx > 0 ? 16u - px : 0u;
        dy = ny < 0 ? py + 1u : ny > 0 ? 16u - py : 0u;
        c = reference_coverage(dx * dx + dy * dy);
        if (state == SC64_FOG_UNKNOWN && c > unknown) unknown = c;
        if (state == SC64_FOG_EXPLORED && c > explored) explored = c;
    }
    unsigned threshold = reference_bayer(px, py);
    unsigned black = unknown * 2u > 16u ? unknown * 2u - 16u : 0u;
    if (center == SC64_FOG_VISIBLE) {
        if (black > threshold) return C_BLACK;
        if (unknown * 2u > threshold || explored > threshold)
            return shade[terrain[py * 16u + px]];
        return terrain[py * 16u + px];
    }
    if (black > threshold) return C_BLACK;
    return shade[terrain[py * 16u + px]];
}

static void check_signature_raster(unsigned sig)
{
    const int x = 40, y = 40;
    sc64_fog_raster_cell(x, y, (u16)sig, terrain, shade);
    for (unsigned py = 0; py < 16; ++py)
        for (unsigned px = 0; px < 16; ++px)
            assert(gfx_test_pixel(x + (int)px, y + (int)py) ==
                   reference_pixel(sig, px, py));
}

static void test_signature_encoding(void)
{
    u8 visible[25] = {0}, explored[25] = {0};
    for (unsigned seed = 0; seed < 100; ++seed) {
        unsigned expected = 0, place = 1;
        for (unsigned cell = 0; cell < 25; ++cell) {
            visible[cell] = (u8)(((cell * 13u + seed * 7u) % 11u) < 3u);
            explored[cell] = (u8)(((cell * 5u + seed * 3u) % 7u) < 3u);
        }
        int col = (int)(seed % 5u), row = (int)((seed / 5u) % 5u);
        for (int dy = -1; dy <= 1; ++dy)
            for (int dx = -1; dx <= 1; ++dx, place *= 3u) {
                int x = col + dx, y = row + dy;
                unsigned state = SC64_FOG_UNKNOWN;
                if (x >= 0 && x < 5 && y >= 0 && y < 5) {
                    unsigned cell = (unsigned)(x + y * 5);
                    if (visible[cell]) state = SC64_FOG_VISIBLE;
                    else if (explored[cell]) state = SC64_FOG_EXPLORED;
                }
                expected += state * place;
            }
        assert(sc64_fog_signature(visible, explored, 5, 5, col, row) == expected);
    }
    /* Explicitly prove visible wins over explored and out-of-map is unknown. */
    memset(visible, 0, sizeof(visible)); memset(explored, 1, sizeof(explored));
    visible[0] = 1;
    u16 sig = sc64_fog_signature(visible, explored, 5, 5, 0, 0);
    assert(reference_state(sig, 4) == SC64_FOG_VISIBLE);
    assert(reference_state(sig, 0) == SC64_FOG_UNKNOWN);
}

static void test_all_neighborhoods(void)
{
    /* Every ternary 3x3 signature, including every center class. */
    for (unsigned sig = 0; sig < SC64_FOG_SIGNATURE_COUNT; ++sig)
        check_signature_raster(sig);
}

static void test_unknown_never_reads_terrain(void)
{
    u8 poisoned[256];
    memset(poisoned, 0xA5, sizeof(poisoned));
    gfx_begin(C_HIGHLIGHT);
    sc64_fog_raster_cell(16, 32, 0, NULL, NULL);
    for (int y = 32; y < 48; ++y)
        for (int x = 16; x < 32; ++x)
            assert(gfx_test_pixel(x, y) == C_BLACK);
    gfx_begin(C_HIGHLIGHT);
    (void)poisoned;
    sc64_fog_raster_cell(16, 32, 0, poisoned, shade);
    for (int y = 32; y < 48; ++y)
        for (int x = 16; x < 32; ++x)
            assert(gfx_test_pixel(x, y) == C_BLACK);
}

static void test_world_anchor_and_clipping(void)
{
    u8 first[256], second[256];
    unsigned visible = 0;
    for (unsigned d = 0, place = 1; d < 9; ++d, place *= 3u)
        visible += 2u * place;
    gfx_begin(C_HIGHLIGHT);
    sc64_fog_raster_cell(16, 32, (u16)visible, terrain, shade);
    for (unsigned i = 0; i < 256; ++i) first[i] = gfx_test_pixel(16 + i % 16, 32 + i / 16);
    gfx_begin(C_HIGHLIGHT);
    sc64_fog_raster_cell(80, 64, (u16)visible, terrain, shade);
    for (unsigned i = 0; i < 256; ++i) second[i] = gfx_test_pixel(80 + i % 16, 64 + i / 16);
    assert(memcmp(first, second, sizeof(first)) == 0);

    u16 unknown_north = (u16)(visible - 6u); /* top-center base-3 digit becomes unknown */
    const int ys[] = {-10, -3, 0, 6, 11, 12, 110, 115, 119, 120, 130};
    for (unsigned k = 0; k < sizeof(ys) / sizeof(ys[0]); ++k)
        for (unsigned kind = 0; kind < 3; ++kind) {
            u16 sig = kind == 0 ? (u16)visible : kind == 1 ? unknown_north : 0;
            gfx_begin(C_HIGHLIGHT);
#ifdef GBA_SCENE_CLIP
            gfx_set_clip_y(10, 130); /* broader than the world band */
#endif
            sc64_fog_raster_cell(32, ys[k], sig,
                kind == 2 ? NULL : terrain, kind == 2 ? NULL : shade);
            for (int y = 0; y < 160; ++y) {
                int expected_row = y - ys[k];
                int allowed = y >= 12 && y < 120 && expected_row >= 0 && expected_row < 16;
#ifdef GBA_SCENE_CLIP
                allowed = allowed && y >= 10 && y < 130;
#endif
                for (int x = 32; x < 48; ++x) {
                    u8 expected = C_HIGHLIGHT;
                    if (allowed) {
                        if (kind == 2) expected = C_BLACK;
                        else if (kind == 0) expected = terrain[expected_row * 16 + x - 32];
                        else expected = reference_pixel(sig, (unsigned)(x - 32),
                                                        (unsigned)expected_row);
                    }
                    assert(gfx_test_pixel(x, y) == expected);
                }
            }
        }

    /* A narrower scene clip can reduce, but never expand, the hard world band. */
    gfx_begin(C_HIGHLIGHT);
#ifdef GBA_SCENE_CLIP
    gfx_set_clip_y(15, 18);
#endif
    sc64_fog_raster_cell(32, 10, (u16)visible, terrain, shade);
    for (int y = 0; y < 160; ++y) {
        int allowed = y >= 12 && y < 26;
#ifdef GBA_SCENE_CLIP
        allowed = allowed && y >= 15 && y < 18;
#endif
        for (int x = 32; x < 48; ++x)
            assert(gfx_test_pixel(x, y) == (allowed ? terrain[(y - 10) * 16 + x - 32]
                                                    : C_HIGHLIGHT));
    }

    /* Horizontal partial rows still use the shared clipped blitter. */
    gfx_begin(C_HIGHLIGHT);
    sc64_fog_raster_cell(-8, 20, (u16)visible, terrain, shade);
    for (int y = 20; y < 36; ++y)
        for (int x = 0; x < 8; ++x)
            assert(gfx_test_pixel(x, y) == terrain[(y - 20) * 16 + x + 8]);
    assert(gfx_test_pixel(8, 20) == C_HIGHLIGHT);

    /* Fully clipped cells must return before dereferencing terrain. */
    gfx_begin(C_HIGHLIGHT);
    sc64_fog_raster_cell(32, 120, (u16)visible, (const u8 *)1, shade);
    sc64_fog_raster_cell(32, -4, unknown_north, (const u8 *)1, shade);
#ifdef GBA_SCENE_CLIP
    gfx_set_clip_y(15, 18);
    sc64_fog_raster_cell(32, 20, unknown_north, (const u8 *)1, shade);
#endif
}

int main(void)
{
    for (unsigned i = 0; i < 256; ++i) {
        terrain[i] = (u8)(1u + (i * 37u) % 200u);
        shade[i] = (u8)(255u - i);
    }
    test_signature_encoding();
    gfx_begin(C_BLACK);
    test_all_neighborhoods();
    test_unknown_never_reads_terrain();
    test_world_anchor_and_clipping();
    puts("fog signatures/raster: all 19683 neighborhoods passed");
    return 0;
}
