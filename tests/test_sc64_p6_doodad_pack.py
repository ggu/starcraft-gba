import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import tempfile
import unittest
from unittest import mock


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    'sc64_p6_doodad_pack', ROOT / 'tools/sc64_p6_doodad_pack.py')
PACK = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(PACK)


def row(raw_type, x, y, owner, unused, flags):
    return struct.pack('<HHHBBH', raw_type, x, y, owner, unused, flags)


def expected_payload():
    parts = []
    for raw, count in PACK.EXPECTED_RAW_COUNTS.items():
        sprite = raw in (120, 121, 122, 124, 127, 128)
        flags = 0x1180 if sprite else 0x2280
        if raw in PACK.DOOR_RAW_TYPES:
            flags = 0x6280 if raw in (206, 208) else 0x2280
        if raw == 205:
            flags = 0xA280
        for i in range(count):
            parts.append(row(raw, 10 + i, 20 + i, 3, 0, flags))
    return b''.join(parts)


class P6DoodadPackTests(unittest.TestCase):
    def test_preserves_numeric_fields_and_marks_unsupported_explicitly(self):
        payload = (row(205, 100, 200, 0, 0, 0xA280) +
                   row(121, 300, 400, 0, 0, 0x1180) +
                   row(128, 500, 600, 0, 0, 0x1080) +
                   row(210, 700, 800, 3, 0, 0x2280) +
                   row(120, 900, 1000, 0, 0, 0x5280))
        decoded = PACK.decode_thg2(payload, 32, 32)
        self.assertEqual([r['index'] for r in decoded], list(range(5)))
        self.assertEqual([r['type'] for r in decoded], [205, 121, 128, 210, 120])
        self.assertEqual([r['source_kind'] for r in decoded],
                         ['unit_backed', 'sprite_only', 'sprite_only',
                          'unit_backed', 'sprite_only'])
        self.assertEqual([r['runtime_disposition'] for r in decoded],
                         ['unit_backed_prop', 'sprite_only', 'sprite_only',
                          'unsupported', 'sprite_only'])
        self.assertEqual([r['initial_enabled'] for r in decoded], [0, 1, 1, 1, 1])
        self.assertEqual((decoded[0]['x'], decoded[0]['y'], decoded[0]['owner'],
                          decoded[0]['unused'], decoded[0]['flags']),
                         (100, 200, 0, 0, 0xA280))
        self.assertEqual(decoded[0]['native_building'], 0)
        self.assertEqual(decoded[1]['native_building'], None)

    def test_enforces_complete_records_bounds_and_doodad_capacity(self):
        with self.assertRaisesRegex(ValueError, 'partial source record'):
            PACK.decode_thg2(b'bad', 128, 128)
        with self.assertRaisesRegex(ValueError, 'outside map bounds'):
            PACK.decode_thg2(row(205, 128 * 32, 1, 0, 0, 0), 128, 128)
        with self.assertRaisesRegex(ValueError, 'outside imported-map bounds'):
            PACK.decode_thg2(b'', 129, 128)
        with self.assertRaisesRegex(ValueError, 'byte-sized source index'):
            PACK.decode_thg2(bytes(256 * PACK.RECORD_SIZE), 128, 128)

    def test_p6_inventory_retains_all_authenticated_rows(self):
        payload = expected_payload()
        decoded = PACK.decode_thg2(payload, 128, 128)
        summary = PACK._validate_p6_rows(decoded)
        self.assertEqual(summary['all_count'], 37)
        self.assertEqual(summary['unit_backed_count'], 27)
        self.assertEqual(summary['sprite_count'], 10)
        self.assertEqual(summary['unsupported_count'], 2)
        self.assertEqual(summary['unsupported_raw_types'], [210])
        with self.assertRaisesRegex(ValueError, 'unsupported THG2 prop types'):
            PACK.require_runtime_supported(summary)
        missing = PACK.decode_thg2(payload[:-PACK.RECORD_SIZE], 128, 128)
        with self.assertRaisesRegex(ValueError, 'type counts differ'):
            PACK._validate_p6_rows(missing)

    def test_parse_verify_and_generate_use_authenticated_p6_scenario(self):
        payload = expected_payload()
        chk = b'pinned synthetic test CHK'
        digest = hashlib.sha256(chk).hexdigest()
        scenario = {'mission': 'P6', 'global_catalog_id': 25,
                    'source_map_path': '008/023', 'source_chk_sha256': digest,
                    'width': 128, 'height': 128}

        class Decoder:
            def parse_source(self, source_root, mission):
                self.mission = mission
                return scenario

            def parse_chk_sections(self, source):
                return {b'THG2': payload}

        decoder = Decoder()
        with tempfile.TemporaryDirectory() as tmp:
            source_root = Path(tmp) / 'source_maps'
            (source_root / 'p6').mkdir(parents=True)
            (source_root / 'p6' / 'source.chk').write_bytes(chk)
            with mock.patch.object(PACK, '_load_scenario_packer', return_value=decoder):
                data = PACK.parse_source(source_root)
                verified = PACK.verify_staged(source_root)
                output = Path(tmp) / 'private' / 'doodads.json'
                generated = PACK.generate(source_root, output)
                header_path = Path(tmp) / 'private' / 'sc64_p6_doodads_generated.h'
                PACK.generate_header(source_root, header_path)
            self.assertEqual(decoder.mission, 'P6')
            self.assertEqual(data['all_count'], 37)
            self.assertEqual(data['doodads'][0]['type'], 120)
            self.assertEqual(verified['unsupported_raw_types'], [210])
            self.assertEqual(generated, verified)
            saved = json.loads(output.read_text())
            self.assertEqual(len(saved['doodads']), 37)
            self.assertNotIn('pixels', output.read_text().lower())
            self.assertNotIn('dialogue', output.read_text().lower())
            header = header_path.read_text()
            self.assertIn('#define SC64_P6_DOODAD_SOURCE_COUNT 37u', header)
            self.assertIn('#define SC64_P6_DOODAD_UNIT_PROP_COUNT 27u', header)
            self.assertIn('#define SC64_P6_DOODAD_SPRITE_COUNT 10u', header)
            # Every source row survives, including raw 210 and sprite IDs that
            # must never be interpreted as UnitType IDs.
            emitted = [line.strip() for line in header.splitlines()
                       if line.startswith('    {')]
            self.assertEqual(len(emitted), 37)
            for decoded, line in zip(data['doodads'], emitted):
                values = [int(v) for v in line.strip('{},').split(',')]
                self.assertEqual(values, [decoded['type'], decoded['x'], decoded['y'],
                    decoded['flags'], decoded['owner'], decoded['index'],
                    decoded['initial_enabled'], int(decoded['source_kind'] == 'sprite_only'),
                    decoded['unused']])

    def test_authentication_and_source_inventory_fail_closed(self):
        payload = expected_payload()
        chk = b'pinned CHK'
        scenario = {'mission': 'P6', 'global_catalog_id': 25,
                    'source_map_path': '008/023',
                    'source_chk_sha256': hashlib.sha256(chk).hexdigest(),
                    'width': 128, 'height': 128}

        class Decoder:
            def parse_source(self, source_root, mission):
                return scenario

            def parse_chk_sections(self, source):
                return {b'THG2': payload}

        with tempfile.TemporaryDirectory() as tmp:
            source_root = Path(tmp)
            (source_root / 'p6').mkdir()
            (source_root / 'p6' / 'source.chk').write_bytes(b'tampered')
            with mock.patch.object(PACK, '_load_scenario_packer', return_value=Decoder()):
                with self.assertRaisesRegex(ValueError, 'changed after scenario authentication'):
                    PACK.parse_source(source_root)

        bad_scenario = dict(scenario, global_catalog_id=24)
        class WrongMissionDecoder(Decoder):
            def parse_source(self, source_root, mission):
                return bad_scenario

        with tempfile.TemporaryDirectory() as tmp:
            source_root = Path(tmp)
            (source_root / 'p6').mkdir()
            (source_root / 'p6' / 'source.chk').write_bytes(chk)
            with mock.patch.object(PACK, '_load_scenario_packer', return_value=WrongMissionDecoder()):
                with self.assertRaisesRegex(ValueError, 'not the pinned P6 map'):
                    PACK.parse_source(source_root)


if __name__ == '__main__':
    unittest.main()
