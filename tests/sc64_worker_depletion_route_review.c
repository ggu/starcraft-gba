/* Authored initial scenes; no mutation of the world after retarget. */
#define main existing_worker_gather_main
#include "sc64_worker_gather_game.c"
#undef main

static int continuation_route(int obstructed)
{
    int w, picked=0, retargeted=0;
    reset_scene(R_PROTOSS,R_TER);
    w=spawn_unit(SIDE,U_PROBE,120,120);
    if (w<0) return 1;
    set_depot(0,SIDE,B_NEXUS,120,120);
    m[2].x=200; m[2].y=200; m[2].amount=0;
    m[3].x=216; m[3].y=200; m[3].amount=24;
    u[w].order=ORDER_MINE; u[w].target=2; u[w].carry=5;
    if (obstructed) {
        /* A solid wall requires routing below its end, while the resource
         * goal and its pickup area stay walkable. */
        for (int y=0;y<=12;++y) SC64_GRID_SET(path_walkable,10+y*MAP_COLS,0);
        path_grid_valid=1;
#ifdef SC64_IMPORTED_MISSION
        sc64_routes_reset();
#endif
    }
    for (int tick=0;tick<6000;++tick) {
        one_tick();
        if (!retargeted && u[w].target==3) {
            retargeted=1;
            printf("%s retarget tick=%d pos=%d,%d ore=%d\n",obstructed?"wall":"open",tick,u[w].x,u[w].y,minerals);
            if (minerals!=5 || u[w].carry) return 2;
        }
        if (retargeted && !picked && u[w].carry==8) {
            picked=1;
            printf("%s pickup tick=%d pos=%d,%d field=%d\n",obstructed?"wall":"open",tick,u[w].x,u[w].y,m[3].amount);
            if (m[3].amount!=16) return 3;
        }
        if (picked && minerals==13 && !u[w].carry) {
            printf("%s deposit tick=%d pos=%d,%d ore=%d\n",obstructed?"wall":"open",tick,u[w].x,u[w].y,minerals);
            return 0;
        }
        if (tick%1000==999) printf("%s tick=%d pos=%d,%d target=%d order=%d carry=%d ore=%d retry=%d\n",obstructed?"wall":"open",tick,u[w].x,u[w].y,u[w].target,u[w].order,u[w].carry,minerals,u[w].path_retry);
        if (u[w].order!=ORDER_MINE) break;
    }
    fprintf(stderr,"%s failed: retarget=%d pickup=%d pos=%d,%d target=%d order=%d carry=%d ore=%d\n",obstructed?"wall":"open",retargeted,picked,u[w].x,u[w].y,u[w].target,u[w].order,u[w].carry,minerals);
    return 4;
}
int main(void)
{
    setbuf(stdout,0);
    int result=continuation_route(0); if (result) return result;
    return continuation_route(1);
}
