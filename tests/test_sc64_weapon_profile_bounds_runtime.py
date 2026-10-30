# SPDX-License-Identifier: GPL-3.0-or-later
"""Exercise actual profile-bound selection with a tiny authored bank."""
import pathlib
import subprocess
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]

class ProfileBoundsRuntimeTest(unittest.TestCase):
    def test_provenance_fallback_and_culling(self):
        src = (ROOT / 'tests/sc64_weapon_source_resolved_trace.c').read_text()
        src = src.split('int main(void)')[0]
        src = src.replace('"../starcraft/', '"' + str(ROOT / 'starcraft') + '/')
        src = src.replace('static Sc64WeaponSourceAttackProfile test_profile;', '''
static const Sc64WeaponSourceAttackProfile sc64_weapon_source_profiles[1] = {
    {.actor_raw=5, .weapon_id=11, .projectile_image=100,
     .weapon_bullet_type=SC64_SOURCE_BULLET_FLY}
};
#define test_profile sc64_weapon_source_profiles[0]
#if BANK_VARIANT != 0
#define SC64_WEAPON_SOURCE_PROFILE_BOUNDS_VERSION BANK_VARIANT
static const uint32_t sc64_weapon_source_profile_bounds[1] = {BOUND_VALUE};
#endif
''')
        src = src.replace('{2, UINT32_MAX, 0, 0, 0, 2, 1, 2, 1, 1, 200}',
                          '{2, UINT32_MAX, 100, 0, 100, 2, 1, 2, 1, 1, 200}')
        src += r'''
int main(void) {
 const Sc64WeaponSourceAttackProfile *p=&sc64_weapon_source_profiles[0];
 Sc64WeaponSourceAttackProfile copy=*p, modified=*p;
 modified.forward_offset=99;
 uint32_t g=sc64_weapon_source_vfx_cached_bounds();
 uint16_t rx,ry; assert(sc64_weapon_source_vfx_bounds_unpack(g,&rx,&ry));
 assert(rx>=100);
#if BANK_VARIANT == 1 && BOUND_VALUE == 0x80000001u
 assert(sc64_weapon_source_vfx_profile_bounds(p)==BOUND_VALUE);
#else
 assert(sc64_weapon_source_vfx_profile_bounds(p)==g);
#endif
 assert(sc64_weapon_source_vfx_profile_bounds(&copy)==g);
 assert(sc64_weapon_source_vfx_profile_bounds(&modified)==g);
 assert(sc64_weapon_source_vfx_profile_bounds(0)==g);
 assert(sc64_weapon_source_vfx_profile_bounds(p+1)==g);
 Sc64WeaponSourceVfxKey key={5,1,11,0,0,0};
 /* Global miss remains a miss for both canonical and copied profiles. */
 assert(sc64_weapon_source_vfx_trace_provably_offscreen_resolved(key,p,-200,20,-200,20,0,0));
 assert(sc64_weapon_source_vfx_trace_provably_offscreen_resolved(key,&copy,-200,20,-200,20,0,0));
 /* A two-pixel authored frame centered at -1 has one on-screen pixel. */
 assert(!sc64_weapon_source_vfx_trace_provably_offscreen_resolved(key,p,-1,20,-1,20,0,0));
#if BANK_VARIANT == 1 && BOUND_VALUE == 0x80000001u
 assert(sc64_weapon_source_vfx_trace_provably_offscreen_resolved(key,p,-2,20,-2,20,0,0));
 assert(!sc64_weapon_source_vfx_trace_provably_offscreen_resolved(key,&copy,-2,20,-2,20,0,0));
#else
 assert(!sc64_weapon_source_vfx_trace_provably_offscreen_resolved(key,p,-2,20,-2,20,0,0));
#endif
 /* The wide path intentionally uses global bounds for the canonical record. */
 for(int x=-40020;x<=-39980;++x)
   assert(sc64_weapon_source_vfx_trace_provably_offscreen_resolved(key,p,x,20,x,20,-40000,0)==
          sc64_weapon_source_vfx_trace_provably_offscreen_resolved(key,&copy,x,20,x,20,-40000,0));
 return 0;
}
'''
        with tempfile.TemporaryDirectory() as d:
            source = pathlib.Path(d) / 'runtime.c'
            source.write_text(src)
            for variant, value in ((1, '0x80000001u'), (1, '0x40000000u'),
                                   (2, '0x80000001u'), (0, '0x80000001u')):
                binary = pathlib.Path(d) / f'test-{variant}-{value}'
                subprocess.run(['clang', '-std=c11', '-O1', '-g', '-Wall', '-Wextra',
                                '-Werror', '-Wno-unused-function', '-Wno-unused-variable',
                                '-fsanitize=address,undefined', f'-DBANK_VARIANT={variant}',
                                f'-DBOUND_VALUE={value}', str(source), '-o', str(binary)],
                               check=True, capture_output=True)
                subprocess.run([str(binary)], check=True, capture_output=True)

if __name__ == '__main__':
    unittest.main()
