/* SPDX-License-Identifier: GPL-3.0-or-later
 * Authored setup compiled against the exact pinned predecessor game sources.
 * It exports the actual old encoder's bytes; no tag or checksum is rewritten. */
#ifndef SC64_Q8_FLASH_BASELINE
#error "This producer must use the qualified predecessor sources"
#endif
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <gba.c>
#include <game.c>
static int letter(char c)
{if(c=='t')return R_TER;if(c=='z')return R_ZERG;assert(c=='p');return R_PROTOSS;}
int main(int argc,char **argv)
{
    assert(argc==5);int human=!strcmp(argv[1],"terran")?R_TER:
        !strcmp(argv[1],"zerg")?R_ZERG:R_PROTOSS;
    assert(!strcmp(argv[1],"terran") || !strcmp(argv[1],"zerg") || !strcmp(argv[1],"protoss"));
    unsigned bots=(unsigned)strlen(argv[2]);assert(bots>=1 && bots<=3 && strlen(argv[3])==1);
    int map=argv[3][0]-'0';assert(map>=0 && map<=SC64_SKIRMISH_IMPORTED_COUNT);
    gba_init();game_init();race_mode=1;race_human=human;race_bots=(int)bots;race_skirmish_map=map;
    for(unsigned i=0;i<bots;++i)race_bot_races[i]=letter(argv[2][i]);
    init_state();memset(host_save,0xff,sizeof host_save);
    int type=human==R_TER?U_SCV:human==R_ZERG?U_DRONE:U_PROBE;
    int actor=spawn_unit(SIDE,type,300,300);assert(actor>=0 && u[actor].hp>1);--u[actor].hp;
    if(human==R_TER) {
        int building=spawn_build(SIDE,B_COMMAND,360,360,0);assert(building>=0 && b[building].hp>1);
        --b[building].hp;
    }
    assert(race_save_game());
    /* Widen the erased device backing, preserving the complete old SRAM
     * verbatim in bank zero. This is not a game-codec conversion. */
    static unsigned char image[131072];memset(image,0xff,sizeof image);
    assert(sizeof host_save==32768);memcpy(image,host_save,sizeof host_save);
    FILE *out=fopen(argv[4],"wb");assert(out && fwrite(image,1,sizeof image,out)==sizeof image && !fclose(out));
    puts("authored predecessor roster writer: PASS");return 0;
}
