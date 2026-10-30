/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <assert.h>
#include <stdint.h>
typedef struct { uint8_t actor_raw, weapon_id, mode, variant; } Sc64WeaponSourceAttackProfile;
typedef struct { uint8_t actor_raw, weapon_id, target_air, variant; } Sc64WeaponSourceVfxKey;
#define SC64_WEAPON_SOURCE_PROFILE_COUNT 256
static const Sc64WeaponSourceAttackProfile sc64_weapon_source_profiles[SC64_WEAPON_SOURCE_PROFILE_COUNT] = {
    {1,2,0,0}, {1,2,0,1}, {2,3,1,0}
};
#include "../starcraft/sc64_trace_cache_word.h"
#include "../starcraft/sc64_trace_profile_cache.h"
int main(void) {
    const int duration[]={0,1,255,256,65535};
    for(unsigned i=0;i<5;i++) {
        unsigned word=sc64_trace_short_duration_word(duration[i]);
        assert((word ? (int)(word&255u) : duration[i])==duration[i]);
    }
    Sc64WeaponSourceVfxKey k={1,2,0,0};
    unsigned index=sc64_trace_profile_slot_index(&sc64_weapon_source_profiles[0],k);
    assert(index==1 && sc64_trace_cached_profile(index,k)==&sc64_weapon_source_profiles[0]);
    assert(!sc64_trace_cached_profile(0,k) && !sc64_trace_cached_profile(257,k));
    k.variant=1;
    assert(!sc64_trace_cached_profile(index,k));
    Sc64WeaponSourceVfxKey zero={0,0,0,0};
    assert(sc64_trace_profile_slot_index(&sc64_weapon_source_profiles[254],zero)==255);
    assert(sc64_trace_profile_slot_index(&sc64_weapon_source_profiles[255],zero)==0);
    assert(!sc64_trace_profile_slot_index(&sc64_weapon_source_profiles[0],k)); /* fallback cannot cache */
    assert(sc64_trace_cached_profile(2,k)==&sc64_weapon_source_profiles[1]);
    unsigned reused=sc64_trace_short_duration_word(1); /* replace clears old index */
    assert(!(reused>>8));
    k.actor_raw=2;k.weapon_id=3;k.target_air=1;k.variant=0;
    assert(!sc64_trace_cached_profile(index,k));
    return 0;
}
