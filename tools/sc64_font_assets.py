#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Decode the verified StarCraft 64 bitmap fonts into a private GBA header."""
from __future__ import annotations

from dataclasses import dataclass
import hashlib
import json
import os
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
ROM_SHA1 = 'a36d91b5aac7bb95df0d7de5f9814f5270e25c2b'
REFERENCE_COMMIT = '09b57f787a247107d1de82b8c5eab2734667ea1d'
FONT_SMALL_PATH = '013/000'
FONT_MEDIUM_PATH = '013/001'
FONT_HEADER_BYTES = 12


@dataclass(frozen=True)
class FontGlyph:
    width: int
    height: int
    x_offset: int
    y_offset: int
    pixels: bytes
    advance: int


@dataclass(frozen=True)
class DecodedFont:
    first_char: int
    end_char: int  # Exclusive, as in the source offset table.
    max_width: int
    max_height: int
    glyphs: tuple[FontGlyph | None, ...]

    @property
    def line_height(self) -> int:
        return self.max_height + 1


def font_advance(font: DecodedFont, codepoint: int) -> int:
    """Return the bounded GBA text advance derived from source glyph bounds.

    StarCraft's FONT record stores ink bounds and bearings, but no cursor
    advance. GBA layout uses max(1, xOffset + width) + one pixel of spacing;
    an absent space glyph uses half the source maximum width, at least 2px.
    """
    if codepoint == 32:
        return max(2, font.max_width // 2)
    index = codepoint - font.first_char
    if 0 <= index < len(font.glyphs):
        glyph = font.glyphs[index]
        if glyph is not None:
            return glyph.advance
    fallback = ord('?') - font.first_char
    if 0 <= fallback < len(font.glyphs) and font.glyphs[fallback] is not None:
        return font.glyphs[fallback].advance
    return max(1, font.max_width)


def decode_font(data: bytes) -> DecodedFont:
    """Decode a StarCraft FONT resource, validating every table and glyph.

    Glyph offsets are absolute file offsets. A zero offset denotes an absent
    glyph. Pixel tokens encode a 0..31 pixel skip in the upper five bits and a
    0..7 source color in the lower three bits; every token then advances one
    pixel. Color zero is retained in the decoded mask as transparent.
    """
    if len(data) < FONT_HEADER_BYTES or data[:4] != b'FONT':
        raise ValueError('FONT resource has a truncated or invalid header')
    # The stored low bound is the space code, but source table entry zero is
    # the first ink glyph ('!'); space has no glyph record. Shift the range so
    # table index zero maps to low + 1 instead of drawing every character as
    # its successor.
    first_char, end_char, max_width, max_height = data[4:8]
    if first_char >= end_char or max_width == 0 or max_height == 0:
        raise ValueError('FONT resource has invalid character range or bounds')
    glyph_count = end_char - first_char
    table_end = FONT_HEADER_BYTES + glyph_count * 4
    if table_end > len(data):
        raise ValueError('FONT glyph offset table exceeds the resource')
    offsets = struct.unpack_from(f'<{glyph_count}I', data, FONT_HEADER_BYTES)
    present = [offset for offset in offsets if offset]
    if any(offset < table_end or offset >= len(data) for offset in present):
        raise ValueError('FONT glyph offset points into its table or past the resource')
    if any(a > b for a, b in zip(present, present[1:])):
        raise ValueError('FONT glyph offsets are not monotonic')

    result: list[FontGlyph | None] = []
    for index, offset in enumerate(offsets):
        if offset == 0:
            result.append(None)
            continue
        end = next((other for other in offsets[index + 1:] if other > offset), len(data))
        if end - offset < 4:
            raise ValueError(f'FONT glyph {first_char + index} has a truncated record')
        width, height = data[offset], data[offset + 1]
        x_offset, y_offset = struct.unpack_from('bb', data, offset + 2)
        if not width or not height or width > max_width or height > max_height:
            raise ValueError(f'FONT glyph {first_char + index} exceeds source bounds')
        pixel_count = width * height
        pixels = bytearray(pixel_count)
        cursor = 0
        for token in data[offset + 4:end]:
            cursor += token >> 3
            # Source records commonly finish with a final skip/token exactly
            # at the exclusive rectangle bound. It is a terminator with no
            # drawable pixel; anything beyond the bound is malformed.
            if cursor > pixel_count:
                raise ValueError(f'FONT glyph {first_char + index} pixel run exceeds its bounds')
            if cursor == pixel_count:
                break
            pixels[cursor] = token & 7
            cursor += 1
        advance = max(1, x_offset + width) + 1
        result.append(FontGlyph(width, height, x_offset, y_offset, bytes(pixels), advance))
    return DecodedFont(first_char + 1, end_char + 1, max_width, max_height, tuple(result))


def _array(name: str, values: bytes, width: int = 20) -> str:
    lines = [','.join(str(value) for value in values[i:i + width])
             for i in range(0, len(values), width)]
    return f'static const u8 {name}[{len(values)}] __attribute__((aligned(4))) = {{\n' + ',\n'.join(lines) + '\n};\n'


def build_header(small: DecodedFont, medium: DecodedFont) -> str:
    output = [
        '/* Generated privately from the supplied verified ROM. Do not publish. */\n',
        f'/* FONT resources 013/000 and 013/001; ROM SHA1 {ROM_SHA1}. */\n',
        '#ifndef SC64_FONT_GENERATED_H\n#define SC64_FONT_GENERATED_H\n',
    ]
    for name, font in (('small', small), ('medium', medium)):
        for code, glyph in enumerate(font.glyphs, font.first_char):
            if glyph is not None:
                pixels_name = f'sc64_font_{name}_{code}_pixels'
                output.append(_array(pixels_name, glyph.pixels))
        output.append(f'static const Sc64FontGlyph sc64_font_{name}_glyphs[{len(font.glyphs)}] = {{\n')
        for code, glyph in enumerate(font.glyphs, font.first_char):
            if glyph is None:
                output.append('    {0,0,0,0,0,0},\n')
                continue
            pixels_name = f'sc64_font_{name}_{code}_pixels'
            output.append(f'    {{{glyph.width},{glyph.height},{glyph.x_offset},{glyph.y_offset},'
                          f'{pixels_name},{glyph.advance}}},\n')
        output.append('};\n')
        output.append(f'static const Sc64Font sc64_font_{name} = {{'
                      f'{font.first_char},{len(font.glyphs)},{font.max_width},{font.max_height},'
                      f'{max(2,font.max_width//2)},'
                      f'{font.line_height},sc64_font_{name}_glyphs}};\n')
    output.append('#endif\n')
    return ''.join(output)


def load_source_fonts(rom_path: Path, reference_path: Path) -> tuple[DecodedFont, DecodedFont]:
    """Read and decode both fonts from a verified ROM using the pinned reader."""
    try:
        reference_commit = __import__('subprocess').check_output(
            ['git', '-C', str(reference_path), 'rev-parse', 'HEAD'], text=True).strip()
    except (OSError, __import__('subprocess').CalledProcessError) as error:
        raise ValueError('Could not verify the pinned sc64-maps reference checkout') from error
    if reference_commit != REFERENCE_COMMIT:
        raise ValueError(f'references/sc64-maps must remain at pinned commit {REFERENCE_COMMIT}')
    raw_rom = rom_path.read_bytes()
    if hashlib.sha1(raw_rom).hexdigest() != ROM_SHA1:
        raise ValueError('Font extraction requires the verified StarCraft 64 USA ROM')
    sys.path.insert(0, str(reference_path))
    from extract_sc64_maps import BoltArchive, load_rom
    archive = BoltArchive(load_rom(str(rom_path)))
    entries = {entry.path: entry for entry in archive.entries()}
    try:
        small = archive.read(entries[FONT_SMALL_PATH])
        medium = archive.read(entries[FONT_MEDIUM_PATH])
    except KeyError as error:
        raise ValueError(f'Missing source font resource {error.args[0]}') from error
    return decode_font(small), decode_font(medium)


def atomic_write(path: Path, data: bytes) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_name(f'.{path.name}.{os.getpid()}.tmp')
    try:
        temporary.write_bytes(data)
        temporary.replace(path)
    finally:
        if temporary.exists():
            temporary.unlink()


def generate(rom_path: Path = ROOT / '.local/sc64/rom/StarCraft64-USA.z64',
             reference_path: Path = ROOT / 'references/sc64-maps',
             out_dir: Path = ROOT / '.local/sc64/interface/generated') -> dict:
    small, medium = load_source_fonts(rom_path, reference_path)
    header = build_header(small, medium).encode('ascii')
    atomic_write(out_dir / 'sc64_font_generated.h', header)
    report = {
        'schema': 1,
        'rom_sha1': ROM_SHA1,
        'reference_commit': REFERENCE_COMMIT,
        'resources': {
            'small': {'path': FONT_SMALL_PATH, 'first_char': small.first_char,
                      'end_char_exclusive': small.end_char, 'glyphs': len(small.glyphs),
                      'present_glyphs': sum(glyph is not None for glyph in small.glyphs),
                      'max_width': small.max_width, 'max_height': small.max_height},
            'medium': {'path': FONT_MEDIUM_PATH, 'first_char': medium.first_char,
                       'end_char_exclusive': medium.end_char, 'glyphs': len(medium.glyphs),
                       'present_glyphs': sum(glyph is not None for glyph in medium.glyphs),
                       'max_width': medium.max_width, 'max_height': medium.max_height},
        },
        'gba_layout_policy': 'advance=max(1,xOffset+width)+1; absent space=max(2,maxWidth//2); source pixels and bearings preserved',
        'generated_header_sha256': hashlib.sha256(header).hexdigest(),
    }
    atomic_write(out_dir / 'sc64_font_asset_report.json',
                 (json.dumps(report, indent=2) + '\n').encode('utf-8'))
    return report


if __name__ == '__main__':
    try:
        report = generate()
    except (OSError, ValueError) as error:
        raise SystemExit(str(error))
    print(f"Decoded {report['resources']['small']['present_glyphs']} small and "
          f"{report['resources']['medium']['present_glyphs']} medium source glyphs")
