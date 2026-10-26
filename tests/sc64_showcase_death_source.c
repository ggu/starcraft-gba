/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Actual Siege Line casualties must use imported Zergling death frames. */
#include "../starcraft/game.c"
#include <assert.h>
#include <stdio.h>

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
            if (layer->under != under ||
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
        if (!layer->under || !layer->remnant ||
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

int main(int argc, char **argv)
{
    static const int captures[] = {0, 30, 60, 90, 120, 180, 240, 300, 360, 420};
    unsigned saw_deaths = 0, saw_remnant = 0, compared_remnant_mask = 0;
    unsigned capture = 0;
    assert(argc == 1 || argc == 2);
    gba_init();
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
