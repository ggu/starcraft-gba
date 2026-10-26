/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Source death art: compiled layers are valid, real kills select them, and
 * the drawn pixels are the imported frames, clipped to the battlefield. */
#include "../common/gba.c"
#include "../starcraft/game.c"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

static void check_descriptor(int raw, const Sc64DeathSource *art)
{
    assert(sc64_death_source_valid(art));
    assert(art->burst_su <= art->total_su && art->remnant_start <= art->total_su);
    assert(art->total_su <= 128 + SC64_DEATH_VFX_RUBBLE_SU);
    for (unsigned i = 0; i < art->layer_count; ++i) {
        const Sc64DeathSourceLayer *l = &art->layers[i];
        unsigned frame;
        assert(l->end_su - l->start_su <= SC64_DEATH_VFX_RUBBLE_SU);
        for (u32 age = l->start_su; age < l->end_su; ++age) {
            assert(sc64_death_source_layer_frame(l, age, &frame));
            const Sc64SpriteFrame *s = sc64_death_image_frame(l->image_id, (unsigned short)frame);
            if (!s) fprintf(stderr, "raw %d layer %u image %u frame %u missing\n",
                            raw, i, l->image_id, frame);
            assert(s && s->w && s->h && s->w <= 256 && s->h <= 256);
        }
        assert(!sc64_death_source_layer_frame(l, l->end_su, &frame));
        if (l->start_su) assert(!sc64_death_source_layer_frame(l, l->start_su - 1u, &frame));
    }
}

static void setup_world(void)
{
    game_init();
    race_mode = 1; race_human = R_ZERG; race_bots = 1;
    race_skirmish_init();
    screen = SCREEN_PLAY;
    memset(visible, 0xff, sizeof(visible));
}

static int kill_and_observe(int type, int x, int y)
{
    death_vfx_scan();
    int victim = spawn_unit(ENEMY, type, x, y);
    assert(victim >= 0);
    death_vfx_scan();
    while (u[victim].alive) hit_target(victim, 500, U_ZERGLING, SIDE);
    death_vfx_scan();
    for (int i = 0; i < SC64_DEATH_VFX_POOL; ++i)
        if (!sc64_death_vfx_expired(&death_vfx[i], (u32)frame) &&
            death_vfx[i].x == x && death_vfx[i].y == y) return i;
    assert(!"death effect was not observed");
    return -1;
}

/* Draw one pass and require that it equals the clipped source blits. */
static void check_source_pass(const Sc64DeathVfx *e, int under, int *drawn)
{
    static u8 expected[240 * 160];
    const Sc64DeathSource *art = sc64_death_source(e->raw);
    u32 age = sc64_death_vfx_age_su(e->birth, (u32)frame);
    int x = e->x - camx, y = e->y - camy + 12;
    gfx_begin(C_PANEL);
    for (unsigned i = 0; i < art->layer_count; ++i) {
        const Sc64DeathSourceLayer *l = &art->layers[i];
        unsigned f;
        if (l->under != under || !sc64_death_source_layer_frame(l, age, &f)) continue;
        death_vfx_blit(sc64_death_image_frame(l->image_id, (unsigned short)f), x + l->x, y + l->y);
    }
    for (int py = 0; py < 160; ++py) for (int px = 0; px < 240; ++px)
        expected[py * 240 + px] = gfx_test_pixel(px, py);
    gfx_begin(C_PANEL);
    draw_death_vfx(under);
    for (int py = 0; py < 160; ++py) for (int px = 0; px < 240; ++px) {
        int p = gfx_test_pixel(px, py);
        assert(p == expected[py * 240 + px]);
        if (py < 12 || py > 119) assert(p == C_PANEL);
        else *drawn += p != C_PANEL;
    }
}

static void test_kill(int type, int expect_under, int expect_over)
{
    setup_world();
    int x = 0, y = 0;
    for (int i = 0; i < MAX_BUILDINGS; ++i) if (b[i].alive && b[i].owner == SIDE) { x = b[i].x + 40; y = b[i].y + 30; break; }
    int slot = kill_and_observe(type, x, y);
    const Sc64DeathVfx *e = &death_vfx[slot];
    const Sc64DeathSource *art = sc64_death_source(e->raw);
    assert(e->source && art && e->total_su == art->total_su && e->burst_su == art->burst_su);
    camx = x - 120; camy = y - 54;
    u32 born = (u32)frame;
    int under = 0, over = 0;
    while (sc64_death_vfx_age_su(born, (u32)frame) < e->total_su) {
        check_source_pass(e, 1, &under);
        check_source_pass(e, 0, &over);
        frame += sc64_death_vfx_age_su(born, (u32)frame) < 60 ? 1 : 37;
    }
    if ((expect_under != 0) != (under > 0) || (expect_over != 0) != (over > 0))
        fprintf(stderr, "type %d raw %u: under %d over %d\n", type, e->raw, under, over);
    assert(expect_under ? under > 0 : under == 0);
    assert(expect_over ? over > 0 : over == 0);
    /* Near the HUD the frames are clipped, never drawn into cached rows. */
    camx = x - 120; camy = y - 12 - 112;
    frame = (int)born;
    int clipped = 0;
    check_source_pass(e, 1, &clipped);
    check_source_pass(e, 0, &clipped);
}

int main(void)
{
    gba_init();
    unsigned actors = 0;
    for (int raw = 0; raw < 256; ++raw) {
        const Sc64DeathSource *art = sc64_death_source(raw);
        if (!art) continue;
        check_descriptor(raw, art); ++actors;
    }
    assert(actors == SC64_DEATH_SOURCE_ACTOR_COUNT && actors >= 100);
    /* Marine: collapse frames then a remnant (both beneath units). */
    test_kill(U_MARINE, 1, 0);
    /* Zergling: collapse frames and remains. */
    test_kill(U_ZERGLING, 1, 0);
    /* Dragoon: collapse and remains, as in the source. */
    test_kill(U_DRAGOON, 1, 0);
    /* Vulture and Probe: orange and blue explosion overlays above units. */
    test_kill(U_VULTURE, 0, 1);
    test_kill(U_PROBE, 0, 1);
    printf("PASS %u source death descriptors; Marine/Zergling/Dragoon collapse+remnant and Vulture/Probe overlays match imported frames\n",
           actors);
    return 0;
}
