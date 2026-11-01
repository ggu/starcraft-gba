/* SPDX-License-Identifier: GPL-3.0-or-later */
#define game_init probe_engine_init
#define game_update probe_engine_update
#include "../starcraft/game.c"
#undef game_init
#undef game_update
volatile u32 probe_tick,probe_fault,probe_first_move,probe_first_hit,probe_worker_ticks,probe_worker_units;
static int a,t,building,start_hp;
void game_init(void) {
 probe_engine_init(); init_state(); memset(u,0,sizeof(u)); memset(b,0,sizeof(b));
 memset(race_player,0xff,sizeof(race_player)); race_player[SIDE]=race_player[ENEMY]=R_ZERG;
 race_mode=1; race_human=R_ZERG; screen=SCREEN_PLAY;
 if(spawn_build(SIDE,B_COMMAND,100,100,0)<0)probe_fault|=1;
 building=spawn_build(ENEMY,B_COMMAND,40*16+8,20*16+8,0);
 a=spawn_unit(SIDE,U_TANK,20*16+8,20*16+8);
 if(spawn_unit(SIDE,U_DRONE,43*16+8,20*16+8)<0 || building<0 || a<0){probe_fault|=2;return;}
 t=MAX_UNITS+building; u[a].hp=u[a].maxhp=65535; b[building].hp=b[building].maxhp=65535;start_hp=b[building].hp;
 memset(path_walkable,0xff,sizeof(path_walkable));path_grid_valid=1;
 for(int row=0;row<MAP_ROWS;row++)if(row!=10)SC64_GRID_SET(path_walkable,30+row*MAP_COLS,0);
 memset(visible,0xff,sizeof(visible));
 {if(!currently_visible(target_x(t),target_y(t)) || direct_path_clear(u[a].x,u[a].y,target_x(t),target_y(t)))probe_fault|=4;}
 u[a].selected=1;if(sc64_direct_attack_issue_target(t)!=1)probe_fault|=8;u[a].selected=0;
 sc64_routes_reset();sc64_direct_firing_cache_invalidate();
#ifdef SC64_RANGE_WORK
 sc64_range_route_reset();
#endif
}
void game_update(u16 held,u16 pressed){
 if(probe_fault)return;int x=u[a].x,y=u[a].y;++probe_tick;
 probe_engine_update(held,pressed);
 if(screen!=SCREEN_PLAY || frame!=(int)probe_tick || !path_grid_valid || SC64_GRID_GET(path_walkable,30+20*MAP_COLS) || !SC64_GRID_GET(path_walkable,30+10*MAP_COLS))probe_fault|=16;
 if(!u[a].alive || !b[building].alive || u[a].order!=ORDER_ATTACK_TARGET || u[a].target!=t || !currently_visible(target_x(t),target_y(t)))probe_fault|=32;
 if(!probe_first_move && (x!=u[a].x || y!=u[a].y))probe_first_move=probe_tick;
 if(!probe_first_hit && b[building].hp<start_hp)probe_first_hit=probe_tick;
#ifdef SC64_RANGE_WORK

#endif
}
