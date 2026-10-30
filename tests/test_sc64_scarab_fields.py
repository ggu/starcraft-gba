# SPDX-License-Identifier: GPL-3.0-or-later
"""Compile the field test with the current engine's actual compact Unit."""
import pathlib
import re
import subprocess
import tempfile
import unittest
ROOT = pathlib.Path(__file__).resolve().parents[1]
class ScarabFieldsTest(unittest.TestCase):
    def test_actual_unit_and_codecs(self):
        game = (ROOT / 'starcraft/game.c').read_text()
        end = game.index('} Unit;') + len('} Unit;')
        start = game.rindex('typedef struct {', 0, end)
        definition = game[start:end]
        fixture = (ROOT / 'tests/sc64_scarab_fields.c').read_text()
        fixture = fixture.replace('/* SC64_REAL_UNIT_DEFINITION */', definition)
        # Capture the actual legacy save/load loops, including the flag
        # widening and BC conditional, so this exercises production casts.
        save = (ROOT / 'starcraft/race_save_adapter.h').read_text()
        encode_start = save.index('    for(int i=0;i<MAX_UNITS;i++)for(unsigned j=0;j<RACE_UNIT_WORDS;j++) {', save.index('static int race_save_game(void)'))
        encode_end = save.index('\n#endif\n#if defined(SC64_T6_STORAGE)', encode_start)
        decode_start = save.index('    for(int i=0;i<MAX_UNITS;i++)for(unsigned j=0;j<RACE_UNIT_WORDS;j++) {', save.index('static int race_load_game(void)'))
        decode_end = save.index('\n#endif\n#if defined(SC64_T6_STORAGE)', decode_start)
        fixture = fixture.replace('/* SC64_REAL_LEGACY_ENCODE */', save[encode_start:encode_end])
        fixture = fixture.replace('/* SC64_REAL_LEGACY_DECODE */', save[decode_start:decode_end])
        fixture = fixture.replace('"../starcraft/', '"' + str(ROOT / 'starcraft') + '/')
        # These are the enum-affecting capabilities in the four prepared
        # Terran, Zerg, P7 and Showcase vectors, with the shared roster enabled.
        profiles = {
            'terran': 'T4_RUNTIME T5_RUNTIME TERRAN_LATE_ACTORS',
            'zerg': 'ZP_CAMPAIGN_ACTORS Z5_ACTORS Z6_ACTORS',
            'p7': 'ZP_CAMPAIGN_ACTORS Z5_ACTORS Z6_ACTORS P5_ACTORS P6_ACTORS P7_ACTORS P7_PROJECTILES_ENABLED',
            'showcase': 'ZP_CAMPAIGN_ACTORS Z5_ACTORS Z6_ACTORS P5_ACTORS P6_ACTORS P7_ACTORS',
        }
        with tempfile.TemporaryDirectory() as d:
            source = pathlib.Path(d) / 'test.c'; binary = pathlib.Path(d) / 'test'
            source.write_text(fixture)
            common = ['clang', '-std=c11', '-O1', '-g', '-Wall', '-Wextra', '-Werror', '-Wno-unused-function', '-Wno-tautological-constant-out-of-range-compare', '-fsanitize=address,undefined', '-DSC64_LATE_TERRAN_STORAGE', '-DSC64_SKIRMISH_ROSTER', '-DSC64_BATTLECRUISER_EXPERIMENT']
            for name, flags in profiles.items():
                with self.subTest(profile=name):
                    defines = ['-DSC64_' + flag for flag in flags.split()]
                    # Derive names from the actual preprocessed old enum and
                    # compare their numeric values in both compilations.
                    include = '#include "sc64_stock_profiles.h"\n#include "race_rules.h"\n'
                    probe = pathlib.Path(d) / 'ids.c'
                    probe.write_text(include)
                    pp = subprocess.check_output(['clang', '-E', '-P', '-I', str(ROOT / 'starcraft'), *defines, '-DSC64_SKIRMISH_ROSTER', '-DSC64_BATTLECRUISER_EXPERIMENT', str(probe)], text=True)
                    enum = re.search(r'enum\s*\{([^{}]*\bSC_RU_COUNT\s*)\}', pp).group(1)
                    names = re.findall(r'\bU_[A-Z0-9_]+\b', enum)
                    probe.write_text(include + '#include <stdio.h>\nint main(void) {\n' + ''.join('printf("%d,", ' + n + ');\n' for n in names) + 'printf("%d\\n", SC_RU_COUNT); return 0; }\n')
                    outputs = []
                    for extra in ([], ['-DSC64_SCARAB_RUNTIME']):
                        subprocess.run(['clang', '-std=c11', '-I', str(ROOT / 'starcraft'), *defines, '-DSC64_SKIRMISH_ROSTER', '-DSC64_BATTLECRUISER_EXPERIMENT', *extra, str(probe), '-o', str(binary)], check=True)
                        outputs.append(subprocess.check_output([str(binary)], text=True).strip().split(','))
                    self.assertEqual(outputs[0][:-1], outputs[1][:-1])
                    self.assertEqual(int(outputs[1][-1]), int(outputs[0][-1]) + 1)
                    for storage in ('compact', 'legacy'):
                        with self.subTest(storage=storage):
                            flags = common if storage == 'compact' else [x for x in common if x != '-DSC64_LATE_TERRAN_STORAGE']
                            subprocess.run([*flags, *defines, '-DSC64_SCARAB_RUNTIME', str(source), '-o', str(binary)], check=True)
                            subprocess.run([str(binary)], check=True)
if __name__ == '__main__': unittest.main()
