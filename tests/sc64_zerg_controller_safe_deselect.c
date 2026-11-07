/* Isolated controller helper regression: authored setup, ordinary input only
 * after selection. Not a mission replay or campaign completion claim. */
#include "../common/gba.c"
#define main worker_fixture_main
#include "sc64_worker_gather_game.c"
#undef main
#include "sc64_zerg_late_controller.h"
#include <assert.h>
int main(void)
{
    reset_scene(R_ZERG,R_TER);
    int worker=spawn_unit(SIDE,U_DRONE,120,120);
    assert(worker>=0);
    set_depot(0,SIDE,B_HATCHERY,128,128);
    race_bots=1; set_depot(1,ENEMY,B_COMMAND,500,500);
    b[0].queue[0]=U_DRONE; b[0].queue_count=1; b[0].larva=1;
    selected_building=0; minerals=500; gas=0;
    clear_selected_building();
    assert(screen==SCREEN_PLAY && selected_building<0);
    assert(b[0].queue_count==1 && b[0].queue[0]==U_DRONE && b[0].larva==1 && minerals==500);
    /* A deliberate L gesture must still cancel and refund after reselection. */
    selected_building=0;
    KEY_TAP(KEY_L);
    assert(b[0].queue_count==0 && b[0].larva==2 && minerals==550);
    puts("PASS ordinary SELECT deselect preserves queue; explicit L cancels");
    return 0;
}
