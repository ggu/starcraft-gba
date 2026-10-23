"""Asset-free tests for the generated race-map table's stable catalog IDs."""
from pathlib import Path
import hashlib
import json
import shutil
import struct
import subprocess
import tempfile
import unittest

import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))

from sc64_races_pack import (T4_REQUIRED_SOURCE_RECORDS, campaign_guard_for_map,
                             emit_descriptor_table, emit_map_count,
                             retain_source_records, retain_t4_source_records)


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
        p3_guard = campaign_guard_for_map(8)
        rows.append(
            f'#if defined(SC64_UNIFIED) && defined(SC64_P3_ENABLED) && ({p3_guard})\n'
            '    {22}, /* P3: Protoss profile only, global ID 22 */\n'
            '#else\n    {0},\n#endif\n'
        )
        t4_guard = campaign_guard_for_map(9)
        rows.append(
            f'#if defined(SC64_UNIFIED) && defined(SC64_T4_ENABLED) && ({t4_guard})\n'
            '    {7}, /* T4: import-only, fixed global ID 7 */\n'
            '#else\n    {0},\n#endif\n'
        )
        t5_guard = campaign_guard_for_map(10)
        rows.append(
            f'#if defined(SC64_UNIFIED) && defined(SC64_T5_ENABLED) && ({t5_guard})\n'
            '    {8}, /* T5: import-only, fixed global ID 8 */\n'
            '#else\n    {0},\n#endif\n'
        )
        t6_guard = campaign_guard_for_map(11)
        rows.append(
            f'#if defined(SC64_UNIFIED) && defined(SC64_T6_ENABLED) && ({t6_guard})\n'
            '    {9}, /* T6: import-only, fixed global ID 9 */\n'
            '#else\n    {0},\n#endif\n'
        )
        for index, global_id in zip(range(12, 16), range(10, 14)):
            guard = campaign_guard_for_map(index)
            rows.append(
                f'#if defined(SC64_UNIFIED) && defined(SC64_TERRAN_LATE_ENABLED) && ({guard})\n'
                f'    {{{global_id}}}, /* late Terran import slot {global_id} */\n'
                '#else\n    {0},\n#endif\n'
            )
        return rows

    def compile_case(self, flags, expected_count, expected_t3, expected_z3, expected_p3,
                     expected_t4=0, expected_t5=0, expected_t6=0,
                     expected_late=False):
        source = (
            'typedef struct { int id; } Sc64RaceMap;\n'
            + emit_map_count()
            + emit_descriptor_table(self.descriptors())
            + '\nint main(void) {\n'
            + f' if (SC64_RACE_MAP_COUNT != {expected_count}) return 1;\n'
            + ' for (int i = 0; i < 6 && i < SC64_RACE_MAP_COUNT; ++i) '
              'if (sc64_race_maps[i].id != i) return 2;\n'
            + f' if (SC64_RACE_MAP_COUNT > 6 && sc64_race_maps[6].id != {expected_t3}) return 3;\n'
            + ' if (SC64_RACE_MAP_COUNT >= 15) {\n'
            + f'  if (sc64_race_maps[7].id != {expected_t4}) return 4;\n'
            + f'  if (sc64_race_maps[8].id != {expected_t5}) return 10;\n'
            + f'  if (sc64_race_maps[9].id != {expected_t6}) return 11;\n'
            + f'  for (int i = {10 if expected_t6 else 9 if expected_t5 else 8}; i <= 13; ++i) if (sc64_race_maps[i].id != 0) return 5;\n'
            + f'  if (sc64_race_maps[14].id != {expected_z3}) return 5;\n'
            + ' } else if (SC64_RACE_MAP_COUNT == 14) {\n'
            + f'  if (sc64_race_maps[10].id != {10 if expected_late else 0}) return 12;\n'
            + f'  if (sc64_race_maps[11].id != {11 if expected_late else 0}) return 13;\n'
            + f'  if (sc64_race_maps[12].id != {12 if expected_late else 0}) return 14;\n'
            + f'  if (sc64_race_maps[13].id != {13 if expected_late else 0}) return 15;\n'
            + ' } else if (SC64_RACE_MAP_COUNT > 7) {\n'
            + f'  if (sc64_race_maps[7].id != {expected_t4}) return 8;\n'
            + ' }\n return 0;\n}\n'
        )
        source = source.replace(
            ' return 0;\n}\n',
            f' if (SC64_RACE_MAP_COUNT > 8 && sc64_race_maps[8].id != {expected_t5}) return 9;\n'
            f' if (SC64_RACE_MAP_COUNT > 9 && sc64_race_maps[9].id != {expected_t6}) return 11;\n'
            ' return 0;\n}\n')
        if expected_count == 23:
            source = source.replace(
                ' return 0;\n}\n',
                ' if (SC64_RACE_MAP_COUNT == 23) {\n'
                '  for (int i = 15; i <= 21; ++i) if (sc64_race_maps[i].id != 0) return 6;\n'
                f'  if (sc64_race_maps[22].id != {expected_p3}) return 7;\n'
                ' }\n return 0;\n}\n')
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
        self.assertEqual(campaign_guard_for_map(8),
                         '!defined(SC64_CAMPAIGN_RACE) || SC64_CAMPAIGN_RACE == 2')
        self.assertEqual(campaign_guard_for_map(9),
                         'defined(SC64_CAMPAIGN_RACE) && SC64_CAMPAIGN_RACE == 0')
        self.assertEqual(campaign_guard_for_map(11),
                         'defined(SC64_CAMPAIGN_RACE) && SC64_CAMPAIGN_RACE == 0')
        self.compile_case([], 4, 0, 0, 0)
        self.compile_case(['-DSC64_T6_ENABLED'], 4, 0, 0, 0)
        self.compile_case(['-DSC64_UNIFIED'], 6, 0, 0, 0)
        self.compile_case(['-DSC64_UNIFIED', '-DSC64_T3_ENABLED'], 7, 6, 0, 0)
        self.compile_case(['-DSC64_UNIFIED', '-DSC64_Z3_ENABLED'], 15, 0, 14, 0)
        self.compile_case(['-DSC64_UNIFIED', '-DSC64_P3_ENABLED'], 23, 0, 0, 22)
        self.compile_case(['-DSC64_UNIFIED', '-DSC64_T4_ENABLED'], 8, 0, 0, 0, 0)
        self.compile_case(['-DSC64_UNIFIED', '-DSC64_T4_ENABLED',
                           '-DSC64_CAMPAIGN_RACE=0'], 8, 0, 0, 0, 7)
        self.compile_case(['-DSC64_UNIFIED', '-DSC64_T4_ENABLED',
                           '-DSC64_CAMPAIGN_RACE=1'], 8, 0, 0, 0, 0)
        self.compile_case(['-DSC64_UNIFIED', '-DSC64_T5_ENABLED'], 9, 0, 0, 0, 0, 0)
        self.compile_case(['-DSC64_UNIFIED', '-DSC64_T5_ENABLED',
                           '-DSC64_CAMPAIGN_RACE=0'], 9, 0, 0, 0, 0, 8)
        self.compile_case(['-DSC64_UNIFIED', '-DSC64_T5_ENABLED',
                           '-DSC64_CAMPAIGN_RACE=1'], 9, 0, 0, 0, 0, 0)
        self.compile_case(['-DSC64_UNIFIED', '-DSC64_T4_ENABLED',
                           '-DSC64_T5_ENABLED', '-DSC64_T3_ENABLED',
                           '-DSC64_CAMPAIGN_RACE=0'], 9, 6, 0, 0, 7, 8)
        self.compile_case(['-DSC64_UNIFIED', '-DSC64_T4_ENABLED',
                           '-DSC64_Z3_ENABLED', '-DSC64_CAMPAIGN_RACE=0'],
                          15, 0, 0, 0, 7)
        self.compile_case(['-DSC64_UNIFIED', '-DSC64_T4_ENABLED',
                           '-DSC64_Z3_ENABLED', '-DSC64_CAMPAIGN_RACE=1'],
                          15, 0, 14, 0, 0)
        self.compile_case(['-DSC64_UNIFIED', '-DSC64_T4_ENABLED',
                           '-DSC64_P3_ENABLED', '-DSC64_CAMPAIGN_RACE=0'],
                          23, 0, 0, 0, 7)
        self.compile_case(['-DSC64_UNIFIED', '-DSC64_T4_ENABLED',
                           '-DSC64_P3_ENABLED', '-DSC64_CAMPAIGN_RACE=2'],
                          23, 0, 0, 22, 0)
        self.compile_case(['-DSC64_UNIFIED', '-DSC64_T5_ENABLED',
                           '-DSC64_Z3_ENABLED', '-DSC64_CAMPAIGN_RACE=1'],
                          15, 0, 14, 0, 0, 0)
        self.compile_case(['-DSC64_UNIFIED', '-DSC64_T5_ENABLED',
                           '-DSC64_P3_ENABLED', '-DSC64_CAMPAIGN_RACE=2'],
                          23, 0, 0, 22, 0, 0)
        self.compile_case(['-DSC64_UNIFIED', '-DSC64_T6_ENABLED',
                           '-DSC64_CAMPAIGN_RACE=0'], 10, 0, 0, 0, 0, 0, 9)
        self.compile_case(['-DSC64_UNIFIED', '-DSC64_T6_ENABLED',
                           '-DSC64_CAMPAIGN_RACE=1'], 10, 0, 0, 0)
        self.compile_case(['-DSC64_UNIFIED', '-DSC64_T5_ENABLED',
                           '-DSC64_T6_ENABLED', '-DSC64_CAMPAIGN_RACE=0'],
                          10, 0, 0, 0, 0, 8, 9)
        self.compile_case(['-DSC64_UNIFIED', '-DSC64_T6_ENABLED',
                           '-DSC64_Z3_ENABLED', '-DSC64_CAMPAIGN_RACE=1'],
                          15, 0, 14, 0)
        self.compile_case(['-DSC64_UNIFIED', '-DSC64_T6_ENABLED',
                           '-DSC64_P3_ENABLED', '-DSC64_CAMPAIGN_RACE=2'],
                          23, 0, 0, 22)
        self.compile_case(['-DSC64_UNIFIED', '-DSC64_T6_ENABLED',
                           '-DSC64_Z3_ENABLED', '-DSC64_CAMPAIGN_RACE=0'],
                          15, 0, 0, 0, 0, 0, 9)
        self.compile_case(['-DSC64_UNIFIED', '-DSC64_Z3_ENABLED',
                           '-DSC64_CAMPAIGN_RACE=0'], 15, 0, 0, 0)
        self.compile_case(['-DSC64_UNIFIED', '-DSC64_T3_ENABLED',
                           '-DSC64_Z3_ENABLED', '-DSC64_CAMPAIGN_RACE=1'],
                          15, 0, 14, 0)
        self.compile_case(['-DSC64_UNIFIED', '-DSC64_T3_ENABLED',
                           '-DSC64_Z3_ENABLED', '-DSC64_CAMPAIGN_RACE=0'],
                          15, 6, 0, 0)
        self.compile_case(['-DSC64_UNIFIED', '-DSC64_T3_ENABLED', '-DSC64_Z3_ENABLED',
                           '-DSC64_P3_ENABLED', '-DSC64_CAMPAIGN_RACE=2'],
                          23, 0, 0, 22)
        self.compile_case(['-DSC64_UNIFIED', '-DSC64_Z3_ENABLED', '-DSC64_P3_ENABLED',
                           '-DSC64_CAMPAIGN_RACE=1'], 23, 0, 14, 0)
        self.compile_case(['-DSC64_UNIFIED', '-DSC64_T3_ENABLED',
                           '-DSC64_T4_ENABLED', '-DSC64_T5_ENABLED',
                           '-DSC64_T6_ENABLED', '-DSC64_TERRAN_LATE_ENABLED',
                           '-DSC64_CAMPAIGN_RACE=0'],
                          14, 6, 0, 0, 7, 8, 9, True)
        self.compile_case(['-DSC64_UNIFIED', '-DSC64_T3_ENABLED',
                           '-DSC64_T4_ENABLED', '-DSC64_T5_ENABLED',
                           '-DSC64_T6_ENABLED', '-DSC64_TERRAN_LATE_ENABLED',
                           '-DSC64_CAMPAIGN_RACE=1'],
                          14, 0, 0, 0, 0, 0, 0, False)


class T4SourceRecordRetentionTests(unittest.TestCase):
    @staticmethod
    def synthetic_chk(missing_tag=None):
        records = []
        payloads = {
            b'UNIT': b'u1', b'DIM ': b'dims', b'ERA ': b'e', b'MTXM': b'tiles',
            b'MRGN': b'locations', b'DD2 ': b'dd2', b'THG2': b'thg2', b'TRIG': b'trig',
            b'FORC': b'force', b'OWNR': b'owners', b'SIDE': b'sides',
            b'UPRP': b'properties', b'UPUS': b'used properties',
            b'OTHR': b'ignored',
        }
        for tag, payload in payloads.items():
            if tag == missing_tag:
                continue
            records.append(tag + struct.pack('<I', len(payload)) + payload)
        return b''.join(records), payloads

    def test_writes_exact_framed_records_and_hash_manifest(self):
        chk, payloads = self.synthetic_chk()
        with tempfile.TemporaryDirectory(prefix='sc64-t4-records-') as td:
            records_dir = Path(td) / 't4' / 'records'
            result = retain_t4_source_records(
                chk, records_dir, rom_sha1='synthetic-rom', extractor_commit='synthetic-pin')
            manifest = json.loads(Path(result['manifest']).read_text())
            self.assertEqual(manifest['source_map_path'], '008/00D')
            self.assertEqual(manifest['source_rom_sha1'], 'synthetic-rom')
            self.assertEqual(manifest['source_chk_sha256'], hashlib.sha256(chk).hexdigest())
            self.assertEqual(manifest['extractor_commit'], 'synthetic-pin')
            self.assertEqual(set(manifest['records']),
                             {'UNIT', 'DIM', 'ERA', 'MTXM', 'MRGN', 'DD2',
                              'THG2', 'TRIG', 'FORC', 'OWNR', 'SIDE', 'UPRP', 'UPUS'})
            for tag, filename in T4_REQUIRED_SOURCE_RECORDS.items():
                key = tag.decode('ascii').strip()
                payload = payloads[(key + (' ' if key in ('DIM', 'ERA', 'DD2') else '')).encode()]
                record = tag + struct.pack('<I', len(payload)) + payload
                written = records_dir / filename
                self.assertEqual(written.read_bytes(), record)
                detail = manifest['records'][key]
                self.assertEqual(detail['record_bytes'], len(record))
                self.assertEqual(detail['payload_bytes'], len(payload))
                self.assertEqual(detail['record_sha256'], hashlib.sha256(record).hexdigest())
                self.assertEqual(detail['payload_sha256'], hashlib.sha256(payload).hexdigest())

    def test_generic_retainer_records_t5_path_without_changing_t4_wrapper(self):
        chk, _ = self.synthetic_chk()
        with tempfile.TemporaryDirectory(prefix='sc64-t5-records-') as td:
            root = Path(td)
            result = retain_source_records(
                chk, root / 't5' / 'records', map_path='008/00E',
                rom_sha1='synthetic-rom', extractor_commit='synthetic-pin')
            manifest = json.loads(Path(result['manifest']).read_text())
            self.assertEqual(manifest['source_map_path'], '008/00E')
            self.assertEqual(manifest['source_chk_sha256'], hashlib.sha256(chk).hexdigest())
            self.assertEqual(set(manifest['records']),
                             {tag.decode('ascii').strip() for tag in T4_REQUIRED_SOURCE_RECORDS})
            with self.assertRaisesRegex(ValueError, 'exactly one TRIG'):
                retain_source_records(
                    chk.replace(b'TRIG', b'NOPE'), root / 'bad' / 'records',
                    map_path='008/00E')

    def test_missing_required_section_is_rejected_before_writing(self):
        chk, _ = self.synthetic_chk(b'TRIG')
        with tempfile.TemporaryDirectory(prefix='sc64-t4-missing-') as td:
            records_dir = Path(td) / 't4' / 'records'
            with self.assertRaisesRegex(ValueError, 'requires exactly one TRIG'):
                retain_t4_source_records(chk, records_dir)
            self.assertFalse(records_dir.exists())


if __name__ == '__main__':
    unittest.main()
