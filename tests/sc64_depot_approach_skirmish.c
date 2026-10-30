/* Deterministic regression using ordinary game inputs on Blood Bath.
 * The mineral scenario does not inject actors, resources, terrain or controller
 * triggers. A separate synthetic loaded-gas trip follows that scenario. */
#define main skirmish_original_main
#include "sc64_skirmish_campaign_run.c"
#undef main

static int depot_field_cached(int home)
{
    for(int i=0;i<SC64_ROUTE_ACTIVE_SLOTS;i++)
        if(sc64_route_valid[i] && sc64_route_goal[i]==(SC64_ROUTE_DEPOT_TAG | (unsigned)home)) return 1;
    return 0;
}

static int issue_reproduced_gateway(void)
{
    for(int attempt=0;attempt<8 && screen==SCREEN_PLAY;attempt++) {
        int worker=-1;
        for(int i=0;i<MAX_UNITS;i++) if(build_worker_available(i,SIDE,U_PROBE)) {worker=i;break;}
        if(worker<0 || !select_unit(worker) || !choose_menu_command(MENU_BARRACKS) ||
           mode!=MODE_BUILD || menu!=B_GATEWAY) return 0;
        move_cursor(96,120);
        tap(KEY_A);
        if(mode==MODE_NORMAL) return 1;
        tap(KEY_B); if(menu_open) tap(KEY_B);
        for(int i=0;i<60;i++) input(0);
    }
    return 0;
}

int main(void) {
    option_map=1; option_human=R_PROTOSS; option_bots=3;
    option_bot_races[0]=R_TER; option_bot_races[1]=R_ZERG; option_bot_races[2]=R_PROTOSS;
    match_limit=10000; gba_init(); assert(launch_match());
    while(host_ticks<1200) input(0);
    assert(issue_reproduced_gateway());
    int gateway=-1;
    while(host_ticks<4000 && gateway<0 && screen==SCREEN_PLAY) {
        input(0);
        for(int i=0;i<MAX_BUILDINGS;i++)
            if(b[i].alive && b[i].owner==SIDE && b[i].type==B_GATEWAY &&
               b[i].x==96 && b[i].y==120) {gateway=i;break;}
    }
    assert(gateway>=0);
    int home=owner_command_center(SIDE),worker=-1,nx,ny,approach=0;
    assert(home>=0 && b[home].type==B_NEXUS);
    while(host_ticks<6000 && worker<0 && screen==SCREEN_PLAY) {
        input(0);
        for(int i=0;i<MAX_UNITS;i++) if(u[i].alive && u[i].owner==SIDE && u[i].type==U_PROBE &&
                u[i].order==ORDER_MINE && u[i].carry>0 && u[i].path_timer>0 &&
                (u[i].path_x!=u[i].x || u[i].path_y!=u[i].y) &&
                dist2(u[i].x,u[i].y,b[home].x,b[home].y)>32*32) {
            if(!path_next_tile(u[i].x,u[i].y,b[home].x,b[home].y,&nx,&ny) &&
                path_next_tile(u[i].x,u[i].y,b[home].x,b[home].y-32,&nx,&ny)) {worker=i;approach=1;break;}
        }
    }
    assert(worker>=0 && approach);
    /* Exercise a real codec save while a loaded worker is approaching. */
    assert(u[worker].path_timer>0 &&
           (u[worker].path_x!=u[worker].x || u[worker].path_y!=u[worker].y));
    assert(worker_depot_next(u[worker].x,u[worker].y,home,&nx,&ny));
    assert(depot_field_cached(home));
    unsigned before_load_builds=sc64_route_builds;
    assert(save_game());
    input(0);
    Unit expected=u[worker];
    assert(load_game());
    assert(!depot_field_cached(home) || sc64_route_builds>before_load_builds);
    input(0);
    assert(u[worker].x==expected.x && u[worker].y==expected.y &&
           u[worker].carry==expected.carry && u[worker].order==expected.order &&
           u[worker].target==expected.target && u[worker].path_x==expected.path_x &&
           u[worker].path_y==expected.path_y && u[worker].path_timer==expected.path_timer &&
           u[worker].path_retry==expected.path_retry);
    int x=u[worker].x,y=u[worker].y,carry=u[worker].carry;unsigned start=host_ticks;
    for(int i=0;i<600 && screen==SCREEN_PLAY && u[worker].alive && u[worker].carry;i++) input(0);
    fprintf(stderr,"worker%d return tick%u to%u xy%d,%d->%d,%d cargo%d->%d home%d xy%d,%d gateway%d xy%d,%d\n",worker,start,host_ticks,x,y,u[worker].x,u[worker].y,carry,u[worker].carry,home,b[home].x,b[home].y,gateway,b[gateway].x,b[gateway].y);
    /* Golden behavior: a reachable depot approach must let the Probe return
     * its already-mined cargo; checking cargo avoids confusing another
     * worker's deposits with this trip. */
    assert(u[worker].alive && u[worker].carry==0);
#ifdef SC64_NEAREST_RESOURCE_DEPOT
    /* A farther center-reachable depot must not win over the nearby Nexus
     * whose center route fails but whose deposit circle is reachable. */
    int farther=spawn_build(SIDE,B_NEXUS,240,200,0);
    assert(farther>=0);
    u[worker].x=22; u[worker].y=40; u[worker].build=-1;
    assert(!path_next_tile(u[worker].x,u[worker].y,b[home].x,b[home].y,&nx,&ny));
    assert(worker_depot_next(u[worker].x,u[worker].y,home,&nx,&ny));
    assert(race_nearest_resource_depot(&u[worker])==home);

    /* Synthetic grid: the near depot has exactly one reachable circle cell,
     * (19,18), which contains none of the old eight sampled approach points.
     * The farther depot is reachable too, so selection must not skip near. */
    int hx=b[home].x,hy=b[home].y,fx=b[farther].x,fy=b[farther].y;
    b[home].x=320; b[home].y=320; b[farther].x=400; b[farther].y=264;
    u[worker].x=312; u[worker].y=264; u[worker].build=-1;
    memset(path_walkable,0,sizeof(path_walkable)); path_grid_valid=1; sc64_routes_reset();
    for(int col=19;col<=24;col++) SC64_GRID_SET(path_walkable,col+16*MAP_COLS,1);
    SC64_GRID_SET(path_walkable,19+17*MAP_COLS,1);
    SC64_GRID_SET(path_walkable,19+18*MAP_COLS,1);
    int goal_px,goal_py;
    assert(sc64_depot_cell_point(320,320,19,18,&goal_px,&goal_py));
    static const int old_dx[8]={0,32,0,-32,22,22,-22,-22};
    static const int old_dy[8]={-32,0,32,0,-22,22,22,-22};
    for(int i=0;i<8;i++) assert((320+old_dx[i])/16!=19 || (320+old_dy[i])/16!=18);
    assert(worker_depot_next(u[worker].x,u[worker].y,farther,&nx,&ny));
    assert(race_nearest_resource_depot(&u[worker])==home);
    b[home].x=hx; b[home].y=hy; b[farther].x=fx; b[farther].y=fy;
    invalidate_path_grid();
#endif
    /* Synthetic setup, isolated after the real-input scenario: exercise the
     * identical blocked approach with gas cargo and a completed refinery. */
    int refinery=spawn_build(SIDE,B_ASSIMILATOR,geysers[1].x,geysers[1].y,0);
    assert(refinery>=0);
    u[worker].x=22; u[worker].y=40; u[worker].order=ORDER_GAS;
    u[worker].target=refinery; u[worker].carry=8; u[worker].build=-1;
    u[worker].path_timer=0; u[worker].path_retry=0;
    int before_gas=gas;
    for(int i=0;i<600 && screen==SCREEN_PLAY && u[worker].alive && u[worker].carry;i++) input(0);
    assert(u[worker].alive && u[worker].carry==0 && gas==before_gas+8);
    assert(u[worker].order==ORDER_GAS && u[worker].target==refinery);

    /* Actual destruction and slot reuse invalidate the tagged depot field. */
    assert(worker_depot_next(22,40,home,&nx,&ny));
    unsigned builds=sc64_route_builds;
    assert(worker_depot_next(22,40,home,&nx,&ny) && sc64_route_builds==builds);
    destroy_building(home,ENEMY);
    int replacement=spawn_build(SIDE,B_NEXUS,240,200,0);
    assert(replacement==home);
    assert(worker_depot_next(22,40,replacement,&nx,&ny));
    assert(sc64_route_builds>builds);
    int px=22,py=40;
    for(int i=0;i<MAP_CELLS && dist2(px,py,b[replacement].x,b[replacement].y)>32*32;i++) {
        assert(worker_depot_next(px,py,replacement,&nx,&ny));
        assert(nx!=px || ny!=py);
        px=nx; py=ny;
    }
    assert(dist2(px,py,b[replacement].x,b[replacement].y)<=32*32);

#ifdef SC64_TERRAN_LATE_RUNTIME
    /* Synthetic late-campaign setup; use the real lift/land entrypoints and
     * verify they invalidate a cached field keyed by this building slot. */
    race_mode=0; race_selected_mission=10;
    int flying=spawn_build(SIDE,B_COMMAND,320,320,0);
    assert(flying>=0 && race_late_active());
    assert(sc64_route_depot_slot(flying,b[flying].x,b[flying].y)>=0);
    assert(depot_field_cached(flying));
    assert(race_late_lift(flying));
    assert(!depot_field_cached(flying));
    assert(sc64_route_depot_slot(flying,b[flying].x,b[flying].y)>=0);
    assert(depot_field_cached(flying));
    int lx=-1,ly=-1;
    for(int y=64;y<WORLD_H-64 && lx<0;y+=32)
        for(int x=64;x<WORLD_W-64;x+=32)
            if(race_late_landing_site(flying,x,y)) {lx=x;ly=y;break;}
    assert(lx>=0 && race_late_flight_order(flying,lx,ly,1));
    for(int i=0;i<10000 && race_late_airborne(flying);i++) {frame++;race_late_flight_tick();}
    assert(!race_late_airborne(flying) && !depot_field_cached(flying));
#endif

    return 0;
}
