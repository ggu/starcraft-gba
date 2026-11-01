/* SPDX-License-Identifier: GPL-3.0-or-later
 * Actual births must affect the active collision index before first motion. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

#if !defined(SC64_RACES)
#error "This fixture needs race collision"
#endif

static unsigned full_world_mask(const Unit *q,int speed)
{
    static const int dx[8]={1,1,0,-1,-1,-1,0,1},dy[8]={0,1,1,1,0,-1,-1,-1};
    unsigned mask=0;
    for(int i=0;i<8;i++) {
        int x=q->x+dx[i]*speed,y=q->y+dy[i]*speed;
        if(x<0||y<0||x>=WORLD_W||y>=WORLD_H||
           !unit_step_clear_race_fallback(q,x,y))mask|=1u<<i;
    }
    return mask;
}

static unsigned indexed_mask(const Unit *q,int speed)
{
#ifdef SC64_CONTACT_CACHE
    return sc64_contact_blocked_mask(q,speed);
#else
    static const int dx[8]={1,1,0,-1,-1,-1,0,1},dy[8]={0,1,1,1,0,-1,-1,-1};
    unsigned mask=0;
    for(int i=0;i<8;i++)if(!unit_step_clear(q,q->x+dx[i]*speed,q->y+dy[i]*speed))mask|=1u<<i;
    return mask;
#endif
}

static void case_birth(int reuse)
{
    sc64_unit_bucket_active=0;
    memset(u,0,sizeof(u));
    int mover=spawn_unit(SIDE,U_MARINE,100,100);
    int old=-1;
    if(reuse)old=spawn_unit(ENEMY,U_MARINE,300,300);
    assert(mover>=0 && (!reuse||old>=0));
    sc64_unit_buckets_begin();assert(sc64_unit_bucket_active);
    unsigned before=full_world_mask(&u[mover],4);
    assert(indexed_mask(&u[mover],4)==before);
    if(reuse)u[old].alive=0;
    int born=spawn_unit(ENEMY,U_MARINE,108,100);
    assert(born>=0 && (!reuse||born==old));
    /* Ground truth is the original full-world scan, not the indexed
     * bucket reference.  Query before the spawned actor ever moves. */
    unsigned expected=full_world_mask(&u[mover],4);
    assert(expected!=before);
    assert(sc64_unit_bucket_active);
    assert(indexed_mask(&u[mover],4)==expected);
    assert(unit_step_clear(&u[mover],104,100)==unit_step_clear_race_fallback(&u[mover],104,100));
}

static void case_out_of_bounds_birth(int x,int y)
{
    sc64_unit_bucket_active=0;memset(u,0,sizeof(u));
    int mover=spawn_unit(SIDE,U_MARINE,100,100);assert(mover>=0);
    sc64_unit_buckets_begin();assert(sc64_unit_bucket_active);
    assert(spawn_unit(ENEMY,U_MARINE,x,y)>=0);
    assert(!sc64_unit_bucket_active);
    assert(indexed_mask(&u[mover],4)==full_world_mask(&u[mover],4));
}

#ifdef SC64_SCARAB_RUNTIME
static void case_noncolliding_birth(void)
{
    sc64_unit_bucket_active=0;memset(u,0,sizeof(u));
    int mover=spawn_unit(SIDE,U_MARINE,100,100);assert(mover>=0);
    sc64_unit_buckets_begin();assert(sc64_unit_bucket_active);
    int born=spawn_unit(SIDE,U_SCARAB,108,100);assert(born>=0);
    assert(SC64_SCARAB_NONCOLLIDING(&u[born]));
    assert(indexed_mask(&u[mover],4)==full_world_mask(&u[mover],4));
}
#endif

#ifdef SC64_SHARED_TRANSPORT
static void case_load_after_birth(void)
{
    sc64_unit_bucket_active=0;memset(u,0,sizeof(u));race_mode=1;
    int mover=spawn_unit(ENEMY,U_MARINE,100,100);
    int carrier=spawn_unit(SIDE,U_DROPSHIP,108,100);
    assert(mover>=0&&carrier>=0);
    sc64_unit_buckets_begin();assert(sc64_unit_bucket_active);
    int born=spawn_unit(SIDE,U_MARINE,108,100);assert(born>=0);
    assert(indexed_mask(&u[mover],4)==full_world_mask(&u[mover],4));
    assert(race_transport_request_board(born,carrier));
    assert(race_transport_tick_unit(born));assert(SC64_TRANSPORT_LOADED(&u[born]));
    assert(indexed_mask(&u[mover],4)==full_world_mask(&u[mover],4));
}
#endif

int main(void)
{
    game_init();
    case_birth(0);
    case_birth(1);
    case_out_of_bounds_birth(-1,100);
    case_out_of_bounds_birth(WORLD_W,100);
#ifdef SC64_SCARAB_RUNTIME
    case_noncolliding_birth();
#endif
#ifdef SC64_SHARED_TRANSPORT
    case_load_after_birth();
#endif
    puts("spawned/reused collision membership and memo agree with full-world scan");
    return 0;
}
