#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Carry a winning P6 SRAM into P7 on the normal Protoss cartridge."""
import argparse
import json
from pathlib import Path
import struct

from sc64_p7_physical_replay import layout, snapshot
from sc64_skirmish_sram_continuation import KEY_A, KEY_L, KEY_START, PhysicalGame
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

    def open_game(name, source, target):
        folder = out / name
        folder.mkdir()
        (folder / "headless.log").symlink_to("/dev/null")
        return PhysicalGame(runner, rom, elf, folder, save_input=source, save_output=target)

    rows = [json.loads(line) for line in trace.read_text().splitlines()]
    require(rows[0]['schema'] == 1 and rows[0]['mission'] == 26
            and rows[0]['profile'] == 'PROTOSS_P10_FLAGS', 'wrong P7 controller tape')
    checks = {row['call_index']: row for row in rows if row['kind'] == 'checkpoint'}
    start, stop = min(checks), max(checks)
    require(checks[start]['label'] == 'launch' and checks[start]['frame'] == 1
            and checks[stop]['label'] == 'final' and checks[stop]['won'] == 1
            and checks[stop]['lost'] == checks[stop]['runtime_fault'] == 0
            and rows[-1]['kind'] == 'strict-verdict' and rows[-1]['ok'] == 1,
            'host did not prove P7')
    edges = {}
    held = 0
    for row in rows:
        if row['kind'] == 'input':
            tick, key, pressed = (row[k] for k in ('call_index', 'held', 'pressed'))
            require(0 <= key <= 1023 and key != held and pressed == key & ~held
                    and key & 48 != 48 and key & 192 != 192
                    and (not edges or tick > next(reversed(edges))), ('bad edge', row))
            edges[tick] = held = key
    require(edges[start - 2] == KEY_START and edges[start - 1] == 0, 'launch release lost')
    modals = {}
    for line in pages.read_text().splitlines():
        cells = line.split()
        if len(cells) == 6 and all(c.isdecimal() for c in cells):
            tick, frame, row, page, held, ack = map(int, cells)
            require(start <= tick < stop - 1 and edges.get(tick) == held | ack
                    and edges.get(tick + 1) == held, ('modal trace mismatch', line))
            for phase in range(3):
                modals.setdefault(tick + phase, []).append((phase, frame, row, page, held, ack))
    require(modals, 'no authored pages')

    out.mkdir()
    victory, readback = out / 'victory.sav', out / 'readback.sav'
    g = open_game('guest', seed, victory)
    observations, dialogue = [], []
    tick = None
    try:
        l = layout(elf, g.sym, g.sizes)

        def state(g):
            p7 = snapshot(g.rsp, g.sym, g.sizes, l)
            return dict(world=g.state(), p7=p7, completed=g.u32('race_completed'),
                        p7_runtime=g.rsp.read(g.sym['race_p7'], g.sizes['race_p7']).hex())

        def compare(host):
            seen = state(g)
            p = seen['p7']
            actual = {k: p[k] for k in ('frame', 'screen', 'won', 'lost', 'outcome', 'switch_bits')}
            actual.update(runtime_fault=p['fault'], heroes_alive=sum(bool(n) for n in p['heroes'].values()),
                          units=len(seen['world']['units']), buildings=len(seen['world']['buildings']),
                          side_units=sum(row[1] == 0 for row in seen['world']['units']),
                          cursor=[g.u32('cx'), g.u32('cy')], bank=[g.u32('minerals'),g.u32('gas')],
                          mode=g.u32('mode'))
            diff = {k: [host[k], v] for k, v in actual.items() if v != host[k]}
            observations.append(dict(call=tick, actual=actual, differences=diff))
            require(not diff, ('P7 checkpoint', tick, diff))

        require(g.u32('screen') == g.u32('frame') == g.front_field('page') == 0,
                'initial process did not start at HOME')
        for key in (0, 0, KEY_L, 0):
            g.step(key)
        prior = g.u32('race_completed')
        require((g.u32('screen'), g.u32('race_selected_mission'), g.u32('won'), g.u32('lost'), prior)
                == (3, 25, 1, 0, 0x03c0000c), 'not the P6 winning result')
        g.step(KEY_A)
        require((g.u32('screen'), g.u32('race_selected_mission'), g.u32('frame')) == (1, 26, 0), 'P7 RESULT A failed')
        offset = g.rsp.completed - (start - 1)
        physical = [(t + offset, k) for t, k in edges.items() if start - 1 <= t < stop]
        caps = set(checks) | set(modals) | set(range(500, stop, 500))
        for tick in sorted(caps):
            g.rsp.play_to(tick + offset, physical)
            count, page, armed, row = struct.unpack('<4H', g.rsp.read(0x06013fe0, 8))
            if tick in modals:
                for phase, frame, want_row, want_page, held, ack in modals[tick]:
                    require(g.u32('frame') == frame, ('modal changed frame', tick, phase))
                    if phase == 2:
                        require(not armed and g.rsp.held == held, ('modal release', tick, armed, g.rsp.held))
                        dialogue.append(tick - 2)
                    else:
                        require(count and (row, page, armed, g.rsp.held) ==
                                (want_row, want_page, ack if phase else 0, held | ack if phase else held),
                                ('modal mismatch', tick, phase, (count, page, armed, row)))
            else:
                require(not count and not armed, ('unexpected dialogue', tick, (count, page, armed, row)))
            if tick != stop:
                require(g.u32('screen') == 1 and not g.u32('won') and not g.u32('lost'), ('premature RESULT', tick))
            if tick in checks:
                compare(checks[tick])
                print(tick, checks[tick]['frame'], flush=True)
        expected = state(g)
        term = expected['p7']
        require((term['screen'], term['won'], term['lost'], term['phase'], term['outcome'], term['fault'],
                 g.u32('mission_unsupported_count'), expected['completed']) == (3, 1, 0, 3, 1, 0, 0, prior | (1 << 26)),
                ('P7 terminal mismatch', term))
        require(term['action_timers_clear'] and term['victory_cursor'] == 0 and len(term['heroes']) == 3
                and all(n == 1 for n in term['heroes'].values()), ('P7 strict result', term))
        g.step(0)
        g.rsp.export_sram()
        g.close()
        g = None
        g = open_game('fresh', victory, readback)
        require(g.u32('screen') == g.u32('frame') == g.front_field('page') == 0,
                'fresh victory process did not start at HOME')
        g.step(KEY_L)
        require(state(g) == expected, 'P7 fresh result mismatch')
        g.step(0)
        g.rsp.export_sram()
        require(victory.read_bytes() == readback.read_bytes(), 'P7 fresh SRAM mismatch')
        require_unchanged(inputs, pins)
        result = dict(result='won', calls=stop + offset, tape_calls=stop, terminal=term,
                      completed=expected['completed'], checkpoints=observations, dialogue=dialogue,
                      victory_sha256=input_hashes((victory,))[str(victory.resolve())], sha256=pins)
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
