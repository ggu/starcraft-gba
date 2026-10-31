#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Fresh-process SRAM continuation of a canonical custom game on ARM.

Process 1 starts the unchanged product ROM with an empty save in the pinned
port-free mGBA backend. Normal menu inputs choose the map, the human race and
the opponents; the human then stays passive for --play updates while the
opponents build. START then B saves through the pause menu, and the saved
world's fingerprint is recorded. A resumes play for --continue updates (the
uninterrupted reference), and the physical 32 KiB SRAM is exported.

Process 2 starts the same ROM with that SRAM. L on HOME loads it through the
game's own loader. The restored fingerprint must equal the saved one, and
--continue neutral updates must reproduce the reference. Only scheduled
controller inputs are written; game state is read, never written.

The fingerprint covers frame, owner banks, mineral fields and the semantic
selected persisted fields of every live unit and building, including active
production queues, read at their DWARF offsets and widths. It is not a complete
save-state comparison; transient UI, path caches, audio and render state are outside it.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
import sc64_races_profile as races  # noqa: E402
from emulator_smoke import symbol_sizes, symbols  # noqa: E402

SCREEN_TITLE, SCREEN_PLAY, SCREEN_PAUSE = 0, 1, 2
FRONT_HOME, FRONT_SKIRMISH = 0, 4
KEY_A, KEY_B, KEY_START, KEY_RIGHT, KEY_UP, KEY_DOWN, KEY_L = 1, 2, 8, 16, 64, 128, 512
SRAM_BYTES = 32768
UNIT_FIELDS = ("owner", "type", "x", "y", "hp", "shield", "order", "tx", "ty", "carry",
               "target", "work_timer", "raw_type", "raw_owner", "cd", "build", "flags", "shield_remainder")
BUILDING_FIELDS = ("owner", "type", "x", "y", "hp", "shield", "under_construction",
                   "build_progress", "queue_count", "queue_head", "queue_progress",
                   "raw_type", "raw_owner", "larva", "level", "flags", "larva_timer", "shield_remainder")
RACES = "tzp"


class ContinuationError(RuntimeError):
    pass


def entity_rows(data: bytes, stride: int, layout, fields: tuple[str, ...], *,
                active_queue: bool = False) -> list[tuple]:
    """Live rows as tuples of the named fields, in slot order."""
    if not stride or len(data) % stride:
        raise ContinuationError("entity bank size does not match its DWARF stride")
    widths = layout.widths
    for name in ("alive",) + fields:
        if name not in layout or widths.get(name) not in (1, 2, 4):
            raise ContinuationError(f"DWARF layout lacks scalar field {name}")
        if type(layout[name]) is not int or layout[name] < 0 or layout[name] + widths[name] > stride:
            raise ContinuationError(f"field {name} exceeds the DWARF stride")
    if active_queue:
        queue = getattr(layout, "arrays", {}).get("queue")
        if ("queue" not in layout or type(layout["queue"]) is not int or layout["queue"] < 0 or
                type(queue) is not tuple or len(queue) != 3 or type(queue[0]) is not int or
                queue[0] != 5 or type(queue[1]) is not int or queue[1] not in (1, 2, 4) or
                type(queue[2]) is not bool or layout["queue"] + queue[0] * queue[1] > stride or
                "queue_head" not in fields or "queue_count" not in fields):
            raise ContinuationError("DWARF layout lacks a valid five-element queue array")
    rows = []
    for slot in range(len(data) // stride):
        row = data[slot * stride:(slot + 1) * stride]
        def get(name, signed=True):
            o, w = layout[name], widths[name]
            return int.from_bytes(row[o:o + w], "little", signed=signed and w > 1)
        if not get("alive", False):
            continue
        values = (slot,) + tuple(get(name) for name in fields)
        if active_queue:
            count, head = get("queue_count"), get("queue_head")
            if not 0 <= count <= 5 or not 0 <= head < 5:
                raise ContinuationError("invalid live building queue count or head")
            width, signed = queue[1:]
            start = layout["queue"]
            active = tuple(int.from_bytes(row[start + ((head + i) % 5) * width:
                                              start + ((head + i) % 5 + 1) * width],
                                          "little", signed=signed) for i in range(count))
            values += (active,)
        rows.append(values)
    return rows


def fingerprint(state: dict) -> str:
    """SHA-256 over a canonical JSON encoding of the semantic state."""
    blob = json.dumps(state, sort_keys=True, separators=(",", ":")).encode()
    return hashlib.sha256(blob).hexdigest()


class Game:
    """One owned headless backend process driving the product ROM."""

    def __init__(self, runner: Path, rom: Path, elf: Path, out: Path, *,
                 save_input: Path | None = None, save_output: Path | None = None):
        import sc64_headless_replay as headless
        self.sym, self.sizes = symbols(elf), symbol_sizes(elf)
        for name in ("game_update", "demo_schedule_events", "demo_schedule_count", "demo_schedule_length",
                     "demo_schedule_tick", "demo_schedule_active", "demo_schedule_index", "screen",
                     "frame", "race_mode", "race_front", "race_human", "race_bots", "race_skirmish_map",
                     "u", "b", "m", "minerals", "gas", "race_minerals", "race_gas"):
            if name not in self.sym:
                raise ContinuationError(f"ELF lacks symbol {name}")
        self.unit_stride, self.unit_layout = races.dwarf_struct_layout(elf, "Unit")
        building = "Building"
        if self.sizes["b"] == 160 * 48:
            building = "Sc64ZpCompactBuilding"
        self.building_stride, self.building_layout = races.dwarf_struct_layout(elf, building)
        import sc64_split_smoke as split
        self.front = split.frontend_offsets(elf)
        out.mkdir(parents=True, exist_ok=True)
        self.proc, self.rsp, self.log = headless.start(runner, rom, out, self.sym, self.sizes,
                                                        save_input=save_input, save_output=save_output)
        # Stop after each game_update, at audio_update: the next update's
        # keys are still unlatched there, so a step runs exactly one update
        # with the keys it names and its state is read afterwards.
        if "audio_update" not in self.sym:
            raise ContinuationError("ELF lacks symbol audio_update")
        self.rsp.breakpoint(self.sym["game_update"] & ~1, False)
        self.update = self.sym["audio_update"] & ~1
        self.rsp.breakpoint(self.update, True)
        self.rsp.tick(expected=self.update)
        self.keys = 0
        self.rsp.write(self.sym["demo_schedule_count"], struct.pack("<I", 0))
        self.rsp.write(self.sym["demo_schedule_index"], struct.pack("<I", 0))
        self.rsp.write(self.sym["demo_schedule_length"], struct.pack("<I", 0x7fffffff))
        self.rsp.write(self.sym["demo_schedule_active"], struct.pack("<I", 1))
        self.updates = 0
        self.stale_update_stops = 0

    def u32(self, name: str, offset: int = 0) -> int:
        return int.from_bytes(self.rsp.read(self.sym[name] + offset, 4), "little", signed=True)

    def step(self, keys: int = 0, n: int = 1) -> None:
        """Run n updates holding keys; the scheduler applies them from the next update."""
        for _ in range(n):
            tick = self.u32("demo_schedule_tick")
            if keys != self.keys:
                self.rsp.write(self.sym["demo_schedule_events"], struct.pack("<2I", tick + 1, keys))
                self.rsp.write(self.sym["demo_schedule_count"], struct.pack("<I", 1))
                self.rsp.write(self.sym["demo_schedule_index"], struct.pack("<I", 0))
                self.keys = keys
            # A hardware breakpoint can be entered again at the same audio
            # boundary (observed on ARM with no scheduler/frame advance).
            # Count completed main-loop updates, not breakpoint notifications.
            for _ in range(8):
                self.rsp.tick(expected=self.update)
                observed = self.u32("demo_schedule_tick")
                if observed == tick + 1:
                    break
                if observed != tick:
                    raise ContinuationError(f"scheduler jumped from {tick} to {observed}")
                self.stale_update_stops += 1
            else:
                raise ContinuationError("scheduler did not advance after eight breakpoint stops")
            self.updates += 1

    def tap(self, key: int) -> None:
        self.step(key)
        self.step(0)

    def front_field(self, name: str, index: int = 0) -> int:
        return self.u32("race_front", self.front[name] + 4 * index)

    def launch(self, map_id: int, human: int, bots: list[int]) -> None:
        if (self.u32("screen"), self.front_field("page")) != (SCREEN_TITLE, FRONT_HOME):
            raise ContinuationError("fresh boot is not on HOME")
        self.step(0, 2)
        self.tap(KEY_DOWN); self.tap(KEY_A)
        if self.front_field("page") != FRONT_SKIRMISH:
            raise ContinuationError("Custom Game page did not open")
        def to_row(row):
            for _ in range(16):
                current = self.front_field("row")
                if current == row:
                    return
                self.tap(KEY_DOWN if current < row else KEY_UP)
            raise ContinuationError(f"menu row {row} not reached")
        def set_field(name, value, index=0):
            for _ in range(3):
                if self.front_field(name, index) == value:
                    return
                self.tap(KEY_RIGHT)
            if self.front_field(name, index) != value:
                raise ContinuationError(f"menu field {name} not set to {value}")
        to_row(0); set_field("map", map_id)
        to_row(1); set_field("race", human)
        to_row(2); set_field("bots", len(bots))
        for i, race in enumerate(bots):
            to_row(3 + i); set_field("bot_races", race, i)
        to_row(3 + len(bots)); self.tap(KEY_A)
        if (self.u32("screen"), self.u32("race_mode"), self.u32("race_skirmish_map"),
                self.u32("race_human"), self.u32("race_bots")) != (SCREEN_PLAY, 1, map_id, human, len(bots)):
            raise ContinuationError("custom game did not start with the selected lobby")

    def state(self) -> dict:
        units = entity_rows(self.rsp.read(self.sym["u"], self.sizes["u"]), self.unit_stride,
                            self.unit_layout, UNIT_FIELDS)
        buildings = entity_rows(self.rsp.read(self.sym["b"], self.sizes["b"]), self.building_stride,
                                self.building_layout, BUILDING_FIELDS, active_queue=True)
        return {"frame": self.u32("frame"), "screen": self.u32("screen"),
                "minerals": self.u32("minerals"), "gas": self.u32("gas"),
                "race_minerals": self.rsp.read(self.sym["race_minerals"], self.sizes["race_minerals"]).hex(),
                "race_gas": self.rsp.read(self.sym["race_gas"], self.sizes["race_gas"]).hex(),
                "mineral_fields": self.rsp.read(self.sym["m"], self.sizes["m"]).hex(),
                "units": units, "buildings": buildings}

    def close(self) -> None:
        try:
            self.proc.stdin.close()
            self.proc.wait(timeout=10)
        except Exception:
            self.proc.kill()
            self.proc.wait()
        self.log.close()


def summary(state: dict) -> dict:
    return {"frame": state["frame"], "screen": state["screen"], "units": len(state["units"]),
            "buildings": len(state["buildings"]), "fingerprint": fingerprint(
                {k: v for k, v in state.items() if k != "screen"})}


def verdict(record: dict) -> dict:
    """Fail closed: every phase and comparison must be present and agree."""
    required = ("saved", "reference", "restored", "resumed", "sram_bytes", "sram_unchanged",
                "loaded", "continue")
    missing = [k for k in required if k not in record]
    if missing:
        raise ContinuationError(f"record lacks {missing}")
    for phase in ("saved", "reference", "restored", "resumed"):
        row = record[phase]
        if (type(row) is not dict or type(row.get("frame")) is not int or
                type(row.get("fingerprint")) is not str or len(row["fingerprint"]) != 64):
            raise ContinuationError(f"{phase} snapshot is malformed")
    checks = {
        "sram_size": record["sram_bytes"] == SRAM_BYTES,
        "sram_unchanged_by_load": record["sram_unchanged"] is True,
        "loaded": record["loaded"] is True,
        "saved_in_pause": record["saved"].get("screen") == SCREEN_PAUSE,
        "restored_matches_saved": (record["restored"]["fingerprint"] == record["saved"]["fingerprint"] and
                                   record["restored"]["frame"] == record["saved"]["frame"]),
        "continuation_matches_reference": (record["resumed"]["fingerprint"] == record["reference"]["fingerprint"] and
                                           record["resumed"]["frame"] == record["reference"]["frame"]),
        "continued": record["reference"]["frame"] > record["saved"]["frame"] and record["continue"] >= 60,
        "world_has_opponents": record["saved"].get("buildings", 0) > 0 and record["saved"].get("units", 0) > 0,
    }
    return {"ok": all(checks.values()), "checks": checks}


def run(args) -> dict:
    out = args.output
    if out.exists():
        raise ContinuationError(f"{out} already exists")
    out.mkdir(parents=True)
    sram = out / "phase1.sav"
    bots = [RACES.index(c) for c in args.bots]
    human = RACES.index(args.human)
    record = {"rom_sha256": hashlib.sha256(args.rom.read_bytes()).hexdigest(),
              "elf_sha256": hashlib.sha256(args.elf.read_bytes()).hexdigest(),
              "runner_sha256": hashlib.sha256(args.runner.read_bytes()).hexdigest(),
              "map": args.map, "human": args.human, "bots": args.bots,
              "play": args.play, "continue": args.continue_updates}
    game = Game(args.runner, args.rom, args.elf, out / "phase1", save_output=sram)
    try:
        game.launch(args.map, human, bots)
        game.step(0, args.play)
        if game.u32("screen") != SCREEN_PLAY:
            raise ContinuationError("match ended before the save point")
        game.tap(KEY_START)
        if game.u32("screen") != SCREEN_PAUSE:
            raise ContinuationError("START did not open the pause menu")
        game.step(KEY_B)
        record["saved"] = summary(game.state())
        game.step(KEY_A)
        game.step(0, args.continue_updates)
        record["reference"] = summary(game.state())
        record["phase1_stale_update_stops"] = game.stale_update_stops
        game.rsp.export_sram()
    finally:
        game.close()
    data = sram.read_bytes()
    if args.tamper_sram:
        # Negative control: flip one byte past the header; the game's loader
        # must reject the record and the verdict must fail.
        data = data[:64] + bytes([data[64] ^ 0xff]) + data[65:]
        sram.write_bytes(data)
        record["tampered"] = True
    record["sram_bytes"], record["sram_sha256"] = len(data), hashlib.sha256(data).hexdigest()
    after = out / "phase2.sav"
    game = Game(args.runner, args.rom, args.elf, out / "phase2", save_input=sram, save_output=after)
    try:
        game.step(0, 2)
        game.step(KEY_L)
        record["loaded"] = bool(game.u32("screen") == SCREEN_PLAY and game.u32("race_mode") == 1)
        record["restored"] = summary(game.state())
        game.step(0, args.continue_updates)
        record["resumed"] = summary(game.state())
        record["phase2_stale_update_stops"] = game.stale_update_stops
        game.rsp.export_sram()
    finally:
        game.close()
    record["sram_unchanged"] = after.read_bytes() == data
    record.update(verdict(record))
    for home in out.glob("phase*/home"):
        for path in sorted(home.rglob("*"), reverse=True):
            path.unlink() if path.is_file() or path.is_symlink() else path.rmdir()
        home.rmdir()
    return record


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rom", type=Path, required=True)
    parser.add_argument("--elf", type=Path, required=True)
    parser.add_argument("--runner", type=Path, required=True, help="port-free mGBA backend executable")
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--map", type=int, choices=(0, 1), default=1)
    parser.add_argument("--human", choices=tuple(RACES), default="t")
    parser.add_argument("--bots", default="zpt")
    parser.add_argument("--play", type=int, default=9000)
    parser.add_argument("--continue", dest="continue_updates", type=int, default=900)
    parser.add_argument("--tamper-sram", action="store_true",
                        help="negative control: corrupt the saved record before phase 2")
    args = parser.parse_args(argv)
    if not 1 <= len(args.bots) <= 3 or any(c not in RACES for c in args.bots):
        parser.error("--bots takes one to three of t, z, p")
    if not 60 <= args.continue_updates <= 20000 or not 60 <= args.play <= 60000:
        parser.error("--play must be in [60,60000] and --continue in [60,20000]")
    record = run(args)
    (args.output / "result.json").write_text(json.dumps(record, indent=1) + "\n")
    print(json.dumps({k: record[k] for k in ("ok", "checks")}))
    return 0 if record["ok"] else 1


if __name__ == "__main__":
    sys.exit(main())
