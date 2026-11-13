#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Verify a completed HOST skirmish trace from fresh HOME on a normal ARM ROM.

Drive physical buttons in headless mGBA. Read, but never write, guest state.
Compare launch and terminal state, plus frame and post-action wallet values
where recorded. Timing is emulated main-loop timing, not hardware FPS.
"""
from __future__ import annotations

import argparse
from collections import Counter, defaultdict
import json
from pathlib import Path
import shutil

from sc64_skirmish_controller_replay import validate_trace, verify_configuration, verify_terminal
from sc64_skirmish_sram_continuation import PhysicalGame
from sc64_z7_normal_replay import input_hashes, require_unchanged, timing_delta, timing_snapshot

START_FREE_BYTES = int(5.25 * 1024**3)
STOP_FREE_BYTES = int(4.15 * 1024**3)


def snapshot(game):
    state = game.state()
    result = {key: state[key] for key in ('frame', 'screen', 'minerals', 'gas')}
    result.update({key: game.u32(key) for key in
                   ('won', 'lost', 'race_mode', 'race_human', 'race_bots', 'race_skirmish_map')})
    for key, symbol in [('bot_races', 'race_bot_races'), ('bot_owners', 'race_bot_owner_ids')]:
        if game.sizes.get(symbol) != 12:
            raise RuntimeError(f'{symbol} must contain three 32-bit slots')
        result[key] = [game.u32(symbol, 4 * i) for i in range(3)]
    for key in ('units', 'buildings'):
        result[key] = {'by_owner': dict(Counter(str(row[1]) for row in state[key]))}
    return result


def save_json(path, record):
    path.write_text(json.dumps(record, indent=2) + '\n')


def run(args):
    paths = (args.trace, args.rom, args.elf, args.runner)
    hashes = input_hashes(paths)
    if hashes[str(args.rom)] != args.rom_sha256:
        raise ValueError('ROM does not match the requested SHA-256')
    raw = json.loads(args.trace.read_text())
    plan = validate_trace(raw, 256, checkpoint_interval=250)
    end = plan['stop_tick']
    if end > 1_000_000:
        raise ValueError('trace exceeds the controller tick limit')
    checks = defaultdict(list)
    for row in raw['events']:
        if row['kind'] in ('start', 'action'):
            checks[row['host_tick']].append(row)
    if shutil.disk_usage(args.output.parent).free < START_FREE_BYTES:
        raise RuntimeError('free at least 5.25 GiB before the replay')
    args.output.mkdir()
    game = PhysicalGame(args.runner, args.rom, args.elf, args.output / 'guest')
    rsp = game.rsp
    observations = []
    try:
        if (game.u32('screen'), game.u32('frame'), game.front_field('page')) != (0, 0, 0):
            raise RuntimeError('fresh boot is not on HOME')
        before = timing_snapshot(rsp, game.sym, game.sizes)
        for tick in sorted(set(checks) | set(range(500, end, 500)) | {end}):
            if shutil.disk_usage(args.output).free < STOP_FREE_BYTES:
                save_json(args.output / 'partial.json', dict(tick=rsp.completed, observations=observations))
                raise RuntimeError('replay stopped by the free-space guard')
            rsp.play_to(tick, plan['inputs'])
            if tick not in checks and tick != end:
                continue
            actual = snapshot(game)
            try:
                for expected in checks[tick]:
                    for observed, recorded in [('frame', 'frame'), ('minerals', 'minerals_after'),
                                               ('gas', 'gas_after')]:
                        if recorded in expected and actual[observed] != expected[recorded]:
                            raise RuntimeError(f'{recorded}={actual[observed]} expected {expected[recorded]}')
                    if expected['kind'] == 'start':
                        if actual['screen'] != 1:
                            raise RuntimeError('launch did not reach the battlefield')
                        verify_configuration(plan['start'], actual)
                if tick == end:
                    verify_terminal(plan['terminal'], actual)
            except RuntimeError as error:
                save_json(args.output / 'divergence.json', dict(tick=tick, actual=actual,
                          expected=checks[tick], error=str(error)))
                raise RuntimeError(f'physical call {tick}: {error}') from error
            observations.append(dict(tick=tick, state=actual))
        require_unchanged(paths, hashes)
        result = dict(status=plan['terminal']['kind'], sha256=hashes,
                      input_transitions=plan['input_count'], action_checks=plan['action_count'],
                      observations=observations,
                      timing=timing_delta(before, timing_snapshot(rsp, game.sym, game.sizes), rsp.completed))
        save_json(args.output / 'result.json', result)
        print(f"{result['status']}: call {end}, {len(observations)} matched observations")
    finally:
        game.close()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for option in ('trace', 'rom', 'elf', 'runner', 'output'):
        parser.add_argument('--' + option, required=True, type=lambda path: Path(path).resolve())
    parser.add_argument('--rom-sha256', required=True)
    args = parser.parse_args()
    try:
        run(args)
    except (OSError, ValueError, RuntimeError) as error:
        parser.exit(1, f'{error}\n')


if __name__ == '__main__':
    main()
