#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Check a campaign victory's A action and subsequent pause save on a normal race ROM."""
import argparse
import hashlib
import json
from pathlib import Path

from sc64_skirmish_sram_continuation import KEY_A, KEY_B, KEY_L, KEY_START, PhysicalGame

MISSIONS = {
    "terran": tuple(range(4, 14)),
    "zerg": (0, 1, *range(14, 22)),
    "protoss": (2, 3, *range(22, 30)),
}
FIELDS = ("screen", "frame", "won", "lost", "race_mode", "race_selected_mission", "race_completed")


def state(game):
    return {field: game.u32(field) for field in FIELDS}


def main():
    if not __debug__:
        raise RuntimeError("Replay validation requires Python assertions")
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("rom", "elf", "runner", "victory", "out"):
        parser.add_argument("--" + name, type=Path, required=True)
    parser.add_argument("--race", choices=MISSIONS, required=True)
    parser.add_argument("--chapter", type=int, choices=range(1, 11), required=True)
    args = parser.parse_args()
    missions = MISSIONS[args.race]
    current = missions[args.chapter - 1]
    following = missions[min(args.chapter, len(missions) - 1)]
    args.out.mkdir(parents=True, exist_ok=False)

    def open_game(label, source, output):
        folder = args.out / label
        folder.mkdir()
        (folder / "headless.log").symlink_to("/dev/null")
        return PhysicalGame(args.runner, args.rom, args.elf, folder,
                            save_input=source, save_output=output)

    pause = args.out / "next.sav"
    readback = args.out / "readback.sav"
    game = open_game("advance", args.victory, pause)
    try:
        game.step(0, 2)
        game.step(KEY_L)
        game.step(0)
        victory = state(game)
        assert (victory["screen"], victory["won"], victory["lost"], victory["race_mode"],
                victory["race_selected_mission"]) == (3, 1, 0, 0, current), victory
        assert victory["race_completed"] & (1 << current), victory
        game.tap(KEY_A)
        game.step(0, 100)
        # Opening dialogue must release input before the pause menu can open.
        for _ in range(80):
            if not int.from_bytes(game.rsp.read(0x06013fe0, 2), "little"):
                break
            game.tap(KEY_A)
            game.step(0, 10)
        else:
            raise RuntimeError("Opening dialogue did not finish")
        advanced = state(game)
        assert (advanced["screen"], advanced["won"], advanced["lost"], advanced["race_mode"],
                advanced["race_selected_mission"], advanced["race_completed"]) == (
                    1, 0, 0, 0, following, victory["race_completed"]), advanced
        game.tap(KEY_START)
        assert game.u32("screen") == 2
        game.tap(KEY_B)
        game.rsp.export_sram()
    finally:
        game.close()

    game = open_game("fresh", pause, readback)
    try:
        game.step(0, 2)
        game.step(KEY_L)
        restored = state(game)
        assert restored == advanced, (restored, advanced)
        game.rsp.export_sram()
        assert readback.read_bytes() == pause.read_bytes()
    finally:
        game.close()
    inputs = (args.rom, args.elf, args.runner, args.victory, pause)
    receipt = dict(race=args.race, chapter=args.chapter, victory=victory,
                   advanced=advanced, restored=restored,
                   sha256={str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in inputs})
    (args.out / "result.json").write_text(json.dumps(receipt, indent=2) + "\n")


if __name__ == "__main__":
    main()
