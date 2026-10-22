#!/usr/bin/env python3
"""Convert local StarCraft 64 Badlands terrain into private GBA asset tables.

BOLT extraction is delegated to the pinned sc64-maps project. The table
interpretation is based on the CV5/VX4/VF4 formats documented by OpenBW and
validated against this USA ROM's index ranges. No game bytes live in this file.
"""
from __future__ import annotations
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
USA_SHA1 = 'a36d91b5aac7bb95df0d7de5f9814f5270e25c2b'
MISSION_ENTRIES = {'wasteland': '008/00A', 'backwater': '008/00B'}


def sections(data: bytes) -> dict[bytes, bytes]:
    result = {}
    pos = 0
    while pos < len(data):
        if len(data) - pos < 8:
            raise ValueError('Truncated CHK section header')
        tag, size = struct.unpack_from('<4sI', data, pos)
        pos += 8
        if size > len(data) - pos:
            raise ValueError(f'Truncated CHK section {tag!r}')
        result[tag] = data[pos:pos + size]
        pos += size
    return result


def decode_tile(tile_id: int, cv5: bytes, vx4: bytes, vf4: bytes, vr4: bytes) -> tuple[bytes, int]:
    """CV5 selects a megatile; VX4 selects/flips sixteen 4×4 N64 minitiles."""
    group, variant = tile_id >> 4, tile_id & 15
    offset = group * 52 + 20 + variant * 2
    if offset + 2 > len(cv5):
        raise ValueError(f'Tile {tile_id}: CV5 group outside archive')
    mega = struct.unpack_from('<H', cv5, offset)[0]
    if (mega + 1) * 32 > min(len(vx4), len(vf4)):
        raise ValueError(f'Tile {tile_id}: megatile outside VX4/VF4')
    refs = struct.unpack_from('<16H', vx4, mega * 32)
    flags = struct.unpack_from('<16H', vf4, mega * 32)
    pixels = bytearray(256)
    walk_mask = 0
    for mini, ref in enumerate(refs):
        image, flip = ref >> 1, ref & 1
        if (image + 1) * 16 > len(vr4):
            raise ValueError(f'Tile {tile_id}: minitile outside N64 VR4')
        left, top = (mini % 4) * 4, (mini // 4) * 4
        if flags[mini] & 1:
            walk_mask |= 1 << mini
        for y in range(4):
            for x in range(4):
                pixels[(top + y) * 16 + left + x] = vr4[image * 16 + y * 4 + (3 - x if flip else x)]
    return bytes(pixels), walk_mask


def palette_rgb555(data: bytes) -> list[int]:
    if len(data) != 512:
        raise ValueError('Expected the 256-word N64 terrain palette')
    return [((v >> 11) & 31) | (((v >> 6) & 31) << 5) | (((v >> 1) & 31) << 10)
            for v in struct.unpack('>256H', data)]


def compact_palette(palette: list[int], weighted_indices: Counter) -> tuple[list[int], bytes]:
    frequency = Counter()
    for idx, count in weighted_indices.items():
        frequency[palette[idx]] += count
    chosen = [color for color, _ in sorted(frequency.items(), key=lambda item: (-item[1], item[0]))[:224]]
    if not chosen:
        raise ValueError('No terrain pixels to convert')
    colors = chosen + [0] * (224 - len(chosen))
    remap = []
    for color in palette:
        def distance(candidate):
            return sum((((color >> shift) & 31) - ((candidate >> shift) & 31)) ** 2 for shift in (0, 5, 10))
        remap.append(32 + min(range(len(chosen)), key=lambda i: distance(chosen[i])))
    return [0] * 32 + colors, bytes(remap)


def c_array(name: str, ctype: str, values, width: int = 24) -> str:
    values = list(values)
    rows = [','.join(str(v) for v in values[i:i + width]) for i in range(0, len(values), width)]
    return f'static const {ctype} {name}[{len(values)}] = {{\n' + ',\n'.join(rows) + '\n};\n'


def convert(rom_path: Path, entry_path: str, out: Path, reference: Path) -> dict:
    sys.path.insert(0, str(reference))
    from extract_sc64_maps import BoltArchive, load_rom
    rom = load_rom(str(rom_path))
    if hashlib.sha1(rom).hexdigest() != USA_SHA1:
        raise ValueError('This converter requires the verified StarCraft 64 USA ROM')
    archive = BoltArchive(rom)
    entries = {e.path: e for e in archive.entries()}
    def read(path):
        return archive.read(entries[path])
    chk = sections(read(entry_path))
    width, height = struct.unpack('<HH', chk[b'DIM '])
    era = struct.unpack('<H', chk[b'ERA '])[0] & 7
    if era != 0:
        raise ValueError('Only the verified Badlands tileset is supported in this first converter')
    if not (1 <= width <= 64 and 1 <= height <= 64) or len(chk[b'MTXM']) != width * height * 2:
        raise ValueError('Map dimensions or terrain length exceed this GBA mission profile')
    ids = struct.unpack(f'<{width * height}H', chk[b'MTXM'])
    source = {label: read(f'00B/{index:03X}') for label, index in
              [('vr4', 0), ('vx4', 1), ('vf4', 2), ('cv5', 3), ('palette', 4)]}
    decoded = {tile: decode_tile(tile, source['cv5'], source['vx4'], source['vf4'], source['vr4']) for tile in set(ids)}
    weighted = Counter()
    for tile, count in Counter(ids).items():
        for idx, pixels in Counter(decoded[tile][0]).items():
            weighted[idx] += pixels * count
    palette, remap = compact_palette(palette_rgb555(source['palette']), weighted)
    tiles, lookup, masks, blocked = [], [], [], []
    dedup = {}
    for tile in ids:
        pixels, walk = decoded[tile]
        packed = pixels.translate(remap)
        if packed not in dedup:
            dedup[packed] = len(tiles)
            tiles.append(packed)
        lookup.append(dedup[packed])
        masks.append(walk)
        # The existing GBA pathfinder operates at a whole 16px tile. Require
        # its central 8×8px footprint to be walkable, preserving VF4 evidence.
        blocked.append(int((walk & 0x0660) != 0x0660))
    out.mkdir(parents=True, exist_ok=True)
    header = ['/* Generated privately from the supplied ROM. Do not publish. */\n#pragma once\n#include "gba.h"\n',
              f'#define SC64_HAS_TERRAIN_ART 1\n#define SC64_TILE_COUNT {len(tiles)}\n',
              c_array('sc64_palette', 'u16', palette),
              c_array('sc64_tile_lookup', 'u16', lookup),
              c_array('sc64_walk_masks', 'u16', masks),
              c_array('sc64_terrain_blocked', 'u8', blocked),
              f'static const u8 sc64_tile_pixels[SC64_TILE_COUNT][256] __attribute__((aligned(4))) = {{\n']
    header += ['{' + ','.join(map(str, tile)) + '},\n' for tile in tiles]
    header.append('};\n')
    (out / 'sc64_assets_generated.h').write_text(''.join(header))
    (out / 'palette-remap.bin').write_bytes(remap)
    (out / 'palette.json').write_text(json.dumps({'gba_rgb555': palette, 'source_rgb555': palette_rgb555(source['palette'])}, indent=2) + '\n')
    rgb = bytearray(width * 16 * height * 16 * 3)
    for cell, tile in enumerate(lookup):
        tx, ty = cell % width, cell // width
        for y in range(16):
            for x in range(16):
                color = palette[tiles[tile][y * 16 + x]]
                dest = ((ty * 16 + y) * width * 16 + tx * 16 + x) * 3
                rgb[dest:dest + 3] = bytes(((color & 31) * 255 // 31, ((color >> 5) & 31) * 255 // 31, ((color >> 10) & 31) * 255 // 31))
    from emulator_smoke import png
    png(out / 'terrain-preview.png', rgb, width * 16, height * 16)
    report = {'schema': 1, 'rom_sha1': USA_SHA1, 'entry': entry_path,
              'width': width, 'height': height, 'tileset': 'Badlands',
              'unique_source_tiles': len(decoded), 'unique_gba_tiles': len(tiles),
              'terrain_pixel_bytes': len(tiles) * 256, 'palette_reserved_indices': [0, 31],
              'blocked_cells': sum(blocked), 'cells': len(ids),
              'collision_policy': 'VF4 bit0, central four minitiles required; coarse GBA approximation',
              'sources': {key: hashlib.sha256(data).hexdigest() for key, data in source.items()},
              'generated_header_sha256': hashlib.sha256((out / 'sc64_assets_generated.h').read_bytes()).hexdigest()}
    (out / 'asset-report.json').write_text(json.dumps(report, indent=2) + '\n')
    return report


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--rom', type=Path, required=True)
    p.add_argument('--entry')
    p.add_argument('--profile', choices=MISSION_ENTRIES, default='wasteland')
    p.add_argument('--out', type=Path)
    p.add_argument('--reference', type=Path, default=ROOT / 'references/sc64-maps')
    args = p.parse_args()
    entry = args.entry or MISSION_ENTRIES[args.profile]
    out = args.out or (ROOT / '.local/sc64/backwater/generated' if args.profile == 'backwater'
                       else ROOT / '.local/sc64/generated')
    try:
        print(json.dumps(convert(args.rom, entry, out, args.reference), indent=2))
    except (ValueError, OSError, KeyError) as error:
        raise SystemExit(str(error))
