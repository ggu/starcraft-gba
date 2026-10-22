#include "gba.h"
static int f;
void game_init(void){f=0;}
void game_update(u16 held,u16 pressed){(void)held;(void)pressed;f++;}
void game_render(void){gfx_begin(C_PANEL);for(int i=0;i<32;i++)gfx_rect((i%8)*30,(i/8)*24,29,23,(u8)i);gfx_text(12,108,"GBA FRAMEBUFFER / INPUT / SRAM",C_WHITE);gfx_num(12,122,f,C_GOLD);gfx_text(12,139,"DIABLO II + STARCRAFT",C_CYAN);gfx_end();}
int game_self_test(void){return 0;}
