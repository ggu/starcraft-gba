#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Sample stock stage counters on a bounded, physical-input Z7 replay window."""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import shutil
import subprocess

from emulator_smoke import symbols, symbol_sizes
from sc64_headless_physical import start
from sc64_z7_normal_replay import (
    build_plan, compare, input_hashes, make_layout, require_unchanged, snapshot, timing_snapshot,
)


FIELDS = ('frame', 'screen', 'demo_last_cycles', 'demo_update_cycles',
          'sc64_orders_cycles', 'sc64_other_cycles', 'sc64_sight_cycles',
          'sc64_background_cycles', 'sc64_map_cycles', 'sc64_hud_cycles',
          'sc64_building_cycles', 'sc64_unit_draw_cycles',
          'sc64_route_builds', 'sc64_route_hits')
START_FREE_BYTES = int(5.25 * 1024**3)
STOP_FREE_BYTES = int(4.15 * 1024**3)


def run(args):
    inputs = (args.trace, args.rom, args.elf, args.runner)
    hashes = input_hashes(inputs)
    for path, expected in zip(inputs, (args.trace_sha256, args.rom_sha256,
                                        args.elf_sha256, args.runner_sha256)):
        if hashes[str(path)] != expected:
            raise ValueError(f'SHA-256 mismatch: {path}')
    if not 1 <= args.first <= args.last or args.last - args.first >= 2048:
        raise ValueError('window must contain 1 to 2048 updates')
    plan, pages = build_plan(args.trace, args.last)
    sym, sizes = symbols(args.elf), symbol_sizes(args.elf)
    for key in FIELDS:
        if sizes.get(key) != 4 or key not in sym:
            raise ValueError(f'missing 32-bit timing symbol {key}')
    layout = make_layout(args.elf, sizes)
    checks = {i: row for i, row in plan['observations'].items() if i <= args.last}
    if args.first - 1 not in checks or args.last not in checks:
        raise ValueError('window must start after and end at a recorded checkpoint')
    if args.plan:
        return {'first': args.first, 'last': args.last, 'checkpoints': len(checks),
                'fields': list(FIELDS), 'sha256': hashes}
    if args.output is None or shutil.disk_usage(args.output.parent).free < START_FREE_BYTES:
        raise RuntimeError('a fresh output path and at least 5.25 GiB free are required')
    args.output.mkdir()
    process, replay, log = start(args.runner, args.rom, args.output, sym, sizes)
    before = None
    samples = []
    checked = 0

    def value(key):
        return int.from_bytes(replay.read(sym[key], 4), 'little')

    try:
        if value('screen') != 0 or value('frame') != 0:
            raise RuntimeError('observer did not start at fresh HOME')
        previous_frame = None
        for page in pages:
            if shutil.disk_usage(args.output).free < STOP_FREE_BYTES:
                raise RuntimeError('replay stopped at the 4.15 GiB disk floor')
            if page['end'] < args.first:
                replay.play_page(page)
                if page['end'] == args.first - 1:
                    before = timing_snapshot(replay, sym, sizes)
                    previous_frame = value('frame')
            else:
                for call in range(max(args.first, page['first'] + 1), page['end'] + 1):
                    replay.play_to(call, page['events'])
                    row = {key: value(key) for key in FIELDS}
                    active = row['screen'] == 1 and row['frame'] == previous_frame + 1
                    previous_frame = row['frame']
                    samples.append(dict(call=call, simulation_advanced=active, **row))
            if replay.read(0x06013fe0, 2) != b'\0\0':
                raise RuntimeError('unexpected authored modal on the Z7 tape')
            if replay.completed in checks:
                compare(checks[replay.completed], snapshot(replay, sym, sizes, layout), replay.completed)
                checked += 1
        after = timing_snapshot(replay, sym, sizes)
        if (before is None or checked != len(checks) or len(samples) != args.last - args.first + 1
                or after['updates'] - before['updates'] != len(samples)
                or after['cycles'] - before['cycles'] != sum(row['demo_last_cycles'] for row in samples)):
            raise RuntimeError('stage window does not reconcile with the completed loop counters')
        require_unchanged(inputs, hashes)
        active_rows = [row for row in samples if row['simulation_advanced']]
        result = {'checkpoints': checked, 'before': before, 'after': after,
                  'mean_active_stage_cycles': {
                      key: sum(row[key] for row in active_rows) / len(active_rows)
                      for key in FIELDS if key.endswith('_cycles') and active_rows},
                  'samples': samples, 'sha256': hashes}
        (args.output / 'result.json').write_text(json.dumps(result, indent=2) + '\n')
        return {'checkpoints': checked, 'samples': len(samples), 'active': len(active_rows)}
    finally:
        if process.poll() is None:
            process.terminate()
            try:
                process.wait(timeout=3)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()
        log.close()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for key in ('trace', 'rom', 'elf', 'runner'):
        parser.add_argument('--' + key, type=lambda p: Path(p).resolve(), required=True)
        parser.add_argument('--' + key + '-sha256', required=True)
    parser.add_argument('--first', type=int, required=True)
    parser.add_argument('--last', type=int, required=True)
    parser.add_argument('--output', type=lambda p: Path(p).resolve())
    parser.add_argument('--plan', action='store_true')
    args = parser.parse_args()
    try:
        print(json.dumps(run(args), indent=2))
    except (OSError, ValueError, RuntimeError) as exc:
        parser.exit(1, f'{exc}\n')


if __name__ == '__main__':
    main()
