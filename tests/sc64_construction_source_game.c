/* SPDX-License-Identifier: GPL-3.0-or-later
 * Optional real-asset renderer/save checks; no extracted pixels in this file. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "../starcraft/game.c"

#ifndef SC64_CONSTRUCTION_SOURCE_ART
#error "Compile with the private construction art profile"
#endif

static u8 first[240*160], second[240*160];

static void write_contact_sample(const char *race, const u8 *pixels)
{
    const char *prefix=getenv("SC64_CONSTRUCTION_CONTACT_SHEET_PREFIX");
    if (!prefix || !*prefix) return;
    char path[512];
    int n=snprintf(path,sizeof(path),"%s-%s.pgm",prefix,race);
    assert(n>0 && (size_t)n<sizeof(path));
    FILE *f=fopen(path,"wb");
    assert(f);
    fprintf(f,"P5\n240 160\n255\n");
    assert(fwrite(pixels,1,sizeof(first),f)==sizeof(first));
    assert(fclose(f)==0);
}

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

static u8 source_pixel(const Sc64SpriteFrame *sprite, int left, int top,
                      int screen_x, int screen_y)
{
    int x=screen_x-left, y=screen_y-top;
    if (!sprite || x<0 || y<0 || x>=sprite->w || y>=sprite->h) return 0;
    return sprite->pixels[y*sprite->w+x];
}

static void composite_source_sprite(u8 *pixels, const Sc64SpriteFrame *sprite,
                                   int center_x, int center_y)
{
    assert(sprite);
    int left=center_x-sprite->canvas_w/2+sprite->offset_x;
    int top=center_y-sprite->canvas_h/2+sprite->offset_y;
    for (int y=0;y<sprite->h;++y) for (int x=0;x<sprite->w;++x) {
        int sx=left+x, sy=top+y;
        u8 pixel=sprite->pixels[y*sprite->w+x];
        if (pixel && sx>=0 && sx<240 && sy>=0 && sy<160)
            pixels[sy*240+sx]=pixel;
    }
}

static const SC64ConstructionSourceStage *source_stage_for(
    Building *building, int progress, uint32_t *elapsed, unsigned *frame_index)
{
    unsigned stage_index;
    int duration=building_build_time(building->type);
    int race=sc64_race_building_rules[building->type].race+1;
    assert(sc64_construction_source_phase(race,progress,duration,
                                          &stage_index,elapsed));
    const SC64ConstructionSourceStage *stage=
        sc64_construction_stage(building->raw_type>=0 ? building->raw_type :
                               sc64_race_raw_building(building->type),stage_index);
    assert(stage && stage->layer_count>0 && stage->layers);
    assert(sc64_construction_animation_frame(stage->animation,*elapsed,frame_index));
    return stage;
}

static void assert_source_layer_pixels(int slot, int progress)
{
    Building *building=&b[slot];
    uint32_t elapsed;
    unsigned root_frame;
    const SC64ConstructionSourceStage *stage=
        source_stage_for(building,progress,&elapsed,&root_frame);
    assert(stage->layer_count>1);
    const Sc64SpriteFrame *root=
        sc64_construction_image_frame(stage->image_id,(unsigned short)root_frame);
    assert(root);
    draw_at_progress(slot,progress,first);
    int cx=building->x-camx, cy=building->y-camy+12;
    int root_left=cx-root->canvas_w/2+root->offset_x;
    int root_top=cy-root->canvas_h/2+root->offset_y;
    const Sc64SpriteFrame *completed=building->raw_type>=0 ?
        sc64_race_unit_sprite(building->raw_type) : mission_building_sprite(building->type);
    if (!completed && building->raw_type>=0)
        completed=sc64_race_building_sprite(building->raw_type,0);
    int ui_top=completed ? cy-completed->canvas_h/2+completed->offset_y : cy-12;
    int ui_height=completed ? completed->h : 24;
    for (unsigned li=1;li<stage->layer_count;++li) {
        const SC64ConstructionSourceLayer *layer=&stage->layers[li];
        unsigned child_frame=root_frame;
        if (elapsed<layer->start_source_updates) continue;
        if (!layer->follows_main_graphic)
            assert(sc64_construction_animation_frame(layer->animation,
                elapsed-layer->start_source_updates,&child_frame));
        const Sc64SpriteFrame *child=
            sc64_construction_image_frame(layer->image_id,(unsigned short)child_frame);
        assert(child);
        int child_left=cx+layer->x_offset-child->canvas_w/2+child->offset_x;
        int child_top=cy+layer->y_offset-child->canvas_h/2+child->offset_y;
        unsigned matches=0;
        for (int py=0;py<child->h;++py) for (int px=0;px<child->w;++px) {
            u8 pixel=child->pixels[py*child->w+px];
            int sx=child_left+px, sy=child_top+py;
            if (!pixel || sx<20 || sx>=220 || sy<40 || sy>=90 ||
                (sy>=ui_top-4 && sy<ui_top-2) ||
                (sy>=ui_top+ui_height+3 && sy<ui_top+ui_height+5)) continue;
            if (layer->under && source_pixel(root,root_left,root_top,sx,sy)) continue;
            assert(first[sy*240+sx]==pixel);
            ++matches;
        }
        assert(matches>0);
    }
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
    int center_x=building->x-camx;
    int center_y=building->y-camy+12;
    const Sc64SpriteFrame *completed=building->raw_type>=0 ?
        sc64_race_unit_sprite(building->raw_type) : mission_building_sprite(building->type);
    if (!completed && building->raw_type>=0)
        completed=sc64_race_building_sprite(building->raw_type,0);
    int ui_top=completed ? center_y-completed->canvas_h/2+
                          completed->offset_y : building->y-camy;
    int ui_height=completed ? completed->h : 24;
    /* Build an independent expected composite in the same semantic order as
     * the renderer: underlays, root image, then spawned overlays. */
    memset(second,0,sizeof(second));
    for (unsigned li=1;li<stage->layer_count;++li) {
        const SC64ConstructionSourceLayer *child=&stage->layers[li];
        if (!child->under || elapsed<child->start_source_updates) continue;
        unsigned child_frame=frame_index;
        if (!child->follows_main_graphic)
            assert(sc64_construction_animation_frame(child->animation,
                elapsed-child->start_source_updates,&child_frame));
        const Sc64SpriteFrame *art=sc64_construction_image_frame(
            child->image_id,(unsigned short)child_frame);
        composite_source_sprite(second,art,center_x+child->x_offset,
                                center_y+child->y_offset);
    }
    composite_source_sprite(second,sprite,center_x,center_y);
    for (unsigned li=1;li<stage->layer_count;++li) {
        const SC64ConstructionSourceLayer *child=&stage->layers[li];
        if (child->under || elapsed<child->start_source_updates) continue;
        unsigned child_frame=frame_index;
        if (!child->follows_main_graphic)
            assert(sc64_construction_animation_frame(child->animation,
                elapsed-child->start_source_updates,&child_frame));
        const Sc64SpriteFrame *art=sc64_construction_image_frame(
            child->image_id,(unsigned short)child_frame);
        composite_source_sprite(second,art,center_x+child->x_offset,
                                center_y+child->y_offset);
    }
    /* Progress/vital bars are separate UI and may overlap the source canvas. */
    unsigned compared=0;
    for (int sy=40;sy<90;++sy) {
        if ((sy>=ui_top-8 && sy<ui_top+2) ||
            (sy>=ui_top+ui_height+3 && sy<ui_top+ui_height+5)) continue;
        for (int sx=20;sx<220;++sx) if (second[sy*240+sx] &&
                                              (sx<center_x-10 || sx>center_x+10)) {
            assert(first[sy*240+sx]==second[sy*240+sx]);
            ++compared;
        }
    }
    assert(compared>8);
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
    const int protoss[]={B_NEXUS,B_GATEWAY,B_PYLON,B_ASSIMILATOR,
                         B_CYBERNETICS_CORE,B_FORGE,B_PHOTON_CANNON,B_STARGATE};
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
    for (unsigned i=0;i<sizeof(protoss)/sizeof(*protoss);++i) {
        int slot=start_building(R_PROTOSS,protoss[i]);
        int total=building_build_time(protoss[i]);
        check_source_pixels(slot,2);
        check_source_pixels(slot,total/2);
        check_source_pixels(slot,total-1);
    }
    /* Source composition includes Terran/Zerg underlays and Protoss warp
       overlays. All three images link their child frames to the parent frame. */
    int layered_terran=start_building(R_TER,B_COMMAND);
    assert_source_layer_pixels(layered_terran,2);
    write_contact_sample("terran",first);
    int layered_zerg=start_building(R_ZERG,B_HATCHERY);
    assert_source_layer_pixels(layered_zerg,2);
    write_contact_sample("zerg",first);
    int layered_protoss=start_building(R_PROTOSS,B_NEXUS);
    assert_source_layer_pixels(layered_protoss,2);
    write_contact_sample("protoss",first);
    int slot=start_building(R_ZERG,B_HATCHERY);
    draw_at_progress(slot,0,first);
    draw_at_progress(slot,15,second);
    assert(memcmp(first,second,sizeof(first))); /* Actual primary morph motion. */
}

static void test_every_constructible_raw_building(void)
{
    unsigned tested=0;
    unsigned by_race[3]={0};
    for (int raw=0;raw<228;++raw) {
        int race, type, race_index;
        if (raw>=106 && raw<=125) { race=R_TER; type=B_COMMAND; race_index=0; }
        else if (raw>=131 && raw<=149) { race=R_ZERG; type=B_HATCHERY; race_index=1; }
        else if (raw>=154 && raw<=172) { race=R_PROTOSS; type=B_NEXUS; race_index=2; }
        else continue;
        if (!sc64_construction_stage(raw,0)) continue;
        int slot=start_building(race,type);
        /* The source-art census includes stock buildings that this campaign
         * profile does not expose as build-menu identities. Keep the test's
         * gameplay rules/race valid while selecting each source raw image. */
        b[slot].raw_type=raw;
        int duration=building_build_time(type);
        assert(duration>0);
        unsigned last_stage=race==R_TER ? 3u : race==R_ZERG ? 2u : 0u;
        for (unsigned stage=0;stage<=last_stage;++stage) {
            int stage_start=0;
            if (race==R_TER && stage)
                stage_start=(duration*(int)stage)/5+1;
            else if (race==R_ZERG && stage)
                stage_start=(stage==1 ? duration/4 : duration/2)+1;
            int progress=stage_start+1;
            if (progress>=duration) progress=duration-1;
            unsigned actual_stage;
            uint32_t elapsed;
            assert(sc64_construction_source_phase(race+1,progress,duration,
                                                  &actual_stage,&elapsed));
            assert(actual_stage==stage);
            check_source_pixels(slot,progress);

            const SC64ConstructionSourceStage *program=
                sc64_construction_stage(raw,stage);
            assert(program);
            for (unsigned li=1;li<program->layer_count;++li) {
                const SC64ConstructionSourceLayer *layer=&program->layers[li];
                /* Invert floor(4*ticks/5) to reach each child's spawn clock.
                 * This exercises the first visible frame even for delayed
                 * layers, while retaining the real race/stage transition. */
                uint32_t source_tick=layer->start_source_updates;
                uint32_t game_ticks=(source_tick*5u+3u)/4u;
                int layer_progress=stage_start+(int)game_ticks;
                if (layer_progress>=duration) continue;
                unsigned reached_stage;
                uint32_t reached_elapsed;
                assert(sc64_construction_source_phase(race+1,layer_progress,
                    duration,&reached_stage,&reached_elapsed));
                if (reached_stage==stage &&
                    reached_elapsed>=layer->start_source_updates)
                    check_source_pixels(slot,layer_progress);
            }
        }
        ++tested;
        by_race[race_index]++;
    }
    assert(tested==50);
    assert(by_race[0]==18 && by_race[1]==16 && by_race[2]==16);
}

static void test_save_load_and_completion(void)
{
    const int races[]={R_TER,R_ZERG,R_PROTOSS};
    const int types[]={B_COMMAND,B_HATCHERY,B_NEXUS};
    for (unsigned i=0;i<3;++i) {
        int slot=start_building(races[i],types[i]);
        int total=building_build_time(types[i]);
        draw_at_progress(slot,total/3,first);
        Building saved=b[slot];
        assert(race_save_game());
        b[slot].build_progress=total-1;
        assert(race_load_game());
        assert(!memcmp(&b[slot],&saved,sizeof(saved)));
        camx=80; camy=100;
        draw_at_progress(slot,saved.build_progress,second);
        assert(!memcmp(first,second,sizeof(first)));

        /* Pausing stops simulation updates while source art remains the same. */
        screen=SCREEN_PAUSE;
        frame+=100;
        game_update(0,0);
        assert(screen==SCREEN_PAUSE);
        draw_at_progress(slot,saved.build_progress,second);
        assert(!memcmp(first,second,sizeof(first)));
        screen=SCREEN_PLAY;

        b[slot].build_progress=total-1;
        if (races[i]==R_TER) race_complete_building(slot);
        else race_tick_buildings();
        assert(!b[slot].under_construction);
        unsigned stage; uint32_t elapsed;
        assert(!sc64_construction_source_phase(races[i]+1,total,total,&stage,&elapsed));
        gfx_begin(C_BLACK);
        draw_building_overlay(&b[slot]);
        capture(second);
        unsigned visible_pixels=0;
        for (unsigned p=0;p<sizeof(second);++p) visible_pixels+=second[p]!=C_BLACK;
        assert(visible_pixels>30);
    }
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
    assert(sc64_construction_stage(sc64_race_raw_building(b[slot].type),0));
    unsigned lit=0;
    for (unsigned i=0;i<sizeof(first);++i) lit+=first[i]!=C_BLACK;
    assert(lit>30);
}

static void test_gas_overlay_source_requests(void)
{
    const SC64ConstructionSourceStage *refinery=sc64_construction_stage(110,0);
    const SC64ConstructionSourceStage *assimilator=sc64_construction_stage(157,0);
    assert(refinery && refinery->layer_count>0);
    assert(assimilator && assimilator->layer_count>0);
    const SC64ConstructionSourceLayer *root=&refinery->layers[0];
    assert(root->gas_overlay_request_count==7 && root->gas_overlay_requests);
    static const signed char selectors[]={0,2,1,2,0,1,2};
    static const uint32_t times[]={34,62,90,118,146,174,202};
    for (unsigned i=0;i<7;++i) {
        const SC64ConstructionGasOverlayRequest *request=&root->gas_overlay_requests[i];
        assert(request->selector==selectors[i]);
        assert(request->start_source_updates==times[i]);
        assert(sc64_construction_image_frame_count(
            SC64_CONSTRUCTION_GAS_SMOKE_IMAGE_BASE+request->selector)>0);
        assert(sc64_construction_image_frame_count(
            SC64_CONSTRUCTION_GAS_SMOKE_DEPLETED_IMAGE_BASE+request->selector)>0);
    }
    assert(assimilator->layers[0].gas_overlay_request_count==6);
}

int main(void)
{
    gba_init();
    unsigned stages=0;
    for (int raw=0;raw<228;++raw) for (unsigned phase=0;phase<4;++phase) {
        const SC64ConstructionSourceStage *stage=sc64_construction_stage(raw,phase);
        if (!stage) continue;
        ++stages;
        for (unsigned step=0;step<stage->animation->step_count;++step) {
            unsigned index=stage->animation->steps[step].frame;
            assert(sc64_construction_image_frame(stage->image_id,index));
            for (unsigned i=1;i<stage->layer_count;++i)
                if (stage->layers[i].follows_main_graphic)
                    assert(sc64_construction_image_frame(stage->layers[i].image_id,index));
        }
    }
    assert(stages==136); /* Full imported constructible/morph census. */
    test_stages_and_morph_loop();
    test_every_constructible_raw_building();
    test_save_load_and_completion();
    test_waiting_and_unsupported_fallback();
    test_gas_overlay_source_requests();
    puts("Source construction stages, primary pixels, frozen progress and SRAM reload passed");
    return 0;
}
