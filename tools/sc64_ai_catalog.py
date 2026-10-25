#!/usr/bin/env python3
"""Privately index the authenticated StarCraft 64 AI script catalog.

This audits only the AIScript-compatible directory/table envelope. It does
not decode programs or establish N64 execution semantics. Output is metadata
only and is restricted to this checkout's ignored .local directory.

Copyright (C) 2026 blizzard-gba contributors
SPDX-License-Identifier: GPL-3.0-or-later
"""
from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import struct
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_ROM = ROOT / '.local/sc64/rom/StarCraft64-USA.z64'
DEFAULT_OUT = ROOT / '.local/sc64/ai-catalog/ai-catalog-index.json'
ROM_SHA1 = 'a36d91b5aac7bb95df0d7de5f9814f5270e25c2b'
ROM_SIZE = 32 * 1024 * 1024
BOLT_PATH = '012/000'
BOLT_SHA256 = '96fd2408033e3c8ba8900fc719b1e90f9e828b932d382f51fe5b16cdff1af054'
EXTRACTOR_COMMIT = '09b57f787a247107d1de82b8c5eab2734667ea1d'
ROW_BYTES = 16


def load_module(path: Path, name: str):
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f'cannot load local helper {path}')
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


def verify_extractor(importer) -> None:
    """Require the exact pinned extractor checkout and source file."""
    if importer.UPSTREAM_COMMIT != EXTRACTOR_COMMIT:
        raise ValueError('sc64_import.py does not name the expected extractor commit')
    upstream = importer.UPSTREAM
    try:
        head = subprocess.run(['git', '-C', str(upstream.parent), 'rev-parse', 'HEAD'],
                              check=True, capture_output=True, text=True).stdout.strip()
        committed = subprocess.run(
            ['git', '-C', str(upstream.parent), 'show',
             f'{EXTRACTOR_COMMIT}:{upstream.name}'],
            check=True, capture_output=True).stdout
    except (OSError, subprocess.CalledProcessError) as exc:
        raise ValueError('cannot verify pinned BOLT extractor checkout') from exc
    if head != EXTRACTOR_COMMIT or upstream.read_bytes() != committed:
        raise ValueError('BOLT extractor is not the clean pinned version')


def parse_catalog(data: bytes) -> dict:
    """Parse the bounded 4-byte-offset / terminated 16-byte row table.

    The row layout is the AIScript.bin-compatible candidate: 4-byte ID,
    code offset, label/string ID, and flags. Zero code offsets and nonzero
    offsets are reported as pointer facts, without interpreting target code.
    """
    if len(data) < 4:
        raise ValueError('AI catalog leaf is shorter than its header')
    (table_offset,) = struct.unpack_from('<I', data)
    if table_offset < 4 or table_offset > len(data) - 4:
        raise ValueError('AI catalog table offset is invalid or truncated')

    entries = []
    seen: set[bytes] = set()
    cursor = table_offset
    terminator_offset = None
    while cursor <= len(data) - 4:
        script_id = data[cursor:cursor + 4]
        if script_id == b'\0\0\0\0':
            terminator_offset = cursor
            break
        if cursor > len(data) - ROW_BYTES:
            raise ValueError('AI catalog ends in a truncated nonterminal row')
        _script_id, code_offset, label_id, flags = struct.unpack_from('<4sIII', data, cursor)
        if not all(0x20 <= byte <= 0x7e for byte in script_id):
            raise ValueError(f'non-printable AI script ID at table offset {cursor}')
        if script_id in seen:
            raise ValueError(f'duplicate AI script ID at table offset {cursor}')
        seen.add(script_id)
        if code_offset and code_offset < 4:
            raise ValueError(f'AI script code offset overlaps the header for {script_id!r}')
        if code_offset and code_offset >= len(data):
            raise ValueError(f'AI script code offset is outside this leaf for {script_id!r}')
        entries.append({
            'id': script_id.decode('ascii'),
            'code_offset': code_offset,
            'has_nonzero_in_range_code_offset': code_offset != 0,
            'label_id': label_id,
            'flags': flags,
        })
        cursor += ROW_BYTES
    if terminator_offset is None:
        raise ValueError('AI catalog table has no null-ID terminator')
    table_end = terminator_offset + 4
    if any(entry['has_nonzero_in_range_code_offset'] and
           table_offset <= entry['code_offset'] < table_end
           for entry in entries):
        raise ValueError('AI program offset overlaps the catalog header/table')
    return {
        'header_table_offset': table_offset,
        'row_bytes': ROW_BYTES,
        'entry_count': len(entries),
        'terminator_offset': terminator_offset,
        'table_end': table_end,
        'entries': entries,
        'zero_code_offset_semantics': 'unresolved; zero code offset has no local pointer',
        'execution_semantics': 'not audited',
    }


def read_bounded_rom(path: Path) -> bytes:
    with path.open('rb') as stream:
        size = os.fstat(stream.fileno()).st_size
        if size != ROM_SIZE:
            raise ValueError(f'expected a 32 MiB USA ROM image, got {size} bytes')
        raw = stream.read(ROM_SIZE + 1)
    if len(raw) != ROM_SIZE:
        raise ValueError(f'ROM changed or exceeded the {ROM_SIZE}-byte read bound')
    return raw


def normalize_rom(raw_rom: bytes, extractor) -> bytes:
    """Normalize byte order in memory using the pinned extractor's constants."""
    if len(raw_rom) != ROM_SIZE:
        raise ValueError(f'expected a {ROM_SIZE}-byte ROM image')
    magic = raw_rom[:4]
    if magic == extractor.Z64_MAGIC:
        return raw_rom
    if magic == extractor.V64_MAGIC:
        buf = bytearray(raw_rom)
        buf[0::2], buf[1::2] = raw_rom[1::2], raw_rom[0::2]
        return bytes(buf)
    if magic == extractor.N64_MAGIC:
        buf = bytearray(ROM_SIZE)
        buf[0::4], buf[1::4], buf[2::4], buf[3::4] = (
            raw_rom[3::4], raw_rom[2::4], raw_rom[1::4], raw_rom[0::4],
        )
        return bytes(buf)
    raise ValueError('ROM header is not a supported N64 byte order')


def verify_normalized_rom(normalized: bytes) -> str:
    digest = hashlib.sha1(normalized).hexdigest()
    if digest != ROM_SHA1:
        raise ValueError(f'ROM is not the verified USA image ({digest})')
    return digest


def audit(rom_path: Path) -> dict:
    if not rom_path.is_file():
        raise ValueError(f'ROM does not exist: {rom_path}')
    raw_rom = read_bounded_rom(rom_path)
    importer = load_module(ROOT / 'tools/sc64_import.py', 'sc64_ai_catalog_import')
    verify_extractor(importer)
    extractor = importer.upstream_module(importer.UPSTREAM)
    normalized = normalize_rom(raw_rom, extractor)
    normalized_sha1 = verify_normalized_rom(normalized)
    archive = extractor.BoltArchive(normalized)
    matches = [entry for entry in archive.entries() if entry.path == BOLT_PATH]
    if len(matches) != 1:
        raise ValueError(f'expected exactly one BOLT leaf {BOLT_PATH}, found {len(matches)}')
    payload = archive.read(matches[0])
    leaf_hash = hashlib.sha256(payload).hexdigest()
    if leaf_hash != BOLT_SHA256:
        raise ValueError(f'authenticated BOLT leaf hash mismatch: {leaf_hash}')
    catalog = parse_catalog(payload)
    return {
        'schema': 1,
        'source': {
            'normalized_rom_sha1': normalized_sha1,
            'raw_rom_sha256': hashlib.sha256(raw_rom).hexdigest(),
            'extractor': 'references/sc64-maps/extract_sc64_maps.py',
            'extractor_commit': EXTRACTOR_COMMIT,
            'bolt_path': BOLT_PATH,
            'bolt_payload_bytes': len(payload),
            'bolt_payload_sha256': leaf_hash,
        },
        'format_assessment': {
            'status': 'AIScript.bin-compatible table envelope; N64 equivalence unproven',
            'basis': 'bounded little-endian table header and 16-byte terminated rows',
            'zero_offset': 'recorded without assuming external-script resolution',
            'program_bytes_included': False,
            'n64_timing_or_execution_claim': False,
        },
        'catalog': catalog,
    }


def private_output_path(path: Path) -> Path:
    checkout_root = ROOT.resolve()
    local_path = ROOT / '.local'
    if local_path.is_symlink():
        raise ValueError('.local must be a real directory, not a symlink')
    local_root = local_path.resolve()
    if local_root == checkout_root or checkout_root not in local_root.parents:
        raise ValueError('.local resolves outside this checkout')
    path = path.resolve()
    if path == local_root or local_root not in path.parents:
        raise ValueError('catalog metadata output must stay below ignored .local/')
    return path


def run(rom_path: Path, output: Path) -> dict:
    output = private_output_path(output)
    rom_resolved = rom_path.resolve()
    if output == rom_resolved or (output.exists() and rom_path.exists() and
                                  os.path.samefile(output, rom_path)):
        raise ValueError('catalog output must not overwrite or alias the input ROM')
    result = audit(rom_path)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2, sort_keys=True) + '\n', encoding='utf-8')
    return result


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rom', type=Path, default=DEFAULT_ROM,
                        help='user-supplied verified USA ROM')
    parser.add_argument('--out', type=Path, default=DEFAULT_OUT,
                        help='private metadata JSON path under this checkout .local/')
    args = parser.parse_args()
    try:
        result = run(args.rom, args.out)
    except (OSError, RuntimeError, ValueError) as exc:
        parser.error(str(exc))
    print(f"indexed {result['catalog']['entry_count']} script rows; private metadata written")


if __name__ == '__main__':
    main()
