#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Whole Custom Game matches on a race cartridge, from the lobby to RESULT.

`author` runs the HOST player (tests/sc64_custom_full_match.c, built from the
cartridge's product vector) and writes its held-key trace. A Blood Bath Melee
start is drawn from the time since power-on, so the lobby keys are first
played on the ROM to read the start order they resolve to, and the player is
then run with that order.

`replay` drives the unchanged ROM with physical buttons only, in the pinned
port-free mGBA backend, and reads guest memory without writing it:

1. Uninterrupted: blank SRAM, HOME, the whole tape. Every recorded check
   (frame, bank, live actors per owner and a hash of every live unit's and
   building's slot, owner, type, position and hit points) must match. The
   tape pauses once during a battle and saves (START, B); the SRAM is
   exported there, and again at RESULT.
2. Continuation: a fresh process loads the pause save with L on HOME. The
   restored world must equal the saved one, and the rest of the tape must
   pass the same checks and reach the same RESULT.
3. Victory record: a fresh process loads the SRAM exported at RESULT. It must
   open the same result, and loading must leave the SRAM unchanged.

`run` does both for one lobby; `matrix` runs a list of lobbies.

Timing is the emulator's guest cycle clock over main-loop updates, including
menus and presentation waits. It is not hardware frame rate.
"""
from __future__ import annotations

import argparse
import concurrent.futures
import hashlib
import json
import os
from pathlib import Path
import shutil
import statistics
import subprocess
import sys
import tempfile

sys.path.insert(0, str(Path(__file__).resolve().parent))
from sc64_skirmish_sram_continuation import (  # noqa: E402
    KEY_L, RACES, SCREEN_PAUSE, SCREEN_PLAY, SCREEN_TITLE, FRONT_HOME, UNIT_FIELDS, PhysicalGame,
    fingerprint,
)

SCREEN_RESULT = 3
OWNERS = (0, 1, 3, 4)
CHECK_KINDS = ("check", "build", "train", "research", "attack", "end")
CHECK_FIELDS = ("frame", "screen", "minerals", "gas", "hash", "owners")
CHECK_INTERVAL = 250  # the author's CHECK_INTERVAL
WORLD_PIXELS = 1024  # both Custom Game maps; the field viewport is 240 x 108
# The author's busy-battle rule (BUSY_* in tests/sc64_custom_full_match.c).
BUSY_OWN, BUSY_HOSTILE, BUSY_UNITS = 6, 5, 16
GBA_HZ = 16777216
PRODUCTS = {
    "terran": ("build/StarCraft-Terran-Transport1-GBA.gba", "build/starcraft-terran.elf"),
    "zerg": ("build/StarCraft-Zerg-Z10-Transport1-GBA.gba", "build/starcraft-zerg-z10.elf"),
    "protoss": ("build/StarCraft-Protoss-P10-Transport1-GBA.gba", "build/starcraft-protoss-p10.elf"),
}
MAP_NAMES = {0: "Outpost", 1: "Blood Bath"}
# PRODUCT:MAP:CPUS[:PLAN] lobbies whose tapes win and replay; see
# docs/audits/2026-11-26-n64-gas-pickup.md.
QUALIFIED: tuple[str, ...] = (
    "terran:0:z:PRODUCERS=3",
    "terran:0:tz:PRODUCERS=2,WORKERS=14",
    "terran:0:zpt:PRODUCERS=3",
    ("terran:1:z:LOBBY_WAIT=8,FIRST_ATTACK=7000,PRODUCERS=3,WORKERS=14,TECH_AFTER=2,"
     "LAB_AFTER=3,ATTACK_MIN=6,ATTACK_RATIO=3,ATTACK_MARGIN=8"),
    "terran:1:pz:LOBBY_WAIT=8,FIRST_ATTACK=7000",
    ("terran:1:zpt:LOBBY_WAIT=8,FIRST_ATTACK=7000,PRODUCERS=3,WORKERS=16,"
     "TECH_AFTER=2,LAB_AFTER=3,ATTACK_MIN=6,ATTACK_RATIO=3,ATTACK_MARGIN=8"),
    ("zerg:0:p:LINE=38,WORKERS=9,DEFENSES=2,PRODUCERS=2,FIRST_ATTACK=7000,"
     "DEFENSE=146,DEFENSE_FRAME=2500,ATTACK_RATIO=1,ATTACK_MIN=6,ATTACK_MARGIN=0"),
    "zerg:0:pz:WORKERS=10,LINE=38",
    "zerg:0:tzp:WORKERS=12,LINE=38",
    "zerg:1:p:LINE=38,WORKERS=12,PRODUCERS=1,FIRST_ATTACK=7000,LOBBY_WAIT=16",
    "zerg:1:tz:LINE=38,WORKERS=12,PRODUCERS=1,FIRST_ATTACK=7000,LOBBY_WAIT=22",
    "protoss:0:t:LINE=66,WORKERS=12,DEFENSES=4,PRODUCERS=2,FIRST_ATTACK=7000,DEFENSE=162",
    "protoss:0:tp:LINE=65,WORKERS=10,DEFENSE=162,DEFENSES=4,PRODUCERS=2,FIRST_ATTACK=7000",
    "protoss:0:ptz:LINE=65,WORKERS=16,DEFENSE=162,DEFENSES=4,PRODUCERS=3,FIRST_ATTACK=7000",
    "protoss:1:t:LINE=66,TECH_FRAME=8000,LOBBY_WAIT=18",
    ("protoss:1:tz:LINE=65,TECH_FRAME=30000,WORKERS=16,PRODUCERS=3,"
     "FIRST_ATTACK=3000,ATTACK_MIN=8,ATTACK_RATIO=1,ATTACK_MARGIN=0,LOBBY_WAIT=4"),
    "protoss:1:tzp:LINE=66,TECH_FRAME=30000,LOBBY_WAIT=18,WORKERS=12,PRODUCERS=3",
)
# (product, map, CPU count) cells with no qualified lobby, and why; see
# docs/audits/2026-11-25-custom-larva-eggs.md#custom-game-full-matches.
OPEN_CELLS: dict[tuple[str, int, int], str] = {
    ("zerg", 1, 3): "with Larva and Eggs no HOST plan for zzz, tpt, tpp, ttt or ppt has won",
}


class MatchError(RuntimeError):
    pass


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def load_trace(path: Path) -> dict:
    """Validate the author's trace and index it by completed update."""
    raw = json.loads(path.read_text())
    if raw.get("profile") != "custom-full-match" or not isinstance(raw.get("events"), list):
        raise MatchError("not a custom-full-match trace")
    terminal = raw.get("terminal")
    if not isinstance(terminal, dict) or terminal.get("kind") not in ("won", "lost", "limit"):
        raise MatchError("trace has no terminal record")
    inputs, checks, start, save, resume = [], {}, None, None, None
    last_tick = last_input = 0
    for index, row in enumerate(raw["events"]):
        kind, tick = row.get("kind"), row.get("host_tick")
        if type(tick) is not int or tick < last_tick or tick < 1:
            raise MatchError(f"event {index} has an invalid update number")
        last_tick = tick
        if kind == "input":
            held = row.get("held")
            if (type(held) is not int or not 0 <= held <= 1023 or held & 48 == 48 or
                    held & 192 == 192 or tick <= last_input):
                raise MatchError(f"event {index} is not a valid key transition")
            last_input = tick
            inputs.append((tick - 1, held))
        elif kind in CHECK_KINDS:
            state = check_state(row, index)
            if tick in checks and check_state(checks[tick], index) != state:
                raise MatchError(f"event {index} contradicts an earlier record of update {tick}")
            checks[tick] = row
        elif kind == "start" and start is None:
            start = row
        elif kind == "plan":
            pass  # the author's settings, kept for the record
        elif kind == "save" and save is None:
            save = row
        elif kind == "resume" and resume is None:
            resume = row
        else:
            raise MatchError(f"event {index} has unexpected kind {kind!r}")
    end = terminal.get("host_tick")
    if start is None or type(end) is not int or end not in checks or checks[end]["kind"] != "end":
        raise MatchError("trace lacks its start or end record")
    if raw.get("check_interval") != CHECK_INTERVAL:
        raise MatchError(f"trace does not check every {CHECK_INTERVAL} updates")
    missing = [tick for tick in range(CHECK_INTERVAL, end + 1, CHECK_INTERVAL) if tick not in checks]
    if missing:
        raise MatchError(f"trace lacks the periodic check of update {missing[0]}")
    if (save is None) != (resume is None) or (save and not save["host_tick"] + 7 == resume["host_tick"]):
        raise MatchError("trace has an incomplete pause save")
    if save is not None:
        for row, keys in ((save, ("frame", "hash", "visible_own", "visible_hostile")), (resume, ("frame", "hash"))):
            if any(type(row.get(key)) is not int for key in keys):
                raise MatchError("the pause save record is incomplete")
        cursor = save.get("cursor")
        if type(cursor) is not list or len(cursor) != 2 or any(type(v) is not int for v in cursor):
            raise MatchError("the pause save record has no cursor")
    bots = start.get("bots")
    if (any(type(start.get(key)) is not int for key in ("map", "human_race", "melee", "start_order")) or
            type(bots) is not list or not 1 <= len(bots) <= 3 or
            any(type(bot) is not dict or type(bot.get("owner")) is not int or type(bot.get("race")) is not int
                for bot in bots)):
        raise MatchError("trace has an incomplete start record")
    flags = (terminal.get("won"), terminal.get("lost"))
    if flags != {"won": (1, 0), "lost": (0, 1), "limit": (0, 0)}[terminal["kind"]] or terminal.get("frame") != checks[end]["frame"]:
        raise MatchError("trace terminal record contradicts itself")
    if save is not None:
        continuation_tail(inputs, save["host_tick"])  # the pause keys
    return {"raw": raw, "inputs": inputs, "checks": checks, "start": start, "save": save,
            "resume": resume, "terminal": terminal, "end": end}


def check_state(row: dict, index: int) -> tuple:
    """The compared fields of a check record; all of them are required."""
    owners = row.get("owners")
    if (any(type(row.get(key)) is not int for key in CHECK_FIELDS[:-1]) or type(owners) is not list or
            len(owners) != len(OWNERS) or
            any(type(pair) is not list or len(pair) != 2 or any(type(v) is not int for v in pair)
                for pair in owners)):
        raise MatchError(f"event {index} is not a complete check record")
    return tuple(row[key] for key in CHECK_FIELDS[:-1]) + (tuple(map(tuple, owners)),)


def world_hash(state: dict) -> int:
    """The author's world_hash(): FNV-1a over live actors, bank and frame."""
    h = 2166136261
    def mix(value: int) -> None:
        nonlocal h
        h = ((h ^ (value & 0xFFFFFFFF)) * 16777619) & 0xFFFFFFFF
    for rows in (state["units"], state["buildings"]):
        for row in rows:
            for value in row[:6]:  # slot, owner, type, x, y, hp
                mix(value)
    mix(state["minerals"]); mix(state["gas"]); mix(state["frame"])
    return h


def observe(game: PhysicalGame) -> dict:
    state = game.state()
    owners = [[sum(1 for row in state[key] if row[1] == owner) for key in ("units", "buildings")]
              for owner in OWNERS]
    return {"frame": state["frame"], "screen": state["screen"], "minerals": state["minerals"],
            "gas": state["gas"], "hash": world_hash(state), "owners": owners,
            "won": game.u32("won"), "lost": game.u32("lost"),
            "fingerprint": fingerprint({k: v for k, v in state.items() if k != "screen"})}


def compare(expected: dict, actual: dict, keys: tuple = ()) -> list[str]:
    """Differences in every check field (or in keys, for a save or resume record)."""
    return [f"{key}={actual[key]} expected {expected[key]}"
            for key in (keys or CHECK_FIELDS) if actual[key] != expected[key]]


def require_home(game: PhysicalGame) -> None:
    if (game.u32("screen"), game.u32("frame"), game.front_field("page")) != (SCREEN_TITLE, 0, FRONT_HOME):
        raise MatchError("fresh boot is not on HOME")


def export_sram(game: PhysicalGame, scratch: Path, target: Path) -> bytes:
    """The backend creates its export file once; move each export away."""
    game.rsp.export_sram()
    scratch.replace(target)
    data = target.read_bytes()
    if len(data) != 32768:
        raise MatchError("exported SRAM is not 32768 bytes")
    return data


def lobby(game: PhysicalGame, trace: dict) -> dict:
    start = trace["start"]
    seats = {"map": game.u32("race_skirmish_map"), "human_race": game.u32("race_human"),
             "melee": game.u32("race_skirmish_melee"),
             "start_order": game.u32("race_skirmish_start_order"),
             "bots": [{"owner": game.u32("race_bot_owner_ids", 4 * i), "race": game.u32("race_bot_races", 4 * i)}
                      for i in range(game.u32("race_bots"))]}
    if game.u32("race_mode") != 1 or any(seats[key] != start[key] for key in seats):
        raise MatchError(f"lobby {seats} does not match the trace start {start}")
    return seats


def play_checks(game: PhysicalGame, trace: dict, inputs: list, shift: int, first: int,
                stops: dict, label: str) -> list[dict]:
    """Play to each recorded check after update `first`; stops maps an update to a callback."""
    rows = []
    ticks = sorted({tick for tick in trace["checks"] if tick > first} | {t for t in stops if t > first})
    for tick in ticks:
        game.rsp.play_to(tick - shift, inputs)
        if tick in trace["checks"]:
            expected, actual = trace["checks"][tick], observe(game)
            problems = compare(expected, actual)
            if problems:
                raise MatchError(f"{label}: update {tick} ({expected['kind']}): " + "; ".join(problems))
            rows.append({"host_tick": tick, "kind": expected["kind"], "frame": actual["frame"],
                         "hash": actual["hash"], "fingerprint": actual["fingerprint"]})
        if tick in stops:
            stops[tick]()
    return rows


def transported_order(elf: Path) -> int | None:
    """ORDER_TRANSPORTED from DWARF, or None when the product has no transports."""
    info = subprocess.run(["arm-none-eabi-readelf", "--debug-dump=info", str(elf)],
                          capture_output=True, text=True, check=True).stdout.splitlines()
    for index, line in enumerate(info):
        if line.rstrip().endswith(": ORDER_TRANSPORTED"):
            for follow in info[index + 1:index + 4]:
                if "DW_AT_const_value" in follow:
                    return int(follow.split(":")[-1])
            raise MatchError("ORDER_TRANSPORTED has no value in the ELF")
    return None


def viewport_counts(game: PhysicalGame, hostile_owners: set, loaded: int | None) -> dict:
    """Units inside the field viewport that the guest cursor implies, as the
    author counts them: units carried inside a transport are left out."""
    cx, cy = game.u32("cx"), game.u32("cy")
    left = min(max(cx - 120, 0), WORLD_PIXELS - 240) & ~3
    top = min(max(cy - 54, 0), WORLD_PIXELS - 108)
    order = 1 + UNIT_FIELDS.index("order")
    inside = [row for row in game.state()["units"]
              if left <= row[3] < left + 240 and top <= row[4] < top + 108 and row[order] != loaded]
    return {"cursor": [cx, cy], "own": sum(1 for row in inside if row[1] == 0),
            "hostile": sum(1 for row in inside if row[1] in hostile_owners)}


def continuation_tail(inputs: list, save_tick: int) -> tuple[int, list]:
    """The tape after the pause, renumbered for a process that loads the save.

    The uninterrupted tape saves with B at update S, resumes with A at S+2
    and the world next advances at S+3. A fresh process spends two neutral
    updates on HOME and loads with L on the third, so its loaded world next
    advances at its fourth update: uninterrupted update n is its n-(S-1).
    Inputs are (zero-based update, held keys) transitions.
    """
    shift = save_tick - 1
    pause = [(tick + 1, held) for tick, held in inputs if save_tick - 2 <= tick + 1 <= save_tick + 3]
    if pause != [(save_tick - 2, 8), (save_tick - 1, 0), (save_tick, 2), (save_tick + 1, 0),
                 (save_tick + 2, 1), (save_tick + 3, 0)]:
        raise MatchError("the tape does not pause with START, save with B and resume with A")
    tail = [(tick - shift, held) for tick, held in inputs if tick + 1 >= save_tick + 3]
    if any(held for tick, held in tail if tick < 7):
        raise MatchError("the tape presses a key before the resumed world settles")
    return shift, tail


def at_resume(game: PhysicalGame, trace: dict, label: str) -> None:
    problems = compare(trace["resume"], observe(game), ("frame", "hash"))
    if problems or game.u32("screen") != SCREEN_PLAY:
        raise MatchError(f"{label}: the resumed world differs from the trace: " + "; ".join(problems))


def displayed_bitmap(game: PhysicalGame) -> bytes:
    control = int.from_bytes(game.rsp.read(0x04000000, 2), "little")
    if (control & 7) != 4 or control & 0x80 or not control & 0x0400:
        raise MatchError("visible-page sampling requires an active Mode 4 bitmap")
    page = 0x0600A000 if control & 16 else 0x06000000
    pixels = game.rsp.read(page, 240 * 160)
    palette = game.rsp.read(0x05000000, 512)
    # Resolve indices so unused palette edits and duplicate colors do not count.
    low, high = palette[::2], bytes(value & 0x7f for value in palette[1::2])
    return hashlib.sha256(pixels.translate(low) + pixels.translate(high)).digest()


def pace(args) -> dict:
    """Time the recorded controls immediately after a qualified battle save."""
    trace = load_trace(args.trace)
    save = trace["save"]
    if not save or args.updates < 1:
        raise MatchError("pacing requires a recorded battle save and a positive update count")
    first = trace["resume"]["host_tick"]
    last = first + args.updates
    if last >= trace["end"]:
        raise MatchError("pacing window reaches the end of the match")
    if not any(first < tick <= last for tick in trace["checks"]):
        raise MatchError("pacing window contains no recorded gameplay checkpoint")
    shift, inputs = continuation_tail(trace["inputs"], save["host_tick"])
    counters = ("demo_update_cycles", "gfx_draw_cycles", "sc64_orders_cycles", "sc64_sight_cycles",
                "sc64_other_cycles", "sc64_background_cycles", "sc64_unit_draw_cycles", "sc64_hud_cycles")
    if args.output.exists():
        raise MatchError(f"{args.output} already exists")
    args.output.mkdir(parents=True, exist_ok=False)
    game = PhysicalGame(args.runner, args.rom, args.elf, args.output / "guest", save_input=args.save)
    try:
        require_home(game)
        game.step(0, 2)
        game.step(KEY_L)
        if game.u32("screen") != SCREEN_PLAY or compare(save, observe(game), ("frame", "hash")):
            raise MatchError("loaded battle differs from the recorded save")
        seats = lobby(game, trace)
        game.rsp.play_to(first - shift, inputs)
        at_resume(game, trace, "pacing")
        clock = game.rsp.clock()
        rows, checks = [], 0
        previous_bitmap = displayed_bitmap(game) if args.video else None
        world_frame = game.u32("frame")
        for tick in range(first + 1, last + 1):
            game.rsp.play_to(tick - shift, inputs)
            now = game.rsp.clock()
            next_frame = game.u32("frame")
            if next_frame - world_frame not in (0, 1) or now <= clock:
                raise MatchError(f"invalid game clock at update {tick}")
            rows.append({"host_tick": tick, "cycles": now - clock, "frame": next_frame,
                         "world_advanced": next_frame != world_frame,
                         **{name: game.u32(name) for name in counters}})
            if args.video:
                bitmap = displayed_bitmap(game)
                rows[-1]["bitmap_changed"] = bitmap != previous_bitmap
                previous_bitmap = bitmap
            world_frame = next_frame
            clock = now
            if tick in trace["checks"]:
                problems = compare(trace["checks"][tick], observe(game))
                if problems:
                    raise MatchError(f"pacing diverged at {tick}: " + "; ".join(problems))
                checks += 1
        final = observe(game)
        if final["screen"] != SCREEN_PLAY:
            raise MatchError("match left play during the pacing window")
    finally:
        game.close()
    cycles = sorted(row["cycles"] for row in rows)
    # Simulation stage timers keep their previous values on a command-menu tick.
    active = [row for row in rows if row["world_advanced"]]
    if not active:
        raise MatchError("the pacing window contains no simulation updates")
    averages = {name: statistics.mean(row[name] for row in active) for name in counters}
    result = {"ok": True, "lobby": seats, "checks": checks, "updates": len(rows),
              "updates_per_second": GBA_HZ / statistics.mean(cycles),
              "p95_cycles": cycles[(95 * len(rows) + 99) // 100 - 1],
              "over_two_fields": sum(c > 561792 for c in cycles),
              "active_updates": len(active), "mean_active_stage_cycles": averages, "end_fingerprint": final["fingerprint"],
              "sha256": {k: sha256(getattr(args, k)) for k in ("rom", "elf", "runner", "save", "trace")},
              "samples": rows}
    if args.video:
        changes = sum(row["bitmap_changed"] for row in rows)
        result["bitmap_changes"] = changes
        result["bitmap_changes_per_second"] = changes * GBA_HZ / sum(cycles)
    (args.output / "pacing.json").write_text(json.dumps(result, indent=1) + "\n")
    return result


def replay(args) -> dict:
    trace = load_trace(args.trace)
    terminal, save, end = trace["terminal"], trace["save"], trace["end"]
    if terminal["kind"] != "won":
        raise MatchError(f"trace ends in {terminal['kind']}, not a player win")
    if save is None:
        raise MatchError("trace has no pause save")
    out = args.output
    if out.exists():
        raise MatchError(f"{out} already exists")
    out.mkdir(parents=True)
    paths = (args.trace, args.rom, args.elf, args.runner)
    record = {"sha256": {path.name: sha256(path) for path in paths},
              "map": trace["raw"]["map"], "human": trace["raw"]["human"], "cpus": trace["raw"]["cpus"],
              "transport": "physical-buttons", "input_transitions": len(trace["inputs"]),
              "checks": len(trace["checks"]), "actions": terminal.get("actions")}
    S = save["host_tick"]
    loaded = transported_order(args.elf)
    pause_sram, victory_sram = out / "pause.sav", out / "victory.sav"
    notes = {}

    # 1. Uninterrupted, with the pause save and the RESULT save exported.
    game = PhysicalGame(args.runner, args.rom, args.elf, out / "full", save_output=out / "export.sav")
    try:
        require_home(game)
        clock = game.rsp.clock()
        def at_start():
            notes["lobby"] = lobby(game, trace)
            if game.u32("screen") != SCREEN_PLAY:
                raise MatchError("the lobby keys did not launch the match")
        def at_save():
            actual = observe(game)
            if actual["screen"] != SCREEN_PAUSE or compare(save, actual, ("frame", "hash")):
                raise MatchError(f"pause save state differs from the trace: {actual} vs {save}")
            # The guest cursor fixes the viewport; count the battle in it again.
            view = viewport_counts(game, {bot["owner"] for bot in notes["lobby"]["bots"]}, loaded)
            if (view["cursor"] != save["cursor"] or view["own"] != save["visible_own"] or
                    view["hostile"] != save["visible_hostile"] or view["own"] < BUSY_OWN or
                    view["hostile"] < BUSY_HOSTILE or view["own"] + view["hostile"] < BUSY_UNITS):
                raise MatchError(f"the saved view {view} does not hold the battle the trace recorded")
            notes["view"] = view
            notes["saved"] = actual
            notes["pause_bytes"] = export_sram(game, out / "export.sav", pause_sram)
            notes["save_clock"] = game.rsp.clock()
        rows = play_checks(game, trace, trace["inputs"], 0, 0,
                           {trace["start"]["host_tick"]: at_start, S: at_save,
                            trace["resume"]["host_tick"]: lambda: at_resume(game, trace, "uninterrupted")},
                           "uninterrupted")
        final = observe(game)
        if (final["screen"], final["won"], final["lost"]) != (SCREEN_RESULT, 1, 0):
            raise MatchError(f"uninterrupted run did not end in a win: {final}")
        cycles = game.rsp.clock() - clock
        notes["victory_bytes"] = export_sram(game, out / "export.sav", victory_sram)
    finally:
        game.close()
    after_save = [row for row in rows if row["host_tick"] > S]
    if notes["victory_bytes"] == notes["pause_bytes"]:
        raise MatchError("RESULT did not write a victory save")
    record.update(lobby=notes["lobby"], result="won", updates=end, frame=final["frame"],
                  owners=final["owners"], bank=[final["minerals"], final["gas"]],
                  uninterrupted={"checks": len(rows), "guest_cycles": cycles,
                                 "mean_cycles_per_update": cycles / end,
                                 "updates_per_second": GBA_HZ * end / cycles,
                                 "after_save_updates_per_second":
                                     GBA_HZ * (end - S) / (cycles - (notes["save_clock"] - clock)),
                                 "fingerprint": final["fingerprint"]},
                  save={"host_tick": S, "frame": save["frame"], "visible_own": save["visible_own"],
                        "visible_hostile": save["visible_hostile"], "cursor": save["cursor"],
                        "guest_view": notes["view"],
                        "fingerprint": notes["saved"]["fingerprint"],
                        "sram_sha256": hashlib.sha256(notes["pause_bytes"]).hexdigest()})

    # 2. Fresh process: load the pause save and play the rest of the tape.
    shift, tail = continuation_tail(trace["inputs"], S)
    game = PhysicalGame(args.runner, args.rom, args.elf, out / "continued",
                        save_input=pause_sram, save_output=out / "export.sav")
    try:
        require_home(game)
        game.step(0, 2)
        game.step(KEY_L)
        restored = observe(game)
        if restored["screen"] != SCREEN_PLAY or game.u32("race_mode") != 1:
            raise MatchError("the pause save did not load from HOME")
        if restored["frame"] != save["frame"] or restored["fingerprint"] != notes["saved"]["fingerprint"]:
            raise MatchError("the loaded world differs from the saved one")
        lobby(game, trace)
        resumed = play_checks(game, trace, tail, shift, S + 2,
                              {trace["resume"]["host_tick"]: lambda: at_resume(game, trace, "continuation")},
                              "continuation")
        final2 = observe(game)
        if (final2["screen"], final2["won"], final2["lost"]) != (SCREEN_RESULT, 1, 0):
            raise MatchError(f"continuation did not end in a win: {final2}")
        continued_victory = export_sram(game, out / "export.sav", out / "victory-continued.sav")
    finally:
        game.close()
    expected_tail = [row for row in after_save if row["host_tick"] > S + 2]
    if resumed != expected_tail or final2["fingerprint"] != final["fingerprint"]:
        raise MatchError("continuation checks differ from the uninterrupted run")
    if continued_victory != notes["victory_bytes"]:
        raise MatchError("the continued run wrote a different victory save")
    record["continuation"] = {"loaded": True, "restored_matches_saved": True, "checks": len(resumed),
                              "same_result": True, "fingerprint": final2["fingerprint"],
                              "victory_sram_identical": continued_victory == notes["victory_bytes"]}

    # 3. Fresh process: load the SRAM written at RESULT.
    game = PhysicalGame(args.runner, args.rom, args.elf, out / "victory",
                        save_input=victory_sram, save_output=out / "export.sav")
    try:
        require_home(game)
        game.step(0, 2)
        game.step(KEY_L)
        shown = observe(game)
        reloaded = export_sram(game, out / "export.sav", out / "victory-reloaded.sav")
    finally:
        game.close()
    record["victory_reload"] = {
        "screen": shown["screen"], "won": shown["won"], "lost": shown["lost"], "frame": shown["frame"],
        "owners": shown["owners"], "state_matches_result": shown["fingerprint"] == final["fingerprint"],
        "sram_unchanged_by_load": reloaded == notes["victory_bytes"],
        "sram_sha256": hashlib.sha256(notes["victory_bytes"]).hexdigest()}
    victory = record["victory_reload"]
    record["ok"] = bool(victory["screen"] == SCREEN_RESULT and victory["won"] == 1 and victory["lost"] == 0 and
                        victory["frame"] == final["frame"] and victory["owners"] == final["owners"] and
                        victory["state_matches_result"] and victory["sram_unchanged_by_load"])
    if {path.name: sha256(path) for path in paths} != record["sha256"]:
        raise MatchError("an input changed during the replay")
    for home in out.glob("*/home"):
        shutil.rmtree(home)
    (out / "result.json").write_text(json.dumps(record, indent=1) + "\n")
    return record


def parse_plan(text: str) -> dict:
    """KEY=VALUE,... settings of the HOST player, e.g. FIRST_ATTACK=4500,ATTACK_MIN=10."""
    plan = {}
    for item in filter(None, (text or "").split(",")):
        key, _, value = item.partition("=")
        if not key.isupper() or not key.replace("_", "").isalpha() or not value.lstrip("-").isdigit():
            raise MatchError(f"invalid plan setting {item!r}")
        plan[key] = int(value)
    return plan


def run_author(controller: Path, map_id: int, human: str, cpus: str, trace: Path, limit: int,
               order: int = 0, save: bool = True, plan: dict | None = None) -> int:
    env = {key: value for key, value in os.environ.items() if not key.startswith("SC64_CFM_")}
    for key, value in (plan or {}).items():
        env["SC64_CFM_" + key] = str(value)
    if order:
        env["SC64_CFM_START_ORDER"] = str(order)
    if not save:
        env["SC64_CFM_SAVE"] = "0"
    with trace.with_suffix(".log").open("w") as log:
        return subprocess.run([str(controller), str(map_id), human, cpus, str(trace), str(limit)],
                              stderr=log, env=env, check=False).returncode


def probe_start_order(args, lobby_trace: Path, out: Path) -> int:
    """Play only the lobby keys on the ROM and read the start order it resolves."""
    if not lobby_trace.is_file():
        raise MatchError("the HOST player wrote no lobby trace")
    raw = json.loads(lobby_trace.read_text())
    start = next((row for row in raw["events"] if row["kind"] == "start"), None)
    if start is None:
        raise MatchError("the HOST player did not launch the lobby")
    inputs = [(row["host_tick"] - 1, row["held"]) for row in raw["events"]
              if row["kind"] == "input" and row["host_tick"] <= start["host_tick"]]
    game = PhysicalGame(args.runner, args.rom, args.elf, out)
    try:
        require_home(game)
        game.rsp.play_to(start["host_tick"], inputs)
        if game.u32("screen") != SCREEN_PLAY or game.u32("race_skirmish_melee") != 1:
            raise MatchError("the lobby keys did not launch a Melee match on the ROM")
        order = game.u32("race_skirmish_start_order")
    finally:
        game.close()
    shutil.rmtree(out, ignore_errors=True)
    if not 1 <= order <= 255:
        raise MatchError("the ROM resolved no start order")
    return order


def author(args) -> dict:
    out = args.output
    out.mkdir(parents=True, exist_ok=True)
    trace, order, plan = out / "trace.json", 0, parse_plan(args.plan)
    if args.map == 1:
        lobby_trace = out / "lobby.json"
        lobby_trace.unlink(missing_ok=True)
        # The wait on the Launch row is part of the lobby keys that pick the draw.
        run_author(args.controller, args.map, args.human, args.cpus, lobby_trace,
                   200 + max(0, plan.get("LOBBY_WAIT", 0)), save=False, plan=plan)
        scratch = Path(tempfile.mkdtemp(prefix="lobby-probe-", dir=out))
        order = probe_start_order(args, lobby_trace, scratch)
    trace.unlink(missing_ok=True)
    code = run_author(args.controller, args.map, args.human, args.cpus, trace, args.limit, order, plan=plan)
    if not trace.is_file():
        raise MatchError(f"the HOST player wrote no trace (exit {code})")
    try:
        terminal = load_trace(trace)["terminal"]
    except (ValueError, KeyError, TypeError, AttributeError) as error:
        raise MatchError(f"the HOST player wrote a malformed trace: {error}") from error
    if {0: "won", 3: "lost", 4: "limit"}.get(code) != terminal["kind"]:
        raise MatchError(f"the HOST player exited {code} with a {terminal['kind']} trace")
    return {"trace": str(trace), "trace_sha256": sha256(trace), "start_order": order, "plan": plan,
            "exit": code, "terminal": terminal}


def run(args) -> dict:
    authored = author(args)
    if authored["terminal"]["kind"] != "won":
        return {"ok": False, "authored": authored}
    args.trace, args.output = args.output / "trace.json", args.output / "replay"
    record = replay(args)
    record["authored"] = authored
    (args.output / "result.json").write_text(json.dumps(record, indent=1) + "\n")
    return record


def matrix_case(job: tuple) -> dict:
    root, runner, product, map_id, human, cpus, plan, limit, out = job
    rom, elf = (root / path for path in PRODUCTS[product])
    args = argparse.Namespace(controller=root / f"build/test-sc64-custom-full-match-{product}",
                              rom=rom, elf=elf, runner=runner, map=map_id, human=human, cpus=cpus,
                              plan=plan, limit=limit, output=out)
    row = {"product": product, "map": map_id, "human": human, "cpus": cpus, "plan": plan}
    try:
        record = run(args)
        row.update(ok=record.get("ok", False), result=record.get("result"),
                   terminal=record["authored"]["terminal"], updates=record.get("updates"))
    except (MatchError, OSError, RuntimeError, ValueError, KeyError, TypeError) as error:
        row.update(ok=False, error=f"{type(error).__name__}: {error}")
    return row


def matrix(args) -> list[dict]:
    root = Path(__file__).resolve().parent.parent
    jobs = []
    cases = args.cases or QUALIFIED
    if not cases:
        raise MatchError("no cases to run")
    for case in cases:
        parts = case.split(":")
        product, map_id, cpus, plan = (parts + [""])[:4]
        if (len(parts) not in (3, 4) or product not in PRODUCTS or map_id not in ("0", "1") or
                not 1 <= len(cpus) <= 3 or any(c not in RACES for c in cpus)):
            raise MatchError(f"invalid case {case!r}")
        parse_plan(plan)
        human = product[0]
        out = args.output / f"{product}-{MAP_NAMES[int(map_id)].lower().replace(' ', '-')}-{cpus}"
        if out.exists() or any(job[-1] == out for job in jobs):
            raise MatchError(f"{out} already exists or is listed twice")
        jobs.append((root, args.runner, product, int(map_id), human, cpus, plan, args.limit, out))
    args.output.mkdir(parents=True, exist_ok=True)
    with concurrent.futures.ProcessPoolExecutor(max_workers=args.jobs) as pool:
        rows = list(pool.map(matrix_case, jobs))
    (args.output / "matrix.json").write_text(json.dumps(rows, indent=1) + "\n")
    return rows


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    def path(value):
        return Path(value).resolve()
    def common(sub, with_lobby):
        if with_lobby:
            sub.add_argument("--controller", required=True, type=path, help="HOST author for this cartridge")
            sub.add_argument("--map", type=int, choices=(0, 1), required=True)
            sub.add_argument("--human", choices=tuple(RACES), required=True)
            sub.add_argument("--cpus", required=True, help="one to three of t, z, p in seat order")
            sub.add_argument("--limit", type=int, default=400000)
            sub.add_argument("--plan", default="", help="HOST player settings, KEY=VALUE,...")
        for name in ("rom", "elf", "runner", "output"):
            sub.add_argument("--" + name, required=True, type=path)
    common(commands.add_parser("author", help="write a trace with the HOST player"), True)
    sub = commands.add_parser("replay", help="replay a trace on the ROM with save and reload checks")
    sub.add_argument("--trace", required=True, type=path)
    common(sub, False)
    sub = commands.add_parser("pace", help="measure guest cycles while continuing a recorded battle")
    sub.add_argument("--trace", required=True, type=path)
    sub.add_argument("--save", required=True, type=path)
    sub.add_argument("--updates", type=int, default=256)
    sub.add_argument("--video", action="store_true", help="sample visible Mode 4 page and palette at update boundaries")
    common(sub, False)
    common(commands.add_parser("run", help="author, then replay"), True)
    sub = commands.add_parser("matrix", help="run PRODUCT:MAP:CPUS[:PLAN] cases (default: the qualified list)")
    sub.add_argument("cases", nargs="*")
    sub.add_argument("--runner", required=True, type=path)
    sub.add_argument("--output", required=True, type=path)
    sub.add_argument("--limit", type=int, default=400000)
    sub.add_argument("--jobs", type=int, default=3)
    args = parser.parse_args(argv)
    if args.command in ("author", "run") and (
            not 1 <= len(args.cpus) <= 3 or any(c not in RACES for c in args.cpus)):
        parser.error("--cpus takes one to three of t, z, p")
    try:
        if args.command == "matrix":
            rows = matrix(args)
            for row in rows:
                print(json.dumps(row))
            return 0 if all(row["ok"] for row in rows) else 1
        record = {"author": author, "replay": replay, "pace": pace, "run": run}[args.command](args)
    except (MatchError, OSError, RuntimeError, ValueError) as error:
        parser.exit(1, f"{error}\n")
    summary = {key: record[key] for key in ("ok", "result", "updates", "frame", "save", "continuation",
                                            "victory_reload", "terminal", "start_order", "exit", "updates_per_second")
               if key in record}
    print(json.dumps(summary))
    return 0 if record.get("ok", args.command == "author") else 1


if __name__ == "__main__":
    sys.exit(main())
