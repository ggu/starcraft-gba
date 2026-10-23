#!/usr/bin/env python3
"""Validate retained T5/T6 PUNI bytes and emit private availability tables.

The raw source record is private, ignored input under .local/sc64. Generated
headers contain only effective availability bits and are likewise private.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_RECORDS = ROOT / '.local/sc64/races/generated/t5/records'
DEFAULT_MANIFEST = ROOT / '.local/sc64/races/generated/t5/manifest.json'
DEFAULT_OUTPUT = ROOT / '.local/sc64/races/generated/sc64_t5_availability_generated.h'
EXPECTED_MAP = '008/00E'
EXPECTED_ROM_SHA1 = 'a36d91b5aac7bb95df0d7de5f9814f5270e25c2b'
EXPECTED_EXTRACTOR = 'references/sc64-maps/extract_sc64_maps.py'
EXPECTED_EXTRACTOR_COMMIT = '09b57f787a247107d1de82b8c5eab2734667ea1d'
EXPECTED_CHK_SHA256 = 'ab54b7b0dd9f27f2ca607a6c323c60bf1b6f948134d304dae7357d1ab0012032'
EXPECTED_PUNI_SHA256 = '2c29eeb09e3708644f1789559276bebcd9e01a9fe55adcb4bddb429f52c52183'
PROFILES = {
    't5': {
        'map': EXPECTED_MAP,
        'chk_sha256': EXPECTED_CHK_SHA256,
        'puni_sha256': EXPECTED_PUNI_SHA256,
    },
    't6': {
        'map': '008/00F',
        'chk_sha256': 'f07f21939bb8dbeeb6e3933fc632a04ff13bc1617ac5f8ae224c0ff978623f35',
        'puni_sha256': 'd73527196647e6cd42f169876015c8916226449935ee2b2899a0342829d48f1c',
    },
}
PLAYERS = 12
UNITS = 228
PUNI_BYTES = PLAYERS * UNITS + UNITS + PLAYERS * UNITS


def read_puni(records_dir: Path = DEFAULT_RECORDS,
              manifest_path: Path = DEFAULT_MANIFEST, *,
              expected_map: str = EXPECTED_MAP,
              expected_rom_sha1: str = EXPECTED_ROM_SHA1,
              expected_extractor: str = EXPECTED_EXTRACTOR,
              expected_extractor_commit: str = EXPECTED_EXTRACTOR_COMMIT,
              expected_chk_sha256: str = EXPECTED_CHK_SHA256,
              expected_puni_sha256: str = EXPECTED_PUNI_SHA256) -> tuple[bytes, dict]:
    """Load PUNI only when its map, pinned inputs and record hashes agree."""
    label = next((name.upper() for name, spec in PROFILES.items()
                  if spec['map'] == expected_map), None)
    if label is None:
        raise ValueError('unsupported availability source map')
    try:
        manifest = json.loads(manifest_path.read_text(encoding='utf-8'))
    except (OSError, json.JSONDecodeError) as exc:
        raise ValueError(f'cannot read {label} source manifest: {exc}') from exc
    if manifest.get('schema') != 1 or manifest.get('source_map_path') != expected_map:
        raise ValueError(f'manifest does not describe {label} ({expected_map})')
    if manifest.get('source_rom_sha1') != expected_rom_sha1:
        raise ValueError('manifest ROM is not the verified StarCraft 64 USA image')
    if manifest.get('extractor') != expected_extractor or manifest.get('extractor_commit') != expected_extractor_commit:
        raise ValueError('manifest extractor pin does not match the verified extractor')
    if manifest.get('source_chk_sha256') != expected_chk_sha256:
        raise ValueError(f'manifest CHK hash does not match the verified {label} source')
    records = manifest.get('records')
    metadata = records.get('PUNI') if isinstance(records, dict) else None
    if not isinstance(metadata, dict) or metadata.get('file') != 'PUNI.bin':
        raise ValueError(f'manifest is missing the retained {label} PUNI record')
    if metadata.get('payload_bytes') != PUNI_BYTES or metadata.get('record_bytes') != PUNI_BYTES + 8:
        raise ValueError('manifest PUNI record has an unexpected size')
    record = (records_dir / 'PUNI.bin').read_bytes()
    if len(record) != PUNI_BYTES + 8 or record[:4] != b'PUNI':
        raise ValueError('retained PUNI file is not the exact framed CHK record')
    if int.from_bytes(record[4:8], 'little') != PUNI_BYTES:
        raise ValueError('retained PUNI frame declares an unexpected payload size')
    payload = record[8:]
    digest = hashlib.sha256(payload).hexdigest()
    if len(payload) != PUNI_BYTES or digest != metadata.get('payload_sha256'):
        raise ValueError('retained PUNI payload size/hash disagrees with its manifest')
    if hashlib.sha256(record).hexdigest() != metadata.get('record_sha256'):
        raise ValueError('retained PUNI framed-record hash disagrees with its manifest')
    if digest != expected_puni_sha256:
        raise ValueError(f'retained PUNI payload is not the verified {label} source record')
    return payload, manifest


def effective_bits(puni: bytes) -> bytes:
    """Pack 12x228 effective 0/1 values in player-major, LSB-first order."""
    if len(puni) != PUNI_BYTES:
        raise ValueError(f'PUNI payload must be exactly {PUNI_BYTES} bytes')
    bits = bytearray((PLAYERS * UNITS + 7) // 8)
    global_start = PLAYERS * UNITS
    selector_start = global_start + UNITS
    for player in range(PLAYERS):
        for raw in range(UNITS):
            index = player * UNITS + raw
            player_start = player * UNITS
            value = puni[global_start + raw] if puni[selector_start + index] else puni[player_start + raw]
            if value:
                bits[index >> 3] |= 1 << (index & 7)
    return bytes(bits)


def render_header(puni: bytes, *, profile: str = 't5') -> str:
    if profile not in PROFILES:
        raise ValueError(f'unsupported availability profile: {profile}')
    bits = effective_bits(puni)
    digest = hashlib.sha256(puni).hexdigest()
    rows = []
    for pos in range(0, len(bits), 16):
        rows.append('    ' + ', '.join(f'0x{value:02x}' for value in bits[pos:pos + 16]) + ',')
    label = profile.upper()
    return '\n'.join([
        '/* Private generated source data. Keep under ignored .local storage. */',
        f'#ifndef SC64_{label}_AVAILABILITY_GENERATED_H',
        f'#define SC64_{label}_AVAILABILITY_GENERATED_H',
        f'/* Effective {label} PUNI SHA-256: {digest} */',
        f'#define SC64_{label}_AVAILABILITY_PLAYERS 12u',
        f'#define SC64_{label}_AVAILABILITY_UNITS 228u',
        f'#define SC64_{label}_AVAILABILITY_BIT_BYTES {len(bits)}u',
        f'static const unsigned char sc64_{profile}_availability_bits[SC64_{label}_AVAILABILITY_BIT_BYTES] = {{',
        *rows,
        '};',
        f'#endif /* SC64_{label}_AVAILABILITY_GENERATED_H */',
        '',
    ])


def generate(records_dir: Path | None = None,
             manifest_path: Path | None = None,
             output: Path | None = None, *, profile: str = 't5', **pins) -> dict:
    if profile not in PROFILES:
        raise ValueError(f'unsupported availability profile: {profile}')
    spec = PROFILES[profile]
    base = ROOT / '.local/sc64/races/generated'
    records_dir = records_dir if records_dir is not None else base / profile / 'records'
    manifest_path = manifest_path if manifest_path is not None else base / profile / 'manifest.json'
    output = output if output is not None else base / f'sc64_{profile}_availability_generated.h'
    pins.setdefault('expected_chk_sha256', spec['chk_sha256'])
    pins.setdefault('expected_puni_sha256', spec['puni_sha256'])
    puni, manifest = read_puni(records_dir, manifest_path,
                              expected_map=spec['map'], **pins)
    text = render_header(puni, profile=profile)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(text, encoding='ascii')
    return {
        'map': manifest['source_map_path'],
        'puni_sha256': hashlib.sha256(puni).hexdigest(),
        'puni_bytes': len(puni),
        'effective_bit_bytes': len(effective_bits(puni)),
        'header': str(output),
        'header_sha256': hashlib.sha256(text.encode('ascii')).hexdigest(),
    }


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--profile', choices=tuple(PROFILES), default='t5')
    parser.add_argument('--records-dir', type=Path)
    parser.add_argument('--manifest', type=Path)
    parser.add_argument('--out', type=Path)
    args = parser.parse_args(argv)
    try:
        print(json.dumps(generate(args.records_dir, args.manifest, args.out,
                                  profile=args.profile), indent=2))
    except (OSError, ValueError, KeyError) as exc:
        print(f'sc64_t5_availability: {exc}', file=sys.stderr)
        return 1
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
