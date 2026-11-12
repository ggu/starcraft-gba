#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Replay the P10 normal HOME tape with physical keys and read-only witnesses."""

import argparse
from collections import defaultdict
import json
from pathlib import Path
import shutil
import struct

import sc64_p8_arm_replay as px
from sc64_races_profile import dwarf_struct_layout
from sc64_skirmish_sram_continuation import BUILDING_FIELDS, UNIT_FIELDS, PhysicalGame
from sc64_z7_normal_replay import input_hashes, require_unchanged, timing_delta, timing_snapshot

def main():
    if not __debug__:
        raise RuntimeError("Replay validation requires Python assertions")
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('trace', 'rom', 'elf', 'runner', 'out'):
        parser.add_argument('--' + name, required=True, type=Path)
    parser.add_argument('--stop', type=int, help='stop after this many calls instead of replaying to victory')
    args = parser.parse_args()
    files = (args.trace, args.rom, args.elf, args.runner, Path(__file__))
    pins = input_hashes(files)
    px.MISSION, px.PREFIX = 29, 'P10'
    plan = px.load_trace(args.trace)
    stop = plan['stop'] if args.stop is None else args.stop
    if not 0 < stop <= plan['stop']:
        parser.error('--stop must fall within the tape')
    assert plan['final']['screen'] == 3 and plan['final']['won'] == 1 and not plan['final']['lost']
    records = defaultdict(list)
    for line in args.trace.read_text().splitlines():
        row = json.loads(line)
        if row['kind'] in ('briefing', 'modal', 'checkpoint', 'mission'):
            records[row['kind']].append(row)
    assert [r['page'] for r in records['briefing']] == list(range(10))
    assert len(records['mission']) == 1
    terminal, = records['mission']
    assert terminal == dict(kind='mission', selected=29, phase=3, objective_flags=1,
                            completed=1 << 29, switches=257, faults=0, modal_pages=14)
    assert len(records['modal']) == terminal['modal_pages']
    keys = dict(plan['transitions'])
    checks = defaultdict(list)
    for kind in ('briefing', 'modal', 'checkpoint'):
        for row in records[kind]:
            checks[row['call_index']].append(row)
            if kind == 'modal':
                tick, held, key = (row[k] for k in ('call_index', 'held', 'key'))
                assert key == (8 if held & 1 else 1) and keys[tick] == held | key and keys[tick + 1] == held
                checks[tick + 1].append(dict(row, kind='modal_armed'))
                checks[tick + 2].append(dict(row, kind='modal_released'))
    print('Validated', plan['stop'], 'ordinary calls and', len(keys), 'held-key changes', flush=True)

    u = {k: i + 1 for i, k in enumerate(UNIT_FIELDS)}
    b = {k: i + 1 for i, k in enumerate(BUILDING_FIELDS)}
    campaign = dwarf_struct_layout(args.elf, 'Sc64RaceCampaignState')[1]
    runtime = dwarf_struct_layout(args.elf, 'Sc64P10PersistentState')[1]
    runner_size, runner = dwarf_struct_layout(args.elf, 'Sc64P10Runner')
    args.out.mkdir()
    g = PhysicalGame(args.runner, args.rom, args.elf, args.out / 'guest')
    r, sym, sizes = g.rsp, g.sym, g.sizes

    def n(k, layout=None, field=None):
        off = 0 if layout is None else layout[field]
        size = sizes[k] if layout is None else layout.widths[field]
        assert size in (1, 2, 4) and off + size <= sizes[k], (k, field, off, size)
        return int.from_bytes(r.read(sym[k] + off, size), 'little')

    def modal():
        return struct.unpack('<4H', r.read(0x06013FE0, 8))

    def observe():
        s = g.state()
        units, buildings = s['units'], s['buildings']
        def hero(raw):
            return next(([x[u['owner']], x[u['hp']]] for x in units if x[u['raw_type']] == raw), [-1, -1])
        return dict(frame=s['frame'], screen=s['screen'], won=n('won'), lost=n('lost'),
                    side_units=sum(x[u['owner']]==0 for x in units),
                    scvs=sum(x[u['owner']]==0 and x[u['raw_type']]==7 for x in units),
                    enemy_units=sum(x[u['owner']]==1 for x in units),
                    enemy_buildings=sum(x[b['owner']]==1 for x in buildings),
                    overmind_hp=next((x[b['hp']] for x in buildings if x[b['raw_type']]==147), -1),
                    zeratul=hero(75), gantrithor=hero(82), hyperion=hero(28),
                    bank=[s['minerals'], s['gas']], cursor=[n('cx'), n('cy')])

    briefing, pages, observations = [], [], []
    try:
        assert n('screen') == n('frame') == g.front_field('page') == 0
        before = timing_snapshot(r, sym, sizes)
        boundaries = set(i for i in checks if i <= stop) | {stop} | set(range(1000, stop, 1000))
        for call in sorted(boundaries):
            if shutil.disk_usage(args.out).free < 4 << 30:
                raise RuntimeError('disk guard')
            r.play_to(call, plan['transitions'])
            rows = checks.get(call, ())
            active = modal() if rows else None
            for row in rows:
                kind = row['kind']
                if kind == 'briefing':
                    obs = dict(page=g.front_field('briefing_page'), revealed=g.front_field('briefing_revealed'),
                               total=g.front_field('briefing_pages'), mission=g.front_field('mission'),
                               front=g.front_field('page'), screen=n('screen'))
                    assert obs == dict(page=row['page'], revealed=1, total=10, mission=29, front=3, screen=0), (row, obs)
                    briefing.append(dict(call_index=call, **obs))
                elif kind == 'modal':
                    count, page, armed, current = active
                    assert 0 < count <= 13 and (page, armed, current) == (row['page'], 0, row['row']), (row, active)
                    assert n('frame') == row['frame'] and r.held == row['held'], row
                    pages.append(row)
                elif kind == 'modal_armed':
                    count, page, armed, current = active
                    assert 0 < count <= 13 and (page, armed, current) == (row['page'], row['key'], row['row']), (row, active)
                    assert n('frame') == row['frame'] and r.held == row['held'] | row['key'], row
                elif kind == 'modal_released':
                    assert active[2] == 0 and n('frame') == row['frame'] and r.held == row['held'], (row, active)
                    assert not active[0] or (active[1], active[3]) != (row['page'], row['row']), (row, active)
                else:
                    assert not active[0] and n('race_mode') == 0 and n('race_human') == 2 and n('race_selected_mission') == 29
                    obs = observe()
                    expected = {k: row[k] for k in obs}
                    if obs != expected:
                        (args.out / 'divergence.json').write_text(json.dumps(dict(row=row, expected=expected, observed=obs), indent=2) + '\n')
                        raise ValueError(f'first observed divergence at {call}: {obs} != {expected}')
                    observations.append(dict(call_index=call, label=row['label'], **obs))
                    print('Matched', call, row['label'], obs['frame'], 'Overmind HP', obs['overmind_hp'], flush=True)
        result_state = None
        if stop == plan['stop']:
            assert len(briefing) == 10 and len(pages) == 14 and len(observations) == len(records['checkpoint'])
            result_state = dict(selected=n('race_selected_mission'), phase=n('race_campaign', campaign, 'phase'),
                                objective_flags=n('race_campaign', campaign, 'objective_flags'),
                                completed=n('race_completed'), switches=n('race_p10', runtime, 'switches'),
                                faults=n('mission_unsupported_count'), modal_pages=len(pages))
            assert dict(kind='mission', **result_state) == terminal, result_state
            assert runner_size > runner['outcome']
            assert r.read(sym['race_p10'] + runtime['runner'] + runner['outcome'], 1) == b'\x01'
        require_unchanged(files, pins)
        result = dict(status='won' if result_state else 'prefix', calls=stop, briefing=briefing,
                      pages=pages, checkpoints=observations, final=result_state, sha256=pins,
                      timing=timing_delta(before, timing_snapshot(r, sym, sizes), stop))
        (args.out / 'result.json').write_text(json.dumps(result, indent=2) + '\n')
        print(result['status'], stop, len(briefing), len(pages), len(observations), result['timing'], flush=True)
    finally:
        g.close()


if __name__ == "__main__":
    main()
