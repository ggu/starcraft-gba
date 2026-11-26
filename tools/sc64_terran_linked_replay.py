#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Replay Terran T6 through T10 from the preceding victory journal.

All gameplay uses physical buttons. Observations and dialogue handling reuse
sc64_t9_normal_replay and the Terran controller observer; no world state is
injected. The complete result world and journal are checked on a fresh boot.
"""
import argparse
from functools import lru_cache
import hashlib
import json
import shutil
from pathlib import Path
import struct

from sc64_skirmish_sram_continuation import KEY_A, KEY_L, KEY_START, PhysicalGame
import sc64_races_profile as races
import sc64_skirmish_sram_continuation as physical
import sc64_t9_normal_replay as normal
import sc64_t9_arm_replay as t9
import sc64_terran_controller_replay as terran
from sc64_t9_normal_replay import (T6_FIELDS, T7_FIELDS, T8_FIELDS, T10_FIELDS, T7_DIALOGUE, T8_DIALOGUE,
                                  load_late_trace, observe_t7_carrier)
from sc64_terran_controller_replay import _compare_trace_row, _find_raw_actor, _snapshot, _verify_terminal
from sc64_t9_arm_replay import COMPARED as T9_FIELDS, load_trace as load_t9_trace

ROUTES = {
    6: (23849, T6_FIELDS, {0, 680, 1792, 3765, 9056, 11559, 19659, 20578, 23846, 23849}),
    7: (4189, T7_FIELDS, {0, 1, 2, 347, 1000, 1264, 2000, 2200, 2334, 3000, 4000, 4189}),
    8: (290459, T8_FIELDS, {0, 120, 332, 507, 1645, 5795, 290459, *range(1000, 290459, 1000)}),
    9: (50204, T9_FIELDS, set()),
    10: (3446, T10_FIELDS, {0, 120, 1000, 2000, 3000, 3446}),
}


def require(condition, message):
    if not condition:
        raise ValueError(message)


def compare(chapter, expected, observed, prior, carrier):
    """Compare the retained tape with inherited completion bits included."""
    row = dict(expected)
    require("race_completed" in row, "checkpoint lacks completion mask")
    row["race_completed"] |= prior
    if chapter == 6:
        _compare_trace_row(row, observed)
        return
    fields = {7: T7_FIELDS, 8: T8_FIELDS, 9: T9_FIELDS, 10: T10_FIELDS}[chapter]
    diff = {key: [row.get(key), observed.get(key)] for key in fields
            if key not in row or key not in observed or row[key] != observed[key]}
    require(not diff, f'T{chapter} checkpoint {row.get("host_tick")} mismatch: {diff}')
    if chapter == 7:
        require(row.get("tracked_carrier") == carrier, 'T7 tracked emitter carrier differs')


def run(args):
    # The ELF is pinned for this run. Its DWARF layouts are immutable, but
    # the shared observer asks for them at every checkpoint.
    cached_dwarf = lru_cache(maxsize=None)(races.dwarf_struct)
    chapter = args.chapter
    mission = chapter + 3
    stop, fields, ticks = ROUTES[chapter]
    plan = load_t9_trace(args.trace) if chapter == 9 else load_late_trace(args.trace, chapter, fields, stop, ticks)
    require(plan['stop'] == stop, 'trace has wrong terminal tick')
    plan["mission"] = mission
    require(plan["final"]["kind"] == "won", 'trace does not end in victory')
    transitions = [(tick - 1, held) for tick, held in plan["transitions"]]
    if not transitions or transitions[0][0] != 0:
        transitions.insert(0, (0, 0))
    files = (args.seed, args.trace, args.rom, args.elf, args.runner, Path(__file__),
             Path(races.__file__), Path(physical.__file__), Path(normal.__file__), Path(terran.__file__),
             Path(t9.__file__))
    pins = {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in files}
    args.out.mkdir()
    victory, readback = args.out / 'victory.sav', args.out / 'readback.sav'

    def open_game(name, seed, output):
        root = args.out / name
        root.mkdir()
        (root / 'headless.log').symlink_to('/dev/null')
        return PhysicalGame(args.runner, args.rom, args.elf, root, save_input=seed, save_output=output)

    g = open_game('game', args.seed, victory)
    carrier = -1
    checkpoints, dialogue = [], []

    def n(name):
        return g.u32(name)

    def observe():
        original = races.dwarf_struct
        races.dwarf_struct = cached_dwarf
        try:
            return _snapshot(g.rsp, g.sym, g.sizes, args.elf, mission)
        finally:
            races.dwarf_struct = original

    def modal():
        return struct.unpack('<4H', g.rsp.read(0x06013fe0, 8))

    def persisted():
        world = g.state()
        world.pop('screen')
        layout = g.unit_layout
        units = g.rsp.read(g.sym['u'], g.sizes['u'])
        width, offset = layout.widths['selected'], layout['selected']
        selected = [slot for slot in range(len(units) // g.unit_stride)
                    if int.from_bytes(units[slot * g.unit_stride + offset:
                                            slot * g.unit_stride + offset + width], 'little')]
        mission_symbol = 'race_t6' if chapter == 6 else 'race_late'
        return dict(world=world, runtime=g.rsp.read(g.sym[mission_symbol], g.sizes[mission_symbol]).hex(),
                    controls={**{key: n(key) for key in ('cx', 'cy', 'menu_last')},
                              'selected_units': selected})

    def acknowledge(tick, frame, limit, allow_fewer=False):
        for _ in range(limit):
            count, page, armed, row = modal()
            if allow_fewer and count == 0:
                break
            require(0 < count <= 13 and not armed and row < 256 and n('frame') == frame,
                    f'invalid dialogue at tick {tick}')
            held = g.rsp.held
            key = KEY_START if held & KEY_A else KEY_A
            g.step(held | key)
            require(modal() == (count, page, key, row) and n('frame') == frame,
                    f'dialogue press diverged at tick {tick}')
            g.step(held)
            require(n('frame') == frame, f'dialogue advanced game at tick {tick}')
            dialogue.append(dict(tick=tick, frame=frame, row=row, page=page))
        require(modal()[0] == 0, f'unclosed dialogue at tick {tick}')

    try:
        require((n('screen'), n('frame'), g.front_field('page')) == (0, 0, 0), 'not at HOME')
        g.step(0, 2)
        g.step(KEY_L)
        g.step(0)
        prior = n('race_completed')
        required = sum(1 << i for i in range(4, mission))
        require(prior & required == required, f'predecessor missing required chapters: {prior:#x}')
        require((n('screen'), n('won'), n('lost'), n('race_selected_mission'),
                 n('mission_unsupported_count')) == (3, 1, 0, mission - 1, 0),
                'seed is not the previous chapter victory')
        g.step(KEY_A)
        require((n('screen'), n('frame'), n('race_selected_mission'), n('race_completed')) ==
                (1, 0, mission, prior), 'result advance did not enter expected chapter')
        offset = g.rsp.completed
        edges = [(tick + offset, mask) for tick, mask in transitions]
        cuts = (T7_DIALOGUE if chapter == 7 else T8_DIALOGUE if chapter == 8 else
                {1: (1, 4), 50189: (49471, 9)} if chapter == 9 else {1: (1, 4)} if chapter == 10 else {})
        scan = range(1, stop + 1) if chapter == 6 else range(289963, stop + 1) if chapter == 8 else ()
        dialogue_count = 13 if chapter == 6 else 7 if chapter == 8 else sum(x[1] for x in cuts.values())
        for tick in sorted(set(plan['observations']) | set(cuts) | set(scan)):
            require(shutil.disk_usage(args.out).free >= 64 << 20, 'replay needs at least 64 MiB free')
            if tick:
                g.rsp.play_to(tick + offset, edges)
                if tick in cuts:
                    acknowledge(tick, *cuts[tick])
                    offset = g.rsp.completed - tick
                    edges = [(t + offset, mask) for t, mask in transitions if t >= tick]
                elif tick in scan and modal()[0]:
                    acknowledge(tick, n('frame'), dialogue_count - len(dialogue), allow_fewer=True)
                    offset = g.rsp.completed - tick
                    edges = [(t + offset, mask) for t, mask in transitions if t >= tick]
                else:
                    require(modal()[0] == 0, f'unexpected dialogue at tick {tick}')
            if tick not in plan['observations']:
                continue
            observed = observe()
            if chapter == 7:
                carrier = observe_t7_carrier(g, observed, carrier)
            elif chapter == 9:
                observed['selected_building'] = n('selected_building')
                observed['selection'] = len(persisted()['controls']['selected_units'])
            elif chapter == 10:
                hero = _find_raw_actor(observed['units'], 19)
                cannon = _find_raw_actor(observed['buildings'], 127)
                observed['hero_slot'] = hero['slot'] if hero else -1
                observed['hero_raw'] = hero['raw_type'] if hero else -1
                observed['ion_cannon_slot'] = cannon['slot'] if cannon else -1
            compare(chapter, plan['observations'][tick], observed, prior, carrier)
            checkpoints.append(dict(tick=tick, frame=observed['frame'], completed=observed['race_completed']))
            print(f'T{chapter}: matched {tick}, frame {observed["frame"]}', flush=True)
        require(len(checkpoints) == len(plan['observations']), 'not every checkpoint ran')
        require(len(dialogue) == dialogue_count,
                'dialogue count differs')
        _verify_terminal(plan, observed)
        final_mask = prior | (1 << mission)
        require((n('race_completed'), n('mission_unsupported_count')) == (final_mask, 0),
                'victory did not preserve completion bits')
        final = persisted()
        g.rsp.export_sram()
    finally:
        g.close()
    g = open_game('fresh', victory, readback)
    try:
        g.step(KEY_L)
        require((n('screen'), n('won'), n('lost'), n('race_selected_mission'), n('race_completed')) ==
                (3, 1, 0, mission, final_mask), 'fresh result reload differs')
        require(persisted() == final, 'fresh result world, mission or selection differs')
        require(n('selected_building') == -1 and n('mode') == n('menu_open') == 0,
                'fresh result controls differ')
        g.step(0, 8)
        require(n('screen') == 3 and persisted() == final, 'result changed after reload')
        g.rsp.export_sram()
        require(readback.read_bytes() == victory.read_bytes(), 'fresh result changed SRAM')
    finally:
        g.close()
    require(pins == {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in files}, 'input changed')
    result = dict(chapter=chapter, prior=prior, completed=final_mask, frame=observed['frame'],
                  checkpoints=checkpoints, dialogue=dialogue, inputs=pins,
                  victory_sha256=hashlib.sha256(victory.read_bytes()).hexdigest())
    (args.out / 'result.json').write_text(json.dumps(result, indent=2) + '\n')
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('seed', 'trace', 'rom', 'elf', 'runner', 'out'):
        parser.add_argument('--' + name, type=Path, required=True)
    parser.add_argument('--chapter', type=int, choices=tuple(ROUTES), required=True)
    args = parser.parse_args()
    run(args)


if __name__ == '__main__':
    main()
