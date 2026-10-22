#!/usr/bin/env python3
"""Replay the private Z3 win trace on the frozen Zerg-Z3 ARM profile.

The script schedules controller inputs only. Game state is read through mGBA's
debugger at bounded scheduler checkpoints and at the exact final tick.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import socket
import struct
import subprocess
import sys
import time

TOOLS = Path(__file__).resolve().parent
if str(TOOLS) not in sys.path:
    sys.path.insert(0, str(TOOLS))

import sc64_menu_smoke as menu
import sc64_races_profile as races
import sc64_split_smoke as split
import sc64_split_profile as profile
from emulator_smoke import ROOT, capture, symbol_sizes, symbols

CPU_HZ = 16_777_216
SCREEN_TITLE, SCREEN_PLAY, SCREEN_RESULT = 0, 1, 3
MISSION_Z3 = 14
MAX_U32 = 0xFFFFFFFF
CHECKPOINT_TICKS = 1000
KEY_MASK = 0x3FF
DEFAULT_TRACE = ROOT / ".local/sc64/z3-controller-asan-win.json"
DEFAULT_OUTPUT = ROOT / ".local/sc64/z3-controller-replay"


def validate_trace(trace: object, capacity: int) -> dict:
    """Validate a controller trace and return a scheduler-ready replay plan."""
    if not isinstance(trace, dict) or trace.get("schema") != 1:
        raise ValueError("trace schema must be an object with schema=1")
    if trace.get("mission") != MISSION_Z3:
        raise ValueError("trace mission must be Z3 global ID 14")
    rows = trace.get("events")
    if not isinstance(rows, list) or not rows:
        raise ValueError("trace events must be a nonempty array")

    inputs: list[tuple[int, int]] = []
    previous_tick = 0
    previous_held = 0
    start = None
    final = None
    for index, row in enumerate(rows):
        if not isinstance(row, dict):
            raise ValueError(f"trace event {index} must be an object")
        kind = row.get("kind")
        if kind == "input":
            tick, held, pressed = row.get("host_tick"), row.get("held"), row.get("pressed")
            if type(tick) is not int or tick < 1 or tick > MAX_U32:
                raise ValueError(f"input event {index} has invalid host_tick")
            if tick <= previous_tick:
                raise ValueError("input host_tick values must be strictly increasing")
            if type(held) is not int or held < 0 or held & ~KEY_MASK:
                raise ValueError(f"input event {index} has held bits outside 0x3ff")
            if held == previous_held:
                raise ValueError(f"input event {index} is not a held-state transition")
            expected_pressed = held & ~previous_held
            if type(pressed) is not int or pressed != expected_pressed:
                raise ValueError(f"input event {index} pressed does not match held transition")
            inputs.append((tick - 1, held))
            previous_tick, previous_held = tick, held
        elif kind == "start":
            if start is not None:
                raise ValueError("trace must contain exactly one start event")
            start = row
        elif kind == "won":
            if final is not None:
                raise ValueError("trace must contain exactly one won event")
            final = row
        elif kind != "periodic":
            raise ValueError(f"trace event {index} has unsupported kind {kind!r}")

    if not inputs or inputs[0][0] != 0:
        raise ValueError("first controller transition must be host_tick=1")
    if len(inputs) > capacity:
        raise ValueError(f"controller queue exceeds ELF capacity ({len(inputs)}/{capacity})")
    if start is None:
        raise ValueError("trace must contain exactly one start event")
    if start.get("host_tick") != 7 or start.get("frame") != 0 or start.get("screen") != SCREEN_PLAY:
        raise ValueError("trace must record Z3 launch at host_tick=7, frame=0, screen=PLAY")
    if final is None or rows[-1] is not final:
        raise ValueError("trace must end with its final won event")
    stop_tick = final.get("host_tick")
    if type(stop_tick) is not int or stop_tick < 1 or stop_tick > MAX_U32:
        raise ValueError("final won event has invalid 32-bit host_tick")
    if stop_tick <= previous_tick:
        raise ValueError("final won event must follow all controller transitions")
    if final.get("screen") != SCREEN_RESULT or final.get("outcome") != 1:
        raise ValueError("final won event must record a won RESULT screen")
    if final.get("win_presence_condition") != 1 or final.get("foes_any", 0xFFFFFFFF) > 6:
        raise ValueError("final won event is missing the source victory predicate")
    expected_final = {"host_tick": 71186, "frame": 70833, "screen": SCREEN_RESULT,
                      "outcome": 1, "side_units": 79, "side_buildings": 8,
                      "enemy_units": 3, "enemy_buildings": 1, "foes_any": 4,
                      "ore": 157, "gas": 1359}
    if any(final.get(key) != value for key, value in expected_final.items()):
        raise ValueError("final won event does not match the verified Z3 controller trace")
    for key in ("frame", "side_units", "side_buildings", "enemy_units", "enemy_buildings", "ore", "gas"):
        if type(final.get(key)) is not int:
            raise ValueError(f"final won event is missing integer field {key}")
    return {"inputs": inputs, "input_count": len(inputs), "stop_tick": stop_tick,
            "start": start, "final": final}


def schedule_capacity(sizes: dict[str, int]) -> int:
    size = sizes.get("demo_schedule_events", 0)
    if not size or size % 8:
        raise RuntimeError("ELF controller event queue has missing or invalid symbol size")
    return size // 8


def scheduler_events(plan: dict) -> list[tuple[int, int]]:
    """Map 1-based trace host ticks to main.c's zero-based scheduler ticks."""
    return list(plan["inputs"])


def check_debugger_port_free() -> None:
    probe = socket.socket()
    try:
        probe.bind(("127.0.0.1", 2345))
    except OSError as exc:
        raise RuntimeError("mGBA debugger port 2345 is occupied; leaving the existing emulator untouched") from exc
    finally:
        probe.close()


def campaign_snapshot(rsp, sym: dict[str, int], sizes: dict[str, int], bank: dict,
                      campaign_layout: tuple[int, dict[str, int]],
                      z3_layout: tuple[int, dict[str, int]]) -> dict:
    state = races.observed_state(rsp, sym, sizes, bank, campaign_layout[1])
    state["race_minerals"] = list(struct.unpack("<5i", rsp.read(sym["race_minerals"], 20)))
    state["race_gas"] = list(struct.unpack("<5i", rsp.read(sym["race_gas"], 20)))
    z3_size, z3_offsets = z3_layout
    raw = rsp.read(sym["race_z3"], z3_size)
    widths = {"fired": 2, "switch7": 1, "outcome": 1, "sequence": 1, "transmission": 1}
    state["race_z3"] = {}
    for field, width in widths.items():
        if field in z3_offsets:
            offset = z3_offsets[field]
            state["race_z3"][field] = int.from_bytes(raw[offset:offset + width], "little")
    action_ticks_size = sizes.get("race_z3_action_ticks", 0)
    if action_ticks_size != 2:
        raise RuntimeError(f"race_z3_action_ticks has unexpected ELF size {action_ticks_size}")
    state["race_z3_action_ticks"] = int.from_bytes(rsp.read(sym["race_z3_action_ticks"], action_ticks_size), "little")
    # Building snapshots include raw type and coordinates; read the verified
    # HP field directly using DWARF rather than assuming a struct offset.
    hp_offset = bank["building_offsets"].get("hp")
    raw_offset = bank["building_offsets"].get("raw_type")
    if hp_offset is not None and raw_offset is not None:
        size = bank["building_stride_bytes"]
        entries = []
        base = sym["b"]
        for item in state["buildings"]["entities"]:
            entries.append({**item, "hp": int.from_bytes(
                rsp.read(base + item["slot"] * size + hp_offset, 4), "little", signed=True)})
        state["buildings"]["entities"] = entries
    return state


def verify_final(trace_final: dict, state: dict) -> None:
    required = {
        "screen": SCREEN_RESULT, "frame": trace_final["frame"], "won": 1, "lost": 0,
        "race_mode": 0, "race_selected_mission": MISSION_Z3,
        "side_units": trace_final["side_units"], "side_buildings": trace_final["side_buildings"],
        "enemy_units": trace_final["enemy_units"], "enemy_buildings": trace_final["enemy_buildings"],
        "minerals": trace_final["ore"], "gas": trace_final["gas"],
    }
    for key, expected in required.items():
        if state.get(key) != expected:
            raise RuntimeError(f"final {key} mismatch: expected {expected}, observed {state.get(key)}")
    if state["enemy_units"] + state["enemy_buildings"] != trace_final["foes_any"]:
        raise RuntimeError("final Foes Any Unit count does not match the verified trace predicate")
    campaign = state.get("campaign", {})
    for key, expected in (("mission", MISSION_Z3), ("elapsed_frames", trace_final["frame"]), ("phase", 3)):
        if campaign.get(key) != expected:
            raise RuntimeError(f"final campaign.{key} mismatch: expected {expected}, got {campaign.get(key)}")
    if not (campaign.get("objective_flags", 0) & 1):
        raise RuntimeError("final campaign primary objective flag is absent")
    z3 = state.get("race_z3", {})
    if z3.get("outcome") != 1 or z3.get("sequence") != 10 or z3.get("switch7") != 1:
        raise RuntimeError(f"final Z3 model is not a completed win: {z3}")
    if state.get("race_z3_action_ticks") != 0:
        raise RuntimeError("final Z3 action timer is not idle")
    if not (state.get("race_completed", 0) & (1 << MISSION_Z3)):
        raise RuntimeError("Z3 completion bit is absent")
    if "enemy_ore" in trace_final and state.get("race_minerals", [None, None])[1] != trace_final["enemy_ore"]:
        raise RuntimeError("final ENEMY mineral array does not match the trace")
    if "enemy_gas" in trace_final and state.get("race_gas", [None, None])[1] != trace_final["enemy_gas"]:
        raise RuntimeError("final ENEMY gas array does not match the trace")
    mature = [bld for bld in state["buildings"]["entities"] if bld.get("raw_type") == 150]
    if len(mature) != 1 or mature[0].get("hp") != 250:
        raise RuntimeError(f"final Mature Chrysalis roster/HP mismatch: {mature}")


def run(trace_path: Path, output: Path, rom_path: Path | None = None,
        elf_path: Path | None = None) -> dict:
    trace = json.loads(trace_path.read_text(encoding="utf-8"))
    rom = rom_path or ROOT / "build/StarCraft-Zerg-Z3-GBA.gba"
    elf = elf_path or ROOT / "build/starcraft-zerg-z3.elf"
    if not rom.is_file() or not elf.is_file():
        raise RuntimeError(f"Zerg-Z3 ROM/ELF missing ({rom}, {elf})")
    sym, sizes = symbols(elf), symbol_sizes(elf)
    capacity = schedule_capacity(sizes)
    plan = validate_trace(trace, capacity)
    campaign_meta = split.read_campaign_metadata(rom, elf, "zerg")
    if MISSION_Z3 not in campaign_meta["enabled_mission_ids"]:
        raise RuntimeError("Zerg-Z3 ELF does not enable global mission 14")
    bank = races.race_bank_profile(elf, sizes)
    campaign_layout = races.dwarf_struct(elf, "Sc64RaceCampaignState")
    z3_layout = races.dwarf_struct(elf, "Sc64Z3State")
    required = ("game_update", "demo_inject_keys", "demo_schedule_events", "demo_schedule_count",
                "demo_schedule_length", "demo_schedule_tick", "demo_replay_checkpoint", "screen",
                "frame", "won", "lost", "race_mode", "race_selected_mission", "race_completed",
                "race_campaign", "race_z3", "race_z3_action_ticks", "race_minerals", "race_gas",
                "race_front", "u", "b")
    missing = [key for key in required if key not in sym]
    if missing:
        raise RuntimeError(f"Zerg-Z3 ELF is missing symbols: {', '.join(missing)}")
    if sizes.get("demo_schedule_events", 0) // 8 != capacity:
        raise RuntimeError("controller queue capacity could not be derived from ELF symbol size")

    run_dir = output / f"z3-win-{time.time_ns()}"
    run_dir.mkdir(parents=True, exist_ok=False)
    trace_hash = hashlib.sha256(trace_path.read_bytes()).hexdigest()
    rom_hash = hashlib.sha256(rom.read_bytes()).hexdigest()
    elf_hash = hashlib.sha256(elf.read_bytes()).hexdigest()
    split.FRONT_FIELDS = split.frontend_offsets(elf)
    check_debugger_port_free()
    proc = sock = rsp = log = None
    try:
        proc, sock, rsp, log = split.start_emulator(run_dir.name, rom, run_dir, sym)
        initial = {"screen": profile.u32(rsp, sym["screen"]),
                   "frame": profile.u32(rsp, sym["frame"]),
                   "page": profile.u32(rsp, sym["race_front"] + split.FRONT_FIELDS["page"])}
        if initial != {"screen": SCREEN_TITLE, "frame": 0, "page": split.FRONT_HOME}:
            raise RuntimeError(f"fresh boot advanced away from HOME before replay: {initial}")

        queue = b"".join(struct.pack("<2I", tick, held) for tick, held in scheduler_events(plan))
        for offset in range(0, len(queue), 256):
            rsp.write(sym["demo_schedule_events"] + offset, queue[offset:offset + 256])
        rsp.write(sym["demo_schedule_length"], struct.pack("<I", plan["stop_tick"]))
        rsp.write(sym["demo_schedule_count"], struct.pack("<I", plan["input_count"]))
        update = sym["game_update"] & ~1
        checkpoint_addr = sym["demo_replay_checkpoint"] & ~1
        if rsp.cmd(f"z1,{update:x},2") != "OK" or rsp.cmd(f"Z1,{checkpoint_addr:x},2") != "OK":
            raise RuntimeError("failed to switch from initial HOME update to replay observer")
        rsp.tick()
        if profile.u32(rsp, sym["demo_schedule_tick"]) != 0:
            raise RuntimeError("scheduler did not stop before host_tick=1 at tick zero")
        initial_after_neutral = {"screen": profile.u32(rsp, sym["screen"]),
                                 "frame": profile.u32(rsp, sym["frame"])}
        if initial_after_neutral != {"screen": SCREEN_TITLE, "frame": 0}:
            raise RuntimeError(f"neutral HOME loop advanced before scheduled input: {initial_after_neutral}")

        timer = profile.hardware_clock(rsp)
        last_tick = 0
        last_state = campaign_snapshot(rsp, sym, sizes, bank, campaign_layout, z3_layout)
        last_clock = timer
        observations = []
        counted_frames = 0
        counted_cycles = 0

        def record(tick: int, elapsed_cycles: int, state: dict) -> dict:
            nonlocal counted_frames, counted_cycles
            frame_delta = (state["frame"] - last_state["frame"]) & MAX_U32
            counted = last_state["screen"] == SCREEN_PLAY and state["screen"] == SCREEN_PLAY and frame_delta > 0
            fps = round(CPU_HZ * frame_delta / max(1, elapsed_cycles), 2) if counted else None
            if counted:
                counted_frames += frame_delta
                counted_cycles += elapsed_cycles
            return {"tick": tick, "scheduler_tick": tick, "host_tick": tick,
                    "elapsed_hardware_cycles": elapsed_cycles,
                    "frame_delta": frame_delta, "counted_live_play_interval": counted,
                    "effective_fps": fps, **state}

        home_capture = capture(rsp, run_dir / "home.png")
        observations.append({"tick": 0, "scheduler_tick": 0, "host_tick": 0, "effective_fps": None,
                             "counted_live_play_interval": False, "capture": home_capture, **last_state})
        profile.write_progress(run_dir / "progress.json", profile="zerg-z3", rom=rom, elf=elf,
                               rom_sha256=rom_hash, elf_sha256=elf_hash,
                               configuration={"mission": MISSION_Z3, "trace": str(trace_path),
                                              "trace_sha256": trace_hash,
                                              "input_transitions": plan["input_count"],
                                              "scheduler_capacity": capacity,
                                              "stop_tick": plan["stop_tick"]},
                               requested_ticks=plan["stop_tick"], checkpoint_interval=CHECKPOINT_TICKS,
                               latest_checkpoint=observations[-1])

        while last_tick < plan["stop_tick"]:
            rsp.tick()
            completed = profile.u32(rsp, sym["demo_schedule_tick"])
            expected = min(plan["stop_tick"], last_tick + CHECKPOINT_TICKS)
            if completed != expected:
                raise RuntimeError(f"expected checkpoint tick {expected}, observed {completed}")
            now = profile.hardware_clock(rsp)
            elapsed = (now - last_clock) & MAX_U32
            state = campaign_snapshot(rsp, sym, sizes, bank, campaign_layout, z3_layout)
            checkpoint = record(completed, elapsed, state)
            if completed in (1000, 10000, 30000, 50000, plan["stop_tick"]):
                checkpoint["capture"] = capture(rsp, run_dir / f"checkpoint-{completed:06d}.png")
            observations.append(checkpoint)
            profile.write_progress(run_dir / "progress.json", profile="zerg-z3", rom=rom, elf=elf,
                                   rom_sha256=rom_hash, elf_sha256=elf_hash,
                                   configuration={"mission": MISSION_Z3, "trace": str(trace_path),
                                                  "trace_sha256": trace_hash,
                                                  "input_transitions": plan["input_count"],
                                                  "scheduler_capacity": capacity,
                                                  "stop_tick": plan["stop_tick"]},
                                   requested_ticks=plan["stop_tick"], checkpoint_interval=CHECKPOINT_TICKS,
                                   latest_checkpoint=checkpoint)
            print(f"zerg-z3 Z3 tick={completed}/{plan['stop_tick']} screen={state['screen']} "
                  f"frame={state['frame']} fps={checkpoint['effective_fps'] if checkpoint['effective_fps'] is not None else 'n/a'}",
                  flush=True)
            last_tick, last_state, last_clock = completed, state, now

        final = observations[-1]
        verify_final(plan["final"], final)
        mean_fps = round(CPU_HZ * counted_frames / max(1, counted_cycles), 2)
        report = {"profile": "zerg-z3", "mission": MISSION_Z3,
                  "rom": str(rom), "elf": str(elf), "trace": str(trace_path),
                  "rom_sha256": rom_hash, "elf_sha256": elf_hash, "trace_sha256": trace_hash,
                  "controller_input_count": plan["input_count"], "controller_capacity": capacity,
                  "scheduler_stop_tick": plan["stop_tick"], "final_host_tick": final["scheduler_tick"],
                  "final_frame": final["frame"], "counted_play_frames": counted_frames,
                  "mean_live_play_effective_fps": mean_fps,
                  "timer_method": "GBA chained hardware timers at 1000-tick and exact-final observers",
                  "fps_exclusions": "Initial HOME/start interval and final result interval are excluded;"
                                    " only positive frame deltas between PLAY checkpoints contribute.",
                  "capacity_source": "ELF symbol size demo_schedule_events / 8 bytes per event",
                  "initial_home": initial, "initial_neutral_home": initial_after_neutral,
                  "captures": {"home": str(run_dir / "home.png"),
                               "final": str(run_dir / f"checkpoint-{plan['stop_tick']:06d}.png")},
                  "checkpoints": observations}
        report_path = run_dir / "profile.json"
        report_path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
        profile.write_progress(run_dir / "progress.json", profile="zerg-z3", rom=rom, elf=elf,
                               rom_sha256=rom_hash, elf_sha256=elf_hash,
                               configuration={"mission": MISSION_Z3, "trace": str(trace_path),
                                              "trace_sha256": trace_hash,
                                              "input_transitions": plan["input_count"],
                                              "scheduler_capacity": capacity,
                                              "stop_tick": plan["stop_tick"]},
                               requested_ticks=plan["stop_tick"], checkpoint_interval=CHECKPOINT_TICKS,
                               latest_checkpoint=final, status="finished", final_report=str(report_path))
        print(json.dumps({"status": "won", "final_frame": final["frame"],
                          "effective_fps": mean_fps, "report": str(report_path)}, indent=2), flush=True)
        return report
    finally:
        if proc is not None:
            if sock is not None:
                try:
                    menu.stop_emulator(proc, sock, log)
                    proc = None
                except Exception:
                    pass
            if proc is not None:
                proc.terminate()
                try:
                    proc.wait(timeout=3)
                except subprocess.TimeoutExpired:
                    proc.kill()
                    proc.wait()
                if log is not None and not log.closed:
                    log.close()


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--trace", type=Path, default=DEFAULT_TRACE)
    parser.add_argument("--rom", type=Path, help="override the default Zerg-Z3 ROM")
    parser.add_argument("--elf", type=Path, help="override the default Zerg-Z3 ELF")
    parser.add_argument("--out", type=Path, default=DEFAULT_OUTPUT)
    return parser


def main() -> None:
    parser = build_parser()
    args = parser.parse_args()
    try:
        run(args.trace, args.out, args.rom, args.elf)
    except (OSError, RuntimeError, ValueError, AssertionError, subprocess.CalledProcessError) as exc:
        parser.exit(1, f"sc64_z3_controller_replay: {exc}\n")


if __name__ == "__main__":
    main()
