/* Spore targeting through the real Z7 simulation update and impact clock. */
#define main sc64_z7_legacy_fixture_main
#include "sc64_z7_colonies_game.c"
#undef main
#include "sc64_save_candidate_fixture.h"

int main(void)
{
    launch_z7_normally();
    int site=find_any_legal_site(B_SPORE_COLONY, 500,500);
    assert(site>=0);
    int x=site%WORLD_W,y=site/WORLD_W;
    int spore=spawn_build(SIDE,B_SPORE_COLONY,x,y,0);
    int ground=spawn_unit(ENEMY,U_ZERGLING,x+8,y+8);
    int air=spawn_unit(ENEMY,U_MUTALISK,x+20,y+20);
    assert(spore>=0 && ground>=0 && air>=0);
    int ghp=u[ground].hp, ahp=u[air].hp;
    for (int tick=0;tick<120 && u[air].alive && u[air].hp==ahp;++tick) input(0);
    assert(u[air].hp<ahp && u[ground].hp==ghp);

    /* A lethal queued impact resolves after production; pausing immediately
     * must save and reload the resulting dead row without transient flags. */
    launch_z7_normally();
    minerals=1000;
    int colony=create_colony_through_normal_build();
    assert(race_colony_start_sunken(colony));
    b[colony].hp=1; /* fixture stages a lethal impact */
    assert(!race_impact_full());
    race_impact_launch(MAX_UNITS+colony,20,1,ENEMY,0,U_MARINE,1);
    input(0);
    assert(!b[colony].alive &&
           !(b[colony].flags&(SC64_Z7_SUNKEN_MORPH_FLAG|SC64_Z7_SPORE_MORPH_FLAG)) &&
           b[colony].queue_progress==0);
    press(KEY_START); assert(screen==SCREEN_PAUSE);
    press(KEY_B);
    static RaceSave saved;
    fixture_capture_save(&saved);
    assert(race_z7_save_candidate_valid(&saved,&saved.z7,(unsigned)race_cols,(unsigned)race_rows));
    game_init(); held_keys=0; press(KEY_L);
    assert(screen==SCREEN_PLAY && race_z7_active() && !b[colony].alive &&
           !(b[colony].flags&(SC64_Z7_SUNKEN_MORPH_FLAG|SC64_Z7_SPORE_MORPH_FLAG)));
    puts("Z7 Spore impact and morph death valid save/load passed");
    return 0;
}
