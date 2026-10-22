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
DEFAULT_TRACE = ROOT / ".local/sc64/t5/controller-probe.json"
DEFAULT_OUTPUT = ROOT / "build/emulator/sc64-t5-controller-replay"
DEFAULT_ROM = ROOT / "build/StarCraft-T5-Controller-Replay.gba"
DEFAULT_ELF = ROOT / "build/starcraft-t5-controller-replay.elf"
TRACE_CHECKPOINTS = frozenset({
    "start", "opening", "rescue-attempt", "command-center-attempt",
    "command-center-approach-cleared", "kerrigan-center-correction",
    "enemy-grant-attempt", "enemy-ai-started", "base-contact-attempt",
    "enemy-base-attack", "force2-area-attack", "paid-production-orders", "periodic",
    "before-starport-tech", "starport-complete", "control-tower-complete",
    "dropship-complete", "dropship-loaded", "dropship-landed", "dropship-unloaded",
    "dropship-destroyed-in-transit", "dropship-landing-assault", "dropship-assault-result",
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


TECH_SNAPSHOT_FIELDS = (
    "starport_slot", "starport_raw", "starport_complete",
    "control_tower_slot", "control_tower_raw", "control_tower_complete",
)
TRANSPORT_SNAPSHOT_FIELDS = (
    "transport_ship_slot", "transport_ship_raw", "transport_ship_alive",
    "transport_ship_hp", "transport_ship_xy", "transport_cargo_space",
    "transport_landing", "transport_landed_count", "transport_loaded",
    "transport_landed", "transport_passenger_snapshot_tick", "transport_passengers",
)


def _bounded_pair(value: object, label: str, *, allow_sentinel: bool) -> None:
    if (not isinstance(value, list) or len(value) != 2 or
            any(type(part) is not int for part in value)):
        raise ValueError(f"{label} must be a two-integer coordinate")
    if allow_sentinel and value == [-1, -1]:
        return
    if any(part < 0 or part >= 96 * 16 for part in value):
        raise ValueError(f"{label} is outside the T5 map bounds")


def validate_aux_snapshot(row: dict, label: str) -> None:
    """Validate optional tech/cargo evidence before consulting live debugger state.

    Older trace files omit these fields. New runner traces include both groups
    on every observation, using -1/empty sentinels before the corresponding
    objects exist. At checkpoints, the replay compares these summaries against
    the decoded building and unit pools rather than trusting host-side values.
    """
    for group_name, fields in (("tech", TECH_SNAPSHOT_FIELDS),
                               ("transport", TRANSPORT_SNAPSHOT_FIELDS)):
        present = [field in row for field in fields]
        if any(present) and not all(present):
            missing = fields[present.index(False)]
            raise ValueError(f"{label} {group_name} snapshot is missing {missing}")

    if all(field in row for field in TECH_SNAPSHOT_FIELDS):
        for prefix in ("starport", "control_tower"):
            slot, raw, complete = (row[f"{prefix}_slot"], row[f"{prefix}_raw"],
                                   row[f"{prefix}_complete"])
            if type(slot) is not int or not (-1 <= slot < 64):
                raise ValueError(f"{label} {prefix}_slot is outside building bounds")
            if type(raw) is not int or not (-1 <= raw <= 255):
                raise ValueError(f"{label} {prefix}_raw is outside raw-type bounds")
            if type(complete) is not int or complete not in (0, 1):
                raise ValueError(f"{label} {prefix}_complete must be boolean")
            if slot == -1 and (raw != -1 or complete != 0):
                raise ValueError(f"{label} absent {prefix} must use canonical sentinels")
            if slot >= 0 and raw < 0:
                raise ValueError(f"{label} present {prefix} needs a raw type")

    if all(field in row for field in TRANSPORT_SNAPSHOT_FIELDS):
        slot, raw = row["transport_ship_slot"], row["transport_ship_raw"]
        alive, hp = row["transport_ship_alive"], row["transport_ship_hp"]
        cargo, landed = row["transport_cargo_space"], row["transport_landed_count"]
        if type(slot) is not int or not (-1 <= slot < 160):
            raise ValueError(f"{label} transport_ship_slot is outside unit bounds")
        if type(raw) is not int or not (-1 <= raw <= 255):
            raise ValueError(f"{label} transport_ship_raw is outside raw-type bounds")
        if type(alive) is not int or alive not in (0, 1):
            raise ValueError(f"{label} transport_ship_alive must be boolean")
        if type(hp) is not int or not (-1 <= hp <= 100000):
            raise ValueError(f"{label} transport_ship_hp is outside bounds")
        if type(cargo) is not int or not (-1 <= cargo <= 8):
            raise ValueError(f"{label} transport_cargo_space is outside capacity bounds")
        if type(landed) is not int or not (-1 <= landed <= 160):
            raise ValueError(f"{label} transport_landed_count is outside actor bounds")
        _bounded_pair(row["transport_ship_xy"], f"{label} transport_ship_xy",
                      allow_sentinel=True)
        _bounded_pair(row["transport_landing"], f"{label} transport_landing",
                      allow_sentinel=True)
        loaded = row["transport_loaded"]
        if not isinstance(loaded, list) or len(loaded) > 8:
            raise ValueError(f"{label} transport_loaded must contain at most eight actors")
        seen_slots = set()
        for index, item in enumerate(loaded):
            if not isinstance(item, dict) or set(item) != {"slot", "raw_type"}:
                raise ValueError(f"{label} transport_loaded[{index}] has invalid shape")
            actor_slot, actor_raw = item["slot"], item["raw_type"]
            if type(actor_slot) is not int or not 0 <= actor_slot < 160:
                raise ValueError(f"{label} transport_loaded[{index}] slot is outside unit bounds")
            if type(actor_raw) is not int or not 0 <= actor_raw <= 255:
                raise ValueError(f"{label} transport_loaded[{index}] raw type is invalid")
            if actor_slot in seen_slots:
                raise ValueError(f"{label} transport_loaded contains duplicate slots")
            seen_slots.add(actor_slot)
        landed_manifest = row["transport_landed"]
        if not isinstance(landed_manifest, list) or len(landed_manifest) > 8:
            raise ValueError(f"{label} transport_landed must contain at most eight actors")
        landed_slots = set()
        for index, item in enumerate(landed_manifest):
            if not isinstance(item, dict) or set(item) != {"slot", "raw_type"}:
                raise ValueError(f"{label} transport_landed[{index}] has invalid shape")
            actor_slot, actor_raw = item["slot"], item["raw_type"]
            if type(actor_slot) is not int or not 0 <= actor_slot < 160:
                raise ValueError(f"{label} transport_landed[{index}] slot is outside unit bounds")
            if type(actor_raw) is not int or not 0 <= actor_raw <= 255:
                raise ValueError(f"{label} transport_landed[{index}] raw type is invalid")
            if actor_slot in landed_slots:
                raise ValueError(f"{label} transport_landed contains duplicate slots")
            landed_slots.add(actor_slot)
        snapshot_tick = row["transport_passenger_snapshot_tick"]
        if (type(snapshot_tick) is not int or snapshot_tick < 0 or
                snapshot_tick > row.get("host_tick", -1)):
            raise ValueError(f"{label} passenger snapshot tick is outside observation time")
        passengers = row["transport_passengers"]
        if not isinstance(passengers, list) or len(passengers) > 8:
            raise ValueError(f"{label} transport_passengers must contain at most eight actors")
        passenger_slots = set()
        for index, item in enumerate(passengers):
            if (not isinstance(item, dict) or
                    set(item) != {"slot", "original_raw_type", "alive", "loaded", "xy"}):
                raise ValueError(f"{label} transport_passengers[{index}] has invalid shape")
            actor_slot, actor_raw = item["slot"], item["original_raw_type"]
            if type(actor_slot) is not int or not 0 <= actor_slot < 160:
                raise ValueError(f"{label} transport_passengers[{index}] slot is outside unit bounds")
            if type(actor_raw) is not int or not 0 <= actor_raw <= 255:
                raise ValueError(f"{label} transport_passengers[{index}] raw type is invalid")
            if type(item["alive"]) is not int or item["alive"] not in (0, 1):
                raise ValueError(f"{label} transport_passengers[{index}] alive must be boolean")
            if type(item["loaded"]) is not int or item["loaded"] not in (0, 1):
                raise ValueError(f"{label} transport_passengers[{index}] loaded must be boolean")
            _bounded_pair(item["xy"], f"{label} transport_passengers[{index}].xy",
                          allow_sentinel=not item["alive"])
            if not item["alive"] and (item["loaded"] or item["xy"] != [-1, -1]):
                raise ValueError(f"{label} dead passenger must use canonical state")
            if actor_slot in passenger_slots:
                raise ValueError(f"{label} transport_passengers contains duplicate slots")
            passenger_slots.add(actor_slot)
        if slot == -1:
            if (raw != -1 or alive != 0 or hp != -1 or cargo != -1 or landed != -1 or
                    row["transport_ship_xy"] != [-1, -1] or loaded or landed_manifest or passengers):
                raise ValueError(f"{label} absent transport must use canonical sentinels")
        elif raw < 0:
            raise ValueError(f"{label} present transport needs a raw type")


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
            validate_aux_snapshot(row, "start")
            start = row
        elif kind in TRACE_CHECKPOINTS - {"start"}:
            if start is None or final is not None:
                raise ValueError(f"{kind} checkpoint must follow start and precede final")
            require_int_fields(row, OBS_FIELDS, kind)
            validate_aux_snapshot(row, kind)
            if checkpoints and tick < checkpoints[-1]["host_tick"]:
                raise ValueError("T5 observation checkpoints must be ordered")
            checkpoints.append(row)
        elif kind in ("won", "lost", "limit"):
            if start is None or final is not None or i != len(rows) - 1:
                raise ValueError("T5 trace must end with exactly one final outcome observation")
            require_int_fields(row, OBS_FIELDS, "final")
            validate_aux_snapshot(row, "final")
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


_TRANSPORT_SPACE = {
    0: 1, 1: 1, 7: 1, 10: 1, 15: 1, 16: 1, 20: 1, 32: 1, 34: 1, 37: 1, 41: 1,
    2: 2, 3: 2, 19: 2, 38: 2, 5: 4, 23: 4,
}
_ORDER_TRANSPORTED = 8
_MAX_UNITS = 160
_BUILDING_TYPES = {"starport": 114, "control_tower": 115}


def _read_signed(row: bytes, offsets: dict[str, int], field: str) -> int:
    if field not in offsets or offsets[field] + 4 > len(row):
        raise RuntimeError(f"T5 replay ELF DWARF lacks unit/building field {field}")
    return int.from_bytes(row[offsets[field]:offsets[field] + 4], "little", signed=True)


def _row_alive(row: dict) -> bool:
    """Profile entity lists contain live actors only; unit-pool rows carry alive explicitly."""
    return bool(row.get("alive", True))


def replay_aux_state(rsp, sym: dict[str, int], bank: dict, buildings: dict) -> dict:
    """Read current tech, transport, and passenger state from the ARM world."""
    building_rows = buildings["entities"]
    result = {}
    for prefix, raw_type in _BUILDING_TYPES.items():
        matches = sorted((row for row in building_rows
                          if row.get("owner") == 0 and row.get("raw_type") == raw_type),
                         key=lambda row: row["slot"])
        if matches:
            row = matches[0]
            result[f"{prefix}_slot"] = row["slot"]
            result[f"{prefix}_raw"] = row["raw_type"]
            result[f"{prefix}_complete"] = int(not row["under_construction"])
        else:
            result[f"{prefix}_slot"] = -1
            result[f"{prefix}_raw"] = -1
            result[f"{prefix}_complete"] = 0
    # Keep the complete decoded building pool private to this observer so a
    # host checkpoint can identify its tracked actor by exact slot. The public
    # scalar summary above remains for report compatibility.
    result["_building_rows"] = building_rows

    unit_capacity = bank["unit_capacity"]
    stride = bank["unit_stride_bytes"]
    offsets = bank["unit_offsets"]
    if "u" not in sym:
        raise RuntimeError("T5 replay ELF has no unit bank symbol")
    pool_size = unit_capacity * stride
    if unit_capacity != _MAX_UNITS:
        raise RuntimeError(f"T5 replay expects {_MAX_UNITS} unit slots, got {unit_capacity}")
    pool = rsp.read(sym["u"], pool_size)
    records = []
    for slot in range(unit_capacity):
        row = pool[slot * stride:(slot + 1) * stride]
        records.append({
            "slot": slot,
            "x": _read_signed(row, offsets, "x"),
            "y": _read_signed(row, offsets, "y"),
            "hp": _read_signed(row, offsets, "hp"),
            "owner": _read_signed(row, offsets, "owner"),
            "type": _read_signed(row, offsets, "type"),
            "alive": bool(_read_signed(row, offsets, "alive")),
            "order": _read_signed(row, offsets, "order"),
            "tx": _read_signed(row, offsets, "tx"),
            "ty": _read_signed(row, offsets, "ty"),
            "target": _read_signed(row, offsets, "target"),
            "raw_type": _read_signed(row, offsets, "raw_type"),
        })
    carrier = next((row for row in records
                    if row["owner"] == 0 and row["raw_type"] == 11), None)
    result.update({
        "transport_ship_slot": carrier["slot"] if carrier else -1,
        "transport_ship_raw": carrier["raw_type"] if carrier else -1,
        "transport_ship_alive": int(carrier["alive"]) if carrier else 0,
        "transport_ship_hp": carrier["hp"] if carrier else -1,
        "transport_ship_xy": [carrier["x"], carrier["y"]] if carrier else [-1, -1],
        "transport_cargo_space": -1,
        "transport_landing": [-1, -1],
        "transport_landed_count": -1,
        "transport_loaded": [],
        "transport_landed": [],
    })

    if carrier and carrier["alive"]:
        loaded = sorted((row for row in records if row["alive"] and
                         row["order"] == _ORDER_TRANSPORTED and row["target"] == carrier["slot"]),
                        key=lambda row: row["slot"])
        result["transport_loaded"] = [
            {"slot": row["slot"], "raw_type": row["raw_type"]} for row in loaded
        ]
        spaces = [_TRANSPORT_SPACE.get(row["raw_type"]) for row in loaded]
        result["transport_cargo_space"] = (sum(spaces) if all(space is not None for space in spaces)
                                            and sum(spaces) <= 8 else -1)
        result["transport_landing"] = ([carrier["tx"], carrier["ty"]]
                                        if carrier["tx"] >= 0 and carrier["ty"] >= 0 else [-1, -1])
    else:
        result["transport_landed_count"] = -1
        result["transport_cargo_space"] = -1
        result["transport_loaded"] = []
    # Keep decoded pool rows internal to the observer. Host passenger manifests
    # are used only as identities at their recorded snapshot tick, never as
    # replay state or debugger inputs.
    result["_unit_rows"] = records
    result["_carrier_slot"] = carrier["slot"] if carrier else -1
    return result


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
    state["kerrigan_alive"] = int(any(_row_alive(row) and row["raw_type"] == 16
                                     for row in state["units"]["entities"]))
    state["raynor_alive"] = int(any(_row_alive(row) and row["raw_type"] == 19
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
    state["replay_aux"] = replay_aux_state(rsp, sym, bank, state["buildings"])
    return state


def compare_observation(expected: dict, actual: dict, label: str) -> None:
    for field in OBS_FIELDS:
        if field in expected and expected[field] != actual.get(field):
            raise RuntimeError(f"{label} {field} mismatch: expected {expected[field]!r}, "
                               f"observed {actual.get(field)!r}")
    compare_aux_observation(expected, actual, label)


def compare_aux_observation(expected: dict, actual: dict, label: str) -> None:
    if not any(field in expected for field in (*TECH_SNAPSHOT_FIELDS, *TRANSPORT_SNAPSHOT_FIELDS)):
        return
    observed = actual.get("replay_aux")
    if not isinstance(observed, dict):
        raise RuntimeError(f"{label} debugger observation has no replay auxiliary state")
    # Harness slot -1 means its checkpoint code has not begun tracking that
    # object yet; the object may already exist in the map. Once tracked, use
    # the recorded slot as the stable identity rather than selecting the first
    # matching actor from a pool that may contain several.
    for prefix in ("starport", "control_tower"):
        expected_slot = expected.get(f"{prefix}_slot", -1)
        if expected_slot < 0:
            continue
        rows = observed.get("_building_rows")
        if not isinstance(rows, list):
            raise RuntimeError(f"{label} debugger observation has no decoded building pool")
        row = next((item for item in rows if item.get("slot") == expected_slot), None)
        raw_field = f"{prefix}_raw"
        complete_field = f"{prefix}_complete"
        expected_raw = expected.get(raw_field)
        if (row is None or row.get("owner") != 0 or
                row.get("raw_type") != expected_raw or
                int(not row.get("under_construction")) != expected.get(complete_field)):
            raise RuntimeError(f"{label} {prefix} identity/state mismatch at tracked slot "
                               f"{expected_slot}: expected raw={expected_raw!r}, "
                               f"complete={expected.get(complete_field)!r}")
    expected_carrier = expected.get("transport_ship_slot", -1)
    if expected_carrier < 0:
        return
    rows = observed.get("_unit_rows")
    if not isinstance(rows, list):
        raise RuntimeError(f"{label} debugger observation has no decoded unit pool")
    units = {row.get("slot"): row for row in rows}
    carrier = units.get(expected_carrier)
    expected_raw = expected.get("transport_ship_raw")
    if carrier is None or carrier.get("owner") != 0 or carrier.get("raw_type") != expected_raw:
        raise RuntimeError(f"{label} transport carrier identity mismatch at tracked slot "
                           f"{expected_carrier}: expected raw={expected_raw!r}")
    actual_ship = {
        "transport_ship_slot": expected_carrier,
        "transport_ship_raw": carrier["raw_type"],
        "transport_ship_alive": int(carrier["alive"]),
        "transport_ship_hp": carrier["hp"],
        "transport_ship_xy": [carrier["x"], carrier["y"]],
        "transport_landing": ([carrier["tx"], carrier["ty"]]
                              if carrier["alive"] and carrier["tx"] >= 0 and carrier["ty"] >= 0
                              else [-1, -1]),
    }
    loaded_rows = sorted((row for row in rows if row["alive"] and
                          row["order"] == _ORDER_TRANSPORTED and
                          row["target"] == expected_carrier), key=lambda row: row["slot"])
    actual_ship["transport_loaded"] = [
        {"slot": row["slot"], "raw_type": row["raw_type"]} for row in loaded_rows
    ]
    spaces = [_TRANSPORT_SPACE.get(row["raw_type"]) for row in loaded_rows]
    actual_ship["transport_cargo_space"] = (
        sum(spaces) if carrier["alive"] and all(space is not None for space in spaces)
        and sum(spaces) <= 8 else -1
    )
    for field, value in actual_ship.items():
        # In the controller trace this field is a frozen planned landing
        # coordinate, not a live destination at every later observation.
        # It is compared against the exact carrier only on landed checkpoints.
        if field == "transport_landing":
            continue
        if field in expected and expected[field] != value:
            raise RuntimeError(f"{label} {field} mismatch: expected {expected[field]!r}, "
                               f"observed {value!r}")
    # The host trace freezes passenger status at landing. Compare that frozen
    # manifest only at the exact source observation tick; later rows can show
    # legitimate movement or deaths after the landing snapshot.
    source_tick = expected.get("source_host_tick", expected.get("host_tick"))
    if ("transport_passenger_snapshot_tick" in expected and
            expected["transport_passenger_snapshot_tick"] == source_tick):
        manifest = expected["transport_passengers"]
        rows = units
        carrier_slot = expected_carrier
        actual_passengers = []
        for item in manifest:
            unit = rows.get(item["slot"])
            alive = bool(unit and unit["alive"] and unit["raw_type"] == item["original_raw_type"])
            loaded = bool(alive and unit["order"] == _ORDER_TRANSPORTED and
                          unit["target"] == carrier_slot)
            actual_passengers.append({
                "slot": item["slot"], "original_raw_type": item["original_raw_type"],
                "alive": int(alive), "loaded": int(loaded),
                "xy": [unit["x"], unit["y"]] if alive else [-1, -1],
            })
        if manifest != actual_passengers:
            raise RuntimeError(f"{label} transport passenger snapshot mismatch: "
                               f"expected {manifest!r}, observed {actual_passengers!r}")
    if (expected.get("kind") in ("dropship-landed", "dropship-unloaded") and
            "transport_landed" in expected):
        carrier_xy = actual_ship["transport_landing"]
        if expected.get("transport_landing") != carrier_xy:
            raise RuntimeError(f"{label} transport_landing mismatch: "
                               f"expected {expected.get('transport_landing')!r}, observed {carrier_xy!r}")
        manifest = expected.get("transport_passengers", [])
        rows = units
        landed = []
        if carrier_xy != [-1, -1]:
            for item in manifest:
                unit = rows.get(item["slot"])
                if (unit and unit["alive"] and unit["raw_type"] == item["original_raw_type"] and
                        unit["owner"] == 0 and
                        not (unit["order"] == _ORDER_TRANSPORTED and
                             unit["target"] == expected_carrier) and
                        (unit["x"] - carrier_xy[0]) ** 2 +
                        (unit["y"] - carrier_xy[1]) ** 2 < 72 ** 2):
                    landed.append({"slot": item["slot"], "raw_type": unit["raw_type"]})
        if expected["transport_landed"] != landed:
            raise RuntimeError(f"{label} transport_landed mismatch: "
                               f"expected {expected['transport_landed']!r}, observed {landed!r}")
        if expected["transport_landed_count"] != len(landed):
            raise RuntimeError(f"{label} transport_landed_count mismatch: "
                               f"expected {expected['transport_landed_count']}, observed {len(landed)}")


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
    rom = rom_path or DEFAULT_ROM
    elf = elf_path or DEFAULT_ELF
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
            public = dict(checkpoint)
            aux = public.get("replay_aux")
            if isinstance(aux, dict):
                public["replay_aux"] = {key: value for key, value in aux.items()
                                        if not key.startswith("_")}
            stream.write(json.dumps(public, separators=(",", ":")) + "\n")
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
        previous_clock = profile.hardware_clock(rsp)
        cycle_names = tuple(getattr(profile, "PROFILE_CYCLE_NAMES", ()))
        cycle_values = {name: profile.u32(rsp, sym[name])
                        for name in cycle_names if name in sym}
        previous_buckets = (list(struct.unpack("<8I", rsp.read(sym["demo_tick_buckets"], 32)))
                            if "demo_tick_buckets" in sym else None)
        last_cycles = {name: profile.u32(rsp, sym[name])
                       for name in ("demo_last_cycles", "demo_peak_cycles") if name in sym}
        record({"tick": 0, "host_tick": 0, "entry": "campaign-menu",
                "elapsed_hardware_cycles": 0, "frame_delta": 0,
                "counted_live_play_interval": False, "effective_fps": None,
                "profile_cycles_last_tick": cycle_values,
                "profile_cycles_scope": "last-tick values; absent diagnostic symbols are unavailable",
                "profile_cycles_missing": [name for name in cycle_names if name not in sym],
                "diagnostic_cycle_symbols_available": any(name.startswith("sc64_update_") or name.startswith("sc64_order_")
                                                           for name in cycle_values),
                "demo_tick_buckets": previous_buckets,
                "demo_last_cycles": last_cycles.get("demo_last_cycles"),
                "demo_peak_cycles": last_cycles.get("demo_peak_cycles"),
                **state})
        capture(rsp, run_dir / "home.png")

        previous_state = state
        counted_frames = counted_cycles = 0
        worst_live_interval = None
        boundaries = [page["end_tick"] for page in pages]
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
            now_clock = profile.hardware_clock(rsp)
            elapsed_cycles = (now_clock - previous_clock) & scheduler.MAX_U32
            frame_delta = (state["frame"] - previous_state["frame"]) & scheduler.MAX_U32
            counted = (previous_state["screen"] == SCREEN_PLAY and
                       state["screen"] == SCREEN_PLAY and frame_delta > 0)
            fps = round(profile.CPU_HZ * frame_delta / max(1, elapsed_cycles), 2) if counted else None
            if counted:
                counted_frames += frame_delta
                counted_cycles += elapsed_cycles
                interval = {"tick": expected_tick, "frame_delta": frame_delta,
                            "elapsed_hardware_cycles": elapsed_cycles,
                            "effective_fps": fps}
                if worst_live_interval is None or fps < worst_live_interval["effective_fps"]:
                    worst_live_interval = interval
            cycles = {name: profile.u32(rsp, sym[name])
                      for name in cycle_names if name in sym}
            buckets = (list(struct.unpack("<8I", rsp.read(sym["demo_tick_buckets"], 32)))
                       if "demo_tick_buckets" in sym else None)
            bucket_delta = ([(value - old) & scheduler.MAX_U32
                             for value, old in zip(buckets, previous_buckets)]
                            if buckets is not None and previous_buckets is not None else None)
            now = time.monotonic()
            if replay_started is None:
                replay_started = now
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
            checkpoint = {"tick": expected_tick, "host_tick": expected_tick,
                          "elapsed_hardware_cycles": elapsed_cycles,
                          "frame_delta": frame_delta,
                          "counted_live_play_interval": counted,
                          "effective_fps": fps,
                          "profile_cycles_last_tick": cycles,
                          "profile_cycles_scope": "last-tick values; absent diagnostic symbols are unavailable",
                          "profile_cycles_missing": [name for name in cycle_names if name not in sym],
                          "diagnostic_cycle_symbols_available": any(name.startswith("sc64_update_") or name.startswith("sc64_order_")
                                                                     for name in cycles),
                          "demo_tick_buckets": buckets,
                          "demo_tick_bucket_interval": bucket_delta,
                          "demo_last_cycles": (profile.u32(rsp, sym["demo_last_cycles"])
                                                if "demo_last_cycles" in sym else None),
                          "demo_peak_cycles": (profile.u32(rsp, sym["demo_peak_cycles"])
                                                if "demo_peak_cycles" in sym else None),
                          "worst_live_interval_so_far": worst_live_interval,
                          **state}
            checkpoint["wall_seconds"] = round(now - replay_started, 6)
            record(checkpoint)
            previous_state = state
            previous_clock, cycle_values, previous_buckets = now_clock, cycles, buckets

        final = observations[-1]
        final_public = dict(final)
        if isinstance(final_public.get("replay_aux"), dict):
            final_public["replay_aux"] = {
                key: value for key, value in final_public["replay_aux"].items()
                if not key.startswith("_")
            }
        status = "prefix" if prefix else plan["final"]["kind"]
        replay_wall = (time.monotonic() - replay_started) if replay_started is not None else 0.0
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
                  "counted_live_play_frames": counted_frames,
                  "counted_live_play_hardware_cycles": counted_cycles,
                  "mean_live_play_effective_fps": round(profile.CPU_HZ * counted_frames /
                                                         max(1, counted_cycles), 2),
                  "worst_live_play_interval": worst_live_interval,
                  "timer_method": "chained GBA timers sampled at each 1000-tick or trace observer boundary",
                  "fps_exclusions": "Only positive frame deltas between PLAY observations count; HOME/menu intervals are excluded.",
                  "checkpoint_history": str(history_path), "captures": {"home": str(run_dir / "home.png")},
                  "final": final_public, "play_frames": play_frames,
                  "replay_wall_seconds": round(replay_wall, 3),
                  "last_tick_profile_counters": final.get("profile_cycles_last_tick", {}),
                  "last_tick_demo_tick_buckets": final.get("demo_tick_buckets"),
                  "completed": False, "final_is_unfinished_limit": status == "limit",
                  "final": final_public, "verified_win": False}
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
