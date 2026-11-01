/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Host-only observation at the real update_orders Egg dispatch. */
#define SC64_CONTACT_GROUP_TEST_EGG_HOOK
static void sc64_contact_group_test_egg_observe(int slot);
#define main sc64_group_membership_main
#include "sc64_contact_group_membership.c"
#undef main

#if !defined(SC64_Z7_EGG_PRODUCTION) || !defined(SC64_SHARED_TRANSPORT)
#error "lifecycle fixture requires the Z7 Egg and shared transport profile"
#endif
static int watch_egg, observed_egg, watched_mover;
static void sc64_contact_group_test_egg_observe(int slot)
{
    if (!watch_egg || u[slot].type == U_Z7_EGG) return;
    assert(sc64_unit_bucket_active);
    assert(u[slot].type == U_ZERGLING);
    int lings=0;
    for (int i=0;i<MAX_UNITS;i++) if (u[i].alive && u[i].type==U_ZERGLING) lings++;
    assert(lings >= 2); /* The real pair birth attempted its second spawn. */
    for (int speed=1;speed<=2;speed++)
        assert(sc64_contact_blocked_mask(&u[watched_mover],speed)==reference_mask(&u[watched_mover],speed));
    observed_egg++;
}
static void init_z7_world(void)
{
    game_init();
    memset(u,0,sizeof(u));memset(b,0,sizeof(b));memset(m,0,sizeof(m));
    memset(geysers,0,sizeof(geysers));
    race_mode=0;race_human=R_ZERG;race_selected_mission=SC64_Z7_GLOBAL_ID;
    race_player[SIDE]=race_player[ENEMY]=race_player[NEUTRAL]=R_ZERG;
    race_player[RESCUABLE]=race_player[4]=R_ZERG;
    race_z7_map=&sc64_race_maps[SC64_Z7_GLOBAL_ID];
    race_cols=race_z7_map->width;race_rows=race_z7_map->height;
    assert(race_cols==SC64_Z7_MAP_COLS && race_rows==SC64_Z7_MAP_ROWS);
    minerals=5000;gas=5000;supply_used=0;supply_cap=0;
    assert(race_z7_active());
}
static void test_real_egg_dispatch_and_pair(void)
{
    init_z7_world();
    int hatch=spawn_build(SIDE,B_HATCHERY,160,160,0);
    assert(hatch>=0 && building_complete(hatch));b[hatch].larva=3;
    assert(spawn_build(SIDE,B_SPAWNING_POOL,224,160,0)>=0);
    watched_mover=spawn_unit(SIDE,U_MARINE,168,160);assert(watched_mover>=0);
    assert(race_queue_unit(SIDE,hatch,U_ZERGLING));
    int egg=-1;
    for(int i=0;i<MAX_UNITS;i++)if(u[i].alive&&u[i].type==U_Z7_EGG&&u[i].target==U_ZERGLING){egg=i;break;}
    assert(egg>=0);u[egg].work_timer=1; /* bounded host fixture setup */
    watch_egg=1;observed_egg=0;update_orders();watch_egg=0;
    assert(observed_egg==1&&u[egg].alive&&u[egg].type==U_ZERGLING);
    sc64_unit_buckets_begin();
    assert(sc64_contact_blocked_mask(&u[watched_mover],2)==reference_mask(&u[watched_mover],2));
    sc64_unit_buckets_end();
}
static void test_real_board_and_unload(void)
{
    init_z7_world();
    int ship=spawn_unit(SIDE,U_DROPSHIP,160,160);
    int passenger=spawn_unit(SIDE,U_MARINE,160,160);
    int mover=spawn_unit(SIDE,U_MARINE,168,160);
    assert(ship>=0&&passenger>=0&&mover>=0&&race_transport_carrier_valid(ship));
    sc64_unit_buckets_begin();
    assert(race_transport_request_board(passenger,ship));
    assert(race_transport_tick_unit(passenger));
    assert(SC64_TRANSPORT_LOADED(&u[passenger]));
    assert(!sc64_unit_bucket_active); /* real boarding invalidates indexes */
    for(int speed=1;speed<=2;speed++)
        assert(sc64_contact_blocked_mask(&u[mover],speed)==reference_mask(&u[mover],speed));
    assert(race_transport_request_unload(ship,u[ship].x,u[ship].y));
    for(int y=1;y<race_rows-1&&SC64_TRANSPORT_LOADED(&u[passenger]);y++)
        for(int x=1;x<race_cols-1&&SC64_TRANSPORT_LOADED(&u[passenger]);x++){
            if(!is_traversable(x,y,-1))continue;
            /* Controlled reposition occurs only between bucket windows. */
            sc64_unit_buckets_end();
            u[ship].x=u[ship].tx=x*16+8;u[ship].y=u[ship].ty=y*16+8;
            sc64_unit_buckets_begin();assert(sc64_unit_bucket_active);
            race_transport_try_unload(ship);
        }
    assert(!SC64_TRANSPORT_LOADED(&u[passenger]));
    assert(!sc64_unit_bucket_active); /* real unloading invalidates indexes */
    /* Query beside the NEW landing position; the passenger must contribute
     * the toward-it bit, so a dropped member cannot pass vacuously. */
    int side = u[passenger].x + 3 < WORLD_W ? 1 : -1;
    u[mover].x=u[passenger].x+3*side;u[mover].y=u[passenger].y;
    sc64_unit_bucket_move(&u[mover]); /* inactive: preserve normal fallback */
    unsigned toward=1u<<(side>0?4:0);
    assert(reference_mask(&u[mover],1)&toward);
    for(int speed=1;speed<=2;speed++)
        assert(sc64_contact_blocked_mask(&u[mover],speed)==reference_mask(&u[mover],speed));
    sc64_unit_buckets_end();
    sc64_unit_buckets_begin();assert(sc64_unit_bucket_active);
    for(int speed=1;speed<=2;speed++){
        unsigned expected=reference_mask(&u[mover],speed);
        assert(sc64_contact_blocked_mask(&u[mover],speed)==expected);
        assert(sc64_contact_blocked_mask(&u[mover],speed)==expected); /* memo path */
    }
    sc64_unit_buckets_end();
}
int main(void)
{
    test_real_egg_dispatch_and_pair();
    test_real_board_and_unload();
    puts("group actual Egg dispatch and transport lifecycle passed");
    return 0;
}
