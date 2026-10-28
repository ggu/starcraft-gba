#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Build private N64-style menu chrome for the SC64 frontend.

The verified ROM and the pinned reference decoder stay local. Generated image
tables and previews belong under .local/sc64/interface/generated and must not
be published.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
if str(ROOT / 'tools') not in sys.path:
    sys.path.insert(0, str(ROOT / 'tools'))
ROM_SHA1 = 'a36d91b5aac7bb95df0d7de5f9814f5270e25c2b'
REFERENCE_COMMIT = '09b57f787a247107d1de82b8c5eab2734667ea1d'
WIDTH, HEIGHT = 240, 160
PALETTE_FIRST, PALETTE_SIZE = 32, 224
WINDOW_RECT = (10, 0, 220, 160)  # preserves the 608x441 N64 window aspect
RAIL_SIZE = (220, 20)
BUTTON_SIZE = (128, 22)
SMALL_BUTTON_SIZE = (120, 11)
TALL_BUTTON_SIZE = (128, 30)
FOOTER_SIZE = RAIL_SIZE
NOTICE_SIZE = (208, 58)
WORLD_BORDER_X, WORLD_BORDER_Y = 6, 5
FRAME_ASSET = '002/010'
TITLE_PLATE_ASSET = '002/012'
RAIL_ASSET = '006/03C'
BUTTON_ASSETS = (('selected', '006/028'), ('idle', '006/029'))
NOTICE_ASSET = '006/033'
GREEN_KEY = (0, 255, 0)


def c_array_values(source: str, name: str, expected: int | None = None) -> list[int]:
    """Read an integer initializer from one generated C array."""
    match = re.search(
        rf'\b{re.escape(name)}\s*\[[^]]+\]\s*(?:__attribute__\s*\(\(.*?\)\))?\s*=\s*\{{(.*?)\}}\s*;',
        source, re.S)
    if not match:
        raise ValueError(f'Could not find generated array {name}')
    body = re.sub(r'/\*.*?\*/|//[^\n]*', '', match.group(1), flags=re.S)
    try:
        values = [int(token.strip(), 0) for token in body.split(',') if token.strip()]
    except ValueError as error:
        raise ValueError(f'{name} contains a non-integer initializer') from error
    if expected is not None and len(values) != expected:
        raise ValueError(f'{name} has {len(values)} values; expected {expected}')
    return values


def rgb555_to_rgb(color: int) -> tuple[int, int, int]:
    if not 0 <= color <= 0x7FFF:
        raise ValueError('RGB555 palette entries must be 15-bit colors')
    return tuple(((color >> shift) & 31) * 255 // 31 for shift in (0, 5, 10))


def indexed_rgb(pixels: bytes, palette555: list[int]) -> bytes:
    if len(palette555) != PALETTE_SIZE:
        raise ValueError('Menu palette must contain 224 RGB555 values')
    colors = [rgb555_to_rgb(color) for color in palette555]
    out = bytearray(len(pixels) * 3)
    for i, index in enumerate(pixels):
        if index < PALETTE_FIRST or index > 255:
            raise ValueError('Menu artwork uses reserved UI palette indices')
        out[i * 3:i * 3 + 3] = bytes(colors[index - PALETTE_FIRST])
    return bytes(out)


def resize_nearest(rgba: bytes, width: int, height: int,
                   target_width: int, target_height: int) -> bytes:
    """Resize RGBA without filtering chroma-key edges."""
    if width <= 0 or height <= 0 or len(rgba) != width * height * 4:
        raise ValueError('RGBA dimensions do not match their byte length')
    if target_width <= 0 or target_height <= 0:
        raise ValueError('Target dimensions must be positive')
    out = bytearray(target_width * target_height * 4)
    for y in range(target_height):
        sy = min(height - 1, int((y + .5) * height / target_height))
        for x in range(target_width):
            sx = min(width - 1, int((x + .5) * width / target_width))
            src, dst = (sy * width + sx) * 4, (y * target_width + x) * 4
            out[dst:dst + 4] = rgba[src:src + 4]
    return bytes(out)


def over_rgb(background: bytearray, source_rgba: bytes, source_size: tuple[int, int],
             rect: tuple[int, int, int, int], *, fill_color=None,
             fill_mix: int = 0, key=GREEN_KEY, extra_keys=(),
             destination_size: tuple[int, int] | None = None) -> None:
    """Composite a keyed original asset into an RGB screen or sprite."""
    width, height = source_size
    x0, y0, out_w, out_h = rect
    if destination_size is not None:
        destination_width, destination_height = destination_size
        if len(background) != destination_width * destination_height * 3:
            raise ValueError('RGB destination dimensions do not match its byte length')
    elif len(background) == WIDTH * HEIGHT * 3:
        destination_width, destination_height = WIDTH, HEIGHT
    elif x0 == 0 and y0 == 0 and len(background) == out_w * out_h * 3:
        destination_width, destination_height = out_w, out_h
    else:
        raise ValueError('RGB destination dimensions are required for an interior composite')
    if width <= 0 or height <= 0 or len(source_rgba) != width * height * 4:
        raise ValueError('RGBA source dimensions do not match their byte length')
    if out_w <= 0 or out_h <= 0 or x0 < 0 or y0 < 0:
        raise ValueError('Composite rectangle must have positive dimensions')
    if x0 + out_w > destination_width or y0 + out_h > destination_height:
        raise ValueError('Composite rectangle lies outside its destination')
    resized = resize_nearest(source_rgba, width, height, out_w, out_h)
    keyed_rgb = tuple(key)
    for y in range(out_h):
        for x in range(out_w):
            src = (y * out_w + x) * 4
            r, g, b, alpha = resized[src:src + 4]
            if not alpha or (r, g, b) == keyed_rgb or (r, g, b) in extra_keys:
                continue
            if fill_color is not None and (r, g, b) == tuple(fill_color):
                r, g, b = fill_color
                mix = fill_mix
            else:
                mix = alpha
            dst_x, dst_y = x0 + x, y0 + y
            dst = (dst_y * destination_width + dst_x) * 3
            inv = 255 - mix
            background[dst] = (r * mix + background[dst] * inv) // 255
            background[dst + 1] = (g * mix + background[dst + 1] * inv) // 255
            background[dst + 2] = (b * mix + background[dst + 2] * inv) // 255


def keyed_sprite(rgba: bytes, source_size: tuple[int, int], target_size: tuple[int, int],
                 panel_color=(11, 16, 23)) -> bytes:
    """Make a bounded opaque sprite; keyed gaps become smoked panel glass."""
    tw, th = target_size
    rgb = bytearray(bytes(panel_color) * (tw * th))
    over_rgb(rgb, rgba, source_size, (0, 0, tw, th))
    return bytes(rgb)


def crop_rgba(rgba: bytes, width: int, height: int,
              crop: tuple[int, int, int, int]) -> bytes:
    x, y, crop_width, crop_height = crop
    if len(rgba) != width * height * 4 or min(x, y) < 0 or min(crop_width, crop_height) <= 0:
        raise ValueError('Invalid RGBA crop dimensions')
    if x + crop_width > width or y + crop_height > height:
        raise ValueError('RGBA crop exceeds source bounds')
    out = bytearray(crop_width * crop_height * 4)
    for row in range(crop_height):
        src = ((y + row) * width + x) * 4
        dst = row * crop_width * 4
        out[dst:dst + crop_width * 4] = rgba[src:src + crop_width * 4]
    return bytes(out)


def nine_slice_button(frame: bytes, source_size: tuple[int, int],
                      selector: bytes, selector_size: tuple[int, int],
                      target_size: tuple[int, int], panel_color=(11, 16, 23)) -> bytes:
    """Retile source 006/023 bevel rails, then add the source selector bar."""
    sw, sh = source_size
    tw, th = target_size
    if (sw, sh) != (368, 131):
        raise ValueError('Expected the verified 006/023 wide panel frame')
    if tw <= 10 or th <= 8:
        raise ValueError('Button target is too small for a source bevel')
    # Crops run along actual straight portions of the purple/amber rails,
    # outside the large left-hand ornament on the original full panel.
    top = crop_rgba(frame, sw, sh, (80, 13, 270, 12))
    bottom = crop_rgba(frame, sw, sh, (80, 119, 270, 12))
    left = crop_rgba(frame, sw, sh, (60, 20, 17, 91))
    right = crop_rgba(frame, sw, sh, (351, 20, 17, 91))
    out = bytearray(bytes(panel_color) * (tw * th))
    top_h, bottom_h, side_w = ((2, 2, 3) if th < 18 else (4, 4, 4))
    over_rgb(out, top, (270, 12), (0, 0, tw, top_h), destination_size=(tw, th))
    over_rgb(out, bottom, (270, 12), (0, th - bottom_h, tw, bottom_h),
             destination_size=(tw, th))
    over_rgb(out, left, (17, 91), (0, top_h, side_w, th - top_h - bottom_h),
             destination_size=(tw, th))
    over_rgb(out, right, (17, 91), (tw - side_w, top_h, side_w, th - top_h - bottom_h),
             destination_size=(tw, th))
    sw2, sh2 = selector_size
    # The source selector sheets use flat 41/82/41 green as their panel fill,
    # separate from the pure-green chroma matte. Leave that fill transparent
    # so the bevel and the smoked button interior remain visible.
    over_rgb(out, selector, (sw2, sh2), (0, 0, tw, th),
             extra_keys=((41, 82, 41),), destination_size=(tw, th))
    return bytes(out)


def rail_nineslice(rgba: bytes, source_size: tuple[int, int],
                   target_size: tuple[int, int], *, cap: int = 12,
                   panel_color=(11, 16, 23), key=GREEN_KEY) -> bytes:
    """Retile an original narrow rail, preserving its arrow end caps."""
    sw, sh = source_size
    tw, th = target_size
    if sw <= 2 * cap or sh <= 0 or len(rgba) != sw * sh * 4:
        raise ValueError('Rail source dimensions do not match its byte length')
    if tw <= 2 * cap or th <= 0:
        raise ValueError('Rail target must leave room for both source end caps')
    out = bytearray(bytes(panel_color) * (tw * th))
    for y in range(th):
        sy = min(sh - 1, int((y + .5) * sh / th))
        for x in range(tw):
            if x < cap:
                sx = x
            elif x >= tw - cap:
                sx = sw - (tw - x)
            else:
                sx = cap + (x - cap) * (sw - 2 * cap) // (tw - 2 * cap)
            src = (sy * sw + sx) * 4
            r, g, b, alpha = rgba[src:src + 4]
            if not alpha or (r, g, b) == tuple(key):
                continue
            dst = (y * tw + x) * 3
            inv = 255 - alpha
            out[dst] = (r * alpha + out[dst] * inv) // 255
            out[dst + 1] = (g * alpha + out[dst + 1] * inv) // 255
            out[dst + 2] = (b * alpha + out[dst + 2] * inv) // 255
    return bytes(out)


def world_panel_map(rgba: bytes, source_size: tuple[int, int],
                    target_size: tuple[int, int], world_palette: list[int],
                    *, key=GREEN_KEY) -> tuple[bytes, list[int]]:
    """Encode a keyed world-space panel as transparent strengths plus indices.

    Palette lookup colors are *indices* from the existing 32..255 world
    palette. The game display curve can continue to transform those entries.
    A flat center/glass color is removed so the terrain remains visible inside.
    """
    if len(world_palette) != 256 or any(not 0 <= c <= 0x7FFF for c in world_palette):
        raise ValueError('World palette must be 256 RGB555 entries')
    sw, sh = source_size
    tw, th = target_size
    if len(rgba) != sw * sh * 4 or sw <= 0 or sh <= 0 or tw <= 0 or th <= 0:
        raise ValueError('World panel source dimensions do not match their byte length')
    center = ((sh // 2) * sw + sw // 2) * 4
    glass = tuple(rgba[center:center + 3])
    scaled = resize_nearest(rgba, sw, sh, tw, th)
    colors_rgb = [rgb555_to_rgb(c) for c in world_palette[32:]]
    mapped = bytearray(tw * th)
    chosen: list[int] = []
    lookup: dict[tuple[int, int, int], int] = {}
    for i in range(tw * th):
        x, y = i % tw, i // tw
        if (WORLD_BORDER_X <= x < tw - WORLD_BORDER_X and
                WORLD_BORDER_Y <= y < th - WORLD_BORDER_Y):
            continue
        r, g, b, alpha = scaled[i * 4:i * 4 + 4]
        color = (r, g, b)
        if (not alpha or color == tuple(key) or color == glass):
            continue
        if color not in lookup:
            best = min(range(224), key=lambda j: sum((color[k] - colors_rgb[j][k]) ** 2
                                                       for k in range(3)))
            if len(chosen) >= 255:
                raise ValueError('World panel needs more than 255 source-color lookup rows')
            lookup[color] = len(chosen) + 1
            chosen.append(best + 32)
        mapped[i] = lookup[color]
    palette_indices = [chosen_index for chosen_index in chosen]
    return bytes(mapped), palette_indices


def world_panel_nineslice(pixels: bytes, source_size: tuple[int, int],
                          target_size: tuple[int, int]) -> bytes:
    """Scale a world panel border while keeping its inner text rectangle clear."""
    sw, sh = source_size
    tw, th = target_size
    bx, by = WORLD_BORDER_X, WORLD_BORDER_Y
    if len(pixels) != sw * sh or sw <= 2 * bx or sh <= 2 * by:
        raise ValueError('World panel strength dimensions do not match')
    if tw <= 2 * bx + 1 or th <= 2 * by + 1:
        raise ValueError('World panel target is too small for fixed rails')
    out = bytearray(tw * th)
    for y in range(th):
        if by <= y < th - by:
            sy = by + ((y - by) * (sh - 2 * by - 1) // (th - 2 * by - 1))
        else:
            sy = y if y < by else sh - (th - y)
        for x in range(tw):
            if bx <= x < tw - bx and by <= y < th - by:
                continue
            if bx <= x < tw - bx:
                sx = bx + ((x - bx) * (sw - 2 * bx - 1) // (tw - 2 * bx - 1))
            else:
                sx = x if x < bx else sw - (tw - x)
            out[y * tw + x] = pixels[sy * sw + sx]
    return bytes(out)


def palette_rgb(images: list[bytes]) -> list[tuple[int, int, int]]:
    from sc64_menu_assets import median_palette
    colors = median_palette(images, PALETTE_SIZE)
    if len(colors) < PALETTE_SIZE:
        colors.extend([(0, 0, 0)] * (PALETTE_SIZE - len(colors)))
    return colors


def nearest_indices(rgb: bytes, colors: list[tuple[int, int, int]]) -> bytes:
    from sc64_menu_assets import nearest
    if len(colors) != PALETTE_SIZE:
        raise ValueError('Chrome palette must contain 224 colors')
    indexed = nearest(rgb, colors)
    if len(indexed) != len(rgb) // 3 or any(v < 32 for v in indexed):
        raise ValueError('Quantized chrome exceeded the reserved UI palette range')
    return indexed


def c_array(name: str, ctype: str, values, width: int = 24,
            alignment: bool = False) -> str:
    values = list(values)
    rows = [','.join(str(v) for v in values[i:i + width])
            for i in range(0, len(values), width)]
    attribute = ' __attribute__((aligned(4)))' if alignment else ''
    return (f'static const {ctype} {name}[{len(values)}]{attribute} = {{\n' +
            ',\n'.join(rows) + '\n};\n')


def indexed_array(name: str, pixels: bytes, width: int, height: int) -> str:
    if len(pixels) != width * height:
        raise ValueError(f'{name} pixel count does not match {width}x{height}')
    return c_array(name, 'u8', pixels, width=24, alignment=True)


def build_header(backgrounds: dict[str, bytes], sprites: dict[str, tuple[bytes, int, int]],
                 colors: list[tuple[int, int, int]],
                 world_panel: tuple[bytes, list[int]] | None = None) -> str:
    if set(backgrounds) != {'home', 'zerg', 'protoss', 'terran'}:
        raise ValueError('Chrome bank requires the four current menu backdrops')
    if len(colors) != PALETTE_SIZE:
        raise ValueError('Chrome palette must have 224 colors')
    indexed_backgrounds = {name: nearest_indices(rgb, colors)
                           for name, rgb in backgrounds.items()}
    indexed_sprites = {name: (nearest_indices(rgb, colors), width, height)
                       for name, (rgb, width, height) in sprites.items()}
    colors555 = [((b >> 3) << 10) | ((g >> 3) << 5) | (r >> 3)
                 for r, g, b in colors]
    out = ['/* Generated privately from the supplied ROM. Do not publish. */\n',
           f'/* Artwork decoded with references/sc64-maps at commit {REFERENCE_COMMIT}. */\n',
           '#pragma once\n#include "gba.h"\n',
           '#define SC64_CHROME_WIDTH 240\n#define SC64_CHROME_HEIGHT 160\n',
           '#define SC64_CHROME_PALETTE_FIRST 32\n#define SC64_CHROME_PALETTE_COUNT 224\n',
           f'#define SC64_CHROME_BUTTON_WIDTH {BUTTON_SIZE[0]}\n#define SC64_CHROME_BUTTON_HEIGHT {BUTTON_SIZE[1]}\n',
           f'#define SC64_CHROME_SMALL_BUTTON_WIDTH {SMALL_BUTTON_SIZE[0]}\n#define SC64_CHROME_SMALL_BUTTON_HEIGHT {SMALL_BUTTON_SIZE[1]}\n',
           f'#define SC64_CHROME_TALL_BUTTON_WIDTH {TALL_BUTTON_SIZE[0]}\n#define SC64_CHROME_TALL_BUTTON_HEIGHT {TALL_BUTTON_SIZE[1]}\n',
           f'#define SC64_CHROME_RAIL_WIDTH {RAIL_SIZE[0]}\n#define SC64_CHROME_RAIL_HEIGHT {RAIL_SIZE[1]}\n',
           '#define SC64_CHROME_TITLE_PLATE_WIDTH 96\n#define SC64_CHROME_TITLE_PLATE_HEIGHT 30\n',
           f'#define SC64_CHROME_FOOTER_WIDTH {FOOTER_SIZE[0]}\n#define SC64_CHROME_FOOTER_HEIGHT {FOOTER_SIZE[1]}\n',
           f'#define SC64_CHROME_NOTICE_WIDTH {NOTICE_SIZE[0]}\n#define SC64_CHROME_NOTICE_HEIGHT {NOTICE_SIZE[1]}\n',
           c_array('sc64_chrome_menu_palette', 'u16', colors555)]
    for name in ('home', 'zerg', 'protoss', 'terran'):
        out.append(indexed_array(f'sc64_chrome_menu_{name}', indexed_backgrounds[name], WIDTH, HEIGHT))
    for name, (pixels, width, height) in indexed_sprites.items():
        out.append(indexed_array(f'sc64_chrome_{name}', pixels, width, height))
    if world_panel is not None:
        world_pixels, world_colors = world_panel
        out.append(f'#define SC64_CHROME_WORLD_PANEL_WIDTH {NOTICE_SIZE[0]}\n'
                   f'#define SC64_CHROME_WORLD_PANEL_HEIGHT {NOTICE_SIZE[1]}\n'
                   f'#define SC64_CHROME_WORLD_PANEL_COLOR_COUNT {len(world_colors)}\n')
        out.append(indexed_array('sc64_chrome_world_panel_pixels', world_pixels,
                                 NOTICE_SIZE[0], NOTICE_SIZE[1]))
        out.append(c_array('sc64_chrome_world_panel_colors', 'u8', world_colors))
    return ''.join(out)


def parse_base_art(menu_header: Path) -> tuple[dict[str, bytes], list[int]]:
    source = menu_header.read_text()
    palette = c_array_values(source, 'sc64_menu_palette', PALETTE_SIZE)
    backgrounds = {}
    for name in ('home', 'zerg', 'protoss', 'terran'):
        indexed = bytes(c_array_values(source, f'sc64_menu_{name}', WIDTH * HEIGHT))
        backgrounds[name] = bytearray(indexed_rgb(indexed, palette))
    return backgrounds, palette


def convert(rom_path: Path, out: Path, reference: Path, menu_header: Path,
            world_header: Path, report_path: Path | None = None) -> dict:
    sys.path.insert(0, str(reference))
    sys.path.insert(0, str(ROOT / 'tools'))
    from extract_sc64_maps import BoltArchive, load_rom
    import decode_art
    import sc64_menu_assets

    try:
        pinned = subprocess.check_output(['git', '-C', str(reference), 'rev-parse', 'HEAD'],
                                         text=True).strip()
    except (OSError, subprocess.CalledProcessError) as error:
        raise ValueError('Could not verify the pinned references/sc64-maps commit') from error
    if pinned != REFERENCE_COMMIT:
        raise ValueError(f'references/sc64-maps must stay at pinned commit {REFERENCE_COMMIT}')
    rom = load_rom(str(rom_path))
    rom_sha1 = hashlib.sha1(rom).hexdigest()
    if rom_sha1 != ROM_SHA1:
        raise ValueError('Expected the verified StarCraft 64 USA ROM')
    archive = BoltArchive(rom)
    raw_images, raw_palettes = decode_art.collect(archive)
    pairings = decode_art.pair_palettes(raw_images, raw_palettes)
    assets = (FRAME_ASSET, RAIL_ASSET, TITLE_PLATE_ASSET,
              *(path for _, path in BUTTON_ASSETS), '006/023', NOTICE_ASSET)
    decoded = {}
    for path in assets:
        palette_path = pairings.get(path)
        if not palette_path or path not in raw_images or palette_path not in raw_palettes:
            raise ValueError(f'Missing verified source art/palette for {path}')
        width, height, rgba = decode_art.decode_image(
            raw_images[path], decode_art.decode_palette(raw_palettes[palette_path]))
        decoded[path] = (width, height, rgba, palette_path)

    backgrounds, old_palette = parse_base_art(menu_header)
    base_hashes = {name: hashlib.sha256(rgb).hexdigest()
                   for name, rgb in backgrounds.items()}
    world_source = world_header.read_text()
    world_palette = c_array_values(world_source, 'sc64_palette', 256)
    frame_w, frame_h, frame_rgba, frame_palette = decoded[FRAME_ASSET]
    # The center pixel is the source frame's flat glass color. Blend that color
    # over the faction art; preserve keyed pixels as the original room behind it.
    center = ((frame_h // 2) * frame_w + frame_w // 2) * 4
    glass_color = tuple(frame_rgba[center:center + 3])
    for image in backgrounds.values():
        over_rgb(image, frame_rgba, (frame_w, frame_h), WINDOW_RECT,
                 fill_color=glass_color, fill_mix=96)

    sprites: dict[str, tuple[bytes, int, int]] = {}
    sw, sh, rgba, _ = decoded[RAIL_ASSET]
    sprites['rail'] = (rail_nineslice(rgba, (sw, sh), RAIL_SIZE), *RAIL_SIZE)
    frame_w, frame_h, button_frame, _ = decoded['006/023']
    for name, asset in BUTTON_ASSETS:
        sw, sh, rgba, _ = decoded[asset]
        sprites[f'button_{name}'] = (
            nine_slice_button(button_frame, (frame_w, frame_h), rgba, (sw, sh), BUTTON_SIZE),
            *BUTTON_SIZE)
        sprites[f'button_small_{name}'] = (
            nine_slice_button(button_frame, (frame_w, frame_h), rgba, (sw, sh), SMALL_BUTTON_SIZE),
            *SMALL_BUTTON_SIZE)
        sprites[f'button_tall_{name}'] = (
            nine_slice_button(button_frame, (frame_w, frame_h), rgba, (sw, sh), TALL_BUTTON_SIZE),
            *TALL_BUTTON_SIZE)
    sw, sh, rgba, _ = decoded[TITLE_PLATE_ASSET]
    sprites['title_plate'] = (keyed_sprite(rgba, (sw, sh), (96, 30)), 96, 30)
    sw, sh, rgba, _ = decoded[NOTICE_ASSET]
    sprites['notice'] = (keyed_sprite(rgba, (sw, sh), NOTICE_SIZE), *NOTICE_SIZE)

    palette_sources = list(backgrounds.values()) + [sprite[0] for sprite in sprites.values()]
    colors = palette_rgb(palette_sources)
    indexed_backgrounds = {name: nearest_indices(rgb, colors) for name, rgb in backgrounds.items()}
    indexed_sprites = {name: (nearest_indices(rgb, colors), width, height)
                       for name, (rgb, width, height) in sprites.items()}
    pw, ph, rgba, panel_palette = decoded[NOTICE_ASSET]
    world_panel = world_panel_map(rgba, (pw, ph), NOTICE_SIZE,
                                  world_palette)
    header_text = build_header(backgrounds, sprites, colors, world_panel)
    out.mkdir(parents=True, exist_ok=True)
    atomic_write(out / 'sc64_chrome_generated.h', header_text.encode())

    def preview(pixels: bytes, path: Path, width=WIDTH, height=HEIGHT):
        rgb = bytearray(width * height * 4)
        for i, index in enumerate(pixels):
            r, g, b = colors[index - PALETTE_FIRST]
            rgb[i * 4:i * 4 + 4] = bytes((r, g, b, 255))
        decode_art.write_png(str(path), width, height, bytes(rgb))

    for name, indexed in indexed_backgrounds.items():
        preview(indexed, out / f'menu-{name}.png')
    for name, (indexed, width, height) in indexed_sprites.items():
        preview(indexed, out / f'{name.replace("_", "-")}.png', width, height)

    report = {
        'schema': 1, 'rom_sha1': rom_sha1, 'reference_commit': pinned,
        'source_menu_header_sha256': hashlib.sha256(menu_header.read_bytes()).hexdigest(),
        'source_world_header_sha256': hashlib.sha256(world_header.read_bytes()).hexdigest(),
        'screen': [WIDTH, HEIGHT], 'palette_range': [PALETTE_FIRST, 255],
        'menu_sources': [{'scene': name, 'base_sha256': base_hashes[name],
                          'composite': '002/010 keyed nine-scale fit; flat green-glass pixels blended 96/255'}
                         for name in ('home', 'zerg', 'protoss', 'terran')],
        'assets': [{'image': path, 'palette': decoded[path][3],
                    'source_size': decoded[path][:2],
                    'image_sha256': hashlib.sha256(raw_images[path]).hexdigest(),
                    'palette_sha256': hashlib.sha256(raw_palettes[decoded[path][3]]).hexdigest()}
                   for path in assets],
        'sprites': {name: [width, height] for name, (_, width, height) in sprites.items()},
        'world_panel': {'image': NOTICE_ASSET, 'palette': panel_palette,
                        'size': list(NOTICE_SIZE),
                        'transparent_pixels': world_panel[0].count(0),
                        'mapped_palette_indices': len(world_panel[1])},
        'generated_header_sha256': hashlib.sha256((out / 'sc64_chrome_generated.h').read_bytes()).hexdigest(),
    }
    report_text = (json.dumps(report, indent=2) + '\n').encode()
    atomic_write(report_path or out / 'sc64_chrome_asset_report.json', report_text)
    return report


def atomic_write(path: Path, data: bytes) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    fd, name = tempfile.mkstemp(prefix=f'.{path.name}.', suffix='.tmp', dir=path.parent)
    try:
        with os.fdopen(fd, 'wb') as stream:
            stream.write(data)
            stream.flush()
            os.fsync(stream.fileno())
        os.replace(name, path)
    except BaseException:
        try:
            os.unlink(name)
        except FileNotFoundError:
            pass
        raise


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rom', type=Path, default=ROOT / '.local/sc64/rom/StarCraft64-USA.z64')
    parser.add_argument('--out', type=Path, default=ROOT / '.local/sc64/interface/generated')
    parser.add_argument('--reference', type=Path, default=ROOT / 'references/sc64-maps')
    parser.add_argument('--menu-header', type=Path,
                        default=ROOT / '.local/sc64/races/generated/sc64_menu_generated.h')
    parser.add_argument('--world-header', type=Path,
                        default=ROOT / '.local/sc64/backwater/generated/sc64_assets_generated.h')
    parser.add_argument('--report', type=Path)
    args = parser.parse_args()
    try:
        print(json.dumps(convert(args.rom, args.out, args.reference, args.menu_header,
                                args.world_header,
                                args.report), indent=2))
    except (ValueError, OSError, KeyError) as error:
        raise SystemExit(str(error))
