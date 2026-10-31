/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Actual paid attack followed by save with a dead, unreused outside row. */
#define SC64_SCARAB_ATTACK_ENTRY scarab_attack_main
#include "sc64_scarab_parent_attack_game.c"
extern int sc64_test_sram_transfer(const char *,int);
int main(int argc,char **argv)
{
    assert(argc==4);char *args[]={argv[0],"phase0",0};assert(scarab_attack_main(2,args)==0);
    int dead=-1;
    for(int i=0;i<MAX_UNITS;i++)if(!u[i].alive && u[i].type==U_SCARAB && u[i].raw_type==85) {assert(dead<0);dead=i;}
    assert(dead>=0 && (((unsigned)u[dead].carry>>8)&3u)==2u && !race_scarab_probe_bullets_pending());
    assert(sc64_test_sram_transfer(argv[1],1));assert(!race_save_game());
    assert(sc64_test_sram_transfer(argv[2],1));
    int reused=spawn_unit(SIDE,U_PROBE,128,128);assert(reused==dead);
    assert(race_save_game() && sc64_test_sram_transfer(argv[3],1));
    puts("post-impact save rejected until dead Scarab slot reused");return 0;
}
