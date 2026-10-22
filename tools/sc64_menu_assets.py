#!/usr/bin/env python3
"""Convert selected StarCraft 64 interface backdrops into private GBA tables.

The source ROM and reference decoder stay local. Generated output belongs under
.local/sc64/races/generated and must never be committed or distributed.
"""
from __future__ import annotations

import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
ROM_SHA1 = 'a36d91b5aac7bb95df0d7de5f9814f5270e25c2b'
REFERENCE_COMMIT = '09b57f787a247107d1de82b8c5eab2734667ea1d'
SCENES = (
    ('home', '006/01B', None),
    ('zerg', '006/036', '009/002'),
    ('protoss', '006/038', '009/004'),
    ('terran', '006/01B', '009/000'),
)
WIDTH, HEIGHT = 240, 160
PALETTE_FIRST, PALETTE_SIZE = 32, 224


def resize_crop(rgb: bytes, width: int, height: int) -> bytes:
    """Center-crop to 3:2 and bilinearly scale to 240x160."""
    target_ratio = WIDTH / HEIGHT
    source_ratio = width / height
    if source_ratio > target_ratio:
        crop_w, crop_h = round(height * target_ratio), height
        left, top = (width - crop_w) // 2, 0
    else:
        crop_w, crop_h = width, round(width / target_ratio)
        left, top = 0, (height - crop_h) // 2
    out = bytearray(WIDTH * HEIGHT * 3)
    for y in range(HEIGHT):
        sy = top + (y + .5) * crop_h / HEIGHT - .5
        y0 = max(0, min(height - 1, int(sy)))
        y1 = min(height - 1, y0 + 1)
        fy = max(0.0, min(1.0, sy - y0))
        for x in range(WIDTH):
            sx = left + (x + .5) * crop_w / WIDTH - .5
            x0 = max(0, min(width - 1, int(sx)))
            x1 = min(width - 1, x0 + 1)
            fx = max(0.0, min(1.0, sx - x0))
            for c in range(3):
                a = rgb[(y0 * width + x0) * 3 + c] * (1 - fx) + rgb[(y0 * width + x1) * 3 + c] * fx
                b = rgb[(y1 * width + x0) * 3 + c] * (1 - fx) + rgb[(y1 * width + x1) * 3 + c] * fx
                out[(y * WIDTH + x) * 3 + c] = round(a * (1 - fy) + b * fy)
    return bytes(out)


def crop_rgb(rgb: bytes, width: int, height: int,
             crop: tuple[int, int, int, int]) -> bytes:
    """Copy an exact (x, y, width, height) RGB crop after validating bounds."""
    x, y, crop_width, crop_height = crop
    if width <= 0 or height <= 0 or len(rgb) != width * height * 3:
        raise ValueError('RGB source dimensions do not match its byte length')
    if x < 0 or y < 0 or crop_width <= 0 or crop_height <= 0:
        raise ValueError('Crop origin and dimensions must be positive')
    if x + crop_width > width or y + crop_height > height:
        raise ValueError('RGB crop exceeds source bounds')
    out = bytearray(crop_width * crop_height * 3)
    for row in range(crop_height):
        source = ((y + row) * width + x) * 3
        target = row * crop_width * 3
        out[target:target + crop_width * 3] = rgb[source:source + crop_width * 3]
    return bytes(out)


def compose_scene(backdrop: bytes, source: bytes | None,
                  source_size: tuple[int, int] | None) -> bytes:
    """Place an unwarped faction figure left; shade right for menu legibility."""
    out = bytearray(backdrop)
    if source is not None and source_size is not None:
        sw, sh = source_size
        # Character art is a 4:3 render surrounded by black. Crop tightly
        # around its central figure and drop the bottom caption band, then fit
        # the original pixels into a 112x144 window without warping faces.
        cw, ch = min(sw, round(sw * .56)), min(sh, round(sw * .60))
        left, top = (sw - cw) // 2, 0
        scale = min(112 / cw, 144 / ch)
        dw, dh = round(cw * scale), round(ch * scale)
        ox, oy = (112 - dw) // 2, 8 + (144 - dh) // 2
        for y in range(dh):
            sy = min(ch - 1, int((y + .5) * ch / dh))
            for x in range(dw):
                sx = min(cw - 1, int((x + .5) * cw / dw))
                si = ((top + sy) * sw + left + sx) * 4
                r, g, b, _ = source[si:si + 4]
                # These loading portraits have a fully opaque black matte.
                # Key the near-black matte so the chosen world scene shows
                # through; retain faint edge detail with a soft alpha ramp.
                matte = max(r, g, b)
                if matte < 7:
                    continue
                alpha = min(255, (matte - 4) * 14)
                di = ((oy + y) * WIDTH + ox + x) * 3
                out[di] = (r * alpha + out[di] * (255 - alpha)) // 255
                out[di + 1] = (g * alpha + out[di + 1] * (255 - alpha)) // 255
                out[di + 2] = (b * alpha + out[di + 2] * (255 - alpha)) // 255
    # A gradual dark glass gradient reserves the right half for the title and
    # vertically stacked menu choices while preserving the source atmosphere.
    for y in range(34, HEIGHT):
        for x in range(84, WIDTH):
            alpha = min(185, max(0, round((x - 84) * 1.2)))
            i = (y * WIDTH + x) * 3
            out[i] = out[i] * (255 - int(alpha)) // 255
            out[i + 1] = out[i + 1] * (255 - int(alpha)) // 255
            out[i + 2] = out[i + 2] * (255 - int(alpha)) // 255
    return bytes(out)


def median_palette(images: list[bytes], count: int) -> list[tuple[int, int, int]]:
    """Weighted median-cut quantization over the actual downsampled scenes."""
    hist = Counter(tuple(image[i:i + 3]) for image in images for i in range(0, len(image), 3))
    boxes = [list(hist.items())]
    while len(boxes) < count:
        candidates = []
        for bi, box in enumerate(boxes):
            if len(box) < 2:
                continue
            ranges = [max(c[k] for c, _ in box) - min(c[k] for c, _ in box) for k in range(3)]
            ch = max(range(3), key=lambda k: ranges[k])
            candidates.append((ranges[ch] * sum(n for _, n in box), bi, ch))
        if not candidates:
            break
        _, bi, ch = max(candidates)
        box = sorted(boxes.pop(bi), key=lambda item: item[0][ch])
        half, total = sum(n for _, n in box) / 2, 0
        split = 1
        for split, (_, n) in enumerate(box, 1):
            total += n
            if total >= half:
                break
        split = max(1, min(len(box) - 1, split))
        boxes.extend((box[:split], box[split:]))
    return [tuple(round(sum(c[k] * n for c, n in box) / sum(n for _, n in box)) for k in range(3))
            for box in boxes]


def nearest(rgb: bytes, colors: list[tuple[int, int, int]]) -> bytes:
    # Restrict to distinct colors first and use squared RGB distance; the menu
    # set is tiny enough that the offline converter can afford exhaustive match.
    unique = list(dict.fromkeys(tuple(rgb[i:i + 3]) for i in range(0, len(rgb), 3)))
    lut = {}
    for color in unique:
        lut[color] = min(range(len(colors)), key=lambda j: sum((color[k] - colors[j][k]) ** 2 for k in range(3))) + PALETTE_FIRST
    return bytes(lut[tuple(rgb[i:i + 3])] for i in range(0, len(rgb), 3))


def c_array(name: str, ctype: str, values, width: int = 24) -> str:
    values = list(values)
    rows = [','.join(str(v) for v in values[i:i + width]) for i in range(0, len(values), width)]
    return f'static const {ctype} {name}[{len(values)}] = {{\n' + ',\n'.join(rows) + '\n};\n'


def indexed_image(name: str, pixels: bytes, width: int = WIDTH, height: int = HEIGHT) -> str:
    if len(pixels) != width * height:
        raise ValueError(f'{name} has {len(pixels)} pixels, expected {width * height}')
    rows = [','.join(str(value) for value in pixels[i:i + 24])
            for i in range(0, len(pixels), 24)]
    return (f'static const u8 {name}[SC64_MENU_ART_WIDTH * SC64_MENU_ART_HEIGHT] '
            '__attribute__((aligned(4))) = {\n' + ',\n'.join(rows) + '\n};\n')


def build_header(indexed: list[bytes], palette: list[int]) -> str:
    if len(indexed) != 4:
        raise ValueError('Expected four indexed menu scenes')
    if len(palette) != 256:
        raise ValueError('Expected a 256-entry palette with reserved UI indices')
    if any(not 0 <= color <= 0x7FFF for color in palette[32:]):
        raise ValueError('Menu palette entries must be RGB555 values')
    if any(len(frame) != WIDTH * HEIGHT for frame in indexed):
        raise ValueError('Menu scene pixel count does not match 240x160')
    if any(value < PALETTE_FIRST or value > 255 for frame in indexed for value in frame):
        raise ValueError('Menu scene pixel indices must stay within 32..255')
    header = ['/* Generated privately from the supplied ROM. Do not publish. */\n',
              f'/* Artwork decoded with references/sc64-maps at commit {REFERENCE_COMMIT}. */\n',
              '#pragma once\n#include "gba.h"\n',
              '#define SC64_MENU_ART_WIDTH 240\n#define SC64_MENU_ART_HEIGHT 160\n#define SC64_MENU_ART_PALETTE_FIRST 32\n#define SC64_MENU_ART_PALETTE_COUNT 224\n',
              c_array('sc64_menu_palette', 'u16', palette[32:256])]
    for name, pixels in zip(('home', 'zerg', 'protoss', 'terran'), indexed):
        header.append(indexed_image(f'sc64_menu_{name}', pixels))
    return ''.join(header)


def convert(rom_path: Path, out: Path, reference: Path) -> dict:
    sys.path.insert(0, str(reference))
    from extract_sc64_maps import BoltArchive, load_rom
    import decode_art

    try:
        reference_commit = subprocess.check_output(
            ['git', '-C', str(reference), 'rev-parse', 'HEAD'], text=True).strip()
    except (OSError, subprocess.CalledProcessError) as error:
        raise ValueError('Could not verify the pinned references/sc64-maps commit') from error
    if reference_commit != REFERENCE_COMMIT:
        raise ValueError(f'references/sc64-maps must stay at pinned commit {REFERENCE_COMMIT}')

    rom = load_rom(str(rom_path))
    rom_sha1 = hashlib.sha1(rom).hexdigest()
    if rom_sha1 != ROM_SHA1:
        raise ValueError('Expected the verified StarCraft 64 USA ROM')
    archive = BoltArchive(rom)
    entries = {e.path: e for e in archive.entries()}
    raw_images, raw_palettes = decode_art.collect(archive)
    pairing = decode_art.pair_palettes(raw_images, raw_palettes)
    decoded = []
    for name, path, portrait_path in SCENES:
        if path not in raw_images or not pairing.get(path):
            raise ValueError(f'Missing paired original artwork {path}')
        w, h, rgba = decode_art.decode_image(raw_images[path], decode_art.decode_palette(raw_palettes[pairing[path]]))
        # Source scene pixels are opaque; compositing alpha against black also
        # makes this robust to menu art whose palette marks unused pixels clear.
        rgb = bytearray(w * h * 3)
        for i in range(w * h):
            r, g, b, a = rgba[i * 4:i * 4 + 4]
            rgb[i * 3:i * 3 + 3] = bytes((r * a // 255, g * a // 255, b * a // 255))
        if name == 'home':
            crop = (380, 90, WIDTH, HEIGHT)
            backdrop = crop_rgb(bytes(rgb), w, h, crop)
        else:
            crop = None
            backdrop = resize_crop(bytes(rgb), w, h)
        portrait_rgba = None
        portrait_size = None
        if portrait_path:
            pw, ph, portrait_rgba = decode_art.decode_image(raw_images[portrait_path], decode_art.decode_palette(raw_palettes[pairing[portrait_path]]))
            portrait_size = (pw, ph)
        composed = compose_scene(backdrop, portrait_rgba, portrait_size)
        decoded.append((name, path, w, h, composed, portrait_path, crop))
    colors = median_palette([item[4] for item in decoded], PALETTE_SIZE)
    if len(colors) < PALETTE_SIZE:
        colors.extend([(0, 0, 0)] * (PALETTE_SIZE - len(colors)))
    indexed = [nearest(item[4], colors) for item in decoded]
    palette = [0] * PALETTE_FIRST + [((b >> 3) << 10) | ((g >> 3) << 5) | (r >> 3) for r, g, b in colors]

    out.mkdir(parents=True, exist_ok=True)
    (out / 'sc64_menu_generated.h').write_text(build_header(indexed, palette))
    # Reuse the reference decoder's self-contained PNG writer for local previews.
    for (name, path, sw, sh, _, portrait_path, crop), frame in zip(decoded, indexed):
        rgb = bytearray(WIDTH * HEIGHT * 3)
        for i, index in enumerate(frame):
            r, g, b = colors[index - PALETTE_FIRST]
            rgb[i * 3:i * 3 + 3] = bytes((r, g, b))
        decode_art.write_png(str(out / f'{name}-preview.png'), WIDTH, HEIGHT,
                             bytes(v for i in range(WIDTH * HEIGHT) for v in (*rgb[i * 3:i * 3 + 3], 255)))
    report = {'schema': 1, 'rom_sha1': rom_sha1, 'reference_commit': reference_commit,
              'size': [WIDTH, HEIGHT],
              'palette_range': [PALETTE_FIRST, PALETTE_FIRST + PALETTE_SIZE - 1],
              'entries': [{'name': n, 'image': p, 'source_size': [w, h], 'palette': pairing[p],
                           'crop_xywh': list(crop) if crop else None,
                           'source_image_sha256': hashlib.sha256(raw_images[p]).hexdigest(),
                           'source_palette_sha256': hashlib.sha256(raw_palettes[pairing[p]]).hexdigest(),
                           'portrait': portrait_path}
                          for n, p, w, h, _, portrait_path, crop in decoded],
              'generated_header_sha256': hashlib.sha256((out / 'sc64_menu_generated.h').read_bytes()).hexdigest()}
    (out / 'sc64_menu_asset_report.json').write_text(json.dumps(report, indent=2) + '\n')
    return report


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rom', type=Path, default=ROOT / '.local/sc64/rom/StarCraft64-USA.z64')
    parser.add_argument('--out', type=Path, default=ROOT / '.local/sc64/races/generated')
    parser.add_argument('--reference', type=Path, default=ROOT / 'references/sc64-maps')
    args = parser.parse_args()
    try:
        print(json.dumps(convert(args.rom, args.out, args.reference), indent=2))
    except (ValueError, OSError, KeyError) as error:
        raise SystemExit(str(error))
