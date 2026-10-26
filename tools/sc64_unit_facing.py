#!/usr/bin/env python3
"""Generate private per-direction unit frames from a verified SC64 ROM.

StarCraft unit GRPs store each animation step as 17 frames: facing north,
then clockwise to south. The other 15 directions are the mirrored frames. The
race sprite importer keeps only frame 0, so units always face north. This
tool writes frames 0-16 of every imported unit GRP (plus the Siege Tank
turret and siege-mode subunits) to a separate header for the default-off
SC64_UNIT_FACING presentation path. Output stays in ignored .local/ folders.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
from tools import sc64_race_sprites as races  # noqa: E402
from tools import sc64_sprites as base  # noqa: E402

OUT = ROOT / '.local/sc64/facing/generated'
DIRECTIONS = 17
# Brood War raw ids below 106 are units; higher ids are buildings and props.
FIRST_BUILDING_ID = 106
# Subunits drawn with the Siege Tank: tank-mode turret, siege-mode hull and turret.
EXTRA_UNIT_IDS = {'siege_tank_turret': 6, 'siege_tank_siege': 30, 'siege_tank_siege_turret': 31}


def unit_ids() -> dict[str, int]:
    merged: dict[str, int] = {}
    seen: set[int] = set()
    tables = [races.RACE_UNIT_IDS, races.T4_UNIT_IDS, races.T6_UNIT_IDS, races.LATE_UNIT_IDS,
              races.ZP_UNIT_IDS, races.Z5_UNIT_IDS, races.Z6_UNIT_IDS, races.P6_UNIT_IDS,
              races.P7_UNIT_IDS, EXTRA_UNIT_IDS]
    for table in tables:
        for name, raw in table.items():
            if raw < FIRST_BUILDING_ID and raw not in seen:
                merged[name] = raw
                seen.add(raw)
    return merged


def frame_pixels(frame: dict, remap: bytes) -> bytes:
    data = bytearray(base.crop_frame(frame))
    x, y, fw = frame['x'], frame['y'], frame['frame_width']
    for row in range(frame['frame_height']):
        for col in range(fw):
            si = (y + row) * frame['width'] + x + col
            di = row * fw + col
            data[di] = remap[data[di]] if frame['alpha'][si] else 0
    return bytes(data)


def pack_word_phases(width: int, height: int, pixels: bytes) -> tuple[tuple[int, ...], ...]:
    """Pack indexed pixels for gfx_blit_sprite_words' four x phases."""
    phases = []
    for phase in range(4):
        groups = (phase + width + 3) // 4
        words = []
        for row in range(height):
            for group in range(groups):
                color = mask = 0
                for lane in range(4):
                    sx = group * 4 + lane - phase
                    pixel = pixels[row * width + sx] if 0 <= sx < width else 0
                    color |= pixel << (lane * 8)
                    if pixel:
                        mask |= 0xff << (lane * 8)
                words.extend((color, mask))
        phases.append(tuple(words))
    return tuple(phases)


def directional_image_flags(images_dat: bytes) -> tuple[int, ...]:
    """Read the source images.dat has_directional_frames table."""
    return base._le_array(images_dat[base.IMAGE_COUNT * 4:], base.IMAGE_COUNT, 1,
                          'images.dat has_directional_frames')


def keep_directional_bank(name: str, directional: bool, frame_count: int) -> bool:
    """Select true source-directional GRPs, plus the authored static Tank hull."""
    if not directional and name != 'siege_tank_mode':
        return False
    if directional and frame_count < DIRECTIONS:
        raise ValueError(f'{name}: images.dat marks directional art, but GRP has only '
                         f'{frame_count} frames')
    return True


def generate(rom: Path, remap_path: Path, out: Path) -> dict:
    extractor = base.load_extractor(base.UPSTREAM)
    normalized = extractor.load_rom(str(rom))
    sha1 = hashlib.sha1(normalized).hexdigest()
    if sha1 != base.EXPECTED_USA_ROM_SHA1:
        raise ValueError(f'Expected verified USA ROM {base.EXPECTED_USA_ROM_SHA1}, got {sha1}')
    remap = remap_path.read_bytes()
    if len(remap) != 256:
        raise ValueError('sprite remap must have 256 entries')
    archive = extractor.BoltArchive(normalized)
    entries = {entry.path: entry for entry in archive.entries()}
    images_dat = base._entry_payload(archive, '00C/000')
    image_directional = directional_image_flags(images_dat)
    refs = base.resolve_grp_indices(archive, unit_ids())
    lines = ['/* Generated from the verified USA ROM; private local asset. */',
             '#ifndef SC64_UNIT_FACING_GENERATED_H',
             '#define SC64_UNIT_FACING_GENERATED_H',
             '#define SC64_UNIT_FACING_BANK_VERSION 2',
             '#define SC64_UNIT_FACING_DIRECTIONAL_METADATA 1',
             '#ifndef SC64_SPRITES_GENERATED_H',
             '#ifndef SC64_RACE_SPRITES_GENERATED_H',
             'typedef struct { short offset_x, offset_y; unsigned short w, h, canvas_w, canvas_h; '
             'const unsigned char *pixels; } Sc64SpriteFrame;',
             '#endif', '#endif']
    manifest, cases, word_cases, total = [], [], [], 0
    for name, ref in sorted(refs.items(), key=lambda item: item[1]['unit_id']):
        grp_data = archive.read(entries[ref['bolt_path']])
        grp = base.parse_grp(grp_data)
        image_id = ref['image_id']
        directional = bool(image_directional[image_id])
        static = name == 'siege_tank_mode'
        if not keep_directional_bank(name, directional, len(grp['frames'])):
            manifest.append({'name': name, 'unit_id': ref['unit_id'],
                             'image_id': image_id, 'directional': False,
                             'frames': len(grp['frames']),
                             'skipped': 'source images.dat marks this image non-directional'})
            continue
        symbol = f'sc64_unit_facing_{ref["unit_id"]}'
        frames = []
        for d in range(1 if static else DIRECTIONS):
            frame = grp['frames'][0 if static else d]
            data = frame_pixels(frame, remap)
            total += len(data)
            lines.append(f'static const unsigned char {symbol}_{d}_pixels[{len(data)}] = {{')
            for i in range(0, len(data), 16):
                lines.append('    ' + ','.join(f'0x{v:02x}' for v in data[i:i + 16]) + ',')
            lines.append('};')
            frames.append(f'{{{frame["x"]},{frame["y"]},{frame["frame_width"]},{frame["frame_height"]},'
                          f'{frame["width"]},{frame["height"]},{symbol}_{d}_pixels}}')
        if static:  # e.g. the siege-mode hull, which does not rotate
            frames = frames * DIRECTIONS
        lines.append(f'static const Sc64SpriteFrame {symbol}[{DIRECTIONS}] = {{')
        lines.extend(f'    {item},' for item in frames)
        lines.append('};')
        cases.append(f'    case {ref["unit_id"]}: return {symbol};')

        # Store all 32 visual directions as pre-mirrored packed sidecars.
        # This is optional ROM data: canonical callers enable the existing
        # SC64_UNIT_WORD_SPRITES blitter, while Showcase stays byte-indexed.
        lines.append('#ifdef SC64_UNIT_WORD_SPRITES')
        word_frame_symbols = []
        for direction in range(32):
            frame_index = direction if direction <= 16 else 32 - direction
            frame = grp['frames'][0 if static else frame_index]
            pixels = frame_pixels(frame, remap)
            offset_x = frame['x']
            if direction > 16:
                pixels = b''.join(pixels[row * frame['frame_width']:(row + 1) * frame['frame_width']][::-1]
                                  for row in range(frame['frame_height']))
                offset_x = frame['width'] - frame['x'] - frame['frame_width']
            phases = pack_word_phases(frame['frame_width'], frame['frame_height'], pixels)
            phase_symbols = []
            for phase, words in enumerate(phases):
                phase_symbol = f'{symbol}_word_{direction}_p{phase}'
                phase_symbols.append(phase_symbol)
                lines.append(f'static const u32 {phase_symbol}[{len(words)}] = {{')
                for i in range(0, len(words), 8):
                    lines.append('    ' + ','.join(f'0x{word:08x}u' for word in words[i:i + 8]) + ',')
                lines.append('};')
            word_symbol = f'{symbol}_word_{direction}'
            lines.append(f'static const GbaSpriteWordFrame {word_symbol} = '
                         f'{{{frame["frame_width"]},{frame["frame_height"]},{{{",".join(phase_symbols)}}}}};')
            word_frame_symbols.append((word_symbol, frame['x'], frame['y'], offset_x,
                                       frame['frame_width'], frame['frame_height'],
                                       frame['width'], frame['height']))
        lines.append('#endif')
        word_cases.append((ref['unit_id'], symbol, word_frame_symbols))
        manifest.append({'name': name, 'unit_id': ref['unit_id'],
                         'image_id': image_id, 'directional': directional,
                         'frames': len(grp['frames']),
                         'sha1': hashlib.sha1(grp_data).hexdigest()})

    lines += ['/* The 17 source-authored directional frames for each unit. */',
              'static inline const Sc64SpriteFrame *sc64_unit_facing_frames(unsigned short raw_type) {',
              '    switch (raw_type) {', *cases, '    default: return 0;', '    }', '}',
              f'#define SC64_UNIT_FACING_PIXEL_BYTES {total}u']
    lines += ['#ifdef SC64_UNIT_WORD_SPRITES',
              '#define SC64_UNIT_FACING_WORD_METADATA 1',
              'typedef struct { const GbaSpriteWordFrame *sprite; short offset_x, offset_y; '
              'unsigned short canvas_w, canvas_h; } Sc64UnitFacingWordFrame;',
              'static inline const Sc64UnitFacingWordFrame *'
              'sc64_unit_facing_word_frame(unsigned short raw_type, unsigned char direction) {',
              '    switch (raw_type) {']
    for raw_type, symbol, frames in word_cases:
        lines += [f'    case {raw_type}:', '        switch (direction & 31) {']
        for direction, (word_symbol, _ox, oy, offset_x, width, height, canvas_w, canvas_h) in enumerate(frames):
            # The frame descriptors have constant geometry and remain in ROM.
            meta_symbol = f'{symbol}_word_meta_{direction}'
            lines.append(f'        case {direction}: {{ static const Sc64UnitFacingWordFrame m = '
                         f'{{&{word_symbol},{offset_x},{oy},{canvas_w},{canvas_h}}}; return &m; }}')
        lines += ['        default: return 0;', '        }']
    lines += ['    default: return 0;', '    }', '}', '#endif /* SC64_UNIT_WORD_SPRITES */', '#endif', '']
    out.mkdir(parents=True, exist_ok=True)
    (out / 'sc64_unit_facing_generated.h').write_text('\n'.join(lines))
    summary = {'rom_sha1': sha1, 'units': len(cases), 'pixel_bytes': total,
               'direction_metadata': 'images.dat has_directional_frames',
               'entries': manifest}
    (out / 'manifest.json').write_text(json.dumps(summary, indent=2) + '\n')
    return summary


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('--rom', type=Path, default=races.ROM)
    parser.add_argument('--remap', type=Path, default=races.PALETTE_REMAP)
    parser.add_argument('--out', type=Path, default=OUT)
    args = parser.parse_args()
    summary = generate(args.rom, args.remap, args.out)
    print(f'{summary["units"]} directional units, {summary["pixel_bytes"]} pixel bytes')
    for entry in summary['entries']:
        if 'skipped' in entry:
            print(f'  skipped {entry["name"]} raw{entry["unit_id"]}: {entry["skipped"]}')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
