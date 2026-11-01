/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Optional prepared-input integration check of real tracer slots. */
#include "../starcraft/game.c"
#include <assert.h>
#include <stdio.h>
int main(void) {
    unsigned checked=0, cached=0, fallback=0, map_long=0; int map_max=0;
    clear_tracers();
    for(unsigned n=0;n<SC64_WEAPON_SOURCE_PROFILE_COUNT;n++) {
        const Sc64WeaponSourceAttackProfile *p=&sc64_weapon_source_profiles[n];
        Sc64WeaponSourceVfxKey key={.actor_raw=p->actor_raw,.weapon_id=p->weapon_id,.target_air=p->mode,.variant=p->variant,.direction=8};
        uint32_t packed;
        if(!sc64_weapon_source_vfx_pack(key,&packed))continue;
        int profile_max=0;
        for(unsigned dir=0;dir<32;dir++) {
            key.direction=(uint8_t)dir; assert(sc64_weapon_source_vfx_pack(key,&packed));
            int d=sc64_trace_duration((int)packed,0,0,2047,2047);
            if(d>profile_max)profile_max=d;
        }
        key.direction=8; assert(sc64_weapon_source_vfx_pack(key,&packed));
        if(profile_max>map_max)map_max=profile_max;
        if(profile_max>255) {
            ++map_long;
            int legal_max=0;
            if(p->actor_raw==124 && p->weapon_id==29 && p->mode==1 && p->variant==2) {
                for(int dx=-111;dx<=111;dx++)for(int dy=-111;dy<=111;dy++) {
                    if(dx*dx+dy*dy>=112*112)continue;
                    key.direction=(uint8_t)sc64_source_effect_direction(dx,dy);
                    assert(sc64_weapon_source_vfx_pack(key,&packed));
                    int d=sc64_trace_duration((int)packed,512,512,512+dx,512+dy);
                    if(d>legal_max)legal_max=d;
                }
                key.direction=8; assert(sc64_weapon_source_vfx_pack(key,&packed));
            }
            printf("map-bound long actor=%u weapon=%u mode=%u variant=%u duration=%d legal_turret_max=%d\n",p->actor_raw,p->weapon_id,p->mode,p->variant,profile_max,legal_max);
        }
        int expected=sc64_trace_duration((int)packed,100,100,160,100);
        if(expected<=0)continue;
        clear_tracers(); add_tracer(100,100,160,100,(int)packed);
        assert(tracers[0].life==expected+1);
        assert(sc64_tracer_duration(0)==expected);
        if((sc64_trace_durations[0]>>8)!=0) {
            assert(expected<=255);
            assert(sc64_trace_cached_profile(sc64_trace_durations[0]>>8,key)==p);
            ++cached;
        }
        int old=sc64_trace_durations[0];
        sc64_trace_durations[0]=(u16)((255u<<8)|(old&255u));
        const Sc64WeaponSourceAttackProfile *hit=sc64_trace_cached_profile(sc64_trace_durations[0]>>8,key);
        if(!hit) hit=sc64_weapon_source_profile_lookup(key.actor_raw,key.weapon_id,key.target_air,key.variant);
        assert(hit==p); /* invalid or mismatched metadata must resolve exactly */
        sc64_trace_duration_cache_store(0,256);
        assert(sc64_trace_durations[0]==0 && sc64_tracer_duration(0)==expected);
        if(p->variant==0 && p->weapon_count>1) {
            Sc64WeaponSourceVfxKey alt=key; alt.variant=1;
            if(sc64_weapon_source_profile_lookup(alt.actor_raw,alt.weapon_id,alt.target_air,1)==p) {
                uint32_t altpacked; assert(sc64_weapon_source_vfx_pack(alt,&altpacked));
                clear_tracers(); add_tracer(100,100,160,100,(int)altpacked);
                if(tracers[0].life) { assert((sc64_trace_durations[0]>>8)==0); ++fallback; }
                clear_tracers(); add_tracer(100,100,160,100,(int)packed);
            }
        }
        for(int i=0;i<3 && tracers[0].life;i++) {
            update_tracers();
            if(tracers[0].life)assert(sc64_tracer_duration(0)==expected);
        }
        int generic=sc64_weapon_vfx_pack(SC64_WEAPON_VFX_STYLE_MUZZLE,0);
        clear_tracers(); add_tracer(100,100,101,100,generic);
        assert((sc64_trace_durations[0]>>8)==0);
        ++checked;
    }
    assert(checked>0 && cached>0 && fallback>0);
    int long_checked=0;
    for(unsigned n=0;n<SC64_WEAPON_SOURCE_PROFILE_COUNT && !long_checked;n++) {
        const Sc64WeaponSourceAttackProfile *p=&sc64_weapon_source_profiles[n];
        Sc64WeaponSourceVfxKey key={.actor_raw=p->actor_raw,.weapon_id=p->weapon_id,.target_air=p->mode,.variant=p->variant,.direction=8};
        uint32_t packed;
        if(!sc64_weapon_source_vfx_pack(key,&packed))continue;
        int expected=sc64_trace_duration((int)packed,100,100,30000,100);
        if(expected<=255 || expected>65535)continue;
        clear_tracers(); add_tracer(100,100,30000,100,(int)packed);
        assert(tracers[0].life==expected+1 && sc64_trace_durations[0]==0);
        for(int t=0;t<expected && tracers[0].life;t++) {
            assert(sc64_tracer_duration(0)==expected && sc64_trace_durations[0]==0);
            update_tracers();
        }
        assert(tracers[0].life==1); update_tracers(); assert(tracers[0].life==0);
        printf("long duration=%d actor=%u weapon=%u update_checks=%d\n",expected,key.actor_raw,key.weapon_id,expected);
        long_checked=1;
    }
    assert(long_checked);
    clear_tracers(); for(int i=0;i<MAX_TRACERS;i++)assert(sc64_trace_durations[i]==0);
    int generic=sc64_weapon_vfx_pack(SC64_WEAPON_VFX_STYLE_MUZZLE,0);
    for(int i=0;i<MAX_TRACERS;i++)add_tracer(100,100,101,100,generic);
    sc64_trace_durations[0]|=0xff00u; tracers[0].life=1;
    add_tracer(100,100,101,100,generic);
    assert((sc64_trace_durations[0]>>8)==0);
    printf("source profiles checked=%u cached=%u variant_fallback=%u max_2048_map_diagonal=%d profiles_over_255=%u\n",checked,cached,fallback,map_max,map_long);
    return 0;
}
