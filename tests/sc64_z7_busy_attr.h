/* SPDX-License-Identifier: GPL-3.0-or-later
 * Optional coarse, nested timing for the bounded Z7 busy diagnostic. */
#ifndef Z7_BUSY_DIAGNOSTIC
#error "Z7 busy attribution requires the bounded busy diagnostic"
#endif
/* 0 update, 1 audio, 2 render incl. present, 3 orders, 4 sight,
 * 5 other update, 6 map, 7 HUD, 8 background, 9 buildings, 10 units.
 * These scopes overlap: never add them into a total. */
#define Z7_ATTR_STAGES 11
static uint64_t attr_sum[Z7_ATTR_STAGES], attr_build_update, attr_no_build_update;
static u32 attr_peak[Z7_ATTR_STAGES], attr_hits_before, attr_builds_before;
static u32 attr_hits, attr_builds, attr_build_frames, attr_no_build_frames, attr_max_builds;
static u32 attr_top[10][6]; /* total, tick, update, render, route builds, route hits */
#ifdef SC64_ROUTE_BUILD_CLASS_DIAGNOSTIC
static u32 attr_usable_min=~0u, attr_usable_max;
#endif

__attribute__((noinline,section(".text"))) static void attr_before(void)
{
    attr_hits_before=sc64_route_hits;attr_builds_before=sc64_route_builds;
#ifdef SC64_ROUTE_BUILD_CLASS_DIAGNOSTIC
    if(!busy_calls)route_class_begin();
    unsigned usable=(unsigned)sc64_range_route_usable_slots();
    if(usable<attr_usable_min)attr_usable_min=usable;
    if(usable>attr_usable_max)attr_usable_max=usable;
#endif
}
__attribute__((noinline,section(".text"))) static void
attr_after(u32 total,u32 update,u32 audio,u32 render)
{
    u32 hits=sc64_route_hits-attr_hits_before, builds=sc64_route_builds-attr_builds_before;
    attr_hits+=hits;attr_builds+=builds;
    if(busy_pre_screen!=SCREEN_PLAY || busy_pre_text || frame<=busy_pre_frame)return;
    if(builds){++attr_build_frames;attr_build_update+=update;}
    else {++attr_no_build_frames;attr_no_build_update+=update;}
    if(builds>attr_max_builds)attr_max_builds=builds;
    u32 v[Z7_ATTR_STAGES]={update,audio,render,sc64_orders_cycles,sc64_sight_cycles,
        sc64_other_cycles,sc64_map_cycles,sc64_hud_cycles,sc64_background_cycles,
        sc64_building_cycles,sc64_unit_draw_cycles};
    for(unsigned i=0;i<Z7_ATTR_STAGES;++i){attr_sum[i]+=v[i];if(v[i]>attr_peak[i])attr_peak[i]=v[i];}
    unsigned j=10;while(j && total>attr_top[j-1][0]){
        if(j<10)for(unsigned k=0;k<6;++k)attr_top[j][k]=attr_top[j-1][k];
        --j;
    }
    if(j<10){attr_top[j][0]=total;attr_top[j][1]=busy_tick;attr_top[j][2]=update;
        attr_top[j][3]=render;attr_top[j][4]=builds;attr_top[j][5]=hits;}
}
__attribute__((noinline,section(".text"))) static void attr_report(void)
{
#ifdef SC64_ROUTE_BUILD_CLASS_DIAGNOSTIC
    out_text("Z7C CLASS");out_field("movement",route_class_movement);
    out_field("component",route_class_component);out_field("depot",route_class_depot);
    out_field("empty",route_class_empty);out_field("evicted",route_class_evicted);
    out_field("resets",route_class_resets);out_field("wraps",route_class_wraps);
    out_field("distinct",route_class_distinct);out_field("repeated",route_class_repeated);
    out_field("untracked",route_class_untracked);out_field("usable_min",attr_usable_min);
    out_field("usable_max",attr_usable_max);out_flush();
#endif
    out_text("Z7A ROUTE");out_field("slots",SC64_ROUTE_SLOTS);out_field("hits",attr_hits);
    out_field("builds",attr_builds);out_field("build_frames",attr_build_frames);
    out_field("no_build_frames",attr_no_build_frames);out_field("max_builds",attr_max_builds);
    out_text(" build_update=");out_u64(attr_build_update);
    out_text(" no_build_update=");out_u64(attr_no_build_update);out_flush();
    for(unsigned i=0;i<Z7_ATTR_STAGES;++i){out_text("Z7A STAGE");out_field("index",i);
        out_text(" sum=");out_u64(attr_sum[i]);out_text(" peak=");out_u64(attr_peak[i]);out_flush();}
    for(unsigned i=0;i<10;++i){out_text("Z7A TOP");out_field("rank",i+1);
        out_field("total",attr_top[i][0]);out_field("tick",attr_top[i][1]);
        out_field("update",attr_top[i][2]);out_field("render",attr_top[i][3]);
        out_field("builds",attr_top[i][4]);out_field("hits",attr_top[i][5]);out_flush();}
}
