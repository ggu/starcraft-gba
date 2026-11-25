#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Continue the qualified T4 victory through a paid T5 air campaign on a normal Terran ROM."""
import argparse
import hashlib
import json
from pathlib import Path

from sc64_skirmish_sram_continuation import KEY_A, KEY_L, UNIT_FIELDS, PhysicalGame

ROUTE = Path(__file__).resolve().parents[1] / "tests/fixtures/terran-t5-paid-route.json"


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def open_game(args, folder, previous, output):
    folder.mkdir()
    (folder / "headless.log").symlink_to("/dev/null")
    return PhysicalGame(args.runner, args.rom, args.elf, folder,
                        save_input=previous, save_output=output)


def verify_result(game):
    state = game.state()
    units = (dict(zip(UNIT_FIELDS, row[1:])) for row in state["units"])
    heroes = sorted(unit["raw_type"] for unit in units if unit["owner"] == 0
                    and unit["raw_type"] in (16, 19) and unit["hp"] > 0)
    enemies = sum(row[1] == 1 for row in state["buildings"])
    if (state["screen"], game.u32("won"), game.u32("lost"), game.u32("race_mode"),
            game.u32("race_selected_mission"), game.u32("race_completed"),
            enemies, heroes) != (3, 1, 0, 0, 8, 0x1f0, 0, [16, 19]):
        raise RuntimeError("T5 result requires both heroes alive, all enemy buildings destroyed and T1–T5 complete")
    return state


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("rom", "elf", "runner", "previous-victory", "out"):
        parser.add_argument("--" + name, type=Path, required=True)
    args = parser.parse_args()
    route = json.loads(ROUTE.read_text())
    previous = args.previous_victory
    if digest(previous) != route["previous_victory_sha256"]:
        parser.error("expected the T4 victory SRAM from the linked opening replay")
    inputs = (args.rom, args.elf, args.runner, previous, ROUTE)
    pinned = {str(p.resolve()): digest(p) for p in inputs}
    args.out.mkdir()
    receipts = []
    for index, stage in enumerate(route["stages"], 1):
        folder = args.out / f"{index:02d}"
        output = folder / "result.sav"
        game = open_game(args, folder, previous, output)
        try:
            start_cycles = game.rsp.clock()
            for held, count in stage["steps"]:
                if type(count) is not int or count < 1:
                    raise ValueError("invalid button duration")
                game.step(held, count)
            cycles = game.rsp.clock() - start_cycles
            game.rsp.export_sram()
            actual = digest(output)
            if actual != stage["sram_sha256"]:
                raise RuntimeError(f"{stage['name']} diverged at frame {game.u32('frame')}: {actual}")
            receipts.append(dict(stage=stage["name"], frame=game.u32("frame"), sha256=actual,
                                 updates=game.rsp.completed, guest_cycles=cycles))
            if index == len(route["stages"]):
                victory = verify_result(game)
        finally:
            game.close()
        print(f"{index:02d} {stage['name']} matched at frame {receipts[-1]['frame']}", flush=True)
        previous = output

    readback = args.out / "victory-readback.sav"
    game = open_game(args, args.out / "fresh-result", previous, readback)
    try:
        game.step(0, 2)
        game.step(KEY_L)
        if verify_result(game) != victory:
            raise RuntimeError("fresh victory load changed the world")
        game.rsp.export_sram()
        if readback.read_bytes() != previous.read_bytes():
            raise RuntimeError("fresh victory load changed SRAM")
        game.step(0)
        game.step(KEY_A)
        if (game.u32("screen"), game.u32("race_selected_mission"),
                game.u32("race_completed"), game.u32("frame")) != (1, 9, 0x1f0, 0):
            raise RuntimeError("T5 victory did not advance to the opening of T6")
    finally:
        game.close()
    if any(digest(p) != pinned[str(p.resolve())] for p in inputs):
        raise RuntimeError("an input changed during the replay")
    (args.out / "result.json").write_text(json.dumps(
        dict(stages=receipts, inputs=pinned, fresh_result=True, advanced_to="T6"), indent=2) + "\n")


if __name__ == "__main__":
    main()
