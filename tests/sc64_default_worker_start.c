/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Original-art opening jobs are explicit, while later idle workers stay idle. */
#include <assert.h>
#include <stdio.h>
#include "../starcraft/game.c"
int main(void)
{
    game_init();init_state();
    int workers=0;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].owner==SIDE&&IS_WORKER(u[i].type)) {
        assert(u[i].order==ORDER_MINE&&u[i].target>=0&&u[i].target<MAX_MINERALS&&m[u[i].target].amount>0);
        ++workers;
    }
    assert(workers==5);
    int n=spawn_unit(SIDE,U_SCV,32,130);assert(n>=0&&u[n].order==ORDER_IDLE);
    for(int i=0;i<120;i++){game_update(0,0);assert(u[n].alive&&u[n].order==ORDER_IDLE);}
#ifdef SC_DEFAULT_UI
    clear_selection();u[n].selected=1;cx=160;cy=120;
    m[0].x=cx;m[0].y=cy;m[0].amount=100;
    SC64_GRID_SET(visible,cx/16+(cy/16)*MAP_COLS,1);
    field_ui_refresh();assert(ui_action==UI_A_MINE);
#endif
    puts("original-art five opening workers assigned; later idle worker remains idle");return 0;
}
