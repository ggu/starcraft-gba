/* Diagnostic only: observe current collection/deposit ordering, not a stock-timing oracle. */
#define main worker_gather_fixture_main
#include "sc64_worker_gather_game.c"
#undef main
int main(void) {
 reset_scene(R_PROTOSS,R_TER);
 int w=spawn_unit(SIDE,U_PROBE,128,96); assert(w>=0);
 set_depot(0,SIDE,B_NEXUS,96,96);
 m[2].x=134; m[2].y=96; m[2].amount=100;
 u[w].order=ORDER_MINE;u[w].target=2;u[w].build=0;u[w].work_timer=0;
 for(int call=1;call<=30;call++) {
  one_tick();
  printf("call=%d carry=%d timer=%u bank=%d remaining=%d order=%d\n",call,u[w].carry,(unsigned)u[w].work_timer,minerals,m[2].amount,u[w].order);
 }
 return 0;
}
