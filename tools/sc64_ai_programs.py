#!/usr/bin/env python3
"""Audit selected Z8 AI programs from the authenticated N64 BOLT leaf.

This tool decodes only the bounded command set present in the three Z8 AI
scripts. It reports command names and numeric operands, never bytes or extracted
program files. Its opcode layout follows the pinned PC AIscript reference;
equivalent N64 execution semantics are not established by this parser.

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
import sys

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_ROM = ROOT / '.local/sc64/rom/StarCraft64-USA.z64'
DEFAULT_OUT = ROOT / '.local/sc64/ai-catalog/z8-ai-policies.json'
CATALOG_TOOL = ROOT / 'tools/sc64_ai_catalog.py'
TARGET_IDS = ('Zer8', 'Ze8T', 'MvTe')
PYMS_COMMIT = 'bfc5d3aad0b5614a5aff72c223f8efa00afddfa4'

# Facts transcribed from the pinned PyMS AIBIN opcode names and parameter
# widths. Unknown commands intentionally fail closed instead of being guessed.
COMMANDS: dict[int, tuple[str, tuple[int, ...]]] = {
    0: ('goto', (2,)),
    2: ('wait', (2,)),
    3: ('start_town', ()),
    6: ('build', (1, 2, 1)),
    7: ('upgrade', (1, 1, 1)),
    8: ('tech', (2, 1)),
    11: ('attack_clear', ()),
    12: ('attack_add', (1, 2)),
    13: ('attack_prepare', ()),
    14: ('attack_do', ()),
    19: ('defensebuild_gg', (1, 2)),
    20: ('defensebuild_ag', (1, 2)),
    21: ('defensebuild_ga', (1, 2)),
    22: ('defensebuild_aa', (1, 2)),
    23: ('defenseuse_gg', (1, 2)),
    24: ('defenseuse_ag', (1, 2)),
    25: ('defenseuse_ga', (1, 2)),
    26: ('defenseuse_aa', (1, 2)),
    30: ('defenseclear_aa', ()),
    34: ('default_min', (1,)),
    35: ('defaultbuild_off', ()),
    36: ('stop', ()),
    38: ('move_dt', ()),
    46: ('max_force', (2,)),
    56: ('start_campaign', ()),
}


def _load_catalog_tool():
    spec = importlib.util.spec_from_file_location('sc64_ai_catalog_for_programs', CATALOG_TOOL)
    if spec is None or spec.loader is None:
        raise RuntimeError(f'cannot load authenticated catalog helper: {CATALOG_TOOL}')
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


def decode_segment(data: bytes, start: int, end: int) -> list[dict]:
    """Decode one catalog-bounded script region using known command widths."""
    if start < 4 or end <= start or end > len(data):
        raise ValueError('AI script region is outside the leaf bounds')
    instructions: list[dict] = []
    cursor = start
    while cursor < end:
        opcode = data[cursor]
        if opcode not in COMMANDS:
            raise ValueError(f'unsupported AI opcode {opcode} at leaf offset {cursor}')
        name, widths = COMMANDS[opcode]
        next_cursor = cursor + 1 + sum(widths)
        if next_cursor > end:
            raise ValueError(f'truncated {name} at leaf offset {cursor}')
        operands: list[int] = []
        pos = cursor + 1
        for width in widths:
            operands.append(int.from_bytes(data[pos:pos + width], 'little'))
            pos += width
        instruction = {
            'offset': cursor,
            'opcode': opcode,
            'name': name,
            'operands': operands,
            'next_offset': next_cursor,
        }
        if opcode == 0:
            instruction['branch_target'] = operands[0]
        instructions.append(instruction)
        cursor = next_cursor
    return instructions


def analyze_catalog(data: bytes, catalog: dict) -> dict:
    """Summarize the three target programs; no raw instruction bytes escape."""
    rows = {row['id']: row for row in catalog['entries']}
    missing = sorted(set(TARGET_IDS) - rows.keys())
    if missing:
        raise ValueError(f'missing required Z8 AI scripts: {", ".join(missing)}')
    starts = sorted({row['code_offset'] for row in catalog['entries'] if row['code_offset']})
    table_offset = catalog['header_table_offset']
    regions = {start: (start, next((x for x in starts if x > start), table_offset))
               for start in starts}
    decoded_regions: dict[int, list[dict]] = {}

    def region_for(address: int) -> tuple[int, int]:
        if not starts or address < starts[0] or address >= table_offset:
            raise ValueError(f'AI branch target {address} is outside code/table bounds')
        start = max(candidate for candidate in starts if candidate <= address)
        return regions[start]

    summaries = []
    for script_id in TARGET_IDS:
        row = rows[script_id]
        start = row['code_offset']
        if start not in regions:
            raise ValueError(f'{script_id} has no valid code region')
        reachable_regions: set[int] = set()
        worklist = [start]
        reachable_targets = []
        while worklist:
            region_start = worklist.pop()
            if region_start in reachable_regions:
                continue
            reachable_regions.add(region_start)
            lo, hi = regions[region_start]
            region_instructions = decoded_regions.setdefault(
                region_start, decode_segment(data, lo, hi))
            boundaries = {instruction['offset'] for instruction in region_instructions}
            for instruction in region_instructions:
                if 'branch_target' not in instruction:
                    continue
                target = instruction['branch_target']
                target_start, target_end = region_for(target)
                target_instructions = decoded_regions.setdefault(
                    target_start, decode_segment(data, target_start, target_end))
                if target not in {item['offset'] for item in target_instructions}:
                    raise ValueError(
                        f'{script_id} goto target {target} is not an instruction boundary')
                reachable_targets.append(target)
                if target_start not in reachable_regions:
                    worklist.append(target_start)
        lo, hi = regions[start]
        instructions = decoded_regions[start]
        op_counts: dict[str, int] = {}
        for instruction in instructions:
            name = instruction['name']
            op_counts[name] = op_counts.get(name, 0) + 1
        summaries.append({
            'id': script_id,
            'code_offset': start,
            'bounded_region_end': hi,
            'instruction_count': len(instructions),
            'op_counts': dict(sorted(op_counts.items())),
            'instructions': [
                {key: value for key, value in item.items() if key != 'opcode'}
                for item in instructions
            ],
            'reachable_code_regions': sorted(reachable_regions),
            'goto_targets': reachable_targets,
        })
    return {
        'reference': {
            'opcode_layout': f'PyMS AIBIN.py @ {PYMS_COMMIT}',
            'authenticated_n64_program_identity': 'BOLT 012/000 leaf SHA256 verified by catalog tool',
            'n64_interpreter_semantics': 'not established by this static parser',
            'wait_operand_units': 'unresolved for N64; raw operand values are retained',
        },
        'programs': summaries,
    }


def audit(rom_path: Path, extractor_path: Path | None = None) -> dict:
    catalog_tool = _load_catalog_tool()
    rom = catalog_tool.read_bounded_rom(rom_path)
    importer = catalog_tool.load_module(ROOT / 'tools/sc64_import.py',
                                        'sc64_ai_programs_import')
    # This source worktree deliberately has no references checkout. Point the
    # existing helper at the already pinned, read-only extractor checkout.
    if extractor_path is not None:
        importer.UPSTREAM = extractor_path
    catalog_tool.verify_extractor(importer)
    extractor = importer.upstream_module(importer.UPSTREAM)
    normalized = catalog_tool.normalize_rom(rom, extractor)
    rom_sha1 = catalog_tool.verify_normalized_rom(normalized)
    archive = extractor.BoltArchive(normalized)
    matches = [entry for entry in archive.entries() if entry.path == catalog_tool.BOLT_PATH]
    if len(matches) != 1:
        raise ValueError('verified ROM did not contain exactly one expected BOLT leaf')
    payload = archive.read(matches[0])
    if hashlib.sha256(payload).hexdigest() != catalog_tool.BOLT_SHA256:
        raise ValueError('authenticated BOLT leaf hash mismatch')
    catalog = catalog_tool.parse_catalog(payload)
    if catalog['table_end'] != len(payload):
        raise ValueError('AI catalog table does not terminate at the BOLT leaf boundary')
    return {
        'schema': 1,
        'source': {
            'normalized_rom_sha1': rom_sha1,
            'extractor_commit': catalog_tool.EXTRACTOR_COMMIT,
            'bolt_path': catalog_tool.BOLT_PATH,
            'bolt_payload_bytes': len(payload),
            'bolt_payload_sha256': catalog_tool.BOLT_SHA256,
        },
        'assessment': {
            'format': 'AIScript.bin-compatible command/table hypothesis',
            'n64_execution_equivalence': 'unproven; see reference limitations',
            'raw_program_bytes_included': False,
        },
        'analysis': analyze_catalog(payload, catalog),
    }


def private_output_path(path: Path) -> Path:
    root = ROOT.resolve()
    local_path = ROOT / '.local'
    if local_path.is_symlink():
        raise ValueError('.local must be a real directory, not a symlink')
    local = local_path.resolve()
    if local == root or root not in local.parents:
        raise ValueError('.local resolves outside this checkout')
    target = path.resolve()
    if target == local or local not in target.parents:
        raise ValueError('AI program metadata output must stay below ignored .local/')
    return target


def run(rom_path: Path, output: Path, extractor_path: Path | None = None) -> dict:
    output = private_output_path(output)
    if output == rom_path.resolve() or (output.exists() and rom_path.exists() and
                                        os.path.samefile(output, rom_path)):
        raise ValueError('AI program output must not overwrite or alias the input ROM')
    result = audit(rom_path, extractor_path)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(result, indent=2, sort_keys=True) + '\n', encoding='utf-8')
    return result


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rom', type=Path, default=DEFAULT_ROM,
                        help='user-supplied verified USA ROM')
    parser.add_argument('--out', type=Path, default=DEFAULT_OUT,
                        help='private metadata JSON path under this checkout .local/')
    parser.add_argument('--extractor', type=Path,
                        help='optional pinned extract_sc64_maps.py path')
    args = parser.parse_args()
    try:
        result = run(args.rom, args.out, args.extractor)
    except (OSError, RuntimeError, ValueError) as exc:
        parser.error(str(exc))
    counts = ', '.join(f"{row['id']}={row['instruction_count']} ops"
                       for row in result['analysis']['programs'])
    print(f'validated {counts}; private sanitized metadata written')


if __name__ == '__main__':
    main()
