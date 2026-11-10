/* SPDX-License-Identifier: GPL-3.0-or-later
 * Authored HOST producer/consumer boundary witness; not guest input. */
#define SC64_Q8_FLASH_NO_FIXTURE_MAIN
#include "sc64_q8_flash_game.c"
/* Alter a real serialized row and repair its checksum. This is a negative
 * HOST fixture, not a compatibility producer or a guest-generated save. */
static void reject_bad_roster(unsigned offset,unsigned value)
{
    static u8 good[131072],bad[131072],debts[MAX_UNITS+MAX_BUILDINGS];
    static Unit units[MAX_UNITS];static Building buildings[MAX_BUILDINGS];
    static int players[5],ore[5],vespene[5];
    GbaFlashJournalRecord record;
    assert(gba_fj_probe(&flash_io,2,&record)==GBA_FJ_OK && record.tag==SC64_Q8_FLASH_TAG);
    assert(offset+sizeof(u32)<=sizeof(RaceSave) && record.length>sizeof(RaceSave));
    memcpy(good,device.bytes,sizeof good);
    u32 payload=gba_fj_metadata(2)+16u;
    gba_fj_put32(device.bytes+payload+offset,value);
    gba_fj_put32(device.bytes+gba_fj_metadata(2)+8u,
                 gba_fj_hash(2166136261u,device.bytes+payload,record.length));
    assert(gba_fj_probe(&flash_io,2,&record)==GBA_FJ_OK);
    memcpy(bad,device.bytes,sizeof bad);memcpy(units,u,sizeof u);memcpy(buildings,b,sizeof b);
    memcpy(debts,sc64_pr_hp_debt,sizeof debts);memcpy(players,race_player,sizeof players);
    memcpy(ore,race_minerals,sizeof ore);memcpy(vespene,race_gas,sizeof vespene);
    int saved_minerals=minerals,saved_gas=gas,saved_mode=race_mode,saved_human=race_human;
    assert(!race_load_game());
    assert(!race_save_game()); /* sole invalid committed record cannot start a new epoch */
    assert(!memcmp(bad,device.bytes,sizeof bad) && !memcmp(units,u,sizeof u) &&
           !memcmp(buildings,b,sizeof b) && !memcmp(debts,sc64_pr_hp_debt,sizeof debts) &&
           !memcmp(players,race_player,sizeof players) && !memcmp(ore,race_minerals,sizeof ore) &&
           !memcmp(vespene,race_gas,sizeof vespene) && minerals==saved_minerals && gas==saved_gas &&
           race_mode==saved_mode && race_human==saved_human);
    memcpy(device.bytes,good,sizeof good);
}
static int race_name(const char *name)
{
    if(!strcmp(name,"terran"))return R_TER;
    if(!strcmp(name,"zerg"))return R_ZERG;
    if(!strcmp(name,"protoss"))return R_PROTOSS;
    assert(0);return -1;
}
static int race_letter(char letter)
{
    if(letter=='t')return R_TER;
    if(letter=='z')return R_ZERG;
    if(letter=='p')return R_PROTOSS;
    assert(0);return -1;
}
static int first_proto_bot(const char *bots)
{
    static const int owners[3]={ENEMY,RESCUABLE,4};
    for(unsigned i=0;bots[i];++i)if(bots[i]=='p')return owners[i];
    return -1;
}
int main(int argc,char **argv)
{
    if(argc==2 && !strcmp(argv[1],"catalog")) {
        printf("%d\n",SC64_SKIRMISH_IMPORTED_COUNT+1);return 0;
    }
    assert(argc==4 || argc==5 || argc==6);int human=race_name(argv[2]);
    const char *bots=argc>=5?argv[3]:"p",*path=argv[argc-1];
    int map=0;if(argc==6) {assert(strlen(argv[4])==1);map=argv[4][0]-'0';}
    assert(map>=0 && map<=SC64_SKIRMISH_IMPORTED_COUNT);
    unsigned count=(unsigned)strlen(bots);assert(count>=1 && count<=3);
    start();race_human=human;race_bots=(int)count;race_skirmish_map=map;
    for(unsigned i=0;i<count;++i)race_bot_races[i]=race_letter(bots[i]);
    init_state();
    if(!strcmp(argv[1],"write")) {
        static u8 previous[131072];memset(device.bytes,0xff,sizeof device.bytes);
        int type=human==R_TER?U_SCV:human==R_ZERG?U_DRONE:U_PROBE;
        int actor=spawn_unit(SIDE,type,300,300);unsigned hp;
        assert(actor>=0 && sc64_pr_q8_actor_hp(actor,&hp) && hp>128);
        assert(sc64_pr_q8_commit_hp(actor,hp-128));
        if(human==R_TER) {
            int building=spawn_build(SIDE,B_COMMAND,360,360,0);
            assert(building>=0 && race_building_can_burn(&b[building]));
            unsigned bhp;assert(sc64_pr_q8_actor_hp(MAX_UNITS+building,&bhp));
            assert(sc64_pr_q8_commit_hp(MAX_UNITS+building,bhp-128));
            assert(((unsigned)b[building].shield_remainder&255u)==128u &&
                   !sc64_pr_hp_debt[MAX_UNITS+building]);
        }
        /* Authored payer setup exercises the real job and codec. It is not
         * evidence that the AI's decision loop requests research itself. */
        int payer=first_proto_bot(bots);
        if(payer>=0) {
            race_minerals[payer]=race_gas[payer]=500;
            int pylon=spawn_build(payer,B_PYLON,400,400,0);
            int forge=spawn_build(payer,B_FORGE,440,400,0);
            assert(pylon>=0 && forge>=0 &&
                   sc64_pr_stage_begin_slot(forge,SC64_PR_GROUND_WEAPONS));
            assert(race_minerals[payer]==400 && race_gas[payer]==400 &&
                   b[forge].queue_progress==5000);
        }
        memcpy(previous,device.bytes,sizeof previous);
        int bad_human=(human+1)%3;
        int old=race_player[SIDE];race_player[SIDE]=bad_human;
        assert(!race_save_game() && !memcmp(previous,device.bytes,sizeof previous));
        race_player[SIDE]=old;assert(race_save_game());
        RaceLoadPlan plan={0};
        int side=race_data.players[SIDE],bot=race_data.state[21];
        race_data.players[SIDE]=bad_human;
        assert(!race_q8_flash_candidate_fractions(&plan,0));
        race_data.players[SIDE]=side;race_data.state[21]=(bot+1)%3;
        assert(!race_q8_flash_candidate_fractions(&plan,0));
        race_data.state[21]=bot;
        reject_bad_roster((unsigned)offsetof(RaceSave,players)+SIDE*sizeof(int),(unsigned)bad_human);
        reject_bad_roster((unsigned)offsetof(RaceSave,state)+21u*sizeof(int),(unsigned)((bot+1)%3));
        if(count>1) {
            unsigned last=count-1;
            reject_bad_roster((unsigned)offsetof(RaceSave,state)+(21u+last)*sizeof(int),
                             (unsigned)((race_letter(bots[last])+1)%3));
        }
        file_write(path,device.bytes,sizeof device.bytes);
    } else if(!strcmp(argv[1],"read") || !strcmp(argv[1],"read-legacy")) {
        int legacy=!strcmp(argv[1],"read-legacy");
        static const int owners[3]={ENEMY,RESCUABLE,4};
        file_read(path,device.bytes,sizeof device.bytes);assert(race_load_game());
        assert(race_mode==1 && race_human==human && race_player[SIDE]==human &&
               race_bots==(int)count && race_skirmish_map==map);
        for(unsigned i=0;i<count;++i)
            assert(race_bot_races[i]==race_letter(bots[i]) && race_player[owners[i]]==race_letter(bots[i]));
        int type=human==R_TER?U_SCV:human==R_ZERG?U_DRONE:U_PROBE,found=-1;
        for(int i=0;i<MAX_UNITS;++i)if(u[i].alive&&u[i].type==type&&u[i].x==300)found=i;
        unsigned hp;assert(found>=0 && sc64_pr_q8_actor_hp(found,&hp));
        assert(hp==(unsigned)u[found].maxhp*256u-(legacy?256u:128u));
        if(human==R_TER) {
            int building=-1;for(int i=0;i<MAX_BUILDINGS;++i)
                if(b[i].alive && b[i].type==B_COMMAND && b[i].x==360)building=i;
            assert(building>=0 && race_building_can_burn(&b[building]));
            unsigned bhp;assert(sc64_pr_q8_actor_hp(MAX_UNITS+building,&bhp));
            assert(bhp==(unsigned)b[building].maxhp*256u-(legacy?256u:128u) &&
                   ((unsigned)b[building].shield_remainder&255u)==(legacy?0u:128u) &&
                   !sc64_pr_hp_debt[MAX_UNITS+building]);
        }
        int payer=first_proto_bot(bots);
        if(payer>=0 && !legacy) {
            int forge=-1;Sc64PrPaidReceipt paid;
            for(int i=0;i<MAX_BUILDINGS;++i)
                if(b[i].alive && b[i].owner==payer && b[i].type==B_FORGE && b[i].x==440)forge=i;
            assert(forge>=0 && sc64_pr_stage_receipt(forge,&paid) && paid.payer==payer &&
                   b[forge].queue_progress==5000 && race_minerals[payer]==400 && race_gas[payer]==400);
            sc64_pr_stage_tick();
            assert(b[forge].queue_progress==4999 && race_minerals[payer]==400 && race_gas[payer]==400);
        }
    } else assert(0);
    puts("authored custom race Flash process: PASS");return 0;
}
