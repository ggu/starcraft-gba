import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]


def load(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


AVAIL = load('sc64_t5_availability_test', ROOT / 'tools/sc64_t5_availability.py')
PACKER = load('sc64_races_pack_test', ROOT / 'tools/sc64_races_pack.py')


class T5AvailabilityTests(unittest.TestCase):
    def fixture(self, root):
        records = root / 'records'
        records.mkdir()
        payload = bytearray(AVAIL.PUNI_BYTES)
        # Raw 5 has a global default on, but player 0 overrides it off.
        payload[AVAIL.PLAYERS * AVAIL.UNITS + 5] = 1
        payload[5] = 0
        payload[AVAIL.PLAYERS * AVAIL.UNITS + AVAIL.UNITS + 5] = 0
        payload[AVAIL.PLAYERS * AVAIL.UNITS + AVAIL.UNITS + AVAIL.UNITS + 5] = 1
        # Player 1 explicitly enables raw 1 while the global default is off.
        payload[AVAIL.UNITS + 1] = 7
        payload[AVAIL.PLAYERS * AVAIL.UNITS + AVAIL.UNITS + AVAIL.UNITS + 1] = 0
        # Explicit zero is distinct from default on.
        payload[2 * AVAIL.UNITS + 5] = 0
        payload[AVAIL.PLAYERS * AVAIL.UNITS + AVAIL.UNITS + 2 * AVAIL.UNITS + 5] = 0
        payload = bytes(payload)
        framed = b'PUNI' + struct.pack('<I', len(payload)) + payload
        (records / 'PUNI.bin').write_bytes(framed)
        manifest = {
            'schema': 1,
            'source_map_path': AVAIL.EXPECTED_MAP,
            'source_rom_sha1': 'rom-pin',
            'source_chk_sha256': 'chk-pin',
            'extractor': 'extractor-pin',
            'extractor_commit': 'commit-pin',
            'records': {'PUNI': {
                'file': 'PUNI.bin', 'record_bytes': len(payload) + 8,
                'payload_bytes': len(payload),
                'record_sha256': hashlib.sha256(framed).hexdigest(),
                'payload_sha256': hashlib.sha256(payload).hexdigest(),
            }},
        }
        manifest_path = root / 'manifest.json'
        manifest_path.write_text(json.dumps(manifest))
        return records, manifest_path, payload

    def read_fixture(self, records, manifest):
        return AVAIL.read_puni(
            records, manifest, expected_rom_sha1='rom-pin',
            expected_extractor='extractor-pin',
            expected_extractor_commit='commit-pin',
            expected_chk_sha256='chk-pin',
            expected_puni_sha256=hashlib.sha256((records / 'PUNI.bin').read_bytes()[8:]).hexdigest())

    def test_effective_table_matches_every_raw_lookup_and_generated_header(self):
        with tempfile.TemporaryDirectory(prefix='sc64-puni-') as td:
            records, manifest, payload = self.fixture(Path(td))
            parsed, _ = self.read_fixture(records, manifest)
            self.assertEqual(parsed, payload)
            bits = AVAIL.effective_bits(parsed)
            for player in range(12):
                for raw in range(228):
                    idx = player * 228 + raw
                    got = (bits[idx >> 3] >> (idx & 7)) & 1
                    start = player * 228 + raw
                    default = parsed[2736 + raw]
                    selector = parsed[2964 + start]
                    expected = int((default if selector else parsed[start]) != 0)
                    self.assertEqual(got, expected, (player, raw))
            self.assertEqual(len(bits), 342)
            self.assertEqual((bits[5 >> 3] >> (5 & 7)) & 1, 0)
            self.assertEqual((bits[(228 + 1) >> 3] >> ((228 + 1) & 7)) & 1, 1)
            output = Path(td) / 'generated.h'
            result = AVAIL.generate(
                records, manifest, output, expected_rom_sha1='rom-pin',
                expected_extractor='extractor-pin',
                expected_extractor_commit='commit-pin',
                expected_chk_sha256='chk-pin',
                expected_puni_sha256=hashlib.sha256(payload).hexdigest())
            self.assertEqual(result['effective_bit_bytes'], 342)
            self.assertIn('SC64_T5_AVAILABILITY_BIT_BYTES 342u', output.read_text())

    def test_manifest_and_payload_integrity_are_fail_closed(self):
        with tempfile.TemporaryDirectory(prefix='sc64-puni-bad-') as td:
            records, manifest_path, payload = self.fixture(Path(td))
            manifest = json.loads(manifest_path.read_text())
            manifest['source_map_path'] = '008/012'
            manifest_path.write_text(json.dumps(manifest))
            with self.assertRaisesRegex(ValueError, 'T5'):
                self.read_fixture(records, manifest_path)
            manifest['source_map_path'] = AVAIL.EXPECTED_MAP
            manifest['records']['PUNI']['payload_sha256'] = '0' * 64
            manifest_path.write_text(json.dumps(manifest))
            with self.assertRaisesRegex(ValueError, 'hash'):
                self.read_fixture(records, manifest_path)
            manifest['records']['PUNI']['payload_sha256'] = hashlib.sha256(payload).hexdigest()
            manifest['records']['PUNI']['record_sha256'] = '1' * 64
            manifest_path.write_text(json.dumps(manifest))
            with self.assertRaisesRegex(ValueError, 'framed-record hash'):
                self.read_fixture(records, manifest_path)
            manifest['records']['PUNI']['record_sha256'] = hashlib.sha256(
                b'PUNI' + struct.pack('<I', len(payload)) + payload).hexdigest()
            manifest_path.write_text(json.dumps(manifest))
            (records / 'PUNI.bin').write_bytes(b'PUNI' + struct.pack('<I', len(payload)) + payload[:-1])
            with self.assertRaisesRegex(ValueError, 'framed CHK record'):
                self.read_fixture(records, manifest_path)

    def test_t6_profile_uses_its_own_source_identity_and_symbols(self):
        with tempfile.TemporaryDirectory(prefix='sc64-t6-puni-') as td:
            records, manifest_path, payload = self.fixture(Path(td))
            manifest = json.loads(manifest_path.read_text())
            manifest['source_map_path'] = AVAIL.PROFILES['t6']['map']
            manifest_path.write_text(json.dumps(manifest))
            output = Path(td) / 't6.h'
            result = AVAIL.generate(
                records, manifest_path, output, profile='t6',
                expected_rom_sha1='rom-pin', expected_extractor='extractor-pin',
                expected_extractor_commit='commit-pin',
                expected_chk_sha256='chk-pin',
                expected_puni_sha256=hashlib.sha256(payload).hexdigest())
            self.assertEqual(result['map'], '008/00F')
            header = output.read_text()
            self.assertIn('SC64_T6_AVAILABILITY_BIT_BYTES 342u', header)
            self.assertIn('sc64_t6_availability_bits[', header)
            self.assertNotIn('SC64_T5_', header)
            with self.assertRaisesRegex(ValueError, 'T5'):
                self.read_fixture(records, manifest_path)

    def test_t6_rejects_rehashed_source_changes_without_replacing_output(self):
        with tempfile.TemporaryDirectory(prefix='sc64-t6-puni-pin-') as td:
            records, manifest_path, payload = self.fixture(Path(td))
            manifest = json.loads(manifest_path.read_text())
            manifest['source_map_path'] = AVAIL.PROFILES['t6']['map']
            modified = bytes([payload[0] ^ 1]) + payload[1:]
            framed = b'PUNI' + struct.pack('<I', len(modified)) + modified
            (records / 'PUNI.bin').write_bytes(framed)
            manifest['records']['PUNI']['payload_sha256'] = hashlib.sha256(modified).hexdigest()
            manifest['records']['PUNI']['record_sha256'] = hashlib.sha256(framed).hexdigest()
            manifest_path.write_text(json.dumps(manifest))
            output = Path(td) / 't6.h'
            output.write_text('existing header')
            with self.assertRaisesRegex(ValueError, 'verified T6 source record'):
                AVAIL.generate(
                    records, manifest_path, output, profile='t6',
                    expected_rom_sha1='rom-pin', expected_extractor='extractor-pin',
                    expected_extractor_commit='commit-pin',
                    expected_chk_sha256='chk-pin',
                    expected_puni_sha256=hashlib.sha256(payload).hexdigest())
            self.assertEqual(output.read_text(), 'existing header')
            manifest['source_map_path'] = AVAIL.EXPECTED_MAP
            manifest_path.write_text(json.dumps(manifest))
            with self.assertRaisesRegex(ValueError, 'T6'):
                AVAIL.generate(records, manifest_path, output, profile='t6')

    def test_availability_profile_names_are_bounded(self):
        with self.assertRaisesRegex(ValueError, 'unsupported availability profile'):
            AVAIL.generate(profile='t11')
        with self.assertRaisesRegex(ValueError, 'unsupported availability profile'):
            AVAIL.render_header(bytes(AVAIL.PUNI_BYTES), profile='t11')

    def test_packer_retains_puni_only_when_requested(self):
        records = dict(PACKER.RETAINED_SOURCE_RECORDS)
        with tempfile.TemporaryDirectory(prefix='sc64-puni-retain-') as td:
            root = Path(td)
            tags = list(records.items()) + [(b'PUNI', 'PUNI.bin')]
            chunks = []
            payloads = {}
            for tag, filename in tags:
                payload = bytes([len(tag) & 255]) * (5700 if tag == b'PUNI' else 1)
                payloads[tag] = payload
                chunks.append(tag + struct.pack('<I', len(payload)) + payload)
            chk = b''.join(chunks)
            legacy_dir = root / 'legacy' / 'records'
            legacy = PACKER.retain_source_records(chk, legacy_dir, map_path='008/00D')
            legacy_manifest = json.loads(Path(legacy['manifest']).read_text())
            self.assertNotIn('PUNI', legacy_manifest['records'])
            self.assertEqual(len(legacy_manifest['records']), 13)
            t5_dir = root / 't5' / 'records'
            t5 = PACKER.retain_source_records(
                chk, t5_dir, map_path='008/00E', retain_puni=True)
            t5_manifest = json.loads(Path(t5['manifest']).read_text())
            self.assertEqual(set(t5_manifest['records']),
                             {tag.decode('ascii').strip() for tag, _ in tags})
            self.assertEqual((t5_dir / 'PUNI.bin').read_bytes(),
                             b'PUNI' + struct.pack('<I', 5700) + payloads[b'PUNI'])
            self.assertEqual(t5_manifest['records']['PUNI']['payload_bytes'], 5700)
            missing = b''.join(chunk for chunk in chunks if not chunk.startswith(b'PUNI'))
            with self.assertRaisesRegex(ValueError, 'PUNI section'):
                PACKER.retain_source_records(
                    missing, root / 'missing' / 'records', map_path='008/00E',
                    retain_puni=True)
            with self.assertRaisesRegex(ValueError, '5700'):
                malformed = b''.join(
                    tag + struct.pack('<I', len(payloads[tag]) - (1 if tag == b'PUNI' else 0)) +
                    (payloads[tag][:-1] if tag == b'PUNI' else payloads[tag])
                    for tag, _ in tags)
                PACKER.retain_source_records(
                    malformed, root / 'bad' / 'records', map_path='008/00E',
                    retain_puni=True)


if __name__ == '__main__':
    unittest.main()
