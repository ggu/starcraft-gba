#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Replay a winning Wasteland host tape through the regular ARM controls."""

import argparse
from hashlib import sha256
import json
from pathlib import Path
import re
import shutil
import struct
import subprocess

from sc64_skirmish_sram_continuation import PhysicalGame, UNIT_FIELDS
from sc64_t1_trace_export import validate

ROUND_TRIP = (8, 0, 2, 0, 256, 0, 8, 0, 512, 0)
WORLD_FIELDS = ("screen", "frame", "race_selected_mission", "race_mode", "minerals", "gas")
COMMAND_FIELDS = ("selected_building", "menu_last", "menu_cursor", "mode", "menu_open")
T1 = 4


def require(condition, why):
    if not condition:
        raise ValueError(why)


def digest(path):
    h = sha256()
    with path.open("rb") as f:
        for chunk in iter(lambda: f.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def read_types(elf):
    info = subprocess.run(["arm-none-eabi-readelf", "--debug-dump=info", elf], check=True,
                          capture_output=True, text=True).stdout
    result = {}
    for name in ("U_SCV", "U_MARINE", "U_ZERGLING", "U_RAYNOR"):
        values = set(re.findall(rf"DW_AT_name\s*:.*\b{name}\s*\n"
                                r"[^\n]*DW_AT_const_value\s*:\s*([0-9]+)", info))
        require(len(values) == 1, f"ELF does not name one value for {name}")
        result[name] = int(values.pop())
    return result


def find_roundtrip(events):
    held = [x["held"] for x in events]
    starts = [events[i]["tick"] for i in range(len(events) - len(ROUND_TRIP) + 1)
              if tuple(held[i:i + len(ROUND_TRIP)]) == ROUND_TRIP
              and all(events[i + d]["tick"] == events[i]["tick"] + d
                      for d in range(len(ROUND_TRIP)))]
    require(len(starts) == 1 and starts[0] > 6, "expected the Wasteland pause/save/restart/reload sequence")
    return starts[0]


def checkpoint(game, types, tick):
    state = game.state()
    owned = UNIT_FIELDS.index("owner") + 1
    kind = UNIT_FIELDS.index("type") + 1

    def count(owner, unit):
        return sum(u[owned] == owner and u[kind] == types[unit] for u in state["units"])

    hero = next((u for u in state["units"] if u[kind] == types["U_RAYNOR"]), None)
    return dict(tick=tick, frame=state["frame"], minerals=state["minerals"],
                marines=count(0, "U_MARINE"), scv=count(0, "U_SCV"), zerg=count(1, "U_ZERGLING"),
                raynor_alive=int(hero is not None), raynor_owner=hero[owned] if hero else -1,
                won=game.u32("won"), lost=game.u32("lost"))


def replay(args):
    paths = (args.trace, args.rom, args.elf, args.runner)
    require(list(map(digest, paths)) == args.pins, "replay inputs do not match their SHA-256 pins")
    total, events, points, terminal = validate(json.loads(args.trace.read_text()))
    require(total <= 25000, "Wasteland replay exceeds its update budget")
    roundtrip = find_roundtrip(events)
    keys = {row["tick"]: row["held"] for row in events}
    require(tuple(keys.get(i) for i in range(5)) == (1, 0, 1, 0, 8), "not a T1 HOME tape")
    require(shutil.disk_usage(args.out.parent).free >= 6 * 2**30, "less than 6 GiB free")
    types = read_types(args.elf)
    points = {p["tick"]: p for p in points}
    points[total] = terminal
    args.out.mkdir()
    save = args.out / "sram.sav"
    g = PhysicalGame(args.runner, args.rom, args.elf, args.out / "guest", save_output=save)
    rsp = g.rsp
    report = dict(pins=args.pins, briefing=[], dialogue=[], checkpoints=[], boundary=[])

    def advance(held=None):
        if held is not None:
            rsp.set_keys(held)
        rsp._advance(1)

    def tap(key):
        advance(key)
        advance(0)

    def modal():
        return struct.unpack("<4H", rsp.read(0x06013fe0, 8))

    def reveal_briefing():
        require((g.front_field("page"), g.front_field("mission"), g.u32("screen")) == (3, T1, 0),
                "did not reach the Wasteland briefing")
        if not g.front_field("briefing_revealed"):
            tap(1)
        for i in range(32):
            page, count = (g.front_field(key) for key in ("briefing_page", "briefing_pages"))
            require(page == i < count <= 32 and g.front_field("briefing_revealed"), "wrong briefing page")
            report["briefing"].append([page, count])
            if page + 1 == count:
                return
            tap(1)
            if not g.front_field("briefing_revealed"):
                tap(1)
        raise ValueError("briefing exceeded its page budget")

    def acknowledge(call):
        for _ in range(48):
            count, page, armed, row = modal()
            if not count:
                return
            require(0 < count <= 13 and not armed and row < 256, "invalid authored dialogue state")
            frame, held = g.u32("frame"), rsp.held
            key = 8 if held & 1 else 1
            advance(held | key)
            require(modal()[:3] == (count, page, key) and g.u32("frame") == frame,
                    "dialogue acknowledgement did not latch")
            advance(held)
            require(g.u32("frame") == frame, "dialogue release advanced the simulation")
            report["dialogue"].append([call, row, page])
        raise ValueError("dialogue exceeded its page budget")

    def world():
        state = g.state()
        del state["screen"]
        return state

    def command():
        result = {k: g.u32(k) for k in COMMAND_FIELDS if g.sizes[k] == 4}
        if g.sizes["menu_last"] == 1:
            result["menu_last"] = rsp.read(g.sym["menu_last"], 1)[0]
        require(result.keys() == set(COMMAND_FIELDS), "unexpected menu state widths")
        return result

    saved = None
    try:
        require(g.u32("screen") == g.u32("frame") == g.front_field("page") == 0, "guest did not reach HOME")
        for tick in range(total):
            if tick == 4:
                reveal_briefing()
            if tick == roundtrip + 2:
                require(g.u32("screen") == 2, "B was not issued from pause")
                saved = world()
                saved_ui = command()
            advance(keys.get(tick))
            if tick == 4:
                require((g.u32("screen"), g.u32("race_selected_mission"), g.u32("race_mode")) == (1, T1, 0),
                        "did not deploy Wasteland")
            if tick == roundtrip + 2:
                require(world() == saved and command() == saved_ui, "save mutated live state")
                rsp.export_sram()
                require(save.stat().st_size == 32768, "incorrect pause journal size")
                (args.out / "pause.sav").write_bytes(save.read_bytes())
            elif tick == roundtrip + 4:
                require(g.u32("screen") == 1 and g.u32("frame") < saved["frame"], "restart did not reset the mission")
            elif tick == roundtrip + 8:
                require(g.u32("screen") == 1 and world() == saved, "pause L did not restore the saved gameplay state")
                loaded = command()
                if args.expect_context:
                    require(saved_ui["menu_last"] != 0 and all(saved_ui[k] == loaded[k] for k in COMMAND_FIELDS[:3]),
                            f"marked journal did not restore the saved command state: {saved_ui}, {loaded}")
                require(loaded["mode"] == loaded["menu_open"] == 0, "load opened the command panel")
                report["roundtrip"] = dict(tick=tick, frame=saved["frame"], saved=saved_ui, loaded=loaded,
                                           pause_sha256=digest(args.out / "pause.sav"))
            acknowledge(tick + 1)
            if roundtrip - 1 <= tick <= roundtrip + 35:
                report["boundary"].append(dict(tick=tick, held=rsp.held, **command(),
                                               **{k: g.u32(k) for k in WORLD_FIELDS}))
            if tick + 1 in points:
                state = checkpoint(g, types, tick + 1)
                report["checkpoints"].append(state)
                differences = {k: (expected, state.get(k)) for k, expected in points[tick + 1].items()
                               if k != "state_hash" and state.get(k) != expected}
                if differences:
                    report["difference"] = differences
                    raise ValueError(f"checkpoint {tick + 1} differs: {differences}")
        require(g.u32("screen") == 3 and report.get("roundtrip") and report["dialogue"],
                "missing Wasteland victory or provenance evidence")
        require(list(map(digest, paths)) == args.pins, "replay inputs changed")
        report["status"] = "won"
    finally:
        report["physical_updates"] = rsp.completed
        g.close()
        (args.out / "result.json").write_text(json.dumps(report, indent=2) + "\n")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for field in ("trace", "rom", "elf", "runner", "out"):
        parser.add_argument(f"--{field}", type=Path, required=True)
    parser.add_argument("--pins", nargs=4, metavar="SHA256", required=True,
                        help="trace, ROM, matching ELF, and physical observer, in that order")
    parser.add_argument("--expect-context", action="store_true", help="require a marked journal to restore the menu state")
    replay(parser.parse_args())


if __name__ == "__main__":
    main()
