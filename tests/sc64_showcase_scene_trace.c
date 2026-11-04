/* SPDX-License-Identifier: GPL-3.0-or-later
 * Focused scene trace; the separate full save-isolation fixture is not replaced. */
#define main sc64_full_showcase_fixture_main
#include "sc64_showcase_game.c"
#undef main
#include <stdlib.h>
static void write_scene_checkpoint(FILE *out,int scene,int tick) {
    if (fwrite(&scene,sizeof(scene),1,out)!=1 || fwrite(&tick,sizeof(tick),1,out)!=1 ||
        fwrite(&frame,sizeof(frame),1,out)!=1 || fwrite(u,sizeof(u),1,out)!=1 ||
        fwrite(b,sizeof(b),1,out)!=1 || fwrite(visible,sizeof(visible),1,out)!=1) abort();
    for(int y=0;y<160;y++)for(int x=0;x<240;x++) {
        u16 pixel=gfx_test_pixel(x,y);if(fwrite(&pixel,sizeof(pixel),1,out)!=1)abort();
    }
}
int main(void) {
    const char *path=getenv("SC64_SHOWCASE_TRACE_PATH");
    if(!path) return fail("missing scene trace path");
    FILE *out=fopen(path,"wb");if(!out)return fail("could not open scene trace");
    gba_init();game_init();previous=0;
    if(!enter_showcase_menu())return fail("could not open showcase");
    press(KEY_A);
    for(int scene=0;scene<8;scene++) {
        if(sc64_showcase_scene!=scene || screen!=SCREEN_PAUSE ||
           !check_scene_contents(scene) || !valid_world())return fail("bad scene start");
        press(KEY_A);
        if(screen!=SCREEN_PLAY)return fail("scene did not resume");
        for(int tick=0;tick<300 && screen==SCREEN_PLAY;tick++) {
            frame_input(0);if(!valid_world())return fail("invalid actor state");
            if((tick+1)%30==0 || screen!=SCREEN_PLAY) {
                game_render();write_scene_checkpoint(out,scene,tick+1);
            }
        }
        if(scene>=2 && scene<=4 && (screen!=SCREEN_RESULT || !lost || sc64_showcase_ticks!=150))
            return fail("unexpected last-stand outcome");
        if(scene<7) {
            if(screen==SCREEN_RESULT || screen==SCREEN_PAUSE)press(KEY_B);
            else {press(KEY_START);press(KEY_B);}
            if(screen!=SCREEN_TITLE || race_front.page!=SC_FRONT_SHOWCASE || race_front.row!=scene)
                return fail("did not return to scene menu");
            press(KEY_DOWN);press(KEY_A);
        }
    }
    if(fclose(out))return fail("could not close scene trace");
    puts("Showcase focused scene trace passed");return 0;
}
