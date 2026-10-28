#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Pack private N64 briefing text, portraits and interface art for the GBA.

The ROM and the pinned reference extractor remain local. Generated output is
written below .local/sc64/briefings/generated and must not be published.
"""
from __future__ import annotations

import argparse
from collections import Counter
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
ROM_SHA1 = 'a36d91b5aac7bb95df0d7de5f9814f5270e25c2b'
REFERENCE_COMMIT = '09b57f787a247107d1de82b8c5eab2734667ea1d'
WIDTH, HEIGHT = 240, 160
PORTRAIT_WIDTH, PORTRAIT_HEIGHT = 40, 38
PORTRAIT_COUNT = 23
PALETTE_FIRST, PALETTE_SIZE = 32, 224
TEXT_COLUMNS, TEXT_ROWS = 36, 7

# The source BOLT order is unrelated to catalog global IDs. Each value is the
# hexadecimal index of the briefing in 007; 007/i pairs with map 008/(i+8).
MISSION_BRIEFING_INDEX = (
    0x00C, 0x00D, 0x016, 0x017, 0x002, 0x003, 0x004, 0x005, 0x006, 0x007,
    0x008, 0x009, 0x00A, 0x00B, 0x00E, 0x00F, 0x010, 0x011, 0x012, 0x013,
    0x014, 0x015, 0x018, 0x019, 0x01A, 0x01B, 0x01C, 0x01D, 0x01E, 0x01F,
)

ROOM_ASSET = '006/02A'
PORTRAIT_FRAME_ASSET = '006/032'
DIALOGUE_FRAME_ASSET = '006/03B'
BAR_FRAME_ASSET = '006/02F'


def crop_rgb(rgb: bytes, width: int, height: int,
             crop: tuple[int, int, int, int]) -> bytes:
    x, y, crop_width, crop_height = crop
    if width <= 0 or height <= 0 or len(rgb) != width * height * 3:
        raise ValueError('RGB source dimensions do not match its byte length')
    if x < 0 or y < 0 or crop_width <= 0 or crop_height <= 0:
        raise ValueError('Crop origin and dimensions must be positive')
    if x + crop_width > width or y + crop_height > height:
        raise ValueError('RGB crop exceeds source bounds')
    out = bytearray(crop_width * crop_height * 3)
    for row in range(crop_height):
        start = ((y + row) * width + x) * 3
        end = start + crop_width * 3
        out[row * crop_width * 3:(row + 1) * crop_width * 3] = rgb[start:end]
    return bytes(out)


def retile_bevel(rgb: bytes, source_size: tuple[int, int], target_size: tuple[int, int],
                 crops: tuple[tuple[int, int, int, int], ...],
                 rails: tuple[int, int, int, int], frame_key=(0, 255, 0)) -> bytes:
    """Retile original top/bottom/side bezel strips around a text-safe pane."""
    if len(crops) != 4 or len(rails) != 4:
        raise ValueError('Nine-slice frame needs four source crops and four rail widths')
    width, height = target_size
    top_h, bottom_h, left_w, right_w = rails
    if (width <= 0 or height <= 0 or min(rails) <= 0
            or top_h + bottom_h >= height or left_w + right_w >= width):
        raise ValueError('Nine-slice target dimensions cannot fit the requested rails')
    sw, sh = source_size
    if len(rgb) != sw * sh * 3:
        raise ValueError('Nine-slice source dimensions do not match its byte length')
    top = crop_rgb(rgb, sw, sh, crops[0])
    bottom = crop_rgb(rgb, sw, sh, crops[1])
    left = crop_rgb(rgb, sw, sh, crops[2])
    right = crop_rgb(rgb, sw, sh, crops[3])
    top = stretch_nearest(top, crops[0][2], crops[0][3], width, top_h)
    bottom = stretch_nearest(bottom, crops[1][2], crops[1][3], width, bottom_h)
    side_h = height - top_h - bottom_h
    left = stretch_nearest(left, crops[2][2], crops[2][3], left_w, side_h)
    right = stretch_nearest(right, crops[3][2], crops[3][3], right_w, side_h)
    out = bytearray(bytes(frame_key) * (width * height))
    out[0:width * top_h * 3] = top
    bottom_y = height - bottom_h
    out[bottom_y * width * 3:] = bottom
    side_y = top_h
    for y in range(side_h):
        dst = ((side_y + y) * width) * 3
        out[dst:dst + left_w * 3] = left[y * left_w * 3:(y + 1) * left_w * 3]
        right_x = width - right_w
        out[dst + right_x * 3:dst + width * 3] = right[y * right_w * 3:(y + 1) * right_w * 3]
    return bytes(out)


def resize_crop(rgb: bytes, width: int, height: int,
                target_width: int, target_height: int) -> bytes:
    """Center-crop to target aspect and bilinearly scale an RGB image."""
    if width <= 0 or height <= 0 or len(rgb) != width * height * 3:
        raise ValueError('RGB source dimensions do not match its byte length')
    target_ratio = target_width / target_height
    source_ratio = width / height
    if source_ratio > target_ratio:
        crop_w, crop_h = round(height * target_ratio), height
        left, top = (width - crop_w) // 2, 0
    else:
        crop_w, crop_h = width, round(width / target_ratio)
        left, top = 0, (height - crop_h) // 2
    out = bytearray(target_width * target_height * 3)
    for y in range(target_height):
        sy = top + (y + 0.5) * crop_h / target_height - 0.5
        y0 = max(0, min(height - 1, int(sy)))
        y1 = min(height - 1, y0 + 1)
        fy = max(0.0, min(1.0, sy - y0))
        for x in range(target_width):
            sx = left + (x + 0.5) * crop_w / target_width - 0.5
            x0 = max(0, min(width - 1, int(sx)))
            x1 = min(width - 1, x0 + 1)
            fx = max(0.0, min(1.0, sx - x0))
            dst = (y * target_width + x) * 3
            for c in range(3):
                a = rgb[(y0 * width + x0) * 3 + c] * (1 - fx) + rgb[(y0 * width + x1) * 3 + c] * fx
                b = rgb[(y1 * width + x0) * 3 + c] * (1 - fx) + rgb[(y1 * width + x1) * 3 + c] * fx
                out[dst + c] = round(a * (1 - fy) + b * fy)
    return bytes(out)


def resize_nearest(rgb: bytes, width: int, height: int,
                   target_width: int, target_height: int) -> bytes:
    """Center-crop then nearest-neighbor scale, retaining pixel-art edges."""
    if width <= 0 or height <= 0 or len(rgb) != width * height * 3:
        raise ValueError('RGB source dimensions do not match its byte length')
    target_ratio = target_width / target_height
    if width / height > target_ratio:
        crop_w, crop_h = round(height * target_ratio), height
        left, top = (width - crop_w) // 2, 0
    else:
        crop_w, crop_h = width, round(width / target_ratio)
        left, top = 0, (height - crop_h) // 2
    out = bytearray(target_width * target_height * 3)
    for y in range(target_height):
        sy = top + min(crop_h - 1, int((y + 0.5) * crop_h / target_height))
        for x in range(target_width):
            sx = left + min(crop_w - 1, int((x + 0.5) * crop_w / target_width))
            src = (sy * width + sx) * 3
            dst = (y * target_width + x) * 3
            out[dst:dst + 3] = rgb[src:src + 3]
    return bytes(out)


def stretch_nearest(rgb: bytes, width: int, height: int,
                    target_width: int, target_height: int) -> bytes:
    """Scale a keyed interface piece to its panel rectangle without cropping."""
    if width <= 0 or height <= 0 or len(rgb) != width * height * 3:
        raise ValueError('RGB source dimensions do not match its byte length')
    out = bytearray(target_width * target_height * 3)
    for y in range(target_height):
        sy = min(height - 1, int((y + 0.5) * height / target_height))
        for x in range(target_width):
            sx = min(width - 1, int((x + 0.5) * width / target_width))
            src = (sy * width + sx) * 3
            dst = (y * target_width + x) * 3
            out[dst:dst + 3] = rgb[src:src + 3]
    return bytes(out)


def rgba_to_rgb(rgba: bytes) -> bytes:
    if len(rgba) % 4:
        raise ValueError('RGBA data length must be divisible by four')
    out = bytearray(len(rgba) // 4 * 3)
    for src in range(0, len(rgba), 4):
        r, g, b, a = rgba[src:src + 4]
        dst = src // 4 * 3
        out[dst:dst + 3] = bytes((r * a // 255, g * a // 255, b * a // 255))
    return bytes(out)


def overlay_keyed(background: bytearray, frame: bytes, frame_size: tuple[int, int],
                  rect: tuple[int, int, int, int], frame_key=(0, 255, 0),
                  panel_color=(41, 82, 41), panel_mix=48) -> None:
    x0, y0, target_width, target_height = rect
    if x0 < 0 or y0 < 0 or x0 + target_width > WIDTH or y0 + target_height > HEIGHT:
        raise ValueError('Interface frame rectangle falls outside the 240x160 screen')
    overlay = stretch_nearest(frame, *frame_size, target_width, target_height)
    key, panel = bytes(frame_key), bytes(panel_color)
    for y in range(target_height):
        for x in range(target_width):
            src = (y * target_width + x) * 3
            color = overlay[src:src + 3]
            if color == key:
                continue
            dst = ((y0 + y) * WIDTH + x0 + x) * 3
            if color == panel:
                background[dst] = (color[0] * panel_mix + background[dst] * (255 - panel_mix)) // 255
                background[dst + 1] = (color[1] * panel_mix + background[dst + 1] * (255 - panel_mix)) // 255
                background[dst + 2] = (color[2] * panel_mix + background[dst + 2] * (255 - panel_mix)) // 255
            else:
                background[dst:dst + 3] = color


def compose_room(room: bytes, room_size: tuple[int, int],
                 layers: list[tuple[bytes, tuple[int, int], tuple[int, int, int, int]]
                               | tuple[bytes, tuple[int, int], tuple[int, int, int, int], int]]) -> bytes:
    """Place original green-keyed UI parts over the industrial briefing room."""
    background = bytearray(resize_crop(room, *room_size, WIDTH, HEIGHT))
    # Bake a smoked, near-black glass treatment into the body rectangle. Keep
    # enough room detail visible underneath to match the transmission screen.
    for y in range(69, 130):
        for x in range(12, 228):
            i = (y * WIDTH + x) * 3
            background[i] = background[i] * 96 // 255
            background[i + 1] = background[i + 1] * 96 // 255
            background[i + 2] = background[i + 2] * 96 // 255
    for layer in layers:
        frame, frame_size, rect = layer[:3]
        panel_mix = layer[3] if len(layer) > 3 else 48
        overlay_keyed(background, frame, frame_size, rect, panel_mix=panel_mix)
    return bytes(background)


def resize_portrait(rgba: bytes, width: int, height: int,
                    target_width: int = PORTRAIT_WIDTH,
                    target_height: int = PORTRAIT_HEIGHT) -> tuple[bytes, bytes]:
    """Nearest-neighbor portrait resize returning RGB pixels and alpha mask."""
    if len(rgba) != width * height * 4 or width <= 0 or height <= 0:
        raise ValueError('Portrait RGBA dimensions do not match its byte length')
    rgb = bytearray(target_width * target_height * 3)
    mask = bytearray(target_width * target_height)
    for y in range(target_height):
        sy = min(height - 1, int((y + 0.5) * height / target_height))
        for x in range(target_width):
            sx = min(width - 1, int((x + 0.5) * width / target_width))
            si = (sy * width + sx) * 4
            di = y * target_width + x
            rgb[di * 3:di * 3 + 3] = rgba[si:si + 3]
            mask[di] = 1 if rgba[si + 3] else 0
    return bytes(rgb), bytes(mask)


def median_palette(images: list[bytes], count: int) -> list[tuple[int, int, int]]:
    hist = Counter(tuple(image[i:i + 3]) for image in images for i in range(0, len(image), 3))
    boxes = [list(hist.items())]
    while len(boxes) < count:
        candidates = []
        for bi, box in enumerate(boxes):
            if len(box) < 2:
                continue
            ranges = [max(c[k] for c, _ in box) - min(c[k] for c, _ in box) for k in range(3)]
            channel = max(range(3), key=lambda k: ranges[k])
            candidates.append((ranges[channel] * sum(n for _, n in box), bi, channel))
        if not candidates:
            break
        _, bi, channel = max(candidates)
        box = sorted(boxes.pop(bi), key=lambda item: item[0][channel])
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
    unique = list(dict.fromkeys(tuple(rgb[i:i + 3]) for i in range(0, len(rgb), 3)))
    lut = {
        color: min(range(len(colors)), key=lambda j: sum((color[k] - colors[j][k]) ** 2 for k in range(3)))
        + PALETTE_FIRST
        for color in unique
    }
    return bytes(lut[tuple(rgb[i:i + 3])] for i in range(0, len(rgb), 3))


def nearest_masked(rgb: bytes, mask: bytes, colors: list[tuple[int, int, int]]) -> bytes:
    if len(rgb) != len(mask) * 3:
        raise ValueError('Portrait pixels and alpha mask have different sizes')
    visible = list(dict.fromkeys(tuple(rgb[i * 3:i * 3 + 3])
                                 for i, is_visible in enumerate(mask) if is_visible))
    lut = {
        color: min(range(len(colors)), key=lambda j: sum((color[k] - colors[j][k]) ** 2 for k in range(3)))
        + PALETTE_FIRST
        for color in visible
    }
    return bytes(lut[tuple(rgb[i * 3:i * 3 + 3])] if mask[i] else 0 for i in range(len(mask)))


def wrap_text(text: str, columns: int = TEXT_COLUMNS, rows: int = TEXT_ROWS) -> list[str]:
    """Wrap whitespace-normalized source copy into bounded page strings."""
    if any(not 0x20 <= ord(char) <= 0x7e for char in text):
        raise ValueError('Source text contains characters outside printable ASCII')
    words = text.split()
    if any(len(word) > columns for word in words):
        raise ValueError('A source word exceeds the configured line width')
    if not words:
        return ['']
    lines: list[str] = []
    current = ''
    for word in words:
        candidate = word if not current else current + ' ' + word
        if len(candidate) > columns:
            lines.append(current)
            current = word
        else:
            current = candidate
    if current:
        lines.append(current)
    return ['\n'.join(lines[i:i + rows]) for i in range(0, len(lines), rows)]


def briefing_pages(briefing) -> list[tuple[str, str, int]]:
    pages: list[tuple[str, str, int]] = []
    for transmission in briefing.transmissions:
        if transmission.kind == 'TEXTC':
            continue  # Fixed "End of Briefing" UI copy, not spoken dialogue.
        body = ' '.join(line.strip() for line in transmission.body if line.strip())
        for text in wrap_text(body):
            portrait = transmission.portrait if transmission.portrait is not None else -1
            pages.append((transmission.speaker, text, portrait))
    objective_lines = []
    for objective in briefing.objectives:
        for wrapped in wrap_text(objective, rows=255):
            objective_lines.extend(wrapped.split('\n'))
    for start in range(0, len(objective_lines), TEXT_ROWS):
        pages.append(('OBJECTIVES', '\n'.join(objective_lines[start:start + TEXT_ROWS]), -1))
    if not pages:
        raise ValueError(f'{briefing.bolt_path} has no renderable briefing pages')
    return pages


def c_string(value: str) -> str:
    if any(char != '\n' and not 0x20 <= ord(char) <= 0x7e for char in value):
        raise ValueError('C output text contains characters outside printable ASCII')
    return '"' + value.replace('\\', '\\\\').replace('"', '\\"').replace('\n', '\\n') + '"'


def c_array(name: str, ctype: str, values, width: int = 24) -> str:
    values = list(values)
    rows = [','.join(str(v) for v in values[i:i + width]) for i in range(0, len(values), width)]
    return f'static const {ctype} {name}[{len(values)}] = {{\n' + ',\n'.join(rows) + '\n};\n'


def indexed_array(name: str, pixels: bytes, width: int, height: int) -> str:
    if len(pixels) != width * height:
        raise ValueError(f'{name}: expected {width * height} pixels, got {len(pixels)}')
    values = list(pixels)
    rows = [','.join(str(v) for v in values[i:i + 24]) for i in range(0, len(values), 24)]
    return (f'static const u8 {name}[{len(values)}] __attribute__((aligned(4))) = {{\n'
            + ',\n'.join(rows) + '\n};\n')


def build_header(background: bytes, portraits: list[bytes], palette: list[int],
                 pages_by_mission: list[list[tuple[str, str, int]]]) -> str:
    if len(background) != WIDTH * HEIGHT:
        raise ValueError('Briefing background must be 240x160')
    if len(portraits) != PORTRAIT_COUNT or any(len(p) != PORTRAIT_WIDTH * PORTRAIT_HEIGHT for p in portraits):
        raise ValueError('Expected 23 portraits, each 40x38')
    if len(palette) != PALETTE_SIZE or any(not 0 <= c <= 0x7fff for c in palette):
        raise ValueError('Expected 224 RGB555 colors')
    if len(pages_by_mission) != len(MISSION_BRIEFING_INDEX):
        raise ValueError('Expected one briefing for every base global mission ID')
    if any(not PALETTE_FIRST <= c <= 255 for c in background):
        raise ValueError('Background pixels must use palette indices 32..255')
    if any(c != 0 and not PALETTE_FIRST <= c <= 255 for p in portraits for c in p):
        raise ValueError('Portrait pixels must use transparent 0 or palette indices 32..255')
    lines = [
        '/* Generated privately from the supplied ROM. Do not publish. */\n',
        f'/* Artwork decoded with references/sc64-maps at commit {REFERENCE_COMMIT}. */\n',
        '#pragma once\n#include "gba.h"\n#include "sc64_briefing.h"\n',
        f'#define SC64_BRIEFING_BACKGROUND_WIDTH {WIDTH}\n',
        f'#define SC64_BRIEFING_BACKGROUND_HEIGHT {HEIGHT}\n',
        f'#define SC64_BRIEFING_PALETTE_FIRST {PALETTE_FIRST}\n',
        f'#define SC64_BRIEFING_PALETTE_COUNT {PALETTE_SIZE}\n',
        f'#define SC64_BRIEFING_PORTRAIT_WIDTH {PORTRAIT_WIDTH}\n',
        f'#define SC64_BRIEFING_PORTRAIT_HEIGHT {PORTRAIT_HEIGHT}\n',
        f'#define SC64_BRIEFING_PORTRAIT_COUNT {PORTRAIT_COUNT}\n',
        c_array('sc64_briefing_palette', 'u16', palette),
        indexed_array('sc64_briefing_background', background, WIDTH, HEIGHT),
        'static const u8 sc64_briefing_portraits[SC64_BRIEFING_PORTRAIT_COUNT]'
        '[SC64_BRIEFING_PORTRAIT_WIDTH * SC64_BRIEFING_PORTRAIT_HEIGHT] '
        '__attribute__((aligned(4))) = {\n',
    ]
    for portrait in portraits:
        lines.append('    {\n')
        lines.append(',\n'.join(','.join(str(v) for v in portrait[i:i + 24])
                                 for i in range(0, len(portrait), 24)))
        lines.append('\n    },\n')
    lines.append('};\n')
    for mission, pages in enumerate(pages_by_mission):
        if not pages:
            raise ValueError(f'Mission {mission} has no briefing pages')
        if len(pages) > 255:
            raise ValueError(f'Mission {mission} exceeds the runtime page-count limit')
        for speaker, text, portrait in pages:
            if portrait < -1 or portrait >= PORTRAIT_COUNT:
                raise ValueError(f'Mission {mission} has invalid portrait ID {portrait}')
            if len(text.split('\n')) > TEXT_ROWS or any(len(line) > TEXT_COLUMNS for line in text.split('\n')):
                raise ValueError(f'Mission {mission} page is not wrapped to 36x7')
        lines.append(f'static const Sc64BriefingPage sc64_briefing_pages_{mission}[] = {{\n')
        lines.extend(f'    {{{c_string(speaker)}, {c_string(text)}, {portrait}}},\n'
                     for speaker, text, portrait in pages)
        lines.append('};\n')
    lines.append(f'static const Sc64Briefing sc64_briefings[{len(pages_by_mission)}] = {{\n')
    lines.extend(f'    {{sc64_briefing_pages_{mission}, '
                 f'(unsigned)(sizeof(sc64_briefing_pages_{mission}) / sizeof(sc64_briefing_pages_{mission}[0]))}},\n'
                 for mission in range(len(pages_by_mission)))
    lines.append('};\n')
    lines.append('static inline const Sc64Briefing *sc64_briefing_get(int global_mission) {\n'
                 '    return global_mission >= 0 && global_mission < '
                 f'{len(pages_by_mission)} ? &sc64_briefings[global_mission] : 0;\n'
                 '}\n')
    return ''.join(lines)


def ppm_bytes(rgb: bytes, width: int, height: int) -> bytes:
    if len(rgb) != width * height * 3:
        raise ValueError('PPM pixel dimensions do not match byte length')
    return f'P6\n{width} {height}\n255\n'.encode('ascii') + rgb


def preview_rgb(indexed: bytes, colors: list[tuple[int, int, int]]) -> bytes:
    out = bytearray(len(indexed) * 3)
    for i, index in enumerate(indexed):
        if index < PALETTE_FIRST or index > 255:
            raise ValueError(f'Preview index {index} is outside the image palette')
        out[i * 3:i * 3 + 3] = bytes(colors[index - PALETTE_FIRST])
    return bytes(out)


def verify_inputs(rom_path: Path, reference: Path) -> tuple[bytes, str]:
    try:
        reference_commit = subprocess.check_output(
            ['git', '-C', str(reference), 'rev-parse', 'HEAD'], text=True).strip()
    except (OSError, subprocess.CalledProcessError) as error:
        raise ValueError('Could not verify the pinned references/sc64-maps commit') from error
    if reference_commit != REFERENCE_COMMIT:
        raise ValueError(f'references/sc64-maps must stay at pinned commit {REFERENCE_COMMIT}')
    sys.path.insert(0, str(reference))
    from extract_sc64_maps import load_rom
    rom = load_rom(str(rom_path))
    rom_sha1 = hashlib.sha1(rom).hexdigest()
    if rom_sha1 != ROM_SHA1:
        raise ValueError('Expected the verified StarCraft 64 USA ROM')
    return rom, rom_sha1


def convert(rom_path: Path, out: Path, reference: Path) -> dict:
    rom, rom_sha1 = verify_inputs(rom_path, reference)
    sys.path.insert(0, str(reference))
    from extract_sc64_maps import BoltArchive
    import decode_art
    import extract_briefings

    archive = BoltArchive(rom)
    raw_images, raw_palettes = decode_art.collect(archive)
    pairing = decode_art.pair_palettes(raw_images, raw_palettes)
    frame_assets = (PORTRAIT_FRAME_ASSET, DIALOGUE_FRAME_ASSET, BAR_FRAME_ASSET)
    for path in (ROOM_ASSET, *frame_assets):
        if path not in raw_images or not pairing.get(path):
            raise ValueError(f'Missing paired briefing art {path}')
    # Decode explicitly after dimensions; this keeps the unpacked dimensions in
    # the report and uses the pinned decoder's verified 8bpp/palette handling.
    room_w, room_h, room_rgba = decode_art.decode_image(
        raw_images[ROOM_ASSET], decode_art.decode_palette(raw_palettes[pairing[ROOM_ASSET]]))
    room = rgba_to_rgb(room_rgba)
    frame_pixels = {}
    frame_sizes = {}
    for path in frame_assets:
        fw, fh, frame_rgba = decode_art.decode_image(
            raw_images[path], decode_art.decode_palette(raw_palettes[pairing[path]]))
        frame_pixels[path] = rgba_to_rgb(frame_rgba)
        frame_sizes[path] = (fw, fh)
    # Strip the source's left-side connector from the wide-message frame so
    # the text-safe area begins near x=12 instead of under a hardware knob.
    portrait_crop = (62, 0, frame_sizes[PORTRAIT_FRAME_ASSET][0] - 62,
                     frame_sizes[PORTRAIT_FRAME_ASSET][1])
    portrait_frame = crop_rgb(frame_pixels[PORTRAIT_FRAME_ASSET],
                              *frame_sizes[PORTRAIT_FRAME_ASSET], portrait_crop)
    portrait_frame_size = (portrait_crop[2], portrait_crop[3])
    body_crops = ((10, 0, 480, 29), (8, frame_sizes[DIALOGUE_FRAME_ASSET][1] - 29, 500, 29),
                  (0, 29, 32, frame_sizes[DIALOGUE_FRAME_ASSET][1] - 58),
                  (480, 29, 35, frame_sizes[DIALOGUE_FRAME_ASSET][1] - 58))
    body_rails = (6, 7, 6, 6)
    body_frame = retile_bevel(frame_pixels[DIALOGUE_FRAME_ASSET],
                              frame_sizes[DIALOGUE_FRAME_ASSET], (228, 75),
                              body_crops, body_rails)
    body_frame_size = (228, 75)
    background_rgb = compose_room(room, (room_w, room_h), [
        (frame_pixels[BAR_FRAME_ASSET], frame_sizes[BAR_FRAME_ASSET], (6, 5, 228, 16), 48),
        (portrait_frame, portrait_frame_size, (8, 15, 54, 55), 48),
        (body_frame, body_frame_size, (6, 62, 228, 75), 48),
        (frame_pixels[BAR_FRAME_ASSET], frame_sizes[BAR_FRAME_ASSET], (6, 138, 228, 16), 48),
    ])

    portrait_rgb, portrait_masks = [], []
    portrait_hashes = []
    for portrait_id in range(PORTRAIT_COUNT):
        path = f'007/{0x60 + portrait_id:03X}'
        if path not in raw_images or not pairing.get(path):
            raise ValueError(f'Missing paired dialogue portrait {path}')
        pw, ph, rgba = decode_art.decode_image(
            raw_images[path], decode_art.decode_palette(raw_palettes[pairing[path]]))
        resized, mask = resize_portrait(rgba, pw, ph)
        portrait_rgb.append(resized)
        portrait_masks.append(mask)
        portrait_hashes.append({'id': portrait_id, 'image': path, 'palette': pairing[path],
                                'source_size': [pw, ph],
                                'source_image_sha256': hashlib.sha256(raw_images[path]).hexdigest()})

    briefings = extract_briefings.collect(archive)
    by_path = {briefing.bolt_path: briefing for briefing, _ in briefings}
    pages_by_mission = []
    briefing_hashes = []
    for global_id, source_index in enumerate(MISSION_BRIEFING_INDEX):
        source_path = f'007/{source_index:03X}'
        briefing = by_path.get(source_path)
        if briefing is None or briefing.is_stub:
            raise ValueError(f'Global mission {global_id} has no written source briefing at {source_path}')
        mission_pages = briefing_pages(briefing)
        if (mission_pages[0][0] == 'OBJECTIVES' or not mission_pages[0][1].strip()
                or mission_pages[-1][0] != 'OBJECTIVES'):
            raise ValueError(f'{source_path} must begin with spoken dialogue and end with objectives')
        pages_by_mission.append(mission_pages)
        briefing_hashes.append({'global_mission': global_id, 'briefing': source_path,
                                'map': briefing.map_path,
                                'objectives': len(briefing.objectives),
                                'transmissions': len(briefing.transmissions),
                                'pages': len(pages_by_mission[-1])})

    colors = median_palette([background_rgb, *portrait_rgb], PALETTE_SIZE)
    if len(colors) < PALETTE_SIZE:
        colors.extend([(0, 0, 0)] * (PALETTE_SIZE - len(colors)))
    bg_indexed = nearest(background_rgb, colors)
    portraits_indexed = [nearest_masked(rgb, mask, colors)
                         for rgb, mask in zip(portrait_rgb, portrait_masks)]
    palette = [((b >> 3) << 10) | ((g >> 3) << 5) | (r >> 3) for r, g, b in colors]
    header = build_header(bg_indexed, portraits_indexed, palette, pages_by_mission)

    out.mkdir(parents=True, exist_ok=True)
    header_path = out / 'sc64_briefing_generated.h'
    atomic_write(header_path, header.encode('ascii'))
    bg_preview = preview_rgb(bg_indexed, colors)
    (out / 'briefing-background.ppm').write_bytes(ppm_bytes(bg_preview, WIDTH, HEIGHT))
    decode_art.write_png(str(out / 'briefing-background.png'), WIDTH, HEIGHT,
                         bytes(v for i in range(WIDTH * HEIGHT)
                               for v in (*bg_preview[i * 3:i * 3 + 3], 255)))

    sheet_columns, sheet_rows = 5, 5
    tile_w, tile_h, gap = PORTRAIT_WIDTH, PORTRAIT_HEIGHT, 2
    sheet_w = sheet_columns * tile_w + (sheet_columns + 1) * gap
    sheet_h = sheet_rows * tile_h + (sheet_rows + 1) * gap
    sheet = bytearray((sheet_w * sheet_h) * 4)
    for i in range(sheet_w * sheet_h):
        sheet[i * 4:i * 4 + 4] = bytes((10, 13, 20, 255))
    for pid, (pixels, mask) in enumerate(zip(portraits_indexed, portrait_masks)):
        ox = gap + (pid % sheet_columns) * (tile_w + gap)
        oy = gap + (pid // sheet_columns) * (tile_h + gap)
        for j, index in enumerate(pixels):
            if not mask[j]:
                continue
            x, y = ox + j % tile_w, oy + j // tile_w
            dst = (y * sheet_w + x) * 4
            sheet[dst:dst + 4] = bytes((*colors[index - PALETTE_FIRST], 255))
    decode_art.write_png(str(out / 'briefing-portraits.png'), sheet_w, sheet_h, bytes(sheet))

    report = {
        'schema': 1,
        'rom_sha1': rom_sha1,
        'reference_commit': REFERENCE_COMMIT,
        'size': [WIDTH, HEIGHT],
        'palette_range': [PALETTE_FIRST, PALETTE_FIRST + PALETTE_SIZE - 1],
        'palette_count': len(palette),
        'room': {'image': ROOM_ASSET, 'palette': pairing[ROOM_ASSET],
                 'source_size': [room_w, room_h],
                 'source_image_sha256': hashlib.sha256(raw_images[ROOM_ASSET]).hexdigest()},
        'composition': {'body_rect_xywh': [12, 69, 216, 61],
                        'body_frame_xywh': [6, 62, 228, 75],
                        'body_glass': 'multiply room RGB by 96/255; original 006/03B bevel rails are nine-sliced outside text bounds',
                        'green_key_rgb': [0, 255, 0]},
        'frames': [{'image': path, 'palette': pairing[path],
                    'source_size': list(frame_sizes[path]),
                    'render_crop_xywh': list(portrait_crop) if path == PORTRAIT_FRAME_ASSET else None,
                    'render_rail_crops_xywh': [list(crop) for crop in body_crops]
                    if path == DIALOGUE_FRAME_ASSET else None,
                    'render_rail_sizes': list(body_rails) if path == DIALOGUE_FRAME_ASSET else None,
                    'source_image_sha256': hashlib.sha256(raw_images[path]).hexdigest(),
                    'chroma_key_rgb': [0, 255, 0]}
                   for path in frame_assets],
        'portraits': portrait_hashes,
        'missions': briefing_hashes,
        'generated_header_sha256': hashlib.sha256(header_path.read_bytes()).hexdigest(),
    }
    atomic_write(out / 'sc64_briefing_asset_report.json',
                 (json.dumps(report, indent=2) + '\n').encode('utf-8'))
    return report


def atomic_write(path: Path, data: bytes) -> None:
    """Publish a complete file by atomically replacing its destination."""
    temporary = path.with_name(f'.{path.name}.{os.getpid()}.tmp')
    try:
        temporary.write_bytes(data)
        temporary.replace(path)
    finally:
        if temporary.exists():
            temporary.unlink()


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rom', type=Path, default=ROOT / '.local/sc64/rom/StarCraft64-USA.z64')
    parser.add_argument('--out', type=Path, default=ROOT / '.local/sc64/briefings/generated')
    parser.add_argument('--reference', type=Path, default=ROOT / 'references/sc64-maps')
    args = parser.parse_args()
    try:
        print(json.dumps(convert(args.rom, args.out, args.reference), indent=2))
    except (ValueError, OSError, KeyError) as error:
        raise SystemExit(str(error))
