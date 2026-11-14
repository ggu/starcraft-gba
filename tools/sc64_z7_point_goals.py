#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Read point-fill inputs on the exact KODAK Z10 dense campaign interval."""
import argparse
from bisect import bisect_left
from collections import Counter
import json
from pathlib import Path
import resource
import shutil
import struct
import subprocess

from emulator_smoke import symbols, symbol_sizes
from sc64_headless_physical import start
from sc64_z7_normal_replay import build_plan, compare, input_hashes, make_layout, require_unchanged, snapshot


FIRST, LAST = 47758, 48237
PINS = ('e23e85d72041951e09c25e0b49111538c35ce174b4875aa4aed51143dfdfc907',
        '74c4b2394c4aee619bf2ea0549c31de7987827a915c32c5c50e7a4fec58ad6c1',
        'e1c7693d633f1ecd486036e797baea555dcaddf1e0a409d4da397f3b2f3fbe17')
FIELDS = {'path_walkable': 2048, 'sc64_route_valid': 14, 'sc64_route_goal': 28,
          'sc64_route_lphys': 14, 'sc64_route_pf_flags': 56, 'sc64_route_active_slots': 4}


def point_seed(grid, goal):
    if not 0 <= goal < 128 * 128 or (grid[goal >> 3] >> (goal & 7)) & 1:
        return ()
    y, x = divmod(goal, 128)
    cells = (128 * ny + nx for ny in range(max(0, y - 1), min(128, y + 2))
             for nx in range(max(0, x - 1), min(128, x + 2)))
    return tuple(c for c in cells if (grid[c >> 3] >> (c & 7)) & 1)


def run(args):
    paths = (args.rom, args.elf, args.trace, args.runner)
    hashes = input_hashes(paths)
    if tuple(hashes[str(p)] for p in paths) != (*PINS, args.runner_sha256):
        raise ValueError('ROM, ELF, tape or observer does not match the requested SHA-256')
    syms, widths = symbols(args.elf), symbol_sizes(args.elf)
    point = syms['sc64_route_fill.isra.0'] & ~1
    topology = syms['sc64_routes_topology_changed'] & ~1
    if any(widths.get(k) != v for k, v in FIELDS.items()):
        raise ValueError('unexpected stock route layout')
    layout = make_layout(args.elf, widths)
    plan, _ = build_plan(args.trace, LAST)
    checks = {i: row for i, row in plan['observations'].items() if i <= LAST}
    if len(checks) != 131 or not {FIRST - 1, LAST} <= checks.keys():
        raise ValueError('unexpected stock replay checkpoints')
    if args.plan:
        return dict(checks=len(checks), first=FIRST, last=LAST, fill=hex(point),
                    topology=hex(topology), hashes=hashes)
    if args.output is None or shutil.disk_usage(args.output.parent).free < int(5.25 * 2**30):
        raise RuntimeError('a fresh output path and at least 5.25 GiB free are required')
    args.output.mkdir()
    resource.setrlimit(resource.RLIMIT_FSIZE, (1 << 20, 1 << 20))
    proc, replay, log = start(args.runner, args.rom, args.output, syms, widths)
    events = [(t - 1, h) for t, h in plan['transitions'] if t <= LAST]
    ticks = [t for t, _ in events]
    held = dict(events)
    checked = 0
    epoch = 0
    rows = []
    recent = {}
    topologies = []

    def read(key):
        return replay.read(syms[key], widths[key])

    def number(key):
        return int.from_bytes(read(key), 'little')

    def stop_at():
        response = replay.cmd('c').split()
        if len(response) != 2 or response[0] != 'STOP':
            raise RuntimeError('observer did not stop at a breakpoint')
        return int(response[1], 16)

    def inspect(call):
        fields = replay.cmd('r').split()
        if len(fields) != 4 or fields[0] != 'REG' or any(len(f) != 8 for f in fields[1:]):
            raise RuntimeError('register observer did not return the fixed r0/r1/lr tuple')
        slot, goal, lr = (int(s, 16) for s in fields[1:])
        slots = number('sc64_route_active_slots')
        code = 0x08000000 <= lr < 0x0a000000 or 0x03000000 <= lr < 0x03005000
        if not 0 <= slot < slots <= 14 or not 0 <= goal < 16384 or not code:
            raise RuntimeError('point fill arguments are outside the pinned layout')
        grid = read('path_walkable')
        seeds = point_seed(grid, goal)
        valid, phys, flags = (read(k) for k in ('sc64_route_valid', 'sc64_route_lphys', 'sc64_route_pf_flags'))
        if any(i >= len(flags) for i in phys[:slots]):
            raise RuntimeError('logical route slot points outside physical flags')
        goals = struct.unpack('<14H', read('sc64_route_goal'))
        aliases = [i for i in range(slots) if i != slot and valid[i] == 1 and not flags[phys[i]] & 1
                   and goals[i] != goal and seeds and point_seed(grid, goals[i]) == seeds]
        previous = recent.get(goal)
        distance = len(rows) - previous if previous is not None else None
        recent[goal] = len(rows)
        rows.append(dict(call=call, slot=slot, goal=goal, caller=hex(lr), epoch=epoch,
                         aliases=aliases, fill_distance=distance))

    try:
        if number('screen') != 0 or number('frame') != 0:
            raise RuntimeError('observer did not begin at HOME')
        for boundary in sorted({i for i in checks if i < FIRST} | set(range(500, FIRST, 500))):
            if shutil.disk_usage(args.output).free < int(4.15 * 2**30):
                raise RuntimeError('replay stopped at the 4.15 GiB disk floor')
            replay.play_to(boundary, events[bisect_left(ticks, replay.completed):])
            if boundary in checks:
                compare(checks[boundary], snapshot(replay, syms, widths, layout), boundary)
                checked += 1
        if replay.completed != FIRST - 1 or not replay.cmd('r').startswith('REG '):
            raise RuntimeError('wrong boundary or observer lacks read-only register support')
        for address in (point, topology):
            replay.breakpoint(address, True)
        for call in range(FIRST, LAST + 1):
            if call % 16 == 0 and shutil.disk_usage(args.output).free < int(4.15 * 2**30):
                raise RuntimeError('replay stopped at the 4.15 GiB disk floor')
            if call - 1 in held:
                replay.set_keys(held[call - 1])
            for _ in range(128):
                address = stop_at()
                if address == replay.keys_pc:
                    if number('demo_frame') == call:
                        break
                    if number('demo_frame') != call - 1:
                        raise RuntimeError('input update counter skipped')
                elif address == point:
                    inspect(call)
                elif address == topology:
                    epoch += 1
                    topologies.append(call)
                    recent.clear()
                else:
                    raise RuntimeError('unexpected breakpoint')
            else:
                raise RuntimeError('too many breakpoint hits in one update')
            replay.completed = call
            if call in checks:
                compare(checks[call], snapshot(replay, syms, widths, layout), call)
                checked += 1
        if checked != len(checks) or not rows:
            raise RuntimeError('incomplete physical point-fill capture')
        require_unchanged(paths, hashes)
        result = dict(checks=checked, topologies=topologies, fill_count=len(rows),
                      equal_seed_candidates=sum(bool(r['aliases']) for r in rows),
                      repeat_distance=dict(Counter(r['fill_distance'] for r in rows
                                                   if r['fill_distance'] is not None)),
                      rows=rows, hashes=hashes)
        (args.output / 'result.json').write_text(json.dumps(result, indent=2) + '\n')
        return {k: result[k] for k in ('checks', 'fill_count', 'equal_seed_candidates')}
    finally:
        if proc.poll() is None:
            proc.terminate()
            try:
                proc.wait(timeout=3)
            except subprocess.TimeoutExpired:
                proc.kill()
                proc.wait()
        log.close()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for key in ('rom', 'elf', 'trace', 'runner'):
        parser.add_argument('--' + key, type=lambda p: Path(p).resolve(), required=True)
    parser.add_argument('--runner-sha256', required=True)
    parser.add_argument('--output', type=lambda p: Path(p).resolve())
    parser.add_argument('--plan', action='store_true')
    try:
        print(json.dumps(run(parser.parse_args()), indent=2))
    except (OSError, RuntimeError, ValueError) as exc:
        parser.exit(1, f'{exc}\n')


if __name__ == '__main__':
    main()
