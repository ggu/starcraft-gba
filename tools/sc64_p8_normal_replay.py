#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Drive the unmodified P8 ROM scheduler using a validated ordinary-input tape.

The backend's allowlist permits writes only to scheduler controls. Checkpoints
compare the observed screen, frame, result and mission to the retained host run.
A bounded prefix is diagnostic; only the complete tape may claim a win.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

import sc64_headless_replay as headless
import sc64_p8_arm_replay as p8
import sc64_z3_controller_replay as scheduler
from emulator_smoke import symbols, symbol_sizes


def u32(rsp, address):
    return struct.unpack('<I', rsp.read(address, 4))[0]


def run(trace, reference, rom, elf, runner, output, stop=None, export_save=False):
    plan = p8.load_trace(trace)
    stop = plan['stop'] if stop is None else stop
    if not 0 < stop <= plan['stop']:
        raise ValueError('prefix must be within the validated tape')
    rows = p8.checks(p8.p8_lines(reference.read_text()))
    if p8.cadence_errors(rows, 0, 'host reference'):
        raise ValueError('host reference checkpoint cadence is incomplete')
    if not rows or rows[-1][0] != plan['stop'] or rows[-1][1]['kind'] != 'won':
        raise ValueError('host reference must end in the validated tape win')
    checks = dict(rows)
    expected = list(range(1000, stop + 1, 1000))
    if stop == plan['stop'] and stop % 1000:
        expected.append(stop)
    if any(tick not in checks for tick in expected):
        raise ValueError('host reference lacks required checkpoint')
    sym, sizes = symbols(elf), symbol_sizes(elf)
    mode, capacity = scheduler.schedule_mode(sym, sizes)
    if mode != 'windowed' or scheduler.checkpoint_ticks(sym) != 1000:
        raise ValueError('normal P8 replay needs the 1000-tick window scheduler')
    pages = scheduler.make_schedule_pages({'inputs': plan['transitions']}, stop, 1000)
    output.mkdir(parents=True, exist_ok=False)
    proc = rsp = log = None
    records = []
    try:
        proc, rsp, log = headless.start(runner, rom, output, sym, sizes,
                                        save_output=output / "saved.sav" if export_save else None)
        if u32(rsp, sym['screen']) != 0 or u32(rsp, sym['frame']) != 0:
            raise RuntimeError('fresh boot is not at title')
        def write_page(events):
            blob = b''.join(struct.pack('<2I', tick, held) for tick, held in events)
            rsp.write(sym['demo_schedule_events'], blob)
            rsp.write(sym['demo_schedule_count'], struct.pack('<I', len(events)))
        write_page(pages[0]['events'][:capacity])
        rsp.write(sym['demo_schedule_index'], struct.pack('<I', 0))
        rsp.write(sym['demo_schedule_length'], struct.pack('<I', stop))
        rsp.write(sym['demo_schedule_active'], struct.pack('<I', 1))
        rsp.breakpoint(sym['game_update'] & ~1, False)
        rsp.breakpoint(sym['demo_replay_checkpoint'] & ~1, True)
        rsp.tick(expected=sym['demo_replay_checkpoint'] & ~1)
        if u32(rsp, sym['demo_schedule_tick']) != 0:
            raise RuntimeError('scheduler did not begin at zero')
        for i, page in enumerate(pages):
            events = page['events']
            chunks = [events[n:n + capacity] for n in range(0, len(events), capacity)] or [[]]
            active_chunk = 0
            if len(chunks) > 1:
                # The ordinary ROM has 1000-tick checkpoints but only 256
                # entries. Stop at real game_update calls, and refill only
                # after the current chunk has been consumed. The current held
                # key remains in the ROM scheduler's own local state.
                rsp.breakpoint(sym['demo_replay_checkpoint'] & ~1, False)
                rsp.breakpoint(sym['game_update'] & ~1, True)
                last_tick = page['first'] - 1
                while last_tick < page['end'] - 1:
                    rsp.tick(expected=sym['game_update'] & ~1)
                    tick = u32(rsp, sym['demo_schedule_tick'])
                    if tick < last_tick or tick >= page['end']:
                        raise RuntimeError('unexpected dense-window update tick')
                    last_tick = tick
                    index = u32(rsp, sym['demo_schedule_index'])
                    if index == len(chunks[active_chunk]) and active_chunk + 1 < len(chunks):
                        active_chunk += 1
                        if chunks[active_chunk][0][0] <= tick:
                            raise RuntimeError('dense-window refill is too late')
                        write_page(chunks[active_chunk])
                        rsp.write(sym['demo_schedule_index'], struct.pack('<I', 0))
                rsp.breakpoint(sym['game_update'] & ~1, False)
                rsp.breakpoint(sym['demo_replay_checkpoint'] & ~1, True)
            rsp.tick(expected=sym['demo_replay_checkpoint'] & ~1)
            tick = page['end']
            if u32(rsp, sym['demo_schedule_tick']) != tick:
                raise RuntimeError('unexpected scheduler tick')
            if active_chunk != len(chunks) - 1 or u32(rsp, sym['demo_schedule_index']) != len(chunks[-1]) or u32(rsp, sym['demo_schedule_count']) != len(chunks[-1]):
                raise RuntimeError('scheduler did not consume its entire input page')
            if i + 1 < len(pages):
                write_page(pages[i + 1]['events'][:capacity])
            state = {k: u32(rsp, sym[k]) for k in ('screen', 'frame', 'won', 'lost')}
            state['mission'] = u32(rsp, sym['race_selected_mission'])
            state['tick'] = tick
            records.append(state)
            (output / 'progress.json').write_text(json.dumps({'checkpoints': records}, indent=2) + '\n')
            if tick in checks:
                mismatches = {k: [int(checks[tick][k]), state[k]] for k in ('screen', 'frame', 'won', 'lost', 'mission') if int(checks[tick][k]) != state[k]}
                if mismatches:
                    raise RuntimeError(f'host checkpoint {tick} differs: {mismatches}')
            print('P8 normal', tick, state, flush=True)
        full = stop == plan['stop']
        if full and (records[-1]['won'] != 1 or records[-1]['lost'] != 0):
            raise RuntimeError('normal ROM did not win')
        if export_save:
            rsp.export_sram()
        result = {'status': 'won' if full else 'prefix', 'stop': stop, 'checkpoints': records,
                  'sha256': {str(p.resolve()): hashlib.sha256(p.read_bytes()).hexdigest() for p in (trace, reference, rom, elf, runner)}}
        (output / 'result.json').write_text(json.dumps(result, indent=2) + '\n')
        return result
    finally:
        if proc is not None:
            proc.terminate()
            try:
                proc.wait(timeout=3)
            except __import__('subprocess').TimeoutExpired:
                proc.kill(); proc.wait()
            if proc.stdin: proc.stdin.close()
            if proc.stdout: proc.stdout.close()
            if log: log.close()


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    for name in ('trace', 'reference', 'rom', 'elf', 'runner', 'output'):
        ap.add_argument('--' + name, type=Path, required=True)
    ap.add_argument('--stop', type=int)
    ap.add_argument('--export-save', action='store_true')
    a = ap.parse_args()
    run(a.trace, a.reference, a.rom, a.elf, a.runner, a.output, a.stop, a.export_save)


if __name__ == '__main__':
    main()
