/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Authored HOST worlds and real journals, never emulator SRAM fixtures. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#ifndef SC64_NYDUS_GAME_SOURCE
#define SC64_NYDUS_GAME_SOURCE "../starcraft/game.c"
#endif
#include SC64_NYDUS_GAME_SOURCE
extern int sc64_test_sram_transfer(const char *, int);

#if !defined(SC64_NYDUS_ENABLED) || !defined(SC64_Z10_RUNTIME) || !defined(SC64_ZP_FULL_STORAGE) || defined(SC64_Z7_DERIVED_SAVE)
#error "Use the prepared late-Zerg compact codec with Nydus retained"
#endif
#ifndef SC64_NYDUS_WIDE_LEGACY_WRITER
_Static_assert(SC_RB_COUNT > 64 && SC_RB_COUNT <= 128,
               "This fixture must exercise the previously rejected type range");
#endif

static int expected_type(int legacy)
{
    if (legacy) return B_FACTORY;
    return SC_RB_COUNT - 1;
}

static void write_world(const char *path, int legacy)
{
    int type=expected_type(legacy);
    gba_init(); game_init();
    race_mode=1; race_human=R_TERRAN; race_bots=1; init_state();
    assert(screen==SCREEN_PLAY && race_mode);
    int slot=spawn_build(SIDE,type,300,300,0);
    assert(slot>=0 && b[slot].type==type);
    b[slot].hp=b[slot].maxhp-13;
    assert(race_save_game());
    assert(sc64_test_sram_transfer(path,1));
    printf("HOST writer type=%d raw=%d journal=%08x\n",type,
           sc64_race_raw_building(type),(unsigned)SAVE_VERSION);
}

static void read_world(const char *path, int legacy)
{
    int type=expected_type(legacy),found=-1;
    gba_init(); game_init();
    assert(sc64_test_sram_transfer(path,0));
    assert(race_load_game() && race_mode && race_human==R_TERRAN);
    for(int i=0;i<MAX_BUILDINGS;++i)
        if(b[i].alive && b[i].owner==SIDE && b[i].type==type &&
           b[i].x==300 && b[i].y==300) {assert(found<0);found=i;}
    assert(found>=0 && b[found].hp==b[found].maxhp-13);
    assert(b[found].maxhp==sc64_race_building_max_hp(type));
    assert(race_save_game()); /* decoded state remains admissible to writer */
    printf("HOST fresh reader type=%d hp=%d: PASS\n",type,b[found].hp);
}

int main(int argc,char **argv)
{
    assert(argc==3 || argc==4);
    int legacy=argc==4 && !strcmp(argv[3],"legacy");
    if(!strcmp(argv[1],"write")) write_world(argv[2],legacy);
    else {assert(!strcmp(argv[1],"read"));read_world(argv[2],legacy);}
    return 0;
}
