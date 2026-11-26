#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Replay a modal-aware Z8 controller trace from HOME or the linked Z7 victory."""
import argparse
from bisect import bisect_left
import json
from pathlib import Path

from sc64_linked_stage_replay import digest, open_game
from sc64_protoss_linked_replay import require
from sc64_skirmish_sram_continuation import BUILDING_FIELDS, UNIT_FIELDS, KEY_A, KEY_L


def observe(game, world):
    units = [dict(zip(UNIT_FIELDS, row[1:])) for row in world['units']]
    buildings = [dict(zip(BUILDING_FIELDS, row[1:])) for row in world['buildings']]
    nexus = {b['raw_owner']: b['hp'] + b['shield'] for b in buildings if b['raw_type'] == 154}
    hero_hp = next((u['hp'] for u in units if u['owner'] == 0 and u['raw_type'] == 51), -1)
    return dict(frame=world['frame'], screen=world['screen'], won=game.u32('won'), lost=game.u32('lost'),
                side_units=sum(u['owner'] == 0 for u in units), enemy_units=sum(u['owner'] == 1 for u in units),
                enemy_buildings=sum(b['owner'] == 1 for b in buildings), nexus=[nexus.get(i, -1) for i in (1, 2)],
                kerrigan=hero_hp, bank=[world['minerals'], world['gas']], cursor=[game.u32('cx'), game.u32('cy')])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('rom', 'elf', 'runner', 'trace', 'out'):
        parser.add_argument('--' + name, type=Path, required=True)
    parser.add_argument('--previous-victory', type=Path, help='freshly re-exported cumulative Z7 result')
    args = parser.parse_args()
    rows = [json.loads(line) for line in args.trace.read_text().splitlines()]
    require(rows[0].get('mission') == 19, 'expected Z8 trace')
    events = [(r['call_index'], r['held']) for r in rows if r['kind'] == 'input']
    checks = [r for r in rows if r['kind'] == 'checkpoint']
    require(checks and checks[0]['label'] == 'launch' and checks[0]['frame'] == 1 and
            checks[-1]['label'] == 'final' and checks[-1]['won'] == 1, 'expected launch and victory')
    require(all(a[0] < b[0] for a, b in zip(events, events[1:])), 'inputs out of order')
    files = [args.rom, args.elf, args.runner, args.trace]
    if args.previous_victory:
        files.append(args.previous_victory)
    hashes = {str(p): digest(p) for p in files}
    args.out.mkdir()
    victory, readback = (args.out / name for name in ('victory.sav', 'readback.sav'))
    g = open_game(args, args.out / 'game', args.previous_victory, victory)
    records = []
    prior = 0
    try:
        cut = 0
        if args.previous_victory:
            for key in (0, 0, KEY_L, 0):
                g.step(key)
            prior = g.u32('race_completed')
            require([g.u32(k) for k in ('screen', 'won', 'lost', 'race_selected_mission')] == [3, 1, 0, 18]
                    and prior == 0x7c003, 'expected earned Z1–Z7 victory')
            g.step(KEY_A)
            g.step(0)
            require(g.u32('race_selected_mission') == 19 and g.u32('frame') == 1, 'Z8 handoff failed')
            first = checks[0]['call_index']
            cut = first - g.rsp.completed
            events = [(t - cut, key) for t, key in events if t >= first]
        times = [tick for tick, _ in events]
        for ref in checks:
            tick = ref['call_index']
            # Keep a single debugger request short even when macOS is busy.
            end = tick - cut
            while g.rsp.completed < end:
                first = g.rsp.completed
                stop = min(end, first + 32)
                g.rsp.play_to(stop, events[bisect_left(times, first):bisect_left(times, stop)])

            world = g.state()
            actual = observe(g, world)
            diff = {k: [ref[k], v] for k, v in actual.items() if ref[k] != v}
            require(not diff, f'Z8 call {tick}: {diff}')
            records.append(dict(tick=tick, **actual))
            print(f"Matched Z8 call {tick}, frame {actual['frame']}", flush=True)
        require(g.u32('mission_unsupported_count') == 0 and actual['screen'] == 3 and actual['lost'] == 0
                and actual['kerrigan'] > 0 and actual['nexus'] == [-1, -1]
                and g.u32('race_completed') == prior | (1 << 19), 'incomplete Z8 victory')
        g.rsp.export_sram()
    finally:
        g.close()
    g = open_game(args, args.out / 'fresh', victory, readback)
    try:
        g.step(KEY_L)
        require(g.state() == world and g.u32('won') and g.u32('race_selected_mission') == 19 and
                g.u32('race_completed') == prior | (1 << 19), 'fresh result differs')
        g.rsp.export_sram()
        require(readback.read_bytes() == victory.read_bytes(), 'fresh SRAM export differs')
    finally:
        g.close()
    require(all(hashes[str(p)] == digest(p) for p in files), 'input changed during replay')
    (args.out / 'result.json').write_text(json.dumps(dict(inputs=hashes, checks=records,
                                          readback_sha256=digest(readback)), indent=2) + '\n')


if __name__ == '__main__':
    main()
