#!/usr/bin/env python3
"""Replay controller-only SC64 Races runs on the actual GBA ARM build.

The runner writes only scheduled controller inputs and their scheduler
metadata. It reads game state and profiling counters through mGBA's debugger.
Campaign traces can verify every host checkpoint before accepting completion.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import socket
import struct
import subprocess
import sys
import time

sys.path.insert(0, str(Path(__file__).resolve().parent))
from emulator_smoke import ROOT, RSP, capture, require_mgba, symbol_sizes, symbols

CPU_HZ = 16_777_216
UNIT_CAPACITIES = (128, 160)
MAX_BUILDINGS = 64
MAX_CONTROLLER_EVENTS = 2048
KEY_A, KEY_B, KEY_SELECT, KEY_START, KEY_RIGHT, KEY_LEFT, KEY_UP, KEY_DOWN, KEY_L = 1, 2, 4, 8, 16, 32, 64, 128, 512

PROFILES = {
    'z1': ('Z1', 0, 'campaign'),
    'z2': ('Z2', 1, 'campaign'),
    'p1': ('P1', 2, 'campaign'),
    'p2': ('P2', 3, 'campaign'),
    'skirmish': ('mixed-race-skirmish', -1, 'skirmish'),
    't1': ('T1', 4, 'campaign'),
    't2': ('T2', 5, 'campaign'),
}


def read_u32(rsp: RSP, address: int) -> int:
    return int.from_bytes(rsp.read(address, 4), 'little')


class DwarfOffsets(dict[str, int]):
    """Offset mapping that also carries compiler-derived field ABI metadata."""

    def __init__(self, offsets: dict[str, int], widths: dict[str, int],
                 signed: dict[str, bool]):
        super().__init__(offsets)
        self.widths = widths
        self.signed = signed


def _dwarf_dies(text: str) -> list[dict]:
    header = re.compile(r'^\s*<(\d+)><([0-9a-f]+)>: Abbrev Number: \d+ \((DW_TAG_[^)]+)\)')
    dies = []
    current = None
    for line in text.splitlines():
        match = header.match(line)
        if match:
            if current is not None:
                dies.append(current)
            current = {'depth': int(match.group(1)), 'offset': int(match.group(2), 16),
                       'tag': match.group(3), 'lines': [line]}
        elif current is not None:
            current['lines'].append(line)
    if current is not None:
        dies.append(current)
    return dies


def _dwarf_attr(die: dict, name: str) -> str | None:
    for line in die['lines']:
        match = re.search(re.escape(name) + r'\s*:\s*(.*)$', line)
        if match:
            value = match.group(1).strip()
            # readelf prints indirect strings as "(indirect string, ...): value".
            if value.startswith('(') and '): ' in value:
                value = value.split('): ', 1)[1]
            return value
    return None


def _dwarf_ref(die: dict, name: str = 'DW_AT_type') -> int | None:
    value = _dwarf_attr(die, name)
    match = re.search(r'<0x([0-9a-f]+)>', value or '')
    return int(match.group(1), 16) if match else None


def _dwarf_name(die: dict) -> str | None:
    value = _dwarf_attr(die, 'DW_AT_name')
    if value is None:
        return None
    match = re.match(r'([A-Za-z_][A-Za-z0-9_]*)\s*$', value)
    return match.group(1) if match else value


def parse_dwarf_struct(text: str, name: str, elf_label: str = 'ELF') -> tuple[int, DwarfOffsets]:
    """Resolve a typedef chain and return member offsets plus compiler ABI widths."""
    dies = _dwarf_dies(text)
    by_offset = {die['offset']: die for die in dies}
    typedef = next((die for die in dies if die['depth'] == 1 and
                    die['tag'] == 'DW_TAG_typedef' and _dwarf_name(die) == name), None)
    if typedef is None:
        raise RuntimeError(f'{elf_label} has no DWARF typedef for {name}')

    type_offset = _dwarf_ref(typedef)
    struct_die = None
    seen = set()
    while type_offset is not None and type_offset not in seen:
        seen.add(type_offset)
        die = by_offset.get(type_offset)
        if die is None:
            break
        if die['tag'] in ('DW_TAG_structure_type', 'DW_TAG_union_type'):
            struct_die = die
            break
        type_offset = _dwarf_ref(die)
    else:
        struct_die = None
    if type_offset is None or struct_die is None:
        raise RuntimeError(f'{elf_label} has no complete DWARF structure for {name}')
    size_text = _dwarf_attr(struct_die, 'DW_AT_byte_size')
    if size_text is None or not size_text.isdigit():
        raise RuntimeError(f'Cannot read {name} DWARF structure size')

    def scalar_type(ref: int | None, trail: set[int] | None = None) -> tuple[int, bool] | None:
        if ref is None:
            return None
        trail = set() if trail is None else trail
        if ref in trail:
            return None
        trail.add(ref)
        info = by_offset.get(ref)
        if info is None:
            return None
        if info['tag'] == 'DW_TAG_base_type':
            byte_text = _dwarf_attr(info, 'DW_AT_byte_size')
            encoding_text = _dwarf_attr(info, 'DW_AT_encoding')
            if byte_text and byte_text.isdigit():
                encoding_match = re.match(r'(\d+)', encoding_text or '')
                encoding = int(encoding_match.group(1)) if encoding_match else 0
                return int(byte_text), encoding in (5, 6)
            return None
        if info['tag'] in ('DW_TAG_typedef', 'DW_TAG_const_type', 'DW_TAG_volatile_type',
                           'DW_TAG_restrict_type', 'DW_TAG_atomic_type'):
            return scalar_type(_dwarf_ref(info), trail)
        return None

    offsets: dict[str, int] = {}
    widths: dict[str, int] = {}
    signed: dict[str, bool] = {}
    struct_index = dies.index(struct_die)
    for member in dies[struct_index + 1:]:
        if member['depth'] <= struct_die['depth']:
            break
        if member['depth'] != struct_die['depth'] + 1 or member['tag'] != 'DW_TAG_member':
            continue
        field_name = _dwarf_name(member)
        offset_text = _dwarf_attr(member, 'DW_AT_data_member_location')
        if field_name is None or offset_text is None:
            continue
        offset_match = re.match(r'(\d+)', offset_text)
        if not offset_match:
            continue
        offsets[field_name] = int(offset_match.group(1))
        layout = scalar_type(_dwarf_ref(member))
        if layout is not None:
            widths[field_name], signed[field_name] = layout
    return int(size_text), DwarfOffsets(offsets, widths, signed)


def dwarf_struct_layout(elf: Path, name: str) -> tuple[int, DwarfOffsets]:
    """Read a C struct's DWARF size, field offsets, and scalar ABI widths."""
    text = subprocess.run(['arm-none-eabi-readelf', '--debug-dump=info', str(elf)],
                          check=True, text=True, capture_output=True).stdout
    return parse_dwarf_struct(text, name, elf.name)


def dwarf_struct(elf: Path, name: str) -> tuple[int, dict[str, int]]:
    """Compatibility wrapper retaining the original (size, offsets) API."""
    return dwarf_struct_layout(elf, name)


def race_bank_profile(elf: Path, sizes: dict[str, int]) -> dict:
    unit_stride, unit_offsets = dwarf_struct_layout(elf, 'Unit')
    building_stride, building_offsets = dwarf_struct_layout(elf, 'Building')
    unit_bytes, building_bytes = sizes.get('u', 0), sizes.get('b', 0)
    if not unit_bytes or not building_bytes or unit_bytes % unit_stride or building_bytes % building_stride:
        raise RuntimeError(f'Races ELF bank sizes do not match DWARF: {unit_bytes}/{building_bytes}, '
                           f'strides {unit_stride}/{building_stride}')
    if unit_stride % 4 or building_stride % 4:
        raise RuntimeError('Unit and Building layouts must be 32-bit aligned')
    unit_capacity, building_capacity = unit_bytes // unit_stride, building_bytes // building_stride
    legacy_bank = unit_capacity in UNIT_CAPACITIES and building_capacity == MAX_BUILDINGS
    compact_zp_bank = (unit_capacity == 240 and building_capacity == 160 and
                       unit_stride == 56 and building_stride == 48)
    # The canonical late-Terran ROM keeps compact 56-byte units beside the
    # legacy int-field buildings; each field is read at its DWARF width.
    late_terran_bank = (unit_capacity == 192 and building_capacity == 96 and
                        unit_stride == 56)
    if not legacy_bank and not compact_zp_bank and not late_terran_bank:
        raise RuntimeError(f'Unexpected Races bank capacity: {unit_capacity}/{building_capacity}')
    def validate_fields(label: str, fields: DwarfOffsets, stride: int,
                        required: dict[str, int], optional: dict[str, int]) -> None:
        expected = {**required, **{key: val for key, val in optional.items() if key in fields}}
        for field, legacy_width in expected.items():
            if field not in fields:
                raise RuntimeError(f'DWARF {label} layout is missing entity field {field}')
            width = fields.widths.get(field)
            offset = fields[field]
            expected_width = (legacy_width if compact_zp_bank else 4)
            if late_terran_bank and width in (legacy_width, 4):
                expected_width = width
            if (width != expected_width or offset < 0 or offset + expected_width > stride):
                raise RuntimeError(f'DWARF {label} field {field} has an unsupported or unbounded ABI width')

    validate_fields('Unit', unit_offsets, unit_stride,
                    {'owner': 1, 'type': 1, 'alive': 1,
                     'x': 2, 'y': 2, 'hp': 4},
                    {'raw_type': 2, 'shield': 4})
    validate_fields('Building', building_offsets, building_stride,
                    {'owner': 1, 'type': 1, 'alive': 1,
                     'x': 2, 'y': 2, 'hp': 4, 'under_construction': 1},
                    {'raw_type': 2, 'shield': 4})
    return {'unit_bank_bytes': unit_bytes, 'building_bank_bytes': building_bytes,
            'unit_capacity': unit_capacity, 'building_capacity': building_capacity,
            'unit_stride_bytes': unit_stride, 'building_stride_bytes': building_stride,
            'unit_words': unit_stride // 4, 'building_words': building_stride // 4,
            'unit_offsets': unit_offsets, 'building_offsets': building_offsets,
            'unit_field_widths': unit_offsets.widths,
            'building_field_widths': building_offsets.widths}


def read_entities(rsp: RSP, address: int, capacity: int, stride: int,
                  fields: dict[str, int], *, building: bool = False) -> dict:
    data = rsp.read(address, capacity * stride)
    if len(data) != capacity * stride:
        raise RuntimeError('Debugger returned a truncated entity bank')
    by_owner: dict[str, int] = {}
    by_owner_type: dict[str, dict[str, int]] = {}
    completed: dict[str, dict[str, int]] = {}
    alive_units = []
    alive_buildings = []
    widths = getattr(fields, 'widths', {})
    signed = getattr(fields, 'signed', {})
    for i in range(capacity):
        row = data[i * stride:(i + 1) * stride]
        def field(name: str) -> int:
            if name not in fields:
                raise RuntimeError(f'DWARF layout is missing entity field {name}')
            offset = fields[name]
            width = widths.get(name, 4)
            if width not in (1, 2, 4) or offset < 0 or offset + width > stride:
                raise RuntimeError(f'DWARF entity field {name} exceeds its bounded ABI width')
            return int.from_bytes(row[offset:offset + width], 'little', signed=signed.get(name, True))
        if not field('alive'):
            continue
        owner, kind = field('owner'), field('type')
        owner_key, kind_key = str(owner), str(kind)
        by_owner[owner_key] = by_owner.get(owner_key, 0) + 1
        type_counts = by_owner_type.setdefault(owner_key, {})
        type_counts[kind_key] = type_counts.get(kind_key, 0) + 1
        entry = {'slot': i, 'owner': owner, 'type': kind, 'x': field('x'), 'y': field('y')}
        if 'raw_type' in fields: entry['raw_type'] = field('raw_type')
        if building:
            entry['under_construction'] = field('under_construction')
            alive_buildings.append(entry)
            if not entry['under_construction']:
                completed.setdefault(owner_key, {})[kind_key] = completed.setdefault(owner_key, {}).get(kind_key, 0) + 1
        else:
            entry['hp'] = field('hp')
            if 'shield' in fields: entry['shield'] = field('shield')
            alive_units.append(entry)
    result = {'alive': sum(by_owner.values()), 'by_owner': by_owner,
              'by_owner_type': by_owner_type}
    if building:
        result['completed_by_owner_type'] = completed
        result['entities'] = alive_buildings
    else:
        result['entities'] = alive_units
    return result


def campaign_state(rsp: RSP, address: int, size: int, offsets: dict[str, int]) -> dict:
    if size < 20:
        raise RuntimeError(f'Race campaign state has unexpected size: {size}')
    raw = rsp.read(address, size)
    widths = {'elapsed_frames': 4, 'objective_flags': 4,
              'rescue_unit': 2, 'chrysalis_unit': 2, 'carrier_unit': 2, 'enemy_units_sent': 2, 'message_ticks': 2,
              'mission': 1, 'objective_stage': 1, 'phase': 1, 'message': 1, 'enemy_wave': 1}
    result = {}
    for field, width in widths.items():
        if field not in offsets: continue
        result[field] = int.from_bytes(raw[offsets[field]:offsets[field] + width], 'little')
    return result


def observed_state(rsp: RSP, sym: dict[str, int], sizes: dict[str, int],
                   bank: dict, campaign_offsets: dict[str, int]) -> dict:
    units = read_entities(rsp, sym['u'], bank['unit_capacity'], bank['unit_stride_bytes'], bank['unit_offsets'])
    buildings = read_entities(rsp, sym['b'], bank['building_capacity'], bank['building_stride_bytes'],
                              bank['building_offsets'], building=True)
    state = {name: read_u32(rsp, sym[name]) for name in
             ('screen', 'frame', 'minerals', 'gas', 'won', 'lost', 'race_mode',
              'race_human', 'race_selected_mission', 'race_bots', 'race_completed')}
    state['units'], state['buildings'] = units, buildings
    if 'race_player' in sym:
        size = sizes.get('race_player', 20)
        state['race_player'] = list(struct.unpack('<' + 'i' * (size // 4), rsp.read(sym['race_player'], size)))
    if 'race_campaign' in sym:
        campaign = campaign_state(rsp, sym['race_campaign'], sizes.get('race_campaign', 24), campaign_offsets)
        state['campaign'] = campaign
        for field in ('elapsed_frames', 'objective_flags', 'phase', 'mission'):
            if field in campaign:
                state[field] = campaign[field]
        for label, index_field in (('rescue', 'rescue_unit'), ('chrysalis', 'chrysalis_unit'),
                                  ('carrier', 'carrier_unit')):
            slot = campaign.get(index_field, 0xFFFF)
            entity = next((unit for unit in units['entities'] if unit['slot'] == slot), None)
            state[f'{label}_owner'] = entity['owner'] if entity else -1
            state[f'{label}_alive'] = int(entity is not None)
            if label != 'rescue':
                state[f'{label}_x'] = entity['x'] if entity else -1
                state[f'{label}_y'] = entity['y'] if entity else -1
        state['carrier_unit'] = campaign.get('carrier_unit', 0xFFFF)
    for side, owner in (('side', 0), ('enemy', 1)):
        state[f'{side}_units'] = units['by_owner'].get(str(owner), 0)
        state[f'{side}_buildings'] = buildings['by_owner'].get(str(owner), 0)
    # Stable campaign aliases use the runtime roster IDs so they include both
    # imported source actors and units/buildings created during play.
    for alias, unit_type in (('drones', 7), ('zerglings', 4),
                             ('hydras', 5), ('overlords', 8)):
        state[alias] = sum(1 for unit in units['entities']
                           if unit['owner'] == 0 and unit['type'] == unit_type)
    for alias, building_type in (('extractors', 20), ('spawning_pools', 10),
                                 ('hydralisk_dens', 11)):
        state[alias] = sum(1 for building in buildings['entities']
                           if building['owner'] == 0 and building['type'] == building_type)
    state['pools'] = state['spawning_pools']
    state['dens'] = state['hydralisk_dens']
    return state


def assert_subset(expected: dict, actual: dict, where: str = 'state') -> None:
    for key, value in expected.items():
        if key not in actual:
            raise AssertionError(f'{where}.{key} is absent from ARM observation')
        observed = actual[key]
        if isinstance(value, dict) and isinstance(observed, dict):
            assert_subset(value, observed, f'{where}.{key}')
        elif observed != value:
            raise AssertionError(f'{where}.{key}: ARM={observed!r}, host={value!r}')


def smoke_events(profile: str, bots: int, human_race: str,
                 unified: bool = False) -> list[tuple[int, int]]:
    events: list[tuple[int, int]] = []
    tick = 0
    def press(key: int) -> None:
        nonlocal tick
        events.extend(((tick, key), (tick + 1, 0)))
        tick += 2
    if profile == 'skirmish':
        press(KEY_DOWN)  # HOME: Skirmish
        press(KEY_A)
        wanted = {'terran': 0, 'zerg': 1, 'protoss': 2}[human_race]
        delta = (wanted - (0 if unified else 1)) % 3  # unified defaults to Terran
        for _ in range(delta): press(KEY_RIGHT)
        press(KEY_DOWN)  # Opponents
        bot_delta = (bots - 2) % 3
        for _ in range(bot_delta): press(KEY_RIGHT)
        press(KEY_DOWN)  # Launch
        press(KEY_A)
    else:
        mission = PROFILES[profile][1]
        press(KEY_A)  # HOME: Campaign
        if unified:
            # Unified campaign opens on Terran. Faction rows are Terran,
            # Zerg, Protoss; legacy builds open on Zerg and use RIGHT once.
            faction_row = 0 if mission >= 4 else 1 if mission < 2 else 2
            for _ in range(faction_row): press(KEY_DOWN)
        elif mission >= 2:
            press(KEY_RIGHT)  # Protoss faction
        press(KEY_A)  # Confirm faction
        if mission & 1: press(KEY_DOWN)
        press(KEY_A)  # Mission selection → briefing
        press(KEY_START)  # Explicitly skip briefing → play
    return events


def campaign_start_events(profile: str, unified: bool = False) -> tuple[list[tuple[int, int]], int]:
    """Build the title navigation prefix and return its first free tick."""
    events = smoke_events(profile, bots=2, human_race='zerg', unified=unified)
    return events, max(t for t, _ in events) + 1


def run(profile: str, *, ticks: int = 10000, checkpoint: int = 1000,
        fps_target: int = 600, trace_path: Path | None = None,
        bots: int = 2, human_race: str = 'zerg', save_load: bool = False,
        fresh_load_save: Path | None = None, unified: bool = False) -> dict:
    mgba = require_mgba()
    if profile not in PROFILES:
        raise ValueError(f'Unsupported races profile {profile!r}')
    if profile in ('t1', 't2') and not unified:
        raise ValueError('T1 and T2 profiles require --unified')
    if save_load and (trace_path is not None or profile != 'z1'):
        raise ValueError('--save-load is a standalone Z1 controller round-trip test')
    if fresh_load_save is not None and (trace_path is not None or profile != 'z1' or save_load):
        raise ValueError('--fresh-load-save is a standalone Z1 cold-load test')
    if save_load:
        ticks = 1000
    if fresh_load_save is not None:
        ticks = 1000
    if ticks < 1 or checkpoint != 1000:
        raise ValueError('ticks must be positive; checkpoints are every 1000 ticks')
    trace = json.loads(trace_path.read_text()) if trace_path else None
    if trace:
        ticks = trace['total_ticks']
        if not isinstance(ticks, int) or ticks < 1:
            raise ValueError('Trace total_ticks must be a positive integer')
        events = [(int(event['tick']), int(event['held'])) for event in trace['events']]
    elif save_load:
        events, prefix_end = campaign_start_events('z1', unified=unified)
        events.extend((prefix_end + offset, held) for offset, held in (
            (20, KEY_START), (21, 0), (22, KEY_B), (23, 0), (24, KEY_A), (25, 0),
            (990 - prefix_end, KEY_START), (991 - prefix_end, 0),
            (992 - prefix_end, KEY_L), (993 - prefix_end, 0),
            (994 - prefix_end, KEY_START), (995 - prefix_end, 0)))
    elif fresh_load_save is not None:
        events = [(0, KEY_B), (1, 0)]
    else:
        events = smoke_events(profile, bots, human_race, unified=unified)
    if len(events) > MAX_CONTROLLER_EVENTS or any(t < 0 or t >= ticks or not 0 <= h <= 1023 for t, h in events):
        raise ValueError(f'Invalid controller input queue ({len(events)} events)')
    if not events or events != sorted(events) or len({t for t, _ in events}) != len(events):
        raise ValueError('Controller events must be nonempty and strictly ordered by tick')

    rom = ROOT / ('build/StarCraft64-GBA.gba' if unified else
                  'build/StarCraft64-Races-GBA.gba')
    elf = ROOT / ('build/starcraft64-unified.elf' if unified else
                  'build/starcraft64-races.elf')
    if not rom.is_file() or not elf.is_file():
        build_name = 'unified' if unified else 'Races'
        raise RuntimeError(f'StarCraft64 {build_name} ARM build is missing')
    sym, sizes = symbols(elf), symbol_sizes(elf)
    bank = race_bank_profile(elf, sizes)
    csize, coffsets = dwarf_struct(elf, 'Sc64RaceCampaignState')
    required = ('game_update', 'demo_schedule_events', 'demo_schedule_length',
                'demo_schedule_count', 'demo_schedule_tick', 'demo_replay_checkpoint',
                'demo_tick_buckets', 'demo_last_cycles', 'demo_peak_cycles', 'u', 'b',
                'screen', 'frame', 'minerals', 'gas', 'won', 'lost', 'race_mode',
                'race_human', 'race_selected_mission', 'race_bots', 'race_completed',
                'race_campaign', 'race_player')
    missing = [name for name in required if name not in sym]
    if missing:
        raise RuntimeError(f'Races ELF is missing symbols: {", ".join(missing)}')
    capacity = sizes['demo_schedule_events'] // 8
    if len(events) > capacity:
        raise ValueError(f'Controller queue exceeds ELF capacity ({len(events)}/{capacity})')

    label = 'smoke' if trace_path is None else trace_path.stem
    result_root = 'build/emulator/sc64-unified' if unified else 'build/emulator/sc64-races'
    save_name = 'StarCraft64-GBA.sav' if unified else 'StarCraft64-Races-GBA.sav'
    run_dir = ROOT / result_root / f'{profile}-{label}-{time.time_ns()}'
    run_dir.mkdir(parents=True, exist_ok=True)
    save_dir = run_dir / 'saves'
    save_dir.mkdir()
    if fresh_load_save is not None:
        source_save = fresh_load_save.resolve()
        if not source_save.is_file() or source_save.stat().st_size != 32768:
            raise ValueError('Fresh load requires a 32 KiB mGBA SRAM snapshot')
        shutil.copy2(source_save, save_dir / save_name)
    log = (run_dir / 'mgba.log').open('ab')
    guard = socket.socket()
    try:
        guard.bind(('127.0.0.1', 2345))
    except OSError as exc:
        log.close()
        raise RuntimeError('Port 2345 is in use; leave the existing mGBA debugger untouched') from exc
    finally:
        guard.close()

    proc = subprocess.Popen([mgba, '-g', '-C', 'mute=1', '-C', 'audioSync=0',
                             '-C', 'videoSync=0', '-C', f'fpsTarget={fps_target}',
                             '-C', f'savegamePath={save_dir}', str(rom)],
                            stdout=log, stderr=log)
    connection = None
    try:
        deadline = time.monotonic() + 12
        while time.monotonic() < deadline:
            try:
                connection = socket.create_connection(('127.0.0.1', 2345), timeout=1)
                break
            except OSError:
                time.sleep(.1)
        if connection is None:
            raise RuntimeError('mGBA GDB server did not start')
        connection.settimeout(30)
        connection.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        rsp = RSP(connection)
        rsp.cmd('qSupported'); rsp.cmd('?')
        update = sym['game_update'] & ~1
        if rsp.cmd(f'Z1,{update:x},2') != 'OK':
            raise RuntimeError('Failed to stop at game_update')
        rsp.tick(2)
        title = capture(rsp, run_dir / 'title.png')
        queue = b''.join(struct.pack('<2I', tick, held) for tick, held in events)
        for offset in range(0, len(queue), 256):
            rsp.write(sym['demo_schedule_events'] + offset, queue[offset:offset + 256])
        rsp.write(sym['demo_schedule_length'], struct.pack('<I', ticks))
        rsp.write(sym['demo_schedule_count'], struct.pack('<I', len(events)))
        if rsp.cmd(f'z1,{update:x},2') != 'OK':
            raise RuntimeError('Failed to resume game_update')
        checkpoint_addr = sym['demo_replay_checkpoint'] & ~1
        if rsp.cmd(f'Z1,{checkpoint_addr:x},2') != 'OK':
            raise RuntimeError('Failed to install replay checkpoint breakpoint')
        rsp.tick()
        if read_u32(rsp, sym['demo_schedule_tick']) != 0:
            raise RuntimeError('Input scheduler did not stop at tick zero')

        timer = rsp.read(0x04000108, 8)
        last_clock = int.from_bytes(timer[:2], 'little') | (int.from_bytes(timer[4:6], 'little') << 16)
        last_tick = 0
        previous_buckets = struct.unpack('<8I', rsp.read(sym['demo_tick_buckets'], 32))
        route_previous = {name: read_u32(rsp, sym[name]) for name in ('sc64_route_hits', 'sc64_route_builds') if name in sym}
        observations = []
        while last_tick < ticks:
            rsp.tick()
            completed = read_u32(rsp, sym['demo_schedule_tick'])
            expected_tick = min(ticks, last_tick + checkpoint)
            if completed != expected_tick:
                raise RuntimeError(f'Expected scheduler tick {expected_tick}, stopped at {completed}')
            timer = rsp.read(0x04000108, 8)
            now = int.from_bytes(timer[:2], 'little') | (int.from_bytes(timer[4:6], 'little') << 16)
            elapsed = (now - last_clock) & 0xFFFFFFFF
            buckets = struct.unpack('<8I', rsp.read(sym['demo_tick_buckets'], 32))
            state = observed_state(rsp, sym, sizes, bank, coffsets)
            expected_checkpoint = next((c for c in (trace or {}).get('checkpoints', [])
                                        if c.get('tick') == completed), None)
            if expected_checkpoint:
                assert_subset({k: v for k, v in expected_checkpoint.items() if k != 'tick'}, state)
            allowed_screens = (1, 2, 3) if save_load else (1, 3)
            if state['screen'] not in allowed_screens:
                raise RuntimeError(f'Races run left the game screen at tick {completed}: {state["screen"]}')
            record = {'tick': completed, 'interval_cycles': elapsed,
                      'interval_effective_fps': round(CPU_HZ * (completed - last_tick) / max(1, elapsed), 2),
                      'last_tick_cycles': read_u32(rsp, sym['demo_last_cycles']),
                      'peak_tick_cycles': read_u32(rsp, sym['demo_peak_cycles']),
                      'hardware_frames_per_tick_histogram': [a - b for a, b in zip(buckets, previous_buckets)],
                      'profile_cycles': {key: read_u32(rsp, sym[key]) for key in
                          ('demo_update_cycles', 'gfx_draw_cycles', 'sc64_orders_cycles', 'sc64_sight_cycles',
                           'sc64_other_cycles', 'sc64_map_cycles', 'sc64_hud_cycles', 'sc64_background_cycles',
                           'sc64_building_cycles', 'sc64_unit_draw_cycles') if key in sym}, **state}
            if route_previous:
                route_now = {name: read_u32(rsp, sym[name]) for name in route_previous}
                record['route_counters'] = route_now
                record['route_interval'] = {name: route_now[name] - route_previous[name] for name in route_now}
                route_previous = route_now
            observations.append(record)
            print(f'ARM races {profile} tick {completed}/{ticks}: fps={record["interval_effective_fps"]:.2f}, '
                  f'units={state["units"]["alive"]}, buildings={state["buildings"]["alive"]}', flush=True)
            if completed == checkpoint:
                record['opening_capture'] = capture(rsp, run_dir / 'opening.png')
            if completed == ticks or state['screen'] == 3:
                record['late_capture'] = capture(rsp, run_dir / 'late.png')
            last_tick, last_clock, previous_buckets = completed, now, buckets
            if state['screen'] == 3:
                break

        final = observations[-1]
        if save_load and (final['screen'] != 2 or final['frame'] >= 100):
            raise AssertionError('SRAM load did not restore the early paused game state: '
                                 f'screen={final["screen"]}, frame={final["frame"]}')
        if fresh_load_save is not None and (final['screen'] != 1 or final['race_mode'] != 0 or
                                             final['race_selected_mission'] != 0):
            raise AssertionError('Fresh title-screen B load did not resume the saved Z1 campaign')
        if profile != 'skirmish':
            if final['race_mode'] != 0 or final['race_selected_mission'] != PROFILES[profile][1]:
                raise AssertionError(f'{profile} ran a different campaign: {final["race_selected_mission"]}')
        elif final['race_mode'] != 1 or final['race_human'] != {'terran':0,'zerg':1,'protoss':2}[human_race]:
            raise AssertionError('Skirmish ran a different faction or mode')
        if trace:
            assert_subset(trace.get('expected', {}), final)
            if PROFILES[profile][2] == 'campaign':
                if not final['won'] or final['lost'] or final['screen'] != 3:
                    raise AssertionError(f'{profile} controller trace did not complete the mission')
                if final['race_selected_mission'] != PROFILES[profile][1]:
                    raise AssertionError(f'{profile} trace completed a different mission')
            elif trace.get('expected', {}).get('won') != 1:
                raise AssertionError('Skirmish completion trace must expect won=1')
        total_cycles = sum(item['interval_cycles'] for item in observations)
        result = {'profile': profile, 'kind': PROFILES[profile][2], 'trace': str(trace_path) if trace_path else None,
                  'controller_completion_verified': bool(trace), 'all_host_checkpoints_match': bool(trace),
                  'save_load_roundtrip_verified': bool(save_load),
                  'fresh_process_sram_load_verified': bool(fresh_load_save),
                  'controller_input': [{'tick': t, 'held': h} for t, h in events],
                  'rom_sha256': hashlib.sha256(rom.read_bytes()).hexdigest(),
                  'elf_sha256': hashlib.sha256(elf.read_bytes()).hexdigest(),
                  'ticks': last_tick, 'requested_ticks': ticks, 'checkpoint_interval': checkpoint,
                  'emulator_fps_target': fps_target, 'source_profile': bank,
                  'mean_interval_fps': round(CPU_HZ * last_tick / max(1, total_cycles), 2),
                  'maximum_tick_cycles': max((item['peak_tick_cycles'] for item in observations), default=0),
                  'title_capture': title, 'captures': {'title': str(run_dir / 'title.png'),
                      'opening': str(run_dir / 'opening.png'), 'late': str(run_dir / 'late.png')},
                  'save_directory': str(save_dir), 'checkpoints': observations}
        if fresh_load_save is not None:
            result['fresh_load_source_save'] = str(fresh_load_save.resolve())
        target = run_dir / 'profile.json'
        target.write_text(json.dumps(result, indent=2) + '\n')
        print(json.dumps({'profile': profile, 'ticks': last_tick, 'mean_interval_fps': result['mean_interval_fps'],
                          'maximum_tick_cycles': result['maximum_tick_cycles'], 'result': str(target)}, indent=2))
        return result
    finally:
        if connection:
            connection.close()
        proc.terminate()
        try:
            proc.wait(timeout=3)
        except subprocess.TimeoutExpired:
            proc.kill(); proc.wait()
        log.close()


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('profile', choices=tuple(PROFILES))
    parser.add_argument('--ticks', type=int, default=10000)
    parser.add_argument('--checkpoint', type=int, default=1000)
    parser.add_argument('--fps-target', type=int, default=600)
    parser.add_argument('--trace', type=Path)
    parser.add_argument('--unified', action='store_true',
                        help='profile the unified StarCraft64 build (adds Terran T1/T2 campaigns)')
    parser.add_argument('--bots', type=int, choices=(1, 2, 3), default=2)
    parser.add_argument('--human-race', choices=('terran', 'zerg', 'protoss'), default='zerg')
    parser.add_argument('--save-load', action='store_true',
                        help='run a controller-only Z1 SRAM save, resume, and load round trip')
    parser.add_argument('--fresh-load-save', type=Path,
                        help='copy a 32 KiB SRAM snapshot and issue title-screen B to load it')
    args = parser.parse_args()
    run(args.profile, ticks=args.ticks, checkpoint=args.checkpoint,
        fps_target=args.fps_target, trace_path=args.trace,
        bots=args.bots, human_race=args.human_race, save_load=args.save_load,
        fresh_load_save=args.fresh_load_save, unified=args.unified)


if __name__ == '__main__':
    main()
