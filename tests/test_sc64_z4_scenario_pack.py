from __future__ import annotations

import importlib.util
from pathlib import Path
import shutil
import struct
import tempfile
import types
import unittest
from unittest import mock

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / '.local/sc64/z4'
SPEC = importlib.util.spec_from_file_location('z4_scenario_pack_test', ROOT / 'tools/sc64_z4_scenario_pack.py')
PACK = importlib.util.module_from_spec(SPEC)
assert SPEC.loader is not None
SPEC.loader.exec_module(PACK)


@unittest.skipUnless((SOURCE / 'manifest.json').is_file(),
                     'requires optional authenticated private Z4 inputs')
class Z4StagedScenarioPackTests(unittest.TestCase):
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


def synthetic_source(root: Path) -> bytes:
    """Construct invented records; no source map or extracted bytes are used."""
    triggers = bytearray(7 * 2400)
    for trigger, count in enumerate(PACK.ACTION_COUNTS):
        start = trigger * 2400
        struct.pack_into('<IIIHBBBBH', triggers, start,
                         trigger + 1, 13, 0x1200 + trigger, 37, 2, 3, 0, 1, 0)
        for index in range(count):
            struct.pack_into('<IIIIIIHBBB3s', triggers, start + 320 + index * 32,
                             index + 1, 700 + index, 900 + index, 1100 + index,
                             13, 0x12340000 + index, 37, 11, 2, 16, b'\1\2\3')
        triggers[start + 2372 + 18] = 1
    sections = {
        b'TRIG': bytes(triggers),
        b'MRGN': b''.join(struct.pack('<IIIIHH', i, i + 1, i + 2, i + 3,
                                      i + 4, 0) for i in range(64)),
        b'OWNR': bytes(range(12)), b'SIDE': bytes(range(12)),
        b'FORC': bytes(range(20)), b'UPRP': bytes(1280), b'UPUS': bytes(64),
        b'STR ': b'INVENTED PRIVATE TEXT MUST NOT BE EMITTED',
    }
    def framed(tag, payload):
        return tag + struct.pack('<I', len(payload)) + payload
    chk = b''.join(framed(tag, payload) for tag, payload in sections.items())
    (root / 'records').mkdir()
    (root / 'source.chk').write_bytes(chk)
    for tag, payload in sections.items():
        if tag == b'STR ': continue
        (root / 'records' / (tag.decode('ascii') + '.bin')).write_bytes(framed(tag, payload))
    return chk


class Z4AssetFreeScenarioPackTests(unittest.TestCase):
    def test_numeric_roundtrip_and_verifier_boundary(self):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            synthetic_source(root)
            verifier = types.SimpleNamespace(verify_staged=mock.Mock())
            with mock.patch.object(PACK, '_load_pack', return_value=verifier):
                data = PACK.parse_source(root)
                verifier.verify_staged.assert_called_once_with(root)
                header = PACK.emit_header(data)
            self.assertEqual(len(data['actions']), 66)
            self.assertEqual(len(data['conditions']), 7)
            row = data['actions'][1]
            self.assertEqual((row['trigger'], row['index'], row['modifier']), (0, 1, 2))
            self.assertEqual(row['group2'], 0x12340001)
            self.assertEqual(row['trailing'], [1, 2, 3])
            self.assertIn('sc64_z4_source_actions[66]', header)
            self.assertNotIn('INVENTED PRIVATE TEXT', header)

    def test_authentication_failure_prevents_generation(self):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            synthetic_source(root)
            output = root / 'generated.h'
            verifier = types.SimpleNamespace(verify_staged=mock.Mock(
                side_effect=ValueError('synthetic authentication failure')))
            with mock.patch.object(PACK, '_load_pack', return_value=verifier):
                with self.assertRaisesRegex(ValueError, 'authentication failure'):
                    PACK.generate(root, output)
            self.assertFalse(output.exists())

    def test_framed_record_tampering_is_rejected_after_authentication(self):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            synthetic_source(root)
            record = root / 'records/TRIG.bin'
            raw = bytearray(record.read_bytes()); raw[100] ^= 1
            record.write_bytes(raw)
            verifier = types.SimpleNamespace(verify_staged=lambda _: None)
            with mock.patch.object(PACK, '_load_pack', return_value=verifier):
                with self.assertRaisesRegex(ValueError, 'differs from source CHK'):
                    PACK.parse_source(root)

    def test_generate_creates_requested_output_from_invented_records(self):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td)
            synthetic_source(root)
            output = root / 'nested/generated.h'
            verifier = types.SimpleNamespace(verify_staged=lambda _: None)
            with mock.patch.object(PACK, '_load_pack', return_value=verifier):
                report = PACK.generate(root, output)
                expected = PACK.emit_header(PACK.parse_source(root))
            self.assertEqual(report['actions'], 66)
            self.assertEqual(report['triggers'], 7)
            self.assertEqual(output.read_text(encoding='ascii'), expected)


if __name__ == '__main__':
    unittest.main()
