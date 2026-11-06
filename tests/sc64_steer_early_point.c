/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Candidate winner vs an independent full-scan collision and selection oracle. */
#define main contact_cache_fixture_main
#include "sc64_contact_cache.c"
#undef main
#ifndef SC64_STEER_EARLY_POINT_EXPERIMENT
#error "fixture requires opt-in steering experiment"
#endif

static int scalar_clear(const Unit *q, int x, int y)
{
    if (x<0 || y<0 || x>=WORLD_W || y>=WORLD_H) return 0;
    if (SC64_SCARAB_NONCOLLIDING(q)) return 1;
    if (IS_WORKER(q->type) && (q->order==ORDER_MINE || q->order==ORDER_GAS)) return 1;
#if defined(SC64_T4_RUNTIME) || defined(SC64_ZP_CAMPAIGN_ACTORS)
    if (sc64_game_unit_actor_flags(q->type) &&
        !(sc64_game_unit_actor_flags(q->type)&SC64_GAME_ACTOR_MOVABLE)) return 1;
#endif
    return unit_step_clear_race_fallback(q,x,y);
}

static void check_winner(Unit *q, int speed, int blocked_x, int blocked_y)
{
    static const int dx[8]={1,1,0,-1,-1,-1,0,1};
    static const int dy[8]={0,1,1,1,0,-1,-1,-1};
    int from=q->x/16+(q->y/16)*MAP_COLS;
    int want=-1, cost=0x7fffffff, got;
    if (scalar_clear(q,blocked_x,blocked_y)) return;
    for(int i=0;i<8;i++) {
        int x=q->x+dx[i]*speed,y=q->y+dy[i]*speed;
        int d=dist2(x,y,q->path_x,q->path_y);
        if (d<cost && is_traversable(x/16,y/16,from) && scalar_clear(q,x,y)) {
            want=i;cost=d;
        }
    }
    static Unit before_units[MAX_UNITS];
    unsigned cached_before=sc64_contact_blocked_mask(q,speed);
    memcpy(before_units,u,sizeof(before_units));
    got=sc64_steer_early_point(q,speed,from,blocked_x,blocked_y);
    assert(!memcmp(before_units,u,sizeof(before_units)));
    assert(sc64_contact_blocked_mask(q,speed)==cached_before);
    if(got==-2) {
        unsigned blocked=sc64_contact_blocked_mask(q,speed); cost=0x7fffffff;got=-1;
        for(int i=0;i<8;i++) {
            int x=q->x+dx[i]*speed,y=q->y+dy[i]*speed;
            int d=dist2(x,y,q->path_x,q->path_y);
            if(d<cost && is_traversable(x/16,y/16,from) && !(blocked&(1u<<i))) {
                got=i;cost=d;
            }
        }
    }
    assert(got==want);
}
static void authored_cases(void)
{
    memset(u,0,sizeof(u));
    u[0].alive=1;u[0].type=U_MARINE;u[0].raw_type=-1;u[0].x=136;u[0].y=136;
    u[1].alive=1;u[1].type=U_MARINE;u[1].raw_type=-1;u[1].x=137;u[1].y=136;
    int from=u[0].x/16+(u[0].y/16)*MAP_COLS;
    path_grid_valid=1; memset(path_walkable,0,sizeof(path_walkable));
    sc64_unit_buckets_begin();
    u[0].path_x=128;u[0].path_y=128;
    assert(!scalar_clear(&u[0],137,136));
    assert(sc64_steer_early_point(&u[0],32,from,137,136)==-1);
    check_winner(&u[0],32,137,136);
    memset(path_walkable,0xff,sizeof(path_walkable));
    u[0].path_x=36;u[0].path_y=36;
    /* The blocked original step is snapped to one pixel while the steering
     * offsets are two pixels: north-west remains the unique cheapest. */
    assert(sc64_steer_early_point(&u[0],2,from,137,136)==5);
    check_winner(&u[0],2,137,136);
    u[0].path_x=u[0].x;u[0].path_y=u[0].y;
    /* Four cardinal directions tie. Lowest-index east is blocked, so the
     * candidate must fall back and let the exact selector decide. */
    assert(sc64_steer_early_point(&u[0],2,from,137,136)==-2);
    check_winner(&u[0],2,137,136);
    sc64_unit_buckets_end();
}
int main(void)
{
    unsigned long checks=0;
    authored_cases();
    for(int state=0;state<120;state++) {
        build_world(state);set_p6(state&1);
        memset(path_walkable,(state%3)==0?0:0xff,sizeof(path_walkable));path_grid_valid=1;
        sc64_unit_buckets_begin();
        for(int i=0;i<MAX_UNITS;i++) if(movable(&u[i])) {
            Unit *q=&u[i]; int speed=1+(int)(next_random()%3u);
            q->path_x=clamp_coord(q->x+(int)(next_random()%101u)-50,WORLD_W);
            q->path_y=clamp_coord(q->y+(int)(next_random()%101u)-50,WORLD_H);
            for(int k=0;k<8;k++) {
                check_winner(q,speed,q->x+sc64_contact_dx[k]*speed,q->y+sc64_contact_dy[k]*speed);
                ++checks;
            }
            /* A snapped waypoint can differ from all full-speed directions. */
            check_winner(q,speed,q->x+1,q->y);++checks;
            /* Equal target coordinates exercise exact cost ties. */
            q->path_x=q->x;q->path_y=q->y;
            check_winner(q,speed,q->x+1,q->y);++checks;
        }
        sc64_unit_buckets_end();
    }
    printf("early steering winner against independent scalar oracle: %lu cases\n",checks);
    return 0;
}
