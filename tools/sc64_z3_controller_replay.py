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


def validate_trace(trace: object, capacity: int, *, windowed: bool = False) -> dict:
    """Validate a controller trace and return a scheduler-ready replay plan."""
    if type(capacity) is not int or capacity < 1:
        raise ValueError("controller queue capacity must be a positive integer")
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
    if not windowed and len(inputs) > capacity:
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
    if windowed:
        last_page = (stop_tick - 1) // CHECKPOINT_TICKS
        peak_page_transitions = max(
            (sum(1 for tick, _ in inputs if tick // CHECKPOINT_TICKS == page)
             for page in range(last_page + 1)), default=0)
        if peak_page_transitions > capacity:
            raise ValueError("controller event page exceeds ELF capacity "
                             f"({peak_page_transitions}/{capacity} in a {CHECKPOINT_TICKS}-tick window)")
    else:
        peak_page_transitions = len(inputs)
    return {"inputs": inputs, "input_count": len(inputs), "stop_tick": stop_tick,
            "start": start, "final": final,
            "scheduler_mode": "windowed" if windowed else "full",
            "peak_page_transitions": peak_page_transitions}


def schedule_capacity(sizes: dict[str, int]) -> int:
    size = sizes.get("demo_schedule_events", 0)
    if not size or size % 8:
        raise RuntimeError("ELF controller event queue has missing or invalid symbol size")
    return size // 8


def schedule_mode(sym: dict[str, int], sizes: dict[str, int]) -> tuple[str, int]:
    """Detect the opted-in page scheduler from its pair of ELF symbols."""
    has_active = "demo_schedule_active" in sym
    has_index = "demo_schedule_index" in sym
    if has_active != has_index:
        raise RuntimeError("ELF has an incomplete windowed scheduler symbol pair")
    capacity = schedule_capacity(sizes)
    if not has_active:
        return "full", capacity
    for name in ("demo_schedule_active", "demo_schedule_index"):
        if sizes.get(name) != 4:
            raise RuntimeError(f"windowed scheduler symbol {name} has unexpected size")
    return "windowed", capacity


def scheduler_events(plan: dict) -> list[tuple[int, int]]:
    """Map 1-based trace host ticks to main.c's zero-based scheduler ticks."""
    return list(plan["inputs"])


def make_schedule_pages(plan: dict, requested_ticks: int, capacity: int,
                        window_ticks: int = CHECKPOINT_TICKS) -> list[dict]:
    """Prevalidate the bounded tick windows to be written before emulation."""
    if type(requested_ticks) is not int or requested_ticks < 1 or requested_ticks > MAX_U32:
        raise ValueError("requested schedule length must be a positive 32-bit integer")
    if type(capacity) is not int or capacity < 1:
        raise ValueError("scheduler capacity must be a positive integer")
    if type(window_ticks) is not int or window_ticks < 1:
        raise ValueError("scheduler window must be a positive integer")
    inputs = plan.get("inputs")
    if not isinstance(inputs, list):
        raise ValueError("scheduler plan inputs must be a list")
    previous = -1
    for tick, held in inputs:
        if type(tick) is not int or tick < 0 or tick <= previous or tick > MAX_U32:
            raise ValueError("scheduler plan contains malformed or unordered input ticks")
        if type(held) is not int or held < 0 or held & ~KEY_MASK:
            raise ValueError("scheduler plan contains held bits outside 0x3ff")
        previous = tick

    pages = []
    index = 0
    for first in range(0, requested_ticks, window_ticks):
        end = min(requested_ticks, first + window_ticks)
        while index < len(inputs) and inputs[index][0] < first:
            index += 1
        page_events = []
        cursor = index
        while cursor < len(inputs) and inputs[cursor][0] < end:
            page_events.append(inputs[cursor])
            cursor += 1
        if len(page_events) > capacity:
            raise ValueError(f"controller event page exceeds ELF capacity "
                             f"({len(page_events)}/{capacity} for ticks [{first},{end}))")
        pages.append({"first": first, "end": end, "events": page_events})
        index = cursor
    return pages


def run_length(plan: dict, ticks: int | None, save_at_end: bool = False) -> tuple[int, bool]:
    """Validate a requested bounded replay length before any emulator work."""
    stop = plan["stop_tick"]
    if ticks is None:
        if save_at_end:
            raise ValueError("--save-at-end requires a --ticks prefix")
        return stop, False
    if type(ticks) is not int or ticks < 1 or ticks > MAX_U32:
        raise ValueError("--ticks must be a positive 32-bit integer")
    if ticks > stop:
        raise ValueError(f"--ticks cannot exceed trace stop tick {stop}")
    if save_at_end and ticks >= stop:
        raise ValueError("--save-at-end requires a prefix ending before the trace stop tick")
    return ticks, True


def route_counter_deltas(current: dict[str, int], previous: dict[str, int]) -> dict[str, int]:
    """Compute unsigned 32-bit interval deltas for cumulative route counters."""
    return {key: (value - previous[key]) & MAX_U32
            for key, value in current.items() if key in previous}


def profile_counters(rsp, sym: dict[str, int]) -> tuple[dict[str, int], dict[str, int]]:
    cycles = {key: profile.u32(rsp, sym[key]) for key in profile.PROFILE_CYCLE_NAMES if key in sym}
    routes = {key: profile.u32(rsp, sym[key])
              for key in ("sc64_route_hits", "sc64_route_builds") if key in sym}
    return cycles, routes


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
        elf_path: Path | None = None, ticks: int | None = None,
        save_at_end: bool = False) -> dict:
    trace = json.loads(trace_path.read_text(encoding="utf-8"))
    rom = rom_path or ROOT / "build/StarCraft-Zerg-Z3-GBA.gba"
    elf = elf_path or ROOT / "build/starcraft-zerg-z3.elf"
    if not rom.is_file() or not elf.is_file():
        raise RuntimeError(f"Zerg-Z3 ROM/ELF missing ({rom}, {elf})")
    sym, sizes = symbols(elf), symbol_sizes(elf)
    scheduler_mode, capacity = schedule_mode(sym, sizes)
    windowed = scheduler_mode == "windowed"
    plan = validate_trace(trace, capacity, windowed=windowed)
    requested_ticks, is_prefix = run_length(plan, ticks, save_at_end)
    pages = (make_schedule_pages(plan, requested_ticks, capacity)
             if windowed else None)
    peak_page_transitions = (max((len(page["events"]) for page in pages), default=0)
                             if pages is not None else plan["input_count"])
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
    if windowed:
        required += ("demo_schedule_active", "demo_schedule_index")
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

        def write_page(events: list[tuple[int, int]]) -> None:
            if len(events) > capacity:
                raise RuntimeError(f"controller page exceeds ELF capacity ({len(events)}/{capacity})")
            queue = b"".join(struct.pack("<2I", tick, held) for tick, held in events)
            for offset in range(0, len(queue), 256):
                rsp.write(sym["demo_schedule_events"] + offset, queue[offset:offset + 256])
            rsp.write(sym["demo_schedule_count"], struct.pack("<I", len(events)))

        if windowed:
            assert pages is not None
            write_page(pages[0]["events"])
            rsp.write(sym["demo_schedule_index"], struct.pack("<I", 0))
            rsp.write(sym["demo_schedule_length"], struct.pack("<I", requested_ticks))
            # Enable only after page data and scheduler metadata are coherent.
            rsp.write(sym["demo_schedule_active"], struct.pack("<I", 1))
        else:
            queue = b"".join(struct.pack("<2I", tick, held)
                             for tick, held in scheduler_events(plan))
            for offset in range(0, len(queue), 256):
                rsp.write(sym["demo_schedule_events"] + offset, queue[offset:offset + 256])
            rsp.write(sym["demo_schedule_length"], struct.pack("<I", requested_ticks))
            rsp.write(sym["demo_schedule_count"], struct.pack("<I", plan["input_count"]))
        update = sym["game_update"] & ~1
        checkpoint_addr = sym["demo_replay_checkpoint"] & ~1
        if rsp.cmd(f"z1,{update:x},2") != "OK" or rsp.cmd(f"Z1,{checkpoint_addr:x},2") != "OK":
            raise RuntimeError("failed to switch from initial HOME update to replay observer")
        rsp.tick()
        if profile.u32(rsp, sym["demo_schedule_tick"]) != 0:
            raise RuntimeError("scheduler did not stop before host_tick=1 at tick zero")
        if windowed:
            assert pages is not None
            initial_page = pages[0]["events"]
            if (profile.u32(rsp, sym["demo_schedule_active"]) != 1 or
                    profile.u32(rsp, sym["demo_schedule_index"]) != 0 or
                    profile.u32(rsp, sym["demo_schedule_count"]) != len(initial_page)):
                raise RuntimeError("windowed scheduler did not stop at the initial page boundary")
        initial_after_neutral = {"screen": profile.u32(rsp, sym["screen"]),
                                 "frame": profile.u32(rsp, sym["frame"])}
        if initial_after_neutral != {"screen": SCREEN_TITLE, "frame": 0}:
            raise RuntimeError(f"neutral HOME loop advanced before scheduled input: {initial_after_neutral}")

        timer = profile.hardware_clock(rsp)
        last_tick = 0
        last_state = campaign_snapshot(rsp, sym, sizes, bank, campaign_layout, z3_layout)
        last_cycles, last_routes = profile_counters(rsp, sym)
        last_clock = timer
        observations = []
        counted_frames = 0
        counted_cycles = 0

        def record(tick: int, elapsed_cycles: int, state: dict,
                   cycle_values: dict[str, int], route_values: dict[str, int]) -> dict:
            nonlocal counted_frames, counted_cycles, last_routes
            frame_delta = (state["frame"] - last_state["frame"]) & MAX_U32
            counted = last_state["screen"] == SCREEN_PLAY and state["screen"] == SCREEN_PLAY and frame_delta > 0
            fps = round(CPU_HZ * frame_delta / max(1, elapsed_cycles), 2) if counted else None
            if counted:
                counted_frames += frame_delta
                counted_cycles += elapsed_cycles
            route_delta = route_counter_deltas(route_values, last_routes)
            last_routes = route_values
            return {"tick": tick, "scheduler_tick": tick, "host_tick": tick,
                    "elapsed_hardware_cycles": elapsed_cycles,
                    "frame_delta": frame_delta, "counted_live_play_interval": counted,
                    "effective_fps": fps,
                    "profile_cycles_last_tick": cycle_values,
                    "route_counters": route_values,
                    "route_interval": route_delta, **state}

        home_capture = capture(rsp, run_dir / "home.png")
        observations.append({"tick": 0, "scheduler_tick": 0, "host_tick": 0, "effective_fps": None,
                             "profile_cycles_last_tick": last_cycles,
                             "route_counters": last_routes,
                             "route_interval": {key: 0 for key in last_routes},
                             "counted_live_play_interval": False, "capture": home_capture, **last_state})
        profile.write_progress(run_dir / "progress.json", profile="zerg-z3", rom=rom, elf=elf,
                               rom_sha256=rom_hash, elf_sha256=elf_hash,
                               configuration={"mission": MISSION_Z3, "trace": str(trace_path),
                                              "trace_sha256": trace_hash,
                                              "input_transitions": plan["input_count"],
                                              "scheduler_capacity": capacity,
                                              "scheduler_mode": scheduler_mode,
                                              "peak_page_transitions": peak_page_transitions,
                                              "trace_stop_tick": plan["stop_tick"],
                                              "requested_ticks": requested_ticks,
                                              "prefix": is_prefix},
                               requested_ticks=requested_ticks, checkpoint_interval=CHECKPOINT_TICKS,
                               latest_checkpoint=observations[-1])

        while last_tick < requested_ticks:
            rsp.tick()
            completed = profile.u32(rsp, sym["demo_schedule_tick"])
            expected = min(requested_ticks, last_tick + CHECKPOINT_TICKS)
            if completed != expected:
                raise RuntimeError(f"expected checkpoint tick {expected}, observed {completed}")
            if windowed:
                assert pages is not None
                prior = pages[(completed - 1) // CHECKPOINT_TICKS]["events"]
                cursor = profile.u32(rsp, sym["demo_schedule_index"])
                count = profile.u32(rsp, sym["demo_schedule_count"])
                if count != len(prior) or cursor != count:
                    raise RuntimeError(f"windowed scheduler page was not fully consumed at tick {completed} "
                                       f"(cursor={cursor}, count={count}, expected={len(prior)})")
                if completed < requested_ticks:
                    next_page = pages[completed // CHECKPOINT_TICKS]["events"]
                    write_page(next_page)
            now = profile.hardware_clock(rsp)
            elapsed = (now - last_clock) & MAX_U32
            state = campaign_snapshot(rsp, sym, sizes, bank, campaign_layout, z3_layout)
            cycle_values, route_values = profile_counters(rsp, sym)
            checkpoint = record(completed, elapsed, state, cycle_values, route_values)
            if completed in (1000, 10000, 30000, 50000, requested_ticks):
                checkpoint["capture"] = capture(rsp, run_dir / f"checkpoint-{completed:06d}.png")
            observations.append(checkpoint)
            profile.write_progress(run_dir / "progress.json", profile="zerg-z3", rom=rom, elf=elf,
                                   rom_sha256=rom_hash, elf_sha256=elf_hash,
                                   configuration={"mission": MISSION_Z3, "trace": str(trace_path),
                                                  "trace_sha256": trace_hash,
                                                  "input_transitions": plan["input_count"],
                                                  "scheduler_capacity": capacity,
                                                  "scheduler_mode": scheduler_mode,
                                                  "peak_page_transitions": peak_page_transitions,
                                                  "trace_stop_tick": plan["stop_tick"],
                                                  "requested_ticks": requested_ticks,
                                                  "prefix": is_prefix},
                                   requested_ticks=requested_ticks, checkpoint_interval=CHECKPOINT_TICKS,
                                   latest_checkpoint=checkpoint)
            print(f"zerg-z3 Z3 tick={completed}/{requested_ticks} screen={state['screen']} "
                  f"frame={state['frame']} fps={checkpoint['effective_fps'] if checkpoint['effective_fps'] is not None else 'n/a'}",
                  flush=True)
            last_tick, last_state, last_clock = completed, state, now

        final = observations[-1]
        if not is_prefix:
            verify_final(plan["final"], final)
        mean_fps = round(CPU_HZ * counted_frames / max(1, counted_cycles), 2)
        report = {"status": "prefix" if is_prefix else "won",
                  "profile": "zerg-z3", "mission": MISSION_Z3,
                  "rom": str(rom), "elf": str(elf), "trace": str(trace_path),
                  "rom_sha256": rom_hash, "elf_sha256": elf_hash, "trace_sha256": trace_hash,
                  "controller_input_count": plan["input_count"], "controller_capacity": capacity,
                  "scheduler_mode": scheduler_mode,
                  "peak_page_transitions": peak_page_transitions,
                  "trace_stop_tick": plan["stop_tick"], "requested_ticks": requested_ticks,
                  "observed_ticks": final["tick"], "prefix": is_prefix,
                  "final_host_tick": final["scheduler_tick"],
                  "final_frame": final["frame"], "counted_play_frames": counted_frames,
                  "mean_live_play_effective_fps": mean_fps,
                  "timer_method": "GBA chained hardware timers at 1000-tick and exact-final observers",
                  "fps_exclusions": "Initial HOME/start interval and final result interval are excluded;"
                                    " only positive frame deltas between PLAY checkpoints contribute.",
                  "capacity_source": "ELF symbol size demo_schedule_events / 8 bytes per event",
                  "initial_home": initial, "initial_neutral_home": initial_after_neutral,
                  "captures": {"home": str(run_dir / "home.png"),
                               "final": str(run_dir / f"checkpoint-{requested_ticks:06d}.png")},
                  "checkpoints": observations}
        if not is_prefix:
            report["verified_win"] = True
        if save_at_end:
            if final["screen"] != SCREEN_PLAY:
                raise RuntimeError("--save-at-end requires the prefix to end during live PLAY")
            if windowed:
                # The ordinary pause/save UI runs outside the replay scheduler.
                rsp.write(sym["demo_schedule_active"], struct.pack("<I", 0))
                rsp.write(sym["demo_schedule_count"], struct.pack("<I", 0))
            saved = profile.save_live_checkpoint(rsp, sym, tick=final["tick"])
            menu.stop_emulator(proc, sock, log)
            proc = sock = log = None
            save_dir = run_dir / "saves" / run_dir.name
            saves = list(save_dir.glob("*.sav"))
            if len(saves) != 1:
                raise RuntimeError(f"checkpoint did not flush exactly one SRAM file: {saves}")
            saved_path = profile.checked_sram(saves[0])
            saved.update({"path": str(saved_path),
                          "sha256": hashlib.sha256(saved_path.read_bytes()).hexdigest()})
            report["checkpoint_save"] = saved
        report_path = run_dir / "profile.json"
        report_path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
        profile.write_progress(run_dir / "progress.json", profile="zerg-z3", rom=rom, elf=elf,
                               rom_sha256=rom_hash, elf_sha256=elf_hash,
                               configuration={"mission": MISSION_Z3, "trace": str(trace_path),
                                              "trace_sha256": trace_hash,
                                              "input_transitions": plan["input_count"],
                                              "scheduler_capacity": capacity,
                                              "scheduler_mode": scheduler_mode,
                                              "peak_page_transitions": peak_page_transitions,
                                              "trace_stop_tick": plan["stop_tick"],
                                              "requested_ticks": requested_ticks,
                                              "prefix": is_prefix},
                               requested_ticks=requested_ticks, checkpoint_interval=CHECKPOINT_TICKS,
                               latest_checkpoint=final, status="prefix" if is_prefix else "finished",
                               final_report=str(report_path))
        print(json.dumps({"status": report["status"], "observed_ticks": final["tick"],
                          "final_frame": final["frame"], "effective_fps": mean_fps,
                          "scheduler_mode": scheduler_mode,
                          "input_transitions": plan["input_count"],
                          "scheduler_capacity": capacity,
                          "peak_page_transitions": peak_page_transitions,
                          "checkpoint_save": report.get("checkpoint_save"),
                          "report": str(report_path)}, indent=2), flush=True)
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
    parser.add_argument("--ticks", type=int, help="stop after this many scheduled host ticks (bounded prefix)")
    parser.add_argument("--save-at-end", action="store_true",
                        help="pause and save SRAM after a live-play prefix; requires --ticks")
    return parser


def main() -> None:
    parser = build_parser()
    args = parser.parse_args()
    try:
        run(args.trace, args.out, args.rom, args.elf, args.ticks, args.save_at_end)
    except (OSError, RuntimeError, ValueError, AssertionError, subprocess.CalledProcessError) as exc:
        parser.exit(1, f"sc64_z3_controller_replay: {exc}\n")


if __name__ == "__main__":
    main()
