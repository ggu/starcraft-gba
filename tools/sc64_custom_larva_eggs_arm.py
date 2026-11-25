#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Custom Game Larva and Eggs on the ARM product ROM, from a blank save.

Process 1 boots the unchanged ROM with an empty SRAM in the pinned port-free
mGBA backend and plays only through the physical buttons. It opens a Custom
Game with a Zerg human and Zerg computer opponents, selects the human's
Hatchery with the cursor and, once the bank holds three Drones' price and a
computer opponent has at least two Eggs in progress, orders three Drones:
three Eggs at once. START then B saves through the pause menu, A resumes, and
the uninterrupted game runs --continue updates. The 32 KiB SRAM is exported.

Process 2 boots the same ROM with that SRAM. L on HOME loads it through the
game's own loader. The restored world must equal the saved one, and the same
number of neutral updates must reproduce process 1.

Guest memory is only read. The comparison is the continuation tool's
fingerprint: frame, banks, mineral fields and the persisted fields of every
live unit and building, including Larva, Larva timers and Egg timers.

--perf instead plays one passive match for --perf-end updates and reports the
emulated CPU cycles of the window that starts at --perf-start, with the unit
and Egg counts at both ends, for comparing two ROMs over the same updates.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from sc64_skirmish_sram_continuation import (  # noqa: E402
    KEY_A, KEY_B, KEY_DOWN, KEY_L, KEY_RIGHT, KEY_START, KEY_UP, RACES, SCREEN_PAUSE, SCREEN_PLAY,
    ContinuationError, PhysicalGame, summary)

KEY_SELECT, KEY_LEFT = 4, 32
EGG_RAW = 36
SIDE, ZERG = 0, RACES.index("z")
DRONE_PRICE = 50
BOT_OWNERS = (1, 3, 4)


def rows(game, bank: str):
    """(slot, fields) of every live row of the unit or building bank."""
    stride = game.unit_stride if bank == "u" else game.building_stride
    layout = game.unit_layout if bank == "u" else game.building_layout
    data = game.rsp.read(game.sym[bank], game.sizes[bank])
    out = []
    for slot in range(len(data) // stride):
        row = data[slot * stride:(slot + 1) * stride]

        def get(name):
            offset, width = layout[name], layout.widths[name]
            return int.from_bytes(row[offset:offset + width], "little", signed=width > 1)
        if get("alive") != 1:
            continue
        names = ("owner", "type", "x", "y", "raw_type", "target", "work_timer") if bank == "u" else \
                ("owner", "type", "x", "y", "under_construction", "larva", "larva_timer", "queue_count")
        out.append((slot, {name: get(name) for name in names}))
    return out


def eggs(game, owner=None):
    return [(slot, unit) for slot, unit in rows(game, "u")
            if unit["raw_type"] == EGG_RAW and (owner is None or unit["owner"] == owner)]


def census(game, halls: dict) -> dict:
    """Per-owner counts a reader can check beside the fingerprints.

    halls maps an owner to the slots of the town halls it started with.
    """
    units, buildings = rows(game, "u"), dict(rows(game, "b"))
    return {str(owner): {
        "units": sum(u["owner"] == owner for _, u in units),
        "eggs": sum(u["owner"] == owner and u["raw_type"] == EGG_RAW for _, u in units),
        "egg_timers": sorted(u["work_timer"] for _, u in units if u["owner"] == owner and u["raw_type"] == EGG_RAW),
        "queued": sum(b["queue_count"] for b in buildings.values() if b["owner"] == owner),
        "start_hall_larva": [buildings[slot]["larva"] for slot in slots
                             if slot in buildings and buildings[slot]["owner"] == owner],
    } for owner, slots in sorted(halls.items())}


def move_cursor(game, x: int, y: int) -> None:
    for _ in range(4000):
        dx, dy = x - game.u32("cx"), y - game.u32("cy")
        if abs(dx) <= 1 and abs(dy) <= 1:
            game.step(0)
            return
        keys = 0
        if dx > 1:
            keys |= KEY_RIGHT
        if dx < -1:
            keys |= KEY_LEFT
        if dy > 1:
            keys |= KEY_DOWN
        if dy < -1:
            keys |= KEY_UP
        game.step(keys)
    raise ContinuationError("the cursor did not reach the Hatchery")


def clock(game) -> int:
    """The emulated cycle clock, read without advancing the game."""
    return int(game.rsp.cmd("t"))


def identities(args) -> dict:
    return {"rom_sha256": hashlib.sha256(args.rom.read_bytes()).hexdigest(),
            "elf_sha256": hashlib.sha256(args.elf.read_bytes()).hexdigest(),
            "runner_sha256": hashlib.sha256(args.runner.read_bytes()).hexdigest()}


def launch(game, args):
    """Start the lobby; return the live buildings and each owner's starting hall slots."""
    bots = [RACES.index(c) for c in args.bots]
    game.launch(args.map, RACES.index(args.human), bots)
    buildings = rows(game, "b")
    halls = {owner: [slot for slot, b in buildings if b["owner"] == owner]
             for owner in (SIDE,) + BOT_OWNERS[:len(bots)]}
    return buildings, halls


def run_eggs(args) -> dict:
    out = args.output
    if out.exists():
        raise ContinuationError(f"{out} already exists")
    out.mkdir(parents=True)
    sram = out / "phase1.sav"
    record = {**identities(args), "map": args.map, "human": args.human, "bots": args.bots,
              "continue": args.continue_updates, "transport": "physical-buttons"}
    game = PhysicalGame(args.runner, args.rom, args.elf, out / "phase1", save_output=sram)
    try:
        for name in ("cx", "cy", "selected_building", "train_type"):
            if name not in game.sym:
                raise ContinuationError(f"ELF lacks symbol {name}")
        buildings, halls = launch(game, args)
        hall_slot, hall = next((slot, b) for slot, b in buildings if b["owner"] == SIDE)
        units = rows(game, "u")
        human_types = [u["type"] for _, u in units if u["owner"] == SIDE]
        drone = max(set(human_types), key=human_types.count)   # four Drones, one Overlord
        record["start"] = {"human_larva": hall["larva"], "census": census(game, halls)}
        if hall["larva"] != 3:
            raise ContinuationError("the starting Hatchery does not hold three Larva")
        move_cursor(game, hall["x"], hall["y"])
        game.tap(KEY_A)
        if game.u32("selected_building") != hall_slot:
            raise ContinuationError("A on the Hatchery did not select it")
        for _ in range(24):
            if game.u32("train_type") == drone:
                break
            game.tap(256)  # R: next product
        if game.u32("train_type") != drone:
            raise ContinuationError("the Drone was not offered by the Hatchery")
        # Wait, Hatchery selected, for three Drones' price and for a computer
        # opponent with at least two Eggs that still have time left.
        for _ in range(args.wait):
            young = [e for e in eggs(game) if e[1]["owner"] != SIDE and e[1]["work_timer"] > 150]
            if game.u32("minerals") >= 3 * DRONE_PRICE and len(young) >= 2:
                break
            game.step(0, 4)
        else:
            raise ContinuationError("no moment with the bank and two computer Eggs was reached")
        bank = game.u32("minerals")
        record["before_orders"] = {"frame": game.u32("frame"), "minerals": bank, "census": census(game, halls)}
        for _ in range(3):
            game.tap(KEY_A)
        mine = eggs(game, SIDE)
        hall_now = dict(rows(game, "b"))[hall_slot]
        record["after_orders"] = {"frame": game.u32("frame"), "minerals": game.u32("minerals"),
                                  "human_egg_slots": [slot for slot, _ in mine],
                                  "human_egg_places": [[e["x"], e["y"]] for _, e in mine],
                                  "human_larva": hall_now["larva"], "census": census(game, halls)}
        if (len(mine) != 3 or any(e["target"] != drone for _, e in mine) or hall_now["larva"] != 0 or
                hall_now["queue_count"] != 0 or game.u32("minerals") > bank - 3 * DRONE_PRICE + 16 or
                len({(e["x"], e["y"]) for _, e in mine}) != 3):
            raise ContinuationError(f"three Drone orders did not make three Eggs: {record['after_orders']}")
        game.step(0, 30)
        game.tap(KEY_SELECT)
        if len(eggs(game, SIDE)) != 3 or len([e for e in eggs(game) if e[1]["owner"] != SIDE]) < 2:
            raise ContinuationError("the Eggs did not last until the save")
        game.tap(KEY_START)
        if game.u32("screen") != SCREEN_PAUSE:
            raise ContinuationError("START did not open the pause menu")
        game.step(KEY_B)
        saved = game.state()
        record["saved"] = {**summary(saved), "census": census(game, halls)}
        game.step(KEY_A)
        game.step(0, args.continue_updates)
        reference = game.state()
        record["reference"] = {**summary(reference), "census": census(game, halls),
                               "human_drones": sum(u["owner"] == SIDE and u["type"] == drone
                                                   for _, u in rows(game, "u"))}
        game.rsp.export_sram()
    finally:
        game.close()
    data = sram.read_bytes()
    record["sram_bytes"], record["sram_sha256"] = len(data), hashlib.sha256(data).hexdigest()
    after = out / "phase2.sav"
    game = PhysicalGame(args.runner, args.rom, args.elf, out / "phase2", save_input=sram, save_output=after)
    try:
        game.step(0, 2)
        game.step(KEY_L)
        record["loaded"] = bool(game.u32("screen") == SCREEN_PLAY and game.u32("race_mode") == 1)
        record["restored"] = {**summary(game.state()), "census": census(game, halls)}
        game.step(0, args.continue_updates)
        record["resumed"] = {**summary(game.state()), "census": census(game, halls)}
        game.rsp.export_sram()
    finally:
        game.close()
    record["sram_unchanged"] = after.read_bytes() == data
    saved_census = record["saved"]["census"]
    bots_with_eggs = [o for o in map(str, BOT_OWNERS) if saved_census.get(o, {}).get("eggs", 0) >= 2]
    record["checks"] = {
        "loaded": record["loaded"],
        "saved_with_three_human_eggs": saved_census[str(SIDE)]["eggs"] == 3,
        "saved_with_two_or_more_computer_eggs": bool(bots_with_eggs),
        "no_serial_queue_entry": all(row["queued"] == 0 for row in saved_census.values()),
        "restored_equals_saved": record["restored"]["fingerprint"] == record["saved"]["fingerprint"] and
                                 record["restored"]["census"] == saved_census,
        "resumed_equals_reference": record["resumed"]["fingerprint"] == record["reference"]["fingerprint"] and
                                    record["resumed"]["census"] == record["reference"]["census"],
        "updates_match": (record["reference"]["frame"] - record["saved"]["frame"] == args.continue_updates and
                          record["resumed"]["frame"] - record["restored"]["frame"] == args.continue_updates),
        "human_eggs_hatched": record["reference"]["human_drones"] >= 7,
        "sram_unchanged_by_load": record["sram_unchanged"],
    }
    record["ok"] = all(record["checks"].values())
    return record


def run_perf(args) -> dict:
    out = args.output
    if out.exists():
        raise ContinuationError(f"{out} already exists")
    out.mkdir(parents=True)
    record = {**identities(args), "map": args.map, "human": args.human, "bots": args.bots,
              "perf_start": args.perf_start, "perf_end": args.perf_end, "transport": "physical-buttons"}
    game = PhysicalGame(args.runner, args.rom, args.elf, out / "perf")
    try:
        _, halls = launch(game, args)
        first = game.u32("frame")
        game.step(0, args.perf_start)
        if game.u32("screen") != SCREEN_PLAY:
            raise ContinuationError("the match ended before the window")
        start_clock, start_frame = clock(game), game.u32("frame")
        record["window_start"] = {"frame": start_frame - first, "census": census(game, halls)}
        game.step(0, args.perf_end - args.perf_start)
        end_clock, end_frame = clock(game), game.u32("frame")
        record["window_end"] = {"frame": end_frame - first, "census": census(game, halls),
                                "screen": game.u32("screen")}
        record["window_updates"] = end_frame - start_frame
        record["window_cycles"] = end_clock - start_clock
        record["cycles_per_update"] = round((end_clock - start_clock) / max(1, end_frame - start_frame))
        record["ok"] = record["window_updates"] == args.perf_end - args.perf_start
    finally:
        game.close()
    return record


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--runner", type=Path, required=True)
    parser.add_argument("--rom", type=Path, required=True)
    parser.add_argument("--elf", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--map", type=int, default=0)
    parser.add_argument("--human", default="z", choices=tuple(RACES))
    parser.add_argument("--bots", default="z")
    parser.add_argument("--wait", type=int, default=3000, help="most four-update waits for the order moment")
    parser.add_argument("--continue-updates", type=int, default=900)
    parser.add_argument("--perf", action="store_true")
    parser.add_argument("--perf-start", type=int, default=9000)
    parser.add_argument("--perf-end", type=int, default=10800)
    args = parser.parse_args(argv)
    if not args.bots or len(args.bots) > 3 or any(c not in RACES for c in args.bots):
        parser.error("--bots is one to three of t, z, p")
    if not args.perf and (args.human != "z" or "z" not in args.bots):
        parser.error("the Egg run needs a Zerg human and a Zerg computer opponent")
    if args.perf and not 0 < args.perf_start < args.perf_end:
        parser.error("--perf-start must precede --perf-end")
    try:
        record = run_perf(args) if args.perf else run_eggs(args)
    except (ContinuationError, RuntimeError, ValueError) as exc:
        print(json.dumps({"ok": False, "error": str(exc)}))
        return 1
    (args.output / "result.json").write_text(json.dumps(record, indent=1) + "\n")
    print(json.dumps(record))
    return 0 if record["ok"] else 1


if __name__ == "__main__":
    sys.exit(main())
