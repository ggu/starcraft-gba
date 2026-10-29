/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Completed custom-game source building art, independently composited. */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/game.c"

#if !defined(SC64_RACES) || !defined(SC64_HAS_SPRITES) || !defined(GBA_SCENE_CLIP)
#error "Use a canonical race profile with source art and scene clipping"
#endif

static unsigned char expected[240 * 160];

static unsigned char background(int x, int y)
{
    return (unsigned char)(1 + ((x * 11 + y * 17 + (x ^ y)) % 251));
}

static const Sc64SpriteFrame *source_frame(unsigned raw)
{
    if (raw == 120 || raw == 172) return sc64_race_building_sprite((unsigned short)raw, 0);
    return sc64_race_unit_sprite((unsigned short)raw);
}

static void check(unsigned raw, int x, int y, int clip_top, int clip_bottom)
{
    const Sc64SpriteFrame *s = source_frame(raw);
    int type = sc64_race_building_from_raw((int)raw);
    int count = 0;
    assert(type >= 0);
    if (!s || !s->pixels || !s->w || !s->h) {
        fprintf(stderr, "missing native source frame raw=%u\n", raw);
        assert(s && s->pixels && s->w && s->h);
    }
    assert(sc64_race_building_art_sprite((int)raw) == s);
    assert((int)s->offset_x >= 0 && (int)s->offset_y >= 0);
    assert((unsigned)s->offset_x + s->w <= s->canvas_w);
    assert((unsigned)s->offset_y + s->h <= s->canvas_h);

    gfx_set_clip_y(0, 160);
    gfx_begin(0);
    for (int py = 0; py < 160; ++py) for (int px = 0; px < 240; ++px) {
        unsigned char c = background(px, py);
        expected[py * 240 + px] = c;
        gfx_pixel(px, py, c);
    }
    int left = x - (int)s->canvas_w / 2 + s->offset_x;
    int top = y - (int)s->canvas_h / 2 + s->offset_y;
    for (int sy = 0; sy < s->h; ++sy) for (int sx = 0; sx < s->w; ++sx) {
        int dx = left + sx, dy = top + sy;
        unsigned char c = s->pixels[sy * s->w + sx];
        if (c && dx >= 0 && dx < 240 && dy >= clip_top && dy < clip_bottom) {
            expected[dy * 240 + dx] = c;
            ++count;
        }
    }
    assert(count > 0);
    Building *q = &b[0];
    memset(q, 0, sizeof(*q));
    q->alive = 1; q->owner = SIDE; q->type = type; q->raw_type = (int)raw;
    q->x = x; q->y = y - 12; q->hp = q->maxhp = 100;
    q->under_construction = 0;
    camx = camy = 0;
    gfx_set_clip_y(clip_top, clip_bottom);
    draw_building_art(q);
    for (int py = 0; py < 160; ++py) for (int px = 0; px < 240; ++px) {
        unsigned char got = gfx_test_pixel(px, py), want = expected[py * 240 + px];
        if (got != want) {
            fprintf(stderr, "raw=%u anchor=(%d,%d) clip=%d..%d pixel=(%d,%d) expected=%u got=%u\n",
                    raw, x, y, clip_top, clip_bottom, px, py, want, got);
            assert(got == want);
        }
    }
    gfx_end();
}

int main(void)
{
    static const unsigned ids[] = {113, 120, 167, 172};
    static const int positions[][4] = {
        {120, 70, 0, 160}, {2, 16, 12, 128},
        {238, 120, 12, 128}, {121, 124, 12, 128},
    };
    gba_init(); game_init(); race_mode = 1; camx = camy = 0;
    for (unsigned i = 0; i < sizeof(ids) / sizeof(ids[0]); ++i)
        for (unsigned j = 0; j < sizeof(positions) / sizeof(positions[0]); ++j)
            check(ids[i], positions[j][0], positions[j][1], positions[j][2], positions[j][3]);
    puts("PASS Factory, Machine Shop, Stargate and Shield Battery source art and scene clipping");
    return 0;
}
