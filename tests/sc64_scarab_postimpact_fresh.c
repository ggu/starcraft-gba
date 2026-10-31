/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Synthetic actual paid attack, save and fresh-load continuation. */
static int scarab_fixture_map;
#define SC64_SCARAB_TEST_MAP scarab_fixture_map
#define SC64_SCARAB_ATTACK_ENTRY scarab_attack_main
#include "sc64_scarab_parent_attack_game.c"
extern int sc64_test_sram_transfer(const char *,int);
static unsigned digest(void) {
    unsigned h=2166136261u;
#define MIX(x) do { h=(h^(unsigned)(x))*16777619u; } while(0)
    MIX(frame);MIX(seed);MIX(minerals);MIX(gas);MIX(race_skirmish_map);MIX(race_cols);MIX(race_rows);
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive) {
        Unit *q=&u[i];MIX(i);MIX(q->type);MIX(q->raw_type);MIX(q->owner);MIX(q->x);MIX(q->y);
        MIX(q->hp);MIX(q->maxhp);MIX(q->shield);MIX(q->shield_remainder);MIX(q->armor);
        MIX(q->order);MIX(q->target);MIX(q->tx);MIX(q->ty);MIX(q->carry);MIX(q->work_timer);
        MIX(q->build);MIX(q->cd);MIX(q->weapon);MIX(q->raw_owner);MIX(q->flags);
    }
    for(int i=0;i<MAX_BUILDINGS;i++)if(b[i].alive) {MIX(i);MIX(b[i].type);MIX(b[i].hp);MIX(b[i].owner);}
#undef MIX
    return h;
}
static void snapshot(FILE *f) {
    Unit zero_u={0};Building zero_b={0};
    int meta[]={frame,(int)seed,minerals,gas,race_skirmish_map,race_cols,race_rows};
    assert(fwrite(meta,sizeof(meta),1,f)==1);
    for(int i=0;i<MAX_UNITS;i++)assert(fwrite(u[i].alive?&u[i]:&zero_u,sizeof(Unit),1,f)==1);
    for(int i=0;i<MAX_BUILDINGS;i++)assert(fwrite(b[i].alive?&b[i]:&zero_b,sizeof(Building),1,f)==1);
    assert(fwrite(race_player,sizeof(race_player),1,f)==1);
    assert(fwrite(race_minerals,sizeof(race_minerals),1,f)==1);
    assert(fwrite(race_gas,sizeof(race_gas),1,f)==1);
    assert(fwrite(m,sizeof(m),1,f)==1);
}
int main(int argc,char **argv) {
    assert(argc==4 || argc==5);scarab_fixture_map=atoi(argv[3]);assert(scarab_fixture_map>=0 && scarab_fixture_map<=1);
    if(!strcmp(argv[1],"write")) {
        char *args[]={argv[0],"building",0};assert(scarab_attack_main(2,args)==0);
        int dead=-1;for(int i=0;i<MAX_UNITS;i++)if(!u[i].alive && u[i].type==U_SCARAB && u[i].raw_type==85){assert(dead<0);dead=i;}
        assert(dead>=0 && (((unsigned)u[dead].carry>>8)&3u)==2u && !race_scarab_probe_bullets_pending());
        Unit before[MAX_UNITS];Building before_b[MAX_BUILDINGS];memcpy(before,u,sizeof(u));memcpy(before_b,b,sizeof(b));
        unsigned pre=digest();assert(race_save_game());assert(pre==digest() && !memcmp(before,u,sizeof(u)) && !memcmp(before_b,b,sizeof(b)));
        assert(sc64_test_sram_transfer(argv[2],1));
    } else {
        assert(!strcmp(argv[1],"read"));setup();assert(sc64_test_sram_transfer(argv[2],0));assert(race_load_game());assert(race_skirmish_map==scarab_fixture_map);
        assert(sc64_test_sram_transfer(argv[2],1)); /* export what load left in SRAM */
        for(int i=0;i<MAX_UNITS;i++)assert(u[i].alive || u[i].type!=U_SCARAB);
    }
    FILE *snap=argc==5?fopen(argv[4],"wb"):0;assert(argc==4 || snap);
    if(snap)snapshot(snap);printf("saved=%08x ",digest());
    for(int n=0;n<50;n++){screen=SCREEN_PLAY;game_update(0,0);}
    if(snap){snapshot(snap);assert(!fclose(snap));}
    printf("continued=%08x\n",digest());return 0;
}
