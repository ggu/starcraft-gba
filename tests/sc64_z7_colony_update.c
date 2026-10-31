/* Spore targeting through the real Z7 simulation update and impact clock. */
#define main sc64_z7_legacy_fixture_main
#include "sc64_z7_colonies_game.c"
#undef main

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
    puts("Z7 Spore real update and delayed air impact passed");
    return 0;
}
