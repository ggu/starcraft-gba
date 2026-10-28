/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Pixel checks for selected unit and building vital bars. */
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

/* Bar geometry mirrors draw_unit/draw_building_overlay: the selected actor's
 * bar starts three rows below its sprite, snapped to 3n+1 columns. */
typedef struct { int left, top, width; } BarBox;

static int
snapped_width(int width)
{
    width-=(width-1)%3;
    return width<13?13:width;
}

static BarBox
unit_bar_for_test(const Unit *q)
{
    int x=q->x-camx,y=q->y-camy+12;
    const Sc64SpriteFrame *s=unit_sprite_for_test(q);
    int sprite_y=s?y-s->canvas_h/2+s->offset_y:y-4;
    int sprite_w=s?s->w:8,sprite_h=s?s->h:8;
    BarBox box;
    box.width=snapped_width(sprite_w+4);
    box.left=x-box.width/2;
    box.top=sprite_y+sprite_h+3;
    return box;
}

static BarBox
building_bar_for_test(const Building *q)
{
    int x=q->x-camx,y=q->y-camy+12;
    const Sc64SpriteFrame *s=building_sprite_for_test(q);
    int sprite_y=s?y-s->canvas_h/2+s->offset_y:y-12;
    int sprite_w=s?s->w:24,sprite_h=s?s->h:24;
    BarBox box;
    box.width=snapped_width(sprite_w>46?46:sprite_w);
    box.left=x-box.width/2;
    box.top=sprite_y+sprite_h+3;
    return box;
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
count_row(const u8 *pixels,BarBox box,int row,u8 color)
{
    int count=0;
    for(int dx=0;dx<box.width;++dx)
        count+=pixels[(box.top+row)*240+box.left+dx]==color;
    return count;
}

/* Cells fill whole: every third column is a black divider. */
static void
assert_cell_row(const u8 *pixels,BarBox box,int row,u8 fill,u8 empty)
{
    for(int dx=0;dx<box.width;++dx) {
        u8 c=pixels[(box.top+row)*240+box.left+dx];
        if(dx%3==0) assert(c==C_BLACK);
        else assert(c==fill||c==empty);
    }
}

static void
assert_black_row(const u8 *pixels,BarBox box,int row)
{
    assert(count_row(pixels,box,row,C_BLACK)==box.width);
}

static void
assert_only_rows_changed(const u8 *a,const u8 *z,int first,int last)
{
    for(int y=0;y<160;++y)if(y<first||y>last)
        assert(memcmp(a+y*240,z+y*240,240)==0);
}

static void
test_unit_shield_bar_and_health_independence(int type,int raw_type,
                                              int hp,int maxhp,int maxshield)
{
    Unit *q=&u[0];
    fixture();
    q->alive=1; q->owner=SIDE; q->type=type; q->selected=1;
    q->raw_type=raw_type; q->raw_owner=7; q->x=100; q->y=70;
    q->hp=hp; q->maxhp=maxhp; q->shield=maxshield;
    assert(sc64_actor_max_shields(q->type,0)==maxshield);
    BarBox box=unit_bar_for_test(q);
    if(box.top>110) { q->y-=box.top-110; box=unit_bar_for_test(q); }
    assert(box.top>=12 && box.top+7<=120);
    capture_unit(q,full_shield_pixels);
    /* Frame, shield light/dark, separator, HP light/dark, frame. */
    assert_black_row(full_shield_pixels,box,0);
    assert_cell_row(full_shield_pixels,box,1,C_BLUE,C_STONE);
    assert_cell_row(full_shield_pixels,box,2,C_BLUE_DARK,C_UI);
    assert_black_row(full_shield_pixels,box,3);
    assert_cell_row(full_shield_pixels,box,4,C_GREEN,C_STONE);
    assert_cell_row(full_shield_pixels,box,5,C_GREEN_DARK,C_UI);
    assert_black_row(full_shield_pixels,box,6);
    assert(count_row(full_shield_pixels,box,1,C_STONE)==0);
    assert(count_row(full_shield_pixels,box,4,C_STONE)==0);

    q->shield=maxshield/2;
    capture_unit(q,partial_shield_pixels);
    assert_only_rows_changed(full_shield_pixels,partial_shield_pixels,
                             box.top+1,box.top+2);
    assert(count_row(partial_shield_pixels,box,1,C_BLUE)>0);
    assert(count_row(partial_shield_pixels,box,1,C_STONE)>0);
    assert(count_row(partial_shield_pixels,box,1,C_BLUE)<
           count_row(full_shield_pixels,box,1,C_BLUE));

    q->shield=0;
    capture_unit(q,empty_shield_pixels);
    assert_only_rows_changed(full_shield_pixels,empty_shield_pixels,
                             box.top+1,box.top+2);
    /* The empty track stays visible: grey cells, no blue. */
    assert(count_row(empty_shield_pixels,box,1,C_BLUE)==0);
    assert_cell_row(empty_shield_pixels,box,1,C_BLUE,C_STONE);
    assert_cell_row(empty_shield_pixels,box,2,C_BLUE_DARK,C_UI);

    q->selected=0;
    capture_unit(q,empty_shield_pixels);
    for(int row=0;row<7;++row)
        for(int dx=0;dx<box.width;++dx)
            assert(empty_shield_pixels[(box.top+row)*240+box.left+dx]==C_BLACK);
}

static void
test_health_tier_colors(void)
{
    static const struct { int hp; u8 light, dark; } tiers[]={
        {40,C_GREEN,C_GREEN_DARK},{27,C_GREEN,C_GREEN_DARK},
        {26,C_GOLD,C_GOLD_DARK},{14,C_GOLD,C_GOLD_DARK},
        {13,C_RED,C_RED_DARK},{1,C_RED,C_RED_DARK}
    };
    Unit *q=&u[0];
    fixture();
    q->alive=1; q->owner=SIDE; q->type=U_MARINE; q->selected=1;
    q->raw_type=sc64_race_raw_unit(U_MARINE); q->raw_owner=7; q->x=100; q->y=70;
    q->maxhp=40; q->shield=0;
    BarBox box=unit_bar_for_test(q);
    for(unsigned i=0;i<sizeof(tiers)/sizeof(tiers[0]);++i) {
        q->hp=tiers[i].hp;
        capture_unit(q,full_shield_pixels);
        assert_cell_row(full_shield_pixels,box,1,tiers[i].light,C_STONE);
        assert_cell_row(full_shield_pixels,box,2,tiers[i].dark,C_UI);
        assert(count_row(full_shield_pixels,box,1,tiers[i].light)>=2);
    }
}

static void
test_unshielded_unit_has_no_added_bar(void)
{
    Unit *q=&u[0];
    fixture();
    q->alive=1; q->owner=SIDE; q->type=U_MARINE; q->selected=1;
    q->raw_type=sc64_race_raw_unit(U_MARINE); q->raw_owner=7; q->x=100; q->y=70;
    q->hp=q->maxhp=40; q->shield=0;
    assert(sc64_actor_max_shields(q->type,0)==0);
    BarBox box=unit_bar_for_test(q);
    capture_unit(q,empty_shield_pixels);
    assert_black_row(empty_shield_pixels,box,0);
    assert_cell_row(empty_shield_pixels,box,1,C_GREEN,C_STONE);
    assert_cell_row(empty_shield_pixels,box,2,C_GREEN_DARK,C_UI);
    assert_black_row(empty_shield_pixels,box,3);
    for(int y=box.top;y<box.top+8;++y)for(int x=0;x<240;++x)
        assert(empty_shield_pixels[y*240+x]!=C_BLUE &&
               empty_shield_pixels[y*240+x]!=C_PURPLE);
    for(int x=0;x<240;++x)
        assert(empty_shield_pixels[(box.top+4)*240+x]==C_BLACK);
}

static void
test_building_shield_bar_and_health_independence(void)
{
    Building *q=&b[0];
    fixture();
    q->alive=1; q->owner=SIDE; q->type=B_PYLON;
    q->raw_type=156; q->raw_owner=7; q->x=100; q->y=70;
    q->hp=q->maxhp=300; q->shield=300;
    assert(sc64_actor_max_shields(q->type,1)==300);
    BarBox box=building_bar_for_test(q);
    capture_building(q,empty_shield_pixels);
    for(int row=0;row<7;++row)
        assert(count_row(empty_shield_pixels,box,row,C_BLACK)==box.width);
    selected_building=0;
    capture_building(q,full_shield_pixels);
    assert_cell_row(full_shield_pixels,box,1,C_BLUE,C_STONE);
    assert(count_row(full_shield_pixels,box,1,C_STONE)==0);
    q->shield=0;
    capture_building(q,empty_shield_pixels);
    assert_only_rows_changed(full_shield_pixels,empty_shield_pixels,
                             box.top+1,box.top+2);
    assert(count_row(empty_shield_pixels,box,1,C_BLUE)==0);
    assert_cell_row(empty_shield_pixels,box,1,C_BLUE,C_STONE);
    assert(memcmp(full_shield_pixels+(box.top+4)*240,
                  empty_shield_pixels+(box.top+4)*240,240*2)==0);
    selected_building=-1;
}

static void
test_energy_strip(void)
{
    gfx_begin(C_BLACK);
    draw_actor_vitals(100,40,13,100,100,80,100,50,200);
    /* Shielded HP ends at row 46; a clear row, then the energy frame. */
    for(int x=93;x<107;++x) assert(gfx_test_pixel(x,47)==C_BLACK);
    BarBox box={94,47,13};
    for(int dx=0;dx<13;++dx) {
        assert(gfx_test_pixel(94+dx,48)==C_BLACK);
        assert(gfx_test_pixel(94+dx,51)==C_BLACK);
    }
    int purple=0,grey=0;
    for(int dx=0;dx<13;++dx) {
        u8 c=gfx_test_pixel(94+dx,49);
        if(dx%3==0) assert(c==C_BLACK);
        purple+=c==C_PURPLE; grey+=c==C_STONE;
        assert(gfx_test_pixel(94+dx,50)==(dx%3==0?C_BLACK:
               c==C_PURPLE?C_ZERG:C_UI));
    }
    assert(purple==2 && grey==6); /* a quarter fills one whole cell */
    (void)box;
}

static void
test_vital_bar_viewport_clipping(void)
{
    /* A wholly offscreen bar disappears; it is not pinned into the
     * resource strip or moved down over actors that remain in view. */
    gfx_begin(C_BLACK);
    draw_actor_vitals(10,5,13,100,100,80,100,0,0);
    for(int y=0;y<160;++y)for(int x=0;x<240;++x)
        assert(gfx_test_pixel(x,y)==C_BLACK);

    gfx_begin(C_BLACK);
    draw_actor_vitals(10,10,13,100,100,80,100,0,0);
    for(int y=0;y<12;++y)for(int x=0;x<240;++x)
        assert(gfx_test_pixel(x,y)==C_BLACK);
    assert(gfx_test_pixel(5,12)==C_BLUE_DARK); /* lower shield row in view */
    assert(gfx_test_pixel(5,14)==C_GREEN);

    gfx_begin(C_BLACK);
    draw_actor_vitals(230,117,13,100,100,80,100,0,0);
    for(int y=120;y<160;++y)for(int x=0;x<240;++x)
        assert(gfx_test_pixel(x,y)==C_BLACK);
    assert(gfx_test_pixel(229,118)==C_BLUE);
    assert(gfx_test_pixel(229,119)==C_BLUE_DARK);
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
    test_health_tier_colors();
    test_unshielded_unit_has_no_added_bar();
    test_building_shield_bar_and_health_independence();
    test_energy_strip();
#ifdef SC64_RACES
    test_selected_hud_reads_live_shield_values();
#endif
    test_vital_bar_viewport_clipping();
    puts("PASS selected-actor vital bars: cells, tiers, shields, energy, clipping and HUD refresh");
    return 0;
}
