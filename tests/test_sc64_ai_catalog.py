"""Asset-free bounds and provenance checks for the private AI catalog index."""
from __future__ import annotations

import importlib.util
from pathlib import Path
import struct
import sys
import tempfile
from types import SimpleNamespace
import unittest
from unittest import mock

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    'sc64_ai_catalog_test', ROOT / 'tools/sc64_ai_catalog.py')
assert SPEC and SPEC.loader
catalog = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = catalog
SPEC.loader.exec_module(catalog)


def row(script_id: bytes, code_offset: int, label: int = 1, flags: int = 0) -> bytes:
    return struct.pack('<4sIII', script_id, code_offset, label, flags)


def sample() -> bytes:
    # Header points to two rows, then a four-byte null ID and small opaque code.
    table = 4
    end = table + 2 * catalog.ROW_BYTES + 4
    return (struct.pack('<I', table) + row(b'Tst1', end, 7, 1) +
            row(b'Tst2', 0, 8, 2) + b'\0\0\0\0' + b'opaque')


def table_after_program() -> bytes:
    code = b'opaque'
    table = 4 + len(code)
    end = table + catalog.ROW_BYTES + 4
    return struct.pack('<I', table) + code + row(b'Tst1', 4, 9, 0) + b'\0' * 4


class Sc64AiCatalogTests(unittest.TestCase):
    def test_parses_rows_and_four_byte_terminator_without_decoding_programs(self):
        parsed = catalog.parse_catalog(sample())
        self.assertEqual(parsed['header_table_offset'], 4)
        self.assertEqual(parsed['entry_count'], 2)
        self.assertEqual(parsed['terminator_offset'], 36)
        self.assertEqual(parsed['table_end'], 40)
        self.assertEqual(parsed['entries'][0], {
            'id': 'Tst1', 'code_offset': 40,
            'has_nonzero_in_range_code_offset': True,
            'label_id': 7, 'flags': 1,
        })
        self.assertFalse(parsed['entries'][1]['has_nonzero_in_range_code_offset'])
        self.assertIn('unresolved', parsed['zero_code_offset_semantics'])
        self.assertEqual(parsed['execution_semantics'], 'not audited')

    def test_program_before_table_and_four_byte_terminator_at_eof(self):
        data = table_after_program()
        parsed = catalog.parse_catalog(data)
        self.assertEqual(parsed['header_table_offset'], 10)
        self.assertEqual(parsed['entries'][0]['code_offset'], 4)
        self.assertEqual(parsed['terminator_offset'], len(data) - 4)
        self.assertEqual(parsed['table_end'], len(data))

    def test_rejects_short_header_and_invalid_table_offsets(self):
        for data in (b'', b'\0\0\0'):
            with self.subTest(data=data):
                with self.assertRaisesRegex(ValueError, 'shorter than its header'):
                    catalog.parse_catalog(data)
        for offset in (0, 2, 100):
            with self.subTest(offset=offset):
                with self.assertRaisesRegex(ValueError, 'table offset'):
                    catalog.parse_catalog(struct.pack('<I', offset) + b'\0' * 16)

    def test_rejects_missing_terminator_and_truncated_row(self):
        missing = struct.pack('<I', 4) + row(b'Tst1', 0)
        with self.assertRaisesRegex(ValueError, 'no null-ID terminator'):
            catalog.parse_catalog(missing)
        truncated = struct.pack('<I', 4) + b'Tst1' + b'\0' * 8
        with self.assertRaisesRegex(ValueError, 'truncated nonterminal row'):
            catalog.parse_catalog(truncated)

    def test_rejects_duplicate_or_nonprintable_ids(self):
        dup = struct.pack('<I', 4) + row(b'Tst1', 40) + row(b'Tst1', 40) + b'\0' * 4
        with self.assertRaisesRegex(ValueError, 'duplicate'):
            catalog.parse_catalog(dup + b'code')
        bad = struct.pack('<I', 4) + row(b'\xffst', 24) + b'\0' * 4
        with self.assertRaisesRegex(ValueError, 'non-printable'):
            catalog.parse_catalog(bad)

    def test_rejects_out_of_bounds_and_table_overlapping_program_offsets(self):
        out_of_bounds = struct.pack('<I', 4) + row(b'Tst1', 100) + b'\0' * 4
        with self.assertRaisesRegex(ValueError, 'outside this leaf'):
            catalog.parse_catalog(out_of_bounds)
        header_overlap = struct.pack('<I', 4) + row(b'Tst1', 2) + b'\0' * 4
        with self.assertRaisesRegex(ValueError, 'overlaps the header'):
            catalog.parse_catalog(header_overlap + b'code')
        overlap = struct.pack('<I', 4) + row(b'Tst1', 8) + b'\0' * 4 + b'code'
        with self.assertRaisesRegex(ValueError, 'overlaps'):
            catalog.parse_catalog(overlap)

    def test_private_output_path_accepts_local_and_rejects_traversal(self):
        with tempfile.TemporaryDirectory(prefix='sc64-ai-root-') as td:
            root = Path(td)
            (root / '.local').mkdir()
            with mock.patch.object(catalog, 'ROOT', root):
                self.assertEqual(catalog.private_output_path(root / '.local' / 'index.json'),
                                 (root / '.local' / 'index.json').resolve())
                with self.assertRaisesRegex(ValueError, 'below ignored .local'):
                    catalog.private_output_path(root / '.local' / '..' / 'index.json')
                with self.assertRaisesRegex(ValueError, 'below ignored .local'):
                    catalog.private_output_path(Path('/tmp/not-private.json'))

    def test_rejects_local_symlink_escape(self):
        with tempfile.TemporaryDirectory(prefix='sc64-ai-root-') as td, \
             tempfile.TemporaryDirectory(prefix='sc64-ai-outside-') as outside:
            root = Path(td)
            tracked = root / 'tracked-output'
            tracked.mkdir()
            for destination in (Path(outside), tracked):
                with self.subTest(destination=destination):
                    (root / '.local').symlink_to(destination, target_is_directory=True)
                    with mock.patch.object(catalog, 'ROOT', root):
                        with self.assertRaisesRegex(ValueError, r'\.local must be a real directory'):
                            catalog.private_output_path(root / '.local' / 'index.json')
                    (root / '.local').unlink()

    def test_rom_normalization_supports_all_three_byte_orders(self):
        extractor = SimpleNamespace(Z64_MAGIC=bytes.fromhex('80371240'),
                                    V64_MAGIC=bytes.fromhex('37804012'),
                                    N64_MAGIC=bytes.fromhex('40123780'))
        with mock.patch.object(catalog, 'ROM_SIZE', 8):
            z64 = bytes.fromhex('80371240a1b2c3d4')
            v64 = bytes.fromhex('37804012b2a1d4c3')
            n64 = bytes.fromhex('40123780d4c3b2a1')
            expected = z64
            self.assertEqual(catalog.normalize_rom(z64, extractor), expected)
            self.assertEqual(catalog.normalize_rom(v64, extractor), expected)
            self.assertEqual(catalog.normalize_rom(n64, extractor), expected)
            with self.assertRaisesRegex(ValueError, 'not a supported N64 byte order'):
                catalog.normalize_rom(b'BAD!1234', extractor)

    def test_run_rejects_samepath_symlink_and_hardlink_to_input_rom(self):
        with tempfile.TemporaryDirectory(prefix='sc64-ai-private-') as td:
            root = Path(td)
            local = root / '.local'
            local.mkdir()
            rom = local / 'input.rom'
            rom.write_bytes(b'not-a-rom')
            symlink = local / 'symlink.rom'
            symlink.symlink_to(rom)
            hardlink = local / 'hardlink.rom'
            hardlink.hardlink_to(rom)
            with mock.patch.object(catalog, 'ROOT', root):
                for output in (rom, symlink, hardlink):
                    with self.subTest(output=output):
                        with self.assertRaisesRegex(ValueError, 'must not overwrite or alias'):
                            catalog.run(rom, output)
            self.assertEqual(rom.read_bytes(), b'not-a-rom')

    def test_bounded_read_and_normalized_rom_provenance_fail_closed(self):
        with tempfile.TemporaryDirectory(prefix='sc64-ai-rom-') as td:
            path = Path(td) / 'oversized.rom'
            path.write_bytes(b'12345')
            with mock.patch.object(catalog, 'ROM_SIZE', 8):
                with self.assertRaisesRegex(ValueError, 'expected a 32 MiB USA ROM image'):
                    catalog.read_bounded_rom(path)
                extractor = SimpleNamespace(Z64_MAGIC=b'Z64!', V64_MAGIC=b'V64!',
                                            N64_MAGIC=b'N64!')
                normalized = catalog.normalize_rom(b'Z64!1234', extractor)
                with mock.patch.object(catalog, 'ROM_SHA1', '0' * 40):
                    with self.assertRaisesRegex(ValueError, 'not the verified USA image'):
                        catalog.verify_normalized_rom(normalized)


if __name__ == '__main__':
    unittest.main()
