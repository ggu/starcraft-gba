from __future__ import annotations

import importlib.util
from pathlib import Path
import shutil
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / '.local/sc64/z4'
SPEC = importlib.util.spec_from_file_location('z4_scenario_pack_test', ROOT / 'tools/sc64_z4_scenario_pack.py')
PACK = importlib.util.module_from_spec(SPEC)
assert SPEC.loader is not None
SPEC.loader.exec_module(PACK)


class Z4ScenarioPackTests(unittest.TestCase):
    def test_verified_staged_source_decodes_all_dispatch_actions(self):
        data = PACK.parse_source(SOURCE)
        self.assertEqual(data['source_chk_sha256'], '2cb8e516f07651801c1d6f519017d494af87c479a0ba8df9e33af34b14ec685a')
        self.assertEqual([t['action_count'] for t in data['triggers']], [5, 14, 1, 24, 14, 3, 5])
        self.assertEqual(len(data['actions']), 66)
        self.assertEqual((data['owners'][3], data['sides'][3]), (6, 0))
        # Keep raw action operands, including script selectors/IDs and switch modifiers.
        self.assertTrue(all(len(a['trailing']) == 3 for a in data['actions']))
        self.assertTrue(any(a['modifier'] in (4, 5) for a in data['actions']))
        self.assertTrue(any(a['group2'] == 879912282 for a in data['actions']))
        self.assertTrue(any(a['group2'] == 1395942746 for a in data['actions']))

    def test_header_is_numeric_only_and_maps_trigger_action_indices(self):
        data = PACK.parse_source(SOURCE)
        header = PACK.emit_header(data)
        self.assertIn('sc64_z4_source_actions[66]', header)
        self.assertIn('sc64_z4_source_triggers[7]', header)
        self.assertIn('sc64_z4_source_locations[64]', header)
        self.assertIn('sc64_z4_source_properties[64][20]', header)
        self.assertNotIn('Chrysalis', header)
        self.assertNotIn('\\x', header)
        self.assertIn('0x3472655au', header)
        self.assertIn('0x5334655au', header)

    def test_tampered_or_missing_staged_record_is_rejected(self):
        with tempfile.TemporaryDirectory() as td:
            copy = Path(td) / 'z4'
            shutil.copytree(SOURCE, copy)
            record = copy / 'records/TRIG.bin'
            raw = bytearray(record.read_bytes())
            raw[100] ^= 1
            record.write_bytes(raw)
            with self.assertRaisesRegex(ValueError, 'differs from the authenticated source CHK'):
                PACK.parse_source(copy)
        with tempfile.TemporaryDirectory() as td:
            with self.assertRaises(ValueError):
                PACK.parse_source(Path(td))

    def test_generate_writes_only_requested_output(self):
        with tempfile.TemporaryDirectory() as td:
            output = Path(td) / 'generated/sc64_z4_scenario_generated.h'
            report = PACK.generate(SOURCE, output)
            self.assertEqual(report['actions'], 66)
            self.assertEqual(report['triggers'], 7)
            self.assertTrue(output.is_file())
            self.assertEqual(output.read_text(encoding='ascii'), PACK.emit_header(PACK.parse_source(SOURCE)))


if __name__ == '__main__':
    unittest.main()
