#!/usr/bin/env python3
"""Generate private all-campaign Protoss upgrade/technology policy from the owned USA ROM.

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
from tools.sc64_races_pack import MISSIONS

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
    if any(settings[i] not in (0, 1) for i in UPGRADES):
        raise ValueError('invalid upgrade setting selector')
    default_costs = [settings[i] for i in UPGRADES]
    # OpenBW reads these optional expansion chunks after base chunks.
    base = decode_restrictions(sections[b'UPGR'], 46, UPGRADES)
    tech = decode_restrictions(sections[b'PTEC'], 24, TECHS, True)
    if b'PUPx' in sections:
        base = decode_restrictions(sections[b'PUPx'], 61, UPGRADES)
    if b'PTEx' in sections:
        tech = decode_restrictions(sections[b'PTEx'], 44, TECHS, True)
    return base, tech, default_costs


def validate_catalog(catalog):
    """Return explicit T/Z/P and 1..10 indexing independent of packer order."""
    result = {}
    paths = set()
    for name, path, _title in catalog:
        if len(name) < 2 or name[0] not in 'TZP' or not name[1:].isdigit():
            raise ValueError('invalid campaign identity')
        number = int(name[1:])
        if number not in range(1, 11) or name != name[0] + str(number):
            raise ValueError('invalid campaign number')
        key = (name[0], number)
        if key in result or path in paths:
            raise ValueError('duplicate campaign identity or path')
        result[key] = path
        paths.add(path)
    if set(result) != {(race, number) for race in 'TZP' for number in range(1, 11)}:
        raise ValueError('incomplete campaign catalog')
    return result


def collect(archive, catalog):
    """Require a unique matching archive entry for every catalog identity."""
    paths = set(catalog.values())
    entries = {}
    for entry in archive.entries():
        if entry.path in paths:
            if entry.path in entries:
                raise ValueError('duplicate campaign archive entry')
            entries[entry.path] = entry
    if set(entries) != paths:
        raise ValueError('missing campaign archive entry')
    result = {}
    for key, path in catalog.items():
        data = archive.read(entries[path])
        result[key] = (policy(parse_chk_sections(data)), digest(data), path)
    return result


def emit(rows):
    lines = ['/* Private ROM-derived numeric policy; do not commit. */',
             '#ifndef SC64_PROTOSS_UPGRADE_POLICY_GENERATED_H',
             '#define SC64_PROTOSS_UPGRADE_POLICY_GENERATED_H',
             '/* Race index: T=0 Z=1 P=2; mission index: chapter minus one; raw owner: 0..7. */',
             '/* Upgrade pairs are maximum, initial; order: ground armor, air armor, ground weapons, air weapons, shields, Singularity. */',
             '/* Tech pairs are available, researched; order: Storm, Hallucination, Recall, Stasis, Archon Warp. */',
             '/* Cost support is 1 only for source-default pricing/time; 0 means unsupported override, not a default price. */']
    for name, index, count in (('sc64_pr_campaign_upgrades', 0, 6), ('sc64_pr_campaign_tech', 1, 5)):
        lines.append(f'static const unsigned char {name}[3][10][8][{count}][2] = {{')
        for race in 'TZP':
            lines.append('  {')
            for number in range(1, 11):
                row, sha, path = rows[(race, number)]
                lines.append(f'    {{ /* {race}{number} {path} CHK SHA-256 {sha} */')
                for owner in row[index]:
                    lines.append('      {' + ','.join('{%d,%d}' % pair for pair in owner) + '},')
                lines.append('    },')
            lines.append('  },')
        lines.append('};')
    lines.append('static const unsigned char sc64_pr_campaign_cost_supported[3][10][6] = {')
    for race in 'TZP':
        lines.append('  {')
        for number in range(1, 11):
            lines.append('    {' + ','.join(str(x) for x in rows[(race, number)][0][2]) + '},')
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
    try:
        rows = collect(archive, validate_catalog(MISSIONS))
    except (KeyError, ValueError) as exc:
        p.error(str(exc))
    a.output.parent.mkdir(parents=True, exist_ok=True)
    with a.output.open('x') as f:
        f.write(emit(rows))
    print(f'wrote private policy: {a.output}')

if __name__ == '__main__':
    main()
