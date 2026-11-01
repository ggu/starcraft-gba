/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Prepared-bank provenance and fallback checks for profile-specific bounds. */
#define main original_cull_bank_main
#include "sc64_weapon_source_vfx_cull_bank.c"
#undef main
int main(void)
{
    uint32_t global=sc64_weapon_source_vfx_cached_bounds();
    unsigned bounded=0, fallback=0;
    assert((global & 0xc0000000u)==SC64_WEAPON_SOURCE_VFX_BOUNDS_VALID);
    assert(sc64_weapon_source_vfx_profile_bounds(0)==global);
    for(unsigned i=0;i<SC64_WEAPON_SOURCE_PROFILE_COUNT;i++) {
        const Sc64WeaponSourceAttackProfile *p=&sc64_weapon_source_profiles[i];
        Sc64WeaponSourceAttackProfile copy=*p;
        uint32_t selected=sc64_weapon_source_vfx_profile_bounds(p);
        uint32_t entry=sc64_weapon_source_profile_bounds[i];
        assert(sc64_weapon_source_vfx_profile_bounds(&copy)==global);
        copy.projectile_image^=1; copy.attack_image^=1;
        assert(sc64_weapon_source_vfx_profile_bounds(&copy)==global);
        assert(sc64_weapon_source_vfx_profile_bounds((const Sc64WeaponSourceAttackProfile *)(const void *)((const uint8_t *)p+1))==global);
        if ((entry & 0xc0000000u)==SC64_WEAPON_SOURCE_VFX_BOUNDS_VALID) {
            uint16_t ax,ay,gx,gy; assert(selected==entry);
            assert(sc64_weapon_source_vfx_bounds_unpack(selected,&ax,&ay));
            assert(sc64_weapon_source_vfx_bounds_unpack(global,&gx,&gy));
            assert(ax<=gx && ay<=gy);++bounded;
        } else {assert(selected==global);++fallback;}
    }
    assert(sc64_weapon_source_vfx_profile_bounds(sc64_weapon_source_profiles+SC64_WEAPON_SOURCE_PROFILE_COUNT)==global);
    printf("prepared profile bounds: %u bounded, %u fallback\n",bounded,fallback);
    return 0;
}
