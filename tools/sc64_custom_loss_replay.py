#!/usr/bin/env python3
"""Replay a Custom Game loss using the normal physical input and checkpoint verifier."""

import argparse
import hashlib
import json
from pathlib import Path

import sc64_custom_full_match as match


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('trace', 'rom', 'elf', 'runner', 'output'):
        parser.add_argument('--' + name, type=Path, required=True)
    args = parser.parse_args()
    data = match.load_trace(args.trace)
    if data['terminal']['kind'] != 'lost':
        raise match.MatchError('trace must end in a loss')
    args.output.mkdir(parents=True, exist_ok=False)
    paths = (args.trace, args.rom, args.elf, args.runner)
    pins = {str(path): sha256(path) for path in paths}
    notes = {}
    full = args.output / 'full'
    full.mkdir()
    (full / 'headless.log').symlink_to('/dev/null')
    game = match.PhysicalGame(args.runner, args.rom, args.elf, full)
    try:
        match.require_home(game)

        def start():
            notes['lobby'] = match.lobby(game, data)

        rows = match.play_checks(game, data, data['inputs'], 0, 0,
                                 {data['start']['host_tick']: start}, 'lost-match')
        final = match.observe(game)
        if (final['screen'], final['won'], final['lost']) != (match.SCREEN_RESULT, 0, 1):
            raise match.MatchError(f'expected loss RESULT, got {final}')
        if pins != {str(path): sha256(path) for path in paths}:
            raise match.MatchError('an input changed during replay')
        report = {'ok': True, 'result': 'lost', 'transport': 'physical-buttons',
                  'checks': len(rows), 'rows': rows, 'final': final,
                  'lobby': notes['lobby'], 'sha256': pins}
        (args.output / 'result.json').write_text(json.dumps(report, indent=2) + '\n')
    finally:
        game.close()


if __name__ == '__main__':
    main()
