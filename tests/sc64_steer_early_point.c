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
    got=sc64_steer_early_point(q,speed,from,blocked_x,blocked_y);
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
int main(void)
{
    unsigned long checks=0;
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
