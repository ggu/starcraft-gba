# SPDX-License-Identifier: GPL-3.0-or-later
"""Exercise actual fast culler with independent, asset-free bank extremes."""
import pathlib
import subprocess
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]

class SyntheticCullTest(unittest.TestCase):
    def test_extreme_and_invalid_bounds(self):
        template = (ROOT / 'tests/sc64_weapon_source_resolved_trace.c').read_text()
        template = template.replace('int main(void)', 'int original_fixture_main(void)')
        template = template.replace('"../starcraft/', '"' + str(ROOT / 'starcraft') + '/')
        original = '{0, UINT32_MAX, 0, 0, 0, 2, 1, 2, 1, 0, 200}'
        self.assertIn(original, template)
        main = r'''
#include <limits.h>
int main(void) {
    uint16_t rx=0,ry=0; (void)rx; (void)ry;
    uint32_t bounds=sc64_weapon_source_vfx_cached_bounds();
#if EXPECT_VALID
    assert(sc64_weapon_source_vfx_bounds_unpack(bounds,&rx,&ry));
    assert(rx==32767 && ry==32767);
#else
    assert(bounds==SC64_WEAPON_SOURCE_VFX_BOUNDS_INVALID);
#endif
    Sc64WeaponSourceAttackProfile p={0}; p.projectile_image=1;
    p.weapon_bullet_type=SC64_SOURCE_BULLET_FLY;
    int vals[]={INT_MIN,-32769,-32768,-32767,-1,0,1,32766,32767,32768,INT_MAX};
    for(unsigned d=0;d<34;d++) for(unsigned i=0;i<sizeof(vals)/sizeof(vals[0]);i++)
    for(unsigned j=0;j<sizeof(vals)/sizeof(vals[0]);j++) {
        Sc64WeaponSourceVfxKey k={0,1,0,(uint8_t)d,0,0};
        int x=vals[i],cx=vals[j],actual=sc64_weapon_source_vfx_trace_provably_offscreen_resolved(k,&p,x,x,x,x,cx,cx);
#if EXPECT_VALID
        int lx,ly,expected=0;
        if(sc64_weapon_source_vfx_launch_origin_checked(&p,k.direction,x,x,&lx,&ly))
            expected=sc64_weapon_source_vfx_bounds_miss_clip(x,x,x,x,cx,cx,rx,ry);
        assert(actual==expected);
#else
        assert(actual==0);
#endif
    }
    return 0;
}
'''
        variants = ((1, '{0, UINT32_MAX, 32767, 32767, 32767, 1, 1, 0, 0, 0, 200}'),
                    (0, '{0, UINT32_MAX, -32768, 0, 0, 1, 1, 0, 0, 0, 200}'))
        with tempfile.TemporaryDirectory() as d:
            for valid, replacement in variants:
                c = pathlib.Path(d) / 'case.c'
                exe = pathlib.Path(d) / 'case'
                c.write_text(template.replace(original, replacement) + main)
                subprocess.run(['clang', '-std=c11', '-O1', '-g', '-Wall', '-Wextra',
                    '-Werror', '-Wno-unused-function', '-fsanitize=address,undefined',
                    f'-DEXPECT_VALID={valid}', str(c), '-o', str(exe)], check=True)
                subprocess.run([str(exe)], check=True)

if __name__ == '__main__':
    unittest.main()
