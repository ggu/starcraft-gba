/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Optional prepared-input integration check of real tracer slots. */
#include "../starcraft/game.c"
#include <assert.h>
#include <stdio.h>
int main(void) {
    unsigned checked=0, cached=0, fallback=0;
    clear_tracers();
    for(unsigned n=0;n<SC64_WEAPON_SOURCE_PROFILE_COUNT;n++) {
        const Sc64WeaponSourceAttackProfile *p=&sc64_weapon_source_profiles[n];
        Sc64WeaponSourceVfxKey key={.actor_raw=p->actor_raw,.weapon_id=p->weapon_id,.target_air=p->mode,.variant=p->variant,.direction=8};
        uint32_t packed;
        if(!sc64_weapon_source_vfx_pack(key,&packed))continue;
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
    clear_tracers(); for(int i=0;i<MAX_TRACERS;i++)assert(sc64_trace_durations[i]==0);
    int generic=sc64_weapon_vfx_pack(SC64_WEAPON_VFX_STYLE_MUZZLE,0);
    for(int i=0;i<MAX_TRACERS;i++)add_tracer(100,100,101,100,generic);
    sc64_trace_durations[0]|=0xff00u; tracers[0].life=1;
    add_tracer(100,100,101,100,generic);
    assert((sc64_trace_durations[0]>>8)==0);
    printf("source profiles checked=%u cached=%u variant_fallback=%u\n",checked,cached,fallback);
    return 0;
}
