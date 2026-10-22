#!/usr/bin/env python3
"""Replay a private T5 controller trace through the normal campaign menu.

Only controller transitions and bounded scheduler pages are written. Mission
state is read from the running GBA build through mGBA's debugger.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
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
import sc64_z3_controller_replay as scheduler
from emulator_smoke import ROOT, capture, symbol_sizes, symbols

MISSION_T5 = 8
ENTRY_TICKS = 9
SCREEN_TITLE, SCREEN_PLAY, SCREEN_RESULT = 0, 1, 3
KEY_A, KEY_RIGHT = 1, 16
CHECKPOINT_TICKS = 1000
DEFAULT_TRACE = ROOT / ".local/sc64/t5/controller-production-final.json"
DEFAULT_OUTPUT = ROOT / "build/emulator/sc64-t5-controller-replay"
TRACE_CHECKPOINTS = frozenset({
    "start", "opening", "rescue-attempt", "command-center-attempt",
    "command-center-approach-cleared", "kerrigan-center-correction",
    "enemy-grant-attempt", "enemy-ai-started", "base-contact-attempt",
    "enemy-base-attack", "force2-area-attack", "paid-production-orders", "periodic",
})
OBS_FIELDS = (
    "frame", "elapsed", "screen", "race_mode", "race_selected_mission",
    "won", "lost", "faults", "switch4", "switch6", "objective",
    "side_units", "enemy_units", "side_buildings", "enemy_buildings",
    "kerrigan_alive", "minerals", "gas", "enemy_minerals", "enemy_gas",
)


def menu_prefix() -> list[tuple[int, int]]:
    """Return (1-based host tick, held keys) for the verified T5 menu route."""
    return [(1, KEY_A), (2, 0), (3, KEY_RIGHT), (4, 0), (5, KEY_RIGHT),
            (6, 0), (7, KEY_A), (8, 0), (9, KEY_A), (10, 0)]


def require_int_fields(row: dict, fields: tuple[str, ...], label: str) -> None:
    for field in fields:
        if field not in row:
            raise ValueError(f"{label} event is missing required field {field}")
        if type(row[field]) is not int:
            raise ValueError(f"{label} field {field} must be an integer")


def validate_trace(trace: object, capacity: int) -> dict:
    """Validate source trace and return absolute normal-menu scheduler plan."""
    if type(capacity) is not int or capacity < 1:
        raise ValueError("controller queue capacity must be a positive integer")
    if not isinstance(trace, dict) or type(trace.get("trace_schema")) is not int or trace["trace_schema"] != 1:
        raise ValueError("T5 trace must have trace_schema=1")
    if trace.get("entry") != "t5-probe-entry":
        raise ValueError("T5 source trace entry must be 't5-probe-entry'")
    if not isinstance(trace.get("policy"), str) or not trace["policy"]:
        raise ValueError("T5 trace policy must be a nonempty string")
    if type(trace.get("tick_limit")) is not int or not 1 <= trace["tick_limit"] <= scheduler.MAX_U32:
        raise ValueError("T5 trace tick_limit must be a positive 32-bit integer")
    if type(trace.get("mission")) is not int or trace["mission"] != MISSION_T5:
        raise ValueError("T5 source trace mission must be global ID 8")
    rows = trace.get("events")
    if not isinstance(rows, list) or not rows:
        raise ValueError("T5 trace events must be a nonempty array")

    start = final = None
    checkpoints: list[dict] = []
    source_inputs: list[tuple[int, int]] = []
    prior_held, prior_input_tick, prior_event_tick = 0, 0, -1
    source_input_events = 0
    final_source_tick = None
    for i, row in enumerate(rows):
        if not isinstance(row, dict):
            raise ValueError(f"trace event {i} must be an object")
        kind = row.get("kind")
        tick = row.get("host_tick")
        if type(tick) is not int or tick < 0 or tick > scheduler.MAX_U32:
            raise ValueError(f"trace event {i} has invalid host_tick")
        if tick < prior_event_tick:
            raise ValueError(f"trace event {i} is out of time order")
        prior_event_tick = tick
        if kind == "input":
            held, pressed = row.get("held"), row.get("pressed")
            if tick < 1 or tick <= prior_input_tick:
                raise ValueError(f"input event {i} must have strictly increasing positive host_tick")
            if type(held) is not int or held < 0 or held & ~scheduler.KEY_MASK:
                raise ValueError(f"input event {i} held keys exceed 0x3ff")
            if type(pressed) is not int or pressed != (held & ~prior_held):
                raise ValueError(f"input event {i} pressed does not match held transition")
            source_input_events += 1
            # The frozen T5 trace starts with an explicit neutral sample at
            # tick 1. It is not a transition and is already represented by the
            # scheduler's default neutral state, so do not queue it twice at
            # the menu-release tick.
            if held == prior_held:
                if source_input_events != 1 or tick != 1 or held != 0:
                    raise ValueError(f"input event {i} is not a held-state transition")
            else:
                source_inputs.append((tick, held))
            prior_input_tick, prior_held = tick, held
        elif kind == "start":
            if start is not None or final is not None or tick != 0:
                raise ValueError("T5 source must have exactly one start observation at host_tick 0")
            if source_inputs:
                raise ValueError("T5 start observation must precede source controller inputs")
            require_int_fields(row, OBS_FIELDS, "start")
            start = row
        elif kind in TRACE_CHECKPOINTS - {"start"}:
            if start is None or final is not None:
                raise ValueError(f"{kind} checkpoint must follow start and precede final")
            require_int_fields(row, OBS_FIELDS, kind)
            if checkpoints and tick < checkpoints[-1]["host_tick"]:
                raise ValueError("T5 observation checkpoints must be ordered")
            checkpoints.append(row)
        elif kind in ("won", "lost", "limit"):
            if start is None or final is not None or i != len(rows) - 1:
                raise ValueError("T5 trace must end with exactly one final outcome observation")
            require_int_fields(row, OBS_FIELDS, "final")
            final = row
            final_source_tick = tick
        else:
            raise ValueError(f"trace event {i} has unsupported kind {kind!r}")

    if start is None or final is None:
        raise ValueError("T5 trace requires start and final observations")
    if rows[0] is not start:
        raise ValueError("T5 start observation must be the first event")
    if start["frame"] != 0 or start["elapsed"] != 0 or start["screen"] != SCREEN_PLAY:
        raise ValueError("T5 start observation must be live PLAY at frame/elapsed zero")
    if start["race_mode"] != 0 or start["race_selected_mission"] != MISSION_T5:
        raise ValueError("T5 start observation is not mission 8 campaign play")
    if start["won"] or start["lost"] or start["faults"]:
        raise ValueError("T5 start observation must have no result or trigger faults")
    assert final_source_tick is not None
    if final_source_tick < prior_input_tick:
        raise ValueError("T5 final observation precedes a controller transition")
    if final_source_tick != trace["tick_limit"]:
        raise ValueError("T5 final observation must match the declared tick_limit")
    if final is not None and final.get("kind") != "limit":
        raise ValueError("the frozen T5 controller trace is an unfinished limit run, not a completion proof")
    if final_source_tick > scheduler.MAX_U32 - ENTRY_TICKS:
        raise ValueError("T5 source trace is too long to offset by the menu entry")

    prefix = menu_prefix()
    previous = 0
    inputs: list[tuple[int, int]] = []
    transitions: list[tuple[int, int]] = []
    for tick, held in prefix:
        pressed = held & ~previous
        inputs.append((tick - 1, held))
        transitions.append((tick, held))
        previous = held
    for source_tick, held in source_inputs:
        tick = source_tick + ENTRY_TICKS
        if transitions and tick <= transitions[-1][0]:
            raise ValueError("source input collides with normal-menu prefix/release")
        pressed = held & ~previous
        inputs.append((tick - 1, held))
        transitions.append((tick, held))
        previous = held

    stop_tick = final_source_tick + ENTRY_TICKS
    translated_checkpoints = []
    for row in [start, *checkpoints]:
        item = dict(row)
        item["source_host_tick"] = row["host_tick"]
        item["host_tick"] = row["host_tick"] + ENTRY_TICKS
        translated_checkpoints.append(item)
    translated_final = dict(final)
    translated_final["source_host_tick"] = final_source_tick
    translated_final["host_tick"] = stop_tick

    plan = {
        "inputs": inputs,
        "input_count": len(inputs),
        "source_input_count": len(source_inputs),
        "source_input_events": source_input_events,
        "source_policy": trace["policy"],
        "source_tick_limit": trace["tick_limit"],
        "entry_input_count": len(prefix),
        "entry": "campaign-menu",
        "start_tick": ENTRY_TICKS,
        "stop_tick": stop_tick,
        "start": translated_checkpoints[0],
        "checkpoints": translated_checkpoints[1:],
        "final": translated_final,
        "scheduler_mode": "windowed",
    }
    return plan


def make_pages(plan: dict, requested_ticks: int, capacity: int) -> list[dict]:
    """Split controller transitions at every observer and trace checkpoint."""
    if type(requested_ticks) is not int or not 1 <= requested_ticks <= scheduler.MAX_U32:
        raise ValueError("requested schedule length must be a positive 32-bit integer")
    if type(capacity) is not int or capacity < 1:
        raise ValueError("scheduler capacity must be positive")
    boundaries = {requested_ticks}
    if requested_ticks >= ENTRY_TICKS:
        boundaries.add(ENTRY_TICKS)
    boundaries.update(range(CHECKPOINT_TICKS, requested_ticks, CHECKPOINT_TICKS))
    for row in [*plan["checkpoints"], plan["final"]]:
        tick = row["host_tick"]
        if ENTRY_TICKS <= tick < requested_ticks:
            boundaries.add(tick)
    ordered = sorted(boundaries)
    starts = [0, *ordered[:-1]]
    inputs = plan["inputs"]
    pages = []
    cursor = 0
    for first, end in zip(starts, ordered):
        if end <= first:
            continue
        page_events = []
        while cursor < len(inputs) and inputs[cursor][0] < first:
            raise ValueError("controller event was omitted before an observer boundary")
        while cursor < len(inputs) and inputs[cursor][0] < end:
            if inputs[cursor][0] < first:
                raise ValueError("controller page event precedes its interval")
            page_events.append(inputs[cursor])
            cursor += 1
        if len(page_events) > capacity:
            raise ValueError(f"controller page exceeds ELF capacity ({len(page_events)}/{capacity})")
        if end - first > CHECKPOINT_TICKS:
            raise ValueError("observer page exceeds the 1000-tick bounded window")
        pages.append({"start_tick": first, "end_tick": end, "events": page_events})
    if requested_ticks >= plan["stop_tick"] and cursor != len(inputs):
        raise ValueError("controller transitions occur after requested stop tick")
    return pages


def t5_snapshot(rsp, sym: dict[str, int], sizes: dict[str, int], bank: dict,
                campaign_layout: tuple[int, dict[str, int]],
                t5_layout: tuple[int, dict[str, int]],
                trigger_layout: tuple[int, dict[str, int]]) -> dict:
    state = profile.observe(rsp, sym, sizes, bank, campaign_layout=campaign_layout)
    if "race_t5" not in sym or sizes.get("race_t5") != t5_layout[0]:
        raise RuntimeError("T5 runtime state symbol is missing or inconsistent")
    raw = rsp.read(sym["race_t5"], t5_layout[0])
    offsets, trigger_size, trigger_offsets = t5_layout[1], trigger_layout[0], trigger_layout[1]
    if "player" not in offsets or "switches" not in offsets:
        raise RuntimeError("T5 runtime DWARF lacks player/switch members")
    player_base = offsets["player"]
    player_widths = {"elapsed_ticks": 4, "wait_ticks": 2, "owner": 1, "force": 1,
                     "cooldown": 1, "outcome": 1, "fault": 1}
    players = []
    for owner in range(8):
        base = player_base + owner * trigger_size
        if base + trigger_size > len(raw):
            raise RuntimeError("T5 player state array exceeds its DWARF structure")
        item = {}
        for field, width in player_widths.items():
            if field not in trigger_offsets or trigger_offsets[field] + width > trigger_size:
                raise RuntimeError(f"T5 trigger DWARF lacks {field}")
            item[field] = int.from_bytes(raw[base + trigger_offsets[field]:
                                             base + trigger_offsets[field] + width], "little")
        players.append(item)
    switches_offset = offsets["switches"] + offsets.get("switches.bits", 0)
    switches = raw[switches_offset:switches_offset + 32]
    if len(switches) != 32:
        raise RuntimeError("T5 switch bank is outside its runtime state")
    state["race_t5"] = {"players": players, "switches": list(switches),
                        "active_mask": raw[offsets["active_mask"]] if "active_mask" in offsets else 0}
    state["race_selected_mission"] = profile.u32(rsp, sym["race_selected_mission"])
    state["race_completed"] = profile.u32(rsp, sym["race_completed"])
    state["race_minerals"] = list(struct.unpack("<5i", rsp.read(sym["race_minerals"], 20)))
    state["race_gas"] = list(struct.unpack("<5i", rsp.read(sym["race_gas"], 20)))
    state["race_t5_minerals"] = list(state["race_minerals"])
    state["race_t5_gas"] = list(state["race_gas"])
    units, buildings = state["units"], state["buildings"]
    state["side_units"] = units["by_owner"].get("0", 0)
    state["enemy_units"] = units["by_owner"].get("1", 0)
    state["side_buildings"] = buildings["by_owner"].get("0", 0)
    state["enemy_buildings"] = buildings["by_owner"].get("1", 0)
    state["kerrigan_alive"] = int(any(row["alive"] and row["raw_type"] == 16
                                     for row in state["units"]["entities"]))
    state["raynor_alive"] = int(any(row["alive"] and row["raw_type"] == 19
                                    for row in state["units"]["entities"]))
    state["human_outcome"] = players[0]["outcome"]
    state["faults"] = sum(bool(p["fault"]) for p in players)
    state["outcomes"] = sum(bool(p["outcome"]) for p in players)
    state["switch4"] = int(bool(switches[0] & 0x10))
    state["switch6"] = int(bool(switches[0] & 0x40))
    campaign = state.get("race_campaign", {})
    state["objective"] = campaign.get("objective_stage", 0)
    state["phase"] = campaign.get("phase")
    state["elapsed"] = campaign.get("elapsed_frames", 0)
    state["enemy_minerals"] = state["race_minerals"][1]
    state["enemy_gas"] = state["race_gas"][1]
    return state


def compare_observation(expected: dict, actual: dict, label: str) -> None:
    for field in OBS_FIELDS:
        if field in expected and expected[field] != actual.get(field):
            raise RuntimeError(f"{label} {field} mismatch: expected {expected[field]!r}, "
                               f"observed {actual.get(field)!r}")


def verify_final(expected: dict, state: dict) -> None:
    compare_observation(expected, state, "final")
    campaign = state.get("race_campaign", {})
    if (state.get("race_mode") != 0 or state.get("race_selected_mission") != MISSION_T5 or
            campaign.get("mission") != MISSION_T5 or campaign.get("elapsed_frames") != state.get("frame") or
            state.get("elapsed") != state.get("frame")):
        raise RuntimeError("final runtime identity or frame clocks do not match T5 campaign")
    outcome = state.get("human_outcome")
    if expected["kind"] == "won":
        if (state["screen"] != SCREEN_RESULT or state["won"] != 1 or state["lost"] != 0 or
                state["phase"] != 3 or outcome != 1 or not state["faults"] == 0 or
                not (state.get("race_completed", 0) & (1 << MISSION_T5))):
            raise RuntimeError("T5 win lacks the runtime human outcome, campaign completion, or clean trigger state")
    elif expected["kind"] == "lost":
        if (state["screen"] != SCREEN_RESULT or state["lost"] != 1 or state["won"] != 0 or
                state["phase"] != 4 or outcome != 2 or state["faults"] != 0):
            raise RuntimeError("T5 loss lacks the runtime human outcome or clean trigger state")
    elif expected["kind"] == "limit":
        if (state["screen"] != SCREEN_PLAY or state["won"] or state["lost"] or
                state["phase"] != 1 or outcome != 0 or state["faults"] != 0):
            raise RuntimeError("T5 limit must remain active with no outcome or trigger fault")
    else:
        raise RuntimeError(f"unsupported T5 final outcome {expected['kind']!r}")


def _atomic_json(path: Path, value: dict) -> None:
    tmp = path.with_suffix(path.suffix + ".tmp")
    with tmp.open("w", encoding="utf-8") as stream:
        json.dump(value, stream, indent=2)
        stream.write("\n")
        stream.flush()
        os.fsync(stream.fileno())
    os.replace(tmp, path)


def run(trace_path: Path, output: Path, rom_path: Path | None = None,
        elf_path: Path | None = None, ticks: int | None = None) -> dict:
    trace_bytes = trace_path.read_bytes()
    plan = validate_trace(json.loads(trace_bytes), capacity=256)
    rom = rom_path or ROOT / "build/StarCraft-T5-Campaign-GBA.gba"
    elf = elf_path or ROOT / "build/starcraft-t5-campaign.elf"
    if not rom.is_file() or not elf.is_file():
        raise RuntimeError(f"T5 campaign-menu replay ROM/ELF missing ({rom}, {elf})")
    sym, sizes = symbols(elf), symbol_sizes(elf)
    mode, capacity = scheduler.schedule_mode(sym, sizes)
    if mode != "windowed":
        raise RuntimeError("T5 replay ELF must expose the windowed scheduler symbols")
    plan = validate_trace(json.loads(trace_bytes), capacity)
    requested, _ = scheduler.run_length(plan, ticks)
    prefix = requested < plan["stop_tick"]
    pages = make_pages(plan, requested, capacity)
    bank = races.race_bank_profile(elf, sizes)
    campaign_layout = races.dwarf_struct(elf, "Sc64RaceCampaignState")
    t5_layout = races.dwarf_struct(elf, "Sc64T5RuntimeState")
    trigger_layout = races.dwarf_struct(elf, "Sc64T4TriggerState")
    if sizes.get("race_t5") != t5_layout[0]:
        raise RuntimeError("T5 runtime state size does not match its ELF symbol")
    required = ("game_update", "demo_schedule_events", "demo_schedule_count", "demo_schedule_length",
                "demo_schedule_tick", "demo_replay_checkpoint", "demo_schedule_active",
                "demo_schedule_index", "race_front", "race_t5", "race_campaign",
                "race_selected_mission", "race_completed", "race_mode", "race_minerals",
                "race_gas", "screen", "frame", "won", "lost", "u", "b")
    missing = [name for name in required if name not in sym]
    if missing:
        raise RuntimeError("T5 replay ELF is missing symbols: " + ", ".join(missing))
    output.mkdir(parents=True, exist_ok=True)
    run_dir = output / f"t5-controller-{time.time_ns()}"
    run_dir.mkdir(parents=False, exist_ok=False)
    rom_hash, elf_hash = hashlib.sha256(rom.read_bytes()).hexdigest(), hashlib.sha256(elf.read_bytes()).hexdigest()
    trace_hash = hashlib.sha256(trace_bytes).hexdigest()
    history_path, report_path = run_dir / "checkpoints.jsonl", run_dir / "profile.json"
    split.FRONT_FIELDS = split.frontend_offsets(elf)
    scheduler.check_debugger_port_free()
    proc = sock = rsp = log = None
    observations: list[dict] = []

    def record(checkpoint: dict) -> None:
        observations.append(checkpoint)
        with history_path.open("a", encoding="utf-8") as stream:
            stream.write(json.dumps(checkpoint, separators=(",", ":")) + "\n")
            stream.flush()
            os.fsync(stream.fileno())

    def write_page(page: dict) -> None:
        events = page["events"]
        if len(events) > capacity:
            raise RuntimeError(f"controller page exceeds ELF capacity ({len(events)}/{capacity})")
        blob = b"".join(struct.pack("<2I", tick, held) for tick, held in events)
        for offset in range(0, len(blob), 256):
            rsp.write(sym["demo_schedule_events"] + offset, blob[offset:offset + 256])
        rsp.write(sym["demo_schedule_count"], struct.pack("<I", len(events)))

    replay_started = None
    play_started = None
    try:
        proc, sock, rsp, log = split.start_emulator(run_dir.name, rom, run_dir, sym)
        initial = {"screen": profile.u32(rsp, sym["screen"]), "frame": profile.u32(rsp, sym["frame"]),
                   "page": profile.u32(rsp, sym["race_front"] + split.FRONT_FIELDS["page"])}
        if initial != {"screen": SCREEN_TITLE, "frame": 0, "page": split.FRONT_HOME}:
            raise RuntimeError(f"fresh T5 campaign ROM did not boot at HOME: {initial}")
        first = pages[0]
        write_page(first)
        rsp.write(sym["demo_schedule_index"], struct.pack("<I", 0))
        rsp.write(sym["demo_schedule_length"], struct.pack("<I", first["end_tick"]))
        rsp.write(sym["demo_schedule_active"], struct.pack("<I", 1))
        if rsp.cmd(f"z1,{sym['game_update'] & ~1:x},2") != "OK" or rsp.cmd(
                f"Z1,{sym['demo_replay_checkpoint'] & ~1:x},2") != "OK":
            raise RuntimeError("failed to install T5 replay breakpoints")
        rsp.tick()
        if profile.u32(rsp, sym["demo_schedule_tick"]) != 0:
            raise RuntimeError("windowed scheduler did not stop before tick 0")

        state = t5_snapshot(rsp, sym, sizes, bank, campaign_layout, t5_layout, trigger_layout)
        if state.get("race_mode") != 0 or state.get("screen") != SCREEN_TITLE:
            raise RuntimeError("T5 normal-menu replay did not begin at campaign HOME")
        record({"tick": 0, "host_tick": 0, "entry": "campaign-menu", **state})
        capture(rsp, run_dir / "home.png")

        previous_state = state
        boundaries = [page["end_tick"] for page in pages]
        previous_tick = 0
        for page_index, expected_tick in enumerate(boundaries):
            rsp.tick()
            observed_tick = profile.u32(rsp, sym["demo_schedule_tick"])
            if observed_tick != expected_tick:
                raise RuntimeError(f"expected scheduler observer at {expected_tick}, got {observed_tick}")
            cursor = profile.u32(rsp, sym["demo_schedule_index"])
            count = profile.u32(rsp, sym["demo_schedule_count"])
            if cursor != count or count != len(pages[page_index]["events"]):
                raise RuntimeError(f"scheduler page incomplete at {expected_tick}: {cursor}/{count}")
            state = t5_snapshot(rsp, sym, sizes, bank, campaign_layout, t5_layout, trigger_layout)
            now = time.monotonic()
            if replay_started is None:
                replay_started = now
            if expected_tick == ENTRY_TICKS:
                play_started = now
            if expected_tick >= ENTRY_TICKS and (state.get("race_mode") != 0 or
                                                  state.get("race_selected_mission") != MISSION_T5):
                raise RuntimeError(f"normal HOME/menu controls failed to launch T5 by tick {expected_tick}")
            expected_rows = [row for row in [plan["start"], *plan["checkpoints"]]
                             if row["host_tick"] == expected_tick]
            if expected_tick == plan["stop_tick"] and not prefix:
                expected_rows = [plan["final"]]
            for row in expected_rows:
                if row["kind"] == "start" and (state["frame"] != 0 or state["screen"] != SCREEN_PLAY):
                    raise RuntimeError("T5 normal-menu launch did not produce frame-zero PLAY at tick 9")
                compare_observation(row, state, row["kind"])
            if expected_tick == requested:
                rsp.write(sym["demo_schedule_active"], struct.pack("<I", 0))
                rsp.write(sym["demo_schedule_count"], struct.pack("<I", 0))
            elif page_index + 1 < len(pages):
                next_page = pages[page_index + 1]
                write_page(next_page)
                rsp.write(sym["demo_schedule_length"], struct.pack("<I", next_page["end_tick"]))
            frame_delta = (state["frame"] - previous_state.get("frame", 0)) & 0xFFFFFFFF
            checkpoint = {"tick": expected_tick, "host_tick": expected_tick,
                          "frame_delta": frame_delta, **state}
            checkpoint["wall_seconds"] = round(now - replay_started, 6)
            record(checkpoint)
            previous_state, previous_tick = state, expected_tick

        final = observations[-1]
        status = "prefix" if prefix else plan["final"]["kind"]
        play_wall = (time.monotonic() - play_started) if play_started is not None else 0.0
        play_frames = max(0, int(final.get("frame", 0)))
        report = {"profile": "terran-t5-campaign-menu", "entry": "campaign-menu",
                  "status": status, "mission": MISSION_T5, "rom": str(rom), "elf": str(elf),
                  "trace": str(trace_path), "rom_sha256": rom_hash, "elf_sha256": elf_hash,
                  "trace_sha256": trace_hash, "scheduler_mode": mode, "scheduler_capacity": capacity,
                  "input_transitions": plan["input_count"],
                  "source_input_transitions": plan["source_input_count"],
                  "source_input_events": plan["source_input_events"],
                  "source_policy": plan["source_policy"],
                  "source_tick_limit": plan["source_tick_limit"],
                  "peak_page_transitions": max((len(p["events"]) for p in pages), default=0),
                  "trace_stop_tick": plan["stop_tick"], "requested_ticks": requested,
                  "observed_ticks": final["tick"], "prefix": prefix,
                  "checkpoint_history": str(history_path), "captures": {"home": str(run_dir / "home.png")},
                  "final": final, "play_frames": play_frames,
                  "play_wall_seconds": round(play_wall, 3),
                  "play_effective_fps": round(play_frames / play_wall, 3) if play_wall > 0 else 0.0,
                  "completed": False, "final_is_unfinished_limit": status == "limit",
                  "final": final, "verified_win": False}
        if not prefix:
            verify_final(plan["final"], final)
            report["verified_win"] = report["status"] == "won"
        _atomic_json(report_path, report)
        return report
    finally:
        if proc is not None:
            try:
                menu.stop_emulator(proc, sock, log)
            except Exception:
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
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--rom", type=Path, help="T5 normal-menu campaign ROM override")
    parser.add_argument("--elf", type=Path, help="T5 normal-menu campaign ELF override")
    parser.add_argument("--ticks", type=int, help="replay a positive prefix of global scheduler ticks")
    return parser


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    try:
        result = run(args.trace, args.output, args.rom, args.elf, args.ticks)
        print(json.dumps({"status": result["status"], "observed_ticks": result["observed_ticks"]}, indent=2))
    except (OSError, ValueError, RuntimeError, KeyError, json.JSONDecodeError,
            subprocess.CalledProcessError, subprocess.TimeoutExpired) as exc:
        print(f"sc64_t5_controller_replay: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
