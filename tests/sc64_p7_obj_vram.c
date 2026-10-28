/* SPDX-License-Identifier: GPL-3.0-or-later */
/* OBJ tile VRAM ownership in the canonical P7 profile: the interceptor
 * projectile arena and the VRAM terrain cache share the 16 KiB at 0x06014000.
 * The host backs that area with one array (gba_vram_scratch), so this fixture
 * exercises the real shared address space: capturing terrain must leave every
 * arena word intact, projectile writes must leave every cached pixel intact,
 * and a frame drawn from the cache must match an uncached frame. A deliberate
 * arena placed over the cache rows shows the fixture detects an overlap. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

enum { FB_WIDTH = 240, FB_HEIGHT = 160, FB_SIZE = FB_WIDTH * FB_HEIGHT };
enum { CACHE_VRAM_BYTES = 240 * SC64_TERRAIN_CACHE_VRAM_ROWS };
static u8 reference_pixels[FB_SIZE], candidate_pixels[FB_SIZE];
static u8 cache_snapshot[CACHE_VRAM_BYTES];
static u16 arena_snapshot[SC64_ARENA_WORDS];
static u8 packet_before[SC64_CODEC_BYTES], packet_after[SC64_CODEC_BYTES];

static void
capture(u8 *out)
{
    for (int y = 0; y < FB_HEIGHT; ++y)
        for (int x = 0; x < FB_WIDTH; ++x)
            out[y * FB_WIDTH + x] = gfx_test_pixel(x, y);
}

static void
setup_profile(void)
{
    game_init();
    race_mode = 0;
    race_human = R_PROTOSS;
    race_selected_mission = SC64_P7_GLOBAL_ID;
    init_state();
    assert(screen == SCREEN_PLAY && !mission_unsupported_count);
    assert(race_p7_active() && !race_p7_projectile_fault);
    mode = MODE_NORMAL; menu_open = 0; box_state = 0; selected_building = -1;
    race_campaign.phase = SC64_RACE_PHASE_ACTIVE;
    race_campaign.message = 0; race_campaign.message_ticks = 0;
    toast_timer = 0; cx = cy = 512; camx = camy = 0;
}

static void
reset_render_caches(void)
{
    sc64_terrain_cache_valid = 0;
    sc64_terrain_cache_reused_base = 0;
    sc64_hud_valid = 0;
}

/* One drawn frame through the production path; the first after a reset
 * captures the terrain band into the cache. */
static void
draw_frame(void)
{
    gfx_begin(C_BLACK);
    draw_map();
    draw_hud();
    gfx_end();
}

static void
poison_both_pages(void)
{
    for (int phase = 0; phase < 2; ++phase) {
        gfx_begin(C_BLACK);
        for (int y = 0; y < FB_HEIGHT; ++y)
            for (int x = 0; x < FB_WIDTH; ++x)
                gfx_pixel(x, y, ((x ^ y) & 1) ? (phase ? C_CYAN : C_ORANGE)
                                              : (phase ? C_RED : C_BLUE));
        gfx_end();
    }
}

/* Valid in-flight rows through the same launch helper the adapter uses. */
static int
fill_pool(Sc64Arena *arena, unsigned count, unsigned salt)
{
    for (unsigned n = 0; n < count; ++n) {
        Sc64ProjectileRow row;
        u8 id, native_index;
        if (!sc64_interceptor_projectile_launch(&row,
                (s32)(200 + n + salt) * 512, (s32)(150 + 2 * n) * 512,
                (s16)(400 + 3 * n), (s16)(90 + n), (u8)(n * 7 + salt),
                (u16)(5 + n), (u8)(3 + n % 20), (u8)(n & 7)))
            return 0;
        if (!sc64_pool_acquire(arena, &id, &native_index)) return 0;
        if (!sc64_pool_set_row(arena, id, &row)) return 0;
    }
    return sc64_pool_validate(arena);
}

static void
snapshot_arena(const Sc64Arena *arena, u16 *out)
{
    for (size_t i = 0; i < SC64_ARENA_WORDS; ++i) out[i] = arena->words[i];
}

static int
arena_matches(const Sc64Arena *arena, const u16 *snapshot)
{
    for (size_t i = 0; i < SC64_ARENA_WORDS; ++i)
        if (arena->words[i] != snapshot[i]) return 0;
    return 1;
}

static int
test_layout_is_disjoint(void)
{
    const u8 *obj = GBA_VRAM_SCRATCH;
    const u8 *arena_lo = (const u8 *)(const void *)race_p7_projectiles.words;
    const u8 *arena_hi = arena_lo + SC64_ARENA_WORDS * 2u;
    const u8 *cache_lo = sc64_terrain_cache_pixels_vram;
    const u8 *cache_hi = cache_lo + CACHE_VRAM_BYTES;
    int failures = 0;
    /* Both owners sit inside the one shared host backing of the OBJ VRAM. */
    if (arena_lo != obj) { fprintf(stderr, "arena is not at the OBJ VRAM base\n"); ++failures; }
    if (arena_hi > cache_lo) { fprintf(stderr, "arena overlaps the terrain cache\n"); ++failures; }
    if (cache_hi > obj + GBA_VRAM_SCRATCH_BYTES) { fprintf(stderr, "terrain cache exceeds OBJ VRAM\n"); ++failures; }
    /* The hardware addresses derive from the same offsets. */
    if (0x06014000u + SC64_OBJ_VRAM_PROJECTILE_BYTES > 0x06014000u + SC64_OBJ_VRAM_CACHE_OFFSET ||
        0x06014000u + SC64_OBJ_VRAM_CACHE_OFFSET + CACHE_VRAM_BYTES > 0x06018000u) {
        fprintf(stderr, "ARM ranges overlap or exceed 0x06018000\n"); ++failures;
    }
    printf("layout: arena [0x06014000, 0x%08x), cache [0x%08x, 0x%08x), %d VRAM rows\n",
           0x06014000u + (unsigned)SC64_OBJ_VRAM_PROJECTILE_BYTES,
           0x06014000u + (unsigned)SC64_OBJ_VRAM_CACHE_OFFSET,
           0x06014000u + (unsigned)SC64_OBJ_VRAM_CACHE_OFFSET + (unsigned)CACHE_VRAM_BYTES,
           SC64_TERRAIN_CACHE_VRAM_ROWS);
    return failures;
}

static int
test_terrain_capture_keeps_projectiles(void)
{
    int failures = 0;
    setup_profile();
    assert(fill_pool(&race_p7_projectiles, 37, 1));
    assert(sc64_codec_encode(&race_p7_projectiles, packet_before, SC64_CODEC_BYTES));
    snapshot_arena(&race_p7_projectiles, arena_snapshot);
    reset_render_caches();
    draw_frame();
    if (!sc64_terrain_cache_valid) { fprintf(stderr, "terrain capture did not happen\n"); ++failures; }
    if (!arena_matches(&race_p7_projectiles, arena_snapshot)) {
        fprintf(stderr, "terrain capture changed projectile arena words\n"); ++failures;
    }
    if (!sc64_pool_validate(&race_p7_projectiles) ||
        !sc64_codec_encode(&race_p7_projectiles, packet_after, SC64_CODEC_BYTES) ||
        memcmp(packet_before, packet_after, SC64_CODEC_BYTES) != 0) {
        fprintf(stderr, "projectile save packet changed across terrain capture\n"); ++failures;
    }
    /* The capture really landed in the shared area next to the arena: the
     * cached rows hold terrain, not the zero fill of a fresh host array. */
    {
        unsigned colors = 0;
        for (int i = 0; i < CACHE_VRAM_BYTES; ++i)
            if (sc64_terrain_cache_pixels_vram[i] != sc64_terrain_cache_pixels_vram[0]) {
                colors = 1; break;
            }
        if (!colors) { fprintf(stderr, "cached VRAM rows hold no terrain\n"); ++failures; }
    }
    printf("terrain capture keeps projectiles: %s\n", failures ? "FAIL" : "ok");
    return failures;
}

static int
test_projectile_writes_keep_terrain(void)
{
    int failures = 0;
    setup_profile();
    reset_render_caches();
    draw_frame();
    assert(sc64_terrain_cache_valid);
    memcpy(cache_snapshot, sc64_terrain_cache_pixels_vram, CACHE_VRAM_BYTES);
    /* Write every arena word: a fresh pool, a full pool, retirements, a
     * decode from a packet and a second full pool with different rows. */
    assert(sc64_pool_init(&race_p7_projectiles));
    assert(fill_pool(&race_p7_projectiles, SC64_POOL_CAPACITY, 2));
    for (u8 id = 1; id <= 40; ++id) assert(sc64_pool_retire(&race_p7_projectiles, id));
    assert(sc64_codec_encode(&race_p7_projectiles, packet_before, SC64_CODEC_BYTES));
    assert(sc64_pool_init(&race_p7_projectiles));
    assert(sc64_codec_decode(&race_p7_projectiles, packet_before, SC64_CODEC_BYTES));
    assert(sc64_pool_init(&race_p7_projectiles));
    assert(fill_pool(&race_p7_projectiles, SC64_POOL_CAPACITY, 3));
    if (memcmp(cache_snapshot, sc64_terrain_cache_pixels_vram, CACHE_VRAM_BYTES) != 0) {
        fprintf(stderr, "projectile writes changed cached terrain pixels\n"); ++failures;
    }
    /* The cached frame still equals an uncached one. */
    reset_render_caches();
    gfx_begin(C_BLACK); draw_map(); draw_hud(); race_p7_projectile_draw(); gfx_end();
    capture(reference_pixels);
    assert(fill_pool(&race_p7_projectiles, 0, 0));
    poison_both_pages();
    gfx_begin(C_BLACK); draw_map(); draw_hud(); race_p7_projectile_draw(); gfx_end();
    capture(candidate_pixels);
    if (memcmp(reference_pixels, candidate_pixels, FB_SIZE) != 0) {
        for (int i = 0; i < FB_SIZE; ++i)
            if (reference_pixels[i] != candidate_pixels[i]) {
                fprintf(stderr, "cached frame differs at (%d,%d)\n", i % FB_WIDTH, i / FB_WIDTH);
                break;
            }
        ++failures;
    }
    printf("projectile writes keep terrain: %s\n", failures ? "FAIL" : "ok");
    return failures;
}

/* Sensitivity: an arena placed where the cache rows are, as the first
 * version of this cache placed the real one, is corrupted by one capture. */
static int
test_overlapping_arena_is_detected(void)
{
    Sc64Arena overlapping = {(volatile u16 *)(void *)sc64_terrain_cache_pixels_vram,
                             SC64_ARENA_WORDS};
    int failures = 0;
    setup_profile();
    assert(sc64_pool_init(&overlapping));
    assert(fill_pool(&overlapping, 37, 4));
    snapshot_arena(&overlapping, arena_snapshot);
    reset_render_caches();
    draw_frame();
    if (arena_matches(&overlapping, arena_snapshot)) {
        fprintf(stderr, "an arena over the cache rows survived a capture; fixture is blind\n");
        ++failures;
    }
    printf("overlapping arena is detected: %s\n", failures ? "FAIL" : "ok");
    /* Leave the real arena usable for any later fixture. */
    setup_profile();
    return failures;
}

int
main(void)
{
    int failures = 0;
    failures += test_layout_is_disjoint();
    failures += test_terrain_capture_keeps_projectiles();
    failures += test_projectile_writes_keep_terrain();
    failures += test_overlapping_arena_is_detected();
    if (failures) {
        fprintf(stderr, "%d OBJ VRAM ownership failure(s)\n", failures);
        return 1;
    }
    puts("P7 OBJ VRAM ownership: ok");
    return 0;
}
