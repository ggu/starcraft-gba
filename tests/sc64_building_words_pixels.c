/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/game.c"

static u8 reference[240 * 160] __attribute__((aligned(4)));
static u8 packed[240 * 160] __attribute__((aligned(4)));

static void fill(int top, int bottom)
{
    gfx_begin(0);
    gfx_set_clip_y(0, 160);
    gfx_rect(0, 0, 240, 160, 17);
    gfx_set_clip_y(top, bottom);
}

int main(void)
{
    static const int x[] = { -12, 0, 96, 120, 239, 252 };
    static const int y[] = { 4, 14, 65, 114, 157 };
    unsigned count = 0;
    gba_init();
    for (unsigned i = 0; i < sizeof(sc64_building_word_sources) /
                                 sizeof(sc64_building_word_sources[0]); ++i) {
        const Sc64SpriteFrame *s = sc64_building_word_sources[i];
        const GbaSpriteWordFrame *word = sc64_building_word_lookup(s);
        assert(word && word->width == s->w && word->height == s->h);
        for (unsigned a = 0; a < sizeof(x) / sizeof(x[0]); ++a)
            for (unsigned b_ = 0; b_ < sizeof(y) / sizeof(y[0]); ++b_)
                for (int phase = 0; phase < 4; ++phase) {
                    fill(12, 120);
                    draw_mission_sprite(s, x[a] + phase, y[b_]);
                    assert(gfx_read_band(0, 160, reference));
                    fill(12, 120);
                    draw_completed_building_sprite(s, x[a] + phase, y[b_]);
                    assert(gfx_read_band(0, 160, packed));
                    assert(!memcmp(reference, packed, sizeof(packed)));
                    ++count;
                }
    }
    assert(count == 1800);
    const Sc64SpriteFrame *fallback = sc64_race_building_sprite(142, 0);
    assert(fallback && sc64_building_word_lookup(fallback) == NULL);
    fill(12, 120);
    draw_mission_sprite(fallback, 120, 65);
    assert(gfx_read_band(0, 160, reference));
    fill(12, 120);
    draw_completed_building_sprite(fallback, 120, 65);
    assert(gfx_read_band(0, 160, packed));
    assert(!memcmp(reference, packed, sizeof(packed)));
    assert(sc64_building_word_lookup(NULL) == NULL);
    printf("building word pixel parity: %u cases\n", count);
    return 0;
}
