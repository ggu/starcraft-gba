/* SPDX-License-Identifier: GPL-3.0-or-later */
#define main sc64_battlecruiser_unused_skirmish_test_main
#include "sc64_skirmish_runtime.c"
#undef main
#include <string.h>
extern int sc64_test_sram_transfer(const char *,int);
static void observe(int a,int t,int a2,int t2,int n) {
    const unsigned char *row=(const unsigned char *)sc64_bc_shots;
    printf("%d frame=%d due=%d rng=%u caster=%d,%d,%d,%d,%d/%d,%d,%d,%d,%d target=%d,%d,%d/%d,%d,%d row=",n,frame,
           sc64_battlecruiser_source_tick_due(),sc64_bc_spell_rng,u[a].alive,u[a].order,u[a].work_timer,u[a].carry,u[a].build,
           u[a2].alive,u[a2].order,u[a2].work_timer,u[a2].carry,u[a2].build,
           u[t].alive,u[t].hp,u[t].shield,u[t2].alive,u[t2].hp,u[t2].shield);
    for (unsigned i=0;i<sizeof(sc64_bc_shots);++i) printf("%02x",row[i]);
    for (int i=0;i<2;++i) {
        Sc64BcMotionFields m={0};
        if (sc64_bc_shots[i].state==SC64_BC_SHOT_FLIGHT) assert(sc64_bc_motion_unpack(&sc64_bc_shots[i],&m));
        printf(" motion%d=%u,%u,%u,%u,%u,%u,%u,%u,%u,%u",i,m.x_q8,m.y_q8,
               m.destination_x,m.destination_y,m.speed,m.turn_rate,m.heading,
               m.velocity_direction,m.lifetime,m.turning);
    }
    printf(" impact=%u:",race_impact_count);
    for (unsigned i=0;i<race_impact_count*sizeof(Sc64ImpactEvent);++i)
        printf("%02x",((const unsigned char *)race_impact_rows)[i]);
    putchar('\n');
}
int main(int argc,char **argv) {
    int a,t,a2,t2; assert(argc==3); game_init(); race_mode=1; race_human=R_TER; race_bots=1; init_state();
    if (!strcmp(argv[1],"write")) {
        a=spawn_unit(ENEMY,unit_raw(12),500,500);t=spawn_unit(SIDE,unit_raw(0),640,500);
        a2=spawn_unit(ENEMY,unit_raw(12),500,600);t2=spawn_unit(SIDE,unit_raw(0),640,600);
        assert(a>=0 && t>=0 && a2>=0 && t2>=0);u[a].work_timer=u[a2].work_timer=64<<3;u[a].carry=u[a2].carry=200*256;
        sc64_battlecruiser_tech_bits=(u16)(SC64_BC_TECH_YAMATO<<(2*ENEMY));
        assert(race_bc_yamato_issue(a,t) && race_bc_yamato_issue(a2,t2));
        if (!sc64_battlecruiser_source_tick_due()) ++frame;
        assert(race_bc_yamato_tick_caster(a) && race_bc_yamato_tick_caster(a2));
        screen=SCREEN_PLAY;
        for(int n=0;n<12;++n) game_update(0,0);
        assert(sc64_bc_shots[0].state==SC64_BC_SHOT_CHARGING && sc64_bc_shots[0].age<44);
        race_impact_launch(t,1,1,ENEMY,0,u[a].type,200);
        assert(race_impact_count==1);
        assert(race_save_game() && sc64_test_sram_transfer(argv[2],1));
    } else {
        assert(!strcmp(argv[1],"read") && sc64_test_sram_transfer(argv[2],0) && race_load_game());
        assert(race_impact_count==1);
        a=sc64_bc_shots[0].caster;t=sc64_bc_shots[0].target;
        a2=sc64_bc_shots[1].caster;t2=sc64_bc_shots[1].target;
    }
    assert(a>=0 && a<MAX_UNITS && t>=0 && t<MAX_UNITS && u[t].alive &&
           a2>=0 && a2<MAX_UNITS && t2>=0 && t2<MAX_UNITS && u[t2].alive);
    screen=SCREEN_PLAY;observe(a,t,a2,t2,0);
    int launched=0;
    for(int n=1;n<=150 && (sc64_bc_shots[0].state||sc64_bc_shots[1].state);++n) {
        game_update(0,0);
        if (!launched && sc64_bc_shots[0].age==44 && sc64_bc_shots[1].age==44) {
            assert(race_bc_yamato_launch_source(0,64,0,0));
            assert(race_bc_yamato_launch_source(1,64,0,0));
            launched=1;
        }
        observe(a,t,a2,t2,n);
    }
    assert(launched && !u[t].alive && !u[t2].alive && !sc64_bc_shots[0].state && !sc64_bc_shots[1].state &&
           !(u[a].work_timer&7) && !(u[a2].work_timer&7));
}
