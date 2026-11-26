#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Carry a winning P7 SRAM into P8 on the normal Protoss cartridge."""
import argparse
import hashlib
import json
from pathlib import Path
import struct

from sc64_p8_normal_home_replay import parse_plan
from sc64_p8_physical_replay import snapshot
from sc64_skirmish_sram_continuation import KEY_A, KEY_L, PhysicalGame, entity_rows
from sc64_z7_normal_replay import input_hashes, require_unchanged


def require(ok, detail):
    if not ok:
        raise ValueError(detail)


def run(args):
    names = ("rom", "elf", "runner", "seed", "trace", "pages")
    rom, elf, runner, seed, trace, pages = (getattr(args, name) for name in names)
    inputs = (rom, elf, runner, seed, trace, pages, Path(__file__))
    pins = input_hashes(inputs)
    for name, path in zip(names, inputs):
        require(pins[str(path.resolve())] == getattr(args, name + "_sha256"),
                f"{name} hash mismatch")
    out = args.out

    plan, keys, checks, _, _ = parse_plan(trace, pages)
    first = min(checks)
    stop = plan['stop']
    require(plan['recorded_dialogue'] and checks[first]['frame'] == 1
            and keys.get(first - 2) == KEY_A and keys.get(first - 1) == 0,
            'P8 tape lacks the ordinary field release')
    modals = {}
    for line in pages.read_text().splitlines():
        cells = line.split()
        if len(cells) == 8 and all(c.isdecimal() for c in cells):
            tick, frame, row, page, _, _, held, ack = map(int, cells)
            require(first <= tick < stop - 1 and keys.get(tick) == held | ack and keys.get(tick + 1) == held,
                    ('P8 modal input edge', line))
            for phase in range(3):
                modals.setdefault(tick + phase, []).append((phase, frame, row, page, held, ack))
    require(modals, 'P8 tape lacks modal pages')

    out.mkdir()
    victory, readback = out / 'victory.sav', out / 'readback.sav'

    def open_game(name, source, target):
        folder = out / name
        folder.mkdir()
        (folder / 'headless.log').symlink_to('/dev/null')
        return PhysicalGame(runner, rom, elf, folder, save_input=source, save_output=target)

    def state(g):
        s = snapshot(g.rsp, g.sym, g.sizes, elf)
        units = entity_rows(g.rsp.read(g.sym['u'], g.sizes['u']), g.unit_stride, g.unit_layout,
                            ('raw_type', 'owner', 'hp', 'shield'))
        fields = {k: s[k] for k in ('screen', 'frame', 'won', 'lost')}
        fields.update(fault=[run['fault'] for run in s['runners']], outcome=s['runners'][0]['outcome'],
                      triggers_done=[run['done'] for run in s['runners']],
                      stasis_hp=s['stasis_cells'][0] if len(s['stasis_cells']) == 1 else -1)
        for name, raw in (('fenix', 78), ('hyperion', 28)):
            matches = [list(unit[2:]) for unit in units if unit[1] == raw]
            require(len(matches) <= 1, ('duplicate hero', name))
            fields[name] = matches[0] if matches else [-1, -1, -1]
        return s, fields

    g = open_game('guest', seed, victory)
    observations, dialogue = [], []
    tick = None
    try:
        require(g.u32('screen') == g.u32('frame') == g.front_field('page') == 0, 'expected HOME')
        for key in (0, 0, KEY_L, 0):
            g.step(key)
        prior = g.u32('race_completed')
        require((g.u32('screen'), g.u32('race_selected_mission'), g.u32('won'), g.u32('lost'), prior)
                == (3, 26, 1, 0, 0x7c0000c), 'expected P7 victory')
        g.step(KEY_A)
        require((g.u32('screen'), g.u32('race_selected_mission'), g.u32('frame')) == (1, 27, 0), 'P8 RESULT A failed')
        offset = g.rsp.completed - (first - 1)
        edges = [(tick + offset, key) for tick, key in keys.items() if tick >= first - 1]
        caps = set(checks) | set(modals) | set(range(500, stop, 500))
        for tick in sorted(caps):
            g.rsp.play_to(tick + offset, edges)
            count, page, armed, row = struct.unpack('<4H', g.rsp.read(0x06013fe0, 8))
            if tick in modals:
                for phase, frame, want_row, want_page, held, ack in modals[tick]:
                    require(g.u32('frame') == frame, ('P8 modal changed frame', tick, phase))
                    if phase == 2:
                        require(not armed and g.rsp.held == held, ('P8 modal release', tick, armed, g.rsp.held))
                        dialogue.append(tick - 2)
                    else:
                        require(count and (row, page, armed, g.rsp.held) ==
                                (want_row,want_page,ack if phase else 0,held | ack if phase else held),
                                ('P8 modal mismatch', tick, phase, (count, page, armed, row)))
            else:
                require(not count and not armed, ('P8 unexpected dialogue', tick, (count, page, armed, row)))
            if tick != stop:
                require(g.u32('screen') != 3 and not g.u32('won') and not g.u32('lost'), ('P8 premature RESULT', tick))
            if tick in checks:
                s, actual = state(g)
                diff = {k: [checks[tick][k], v] for k, v in actual.items() if checks[tick][k] != v}
                observations.append(dict(call=tick, actual=actual, differences=diff))
                print('P8', tick, actual['frame'], diff, flush=True)
                require(not diff, ('P8 checkpoint', tick, diff))
        expected = state(g)
        term, final = expected
        require((term['screen'], term['won'], term['lost'], term['campaign']['phase'], final['outcome'], final['fault'],
                 term['race_completed'], g.u32('mission_unsupported_count'))
                == (3, 1, 0, 3, 1, [0, 0, 0], prior | (1 << 27), 0), ('P8 terminal', term))
        require(not term['stasis_cells'] and final['triggers_done'][0] & 16 and term['campaign']['objective_flags'] & 1,
                ('P8 missing objective', final))
        g.step(0)
        g.rsp.export_sram()
        g.close()
        g = None
        g = open_game('fresh', victory, readback)
        require(g.u32('screen') == g.u32('frame') == g.front_field('page') == 0,
                'fresh victory process did not start at HOME')
        g.step(KEY_L)
        require(state(g) == expected, 'P8 fresh result mismatch')
        g.step(0)
        g.rsp.export_sram()
        require(victory.read_bytes() == readback.read_bytes(), 'P8 fresh SRAM mismatch')
        require_unchanged(inputs, pins)
        result = dict(result='won', calls=stop + offset, tape_calls=stop, terminal=final, campaign=term['campaign'],
                      completed=term['race_completed'], checkpoints=observations, dialogue=dialogue,
                      victory_sha256=hashlib.sha256(victory.read_bytes()).hexdigest(), sha256=pins)
        (out / 'result.json').write_text(json.dumps(result, indent=2) + '\n')
        return dict(calls=result['calls'], checkpoints=len(observations), dialogue=len(dialogue),
                    victory_sha256=result['victory_sha256'])
    except Exception as exc:
        (out / 'divergence.json').write_text(json.dumps(dict(error=str(exc), call=tick,
            checkpoints=observations, dialogue=dialogue, sha256=pins), indent=2) + '\n')
        raise
    finally:
        if g is not None:
            g.close()

if __name__ == "__main__":
    cli = argparse.ArgumentParser(description=__doc__)
    for name in ("rom", "elf", "runner", "seed", "trace", "pages"):
        cli.add_argument("--" + name, required=True, type=Path)
        cli.add_argument("--" + name + "-sha256", required=True)
    cli.add_argument("--out", required=True, type=Path)
    print(json.dumps(run(cli.parse_args()), indent=2))
