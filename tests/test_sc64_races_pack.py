"""Asset-free tests for the generated race-map table's stable catalog IDs."""
from collections import Counter
from pathlib import Path
import hashlib
import json
import shutil
import struct
import subprocess
import tempfile
import unittest
from unittest.mock import patch

import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))

from sc64_races_pack import (T4_REQUIRED_SOURCE_RECORDS, campaign_guard_for_map,
                             emit_descriptor_table, emit_map_count,
                             global_id_for_map, retain_complete_map_source,
                             retain_source_records, retain_t4_source_records,
                             source_actor_audit, source_placement_category,
                             verify_extractor_source)


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
        for index in range(16, 30):
            guard = campaign_guard_for_map(index)
            global_id = global_id_for_map(index)
            rows.append(
                f'#if {guard}\n    {{{global_id}}}, /* staged chapter data */\n'
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
        self.assertEqual(campaign_guard_for_map(16),
                         '!defined(SC64_CAMPAIGN_RACE) || SC64_CAMPAIGN_RACE == 1')
        self.assertEqual(campaign_guard_for_map(23),
                         '!defined(SC64_CAMPAIGN_RACE) || SC64_CAMPAIGN_RACE == 2')
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

    def test_explicit_data_gates_reserve_stable_high_water_ids_without_playability(self):
        cases = [
            (['-DSC64_UNIFIED', '-DSC64_Z4_DATA_ENABLED', '-DSC64_CAMPAIGN_RACE=1'], 16, 15, 1),
            (['-DSC64_UNIFIED', '-DSC64_Z10_DATA_ENABLED', '-DSC64_CAMPAIGN_RACE=1'], 22, 21, 1),
            (['-DSC64_UNIFIED', '-DSC64_P4_DATA_ENABLED', '-DSC64_CAMPAIGN_RACE=2'], 24, 23, 2),
            (['-DSC64_UNIFIED', '-DSC64_P10_DATA_ENABLED', '-DSC64_CAMPAIGN_RACE=2'], 30, 29, 2),
        ]
        for flags, expected_count, expected_id, race in cases:
            with self.subTest(flags=flags):
                source = (
                    'typedef struct { int id; } Sc64RaceMap;\n' + emit_map_count()
                    + emit_descriptor_table(self.descriptors())
                    + f'int main(void) {{ if (SC64_RACE_MAP_COUNT != {expected_count}) return 1; '
                    + f'if (sc64_race_maps[{expected_id}].id != {expected_id}) return 2; '
                    + 'return 0; }\n'
                )
                with tempfile.TemporaryDirectory(prefix='sc64-race-data-gate-') as td:
                    c_path, exe = Path(td) / 'fixture.c', Path(td) / 'fixture'
                    c_path.write_text(source)
                    subprocess.run([self.cc, '-std=c11', '-Wall', '-Wextra', '-Werror',
                                    *flags, str(c_path), '-o', str(exe)],
                                   check=True, capture_output=True, text=True)
                    subprocess.run([str(exe)], check=True, capture_output=True, text=True)
                self.assertIn(f'-DSC64_CAMPAIGN_RACE={race}', flags)
        # A mismatched profile reserves the direct ID but the race guard zeros it.
        source = ('typedef struct { int id; } Sc64RaceMap;\n' + emit_map_count()
                 + emit_descriptor_table(self.descriptors())
                 + 'int main(void) { return SC64_RACE_MAP_COUNT != 16 || '
                   'sc64_race_maps[15].id != 0; }\n')
        with tempfile.TemporaryDirectory(prefix='sc64-race-wrong-gate-') as td:
            c_path, exe = Path(td) / 'fixture.c', Path(td) / 'fixture'
            c_path.write_text(source)
            subprocess.run([self.cc, '-std=c11', '-Wall', '-Wextra', '-Werror',
                            '-DSC64_UNIFIED', '-DSC64_Z4_DATA_ENABLED',
                            '-DSC64_CAMPAIGN_RACE=2', str(c_path), '-o', str(exe)],
                           check=True, capture_output=True, text=True)
            subprocess.run([str(exe)], check=True, capture_output=True, text=True)


class FullCampaignSourceAuditTests(unittest.TestCase):
    def test_stable_catalog_id_ranges(self):
        self.assertEqual([global_id_for_map(i) for i in range(16, 23)], list(range(15, 22)))
        self.assertEqual([global_id_for_map(i) for i in range(23, 30)], list(range(23, 30)))

    def test_source_pool_classification_keeps_raw_150_and_216_distinct(self):
        rows = [{'source_type': raw} for raw in (150, 216, 214, 115, 69, 196)]
        z = source_actor_audit(rows, race=1)
        self.assertEqual(z['source_pool_demand'], {
            'unit_slots': 2, 'unit_slots_upper_bound_including_unclassified': 2,
            'building_slots': 3, 'building_slots_upper_bound_including_unclassified': 3,
            'mineral_fields': 0, 'geysers': 0,
            'start_markers_excluded_from_actor_pools': 1,
            'unclassified_actor_slots': 0,
        })
        self.assertEqual(z['unsupported_or_unclassified_raw_types'], [
            {'raw_type': 69, 'count': 1, 'source_class': 'unit'},
            {'raw_type': 115, 'count': 1, 'source_class': 'building'},
            {'raw_type': 196, 'count': 1, 'source_class': 'building'},
        ])

    def test_z7_gated_support_does_not_change_legacy_capability_report(self):
        rows = ([{'source_type': 39} for _ in range(5)] +
                [{'source_type': 44} for _ in range(5)] +
                [{'source_type': 138}, {'source_type': 140},
                 {'source_type': 52}, {'source_type': 136}])
        legacy = source_actor_audit(rows, race=1)
        self.assertEqual(legacy['source_pool_demand']['unit_slots'], 11)
        self.assertEqual(legacy['source_pool_demand']['building_slots'], 3)
        self.assertEqual(legacy['unsupported_or_unclassified_raw_types'], [
            {'raw_type': 39, 'count': 5, 'source_class': 'unit'},
            {'raw_type': 44, 'count': 5, 'source_class': 'unit'},
            {'raw_type': 52, 'count': 1, 'source_class': 'unit'},
            {'raw_type': 136, 'count': 1, 'source_class': 'building'},
            {'raw_type': 138, 'count': 1, 'source_class': 'building'},
            {'raw_type': 140, 'count': 1, 'source_class': 'building'},
        ])

        z7 = source_actor_audit(rows, race=1, support_profile='z7')
        self.assertEqual(z7['source_pool_demand'], legacy['source_pool_demand'])
        self.assertEqual(z7['unsupported_or_unclassified_raw_types'], [
            {'raw_type': 52, 'count': 1, 'source_class': 'unit'},
            {'raw_type': 136, 'count': 1, 'source_class': 'building'},
        ])

    def test_z7_support_profile_is_explicit_and_race_scoped(self):
        with self.assertRaisesRegex(ValueError, 'unknown runtime support profile'):
            source_actor_audit([{'source_type': 39}], race=1,
                               support_profile='campaign')
        with self.assertRaisesRegex(ValueError, 'only valid for Zerg'):
            source_actor_audit([{'source_type': 39}], race=2,
                               support_profile='z7')

    def test_source_placement_categories_scope_raw_195_to_z5(self):
        self.assertEqual(source_placement_category(195, 9, 'T4'), 'unclassified')
        self.assertEqual(source_placement_category(195, 10, 'T5'), 'building')
        self.assertEqual(source_placement_category(195, 16, 'Z4'), 'unit')
        self.assertEqual(source_placement_category(195, 17, 'Z5'), 'building')
        self.assertEqual(source_placement_category(214, 17, 'Z5'), 'start_marker')
        self.assertEqual(source_placement_category(150, 17, 'Z5'), 'building')

    def test_z5_synthetic_source_census_counts_beacons_as_buildings(self):
        rows = ([{'source_type': 0} for _ in range(135)] +
                [{'source_type': 195} for _ in range(7)] +
                [{'source_type': 214} for _ in range(7)])
        categories = Counter(source_placement_category(
            row['source_type'], 17, 'Z5') for row in rows)
        self.assertEqual(categories, {
            'unit': 135, 'building': 7, 'start_marker': 7,
        })

    def test_complete_source_chk_is_exact_and_bounded(self):
        chk = b'UNIT' + struct.pack('<I', 3) + b'abc' + b'TRIG' + struct.pack('<I', 0)
        with tempfile.TemporaryDirectory(prefix='sc64-source-map-') as td:
            result = retain_complete_map_source(chk, Path(td) / 'z4',
                                                map_path='008/017', mission_id='Z4',
                                                rom_sha1='fixture', extractor_commit='fixture-pin')
            self.assertEqual(Path(result['source_chk_file']).read_bytes(), chk)
            manifest = json.loads(Path(result['manifest']).read_text())
            self.assertEqual(manifest['source_chk_sha256'], hashlib.sha256(chk).hexdigest())
            self.assertEqual(manifest['source_map_path'], '008/017')
            self.assertEqual(manifest['extractor_commit'], 'fixture-pin')
            self.assertEqual([row['tag'] for row in manifest['sections']], ['UNIT', 'TRIG'])
            with self.assertRaisesRegex(ValueError, 'exactly one UNIT'):
                retain_complete_map_source(b'TRIG' + struct.pack('<I', 0),
                                           Path(td) / 'bad', map_path='008/017', mission_id='Z4')
            with self.assertRaisesRegex(ValueError, 'truncated'):
                retain_complete_map_source(chk + b'bad', Path(td) / 'truncated',
                                           map_path='008/017', mission_id='Z4')

    def test_extractor_bytes_must_match_pinned_git_blob(self):
        with tempfile.TemporaryDirectory(prefix='sc64-extractor-pin-') as td:
            path = Path(td) / 'extract_sc64_maps.py'
            path.write_bytes(b'clean pinned source')
            completed = subprocess.CompletedProcess([], 0, stdout=b'clean pinned source')
            with patch('sc64_races_pack.subprocess.run', return_value=completed) as run:
                verify_extractor_source(path)
                self.assertEqual(run.call_args.args[0][0:3],
                                 ['git', '-C', td])
            path.write_bytes(b'modified extractor at pinned HEAD')
            with patch('sc64_races_pack.subprocess.run', return_value=completed):
                with self.assertRaisesRegex(ValueError, 'differs from its committed bytes'):
                    verify_extractor_source(path)


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
