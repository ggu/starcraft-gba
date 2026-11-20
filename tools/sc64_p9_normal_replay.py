#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Replay the P9 normal HOME tape with physical keys and read-only witnesses."""

import argparse
from collections import defaultdict
import hashlib
import json
from pathlib import Path
import shutil
import resource
import struct

import sc64_p8_arm_replay as px
from sc64_carrier_release_physical import slot
from sc64_p8_physical_save import P10_CARGO_TAGS, SLOT_SIZE
from sc64_races_profile import dwarf_struct_layout
from sc64_skirmish_sram_continuation import (
    BUILDING_FIELDS, UNIT_FIELDS, KEY_B, KEY_L, KEY_START, PhysicalGame,
)
from sc64_z7_normal_replay import input_hashes, require_unchanged, timing_delta, timing_snapshot

def save_window(plan, stop):
    record = plan['save']
    if plan['profile'] != 'PROTOSS_P10_FLAGS' or record is None:
        raise ValueError('fresh SRAM requires the normal P10 tape with a pause-save record')
    first = record['first_call']
    end = record['resume_call']
    transitions = dict(plan['transitions'])
    expected = (KEY_START, 0, KEY_B, 0, KEY_START, 0)
    if (not 0 < first < end <= stop or end != first + len(expected) or
            tuple(transitions.get(first + i) for i in range(len(expected))) != expected or
            record['screen'] != 1 or record['menu_last'] != 0):
        raise ValueError('save record is not a neutral ordinary pause, save and resume')
    return first, first + 4


def join_timing(prefix, tail):
    if prefix is None or tail is None:
        return None
    updates = prefix['updates'] + tail['updates']
    cycles = prefix['guest_cycles'] + tail['guest_cycles']
    return dict(tail, updates=updates, guest_cycles=cycles, mean_cycles=cycles / updates,
                effective_main_loop_hz=16777216 * updates / cycles,
                peak_since_boot=max(prefix['peak_since_boot'], tail['peak_since_boot']),
                vblank_buckets=[a + b for a, b in zip(prefix['vblank_buckets'], tail['vblank_buckets'])])


def main():
    if not __debug__:
        raise RuntimeError("Replay validation requires Python assertions")
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('trace', 'rom', 'elf', 'runner', 'out'):
        parser.add_argument('--' + name, required=True, type=Path)
    parser.add_argument('--pages', required=True, type=Path)
    parser.add_argument('--stop', type=int, help='stop after this many calls instead of replaying to victory')
    parser.add_argument('--fresh-save', action='store_true',
                        help='resume the tape from a real pause-menu save in a new emulator process')
    args = parser.parse_args()
    resource.setrlimit(resource.RLIMIT_FSIZE, (16 * 1024**2, 16 * 1024**2))
    files = (args.trace, args.pages, args.rom, args.elf, args.runner, Path(__file__))
    pins = input_hashes(files)
    px.MISSION, px.PREFIX = 28, 'P9'
    plan = px.load_trace(args.trace)
    stop = plan['stop'] if args.stop is None else args.stop
    if not 0 < stop <= plan['stop']:
        parser.error('--stop must fall within the tape')
    save_cut = save_window(plan, stop) if args.fresh_save else None
    assert plan['final']['screen'] == 3 and plan['final']['won'] == 1 and not plan['final']['lost']
    records = defaultdict(list)
    for line in args.trace.read_text().splitlines():
        row = json.loads(line)
        if row['kind'] == 'checkpoint':
            records['checkpoint'].append(row)
    for line in args.pages.read_text().splitlines():
        values=line.split()
        if len(values)==4 and values[1]=='briefing':
            call,page,total=map(int,[values[0],*values[2:]])
            records['briefing'].append(dict(kind='briefing',call_index=call,page=page,total=total))
        elif len(values)==8 and all(v.isdecimal() for v in values):
            call,frame,row,page,trigger,action,held,key=map(int,values)
            records['modal'].append(dict(kind='modal',call_index=call,frame=frame,row=row,
                                         page=page,trigger=trigger,action=action,held=held,key=key))
    briefing_count = len(records['briefing'])
    assert 0<briefing_count<=32 and [r['page'] for r in records['briefing']]==list(range(briefing_count))
    assert all(r['total']==briefing_count for r in records['briefing'])
    assert plan['final']['outcome']==1 and plan['final']['switches']&3==3
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
    runtime = dwarf_struct_layout(args.elf, 'Sc64P9PersistentState')[1]
    runner_size, runner = dwarf_struct_layout(args.elf, 'Sc64P9Runner')
    args.out.mkdir()
    def open_game(name, **options):
        guest = args.out / name
        guest.mkdir()
        (guest / 'headless.log').symlink_to('/dev/null')
        return PhysicalGame(args.runner, args.rom, args.elf, guest, **options)

    save_path = args.out / 'p9-pause.sav'
    g = open_game('guest', **({'save_output': save_path} if save_cut else {}))
    r, sym, sizes = g.rsp, g.sym, g.sizes
    transitions, offset, journal, prefix = plan['transitions'], 0, None, None
    saved_world = None
    if save_cut:
        _, layout = dwarf_struct_layout(args.elf, 'RaceSave')

    def n(k, layout=None, field=None):
        off = 0 if layout is None else layout[field]
        size = sizes[k] if layout is None else layout.widths[field]
        assert size in (1, 2, 4) and off + size <= sizes[k], (k, field, off, size)
        return int.from_bytes(r.read(sym[k] + off, size), 'little')

    def persisted_world():
        world = g.state()
        # Compare the game state while paused as well as after the HOME load.
        world.pop('screen')
        return dict(world=world, campaign=r.read(sym['race_p9'], sizes['race_p9']),
                    controls={k: n(k) for k in ('cx', 'cy', 'camx', 'camy',
                                                 'mode', 'selected_building', 'menu_open', 'menu_last')})

    def modal():
        return struct.unpack('<4H', r.read(0x06013FE0, 8))

    def observe():
        s = g.state()
        units, buildings = s['units'], s['buildings']
        def hero(raw, fields):
            return next(([x[u[k]] for k in fields] for x in units if x[u['raw_type']]==raw),
                        [-1]*len(fields))
        switches=n('race_p9', runtime, 'switches')
        cerebrate=[0,-1]
        for i,owner in enumerate((6,5)):
            if not switches & (1<<i):
                actor=next((x for x in buildings if x[b['raw_type']]==151 and
                            x[b['raw_owner']]==owner),None)
                if actor:
                    cerebrate=[i+1,actor[b['hp']]]
                    break
        state=r.read(sym['race_p9'],sizes['race_p9'])
        def runner_value(i,key):
            start=runtime['runner']+runner_size*i+runner[key]
            return int.from_bytes(state[start:start+runner.widths[key]],'little')
        return dict(frame=s['frame'], screen=s['screen'], won=n('won'), lost=n('lost'),
                    side_units=sum(x[u['owner']]==0 for x in units),
                    probes=sum(x[u['owner']]==0 and x[u['raw_type']]==64 for x in units),
                    enemy_units=sum(x[u['owner']]==1 for x in units),
                    enemy_buildings=sum(x[b['owner']]==1 for x in buildings),
                    cerebrate=cerebrate, zeratul=hero(75,('owner','hp','shield','x','y')),
                    fenix=hero(78,('owner','hp','shield')), bank=[s['minerals'],s['gas']],
                    supply=[n('supply_used'),min(n('supply_cap'), 400)], switches=switches,
                    done=[runner_value(i,'done') for i in range(4)], outcome=runner_value(0,'outcome'))

    briefing, pages, observations = [], [], []
    try:
        assert n('screen') == n('frame') == g.front_field('page') == 0
        before = timing_snapshot(r, sym, sizes)
        boundaries = set(i for i in checks if i <= stop) | {stop} | set(range(1000, stop, 1000))
        if save_cut:
            boundaries.update(save_cut)
        for call in sorted(boundaries):
            if shutil.disk_usage(args.out).free < 4 << 30:
                raise RuntimeError('disk guard')
            r.play_to(call + offset, transitions)
            if save_cut and call == save_cut[0]:
                count, _, armed, _ = modal()
                assert n('screen') == 1 and count == armed == r.held == 0
                saved_world = persisted_world()
                assert saved_world['controls']['mode'] == saved_world['controls']['menu_open'] == 0
            if save_cut and call == save_cut[1]:
                assert n('screen') == 2 and n('pause_notice') & 0xc0 == 0x40 and r.held == 0
                assert saved_world == persisted_world(), 'ordinary pause changed the saved world'
                r.export_sram()
                image = save_path.read_bytes()
                inner, outer = P10_CARGO_TAGS
                payload = slot(image, 0, outer, 1)
                assert image[SLOT_SIZE:] == b'\xff' * SLOT_SIZE
                assert len(payload) >= layout['state'] + 32
                assert struct.unpack_from('<I', payload, layout['version'])[0] == inner
                state = struct.unpack_from('<8I', payload, layout['state'])
                assert (state[0], state[2], state[7]) == (0, 28, saved_world['world']['frame'])
                assert state[7] + 1 == plan['save']['frame']
                journal = dict(call=call, frame=state[7], sequence=1, payload_bytes=len(payload),
                               tag=f'{outer:#010x}', sha256=hashlib.sha256(image).hexdigest(),
                               under_construction=plan['save']['under_construction'])
                prefix = timing_delta(before, timing_snapshot(r, sym, sizes), call)
                g.close()
                g = None
                g = open_game('fresh', save_input=save_path)
                r, sym, sizes = g.rsp, g.sym, g.sizes
                before = timing_snapshot(r, sym, sizes)
                assert n('screen') == n('frame') == g.front_field('page') == 0
                g.step(KEY_L)
                assert n('screen') == 1 and n('race_selected_mission') == 28
                loaded = persisted_world()
                assert loaded == saved_world, ('HOME L diverged from the save',
                                                [k for k in loaded if loaded[k] != saved_world[k]])
                offset = r.completed - (call + 1)
                transitions = [(tick + offset, mask) for tick, mask in transitions if tick > call]
                (args.out / 'save-cut.json').write_text(json.dumps(journal, indent=2) + '\n')
                print('Reloaded', call, 'at frame', state[7], 'from fresh HOME', flush=True)
            rows = checks.get(call, ())
            active = modal() if rows else None
            for row in rows:
                kind = row['kind']
                if kind == 'briefing':
                    obs = dict(page=g.front_field('briefing_page'), revealed=g.front_field('briefing_revealed'),
                               total=g.front_field('briefing_pages'), mission=g.front_field('mission'),
                               front=g.front_field('page'), screen=n('screen'))
                    assert obs == dict(page=row['page'], revealed=1, total=briefing_count, mission=28, front=3, screen=0), (row, obs)
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
                    assert not active[0] and n('race_mode') == 0 and n('race_human') == 2 and n('race_selected_mission') == 28
                    obs = observe()
                    expected = {k: row[k] for k in obs}
                    if obs != expected:
                        (args.out / 'divergence.json').write_text(json.dumps(dict(row=row, expected=expected, observed=obs), indent=2) + '\n')
                        raise ValueError(f'first observed divergence at {call}: {obs} != {expected}')
                    observations.append(dict(call_index=call, label=row['label'], **obs))
                    print('Matched', call, row['label'], obs['frame'], 'switches', obs['switches'], flush=True)
        result_state = None
        if stop == plan['stop']:
            assert (len(briefing), len(pages), len(observations)) == (
                briefing_count, len(records['modal']), len(records['checkpoint']))
            result_state = dict(selected=n('race_selected_mission'), phase=n('race_campaign', campaign, 'phase'),
                                objective_flags=n('race_campaign', campaign, 'objective_flags'),
                                completed=n('race_completed'), switches=n('race_p9', runtime, 'switches'),
                                faults=n('mission_unsupported_count'), modal_pages=len(pages))
            assert result_state['selected']==28 and result_state['phase']==3
            assert result_state['objective_flags']&1 and result_state['completed']==1<<28
            assert result_state['switches']&3==3 and result_state['faults']==0
            assert runner_size > runner['outcome']
            assert r.read(sym['race_p9'] + runtime['runner'] + runner['outcome'], 1) == b'\x01'
        require_unchanged(files, pins)
        timing = timing_delta(before, timing_snapshot(r, sym, sizes), stop - (save_cut[1] if save_cut else 0))
        if save_cut:
            assert journal and hashlib.sha256(save_path.read_bytes()).hexdigest() == journal['sha256']
            timing = join_timing(prefix, timing)
        result = dict(status='won' if result_state else 'prefix', calls=stop, briefing=briefing,
                      pages=pages, checkpoints=observations, final=result_state, sha256=pins, timing=timing)
        if journal:
            result['fresh_save'] = journal
            result['post_load_checkpoints'] = sum(row['call_index'] > journal['call'] for row in observations)
        (args.out / 'result.json').write_text(json.dumps(result, indent=2) + '\n')
        print(result['status'], stop, len(briefing), len(pages), len(observations), result['timing'], flush=True)
    finally:
        if g is not None:
            g.close()


if __name__ == "__main__":
    main()
