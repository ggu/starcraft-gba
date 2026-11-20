#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Replay the P10 normal HOME tape with physical keys and read-only witnesses."""

import argparse
from collections import defaultdict
import hashlib
import json
from pathlib import Path
import resource
import shutil
import struct

import sc64_p8_arm_replay as px
from sc64_carrier_release_physical import slot
from sc64_p8_physical_save import P10_CARGO_TAGS, SLOT_SIZE
from sc64_p10_tape_save import join_timing, save_window
from sc64_races_profile import dwarf_struct_layout
from sc64_skirmish_sram_continuation import BUILDING_FIELDS, UNIT_FIELDS, KEY_L, PhysicalGame
from sc64_z7_normal_replay import input_hashes, require_unchanged, timing_delta, timing_snapshot

def main():
    if not __debug__:
        raise RuntimeError("Replay validation requires Python assertions")
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('trace', 'rom', 'elf', 'runner', 'out'):
        parser.add_argument('--' + name, required=True, type=Path)
    parser.add_argument('--stop', type=int, help='stop after this many calls instead of replaying to victory')
    parser.add_argument('--fresh-save', action='store_true',
                        help='reload the pause SRAM, then verify the result autosave on a full run')
    args = parser.parse_args()
    if args.fresh_save:
        resource.setrlimit(resource.RLIMIT_FSIZE, (16 * 1024**2, 16 * 1024**2))
    files = (args.trace, args.rom, args.elf, args.runner, Path(__file__),
             Path(__file__).with_name('sc64_p10_tape_save.py'))
    pins = input_hashes(files)
    px.MISSION, px.PREFIX = 29, 'P10'
    plan = px.load_trace(args.trace)
    stop = plan['stop'] if args.stop is None else args.stop
    if not 0 < stop <= plan['stop']:
        parser.error('--stop must fall within the tape')
    save_cut = save_window(plan, stop) if args.fresh_save else None
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
    def open_game(name, **options):
        directory = args.out / name
        if save_cut:
            directory.mkdir()
            (directory / 'headless.log').symlink_to('/dev/null')
        return PhysicalGame(args.runner, args.rom, args.elf, directory, **options)

    pause_path = args.out / 'pause.sav'
    result_path = args.out / 'victory.sav'
    g = open_game('guest', save_output=pause_path if save_cut else None)
    r, sym, sizes = g.rsp, g.sym, g.sizes
    transitions, offset = plan['transitions'], 0
    saved_world = pause_image = journal = result_journal = prefix = None
    if save_cut:
        _, save_layout = dwarf_struct_layout(args.elf, 'RaceSave')

    def n(k, layout=None, field=None):
        off = 0 if layout is None else layout[field]
        size = sizes[k] if layout is None else layout.widths[field]
        assert size in (1, 2, 4) and off + size <= sizes[k], (k, field, off, size)
        return int.from_bytes(r.read(sym[k] + off, size), 'little')

    def persisted_world():
        world = g.state()
        world.pop('screen')
        return dict(world=world, mission=r.read(sym['race_p10'], sizes['race_p10']),
                    controls={k: n(k) for k in ('cx', 'cy', 'camx', 'camy',
                                                 'selected_building', 'menu_last')})

    def journal_bank(image, index, expected_frame, victory):
        inner, outer = P10_CARGO_TAGS
        payload = slot(image, index, outer, index + 1)
        assert len(payload) >= save_layout['state'] + 80
        assert struct.unpack_from('<I', payload, save_layout['version'])[0] == inner
        state = struct.unpack_from('<20I', payload, save_layout['state'])
        assert (state[0], state[1], state[2], state[7]) == (0, 2, 29, expected_frame)
        assert (state[3], state[18], state[19]) == (int(victory) << 29, int(victory), 0)
        return dict(sequence=index + 1, frame=expected_frame, payload_bytes=len(payload),
                    tag=f'{outer:#010x}', sha256=hashlib.sha256(image).hexdigest())

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
        if save_cut:
            boundaries.update(save_cut)
        for call in sorted(boundaries):
            if shutil.disk_usage(args.out).free < 4 << 30:
                raise RuntimeError('disk guard')
            r.play_to(call + offset, transitions)
            if save_cut and call == save_cut[0]:
                count, _, armed, _ = modal()
                assert n('screen') == 1 and count == armed == r.held == 0
                assert n('mode') == n('menu_open') == 0
                saved_world = persisted_world()
            if save_cut and call == save_cut[1]:
                assert n('screen') == 2 and n('pause_notice') & 0xc0 == 0x40 and r.held == 0
                assert saved_world == persisted_world(), 'pause changed the saved world'
                r.export_sram()
                pause_image = pause_path.read_bytes()
                assert pause_image[SLOT_SIZE:] == b'\xff' * SLOT_SIZE
                saved_frame = saved_world['world']['frame']
                assert saved_frame + 1 == plan['save']['frame']
                journal = dict(call=call, **journal_bank(pause_image, 0, saved_frame, False))
                prefix = timing_delta(before, timing_snapshot(r, sym, sizes), call)
                g.close()
                g = None
                g = open_game('fresh', save_input=pause_path, save_output=result_path)
                r, sym, sizes = g.rsp, g.sym, g.sizes
                before = timing_snapshot(r, sym, sizes)
                assert n('screen') == n('frame') == g.front_field('page') == 0
                g.step(KEY_L)
                assert n('screen') == 1 and n('race_selected_mission') == 29
                loaded = persisted_world()
                assert loaded == saved_world, ('pause reload diverged',
                                               [k for k in loaded if loaded[k] != saved_world[k]])
                offset = r.completed - call - 1
                transitions = [(tick + offset, mask) for tick, mask in transitions if tick > call]
                (args.out / 'save-cut.json').write_text(json.dumps(journal, indent=2) + '\n')
                print('Reloaded', call, 'at frame', saved_frame, 'from fresh HOME', flush=True)
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
        timing = timing_delta(before, timing_snapshot(r, sym, sizes),
                              stop - (save_cut[1] if save_cut else 0))
        if save_cut:
            timing = join_timing(prefix, timing)
            assert journal and pause_path.read_bytes() == pause_image
            if result_state:
                victory_world = persisted_world()
                r.export_sram()
                victory_image = result_path.read_bytes()
                assert victory_image[:SLOT_SIZE] == pause_image[:SLOT_SIZE], 'victory overwrote the pause bank'
                result_journal = journal_bank(victory_image, 1, n('frame'), True)
                g.close()
                g = None
                readback = args.out / 'victory-readback.sav'
                g = open_game('victory-load', save_input=result_path, save_output=readback)
                r, sym, sizes = g.rsp, g.sym, g.sizes
                assert n('screen') == n('frame') == g.front_field('page') == 0
                g.step(KEY_L)
                assert (n('screen'), n('won'), n('lost'), n('race_completed')) == (3, 1, 0, 1 << 29)
                loaded = persisted_world()
                assert loaded == victory_world, ('victory reload diverged',
                                                [k for k in loaded if loaded[k] != victory_world[k]])
                assert n('race_impact_count') == 0
                g.step(0, 8)
                assert n('screen') == 3 and persisted_world() == loaded
                r.export_sram()
                assert readback.read_bytes() == result_path.read_bytes() == victory_image
                result_journal['fresh_load'] = True
                result_journal['neutral_result_updates'] = 8
        require_unchanged(files, pins)
        result = dict(status='won' if result_state else 'prefix', calls=stop, briefing=briefing,
                      pages=pages, checkpoints=observations, final=result_state, sha256=pins, timing=timing)
        if journal:
            result['fresh_save'] = journal
            result['post_load_checkpoints'] = sum(row['call_index'] > journal['call'] for row in observations)
        if result_journal:
            result['victory_autosave'] = result_journal
        (args.out / 'result.json').write_text(json.dumps(result, indent=2) + '\n')
        print(result['status'], stop, len(briefing), len(pages), len(observations), result['timing'], flush=True)
    finally:
        if g is not None:
            g.close()


if __name__ == "__main__":
    main()
