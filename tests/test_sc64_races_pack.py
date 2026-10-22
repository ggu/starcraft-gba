"""Asset-free tests for the generated race-map table's stable catalog IDs."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))

from sc64_races_pack import (campaign_guard_for_map, emit_descriptor_table,
                             emit_map_count)


class RaceMapTableEmissionTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.cc = shutil.which('clang') or shutil.which('cc')

    def setUp(self):
        if not self.cc:
            self.skipTest('clang or cc is required to exercise preprocessor guards')

    @staticmethod
    def descriptors():
        rows = [f'    {{{i}}}, /* map {i} */\n' for i in range(6)]
        t3_guard = campaign_guard_for_map(6)
        rows.append(
            f'#if {t3_guard}\n'
            '    {6}, /* T3: Terran profile only */\n'
            '#else\n    {0},\n#endif\n'
        )
        z3_guard = campaign_guard_for_map(7)
        rows.append(
            f'#if {z3_guard}\n'
            '    {14}, /* Z3: Zerg profile only, global ID 14 */\n'
            '#else\n    {0},\n#endif\n'
        )
        return rows

    def compile_case(self, flags, expected_count, expected_t3, expected_z3):
        source = (
            'typedef struct { int id; } Sc64RaceMap;\n'
            + emit_map_count()
            + emit_descriptor_table(self.descriptors())
            + '\nint main(void) {\n'
            + f' if (SC64_RACE_MAP_COUNT != {expected_count}) return 1;\n'
            + ' for (int i = 0; i < 6 && i < SC64_RACE_MAP_COUNT; ++i) '
              'if (sc64_race_maps[i].id != i) return 2;\n'
            + f' if (SC64_RACE_MAP_COUNT > 6 && sc64_race_maps[6].id != {expected_t3}) return 3;\n'
            + ' if (SC64_RACE_MAP_COUNT == 15) {\n'
            + '  for (int i = 7; i <= 13; ++i) if (sc64_race_maps[i].id != 0) return 4;\n'
            + f'  if (sc64_race_maps[14].id != {expected_z3}) return 5;\n'
            + ' }\n return 0;\n}\n'
        )
        with tempfile.TemporaryDirectory(prefix='sc64-map-table-') as directory:
            c_path = Path(directory) / 'fixture.c'
            exe_path = Path(directory) / 'fixture'
            c_path.write_text(source)
            subprocess.run([self.cc, '-std=c11', '-Wall', '-Wextra', '-Werror',
                            *flags, str(c_path), '-o', str(exe_path)],
                           check=True, capture_output=True, text=True)
            subprocess.run([str(exe_path)], check=True, capture_output=True, text=True)

    def test_flag_matrix_keeps_direct_global_ids_and_unavailable_holes(self):
        self.assertEqual(campaign_guard_for_map(6),
                         '!defined(SC64_CAMPAIGN_RACE) || SC64_CAMPAIGN_RACE == 0')
        self.assertEqual(campaign_guard_for_map(7),
                         '!defined(SC64_CAMPAIGN_RACE) || SC64_CAMPAIGN_RACE == 1')
        self.compile_case([], 4, 0, 0)
        self.compile_case(['-DSC64_UNIFIED'], 6, 0, 0)
        self.compile_case(['-DSC64_UNIFIED', '-DSC64_T3_ENABLED'], 7, 6, 0)
        self.compile_case(['-DSC64_UNIFIED', '-DSC64_Z3_ENABLED'], 15, 0, 14)
        self.compile_case(['-DSC64_UNIFIED', '-DSC64_Z3_ENABLED',
                           '-DSC64_CAMPAIGN_RACE=0'], 15, 0, 0)
        self.compile_case(['-DSC64_UNIFIED', '-DSC64_T3_ENABLED',
                           '-DSC64_Z3_ENABLED', '-DSC64_CAMPAIGN_RACE=1'],
                          15, 0, 14)
        self.compile_case(['-DSC64_UNIFIED', '-DSC64_T3_ENABLED',
                           '-DSC64_Z3_ENABLED', '-DSC64_CAMPAIGN_RACE=0'],
                          15, 6, 0)


if __name__ == '__main__':
    unittest.main()
