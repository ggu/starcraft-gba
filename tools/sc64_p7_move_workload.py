#!/usr/bin/env python3
"""Measure the experimental P7 opening and an attack-move workload in mGBA.

The runner launches Homeland through the normal HOME/mission/briefing inputs,
runs neutral 100-tick intervals, and can then issue one ordinary attack-move:
R (combat group), B (field menu), DOWN, A (attack mode), D-pad until the
cursor reaches the enemy building nearest the selected army, A. The debugger
writes only the controller injection field; game state is read only. Its
D-pad schedule depends only on read state, so equal builds see equal inputs.
Effective fps uses emulated GBA timer deltas over complete in-play intervals.
This is a bounded emulator workload, not a mission win or hardware result.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys

TOOLS = Path(__file__).resolve().parent
if str(TOOLS) not in sys.path:
    sys.path.insert(0, str(TOOLS))

import sc64_menu_smoke as menu
import sc64_races_profile as races
import sc64_split_profile as profile
import sc64_split_smoke as split
import sc64_z3_controller_replay as replay
from emulator_smoke import capture, symbol_sizes, symbols

KEY_A, KEY_B, KEY_RIGHT, KEY_LEFT, KEY_UP, KEY_DOWN, KEY_R = 1, 2, 16, 32, 64, 128, 256
P7_MISSION, P7_ORDINAL, PROTOSS_MISSIONS = 26, 6, 7
STATE_BANKS = ("u", "b", "m", "geysers", "geyser_count", "race_campaign", "race_p7",
               "race_minerals", "race_gas", "race_supply_used", "race_supply_cap",
               "race_player", "weapons_level", "armor_level", "visible", "explored")
TIMING_FIELDS = {"elapsed_hardware_cycles", "effective_fps", "profile_cycles_last_tick", "routes"}
MAX_TICKS = 12000


def ticks_arg(value: str) -> int:
    ticks = int(value)
    if ticks < 0 or ticks > MAX_TICKS or ticks % 100:
        raise argparse.ArgumentTypeError(f"ticks must be a multiple of 100 in [0,{MAX_TICKS}]")
    return ticks


def compare(reference: Path, candidate: Path) -> int:
    """Compare every non-timing checkpoint field, inputs and the command phase."""
    a = json.loads(reference.read_text())
    b = json.loads(candidate.read_text())
    problems = []
    if len(a["checkpoints"]) != len(b["checkpoints"]):
        problems.append("checkpoint count")
    for x, y in zip(a["checkpoints"], b["checkpoints"]):
        for key in sorted(set(x) | set(y)):
            if key not in TIMING_FIELDS and x.get(key) != y.get(key):
                problems.append(f"{x.get('label')}: {key}")
    strip = lambda r: [(i["demo_frame"], i["held"]) for i in r["inputs"]]
    if strip(a) != strip(b):
        problems.append("controller inputs")
    if a["command"] != b["command"] or a["ended"] != b["ended"]:
        problems.append("command or ending")
    print(json.dumps({"match": not problems, "differences": problems[:20],
                      "checkpoints": len(a["checkpoints"]), "inputs": len(a["inputs"]),
                      "ended": a["ended"],
                      "neutral_fps": [a["neutral_fps"], b["neutral_fps"]],
                      "attack_fps": [a["attack_fps"], b["attack_fps"]]}, indent=2))
    return 0 if not problems else 1


def run(args: argparse.Namespace) -> int:
    args.out.mkdir(parents=True, exist_ok=False)
    sym, sizes = symbols(args.elf), symbol_sizes(args.elf)
    split.FRONT_FIELDS = split.frontend_offsets(args.elf)
    bank = races.race_bank_profile(args.elf, sizes)
    _, campaign = races.dwarf_struct(args.elf, "Sc64RaceCampaignState")
    _, persistent = races.dwarf_struct(args.elf, "Sc64P7PersistentState")
    _, runtime = races.dwarf_struct(args.elf, "Sc64P7RuntimeState")
    unit_stride, unit_fields = bank["unit_stride_bytes"], bank["unit_offsets"]
    inputs: list[dict[str, int]] = []
    observations: list[dict] = []
    command: dict = {}
    proc = sock = rsp = log = None
    try:
        proc, sock, rsp, log = split.start_emulator("p7-move", args.rom, args.out, sym)
        write = rsp.write

        def input_only(address: int, data: bytes) -> None:
            if address != sym["demo_inject_keys"] or len(data) != 2:
                raise RuntimeError("only controller input may be written")
            inputs.append({"demo_frame": menu.u32(rsp, sym["demo_frame"]),
                           "held": int.from_bytes(data, "little")})
            write(address, data)
        rsp.write = input_only
        launch = split.campaign_launch(rsp, sym, "protoss", P7_MISSION, P7_ORDINAL,
                                       PROTOSS_MISSIONS)

        def s32(name: str) -> int:
            return struct.unpack("<i", rsp.read(sym[name], 4))[0]

        def selected_slots() -> set[int]:
            raw = rsp.read(sym["u"], bank["unit_capacity"] * unit_stride)
            widths = unit_fields.widths

            def field(row: bytes, name: str) -> int:
                at = unit_fields[name]
                return int.from_bytes(row[at:at + widths.get(name, 4)], "little")
            rows = (raw[i * unit_stride:(i + 1) * unit_stride] for i in range(bank["unit_capacity"]))
            return {i for i, row in enumerate(rows) if field(row, "alive") and field(row, "selected")}

        def state() -> dict:
            s = races.observed_state(rsp, sym, sizes, bank, campaign)
            s["runtime_fault"] = rsp.read(sym["race_p7"] + persistent["runtime"] + runtime["fault"], 1)[0]
            s["unsupported_count"] = menu.u32(rsp, sym["mission_unsupported_count"])
            s["state_sha256"] = {n: hashlib.sha256(rsp.read(sym[n], sizes[n])).hexdigest()
                                 for n in STATE_BANKS if n in sym and sizes.get(n, 0) > 0}
            hp: dict[str, int] = {}
            for unit in s["units"]["entities"]:
                key = str(unit["owner"])
                hp[key] = hp.get(key, 0) + unit["hp"] + unit.get("shield", 0)
            s["unit_hp_by_owner"] = hp
            s["selected"] = len(selected_slots())
            s["profile_cycles_last_tick"], s["routes"] = replay.profile_counters(rsp, sym)
            return s

        def interval(label: str, previous: dict, clock: int, total: list[int]):
            now = profile.hardware_clock(rsp)
            s = state()
            if s["runtime_fault"] or s["unsupported_count"]:
                raise RuntimeError(f"{label}: runtime fault or unsupported source row")
            elapsed = (now - clock) & 0xFFFFFFFF
            frames = (s["frame"] - previous["frame"]) & 0xFFFFFFFF
            observations.append({"label": label, "elapsed_hardware_cycles": elapsed,
                                 "frame_delta": frames,
                                 "effective_fps": round(profile.CPU_HZ * frames / elapsed, 3),
                                 **s})
            if s["screen"] == 1 and frames == 100:
                total[0] += elapsed
                total[1] += frames
            return s, now

        previous = state()
        if previous["race_selected_mission"] != P7_MISSION or previous["screen"] != 1:
            raise RuntimeError("normal menu inputs did not launch P7")
        observations.append({"label": "launch", **previous})
        clock = profile.hardware_clock(rsp)
        neutral, attack = [0, 0], [0, 0]
        for tick in range(100, args.neutral_ticks + 1, 100):
            rsp.tick(100)
            previous, clock = interval(f"neutral+{tick}", previous, clock, neutral)
        if args.attack_ticks:
            for key in (KEY_R, KEY_B, KEY_DOWN, KEY_A):
                split.key(rsp, sym, key)
            mode = s32("mode")
            snapshot = races.observed_state(rsp, sym, sizes, bank, campaign)
            chosen = selected_slots()
            army = [e for e in snapshot["units"]["entities"] if e["owner"] == 0 and e["slot"] in chosen]
            enemy = [e for e in snapshot["buildings"]["entities"] if e["owner"] == 1]
            if not army or not enemy:
                raise RuntimeError("attack-move needs a selected army and an enemy building")
            ax = sum(e["x"] for e in army) // len(army)
            ay = sum(e["y"] for e in army) // len(army)
            target = min(enemy, key=lambda e: ((e["x"] - ax) ** 2 + (e["y"] - ay) ** 2, e["slot"]))
            held = steps = 0
            while True:
                x, y = s32("cx"), s32("cy")
                want = (KEY_RIGHT if x < target["x"] - 2 else KEY_LEFT if x > target["x"] + 2 else 0)
                want |= (KEY_DOWN if y < target["y"] - 2 else KEY_UP if y > target["y"] + 2 else 0)
                if want != held:
                    rsp.write(sym["demo_inject_keys"], struct.pack("<H", want))
                    held = want
                if not want:
                    break
                rsp.tick(1)
                steps += 1
                if steps >= 2000:
                    raise RuntimeError("cursor did not reach the attack target")
            rsp.tick(2)
            split.key(rsp, sym, KEY_A)
            command = {"selected_army": len(army), "army_centroid": [ax, ay],
                       "target_building": {k: target[k] for k in ("slot", "type", "x", "y")},
                       "mode_after_menu": mode, "mode_after_issue": s32("mode"),
                       "dpad_ticks": steps, "cursor": [s32("cx"), s32("cy")]}
            previous = state()
            clock = profile.hardware_clock(rsp)
            observations.append({"label": "after-command", **previous})
            for tick in range(100, args.attack_ticks + 1, 100):
                rsp.tick(100)
                previous, clock = interval(f"attack+{tick}", previous, clock, attack)
                if previous["screen"] != 1:
                    break
        capture(rsp, args.out / "final.png")
        fps = lambda total: round(profile.CPU_HZ * total[1] / total[0], 3) if total[0] else None
        result = {
            "runner_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
            "status": "bounded emulator workload; no mission win, fresh SRAM or hardware qualification",
            "launch": launch, "command": command,
            "neutral_fps": fps(neutral), "attack_fps": fps(attack),
            "neutral_cycles_frames": neutral, "attack_cycles_frames": attack,
            "ended": {k: previous[k] for k in ("screen", "frame", "won", "lost")},
            "rom_sha256": hashlib.sha256(args.rom.read_bytes()).hexdigest(),
            "elf_sha256": hashlib.sha256(args.elf.read_bytes()).hexdigest(),
            "inputs": inputs, "checkpoints": observations,
        }
        (args.out / "result.json").write_text(json.dumps(result, indent=2) + "\n")
        print(json.dumps({k: result[k] for k in ("neutral_fps", "attack_fps", "ended", "command")}))
        return 0
    finally:
        if proc is not None:
            menu.stop_emulator(proc, sock, log)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--elf", type=Path)
    parser.add_argument("--rom", type=Path)
    parser.add_argument("--out", type=Path)
    parser.add_argument("--neutral-ticks", type=ticks_arg, default=600)
    parser.add_argument("--attack-ticks", type=ticks_arg, default=0)
    parser.add_argument("--compare", type=Path, nargs=2, metavar=("REFERENCE", "CANDIDATE"))
    args = parser.parse_args()
    if args.compare:
        return compare(*args.compare)
    if not (args.elf and args.rom and args.out):
        parser.error("--elf, --rom and --out are required unless --compare is used")
    return run(args)


if __name__ == "__main__":
    raise SystemExit(main())
