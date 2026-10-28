#!/usr/bin/env python3
"""Generate private command-card icons from a verified SC64 ROM.

StarCraft draws every command button from one GRP (BOLT 011/016): frames
0-227 are the unit and structure portraits by raw unit id, and the later
frames are the order, spell and upgrade buttons. Frame pixels are not palette
colours but a 1-15 shade ramp (1 brightest) that the game tints per button
state. This tool keeps the frames the race command list uses, quantised to
three shades in 2 bits per pixel, so the HUD can tint them green, gold or
grey. Order icons are checked against the ROM's own button sets, and upgrade
and spell icons are read from upgrades.dat and techdata.dat. Output stays in
ignored .local/ folders.

Copyright (C) 2026 blizzard-gba contributors
SPDX-License-Identifier: GPL-3.0-or-later
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
from tools import sc64_sprites as base  # noqa: E402

OUT = ROOT / '.local/sc64/command-icons/generated'
ICON_GRP = '011/016'
UPGRADES_DAT = '00C/006'
TECHDATA_DAT = '00C/008'
STAT_TXT = '00A/004'
UPGRADE_COUNT = 61
TECH_COUNT = 44
MAX_SIDE = 20

# (name, source, id): 'unit' frames are raw unit ids, 'order' frames are
# checked against the button sets, 'upgrade' and 'tech' read the icon column.
ICONS = (
    ('MOVE', 'order', 228), ('STOP', 'order', 229), ('ATTACK', 'order', 230),
    ('UNLOAD', 'order', 312), ('LIFT', 'order', 282), ('LAND', 'order', 283),
    ('COMMAND_CENTER', 'unit', 106), ('SUPPLY_DEPOT', 'unit', 109),
    ('REFINERY', 'unit', 110), ('BARRACKS', 'unit', 111), ('ACADEMY', 'unit', 112),
    ('FACTORY', 'unit', 113), ('STARPORT', 'unit', 114), ('CONTROL_TOWER', 'unit', 115),
    ('ENGINEERING_BAY', 'unit', 122), ('HATCHERY', 'unit', 131), ('LAIR', 'unit', 132),
    ('HYDRALISK_DEN', 'unit', 135), ('EVOLUTION_CHAMBER', 'unit', 139),
    ('SPIRE', 'unit', 141), ('SPAWNING_POOL', 'unit', 142), ('CREEP_COLONY', 'unit', 143),
    ('EXTRACTOR', 'unit', 149), ('NEXUS', 'unit', 154), ('PYLON', 'unit', 156),
    ('ASSIMILATOR', 'unit', 157), ('GATEWAY', 'unit', 160), ('PHOTON_CANNON', 'unit', 162),
    ('CYBERNETICS_CORE', 'unit', 164), ('FORGE', 'unit', 166),
    ('SHIELD_BATTERY', 'unit', 172),
    ('TERRAN_WEAPONS', 'upgrade', 7), ('TERRAN_ARMOR', 'upgrade', 0),
    ('ZERG_WEAPONS', 'upgrade', 11), ('ZERG_ARMOR', 'upgrade', 3),
    ('PROTOSS_WEAPONS', 'upgrade', 13), ('PROTOSS_ARMOR', 'upgrade', 5),
    ('VENTRAL_SACS', 'upgrade', 24),
    ('CLOAK', 'tech', 10), ('CONSUME', 'tech', 16), ('STORM', 'tech', 19),
)
# Upgrade names from stat_txt, so a shifted table cannot pass silently.
UPGRADE_LABELS = {7: 'Infantry Weapons', 0: 'Infantry Armor', 11: 'Missile Attacks',
                  3: 'Carapace', 13: 'Ground Weapons', 5: 'Protoss Armor', 24: 'Ventral Sacs'}
# Button-set records (24 bytes, big-endian): position, icon, condition and
# action pointers, their two arguments, then the enabled string. Each order
# icon must appear with Brood War's string id for that order; Land carries
# the structure type and the spells carry their tech id.
ORDER_STRINGS = {228: 664, 229: 665, 230: 666, 312: 684, 282: 671, 283: 670}
TECH_ARGUMENTS = {10: 252, 16: 302, 19: 275}


def button_records(rom: bytes):
    for offset in range(0, len(rom) - 23, 4):
        position, icon, condition, action, _cv, argument, string = struct.unpack_from('>HHIIHHH', rom, offset)
        if (1 <= position <= 9 and icon < 400 and 0x80000000 <= condition < 0x80800000
                and 0x80000000 <= action < 0x80800000):
            yield icon, argument, string


def check_buttons(rom: bytes) -> dict:
    found = {icon: 0 for icon in ORDER_STRINGS}
    techs = {tech: 0 for tech in TECH_ARGUMENTS}
    for icon, argument, string in button_records(rom):
        if ORDER_STRINGS.get(icon) == string:
            found[icon] += 1
        for tech, tech_icon in TECH_ARGUMENTS.items():
            if icon == tech_icon and argument == tech:
                techs[tech] += 1
    missing = [icon for icon, count in found.items() if not count]
    missing += [f'tech {tech}' for tech, count in techs.items() if not count]
    if missing:
        raise ValueError(f'button sets do not confirm icons {missing}')
    return {'orders': found, 'techs': techs}


def stat_string(table: bytes, string_id: int) -> str:
    count = struct.unpack_from('<H', table)[0]
    if not 1 <= string_id <= count:
        raise ValueError(f'stat_txt string {string_id} out of range')
    offset = struct.unpack_from('<H', table, 2 * string_id)[0]
    return table[offset:table.index(b'\0', offset)].decode('latin-1')


def frame_id(kind: str, ident: int, upgrades: bytes, techdata: bytes, strings: bytes) -> int:
    if kind in ('unit', 'order'):
        return ident
    if kind == 'upgrade':
        icon = struct.unpack_from('<H', upgrades, 7 * 2 * UPGRADE_COUNT + 2 * ident)[0]
        label = struct.unpack_from('<H', upgrades, 8 * 2 * UPGRADE_COUNT + 2 * ident)[0]
        if stat_string(strings, label) != UPGRADE_LABELS[ident]:
            raise ValueError(f'upgrade {ident} is not {UPGRADE_LABELS[ident]}')
        return icon
    icon = struct.unpack_from('<H', techdata, 6 * 2 * TECH_COUNT + 2 * ident)[0]
    if icon != TECH_ARGUMENTS[ident]:
        raise ValueError(f'tech {ident} icon {icon} disagrees with its buttons')
    return icon


def shade(value: int) -> int:
    """2-bit shade of a 1-15 ramp value: 3 bright, 2 mid, 1 dark."""
    return 3 if value <= 5 else 2 if value <= 9 else 1


def pack(frame: dict) -> tuple[int, int, bytes]:
    width = frame['width']
    points = [i for i, alpha in enumerate(frame['alpha']) if alpha]
    x0 = min(i % width for i in points)
    x1 = max(i % width for i in points) + 1
    y0 = min(i // width for i in points)
    y1 = max(i // width for i in points) + 1
    w, h = x1 - x0, y1 - y0
    if w > MAX_SIDE or h > MAX_SIDE:
        raise ValueError(f'icon {w}x{h} exceeds {MAX_SIDE}x{MAX_SIDE}')
    stride = (w + 3) // 4
    data = bytearray(stride * h)
    for y in range(h):
        for x in range(w):
            i = (y0 + y) * width + x0 + x
            if frame['alpha'][i]:
                data[y * stride + x // 4] |= shade(frame['pixels'][i]) << (2 * (x & 3))
    return w, h, bytes(data)


def generate(rom: Path, out: Path) -> dict:
    extractor = base.load_extractor(base.UPSTREAM)
    normalized = extractor.load_rom(str(rom))
    sha1 = hashlib.sha1(normalized).hexdigest()
    if sha1 != base.EXPECTED_USA_ROM_SHA1:
        raise ValueError(f'Expected verified USA ROM {base.EXPECTED_USA_ROM_SHA1}, got {sha1}')
    archive = extractor.BoltArchive(normalized)
    grp_data = base._entry_payload(archive, ICON_GRP)
    grp = base.parse_grp(grp_data)
    upgrades = base._entry_payload(archive, UPGRADES_DAT)
    techdata = base._entry_payload(archive, TECHDATA_DAT)
    strings = base._entry_payload(archive, STAT_TXT)
    buttons = check_buttons(normalized)
    lines = ['/* Generated by tools/sc64_command_icons.py from a verified SC64 ROM. Private. */',
             '#ifndef SC64_COMMAND_ICONS_GENERATED_H', '#define SC64_COMMAND_ICONS_GENERATED_H',
             'enum {']
    lines += [f'    SC64_CMD_ICON_{name},' for name, _kind, _ident in ICONS]
    lines += ['    SC64_CMD_ICON_COUNT', '};',
              '/* 2 bits per pixel, low bits first: 0 clear, 1 dark, 2 mid, 3 bright. */',
              'static const unsigned char sc64_command_icon_bits[] = {']
    sizes, offsets, manifest, total = [], [], [], 0
    for name, kind, ident in ICONS:
        frame_index = frame_id(kind, ident, upgrades, techdata, strings)
        w, h, data = pack(grp['frames'][frame_index])
        offsets.append(total)
        sizes.append((w, h))
        total += len(data)
        lines.append(f'    /* {name}: frame {frame_index} */ ' + ','.join(str(v) for v in data) + ',')
        manifest.append({'name': name, 'source': kind, 'id': ident, 'frame': frame_index,
                         'w': w, 'h': h, 'bytes': len(data)})
    lines += ['};',
              'static const unsigned short sc64_command_icon_offset[SC64_CMD_ICON_COUNT] = {'
              + ','.join(str(v) for v in offsets) + '};',
              'static const unsigned char sc64_command_icon_size[SC64_CMD_ICON_COUNT][2] = {'
              + ','.join(f'{{{w},{h}}}' for w, h in sizes) + '};',
              '#endif', '']
    out.mkdir(parents=True, exist_ok=True)
    (out / 'sc64_command_icons_generated.h').write_text('\n'.join(lines))
    summary = {'rom_sha1': sha1, 'grp': ICON_GRP, 'grp_sha1': hashlib.sha1(grp_data).hexdigest(),
               'icons': len(ICONS), 'pixel_bytes': total, 'button_checks': buttons,
               'entries': manifest}
    (out / 'manifest.json').write_text(json.dumps(summary, indent=2) + '\n')
    return summary


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('--rom', type=Path, default=base.DEFAULT_ROM)
    parser.add_argument('--out', type=Path, default=OUT)
    args = parser.parse_args()
    summary = generate(args.rom, args.out)
    print(f"command icons: {summary['icons']} icons, {summary['pixel_bytes']} bytes")
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
