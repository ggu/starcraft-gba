#!/usr/bin/env python3
"""Generate private campaign upgrade/technology policy from the owned USA ROM.

Uses the pinned OpenBW version-59/63 UMS chunk precedence as a reference,
not a claim of observed N64 behavior. No extracted source asset is written.
SPDX-License-Identifier: GPL-3.0-or-later
"""
import argparse
import hashlib
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
from tools.sc64_sprites import load_extractor
from tools.sc64_zp_scenario_pack import parse_chk_sections

ROM_SHA256 = 'e7d21880c13953b45b8f8db6b2b6cfa02daa29d35b23799a967e96f21001b020'
EXTRACTOR_SHA256 = '6a84761a9a8074fcc393527a7b17cea194084e8ab49176ff82fa3bef8e1b1dc2'
UPGRADES = (5, 6, 13, 14, 15, 33)
TECHS = (19, 20, 21, 22, 23)


def digest(data):
    return hashlib.sha256(data).hexdigest()


def decode_restrictions(payload, count, ids, boolean=False):
    """Resolve player-specific and global defaults for eight source owners."""
    if len(payload) != 38 * count:
        raise ValueError('invalid restriction section length')
    out = []
    for owner in range(8):
        row = []
        for ident in ids:
            if ident >= count:
                raise ValueError('id outside section')
            i = owner * count + ident
            use_global = payload[26 * count + i]
            if use_global not in (0, 1):
                raise ValueError('invalid global selector')
            first = payload[24 * count + ident] if use_global else payload[i]
            second = payload[25 * count + ident] if use_global else payload[12 * count + i]
            if boolean and (first not in (0, 1) or second not in (0, 1)):
                raise ValueError('invalid technology restriction')
            if not boolean and (first > 3 or second > first or (ident == 33 and first > 1)):
                raise ValueError('invalid upgrade level')
            row.append((first, second))
        out.append(row)
    return out


def policy(sections):
    version = sections.get(b'VER ', b'')
    if len(version) != 2 or struct.unpack('<H', version)[0] not in (59, 63):
        raise ValueError('unsupported CHK version')
    # This compact policy supports only default upgrade costs/times. OpenBW
    # reads UPGx after UPGS; reject overrides rather than misprice a job.
    settings = sections[b'UPGS']
    if len(settings) != 598:
        raise ValueError('invalid UPGS length')
    count = 46
    if b'UPGx' in sections:
        settings = sections[b'UPGx']; count = 61
        if len(settings) != 794:
            raise ValueError('invalid UPGx length')
    if any(settings[i] != 1 for i in UPGRADES):
        raise ValueError('custom upgrade prices/times not supported')
    # OpenBW reads these optional expansion chunks after base chunks.
    base = decode_restrictions(sections[b'UPGR'], 46, UPGRADES)
    tech = decode_restrictions(sections[b'PTEC'], 24, TECHS, True)
    if b'PUPx' in sections:
        base = decode_restrictions(sections[b'PUPx'], 61, UPGRADES)
    if b'PTEx' in sections:
        tech = decode_restrictions(sections[b'PTEx'], 44, TECHS, True)
    return base, tech


def emit(rows, hashes):
    lines = ['/* Private ROM-derived numeric policy; do not commit. */',
             '#ifndef SC64_PROTOSS_UPGRADE_POLICY_GENERATED_H',
             '#define SC64_PROTOSS_UPGRADE_POLICY_GENERATED_H',
             '/* Upgrade pairs are maximum, initial; order: ground armor, air armor, ground weapons, air weapons, shields, Singularity. */',
             '/* Tech pairs are available, researched; order: Storm, Hallucination, Recall, Stasis, Archon Warp. */']
    for name, index in (('sc64_pr_campaign_upgrades', 0), ('sc64_pr_campaign_tech', 1)):
        count = 6 if index == 0 else 5
        lines.append(f'static const unsigned char {name}[10][8][{count}][2] = {{')
        for mission, row in enumerate(rows):
            lines.append('  { /* P%d, CHK SHA-256 %s */' % (mission + 1, hashes[mission]))
            for owner in row[index]:
                lines.append('    {' + ','.join('{%d,%d}' % pair for pair in owner) + '},')
            lines.append('  },')
        lines.append('};')
    return '\n'.join(lines + ['#endif', ''])


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--rom', type=Path, required=True)
    p.add_argument('--extractor', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    a = p.parse_args()
    if '.local' not in a.output.resolve().parts:
        p.error('output must be in a private .local directory')
    if a.output.exists():
        p.error('output must be new')
    if digest(a.rom.read_bytes()) != ROM_SHA256 or digest(a.extractor.read_bytes()) != EXTRACTOR_SHA256:
        p.error('ROM or extractor authentication failed')
    extractor = load_extractor(a.extractor)
    archive = extractor.BoltArchive(extractor.load_rom(str(a.rom)))
    entries = {}
    for entry in archive.entries():
        if entry.path in {f'008/{i:03X}' for i in range(0x1e, 0x28)}:
            if entry.path in entries:
                p.error('duplicate campaign map')
            entries[entry.path] = entry
    rows, hashes = [], []
    for i in range(0x1e, 0x28):
        path = f'008/{i:03X}'
        if path not in entries:
            p.error(f'missing campaign map {path}')
        data = archive.read(entries[path]); hashes.append(digest(data))
        rows.append(policy(parse_chk_sections(data)))
    a.output.parent.mkdir(parents=True, exist_ok=True)
    with a.output.open('x') as f:
        f.write(emit(rows, hashes))
    print(f'wrote private policy: {a.output}')

if __name__ == '__main__':
    main()
