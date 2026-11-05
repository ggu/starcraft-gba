#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Build an opt-in shared Cocoon Init-loop bank from verified private assets."""
from __future__ import annotations
import argparse
import json
import sys
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
from tools import sc64_guardian_cocoon_assets as assets
from tools import sc64_sprites as sprites
from tools import sc64_construction_sprites as construction
from tools import sc64_effect_palette as effect

VARIANTS = 1
DURATION = 750
MAX_BANK_BYTES = 32768

def compile_timeline(payload: bytes, start: int, frame_count: int, variant: int, loop_target: int) -> bytes:
    """Bounded Init-only interpreter, run at preparation time, not on draw."""
    if not 0 <= variant < VARIANTS or not 1 <= frame_count <= 256:
        raise ValueError('invalid timeline parameters')
    pc, frame, rng = start, 0, variant + 1
    seen_underlay = False
    result = bytearray()
    steps = 0
    while len(result) <= DURATION:
        steps += 1
        if steps > 10000:
            raise ValueError('non-yielding or excessive source animation')
        op, args, next_pc, jump = construction.decode_instruction(payload, pc)
        if op == 9:
            if seen_underlay or args != [12, 0, 42]:
                raise ValueError('unsupported underlay instruction')
            seen_underlay = True
        elif op == 0:
            frame = args[0]
            if frame >= frame_count:
                raise ValueError('source frame out of bounds')
        elif op in (5, 6):
            if not seen_underlay:
                raise ValueError('wait before underlay')
            if op == 5:
                duration = args[0] + 1
            else:
                low, high = args
                if low > high:
                    raise ValueError('invalid random wait')
                rng = (rng * 1664525 + 1013904223) & 0xffffffff
                duration = low + (rng % (high - low + 1)) + 1
            result.extend([frame] * min(duration, DURATION + 1 - len(result)))
        elif op == 7:
            if jump is None or jump != loop_target or not 0 <= jump < len(payload):
                raise ValueError('invalid animation jump')
            pc = jump
            continue
        else:
            raise ValueError(f'unsupported reachable opcode {op}')
        pc = next_pc
    if not seen_underlay:
        raise ValueError('missing underlay')
    return bytes(result)

def c_array(name: str, data: bytes) -> str:
    rows = [f'static const unsigned char {name}[{len(data)}] = {{']
    rows += ['    ' + ','.join(str(x) for x in data[i:i+24]) + ',' for i in range(0, len(data), 24)]
    return '\n'.join(rows + ['};'])

def build(rom: Path, extractor: Path, out: Path, remap_path: Path, palette_path: Path) -> dict:
    if out.exists():
        raise ValueError('output already exists; choose a fresh private directory')
    report = assets.prepare(rom, extractor, out)
    child = report['images']['12'].get('Init', [])
    if child != [(5, [1]), (29, []), (7, [31250])]:
        raise ValueError('unsupported underlay animation')
    loader = sprites.load_extractor(extractor)
    archive = loader.BoltArchive(loader.load_rom(str(rom)))
    payload = sprites._entry_payload(archive, '012/002')
    scripts = construction.script_index(payload)
    start = construction.image_animation_starts(payload, scripts[8])['Init']
    timeline = b''.join(compile_timeline(payload, start, 23, v, 21338) for v in range(VARIANTS))
    grp = sprites.parse_grp(sprites._entry_payload(archive, report['unit']['bolt_path']))
    remap = remap_path.read_bytes()
    if len(remap) != 256: raise ValueError('invalid palette remap')
    source_palette = sprites._entry_payload(archive, '00B/004')
    dark = sprites._entry_payload(archive, '00B/005')
    if len(dark) < 19*256: raise ValueError('missing source shadow row')
    palette = json.loads(palette_path.read_text())['gba_rgb555']
    shadow = effect.project_light_table(source_palette, dark[18*256:19*256], palette)
    lines = ['/* Private generated source art. Do not commit. */',
             '#ifndef SC64_SHARED_COCOON_BANK_GENERATED_H', '#define SC64_SHARED_COCOON_BANK_GENERATED_H']
    entries = []
    for n, frame in enumerate(grp['frames']):
        x,y,w,h = frame['x'],frame['y'],frame['frame_width'],frame['frame_height']
        primary, mask = bytearray(), bytearray()
        for yy in range(y,y+h):
            for xx in range(x,x+w):
                pos=yy*frame['width']+xx; opaque=frame['alpha'][pos]
                value=remap[frame['pixels'][pos]] if opaque else 0
                if opaque and not value: raise ValueError('opaque pixel maps to transparent')
                primary.append(value);mask.append(1 if opaque else 0)
        lines += [c_array(f'cocoon_p{n}',bytes(primary)), c_array(f'cocoon_s{n}',bytes(mask))]
        entries.append(f'{{{x},{y},{w},{h},48,48,cocoon_p{n},cocoon_s{n}}}')
    lines += [c_array('cocoon_shadow_lut',shadow),c_array('cocoon_timeline',timeline),
              'static const Sc64CocoonFrame cocoon_frames[23] = {\n    '+',\n    '.join(entries)+'\n};',
              'static const Sc64CocoonBank cocoon_bank = {cocoon_frames,23,cocoon_timeline,1,751,cocoon_shadow_lut,0,42};',
              '#endif']
    frame_bytes=sum(2*f['frame_width']*f['frame_height'] for f in grp['frames'])
    # Conservative 64-bit host descriptor allowance; code and alignment excluded.
    data_budget=frame_bytes+len(timeline)+len(shadow)+23*40+64
    if data_budget > MAX_BANK_BYTES:
        raise ValueError('staged Cocoon data exceeds 32 KiB budget')
    text='\n'.join(lines)+'\n'
    (out/'sc64_shared_cocoon_bank_generated.h').write_text(text)
    report.update({'timeline_bytes':len(timeline),'frame_payload_bytes':sum(2*f['frame_width']*f['frame_height'] for f in grp['frames']),
                   'shadow_lut_bytes':len(shadow),'data_budget_bytes':data_budget,'data_budget_limit':MAX_BANK_BYTES,'timeline_policy':'one deterministic approximation of source random waits; update cadence unverified',
                   'render_scope':'separate primary and modifier-10 underlay; no game hook'})
    (out/'bank-manifest.json').write_text(json.dumps(report,indent=2)+'\n')
    return report

def main() -> int:
    p=argparse.ArgumentParser(description=__doc__)
    for arg in ('rom','extractor','out','remap','palette'): p.add_argument('--'+arg,required=True,type=Path)
    a=p.parse_args()
    try: report=build(a.rom,a.extractor,a.out,a.remap,a.palette)
    except (OSError,ValueError,KeyError) as e:
        print(f'Cocoon bank: {e}',file=sys.stderr);return 1
    print(f"Prepared private bank: {report['frame_payload_bytes']} frame bytes + {report['timeline_bytes']} timeline bytes + 256 LUT bytes")
    return 0
if __name__ == '__main__': raise SystemExit(main())
