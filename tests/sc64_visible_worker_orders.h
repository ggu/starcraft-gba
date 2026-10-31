/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_VISIBLE_WORKER_ORDERS_H
#define SC64_VISIBLE_WORKER_ORDERS_H
/* Assign idle SCVs through real field input after the opening threats clear.
 * Patch candidates must be currently visible; no world state is written. */
static int
order_idle_workers_to_visible_minerals(void)
{
    static const int dx[] = {0, 8, -8, 0, 0, 8, -8, 8, -8};
    static const int dy[] = {0, 0, 0, 8, -8, 8, 8, -8, -8};
    int assigned = 0;
    movement_base = 0;
    for (int worker = 0; worker < MAX_UNITS && screen == SCREEN_PLAY; ++worker) {
        if (!u[worker].alive || u[worker].owner != SIDE || u[worker].type != U_SCV ||
            u[worker].order != ORDER_IDLE || u[worker].carry) continue;
        if (selected_building >= 0) {
            press_key(KEY_SELECT);
            if (selected_building >= 0 || screen != SCREEN_PLAY) continue;
        }
        for (int attempt=0; attempt<=MAX_UNITS && screen==SCREEN_PLAY &&
             u[worker].alive && !u[worker].selected; ++attempt) press_key(KEY_L);
        if (!u[worker].alive || !u[worker].selected || selected_count()!=1) continue;
        int target=-1,best=0x7fffffff;
        for(int p=0;p<MAX_MINERALS;++p)if(currently_visible(m[p].x,m[p].y)&&m[p].amount>0) {
            int d=dist2(u[worker].x,u[worker].y,m[p].x,m[p].y);
            if(d<best){best=d;target=p;}
        }
        if(target<0)continue;
        for(unsigned k=0;k<sizeof(dx)/sizeof(dx[0]);++k) {
            int x=m[target].x+dx[k],y=m[target].y+dy[k];
            move_cursor_near(x,y);
            if(abs_i(cx-x)>2 || abs_i(cy-y)>2 || !u[worker].alive || !u[worker].selected)continue;
#ifdef SC64_FIELD_CONTROLS
            if(field_ui_a_action(selected_count())!=UI_A_MINE)continue;
#else
            if(selected_worker_mineral_target()!=target)continue;
#endif
            press_key(KEY_A);
            if(u[worker].alive && u[worker].order==ORDER_MINE && u[worker].target==target) {
                ++assigned;
                fprintf(stderr,"native mineral command worker=%d field=%d frame=%d\n",worker,target,frame);
            }
            break;
        }
    }
    return assigned;
}

#endif
