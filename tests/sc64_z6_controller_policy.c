/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Isolated synthetic controller checks. This process never generates a
 * campaign controller tape or claims a mission win. */
#define main sc64_z6_campaign_main
#include "sc64_z6_campaign_run.c"
#undef main

#ifdef SC64_DIRECT_ATTACK
static void controller_reachability_regression(void)
{
    int nx,ny;
    /* This map point lies between a ground actor and the objective but is in
     * impassable terrain. Reachability must be checked from the member. */
    const int centroid_x=1154,centroid_y=1267;
    const int member_x=1831,member_y=1636;
    const int goal_x=1717,goal_y=1712;
    int centroid_reaches=path_next_tile(centroid_x,centroid_y,goal_x,goal_y,&nx,&ny);
    int member_reaches=path_next_tile(member_x,member_y,goal_x,goal_y,&nx,&ny);
    fprintf(stderr,"member reachability regression centroid=%d member=%d flyer=%d\n",
        centroid_reaches,member_reaches,
        controller_attackmove_member_reachable(0,0,goal_x,goal_y,1));
    assert(!centroid_reaches&&member_reaches);
    assert(controller_attackmove_member_reachable(0,0,goal_x,goal_y,1));
}
#endif

static void z6_reaver_retreat_regression(void)
{
    int hero=kerrigan(),slot=-1;
    int old_cloak=race_z6.cloak.active;
    Unit saved_units[MAX_UNITS];
    Building saved_buildings[MAX_BUILDINGS];
    if(hero<0)return;
    for(int i=0;i<MAX_UNITS;i++)if(!u[i].alive){slot=i;break;}
    assert(slot>=0);
    memcpy(saved_units,u,sizeof(saved_units));
    memcpy(saved_buildings,b,sizeof(saved_buildings));
    for(int i=0;i<MAX_UNITS;i++)if(owners_hostile(SIDE,u[i].owner))u[i].alive=0;
    for(int i=0;i<MAX_BUILDINGS;i++)if(owners_hostile(SIDE,b[i].owner))b[i].hp=0;
    race_z6.cloak.active=0;
    u[hero].x=1728;u[hero].y=1461;
    memset(&u[slot],0,sizeof(u[slot]));
    u[slot].alive=1;u[slot].type=U_REAVER;u[slot].raw_type=83;
    u[slot].raw_owner=1;u[slot].owner=1;u[slot].hp=100;
    u[slot].x=1652;u[slot].y=1563;
    assert(z6_point_threatened(hero,1728,1461,48));
    assert(!z6_point_threatened(hero,1824,1370,80));
    u[slot].x=1770;u[slot].y=1420;
    assert(z6_point_threatened(hero,1824,1370,80));
    memcpy(u,saved_units,sizeof(saved_units));
    memcpy(b,saved_buildings,sizeof(saved_buildings));
    race_z6.cloak.active=old_cloak;
    fprintf(stderr,"Reaver retreat regression: initial threat and moving-attacker safe-point invalidation PASS\n");
}

int main(void)
{
    launch();
    assert(kerrigan() >= 0);
#ifdef SC64_DIRECT_ATTACK
    controller_reachability_regression();
#endif
    z6_reaver_retreat_regression();
    return 0;
}
