#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Replay P9 and P10 physical-input tapes from a completed P8 SRAM journal.

Requires the ordinary headless runner, matching ROM/ELF and JSONL controller
traces. Checkpoints compare the explicitly enumerated observable fields only;
result reload compares the complete state returned by PhysicalGame.state().
"""
import argparse
import hashlib
import json
from pathlib import Path

from sc64_p8_arm_replay import load_trace
from sc64_races_profile import dwarf_struct_layout
from sc64_protoss_observe import observe_p9, observe_p10
from sc64_skirmish_sram_continuation import KEY_A, KEY_L, PhysicalGame


def require(condition, message):
    if not condition:
        raise ValueError(message)


def load_tape(path, mission, cut):
    rows = [json.loads(line) for line in path.read_text().splitlines() if line.strip()]
    plan = load_trace(path, mission=mission)
    inputs = plan['transitions']
    require((cut + 4, KEY_A) in inputs and (cut + 5, 0) in inputs,
            f'{path}: launch transition missing')
    checks = [r for r in rows if r.get('kind') == 'checkpoint']
    require(checks and all(a['call_index'] <= b['call_index'] for a, b in zip(checks, checks[1:])),
            f'{path}: missing or unordered checkpoints')
    require(all(r['call_index'] > cut + 5 for r in checks), f'{path}: checkpoint before launch')
    require(checks[-1] == plan['final'] and checks[-1].get('won') == 1,
            f'{path}: tape must finish at a victorious final checkpoint')
    events = [(0, 0), (2, KEY_L), (3, 0), (4, KEY_A)] + [(k-cut, v) for k, v in inputs if k >= cut+5]
    return events, checks


def checkpoint(g, mission, layout):
    runtime, runner_size, runner = layout
    r, sym, sizes = g.rsp, g.sym, g.sizes
    def n(k, layout=None, field=None):
        off = 0 if layout is None else layout[field]
        size = sizes[k] if layout is None else layout.widths[field]
        require(size in (1, 2, 4) and off + size <= sizes[k], f'invalid field {k}.{field}')
        return int.from_bytes(r.read(sym[k] + off, size), 'little')
    if mission == 28:
        return observe_p9(g, n, runtime, runner, runner_size)
    return observe_p10(g, n)


def compare_checkpoint(row, got, call):
    missing = [k for k in got if k not in row]
    require(not missing, f'checkpoint {call}: missing fields {missing}')
    diff = {k: [row[k], v] for k, v in got.items() if row[k] != v}
    require(not diff, f'checkpoint {call} mismatch: {diff}')


def result_state(g, mission):
    return dict(world=g.state(), completed=g.u32('race_completed'),
                mission=g.u32('race_selected_mission'), won=g.u32('won'),
                lost=g.u32('lost'), faults=g.u32('mission_unsupported_count'),
                runtime=g.rsp.read(g.sym[f'race_p{mission-19}'], g.sizes[f'race_p{mission-19}']).hex())


def run_one(args, mission, seed, trace, cut):
    events, checks = load_tape(trace, mission, cut)
    _, runtime = dwarf_struct_layout(args.elf, f'Sc64P{mission-19}PersistentState')
    runner_size, runner = dwarf_struct_layout(args.elf, f'Sc64P{mission-19}Runner')
    layout = (runtime, runner_size, runner)
    root = args.out / f'p{mission-19}'
    root.mkdir()
    victory, readback = root/'victory.sav', root/'readback.sav'
    def open_game(name, source, target):
        directory = root/name
        directory.mkdir()
        (directory/'headless.log').symlink_to('/dev/null')
        return PhysicalGame(args.runner, args.rom, args.elf, directory,
                            save_input=source, save_output=target)
    g = open_game('game', seed, victory)
    observed = []
    try:
        g.rsp.play_to(3, events)
        previous = mission - 1
        prior = g.u32('race_completed')
        # P1 and P2 occupy map slots 2 and 3, P3 onwards starts at slot 22.
        required = (1 << 2) | (1 << 3) | sum(1 << i for i in range(22, previous + 1))
        require(prior & required == required and (g.u32('race_selected_mission'),
                g.u32('won'), g.u32('lost'), g.u32('mission_unsupported_count')) ==
                (previous, 1, 0, 0),
                f'P{mission-19}: input save is not the expected prior victory')
        for row in checks:
            g.rsp.play_to(row['call_index']-cut, events)
            got = checkpoint(g, mission, layout)
            compare_checkpoint(row, got, row['call_index'])
            observed.append(dict(call_index=row['call_index'], **got))
            print(f'P{mission-19}: matched {row["call_index"]}', flush=True)
        s = result_state(g, mission)
        expected = prior | (1 << mission)
        require((s['mission'], s['won'], s['lost'], s['faults'], s['completed']) ==
                (mission, 1, 0, 0, expected), f'P{mission-19}: expected victory and completed mask {expected:#x}')
        g.rsp.export_sram()
    finally:
        g.close()
    g = open_game('fresh', victory, readback)
    try:
        g.step(KEY_L)
        require(result_state(g, mission) == s, f'P{mission-19}: result reload differs')
        g.rsp.export_sram()
        require(readback.read_bytes() == victory.read_bytes(), f'P{mission-19}: SRAM changed on reload')
    finally:
        g.close()
    print(f'P{mission-19}: result and SRAM matched on fresh load', flush=True)
    return readback, observed


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('p8-save', 'p9-trace', 'p10-trace', 'runner', 'rom', 'elf', 'out'):
        parser.add_argument('--'+name, type=Path, required=True)
    args = parser.parse_args()
    for p in (args.p8_save, args.p9_trace, args.p10_trace, args.runner, args.rom, args.elf):
        require(p.is_file(), f'input missing: {p}')
    inputs = (args.p8_save, args.p9_trace, args.p10_trace, args.runner, args.rom, args.elf)
    pins = {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in inputs}
    args.out.mkdir()
    seed, p9 = run_one(args, 28, args.p8_save, args.p9_trace, 50)
    finale, p10 = run_one(args, 29, seed, args.p10_trace, 48)
    require(pins == {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in inputs},
            'an input changed during replay')
    report = dict(p9=p9, p10=p10, inputs=pins,
                  victory_sram={s.parent.name: hashlib.sha256(s.read_bytes()).hexdigest()
                                for s in (seed, finale)})
    (args.out/'result.json').write_text(json.dumps(report, indent=2)+'\n')


if __name__ == '__main__':
    main()
