#!/usr/bin/env python3
"""Replay controller-only Wasteland completion on the actual GBA ARM ROM.

The host driver emits input changes and read-only gameplay checkpoints. This
runner writes only the input-injection register, never game state or save data.
Each checkpoint must match the host simulation before victory is accepted.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import socket
import struct
import subprocess
import shutil
import time
from emulator_smoke import ROOT, RSP, symbols, capture, require_mgba


def hardware_clock(r: RSP) -> int:
    # The CPU is halted at each checkpoint, so the cascaded timer is stable.
    data = r.read(0x04000108, 8)
    return int.from_bytes(data[:2], 'little') | (int.from_bytes(data[4:6], 'little') << 16)


def game_state(r: RSP, sym: dict[str, int]) -> dict[str, int]:
    result = {k: int.from_bytes(r.read(sym[k], 4), 'little')
              for k in ('frame', 'minerals', 'won', 'lost', 'screen')}
    units = r.read(sym['u'], 48 * 88)
    result.update(marines=0, scv=0, zerg=0, raynor_alive=0, raynor_owner=-1)
    for i in range(48):
        row = struct.unpack_from('<22i', units, i * 88)
        owner, kind, alive = row[4:7]
        if kind == 6:
            result['raynor_alive'], result['raynor_owner'] = alive, owner
        if alive:
            if owner == 0 and kind == 1: result['marines'] += 1
            if owner == 0 and kind == 0: result['scv'] += 1
            if owner == 1 and kind == 4: result['zerg'] += 1
    buildings = r.read(sym['b'], 16 * 72)
    result['barracks'] = sum(struct.unpack_from('<18i', buildings, i * 72)[4:7] == (0, 1, 1)
                             and struct.unpack_from('<18i', buildings, i * 72)[16] == 0
                             for i in range(16))
    return result


def main() -> None:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--trace', type=Path, default=ROOT / 'build/sc64-completion-input.json')
    args = p.parse_args()
    mgba = require_mgba()
    trace_bytes = args.trace.read_bytes()
    trace = json.loads(trace_bytes)
    ticks = trace['total_ticks']
    if not isinstance(ticks, int) or not 1 <= ticks <= 200000:
        raise ValueError('Invalid replay length')
    events = {e['tick']: e['held'] for e in trace['events']}
    if len(events) != len(trace['events']) or not 1 <= len(events) <= 128 or any(not 0 <= k < ticks or not 0 <= v <= 1023 for k, v in events.items()):
        raise ValueError('Invalid input events')
    checkpoints = {c['tick']: c for c in trace['checkpoints']}
    rom = ROOT / 'build/StarCraft64-Wasteland-GBA.gba'
    rom_sha256 = hashlib.sha256(rom.read_bytes()).hexdigest()
    sym = symbols(ROOT / 'build/starcraft64.elf')
    out = ROOT / 'build/emulator'
    out.mkdir(exist_ok=True)
    replay_rom = out / 'SC64 Completion Test.gba'
    shutil.copyfile(rom, replay_rom)
    save_dir = out / 'saves/sc64-completion'
    save_dir.mkdir(parents=True, exist_ok=True)
    check = socket.socket()
    try:
        check.bind(('127.0.0.1', 2345))
    finally:
        check.close()
    log = (out / 'sc64-completion.log').open('ab')
    # Accelerate host playback only. Performance below is derived from the
    # emulated 16.78 MHz hardware timer, independently of desktop playback rate.
    proc = subprocess.Popen([mgba, '-g', '-C', 'mute=1', '-C', 'audioSync=0', '-C', 'videoSync=0', '-C', 'fpsTarget=600',
                             '-C', f'savegamePath={save_dir}', str(replay_rom)], stdout=log, stderr=log)
    connection = None
    try:
        deadline = time.monotonic() + 12
        while time.monotonic() < deadline:
            try:
                connection = socket.create_connection(('127.0.0.1', 2345), timeout=1)
                break
            except OSError:
                time.sleep(.1)
        if connection is None: raise RuntimeError('mGBA debugger did not start')
        connection.settimeout(30)
        connection.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        r = RSP(connection)
        r.cmd('qSupported'); r.cmd('?')
        assert r.cmd(f'Z1,{sym["game_update"] & ~1:x},2') == 'OK'
        r.tick(2)
        # Write only the controller-event queue and its length/count. The same
        # main loop reads these held inputs and derives pressed edges normally.
        # The game data/state is never written by this runner.
        queue = b''.join(struct.pack('<2I', tick, held) for tick, held in sorted(events.items()))
        for offset in range(0, len(queue), 256):
            r.write(sym['demo_schedule_events'] + offset, queue[offset:offset+256])
        r.write(sym['demo_schedule_length'], struct.pack('<I', ticks))
        r.write(sym['demo_schedule_count'], struct.pack('<I', len(events)))
        assert r.cmd(f'z1,{sym["game_update"] & ~1:x},2') == 'OK'
        assert r.cmd(f'Z1,{sym["demo_replay_checkpoint"] & ~1:x},2') == 'OK'
        r.tick()
        assert int.from_bytes(r.read(sym['demo_schedule_tick'], 4), 'little') == 0
        print(f'ARM replay scheduled: {ticks} controller ticks, {len(events)} events (mGBA PID {proc.pid})', flush=True)
        last_clock = hardware_clock(r)
        start_buckets = struct.unpack('<8I', r.read(sym['demo_tick_buckets'], 32))
        total_cycles, last_tick = 0, 0
        observations = []
        completed = 0
        while completed < ticks:
            r.tick()
            completed = int.from_bytes(r.read(sym['demo_schedule_tick'], 4), 'little')
            if completed in checkpoints:
                state = game_state(r, sym)
                for key, expected in checkpoints[completed].items():
                    if key != 'tick' and state[key] != expected:
                        raise AssertionError(f'ARM replay diverged at input tick {completed}: {key}={state[key]}, host={expected}')
                clock = hardware_clock(r)
                cycles = (clock - last_clock) & 0xFFFFFFFF
                total_cycles += cycles
                fps = 16777216 * (completed - last_tick) / cycles
                profile = {key: int.from_bytes(r.read(sym[key], 4), 'little')
                           for key in ('demo_update_cycles','gfx_draw_cycles','sc64_orders_cycles','sc64_sight_cycles','sc64_map_cycles','sc64_hud_cycles')}
                observations.append(dict(tick=completed, effective_fps=round(fps, 2), profile_cycles=profile, **state))
                print(f'ARM tick {completed}/{ticks}: minerals={state["minerals"]} Marines={state["marines"]} SCVs={state["scv"]} rate={fps:.2f}', flush=True)
                last_clock, last_tick = clock, completed
                if completed in (2000, 6000, 20000):
                    capture(r, out / f'sc64-completion-{completed}.png')
        final = game_state(r, sym)
        for key, expected in trace['expected'].items():
            assert final[key] == expected, f'Final {key}: ARM={final[key]}, host={expected}'
        assert final['won'] == 1 and final['lost'] == 0 and final['screen'] == 3
        assert final['raynor_alive'] and final['raynor_owner'] == 0 and final['barracks'] >= 1
        total_cycles += (hardware_clock(r) - last_clock) & 0xFFFFFFFF
        buckets = [a-b for a,b in zip(struct.unpack('<8I', r.read(sym['demo_tick_buckets'], 32)), start_buckets)]
        records = []
        for offset in (0, 16384):
            tag, size, checksum, sequence = struct.unpack('<4I', r.read(0x0E000000 + offset, 16))
            if tag == 0x53433631 and size == 10088 and sequence:
                value = 2166136261
                for byte in r.read(0x0E000000 + offset + 16, size): value = ((value ^ byte) * 16777619) & 0xFFFFFFFF
                if value == checksum: records.append(dict(tag=hex(tag), bytes=size, sequence=sequence))
        assert records, 'Mid-mission save has no valid SRAM record'
        result = dict(rom_sha256=rom_sha256,
                      trace_sha256=hashlib.sha256(trace_bytes).hexdigest(), input_ticks=ticks,
                      all_checkpoints_match=True, final=final, checkpoints=observations,
                      mean_tick_cycles=round(total_cycles/ticks), effective_fps=round(16777216*ticks/total_cycles, 2),
                      peak_tick_cycles=int.from_bytes(r.read(sym['demo_peak_cycles'], 4), 'little'),
                      save_records=records,
                      hardware_frames_per_tick_histogram=buckets,
                      victory_capture=capture(r, out / 'sc64-completion-victory.png'))
        (out / 'sc64-completion.json').write_text(json.dumps(result, indent=2) + '\n')
        print(json.dumps({k:v for k,v in result.items() if k != 'checkpoints'}, indent=2), flush=True)
    finally:
        if connection: connection.close()
        proc.terminate()
        try: proc.wait(timeout=3)
        except subprocess.TimeoutExpired: proc.kill(); proc.wait()
        log.close()


if __name__ == '__main__':
    main()
