#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Check refinery queue order across an ordinary pause save and fresh GBA boot."""
import argparse
import hashlib
import json
from pathlib import Path

from sc64_skirmish_sram_continuation import (
    KEY_A, KEY_B, KEY_L, KEY_START, SCREEN_PAUSE, SCREEN_PLAY, PhysicalGame, summary,
)


def read_queues(raw, sites, units):
    """Decode one-based worker links, checking every member and both ends."""
    if len(raw) != 2 * (sites + units):
        raise ValueError("unexpected gas queue layout")
    heads = raw[:sites]
    tails = raw[sites:2 * sites]
    links = raw[2 * sites:2 * sites + units]
    membership = raw[2 * sites + units:]
    seen = set()
    queues = {}
    for site, link in enumerate(heads):
        queue = []
        while link:
            if link > units or link in seen or membership[link - 1] != site + 1:
                raise ValueError(f"invalid worker link at gas site {site}")
            seen.add(link)
            queue.append(link - 1)
            link = links[link - 1]
        if tails[site] != (queue[-1] + 1 if queue else 0):
            raise ValueError(f"incorrect queue tail at gas site {site}")
        if queue:
            queues[site] = queue
    if any((owner or links[slot]) and slot + 1 not in seen
           for slot, owner in enumerate(membership)):
        raise ValueError("orphaned gas queue member")
    return queues


def snapshot(game):
    queues = read_queues(game.rsp.read(game.sym['gas_service'], game.sizes['gas_service']),
                         game.sizes['b'] // game.building_stride,
                         game.sizes['u'] // game.unit_stride)
    state = summary(game.state())
    return dict(frame=state['frame'], world=state['fingerprint'], queues=queues,
                mode=game.u32('race_mode'), mission=game.u32('race_selected_mission'))


def boot(args, phase, seed):
    return PhysicalGame(args.runner, args.rom, args.elf, args.out / phase,
                        save_input=seed, save_output=args.out / (phase + '.sav'))


def load(game):
    game.step(0, 2)
    game.step(KEY_L)
    if game.u32('screen') != SCREEN_PLAY:
        raise ValueError("fresh boot did not load a game in progress")


def continuation(game, count):
    result = []
    for _ in range(count):
        game.step()
        result.append(snapshot(game))
    return result


def run(args):
    game = boot(args, 'reference', args.seed)
    try:
        load(game)
        if args.advance:
            game.step(0, args.advance)
        game.tap(KEY_START)
        if game.u32('screen') != SCREEN_PAUSE:
            raise ValueError("START did not pause the match")
        game.step(KEY_B)
        saved = snapshot(game)
        if not any(len(q) >= 2 for q in saved['queues'].values()):
            raise ValueError("choose a save point with a worker waiting behind another")
        game.rsp.export_sram()
        game.step(KEY_A)
        expected = continuation(game, args.steps)
    finally:
        game.close()
    sram = args.out / 'reference.sav'
    game = boot(args, 'fresh', sram)
    try:
        load(game)
        loaded = snapshot(game)
        resumed = continuation(game, args.steps)
    finally:
        game.close()
    cuts = [(saved, loaded), *zip(expected, resumed)]
    mismatches = [[a['frame'], k] for a, b in cuts for k in a if a[k] != b[k]]
    return dict(saved=saved, restored=loaded, steps=args.steps, mismatches=mismatches,
                rom_sha256=hashlib.sha256(args.rom.read_bytes()).hexdigest(),
                seed_sha256=hashlib.sha256(args.seed.read_bytes()).hexdigest(),
                sram_sha256=hashlib.sha256(sram.read_bytes()).hexdigest())


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('runner', 'rom', 'elf', 'seed', 'out'):
        parser.add_argument(name, type=Path)
    parser.add_argument('--advance', type=int, default=17)
    parser.add_argument('--steps', type=int, default=128)
    args = parser.parse_args()
    if args.advance < 0 or args.steps < 1:
        parser.error("advance must be nonnegative and steps must be positive")
    args.out.mkdir(parents=True, exist_ok=False)
    report = run(args)
    (args.out / 'result.json').write_text(json.dumps(report, indent=2) + '\n')
    if report['mismatches']:
        parser.exit(1, f"First mismatch {report['mismatches'][0]}\n")
    print(f"Queue order and world state matched on fresh load and {args.steps} updates.")


if __name__ == '__main__':
    main()
