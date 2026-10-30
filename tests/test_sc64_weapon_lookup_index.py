# SPDX-License-Identifier: GPL-3.0-or-later
"""Asset-free comparison of indexed and original lookup semantics."""
import importlib.util
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('idx', ROOT / 'tools/sc64_weapon_lookup_index.py')
idx = importlib.util.module_from_spec(spec)
spec.loader.exec_module(idx)
BANK = '''#include <stdint.h>
#define SC64_WEAPON_SOURCE_IMAGE_COUNT 3
#define SC64_WEAPON_SOURCE_ANIMATION_COUNT 4
typedef struct { uint16_t image_id; } Sc64WeaponSourceImage;
typedef struct { uint16_t image_id; uint8_t animation_id; } Sc64WeaponSourceAnimation;
static const Sc64WeaponSourceImage sc64_weapon_source_images[3] = {
{0}, {3}, {741}
};
static const Sc64WeaponSourceAnimation sc64_weapon_source_animations[4] = {
{0,0}, {0,7}, {3,1}, {741,255}
};
static inline const Sc64WeaponSourceImage *sc64_weapon_source_image_lookup(uint16_t id) { for(unsigned i=0;i<3;i++) if(sc64_weapon_source_images[i].image_id==id)return &sc64_weapon_source_images[i];return 0; }
static inline const Sc64WeaponSourceAnimation *sc64_weapon_source_animation_lookup(uint16_t id,uint8_t anim) { for(unsigned i=0;i<4;i++) if(sc64_weapon_source_animations[i].image_id==id&&sc64_weapon_source_animations[i].animation_id==anim)return &sc64_weapon_source_animations[i];return 0; }
'''
class IndexTest(unittest.TestCase):
    def test_exhaustive(self):
        derived = idx.make_index(BANK)
        ref = BANK.replace('sc64_weapon_source_image_lookup(', 'reference_image(', 1).replace('sc64_weapon_source_animation_lookup(', 'reference_anim(', 1)
        # References use the indexed header's same data; remove duplicate declarations.
        functions = '\n'.join(line for line in ref.splitlines() if line.startswith('static inline'))
        code = derived + functions + '''\n#include <assert.h>
int main(void) { for(unsigned i=0;i<65536;i++){assert(sc64_weapon_source_image_lookup(i)==reference_image(i));for(unsigned j=0;j<256;j++)assert(sc64_weapon_source_animation_lookup(i,j)==reference_anim(i,j));}return 0; }
'''
        with tempfile.TemporaryDirectory() as directory:
            src = Path(directory)/'test.c'; binary = Path(directory)/'test'; src.write_text(code)
            subprocess.run(['clang','-std=c11','-O2','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',str(src),'-o',str(binary)],check=True)
            subprocess.run([str(binary)],check=True)
    def test_reject_invalid(self):
        for bad in [BANK.replace('{0}, {3}, {741}', '{0}, {0}, {741}'),
                    BANK.replace('{741,255}', '{741,256}'),
                    BANK.replace('{741}', '{5000}')]:
            with self.assertRaises(ValueError): idx.make_index(bad)
    def test_logical_counts_and_format_fail_closed(self):
        bad_banks = [BANK.replace('SC64_WEAPON_SOURCE_IMAGE_COUNT 3', 'SC64_WEAPON_SOURCE_IMAGE_COUNT 0'),
                     BANK.replace('SC64_WEAPON_SOURCE_ANIMATION_COUNT 4', 'SC64_WEAPON_SOURCE_ANIMATION_COUNT 3'),
                     BANK.replace('SC64_WEAPON_SOURCE_ANIMATION_COUNT 4', 'SC64_WEAPON_SOURCE_ANIMATION_COUNT 0').replace('animations[4]', 'animations[1]').replace('{0,0}, {0,7}, {3,1}, {741,255}', '{0,0}'),
                     BANK.replace('IMAGE_COUNT 3', 'IMAGE_COUNT 0').replace('images[3]', 'images[1]').replace('{0}, {3}, {741}', '{0}'),
                     BANK.replace('lookup(uint16_t id) { for', 'lookup(uint16_t id) {\n for', 1),
                     BANK.replace('lookup(uint16_t id) { for', 'lookup(uint16_t id) { { for', 1),
                     idx.make_index(BANK)]
        for bad in bad_banks:
            with self.subTest(bank=bad[:70]), self.assertRaises(ValueError): idx.make_index(bad)

    def test_output_guard_and_aliases(self):
        with tempfile.TemporaryDirectory() as d:
            directory = Path(d); src = directory/'source.h'; src.write_text(BANK)
            source_bytes = src.read_bytes()
            existing = directory/'existing.h'; existing.write_text('keep')
            alias = directory/'alias.h'; alias.symlink_to(src)
            for dest in (src, existing, alias, ROOT/'tests'/'private-output-must-not-exist.h'):
                with self.subTest(dest=dest), self.assertRaises((ValueError, FileExistsError)):
                    idx.validate_output(src, dest)
            self.assertEqual(src.read_bytes(), source_bytes)
            self.assertEqual(existing.read_text(), 'keep')
            self.assertFalse((ROOT/'tests'/'private-output-must-not-exist.h').exists())
            idx.validate_output(src, directory/'new.h')
            idx.validate_output(src, ROOT/'.local'/'allowed-new-private-output.h')
            # Even an ignored-looking symlink cannot target a tracked directory.
            ignored = ROOT/'.local'/'lookup-index-guard-test'
            ignored.mkdir(parents=True, exist_ok=True)
            link = ignored/'redirect'
            link.symlink_to(ROOT/'tests', target_is_directory=True)
            try:
                with self.assertRaises(ValueError): idx.validate_output(src, link/'private-output-must-not-exist.h')
            finally:
                link.unlink(); ignored.rmdir()
if __name__ == '__main__': unittest.main()
