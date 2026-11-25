#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Ordinary-button custom-roster openings with fresh SRAM continuation."""
import argparse
from concurrent.futures import ThreadPoolExecutor, as_completed
import hashlib
import itertools
import json
from pathlib import Path
import shutil
import traceback

import sc64_skirmish_sram_continuation as continuation


class QuietPhysicalGame(continuation.PhysicalGame):
    def __init__(self, runner, rom, elf, out, **kwargs):
        out.mkdir(parents=True, exist_ok=True)
        (out / 'headless.log').symlink_to('/dev/null')
        super().__init__(runner, rom, elf, out, **kwargs)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('runner', 'rom', 'elf', 'output'):
        parser.add_argument('--' + name, type=Path, required=True)
    parser.add_argument('--jobs', type=int, default=2, choices=(1, 2))
    args = parser.parse_args()
    paths = (args.runner, args.rom, args.elf)
    pins = {str(p.resolve()): digest(p) for p in paths}
    args.output.mkdir(parents=True, exist_ok=False)
    continuation.PhysicalGame = QuietPhysicalGame
    cases = [(m, human, ''.join(bots)) for m in range(2) for human in 'tzp'
             for count in (1, 2, 3) for bots in itertools.product('tzp', repeat=count)]

    def run(case):
        index, (map_id, human, bots) = case
        out = args.output / f'case-{index:03d}'
        row = {'index': index, 'map': map_id, 'human': human, 'bots': bots}
        try:
            options = argparse.Namespace(output=out, runner=args.runner, rom=args.rom, elf=args.elf,
                map=map_id, human=human, bots=bots, seats=bots + '-' * (3 - len(bots)),
                melee=False, play=120, continue_updates=60, physical=True, tamper_sram=False)
            result = continuation.run(options)
            row.update(ok=result['ok'], checks=result['checks'], seats=result.get('launch_seats'),
                       saved=result['saved'], restored=result['restored'], sram_sha256=result['sram_sha256'])
            if result['ok']:
                shutil.rmtree(out)
        except Exception as error:
            row.update(ok=False, error=repr(error), traceback=traceback.format_exc())
        (args.output / f'result-{index:03d}.json').write_text(json.dumps(row, indent=2) + '\n')
        print(json.dumps({'index': index, 'ok': row['ok']}), flush=True)
        return row

    with ThreadPoolExecutor(max_workers=args.jobs) as pool:
        rows = [future.result() for future in as_completed(
            [pool.submit(run, case) for case in enumerate(cases)])]
    unchanged = pins == {str(p.resolve()): digest(p) for p in paths}
    rows.sort(key=lambda row: row['index'])
    report = {'ok': unchanged and len(rows) == 234 and all(row['ok'] for row in rows),
              'scope': 'normal custom mode, 120 updates, pause save, fresh reload and 60-update continuation',
              'pins': pins, 'unchanged': unchanged, 'cases': rows}
    (args.output / 'matrix.json').write_text(json.dumps(report, indent=2) + '\n')
    return 0 if report['ok'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
