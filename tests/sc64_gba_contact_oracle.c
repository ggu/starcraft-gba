/* SPDX-License-Identifier: GPL-3.0-or-later
 * Compare actual GBA contact on the source oracle's 2:1 coordinate scene. */
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"
int main(void) {
 gba_init();game_init();memset(u,0,sizeof(u));
 u[0].alive=1;u[0].type=U_ZP_HIGH_TEMPLAR;u[0].raw_type=67;u[0].x=100;u[0].y=100;u[0].owner=0;u[0].order=ORDER_IDLE;
 u[1].alive=1;u[1].type=U_ZP_RAYNOR_VULTURE;u[1].raw_type=19;u[1].x=114;u[1].y=100;u[1].owner=1;u[1].order=ORDER_IDLE;
 sc64_unit_buckets_begin();
 printf("approach,%d\n",!unit_step_clear(&u[0],101,100));
 printf("escape,%d\n",!unit_step_clear(&u[0],99,100));
 sc64_unit_buckets_end();return 0;
}
