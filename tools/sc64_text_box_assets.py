#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Pack the N64 campaign's in-mission text boxes for the GBA.

Each Transmission and DisplayTextMessage trigger row becomes one row keyed by
its source string, with a speaker, a head image and text prewrapped to the GBA
box. Speaker and head selection follows the N64's own tables; see
docs/TEXT-BOXES.md. The ROM and the pinned extractor stay local. Everything
written below .local/sc64/text-box/generated is game content: do not publish it.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
from tools.sc64_briefing_assets import (MISSION_BRIEFING_INDEX, atomic_write, c_string,
                                        verify_inputs)

TRIGGER_SIZE = 2400
ACTION_BASE, ACTION_SIZE, ACTION_COUNT = 320, 32, 64
OP_TRANSMISSION, OP_DISPLAY_TEXT = 7, 9
ACTION_DISABLED = 2

# N64 tables, in the verified USA ROM. HEAD_TABLE holds one byte for each unit
# id, plus three advisor entries at 229 + race: the head index plus one.
HEAD_TABLE_ROM_OFFSET = 0xC76BC
HEAD_TABLE_BYTES = 232
ADVISOR_UNIT_BASE = 229
ADVISOR_UNIT = 106  # The N64 shows a Command Center's lines as the Advisor's.
ADVISOR_NAME = 'Advisor'
UNIT_NAME_TABLE = '00A/004'
HEAD_BANK = '016'
HEAD_WIDTH, HEAD_HEIGHT = 30, 28
HEAD_TRANSPARENT_NOTE = 'source index 0x8A is a dark backdrop color on the N64 and stays opaque'

# Global campaign ids, in the catalog's order. The N64's race numbers.
RACE_ZERG, RACE_TERRAN, RACE_PROTOSS = 0, 1, 2
MISSION_RACES = (
    (RACE_ZERG,) * 2 + (RACE_PROTOSS,) * 2 + (RACE_TERRAN,) * 10 +
    (RACE_ZERG,) * 8 + (RACE_PROTOSS,) * 8)
MISSION_NAMES = ('Z1', 'Z2', 'P1', 'P2', 'T1', 'T2', 'T3', 'T4', 'T5', 'T6', 'T7', 'T8', 'T9',
                 'T10', 'Z3', 'Z4', 'Z5', 'Z6', 'Z7', 'Z8', 'Z9', 'Z10', 'P3', 'P4', 'P5', 'P6',
                 'P7', 'P8', 'P9', 'P10')

# GBA box layout: the text column to the right of the head.
TEXT_PIXEL_WIDTH = 188
TEXT_ROWS = 4
TEXT_LINE_HEIGHT = 10
MAX_PAGES_PER_ROW = 8


def wrap_paragraphs(text: str, advances: dict[int, int], pixel_width: int = TEXT_PIXEL_WIDTH,
                    rows: int = TEXT_ROWS) -> list[str]:
    """Wrap text into pages, keeping the source's own line breaks."""
    lines: list[str] = []
    for paragraph in text.replace('\r\n', '\n').replace('\r', '\n').split('\n'):
        words = paragraph.split()
        if not words:
            continue
        current = ''
        for word in words:
            candidate = word if not current else f'{current} {word}'
            width = sum(advances[ord(ch)] for ch in candidate)
            if width <= pixel_width:
                current = candidate
                continue
            if not current:
                raise ValueError('A source word exceeds the text box width')
            lines.append(current)
            current = word
        lines.append(current)
    if not lines:
        raise ValueError('A text row has no printable text')
    return ['\n'.join(lines[i:i + rows]) for i in range(0, len(lines), rows)]


def printable(text: str) -> str:
    """Drop source control bytes, keeping line breaks and printable ASCII."""
    out = []
    for ch in text:
        if ch in '\r\n' or 0x20 <= ord(ch) <= 0x7e:
            out.append(ch)
        elif ch == '\t':
            out.append(' ')
    return ''.join(out)


def action_rows(payload: bytes, string_table: bytes, extractor) -> list[dict]:
    """Every enabled text action, with its source trigger and action slot."""
    if len(payload) % TRIGGER_SIZE:
        raise ValueError('TRIG size is not a multiple of the trigger record')
    rows = []
    for trigger, base in enumerate(range(0, len(payload), TRIGGER_SIZE)):
        for slot in range(ACTION_COUNT):
            raw = payload[base + ACTION_BASE + slot * ACTION_SIZE:][:ACTION_SIZE]
            string_id = struct.unpack_from('<I', raw, 4)[0]
            unit = struct.unpack_from('<H', raw, 24)[0]
            opcode, flags = raw[26], raw[28]
            if opcode not in (OP_TRANSMISSION, OP_DISPLAY_TEXT) or flags & ACTION_DISABLED:
                continue
            location = struct.unpack_from('<I', raw, 0)[0]
            # A Transmission at location 0 completes without opening a box.
            if opcode == OP_TRANSMISSION and location == 0:
                continue
            if not string_id:
                continue
            rows.append({'trigger': trigger, 'action': slot, 'opcode': opcode,
                         'unit': unit, 'string_id': string_id,
                         'text': extractor._chk_string(string_table, string_id)})
    return rows


def unit_name_overrides(sections: dict[bytes, bytes], extractor) -> dict[int, str]:
    """A map's own unit names: UNIS use-default flags, then each unit's string."""
    settings = sections.get(b'UNIS', b'')
    if not settings:
        return {}
    names = struct.unpack_from('<228H', settings, 228 + 228 * 13)
    return {unit: extractor._chk_string(sections.get(b'STR ', b''), name)
            for unit, name in enumerate(names) if name and not settings[unit]}


def table_string(table: bytes, index: int) -> str:
    count = struct.unpack_from('<H', table, 0)[0]
    if index >= count:
        raise ValueError('Unit name index exceeds its table')
    offset = struct.unpack_from('<H', table, 2 + index * 2)[0]
    end = table.index(b'\0', offset)
    return table[offset:end].decode('ascii')


def build_header(heads: list[bytes], speakers: list[str], pages: list[str],
                 missions: list[list[dict]]) -> str:
    lines = [
        '/* Generated privately from the supplied ROM. Do not publish. */\n',
        '#pragma once\n#include "gba.h"\n#include "sc64_text_box.h"\n',
        f'#define SC64_TEXT_BOX_HEAD_WIDTH {HEAD_WIDTH}\n',
        f'#define SC64_TEXT_BOX_HEAD_HEIGHT {HEAD_HEIGHT}\n',
        f'#define SC64_TEXT_BOX_HEAD_COUNT {len(heads)}\n',
        f'#define SC64_TEXT_BOX_TEXT_WIDTH {TEXT_PIXEL_WIDTH}\n',
        f'#define SC64_TEXT_BOX_TEXT_ROWS {TEXT_ROWS}\n',
        'static const u8 sc64_text_box_heads[SC64_TEXT_BOX_HEAD_COUNT]'
        '[SC64_TEXT_BOX_HEAD_WIDTH * SC64_TEXT_BOX_HEAD_HEIGHT] __attribute__((aligned(4))) = {\n',
    ]
    for head in heads:
        lines.append('{' + ','.join(str(v) for v in head) + '},\n')
    lines.append('};\n')
    lines.append(f'static const char *const sc64_text_box_speakers[{len(speakers)}] = {{\n')
    lines.extend(f'    {c_string(name)},\n' for name in speakers)
    lines.append('};\n')
    lines.append(f'static const char *const sc64_text_box_pages[{len(pages)}] = {{\n')
    lines.extend(f'    {c_string(page)},\n' for page in pages)
    lines.append('};\n')
    for index, rows in enumerate(missions):
        if not rows:
            continue
        lines.append(f'static const Sc64TextBoxRow sc64_text_box_rows_{index}[{len(rows)}] = {{\n')
        lines.extend(f"    {{{row['string_id']}, {row['first_page']}, {row['trigger']}, "
                     f"{row['action']}, {row['head']}, {row['speaker']}, {row['page_count']}}},\n"
                     for row in rows)
        lines.append('};\n')
    lines.append(f'static const Sc64TextBoxMission sc64_text_box_missions[{len(missions)}] = {{\n')
    for index, rows in enumerate(missions):
        lines.append(f'    {{sc64_text_box_rows_{index}, {len(rows)}}},\n' if rows else '    {0, 0},\n')
    lines.append('};\n')
    return ''.join(lines)


def convert(rom_path: Path, out: Path, reference: Path, remap_path: Path) -> dict:
    rom, rom_sha1 = verify_inputs(rom_path, reference)
    from tools.sc64_font_assets import font_advance, load_source_fonts
    small_font, _ = load_source_fonts(rom_path, reference)
    advances = {ch: font_advance(small_font, ch) for ch in range(32, 127)}
    if small_font.line_height != TEXT_LINE_HEIGHT:
        raise ValueError('The source font no longer matches the text box line height')
    sys.path.insert(0, str(reference))
    from extract_sc64_maps import BoltArchive, find_maps
    import extract_sc64_maps as extractor

    remap = remap_path.read_bytes()
    if len(remap) != 256:
        raise ValueError('The world palette remap must have 256 entries')
    archive = BoltArchive(rom)
    entries = {entry.path: entry for entry in archive.entries()}
    head_table = rom[HEAD_TABLE_ROM_OFFSET:HEAD_TABLE_ROM_OFFSET + HEAD_TABLE_BYTES]
    unit_names = archive.read(entries[UNIT_NAME_TABLE])

    maps = {}
    for info, chk in find_maps(archive):
        maps[info.bolt_path] = chk

    speakers: dict[str, int] = {}
    head_source: dict[int, int] = {}
    pages: list[str] = []
    page_index: dict[str, int] = {}
    missions: list[list[dict]] = []
    report_missions = []
    unit_map: dict[int, dict] = {}
    longest = 0
    for global_id, briefing_index in enumerate(MISSION_BRIEFING_INDEX):
        map_path = f'008/{briefing_index + 8:03X}'
        if map_path not in maps:
            raise ValueError(f'Missing campaign map {map_path}')
        sections = dict(extractor.chk_sections(maps[map_path]))
        race = MISSION_RACES[global_id]
        overrides = unit_name_overrides(sections, extractor)
        rows = []
        seen: dict[int, dict] = {}
        for row in action_rows(sections.get(b'TRIG', b''), sections.get(b'STR ', b''), extractor):
            if row['opcode'] == OP_DISPLAY_TEXT:
                unit = ADVISOR_UNIT_BASE + race
                name = ADVISOR_NAME
            else:
                unit = row['unit']
                name = ADVISOR_NAME if unit == ADVISOR_UNIT else \
                    overrides.get(unit) or table_string(unit_names, unit)
            if unit >= len(head_table):
                raise ValueError(f'{map_path}: speaker {unit} has no head table entry')
            head_id = head_table[unit]
            if head_id:
                head_id -= 1
            else:
                # Native fallback: a zero entry means the Advisor, by race.
                head_id = head_table[ADVISOR_UNIT_BASE + race] - 1
                name = ADVISOR_NAME
            unit_map[unit] = {'unit': unit, 'head': head_id, 'name': name}
            head = head_source.setdefault(head_id, len(head_source))
            speaker = speakers.setdefault(name, len(speakers))
            text_pages = wrap_paragraphs(printable(row['text']), advances)
            if len(text_pages) > MAX_PAGES_PER_ROW:
                raise ValueError(f'{map_path}: a text row exceeds {MAX_PAGES_PER_ROW} pages')
            duplicate = seen.get(row['string_id'])
            if duplicate is not None:
                # The same string in another trigger keeps its own trigger
                # and action, sharing the wrapped pages.
                first_page = duplicate['first_page']
            else:
                first_page = len(pages)
                pages.extend(text_pages)
                longest = max(longest, *(len(page) for page in text_pages))
            built = {'string_id': row['string_id'], 'first_page': first_page,
                     'trigger': row['trigger'], 'action': row['action'], 'head': head,
                     'speaker': speaker, 'page_count': len(text_pages),
                     'opcode': row['opcode']}
            seen[row['string_id']] = built
            rows.append(built)
        missions.append(rows)
        report_missions.append({'global_mission': global_id, 'name': MISSION_NAMES[global_id],
                                'map': map_path, 'rows': len(rows),
                                'pages': sum(row['page_count'] for row in rows)})

    heads = []
    for head_id, _ in sorted(head_source.items(), key=lambda item: item[1]):
        raw = archive.read(entries[f'{HEAD_BANK}/{head_id:03X}'])
        if len(raw) != HEAD_WIDTH * HEAD_HEIGHT:
            raise ValueError(f'Head {head_id} is not {HEAD_WIDTH}x{HEAD_HEIGHT}')
        heads.append(raw.translate(remap))

    speaker_list = [name for name, _ in sorted(speakers.items(), key=lambda item: item[1])]
    header = build_header(heads, speaker_list, pages, missions)
    out.mkdir(parents=True, exist_ok=True)
    header_path = out / 'sc64_text_box_generated.h'
    atomic_write(header_path, header.encode('ascii'))
    transmissions = sum(row['opcode'] == OP_TRANSMISSION for rows in missions for row in rows)
    display_text = sum(row['opcode'] == OP_DISPLAY_TEXT for rows in missions for row in rows)
    report = {
        'schema': 1,
        'rom_sha1': rom_sha1,
        'text_rows': {'transmission': transmissions, 'display_text': display_text},
        'speaker_units': len({u for u in unit_map if u < ADVISOR_UNIT_BASE}),
        'heads': len(heads),
        'head_bytes': len(heads) * HEAD_WIDTH * HEAD_HEIGHT,
        'head_note': HEAD_TRANSPARENT_NOTE,
        'unit_map': sorted(unit_map.values(), key=lambda item: item['unit']),
        'text_bytes': sum(len(page) + 1 for page in pages),
        'pages': len(pages),
        'longest_page_bytes': longest,
        'missions': report_missions,
        'generated_header_sha256': hashlib.sha256(header_path.read_bytes()).hexdigest(),
    }
    atomic_write(out / 'sc64_text_box_report.json', (json.dumps(report, indent=2) + '\n').encode('utf-8'))
    return report


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rom', type=Path, default=ROOT / '.local/sc64/rom/StarCraft64-USA.z64')
    parser.add_argument('--out', type=Path, default=ROOT / '.local/sc64/text-box/generated')
    parser.add_argument('--reference', type=Path, default=ROOT / 'references/sc64-maps')
    parser.add_argument('--palette-remap', type=Path,
                        default=ROOT / '.local/sc64/backwater/generated/palette-remap.bin')
    args = parser.parse_args()
    try:
        report = convert(args.rom, args.out, args.reference, args.palette_remap)
    except (ValueError, OSError, KeyError) as error:
        raise SystemExit(str(error))
    print(json.dumps({key: report[key] for key in
                      ('text_rows', 'speaker_units', 'heads', 'head_bytes', 'text_bytes', 'pages')}))


if __name__ == '__main__':
    main()
