#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Compare ordinary saves with and without the race command context word."""

import argparse
from hashlib import sha256
import json
from pathlib import Path
import shutil

from sc64_skirmish_sram_continuation import (
    KEY_A, KEY_B, KEY_DOWN, KEY_L, KEY_RIGHT, KEY_START, KEY_UP,
    PhysicalGame, RACES, SEAT_OPTIONS, physical_seats, validate_launch_seats,
)

PLAY, CONTINUE = 600, 180
RESERVE = 5 * 1024**3
RESET_UI = dict(selected_building=-1, menu_cursor=0, menu_last=0,
                mode=0, menu_open=0)


def require(condition, message):
    if not condition:
        raise ValueError(message)


def digest(path):
    checksum = sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(2**20), b""):
            checksum.update(chunk)
    return checksum.hexdigest()


def scalar(game, name):
    return int.from_bytes(game.rsp.read(game.sym[name], game.sizes[name]),
                          "little", signed=name == "selected_building")


def ui(game):
    return {key: scalar(game, key) for key in RESET_UI}


def world(game):
    state = game.state()
    del state["screen"]
    state["seats"] = physical_seats(game)
    return state


def fingerprint(state):
    return sha256(json.dumps(state, sort_keys=True, separators=(",", ":"))
                  .encode()).hexdigest()


def select_context(game):
    """Choose a non-default command and select the town hall by button."""
    game.tap(KEY_B)
    require(scalar(game, "menu_open") == 1, "B did not open the command card")
    for key in (KEY_DOWN, KEY_RIGHT, KEY_UP):
        if scalar(game, "menu_cursor") != 0:
            break
        game.tap(key)
    row = scalar(game, "menu_cursor")
    require(0 < row <= 63, "no supported non-default command row")
    game.tap(KEY_A)
    require(scalar(game, "menu_last") == row, "command did not remember its row")
    for _ in range(3):
        if scalar(game, "menu_open") == scalar(game, "mode") == 0:
            break
        game.tap(KEY_B)
    require(scalar(game, "menu_open") == scalar(game, "mode") == 0,
            "could not return to the normal field cursor")
    if scalar(game, "selected_building") < 0:
        game.tap(KEY_A)
    state = ui(game)
    require(state["menu_last"] == row and state["mode"] == 0 and
            state["selected_building"] >= 0, "town hall not selected at the cursor")
    return state


def produce(launch, path, game_map, human, requested):
    archive = path / "pause.sav"
    game = launch(path / "home", save_output=archive)
    try:
        seats = [SEAT_OPTIONS.index(seat) for seat in requested]
        bots = [seat for seat in seats if seat != SEAT_OPTIONS.index("-")]
        game.launch(game_map, RACES.index(human), bots, seats=seats)
        validate_launch_seats(game, requested, physical_seats(game))
        game.step(0, PLAY)
        require(scalar(game, "screen") == 1, "match ended before the save")
        context, saved = select_context(game), world(game)
        game.tap(KEY_START)
        require(scalar(game, "screen") == 2, "START did not reach pause")
        game.step(KEY_B)
        require(scalar(game, "screen") == 2 and ui(game) == context and world(game) == saved,
                "B in the pause menu altered the saved world or its UI")
        game.rsp.export_sram()
        game.step(KEY_A)
        game.step(0, CONTINUE)
        continued = world(game)
        require(scalar(game, "screen") == 1 and continued["frame"] == saved["frame"] + CONTINUE,
                "uninterrupted continuation did not stay in play")
    finally:
        game.close()
    require(archive.stat().st_size == 32768, "invalid journal export length")
    return dict(archive=archive, world=saved, ui=context, continued=continued)


def replay(launch, path, source, expected):
    archive = source["archive"]
    original = digest(archive)
    exported = path / "roundtrip.sav"
    game = launch(path / "title-l", save_input=archive, save_output=exported)
    try:
        require(scalar(game, "screen") == scalar(game, "frame") == 0,
                "fresh process did not reach HOME")
        game.step(KEY_L)
        require(scalar(game, "screen") == 1, "L did not restore the saved match")
        restored = ui(game)
        require(restored == expected, f"restored command context {restored} != {expected}")
        require(world(game) == source["world"], "restored world differs")
        game.step(0, CONTINUE)
        require(scalar(game, "screen") == 1 and world(game) == source["continued"],
                "continued world differs after fresh load")
        game.rsp.export_sram()
    finally:
        game.close()
    require(digest(exported) == digest(archive) == original, "journal changed during load")
    return dict(restored=restored, sha256=original)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("regular-rom", "regular-elf", "context-rom", "context-elf", "runner", "out"):
        parser.add_argument("--" + name, type=Path, required=True)
    parser.add_argument("--pins", nargs=5, metavar="SHA256", required=True,
                        help="regular ROM/ELF, context ROM/ELF, then runner")
    parser.add_argument("--map", type=int, choices=(0, 1), required=True)
    parser.add_argument("--human", choices=tuple(RACES), required=True)
    parser.add_argument("--seats", required=True,
                        help="three physical CPU seats: t, z, p, r (Random), or - (Closed)")
    args = parser.parse_args(argv)
    require(len(args.seats) == 3 and args.seats != "---" and
            all(seat in SEAT_OPTIONS for seat in args.seats), "invalid CPU seats")
    paths = (args.regular_rom, args.regular_elf, args.context_rom, args.context_elf, args.runner)
    require(len({p.resolve() for p in paths}) == 5 and args.pins[0] != args.pins[2],
            "need distinct regular and context images")
    require(list(map(digest, paths)) == args.pins, "digest mismatch in replay inputs")
    require(shutil.disk_usage(args.out.parent).free >= RESERVE, "less than 5 GiB free")
    args.out.mkdir()

    builds = {"regular": paths[:2], "context": paths[2:4]}

    def start(name):
        rom, elf = builds[name]
        return lambda out, **kw: PhysicalGame(args.runner, rom, elf, out, **kw)

    states = {name: produce(start(name), args.out / name, args.map, args.human, args.seats)
              for name in builds}
    a, b = states.values()
    require(all(a[k] == b[k] for k in ("world", "ui", "continued")),
            "control and candidate differed before fresh load")
    reads = {}
    for reader in builds:
        for writer in builds:
            name = f"{reader}-reads-{writer}"
            expected = b["ui"] if reader == writer == "context" else RESET_UI
            reads[name] = replay(start(reader), args.out / name, states[writer], expected)
    require(list(map(digest, paths)) == args.pins, "replay inputs changed")
    report = dict(pins=args.pins, map=args.map, human=args.human, seats=args.seats,
                  saved_frame=a["world"]["frame"], saved_ui=a["ui"], reads=reads,
                  world_sha256=fingerprint(a["world"]), continued_sha256=fingerprint(a["continued"]))
    (args.out / "result.json").write_text(json.dumps(report, indent=2) + "\n")


if __name__ == "__main__":
    main()
