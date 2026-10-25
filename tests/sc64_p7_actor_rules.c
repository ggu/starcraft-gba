#include <assert.h>
#include <stdio.h>
#include "race_rules.h"
#include "sc64_zp_actor_rules.h"

int main(void)
{
#ifdef SC64_P7_ACTORS
    static const int au[][23]={
        {10,350,0,0,0,300,256,16,16,15,15,0,2,30,3,20,64,1,30,3,20,64,1},
        {200,150,1,100,350,2400,288,22,22,21,21,1,2,10,1,45,160,1,10,1,45,160,1},
        {300,150,4,350,250,2100,352,32,32,31,31,1,2,0,0,0,0,0,0,0,0,0,0},
        {40,20,0,25,75,600,288,16,16,15,15,1,0,0,0,0,0,0,0,0,0,0,0}};
    static const unsigned af[]={0x1c000100u,0x5a200004u,0xdc000004u,0x5840c004u};
    static const int av[][14]={
        {250,250,1,50,100,450,320,3,2,44,16,44,28,2},
        {450,450,1,150,100,900,320,3,2,24,24,40,24,2},
        {500,500,1,150,200,900,320,3,2,32,24,32,24,2},
        {500,500,1,300,200,900,320,3,2,40,32,47,24,2},
        {500,500,1,200,150,900,320,3,2,44,28,44,28,2},
        {450,450,1,150,100,450,320,3,2,32,32,32,20,2},
        {200,200,1,100,0,450,320,3,2,32,16,32,16,2}};
    static const unsigned bf[]={0x44080001u,0x44080001u,0x44080001u,0x44080001u,
        0x44080001u,0x44080001u,0x44280001u};
#endif
    int i;
#ifndef SC64_P7_ACTORS
    static const int absent_units[]={68,71,72,84};
    static const int absent_buildings[]={159,163,165,169,170,171,172};
    assert(SC_RU_COUNT==33 && SC_RB_COUNT==41);
    for(i=0;i<4;i++) assert(sc64_race_unit_from_raw(absent_units[i])==-1);
    for(i=0;i<7;i++) assert(sc64_race_building_from_raw(absent_buildings[i])==-1);
    puts("P7 off: prior enums and raw rejection preserved");
    return 0;
#else
    static const int ur[]={68,71,72,84};
    static const int ut[]={U_P7_ARCHON,U_P7_ARBITER,U_P7_CARRIER,U_P7_OBSERVER};
    static const int br[]={159,163,165,169,170,171,172};
    static const int bt[]={B_P7_OBSERVATORY,B_P7_CITADEL_ADUN,B_P7_TEMPLAR_ARCHIVES,
        B_P7_FLEET_BEACON,B_P7_ARBITER_TRIBUNAL,B_P7_ROBOTICS_SUPPORT_BAY,
        B_P7_SHIELD_BATTERY};
    static const int weapons[]={70,77,130,130};
    static const int supply[]={8,8,12,2};
    static const unsigned gaps[]={SC64_P7_NEEDS_ARCHON_RADIAL_SPLASH,
        SC64_P7_NEEDS_ARBITER_STASIS_AND_CLOAK_FIELD,
        SC64_P7_NEEDS_CARRIER_INTERCEPTORS,SC64_P7_NEEDS_OBSERVER_DETECTION_AND_CLOAK};
    for(i=0;i<4;i++) {
        const Sc64P7ActorFact *p;
        const Sc64ZpActorFact *f;
        const Sc64RaceUnitRule *r;
        assert(sc64_race_unit_from_raw(ur[i])==ut[i] && sc64_race_raw_unit(ut[i])==ur[i]);
        p=sc64_p7_actor_fact(ut[i]); f=sc64_zp_actor_fact(ut[i]);
        assert(p && f==&p->fact && f->raw==ur[i]);
        assert(f->hp==au[i][0] && f->shields==au[i][1] && f->armor==au[i][2]);
        assert(f->minerals==au[i][3] && f->gas==au[i][4] && f->build_frames==au[i][5]);
        assert(f->sight_px==au[i][6] && f->left==au[i][7] && f->up==au[i][8]);
        assert(f->right==au[i][9] && f->down==au[i][10] && f->flying==au[i][11]);
        assert(f->size==au[i][12] && f->ground_damage==au[i][13]);
        assert(f->ground_bonus==au[i][14] && f->ground_cooldown==au[i][15]);
        assert(f->ground_range==au[i][16] && f->ground_hits==au[i][17]);
        assert(f->air_damage==au[i][18] && f->air_bonus==au[i][19]);
        assert(f->air_cooldown==au[i][20] && f->air_range==au[i][21] && f->air_hits==au[i][22]);
        if(p->source_flags!=af[i] || p->source_ground_weapon!=weapons[i])
            fprintf(stderr,"actor %d flags %08x expected %08x weapon %d expected %d\n",
                    i,p->source_flags,af[i],p->source_ground_weapon,weapons[i]);
        assert(p->source_flags==af[i] && p->source_ground_weapon==weapons[i]);
        assert(p->source_air_weapon==weapons[i] && p->supply_required==supply[i]);
        assert(p->ground_splash_inner==(i==0?3:0));
        assert(p->ground_splash_mid==(i==0?15:0));
        assert(p->ground_splash_outer==(i==0?30:0));
        assert(p->air_splash_inner==(i==0?3:0));
        assert(p->air_splash_mid==(i==0?15:0));
        assert(p->air_splash_outer==(i==0?30:0));
        assert(p->can_move==1 && p->can_attack==(i==3?0:1));
        assert(p->required_capabilities==gaps[i] && p->implemented_capabilities==0);
        r=sc64_race_unit_rule(ut[i]);
        assert(r && r->race==R_PROTOSS && r->producer==255);
        assert(r->flags==SC_RACE_F_SHIELDS && r->supply==supply[i]);
        assert(r->minerals==au[i][3] && r->gas==au[i][4]);
        assert(r->build_frames==au[i][5] && r->hp==au[i][0] && r->shields==au[i][1]);
        if(i>=2) assert(!(f->flags&SC64_ZP_ACTOR_WEAPON));
    }
    assert(sc64_p7_actor_fact(U_P7_OBSERVER)->source_flags & SC64_P7_SRC_DETECTOR);
    assert(sc64_p7_actor_fact(U_P7_OBSERVER)->source_flags & SC64_P7_SRC_PERMA_CLOAK);
    for(i=0;i<7;i++) {
        const Sc64P7BuildingFact *p;
        const Sc64ZpBuildingFact *f;
        const Sc64RaceBuildingRule *r;
        assert(sc64_race_building_from_raw(br[i])==bt[i] && sc64_race_raw_building(bt[i])==br[i]);
        p=sc64_p7_building_fact(bt[i]); f=sc64_zp_building_fact(bt[i]);
        assert(p && f==&p->fact && f->raw==br[i]);
        assert(f->hp==av[i][0] && f->shields==av[i][1] && f->armor==av[i][2]);
        assert(f->minerals==av[i][3] && f->gas==av[i][4] && f->build_frames==av[i][5]);
        assert(f->sight_px==av[i][6] && f->width_tiles==av[i][7] && f->height_tiles==av[i][8]);
        assert(f->left==av[i][9] && f->up==av[i][10] && f->right==av[i][11]);
        assert(f->down==av[i][12] && f->size==av[i][13] && p->source_flags==bf[i]);
        assert(p->implemented_capabilities==0);
        r=sc64_race_building_rule(bt[i]);
        assert(r && r->race==R_PROTOSS && r->worker==255 && !r->supply_provided);
        assert(r->flags==(SC_RACE_F_BUILDING|SC_RACE_F_SHIELDS|SC_RACE_F_REQUIRES_POWER));
        assert(r->minerals==av[i][3] && r->gas==av[i][4]);
        assert(r->build_frames==av[i][5] && r->hp==av[i][0]);
    }
    assert(sc64_p7_actor_fact(U_P7_CARRIER)->can_attack==1);
    assert(sc64_p7_actor_fact(U_P7_CARRIER)->source_ground_weapon==130);
    assert(sc64_p7_actor_fact(U_P7_ARCHON)->required_capabilities==
           SC64_P7_NEEDS_ARCHON_RADIAL_SPLASH);
#ifdef SC64_P6_ACTORS
    assert(sc64_race_unit_from_raw(74)==U_P6_HERO_DARK_TEMPLAR);
#endif
    assert(sc64_p7_actor_fact(-1)==0 && sc64_p7_actor_fact(SC_RU_COUNT)==0);
    assert(sc64_p7_building_fact(-1)==0 && sc64_p7_building_fact(SC_RB_COUNT)==0);
    printf("ENUMS:%d,%d\n",SC_RU_COUNT,SC_RB_COUNT);
    puts("P7 actor facts verified");
    return 0;
#endif
}
