import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    'sc64_t4_scenario_pack', ROOT / 'tools/sc64_t4_scenario_pack.py')
PACKER = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(PACKER)


def framed(tag, payload):
    return tag + struct.pack('<I', len(payload)) + payload


def fixture_records(root):
    records_dir = root / 'records'
    records_dir.mkdir()
    loc = struct.pack('<IIIIHH', 0x10203040, 0x20304050, 0x30405060,
                      0x40506070, 0x1234, 0xABCD)

    trig = bytearray(2400)
    condition = struct.pack('<IIIHBBBBH', 0x11223344, 0x55667788,
                            0x99AABBCC, 0xDDEE, 0x11, 0x22, 0x33,
                            0x44, 0x5566)
    trig[20:40] = condition  # leave slot 0 empty to prove holes retain indices
    action = struct.pack('<IIIIIIHBBB3s', 1, 2, 3, 4, 5, 6, 0x7788,
                         0x99, 0xAA, 0xBB, b'xyz')
    action_start = 16 * 20
    trig[action_start + 32:action_start + 64] = action
    struct.pack_into('<I', trig, 2368, 0xA1B2C3D4)
    trig[2372:2399] = bytes([1, 0, 1] + [0] * 14 + [1] + [0] * 9)
    trig[2399] = 7

    doodad = struct.pack('<HHHBBH', 0x1234, 0x2345, 0x3456,
                         0x67, 0x89, 0xABCD)
    payloads = {'MRGN': loc, 'TRIG': bytes(trig), 'THG2': doodad}
    files = {}
    manifest_records = {}
    for name, (filename, tag, _) in PACKER.REQUIRED.items():
        data = framed(tag, payloads[name])
        (records_dir / filename).write_bytes(data)
        manifest_records[name] = {
            'file': filename,
            'record_bytes': len(data),
            'payload_bytes': len(payloads[name]),
            'record_sha256': hashlib.sha256(data).hexdigest(),
            'payload_sha256': hashlib.sha256(payloads[name]).hexdigest(),
        }
        files[name] = data
    manifest = {
        'schema': 1,
        'source_map_path': '008/00D',
        'source_rom_sha1': PACKER.VERIFIED_ROM_SHA1,
        'source_chk_sha256': 'a' * 64,
        'extractor_commit': 'synthetic-test-fixture',
        'records': manifest_records,
    }
    manifest_path = root / 'manifest.json'
    manifest_path.write_text(json.dumps(manifest), encoding='utf-8')
    return records_dir, manifest_path, files


class T4ScenarioPackTests(unittest.TestCase):
    def test_decodes_all_fields_and_preserves_source_slots(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            records, manifest, framed_files = fixture_records(root)
            raw_trigger = framed_files['TRIG'][8:]
            self.assertEqual(struct.unpack_from('<I', raw_trigger, 2368)[0],
                             0xA1B2C3D4)
            self.assertEqual(raw_trigger[2372], 1)   # source player 0
            self.assertEqual(raw_trigger[2389], 1)   # source all-players slot 17
            self.assertEqual(raw_trigger[2399], 7)   # initial action cursor
            self.assertEqual(raw_trigger[2371], 0xA1)  # nonzero high flag byte
            decoded = PACKER.parse_source(
                records, manifest, expected_locations=1, expected_triggers=1,
                expected_doodads=1, expected_active_conditions=1,
                expected_active_actions=1)

            location = decoded['locations'][0]
            self.assertEqual((location['left'], location['top'], location['right'],
                              location['bottom'], location['name_id'],
                              location['elevation']),
                             (0x10203040, 0x20304050, 0x30405060,
                              0x40506070, 0x1234, 0xABCD))
            self.assertEqual(location['raw'], framed_files['MRGN'][8:])
            trigger = decoded['triggers'][0]
            self.assertEqual(trigger['conditions'][0]['opcode'], 0)
            self.assertEqual(trigger['conditions'][1], {
                'location': 0x11223344, 'player': 0x55667788,
                'amount': 0x99AABBCC, 'unit': 0xDDEE,
                'comparison': 0x11, 'opcode': 0x22, 'resource': 0x33,
                'flags': 0x44, 'mask': 0x5566})
            self.assertEqual(trigger['actions'][0]['opcode'], 0)
            self.assertEqual(trigger['actions'][1]['trailing'], b'xyz')
            enabled = bytes([1, 0, 1] + [0] * 14 + [1] + [0] * 9)
            self.assertEqual(trigger['enabled_players'], enabled)
            self.assertEqual(trigger['enabled_players'][17], 1)
            self.assertEqual(trigger['execution_flags'] >> 24, 0xA1)
            self.assertEqual(trigger['initial_cursor'], 7)
            self.assertEqual(trigger['execution_flags'], 0xA1B2C3D4)
            doodad = decoded['doodads'][0]
            self.assertEqual((doodad['type'], doodad['x'], doodad['y'],
                              doodad['owner'], doodad['unused'], doodad['flags']),
                             (0x1234, 0x2345, 0x3456, 0x67, 0x89, 0xABCD))
            self.assertEqual(doodad['raw'], framed_files['THG2'][8:])

    def test_generated_header_compiles_with_fixed_layout(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            records, manifest, _ = fixture_records(root)
            data = PACKER.parse_source(
                records, manifest, expected_locations=1, expected_triggers=1,
                expected_doodads=1, expected_active_conditions=1,
                expected_active_actions=1)
            generated = root / 'scenario.h'
            generated.write_text(PACKER.emit_header(data), encoding='ascii')
            source = root / 'check.c'
            source.write_text(
                '#include "scenario.h"\n'
                'int main(void) {\n'
                '  return !(sizeof(Sc64T4Action) == 32 &&\n'
                '    sc64_t4_triggers[0].actions[1].trailing[2] == \'z\' &&\n'
                '    sc64_t4_doodads[0].flags == 0xabcd &&\n'
                '    sc64_t4_triggers[0].execution_flags == 0xa1b2c3d4u);\n'
                '}\n', encoding='ascii')
            result = subprocess.run(
                ['cc', '-std=c11', '-Wall', '-Wextra', '-Werror',
                 '-DSC64_T4_RUNTIME', '-I', str(ROOT / 'starcraft'),
                 '-I', str(root), str(source), '-o', str(root / 'check')],
                capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            run = subprocess.run([str(root / 'check')], capture_output=True)
            self.assertEqual(run.returncode, 0)

    def test_invalid_enabled_byte_rejected_before_output(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            records, manifest_path, _ = fixture_records(root)
            trig_path = records / 'TRIG.bin'
            data = bytearray(trig_path.read_bytes())
            data[8 + 2372 + 4] = 2
            trig_path.write_bytes(data)
            manifest = json.loads(manifest_path.read_text())
            framed = bytes(data)
            payload = framed[8:]
            manifest['records']['TRIG'].update(
                record_bytes=len(framed), payload_bytes=len(payload),
                record_sha256=hashlib.sha256(framed).hexdigest(),
                payload_sha256=hashlib.sha256(payload).hexdigest())
            manifest_path.write_text(json.dumps(manifest), encoding='utf-8')
            with self.assertRaisesRegex(ValueError, 'invalid enabled-player'):
                PACKER.parse_source(records, manifest_path, expected_locations=1,
                                    expected_triggers=1, expected_doodads=1,
                                    expected_active_conditions=1,
                                    expected_active_actions=1)

    def test_missing_record_and_bad_integrity_are_rejected(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            records, manifest_path, _ = fixture_records(root)
            (records / 'THG2.bin').unlink()
            with self.assertRaisesRegex(ValueError, 'cannot read THG2'):
                PACKER.parse_source(records, manifest_path, expected_locations=1,
                                    expected_triggers=1, expected_doodads=1,
                                    expected_active_conditions=1,
                                    expected_active_actions=1)

        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            records, manifest_path, _ = fixture_records(root)
            manifest = json.loads(manifest_path.read_text())
            manifest['records']['MRGN']['record_sha256'] = '0' * 64
            manifest_path.write_text(json.dumps(manifest), encoding='utf-8')
            with self.assertRaisesRegex(ValueError, 'MRGN record integrity'):
                PACKER.parse_source(records, manifest_path, expected_locations=1,
                                    expected_triggers=1, expected_doodads=1,
                                    expected_active_conditions=1,
                                    expected_active_actions=1)


if __name__ == '__main__':
    unittest.main()
