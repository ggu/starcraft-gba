#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Play a modal-aware P2 controller trace from the actual P1 victory SRAM."""
import argparse
import hashlib
import json
from pathlib import Path

from sc64_protoss_linked_replay import require
from sc64_races_profile import dwarf_struct_layout
from sc64_skirmish_sram_continuation import KEY_A, KEY_L, PhysicalGame


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('rom', 'elf', 'runner', 'seed', 'trace', 'out'):
        parser.add_argument('--' + name, type=Path, required=True)
    args = parser.parse_args()
    trace = json.loads(args.trace.read_text())
    require(trace['mission'] == 3 and trace['event_overflow'] == 0, 'expected complete P2 tape')
    # HOME and the P1 RESULT menu reach the same P2 simulation at different ticks.
    cut = 56
    keys = {row['tick']: row['held'] for row in trace['events']}
    require(keys.get(cut + 4) == KEY_A and keys.get(cut + 5) == 0, 'missing launch transition')
    events = [(tick - cut, held) for tick, held in keys.items() if tick > cut + 5]
    size, fields = dwarf_struct_layout(args.elf, 'Sc64RaceCampaignState')
    args.out.mkdir()

    def open_game(name, seed, target):
        folder = args.out / name
        folder.mkdir()
        (folder / 'headless.log').symlink_to('/dev/null')
        return PhysicalGame(args.runner, args.rom, args.elf, folder, save_input=seed, save_output=target)

    def observe(game):
        data = game.rsp.read(game.sym['race_campaign'], size)
        row = {key: game.u32(key) for key in ('screen', 'frame', 'won', 'lost')}
        for key in ('objective_flags', 'elapsed_frames', 'phase'):
            offset = fields[key]
            row[key] = int.from_bytes(data[offset:offset + fields.widths[key]], 'little')
        return row

    victory, readback = [args.out / name for name in ('victory.sav', 'readback.sav')]
    game = open_game('game', args.seed, victory)
    checks = []
    try:
        for key in (0, 0, KEY_L, 0):
            game.step(key)
        require([game.u32(k) for k in ('screen', 'won', 'lost', 'race_selected_mission',
                                        'race_completed')] == [3, 1, 0, 2, 4], 'expected P1 victory')
        game.step(KEY_A)
        game.step(0)
        require(game.u32('race_selected_mission') == 3 and game.u32('frame') == 1, 'P2 launch failed')
        for expected in [*trace['checkpoints'], dict(trace['expected'], tick=trace['total_ticks'])]:
            tick = expected['tick']
            game.rsp.play_to(tick - cut, events)
            got = observe(game)
            world = game.state()
            for name in ('units', 'buildings'):
                for owner, side in enumerate(('side', 'enemy')):
                    key = side + '_' + name
                    if key in expected:
                        got[key] = sum(entity[1] == owner for entity in world[name])
            diff = {k: (expected[k], v) for k, v in got.items() if expected[k] != v}
            require(not diff, f'P2 call {tick} differs: {diff}')
            checks.append(dict(tick=tick, **got))
        require((got['screen'], got['won'], got['lost'], game.u32('race_completed'),
                 game.u32('mission_unsupported_count')) == (3, 1, 0, 12, 0), 'expected cumulative P2 victory')
        game.rsp.export_sram()
    finally:
        game.close()
    fresh = open_game('fresh', victory, readback)
    try:
        fresh.step(KEY_L)
        require(fresh.state() == world and observe(fresh) == observe_final(checks[-1]),
                'loaded world or objectives differ')
        require(fresh.u32('race_completed') == 12 and fresh.u32('race_selected_mission') == 3,
                'loaded campaign progress differs')
        fresh.rsp.export_sram()
        require(readback.read_bytes() == victory.read_bytes(), 'SRAM changed on reload')
    finally:
        fresh.close()
    files = (args.rom, args.elf, args.runner, args.seed, args.trace, victory, readback)
    hashes = {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in files}
    (args.out / 'result.json').write_text(json.dumps(dict(checks=checks, sha256=hashes), indent=2))


def observe_final(row):
    return {k: row[k] for k in ('screen', 'frame', 'won', 'lost', 'objective_flags', 'elapsed_frames', 'phase')}


if __name__ == '__main__':
    main()
