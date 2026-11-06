/* SPDX-License-Identifier: GPL-3.0-or-later
 * Authored HOST producer/consumer boundary witness; not guest input. */
#define main q8_flash_original_fixture_main
#include "sc64_q8_flash_game.c"
#undef main
int main(int argc,char **argv)
{
    assert(argc==4);int human=!strcmp(argv[2],"terran")?R_TER:R_ZERG;
    assert(!strcmp(argv[2],"terran") || !strcmp(argv[2],"zerg"));
    start();race_human=human;race_bot_races[0]=R_PROTOSS;init_state();
    if(!strcmp(argv[1],"write")) {
        static u8 previous[131072];memset(device.bytes,0xff,sizeof device.bytes);
        int type=human==R_TER?U_SCV:U_DRONE;
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
        memcpy(previous,device.bytes,sizeof previous);
        int old=race_player[SIDE];race_player[SIDE]=R_PROTOSS;
        assert(!race_save_game() && !memcmp(previous,device.bytes,sizeof previous));
        race_player[SIDE]=old;assert(race_save_game());
        RaceLoadPlan plan={0};
        int side=race_data.players[SIDE],bot=race_data.state[21];
        race_data.players[SIDE]=R_PROTOSS;
        assert(!race_q8_flash_candidate_fractions(&plan,0));
        race_data.players[SIDE]=side;race_data.state[21]=(bot+1)%3;
        assert(!race_q8_flash_candidate_fractions(&plan,0));
        race_data.state[21]=bot;
        file_write(argv[3],device.bytes,sizeof device.bytes);
    } else if(!strcmp(argv[1],"read")) {
        file_read(argv[3],device.bytes,sizeof device.bytes);assert(race_load_game());
        assert(race_mode==1 && race_human==human && race_player[SIDE]==human);
        int type=human==R_TER?U_SCV:U_DRONE,found=-1;
        for(int i=0;i<MAX_UNITS;++i)if(u[i].alive&&u[i].type==type&&u[i].x==300)found=i;
        unsigned hp;assert(found>=0 && sc64_pr_q8_actor_hp(found,&hp));
        assert(hp==(unsigned)u[found].maxhp*256u-128u);
        if(human==R_TER) {
            int building=-1;for(int i=0;i<MAX_BUILDINGS;++i)
                if(b[i].alive && b[i].type==B_COMMAND && b[i].x==360)building=i;
            assert(building>=0 && race_building_can_burn(&b[building]));
            unsigned bhp;assert(sc64_pr_q8_actor_hp(MAX_UNITS+building,&bhp));
            assert(bhp==(unsigned)b[building].maxhp*256u-128u &&
                   ((unsigned)b[building].shield_remainder&255u)==128u &&
                   !sc64_pr_hp_debt[MAX_UNITS+building]);
        }
    } else assert(0);
    puts("authored custom race Flash process: PASS");return 0;
}
