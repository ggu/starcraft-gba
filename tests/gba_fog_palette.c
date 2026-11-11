/* Asset-free oracle for the RGB555 fog shade palette mapper. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../common/gba_fog_palette.h"
#ifdef SC64_SOFT_FOG
#include "../starcraft/sc64_obj_vram.h"
#endif

static u16 palette[256];
static u8 lookup[256];

static unsigned red(u16 color) { return color & 31u; }
static unsigned green(u16 color) { return (color >> 5) & 31u; }
static unsigned blue(u16 color) { return (color >> 10) & 31u; }
static unsigned luma(u16 color)
{
    return 3u * red(color) + 6u * green(color) + blue(color);
}
static u16 pack(unsigned r, unsigned g, unsigned b)
{
    return (u16)((r & 31u) | ((g & 31u) << 5) | ((b & 31u) << 10));
}

/* Independent exhaustive oracle: no decoded palette caches and no helper
 * calls. It scans every candidate in ascending index order. */
static u8 oracle(unsigned src)
{
    unsigned sr = red(palette[src]), sg = green(palette[src]), sb = blue(palette[src]);
    unsigned source_luma = 3u * sr + 6u * sg + sb;
    unsigned tr = sr * 3u / 5u, tg = sg * 3u / 5u, tb = sb * 3u / 5u;
    unsigned best = src, best_distance = ~0u, darkest = ~0u;
    int target_black = !(tr | tg | tb);

    for (unsigned candidate = 0; candidate < 256; ++candidate) {
        unsigned cr = red(palette[candidate]), cg = green(palette[candidate]);
        unsigned cb = blue(palette[candidate]);
        unsigned candidate_luma = 3u * cr + 6u * cg + cb;
        if (candidate_luma > source_luma) continue;
        if (target_black) {
            if (candidate_luma < darkest) {
                darkest = candidate_luma;
                best = candidate;
            }
        } else {
            int dr = (int)cr - (int)tr;
            int dg = (int)cg - (int)tg;
            int db = (int)cb - (int)tb;
            unsigned distance = (unsigned)(dr * dr + dg * dg + db * db);
            if (distance < best_distance) {
                best_distance = distance;
                best = candidate;
            }
        }
    }
    return (u8)best;
}

static void check_palette(void)
{
    gba_fog_palette_make(palette, lookup);
    for (unsigned src = 0; src < 256; ++src) {
        assert(lookup[src] == oracle(src));
        assert(luma(palette[lookup[src]]) <= luma(palette[src]));
    }
}

static void test_grayscale_duplicates(void)
{
    for (unsigned i = 0; i < 256; ++i) {
        unsigned gray = (i * 7u + i / 9u) & 31u;
        palette[i] = pack(gray, gray, gray);
    }
    check_palette();
}

static void test_color_palette(void)
{
    for (unsigned i = 0; i < 256; ++i) {
        unsigned r = (i * 13u + i / 3u) & 31u;
        unsigned g = (i * 7u + i / 5u + 11u) & 31u;
        unsigned b = (i * 19u + i / 7u + 3u) & 31u;
        palette[i] = (u16)(pack(r, g, b) | ((i & 1u) ? 0x8000u : 0u));
    }
    check_palette();
}

static void test_ties_and_black(void)
{
    for (unsigned i = 0; i < 256; ++i) palette[i] = pack(31, 31, 31);
    palette[0] = pack(0, 0, 0);
    palette[1] = pack(17, 19, 18);
    palette[2] = pack(19, 17, 18); /* same squared RGB distance from 18,18,18 */
    palette[3] = pack(0, 0, 0);
    palette[255] = pack(31, 31, 31);
    check_palette();
    assert(lookup[255] == 1); /* equal-distance tie selects the lower index */
    assert(lookup[0] == 0);   /* black stays at the darkest available index */
}

static void test_black_only_palette(void)
{
    memset(palette, 0, sizeof(palette));
    check_palette();
    for (unsigned i = 0; i < 256; ++i) assert(lookup[i] == 0);
}

#ifdef SC64_SOFT_FOG
static u8 saved_lut[256];

static u8 scratch_canary(unsigned offset)
{
    return (u8)(offset * 37u + 0x53u);
}

static volatile u8 *fog_lut_bytes(void)
{
    return (volatile u8 *)(void *)SC64_OBJ_VRAM_FOG;
}

static void fill_scratch_canary(void)
{
    for (unsigned i = 0; i < GBA_VRAM_SCRATCH_BYTES; ++i)
        GBA_VRAM_SCRATCH[i] = scratch_canary(i);
}

static void check_runtime_lut(void)
{
    volatile u8 *runtime = fog_lut_bytes();
    for (unsigned i = 0; i < 256; ++i) {
        assert(runtime[i] == oracle(i));
        assert(luma(palette[runtime[i]]) <= luma(palette[i]));
    }
}

static void save_runtime_lut(void)
{
    volatile u8 *runtime = fog_lut_bytes();
    for (unsigned i = 0; i < 256; ++i) saved_lut[i] = runtime[i];
}

static void assert_runtime_lut_saved(void)
{
    volatile u8 *runtime = fog_lut_bytes();
    for (unsigned i = 0; i < 256; ++i) assert(runtime[i] == saved_lut[i]);
}

static void check_scratch_canaries(void)
{
    unsigned fog_start = SC64_OBJ_VRAM_FOG_OFFSET;
    unsigned fog_end = fog_start + SC64_OBJ_VRAM_FOG_BYTES;
    for (unsigned i = 0; i < fog_start; ++i)
        assert(GBA_VRAM_SCRATCH[i] == scratch_canary(i));
    for (unsigned i = fog_end; i < GBA_VRAM_SCRATCH_BYTES; ++i)
        assert(GBA_VRAM_SCRATCH[i] == scratch_canary(i));
}

static void simulate_other_vram_owners(void)
{
    /* Projectile state uses halfword writes in the optional prefix arena. */
    volatile u16 *projectiles = SC64_OBJ_VRAM_PROJECTILES;
    for (unsigned i = 0; i < SC64_OBJ_VRAM_PROJECTILE_BYTES / 2u; ++i)
        projectiles[i] = (u16)(0x9e37u + i);
    /* Terrain cache rows own the tail after the fog slot. */
    for (unsigned i = SC64_OBJ_VRAM_CACHE_OFFSET; i < GBA_VRAM_SCRATCH_BYTES; ++i)
        GBA_VRAM_SCRATCH[i] = (u8)(0xc3u ^ i);
}

static void test_runtime_vram_and_palette(void)
{
    const unsigned palettes = 3;

    gba_init();
    for (unsigned variant = 0; variant < palettes; ++variant) {
        for (unsigned i = 0; i < 256; ++i) {
            if (variant == 0) {
                unsigned gray = (i * 3u + i / 7u) & 31u;
                palette[i] = pack(gray, gray, gray);
            } else if (variant == 1) {
                palette[i] = pack((i * 11u + 2u) & 31u,
                                  (i * 17u + i / 3u) & 31u,
                                  (i * 23u + i / 5u + 7u) & 31u);
            } else {
                palette[i] = (u16)(pack((i * 5u + 1u) & 31u,
                                       (i * 9u + 13u) & 31u,
                                       (i * 27u + 3u) & 31u) |
                                   ((i & 1u) ? 0x8000u : 0u));
            }
        }

        gfx_set_palette(0, 256, palette);
        fill_scratch_canary();
        gfx_fog_palette_prepare(SC64_OBJ_VRAM_FOG);
        check_runtime_lut();
        check_scratch_canaries();
        save_runtime_lut();

        simulate_other_vram_owners();
        assert_runtime_lut_saved();

        gfx_begin(C_BLACK);
        gfx_end();
        assert_runtime_lut_saved();
        gba_init();
        assert_runtime_lut_saved();
    }

    /* Installing a different palette and preparing again must regenerate all
     * 256 bytes, while the audio and gap canaries stay intact. */
    for (unsigned i = 0; i < 256; ++i)
        palette[i] = pack((i + 4u) & 31u, (i * 3u + 8u) & 31u, (i * 7u + 2u) & 31u);
    gfx_set_palette(0, 256, palette);
    fill_scratch_canary();
    gfx_fog_palette_prepare(SC64_OBJ_VRAM_FOG);
    check_runtime_lut();
    volatile u8 *runtime = fog_lut_bytes();
    int changed = 0;
    for (unsigned i = 0; i < 256; ++i) changed |= runtime[i] != saved_lut[i];
    assert(changed);
    check_scratch_canaries();
}
#endif

int main(void)
{
    test_grayscale_duplicates();
    test_color_palette();
    test_ties_and_black();
    test_black_only_palette();
#ifdef SC64_SOFT_FOG
    test_runtime_vram_and_palette();
#endif
#ifdef SC64_SOFT_FOG
    puts("fog palette: helper and runtime VRAM checks passed");
#else
    puts("fog palette: all entries passed across four synthetic palettes");
#endif
    return 0;
}
