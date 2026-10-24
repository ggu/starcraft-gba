"""Asset-free tests for the opt-in Z4 private source importer."""
from __future__ import annotations

import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import sys
import tempfile
import unittest
from types import SimpleNamespace
from contextlib import redirect_stderr, redirect_stdout
from io import StringIO
from unittest import mock


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    'sc64_z4_pack_test', ROOT / 'tools/sc64_z4_pack.py')
assert SPEC is not None and SPEC.loader is not None
z4 = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = z4
SPEC.loader.exec_module(z4)


def framed(tag: bytes, payload: bytes) -> bytes:
    return tag + len(payload).to_bytes(4, 'little') + payload


def synthetic_source_chk() -> tuple[bytes, dict[bytes, bytes]]:
    sections = {}
    for tag, size in z4.EXPECTED_RECORD_COUNTS.items():
        sections[tag] = bytes(size)
    # Add the variable-sized source string table and an unrelated section to
    # confirm that the source.chk verifier preserves every CHK record.
    sections[b'STR '] = b'Z4\0'
    sections[b'JUNK'] = b'opaque source record'
    return b''.join(framed(tag, payload) for tag, payload in sections.items()), sections


def create_staged_fixture(out: Path, chk_hash: str) -> tuple[bytes, dict[bytes, bytes]]:
    chk, sections = synthetic_source_chk()
    records = out / 'records'
    records.mkdir()
    record_manifest = {}
    for tag, filename in z4.RETAINED_SECTIONS.items():
        payload = sections[tag]
        record = framed(tag, payload)
        (records / filename).write_bytes(record)
        record_manifest[tag.decode('ascii').strip()] = {
            'file': filename,
            'record_bytes': len(record),
            'payload_bytes': len(payload),
            'record_sha256': hashlib.sha256(record).hexdigest(),
            'payload_sha256': hashlib.sha256(payload).hexdigest(),
        }
    (out / 'source.chk').write_bytes(chk)
    manifest = {
        'schema': 1,
        'mission': {'id': z4.MISSION_ID,
                    'global_catalog_index': z4.GLOBAL_CATALOG_INDEX,
                    'source_map_path': z4.BOLT_PATH},
        'source_rom_sha1': z4.ROM_SHA1,
        'source_chk_sha256': chk_hash,
        'extractor_commit': z4.EXTRACTOR_COMMIT,
        'source_chk_file': 'source.chk',
        'source_chk_bytes': len(chk),
        'records': record_manifest,
    }
    (out / 'manifest.json').write_text(json.dumps(manifest), encoding='ascii')
    return chk, sections


class Z4SourceImportTests(unittest.TestCase):
    def test_source_identity_is_explicit_and_not_playable(self):
        self.assertEqual(z4.BOLT_PATH, '008/017')
        self.assertEqual(z4.GLOBAL_CATALOG_INDEX, 15)
        self.assertEqual(z4.MISSION_ID, 'Z4')
        self.assertEqual(z4.EXPECTED_DIMENSIONS, (96, 96))
        self.assertEqual(z4.EXPECTED_TILESET_ID, 3)
        self.assertEqual(sum(z4.EXPECTED_SOURCE_TYPES.values()), 184)
        self.assertEqual(z4.EXPECTED_SOURCE_TYPES[214], 3)
        self.assertEqual(z4.source_pool_demand(z4.EXPECTED_SOURCE_TYPES),
                         {'units': 74, 'buildings': 66, 'minerals': 39, 'geysers': 2})

    def test_complete_source_histogram_passes(self):
        rows = []
        for raw_type, count in z4.EXPECTED_SOURCE_TYPES.items():
            rows.extend({'source_type': raw_type} for _ in range(count))
        self.assertEqual(z4.source_type_counts(rows), z4.EXPECTED_SOURCE_TYPES)

    def test_missing_or_unexpected_source_rows_fail_closed(self):
        rows = []
        for raw_type, count in z4.EXPECTED_SOURCE_TYPES.items():
            rows.extend({'source_type': raw_type} for _ in range(count))
        with self.assertRaisesRegex(ValueError, 'histogram'):
            z4.source_type_counts(rows[:-1])
        rows.append({'source_type': 65535})
        with self.assertRaisesRegex(ValueError, 'histogram'):
            z4.source_type_counts(rows)

    def test_invalid_source_type_fails_closed(self):
        with self.assertRaisesRegex(ValueError, 'invalid source type'):
            z4.source_type_counts([{'source_type': -1}])

    def test_chk_parser_rejects_duplicate_and_truncated_sections(self):
        with self.assertRaisesRegex(ValueError, 'duplicate'):
            z4.parse_chk_sections(framed(b'TEST', b'a') + framed(b'TEST', b'b'))
        with self.assertRaisesRegex(ValueError, 'truncated'):
            z4.parse_chk_sections(b'bad')
        with self.assertRaisesRegex(ValueError, 'bounds'):
            z4.parse_chk_sections(b'TEST' + struct.pack('<I', 4) + b'x')

    def test_audit_rejects_wrong_map_or_chk_before_import(self):
        info = SimpleNamespace(bolt_path='008/017', width=96, height=96,
                               tileset=3, name='Z4', tileset_name='Ashworld')
        with self.assertRaisesRegex(ValueError, 'BOLT'):
            z4.audit_source(SimpleNamespace(**{**vars(info), 'bolt_path': '008/016'}),
                            b'x', None, None)
        with self.assertRaisesRegex(ValueError, 'dimensions'):
            z4.audit_source(SimpleNamespace(**{**vars(info), 'width': 128}),
                            b'x', None, None)
        with self.assertRaisesRegex(ValueError, 'CHK'):
            z4.audit_source(info, b'x', None, None)

    def test_wrong_rom_fails_before_map_extraction(self):
        fake_extractor = SimpleNamespace(load_rom=lambda _path: b'wrong ROM')
        fake_importer = SimpleNamespace(
            UPSTREAM_COMMIT=z4.EXTRACTOR_COMMIT,
            UPSTREAM=ROOT / 'references/sc64-maps/extract_sc64_maps.py',
            upstream_module=lambda _path: fake_extractor)
        with mock.patch.object(z4, '_load_module', return_value=fake_importer), \
             mock.patch.object(z4, '_verify_extractor_checkout') as verify_checkout:
            with tempfile.TemporaryDirectory() as tmp:
                with self.assertRaisesRegex(ValueError, 'verified StarCraft 64 USA image'):
                    z4.pack(Path('unused-rom'), Path(tmp) / 'out')
            verify_checkout.assert_called_once_with(fake_importer)

    def test_wrong_extractor_revision_fails_closed(self):
        fake_importer = SimpleNamespace(
            UPSTREAM_COMMIT='wrong-revision',
            UPSTREAM=ROOT / 'references/sc64-maps/extract_sc64_maps.py')
        with self.assertRaisesRegex(ValueError, 'expected pinned extractor'):
            z4._verify_extractor_checkout(fake_importer)

    def test_dirty_extractor_source_fails_pinned_provenance(self):
        with tempfile.TemporaryDirectory() as tmp:
            repo = Path(tmp)
            source = repo / 'extract_sc64_maps.py'
            source.write_bytes(b'modified working tree source')
            fake_importer = SimpleNamespace(
                UPSTREAM_COMMIT=z4.EXTRACTOR_COMMIT, UPSTREAM=source)
            outputs = [
                SimpleNamespace(stdout=z4.EXTRACTOR_COMMIT + '\n'),
                SimpleNamespace(stdout=b'pinned committed source'),
            ]
            with mock.patch.object(z4.subprocess, 'run',
                                   side_effect=outputs) as run:
                with self.assertRaisesRegex(ValueError, 'differs from the pinned'):
                    z4._verify_extractor_checkout(fake_importer)
            self.assertEqual(run.call_count, 2)
            self.assertIn(f'{z4.EXTRACTOR_COMMIT}:extract_sc64_maps.py',
                          run.call_args_list[1].args[0])

    def test_staged_record_manifest_checks_all_sections_and_opaque_chk_data(self):
        chk, sections = synthetic_source_chk()
        chk_hash = hashlib.sha256(chk).hexdigest()
        with mock.patch.object(z4, 'CHK_SHA256', chk_hash):
            with tempfile.TemporaryDirectory() as tmp:
                out = Path(tmp)
                create_staged_fixture(out, chk_hash)
                self.assertEqual(z4.verify_staged(out)['sections_verified'],
                                 len(z4.RETAINED_SECTIONS))
                records = out / 'records'
                with (records / 'UNIT.bin').open('ab') as stream:
                    stream.write(b'corruption')
                with self.assertRaisesRegex(ValueError, 'differs from'):
                    z4.verify_staged(out)

    def test_staged_manifest_and_chk_tampering_fail_closed(self):
        chk, _sections = synthetic_source_chk()
        chk_hash = hashlib.sha256(chk).hexdigest()
        with mock.patch.object(z4, 'CHK_SHA256', chk_hash):
            with tempfile.TemporaryDirectory() as tmp:
                out = Path(tmp)
                create_staged_fixture(out, chk_hash)
                manifest_path = out / 'manifest.json'
                manifest = json.loads(manifest_path.read_text(encoding='ascii'))
                manifest['source_chk_file'] = '../escape.chk'
                manifest_path.write_text(json.dumps(manifest), encoding='ascii')
                with self.assertRaisesRegex(ValueError, 'unexpected source CHK file'):
                    z4.verify_staged(out)

            with tempfile.TemporaryDirectory() as tmp:
                out = Path(tmp)
                create_staged_fixture(out, chk_hash)
                source_path = out / 'source.chk'
                source_path.write_bytes(source_path.read_bytes() + b'changed')
                with self.assertRaisesRegex(ValueError, 'CHK hash'):
                    z4.verify_staged(out)

            with tempfile.TemporaryDirectory() as tmp:
                out = Path(tmp)
                create_staged_fixture(out, chk_hash)
                manifest_path = out / 'manifest.json'
                manifest = json.loads(manifest_path.read_text(encoding='ascii'))
                manifest['records']['UNIT']['record_sha256'] = '0' * 64
                manifest_path.write_text(json.dumps(manifest), encoding='ascii')
                with self.assertRaisesRegex(ValueError, 'integrity check failed'):
                    z4.verify_staged(out)

    def test_cli_verify_is_rom_free_and_rejects_rom_argument(self):
        chk, _sections = synthetic_source_chk()
        chk_hash = hashlib.sha256(chk).hexdigest()
        with mock.patch.object(z4, 'CHK_SHA256', chk_hash):
            with tempfile.TemporaryDirectory() as tmp:
                out = Path(tmp)
                create_staged_fixture(out, chk_hash)
                stdout = StringIO()
                with mock.patch.object(z4, '_load_module', side_effect=AssertionError), \
                     redirect_stdout(stdout):
                    self.assertEqual(z4.main(['--verify-staged', '--out', str(out)]), 0)
                self.assertIn('verified_sections', stdout.getvalue())
                with mock.patch.object(z4, 'pack') as pack_call, \
                     redirect_stderr(StringIO()):
                    with self.assertRaises(SystemExit) as raised:
                        z4.main(['--verify-staged', '--rom', 'ignored.z64',
                                 '--out', str(out)])
                self.assertEqual(raised.exception.code, 2)
                pack_call.assert_not_called()


if __name__ == '__main__':
    unittest.main()
