/* SPDX-License-Identifier: GPL-3.0-or-later
 * Actual-engine fixture for Arbiter cache behavior under generations 39/40. */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../starcraft/game.c"

static int make_actor(int live_owner,unsigned raw,int type,int owner,int x,int y)
{
    int n=spawn_unit(live_owner,type,x,y);
    assert(n>=0);
    u[n].raw_type=(int)raw;
    u[n].raw_owner=owner;
    return n;
}

static void source_frame(void)
{
    sc64_p7_arbiter_cache_frame_tick();
    sc64_p7_arbiter_cache_process_due();
}

static int region_has_pixels(int x0,int y0,int x1,int y1)
{
    for(int y=y0;y<=y1;++y) for(int x=x0;x<=x1;++x)
        if(gfx_test_pixel(x,y)!=C_BLACK) return 1;
    return 0;
}

int main(void)
{
#ifdef SC64_P7_TYPED_COMPACT_ENABLED
    assert(SC64_RACE_SAVE_GENERATION==40u);
#else
    assert(SC64_RACE_SAVE_GENERATION==39u);
#endif
    memset(u,0,sizeof(u));
    memset(b,0,sizeof(b));
    memset(&race_p7,0,sizeof(race_p7));
    race_mode=0;
    race_selected_mission=SC64_P7_GLOBAL_ID;
    race_p7.reserved[1]=150;
    int arb=make_actor(ENEMY,71,U_P7_ARBITER,0,160,160);
    int target=make_actor(ENEMY,65,U_ZEALOT,0,256,160);
    int scv=make_actor(SIDE,0,U_SCV,7,80,80);
    assert(u[scv].type==0 && sc64_p7_arbiter_cache_unit_valid(&u[scv]));
    static const Unit empty={0};
    assert(sc64_p7_arbiter_cache_unit_valid(&empty));
    u[target].order=ORDER_ATTACK_TARGET;
    u[target].target=arb;

    /* Inner finder box clips candidates before the inclusive outer-box
       distance check. This is the v2 false positive at x=256. */
    assert(!sc64_p7_arbiter_finder_square(arb,target));
    u[target].x=245;
    assert(sc64_p7_arbiter_finder_square(arb,target));
    assert(sc64_p7_observer_rect_distance(71,160,160,0,65,245,160,0)<=160);

    /* Exercise the existing cell index used by update_orders: broadphase
       enumerates local 64-pixel buckets, then exact geometry filters. */
    sc64_target_index_begin();
    assert(sc64_target_index_active);
    /* Launch-time first refresh is immediate, before the first orders pass. */
    sc64_p7_arbiter_cache_process_due();
    assert(sc64_p7_arbiter_cache_initialized);
    assert(sc64_p7_observer_arbiter_aura_cloaks(target));
    assert(sc64_p7_observer_target_is_cloaked(target));
    assert(sc64_p7_arbiter_cache_unit_valid(&u[target]));
    Unit malformed=u[target];
    malformed.weapon=(u16)((malformed.weapon&~(31u<<8))|(31u<<8));
    assert(!sc64_p7_arbiter_cache_unit_valid(&malformed));
    malformed=u[target]; malformed.type=0;
    assert(sc64_p7_arbiter_cache_unit_valid(&malformed));
    assert(u[target].order==ORDER_ATTACK_TARGET && u[target].target==arb);
    u[target].x=256;
    sc64_target_index_begin();
    sc64_p7_arbiter_cache_source_tick();
    assert(!sc64_p7_observer_arbiter_aura_cloaks(target));

    /* Owner 8 and a mismatched collapsed force fail closed on cached cloak. */
    u[target].x=245;
    sc64_target_index_begin();
    sc64_p7_arbiter_cache_source_tick();
    assert(!sc64_p7_observer_can_attack_from(U_MARINE,target,ENEMY,8));
    assert(!sc64_p7_observer_can_attack_from(U_MARINE,target,ENEMY,4));

    /* Clock conversion is exactly 24 source updates per 30 GBA frames and
       does not advance from rendering/query calls. */
    unsigned before=(unsigned)race_p7.reserved[1] |
                    ((unsigned)race_p7.reserved[2]<<8);
    for(int i=0;i<30;++i) {
        (void)sc64_p7_observer_target_is_cloaked(target);
    }
    assert((((unsigned)race_p7.reserved[1] |
             ((unsigned)race_p7.reserved[2]<<8)))==before);
    for(int i=0;i<30;++i) source_frame();
    unsigned after=(unsigned)race_p7.reserved[1] |
                   ((unsigned)race_p7.reserved[2]<<8);
    assert(before-after==24u);

    /* Native resets occur before target checks and stagger every represented
       visible Unit in slot order, including the Arbiter and uncloaked SCV. */
    race_p7.reserved[1]=1; race_p7.reserved[2]=0;
    sc64_p7_arbiter_cache_source_tick();
    assert(((unsigned)race_p7.reserved[1] |
            ((unsigned)race_p7.reserved[2]<<8))==300u);
    assert(SC64_P7_ARBITER_COUNTDOWN(u[arb].weapon)==0u);
    assert(SC64_P7_ARBITER_COUNTDOWN(u[target].weapon)==0u);
    assert(SC64_P7_ARBITER_COUNTDOWN(u[scv].weapon)==2u);

    /* Reused rows start with a fresh valid marker and no aura/detection bits. */
    u[target].alive=0;
    int reused=make_actor(ENEMY,65,U_ZEALOT,0,245,160);
    assert(reused==target);
    assert(SC64_P7_ARBITER_CACHE_VALID(u[reused].weapon));
    assert(!(u[reused].weapon&SC64_P7_ARBITER_AURA));
    assert(SC64_P7_ARBITER_DETECTED_MASK(u[reused].weapon)==0);

    /* An Observer is intrinsically tracked even without an Arbiter. This
       covers the target-owned native detection timer independently. */
    u[arb].alive=0;
    int observer=make_actor(ENEMY,84,U_P7_OBSERVER,0,245,160);
    int detector=make_actor(SIDE,84,U_P7_OBSERVER,7,245,160);
    SC64_GRID_SET(visible,(245/16)+(160/16)*MAP_COLS,1);
    sc64_target_index_begin();
    sc64_p7_arbiter_cache_source_tick();
    assert((u[observer].weapon&SC64_P7_ARBITER_TRACKED) &&
           !(u[observer].weapon&SC64_P7_ARBITER_AURA));
    assert(sc64_p7_observer_target_is_cloaked(observer));
    assert(sc64_p7_observer_can_attack_from(U_MARINE,observer,SIDE,7));
    assert(sc64_p7_observer_target_revealed_to(observer,7,SIDE));
    /* Main-view and minimap use the exact same cache/optical predicate. */
    for(int i=0;i<MAX_UNITS;++i)
        if(i!=observer && i!=detector) u[i].alive=0;
    u[detector].x=0; u[detector].y=0;
    camx=0; camy=100;
    gfx_begin(C_BLACK);
    draw_unit(&u[observer]);
    assert(region_has_pixels(225,55,255,90));
    camx=800; camy=800;
    gfx_begin(C_BLACK);
    draw_minimap();
    int mini_x=197+sc64_minimap_pixel((unsigned)u[observer].x,MAP_COLS);
    int mini_y=129+sc64_minimap_pixel((unsigned)u[observer].y,MAP_ROWS);
    assert(gfx_test_pixel(mini_x,mini_y)==owner_color(u[observer].owner));
    SC64_GRID_SET(visible,(245/16)+(160/16)*MAP_COLS,0);
    gfx_begin(C_BLACK);
    draw_unit(&u[observer]);
    assert(!region_has_pixels(225,55,255,90));
    gfx_begin(C_BLACK);
    draw_minimap();
    assert(gfx_test_pixel(mini_x,mini_y)!=owner_color(u[observer].owner));
    SC64_GRID_SET(visible,(245/16)+(160/16)*MAP_COLS,1);
    assert(sc64_p7_arbiter_cache_world_valid());
    u[detector].x=0; u[detector].y=0;
    for(int i=0;i<30;++i) {
        sc64_p7_arbiter_cache_source_tick();
        assert(sc64_p7_observer_can_attack_from(U_MARINE,observer,SIDE,7));
    }
    assert(SC64_P7_ARBITER_COUNTDOWN(u[observer].weapon)==0);
    sc64_p7_arbiter_cache_source_tick();
    assert(!sc64_p7_observer_can_attack_from(U_MARINE,observer,SIDE,7));
    assert(!sc64_p7_observer_target_revealed_to(observer,7,SIDE));

    /* Zeratul is an authenticated campaign actor in P7 and has the native
       requires-detector trait. Its cache must be retained without an Arbiter. */
    u[observer].alive=0;
    int zeratul=make_actor(ENEMY,75,U_P6_ZERATUL,2,245,160);
    detector=make_actor(SIDE,84,U_P7_OBSERVER,7,245,160);
    SC64_GRID_SET(visible,(245/16)+(160/16)*MAP_COLS,1);
    sc64_target_index_begin();
    sc64_p7_arbiter_cache_source_tick();
    assert((u[zeratul].weapon&SC64_P7_ARBITER_TRACKED) &&
           !(u[zeratul].weapon&SC64_P7_ARBITER_AURA));
    assert(sc64_p7_observer_is_cloaked(74u));
    assert(sc64_p7_observer_is_cloaked(75u));
    assert(sc64_p7_observer_target_is_cloaked(zeratul));
    assert(sc64_p7_observer_can_attack_from(U_MARINE,zeratul,SIDE,7));
    u[detector].x=0; u[detector].y=0;
    for(int i=0;i<30;++i) {
        sc64_p7_arbiter_cache_source_tick();
        assert(sc64_p7_observer_can_attack_from(U_MARINE,zeratul,SIDE,7));
    }
    sc64_p7_arbiter_cache_source_tick();
    assert(!sc64_p7_observer_can_attack_from(U_MARINE,zeratul,SIDE,7));
    u[zeratul].alive=0;
    int hero_dt=make_actor(ENEMY,74,U_P6_HERO_DARK_TEMPLAR,2,245,160);
    detector=make_actor(SIDE,84,U_P7_OBSERVER,7,245,160);
    sc64_target_index_begin();
    sc64_p7_arbiter_cache_source_tick();
    assert((u[hero_dt].weapon&SC64_P7_ARBITER_TRACKED) &&
           sc64_p7_observer_target_is_cloaked(hero_dt));
    assert(sc64_p7_observer_can_attack_from(U_MARINE,hero_dt,SIDE,7));

    /* Entering an Arbiter aura immediately refreshes detection even when a
       native stagger countdown from the prior uncloaked state is still live. */
    int entry_arbiter=make_actor(ENEMY,71,U_P7_ARBITER,0,800,400);
    int entry_target=make_actor(ENEMY,65,U_ZEALOT,0,800,400);
    detector=make_actor(SIDE,84,U_P7_OBSERVER,7,800,400);
    unsigned entry_cell=(800/16)+(400/16)*MAP_COLS;
    SC64_GRID_SET(visible,entry_cell,1);
    race_p7.reserved[1]=200; race_p7.reserved[2]=0;
    u[entry_target].weapon=(u16)sc64_p7_arbiter_pack(0,7,0,0);
    sc64_target_index_begin();
    sc64_p7_arbiter_cache_source_tick();
    assert(u[entry_arbiter].alive);
    assert(SC64_P7_ARBITER_CACHE_VALID(u[entry_target].weapon));
    assert((u[entry_target].weapon&SC64_P7_ARBITER_AURA)!=0);
    assert(SC64_P7_ARBITER_DETECTED_MASK(u[entry_target].weapon)&(1u<<7));
    assert(SC64_P7_ARBITER_COUNTDOWN(u[entry_target].weapon)==30);
    assert(sc64_p7_observer_can_attack_from(U_MARINE,entry_target,SIDE,7));
    u[detector].x=0; u[detector].y=0;
    for(int i=0;i<30;++i) {
        sc64_p7_arbiter_cache_source_tick();
        assert(sc64_p7_observer_can_attack_from(U_MARINE,entry_target,SIDE,7));
    }
    assert(SC64_P7_ARBITER_COUNTDOWN(u[entry_target].weapon)==0);
    sc64_p7_arbiter_cache_source_tick();
    assert(!sc64_p7_observer_can_attack_from(U_MARINE,entry_target,SIDE,7));

#ifdef SC64_P7_TYPED_COMPACT_ENABLED
    puts("P7 generation-40 cache geometry/cadence/reuse checks passed");
#else
    puts("P7 generation-39 cache geometry/cadence/reuse checks passed");
#endif
    return 0;
}
