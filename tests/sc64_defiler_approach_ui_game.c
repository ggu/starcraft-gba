/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Synthetic actor, actual input/command/menu handlers; no identity or owner injection. */
#define main previous_defiler_test
#include "sc64_defiler_executor_game.c"
#undef main
static void choose_row(int row) {
 game_update(0,0);game_update(KEY_B,KEY_B);assert(menu_open);
 for(int n=0;n<64 && menu_cursor!=row;n++){game_update(0,0);game_update(KEY_DOWN,KEY_DOWN);}
 assert(menu_cursor==row);game_update(0,0);game_update(KEY_A,KEY_A);assert(!menu_open);
}
static void new_cast(int x,int y) {
 choose_row(field_menu_count_base());assert(mode==MODE_DARK_SWARM_STAGE);
 cx=x;cy=y;game_update(0,0);game_update(KEY_A,KEY_A);assert(mode==MODE_NORMAL);
}
int main(void){
 game_init();race_mode=1;race_human=R_ZERG;race_bots=1;race_skirmish_map=0;race_skirmish_init();
 memset(path_walkable,0xff,sizeof(path_walkable));path_grid_valid=1;
 int c=spawn_unit(SIDE,U_SHARED_DEFILER,160,160);assert(c>=0);test_phase=0;click_cast(c,360,160);
 for(int n=0;n<12;n++)game_update(0,0);
 assert(u[c].x>160 && sc64_defiler_stage[c].intent);choose_row(MENU_STOP);
 int x=u[c].x,y=u[c].y;assert(!sc64_defiler_stage[c].intent);
 for(int n=0;n<10;n++)game_update(0,0);
 assert(u[c].x==x&&u[c].y==y&&!sc64_shared_projectiles.active);
 new_cast(360,160);assert(sc64_defiler_stage[c].intent);
 assert(sc64_defiler_stage[c].motion_age<=1&&!sc64_defiler_stage[c].blocked);
 for(int n=0;n<12;n++)game_update(0,0);assert(u[c].x>x);
 choose_row(MENU_MOVE);assert(mode==MODE_MOVE);cx=100;cy=u[c].y;
 game_update(0,0);game_update(KEY_A,KEY_A);assert(!sc64_defiler_stage[c].intent&&u[c].order==ORDER_MOVE);
 for(int n=0;n<10;n++)game_update(0,0);
 int moved_x=u[c].x;choose_row(field_menu_count_base());assert(mode==MODE_DARK_SWARM_STAGE);
 cx=360;cy=u[c].y;game_update(0,0);game_update(KEY_A,KEY_A);
 assert(!sc64_defiler_stage[c].intent && u[c].order==ORDER_MOVE);
 choose_row(MENU_STOP);assert(u[c].order==ORDER_IDLE);
 x=u[c].x;new_cast(360,u[c].y);assert(sc64_defiler_stage[c].intent);
 assert(sc64_defiler_stage[c].motion_age<=1&&!sc64_defiler_stage[c].blocked);
 for(int n=0;n<12;n++)game_update(0,0);assert(u[c].x>x);
 printf("actual STOP, move replacement, moving-cast refusal and renewed cast pass (before_move=%d)\n",moved_x);
 return 0;
}
