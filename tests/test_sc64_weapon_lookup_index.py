# SPDX-License-Identifier: GPL-3.0-or-later
"""Asset-free comparison of indexed and original lookup semantics."""
import importlib.util
import hashlib
import re
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
def source_array_hashes(source):
    # Match full original-namespace array definitions, including payload bytes,
    # rather than evaluating either lookup against the transformed arrays.
    pattern = re.compile(r'^static const [^\n]*\b(sc64_weapon_source_[A-Za-z0-9_]+)\[[^\]\n]+\][ \t]*=[ \t]*\{.*?^\};', re.M | re.S)
    arrays = {}
    for match in pattern.finditer(source):
        name = match.group(1)
        if name in arrays: raise ValueError(f'duplicate array {name}')
        arrays[name] = hashlib.sha256(match.group(0).encode()).hexdigest()
    return arrays

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
    def test_original_payload_definitions_unchanged(self):
        synthetic = BANK + """static const uint8_t sc64_weapon_source_pixels[5] = {
0, 19, 255, 73, 1
};
static const uint16_t sc64_weapon_source_frames[3] = {
65535, 17, 42
};
static const uint32_t sc64_weapon_source_profiles[2] = {
123456, 987654
};
static const uint8_t sc64_weapon_source_lo_offsets[2] = {
31, 7
};
"""
        original = source_array_hashes(synthetic)
        self.assertEqual(set(original), {'sc64_weapon_source_images', 'sc64_weapon_source_animations',
                                         'sc64_weapon_source_pixels', 'sc64_weapon_source_frames',
                                         'sc64_weapon_source_profiles', 'sc64_weapon_source_lo_offsets'})
        transformed = idx.make_index(synthetic)
        self.assertEqual(source_array_hashes(transformed), original)
        names = set(re.findall(r'^static const [^\n]*\b([A-Za-z_][A-Za-z0-9_]*)\[[^\]\n]*\][ \t]*=', transformed, re.M))
        self.assertEqual(names - set(original), {'sc64_vfx_image_index', 'sc64_vfx_anim_start', 'sc64_vfx_anim_count'})
        self.assertNotEqual(source_array_hashes(transformed.replace('0, 19, 255, 73, 1', '0, 19, 254, 73, 1')), original)

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

class ManagedOutputTest(unittest.TestCase):
    def test_update_and_failure_preserve(self):
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory)/'source.h'; output = Path(directory)/'indexed.h'
            source.write_text(BANK)
            tool = str(ROOT/'tools/sc64_weapon_lookup_index.py')
            def run():
                return subprocess.run(['python3', tool, '--managed-update', str(source), str(output)], capture_output=True)
            self.assertEqual(run().returncode, 0)
            self.assertTrue(output.read_text().startswith(idx.MANAGED_MARKER))
            before = output.read_bytes()
            source.write_text('bad bank')
            self.assertNotEqual(run().returncode, 0)
            self.assertEqual(output.read_bytes(), before)
            source.write_text(BANK.replace('{741}', '{740}').replace('{741,255}', '{740,255}'))
            self.assertEqual(run().returncode, 0)
            self.assertNotEqual(output.read_bytes(), before)
            output.write_text('unmanaged')
            self.assertNotEqual(run().returncode, 0)
            self.assertEqual(output.read_text(), 'unmanaged')

    def test_make_incremental_regeneration(self):
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory)/'source.h'; output = Path(directory)/'indexed.h'
            source.write_text(BANK)
            makefile = Path(directory)/'Makefile'
            makefile.write_text(f'''{output}: {source} {ROOT / "tools/sc64_weapon_lookup_index.py"}
\tpython3 {ROOT / "tools/sc64_weapon_lookup_index.py"} --managed-update {source} {output}
''')
            subprocess.run(['make','-f',str(makefile)],check=True,capture_output=True)
            first = output.read_bytes()
            source.write_text(BANK.replace('{741}', '{740}').replace('{741,255}', '{740,255}'))
            # Explicitly make the input newer even on filesystems with coarse timestamps.
            import os
            os.utime(source, ns=(output.stat().st_mtime_ns + 1_000_000_000,)*2)
            subprocess.run(['make','-f',str(makefile)],check=True,capture_output=True)
            self.assertNotEqual(output.read_bytes(), first)
