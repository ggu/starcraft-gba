#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Extract authenticated Z1-Z4 permission inputs into a new private directory."""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

from tools.sc64_sprites import load_extractor, _entry_payload
from tools.sc64_t5_availability import effective_bits
from tools.sc64_zp_scenario_pack import parse_chk_sections

ROOT = Path(__file__).resolve().parents[1]
EXTRACTOR_COMMIT = '09b57f787a247107d1de82b8c5eab2734667ea1d'
ROM_SHA256 = 'e7d21880c13953b45b8f8db6b2b6cfa02daa29d35b23799a967e96f21001b020'
MISSIONS = (('Z1', '008/014', 0), ('Z2', '008/015', 1),
            ('Z3', '008/016', 14), ('Z4', '008/017', 15))


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def decode(chk: bytes) -> dict:
    sections = parse_chk_sections(chk)
    for tag, length in ((b'PUNI', 5700), (b'OWNR', 12), (b'SIDE', 12), (b'DIM ', 4)):
        if len(sections.get(tag, b'')) != length:
            raise ValueError(f'invalid {tag!r} section length')
    return {'puni': sections[b'PUNI'], 'effective': effective_bits(sections[b'PUNI']),
            'owners': sections[b'OWNR'], 'races': sections[b'SIDE']}


def render_header(rows: list[dict]) -> str:
    if [row['mission'] for row in rows] != [0, 1, 14, 15]:
        raise ValueError('expected stable Z1-Z4 mission identities')
    if any(len(row['owners']) != 12 or len(row['races']) != 12 or len(row['effective']) != 342 for row in rows):
        raise ValueError('invalid permission row dimensions')
    lines = ['/* Private owned-ROM derivative; do not distribute. */',
             '#ifndef SC64_EARLY_ZERG_PERMISSIONS_GENERATED_H',
             '#define SC64_EARLY_ZERG_PERMISSIONS_GENERATED_H',
             'typedef struct { unsigned char mission, owner_kind[12], source_race[12], bits[342]; } Sc64EarlyZergPermission;',
             'static const Sc64EarlyZergPermission sc64_early_zerg_permissions[4] = {']
    for row in rows:
        lines += ['  { ' + str(row['mission']) + ', {' + ','.join(map(str, row['owners'])) + '},',
                  '    {' + ','.join(map(str, row['races'])) + '}, {']
        for offset in range(0, len(row['effective']), 16):
            lines.append('    ' + ','.join(f'0x{x:02x}' for x in row['effective'][offset:offset+16]) + ',')
        lines.append('  } },')
    lines += ['};', '#endif', '']
    return '\n'.join(lines)


def verify_extractor(path: Path) -> None:
    path = path.resolve()
    if path.name != 'extract_sc64_maps.py':
        raise ValueError('expected pinned extractor filename')
    head = subprocess.check_output(['git', '-C', str(path.parent), 'rev-parse', 'HEAD'], text=True).strip()
    if head != EXTRACTOR_COMMIT:
        raise ValueError('extractor checkout is not pinned')
    expected = subprocess.check_output(['git', '-C', str(path.parent), 'show', f'{EXTRACTOR_COMMIT}:{path.name}'])
    if path.read_bytes() != expected:
        raise ValueError('extractor file differs from pinned Git object')


def run(rom_path: Path, extractor_path: Path, output: Path) -> dict:
    output = output.resolve()
    if not output.is_relative_to((ROOT / '.local').resolve()):
        raise ValueError('output must stay in this checkout\'s ignored .local directory')
    if output.exists():
        raise ValueError('output already exists; choose a new private directory')
    verify_extractor(extractor_path)
    extractor = load_extractor(extractor_path)
    rom = extractor.load_rom(str(rom_path))
    if digest(rom) != ROM_SHA256:
        raise ValueError('ROM is not the verified normalized USA image')
    archive = extractor.BoltArchive(rom)
    rows, artifacts = [], {}
    for name, bolt_path, mission in MISSIONS:
        chk = _entry_payload(archive, bolt_path)
        row = decode(chk)
        row.update(name=name, mission=mission)
        rows.append(row)
        artifacts[f'{name.lower()}/source.chk'] = chk
        artifacts[f'{name.lower()}/PUNI.bin'] = row['puni']
        artifacts[f'{name.lower()}/effective.bin'] = row['effective']
        artifacts[f'{name.lower()}/OWNR.bin'] = row['owners']
        artifacts[f'{name.lower()}/SIDE.bin'] = row['races']
    artifacts['sc64_early_zerg_permissions_generated.h'] = render_header(rows).encode('ascii')
    manifest = {'rom_sha256': ROM_SHA256, 'extractor_commit': EXTRACTOR_COMMIT,
                'extractor_sha256': digest(extractor_path.read_bytes()),
                'missions': [{'name': n, 'path': p, 'global_id': i} for n, p, i in MISSIONS],
                'files': {name: {'sha256': digest(data), 'bytes': len(data)} for name, data in artifacts.items()}}
    output.mkdir(parents=True)
    for name, data in artifacts.items():
        dest = output / name
        dest.parent.mkdir(parents=True, exist_ok=True)
        dest.write_bytes(data)
    (output / 'manifest.json').write_text(json.dumps(manifest, indent=2) + '\n')
    return manifest


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rom', required=True, type=Path)
    parser.add_argument('--extractor', required=True, type=Path)
    parser.add_argument('--out', type=Path, default=ROOT / '.local/sc64/early-permissions')
    args = parser.parse_args()
    manifest = run(args.rom, args.extractor, args.out)
    print(f"verified Z1-Z4 permissions: {len(manifest['files'])} private files")


if __name__ == '__main__':
    main()
