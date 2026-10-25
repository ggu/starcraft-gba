"""Asset-free checks for authenticated private P4 source staging."""
from __future__ import annotations

import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import sys
import tempfile
import unittest
from unittest import mock


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    'sc64_p4_pack_test', ROOT / 'tools/sc64_p4_pack.py')
assert SPEC is not None and SPEC.loader is not None
p4 = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = p4
SPEC.loader.exec_module(p4)


def framed(tag: bytes, payload: bytes) -> bytes:
    return tag + len(payload).to_bytes(4, 'little') + payload


def staged_fixture(out: Path) -> tuple[bytes, str]:
    sections = {tag: bytes(size) for tag, size in p4.EXPECTED_SECTION_SIZES.items()}
    sections[b'STR '] = b'private fixture strings\0'
    sections[b'UNIS'] = b'opaque complete source section'
    chk = b''.join(framed(tag, data) for tag, data in sections.items())
    digest = hashlib.sha256(chk).hexdigest()
    (out / 'records').mkdir(parents=True)
    records = {}
    for tag, filename in p4.RETAINED_SECTIONS.items():
        payload = sections[tag]
        data = framed(tag, payload)
        (out / 'records' / filename).write_bytes(data)
        records[tag.decode('ascii').strip()] = {
            'file': filename, 'record_bytes': len(data),
            'payload_bytes': len(payload), 'record_sha256': hashlib.sha256(data).hexdigest(),
            'payload_sha256': hashlib.sha256(payload).hexdigest()}
    (out / 'source.chk').write_bytes(chk)
    manifest = {'schema': 1,
                'mission': {'id': p4.MISSION_ID,
                            'global_catalog_index': p4.GLOBAL_CATALOG_INDEX,
                            'source_map_path': p4.BOLT_PATH},
                'source_rom_sha1': p4.ROM_SHA1, 'source_chk_sha256': digest,
                'extractor_commit': p4.EXTRACTOR_COMMIT,
                'source_chk_file': 'source.chk', 'source_chk_bytes': len(chk),
                'records': records}
    (out / 'manifest.json').write_text(json.dumps(manifest), encoding='ascii')
    return chk, digest


class P4SourceImportTests(unittest.TestCase):
    def test_source_identity_and_full_type_classification(self):
        self.assertEqual((p4.MISSION_ID, p4.GLOBAL_CATALOG_INDEX, p4.BOLT_PATH),
                         ('P4', 23, '008/021'))
        self.assertEqual(p4.CHK_SHA256,
                         '40f30ccedf87a3b9c88308dc3092611ceff10185ddaf32471104902042a93ecd')
        self.assertEqual(sum(p4.EXPECTED_SOURCE_TYPES.values()), 201)
        self.assertEqual(p4.EXPECTED_SOURCE_TYPES[214], 5)
        self.assertEqual(p4.pool_demand({150: 1}),
                         {'units': 0, 'buildings': 1, 'minerals': 0, 'geysers': 0})
        self.assertEqual(p4.pool_demand({raw: 1 for raw in range(106, 176)})['buildings'], 70)
        self.assertEqual(p4.pool_demand({216: 1})['units'], 1)

    def test_histogram_rejects_missing_unexpected_and_invalid_types(self):
        rows = [{'source_type': raw} for raw, count in p4.EXPECTED_SOURCE_TYPES.items()
                for _ in range(count)]
        self.assertEqual(p4.source_type_counts(rows), p4.EXPECTED_SOURCE_TYPES)
        with self.assertRaisesRegex(ValueError, 'histogram'):
            p4.source_type_counts(rows[:-1])
        with self.assertRaisesRegex(ValueError, 'histogram'):
            p4.source_type_counts(rows + [{'source_type': 65535}])
        with self.assertRaisesRegex(ValueError, 'invalid source type'):
            p4.source_type_counts([{'source_type': -1}])

    def test_chk_parser_rejects_duplicate_truncated_and_overflow_records(self):
        with self.assertRaisesRegex(ValueError, 'duplicate'):
            p4.parse_chk_sections(framed(b'TEST', b'a') + framed(b'TEST', b'b'))
        with self.assertRaisesRegex(ValueError, 'truncated'):
            p4.parse_chk_sections(b'bad')
        with self.assertRaisesRegex(ValueError, 'bounds'):
            p4.parse_chk_sections(b'TEST' + struct.pack('<I', 4) + b'x')

    def test_authenticated_staged_records_reject_tampering_and_truncation(self):
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp)
            chk, digest = staged_fixture(out)
            with mock.patch.object(p4, 'CHK_SHA256', digest):
                self.assertEqual(p4.verify_staged(out)['sections_verified'],
                                 len(p4.RETAINED_SECTIONS))
                target = out / 'records' / 'UNIT.bin'
                target.write_bytes(target.read_bytes()[:-1])
                with self.assertRaisesRegex(ValueError, 'differs from authenticated'):
                    p4.verify_staged(out)
                target.write_bytes(target.read_bytes() + b'\0')
                manifest_path = out / 'manifest.json'
                manifest = json.loads(manifest_path.read_text(encoding='ascii'))
                manifest['source_chk_file'] = '../wrong.chk'
                manifest_path.write_text(json.dumps(manifest), encoding='ascii')
                with self.assertRaisesRegex(ValueError, 'unexpected source CHK'):
                    p4.verify_staged(out)
                manifest['source_chk_file'] = 'source.chk'
                manifest_path.write_text(json.dumps(manifest), encoding='ascii')
                (out / 'source.chk').write_bytes(chk[:-1])
                with self.assertRaisesRegex(ValueError, 'CHK hash'):
                    p4.verify_staged(out)


if __name__ == '__main__':
    unittest.main()
