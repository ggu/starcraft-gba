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
#ifdef SC64_TERRAN_BASE_BUILDING_WORDS_EXPECTED
    assert(count == 360 && sc64_building_word_sources[0] == sc64_race_building_art_sprite(106) &&
           sc64_building_word_sources[1] == sc64_race_building_art_sprite(111) &&
           sc64_building_word_sources[2] == sc64_race_building_art_sprite(109));
#elif defined(SC64_P10_TERRAN_BUILDING_WORDS_EXPECTED)
    assert(count == 720 && sc64_building_word_sources[0] == &sc64_race_sprite_hatchery &&
           sc64_building_word_sources[1] == &sc64_race_sprite_overmind_with_shell &&
           sc64_building_word_sources[2] == &sc64_race_sprite_nexus &&
           sc64_building_word_sources[3] == sc64_race_building_art_sprite(106) &&
           sc64_building_word_sources[4] == sc64_race_building_art_sprite(111) &&
           sc64_building_word_sources[5] == sc64_race_building_art_sprite(109));
#elif defined(SC64_HATCHERY_WORDS_EXPECTED)
    assert(count == 120 && sc64_building_word_sources[0] == &sc64_race_sprite_hatchery);
#elif defined(SC64_HATCHERY_OVERMIND_WORDS_EXPECTED)
    assert(count == 240 && sc64_building_word_sources[0] == &sc64_race_sprite_hatchery &&
           sc64_building_word_sources[1] == &sc64_race_sprite_overmind_with_shell);
#elif defined(SC64_HATCHERY_OVERMIND_NEXUS_WORDS_EXPECTED)
    assert(count == 360 && sc64_building_word_sources[0] == &sc64_race_sprite_hatchery &&
           sc64_building_word_sources[1] == &sc64_race_sprite_overmind_with_shell &&
           sc64_building_word_sources[2] == &sc64_race_sprite_nexus);
#elif defined(SC64_Z10_TERRAN_BUILDING_WORDS_EXPECTED)
    assert(count == 2160 && sc64_building_word_sources[15] == sc64_race_building_art_sprite(106) &&
           sc64_building_word_sources[16] == sc64_race_building_art_sprite(111) &&
           sc64_building_word_sources[17] == sc64_race_building_art_sprite(109));
#else
    assert(count == 1800);
#endif
    const Sc64SpriteFrame *fallback = sc64_race_building_art_sprite(142);
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
