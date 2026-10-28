/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef SC64_TRACE_DURATION_CACHE
#define SC64_TRACE_DURATION_CACHE 1
#endif
#include "../starcraft/game.c"
#include <assert.h>
#include <stdio.h>

static void fixture(void)
{
    gba_init();
    game_init();
    init_state();
    for(int i=0;i<MAX_TRACERS;++i) assert(!sc64_trace_durations[i]);
}

static int effect(int style)
{
    return sc64_weapon_vfx_pack(style,C_RED);
}

static void creation_expiry_and_reuse(void)
{
    fixture();
    int a=effect(SC64_WEAPON_VFX_STYLE_MISSILE);
    int bcolor=effect(SC64_WEAPON_VFX_STYLE_MUZZLE);
    int da=sc64_trace_duration(a,20,30,90,80);
    int db=sc64_trace_duration(bcolor,40,50,120,100);
    assert(da>db && db>0);
    add_tracer(20,30,90,80,a);
    assert(sc64_trace_durations[0]==da);
    for(int n=0;n<da;++n){
        assert(sc64_tracer_duration(0)==da);
        --tracers[0].life;
    }
    tracers[0].life=0;
    add_tracer(40,50,120,100,bcolor);
    assert(tracers[0].color==bcolor && sc64_tracer_duration(0)==db);
    assert(sc64_trace_durations[0]==db);

    /* Long/nonpositive durations stay uncached instead of truncating into
     * a plausible short lifetime. Lazy lookup recovers the real duration. */
    sc64_trace_duration_cache_store(0,65536);
    assert(sc64_trace_durations[0]==0);
    assert(sc64_tracer_duration(0)==db);
    sc64_trace_duration_cache_store(0,-1);
    assert(sc64_trace_durations[0]==0);
    sc64_trace_duration_cache_store(0,65535);
    assert(sc64_trace_durations[0]==65535);
    init_state();
    for(int i=0;i<MAX_TRACERS;++i) assert(!sc64_trace_durations[i]);
}

static void full_pool_keeps_original_oldest_choice(void)
{
    fixture();
    for(int i=0;i<MAX_TRACERS;++i)
        add_tracer(10+i,30,70+i,80,effect(SC64_WEAPON_VFX_STYLE_MISSILE));
    int expected=-1,oldest=-1;
    for(int i=0;i<MAX_TRACERS;++i){
        tracers[i].life=i==MAX_TRACERS/2?1:3+i%4;
        int age=sc64_trace_duration(tracers[i].color,tracers[i].x0,
            tracers[i].y0,tracers[i].x1,tracers[i].y1)+1-tracers[i].life;
        if(age>oldest){oldest=age;expected=i;}
    }
    Tracer before[MAX_TRACERS];
    memcpy(before,tracers,sizeof(before));
    int replacement=effect(SC64_WEAPON_VFX_STYLE_FLAME);
    add_tracer(140,90,160,100,replacement);
    assert(expected>=0 && tracers[expected].color==replacement);
    assert(sc64_tracer_duration(expected)==
        sc64_trace_duration(replacement,140,90,160,100));
    for(int i=0;i<MAX_TRACERS;++i)
        if(i!=expected) assert(!memcmp(&tracers[i],&before[i],sizeof(Tracer)));
}

#ifndef SC64_RACES
static void save_reload_invalidates_transient_cache(void)
{
    fixture();
    add_tracer(20,30,90,80,effect(SC64_WEAPON_VFX_STYLE_MISSILE));
    Tracer saved=tracers[0];
    int duration=sc64_tracer_duration(0);
    assert(save_game());
    tracers[0].life=0;
    add_tracer(40,50,120,100,effect(SC64_WEAPON_VFX_STYLE_MUZZLE));
    assert(sc64_tracer_duration(0)!=duration);
    assert(load_game());
    assert(!memcmp(&tracers[0],&saved,sizeof(saved)));
    for(int i=0;i<MAX_TRACERS;++i) assert(!sc64_trace_durations[i]);
    assert(sc64_tracer_duration(0)==duration);
}
#endif

#ifdef SC64_WEAPON_SOURCE_VFX
static void source_profiles_keep_endpoint_and_pose_durations(void)
{
    unsigned checked=0;
    fixture();
    for(unsigned i=0;i<SC64_WEAPON_SOURCE_PROFILE_COUNT;++i){
        const Sc64WeaponSourceAttackProfile *p=&sc64_weapon_source_profiles[i];
        if(!p->projectile_image) continue;
        for(unsigned dir=0;dir<32;dir+=7){
            Sc64WeaponSourceVfxKey key={p->actor_raw,1,p->weapon_id,
                dir,p->mode,p->variant};
            uint32_t packed;
            if(!sc64_weapon_source_vfx_pack(key,&packed)) continue;
            for(int distance=16;distance<=256;distance*=2){
                tracers[0].life=0;
                int want=sc64_trace_duration((int)packed,100,100,
                                             100+distance,120);
                add_tracer(100,100,100+distance,120,(int)packed);
                if(want<=0) continue;
                assert(tracers[0].color==(int)packed);
                assert(sc64_tracer_duration(0)==want);
                assert(tracers[0].life==want+1);
                ++checked;
            }
        }
    }
    assert(checked>0);
    printf("source duration cases=%u\n",checked);
}
#endif

int main(void)
{
    creation_expiry_and_reuse();
    full_pool_keeps_original_oldest_choice();
#ifndef SC64_RACES
    save_reload_invalidates_transient_cache();
#endif
#ifdef SC64_WEAPON_SOURCE_VFX
    source_profiles_keep_endpoint_and_pose_durations();
#endif
    puts("PASS transient trace duration cache: replacement, expiry, reset and continuity");
    return 0;
}
