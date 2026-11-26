#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Compare one earned race campaign result and its next chapter on two cartridges."""
import argparse
import json
from pathlib import Path

from sc64_linked_stage_replay import digest, open_game
from sc64_protoss_linked_replay import require
from sc64_skirmish_sram_continuation import KEY_A, KEY_L, summary

MISSIONS = {
    'terran': tuple(range(4, 14)),
    'zerg': (0, 1, *range(14, 22)),
    'protoss': (2, 3, *range(22, 30)),
}


def replay(args, label, slots):
    mask = sum(1 << slot for slot in slots[:args.chapter])
    result = args.out / f'{label}.sav'
    game = open_game(args, args.out / label, args.victory, result)
    try:
        game.step(KEY_L)
        observed = tuple(game.u32(k) for k in ('screen', 'race_selected_mission', 'race_completed',
                                                'won', 'lost', 'mission_unsupported_count'))
        require(observed == (3, slots[args.chapter - 1], mask, 1, 0, 0),
                f'{label} loaded an unexpected campaign result: {observed}')
        world = game.state()
        game.rsp.export_sram()
        require(result.read_bytes() == args.victory.read_bytes(), f'{label} changed the exported SRAM')
        game.step(0)
        game.step(KEY_A)
        advance = [game.u32(k) for k in ('screen', 'race_selected_mission', 'race_completed', 'frame')]
        if args.chapter < len(slots):
            require(advance == [1, slots[args.chapter], mask, 0], f'{label} handoff differs: {advance}')
        return world, advance
    finally:
        game.close()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--race', choices=MISSIONS, required=True)
    parser.add_argument('--chapter', type=int, choices=range(1, 11), required=True)
    for name in ('reference-rom', 'reference-elf', 'rom', 'elf', 'runner', 'victory', 'out'):
        parser.add_argument('--' + name, type=Path, required=True)
    args = parser.parse_args()
    args.out.mkdir()
    files = (args.rom, args.elf, args.reference_rom, args.reference_elf, args.victory, args.runner)
    pins = {str(p): digest(p) for p in files}
    reference = argparse.Namespace(**vars(args))
    reference.rom, reference.elf = args.reference_rom, args.reference_elf
    before = replay(reference, 'reference', MISSIONS[args.race])
    after = replay(args, 'candidate', MISSIONS[args.race])
    require(before == after, 'the observed world or chapter advance differs between builds')
    require(all(digest(p) == pins[str(p)] for p in files), 'input changed during comparison')
    (args.out / 'result.json').write_text(json.dumps(dict(race=args.race, chapter=args.chapter,
                                          inputs=pins, world=summary(after[0]), advance=after[1]), indent=2) + '\n')


if __name__ == '__main__':
    main()
