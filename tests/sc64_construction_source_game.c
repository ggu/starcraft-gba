/* SPDX-License-Identifier: GPL-3.0-or-later
 * Optional real-asset renderer/save checks; no extracted pixels in this file. */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/game.c"

#ifndef SC64_CONSTRUCTION_SOURCE_ART
#error "Compile with the private construction art profile"
#endif

static u8 first[240*160], second[240*160];

static void capture(u8 *pixels)
{
    for (int y=0;y<160;++y)
        for (int x=0;x<240;++x) pixels[y*240+x]=gfx_test_pixel(x,y);
}

static int start_building(int race, int type)
{
    game_init();
    race_mode=1; race_human=race; race_bots=1;
    race_skirmish_init(); screen=SCREEN_PLAY;
    int slot=spawn_build(SIDE,type,160,160,1);
    assert(slot>=0);
    camx=80; camy=100;
    memset(visible,0xff,sizeof(visible));
    return slot;
}

static void draw_at_progress(int slot, int progress, u8 *pixels)
{
    b[slot].build_progress=progress;
    Building before=b[slot];
    gfx_begin(C_BLACK);
    draw_building_overlay(&b[slot]);
    capture(pixels);
    assert(!memcmp(&b[slot],&before,sizeof(before)));
}

static void check_source_pixels(int slot, int progress)
{
    unsigned stage_index, frame_index;
    uint32_t elapsed;
    Building *building=&b[slot];
    int duration=building_build_time(building->type);
    int race=sc64_race_building_rules[building->type].race+1;
    assert(sc64_construction_source_phase(race,progress,duration,
                                          &stage_index,&elapsed));
    const SC64ConstructionSourceStage *stage=
        sc64_construction_stage(building->raw_type>=0 ? building->raw_type :
                               sc64_race_raw_building(building->type),stage_index);
    assert(stage);
    assert(sc64_construction_animation_frame(stage->animation,elapsed,&frame_index));
    const Sc64SpriteFrame *sprite=
        sc64_construction_image_frame(stage->image_id,frame_index);
    assert(sprite);
    draw_at_progress(slot,progress,first);
    int left=building->x-camx-sprite->canvas_w/2+sprite->offset_x;
    int top=building->y-camy+12-sprite->canvas_h/2+sprite->offset_y;
    const Sc64SpriteFrame *completed=building->raw_type>=0 ?
        sc64_race_unit_sprite(building->raw_type) : mission_building_sprite(building->type);
    int ui_top=completed ? building->y-camy+12-completed->canvas_h/2+
                          completed->offset_y : building->y-camy;
    int ui_height=completed ? completed->h : 24;
    /* Source primary pixels must survive composition. Restrict to the inner
     * screen region so HP/progress bars cannot mask the asserted sprite. */
    unsigned matches=0;
    for (int y=0;y<sprite->h;++y)
        for (int x=0;x<sprite->w;++x) {
            int sx=left+x, sy=top+y;
            u8 pixel=sprite->pixels[y*sprite->w+x];
            if ((sy>=ui_top-4 && sy<ui_top-2) ||
                (sy>=ui_top+ui_height+3 && sy<ui_top+ui_height+5)) continue;
            if (pixel && sx>=20 && sx<220 && sy>=40 && sy<90) {
                assert(first[sy*240+sx]==pixel);
                ++matches;
            }
        }
    assert(matches>8);
    /* A frozen build is independent of the global render clock. */
    frame+=100;
    draw_at_progress(slot,progress,second);
    assert(!memcmp(first,second,sizeof(first)));
}

static void test_stages_and_morph_loop(void)
{
    const int terran[]={B_COMMAND,B_BARRACKS,B_DEPOT,B_ACADEMY,
                        B_BUNKER,B_STARPORT,B_MISSILE_TURRET};
    const int zerg[]={B_HATCHERY,B_SPAWNING_POOL,B_HYDRALISK_DEN,
                      B_EVOLUTION_CHAMBER,B_CREEP_COLONY,B_EXTRACTOR,
                      B_LAIR,B_SUNKEN_COLONY,B_SPORE_COLONY,B_SPIRE};
    for (unsigned i=0;i<sizeof(terran)/sizeof(*terran);++i) {
        int slot=start_building(R_TER,terran[i]);
        int total=building_build_time(terran[i]);
        for (int fifth=0;fifth<4;++fifth)
            check_source_pixels(slot,(total*fifth)/5+2);
    }
    for (unsigned i=0;i<sizeof(zerg)/sizeof(*zerg);++i) {
        int slot=start_building(R_ZERG,zerg[i]);
        int total=building_build_time(zerg[i]);
        check_source_pixels(slot,2);
        check_source_pixels(slot,total/3);
        check_source_pixels(slot,2*total/3);
    }
    int slot=start_building(R_ZERG,B_HATCHERY);
    draw_at_progress(slot,0,first);
    draw_at_progress(slot,15,second);
    assert(memcmp(first,second,sizeof(first))); /* Actual primary morph motion. */
}

static void test_save_load_and_completion(void)
{
    int slot=start_building(R_ZERG,B_HATCHERY);
    int total=building_build_time(B_HATCHERY);
    draw_at_progress(slot,total/3,first);
    Building saved=b[slot];
    assert(race_save_game());
    b[slot].build_progress=total-1;
    assert(race_load_game());
    assert(!memcmp(&b[slot],&saved,sizeof(saved)));
    /* Load may move the camera; restoring the same view reproduces the art. */
    camx=80; camy=100;
    draw_at_progress(slot,saved.build_progress,second);
    assert(!memcmp(first,second,sizeof(first)));
    b[slot].build_progress=total-1;
    race_tick_buildings();
    assert(!b[slot].under_construction);
    unsigned stage; uint32_t elapsed;
    assert(!sc64_construction_source_phase(2,total,total,&stage,&elapsed));
}

static void test_waiting_and_unsupported_fallback(void)
{
    int slot=start_building(R_ZERG,B_HATCHERY);
    draw_at_progress(slot,-1,first);
    frame+=100;
    draw_at_progress(slot,-1,second);
    assert(!memcmp(first,second,sizeof(first)));
    assert(!sc64_construction_stage(-1,0));
    assert(!sc64_construction_stage(65536,0));
    assert(!sc64_construction_stage(131,100));
    assert(!sc64_construction_image_frame(65535,0));
    slot=start_building(R_PROTOSS,B_GATEWAY);
    draw_at_progress(slot,2,first);
    assert(!sc64_construction_stage(sc64_race_raw_building(b[slot].type),0));
    unsigned lit=0;
    for (unsigned i=0;i<sizeof(first);++i) lit+=first[i]!=C_BLACK;
    assert(lit>30);
}

int main(void)
{
    gba_init();
    test_stages_and_morph_loop();
    test_save_load_and_completion();
    test_waiting_and_unsupported_fallback();
    puts("Source construction stages, primary pixels, frozen progress and SRAM reload passed");
    return 0;
}
