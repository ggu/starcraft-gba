#!/usr/bin/env python3
"""Replay a private Jacobs Installation controller trace on the T4 ARM build.

The replay writes only scheduled controller input and scheduler metadata.
Mission, actor, trigger, and objective state is observed through mGBA symbols.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
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
import sc64_z3_controller_replay as scheduler
from emulator_smoke import ROOT, capture, symbol_sizes, symbols

MISSION_T4 = 7
SCREEN_TITLE, SCREEN_PLAY, SCREEN_RESULT = 0, 1, 3
KEY_A, KEY_SELECT, KEY_START = 1, 4, 8
KEY_RIGHT, KEY_DOWN = 16, 128
DEFAULT_TRACE = ROOT / ".local/sc64/t4-controller-attempt.json"
DEFAULT_OUTPUT = ROOT / "build/emulator/sc64-t4-controller-replay"
TRIGGER_BYTES = 144
T4_STATE_BYTES = 1268
T4_PLAYERS = 8
T4_TRIGGER_LIMIT = 64
CHECKPOINT_TICKS = scheduler.CHECKPOINT_TICKS
T4_CYCLE_NAMES = ("sc64_t4_rescue_cycles", "sc64_t4_script_cycles", "sc64_t4_trap_cycles",
                  "sc64_hud_minimap_cycles")


def profile_label(entry: str) -> str:
    """Name the user-visible T4 entry route in reports and progress files."""
    return "terran-t4-campaign-menu" if entry == "campaign-menu" else "terran-t4-internal"


def profile_counters(rsp, sym: dict[str, int]) -> tuple[dict, dict]:
    cycles, routes = scheduler.profile_counters(rsp, sym)
    cycles.update({name: profile.u32(rsp, sym[name]) for name in T4_CYCLE_NAMES if name in sym})
    return cycles, routes


def terrain_counters(rsp, sym: dict[str, int]) -> dict[str, int]:
    """Optional cumulative event/pixel/tile counts, separate from cycle samples."""
    return {name: profile.u32(rsp, address) for name, address in sym.items()
            if name.startswith("sc64_terrain_diag_") or
            name in ("sc64_terrain_requests", "sc64_terrain_hits", "sc64_terrain_partial")}


def validate_trace(trace: object, capacity: int, *, windowed: bool = True) -> dict:
    """Validate a T4 trace before opening an emulator or scheduling input."""
    if not windowed:
        raise ValueError("T4 replay requires the paged windowed controller scheduler")
    if type(capacity) is not int or capacity < 1:
        raise ValueError("controller queue capacity must be a positive integer")
    if not isinstance(trace, dict) or type(trace.get("schema")) is not int or trace["schema"] != 1:
        raise ValueError("trace schema must be an object with schema=1")
    if type(trace.get("mission")) is not int or trace["mission"] != MISSION_T4:
        raise ValueError("trace mission must be T4 internal global ID 7")
    entry = trace.get("entry", "internal-chord")
    if entry not in ("internal-chord", "campaign-menu"):
        raise ValueError("trace entry must be 'internal-chord' or 'campaign-menu'")
    entry_prefix = ([(1, KEY_START | KEY_SELECT), (2, 0), (3, KEY_A)]
                    if entry == "internal-chord" else
                    [(1, KEY_A), (2, 0), (3, KEY_RIGHT), (4, 0), (5, KEY_DOWN),
                     (6, 0), (7, KEY_A), (8, 0), (9, KEY_A)])
    launch_tick = 3 if entry == "internal-chord" else 9
    if not isinstance(trace.get("policy"), str) or not trace["policy"]:
        raise ValueError("trace policy must be a nonempty string")
    rows = trace.get("events")
    if not isinstance(rows, list) or not rows:
        raise ValueError("trace events must be a nonempty array")

    inputs: list[tuple[int, int]] = []
    input_transitions: list[tuple[int, int]] = []
    previous_tick, previous_held = 0, 0
    previous_event_tick = 0
    start = final = None
    start_input_count = None
    periodic: list[dict] = []
    for index, row in enumerate(rows):
        if not isinstance(row, dict):
            raise ValueError(f"trace event {index} must be an object")
        kind = row.get("kind")
        tick = row.get("host_tick")
        if type(tick) is not int or not 1 <= tick <= 0xFFFFFFFF or tick < previous_event_tick:
            raise ValueError(f"trace event {index} has invalid or unordered host_tick")
        previous_event_tick = tick
        if kind == "input":
            held, pressed = row.get("held"), row.get("pressed")
            if tick <= previous_tick:
                raise ValueError(f"input event {index} is not strictly ordered")
            if type(held) is not int or held < 0 or held & ~scheduler.KEY_MASK:
                raise ValueError(f"input event {index} has held bits outside 0x3ff")
            if held == previous_held:
                raise ValueError(f"input event {index} is not a held-state transition")
            if type(pressed) is not int or pressed != (held & ~previous_held):
                raise ValueError(f"input event {index} pressed does not match held transition")
            inputs.append((tick - 1, held))
            input_transitions.append((tick, held))
            previous_tick, previous_held = tick, held
        elif kind == "start":
            if start is not None or final is not None:
                raise ValueError("trace must contain exactly one start event before its final event")
            if len(inputs) != len(entry_prefix):
                raise ValueError("start event must immediately follow the exact entry controls")
            start = row
            start_input_count = len(inputs)
        elif kind == "periodic":
            if start is None or final is not None:
                raise ValueError("periodic checkpoints must occur after start and before final")
            if periodic and tick <= periodic[-1]["host_tick"]:
                raise ValueError("periodic checkpoint ticks must increase")
            if tick % CHECKPOINT_TICKS:
                raise ValueError(f"periodic checkpoint must align to {CHECKPOINT_TICKS}-tick observer")
            periodic.append(row)
        elif kind in ("won", "lost", "limit"):
            if start is None or final is not None or index != len(rows) - 1:
                raise ValueError("trace must end with exactly one won, lost, or limit event")
            final = row
        else:
            raise ValueError(f"trace event {index} has unsupported kind {kind!r}")

    if input_transitions[:len(entry_prefix)] != entry_prefix:
        raise ValueError(f"trace input prefix does not match {entry} entry controls")
    if start_input_count != len(entry_prefix):
        raise ValueError("start event must follow exactly the whitelisted entry controls")
    if start is None or final is None or rows[-1] is not final:
        raise ValueError("trace requires start and final outcome events")
    required_observation = ("frame", "elapsed", "screen", "won", "lost", "race_mode",
                            "race_selected_mission", "faults", "outcomes", "human_outcome",
                            "objective", "actor_slot", "actor_alive", "actor_x", "actor_y",
                            "raynor_alive", "side_units", "enemy_units")
    require_trace_fields(start, required_observation, "start")
    for checkpoint in periodic:
        require_trace_fields(checkpoint, required_observation, "periodic")
    require_trace_fields(final, required_observation, "final")
    if start["host_tick"] != launch_tick or start["frame"] != 0 or start["elapsed"] != 0:
        raise ValueError(f"start event must record {entry} launch at host_tick={launch_tick} "
                         "and frame/elapsed zero")
    if start["screen"] != SCREEN_PLAY:
        raise ValueError("start event must capture live PLAY after normal menu deployment")
    if start["race_mode"] != 0 or start["race_selected_mission"] != MISSION_T4:
        raise ValueError("start event is not the T4 campaign mission")
    if start["won"] or start["lost"] or start["faults"] or start["outcomes"] or start["human_outcome"]:
        raise ValueError("start event must be live with no result, trigger fault, or outcome")
    stop_tick = final.get("host_tick")
    if type(stop_tick) is not int or stop_tick < previous_tick or stop_tick < start["host_tick"]:
        raise ValueError("final event must follow all controller transitions")
    if periodic and periodic[-1]["host_tick"] > stop_tick:
        raise ValueError("periodic checkpoint occurs after final result")
    if final["frame"] < start["frame"]:
        raise ValueError("final frame precedes the start frame")
    if final["kind"] == "won":
        if final["screen"] != SCREEN_RESULT or final["won"] != 1 or final["lost"] != 0:
            raise ValueError("won event must record a won RESULT screen")
        if final["human_outcome"] != 1 or final["outcomes"] < 1 or final["faults"] != 0:
            raise ValueError("won event requires the human trigger outcome and no trigger faults")
        expected_phase = 3
    elif final["kind"] == "lost":
        if final["screen"] != SCREEN_RESULT or final["lost"] != 1 or final["won"] != 0:
            raise ValueError("lost event must record a lost RESULT screen")
        if final["human_outcome"] != 2 or final["outcomes"] < 1 or final["faults"] != 0:
            raise ValueError("lost event requires the human defeat outcome and no trigger faults")
        expected_phase = 4
    else:
        if final["screen"] != SCREEN_PLAY or final["won"] or final["lost"]:
            raise ValueError("limit event must record unfinished live play")
        if final["human_outcome"] != 0 or final["outcomes"] != 0 or final["faults"] != 0:
            raise ValueError("limit event requires no human outcome or trigger faults")
        expected_phase = 1
    # Older host traces omit the campaign phase. When present, reject a label
    # that contradicts the final result; the live verifier always checks it.
    if "phase" in final and (type(final["phase"]) is not int or
                             final["phase"] != expected_phase):
        raise ValueError("final event phase contradicts its outcome label")

    event_list = scheduler.scheduler_events({"inputs": inputs})
    plan = {"inputs": event_list, "input_count": len(inputs), "entry": entry,
            "start_tick": launch_tick, "stop_tick": stop_tick,
            "start": start, "final": final, "periodic": periodic,
            "scheduler_mode": "windowed"}
    pages = scheduler.make_schedule_pages(plan, stop_tick, capacity)
    plan["pages"] = pages
    plan["peak_page_transitions"] = max((len(page["events"]) for page in pages), default=0)
    return plan


def require_trace_fields(row: dict, fields: tuple[str, ...], label: str) -> None:
    for field in fields:
        if field not in row:
            raise ValueError(f"{label} event is missing required field {field}")
        if type(row[field]) is not int:
            raise ValueError(f"{label} field {field} must be an integer")


def read_unit_slots(rsp, address: int, bank: dict) -> list[dict]:
    fields = bank["unit_offsets"]
    needed = ("alive", "owner", "raw_owner", "type", "raw_type", "x", "y")
    if any(name not in fields for name in needed):
        raise RuntimeError("Unit DWARF lacks fields needed for T4 actor/Raynor verification")
    capacity, stride = bank["unit_capacity"], bank["unit_stride_bytes"]
    raw = rsp.read(address, capacity * stride)
    slots = []
    for slot in range(capacity):
        base = slot * stride
        entry = {name: int.from_bytes(raw[base + fields[name]:base + fields[name] + 4],
                                       "little", signed=True) for name in needed}
        entry["slot"] = slot
        slots.append(entry)
    return slots


def raynor_alive_from_slots(slots: list[dict]) -> bool:
    # Match the host controller harness: raw actor identity is stable while
    # live owner/raw-owner can differ after source rescue conversion.
    return any(row["alive"] and row["raw_type"] == 20 for row in slots)


def requested_is_prefix(requested_ticks: int, trace_stop_tick: int) -> bool:
    return requested_ticks < trace_stop_tick


def decode_t4_state(raw: bytes, offsets: dict[str, int],
                    trigger_size: int, trigger_offsets: dict[str, int]) -> dict:
    """Decode T4 trigger/world state with sizes and offsets read from ELF DWARF."""
    if len(raw) != T4_STATE_BYTES or trigger_size != TRIGGER_BYTES:
        raise RuntimeError(f"unexpected T4 state sizes: {len(raw)}/{trigger_size}")
    required = {"player", "switches", "location30", "message_ticks", "ping_x", "ping_y",
                "ping_ticks", "location30_moved", "message", "active_mask",
                "doodad_slot", "doodad_enabled"}
    if required - offsets.keys():
        raise RuntimeError(f"T4 runtime DWARF is incomplete: {sorted(required - offsets.keys())}")
    widths = {"elapsed_ticks": 4, "wait_ticks": 2, "owner": 1, "force": 1,
              "cooldown": 1, "outcome": 1, "active_wait": 1, "fault": 1,
              "fault_trigger": 1, "fault_slot": 1, "reserved": 1}
    if widths.keys() - trigger_offsets.keys():
        raise RuntimeError("T4 trigger-state DWARF is incomplete")

    def read(offset: int, size: int, signed: bool = False) -> int:
        if offset < 0 or offset + size > len(raw):
            raise RuntimeError("T4 runtime DWARF member lies outside its structure")
        return int.from_bytes(raw[offset:offset + size], "little", signed=signed)

    def read_bytes(offset: int, size: int) -> bytes:
        if offset < 0 or offset + size > len(raw):
            raise RuntimeError("T4 runtime DWARF array lies outside its structure")
        return raw[offset:offset + size]

    players = []
    player_base = offsets["player"]
    for owner in range(T4_PLAYERS):
        base = player_base + owner * trigger_size
        if base + trigger_size > len(raw):
            raise RuntimeError("T4 player contexts exceed the runtime state")
        for name, width in widths.items():
            if trigger_offsets[name] < 0 or trigger_offsets[name] + width > trigger_size:
                raise RuntimeError(f"T4 trigger-state scalar {name} lies outside its context")
        player = {name: read(base + trigger_offsets[name], width) for name, width in widths.items()}
        for field in ("cursor", "flags"):
            offset = trigger_offsets.get(field)
            if offset is None:
                raise RuntimeError(f"T4 trigger-state DWARF lacks {field}")
            length = T4_TRIGGER_LIMIT
            if base + offset + length > base + trigger_size:
                raise RuntimeError(f"T4 trigger-state {field} lies outside its context")
            player[field] = list(read_bytes(base + offset, length))
        players.append(player)
    switches_offset = offsets["switches"]
    switches_bits = offsets.get("switches.bits")
    # DWARF tools report only the outer member; its nested bits member starts at byte zero.
    switches_offset += switches_bits or 0
    doodad_slots, doodad_enabled = offsets["doodad_slot"], offsets["doodad_enabled"]
    return {
        "players": players,
        "switches": list(read_bytes(switches_offset, 32)),
        "location30": [read(offsets["location30"] + i * 2, 2) for i in range(4)],
        "message_ticks": read(offsets["message_ticks"], 2),
        "ping": {"x": read(offsets["ping_x"], 2), "y": read(offsets["ping_y"], 2),
                 "ticks": read(offsets["ping_ticks"], 2)},
        "location30_moved": read(offsets["location30_moved"], 1),
        "message": read(offsets["message"], 1),
        "active_mask": read(offsets["active_mask"], 1),
        "doodad_slot": list(read_bytes(doodad_slots, 32)),
        "doodad_enabled": list(read_bytes(doodad_enabled, 32)),
    }


def t4_snapshot(rsp, sym: dict[str, int], sizes: dict[str, int], bank: dict,
                 campaign_layout: tuple[int, dict[str, int]],
                 t4_layout: tuple[int, dict[str, int]],
                 trigger_layout: tuple[int, dict[str, int]]) -> dict:
    state = profile.observe(rsp, sym, sizes, bank, campaign_layout=campaign_layout)
    if "race_t4" not in sym:
        raise RuntimeError("T4 runtime state symbol is missing")
    raw = rsp.read(sym["race_t4"], t4_layout[0])
    state["race_t4"] = decode_t4_state(raw, t4_layout[1], trigger_layout[0], trigger_layout[1])
    state["race_selected_mission"] = profile.u32(rsp, sym["race_selected_mission"])
    state["race_completed"] = profile.u32(rsp, sym["race_completed"])
    state["race_minerals"] = list(struct.unpack("<5i", rsp.read(sym["race_minerals"], 20)))
    state["race_gas"] = list(struct.unpack("<5i", rsp.read(sym["race_gas"], 20)))
    state["supply_used"] = profile.u32(rsp, sym["supply_used"])
    state["supply_cap"] = profile.u32(rsp, sym["supply_cap"])
    state["unit_slots"] = read_unit_slots(rsp, sym["u"], bank)
    units = state["units"]["by_owner"]
    buildings = state["buildings"]["by_owner"]
    state["side_units"] = units.get("0", 0)
    state["enemy_units"] = units.get("1", 0)
    state["force4_units"] = units.get("4", 0)
    state["side_buildings"] = buildings.get("0", 0)
    state["enemy_buildings"] = buildings.get("1", 0) + buildings.get("4", 0)
    state["human_outcome"] = state["race_t4"]["players"][0]["outcome"]
    state["faults"] = sum(bool(p["fault"]) for p in state["race_t4"]["players"])
    state["outcomes"] = sum(bool(p["outcome"]) for p in state["race_t4"]["players"])
    state["switch4"] = bool(state["race_t4"]["switches"][0] & 0x10)
    state["switch7"] = bool(state["race_t4"]["switches"][0] & 0x80)
    state["objective"] = state.get("race_campaign", {}).get("objective_stage", 0)
    state["phase"] = state.get("race_campaign", {}).get("phase")
    state["elapsed"] = state.get("race_campaign", {}).get("elapsed_frames", 0)
    state["raynor_alive"] = int(raynor_alive_from_slots(state["unit_slots"]))
    return state


TRACE_STATE_FIELDS = ("frame", "elapsed", "screen", "won", "lost", "outcomes", "faults", "switch4",
                      "switch7", "objective", "phase", "actor_alive", "actor_x", "actor_y",
                      "raynor_alive", "human_outcome", "side_units", "enemy_units")


def compare_observation(expected: dict, actual: dict, label: str) -> None:
    """Compare every trace field represented by the runtime snapshot."""
    if "actor_slot" in expected:
        slot = expected["actor_slot"]
        slots = actual.get("unit_slots", [])
        actor = slots[slot] if type(slot) is int and 0 <= slot < len(slots) else None
        actual["actor_slot"] = slot
        actual["actor_alive"] = int(bool(actor and actor["alive"]))
        actual["actor_x"] = actor.get("x", -1) if actor else -1
        actual["actor_y"] = actor.get("y", -1) if actor else -1
    for field in TRACE_STATE_FIELDS:
        if field in expected and expected[field] != actual.get(field):
            raise RuntimeError(f"{label} {field} mismatch: expected {expected[field]!r}, "
                               f"observed {actual.get(field)!r}")


def verify_final(final: dict, state: dict) -> None:
    compare_observation(final, state, "final")
    kind = final["kind"]
    if state.get("race_mode") != 0 or state.get("race_selected_mission") != MISSION_T4:
        raise RuntimeError("final runtime is not T4 campaign mission 7")
    campaign = state.get("race_campaign", {})
    if (campaign.get("mission") != MISSION_T4 or
            campaign.get("elapsed_frames") != state.get("elapsed") or
            campaign.get("elapsed_frames") != state.get("frame")):
        raise RuntimeError("final campaign identity or elapsed frame count does not match runtime")
    if kind == "won":
        if state.get("screen") != SCREEN_RESULT or state.get("won") != 1 or state.get("lost") != 0:
            raise RuntimeError("trace claims a win but the runtime did not reach WON result")
        if (campaign.get("phase") != 3 or state.get("human_outcome") != 1 or
                state.get("outcomes", 0) < 1 or
                state.get("faults") != 0):
            raise RuntimeError("T4 win lacks the human trigger outcome or contains a trigger fault")
        if not (campaign.get("objective_flags", 0) & 1):
            raise RuntimeError("T4 primary objective completion bit is absent")
    elif kind == "lost":
        if state.get("screen") != SCREEN_RESULT or state.get("lost") != 1 or state.get("won") != 0:
            raise RuntimeError("trace claims a loss but the runtime did not reach LOST result")
        if (campaign.get("phase") != 4 or state.get("human_outcome") != 2 or
                state.get("outcomes", 0) < 1 or
                state.get("faults") != 0 or
                (campaign.get("objective_flags", 0) & 1)):
            raise RuntimeError("T4 loss phase, human outcome, faults, or primary objective is inconsistent")
    else:
        if state.get("screen") != SCREEN_PLAY or state.get("won") or state.get("lost"):
            raise RuntimeError("trace limit must be an unfinished live PLAY state")
        if (campaign.get("phase") != 1 or state.get("human_outcome") != 0 or
                state.get("outcomes") != 0 or
                state.get("faults") != 0 or
                (campaign.get("objective_flags", 0) & 1)):
            raise RuntimeError("T4 limit phase, human outcome, faults, or primary objective is inconsistent")


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
    trace = json.loads(trace_bytes)
    rom = rom_path or ROOT / "build/StarCraft-T4-Internal-GBA.gba"
    elf = elf_path or ROOT / "build/starcraft-t4-replay.elf"
    if not rom.is_file() or not elf.is_file():
        raise RuntimeError(f"T4 internal replay ROM/ELF missing ({rom}, {elf})")
    sym, sizes = symbols(elf), symbol_sizes(elf)
    mode, capacity = scheduler.schedule_mode(sym, sizes)
    if mode != "windowed":
        raise RuntimeError("T4 replay ELF must use the windowed controller scheduler")
    plan = validate_trace(trace, capacity, windowed=True)
    requested, _ = scheduler.run_length(plan, ticks)
    prefix = requested_is_prefix(requested, plan["stop_tick"])
    pages = scheduler.make_schedule_pages(plan, requested, capacity)
    trace_peak_page_transitions = plan["peak_page_transitions"]
    requested_peak_page_transitions = max((len(page["events"]) for page in pages), default=0)
    bank = races.race_bank_profile(elf, sizes)
    campaign_layout = races.dwarf_struct(elf, "Sc64RaceCampaignState")
    t4_layout = races.dwarf_struct(elf, "Sc64T4RuntimeState")
    trigger_layout = races.dwarf_struct(elf, "Sc64T4TriggerState")
    if t4_layout[0] != T4_STATE_BYTES or trigger_layout[0] != TRIGGER_BYTES:
        raise RuntimeError("T4 runtime DWARF size does not match the audited state layout")
    required = ("game_update", "demo_schedule_events", "demo_schedule_count", "demo_schedule_length",
                "demo_schedule_tick", "demo_replay_checkpoint", "demo_schedule_active",
                "demo_schedule_index", "demo_inject_keys", "race_front", "race_t4",
                "race_campaign", "race_selected_mission", "race_completed", "race_mode",
                "race_minerals", "race_gas", "supply_used", "supply_cap", "screen", "frame",
                "won", "lost", "u", "b", "demo_tick_buckets", "demo_last_cycles",
                "demo_peak_cycles")
    missing = [name for name in required if name not in sym]
    if missing:
        raise RuntimeError("T4 replay ELF is missing symbols: " + ", ".join(missing))
    if sizes.get("race_t4") != T4_STATE_BYTES:
        raise RuntimeError(f"race_t4 symbol size is not {T4_STATE_BYTES}")
    if sizes.get("demo_tick_buckets") != 32:
        raise RuntimeError("demo_tick_buckets must contain eight 32-bit histogram bins")

    output.mkdir(parents=True, exist_ok=True)
    run_dir = output / f"t4-controller-{time.time_ns()}"
    run_dir.mkdir(parents=False, exist_ok=False)
    rom_hash = hashlib.sha256(rom.read_bytes()).hexdigest()
    elf_hash = hashlib.sha256(elf.read_bytes()).hexdigest()
    trace_hash = hashlib.sha256(trace_bytes).hexdigest()
    history_path = run_dir / "checkpoints.jsonl"
    report_path = run_dir / "profile.json"
    split.FRONT_FIELDS = split.frontend_offsets(elf)
    scheduler.check_debugger_port_free()
    proc = sock = rsp = log = None
    observations: list[dict] = []
    previous_terrain: dict[str, int] = {}

    def save_report(report: dict) -> None:
        _atomic_json(report_path, report)

    def record(checkpoint: dict) -> None:
        nonlocal previous_terrain
        terrain = terrain_counters(rsp, sym)
        if terrain:
            checkpoint["terrain_counters"] = terrain
            checkpoint["terrain_interval"] = scheduler.route_counter_deltas(terrain, previous_terrain)
            previous_terrain = terrain
        observations.append(checkpoint)
        with history_path.open("a", encoding="utf-8") as stream:
            stream.write(json.dumps(checkpoint, separators=(",", ":")) + "\n")
            stream.flush()
            os.fsync(stream.fileno())

    configuration = {"mission": MISSION_T4, "entry": plan["entry"],
                     "trace": str(trace_path), "trace_sha256": trace_hash,
                     "input_transitions": plan["input_count"], "scheduler_mode": mode,
                     "scheduler_capacity": capacity,
                     "peak_page_transitions": requested_peak_page_transitions,
                     "trace_peak_page_transitions": trace_peak_page_transitions,
                     "trace_stop_tick": plan["stop_tick"], "requested_ticks": requested, "prefix": prefix}
    try:
        proc, sock, rsp, log = split.start_emulator(run_dir.name, rom, run_dir, sym)
        initial = {"screen": profile.u32(rsp, sym["screen"]), "frame": profile.u32(rsp, sym["frame"]),
                   "page": profile.u32(rsp, sym["race_front"] + split.FRONT_FIELDS["page"])}
        if initial != {"screen": SCREEN_TITLE, "frame": 0, "page": split.FRONT_HOME}:
            raise RuntimeError(f"fresh boot advanced away from HOME: {initial}")

        def write_page(events: list[tuple[int, int]]) -> None:
            if len(events) > capacity:
                raise RuntimeError(f"controller page exceeds ELF capacity ({len(events)}/{capacity})")
            blob = b"".join(struct.pack("<2I", tick, held) for tick, held in events)
            for offset in range(0, len(blob), 256):
                rsp.write(sym["demo_schedule_events"] + offset, blob[offset:offset + 256])
            rsp.write(sym["demo_schedule_count"], struct.pack("<I", len(events)))

        write_page(pages[0]["events"])
        rsp.write(sym["demo_schedule_index"], struct.pack("<I", 0))
        rsp.write(sym["demo_schedule_length"], struct.pack("<I", requested))
        rsp.write(sym["demo_schedule_active"], struct.pack("<I", 1))
        if rsp.cmd(f"z1,{sym['game_update'] & ~1:x},2") != "OK" or rsp.cmd(
                f"Z1,{sym['demo_replay_checkpoint'] & ~1:x},2") != "OK":
            raise RuntimeError("failed to install controller replay observer")
        rsp.tick()
        if profile.u32(rsp, sym["demo_schedule_tick"]) != 0:
            raise RuntimeError("scheduler did not stop before first controller input")

        previous = t4_snapshot(rsp, sym, sizes, bank, campaign_layout, t4_layout, trigger_layout)
        previous_clock = profile.hardware_clock(rsp)
        previous_cycles, previous_routes = profile_counters(rsp, sym)
        previous_buckets = list(struct.unpack("<8I", rsp.read(sym["demo_tick_buckets"], 32)))
        counted_frames = counted_cycles = 0
        record({"tick": 0, "host_tick": 0, "elapsed_hardware_cycles": 0, "frame_delta": 0,
                "counted_live_play_interval": False, "effective_fps": None,
                "profile_cycles_last_tick": previous_cycles, "route_counters": previous_routes,
                "route_interval": {key: 0 for key in previous_routes},
                "demo_last_cycles": profile.u32(rsp, sym["demo_last_cycles"]),
                "demo_peak_cycles": profile.u32(rsp, sym["demo_peak_cycles"]),
                "demo_tick_buckets": previous_buckets, **previous})
        capture(rsp, run_dir / "home.png")
        profile.write_progress(run_dir / "progress.json", profile=profile_label(plan["entry"]),
                               rom=rom, elf=elf, rom_sha256=rom_hash, elf_sha256=elf_hash,
                               configuration=configuration, requested_ticks=requested,
                               checkpoint_interval=CHECKPOINT_TICKS, latest_checkpoint=observations[0])
        tick = 0
        while tick < requested:
            rsp.tick()
            completed = profile.u32(rsp, sym["demo_schedule_tick"])
            expected = min(requested, tick + CHECKPOINT_TICKS)
            if completed != expected:
                raise RuntimeError(f"expected observer tick {expected}, got {completed}")
            page = pages[(completed - 1) // CHECKPOINT_TICKS]
            cursor = profile.u32(rsp, sym["demo_schedule_index"])
            count = profile.u32(rsp, sym["demo_schedule_count"])
            if cursor != count or count != len(page["events"]):
                raise RuntimeError(f"scheduler page incomplete at {completed}: {cursor}/{count}")
            if completed < requested:
                write_page(pages[completed // CHECKPOINT_TICKS]["events"])
            now = profile.hardware_clock(rsp)
            elapsed = (now - previous_clock) & 0xFFFFFFFF
            state = t4_snapshot(rsp, sym, sizes, bank, campaign_layout, t4_layout, trigger_layout)
            if completed >= plan["start"]["host_tick"]:
                if state.get("race_mode") != 0 or state.get("race_selected_mission") != MISSION_T4:
                    raise RuntimeError(f"normal HOME/briefing controls did not launch T4 by tick {completed}")
            frame_delta = (state["frame"] - previous["frame"]) & 0xFFFFFFFF
            counted = previous["screen"] == SCREEN_PLAY and state["screen"] == SCREEN_PLAY and frame_delta > 0
            fps = round(profile.CPU_HZ * frame_delta / max(1, elapsed), 2) if counted else None
            if counted:
                counted_frames += frame_delta
                counted_cycles += elapsed
            cycles, routes = profile_counters(rsp, sym)
            buckets = list(struct.unpack("<8I", rsp.read(sym["demo_tick_buckets"], 32)))
            bucket_delta = [(value - old) & 0xFFFFFFFF for value, old in zip(buckets, previous_buckets)]
            route_delta = scheduler.route_counter_deltas(routes, previous_routes)
            checkpoint = {"tick": completed, "host_tick": completed,
                          "elapsed_hardware_cycles": elapsed, "frame_delta": frame_delta,
                          "counted_live_play_interval": counted, "effective_fps": fps,
                          "profile_cycles_last_tick": cycles, "route_counters": routes,
                          "route_interval": route_delta,
                          "demo_last_cycles": profile.u32(rsp, sym["demo_last_cycles"]),
                          "demo_peak_cycles": profile.u32(rsp, sym["demo_peak_cycles"]),
                          "demo_tick_buckets": buckets, "demo_tick_bucket_interval": bucket_delta, **state}
            expected_trace_rows = [e for e in plan["periodic"] if e["host_tick"] == completed]
            if completed == plan["stop_tick"] and not prefix:
                expected_trace_rows = [plan["final"]]
            if completed in (1000, 10000, 30000, 50000, requested):
                checkpoint["capture"] = capture(rsp, run_dir / f"checkpoint-{completed:06d}.png")
            record(checkpoint)
            profile.write_progress(run_dir / "progress.json", profile=profile_label(plan["entry"]),
                                   rom=rom, elf=elf, rom_sha256=rom_hash, elf_sha256=elf_hash,
                                   configuration=configuration, requested_ticks=requested,
                                   checkpoint_interval=CHECKPOINT_TICKS, latest_checkpoint=checkpoint)
            for row in expected_trace_rows:
                compare_observation(row, state, row["kind"])
            print(f"T4 tick={completed}/{requested} frame={state['frame']} screen={state['screen']} "
                  f"fps={fps if fps is not None else 'n/a'}", flush=True)
            previous, previous_clock, previous_routes, previous_buckets = state, now, routes, buckets
            tick = completed

        final = observations[-1]
        status = "prefix" if prefix else plan["final"]["kind"]
        report = {"profile": profile_label(plan["entry"]), "entry": plan["entry"],
                  "status": status, "mission": MISSION_T4,
                  "rom": str(rom), "elf": str(elf), "trace": str(trace_path),
                  "rom_sha256": rom_hash, "elf_sha256": elf_hash, "trace_sha256": trace_hash,
                  "input_transitions": plan["input_count"], "scheduler_mode": mode,
                  "scheduler_capacity": capacity,
                  "peak_page_transitions": requested_peak_page_transitions,
                  "trace_peak_page_transitions": trace_peak_page_transitions,
                  "trace_stop_tick": plan["stop_tick"], "requested_ticks": requested,
                  "observed_ticks": final["tick"], "prefix": prefix,
                  "counted_live_play_frames": counted_frames,
                  "mean_live_play_effective_fps": round(profile.CPU_HZ * counted_frames / max(1, counted_cycles), 2),
                  "timer_method": "chained GBA timers sampled every 1000 ticks and at exact final tick",
                  "fps_exclusions": "Only positive frame deltas between PLAY checkpoints count.",
                  "final": final, "checkpoints": observations,
                  "checkpoint_history": str(history_path), "captures": {"home": str(run_dir / "home.png")},
                  "verified_win": False}
        if not prefix:
            try:
                verify_final(plan["final"], final)
            except Exception as exc:
                report.update({"status": "verification_failed", "verification_failed": str(exc)})
                save_report(report)
                profile.write_progress(run_dir / "progress.json", profile=profile_label(plan["entry"]),
                                       rom=rom, elf=elf, rom_sha256=rom_hash, elf_sha256=elf_hash,
                                       configuration=configuration, requested_ticks=requested,
                                       checkpoint_interval=CHECKPOINT_TICKS, latest_checkpoint=final,
                                       status="verification_failed", final_report=str(report_path))
                raise
            report["verified_win"] = report["status"] == "won"
        save_report(report)
        profile.write_progress(run_dir / "progress.json", profile=profile_label(plan["entry"]),
                               rom=rom, elf=elf, rom_sha256=rom_hash, elf_sha256=elf_hash,
                               configuration=configuration, requested_ticks=requested,
                               checkpoint_interval=CHECKPOINT_TICKS, latest_checkpoint=final,
                               status=status, final_report=str(report_path))
        print(json.dumps({"status": report["status"], "observed_ticks": final["tick"],
                          "report": str(report_path)}, indent=2), flush=True)
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
    parser.add_argument("--rom", type=Path, help="T4 internal or campaign-menu ROM override")
    parser.add_argument("--elf", type=Path, help="T4 internal or campaign-menu ELF override")
    parser.add_argument("--ticks", type=int, help="replay a positive controller-trace prefix")
    return parser


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    try:
        run(args.trace, args.output, args.rom, args.elf, args.ticks)
    except (OSError, ValueError, RuntimeError, KeyError, json.JSONDecodeError,
            subprocess.CalledProcessError, subprocess.TimeoutExpired) as exc:
        print(f"sc64_t4_controller_replay: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
