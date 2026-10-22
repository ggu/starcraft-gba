#!/usr/bin/env python3
"""Measure a long controller-only StarCraft64 ARM run in mGBA.

The runner injects a controller trace, or starts a match with neutral input
afterwards. Gameplay state is read through symbols and the hardware timer;
no world state, resources, unit health or save contents are written.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import socket
import struct
import subprocess
import time

from emulator_smoke import ROOT, MGBA, RSP, symbols, symbol_sizes, capture


CPU_HZ = 16_777_216
UNIT_BYTES = 88
BUILDING_BYTES = 72


def hardware_clock(rsp: RSP) -> int:
    data = rsp.read(0x04000108, 8)
    return int.from_bytes(data[:2], 'little') | (int.from_bytes(data[4:6], 'little') << 16)


def read_u32(rsp: RSP, address: int) -> int:
    return int.from_bytes(rsp.read(address, 4), 'little')


def bank_profile(sizes: dict[str, int], name: str) -> dict[str, int]:
    unit_bytes, building_bytes = sizes.get('u', 0), sizes.get('b', 0)
    if unit_bytes % UNIT_BYTES or building_bytes % BUILDING_BYTES:
        raise RuntimeError(f'{name} unit/building bank layout is not recognized: {unit_bytes}/{building_bytes} bytes')
    if not unit_bytes or not building_bytes:
        raise RuntimeError('ARM ELF must expose sized u and b symbols for bank verification')
    return {'unit_bank_bytes': unit_bytes, 'building_bank_bytes': building_bytes,
            'unit_capacity': unit_bytes // UNIT_BYTES,
            'building_capacity': building_bytes // BUILDING_BYTES}


def count_entities(rsp: RSP, sym: dict[str, int], profile: dict[str, int]) -> dict:
    units = rsp.read(sym['u'], profile['unit_bank_bytes'])
    raynor_alive, raynor_owner = 0, -1
    unit_by_owner: dict[str, int] = {}
    unit_by_owner_type: dict[str, dict[str, int]] = {}
    for slot in range(profile['unit_capacity']):
        row = struct.unpack_from('<22i', units, slot * UNIT_BYTES)
        owner, kind, alive = row[4:7]
        if kind == 6: raynor_alive, raynor_owner = alive, owner
        if not alive:
            continue
        owner_key, kind_key = str(owner), str(kind)
        unit_by_owner[owner_key] = unit_by_owner.get(owner_key, 0) + 1
        bucket = unit_by_owner_type.setdefault(owner_key, {})
        bucket[kind_key] = bucket.get(kind_key, 0) + 1

    buildings = rsp.read(sym['b'], profile['building_bank_bytes'])
    completed_by_owner_type: dict[str, dict[str, int]] = {}
    building_by_owner: dict[str, int] = {}
    building_by_owner_type: dict[str, dict[str, int]] = {}
    for slot in range(profile['building_capacity']):
        row = struct.unpack_from('<18i', buildings, slot * BUILDING_BYTES)
        owner, kind, alive = row[4:7]
        if not alive:
            continue
        owner_key, kind_key = str(owner), str(kind)
        building_by_owner[owner_key] = building_by_owner.get(owner_key, 0) + 1
        bucket = building_by_owner_type.setdefault(owner_key, {})
        bucket[kind_key] = bucket.get(kind_key, 0) + 1
        if not row[16]:
            complete = completed_by_owner_type.setdefault(owner_key, {})
            complete[kind_key] = complete.get(kind_key, 0) + 1
    return {
        'raynor_alive': raynor_alive, 'raynor_owner': raynor_owner,
        'units': {'alive': sum(unit_by_owner.values()), 'by_owner': unit_by_owner,
                  'by_owner_type': unit_by_owner_type},
        'buildings': {'alive': sum(building_by_owner.values()), 'by_owner': building_by_owner,
                      'by_owner_type': building_by_owner_type, 'completed_by_owner_type': completed_by_owner_type},
    }


def completion_state(observation: dict, rsp: RSP, sym: dict[str, int]) -> dict:
    human = observation['units']['by_owner_type'].get('0', {})
    enemy = observation['units']['by_owner_type'].get('1', {})
    buildings = observation['buildings']['by_owner_type'].get('1', {})
    result = dict(observation, marines=human.get('1', 0), firebats=human.get('2', 0),
        scvs=human.get('0', 0), scv=human.get('0', 0), zerg=enemy.get('4', 0),
        zerglings=enemy.get('4', 0), infested_cc=buildings.get('8', 0),
        barracks=observation['buildings']['completed_by_owner_type'].get('0', {}).get('1', 0))
    # Sc64BackwaterState: two u16, 15 u16 timers, 15 phases, outcome, objective.
    if 'mission_objective_stage' in sym: result['mission_objective_stage'] = read_u32(rsp, sym['mission_objective_stage'])
    for key in ('campaign_mission', 'campaign_completed'):
        if key in sym: result[key] = read_u32(rsp, sym[key])
    if 'backwater' in sym: result['objective'] = rsp.read(sym['backwater'] + 50, 1)[0]
    return result


def read_ai_bank(rsp: RSP, sym: dict[str, int], sizes: dict[str, int]) -> list[dict] | None:
    address, size = sym.get('skirmish_ai'), sizes.get('skirmish_ai', 0)
    if address is None or not size:
        return None
    stride, player_count = 44, size // 44
    if size % stride:
        return {'bytes': size, 'raw_hex': rsp.read(address, size).hex(), 'decode': 'unknown Sc64AiState size'}
    raw = rsp.read(address, size)
    result = []
    for i in range(player_count):
        off = i * stride
        head = struct.unpack_from('<13B', raw, off)
        values = struct.unpack_from('<15H', raw, off + 14)
        result.append({
            'player': head[0], 'team': head[1], 'active': head[2],
            'has_townhall': head[3], 'barracks': head[4],
            'relations': list(head[5:13]),
            'minerals': values[0], 'gas': values[1],
            'supply_used': values[2], 'supply_cap': values[3],
            'workers': values[4], 'workers_mining': values[5],
            'workers_gas': values[6], 'army': values[7],
            'queued_workers': values[8], 'queued_army': values[9],
            'queued_buildings': values[10], 'desired_workers': values[11],
            'desired_gas_workers': values[12], 'attack_threshold': values[13],
            'income_remainder': values[14],
        })
    return result


def run(profile_name: str, ticks: int = 6000, checkpoint: int = 1000,
        fps_target: int = 600, trace_path: Path | None = None, bots: int = 2) -> dict:
    stems = {
        'backwater': ('StarCraft64-Backwater-GBA', 'starcraft64-backwater'),
        'skirmish': ('StarCraft64-Skirmish-GBA', 'starcraft64-skirmish'),
        'campaign': ('StarCraft64-Terran-Campaign-GBA', 'starcraft64-campaign'),
    }
    if profile_name not in stems:
        raise ValueError(f'Unsupported profile: {profile_name}')
    trace = json.loads(trace_path.read_text()) if trace_path else None
    if trace: ticks = trace['total_ticks']
    if ticks < 1 or checkpoint != 1000:
        raise ValueError('ticks must be positive; firmware checkpoints are every 1000 ticks')
    stem, elf_stem = stems[profile_name]
    rom, elf = ROOT / 'build' / f'{stem}.gba', ROOT / 'build' / f'{elf_stem}.elf'
    if not rom.is_file() or not elf.is_file():
        raise RuntimeError(f'{stem} ARM build is missing; wait for its build to finish')
    sym, sizes = symbols(elf), symbol_sizes(elf)
    bank = bank_profile(sizes, profile_name)
    if (bank['unit_capacity'], bank['building_capacity']) != (96, 32):
        raise RuntimeError(f'{profile_name} ELF is not a 96-unit/32-building build: {bank}')
    required = ('game_update', 'demo_inject_keys', 'demo_schedule_events',
                'demo_schedule_length', 'demo_schedule_count', 'demo_schedule_tick',
                'demo_replay_checkpoint', 'demo_tick_buckets', 'demo_last_cycles',
                'demo_peak_cycles', 'u', 'b', 'screen', 'frame', 'minerals', 'gas', 'won', 'lost')
    missing = [key for key in required if key not in sym]
    if missing:
        raise RuntimeError(f'ARM ELF is missing profiling symbols: {", ".join(missing)}')

    out = ROOT / 'build' / 'emulator'
    out.mkdir(exist_ok=True)
    suffix = ('-full' if trace_path.name == 'campaign-completion-input.json' else '-wasteland' if 'wasteland' in trace_path.name else '-backwater') if profile_name == 'campaign' and trace_path else ''
    run_dir = out / f'sc64-{profile_name}{suffix}'
    run_dir.mkdir(exist_ok=True)
    save_dir = run_dir / f'saves-{time.time_ns()}'
    save_dir.mkdir(exist_ok=True)
    log = (run_dir / 'mgba.log').open('ab')
    probe = socket.socket()
    try:
        probe.bind(('127.0.0.1', 2345))
    except OSError as exc:
        log.close()
        raise RuntimeError('Port 2345 is already in use; close the existing mGBA debugger before profiling') from exc
    finally:
        probe.close()

    proc = subprocess.Popen([
        str(MGBA), '-g', '-C', 'mute=1', '-C', 'audioSync=0', '-C', 'videoSync=0',
        '-C', f'fpsTarget={fps_target}', '-C', f'savegamePath={save_dir}', str(rom),
    ], stdout=log, stderr=log)
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
        rsp.cmd('qSupported')
        rsp.cmd('?')
        update = sym['game_update'] & ~1
        if rsp.cmd(f'Z1,{update:x},2') != 'OK':
            raise RuntimeError('failed to stop at game_update')
        rsp.tick(2)
        title = capture(rsp, run_dir / 'title.png')

        # At tick zero, press A to leave the title screen; tick one releases it.
        events = [(e['tick'], e['held']) for e in trace['events']] if trace else [(0, 1), (1, 0)]
        if not trace and profile_name == 'skirmish' and bots != 2:
            events = [(0, 64 if bots == 3 else 128), (1, 0), (2, 1), (3, 0)]
        capacity = sizes['demo_schedule_events'] // 8
        if len(events) > capacity or any(t < 0 or t >= ticks or not 0 <= h <= 1023 for t, h in events):
            raise ValueError(f'Invalid controller queue or capacity exceeded ({len(events)}/{capacity})')
        queue = b''.join(struct.pack('<2I', tick, held) for tick, held in events)
        for offset in range(0, len(queue), 256):
            rsp.write(sym['demo_schedule_events'] + offset, queue[offset:offset+256])
        rsp.write(sym['demo_schedule_length'], struct.pack('<I', ticks))
        rsp.write(sym['demo_schedule_count'], struct.pack('<I', len(events)))
        if rsp.cmd(f'z1,{update:x},2') != 'OK':
            raise RuntimeError('failed to remove game_update breakpoint')
        checkpoint_addr = sym['demo_replay_checkpoint'] & ~1
        if rsp.cmd(f'Z1,{checkpoint_addr:x},2') != 'OK':
            raise RuntimeError('failed to set scheduler checkpoint breakpoint')
        rsp.tick()
        if read_u32(rsp, sym['demo_schedule_tick']) != 0:
            raise RuntimeError('input scheduler did not stop at tick zero')

        last_clock = hardware_clock(rsp)
        last_tick = 0
        previous_buckets = struct.unpack('<8I', rsp.read(sym['demo_tick_buckets'], 32))
        checkpoints = []
        while last_tick < ticks:
            rsp.tick()
            completed = read_u32(rsp, sym['demo_schedule_tick'])
            if completed != min(ticks, last_tick + checkpoint):
                raise RuntimeError(f'expected tick {last_tick + checkpoint}, scheduler stopped at {completed}')
            now = hardware_clock(rsp)
            interval_cycles = (now - last_clock) & 0xFFFFFFFF
            total_cycles = interval_cycles
            buckets_now = struct.unpack('<8I', rsp.read(sym['demo_tick_buckets'], 32))
            histogram = [current - previous for current, previous in zip(buckets_now, previous_buckets)]
            state = {key: read_u32(rsp, sym[key]) for key in ('screen', 'frame', 'minerals', 'gas', 'won', 'lost')}
            if state['screen'] != 1 and not (state['screen'] == 3 and (not trace or completed == ticks)):
                raise RuntimeError(f'{profile_name} left PLAY screen at tick {completed}: {state}')
            observation = {
                'tick': completed,
                'profile_cycles': {key: read_u32(rsp, sym[key]) for key in ('demo_update_cycles', 'gfx_draw_cycles', 'sc64_orders_cycles', 'sc64_sight_cycles', 'sc64_other_cycles', 'sc64_map_cycles', 'sc64_hud_cycles', 'sc64_background_cycles', 'sc64_building_cycles', 'sc64_unit_draw_cycles') if key in sym},
                'cache_counters': {key: read_u32(rsp, sym[key]) for key in ('sc64_terrain_requests', 'sc64_terrain_hits', 'sc64_terrain_partial', 'sc64_route_hits', 'sc64_route_builds') if key in sym},
                'interval_cycles': interval_cycles,
                'interval_effective_fps': round(CPU_HZ * (completed-last_tick) / max(1, interval_cycles), 2),
                'last_tick_cycles': read_u32(rsp, sym['demo_last_cycles']),
                'peak_tick_cycles': read_u32(rsp, sym['demo_peak_cycles']),
                'hardware_frames_per_tick_histogram': histogram,
                **state,
                **count_entities(rsp, sym, bank),
            }
            if 'skirmish_ai' in sym:
                observation['skirmish_ai'] = read_ai_bank(rsp, sym, sizes)
            if trace:
                expected_checkpoint = next((item for item in trace.get('checkpoints', []) if item['tick'] == completed), None)
                if expected_checkpoint:
                    observed = completion_state(observation, rsp, sym)
                    for key, expected in expected_checkpoint.items():
                        if observed.get(key) != expected:
                            raise AssertionError(f'ARM diverged at {completed}: {key}={observed.get(key)}, host={expected}')
            checkpoints.append(observation)
            print(f'ARM {profile_name} tick {completed}/{ticks}: fps={observation["interval_effective_fps"]:.2f}, units={observation["units"]["alive"]}, buildings={observation["buildings"]["alive"]}, update={observation["profile_cycles"]["demo_update_cycles"]}, orders={observation["profile_cycles"]["sc64_orders_cycles"]}, script={observation["profile_cycles"]["sc64_other_cycles"]}, render={observation["profile_cycles"]["gfx_draw_cycles"]}, bg={observation["profile_cycles"].get("sc64_background_cycles", 0)}, sprites={observation["profile_cycles"].get("sc64_unit_draw_cycles", 0)}', flush=True)
            if completed == checkpoint:
                observation['opening_capture'] = capture(rsp, run_dir / 'opening.png')
            if completed == ticks or state['screen'] == 3:
                observation['late_capture'] = capture(rsp, run_dir / 'late.png')
            last_tick, last_clock, previous_buckets = completed, now, buckets_now
            if state['screen'] == 3: break

        final_verified = False
        if trace:
            final = checkpoints[-1]
            observed = completion_state(final, rsp, sym)
            for key, expected in trace['expected'].items():
                if observed.get(key) != expected:
                    raise AssertionError(f'ARM replay diverged: {key}={observed.get(key)}, host={expected}')
            if not observed['raynor_alive'] or observed['raynor_owner'] != 0 or not final['won'] or final['lost'] or final['screen'] != 3:
                raise AssertionError('No living friendly Raynor or victory result')
            final_verified = True
        elapsed_cycles = sum(item['interval_cycles'] for item in checkpoints)
        total = CPU_HZ * completed / max(1, elapsed_cycles)
        save_records, save_snapshot = [], None
        if profile_name == 'campaign' and trace:
            raw = rsp.read(0x0E000000, 32768)
            for offset in (0, 16384):
                tag, size, checksum, sequence = struct.unpack_from('<4I', raw, offset)
                if tag != 0x53435431 or size != sizes['data'] or not sequence: continue
                value = 2166136261
                for byte in raw[offset+16:offset+16+size]: value = ((value ^ byte) * 16777619) & 0xFFFFFFFF
                if value == checksum: save_records.append(dict(tag=hex(tag), bytes=size, sequence=sequence))
            if not save_records: raise AssertionError('Campaign has no valid SRAM record')
            save_snapshot = str(run_dir / 'completed.sav')
            Path(save_snapshot).write_bytes(raw)
        result = {
            'profile': profile_name,
            'save_records': save_records, 'save_snapshot': save_snapshot,
            'controller_completion_verified': final_verified,
            'all_host_checkpoints_match': bool(trace),
            'save_directory': str(save_dir),
            'rom_sha256': hashlib.sha256(rom.read_bytes()).hexdigest(),
            'ticks': completed, 'requested_ticks': ticks, 'checkpoint_interval': checkpoint,
            'emulator_fps_target': fps_target,
            'controller_input': [{'tick': tick, 'held': held} for tick, held in events],
            'title_capture': title,
            'source_profile': bank,
            'mean_interval_fps': round(total, 2),
            'maximum_tick_cycles': max((item['peak_tick_cycles'] for item in checkpoints), default=0),
            'checkpoints': checkpoints,
            'captures': {'title': str(run_dir / 'title.png'),
                         'opening': str(run_dir / 'opening.png'),
                         'late': str(run_dir / 'late.png')},
        }
        target = run_dir / 'profile.json'
        target.write_text(json.dumps(result, indent=2) + '\n')
        print(json.dumps({'profile': profile_name, 'mean_interval_fps': result['mean_interval_fps'],
                          'maximum_tick_cycles': result['maximum_tick_cycles'],
                          'source_profile': bank, 'result': str(target)}, indent=2), flush=True)
        return result
    finally:
        if connection:
            connection.close()
        proc.terminate()
        try:
            proc.wait(timeout=3)
        except subprocess.TimeoutExpired:
            proc.kill()
            proc.wait()
        log.close()


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('profile', choices=('backwater', 'skirmish', 'campaign'))
    parser.add_argument('--ticks', type=int, default=6000)
    parser.add_argument('--checkpoint', type=int, default=1000)
    parser.add_argument('--fps-target', type=int, default=600,
                        help='mGBA desktop playback target; rates are measured from the GBA hardware timer')
    parser.add_argument('--trace', type=Path)
    parser.add_argument('--bots', type=int, choices=(1, 2, 3), default=2)
    args = parser.parse_args()
    run(args.profile, args.ticks, args.checkpoint, args.fps_target, args.trace, args.bots)


if __name__ == '__main__':
    main()
