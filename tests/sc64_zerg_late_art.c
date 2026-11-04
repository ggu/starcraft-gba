/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Real prepared source sprites and authenticated late map/create coverage.
 * Imported pixels and scenario data remain in the ignored prepared inputs. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

#if !defined(SC64_Z10_RUNTIME) || !defined(SC64_ZERG_LATE_ART_COMPLETE)
#error "The complete late-Zerg sprite bank and Z10 flags are required"
#endif

static void require_art(unsigned raw)
{
    /* 101 is a source-invisible map revealer; 214 is a start marker. */
    if (raw == 101 || raw == 214) return;
    const Sc64SpriteFrame *sprite = sc64_p8_unit_sprite_for_raw((u16)raw);
    if (!sprite) sprite = sc64_race_building_sprite((u16)raw, 0);
    if (!sprite) fprintf(stderr, "missing visible late-Zerg raw %u\n", raw);
    assert(sprite && sprite->pixels && sprite->w && sprite->h);
}

static void check_map_and_creates(unsigned mission, const Sc64Z8Action *actions, unsigned count)
{
    const Sc64RaceMap *map = &sc64_race_maps[mission];
    for (unsigned i=0; i<map->unit_count; ++i) require_art(map->units[i].source_type);
    for (unsigned i=0; i<count; ++i)
        if (actions[i].opcode == 11) require_art(actions[i].unit);
}

static unsigned check_pixels(const Sc64SpriteFrame *s, int x, int y)
{
    unsigned seen=0;
    int sx=x-s->canvas_w/2+s->offset_x, sy=y-s->canvas_h/2+s->offset_y;
    for (int j=0; j<s->h; ++j) for (int i=0; i<s->w; ++i) {
        int px=sx+i, py=sy+j; unsigned color=s->pixels[j*s->w+i];
        if (!color || px<0 || px>=240 || py<12 || py>=120) continue;
        assert(gfx_test_pixel(px,py)==color); ++seen;
    }
    assert(seen>0); return seen;
}

int main(void)
{
    const Sc64SpriteFrame *formation=sc64_race_unit_sprite(173);
    const Sc64SpriteFrame *temple=sc64_race_unit_sprite(174);
    const Sc64SpriteFrame *crystal=sc64_race_unit_sprite(219);
    assert(formation && temple && crystal);
    assert(formation==sc64_race_building_art_sprite(173));
    assert(temple==sc64_race_building_art_sprite(174));
    assert(crystal==sc64_p8_unit_sprite_for_raw(219));
    assert(crystal!=sc64_race_unit_sprite(216));
    check_map_and_creates(SC64_Z9_GLOBAL_ID,sc64_z9_program_actions,SC64_Z9_PROGRAM_ACTION_COUNT);
    check_map_and_creates(SC64_Z10_GLOBAL_ID,sc64_z10_program_actions,SC64_Z10_PROGRAM_ACTION_COUNT);
    assert(!sc64_race_unit_sprite(101));
    gba_init(); game_init(); camx=camy=0;
    for (unsigned raw=173;raw<=174;++raw) {
        Building q; memset(&q,0,sizeof(q));
        q.alive=1; q.owner=SIDE; q.raw_type=(int)raw;
        int type=sc64_race_building_from_raw((int)raw); assert(type>=0);
        q.type=type;
        q.x=120; q.y=60;
        gfx_begin(C_BLACK); gfx_set_clip_y(12,120); draw_building_art(&q);
        check_pixels(raw==173?formation:temple,120,72);
        /* Visible art at an anchor outside the ordinary +/-20 envelope. */
        q.x=55; camx=80;
        gfx_begin(C_BLACK); gfx_set_clip_y(12,120); draw_building_art(&q);
        check_pixels(raw==173?formation:temple,-25,72);
        camx=0;
    }
    /* The static cache must see edge entry/exit for the larger Temple. */
    memset(b,0,sizeof(b)); b[0].alive=1; b[0].owner=SIDE; b[0].raw_type=174;
    b[0].type=sc64_race_building_from_raw(174); b[0].y=60;
    camx=80; b[0].x=55; /* screen anchor -25, still partly visible */
    unsigned key=sc64_static_objects_key(); assert(sc64_static_objects_key()==key);
    b[0].x=20; assert(sc64_static_objects_key()!=key); /* anchor -60 */
    key=sc64_static_objects_key(); b[0].x=55; assert(sc64_static_objects_key()!=key);
    camx=0;
    memset(u,0,sizeof(u)); u[0].alive=1; u[0].owner=SIDE;
    u[0].type=U_CHRYSALIS; u[0].raw_type=219; u[0].x=120; u[0].y=60;
    gfx_begin(C_BLACK); gfx_set_clip_y(12,120); draw_unit(&u[0]);
    check_pixels(crystal,120,72);
    puts("Z9/Z10 map and created-actor source art and clipped draws pass");
    return 0;
}
