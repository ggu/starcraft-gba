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
from types import SimpleNamespace

import sc64_races_profile as races
import sc64_skirmish_sram_continuation as continuation

import sc64_headless_replay as headless
import sc64_p8_arm_replay as p8
import sc64_z3_controller_replay as scheduler
from emulator_smoke import symbols, symbol_sizes


def u32(rsp, address):
    return struct.unpack('<I', rsp.read(address, 4))[0]


def persisted_summary(rsp, sym, sizes, elf):
    """Sample the shared continuation helper's bounded semantic world fields."""
    unit_stride, unit_layout = races.dwarf_struct_layout(elf, 'Unit')
    building_name = 'Sc64ZpCompactBuilding' if sizes['b'] == 160 * 48 else 'Building'
    building_stride, building_layout = races.dwarf_struct_layout(elf, building_name)
    proxy = SimpleNamespace(rsp=rsp, sym=sym, sizes=sizes, unit_stride=unit_stride,
                            unit_layout=unit_layout, building_stride=building_stride,
                            building_layout=building_layout,
                            u32=lambda name: int.from_bytes(rsp.read(sym[name], 4), 'little', signed=True))
    return continuation.summary(continuation.Game.state(proxy))


def resume_inputs(transitions, resume):
    """Quick-load with L/release, then continue the original held-key tape."""
    previous = 0
    for tick, held in transitions:
        if tick > resume:
            break
        previous = held
    inputs = [(0, 512), (1, 0)]
    if previous:
        inputs.append((2, previous))
    inputs += [(2 + tick - resume, held) for tick, held in transitions if tick > resume]
    return inputs


def verify_restored(before, restored):
    """Require an intact saved summary and matching bounded restored sample."""
    for sample in (before, restored):
        if not isinstance(sample, dict) or type(sample.get('frame')) is not int:
            raise ValueError('invalid persisted-state summary')
        fingerprint = sample.get('fingerprint')
        if not isinstance(fingerprint, str) or len(fingerprint) != 64 or any(c not in '0123456789abcdef' for c in fingerprint):
            raise ValueError('invalid persisted-state fingerprint')
    if restored['fingerprint'] != before['fingerprint'] or restored['frame'] != before['frame']:
        raise RuntimeError(f'restored world differs from saved sample: {restored} vs {before}')
    return True


def validate_save_boundary(plan, stop):
    if not plan.get('save') or stop != plan['save'].get('resume_call'):
        raise ValueError('SRAM export requires the tape pause-menu save boundary')


def run(trace, reference, rom, elf, runner, output, stop=None, export_save=False, save_input=None, saved_state=None):
    plan = p8.load_trace(trace)
    if save_input and (not saved_state or export_save or stop is not None or not plan['save']):
        raise ValueError('resume requires a save and saved-state summary, without prefix/export options')
    save_hash = hashlib.sha256(save_input.read_bytes()).hexdigest() if save_input else None
    if export_save:
        validate_save_boundary(plan, plan['stop'] if stop is None else stop)
    original_stop = plan['stop']
    resume = plan['save']['resume_call'] if save_input else 0
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
    transitions = plan['transitions']
    if save_input:
        inputs = resume_inputs(transitions, resume)
        stop = 2 + original_stop - resume
    else:
        inputs = transitions
    pages = scheduler.make_schedule_pages({'inputs': inputs}, stop, 1000)
    output.mkdir(parents=True, exist_ok=False)
    proc = rsp = log = None
    records = []
    try:
        proc, rsp, log = headless.start(runner, rom, output, sym, sizes,
                                        save_input=save_input,
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
        if save_input:
            rsp.breakpoint(sym['demo_replay_checkpoint'] & ~1, False)
            rsp.breakpoint(sym['game_update'] & ~1, True)
            for _ in range(16):
                rsp.tick(expected=sym['game_update'] & ~1)
                if u32(rsp, sym['demo_schedule_tick']) == 2: break
            else:
                raise RuntimeError('fresh-process load did not reach two updates')
            restored = persisted_summary(rsp, sym, sizes, elf)
            (output / 'restored-state.json').write_text(json.dumps(restored, indent=2) + '\n')
            before = json.loads(saved_state.read_text())
            verify_restored(before, restored)
            if u32(rsp, sym['screen']) != 1 or u32(rsp, sym['race_selected_mission']) != 27:
                raise RuntimeError('fresh process did not load P8 into play')
            rsp.breakpoint(sym['game_update'] & ~1, False)
            rsp.breakpoint(sym['demo_replay_checkpoint'] & ~1, True)
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
            reference_tick = original_stop if save_input and tick == stop else tick if not save_input else -1
            if reference_tick in checks:
                mismatches = {k: [int(checks[reference_tick][k]), state[k]] for k in ('screen', 'frame', 'won', 'lost', 'mission') if int(checks[reference_tick][k]) != state[k]}
                if mismatches:
                    raise RuntimeError(f'host checkpoint {tick} differs: {mismatches}')
            print('P8 normal', tick, state, flush=True)
        full = bool(save_input) or stop == plan['stop']
        if full and (records[-1]['won'] != 1 or records[-1]['lost'] != 0):
            raise RuntimeError('normal ROM did not win')
        if export_save:
            (output / 'saved-state.json').write_text(json.dumps(persisted_summary(rsp, sym, sizes, elf), indent=2) + '\n')
            rsp.export_sram()
        if save_input and hashlib.sha256(save_input.read_bytes()).hexdigest() != save_hash:
            raise RuntimeError('fresh-process SRAM input changed')
        result = {'status': 'won' if full else 'prefix', 'stop': stop, 'checkpoints': records,
                  'sha256': {str(p.resolve()): hashlib.sha256(p.read_bytes()).hexdigest() for p in (trace, reference, rom, elf, runner)}}
        if save_input:
            result['resume'] = {'save_input': str(save_input.resolve()), 'save_sha256': save_hash,
                                'saved_state': str(saved_state.resolve()),
                                'saved_state_sha256': hashlib.sha256(saved_state.read_bytes()).hexdigest(),
                                'saved': before, 'restored': restored,
                                'restored_matches': True, 'sram_unchanged': True,
                                'scope': 'bounded entity and economy fingerprint; final five-field match only'}
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
    ap.add_argument('--save-input', type=Path)
    ap.add_argument('--saved-state', type=Path)
    a = ap.parse_args()
    run(a.trace, a.reference, a.rom, a.elf, a.runner, a.output, a.stop, a.export_save, a.save_input, a.saved_state)


if __name__ == '__main__':
    main()
