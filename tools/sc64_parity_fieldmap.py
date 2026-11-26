#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""N64 field map: mission-launch censuses, table anchors and decoded pools.

Reads capture runs (experiments/ares-headless/capture.py) and the owned ROM's
CHK maps and .dat tables. Everything it prints is derived numbers; capture
directories, dumps and the ROM stay private.

  sc64_parity_fieldmap.py census RUN --mission T1     launch census vs the CHK
  sc64_parity_fieldmap.py tables DUMP [DUMP...]       .dat columns in guest RAM

The census compares the first in-mission tick of a capture against the
mission's CHK: placed units (type, owner, position), mineral and geyser
amounts, and the start banks SetResources sets on the first trigger pass.
The table anchor finds each units/weapons/flingy/upgrades/techdata column in
a full-RAM dump as a big-endian column array and compares it with the BOLT
table the ROM carries.
"""
import argparse
import json
import pathlib
import struct
import sys
from collections import Counter, defaultdict

ROOT = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT))
sys.path.insert(0, str(ROOT / 'tools'))

START_LOCATION, GEYSER = 214, 188
MINERALS = (176, 177, 178)
LARVA, EGG = 35, 36
MOVED_RADIUS = 8   # N64 px: a placed unit already moving on the first in-mission tick
DISPLACE_RADIUS = 64  # N64 px: a stacked placement the cartridge spread out at creation (as the L0 census)
HANGAR = {72: 73, 82: 73, 83: 85, 81: 85}  # Carrier, Gantrithor: Interceptors (73); Reaver, Warbringer: Scarabs (85)
STACK_RADIUS = 16  # N64 px (the L0 census's 8 GBA px): another placement this close is a stack the cartridge spread
RAM_BASE = 0x80000000

# Row offsets this tool reads, by layout field name (tools/parity/sc64-usa-v1.json).
ROW = {'x': (0x28, '>H'), 'y': (0x2A, '>H'), 'owner': (0x4C, '>B'), 'order': (0x4D, '>B'),
       'type': (0x64, '>H'), 'resource_amount': (0xCC, '>H'), 'hp': (0x08, '>I')}


def row_field(row: bytes, name: str) -> int:
    offset, fmt = ROW[name]
    return struct.unpack_from(fmt, row, offset)[0]


# ---- CHK side ---------------------------------------------------------------

def rom_inputs(root: pathlib.Path = ROOT):
    """(maps {bolt path: (info, chk)}, extractor module, tables {name: bytes}); raises
    sc64_parity_tables.InputsMissing when the private ROM or extractor is absent."""
    from tools import sc64_import as importer
    from tools.sc64_parity_tables import find_input
    from tools.sc64_openbw_oracle_tables import read_tables
    rom_path = find_input(root, '.local/sc64/rom/StarCraft64-USA.z64', None)
    extractor_path = find_input(root, 'references/sc64-maps/extract_sc64_maps.py', None)
    extractor = importer.upstream_module(extractor_path)
    archive = extractor.BoltArchive(extractor.load_rom(str(rom_path)))
    maps = {info.bolt_path: (info, chk) for info, chk in extractor.find_maps(archive)}
    return maps, extractor, read_tables(rom_path, extractor_path)


def mission_chk(label: str, maps, extractor) -> dict:
    """Placements, start banks and map size of one campaign mission (T1-T10, Z1-Z10, P1-P10)."""
    from tools import sc64_import as importer
    from tools import sc64_parity_content as content
    path = {lab: p for _, lab, p in content.MISSIONS}.get(label)
    if path is None or path not in maps:
        raise KeyError(f'no CHK for mission {label}')
    info, chk = maps[path]
    sections = importer.sections(chk, extractor)
    rows = content.unit_rows(sections.get(b'UNIT', b'')) + content.doodad_units(sections.get(b'THG2', b''))
    triggers = importer.trigger_rows(sections.get(b'TRIG', b''), sections.get(b'STR ', b''), extractor)
    banks = content.start_banks(triggers, sections.get(b'FORC', b''), rows, sections.get(b'OWNR', b''))
    width, height = struct.unpack_from('<HH', sections[b'DIM '], 0)
    return {'label': label, 'chk': path, 'title': info.name, 'rows': rows, 'banks': banks,
            'width': width, 'height': height, 'owners': sections.get(b'OWNR', b'')[:12],
            'sections': sections}


# ---- census -----------------------------------------------------------------

def max_pairs(left: list, right: list, fits) -> list:
    """A best one-to-one pairing of left and right items that fit, as [(left, right)] in left
    order: the most pairs, then the most exact positions, then the most equal resource
    amounts (minerals and geysers only), then the least total distance. Searched exhaustively (at
    most EXHAUSTIVE_GROUP placements per connected group of candidates); larger groups fall back
    to augmenting paths."""
    near = lambda a, b: abs(a['x'] - b['x']) + abs(a['y'] - b['y'])
    resource = lambda a: a['type'] in MINERALS + (GEYSER,)
    score = lambda a, b: (1, near(a, b) == 0,
                          resource(a) and a.get('resources', 0) == b.get('resource_amount', 0), -near(a, b))
    chosen = {}
    # Connected groups of candidates (left i and right j joined when they fit): each is searched
    # on its own, so isolated placements never enlarge a search.
    parent = {}

    def find(x):
        root = x
        while parent.setdefault(root, root) != root:
            root = parent[root]
        while parent[x] != root:   # path compression
            parent[x], x = root, parent[x]
        return root
    for i, a in enumerate(left):
        for j, b in enumerate(right):
            if fits(a, b):
                parent[find(('L', i))] = find(('R', j))
    groups = defaultdict(lambda: ([], []))
    for i in range(len(left)):
        groups[find(('L', i))][0].append(i)
    for j in range(len(right)):
        groups[find(('R', j))][1].append(j)
    for li, ri in groups.values():
        if not li or not ri:
            continue
        if len(li) > EXHAUSTIVE_GROUP or len(ri) > EXHAUSTIVE_GROUP:
            for x, y in _augmenting_pairs([left[i] for i in li], [right[j] for j in ri], fits, near):
                chosen[next(i for i in li if left[i] is x)] = next(j for j in ri if right[j] is y)
            continue
        best = (None, {})

        def search(k, taken, total, pairs):
            nonlocal best
            if k == len(li):
                if best[0] is None or total > best[0]:
                    best = (total, dict(pairs))
                return
            search(k + 1, taken, total, pairs)
            a = left[li[k]]
            for j in ri:
                if j not in taken and fits(a, right[j]):
                    gain = score(a, right[j])
                    pairs[li[k]] = j
                    search(k + 1, taken | {j}, tuple(x + y for x, y in zip(total, gain)), pairs)
                    del pairs[li[k]]
        search(0, frozenset(), (0, 0, 0, 0), {})
        chosen.update(best[1])
    return [(left[i], right[chosen[i]]) for i in sorted(chosen)]


EXHAUSTIVE_GROUP = 6


def _augmenting_pairs(left: list, right: list, fits, near) -> list:
    options = [sorted((j for j, b in enumerate(right) if fits(a, b)), key=lambda j: near(a, right[j])) for a in left]
    owner = {}

    def augment(i, seen):
        for j in options[i]:
            if j not in seen:
                seen.add(j)
                if j not in owner or augment(owner[j], seen):
                    owner[j] = i
                    return True
        return False
    for i in range(len(left)):
        augment(i, set())
    return [(left[i], right[j]) for j, i in sorted(owner.items(), key=lambda p: p[1])]


def census_compare(chk_rows: list, n64_rows: list, subunits: dict) -> dict:
    """Join CHK placements to N64 rows on (type, owner, x, y).

    chk_rows: CHK placements ({'type','owner','x','y','resources','section'[, 'disabled']}).
    n64_rows: live N64 rows at the first in-mission tick ({'type','owner','x','y','resource_amount'}).
    subunits: {parent type: turret type} (units.dat subunit1 below 228).
    Start locations are not units. THG2 unit doodads join on type and spot whatever their
    owner (the owner rule is reported under 'doodads'). A row that
    matches no placement is a turret when a matched parent of its subunit type stands
    on the same spot, larva or eggs when a placed hatchery-family building of that
    owner exists, Interceptors and Scarabs on a placed Carrier's or Reaver's spot, and
    otherwise extra. Units within MOVED_RADIUS of their spot are 'moved' (already walking),
    within DISPLACE_RADIUS 'displaced' (a stacked placement spread out at creation). A missing placement and an extra row of the same
    type on the same spot are one unit whose owner a start trigger changed."""
    expected = [r for r in chk_rows if r['type'] != START_LOCATION]
    pool = defaultdict(list)
    for i, r in enumerate(n64_rows):
        pool[(r['type'], r['owner'], r['x'], r['y'])].append(i)
    used, matched, missing, amount_differs = set(), [], [], []
    for r in expected:
        key = (r['type'], r['owner'], r['x'], r['y'])
        if pool[key]:
            i = pool[key].pop(0)
            used.add(i)
            matched.append((r, n64_rows[i]))
        else:
            missing.append(r)
    # Unit doodads (THG2 rows: doors, traps) join on type and spot; the cartridge gives
    # enabled ones to player 11 and keeps the CHK owner of disabled ones (reported).
    for r in list(missing):
        if r.get('section') != 'THG2':
            continue
        spot = next((i for i, n in enumerate(n64_rows) if i not in used and n['type'] == r['type'] and
                     n['x'] == r['x'] and n['y'] == r['y']), None)
        if spot is not None:
            used.add(spot)
            matched.append((r, n64_rows[spot]))
            missing.remove(r)
    doodad_owner = [{'chk_owner': r['owner'], 'n64_owner': n['owner'], 'disabled': bool(r.get('disabled'))}
                    for r, n in matched if r.get('section') == 'THG2']
    # Units already moving on the first tick: same type and owner within MOVED_RADIUS px.
    out = {}
    stacked = lambda r: any(o is not r and abs(o['x'] - r['x']) <= STACK_RADIUS and abs(o['y'] - r['y']) <= STACK_RADIUS
                            for o in expected)
    index_of = {id(n): i for i, n in enumerate(n64_rows)}
    for label, radius in (('moved', MOVED_RADIUS), ('displaced', DISPLACE_RADIUS)):
        out[label] = []
        # An isolated placement has nothing to be spread from, so only stacked ones may be displaced.
        eligible = [r for r in missing if label == 'moved' or stacked(r)]
        free_rows = [n for i, n in enumerate(n64_rows) if i not in used]
        fits = lambda r, n: (n['type'], n['owner']) == (r['type'], r['owner']) and \
            max(abs(n['x'] - r['x']), abs(n['y'] - r['y'])) <= radius
        taken = set()
        for r, n in max_pairs(eligible, free_rows, fits):   # most pairs, then the least distance
            used.add(index_of[id(n)])
            taken.add(id(r))
            matched.append((r, n))
            out[label].append({'type': r['type'], 'owner': r['owner'], 'chk': [r['x'], r['y']], 'n64': [n['x'], n['y']]})
        missing = [r for r in missing if id(r) not in taken]
    moved, displaced = out['moved'], out['displaced']
    missing = [{k: r[k] for k in ('type', 'owner', 'x', 'y', 'section', 'resources')} for r in missing]
    spots = {(r['x'], r['y'], subunits.get(r['type'])) for r, _ in matched if subunits.get(r['type'])}
    hatcheries = {r['owner'] for r, _ in matched if r['type'] in (131, 132, 133)}
    hangars = {(n['x'], n['y'], n['owner'], HANGAR[r['type']]) for r, n in matched if r['type'] in HANGAR}
    turrets, larvae, extra, hangar = [], [], [], []
    turret_types = Counter()
    for i, n in enumerate(n64_rows):
        if i in used:
            continue
        brief = {k: n[k] for k in ('type', 'owner', 'x', 'y', 'resource_amount')}
        if (n['x'], n['y'], n['type']) in spots:
            turrets.append(brief)
            turret_types[n['type']] += 1
        elif n['type'] in (LARVA, EGG) and n['owner'] in hatcheries:
            larvae.append(brief)
        elif (n['x'], n['y'], n['owner'], n['type']) in hangars:
            hangar.append(brief)
        else:
            extra.append(brief)
    # A start trigger (Give Units, for one) can change a placed unit's owner before the
    # first tick is read: same type and spot, another owner.
    owner_changed = []
    for m, twin in max_pairs(missing, extra, lambda m, x: x['type'] == m['type'] and
                             abs(x['x'] - m['x']) <= MOVED_RADIUS and abs(x['y'] - m['y']) <= MOVED_RADIUS):
        extra.remove(twin)
        missing.remove(m)
        owner_changed.append({'type': m['type'], 'x': m['x'], 'y': m['y'], 'chk_owner': m['owner'],
                              'n64_owner': twin['owner'], 'chk_amount': m.get('resources', 0),
                              'n64_amount': twin.get('resource_amount', 0)})
    # Amounts after every join, so a moved or re-owned field is checked too.
    for r, n in matched:
        if r['type'] in MINERALS + (GEYSER,) and n['resource_amount'] != r['resources']:
            amount_differs.append({'type': r['type'], 'x': r['x'], 'y': r['y'], 'chk': r['resources'],
                                   'n64': n['resource_amount']})
    for o in owner_changed:
        if o['type'] in MINERALS + (GEYSER,) and o['chk_amount'] != o['n64_amount']:
            amount_differs.append({'type': o['type'], 'x': o['x'], 'y': o['y'], 'chk': o['chk_amount'],
                                   'n64': o['n64_amount']})
    minerals = [r for r in expected if r['type'] in MINERALS]
    geysers = [r for r in expected if r['type'] == GEYSER]
    return {
        'verdict': 'exact' if not missing and not extra and not amount_differs else 'differs',
        'chk_placements': len(expected), 'matched': len(matched), 'moved': moved, 'displaced': displaced,
        'hangar': len(hangar), 'owner_changed': owner_changed,
        'doodads': {'rows': len(doodad_owner),
                    'owners': sorted({(d['chk_owner'], d['n64_owner'], d['disabled']) for d in doodad_owner})},
        'missing': missing, 'extra': extra,
        'turrets': dict(sorted(turret_types.items())), 'larvae': len(larvae),
        'minerals': {'chk': len(minerals), 'matched': sum(1 for r, _ in matched if r['type'] in MINERALS)},
        'geysers': {'chk': len(geysers), 'matched': sum(1 for r, _ in matched if r['type'] == GEYSER)},
        'amount_differs': amount_differs,
        'start_locations': sum(1 for r in chk_rows if r['type'] == START_LOCATION),
    }


def bank_compare(rows: list, series: list, window: int = 32) -> list:
    """Each CHK first-pass SetResources row against the N64 bank arrays.

    series: [(source frame, minerals[12], gas[12])] from the first in-mission tick.
    A 'set' row matches when the player's bank equals the amount within `window`
    unpaused source frames of launch; the first such frame is reported."""
    out = []
    for row in rows:
        kinds = ('minerals', 'gas') if row['resource'] == 'minerals+gas' else (row['resource'],)
        for player in row['players']:
            for kind in kinds:
                verdict, when, seen = 'no match', None, None
                for sf, minerals, gas in series:
                    if sf > window:
                        break
                    value = (minerals if kind == 'minerals' else gas)[player]
                    seen = value if seen is None else seen
                    if row['mode'] == 'set' and value == row['amount']:
                        verdict, when = 'match', sf
                        break
                if row['mode'] != 'set':
                    verdict = f"not compared ({row['mode']})"
                out.append({'trigger': row['trigger'], 'player': player, 'resource': kind, 'mode': row['mode'],
                            'chk': row['amount'], 'n64_at_launch': seen, 'verdict': verdict, 'source_frame': when,
                            'when': row['when']})
    return out


def capture_census(run: pathlib.Path, label: str, inputs=None) -> dict:
    import parity_decode as pdx
    maps, extractor, tables = inputs or rom_inputs()
    chk = mission_chk(label, maps, extractor)
    cap = pdx.Capture(run)
    first = next((r for r in cap.ticks if r['table'] and r['rows']), None)
    if first is None:
        return {'mission': label, 'verdict': 'no table', 'run': run.name}
    n64_rows = [{name: row_field(b, name) for name in ('type', 'owner', 'x', 'y', 'resource_amount')}
                for b in first['rows'].values()]
    from tools.sc64_parity_tables import UNIT_FIELDS, column_major
    subunit1 = column_major(tables['units.dat'], 228, UNIT_FIELDS, 'units')['subunit1']
    subunits = {t: s for t, s in enumerate(subunit1) if s is not None and s < 228}
    result = census_compare(chk['rows'], n64_rows, subunits)
    series = []
    for r in cap.ticks:
        if r['table'] and not r['paused'] and r['sf'] >= 0:
            g = r['globals']
            series.append((r['sf'], struct.unpack_from('>12I', g, 0), struct.unpack_from('>12I', g, 0x30)))
    result['banks'] = bank_compare(chk['banks'], series)
    selectors = sorted({r['selector'] for r in cap.ticks if r['table']})
    result.update(mission=label, title=chk['title'], chk=chk['chk'], run=run.name, first_tick=first['tick'],
                  first_step=first['step'], n64_rows=len(n64_rows), selectors=selectors,
                  pokes=cap.manifest.get('pokes', 0), capture_ok=cap.manifest.get('ok') is True,
                  verification_problems=list(cap.verified), faults=cap.faults,
                  unpaused_ticks=sum(1 for r in cap.ticks if r['table'] and not r['paused']))
    return result


# ---- heap blocks, fog, sprites and images ------------------------------------

HEAP_GUARD = struct.pack('>III', 0xBABEFACE, 0xBABEFACE, 0xBABEFACE)
IMAGE_POOL = (240000, 60)    # 4000 records of 60 bytes
SPRITE_POOL = (79200, 36)    # 2200 records of 36 bytes
BULLET_POOL = (11200, 112)   # 100 records of 112 bytes
UNIT_POOL = (0x448E0, 312)


def heap_blocks(ram: bytes) -> list:
    """[(KSEG0 data address, size)] of every heap block: a u32 size word, then three
    0xBABEFACE guard words, then the data (the unit table's locator, generalised)."""
    out, at = [], 4
    while True:
        at = ram.find(HEAP_GUARD, at)
        if at < 0:
            return out
        size = struct.unpack_from('>I', ram, at - 4)[0]
        out.append((RAM_BASE + at + 12, size))
        at += 12


def block_of_size(ram: bytes, size: int) -> list:
    return [a for a, n in heap_blocks(ram) if n == size]


def fog_masks(block: bytes, width: int, height: int, player: int):
    """(visible, explored) tile sets of one player from the fog tile block: one u32 per
    tile, row-major, big-endian [u16 tile flags][u8 explored mask][u8 visible mask];
    a clear player bit means explored / visible (the BWAPI sense, inverted bits)."""
    if len(block) < width * height * 4:
        raise ValueError('fog block shorter than the map')
    bit = 1 << player
    visible, explored = set(), set()
    for i in range(width * height):
        if not block[4 * i + 3] & bit:
            visible.add(i)
        if not block[4 * i + 2] & bit:
            explored.add(i)
    return visible, explored


def fog_consistency(block: bytes, width: int, height: int) -> dict:
    """Counts behind the byte assignment: a tile is never visible to a player that has
    not explored it, so the visible mask's clear bits must be a subset of the explored
    mask's."""
    bad = sum(1 for i in range(width * height) if ~block[4 * i + 3] & block[4 * i + 2] & 0xFF)
    return {'tiles': width * height, 'visible_not_explored': bad}


def find_fog_block(ram: bytes, width: int, height: int) -> list:
    """Heap blocks of exactly width*height*4 bytes whose masks pass fog_consistency."""
    found = []
    for address in block_of_size(ram, width * height * 4):
        block = ram[address - RAM_BASE:address - RAM_BASE + width * height * 4]
        c = fog_consistency(block, width, height)
        if c['visible_not_explored'] == 0 and any(b != 0xFF for b in block[3::4]):
            found.append(address)
    return found


IMAGE_FIELDS = {'prev': (0x00, '>I'), 'next': (0x04, '>I'), 'image_id': (0x08, '>H'), 'flags': (0x0A, '>B'),
                'draw_function': (0x0B, '>B'), 'header': (0x14, '>H'), 'offset': (0x16, '>H'),
                'sprite': (0x38, '>I')}
SPRITE_FIELDS = {'prev': (0x00, '>I'), 'next': (0x04, '>I'), 'sprite_id': (0x08, '>H'), 'owner': (0x0A, '>B'),
                 'visibility': (0x0C, '>B'), 'x': (0x14, '>H'), 'y': (0x16, '>H'), 'image_primary': (0x18, '>I'),
                 'draw_head': (0x1C, '>I'), 'image_tail': (0x20, '>I')}


def record(blob: bytes, fields: dict) -> dict:
    return {name: struct.unpack_from(fmt, blob, offset)[0] for name, (offset, fmt) in fields.items()}


def unit_images(ram: bytes, row: bytes, limit: int = 16) -> list:
    """The images of one live unit row, head first, read through its sprite."""
    sprite = struct.unpack_from('>I', row, 0x0C)[0]
    if not RAM_BASE <= sprite < RAM_BASE + len(ram):
        return []
    s = record(ram[sprite - RAM_BASE:sprite - RAM_BASE + 36], SPRITE_FIELDS)
    out, at = [], s['image_primary']
    while at and RAM_BASE <= at < RAM_BASE + len(ram) and len(out) < limit:
        img = ram[at - RAM_BASE:at - RAM_BASE + 60]
        out.append(dict(record(img, IMAGE_FIELDS), address=at, raw=img))
        at = struct.unpack_from('>I', img, 4)[0]
    return out


# ---- per-player upgrade and tech arrays ----------------------------------------

UPGRADES, TECHS = 61, 44
PLAYER_TABLES = {  # KSEG0 address, entries per player; tools/parity/sc64-usa-v1.json player_tables
    'upgrade_max': (0x800BA41C, UPGRADES), 'upgrade_level': (0x800BA6F8, UPGRADES),
    'tech_available': (0x800B9FB4, TECHS), 'tech_researched': (0x800BA1C4, TECHS),
}


def chk_settings(section: bytes, count: int):
    """Effective per-player (maximum or available, start level or researched) arrays of a
    PUPx/PTEx section: [12][count] each, a player's 'uses default' byte picking the default row."""
    if len(section) < 38 * count:
        raise ValueError(f'settings section of {len(section)} bytes is short for {count} entries')
    first, second = bytearray(), bytearray()
    for p in range(12):
        for k in range(count):
            default = section[26 * count + p * count + k]
            first.append(section[24 * count + k] if default else section[p * count + k])
            second.append(section[25 * count + k] if default else section[12 * count + p * count + k])
    return bytes(first), bytes(second)


def player_tables_check(ram: bytes, sections: dict) -> dict:
    """Each player table in RAM against the CHK's effective settings at launch."""
    upgrade_max, upgrade_start = chk_settings(sections[b'PUPx'], UPGRADES)
    tech_available, tech_researched = chk_settings(sections[b'PTEx'], TECHS)
    expected = {'upgrade_max': upgrade_max, 'upgrade_level': upgrade_start,
                'tech_available': tech_available, 'tech_researched': tech_researched}
    out = {}
    for name, (address, count) in PLAYER_TABLES.items():
        got = ram[address - RAM_BASE:address - RAM_BASE + 12 * count]
        want = expected[name]
        cells = [(p, k, want[p * count + k], got[p * count + k]) for p in range(12) for k in range(count)
                 if want[p * count + k] != got[p * count + k]]
        out[name] = {'equal': not cells, 'nonzero_expected': sum(1 for b in want if b),
                     'differ': [{'player': p, 'entry': k, 'chk': w, 'ram': g} for p, k, w, g in cells[:12]],
                     'cells_differ': len(cells)}
    return out


# ---- table anchor (S12) -----------------------------------------------------

def table_columns(tables: dict) -> list:
    """[(name, struct format, rows, little-endian bytes, values)] for every column of the
    five gameplay .dat tables, in file order."""
    from tools.sc64_parity_tables import UNIT_FIELDS, UPGRADE_FIELDS, TECH_FIELDS
    from tools.sc64_weapon_source_audit import FLINGY_FIELDS, WEAPON_FIELDS
    out = []
    for table, data, count, fields in (('units', tables['units.dat'], 228, UNIT_FIELDS),
                                       ('upgrades', tables['upgrades.dat'], 61, UPGRADE_FIELDS),
                                       ('techdata', tables['techdata.dat'], 44, TECH_FIELDS),
                                       ('weapons', tables['weapons.dat'], 130, WEAPON_FIELDS),
                                       ('flingy', tables['flingy.dat'], 209, FLINGY_FIELDS)):
        cursor = 0
        for spec in fields:
            name, fmt = spec[0], spec[1]
            rows = spec[3] if len(spec) > 2 else count
            size = struct.calcsize('<' + fmt)
            raw = data[cursor:cursor + rows * size]
            cursor += rows * size
            out.append((f'{table}.{name}', fmt, rows, raw))
        if cursor != len(data):
            raise ValueError(f'{table}: parsed {cursor} of {len(data)} bytes')
    return out


def table_hints() -> dict:
    """The first-column addresses the layout records (tools/parity/sc64-usa-v1.json dat_tables)."""
    import parity_decode as pdx
    return {k: v for k, v in pdx.Layout().doc.get('dat_tables', {}).items() if '.' in k}


def big_endian(fmt: str, raw: bytes) -> bytes:
    """A little-endian column re-encoded big-endian, one value at a time. Columns that
    pack u16 halves (placement, addon, dimensions) are stored per u16 instead (per_u16)."""
    size = struct.calcsize('<' + fmt)
    return b''.join(raw[i:i + size][::-1] for i in range(0, len(raw), size))


def per_u16(raw: bytes) -> bytes:
    return b''.join(raw[i:i + 2][::-1] for i in range(0, len(raw), 2))


def anchor_columns(ram: bytes, columns: list, base_hint: dict | None = None) -> list:
    """Locate each column in a full-RAM image. A column is 'equal' where its
    big-endian (or per-u16) encoding occurs; otherwise it is expected right after
    the previous column of the same table (columns follow file order, each starting on
    a 4-byte boundary), and the rows that differ there are listed. A table's first column
    is taken at base_hint[column] (KSEG0 address, the layout's dat_tables) when it matches
    there; otherwise at a 4-byte aligned occurrence, preferring one whose next column also
    matches right after it."""
    align = lambda a: (a + 3) & ~3
    out, previous = [], {}
    hints = {k: int(v, 16) - RAM_BASE for k, v in (base_hint or {}).items() if isinstance(v, str) and v.startswith('0x')}
    following = {columns[k][0]: columns[k + 1] for k in range(len(columns) - 1)
                 if columns[k][0].split('.')[0] == columns[k + 1][0].split('.')[0]}

    def next_matches(name, at):
        nxt = following.get(name)
        if nxt is None:
            return False
        blob = big_endian(nxt[1], nxt[3])
        return ram[at:at + len(blob)] == blob

    for name, fmt, rows, raw in columns:
        table = name.split('.')[0]
        size = len(raw)
        expected = previous.get(table)
        # Columns that pack u16 halves (placement, addon, dimensions: 'I' and 'Q') may be stored
        # per u16; every other column only as big-endian values.
        candidates = (('be', big_endian(fmt, raw)),) + ((('u16', per_u16(raw)),) if fmt in ('I', 'Q') else ())
        found = None
        if expected is not None:
            # The spot after the previous column decides first: a short or uniform column
            # (all zeros, say) also occurs by chance elsewhere.
            found = next(((enc, expected, ram.count(blob)) for enc, blob in candidates
                          if ram[expected:expected + size] == blob), None)
        else:
            hint = hints.get(name)
            for enc, blob in candidates:
                if hint is not None and ram[hint:hint + size] == blob:
                    found = (enc, hint, ram.count(blob))
                    break
                spots, at = [], ram.find(blob)
                while at >= 0 and len(spots) < 64:
                    if at % 4 == 0:
                        spots.append(at)
                    at = ram.find(blob, at + 1)
                if spots:
                    best = next((a for a in spots if next_matches(name, align(a + size))), spots[0])
                    found = (enc, best, ram.count(blob))
                    break
        if found:
            encoding, at, count = found
            out.append({'column': name, 'address': f'{RAM_BASE + at:08x}', 'status': 'equal', 'encoding': encoding,
                        'occurrences': count})
            previous[table] = align(at + size)
            continue
        entry = {'column': name, 'status': 'not found', 'address': None}
        if expected is not None:
            at = expected
            blob = big_endian(fmt, raw)
            got = ram[at:at + size]
            width = struct.calcsize('<' + fmt)
            diffs = [{'row': i, 'rom': struct.unpack_from('>' + fmt, blob, i * width)[0],
                      'ram': struct.unpack_from('>' + fmt, got, i * width)[0]}
                     for i in range(rows) if blob[i * width:(i + 1) * width] != got[i * width:(i + 1) * width]]
            entry.update(status='differs', address=f'{RAM_BASE + at:08x}', rows_differ=len(diffs), diffs=diffs[:40])
            previous[table] = align(at + size)
        out.append(entry)
    return out


# ---- golden-backed field checks (scenario t1-fields) -----------------------------
#
# N64 tape: the T1 launch tape with the fog, image, sprite and bullet regions
# recorded, then the S4 box-select and Move of the five Marines ('# mark command'),
# then Stop, Hold, a second Move and an attack-move (marks stop, hold, move2,
# attack). The checks use the first Move only, up to the 'stop' mark, so the GBA
# side is the existing 'move' runner scenario with the same parameters as t1-move.

T1_MAP = (64, 64)
N64_IDLE, N64_MOVE = {2, 3}, {6}          # Guard / PlayerGuard; Move (layout orders)
GBA_IDLE, GBA_MOVE = {0}, {1}             # ORDER_IDLE, ORDER_MOVE (starcraft/game.c)
N64_WALKING = 11                          # iscript animation id of Walking
GBA_MOVING = 1                            # trace anim kind: move
ORDER_TOLERANCE = 2                       # source frames, +-1 tick at each end
FOG_TIME_TOLERANCE = 2
FOG_COUNT_TOLERANCE = 0.03                # relative, at least FOG_COUNT_FLOOR tiles
FOG_COUNT_FLOOR = 8
WALK_TOLERANCE = 1                        # source frames per cycle
REST_AFTER_ARRIVAL = 60                   # frames from the last arrival to the rest frame
FIELD_FIELDS = ['unit.order', 'order.2', 'order.3', 'order.6', 'pool.fog_tiles', 'pool.sprite', 'pool.image',
                'unit.type', 'unit.owner', 'unit.x', 'unit.y', 'unit.sprite', 'clock.tick_counter',
                'clock.pause_halfword']


class _Before:
    """A capture whose series stops before a frontend step (the first Move only)."""
    def __init__(self, cap, last_step):
        self.cap, self.last_step = cap, last_step

    def series(self, gating=True):
        s = self.cap.series(gating)
        s['frames'] = [f for f in s['frames'] if f['step'] < self.last_step]
        return s


def order_times(history, t0, end, idle, move):
    """(first frame >= t0 in a move order, first later frame back in an idle order) or None."""
    start = back = None
    for sf, value in history:
        if sf < t0 or sf >= end:
            continue
        if start is None and value in move:
            start = sf
        elif start is not None and value in idle:
            back = sf
            break
    return (start, back) if start is not None and back is not None else None


def fog_at(keyframe, changes, sf):
    """GBA human (visible, explored) sets at a source frame from the trace keyframe and changes."""
    k_sf, visible, explored = keyframe
    visible, explored = set(visible), set(explored)
    for at, cell, v, e in changes:
        if at > sf:
            break
        (visible.add if v else visible.discard)(cell)
        (explored.add if e else explored.discard)(cell)
    return visible, explored


def fog_measure(at, t0, end, goal_tile, rest_sf):
    """at(sf) -> (visible, explored) or None. Times are from t0: the first frame a tile joins
    the visible set, the first frame one leaves it, and the counts at the rest frame. The goal
    tile is recorded for reports only."""
    first = at(t0)
    reveal = clear = None
    previous = first[0]
    for sf in range(t0 + 1, end):
        now = at(sf)
        if now is None:
            continue
        if reveal is None and now[0] - previous:
            reveal = sf - t0
        if clear is None and previous - now[0]:
            clear = sf - t0
        previous = now[0]
    rest = at(rest_sf)
    return {'first_reveal': reveal, 'first_clear': clear,
            'visible_at_rest': len(rest[0]) if rest else None, 'explored_at_rest': len(rest[1]) if rest else None}


def walk_period(points):
    """Median gap, in frames, between returns to the cycle's first state while walking.
    points: [(sf, state, walking)] in frame order, one per frame or per change."""
    from statistics import median
    walking = [(sf, state) for sf, state, on in points if on]
    if len(walking) < 4:
        return None
    first = walking[0][1]
    entries, last = [], None
    for sf, state in walking:
        if state == first and last != first:
            entries.append(sf)
        last = state
    gaps = [b - a for a, b in zip(entries, entries[1:]) if b - a < 64]
    return median(gaps) if gaps else None


def fields_observations(m, golden_band=None):
    import sc64_parity_metrics as mt
    t, ft = ORDER_TOLERANCE, FOG_TIME_TOLERANCE
    count_band = lambda v: max(FOG_COUNT_FLOOR, round(v * FOG_COUNT_TOLERANCE))
    obs = [
        mt.observation('order.move_latency', 'Marine x5', m['order_start'], m['order_start'] - t, m['order_start'] + t,
                       'report only: the N64 command tick comes from the tape step', False, FIELD_FIELDS,
                       unit='source frames from the command to the first Move order'),
        mt.observation('order.move_end_first', 'Marine x5', m['order_end_first'], m['order_end_first'] - t,
                       m['order_end_first'] + t, f'+-{t} source frames', True, FIELD_FIELDS,
                       unit='source frames from the command to the first Marine back in an idle order'),
        mt.observation('order.move_end_last', 'Marine x5', m['order_end_last'], m['order_end_last'] - t,
                       m['order_end_last'] + t, f'+-{t} source frames', True, FIELD_FIELDS,
                       unit='source frames from the command to the last Marine back in an idle order'),
        mt.observation('fog.first_reveal', 'human, T1', m['fog']['first_reveal'], m['fog']['first_reveal'] - ft,
                       m['fog']['first_reveal'] + ft, f'+-{ft} source frames', True, FIELD_FIELDS,
                       unit='source frames from the command until a tile first becomes visible'),
        mt.observation('fog.first_clear', 'human, T1', m['fog']['first_clear'], m['fog']['first_clear'] - ft,
                       m['fog']['first_clear'] + ft, f'+-{ft} source frames', True, FIELD_FIELDS,
                       unit='source frames from the command until a visible tile is first dropped'),
        mt.observation('fog.visible_at_rest', 'human, T1', m['fog']['visible_at_rest'],
                       m['fog']['visible_at_rest'] - count_band(m['fog']['visible_at_rest']),
                       m['fog']['visible_at_rest'] + count_band(m['fog']['visible_at_rest']),
                       f'+-{FOG_COUNT_TOLERANCE:.0%} (at least {FOG_COUNT_FLOOR}) tiles', True, FIELD_FIELDS,
                       unit=f'human visible tiles {m["rest_offset"]} frames after the command'),
        mt.observation('fog.explored_at_rest', 'human, T1', m['fog']['explored_at_rest'],
                       m['fog']['explored_at_rest'] - count_band(m['fog']['explored_at_rest']),
                       m['fog']['explored_at_rest'] + count_band(m['fog']['explored_at_rest']),
                       f'+-{FOG_COUNT_TOLERANCE:.0%} (at least {FOG_COUNT_FLOOR}) tiles', True, FIELD_FIELDS,
                       unit=f'human explored tiles {m["rest_offset"]} frames after the command'),
        mt.observation('anim.walk_cycle', 'Marine', m['walk_cycle'], m['walk_cycle'] - WALK_TOLERANCE,
                       m['walk_cycle'] + WALK_TOLERANCE, f'+-{WALK_TOLERANCE} source frames', True, FIELD_FIELDS,
                       unit='source frames per walk cycle (N64: iscript Walking loop; GBA: stride phases)'),
    ]
    return obs


def fields_measure_n64(cap, marks):
    import sc64_parity_metrics as mt
    stop = marks['stop']
    base = mt.move_measure_n64(_Before(cap, stop), marks)
    measured = base['measured']
    movers = sorted(int(k) for k in measured['units'])
    t0 = measured['t0']
    end = max(r['sf'] for r in cap.ticks if r['table'] and r['step'] < stop) + 1
    spans = [order_times(cap.orders(uid), t0, end, N64_IDLE, N64_MOVE) for uid in movers]
    if any(s is None for s in spans):
        raise SystemExit('t1-fields: a Marine never went from a Move order back to an idle one before the stop mark')
    first = next(r for r in cap.ticks if r['table'])
    owner = cap.unit(first['rows'][movers[0]], 'owner')
    width, height = T1_MAP
    by_sf = {r['sf']: r for r in cap.ticks if r['table'] and not r['paused'] and r['sf'] >= 0 and r['step'] < stop}
    uncovered = [sf for sf in range(t0, end) if sf not in by_sf or 'extra' not in by_sf[sf]]
    if uncovered or cap.faults['tick_skips']:
        raise SystemExit(f't1-fields: the fog and image regions miss {len(uncovered)} frames of the window '
                         f'(first {uncovered[:3]}) or ticks were skipped; record every tick')
    at = lambda sf: cap.fog(by_sf[sf], owner, width, height)
    gx, gy = measured['centroid_end']
    goal_tile = int(gx // 32) + int(gy // 32) * width
    # Rest frame: the last frame before the stop mark, at least REST_AFTER_ARRIVAL frames after
    # the last arrival so an N64 fog refresh (about every 100 frames) has dropped the start area.
    rest_offset = end - 1 - t0
    if rest_offset < measured['latency'] + measured['arrival_last'] + REST_AFTER_ARRIVAL:
        raise SystemExit('t1-fields: the stop mark comes too soon after the last arrival for the fog rest frame')
    fog = fog_measure(at, t0, end, goal_tile, t0 + rest_offset)
    if None in fog.values():
        raise SystemExit(f't1-fields: fog measure incomplete {fog}')
    # Walk cycle: the first mover's primary image while its animation is Walking.
    points = []
    for sf in sorted(by_sf):
        if t0 <= sf < end:
            images = cap.unit_images(by_sf[sf], movers[0])
            if not images:
                raise SystemExit(f't1-fields: the first Marine\'s primary image is not readable at frame {sf}')
            points.append((sf, images[0]['iscript_offset'], images[0]['animation'] == N64_WALKING))
    m = {'order_start': min(s[0] for s in spans) - t0, 'order_end_first': min(s[1] for s in spans) - t0,
         'order_end_last': max(s[1] for s in spans) - t0, 'fog': fog, 'rest_offset': rest_offset,
         'walk_cycle': walk_period(points), 'goal_tile': goal_tile, 'human': owner}
    if m['walk_cycle'] is None:
        raise SystemExit('t1-fields: no Walking cycle recorded for the first Marine')
    obs = base['observations'] + fields_observations(m)
    gba = dict(base['gba'], fields={'rest_offset': rest_offset, 'goal_tile': goal_tile, 'map': list(T1_MAP),
                                    'end_offset': end - t0})
    # The GBA run must reach the rest frame: its command lands near source frame at_step * 4 / 5.
    gba['steps'] = max(int(gba['steps']), int((gba['at_step'] * 4 // 5 + end - t0 + 8) * 5 / 4) + 10)
    notes = base['notes'] + [f'fields: human owner {owner}; checks use the first Move only (frames before the '
                             f'stop mark); N64 idle orders {sorted(N64_IDLE)}, Move {sorted(N64_MOVE)}']
    return {'observations': obs, 'gba': gba, 'notes': notes, 'curves': base['curves'],
            'measured': dict(measured, fields=m)}


def fields_measure_gba(series, golden, report):
    import sc64_parity_metrics as mt
    base, err = mt.move_measure_gba(series, golden, report)
    if err:
        return None, err
    params = golden['gba']['fields']
    t0 = report['command_sf']
    movers = report['movers']
    end = t0 + int(params['end_offset'])
    spans = [order_times(series['orders'].get(uid, []), t0, end, GBA_IDLE, GBA_MOVE) for uid in movers]
    if any(s is None for s in spans):
        return None, 'a Marine never went from ORDER_MOVE back to ORDER_IDLE'
    fog = series['fog']
    if fog['keyframe'] is None or (fog['cols'], fog['rows']) != tuple(params['map']):
        return None, f"GBA fog keyframe missing or map {fog['cols']}x{fog['rows']} is not {params['map']}"
    if series['frames'][-1]['sf'] < end - 1:
        return None, f"the GBA run ends at source frame {series['frames'][-1]['sf']}, before the rest window ({end})"
    at = lambda sf: fog_at(fog['keyframe'], fog['changes'], sf)
    f = fog_measure(at, t0, end, int(params['goal_tile']), t0 + int(params['rest_offset']))
    if None in f.values():
        return None, f'fog measure incomplete {f}'
    strides = series['walks'].get(movers[0], [])
    points = [(sf, walk, anim == GBA_MOVING) for sf, walk, anim in strides if t0 <= sf < end]
    walk = walk_period(points)
    if walk is None:
        return None, 'no GBA stride cycle recorded for the first Marine'
    values = dict(base['values'])
    values.update({'order.move_latency': min(s[0] for s in spans) - t0,
                   'order.move_end_first': min(s[1] for s in spans) - t0,
                   'order.move_end_last': max(s[1] for s in spans) - t0,
                   'fog.first_reveal': f['first_reveal'], 'fog.first_clear': f['first_clear'],
                   'fog.visible_at_rest': f['visible_at_rest'], 'fog.explored_at_rest': f['explored_at_rest'],
                   'anim.walk_cycle': walk})
    return {'values': values, 'curves': base['curves'], 'measured': dict(base['measured'], fog=f)}, None


# ---- report-only opening check (scenario t3-opening) -----------------------------
#
# N64 tape: the T3 launch tape recording the bullet pool and the player arrays for
# 15,000 in-mission steps, no input after the text boxes. GBA: the census runner on
# catalog mission 6 (T3) for as many source frames. T3 is busy enough that some ticks
# fall one frontend step apart and lose their follow-up record, and about one follow-up
# read in eleven sees a tick half done, so times here come from the tick counter
# (tick_clock), every observation is report-only, and the scenario admits tick skips.

T3_HUMAN, T3_COMPUTER = 1, 4
START_GRANT_WINDOW = 8                  # ticks: start banks are set by then (launch censuses)
BURROWING, NO_TECH = 11, 44
RESEARCH_TECH = 75                      # order value seen while the Hatchery researched (candidate)
TECH_RESEARCHED = 0x800BA1C4            # u8[12][44]
PLAYER_REGION = 0x800B9F00
GRENADE, GAUSS = 4, 0                   # weapons.dat ids: Fragmentation Grenade (Vulture), Gauss Rifle (Marine)
HYDRALISK = 38
T3_FIELDS = ['unit.order', 'unit.hp', 'unit.type', 'unit.owner', 'pool.bullet', 'player.tech_researched',
             'banks.minerals', 'clock.tick_counter', 'clock.pause_halfword']


def tick_clock(ticks):
    """{tick value: unpaused ticks since the first in-mission tick}, from the tick counter.
    A tick with no follow-up record (two ticks one step apart) counts as unpaused when the
    next record is; a paused record's own tick does not count."""
    clock, out, last = 0, {}, None
    for r in ticks:
        if not r['table']:
            continue
        if last is not None:
            gap = (r['tick'] - last) & 0xFFFFFFFF
            clock += gap - (1 if r['paused'] else 0)
        out[r['tick']] = clock
        last = r['tick']
    return out


def bullet_rows(block, table_address):
    """Live bullets of the 100 x 112-byte pool: {slot: (weapon, source row, target row, x, y, time)}."""
    def row_of(pointer):
        offset = pointer - table_address
        return offset // UNIT_POOL[1] if 0 <= offset < UNIT_POOL[0] and offset % UNIT_POOL[1] == 0 else None
    out = {}
    for k in range(BULLET_POOL[0] // BULLET_POOL[1]):
        b = block[k * BULLET_POOL[1]:(k + 1) * BULLET_POOL[1]]
        if struct.unpack_from('>I', b, 0x0C)[0]:
            out[k] = (b[0x60], row_of(struct.unpack_from('>I', b, 0x64)[0]), row_of(struct.unpack_from('>I', b, 0x5C)[0]),
                      struct.unpack_from('>H', b, 0x28)[0], struct.unpack_from('>H', b, 0x2A)[0], b[0x61])
    return out


def t3_measure_n64(cap, marks):
    import statistics
    ticks = [r for r in cap.ticks if r['table']]
    clock = tick_clock(ticks)
    bullet_index = next((i for i, (a, n) in enumerate(cap.regions) if n == BULLET_POOL[0]), None)
    player_index = next((i for i, (a, n) in enumerate(cap.regions) if a == PLAYER_REGION), None)
    if bullet_index is None or player_index is None:
        raise SystemExit('t3-opening: the capture does not record the bullet pool and the player arrays')
    table_address = None   # set from the first record's table offset (tap.bin header)
    hp = lambda row: cap.unit(row, 'hp') >> 8
    research_start = research_done = debit = None
    first_hit = first_hydra = first_delivery = None
    human_start = None
    grenades, gauss, live, last_hp = [], [], {}, {}
    hydra_launch = sum(1 for row in ticks[0]['rows'].values()
                       if cap.unit(row, 'owner') == T3_COMPUTER and cap.unit(row, 'type') == HYDRALISK)
    previous_bank = None
    for r in ticks:
        t = clock[r['tick']]
        rows = r['rows']
        minerals = cap.bank(r, 'minerals', T3_HUMAN)
        if t <= START_GRANT_WINDOW:
            human_start = minerals        # the first-pass SetResources grant lands in the first frames
        elif first_delivery is None and minerals > human_start:
            first_delivery = t
        bank = (cap.bank(r, 'minerals', T3_COMPUTER), cap.bank(r, 'gas', T3_COMPUTER, gating=False))
        hydras = 0
        for index, row in rows.items():
            owner, kind = cap.unit(row, 'owner'), cap.unit(row, 'type')
            if owner == T3_COMPUTER and kind == HYDRALISK:
                hydras += 1
            if research_start is None and owner == T3_COMPUTER and cap.unit(row, 'order') == RESEARCH_TECH \
                    and row[0xC4] == BURROWING:
                research_start = t
                if previous_bank is not None:
                    debit = (previous_bank[0] - bank[0], previous_bank[1] - bank[1])
            if owner == T3_HUMAN:
                now = hp(row)
                if first_hit is None and index in last_hp and now < last_hp[index]:
                    first_hit = t
                last_hp[index] = now
        if first_hydra is None and hydras > hydra_launch:
            first_hydra = t
        previous_bank = bank
        block = r.get('extra', [None] * len(cap.regions))[player_index]
        if research_done is None and block is not None and \
                block[TECH_RESEARCHED - PLAYER_REGION + T3_COMPUTER * TECHS + BURROWING]:
            research_done = t
        bullets = r.get('extra', [None] * len(cap.regions))[bullet_index]
        if bullets is None:
            continue
        if table_address is None:
            table_address = 0x80000000 | r['table_offset']
        now = bullet_rows(bullets, table_address)
        for slot, b in now.items():
            if slot not in live or live[slot]['b'][:3] != b[:3]:
                live[slot] = {'b': b, 'spawn': t, 'hit': None}
        aimed = Counter(entry['b'][2] for entry in live.values())
        for slot, entry in live.items():
            target = entry['b'][2]
            if entry['hit'] is None and target is not None and target in rows:
                before = entry.get('hp')
                value = hp(rows[target])
                if before is not None and value < before:
                    # Only a bullet alone on its target owns the hit; otherwise the hit is ambiguous.
                    entry['hit'] = t - entry['spawn'] if aimed[target] == 1 else 'ambiguous'
                entry['hp'] = value
        for slot in [s for s in live if s not in now]:
            entry = live.pop(slot)
            if entry['b'][0] == GRENADE and isinstance(entry['hit'], int):
                grenades.append(entry['hit'])
            if entry['b'][0] == GAUSS:
                gauss.append(t - entry['spawn'])
    if research_start is None or research_done is None:
        raise SystemExit('t3-opening: the computer Burrowing research was not seen from start to finish')
    if not grenades or not gauss:
        raise SystemExit('t3-opening: no Vulture grenade hit or Gauss Rifle bullet recorded')
    m = {'research_ticks': research_done - research_start, 'research_debit': debit,
         'grenade_hit': statistics.median(grenades), 'grenades': len(grenades),
         'gauss_life': statistics.median(gauss), 'gauss': len(gauss),
         'first_hydralisk': first_hydra, 'first_human_hit': first_hit, 'first_human_delivery': first_delivery,
         'horizon': max(clock.values())}
    o = lambda check, subject, value, unit: __import__('sc64_parity_metrics').observation(
        check, subject, value, value, value, 'report only: busy-mission records (tick clock)', False, T3_FIELDS, unit=unit)
    obs = [o('research.duration', 'Burrowing, computer Hatchery', m['research_ticks'],
             'unpaused ticks from the ResearchTech order to the researched flag'),
           o('bullet.grenade_hit', 'Vulture Fragmentation Grenade', m['grenade_hit'],
             'ticks from the bullet appearing to the target losing hit points (median)'),
           o('bullet.gauss_life', 'Marine Gauss Rifle', m['gauss_life'], 'ticks a Gauss Rifle bullet stays live (median)'),
           o('timeline.first_hydralisk', 'T3 computer (player 4)', m['first_hydralisk'],
             'unpaused ticks from launch to the computer\'s first new Hydralisk'),
           o('timeline.first_human_hit', 'T3 human', m['first_human_hit'],
             'unpaused ticks from launch to a human unit first losing hit points'),
           o('timeline.first_human_delivery', 'T3 human', m['first_human_delivery'],
             'unpaused ticks from launch to the human bank first rising')]
    gba = {'scenario': 'census', 'profile': 'terran', 'mission': 6, 'at_step': 5,
           'steps': int(m['horizon'] * 5 / 4) + 10, 'human': T3_HUMAN, 'computer': T3_COMPUTER}
    notes = [f'research debit {debit}; {len(grenades)} grenade hits, {len(gauss)} Gauss bullets; '
             f'tick skips {cap.faults["tick_skips"]}']
    return {'observations': obs, 'gba': gba, 'notes': notes, 'curves': {}, 'measured': m}


def t3_parse_gba(trace):
    return gba_timeline(trace, {T3_HUMAN})


def t3_measure_gba(series, golden, report):
    g = report.get('parsed')
    if g is None:
        return None, 'the GBA census trace was not parsed'
    human_engine = [e for e, src in g['engine_to_source_owner'].items() if src == T3_HUMAN]
    bank = g['banks_engine_owner'].get(human_engine[0], []) if human_engine else []
    start = max((minerals for sf, minerals, _ in bank if sf <= START_GRANT_WINDOW), default=None)
    delivery = next((sf for sf, minerals, _ in bank if sf > START_GRANT_WINDOW and start is not None and minerals > start),
                    None)
    hit = g['first_hit']['sf'] if g['first_hit'] else None
    return {'values': {'research.duration': None, 'bullet.grenade_hit': None, 'bullet.gauss_life': None,
                       'timeline.first_hydralisk': g['first'].get((T3_COMPUTER, HYDRALISK)),
                       'timeline.first_human_hit': hit, 'timeline.first_human_delivery': delivery},
            'curves': {}, 'measured': {'frames': g['frames']}}, None


FIELD_SCENARIOS = {
    't1-fields': {'title': 'T1 Wasteland: the first Move of five Marines, with order, fog and animation fields',
                  'measure_n64': fields_measure_n64, 'measure_gba': fields_measure_gba,
                  'profile': 'terran', 'runner': 'move'},
    't3-opening': {'title': 'T3 Desperate Alliance opening, no human input: research, bullets, timeline (report only)',
                   'measure_n64': t3_measure_n64, 'measure_gba': t3_measure_gba, 'parse_gba': t3_parse_gba,
                   'profile': 'terran', 'runner': 'census', 'tick_skips_ok': True},
}


def launch_report(run: pathlib.Path, label: str, inputs, columns, t1_tables=None) -> dict:
    """Census, table anchor and player arrays of one mission launch capture."""
    maps, extractor, _ = inputs
    entry = {'mission': label, 'run': run.name}
    manifest = json.loads((run / 'manifest.json').read_text())
    entry['capture_ok'] = manifest.get('ok') is True
    if not entry['capture_ok']:
        return dict(entry, verdict='capture failed')
    entry['census'] = capture_census(run, label, inputs)
    dump = run / 'start.ram'
    if dump.is_file():
        ram = dump.read_bytes()
        anchor = anchor_columns(ram, columns, table_hints())
        entry['tables'] = {'equal': sum(1 for f in anchor if f['status'] == 'equal'),
                           'differs': {f['column']: f.get('rows_differ') for f in anchor if f['status'] != 'equal'},
                           'diffs': {f['column']: f.get('diffs') for f in anchor if f['status'] != 'equal'}}
        entry['player_tables'] = player_tables_check(ram, mission_chk(label, maps, extractor)['sections'])
    return entry


def timeline_command(run: pathlib.Path, label: str, out, inputs) -> int:
    """N64 opening timeline of a launch capture against the GBA census runner for the
    same mission and horizon (the release profile's fast-trace build in build/parity)."""
    import subprocess
    import tempfile
    import parity_decode as pdx
    from tools import sc64_parity_content as content
    from tools.sc64_parity_tables import check_private, write_private, fixture_home
    maps, extractor, _ = inputs
    chk = mission_chk(label, maps, extractor)
    humans = {p for p in range(8) if p < len(chk['owners']) and chk['owners'][p] == 6}
    cap = pdx.Capture(run)
    ticks = [r for r in cap.ticks if r['table']]
    clock = tick_clock(ticks)
    n64 = n64_timeline(cap, humans)
    # Re-time the N64 firsts on the tick clock (records can skip ticks in busy missions).
    by_sf = {r['sf']: clock[r['tick']] for r in ticks if not r['paused'] and r['sf'] >= 0}
    n64['first'] = {k: by_sf.get(v, v) for k, v in n64['first'].items()}
    if n64['first_hit']:
        n64['first_hit']['sf'] = by_sf.get(n64['first_hit']['sf'], n64['first_hit']['sf'])
    n64['frames'] = max(clock.values())
    mission = next(m for m, lab, _ in content.MISSIONS if lab == label)
    profile = content.PROFILE_BY_RACE[label[0]]
    binary = ROOT / 'build' / 'parity' / f'{profile}-fast-trace'
    if not binary.is_file():
        print(f'SKIP: {binary} is not built')
        return 0
    with tempfile.TemporaryDirectory(prefix='sc64-timeline-') as temp:
        trace = pathlib.Path(temp) / 'census.trace'
        steps = int(n64['frames'] * 5 / 4) + 10
        run_gba = subprocess.run([str(binary), '--scenario', 'census', '--mission', str(mission), '--steps', str(steps),
                                  '--trace', str(trace), '--no-route-checks', '--profile', profile],
                                 capture_output=True, text=True)
        if run_gba.returncode:
            print(f'GBA census runner failed: {run_gba.stderr.strip()[-300:]}')
            return 1
        gba = gba_timeline(trace, humans)
    result = timeline_compare(n64, gba, humans)
    result.update(mission=label, humans=sorted(humans), n64_banks={o: v[:12] for o, v in n64['banks'].items()},
                  gba_banks={o: v[:12] for o, v in gba['banks_engine_owner'].items()},
                  gba_engine_to_source_owner=gba['engine_to_source_owner'], tick_skips=cap.faults['tick_skips'])
    for row in result['first']:
        print(f"first owner {row['owner']:2} type {row['type']:3}  n64 {row['n64']}  gba {row['gba']}  delta {row['delta']}")
    print('first human hit', result['first_hit'])
    print('n64 banks', {o: v[:4] for o, v in result['n64_banks'].items()})
    print('gba banks', {o: v[:4] for o, v in result['gba_banks'].items()}, 'engine->source', gba['engine_to_source_owner'])
    target = out or fixture_home(ROOT) / 'fieldmap' / f'timeline-{label}.json'
    check_private(ROOT, target.parent)
    target.parent.mkdir(parents=True, exist_ok=True)
    write_private(ROOT, {target: json.dumps(result, indent=1, sort_keys=True, default=list) + '\n'})
    print(f'wrote {target}')
    return 0


def census_all(runs: pathlib.Path, out, inputs) -> int:
    from tools import sc64_parity_content as content
    from tools.sc64_parity_tables import check_private, write_private, fixture_home
    columns = table_columns(inputs[2])
    report = []
    for _, label, _ in sorted(content.MISSIONS, key=lambda m: ('TZP'.index(m[1][0]), int(m[1][1:]))):
        run = runs / f'm-{label}'
        if not (run / 'manifest.json').is_file():
            report.append({'mission': label, 'verdict': 'not captured'})
            continue
        report.append(launch_report(run, label, inputs, columns))
    for e in report:
        c = e.get('census', {})
        tables = e.get('tables', {})
        players = e.get('player_tables', {})
        banks = c.get('banks', [])
        print(f"{e['mission']:4} {c.get('verdict', e.get('verdict')):9} matched {c.get('matched')}/{c.get('chk_placements')} "
              f"moved {len(c.get('moved', []))} displaced {len(c.get('displaced', []))} "
              f"owner {len(c.get('owner_changed', []))} "
              f"extra {len(c.get('extra', []))} missing {len(c.get('missing', []))} "
              f"minerals {c.get('minerals', {}).get('matched')}/{c.get('minerals', {}).get('chk')} "
              f"geysers {c.get('geysers', {}).get('matched')}/{c.get('geysers', {}).get('chk')} "
              f"amounts {len(c.get('amount_differs', []))} banks "
              f"{sum(1 for b in banks if b['verdict'] == 'match')}/{len(banks)} selector {c.get('selectors')} "
              f"tables {tables.get('equal')} differ {sorted(tables.get('differs', {}))} "
              f"players {' '.join(k + ('=' if v['equal'] else '!') for k, v in players.items())}")
    target = out or fixture_home(ROOT) / 'fieldmap' / 'launches.json'
    check_private(ROOT, target.parent)
    target.parent.mkdir(parents=True, exist_ok=True)
    write_private(ROOT, {target: json.dumps(report, indent=1, sort_keys=True) + '\n'})
    print(f'wrote {target}')
    return 0


# ---- campaign opening timelines (WP10, first slice) ----------------------------

def first_rises(samples):
    """samples: [(sf, Counter of (owner, type))] in frame order. The first frame each key's
    count rises above its count in the first sample (a unit produced, morphed or created)."""
    if not samples:
        return {}, Counter()
    launch = samples[0][1]
    first = {}
    for sf, counts in samples[1:]:
        for key, n in counts.items():
            if key not in first and n > launch.get(key, 0):
                first[key] = sf
    return first, launch


def n64_timeline(cap, humans: set) -> dict:
    """Computer behaviour from a launch capture: first rises per (owner, type), the first
    hit point loss of a human unit (candidate hp field, report only) and each owner's
    mineral and gas bank changes, in unpaused source frames from the first in-mission tick."""
    samples, hp, first_hit, banks = [], {}, None, defaultdict(list)
    for r in cap.ticks:
        if not r['table'] or r['paused'] or r['sf'] < 0:
            continue
        counts = Counter()
        for index, row in r['rows'].items():
            owner, kind = row_field(row, 'owner'), row_field(row, 'type')
            counts[(owner, kind)] += 1
            if owner in humans:
                value = row_field(row, 'hp')
                if first_hit is None and index in hp and value < hp[index] and kind not in (LARVA, EGG):
                    first_hit = {'sf': r['sf'], 'type': kind, 'owner': owner}
                hp[index] = value
        samples.append((r['sf'], counts))
        g = r['globals']
        minerals, gas = struct.unpack_from('>12I', g, 0), struct.unpack_from('>12I', g, 0x30)
        for owner in range(12):
            value = (minerals[owner], gas[owner])
            if not banks[owner] or banks[owner][-1][1:] != value:
                banks[owner].append((r['sf'],) + value)
    first, launch = first_rises(samples)
    return {'frames': samples[-1][0] if samples else 0, 'first': first, 'launch': launch, 'first_hit': first_hit,
            'banks': {o: v for o, v in banks.items() if len(v) > 1 or v[0][1:] != (0, 0)}}


def gba_timeline(trace_path, humans: set) -> dict:
    """The same from a GBA census trace (tests/sc64_parity_trace.h): unit and building rows
    with raw types and source owners, D hit records and P banks (engine owners)."""
    live, gone, raw_owner_of, samples, banks = {}, {}, {}, [], defaultdict(list)
    first_hit, step, max_units = None, None, None

    def flush():
        if step is not None:
            counts = Counter((o, t) for o, t in live.values())
            sf = step * 4 // 5
            if samples and samples[-1][0] == sf:
                samples[-1] = (sf, counts)
            else:
                samples.append((sf, counts))
    with open(trace_path) as handle:
        if not handle.readline().startswith('#sc64-parity-trace 1'):
            raise ValueError(f'{trace_path}: not a sc64-parity-trace/1 file')
        for line in handle:
            w = line.split()
            if not w:
                continue
            tag = w[0]
            if tag == 'H':
                fields = dict(x.split('=', 1) for x in w[1:] if '=' in x)
                if 'max_units' in fields:
                    max_units = int(fields['max_units'])
            elif tag == 'S':
                flush()
                step = int(w[1])
                gone = {}
            elif tag in ('U', 'B') and len(w) > 5:
                key = (tag, int(w[1]))
                raw_owner = int(w[5]) if int(w[5]) >= 0 else int(w[4])
                live[key] = (raw_owner, int(w[3]))
                raw_owner_of[int(w[4])] = raw_owner
            elif tag in ('-U', '-B'):
                # Removals are written before the step's hits: keep the row for this step so a
                # first hit that kills still finds its victim.
                key = (tag[1], int(w[1]))
                if key in live:
                    gone[key] = live.pop(key)
            elif tag == 'D' and first_hit is None and step is not None and max_units is not None:
                # The target is a unit slot, or max_units + a building slot.
                target = int(w[1])
                key = ('U', target) if target < max_units else ('B', target - max_units)
                row = live.get(key) or gone.get(key)
                if row is not None and row[0] in humans:
                    first_hit = {'sf': step * 4 // 5, 'type': row[1], 'owner': row[0]}
            elif tag == 'P' and step is not None:
                owner = int(w[1])
                value = (int(w[2]), int(w[3]))
                if not banks[owner] or banks[owner][-1][1:] != value:
                    banks[owner].append((step * 4 // 5,) + value)
    flush()
    first, launch = first_rises(samples)
    return {'frames': samples[-1][0] if samples else 0, 'first': first, 'launch': launch, 'first_hit': first_hit,
            'banks_engine_owner': dict(banks), 'engine_to_source_owner': raw_owner_of}


def timeline_compare(n64: dict, gba: dict, humans: set) -> dict:
    horizon = min(n64['frames'], gba['frames'])
    keys = sorted(set(n64['first']) | set(gba['first']))
    rows = []
    for owner, kind in keys:
        if owner in humans:
            continue
        a, b = n64['first'].get((owner, kind)), gba['first'].get((owner, kind))
        if (a is None or a > horizon) and (b is None or b > horizon):
            continue
        rows.append({'owner': owner, 'type': kind,
                     'n64': a if a is not None and a <= horizon else None,
                     'gba': b if b is not None and b <= horizon else None,
                     'delta': (b - a) if a is not None and b is not None and a <= horizon and b <= horizon else None})
    launch_same = {k: (n64['launch'].get(k, 0), gba['launch'].get(k, 0)) for k in set(n64['launch']) | set(gba['launch'])
                   if n64['launch'].get(k, 0) != gba['launch'].get(k, 0)}
    return {'horizon': horizon, 'first': rows, 'launch_differs': {f'{o}:{t}': v for (o, t), v in launch_same.items()},
            'first_hit': {'n64': n64['first_hit'], 'gba': gba['first_hit']}}


# ---- command line ------------------------------------------------------------

def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest='command', required=True)
    c = sub.add_parser('census')
    c.add_argument('run', type=pathlib.Path)
    c.add_argument('--mission', required=True)
    t = sub.add_parser('tables')
    t.add_argument('dumps', nargs='+', type=pathlib.Path)
    w = sub.add_parser('timeline', help='opening timeline of one launch capture against the GBA census runner')
    w.add_argument('run', type=pathlib.Path)
    w.add_argument('--mission', required=True)
    w.add_argument('--out', type=pathlib.Path, help='private JSON report (default $SC64_PARITY_HOME/fieldmap/timeline-<MISSION>.json)')
    a = sub.add_parser('census-all', help='every m-<MISSION> launch run in a directory: census, tables, player arrays')
    a.add_argument('runs', type=pathlib.Path)
    a.add_argument('--out', type=pathlib.Path, help='private JSON report (default $SC64_PARITY_HOME/fieldmap/launches.json)')
    args = parser.parse_args(argv)
    from tools.sc64_parity_tables import InputsMissing
    try:
        inputs = rom_inputs()
    except InputsMissing as error:
        print(f'SKIP: private inputs absent: {error}')
        return 0
    if args.command == 'census':
        print(json.dumps(capture_census(args.run, args.mission, inputs), indent=1))
        return 0
    if args.command == 'census-all':
        return census_all(args.runs, args.out, inputs)
    if args.command == 'timeline':
        return timeline_command(args.run, args.mission, args.out, inputs)
    columns = table_columns(inputs[2])
    for dump in args.dumps:
        found = anchor_columns(dump.read_bytes(), columns, table_hints())
        tally = Counter(f['status'] for f in found)
        print(json.dumps({'dump': dump.name, 'tally': tally,
                          'first': {f['column']: f['address'] for f in found if f['column'].endswith(('.flingy', '.mineral_base', '.minerals', '.label', '.sprite'))},
                          'columns': [f for f in found if f['status'] != 'equal']},
                         indent=1))
    return 0


if __name__ == '__main__':
    sys.exit(main())
