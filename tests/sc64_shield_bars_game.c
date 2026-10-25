/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Pixel checks for Protoss unit and building shield/health overlays. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

enum { FB_PIXELS = 240 * 160 };
static _Alignas(4) u8 full_shield_pixels[FB_PIXELS];
static _Alignas(4) u8 partial_shield_pixels[FB_PIXELS];
static _Alignas(4) u8 empty_shield_pixels[FB_PIXELS];

static void
fixture(void)
{
    game_init();
    race_mode=1; race_human=R_PROTOSS; race_bots=1; race_skirmish_map=0;
    race_skirmish_init();
    screen=SCREEN_PLAY;
    memset(u,0,sizeof(u));
    memset(b,0,sizeof(b));
    camx=camy=0;
    race_cols=race_rows=64;
    screen=SCREEN_PLAY;
}

static const Sc64SpriteFrame *
unit_sprite_for_test(const Unit *q)
{
#ifdef SC64_HAS_SPRITES
#ifdef SC64_RACES
    if(q->raw_type>=0)return sc64_race_unit_sprite(q->raw_type);
#endif
    return mission_unit_sprite(q->type);
#else
    (void)q;
    return 0;
#endif
}

static const Sc64SpriteFrame *
building_sprite_for_test(const Building *q)
{
#ifdef SC64_HAS_SPRITES
#ifdef SC64_RACES
    if(q->raw_type>=0)return sc64_race_unit_sprite(q->raw_type);
#endif
    return mission_building_sprite(q->type);
#else
    (void)q;
    return 0;
#endif
}

static int
unit_sprite_y_for_test(const Unit *q)
{
    int y=q->y-camy+12;
    const Sc64SpriteFrame *s=unit_sprite_for_test(q);
    return s?y-s->canvas_h/2+s->offset_y:y-4;
}

static int
building_sprite_y_for_test(const Building *q)
{
    int y=q->y-camy+12;
    const Sc64SpriteFrame *s=building_sprite_for_test(q);
    return s?y-s->canvas_h/2+s->offset_y:y-12;
}

static void
capture_unit(Unit *q,u8 *out)
{
    gfx_begin(C_BLACK);
    draw_unit(q);
    for(int y=0;y<160;++y)for(int x=0;x<240;++x)
        out[y*240+x]=gfx_test_pixel(x,y);
}

static void
capture_building(Building *q,u8 *out)
{
    gfx_begin(C_BLACK);
    draw_building_overlay(q);
    for(int y=0;y<160;++y)for(int x=0;x<240;++x)
        out[y*240+x]=gfx_test_pixel(x,y);
}

static int
is_blue_shield_color(u8 color)
{
    return color==C_BLUE || color==C_BLUE_DARK || color==C_CYAN;
}

static int
count_shield_row(const u8 *pixels,int x,int y,int width)
{
    int count=0;
    for(int dx=0;dx<width;++dx)
        count+=is_blue_shield_color(pixels[y*240+x+dx]);
    return count;
}

static void
assert_only_shield_rows_changed(const u8 *a,const u8 *z,int shield_y)
{
    for(int y=0;y<160;++y)if(y!=shield_y&&y!=shield_y+1)
        assert(memcmp(a+y*240,z+y*240,240)==0);
}

static void
test_unit_shield_bar_and_health_independence(int type,int raw_type,
                                              int hp,int maxhp,int maxshield)
{
    Unit *q=&u[0];
    fixture();
    q->alive=1; q->owner=SIDE; q->type=type;
    q->raw_type=raw_type; q->raw_owner=7; q->x=100; q->y=80;
    q->hp=hp; q->maxhp=maxhp; q->shield=maxshield;
    assert(sc64_actor_max_shields(q->type,0)==maxshield);
    int sprite_y=unit_sprite_y_for_test(q),shield_y=sprite_y-7,hp_y=sprite_y-4;
    capture_unit(q,full_shield_pixels);
    assert(count_shield_row(full_shield_pixels,94,shield_y,12)>0);
    assert(count_shield_row(full_shield_pixels,94,shield_y+1,12)>0);

    q->shield=maxshield/2;
    capture_unit(q,partial_shield_pixels);
    assert(memcmp(full_shield_pixels,partial_shield_pixels,FB_PIXELS)!=0);
    assert_only_shield_rows_changed(full_shield_pixels,partial_shield_pixels,shield_y);
    assert(count_shield_row(partial_shield_pixels,94,shield_y,12)>0);
    assert(memcmp(full_shield_pixels+hp_y*240,partial_shield_pixels+hp_y*240,240)==0);

    q->shield=0;
    capture_unit(q,empty_shield_pixels);
    assert_only_shield_rows_changed(full_shield_pixels,empty_shield_pixels,shield_y);
    assert(count_shield_row(empty_shield_pixels,94,shield_y,12)>0);
    assert(count_shield_row(empty_shield_pixels,94,shield_y+1,12)>0);
    assert(memcmp(partial_shield_pixels,empty_shield_pixels,FB_PIXELS)!=0);
}

static void
test_unshielded_unit_has_no_added_bar(void)
{
    Unit *q=&u[0];
    fixture();
    q->alive=1; q->owner=SIDE; q->type=U_MARINE;
    q->raw_type=sc64_race_raw_unit(U_MARINE); q->raw_owner=7; q->x=100; q->y=80;
    q->hp=q->maxhp=40; q->shield=0;
    assert(sc64_actor_max_shields(q->type,0)==0);
    int sprite_y=unit_sprite_y_for_test(q),shield_y=sprite_y-7;
    capture_unit(q,empty_shield_pixels);
    for(int x=94;x<106;++x)
        assert(empty_shield_pixels[shield_y*240+x]==C_BLACK &&
               empty_shield_pixels[(shield_y+1)*240+x]==C_BLACK);
}

static void
test_building_shield_bar_and_health_independence(void)
{
    Building *q=&b[0];
    fixture();
    q->alive=1; q->owner=SIDE; q->type=B_PYLON;
    q->raw_type=156; q->raw_owner=7; q->x=100; q->y=80;
    q->hp=q->maxhp=300; q->shield=300;
    assert(sc64_actor_max_shields(q->type,1)==300);
    int sprite_y=building_sprite_y_for_test(q),shield_y=sprite_y-7,hp_y=sprite_y-4;
    capture_building(q,full_shield_pixels);
    assert(count_shield_row(full_shield_pixels,91,shield_y,18)>0);
    q->shield=0;
    capture_building(q,empty_shield_pixels);
    assert_only_shield_rows_changed(full_shield_pixels,empty_shield_pixels,shield_y);
    assert(count_shield_row(empty_shield_pixels,91,shield_y,18)>0);
    assert(count_shield_row(empty_shield_pixels,91,shield_y+1,18)>0);
    assert(memcmp(full_shield_pixels+hp_y*240,empty_shield_pixels+hp_y*240,240)==0);
}

static void
test_vital_bar_viewport_clipping(void)
{
    /* A wholly offscreen rectangle disappears; it is not pinned into the
     * resource strip or moved down over actors that remain in view. */
    gfx_begin(C_BLACK);
    draw_actor_vitals(10,10,12,100,100,80,100,C_GREEN);
    for(int y=0;y<160;++y)for(int x=0;x<240;++x)
        assert(gfx_test_pixel(x,y)==C_BLACK);

    gfx_begin(C_BLACK);
    draw_actor_vitals(10,14,12,100,100,80,100,C_GREEN);
    for(int y=0;y<12;++y)for(int x=0;x<240;++x)
        assert(gfx_test_pixel(x,y)==C_BLACK);
    for(int y=120;y<160;++y)for(int x=0;x<240;++x)
        assert(gfx_test_pixel(x,y)==C_BLACK);
    assert(gfx_test_pixel(10,12)==C_CYAN); /* upper bar is clipped into view */

    gfx_begin(C_BLACK);
    draw_actor_vitals(230,121,12,100,100,80,100,C_GREEN);
    for(int y=0;y<12;++y)for(int x=0;x<240;++x)
        assert(gfx_test_pixel(x,y)==C_BLACK);
    for(int y=120;y<160;++y)for(int x=0;x<240;++x)
        assert(gfx_test_pixel(x,y)==C_BLACK);
    assert(gfx_test_pixel(230,118)==C_CYAN);
    assert(gfx_test_pixel(230,119)==C_CYAN);
}

static void
test_selected_hud_reads_live_shield_values(void)
{
    Unit *q=&u[0];
    fixture();
    q->alive=1; q->owner=SIDE;
#ifdef SC64_P7_ACTORS
    q->type=U_P7_ARCHON;
#else
    q->type=U_DRAGOON;
#endif
    q->raw_type=sc64_race_raw_unit(q->type); q->raw_owner=7; q->selected=1;
    q->x=100; q->y=80; q->hp=q->maxhp=sc64_race_unit_rules[q->type].hp;
    q->shield=0;
    sc64_hud_valid=0;
    gfx_begin(C_BLACK); draw_hud();
    assert(sc64_hud_valid);
    assert(sc64_hud_changed()==0);
    q->shield=1;
    assert(sc64_hud_changed()&2);
}

int
main(void)
{
    gba_init();
#ifdef SC64_P7_ACTORS
    test_unit_shield_bar_and_health_independence(U_P7_ARCHON,68,10,10,350);
    test_unit_shield_bar_and_health_independence(U_P7_CARRIER,72,300,300,150);
#else
    test_unit_shield_bar_and_health_independence(U_DRAGOON,
        sc64_race_raw_unit(U_DRAGOON),100,100,80);
#endif
    test_unshielded_unit_has_no_added_bar();
    test_building_shield_bar_and_health_independence();
#ifdef SC64_RACES
    test_selected_hud_reads_live_shield_values();
#endif
    test_vital_bar_viewport_clipping();
    puts("PASS unit/building shield pixels, zero-shield baseline and selected HUD refresh");
    return 0;
}
