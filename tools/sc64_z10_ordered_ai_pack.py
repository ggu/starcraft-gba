#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Generate private, bounded Z10 attack-request tables from the AI audit.

This stages ordered requests, not an AI bytecode interpreter. Timing converts
source waits at the existing provisional 5/4 ratio. Unknown branches, carryover
attack groups and ambiguous loop state fail closed.
"""
import argparse
import hashlib
import json
from pathlib import Path

REPORT_SHA256 = '485aadfcae3ec6bcf3cad68c6015d5278a4c63d9bcbc78917e3d7ac101d3bc47'
ROM_SHA1 = 'a36d91b5aac7bb95df0d7de5f9814f5270e25c2b'
BOLT_SHA256 = '96fd2408033e3c8ba8900fc719b1e90f9e828b932d382f51fe5b16cdff1af054'
EXTRACTOR = '09b57f787a247107d1de82b8c5eab2734667ea1d'
IDS = ('Z10a', 'Z10d', 'Z10e')
RAW_TYPES = (65, 66, 70, 71, 72, 83)


def extract(program):
    ins = program['instructions']
    positions = {v['offset']: i for i, v in enumerate(ins)}
    if len(positions) != len(ins):
        raise ValueError('duplicate instruction offset')
    pc = 0
    waits = 0
    groups = {}
    prepared = False
    clear_required = False
    attacks = []
    loops = []
    branch_count = 0
    while pc < len(ins):
        op = ins[pc]
        name, args = op['name'], op['operands']
        if name == 'wait':
            if len(args) != 1 or args[0] < 0: raise ValueError('invalid wait')
            waits += args[0]
        elif name == 'attack_add':
            if clear_required or prepared or len(args) != 2 or args[1] not in RAW_TYPES or not 1 <= args[0] <= 255:
                raise ValueError('invalid attack_add state or operands')
            if args[1] in groups: raise ValueError('duplicate attack type is ambiguous')
            groups[args[1]] = args[0]
        elif name == 'attack_prepare':
            if clear_required or prepared or not groups: raise ValueError('invalid attack_prepare state')
            prepared = True
        elif name == 'attack_do':
            if clear_required or not prepared or not groups: raise ValueError('invalid attack_do state')
            delay = (waits * 5 + 3) // 4
            if not 1 <= delay <= 65535: raise ValueError('invalid scaled wait')
            attacks.append((delay, tuple(groups.items())))
            waits = 0; prepared = False; clear_required = True
        elif name == 'attack_clear':
            if not clear_required: raise ValueError('ambiguous attack_clear')
            groups = {}; clear_required = False
        elif name == 'goto':
            if clear_required or prepared or groups or len(args) != 1 or args[0] not in positions:
                raise ValueError('ambiguous loop entry')
            if args[0] >= op['offset']: raise ValueError('expected backward loop')
            loops.append(len(attacks)); branch_count += 1
            if branch_count == 2: break
            pc = positions[args[0]]; continue
        elif name.startswith('attack_'):
            raise ValueError('unsupported attack opcode')
        pc += 1
    if branch_count != 2 or len(loops) != 2:
        raise ValueError('expected one reproducible backward loop')
    loop_len = loops[1] - loops[0]
    if loop_len <= 0 or loop_len > loops[0]: raise ValueError('invalid loop length')
    loop_start = loops[0] - loop_len
    if attacks[loop_start:loops[0]] != attacks[loops[0]:loops[1]]:
        raise ValueError('loop attack requests or timing are not stable')
    if loops[0] > 255: raise ValueError('too many attack steps')
    return attacks[:loops[0]], loop_start


def pack(report):
    if report.get('schema') != 1: raise ValueError('unsupported report schema')
    source = report.get('source', {})
    if (source.get('normalized_rom_sha1'), source.get('bolt_payload_sha256'), source.get('extractor_commit')) != (ROM_SHA1, BOLT_SHA256, EXTRACTOR):
        raise ValueError('unexpected authenticated source provenance')
    rows = report.get('analysis', {}).get('programs', [])
    programs = {p['id']: p for p in rows}
    if len(programs) != len(rows) or not all(i in programs for i in IDS): raise ValueError('missing or duplicate programs')
    out = ['/* Generated privately from the authenticated Z10 AI audit. */',
           '#ifndef SC64_Z10_ORDERED_AI_GENERATED_H', '#define SC64_Z10_ORDERED_AI_GENERATED_H']
    for name in IDS:
        steps, loop = extract(programs[name])
        out.append(f'static const Sc64Z10OrderedStep sc64_{name.lower()}_steps[] = {{')
        for delay, counts in steps:
            padded = list(counts) + [(0, 0)] * (len(RAW_TYPES) - len(counts))
            out.append('    { %du, { %s }, { %s } },' %
                       (delay, ', '.join(f'{raw}u' for raw, _ in padded),
                        ', '.join(f'{count}u' for _, count in padded)))
        out.extend(['};', f'#define SC64_{name.upper()}_LOOP {loop}u',
                    f'#define SC64_{name.upper()}_COUNT {len(steps)}u'])
    out.extend(['#endif', ''])
    return '\n'.join(out)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('report', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    root = Path.cwd().resolve()
    output = args.output.resolve()
    if not (output.is_relative_to(root / 'build') or output.is_relative_to(root / '.local')):
        parser.error('generated header must stay in ignored build/ or .local/')
    if output == args.report.resolve() or (output.exists() and output.samefile(args.report)):
        raise ValueError('generated header must not overwrite the input report')
    report_bytes = args.report.read_bytes()
    if hashlib.sha256(report_bytes).hexdigest() != REPORT_SHA256:
        raise ValueError('AI audit report does not match the pinned authenticated report')
    result = pack(json.loads(report_bytes))
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(result)


if __name__ == '__main__': main()
