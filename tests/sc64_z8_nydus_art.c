/* Z8 source-art binding and actual completed-building draw. The generated
 * assets remain private; this fixture commits no extracted pixel data. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

#if !defined(SC64_Z8_RUNTIME) || !defined(SC64_Z8_BUILDING_SPRITE_SET_COMPLETE)
#error "Z8 Nydus source sprite bank is required for this fixture"
#endif

int main(int argc, char **argv)
{
    Building q;
    const Sc64SpriteFrame *sprite = sc64_race_building_sprite(134, 0);
    unsigned opaque = 0;
    assert(sprite && sprite == sc64_race_building_art_sprite(134));
    assert(sprite->canvas_w == 64 && sprite->canvas_h == 64);
    assert(sprite->offset_x == 14 && sprite->offset_y == 11);
    assert(sprite->w == 36 && sprite->h == 36);
    for (unsigned i = 0; i < 5; ++i) assert(sc64_race_building_sprite(134, i));
    assert(!sc64_race_building_sprite(134, 5));
#ifdef SC64_CONSTRUCTION_SOURCE_ART
    for (unsigned stage = 0; stage < 3; ++stage) {
        const SC64ConstructionSourceStage *source = sc64_construction_stage(134, stage);
        assert(source && source->animation && source->layer_count);
        assert(sc64_construction_image_frame(source->image_id, 0));
    }
#else
#error "Z8 construction source art is required for this fixture"
#endif
    gba_init(); game_init();
    memset(&q, 0, sizeof(q)); q.alive = 1; q.type = B_NYDUS_CANAL;
    q.raw_type = 134; q.owner = SIDE; q.x = 96; q.y = 60;
    camx = camy = 0;
    gfx_begin(C_BLACK); draw_building_art(&q);
    for (unsigned y = 0; y < sprite->h; ++y)
        for (unsigned x = 0; x < sprite->w; ++x) {
            unsigned color = sprite->pixels[y * sprite->w + x];
            if (!color) continue;
            assert(gfx_test_pixel(96 - sprite->canvas_w / 2 + sprite->offset_x + x,
                                  72 - sprite->canvas_h / 2 + sprite->offset_y + y) == color);
            ++opaque;
        }
    assert(opaque > 0);
    if (argc == 2) { gfx_end(); gfx_dump_ppm(argv[1]); }
    printf("Z8 raw 134 source art: %u completed sprite pixels, 3 construction stages\n", opaque);
    return 0;
}
