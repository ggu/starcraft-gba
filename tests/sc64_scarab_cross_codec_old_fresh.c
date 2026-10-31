/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Synthetic current pre-namespace writer; ordinary spawn, no Scarab payload. */
#define main sc64_impact_unused_main
#include "sc64_impact_events_game.c"
#undef main
#include <stdlib.h>
extern int sc64_test_sram_transfer(const char *,int);
int main(int argc,char **argv) {
    assert(argc==3);setup();race_skirmish_map=atoi(argv[2]);race_skirmish_init();
    memset(u,0,sizeof(u));memset(b,0,sizeof(b));minerals=500;
    int parent=spawn_unit(SIDE,U_REAVER,128,128);assert(parent==0 && u[parent].carry==0);
    assert(race_save_game() && sc64_test_sram_transfer(argv[1],1));
    puts("pre-namespace empty Reaver saved");return 0;
}
