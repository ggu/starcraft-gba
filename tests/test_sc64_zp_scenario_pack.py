"""Asset-free authentication and numeric-preservation tests for Z/P scenarios."""
from __future__ import annotations

import hashlib
import contextlib
import importlib.util
import io
import json
from pathlib import Path
import struct
import sys
import tempfile
import types
import unittest
from unittest import mock

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    'sc64_zp_scenario_pack_test', ROOT / 'tools/sc64_zp_scenario_pack.py')
assert SPEC is not None and SPEC.loader is not None
PACK = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = PACK
SPEC.loader.exec_module(PACK)


def frame(tag: bytes, payload: bytes) -> bytes:
    return tag + struct.pack('<I', len(payload)) + payload


def trigger_fixture() -> bytes:
    data = bytearray(2 * PACK.TRIGGER_BYTES)
    # One valid condition; the numeric row must survive exactly.
    struct.pack_into('<IIIHBBBBH', data, 0, 65, 13, 999, 37, 2, 13, 1, 3, 0x1234)
    # PreserveTrigger and two repeated Create Unit with Properties rows.
    for index, opcode in enumerate((3, 11, 11)):
        start = 320 + index * 32
        struct.pack_into('<IIIIIIHBBB3s', data, start,
                         65 if index else 0, 0x1100 + index, 0x2200 + index,
                         0x3300 + index, 13, 4, 37, opcode,
                         4 if opcode == 11 else 0, 7, b'\x01\x02\x03')
    data[2372 + 13] = 1
    return bytes(data)


def staged_fixture(source_root: Path) -> tuple[Path, bytes]:
    source = source_root / 'z5'
    source.mkdir(parents=True)
    sections = {
        b'DIM ': struct.pack('<HH', 128, 128),
        b'MRGN': b''.join(struct.pack('<IIIIHH', i, i + 1, i + 2, i + 3,
                                      i + 4, i + 5) for i in range(255)),
        b'OWNR': bytes(range(12)),
        b'SIDE': bytes(range(12, 24)),
        b'FORC': bytes(range(20)),
        b'UPRP': bytes((i * 7) & 0xff for i in range(64 * 20)),
        b'UPUS': bytes(i % 2 for i in range(64)),
        b'PUNI': bytes((i * 3) & 0xff for i in range(5700)),
        b'TRIG': trigger_fixture(),
        b'STR ': b'PRIVATE SOURCE TEXT MUST NOT APPEAR',
        b'WAV ': b'PRIVATE WAVE BYTES MUST NOT APPEAR',
    }
    chk = b''.join(frame(tag, payload) for tag, payload in sections.items())
    section_manifest = []
    for tag, payload in sections.items():
        framed = frame(tag, payload)
        section_manifest.append({'tag': tag.decode('ascii'),
                                 'record_bytes': len(framed),
                                 'payload_bytes': len(payload),
                                 'record_sha256': hashlib.sha256(framed).hexdigest()})
    digest = hashlib.sha256(chk).hexdigest()
    manifest = {
        'schema': 1, 'mission_id': 'Z5', 'source_map_path': '008/018',
        'source_rom_sha1': PACK.ROM_SHA1, 'source_chk_bytes': len(chk),
        'source_chk_sha256': digest, 'source_chk_file': 'source.chk',
        'extractor_commit': PACK.EXTRACTOR_COMMIT, 'sections': section_manifest,
    }
    (source / 'source.chk').write_bytes(chk)
    (source / 'manifest.json').write_text(json.dumps(manifest), encoding='utf-8')
    records = source / 'records'
    records.mkdir()
    for tag in (b'DIM ', b'MRGN', b'TRIG'):
        name = tag.decode('ascii').rstrip().replace(' ', '') + '.bin'
        (records / name).write_bytes(frame(tag, sections[tag]))
    return source, chk


class ZpScenarioPackTests(unittest.TestCase):
    def _catalog_stub(self, digest: str):
        return types.SimpleNamespace(
            MISSIONS=[('Z5', '008/018', 'synthetic')],
            global_id_for_map=lambda index: 16,
            RACE_EXTRA_CHK_SHA256={'008/018': digest})

    def test_preserves_all_numeric_rows_and_never_emits_text_or_wav(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            source, chk = staged_fixture(root)
            digest = hashlib.sha256(chk).hexdigest()
            with mock.patch.object(PACK, '_load_catalog',
                                   return_value=self._catalog_stub(digest)):
                parsed = PACK.parse_source(root, 'Z5')
                header = PACK.emit_header(parsed)
                verified = PACK.verify_staged(root, 'Z5')
            self.assertEqual(parsed['global_catalog_id'], 16)
            self.assertEqual((parsed['width'], parsed['height']), (128, 128))
            self.assertEqual(parsed['source_map_path'], '008/018')
            self.assertEqual(len(parsed['triggers']), 2)
            self.assertEqual(len(parsed['actions']), 3)
            self.assertEqual([row['opcode'] for row in parsed['actions']], [3, 11, 11])
            self.assertEqual(parsed['actions'][1]['modifier'], 4)
            self.assertEqual(parsed['actions'][1]['group1'], 13)
            self.assertEqual(parsed['actions'][1]['group2'], 4)
            self.assertEqual(parsed['actions'][1]['trailing'], [1, 2, 3])
            self.assertEqual(parsed['conditions'][0]['mask'], 0x1234)
            self.assertEqual(parsed['locations'][64]['index'], 65)
            self.assertEqual(len(parsed['properties']), 64)
            self.assertEqual(len(parsed['property_used']), 64)
            self.assertEqual(len(parsed['puni']), 5700)
            self.assertEqual(verified['preserve_trigger_actions'], 1)
            self.assertEqual(verified['create_unit_actions'], 2)
            self.assertIn('sc64_z5_source_actions[3]', header)
            self.assertIn('sc64_z5_source_triggers[2]', header)
            self.assertIn('sc64_z5_source_locations[255]', header)
            self.assertIn('sc64_z5_source_puni[5700]', header)
            self.assertNotIn('PRIVATE SOURCE TEXT', header)
            self.assertNotIn('PRIVATE WAVE BYTES', header)

    def test_pinned_ids_and_global_catalog_paths(self):
        self.assertEqual(PACK.MISSIONS['Z5'], (16, '008/018'))
        self.assertEqual(PACK.MISSIONS['Z10'], (21, '008/01D'))
        self.assertEqual(PACK.MISSIONS['P5'], (24, '008/022'))
        self.assertEqual(PACK.MISSIONS['P10'], (29, '008/027'))

    def test_authentication_and_framed_record_tamper_fail_closed(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            source, chk = staged_fixture(root)
            digest = hashlib.sha256(chk).hexdigest()
            with mock.patch.object(PACK, '_load_catalog',
                                   return_value=self._catalog_stub(digest)):
                record = source / 'records/TRIG.bin'
                record.write_bytes(record.read_bytes()[:-1] + b'X')
                with self.assertRaisesRegex(ValueError, 'framed record differs'):
                    PACK.parse_source(root, 'Z5')
                record.write_bytes(record.read_bytes()[:-1] + b'\x03')
                (source / 'source.chk').write_bytes(chk[:-1] + b'X')
                with self.assertRaisesRegex(ValueError, 'hash differs'):
                    PACK.parse_source(root, 'Z5')

    def test_manifest_provenance_and_section_hashes_are_enforced(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            source, chk = staged_fixture(root)
            digest = hashlib.sha256(chk).hexdigest()
            catalog = self._catalog_stub(digest)
            manifest_path = source / 'manifest.json'
            manifest = json.loads(manifest_path.read_text())
            manifest['extractor_commit'] = '0' * 40
            manifest_path.write_text(json.dumps(manifest))
            with mock.patch.object(PACK, '_load_catalog', return_value=catalog):
                with self.assertRaisesRegex(ValueError, 'extractor_commit'):
                    PACK.parse_source(root, 'Z5')
            manifest['extractor_commit'] = PACK.EXTRACTOR_COMMIT
            manifest['sections'][0]['record_sha256'] = 'f' * 64
            manifest_path.write_text(json.dumps(manifest))
            with mock.patch.object(PACK, '_load_catalog', return_value=catalog):
                with self.assertRaisesRegex(ValueError, 'manifest section'):
                    PACK.parse_source(root, 'Z5')

    def test_section_lengths_and_opcode_bounds_are_checked(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            source, chk = staged_fixture(root)
            digest = hashlib.sha256(chk).hexdigest()
            catalog = self._catalog_stub(digest)
            with mock.patch.object(PACK, '_load_catalog', return_value=catalog):
                parsed = PACK.parse_source(root, 'Z5')
                self.assertEqual(len(parsed['puni']), 5700)

            # Corrupt an action opcode, while keeping all section framing and
            # manifest hashes self-consistent, so opcode validation is tested.
            sections = PACK.parse_chk_sections(chk)
            trig = bytearray(sections[b'TRIG'])
            trig[320 + 26] = PACK.ACTION_OPCODE_LIMIT
            sections[b'TRIG'] = bytes(trig)
            changed = b''.join(frame(tag, payload) for tag, payload in sections.items())
            (source / 'source.chk').write_bytes(changed)
            manifest = json.loads((source / 'manifest.json').read_text())
            manifest['source_chk_bytes'] = len(changed)
            manifest['source_chk_sha256'] = hashlib.sha256(changed).hexdigest()
            for row in manifest['sections']:
                tag = row['tag'].encode('ascii')
                payload = sections[tag]
                framed = frame(tag, payload)
                row['payload_bytes'] = len(payload)
                row['record_bytes'] = len(framed)
                row['record_sha256'] = hashlib.sha256(framed).hexdigest()
            (source / 'manifest.json').write_text(json.dumps(manifest))
            # Optional record copies are also kept consistent with changed CHK.
            record = source / 'records/TRIG.bin'
            record.write_bytes(frame(b'TRIG', sections[b'TRIG']))
            catalog = self._catalog_stub(hashlib.sha256(changed).hexdigest())
            with mock.patch.object(PACK, '_load_catalog', return_value=catalog):
                with self.assertRaisesRegex(ValueError, 'opcode is outside'):
                    PACK.parse_source(root, 'Z5')

    def test_puni_and_uprp_lengths_are_bounded(self):
        for bad_tag in (b'PUNI', b'UPRP'):
            with self.subTest(section=bad_tag.decode('ascii')):
                with tempfile.TemporaryDirectory() as tmp:
                    root = Path(tmp)
                    source, chk = staged_fixture(root)
                    sections = PACK.parse_chk_sections(chk)
                    sections[bad_tag] = sections[bad_tag][:-1]
                    changed = b''.join(frame(tag, payload)
                                       for tag, payload in sections.items())
                    (source / 'source.chk').write_bytes(changed)
                    manifest = json.loads((source / 'manifest.json').read_text())
                    manifest['source_chk_bytes'] = len(changed)
                    manifest['source_chk_sha256'] = hashlib.sha256(changed).hexdigest()
                    for row in manifest['sections']:
                        tag = row['tag'].encode('ascii')
                        framed = frame(tag, sections[tag])
                        row['payload_bytes'] = len(sections[tag])
                        row['record_bytes'] = len(framed)
                        row['record_sha256'] = hashlib.sha256(framed).hexdigest()
                    (source / 'manifest.json').write_text(json.dumps(manifest))
                    catalog = self._catalog_stub(hashlib.sha256(changed).hexdigest())
                    with mock.patch.object(PACK, '_load_catalog', return_value=catalog):
                        label = bad_tag.decode('ascii')
                        with self.assertRaisesRegex(ValueError, label):
                            PACK.parse_source(root, 'Z5')

    def test_chk_duplicate_and_truncated_section_headers_reject(self):
        with tempfile.TemporaryDirectory() as tmp:
            source, _ = staged_fixture(Path(tmp))
            chk = (source / 'source.chk').read_bytes()
            with self.assertRaisesRegex(ValueError, 'duplicate section'):
                PACK.parse_chk_sections(chk + frame(b'DIM ', struct.pack('<HH', 128, 128)))
            with self.assertRaisesRegex(ValueError, 'truncated section header'):
                PACK.parse_chk_sections(chk + b'TR')

    def test_cli_rejects_verify_output_conflict(self):
        with mock.patch.object(sys, 'argv', ['sc64_zp_scenario_pack.py',
                                             '--mission', 'Z5', '--verify-staged',
                                             '--output', 'unused.h']):
            with self.assertRaises(SystemExit) as exc, contextlib.redirect_stderr(io.StringIO()):
                PACK.main()
            self.assertEqual(exc.exception.code, 2)


if __name__ == '__main__':
    unittest.main()
