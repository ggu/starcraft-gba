#!/usr/bin/env python3
"""Import source-authored unit walk cycles from a verified SC64 ROM.

Each unit image's IScript Walking animation loops over 17-frame poses with
`playfram` and `wait`. This tool follows that loop for every directional unit
in the facing bank and writes its timed pose steps to a private header for
SC64_UNIT_WALK_FRAMES. Each pose stores all 32 directions, the 15 western ones
pre-mirrored, so a walking unit always takes the plain sprite blitter; the
mirrored blitter runs from ROM and costs several times as much. Pose 0's
eastern pixels are shared with the facing bank. Units whose loop shows a
single pose (the SCV, Probe and most flyers hover) are recorded in the
manifest and keep their facing frame. Output stays in ignored .local/ folders.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
from tools import sc64_construction_sprites as construction  # noqa: E402
from tools import sc64_race_sprites as races  # noqa: E402
from tools import sc64_sprites as sprites  # noqa: E402
from tools import sc64_unit_attack_sprites as attack  # noqa: E402
from tools import sc64_unit_facing as facing  # noqa: E402

OUT = ROOT / '.local/sc64/walk/generated'
FACING_MANIFEST = facing.OUT / 'manifest.json'
DIRECTIONS = facing.DIRECTIONS
HEADINGS = 32
MAX_SCRIPT_STEPS = 256
# Bounds the runtime sampler: steps per cycle and source ticks per cycle.
MAX_CYCLE_STEPS = 16
MAX_CYCLE_TICKS = 255
OP_PLAYFRAM, OP_WAIT, OP_WAITRAND, OP_GOTO = 0, 5, 6, 7
OP_END, OP_CALL, OP_RETURN = 22, 53, 54


class Unsupported(ValueError):
    pass


def walk_cycle(payload: bytes, start: int) -> tuple[list[tuple[int, int]], int]:
    """Return the looping (tick, frame) steps of one Walking animation.

    Ticks are source updates from the loop label; anything before the label
    (such as a setup pose) plays once and is not part of the cycle.
    """
    pc, tick = start, 0
    events: list[tuple[int, int]] = []
    entered: dict[int, tuple[int, int]] = {}
    for _ in range(MAX_SCRIPT_STEPS):
        if pc in entered:
            loop_tick, loop_index = entered[pc]
            break
        entered[pc] = (tick, len(events))
        opcode, args, next_pc, jump = construction.decode_instruction(payload, pc)
        if opcode == OP_PLAYFRAM:
            events.append((tick, args[0]))
        elif opcode == OP_WAIT:
            tick += args[0]
        elif opcode == OP_WAITRAND:
            raise Unsupported('Walking uses waitrand')
        elif opcode in (OP_END, OP_RETURN):
            raise Unsupported('Walking ends without looping')
        elif opcode == OP_CALL:
            raise Unsupported('Walking calls a subroutine')
        elif opcode == OP_GOTO:
            pc = jump
            continue
        elif jump is not None:
            raise Unsupported(f'Walking branches with opcode {opcode}')
        pc = next_pc
    else:
        raise Unsupported('Walking loop exceeds the bounded script walk')
    cycle_ticks = tick - loop_tick
    if cycle_ticks <= 0:
        raise Unsupported('Walking loop has no wait')
    if cycle_ticks > MAX_CYCLE_TICKS:
        raise Unsupported(f'Walking loop lasts {cycle_ticks} source ticks')
    loop_events = events[loop_index:]
    if not loop_events:
        raise Unsupported('Walking loop shows no frame')
    # A playfram after the lap's last wait lands on tick 0 of the next lap,
    # ahead of that lap's own tick-0 frames. Without a tick-0 frame, a lap
    # opens on the pose the previous lap ended with.
    timeline = sorted(((tick - loop_tick) % cycle_ticks,
                       0 if tick - loop_tick == cycle_ticks else 1, order, frame)
                      for order, (tick, frame) in enumerate(loop_events))
    steps: list[tuple[int, int]] = []
    if timeline[0][0] != 0:
        steps.append((0, timeline[-1][3]))
    for rel, _wrapped, _order, frame in timeline:
        if steps and steps[-1][0] == rel:
            steps[-1] = (rel, frame)
        else:
            steps.append((rel, frame))
    merged = [steps[0]]
    for step in steps[1:]:
        if step[1] != merged[-1][1]:
            merged.append(step)
    if len({frame for _tick, frame in merged}) < 2:
        raise Unsupported('Walking loop holds one pose')
    if len(merged) > MAX_CYCLE_STEPS:
        raise Unsupported(f'Walking loop has {len(merged)} poses')
    return merged, cycle_ticks


def emit_pixels(lines: list[str], symbol: str, data: bytes) -> None:
    lines.append(f'static const unsigned char {symbol}[{len(data)}] = {{')
    for i in range(0, len(data), 16):
        lines.append('    ' + ','.join(f'0x{v:02x}' for v in data[i:i + 16]) + ',')
    lines.append('};')


def generate(rom: Path, remap_path: Path, out: Path, facing_manifest: Path) -> dict:
    extractor = sprites.load_extractor(sprites.UPSTREAM)
    normalized = extractor.load_rom(str(rom))
    sha1 = hashlib.sha1(normalized).hexdigest()
    if sha1 != sprites.EXPECTED_USA_ROM_SHA1:
        raise ValueError(f'Expected verified USA ROM {sprites.EXPECTED_USA_ROM_SHA1}, got {sha1}')
    remap = remap_path.read_bytes()
    if len(remap) != 256:
        raise ValueError('sprite remap must have 256 entries')
    facing_summary = json.loads(facing_manifest.read_text())
    if facing_summary.get('rom_sha1') != sha1:
        raise ValueError('facing bank was generated from a different ROM')
    # Pose 0 reuses the facing bank's pixels; only units that bank kept qualify.
    facing_kept = {entry['unit_id']: entry for entry in facing_summary['entries']
                   if 'skipped' not in entry and entry.get('directional')}
    archive = extractor.BoltArchive(normalized)
    entries = {entry.path: entry for entry in archive.entries()}

    def read(path: str) -> bytes:
        entry = entries.get(path)
        if entry is None:
            raise ValueError(f'ROM BOLT entry missing: {path}')
        return archive.read(entry)

    units = sprites._le_array(read('00C/001'), sprites.UNIT_COUNT, 1, 'units.dat')
    flingy = sprites._le_array(read('00C/002'), sprites.FLINGY_COUNT, 2, 'flingy.dat')
    sprite_image = sprites._le_array(read('00C/003'), sprites.SPRITE_COUNT, 2, 'sprites.dat')
    directional, image_iscript, image_grp = attack.image_tables(read('00C/000'))
    payload = read('012/002')
    scripts = construction.script_index(payload)

    lines = ['/* Generated from the verified USA ROM; private local asset. */',
             '#ifndef SC64_UNIT_WALK_GENERATED_H',
             '#define SC64_UNIT_WALK_GENERATED_H',
             '#define SC64_UNIT_WALK_BANK_VERSION 1',
             '#if !defined(SC64_UNIT_FACING_BANK_VERSION) || SC64_UNIT_FACING_BANK_VERSION != 2',
             '#error "sc64_unit_walk_generated.h needs the version 2 unit-facing bank"',
             '#endif',
             '/* One timed pose of a walk cycle; frames holds its 32 directions. */',
             'typedef struct { unsigned char tick; const Sc64SpriteFrame *frames; } Sc64UnitWalkStep;',
             'typedef struct { const Sc64UnitWalkStep *steps; unsigned char count, cycle_ticks; } '
             'Sc64UnitWalkCycle;']
    manifest: list[dict] = []
    pose_symbols: dict[tuple[int, int], str] = {}
    cycle_symbols: dict[int, str] = {}
    pixel_symbols: dict[tuple[int, int, bytes], str] = {}
    cases: list[tuple[int, str]] = []
    total = 0
    grps: dict[int, dict] = {}
    for name, raw in sorted(facing.unit_ids().items(), key=lambda item: item[1]):
        record: dict = {'name': name, 'unit_id': raw}
        manifest.append(record)
        if raw not in facing_kept:
            record['skipped'] = 'no directional facing frames'
            continue
        image_id = sprite_image[flingy[units[raw]]]
        record['image_id'] = image_id
        if image_id != facing_kept[raw]['image_id'] or not directional[image_id]:
            record['skipped'] = 'facing bank uses a different or non-directional image'
            continue
        script_id = image_iscript[image_id]
        starts = (construction.image_animation_starts(payload, scripts[script_id])
                  if script_id in scripts else {})
        if 'Walking' not in starts:
            record['skipped'] = 'image has no Walking animation'
            continue
        try:
            steps, cycle_ticks = walk_cycle(payload, starts['Walking'])
        except Unsupported as error:
            record['skipped'] = str(error)
            continue
        if image_id not in grps:
            flat = image_grp[image_id] - 1
            grp_data = read(f'{0xD + flat // 256:03X}/{flat % 256:03X}')
            if hashlib.sha1(grp_data).hexdigest() != facing_kept[raw]['sha1']:
                raise ValueError(f'{name}: GRP differs from the facing bank')
            grps[image_id] = sprites.parse_grp(grp_data)
        grp = grps[image_id]
        for _tick, base in steps:
            if base % DIRECTIONS or base + DIRECTIONS > len(grp['frames']):
                raise ValueError(f'{name}: walk frame {base} is not a directional pose in its GRP')
        record.update({'cycle_ticks': cycle_ticks,
                       'steps': [{'tick': tick, 'pose': base // DIRECTIONS} for tick, base in steps]})
        if image_id in cycle_symbols:
            cases.append((raw, cycle_symbols[image_id]))
            continue
        step_entries = []
        for tick, base in steps:
            key = (image_id, base)
            if key not in pose_symbols:
                symbol = f'sc64_unit_walk_i{image_id}_p{base // DIRECTIONS}'
                frames = []
                for heading in range(HEADINGS):
                    mirror = heading > 16
                    d = HEADINGS - heading if mirror else heading
                    frame = grp['frames'][base + d]
                    width, height = frame['frame_width'], frame['frame_height']
                    data = facing.frame_pixels(frame, remap)
                    if mirror:
                        data = b''.join(data[row * width:(row + 1) * width][::-1]
                                        for row in range(height))
                    pkey = (width, height, data)
                    psym = pixel_symbols.get(pkey)
                    if psym is None and base == 0 and not mirror:
                        # The facing bank already holds pose 0's eastern frames.
                        psym = f'sc64_unit_facing_{raw}_{d}_pixels'
                        pixel_symbols[pkey] = psym
                    if psym is None:
                        psym = f'{symbol}_{heading}_pixels'
                        pixel_symbols[pkey] = psym
                        total += len(data)
                        emit_pixels(lines, psym, data)
                    offset_x = frame['width'] - frame['x'] - width if mirror else frame['x']
                    frames.append(f'{{{offset_x},{frame["y"]},{width},{height},'
                                  f'{frame["width"]},{frame["height"]},{psym}}}')
                lines.append(f'static const Sc64SpriteFrame {symbol}[{HEADINGS}] = {{')
                lines.extend(f'    {item},' for item in frames)
                lines.append('};')
                pose_symbols[key] = symbol
            step_entries.append(f'{{{tick}u,{pose_symbols[key]}}}')
        cycle = f'sc64_unit_walk_i{image_id}'
        lines.append(f'static const Sc64UnitWalkStep {cycle}_steps[{len(step_entries)}] = {{')
        lines.extend(f'    {item},' for item in step_entries)
        lines.append('};')
        lines.append(f'static const Sc64UnitWalkCycle {cycle} = '
                     f'{{{cycle}_steps,{len(step_entries)}u,{cycle_ticks}u}};')
        cycle_symbols[image_id] = cycle
        cases.append((raw, cycle))

    lines += ['/* The source walk cycle of a unit, or 0 when it has none. */',
              'static inline const Sc64UnitWalkCycle *sc64_unit_walk_cycle(unsigned short raw_type) {',
              '    switch (raw_type) {',
              *(f'    case {raw}: return &{cycle};' for raw, cycle in cases),
              '    default: return 0;', '    }', '}',
              f'#define SC64_UNIT_WALK_MAX_STEPS {MAX_CYCLE_STEPS}u',
              f'#define SC64_UNIT_WALK_HEADINGS {HEADINGS}u',
              f'#define SC64_UNIT_WALK_UNITS {len(cases)}u',
              f'#define SC64_UNIT_WALK_PIXEL_BYTES {total}u',
              '#endif', '']
    out.mkdir(parents=True, exist_ok=True)
    header = out / 'sc64_unit_walk_generated.h'
    header.write_text('\n'.join(lines))
    summary = {'rom_sha1': sha1, 'units': len(cases), 'pixel_bytes': total,
               'source': 'images.dat IScript Walking animation (playfram/wait loop)',
               'header_sha256': hashlib.sha256(header.read_bytes()).hexdigest(),
               'entries': manifest}
    (out / 'manifest.json').write_text(json.dumps(summary, indent=2) + '\n')
    return summary


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('--rom', type=Path, default=races.ROM)
    parser.add_argument('--remap', type=Path, default=races.PALETTE_REMAP)
    parser.add_argument('--facing-manifest', type=Path, default=FACING_MANIFEST)
    parser.add_argument('--out', type=Path, default=OUT)
    args = parser.parse_args()
    summary = generate(args.rom, args.remap, args.out, args.facing_manifest)
    print(f'{summary["units"]} walking units, {summary["pixel_bytes"]} pixel bytes')
    for entry in summary['entries']:
        if 'skipped' in entry:
            print(f'  kept facing {entry["name"]} raw{entry["unit_id"]}: {entry["skipped"]}')
        else:
            poses = ','.join(str(step['pose']) for step in entry['steps'])
            print(f'  {entry["name"]} raw{entry["unit_id"]}: poses {poses} over '
                  f'{entry["cycle_ticks"]} source ticks')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
