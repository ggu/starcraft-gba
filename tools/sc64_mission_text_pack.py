#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Pack the campaign's in-mission text boxes from a verified StarCraft 64 ROM.

Every enabled Transmission and DisplayTextMessage row of the 30 campaign
triggers becomes one row of a private header: its trigger record, action index,
opcode, string ID, speaker unit and wrapped text. The engine finds a row from
whichever of those facts its mission interpreter still holds, then presents it
through starcraft/sc64_mission_text.h.

The ROM and pinned reference extractor stay local. Output is written below
.local/sc64/mission-text/generated and must not be published.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
from tools.sc64_briefing_assets import (  # noqa: E402
    MISSION_BRIEFING_INDEX, PORTRAIT_COUNT, atomic_write, c_string, verify_inputs, wrap_text)

SCHEMA = 2
TRANSMISSION, DISPLAY_TEXT = 7, 9
ACTION_DISABLED = 2
# The box's text column: 186 source-font pixels by five 10-pixel rows.
TEXT_PIXEL_WIDTH, TEXT_ROWS = 186, 5
ROW_BREAK, PAGE_BREAK = '\n', '\f'
WORLD_FIRST_COLOUR, WORLD_COLOURS = 32, 224

# Speaker unit -> (name, briefing portrait). The trigger rows carry a unit ID,
# the briefings a portrait ID; each character here is matched by the speaker
# name the briefings give that portrait. Duke speaks through his Norad II and
# Tassadar through the Gantrithor. A portrait of None has no briefing art: the
# N64 draws its own face for these units, and that art has not been located, so
# the engine falls back to the HUD's portrait button for the same raw unit.
SPEAKERS: dict[int, tuple[str, int | None]] = {
    0: ('Marine', None),
    7: ('SCV', None),
    11: ('Dropship', None),
    15: ('Civilian', None),
    16: ('Kerrigan', 9),
    19: ('Jim Raynor', 8),
    20: ('Jim Raynor', 8),
    23: ('Duke', 3),
    27: ('Mengsk', 13),
    28: ('Jim Raynor', 8),
    29: ('Duke', 3),
    32: ('Firebat', None),
    41: ('Drone', None),
    51: ('Infested Kerrigan', 12),
    67: ('High Templar', None),
    75: ('Zeratul', 16),
    77: ('Fenix', 7),
    78: ('Fenix', 6),
    79: ('Tassadar', 14),
    82: ('Tassadar', 14),
    87: ('Aldaris', 2),
    106: ('Advisor', 0),
    126: ('Duke', 3),
    147: ('Zerg Overmind', 1),
    151: ('Zasz', 15),
    152: ('Daggoth', 4),
}
# DisplayTextMessage rows name no unit; the N64 shows them as the Advisor.
ADVISOR_UNIT = 106


def header_palette(path: Path, name: str, count: int) -> list[int]:
    """Read a generated u16 colour array back from its private header."""
    match = re.search(name + r'\[\d+\]\s*=\s*\{(.*?)\}', path.read_text(encoding='ascii'), re.S)
    if not match:
        raise ValueError(f'{path} has no {name}')
    values = [int(value) for value in match.group(1).replace('\n', ' ').split(',') if value.strip()]
    if len(values) != count:
        raise ValueError(f'{name} holds {len(values)} colours, not {count}')
    return values


def portrait_lut(briefing: list[int], world: list[int]) -> list[int]:
    """Map each briefing colour to its nearest world colour, as a palette index.

    Portraits index the briefing palette, which isn't live during a mission.
    Every race product shares one world palette, so this fits in ROM; working
    RAM has no room for it on the fullest candidates.
    """
    table = []
    for colour in briefing:
        red, green, blue = colour & 31, (colour >> 5) & 31, (colour >> 10) & 31
        best = min(range(len(world)), key=lambda i: (
            (red - (world[i] & 31)) ** 2 * 3 + (green - ((world[i] >> 5) & 31)) ** 2 * 4 +
            (blue - ((world[i] >> 10) & 31)) ** 2 * 2, i))
        table.append(WORLD_FIRST_COLOUR + best)
    return table


def map_path(global_mission: int) -> str:
    """Global mission -> its map; a briefing 007/i pairs with map 008/(i+8)."""
    return f'008/{MISSION_BRIEFING_INDEX[global_mission] + 8:03X}'


def wrap_pages(text: str, advances: dict[int, int] | None = None,
               pixel_width: int = TEXT_PIXEL_WIDTH, rows: int = TEXT_ROWS) -> str:
    """Wrap source text to box pages, keeping the source's own line breaks."""
    lines: list[str] = []
    for paragraph in text.replace('\r\n', '\n').replace('\r', '\n').split('\n'):
        if not paragraph.strip():
            continue
        wrapped = wrap_text(paragraph, rows=1 << 16, advances=advances, pixel_width=pixel_width)
        lines.extend(wrapped[0].split('\n'))
    if not lines:
        raise ValueError('A presented row has no text')
    return PAGE_BREAK.join(ROW_BREAK.join(lines[i:i + rows]) for i in range(0, len(lines), rows))


def speaker_of(opcode: int, unit: int) -> int:
    speaker = ADVISOR_UNIT if opcode == DISPLAY_TEXT else unit
    if speaker not in SPEAKERS:
        raise ValueError(f'Speaker unit {speaker} has no mapping')
    return speaker


def select_rows(triggers: list[dict]) -> list[dict]:
    """The presented rows of one mission, in source order."""
    rows = []
    for trigger in triggers:
        for action in trigger['actions']:
            if action['opcode'] not in (TRANSMISSION, DISPLAY_TEXT):
                continue
            if action['flags'] & ACTION_DISABLED:
                continue
            if not action.get('text', '').strip():
                raise ValueError(f"Trigger {trigger['record']} presents an empty row")
            rows.append({
                'trigger': trigger['record'], 'action': action['index'],
                'opcode': action['opcode'], 'string_id': action['string_id'],
                'unit': action['unit_type'], 'text': action['text'],
            })
    return rows


def build_header(rows_by_mission: list[list[dict]], advances: dict[int, int] | None = None,
                 lut: list[int] | None = None) -> tuple[str, dict]:
    """Render the private header and a size report (without any source text)."""
    speakers = sorted(SPEAKERS)
    speaker_index = {unit: index for index, unit in enumerate(speakers)}
    pool: dict[str, int] = {}
    pool_chunks: list[str] = []
    pool_bytes = 0
    max_pages = 0
    table: list[str] = []
    firsts: list[int] = []
    used: set[int] = set()
    for rows in rows_by_mission:
        firsts.append(len(table))
        for row in rows:
            for field, limit in (('trigger', 255), ('action', 255), ('string_id', 65535), ('unit', 65535)):
                if not 0 <= row[field] <= limit:
                    raise ValueError(f'Row {field} {row[field]} exceeds the runtime field')
            wrapped = wrap_pages(row['text'], advances)
            max_pages = max(max_pages, wrapped.count(PAGE_BREAK) + 1)
            if wrapped not in pool:
                pool[wrapped] = pool_bytes
                pool_chunks.append(wrapped)
                pool_bytes += len(wrapped) + 1
            if pool[wrapped] > 65535:
                raise ValueError('The text pool exceeds its 16-bit offsets')
            speaker = speaker_of(row['opcode'], row['unit'])
            used.add(speaker)
            table.append(
                f"    {{{row['string_id']}, {row['unit']}, {pool[wrapped]}, "
                f"{row['trigger']}, {row['action']}, {row['opcode']}, {speaker_index[speaker]}}},\n")
    firsts.append(len(table))
    lut = lut if lut is not None else [WORLD_FIRST_COLOUR] * WORLD_COLOURS
    if len(lut) != WORLD_COLOURS:
        raise ValueError('The portrait colour table needs one entry per briefing colour')
    if len(table) > 65535:
        raise ValueError('The row table exceeds its 16-bit indices')
    lines = [
        '/* Generated from a user-supplied StarCraft 64 ROM by tools/sc64_mission_text_pack.py.\n',
        ' * Keep this derived copyrighted data private; .local/ is ignored by Git. */\n',
        '#ifndef SC64_MISSION_TEXT_GENERATED_H\n#define SC64_MISSION_TEXT_GENERATED_H\n',
        f'#define SC64_MISSION_TEXT_SCHEMA_GENERATED {SCHEMA}\n',
        f'#define SC64_MISSION_TEXT_MISSION_COUNT {len(rows_by_mission)}\n',
        f'#define SC64_MISSION_TEXT_ROW_COUNT {len(table)}\n',
        f'#define SC64_MISSION_TEXT_SPEAKER_COUNT {len(speakers)}\n',
        f'#define SC64_MISSION_TEXT_POOL_BYTES {pool_bytes}\n',
        'static const char sc64_mission_text_pool[] =\n',
    ]
    for chunk in pool_chunks:
        pages = ' "\\f" '.join(c_string(page) for page in chunk.split(PAGE_BREAK))
        lines.append(f'    {pages} "\\0"\n')
    lines.append('    ;\n')
    lines.append('static const Sc64MissionTextSpeaker '
                 'sc64_mission_text_speakers[SC64_MISSION_TEXT_SPEAKER_COUNT] = {\n')
    for unit in speakers:
        name, portrait = SPEAKERS[unit]
        if portrait is not None and not 0 <= portrait < PORTRAIT_COUNT:
            raise ValueError(f'Speaker {name} has an invalid portrait {portrait}')
        lines.append(f"    {{{c_string(name)}, {-1 if portrait is None else portrait}, {unit}}},\n")
    lines.append('};\n')
    lines.append('static const Sc64MissionTextRow '
                 'sc64_mission_text_rows[SC64_MISSION_TEXT_ROW_COUNT] = {\n')
    lines.extend(table)
    lines.append('};\n')
    lines.append('static const u16 sc64_mission_text_first['
                 'SC64_MISSION_TEXT_MISSION_COUNT + 1] = {\n    ')
    lines.append(', '.join(str(first) for first in firsts))
    lines.append('\n};\n')
    lines.append(f'static const u8 sc64_mission_text_portrait_lut[{WORLD_COLOURS}] = {{\n    ')
    lines.append(', '.join(str(entry) for entry in lut))
    lines.append('\n};\n#endif\n')
    report = {
        'schema': SCHEMA,
        'rows': len(table),
        'rows_by_mission': [len(rows) for rows in rows_by_mission],
        'transmissions': sum(row['opcode'] == TRANSMISSION for rows in rows_by_mission for row in rows),
        'display_text': sum(row['opcode'] == DISPLAY_TEXT for rows in rows_by_mission for row in rows),
        'max_pages_per_row': max_pages,
        'pool_bytes': pool_bytes,
        'distinct_texts': len(pool),
        'speakers': len(speakers),
        'speakers_used': len(used),
        'speakers_without_briefing_portrait': [SPEAKERS[unit][0] for unit in sorted(used)
                                               if SPEAKERS[unit][1] is None],
        'portraits_used': sorted({SPEAKERS[unit][1] for unit in used if SPEAKERS[unit][1] is not None}),
        'max_rows_in_one_trigger': max(
            (max(sum(row['trigger'] == t for row in rows) for t in {row['trigger'] for row in rows})
             for rows in rows_by_mission if rows), default=0),
    }
    return ''.join(lines), report


def convert(rom_path: Path, out: Path, reference: Path, briefings: Path, world_assets: Path) -> dict:
    rom, rom_sha1 = verify_inputs(rom_path, reference)
    from tools.sc64_font_assets import font_advance, load_source_fonts
    from tools import sc64_import
    small_font, _ = load_source_fonts(rom_path, reference)
    advances = {ch: font_advance(small_font, ch) for ch in range(32, 127)}
    extractor = sc64_import.upstream_module(reference / 'extract_sc64_maps.py')
    archive = extractor.BoltArchive(rom)
    by_path = {}
    for info, chk in extractor.find_maps(archive):
        by_path[info.bolt_path] = chk
    rows_by_mission = []
    for global_mission in range(len(MISSION_BRIEFING_INDEX)):
        path = map_path(global_mission)
        if path not in by_path:
            raise ValueError(f'Global mission {global_mission} has no map at {path}')
        chk_sections = sc64_import.sections(by_path[path], extractor)
        triggers = sc64_import.trigger_rows(chk_sections.get(b'TRIG', b''),
                                            chk_sections.get(b'STR ', b''), extractor)
        rows_by_mission.append(select_rows(triggers))
    lut = portrait_lut(header_palette(briefings, 'sc64_briefing_palette', WORLD_COLOURS),
                       header_palette(world_assets, 'sc64_palette', 256)[WORLD_FIRST_COLOUR:])
    header, report = build_header(rows_by_mission, advances, lut)
    out.mkdir(parents=True, exist_ok=True)
    report.update({
        'rom_sha1': rom_sha1,
        'generated_header_sha256': hashlib.sha256(header.encode('ascii')).hexdigest(),
    })
    atomic_write(out / 'sc64_mission_text_generated.h', header.encode('ascii'))
    atomic_write(out / 'sc64_mission_text_report.json',
                 (json.dumps(report, indent=2, sort_keys=True) + '\n').encode('ascii'))
    return report


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--rom', type=Path, default=ROOT / '.local/sc64/rom/StarCraft64-USA.z64')
    parser.add_argument('--out', type=Path, default=ROOT / '.local/sc64/mission-text/generated')
    parser.add_argument('--reference', type=Path, default=ROOT / 'references/sc64-maps')
    parser.add_argument('--briefings', type=Path,
                        default=ROOT / '.local/sc64/briefings/generated/sc64_briefing_generated.h')
    parser.add_argument('--world-assets', type=Path,
                        default=ROOT / '.local/sc64/backwater/generated/sc64_assets_generated.h')
    args = parser.parse_args()
    try:
        print(json.dumps(convert(args.rom, args.out, args.reference, args.briefings, args.world_assets),
                         indent=2))
    except (ValueError, OSError, KeyError) as error:
        raise SystemExit(str(error))
