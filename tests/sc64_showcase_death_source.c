/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Actual Siege Line casualties must use imported Zergling death frames. */
#include "../starcraft/game.c"
#include <assert.h>
#include <stdio.h>

#if defined(SC64_DEATH_SOURCE_DEPTH_LAYERS) && !defined(SC64_DEATH_WORLD_DEPTH)
#error "Showcase must enable world depth for the current death bank"
#endif

static unsigned compare_source_pass(int under)
{
    static u8 expected[240 * 160];
    unsigned drawn = 0;

    gfx_begin(C_PANEL);
    for (int i = 0; i < SC64_DEATH_VFX_POOL; ++i) {
        const Sc64DeathVfx *e = &death_vfx[i];
        const Sc64DeathSource *art;
        u32 age;
        int x, y;
        if (!e->style || !e->source || sc64_death_vfx_expired(e, death_vfx_now())) continue;
        art = sc64_death_source(e->raw);
        assert(sc64_death_source_valid(art));
        age = sc64_death_vfx_age_su(e->birth, death_vfx_now());
        x = e->x - camx;
        y = e->y - camy + 12;
        if (x < -64 || x > 304 || y < -52 || y > 172 || !currently_visible(e->x, e->y)) continue;
        for (unsigned j = 0; j < art->layer_count; ++j) {
            const Sc64DeathSourceLayer *layer = &art->layers[j];
            unsigned frame;
#ifdef SC64_DEATH_WORLD_DEPTH
            if ((layer->flags & SC64_DEATH_SOURCE_FIXED_LOW) ||
                (e->actor_layer_plus1 && (layer->flags &
                 (SC64_DEATH_SOURCE_ACTOR_SPRITE | SC64_DEATH_SOURCE_DETACHED_SPRITE)))) continue;
#endif
            if ((layer->flags & SC64_DEATH_SOURCE_UNDER) != under ||
                !sc64_death_source_layer_frame(layer, age, &frame)) continue;
            const Sc64SpriteFrame *sprite =
                sc64_death_image_frame(layer->image_id, (unsigned short)frame);
            if (sprite) death_vfx_blit(sprite, x + layer->x, y + layer->y);
        }
    }
    for (int y = 0; y < 160; ++y)
        for (int x = 0; x < 240; ++x)
            expected[y * 240 + x] = gfx_test_pixel(x, y);

    gfx_begin(C_PANEL);
    draw_death_vfx(under);
    for (int y = 0; y < 160; ++y) {
        for (int x = 0; x < 240; ++x) {
            int pixel = gfx_test_pixel(x, y);
            assert(pixel == expected[y * 240 + x]);
            if (pixel != C_PANEL) ++drawn;
        }
    }
    return drawn;
}

static unsigned active_zergling_deaths(void)
{
    unsigned count = 0;
    for (int i = 0; i < SC64_DEATH_VFX_POOL; ++i) {
        const Sc64DeathVfx *e = &death_vfx[i];
        if (!e->style || sc64_death_vfx_expired(e, death_vfx_now()) || e->raw != 37) continue;
        assert(e->source);
        const Sc64DeathSource *art = sc64_death_source(e->raw);
        assert(sc64_death_source_valid(art));
        ++count;
    }
    return count;
}

static int source_remnant_differs_from_fallback(const Sc64DeathVfx *e, u32 age)
{
    static u8 source_mask[240 * 160];
    const Sc64DeathSource *art = sc64_death_source(e->raw);
    int x = e->x - camx, y = e->y - camy + 12;
    unsigned pixels_different = 0;

    gfx_begin(C_PANEL);
    for (unsigned i = 0; i < art->layer_count; ++i) {
        const Sc64DeathSourceLayer *layer = &art->layers[i];
        unsigned frame;
        if (!(layer->flags & SC64_DEATH_SOURCE_UNDER) || !layer->remnant ||
            !sc64_death_source_layer_frame(layer, age, &frame)) continue;
        const Sc64SpriteFrame *sprite =
            sc64_death_image_frame(layer->image_id, (unsigned short)frame);
        if (sprite) death_vfx_blit(sprite, x + layer->x, y + layer->y);
    }
    for (int py = 0; py < 160; ++py)
        for (int px = 0; px < 240; ++px)
            source_mask[py * 240 + px] = gfx_test_pixel(px, py);
    gfx_begin(C_PANEL);
    sc64_death_vfx_draw_remnant(e, x, y, age);
    for (int py = 0; py < 160; ++py)
        for (int px = 0; px < 240; ++px)
            pixels_different += source_mask[py * 240 + px] != gfx_test_pixel(px, py);
    return pixels_different != 0;
}

#ifdef SC64_DEATH_WORLD_DEPTH
static void capture_death_overlap(u8 *pixels)
{
    gfx_begin(C_PANEL);
    gfx_set_clip_y(12, 120);
    draw_death_vfx(1);
    sc64_draw_depth_ordered_actors();
    draw_death_vfx(0);
    assert(gfx_read_band(12, 108, pixels));
    for (int y = 0; y < 160; ++y)
        if (y < 12 || y >= 120)
            for (int x = 0; x < 240; ++x) assert(gfx_test_pixel(x, y) == C_PANEL);
}

static void test_death_depth(void)
{
    static _Alignas(4) u8 actor_pixels[240 * 108], effect_pixels[240 * 108], pair[240 * 108];
    static const struct {
        int y, dy, type, elapsed, layer, effect_in_front, victim_raw;
    } cases[] = {
        { 52, -3, U_TANK, 5, 4, 1, 37 },
        { 52,  3, U_TANK, 5, 4, 0, 37 },
        { 52,  0, U_WRAITH, 5, 4, 0, 37 },
        {  1,  0, U_WRAITH, 5, 4, 0, 37 },
        { 106, 0, U_WRAITH, 5, 4, 0, 37 },
        { 52,  0, U_TANK, 30, 1, 0, 37 },
        { 52, -3, U_TANK, 5, 4, 1, 7 },
        { 52,  3, U_TANK, 5, 4, 0, 7 },
        { 52,  0, U_WRAITH, 5, 4, 0, 7 },
        /* Each gas structure spawns a separate burst one tier above itself. */
        { 52,  3, U_TANK, 5, 5, 1, 110 },
        { 52,  0, U_WRAITH, 5, 5, 0, 110 },
        {  1,  0, U_WRAITH, 5, 5, 0, 110 },
        { 106, 0, U_WRAITH, 5, 5, 0, 110 },
        { 52,  3, U_TANK, 5, 5, 1, 149 },
        { 52,  0, U_WRAITH, 5, 5, 0, 157 },
        /* The Queen's attached image outlasts its detached sprite body. */
        { 52,  0, U_WRAITH, 5, 19, 1, 45 },
    };
    for (unsigned n = 0; n < sizeof(cases) / sizeof(cases[0]); ++n) {
        game_init();
        memset(u, 0, sizeof(u));
        memset(b, 0, sizeof(b));
        memset(m, 0, sizeof(m));
        memset(visible, 0xff, sizeof(visible));
        geyser_count = 0;
        camx = camy = 0;
        int y = cases[n].y;
        int raw = cases[n].victim_raw;
        int burst_image = 0;
        if (raw == 110 || raw == 149 || raw == 157) {
            int type = raw == 110 ? B_REFINERY : raw == 149 ? B_EXTRACTOR : B_ASSIMILATOR;
            int victim = spawn_build(SIDE, type, 96, y, 0);
            assert(victim >= 0);
            b[victim].raw_type = raw;
            death_vfx_scan();
            b[victim].hp = 0;
            b[victim].alive = 0;
            burst_image = raw == 110 ? 334 : raw == 149 ? 60 : 215;
        } else {
            int type = raw == 7 ? U_SCV : raw == 45 ? U_QUEEN : U_ZERGLING;
            int victim = spawn_unit(SIDE, type, 96, y);
            assert(victim >= 0);
            u[victim].raw_type = raw;
            death_vfx_scan();
            u[victim].hp = 0;
            u[victim].alive = 0;
            if (raw == 45) burst_image = 58;
        }
        death_vfx_scan();
        assert(death_vfx[0].source &&
               death_vfx[0].actor_layer_plus1 == (raw == 45 ? 19 : 5));
        frame += cases[n].elapsed;

        u32 keys[SC64_DEATH_SOURCE_DEPTH_LAYERS];
        unsigned size = 0;
        sc64_depth_push_deaths(keys, &size);
        assert(size == 1 && (keys[0] >> DEPTH_LAYER_BITS) == (unsigned)cases[n].layer);
        if (burst_image) {
            const Sc64DeathSource *art = sc64_death_source(raw);
            unsigned index = (keys[0] & DEPTH_SLOT_MASK) % SC64_DEATH_SOURCE_MAX_LAYERS;
            assert(art && index < art->layer_count);
            assert(art->layers[index].image_id == burst_image &&
                   (art->layers[index].flags & SC64_DEATH_SOURCE_DETACHED_SPRITE));
        }
        capture_death_overlap(effect_pixels);

        int actor = spawn_unit(SIDE, cases[n].type, 96, y + cases[n].dy);
        assert(actor >= 0);
        Sc64DeathVfx effect = death_vfx[0];
        death_vfx[0].style = 0;
        capture_death_overlap(actor_pixels);
        death_vfx[0] = effect;
        capture_death_overlap(pair);

        unsigned overlap = 0;
        for (unsigned i = 0; i < sizeof(pair); ++i) {
            if (actor_pixels[i] == C_PANEL || effect_pixels[i] == C_PANEL ||
                actor_pixels[i] == effect_pixels[i]) continue;
            assert(pair[i] == (cases[n].effect_in_front ? effect_pixels[i] : actor_pixels[i]));
            ++overlap;
        }
        assert(overlap && "death and live sprites must have distinguishable overlap pixels");
        printf("PASS source death raw %d layer %d overlap at Y%d dY%d, %u pixels\n",
               raw, cases[n].layer, y, cases[n].dy, overlap);
    }
}
#endif

int main(int argc, char **argv)
{
    static const int captures[] = {0, 30, 60, 90, 120, 180, 240, 300, 360, 420};
    unsigned saw_deaths = 0, saw_remnant = 0, compared_remnant_mask = 0;
    unsigned capture = 0;
    assert(argc == 1 || argc == 2);
    gba_init();
#ifdef SC64_DEATH_WORLD_DEPTH
    test_death_depth();
#endif
    game_init();
    sc64_showcase_start(1); /* Siege Line: six tanks against 32 Zerglings. */
    screen = SCREEN_PLAY;

    for (int step = 0; step <= 450; ++step) {
        /* Run the normal render path first so the showcase camera follows cx/cy. */
        game_render();
        unsigned active = active_zergling_deaths();
        if (active) saw_deaths += active;
        for (int i = 0; i < SC64_DEATH_VFX_POOL; ++i) {
            const Sc64DeathVfx *e = &death_vfx[i];
            if (!e->style || e->raw != 37 || sc64_death_vfx_expired(e, death_vfx_now())) continue;
            const Sc64DeathSource *art = sc64_death_source(e->raw);
            u32 age = sc64_death_vfx_age_su(e->birth, death_vfx_now());
            if (age >= art->remnant_start && age < art->total_su) {
                saw_remnant++;
                if (!compared_remnant_mask) {
                    compared_remnant_mask = source_remnant_differs_from_fallback(e, age);
                    if (!compared_remnant_mask) fprintf(stderr, "same/remnant no diff at frame=%d age=%u ext=%d\n", step, age, e->extent);
                }
            }
        }

        if (argc == 2 && capture < sizeof(captures) / sizeof(captures[0]) &&
            step == captures[capture]) {
            char path[512];
            snprintf(path, sizeof(path), "%s/siege-source-%03d.ppm", argv[1], step);
            gfx_dump_ppm(path);
            ++capture;
        }

        /* Independently check that the scene's actual source render is the
         * exact composite of its authored layer frames, including remnants. */
        unsigned under = compare_source_pass(1);
        unsigned over = compare_source_pass(0);
        (void)under;
        (void)over;
        game_update(0, 0);
    }

    if (argc == 2) assert(capture == sizeof(captures) / sizeof(captures[0]));
    assert(saw_deaths > 0 && saw_remnant > 0 && compared_remnant_mask);
    assert(active_zergling_deaths() == 0);
    printf("PASS Siege Line source deaths: observed %u active raw-37 entries, %u remnant frames, all expired by frame 450\n",
           saw_deaths, saw_remnant);
    return 0;
}
