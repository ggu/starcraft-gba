#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Offline parity layer L0, content half: openings, iscript sequences, sounds.

tools/sc64_parity_tables.py diffs the ROM's rule tables. This sibling covers
the three L0 checks that table differ leaves out, with the same privacy rules,
writer and provenance tag (rom-table):

1. Opening censuses. For each of the 30 campaign missions, and the Blood Bath
   skirmish map, the ROM CHK's UNIT placements are joined with the world the
   GBA release profile builds at its launch: on raw type, raw owner and the
   source position halved (the GBA world is half the source pixels). The GBA
   side comes from tests/sc64_parity_run.c --scenario census, a host build of
   each mission's release profile writing the #895 trace format. Each mission
   is reported exact, collision (stacked placements the engine displaced),
   cap-dropped (over the GBA unit or building cap) or differs. Minerals,
   geysers and the starting banks are reported beside it.
2. Iscript sequences. For each unit the engine draws, the ROM iscript's
   Walking, ground and air attack, Death and construction animations are
   walked linearly (playfram, wait and waitrand, unconditional goto, sound
   opcodes) and compared with the engine's baked walk, attack, death and
   construction tables: frame order and per-frame wait where the engine
   models them. Animations with no baked table are listed as not modelled.
3. Sound ids. tools/sc64_audio_pack.py writes, beside its bank, the sfxdata
   ids behind every bank index (sc64_audio_sound_ids.json). For each unit and
   kind (ready, yes, what, annoyed, attack, death) the ROM's ids from units.dat
   and the iscript opcodes are compared with the ids the bank holds for that
   unit and kind.

Every ROM row is compared; anything the engine lacks is reported as absent,
never skipped. Output goes to $SC64_PARITY_HOME/content (default: .local/parity
in the main checkout), never to a tracked path. Engine values are read, never
changed.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import shutil
import struct
import subprocess
import sys
import tempfile
from collections import Counter, defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from tools.sc64_parity_tables import (  # noqa: E402
    PROVENANCE, InputsMissing, check_private, find_input, fixture_home, stock_names, write_private,
)
from tools.sc64_construction_sprites import (  # noqa: E402
    ANIMATIONS, decode_instruction, image_animation_starts, script_index, source_stage_inputs, table_images,
)

SCHEMA = 1
UNIT_COUNT, FLINGY_COUNT, SPRITE_COUNT, IMAGE_COUNT, WEAPON_COUNT = 228, 209, 517, 999, 130
VOICED_UNITS = 106
SOUND_COUNT = 1144

# Catalog id (starcraft/campaign_catalog.h), label and ROM map, in id order.
MISSIONS = (
    (0, 'Z1', '008/014'), (1, 'Z2', '008/015'), (2, 'P1', '008/01E'), (3, 'P2', '008/01F'),
    (4, 'T1', '008/00A'), (5, 'T2', '008/00B'), (6, 'T3', '008/00C'),
) + tuple((7 + i, f'T{4 + i}', f'008/{0x00D + i:03X}') for i in range(7)) \
  + tuple((14 + i, f'Z{3 + i}', f'008/{0x016 + i:03X}') for i in range(8)) \
  + tuple((22 + i, f'P{3 + i}', f'008/{0x020 + i:03X}') for i in range(8))
SKIRMISH = ('BB', '008/04F')  # Blood Bath, the imported skirmish map (lobby map 1)
PROFILE_BY_RACE = {'T': 'terran', 'Z': 'zerg-z10', 'P': 'protoss-p10'}
SKIRMISH_PROFILE = 'zerg-z10'  # the runner's lobby: Zerg human, one Terran computer
SKIRMISH_PLAYERS = 2           # so the lobby places two bases
SKIRMISH_OWNERS = {0, 1}       # the human (SIDE) and the one computer
SKIRMISH_HALLS = {0: 131, 1: 106}  # the runner's lobby: Zerg human (Hatchery), Terran computer (Command Center)
HUMAN_OWNER = 0                # engine owner of the human player (SIDE)
TOWN_HALLS = (106, 131, 132, 133, 154)  # Command Center, Hatchery, Lair, Hive, Nexus
START_LOCATION, GEYSER = 214, 188
MINERALS = (176, 177, 178)
# Units.dat ids drawn as GBA Building rows (the engine's structure range).
BUILDING_RAW = range(106, 202)
# Two placements closer than this (GBA px, both axes) are stacked: the engine
# cannot give both the same spot, so a displaced one is a collision.
STACK_RADIUS = 8
# A displaced stacked placement must land this close (GBA px, both axes).
DISPLACE_RADIUS = 64

# ---- iscript -------------------------------------------------------------

OP_PLAYFRAM, OP_WAIT, OP_WAITRAND, OP_GOTO = 0, 5, 6, 7
OP_END, OP_RETURN, OP_IGNOREREST = 22, 54, 48
OP_PLAYSND, OP_PLAYSNDRAND, OP_PLAYSNDBTWN, OP_ATTACKMELEE = 24, 25, 26, 28
SPRITE_OPS = {15, 16, 17, 19, 20, 21}
IMAGE_OPS = {8, 9, 10, 11, 13, 14}
ATTACK_ANIMATIONS = ('GndAttkInit', 'AirAttkInit', 'GndAttkRpt', 'AirAttkRpt')
CONSTRUCTION_ANIMATIONS = ('Init', 'SpecialState1', 'SpecialState2', 'AlmostBuilt')
# ROM animations a unit can carry; those with a baked engine table are named
# by the section that compares them.
MODELLED = {'Walking': 'walk', 'GndAttkInit': 'attack', 'AirAttkInit': 'attack', 'Death': 'death'}


def rom_sequence(payload: bytes, start: int, limit: int = 256, frame=None, steady: bool = False) -> dict:
    """Walk one animation linearly, as the source runs it with no branch taken.

    Returns steps [(frame, wait_low, wait_high)] in order, how the path ends
    ('end', 'hold' for ignorerest, 'loop' or 'limit'), the sound opcodes with
    the step index before which each plays, and the images and sprites it
    spawns. 'wait N' holds the frame N ticks and 'waitrand a b' a to b ticks,
    as OpenBW runs them (state.wait = N - 1, then one decrement per tick).
    Conditional jumps, calls and spawned images are not followed. A looping
    path also gets 'cycle': the steady loop, walked again from its head with
    the frame the loop body leaves, so a wait before the first playfram shows
    the right frame."""
    steps: list[tuple] = []
    sounds: list[tuple] = []
    spawns: list[tuple] = []
    pc = start
    visited: dict[int, int] = {}
    end = 'limit'
    loop_pc = None
    first_goto = goto_frame = None
    for _ in range(limit):
        if pc in visited:
            end, loop_pc = 'loop', pc
            break
        visited[pc] = len(steps)
        opcode, args, next_pc, jump = decode_instruction(payload, pc)
        if opcode == OP_PLAYFRAM:
            frame = args[0]
        elif opcode == OP_WAIT:
            steps.append((frame, args[0], args[0]))
        elif opcode == OP_WAITRAND:
            steps.append((frame, args[0], args[1]))
        elif opcode == OP_PLAYSND:
            sounds.append((len(steps), [args[0]]))
        elif opcode in (OP_PLAYSNDRAND, OP_ATTACKMELEE):
            sounds.append((len(steps), list(args[-1])))
        elif opcode == OP_PLAYSNDBTWN:
            sounds.append((len(steps), list(range(args[0], args[1] + 1))))
        elif opcode in SPRITE_OPS and args:
            spawns.append(('sprite', args[0]))
        elif opcode in IMAGE_OPS and args:
            spawns.append(('image', args[0]))
        if opcode == OP_GOTO and jump is not None:
            if first_goto is None:
                first_goto, goto_frame = len(steps), frame
            pc = jump
            continue
        if opcode in (OP_END, OP_RETURN):
            end = 'end'
            break
        if opcode == OP_IGNOREREST:
            end = 'hold'
            break
        pc = next_pc
    result = {'steps': steps, 'end': end, 'sounds': sounds, 'spawns': spawns, 'last_frame': frame,
              'first_goto': first_goto, 'goto_frame': goto_frame,
              'loop': visited[loop_pc] if loop_pc is not None else None}
    if steady:
        return result
    head = visited[loop_pc] if loop_pc is not None else None
    if head is not None and head >= len(steps):
        result['end'] = 'waitless loop'
        result['cycle'] = []
    elif loop_pc is not None and frame is not None and steps and steps[head][0] != frame:
        again = rom_sequence(payload, loop_pc, limit, frame, steady=True)
        result['cycle'] = again['steps'][:len(steps) - visited[loop_pc]]
    elif loop_pc is not None:
        result['cycle'] = steps[visited[loop_pc]:]
    return result


def collapse(steps) -> list:
    """Merge consecutive steps that show the same frame: [(frame, ticks)];
    a waitrand keeps its range as [low, high]."""
    out: list = []
    for frame, low, high in steps:
        if out and out[-1][0] == frame:
            prev = out[-1][1]
            lo = (prev[0] if isinstance(prev, list) else prev) + low
            hi = (prev[1] if isinstance(prev, list) else prev) + high
            out[-1] = (frame, lo if lo == hi else [lo, hi])
        else:
            out.append((frame, low if low == high else [low, high]))
    return out


def collapsed_index(steps, raw_index: int) -> int:
    """The collapse() group that holds raw step raw_index."""
    group = -1
    for i, (frame, _, _) in enumerate(steps[:raw_index + 1]):
        if i == 0 or frame != steps[i - 1][0]:
            group += 1
    return max(group, 0)


HOLD_LAST = 0xFF  # SC64_CONSTRUCTION_HOLD_LAST
MAX_ATTACK_TICKS = 1024  # far above the attack importer's 255-tick bound


def per_tick(steps, ticks: int, hold=None) -> list:
    """One frame per tick (a waitrand counts its low bound). A hold frame
    fills the remaining ticks, as ignorerest leaves the image."""
    out = []
    for frame, low, _ in steps:
        out.extend([frame] * low)
        if len(out) >= ticks:
            break
    if hold is not None and len(out) < ticks:
        out.extend([hold] * (ticks - len(out)))
    return out[:ticks]


def refines(fine: list, coarse: list) -> bool:
    """True when every label of fine maps to a single label of coarse: the
    fine sequence tells frames apart at least wherever the coarse one does."""
    seen: dict = {}
    return len(fine) == len(coarse) and all(seen.setdefault(f, c) == c for f, c in zip(fine, coarse))


def relabel(values) -> list:
    """Frame identities by first appearance, so two tables that name the same
    frames differently compare equal when the order and repeats agree."""
    names: dict = {}
    return [names.setdefault(v, len(names)) for v in values]


class Scripts:
    """DAT and iscript lookups over the authenticated tables."""

    def __init__(self, tables: dict):
        units, flingy, sprites, images = (tables['units.dat'], tables['flingy.dat'],
                                          tables['sprites.dat'], tables['images.dat'])
        weapons = tables['weapons.dat']
        u16 = lambda data, off, n: struct.unpack_from(f'<{n}H', data, off)
        self.unit_flingy = units[:UNIT_COUNT]
        self.subunit = u16(units, UNIT_COUNT, UNIT_COUNT)
        self.construction = struct.unpack_from(f'<{UNIT_COUNT}I', units, UNIT_COUNT * 5 + 96 * 2)
        self.ground_weapon = units[5892:5892 + UNIT_COUNT]
        self.air_weapon = units[6348:6348 + UNIT_COUNT]
        self.ready = u16(units, 9312, VOICED_UNITS)
        self.what = (u16(units, 9524, UNIT_COUNT), u16(units, 9980, UNIT_COUNT))
        self.annoyed = (u16(units, 10436, VOICED_UNITS), u16(units, 10648, VOICED_UNITS))
        self.yes = (u16(units, 10860, VOICED_UNITS), u16(units, 11072, VOICED_UNITS))
        self.flingy_sprite = u16(flingy, 0, FLINGY_COUNT)
        self.sprite_image = u16(sprites, 0, SPRITE_COUNT)
        self.image_turns = images[IMAGE_COUNT * 4:IMAGE_COUNT * 5]
        self.image_script = struct.unpack_from(f'<{IMAGE_COUNT}I', images, IMAGE_COUNT * 10)
        self.weapon_flingy = struct.unpack_from(f'<{WEAPON_COUNT}I', weapons, WEAPON_COUNT * 2)
        self.payload = tables['iscript.bin']
        self.index = script_index(self.payload)
        self.tables = table_images(units, flingy, sprites, images)

    def unit_image(self, raw: int) -> int:
        return self.sprite_image[self.flingy_sprite[self.unit_flingy[raw]]]

    def directional(self, image: int) -> bool:
        return bool(self.image_turns[image])

    def animations(self, image: int) -> dict:
        offset = self.index.get(self.image_script[image]) if image < IMAGE_COUNT else None
        return {} if offset is None else image_animation_starts(self.payload, offset)

    def sequence(self, image: int, animation: str):
        start = self.animations(image).get(animation)
        return None if start is None else rom_sequence(self.payload, start)

    def animation_sounds(self, image: int, names, spawned: bool) -> list:
        found = []
        for name in names:
            seq = self.sequence(image, name)
            if not seq:
                continue
            for _, ids in seq['sounds']:
                found.extend(ids)
            if spawned:  # explosions and remnants are separate images
                for kind, value in seq['spawns']:
                    child = self.sprite_image[value] if kind == 'sprite' and value < SPRITE_COUNT else \
                        value if kind == 'image' and value < IMAGE_COUNT else None
                    if child is not None and child != image:
                        found.extend(self.animation_sounds(child, ('Init',), False))
        return found

    def rom_sounds(self, raw: int) -> dict:
        """Every sound id the ROM gives one unit, by kind, in table order."""
        def span(pair):
            first, last = pair[0][raw], pair[1][raw]
            return list(range(first, last + 1)) if first else []
        images = [self.unit_image(raw)]
        if self.subunit[raw] < UNIT_COUNT:
            images.append(self.unit_image(self.subunit[raw]))
        attack = []
        for image in images:
            attack.extend(self.animation_sounds(image, ATTACK_ANIMATIONS, False))
        weapons = [self.ground_weapon[raw], self.air_weapon[raw]]
        if self.subunit[raw] < UNIT_COUNT:
            weapons += [self.ground_weapon[self.subunit[raw]], self.air_weapon[self.subunit[raw]]]
        for weapon in weapons:
            if weapon < WEAPON_COUNT and self.weapon_flingy[weapon] < FLINGY_COUNT:
                bullet = self.sprite_image[self.flingy_sprite[self.weapon_flingy[weapon]]]
                attack.extend(self.animation_sounds(bullet, ('Init',), False))
        voiced = raw < VOICED_UNITS
        sounds = {
            'ready': [self.ready[raw]] if voiced and self.ready[raw] else [],
            'yes': span(self.yes) if voiced else [],
            'what': span(self.what),
            'annoyed': span(self.annoyed) if voiced else [],
            'attack': attack,
            'death': self.animation_sounds(self.unit_image(raw), ('Death',), True),
        }
        return {k: list(dict.fromkeys(v for v in values if 0 < v < SOUND_COUNT))
                for k, values in sounds.items()}


# ---- 1. opening censuses -------------------------------------------------

def unit_rows(payload: bytes) -> list[dict]:
    """CHK UNIT records (36 bytes): position, type, owner, resources."""
    if len(payload) % 36:
        raise ValueError(f'UNIT section size {len(payload)} is not a multiple of 36')
    rows = []
    for off in range(0, len(payload), 36):
        x, y, raw = struct.unpack_from('<HHH', payload, off + 4)
        rows.append({'x': x, 'y': y, 'type': raw, 'owner': payload[off + 16],
                     'resources': struct.unpack_from('<I', payload, off + 20)[0], 'section': 'UNIT'})
    return rows


THG2_SPRITE = 0x1000   # the number is a sprites.dat id; clear: a units.dat id
THG2_DISABLED = 0x8000


def doodad_units(payload: bytes) -> list[dict]:
    """THG2 records (10 bytes) that place a unit, not a sprite: the doors,
    traps and other unit doodads of installation and temple maps."""
    rows = []
    for off in range(0, len(payload) - len(payload) % 10, 10):
        number, x, y, owner, _, flags = struct.unpack_from('<HHHBBH', payload, off)
        if not flags & THG2_SPRITE:
            rows.append({'x': x, 'y': y, 'type': number, 'owner': owner, 'resources': 0,
                         'section': 'THG2', 'disabled': bool(flags & THG2_DISABLED)})
    return rows


DISABLED = 0x02  # trigger condition and action flag: the game skips it
# Condition comparisons (CHK): 0 at least, 1 at most, 10 exactly.
AT_LEAST, AT_MOST, EXACTLY = 0, 1, 10
ANY_UNIT, MEN, BUILDINGS = 229, 230, 231


def executors(executed: list, forces: bytes, active: set) -> list:
    """Active players 0-7 that run a trigger: their own byte, All Players
    (17) or their force's byte (18-21)."""
    force = lambda p: forces[p] if len(forces) >= 8 else None
    return [p for p in range(8) if p in active and any(
        i < len(executed) and executed[i]
        for i in (p, 17, 18 + force(p) if force(p) is not None else 99))]


def resolve_players(group: int, current: int, forces: bytes, active: set) -> list:
    """Active players 0-7 a CHK player group names, for one executing player
    (Current Player, 13). Foes, Allies and neutral groups resolve to none."""
    if 0 <= group < 8:
        found = [group]
    elif group == 13:
        found = [current]
    elif group == 17:
        found = list(range(8))
    elif 18 <= group <= 21 and len(forces) >= 8:
        found = [p for p in range(8) if forces[p] == group - 18]
    else:
        found = []
    return [p for p in found if p in active]


def placed_count(placements: list[dict], players: list, unit_type: int) -> int:
    def counted(raw):
        if unit_type == ANY_UNIT:
            return True
        if unit_type == MEN:
            return raw not in BUILDING_RAW
        if unit_type == BUILDINGS:
            return raw in BUILDING_RAW
        return raw == unit_type
    return sum(1 for r in placements if r['owner'] in players and r['type'] != START_LOCATION
               and counted(r['type']))


def start_condition(c: dict, current: int, forces: bytes, active: set, placements: list[dict]) -> str | None:
    """A description when the condition holds on the first trigger pass for
    one executing player, from the CHK alone; None when it does not or cannot
    be decided statically."""
    name, cmp, amount = c['name'], c['comparison'], c['amount']
    if name == 'Always':
        return 'always'
    if name == 'ElapsedTime' and (cmp == AT_MOST or (cmp == AT_LEAST and amount <= 1)):
        return ('at most ' if cmp == AT_MOST else 'at least ') + f'{amount} s elapsed'
    if name == 'CountdownTimer' and cmp == AT_LEAST and amount == 0:
        return 'countdown at least 0'
    if name == 'Command' and cmp in (AT_LEAST, AT_MOST, EXACTLY) and c['unit_type'] in \
            set(range(UNIT_COUNT)) | {ANY_UNIT, MEN, BUILDINGS}:
        players = resolve_players(c['player_group'], current, forces, active)
        n = placed_count(placements, players, c['unit_type'])
        holds = n >= amount if cmp == AT_LEAST else n <= amount if cmp == AT_MOST else n == amount
        if players and holds:
            word = {AT_LEAST: 'at least', AT_MOST: 'at most', EXACTLY: 'exactly'}[cmp]
            return f"group {c['player_group']} commands {word} {amount} of type {c['unit_type']} ({n} placed)"
    return None


def active_players(owners: bytes) -> set:
    """Slots 0-7 the CHK's OWNR section fills (0 is an inactive slot, 7 neutral)."""
    return {p for p in range(min(8, len(owners))) if owners[p] not in (0, 7)}


def playing_players(owners: bytes) -> set:
    """Slots that run triggers: computer (5) and human (6) players; a
    rescuable (3) slot can be a target but executes nothing."""
    return {p for p in range(min(8, len(owners))) if owners[p] in (5, 6)}


def start_banks(triggers: list[dict], forces: bytes, placements: list[dict], owners: bytes) -> list[dict]:
    """SetResources rows whose trigger fires on its first pass. Each active
    executing player is evaluated alone: every condition must hold for it
    from the CHK (start_condition), and the action's group is resolved for
    it. One row per (trigger, action, target player)."""
    active = active_players(owners)
    rows: dict[tuple, dict] = {}
    for t in triggers:
        if not t['conditions']:
            continue
        for current in executors(t['executed_for_player_values'], forces, playing_players(owners)):
            reasons = [start_condition(c, current, forces, active, placements)
                   for c in t['conditions'] if not c.get('flags', 0) & DISABLED]
            if None in reasons:
                continue
            for index, a in enumerate(t['actions']):
                if a['name'] != 'SetResources' or a.get('flags', 0) & DISABLED:
                    continue
                kind = {0: 'minerals', 1: 'gas', 2: 'minerals+gas'}.get(a['unit_type'], f"resource {a['unit_type']}")
                mode = {7: 'set', 8: 'add', 9: 'subtract'}.get(a['modifier'], f"modifier {a['modifier']}")
                for player in resolve_players(a['group1'], current, forces, active):
                    key = (t['record'], index, player)
                    row = rows.setdefault(key, {'trigger': t['record'], 'players': [player], 'group': a['group1'],
                                                'resource': kind, 'mode': mode, 'amount': a['group2'],
                                                'when': '; '.join(reasons), 'executors': []})
                    row['executors'].append(current)
    return list(rows.values())


def parse_census_trace(text: str) -> dict:
    """Read one census trace: the launch record set (placements), then any
    later steps, of which only the bank rows are kept as a history."""
    lines = text.splitlines()
    if not lines or lines[0] != '#sc64-parity-trace 1':
        raise ValueError('not an sc64-parity-trace/1 file')
    header, steps, units, buildings, minerals, geysers, closing = {}, [], {}, {}, [], [], None
    history: dict[int, list] = {}
    for line in lines[1:]:
        tag, *rest = line.split()
        first = len(steps) == 1
        if tag in ('S', 'U', 'B', 'M', 'G', 'P') and \
                len(rest) < {'S': 11, 'U': 23, 'B': 22, 'M': 4, 'G': 3, 'P': 5}[tag]:
            raise ValueError(f'short {tag} record: {line!r}')
        if tag == 'H':
            header.update(field.split('=', 1) for field in rest)
        elif tag == 'S':
            steps.append([int(v) for v in rest])
        elif tag == 'U' and first:
            v = [int(x) for x in rest]
            units[v[0]] = dict(zip(('type', 'raw_type', 'owner', 'raw_owner', 'x', 'y', 'hp'), v[1:8]))
        elif tag == 'B' and first:
            v = [int(x) for x in rest]
            buildings[v[0]] = dict(zip(('type', 'raw_type', 'owner', 'raw_owner', 'x', 'y', 'hp'), v[1:8]))
        elif tag == 'M' and first:
            v = [int(x) for x in rest]
            minerals.append({'x': v[1], 'y': v[2], 'amount': v[3]})
        elif tag == 'G':
            v = [int(x) for x in rest]
            geysers.append({'x': v[1], 'y': v[2]})
        elif tag == 'P':
            v = [int(x) for x in rest]
            history.setdefault(v[0], []).append({'step': steps[-1][0] if steps else None, 'minerals': v[1],
                                                 'gas': v[2], 'supply': v[3], 'supply_cap': v[4]})
        elif tag == 'Z':
            closing = dict(field.split('=', 1) for field in rest)
    if closing is None or closing.get('overflow') != '0' or closing.get('write_errors') != '0':
        raise ValueError('incomplete trace: no Z line with overflow=0 write_errors=0')
    if not steps:
        raise ValueError('a census trace needs its launch record set (no S line)')
    launch = steps[0][0]
    banks = {o: h[0] for o, h in history.items() if h[0]['step'] == launch}
    return {'header': header, 'step': launch, 'frame': steps[0][3], 'last_step': steps[-1][0],
            'units': list(units.values()), 'buildings': list(buildings.values()),
            'minerals': minerals, 'geysers': geysers, 'banks': banks, 'bank_history': history,
            'max_units': int(header.get('max_units', 0)),
            'max_buildings': int(header.get('max_buildings', 0)),
            'max_minerals': int(header.get('max_minerals', 0))}


def near(a, b, radius):
    return abs(a[0] - b[0]) <= radius and abs(a[1] - b[1]) <= radius


def compare_census(chk: list[dict], gba: dict) -> dict:
    """Join CHK placements with a GBA census on raw type, raw owner and the
    halved position. Returns the verdict, per-class rows and counts."""
    actors = [r for r in chk if r['type'] not in MINERALS and r['type'] not in (GEYSER, START_LOCATION)]
    rom = [(r['type'], r['owner'], r['x'] // 2, r['y'] // 2) for r in actors]
    # Doors and traps placed by THG2 live in the GBA building table.
    doodad_keys = {(r['type'], r['owner'], r['x'] // 2, r['y'] // 2) for r in actors if r.get('section') == 'THG2'}
    gba_rows = [(g['raw_type'], g['raw_owner'], g['x'], g['y'], 'B' if kind == 'buildings' else 'U')
                for kind in ('units', 'buildings') for g in gba[kind]]
    left = Counter(r[:4] for r in gba_rows)
    # Which GBA table a raw type lives in, as this census shows it (doors and
    # traps are building rows outside the structure id range).
    building_types = {r[0] for r in gba_rows if r[4] == 'B'}
    unit_types = {r[0] for r in gba_rows if r[4] == 'U'}
    matched, unmatched = 0, []
    for key in rom:
        if left[key] > 0:
            left[key] -= 1
            matched += 1
        else:
            unmatched.append(key)
    spare = [k for k, n in left.items() for _ in range(n)]
    classes = {'collision': [], 'moved': [], 'cap-dropped': [], 'missing': [], 'extra': []}
    full_units = len(gba['units']) >= gba['max_units'] > 0
    full_buildings = len(gba['buildings']) >= gba['max_buildings'] > 0
    def dist(key, cand):
        return max(abs(cand[2] - key[2]), abs(cand[3] - key[3]))

    stacked = {key: sum(near(key[2:], other[2:], STACK_RADIUS) for other in rom) > 1 for key in set(unmatched)}
    left_rom = sorted(unmatched)
    # Phase 1: as many stacked placements as possible pair with nearby
    # same-type actors (maximum bipartite matching, Kuhn's algorithm). Nodes
    # and edges are visited in sorted order, so CHK row order decides nothing.
    left_keys = sorted(k for k in left_rom if stacked[k])
    edges = {i: sorted(((dist(k, c), c) for c in set(spare) if c[:2] == k[:2] and
                        near(k[2:], c[2:], DISPLACE_RADIUS)))
             for i, k in enumerate(left_keys)}
    pool = sorted(spare)
    owner_of: dict = {}

    def augment(i, seen):
        for _, cand in edges[i]:
            slots = [j for j, c in enumerate(pool) if c == cand and j not in seen]
            for j in slots:
                seen.add(j)
                if j not in owner_of or augment(owner_of[j], seen):
                    owner_of[j] = i
                    return True
        return False
    for i in range(len(left_keys)):
        augment(i, set())
    for j, i in sorted(owner_of.items(), key=lambda kv: kv[1]):
        key, cand = left_keys[i], pool[j]
        left_rom.remove(key)
        spare.remove(cand)
        classes['collision'].append({'type': key[0], 'owner': key[1], 'rom_half': list(key[2:]),
                                     'gba': list(cand[2:]), 'offset': [cand[2] - key[2], cand[3] - key[3]]})
    # Phase 2: the rest pair nearest first; a pair is a move. Indexed, so
    # identical placements and actors each pair once.
    rom_left, cand_left = sorted(left_rom), sorted(spare)
    pairs = sorted((dist(k, c), i, j) for i, k in enumerate(rom_left) for j, c in enumerate(cand_left)
                   if c[:2] == k[:2])
    used_rom, used_cand = set(), set()
    for _, i, j in pairs:
        if i in used_rom or j in used_cand:
            continue
        used_rom.add(i)
        used_cand.add(j)
        key, cand = rom_left[i], cand_left[j]
        left_rom.remove(key)
        spare.remove(cand)
        classes['moved'].append({'type': key[0], 'owner': key[1], 'rom_half': list(key[2:]),
                                 'gba': list(cand[2:]), 'offset': [cand[2] - key[2], cand[3] - key[3]]})
    for key in left_rom:
        building = key in doodad_keys or key[0] in building_types or \
            (key[0] not in unit_types and key[0] in BUILDING_RAW)
        full = full_buildings if building else full_units
        classes['cap-dropped' if full else 'missing'].append(
            {'type': key[0], 'owner': key[1], 'rom_half': list(key[2:]), 'stacked': stacked[key]})
    for s in spare:
        classes['extra'].append({'type': s[0], 'owner': s[1], 'gba': list(s[2:])})
    # Resources: minerals join on halved position, then compare amounts.
    rom_min = defaultdict(Counter)
    for row in chk:
        if row['type'] in MINERALS:
            rom_min[row['x'] // 2, row['y'] // 2][row['resources']] += 1
    gba_min = defaultdict(Counter)
    for patch in gba['minerals']:
        gba_min[patch['x'], patch['y']][patch['amount']] += 1
    minerals = {'rom': sum(sum(amounts.values()) for amounts in rom_min.values()),
                'gba': len(gba['minerals']), 'placed': 0, 'amount_differs': [], 'missing': [], 'extra': []}
    for pos in sorted(rom_min.keys() | gba_min.keys()):
        source, actual = rom_min[pos], gba_min[pos]
        # Match equal amounts first so coincident patches can appear in any order.
        rom_left = sorted((source - actual).elements())
        gba_left = sorted((actual - source).elements())
        pairs = min(len(rom_left), len(gba_left))
        minerals['placed'] += sum((source & actual).values()) + pairs
        minerals['amount_differs'].extend({'half': list(pos), 'rom': rom_amount, 'gba': gba_amount}
                                         for rom_amount, gba_amount in zip(rom_left, gba_left))
        minerals['missing'].extend({'half': list(pos), 'rom': amount} for amount in rom_left[pairs:])
        minerals['extra'].extend(list(pos) for _ in gba_left[pairs:])
    minerals['cap'] = gba['max_minerals']
    rom_gey = Counter((r['x'] // 2, r['y'] // 2) for r in chk if r['type'] == GEYSER)
    gba_gey = Counter((g['x'], g['y']) for g in gba['geysers'])
    geysers = {'rom': sum(rom_gey.values()), 'gba': sum(gba_gey.values()),
               'placed': sum((rom_gey & gba_gey).values()),
               'missing': [list(p) for p, n in (rom_gey - gba_gey).items() for _ in range(n)],
               'extra': [list(p) for p, n in (gba_gey - rom_gey).items() for _ in range(n)]}
    resources_bad = (minerals['missing'] or minerals['extra'] or minerals['amount_differs']
                     or geysers['missing'] or geysers['extra'])
    if classes['moved'] or classes['missing'] or classes['extra'] or resources_bad:
        verdict = 'differs'
    elif classes['cap-dropped']:
        verdict = 'cap-dropped'
    elif classes['collision']:
        verdict = 'collision'
    else:
        verdict = 'exact'
    return {'verdict': verdict, 'rom_actors': len(rom), 'gba_actors': len(gba_rows), 'matched': matched,
            'counts': {k: len(v) for k, v in classes.items()}, 'rows': classes,
            'minerals': minerals, 'geysers': geysers}


def owner_map(gba: dict) -> dict:
    """Raw owner -> engine owner, from the GBA rows that carry both."""
    seen: dict = {}
    for kind in ('units', 'buildings'):
        for g in gba[kind]:
            if g['raw_owner'] >= 0:
                seen.setdefault(g['raw_owner'], Counter())[g['owner']] += 1
    return {raw: c.most_common(1)[0][0] for raw, c in seen.items()}


def bank_parts(bank: dict, resource: str) -> list:
    return {'minerals': [bank['minerals']], 'gas': [bank['gas']],
            'minerals+gas': [bank['minerals'], bank['gas']]}.get(resource, [])


def compare_banks(rows: list[dict], gba: dict, owners: dict) -> list[dict]:
    """The ROM's first-pass SetResources rows against the GBA bank history:
    at launch, or at the first later step of the census run where the bank
    holds the value. A player the GBA census does not place has no bank."""
    out = []
    sharing = Counter(owners.values())
    for row in rows:
        for player in row['players']:
            engine = owners.get(player)
            history = gba['bank_history'].get(engine, []) if engine is not None else []
            entry = {'player': player, 'engine_owner': engine, 'resource': row['resource'],
                     'mode': row['mode'], 'rom': row['amount'], 'when': row['when'],
                     'trigger': row['trigger']}
            if not history:
                out.append(dict(entry, gba=None, verdict='absent (no GBA owner for this player)'))
                continue
            launch = bank_parts(history[0], row['resource'])
            entry['gba'] = launch[0] if len(launch) == 1 else launch
            if row['mode'] != 'set':
                out.append(dict(entry, verdict=f"not compared ({row['mode']})"))
                continue
            # The grant must be the launch value or the first change of the
            # granted balance (both resources at once for minerals+gas): a
            # later total reached by mining does not count.
            parts = {'minerals': ('minerals',), 'gas': ('gas',), 'minerals+gas': ('minerals', 'gas')}.get(
                row['resource'], ())
            if not parts:
                out.append(dict(entry, verdict=f"not compared (unknown resource type: {row['resource']})"))
                continue
            values = [(h['step'], tuple(h[part] for part in parts)) for h in history]
            want = tuple(row['amount'] for _ in parts)
            first_change = next(((st, v) for st, v in values if v != values[0][1]), None)
            if sharing[engine] > 1:
                # Several source slots share this engine owner; a balance
                # cannot be told apart from one slot's own wallet.
                verdict = (f'not compared (source slots {sorted(p for p, o in owners.items() if o == engine)} '
                           f'share engine owner {engine}; campaign AI wallets are not in the trace)')
            elif values[0][1] == want:
                verdict = 'match at launch'
            elif first_change and first_change[1] == want:
                verdict = f'match at step {first_change[0]}'
            elif gba['last_step'] - gba['step'] < gba.get('requested_steps', 0):
                # The run stopped early (a modal text box): the grant may not have run yet.
                verdict = (f"not compared (the census stopped at step {gba['last_step']}, before the "
                           f"{gba['requested_steps']} input-free steps a grant needs)")
            elif engine != HUMAN_OWNER:
                # Campaign computers keep mission-specific wallets per source
                # slot (race_p10.wallet_*, for one) that the trace does not
                # carry; the engine-owner bank says nothing about them.
                verdict = 'not compared (computer grant; campaign AI wallets are not in the trace)'
            elif first_change:
                verdict = f'differs (first changed to {list(first_change[1])} at step {first_change[0]})'
            else:
                verdict = f"differs (stays {list(values[0][1])} through step {gba['last_step']})"
            out.append(dict(entry, verdict=verdict))
    return out


def unexplained_banks(rows: list[dict], gba: dict, owners: dict) -> list[dict]:
    """Engine owners that start with a bank no first-pass CHK row sets."""
    covered = {owners.get(p) for r in rows for p in r['players']}
    return [{'engine_owner': o, 'minerals': b['minerals'], 'gas': b['gas']}
            for o, b in sorted(gba['banks'].items())
            if (b['minerals'] or b['gas']) and o not in covered]


def skirmish_census(chk: list[dict], gba: dict, hall_types: dict) -> dict:
    """Blood Bath is a melee map: its CHK holds only start locations and
    resources. The GBA lobby places one town hall per player; each must stand
    on its own start location. Skirmish rows carry no raw type, so town halls
    are recognised by the engine types the same profile's campaign censuses
    show for raw Command Center, Hatchery, Lair, Hive and Nexus."""
    starts = [(r['x'] // 2, r['y'] // 2) for r in chk if r['type'] == START_LOCATION]
    halls = [b for b in gba['buildings'] if b['type'] in hall_types.values()]
    used: set = set()
    offsets = []
    for h in halls:
        start = next((i for i, s in enumerate(starts) if i not in used and s == (h['x'], h['y'])), None)
        if start is not None:
            used.add(start)
            offsets.append([h['x'] - starts[start][0], h['y'] - starts[start][1]])
    owners = {h['owner'] for h in halls}
    base = compare_census([r for r in chk if r['type'] != START_LOCATION], dict(gba, units=[], buildings=[]))
    races_ok = all(hall_types.get(SKIRMISH_HALLS[h['owner']]) == h['type'] for h in halls
                   if h['owner'] in SKIRMISH_HALLS)
    halls_ok = bool(hall_types) and len(halls) == len(used) == SKIRMISH_PLAYERS and owners == SKIRMISH_OWNERS \
        and races_ok
    resources_ok = not (base['minerals']['missing'] or base['minerals']['extra'] or base['geysers']['missing']
                        or base['geysers']['extra'] or base['minerals']['amount_differs'])
    return {'start_locations': len(starts), 'occupied_by_gba_town_halls': len(used),
            'gba_town_halls': len(halls), 'gba_hall_owners': sorted(owners),
            'hall_types': {str(k): v for k, v in sorted(hall_types.items())}, 'halls_match_lobby_races': races_ok,
            'hall_offsets': offsets,
            'start_assignment': 'chosen by the lobby; the CHK assigns no start to a player',
            'gba_buildings': len(gba['buildings']), 'gba_units': len(gba['units']),
            'gba_identity': 'skirmish rows carry no raw type or owner (raw_type -1)',
            'minerals': base['minerals'], 'geysers': base['geysers'], 'gba_banks': gba['banks'],
            'verdict': ('differs' if not resources_ok  # a confirmed resource mismatch stands
                        else 'not compared' if not all(raw in hall_types for raw in SKIRMISH_HALLS.values())
                        else 'exact' if halls_ok else 'differs')}


def file_sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with open(path, 'rb') as f:
        for block in iter(lambda: f.read(1 << 20), b''):
            digest.update(block)
    return digest.hexdigest()


def binary_record(binary: Path, root: Path, work: Path) -> dict:
    """Check a census binary against the build record make wrote beside it
    (<binary>.inputs: the executable's SHA-256 and every generated and source
    input's SHA-256 at build time), as data only: no recorded command is run.
    A verified binary is copied into the work directory and that copy is what
    runs, so a rebuild during the report cannot change what was verified.
    A binary that fails is never run and its censuses read absent."""
    try:
        sidecar = Path(str(binary) + '.inputs')
        if not binary.is_file() or not sidecar.is_file():  # never open a FIFO or a device
            raise FileNotFoundError(2, 'not a regular file', str(sidecar if binary.is_file() else binary))
        record = json.loads(sidecar.read_text())
        recorded = record['binary_sha256']
        inputs = {**record['inputs']['generated'], **record['inputs']['sources']}
        if not isinstance(recorded, str) or not isinstance(inputs, dict) or not inputs:
            raise ValueError('malformed build record')
        fd, name = tempfile.mkstemp(prefix=f'{binary.name}.', suffix='.verified', dir=work)
        os.close(fd)
        snapshot = Path(name)  # unique per attempt, so one profile's check cannot replace another's copy
        shutil.copy2(binary, snapshot)
        if file_sha256(snapshot) != recorded:
            return {'error': f'{binary}: its .inputs record belongs to another build; rebuild with make'}
        changed = []
        for name, digest in sorted(inputs.items()):
            path = Path(name) if os.path.isabs(name) else root / name
            if not path.is_file() or file_sha256(path) != digest:
                changed.append(name)
        if changed:
            return {'error': f"{binary}: inputs changed since it was built ({', '.join(changed[:5])}); "
                             'rebuild with make'}
    except FileNotFoundError as error:
        return {'error': f'{binary}: no build record ({error.filename}); rebuild with make'}
    except (OSError, ValueError, KeyError, TypeError) as error:
        return {'error': f'{binary}: unreadable build record ({type(error).__name__}: {error})'}
    return {'binary_sha256': recorded, 'snapshot': snapshot, 'inputs_checked': len(inputs)}


# Steps the census runs on after the launch, with no input, so first-pass
# triggers (SetResources at 0-1 s) have run: 120 updates are 96 source frames,
# six seconds of mission time at the native 16 frames per elapsed second.
BANK_STEPS = 120


def run_census(binary: Path, mission: str, profile: str, work: Path) -> dict:
    trace = work / f'census-{profile}-{mission}.trace'
    proc = subprocess.run([str(binary.resolve()), '--scenario', 'census', '--mission', mission,
                           '--trace', str(trace), '--profile', profile, '--steps', str(BANK_STEPS)],
                          capture_output=True, text=True, timeout=300)
    if proc.returncode != 0:
        raise RuntimeError(f'{binary.name} --mission {mission} exited {proc.returncode}: '
                           f'{proc.stderr.strip()[-300:]}')
    census = parse_census_trace(trace.read_text())
    census['requested_steps'] = BANK_STEPS  # so a run a modal stopped early reads incomplete
    return census


def census_section(maps: dict, binaries: Path | None, extractor, absent_profiles=(), root: Path = ROOT) -> dict:
    with tempfile.TemporaryDirectory(prefix='sc64-parity-census-') as temp:
        missions = census_missions(maps, binaries, extractor, Path(temp), set(absent_profiles), root)
    verdicts = Counter(m['verdict'] for m in missions)
    return {'missions': missions, 'verdicts': dict(verdicts), 'bank_steps': BANK_STEPS}


def census_missions(maps: dict, binaries: Path | None, extractor, work: Path, absent_profiles=frozenset(),
                    root: Path = ROOT) -> list:
    from tools import sc64_import as importer
    missions = []
    hall_types: dict = {}
    verified: dict = {}
    for mission, label, path in MISSIONS + ((None,) + SKIRMISH,):
        entry = {'mission': mission, 'label': label, 'chk': path,
                 'profile': SKIRMISH_PROFILE if mission is None else PROFILE_BY_RACE[label[0]]}
        if path not in maps:
            entry.update(verdict='absent', note='CHK not found in the ROM')
            missions.append(entry)
            continue
        info, chk = maps[path]
        sections = importer.sections(chk, extractor)
        rows = unit_rows(sections.get(b'UNIT', b'')) + doodad_units(sections.get(b'THG2', b''))
        triggers = importer.trigger_rows(sections.get(b'TRIG', b''), sections.get(b'STR ', b''), extractor)
        bank_rows = start_banks(triggers, sections.get(b'FORC', b''), rows, sections.get(b'OWNR', b''))
        entry.update(title=info.name, rom_placements=len(rows),
                     rom_unit_doodads=sum(r['section'] == 'THG2' for r in rows),
                     rom_start_locations=sum(r['type'] == START_LOCATION for r in rows))
        binary = binaries / f"{entry['profile']}-fast-trace" if binaries else None
        if binary:
            try:
                binary = binary.resolve()  # checked and read through the same path
            except (OSError, RuntimeError) as error:
                entry.update(verdict='absent', note=f'census binary path does not resolve: {error}',
                             rom_start_banks=bank_rows)
                missions.append(entry)
                continue
        if binary and not binary.is_relative_to(root.resolve() / 'build'):
            entry.update(verdict='absent', note=f'census binary {binary} is not in this checkout\'s build tree',
                         rom_start_banks=bank_rows)
            missions.append(entry)
            continue
        if entry['profile'] in absent_profiles:
            entry.update(verdict='absent', note=f"the {entry['profile']} census binary failed to build",
                         rom_start_banks=bank_rows)
            missions.append(entry)
            continue
        if not binary or not binary.is_file():
            entry.update(verdict='absent', note=f'GBA census binary {binary} not built',
                         rom_start_banks=bank_rows)
            missions.append(entry)
            continue
        record = verified.get(entry['profile'])
        if record is None:
            record = verified[entry['profile']] = binary_record(binary, root, work)
        if 'error' in record:
            entry.update(verdict='absent', note=f"census binary not trusted: {record['error']}",
                         rom_start_banks=bank_rows)
            missions.append(entry)
            continue
        entry['gba_binary_sha256'] = record['binary_sha256']
        try:
            gba = run_census(record['snapshot'], 'skirmish' if mission is None else str(mission),
                             entry['profile'], work)
        except (RuntimeError, ValueError, OSError, subprocess.TimeoutExpired) as error:
            entry.update(verdict='absent', note=f'GBA census failed: {error}', rom_start_banks=bank_rows)
            missions.append(entry)
            continue
        entry.update(gba_step=gba['step'], gba_frame=gba['frame'], gba_last_step=gba['last_step'])
        for kind in ('units', 'buildings'):  # engine type of each raw town hall, per profile
            for g in gba[kind]:
                if g['raw_type'] in TOWN_HALLS:
                    hall_types.setdefault(entry['profile'], {})[g['raw_type']] = g['type']
        if mission is None:
            entry.update(skirmish_census(rows, gba, hall_types.get(entry['profile'], {})))
            entry['rom_start_banks'] = bank_rows
        else:
            entry.update(compare_census(rows, gba))
            owners = owner_map(gba)
            entry['banks'] = compare_banks(bank_rows, gba, owners)
            entry['banks_unexplained'] = unexplained_banks(bank_rows, gba, owners)
            entry['gba_banks'] = gba['banks']
        missions.append(entry)
    return missions


# ---- 2. iscript sequences ------------------------------------------------

BANK_SECTIONS = {'walk': ('entries',), 'attack': ('actors',), 'death': ('actors', 'images'),
                 'construction': ('stages',)}


def load_manifest(path: Path, header: Path, sections=()) -> tuple[dict | None, str]:
    """A baked table's manifest, trusted only when it pins the header beside
    it; returns (manifest or None, status)."""
    if not path.is_file() or not header.is_file():
        return None, f'absent ({path.parent})'
    try:
        manifest = json.loads(path.read_text())
        header_bytes = header.read_bytes()
        digest = hashlib.sha256(header_bytes).hexdigest()
    except (OSError, ValueError) as error:
        return None, f'unavailable: {path} or its header does not read ({error})'
    if not isinstance(manifest, dict):
        return None, f'unavailable: {path} is not a JSON object'
    def field(key):
        value = manifest.get(key)
        return value if isinstance(value, dict) else {}
    pinned = manifest.get('header_sha256') or field('generator').get('header_sha256') \
        or field('generated_header').get('sha256')
    if not isinstance(pinned, str) or not pinned:
        return None, f'unavailable: {path} pins no header digest'
    if pinned != digest:
        return None, f'stale: {path} pins header {pinned[:12]}, header is {digest[:12]}'
    bad = [key for key in sections if not isinstance(manifest.get(key), list) or not manifest.get(key)]
    if bad:
        return None, f"unavailable: {path} has no {', '.join(bad)} entries"
    # The readers parse these bytes, the ones the digest covered.
    manifest['_header_text'] = header_bytes.decode('ascii', 'replace')
    return manifest, 'ok'


ATTACK_STEP = re.compile(r'sc64_unit_attack_raw(\d+)_(ground|air)_full_steps\[\d+\] = \{(.*?)\n\};', re.S)
ATTACK_ROW = re.compile(r'\{(\d+)u,\{&sc64_unit_attack_frame_(\d+)')
CONSTRUCTION_STEPS = re.compile(
    r'sc64_construction_raw_(\d+)_stage_(\d+)_layer_0_steps\[\] = \{(.*?)\};\s*'
    r'static const SC64ConstructionAnimDesc sc64_construction_raw_\1_stage_\2_layer_0_animation = \{\s*'
    r'\w+, (\d+)u, (\d+)u, (\d+)u', re.S)
CONSTRUCTION_ROW = re.compile(r'\{ (\d+)u, (\d+)u \}')


def engine_attack_frames(header_text: str) -> dict:
    """(raw, 'ground'|'air') -> per-tick heading-0 frame ids of the full
    attack timeline (bank frame numbers, comparable only up to relabelling)."""
    out = {}
    for raw, mode, body in ATTACK_STEP.findall(header_text):
        rows = [(int(t), int(f)) for t, f in ATTACK_ROW.findall(body)]
        out[(int(raw), mode)] = rows
    return out


def engine_construction(header_text: str) -> dict:
    out = {}
    for raw, stage, body, count, loop, frames in CONSTRUCTION_STEPS.findall(header_text):
        out[(int(raw), int(stage))] = {'steps': [(int(f), int(d)) for f, d in CONSTRUCTION_ROW.findall(body)],
                                       'loop_start': int(loop), 'frame_count': int(frames)}
    return out


def sounds_of(seq) -> list:
    return sorted({i for _, ids in seq['sounds'] for i in ids}) if seq else []


def compare_walk(scripts: Scripts, entry: dict) -> dict:
    image = entry['image_id'] if 'image_id' in entry else scripts.unit_image(entry['unit_id'])
    seq = scripts.sequence(image, 'Walking')
    if 'steps' not in entry:  # the walk importer left this unit out
        row = {'animation': 'Walking', 'image': image, 'verdict': 'absent',
               'note': f"no baked walk cycle (walk importer: {entry.get('skipped', 'skipped')})"}
        if seq:
            row.update(rom_frames=[f for f, _ in collapse(seq.get('cycle') or seq['steps'])],
                       sounds=sounds_of(seq))
        return row
    engine = [(s['tick'], s['pose']) for s in entry['steps']]
    cycle = entry.get('cycle_ticks') or (engine[-1][0] + 1 if engine else 0)
    eng_waits = [b[0] - a[0] for a, b in zip(engine, engine[1:])] + ([cycle - engine[-1][0]] if engine else [])
    row = {'animation': 'Walking', 'image': image, 'engine_frames': [p for _, p in engine],
           'engine_waits': eng_waits}
    if not seq:
        return dict(row, verdict='absent', note='ROM image has no Walking animation')
    if seq['end'] != 'loop' or not seq.get('cycle'):
        return dict(row, verdict='not compared', rom_end=seq['end'], sounds=sounds_of(seq),
                    note=f"ROM Walking path has no steady loop ({seq['end']}); the cycle is not compared")
    steps = collapse(seq['cycle'])
    directional = scripts.directional(image)
    rom_frames = [(f // 17 if directional and f is not None else f) for f, _ in steps]
    rom_waits = [w for _, w in steps]
    row.update(rom_frames=rom_frames, rom_waits=rom_waits, rom_end=seq['end'], sounds=sounds_of(seq))
    # The engine cycle may start at another step of the same loop.
    rotations = [k for k in range(len(rom_frames))
                 if rom_frames[k:] + rom_frames[:k] == row['engine_frames']]
    matching = next((k for k in rotations if rom_waits[k:] + rom_waits[:k] == eng_waits), None)
    if not rotations:
        row.update(verdict='differs', note='frame order differs')
    elif matching is None:
        row.update(verdict='differs', note='per-frame wait differs')
    else:
        row['verdict'] = 'match'
        if matching:
            row['note'] = f'engine cycle starts at ROM step {matching}'
    return row


def compare_attack(scripts: Scripts, raw: int, actor: dict, frames: dict) -> list:
    rows = []
    if 'root_image_id' not in actor or 'modes' not in actor:
        reason = actor.get('status') or actor.get('reason') or 'unsupported'
        return [{'animation': name, 'verdict': 'absent', 'note': f'attack bank entry has no timeline ({reason})'}
                for name in ('GndAttkInit', 'AirAttkInit')]
    for mode in actor['modes']:
        kind = 'air' if mode['target_air'] else 'ground'
        animation = mode['animation']
        image = actor['root_image_id']
        seq = scripts.sequence(image, animation)
        row = {'animation': animation, 'image': image, 'engine_status': mode.get('status'),
               'engine_ticks': mode.get('full_duration_source_ticks')}
        timeline = frames.get((raw, kind))
        if not seq:
            rows.append(dict(row, verdict='absent', note=f'ROM image has no {animation}'))
            continue
        row['sounds'] = sounds_of(seq)
        if not timeline:
            rows.append(dict(row, verdict='absent', note='no baked attack timeline for this mode'))
            continue
        starts = [t for t, _ in timeline]
        if starts != sorted(set(starts)) or starts[0] != 0:
            rows.append(dict(row, verdict='differs', note=f'engine timeline ticks {starts} are not increasing from 0'))
            continue
        # The declared duration is an exclusive bound: the runtime never shows
        # a row keyed at or after it.
        ticks = mode.get('full_duration_source_ticks') or starts[-1] + 1
        if not isinstance(ticks, int) or not 0 < ticks <= MAX_ATTACK_TICKS:  # before any expansion
            rows.append(dict(row, verdict='absent', note=f'engine timeline of {ticks} ticks is implausible'))
            continue
        timeline = [(t, f) for t, f in timeline if t < ticks]
        bounds = [t for t, _ in timeline][1:] + [ticks]
        eng = [f for (t, f), stop in zip(timeline, bounds) for _ in range(stop - t)]
        # The attack path ends at ignorerest; the engine's recovery is the
        # ToIdle animation, as if the order ended with the path.
        path_ticks = sum(low for _, low, _ in seq['steps'])
        idle_name = 'AirAttkToIdle' if mode['target_air'] else 'GndAttkToIdle'
        idle = scripts.sequence(image, idle_name)
        # ToIdle up to its first goto (which leads into the idle loop); the
        # frame shown at that goto stays after it.
        tail = idle['steps'][:idle['first_goto'] if idle['first_goto'] is not None else len(idle['steps'])] \
            if idle and seq['end'] == 'hold' else []
        # A recovery tick before its first playfram still shows the attack's
        # last frame.
        carried, tail = seq['last_frame'], list(tail)
        for i, (f, lo, hi) in enumerate(tail):
            if f is not None:
                break
            tail[i] = (carried, lo, hi)
        finite = path_ticks + sum(low for _, low, _ in tail) if seq['end'] in ('hold', 'end') else None
        if seq['end'] != 'hold':
            hold = None
        elif idle and idle['first_goto'] is not None:
            # The frame ToIdle shows when it jumps to the idle loop; a ToIdle
            # that set no frame by then still shows the attack's last one.
            hold = idle['goto_frame'] if idle['goto_frame'] is not None else seq['last_frame']
        elif idle:
            # ToIdle never jumps: the frame it ends on, or the attack's.
            hold = idle['last_frame'] if idle['last_frame'] is not None else seq['last_frame']
        else:
            hold = seq['last_frame']  # no ToIdle: ignorerest keeps the attack frame
        path = list(seq['steps']) + tail
        cycle_ticks = sum(low for _, low, _ in seq.get('cycle') or [])
        if seq['end'] == 'loop' and cycle_ticks:  # a looping path repeats its steady cycle
            short = ticks - sum(low for _, low, _ in path)
            path += list(seq['cycle']) * max(0, -(-short // cycle_ticks))
        rom = per_tick(path, ticks, hold)
        if seq['end'] == 'hold' and len(rom) < ticks:  # ignorerest holds a frame we may not know
            rom += [hold] * (ticks - len(rom))
        row.update(rom_ticks=path_ticks, rom_end=seq['end'], rom_frames=[f for f, _ in collapse(seq['steps'])],
                   rom_recovery=idle_name if tail else None)
        notes = []
        keep = []
        row['rom_finite_ticks'] = finite
        if len(rom) < ticks:
            row['verdict'] = 'differs'
            notes.append(f'ROM path holds {len(rom)} ticks and ends ({seq["end"]}); engine {ticks}')
        elif finite is not None and finite > ticks:
            row['verdict'] = 'differs'
            notes.append(f'engine timeline ({ticks} ticks) is shorter than the ROM path and recovery ({finite})')
        else:
            # A tick before the first playfram shows the frame the previous
            # animation left; it is not compared.
            keep = [i for i, f in enumerate(rom) if f is not None]
            rp, ep = relabel([rom[i] for i in keep]), relabel([eng[i] for i in keep])
            row.update(rom_pattern=rp, engine_pattern=ep, uncompared_ticks=ticks - len(keep))
            if not keep:
                row['verdict'] = 'not compared'
                notes.append('no ROM frame is known on any engine tick (the path never plays a frame)')
            elif rp == ep and len(keep) < ticks:
                # Agreement on the known ticks only: a tick that shows the frame
                # the previous animation left cannot be checked.
                row['verdict'] = 'not compared'
                notes.append('the known ticks agree up to relabelling (heading 0, root image)')
            elif rp == ep:
                row['verdict'] = 'match'
                notes.append('frame order and waits agree up to relabelling (heading 0, root image)')
            elif len(actor.get('components', [])) > 1 and refines(ep, rp):
                # The engine bakes composites: an animated overlay over a
                # repeating root frame gives more distinct engine frames, so
                # the bank cannot show the root pose order.
                row['verdict'] = 'not compared'
                notes.append('the engine composite has more distinct frames than the root image (overlays); '
                             'root pose order cannot be read from the bank')
            else:
                first = keep[next(i for i, (x, y) in enumerate(zip(rp, ep)) if x != y)]
                row['verdict'] = 'differs'
                notes.append(f'first difference at tick {first}')
        if tail and ticks > path_ticks:
            notes.append(f'ticks {path_ticks}-{ticks - 1} compare {idle_name}, assuming the order ends with the path')
        if keep and len(keep) < ticks:
            notes.append(f'{ticks - len(keep)} tick(s) before the first playfram not compared')
        if any(low != high for _, low, high in seq['steps']):
            notes.append('the ROM path has a waitrand; its low bound is used')
        row['note'] = '; '.join(notes)
        rows.append(row)
    return rows


def compare_death(scripts: Scripts, raw: int, actor: dict, images: dict) -> list:
    """Each burst layer of the engine's death record against the iscript of
    its image: Death for the unit's own image, Init for a spawned explosion."""
    rows = []
    own = actor['image_id']
    for layer in actor['layers']:
        image = layer['image_id']
        animation = 'Death' if image == own else 'Init'
        seq = scripts.sequence(image, animation)
        row = {'animation': 'Death' if image == own else 'Death (spawned image)', 'image': image,
               'rom_animation': animation, 'remnant': layer.get('remnant', False)}
        if not seq:
            rows.append(dict(row, verdict='absent', note=f'ROM image {image} has no {animation}'))
            continue
        steps = collapse(seq['steps'])
        rom_frames = [f for f, _ in steps if f is not None]
        # The burst starts at the first wait that shows a frame; a waitrand
        # gives a range of durations.
        framed = seq['steps'][next((i for i, st in enumerate(seq['steps']) if st[0] is not None),
                                   len(seq['steps'])):]
        rom_lo, rom_hi = sum(lo for _, lo, _ in framed), sum(hi for _, _, hi in framed)
        eng_frames = images.get(image, {}).get('source_frames')
        eng_ticks = layer['end'] - layer['start']
        row.update(rom_frames=rom_frames, rom_ticks=rom_lo if rom_lo == rom_hi else [rom_lo, rom_hi],
                   rom_end=seq['end'], sounds=sounds_of(seq), engine_frames=eng_frames, engine_ticks=eng_ticks)
        notes = []
        if eng_frames is None:
            notes.append('engine image has no frame list')
        elif sorted(set(eng_frames)) != sorted(set(rom_frames)):
            notes.append('frame set differs')
        elif eng_frames != rom_frames:
            notes.append('same frames; the engine record lists them in index order, so play order is unchecked')
        frames_bad = any(n.startswith(('frame', 'engine image')) for n in notes)
        if layer.get('remnant'):
            notes.append('remnant layer: duration is the engine cap, not compared')
            row['verdict'] = 'differs' if frames_bad else 'not compared'
        elif seq['end'] != 'end':
            notes.append(f'ROM path does not end ({seq["end"]}); duration not compared')
            row['verdict'] = 'differs' if frames_bad else 'not compared'
        else:
            if not rom_lo <= eng_ticks <= rom_hi:
                rng = rom_lo if rom_lo == rom_hi else f'{rom_lo}-{rom_hi}'
                notes.append(f'duration differs (ROM {rng}, engine {eng_ticks} source ticks)')
            elif rom_lo != rom_hi:
                notes.append(f'ROM waitrand: engine duration {eng_ticks} lies in the ROM range {rom_lo}-{rom_hi}')
            row['verdict'] = 'differs' if frames_bad or any(n.startswith('duration') for n in notes) else 'match'
        if notes:
            row['note'] = '; '.join(notes)
        rows.append(row)
    return rows


def compare_construction(scripts: Scripts, stage: dict, baked: dict) -> dict:
    """One construction stage: frames and waits step by step, and how the
    program ends (loop entry, or the frame it holds). Steps are compared one
    to one when both programs have the same number; otherwise runs of one
    frame are merged first and the loop entry is compared on merged runs."""
    raw, number, animation, image = stage['raw_unit_id'], stage['stage'], stage['animation'], stage['image_id']
    row = {'animation': f'construction {animation} (stage {number})', 'image': image}
    seq = scripts.sequence(image, animation)
    engine = baked.get((raw, number))
    if not seq:
        return dict(row, verdict='absent', note=f'ROM image has no {animation}')
    rom_raw = [(f, lo if lo == hi else [lo, hi]) for f, lo, hi in seq['steps']]
    row.update(rom_steps=rom_raw, rom_end=seq['end'], sounds=sounds_of(seq))
    if not engine:
        return dict(row, verdict='absent', note='no baked construction program for this stage')
    eng_raw = list(engine['steps'])
    eng_loop = None if engine['loop_start'] == HOLD_LAST else engine['loop_start']
    rom_loop = seq['loop'] if seq['end'] == 'loop' else None
    # A loop's steady cycle can show another frame where it re-enters (the
    # frame the body leaves), so the first pass and the steady cycle are
    # compared separately: the ROM's with the engine program from its start,
    # and from its loop entry.
    cycle = list(seq.get('cycle') or []) if rom_loop is not None else []
    notes = []
    eng_steps = [(f, d, d) for f, d in eng_raw]
    unknown_frame = any(f is None for f, _, _ in seq['steps'])
    if len(rom_raw) == len(eng_raw):
        first = (rom_raw, eng_raw)
        steady = ([(f, lo if lo == hi else [lo, hi]) for f, lo, hi in cycle], eng_raw[eng_loop:]) \
            if cycle and eng_loop is not None else None
    else:
        inside = [name for name, steps, loop in (('ROM', seq['steps'], rom_loop), ('engine', eng_steps, eng_loop))
                  if loop is not None and 0 < loop < len(steps) and steps[loop - 1][0] == steps[loop][0]]
        if inside:
            notes.append(f"loop entry inside a run of one frame ({', '.join(inside)}); "
                         'the steps cannot be merged to compare')
        first = (collapse(seq['steps']), collapse(eng_steps))
        steady = (collapse(cycle), collapse(eng_steps[eng_loop:])) if cycle and eng_loop is not None else None
        rom_loop = collapsed_index(seq['steps'], rom_loop) if rom_loop is not None else None
        eng_loop = collapsed_index(eng_steps, eng_loop) if eng_loop is not None else None
        notes.append('step counts differ; runs of one frame merged')
    row.update(engine_steps=eng_raw, engine_loop=engine['loop_start'], rom_loop=seq['loop'],
               rom_cycle=[(f, lo if lo == hi else [lo, hi]) for f, lo, hi in cycle] or None)
    eng = first[1]
    for label, pair in (('first pass', first), ('steady cycle', steady)):
        if pair is None:
            continue
        rom, engine_part = pair
        if len(rom) != len(engine_part) or any(r is not None and r != e for (r, _), (e, _) in zip(rom, engine_part)):
            notes.append(f'{label}: frame order differs')
            continue
        waits = [(w, d) for (_, w), (_, d) in zip(rom, engine_part)]
        if any(w != d for w, d in waits):
            deltas = [d - w if isinstance(w, int) else f'{d} vs {w}' for w, d in waits]
            notes.append(f'{label}: per-frame wait differs (engine minus ROM ticks per frame: {deltas})')
    if seq['end'] == 'loop' and eng_loop is None:
        notes.append(f'ROM loops back to step {rom_loop}; engine holds its last frame')
    elif seq['end'] in ('end', 'hold') and eng_loop is not None:
        notes.append(f'ROM stops ({seq["end"]}); engine loops back to step {eng_loop}')
    elif seq['end'] == 'loop' and eng_loop != rom_loop:
        notes.append(f'loop entry differs (ROM step {rom_loop}, engine step {eng_loop})')
    elif seq['end'] == 'hold' and seq['last_frame'] is None:
        notes.append('ROM path ends by holding an inherited frame; termination not compared')
    elif seq['end'] in ('end', 'hold') and eng and seq['last_frame'] != eng[-1][0]:
        notes.append(f"held frame differs (ROM {seq['last_frame']}, engine {eng[-1][0]})")
    elif seq['end'] == 'end':
        notes.append('ROM image ends (it is removed); engine holds its last frame')
    elif seq['end'] not in ('loop', 'end', 'hold'):
        notes.append(f'ROM path ends by {seq["end"]}; termination not compared')
    blocking = [n for n in notes if not n.startswith(('ROM path ends by', 'step counts differ'))]
    unknown_end = any(n.startswith('ROM path ends by') for n in notes)
    if unknown_frame and not blocking:
        notes.append('a ROM step shows the frame the previous stage left; it is not compared')
        unknown_end = True
    row['verdict'] = 'differs' if blocking else 'not compared' if unknown_end else 'match'
    if notes:
        row['note'] = '; '.join(notes)
    return row


def iscript_section(scripts: Scripts, root: Path, banks: dict, names: dict) -> dict:
    status = {}
    walk, status['walk'] = load_manifest(*banks['walk'], BANK_SECTIONS['walk'])
    attack, status['attack'] = load_manifest(*banks['attack'], BANK_SECTIONS['attack'])
    death, status['death'] = load_manifest(*banks['death'], BANK_SECTIONS['death'])
    construction, status['construction'] = load_manifest(*banks['construction'], BANK_SECTIONS['construction'])
    units: dict[int, dict] = {}

    def unit(raw):
        if type(raw) is not int or not 0 <= raw < UNIT_COUNT:  # bool is not a unit id
            raise ValueError(f'raw unit id {raw!r} out of range')
        return units.setdefault(raw, {'raw': raw, 'name': names.get(raw, f'raw {raw}'), 'rows': []})

    covered: set = set()  # (raw, image, animation) with a parsed construction program

    def bank_rows(name, build):
        """Compare one bank; a malformed or unreadable bank is reported
        unavailable and drops only its own rows."""
        snapshot = {raw: list(u['rows']) for raw, u in units.items()}
        try:
            build()
        except (OSError, ValueError, TypeError, KeyError, IndexError, AttributeError) as error:
            for raw in list(units):
                if raw in snapshot:
                    units[raw]['rows'] = snapshot[raw]
                else:
                    del units[raw]
            status[name] = f'unavailable: malformed bank ({type(error).__name__}: {error})'
            return {}

    def walk_rows():
        for entry in (walk or {}).get('entries', []):
            unit(entry['unit_id'])['rows'].append(compare_walk(scripts, entry))

    def attack_rows():
        frames = engine_attack_frames(attack['_header_text']) if attack else {}
        for actor in (attack or {}).get('actors', []):
            unit(actor['raw_unit'])['rows'].extend(compare_attack(scripts, actor['raw_unit'], actor, frames))

    def death_rows():
        images = {i['image_id']: i for i in (death or {}).get('images', [])}
        for actor in (death or {}).get('actors', []):
            unit(actor['raw_unit_id'])['rows'].extend(compare_death(scripts, actor['raw_unit_id'], actor, images))

    def construction_rows():
        programs = engine_construction(construction['_header_text']) if construction else {}
        found = set()
        for stage in (construction or {}).get('stages', []):
            row = compare_construction(scripts, stage, programs)
            unit(stage['raw_unit_id'])['rows'].append(row)
            if row['verdict'] != 'absent':
                found.add((stage['raw_unit_id'], stage['image_id'], stage['animation']))
        covered.update(found)  # only once the whole bank has been read

    for name, build in (('walk', walk_rows), ('attack', attack_rows), ('death', death_rows),
                        ('construction', construction_rows)):
        bank_rows(name, build)
    missing = {name: state for name, state in status.items() if state != 'ok'}
    for raw in range(UNIT_COUNT):  # every unit with an animation a bank models appears, rows or not
        present = scripts.animations(scripts.unit_image(raw))
        if any(name in present for name in MODELLED) or scripts.construction[raw]:
            unit(raw)
    stages_by_raw: dict = {}
    if 'construction' in missing:  # the stages the construction importer would bake
        for st in source_stage_inputs(scripts.tables, scripts.payload):
            stages_by_raw.setdefault(st['raw_unit_id'], []).append(st)
    for raw in range(UNIT_COUNT) if missing else ():
        image = scripts.unit_image(raw)
        present = scripts.animations(image)
        wanted = []
        if 'walk' in missing and 'Walking' in present:
            wanted.append(('Walking', image, 'walk'))
        if 'attack' in missing:
            wanted += [(name, image, 'attack') for name in ('GndAttkInit', 'AirAttkInit') if name in present]
        if 'death' in missing and 'Death' in present:
            wanted.append(('Death', image, 'death'))
        if 'construction' in missing:
            wanted += [(f"construction {st['animation']} (stage {st['stage']})", st['image_id'], 'construction')
                       for st in stages_by_raw.get(raw, [])]
        for animation, img, name in wanted:
            unit(raw)['rows'].append({'animation': animation, 'image': img, 'verdict': 'absent',
                                      'note': f'{name} bank {missing[name]}'})
    if status['construction'] != 'ok':
        construction = None
    # Construction stages run on the construction image, which may not be the
    # unit's own: coverage is per (unit, image, animation).
    modelled_construction = covered
    for raw, entry in units.items():
        image = scripts.unit_image(raw)
        present = scripts.animations(image)
        # Only a baked program counts as modelled: a row that reports no bake
        # (verdict absent) leaves the animation in the not-modelled list.
        compared = {r['animation'].split(' ')[0] if not r['animation'].startswith('construction') else 'construction'
                    for r in entry['rows'] if r['verdict'] != 'absent'}
        missing = []
        for name in present:
            if name in MODELLED and name in compared:
                continue
            if (raw, image, name) in modelled_construction:
                continue
            missing.append(name)
        entry['image'] = image
        entry['not_modelled'] = missing
        entry['rom_sounds_by_animation'] = {
            name: sorted({i for _, ids in (scripts.sequence(image, name) or {'sounds': []})['sounds'] for i in ids})
            for name in present}
        entry['rom_sounds_by_animation'] = {k: v for k, v in entry['rom_sounds_by_animation'].items() if v}
    rows = [r for u in units.values() for r in u['rows']]
    counts = Counter(r['verdict'] for r in rows)
    by_animation = {}
    for r in rows:
        key = r['animation'].split(' (')[0]
        by_animation.setdefault(key, Counter())[r['verdict']] += 1
    not_modelled = Counter(name for u in units.values() for name in u['not_modelled'])
    return {'banks': status, 'units': [units[k] for k in sorted(units)], 'counts': dict(counts),
            'by_animation': {k: dict(v) for k, v in sorted(by_animation.items())},
            'not_modelled': dict(sorted(not_modelled.items(), key=lambda kv: -kv[1])),
            'limits': ['linear iscript paths: unconditional goto only, no conditional branch or call',
                       'attack frames are compared for heading 0 of the root image, up to relabelling, '
                       'because the engine bank numbers its own frames; recovery is compared with '
                       'Gnd/AirAttkToIdle as if the order ended with the path',
                       'death records list each image\'s frames as a set in index order: the frame set and '
                       'burst duration are compared, play order is not',
                       'overlays, shadows and spawned images are not compared frame by frame',
                       'waitrand is a range in the ROM; the engine tables hold one value']}


# ---- 3. sound ids --------------------------------------------------------

SOUND_KINDS = ('ready', 'yes', 'what', 'annoyed', 'attack', 'death')
ENGINE_KINDS = ('ready', 'what', 'yes', 'attack', 'death', 'built')  # game_audio.h emits these


def manifest_blob(audio_dir: Path):
    """The blob SHA-256 the bank's manifest pins, or None."""
    path = audio_dir / 'sc64_audio_manifest.json'
    if not path.is_file():  # never open a FIFO or a device
        return None
    try:
        manifest = json.loads(path.read_text())
        return manifest['output']['blob_sha256']
    except (OSError, ValueError, KeyError, TypeError):
        return None


def header_array(text: str, name: str) -> list[int]:
    match = re.search(r'\b' + re.escape(name) + r'\[\d+\][^=]*= \{(.*?)\};', text, re.S)
    if not match:
        raise ValueError(f'{name} not found in the audio header')
    return [int(v) for v in re.findall(r'\d+', match.group(1))]


def engine_unit_sounds(header_text: str, id_map: dict) -> dict:
    """raw -> kind -> sfxdata ids the bank plays for that unit and kind.

    The ids come from the pack's id map; each is checked against the
    header's own per-unit bank indices, so the map must describe exactly the
    table the engine compiles."""
    enum = re.search(r'enum \{ (SC64_AUDIO_[^}]*)\}', header_text)
    kinds = [k.lower() for k in re.findall(r'SC64_AUDIO_([A-Z]+)', enum.group(1))] if enum else []
    if kinds != list(id_map.get('kinds', [])):
        raise ValueError(f'audio kinds {kinds} differ from the id map {id_map.get("kinds")}')
    first = header_array(header_text, 'sc64_audio_unit_first')
    count = header_array(header_text, 'sc64_audio_unit_count')
    flat = header_array(header_text, 'sc64_audio_unit_sound')
    def sound_list(value):
        return isinstance(value, list) and all(type(v) is int and 0 < v < SOUND_COUNT for v in value)
    if not isinstance(id_map.get('bank'), list) or not all(sound_list(ids) for ids in id_map['bank']) or \
            not isinstance(id_map.get('units'), list) or \
            not all(isinstance(u, dict) and all(sound_list(u.get(k)) for k in kinds) for u in id_map['units']):
        raise ValueError('the id map holds malformed sound lists')
    index_of = {sound: i for i, ids in enumerate(id_map['bank']) for sound in ids}
    if len(first) != len(kinds) * len(id_map['units']):
        raise ValueError('the id map and the header list different unit counts')
    out = {}
    for raw, sets in enumerate(id_map['units']):
        out[raw] = {}
        for k, kind in enumerate(kinds):
            row = raw * len(kinds) + k
            indices = flat[first[row]:first[row] + count[row]]
            ids = sets[kind]
            if [index_of.get(sound) for sound in ids] != indices:
                raise ValueError(f'id map row {raw}/{kind} {ids} does not give the header indices {indices}')
            out[raw][kind] = ids
    return out


def sound_verdict(rom: list, audible: set, gba: list) -> str:
    want = [i for i in rom if i in audible]
    have = set(gba)
    if not want and not have:
        return 'none' if not rom else 'silent in ROM'
    if have - set(rom):
        return 'extra'
    if not have:
        return 'absent'
    return 'match' if have == set(want) else 'partial'


def sound_section(scripts: Scripts, units: list, audio_dir: Path, lengths: list, names: dict) -> dict:
    header = audio_dir / 'sc64_audio_generated.h'
    side = audio_dir / 'sc64_audio_sound_ids.json'
    audible = {i for i, n in enumerate(lengths) if n > 8}
    status = 'ok'
    engine = None
    if not header.is_file():
        status = f'absent: no audio bank at {audio_dir}'
    elif not side.is_file():
        status = (f'absent: {side.name} not found; regenerate the bank with tools/sc64_audio_pack.py '
                  'so it records the sfxdata id behind each bank index')
    else:
        try:
            id_map = json.loads(side.read_text())
        except (OSError, ValueError):
            id_map = None
        if not isinstance(id_map, dict):
            id_map = {}  # reads as stale below: it pins no header
        blob = audio_dir / 'sc64_audio_generated.bin'
        try:
            header_bytes = header.read_bytes()
            blob_digest = hashlib.sha256(blob.read_bytes()).hexdigest() if blob.is_file() else None
        except OSError:
            header_bytes, blob_digest = b'', None
        digest = hashlib.sha256(header_bytes).hexdigest()
        if id_map.get('header_sha256') != digest:
            status = f'stale: {side.name} describes another header ({str(id_map.get("header_sha256"))[:12]})'
        elif not blob_digest or not id_map.get('blob_sha256'):
            status = f'absent: the bank blob or its digest in {side.name} is missing; regenerate the bank'
        elif id_map.get('blob_sha256') != blob_digest:
            status = f'stale: {side.name} and the blob come from different generations'
        elif manifest_blob(audio_dir) != blob_digest:
            status = 'stale: the manifest pins another blob than the one beside it'
        else:
            try:  # parse the same bytes the digest covers
                engine = engine_unit_sounds(header_bytes.decode('ascii'), id_map)
            except (ValueError, TypeError, KeyError, IndexError, AttributeError) as error:
                status = f'stale: malformed id map or header ({type(error).__name__}: {error})'
                engine = None
    if engine is not None:  # units the bank voices count even with no animation bank
        units = sorted(set(units) | {raw for raw, kinds in engine.items() if any(kinds.values())})
    rows = []
    for raw in units:
        rom = scripts.rom_sounds(raw)
        entry = {'raw': raw, 'name': names.get(raw, f'raw {raw}'), 'kinds': {}}
        for kind in SOUND_KINDS:
            gba = None if engine is None or kind not in ENGINE_KINDS else engine.get(raw, {}).get(kind, [])
            if engine is None:
                verdict = 'absent (no bank id map)' if rom[kind] else 'none'
            elif kind not in ENGINE_KINDS:
                verdict = 'absent (engine has no annoyed kind)' if any(i in audible for i in rom[kind]) else \
                    'none' if not rom[kind] else 'silent in ROM'
            else:
                verdict = sound_verdict(rom[kind], audible, gba)
            entry['kinds'][kind] = {'rom': rom[kind], 'rom_silent': [i for i in rom[kind] if i not in audible],
                                    'gba': gba, 'verdict': verdict}
        rows.append(entry)
    counts = Counter(k['verdict'] for r in rows for k in r['kinds'].values())
    by_kind = {kind: dict(Counter(r['kinds'][kind]['verdict'] for r in rows)) for kind in SOUND_KINDS}
    return {'bank': status, 'units': rows, 'counts': dict(counts), 'by_kind': by_kind}


# ---- report ----------------------------------------------------------------

MISMATCH_SOUND = ('partial', 'absent', 'extra')


def resource_text(m: dict) -> str:
    mi, ge = m['minerals'], m['geysers']
    return (f"minerals {mi['placed']}/{mi['rom']} (missing {len(mi['missing'])}, extra {len(mi['extra'])}, "
            f"amount differs {len(mi['amount_differs'])}, GBA cap {mi['cap']}) "
            f"geysers {ge['placed']}/{ge['rom']} (missing {len(ge['missing'])}, extra {len(ge['extra'])})")


def report_lines(result: dict) -> list[str]:
    lines = []
    census = result['census']
    lines.append('== opening censuses: ' + ', '.join(f'{k} {v}' for k, v in sorted(census['verdicts'].items())))
    for m in census['missions']:
        head = f"census {m['label']:<4} {m['chk']} {m['profile']:<11} {m['verdict']:<11}"
        if 'counts' in m:
            c = m['counts']
            head += (f" ROM {m['rom_actors']} GBA {m['gba_actors']} matched {m['matched']}"
                     f" collision {c['collision']} moved {c['moved']} cap-dropped {c['cap-dropped']}"
                     f" missing {c['missing']} extra {c['extra']}"
                     f" | {resource_text(m)}")
        elif 'start_locations' in m:
            head += (f" starts {m['start_locations']} GBA halls on starts {m['occupied_by_gba_town_halls']}"
                     f" | {resource_text(m)}")
        if m.get('note'):
            head += f" ({m['note']})"
        lines.append(head)
        for cls in ('collision', 'moved', 'cap-dropped', 'missing', 'extra'):
            rows = m.get('rows', {}).get(cls, [])
            if rows:
                types = Counter((r['type'], r['owner']) for r in rows)
                lines.append(f"    {cls}: " + ', '.join(f"type {t} owner {o} x{n}" for (t, o), n in sorted(types.items())))
        for b in m.get('banks', []):
            lines.append(f"    bank player {b['player']} (engine owner {b['engine_owner']}) {b['resource']} "
                         f"{b['mode']} ROM={b['rom']} [trigger {b['trigger']}: {b['when']}] "
                         f"GBA at launch={b['gba']} {b['verdict']}")
        for b in m.get('banks_unexplained', []):
            lines.append(f"    bank engine owner {b['engine_owner']} starts with {b['minerals']}/{b['gas']}; "
                         "no first-pass SetResources row in the CHK sets it")
        if 'counts' in m and not m.get('banks'):
            lines.append('    banks: no first-pass SetResources row in the CHK')
        if m.get('rom_start_banks') is not None and 'counts' not in m:
            for b in m['rom_start_banks']:
                lines.append(f"    ROM start bank players {b['players']} {b['resource']} {b['mode']} {b['amount']} "
                             f"[{b['when']}]")
            if m.get('gba_banks'):
                lines.append('    GBA banks at the census step: ' + ', '.join(
                    f"owner {o} {v['minerals']}/{v['gas']}" for o, v in sorted(m['gba_banks'].items())
                    if v['minerals'] or v['gas']))
    isc = result['iscript']
    lines.append('== iscript sequences: ' + ', '.join(f'{k} {v}' for k, v in sorted(isc['counts'].items())) +
                 ' | banks ' + ', '.join(f'{k} {v}' for k, v in isc['banks'].items()))
    for name, counts in isc['by_animation'].items():
        lines.append(f"    {name}: " + ', '.join(f'{k} {v}' for k, v in sorted(counts.items())))
    for u in isc['units']:
        for r in u['rows']:
            if r['verdict'] == 'match':
                continue
            detail = r.get('note', '')
            if 'rom_frames' in r and 'engine_frames' in r:
                detail += f" ROM frames {r['rom_frames']} waits {r.get('rom_waits', r.get('rom_ticks'))}" \
                          f" engine {r['engine_frames']} waits {r.get('engine_waits', r.get('engine_ticks'))}"
            elif 'rom_steps' in r and 'engine_steps' in r:
                detail += f" ROM {r['rom_steps']} engine {r['engine_steps']}"
            lines.append(f"iscript {u['raw']:>3} {u['name']:<28} {r['animation']:<34} {r['verdict']:<8} {detail}")
    lines.append('== iscript animations the engine does not model (ROM animation: units carrying it): ' +
                 ', '.join(f'{k} {v}' for k, v in isc['not_modelled'].items()))
    for u in isc['units']:
        if u['not_modelled']:
            lines.append(f"not-modelled {u['raw']:>3} {u['name']:<28} {', '.join(u['not_modelled'])}")
    for limit in isc['limits']:
        lines.append(f'    limit: {limit}')
    snd = result['sounds']
    lines.append(f"== sounds (bank {snd['bank']}): " + ', '.join(f'{k} {v}' for k, v in sorted(snd['counts'].items())))
    for kind, counts in snd['by_kind'].items():
        lines.append(f"    {kind}: " + ', '.join(f'{k} {v}' for k, v in sorted(counts.items())))
    for u in snd['units']:
        for kind, k in u['kinds'].items():
            if k['verdict'] in ('match', 'none', 'silent in ROM'):
                continue
            lines.append(f"sound {u['raw']:>3} {u['name']:<28} {kind:<8} {k['verdict']:<36} "
                         f"ROM={k['rom']} GBA={k['gba']}" + (f" silent={k['rom_silent']}" if k['rom_silent'] else ''))
    lines.append('== summary: ' + ', '.join(f'{k} {v}' for k, v in result['summary'].items()))
    return lines


def summarize(result: dict) -> dict:
    isc, snd = result['iscript'], result['sounds']
    return {
        'census exact': result['census']['verdicts'].get('exact', 0),
        'census collision': result['census']['verdicts'].get('collision', 0),
        'census cap-dropped': result['census']['verdicts'].get('cap-dropped', 0),
        'census differs': result['census']['verdicts'].get('differs', 0),
        'census absent': result['census']['verdicts'].get('absent', 0),
        'animation rows': sum(isc['counts'].values()),
        'animation mismatches': isc['counts'].get('differs', 0),
        'animation absent': isc['counts'].get('absent', 0),
        'animation not compared': isc['counts'].get('not compared', 0),
        'sound mismatches': sum(v for k, v in snd['counts'].items() if k.split(' ')[0] in MISMATCH_SOUND),
    }


def run(args) -> int:
    root = Path(args.root).resolve()
    try:
        rom_path = find_input(root, '.local/sc64/rom/StarCraft64-USA.z64', args.rom)
        extractor_path = find_input(root, 'references/sc64-maps/extract_sc64_maps.py', args.extractor)
    except InputsMissing as missing:
        print(f'SKIP sc64_parity_content: private ROM inputs absent ({missing}). '
              'Pass --rom and --extractor, or prepare .local/sc64/rom.', file=sys.stderr)
        return 2 if args.require_inputs else 0
    from tools.sc64_openbw_oracle_tables import EXPECTED_ROM_SHA1, TABLES, read_tables
    from tools import sc64_import as importer
    from tools.sc64_audio_pack import SOUND_TABLE, parse_sound_table
    tables = read_tables(rom_path, extractor_path)
    extractor = importer.upstream_module(extractor_path)
    rom = extractor.load_rom(str(rom_path))
    archive = extractor.BoltArchive(rom)
    maps = {info.bolt_path: (info, chk) for info, chk in extractor.find_maps(archive)}
    entries = {e.path: e for e in archive.entries()}
    _, lengths = parse_sound_table(archive.read(entries[SOUND_TABLE]))
    scripts = Scripts(tables)
    names = stock_names((root / 'starcraft/sc64_stock_profiles.h').read_text(), 'sc64_stock_unit_profiles[')
    local = Path(args.local) if args.local else None

    def bank(relative):
        # No fallback to the main checkout: this checkout's builds read only
        # its own .local, so another checkout's bank would describe another build.
        return (local or root / '.local/sc64') / relative
    banks = {'walk': (bank('walk/generated/manifest.json'), bank('walk/generated/sc64_unit_walk_generated.h')),
             'attack': (bank('attack-poses/generated/manifest.json'),
                        bank('attack-poses/generated/sc64_unit_attack_source_generated.h')),
             'death': (bank('death/generated/manifest.json'), bank('death/generated/sc64_death_sprites_generated.h')),
             'construction': (bank('construction/generated/manifest.json'),
                              bank('construction/generated/sc64_construction_sprites_generated.h'))}
    binaries = Path(args.binaries) if args.binaries else None
    result = {'census': census_section(maps, binaries, extractor, args.absent_profile, root)}
    result['iscript'] = iscript_section(scripts, root, banks, names)
    # Independent of which banks loaded: every unit the ROM voices, and every
    # unit the animation report names.
    drawn = sorted({u['raw'] for u in result['iscript']['units']} |
                   {raw for raw in range(UNIT_COUNT) if any(scripts.rom_sounds(raw).values())})
    audio = Path(args.audio) if args.audio else bank('audio/generated')
    result['sounds'] = sound_section(scripts, drawn, audio, lengths, names)
    result['summary'] = summarize(result)
    lines = report_lines(result)
    print('\n'.join(lines))
    out_dir = (Path(args.out) if args.out else fixture_home(root) / 'content').resolve()
    check_private(root, out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)
    check_private(root, out_dir)
    commit = subprocess.run(['git', 'rev-parse', 'HEAD'], cwd=root, text=True, capture_output=True).stdout.strip()
    document = {'schema': SCHEMA, 'provenance': PROVENANCE, 'tool': 'tools/sc64_parity_content.py',
                'engine_commit': commit, 'rom_sha1': EXPECTED_ROM_SHA1,
                'table_sha256': {name: digest for name, (_, digest) in TABLES.items()},
                'inputs': {'binaries': str(binaries) if binaries else None, 'audio': str(audio),
                           **{k: str(v[1]) for k, v in banks.items()}},
                **result}
    write_private(root, {out_dir / 'l0-content.json': json.dumps(document, indent=1) + '\n',
                         out_dir / 'l0-content.txt': '\n'.join(lines) + '\n'})
    print(f'wrote {out_dir / "l0-content.json"}', file=sys.stderr)
    return 0


def main(argv=None) -> int:
    p = argparse.ArgumentParser(description=__doc__.split('\n\n')[0])
    p.add_argument('--root', default=str(ROOT), help='engine source tree (default: this checkout)')
    p.add_argument('--rom', help='StarCraft 64 USA ROM (default: .local/sc64/rom/StarCraft64-USA.z64)')
    p.add_argument('--extractor', help='pinned BOLT extractor (default: references/sc64-maps/extract_sc64_maps.py)')
    p.add_argument('--binaries', help='directory with <profile>-fast-trace census binaries (default: none, '
                                      'censuses reported absent)')
    p.add_argument('--absent-profile', action='append', default=[], metavar='PROFILE',
                   help='report this profile\'s censuses as absent (its census binary failed to build)')
    p.add_argument('--local', help='private generated inputs (default: .local/sc64 of --root)')
    p.add_argument('--audio', help='audio bank directory (default: <local>/audio/generated)')
    p.add_argument('--out', help='output directory (default: $SC64_PARITY_HOME/content)')
    p.add_argument('--require-inputs', action='store_true', help='exit 2 instead of skipping when inputs are absent')
    return run(p.parse_args(argv))


if __name__ == '__main__':
    sys.exit(main())
