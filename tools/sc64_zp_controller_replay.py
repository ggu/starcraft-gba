#!/usr/bin/env python3
"""Replay private staged Zerg/Protoss controller-win traces using only scheduled inputs.

ROM/ELF catalog metadata and debugger symbols are checked before launch. The
replay writes the controller queue and scheduler metadata; game/model state is
read through symbols and DWARF at the compiled scheduler checkpoint period.
Use --validate-only for offline trace-contract and controller-edge checks.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import socket
import struct
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

SCREEN_TITLE, SCREEN_PLAY, SCREEN_RESULT = 0, 1, 3
KEY_START = 8
MISSION_BY_PROFILE = {"zerg-z4": 15, "protoss-p4": 23, "zerg-z5": 16,
                      "zerg-z6": 17, "zerg-z7": 18,
                      "protoss-p5": 24, "protoss-p6": 25}
FACTION_BY_PROFILE = {name: ("zerg" if name.startswith("zerg") else "protoss")
                      for name in MISSION_BY_PROFILE}
MODEL_TYPES = {"zerg-z4": "Sc64Z4State", "protoss-p4": "Sc64P4State",
               "zerg-z5": "Sc64Z5State", "protoss-p5": "Sc64P5State",
               "zerg-z6": "Sc64Z6State", "protoss-p6": "Sc64P6State",
               "zerg-z7": "Sc64Z7State"}
RUNTIME_TYPES = {"zerg-z4": "Sc64Z4RuntimeState", "protoss-p4": "Sc64P4RuntimeState",
                 "zerg-z5": "Sc64Z5RuntimeState", "protoss-p5": "Sc64P5RuntimeState",
                 "zerg-z6": "Sc64Z6RuntimeState", "protoss-p6": "Sc64P6RuntimeState",
                 "zerg-z7": "Sc64Z7RuntimeState"}
PERSISTENT_TYPES = {"zerg-z5": "Sc64Z5PersistentState", "zerg-z6": "Sc64Z6PersistentState", "protoss-p5": "Sc64P5PersistentState",
                    "protoss-p6": "Sc64P6PersistentState",
                    "zerg-z7": "Sc64Z7PersistentState"}
RUNTIME_SYMBOLS = {"zerg-z4": "race_z4", "protoss-p4": "race_p4",
                   "zerg-z5": "race_z5", "zerg-z6": "race_z6", "protoss-p5": "race_p5",
                   "protoss-p6": "race_p6", "zerg-z7": "race_z7"}
DEFAULT_TRACE = ROOT / ".local/sc64/z4-controller-win.json"
DEFAULT_TRACES = {"zerg-z4": DEFAULT_TRACE,
                 "protoss-p4": ROOT / ".local/sc64/p4-controller-win.json",
                 "zerg-z5": ROOT / ".local/sc64/z5-controller-win.json",
                 "zerg-z6": ROOT / ".local/sc64/z6-controller-win.json",
                 "protoss-p5": ROOT / ".local/sc64/p5-controller-win.json",
                 "protoss-p6": ROOT / ".local/sc64/p6-controller-win.json",
                 "zerg-z7": ROOT / ".local/build/z7-controller/controller-trace.json"}
DEFAULT_OUTPUT = ROOT / "build/emulator/zp-controller-replay"
CHECKPOINT_TICKS = 1000
MENU_PREFIX = ((1, 1), (2, 0), (3, 16), (4, 0), (5, 128),
               (6, 0), (7, 1), (8, 0), (9, KEY_START))

FIFTH_MENU_PREFIX = tuple((tick, 16 if tick == 5 else held)
                          for tick, held in MENU_PREFIX)
SIXTH_MENU_PREFIX = ((1, 1), (2, 0), (3, 16), (4, 0), (5, 16), (6, 0),
                     (7, 128), (8, 0), (9, 1), (10, 0), (11, KEY_START), (12, 0))
SEVENTH_MENU_PREFIX = ((1, 1), (2, 0), (3, 16), (4, 0), (5, 16), (6, 0),
                       (7, 16), (8, 0), (9, 1), (10, 0), (11, KEY_START), (12, 0))
Z7_ACTION_COUNTS = (1, 3, 1, 5, 2, 1, 1, 1)
P6_ACTION_COUNTS = (1, 1, 2, 2, 2, 2, 2, 2, 5, 2, 2, 8, 8,
                    1, 7, 12, 5, 2, 3, 3, 7, 12, 3, 1, 3, 1)
Z6_ACTION_COUNTS = (1, 2, 1, 4, 2, 13, 5, 18, 2)
P6_SWITCH_MASK = 0x3B
P6_SPRITE_ONLY_DOODAD_MASK = (0x01, 0xF0, 0x9B, 0, 0)


def storm_bytes_valid(value: object) -> bool:
    """Validate the bounded four-effect 30Hz policy's complete saved bytes."""
    if (not isinstance(value, list) or len(value) != 32 or
            any(type(x) is not int or not 0 <= x <= 255 for x in value)):
        return False
    for x, y, age, left, active in struct.iter_unpack("<HHHBB", bytes(value)):
        if active == 0:
            if x or y or age or left:
                return False
        elif active == 1:
            done = 0 if age < 20 else 1 + (age - 20) // 10
            if x >= 2048 or y >= 2048 or done >= 8 or left != 8 - done:
                return False
        else:
            return False
    return True


def validate_trace(trace: object, capacity: int, profile_name: str,
                   *, windowed: bool = True, window_ticks: int = CHECKPOINT_TICKS,
                   attempt_prefix: bool = False, prefix_ticks: int | None = None) -> dict:
    """Strictly validate an authenticated staged campaign trace before emulator launch."""
    if profile_name not in MISSION_BY_PROFILE:
        raise ValueError("unknown staged Zerg/Protoss profile")
    if attempt_prefix and profile_name != "zerg-z7":
        raise ValueError("--attempt-prefix is supported only for zerg-z7")
    if prefix_ticks is not None and (not attempt_prefix or type(prefix_ticks) is not int or prefix_ticks < 1):
        raise ValueError("attempt prefix ticks must be a positive Z7 prefix length")
    if type(capacity) is not int or capacity < 1:
        raise ValueError("controller queue capacity must be positive")
    if type(window_ticks) is not int or window_ticks not in (250, CHECKPOINT_TICKS):
        raise ValueError("controller checkpoint period must be 250 or 1000 ticks")
    if not isinstance(trace, dict) or type(trace.get("schema")) is not int or trace["schema"] != 1:
        raise ValueError("trace schema must be an object with schema=1")
    mission = MISSION_BY_PROFILE[profile_name]
    if type(trace.get("mission")) is not int or trace["mission"] != mission:
        raise ValueError(f"trace mission must be global ID {mission}")
    if trace.get("profile") != profile_name:
        raise ValueError(f"trace profile must be {profile_name}")
    if not isinstance(trace.get("policy"), str) or not trace["policy"]:
        raise ValueError("trace policy must be a nonempty string")
    rows = trace.get("events")
    if not isinstance(rows, list) or not rows:
        raise ValueError("trace events must be a nonempty array")

    inputs: list[tuple[int, int]] = []
    transitions: list[tuple[int, int]] = []
    previous_tick = previous_held = previous_event_tick = 0
    start = final = None
    start_input_tick = None
    start_transition_count = None
    checkpoints: list[dict] = []
    for index, row in enumerate(rows):
        if not isinstance(row, dict):
            raise ValueError(f"trace event {index} must be an object")
        kind, tick = row.get("kind"), row.get("host_tick")
        if type(tick) is not int or not 1 <= tick <= 0xFFFFFFFF or tick < previous_event_tick:
            raise ValueError(f"trace event {index} has invalid or unordered host_tick")
        previous_event_tick = tick
        if kind == "input":
            held, pressed = row.get("held"), row.get("pressed")
            if tick <= previous_tick:
                raise ValueError(f"input event {index} is not strictly ordered")
            if type(held) is not int or held < 0 or held & ~scheduler.KEY_MASK:
                raise ValueError(f"input event {index} held bits are outside 0x3ff")
            if held == previous_held:
                raise ValueError(f"input event {index} is not a held-state transition")
            if type(pressed) is not int or pressed != (held & ~previous_held):
                raise ValueError(f"input event {index} pressed does not match held transition")
            inputs.append((tick - 1, held))
            transitions.append((tick, held))
            previous_tick, previous_held = tick, held
        elif kind == "start":
            if start is not None or final is not None:
                raise ValueError("trace must contain exactly one start before final")
            start = row
            start_input_tick = previous_tick
            start_transition_count = len(transitions)
        elif kind in ("periodic", "prehatch", "posthatch") or (
                kind == "checkpoint" and profile_name == "zerg-z7") or (
                kind in ("rescued", "boarded", "unloaded") and profile_name == "protoss-p5") or (
                kind == "both-rescued" and profile_name == "protoss-p4"):
            if start is None or final is not None:
                raise ValueError(f"{kind} observation must follow start and precede final")
            if kind == "periodic":
                if checkpoints and tick <= checkpoints[-1]["host_tick"]:
                    raise ValueError("periodic observation ticks must strictly increase")
            checkpoints.append(row)
        elif kind in ("won", "lost", "limit"):
            if start is None or final is not None or index != len(rows) - 1:
                raise ValueError("trace must end with exactly one terminal event")
            final = row
        else:
            raise ValueError(f"trace event {index} has unsupported kind {kind!r}")

    if not inputs or inputs[0][0] != 0:
        raise ValueError("first controller transition must be host_tick=1")
    entry = (7 if profile_name == "zerg-z7" else
             6 if profile_name in ("protoss-p6", "zerg-z6") else
             5 if profile_name in PERSISTENT_TYPES else 4)
    menu_prefix = (SEVENTH_MENU_PREFIX if entry == 7 else
                   SIXTH_MENU_PREFIX if entry == 6 else
                   FIFTH_MENU_PREFIX if entry == 5 else MENU_PREFIX)
    if (start_transition_count != len(menu_prefix) or
            transitions[:start_transition_count] != list(menu_prefix)):
        raise ValueError(f"normal HOME/menu inputs do not match the {entry}th-entry deployment route")
    checked_inputs = ([event for event in inputs if event[0] < prefix_ticks]
                      if attempt_prefix and prefix_ticks is not None else inputs)
    if not windowed and len(checked_inputs) > capacity:
        raise ValueError(f"controller queue exceeds ELF capacity ({len(checked_inputs)}/{capacity})")
    expected_start_frame = 1 if profile_name in ("protoss-p6", "zerg-z6", "zerg-z7") else 0
    if (start is None or start.get("screen") != SCREEN_PLAY or
            start.get("frame") != expected_start_frame):
        raise ValueError("trace start must match the profile's live PLAY handoff frame")
    launch_tick = 12 if entry in (6, 7) else 9
    if start.get("host_tick") != launch_tick or start.get("host_tick") != start_input_tick:
        raise ValueError(f"normal {entry}th-entry HOME/menu deployment must launch at host_tick={launch_tick}")
    if start.get("won") != 0 or start.get("lost") != 0 or start.get("outcome") != 0:
        raise ValueError("trace start must be live and unfinished")
    if profile_name == "protoss-p6":
        for name, size in (("action_cursors", 26), ("action_ticks", 26)):
            value = start.get(name)
            if (not isinstance(value, list) or len(value) != size or
                    any(type(x) is not int or x < 0 for x in value)):
                raise ValueError(f"P6 start observation requires 26 integer {name} values")
        if (type(start.get("started")) is not int or not 0 <= start["started"] < (1 << 26) or
                type(start.get("fired")) is not int or start["fired"] != start["started"] or
                type(start.get("switch_bits")) is not int or
                start["switch_bits"] & ~P6_SWITCH_MASK or
                type(start.get("fault")) is not int or start["fault"] != 0):
            raise ValueError("P6 start observation has invalid model or switch state")
        for i, (cursor, ticks) in enumerate(zip(start["action_cursors"], start["action_ticks"])):
            if (cursor > P6_ACTION_COUNTS[i] or
                    (cursor and not (start["started"] & (1 << i))) or
                    ((cursor != 0) != (ticks != 0))):
                raise ValueError("P6 start cursor/timer state violates source action bounds")
    elif profile_name == "zerg-z6":
        for name in ("action_cursors", "action_ticks"):
            value = start.get(name)
            if not isinstance(value, list) or len(value) != 9 or any(type(x) is not int or x < 0 for x in value):
                raise ValueError(f"Z6 start observation requires 9 integer {name} values")
        started = start.get("started")
        if (type(started) is not int or not 0 <= started < (1 << 9) or
                start.get("fired") != started or type(start.get("switch_bits")) is not list or
                len(start["switch_bits"]) != 8 or
                any(type(x) is not int or not 0 <= x <= 0xFFFFFFFF for x in start["switch_bits"]) or
                start.get("fault") != 0):
            raise ValueError("Z6 start observation has invalid model, switch, or fault state")
        for i, (cursor, ticks) in enumerate(zip(start["action_cursors"], start["action_ticks"])):
            if (cursor > Z6_ACTION_COUNTS[i] or (cursor and not (started & (1 << i))) or
                    ((cursor != 0) != (ticks != 0))):
                raise ValueError("Z6 start cursor/timer state violates source action bounds")
    elif profile_name == "zerg-z7":
        _validate_z7_observation(start, "start", live=True)
    if final is None or rows[-1] is not final:
        raise ValueError("controller trace must end with exactly one terminal event")
    if not attempt_prefix and final.get("kind") != "won":
        raise ValueError("full controller trace must end in a won event")
    if attempt_prefix and final.get("kind") not in ("lost", "limit"):
        raise ValueError("--attempt-prefix requires a failed or limited Z7 trace")
    stop = final.get("host_tick")
    # host_tick is one-based for inputs: the transition at tick N is applied
    # by update N. A result observed after that update may share its timestamp.
    if type(stop) is not int or not 1 <= stop <= 0xFFFFFFFF or stop < previous_tick:
        raise ValueError("terminal update must include every controller transition")
    if stop <= start["host_tick"]:
        raise ValueError("terminal update must follow the live start observation")
    if attempt_prefix and prefix_ticks is not None and prefix_ticks >= stop:
        raise ValueError("--attempt-prefix ticks must end before the failed/limited terminal tick")
    if checkpoints and checkpoints[-1]["host_tick"] > stop:
        raise ValueError("checkpoint occurs after final result")
    if not attempt_prefix:
        if final.get("screen") != SCREEN_RESULT or final.get("won") != 1 or final.get("lost") != 0:
            raise ValueError("final event must record a won RESULT screen")
        if final.get("outcome") != 1:
            raise ValueError("final event must authenticate a completed campaign-model win")
    elif final.get("kind") == "lost" and (
            final.get("screen") != SCREEN_RESULT or final.get("won") != 0 or
            final.get("lost") != 1 or final.get("outcome") != 2):
        raise ValueError("failed Z7 attempt must record the authentic lost RESULT state")
    elif final.get("kind") == "limit" and (
            final.get("screen") != SCREEN_PLAY or final.get("won") != 0 or
            final.get("lost") != 0 or final.get("outcome") != 0):
        raise ValueError("limited Z7 attempt must remain live and unfinished")
    for field in ("frame", "fired"):
        if type(final.get(field)) is not int:
            raise ValueError(f"final event is missing integer field {field}")
    if final["frame"] <= start["frame"]:
        raise ValueError("final frame must follow mission start")
    if profile_name == "protoss-p6":
        started = final.get("started")
        if type(started) is not int or not 0 <= started < (1 << 26):
            raise ValueError("P6 final started mask must fit 26 source triggers")
        if not (started & (1 << 15)) or started & ((1 << 1) | (1 << 8)):
            raise ValueError("P6 winning trace must include victory and exclude defeat triggers")
        if final["fired"] != started:
            raise ValueError("P6 final fired alias must match the started trigger mask")
        for name, limit in (("action_cursors", 255), ("action_ticks", 65535)):
            value = final.get(name)
            if (not isinstance(value, list) or len(value) != 26 or
                    any(type(x) is not int or not 0 <= x <= limit for x in value)):
                raise ValueError(f"P6 final observation requires 26 bounded {name} values")
        for i, (cursor, ticks) in enumerate(zip(final["action_cursors"], final["action_ticks"])):
            if (cursor > P6_ACTION_COUNTS[i] or
                    (cursor and not (started & (1 << i))) or
                    ((cursor != 0) != (ticks != 0))):
                raise ValueError("P6 final cursor/timer state violates source action bounds")
        if (type(final.get("switch_bits")) is not int or
                final["switch_bits"] & ~P6_SWITCH_MASK):
            raise ValueError("P6 final switch_bits exceed the authenticated switch set")
        if type(final.get("fault")) is not int or final["fault"] != 0:
            raise ValueError("P6 final observation must report zero adapter faults")
    elif profile_name == "zerg-z6":
        started = final.get("started")
        if type(started) is not int or not 0 <= started < (1 << 9):
            raise ValueError("Z6 final started mask must fit 9 source triggers")
        if not (started & (1 << 5)) or started & ((1 << 0) | (1 << 6)):
            raise ValueError("Z6 winning trace must include victory and exclude both defeat triggers")
        if final["fired"] != started:
            raise ValueError("Z6 final fired alias must match the started trigger mask")
        for name, limit in (("action_cursors", 255), ("action_ticks", 65535)):
            value = final.get(name)
            if (not isinstance(value, list) or len(value) != 9 or
                    any(type(x) is not int or not 0 <= x <= limit for x in value)):
                raise ValueError(f"Z6 final observation requires 9 bounded {name} values")
        for i, (cursor, ticks) in enumerate(zip(final["action_cursors"], final["action_ticks"])):
            if (cursor > Z6_ACTION_COUNTS[i] or (cursor and not (started & (1 << i))) or
                    ((cursor != 0) != (ticks != 0))):
                raise ValueError("Z6 final cursor/timer state violates source action bounds")
        switch = final.get("switch_bits")
        if (not isinstance(switch, list) or len(switch) != 8 or
                any(type(x) is not int or not 0 <= x <= 0xFFFFFFFF for x in switch) or
                final.get("fault") != 0):
            raise ValueError("Z6 final switch/fault state is malformed")
        if final.get("kerrigan_alive") != 1:
            raise ValueError("Z6 final observation requires the live source Kerrigan")
    elif profile_name == "zerg-z7":
        _validate_z7_observation(final, "final", live=False)
        if not attempt_prefix:
            _validate_z7_trace_win(final)
    if windowed:
        check_stop = prefix_ticks if attempt_prefix and prefix_ticks is not None else stop
        last_page = (check_stop - 1) // window_ticks
        peak = max((sum(1 for tick, _ in checked_inputs if tick // window_ticks == page)
                    for page in range(last_page + 1)), default=0)
        if peak > capacity:
            raise ValueError(f"controller event page exceeds ELF capacity ({peak}/{capacity})")
    else:
        peak = len(checked_inputs)
    return {"inputs": inputs, "input_count": len(inputs), "stop_tick": stop,
            "start": start, "final": final, "checkpoints": checkpoints,
            "peak_page_transitions": peak, "scheduler_mode": "windowed" if windowed else "full"}


def _validate_z7_observation(row: dict, label: str, *, live: bool) -> None:
    """Check the source-defined Z7 runtime observation shape and bounds."""
    for name, maximum in (("started", 0xFF), ("fired", 0xFF), ("switch10", 1),
                          ("ai_script_mask", 15), ("ai_plan_ticks", 240),
                          ("ai_plan_phase", 3), ("fault", 255), ("outcome", 2)):
        value = row.get(name)
        if type(value) is not int or not 0 <= value <= maximum:
            raise ValueError(f"Z7 {label} observation has invalid {name}")
    if row["fired"] != row["started"]:
        raise ValueError(f"Z7 {label} fired alias differs from started mask")
    if row["ai_script_mask"] not in (0, 1, 3, 7, 15):
        raise ValueError(f"Z7 {label} AI script mask is not a source-valid prefix")
    if bool(row["ai_script_mask"]) != bool(row["ai_plan_ticks"]):
        raise ValueError(f"Z7 {label} AI cadence disagrees with dispatched scripts")
    if row["ai_script_mask"] == 0 and row["ai_plan_phase"] != 0:
        raise ValueError(f"Z7 {label} idle AI state has a nonzero phase")
    if live and (row["fault"] != 0 or row["outcome"] != 0):
        raise ValueError(f"Z7 {label} observation must be running without adapter faults")
    cursors, timers = row.get("action_cursors"), row.get("action_ticks")
    if (not isinstance(cursors, list) or len(cursors) != 8 or
            any(type(v) is not int for v in cursors)):
        raise ValueError(f"Z7 {label} observation requires 8 source action cursors")
    if (not isinstance(timers, list) or len(timers) != 8 or
            any(type(v) is not int for v in timers)):
        raise ValueError(f"Z7 {label} observation requires 8 source action timers")
    # Only the initial switch's ten-second wait has a timer longer than one
    # scheduler tick. The adapter caps all source waits at 36,000 ticks.
    for i, (cursor, ticks, count) in enumerate(zip(cursors, timers, Z7_ACTION_COUNTS)):
        max_ticks = 300 if i == 4 and cursor == 1 else 1
        if (not 0 <= cursor <= count or not 0 <= ticks <= max_ticks or
                bool(cursor) != bool(ticks) or (cursor and not row["started"] & (1 << i))):
            raise ValueError(f"Z7 {label} action cursor/timer {i} violates source bounds")
    loss_started, victory_started = bool(row["started"] & 1), bool(row["started"] & 4)
    if ((row["outcome"] == 0 and
         ((loss_started and cursors[0] == 0) or (victory_started and cursors[2] == 0))) or
        (row["outcome"] == 1 and
         (not victory_started or loss_started or cursors[2] != 0 or cursors[0] != 0)) or
        (row["outcome"] == 2 and
         (not loss_started or victory_started or cursors[0] != 0 or cursors[2] != 0))):
        raise ValueError(f"Z7 {label} terminal cursors disagree with the source outcome")
    ai_cursor = cursors[3]
    expected_mask = (15 if not ai_cursor else
                     ((1 << (ai_cursor - 1)) - 1 if ai_cursor >= 2 else 0)) \
        if row["started"] & (1 << 3) else 0
    if row["ai_script_mask"] != expected_mask:
        raise ValueError(f"Z7 {label} AI mask disagrees with the source AI action cursor")


def _validate_z7_trace_win(row: dict) -> None:
    """Require complete source victory state and a surviving player actor."""
    if (row.get("outcome") != 1 or not row["started"] & (1 << 2) or
            row["started"] & 1 or any(row["action_cursors"]) or
            any(row["action_ticks"]) or row["fault"] != 0):
        raise ValueError("Z7 trace lacks a completed victory trigger, has defeat, or reports faults")
    for key in ("side_units", "side_buildings", "enemy_units", "enemy_buildings", "workers"):
        if type(row.get(key)) is not int or row[key] < 0:
            raise ValueError(f"Z7 winning trace lacks a nonnegative {key} census")
    if row["enemy_buildings"] != 0:
        raise ValueError("Z7 win requires zero source enemy buildings")
    if row["side_units"] + row["side_buildings"] < 1:
        raise ValueError("Z7 win requires a surviving player actor")


def decode_runtime(raw: bytes, runtime_layout: tuple[int, dict[str, int]],
                   model_layout: tuple[int, dict[str, int]], profile_name: str) -> dict:
    """Decode both source model and adapter cursors using DWARF byte offsets."""
    size, fields = runtime_layout
    model_size, model_fields = model_layout
    if len(raw) != size or "model" not in fields:
        raise ValueError("runtime state bytes do not match DWARF layout")
    base = fields["model"]
    if base < 0 or base + model_size > len(raw):
        raise ValueError("nested model lies outside runtime state")
    model_raw = raw[base:base + model_size]
    if profile_name == "zerg-z7":
        return decode_z7_runtime(raw, runtime_layout, model_layout)
    if profile_name == "protoss-p6":
        return decode_p6_runtime(raw, runtime_layout, model_layout)
    if profile_name == "zerg-z6":
        return decode_z6_runtime(raw, runtime_layout, model_layout)
    if profile_name in PERSISTENT_TYPES:
        return decode_fifth_runtime(raw, runtime_layout, model_layout, profile_name)
    model_widths = ({"fired": 2, "outcome": 1, "action_cursor": 7}
                    if profile_name == "zerg-z4" else
                    {"fired": 2, "outcome": 1, "sequence_trigger": 1,
                     "action_index": 1, "sequence_active": 1, "reserved": 1})
    model = {}
    for name, width in model_widths.items():
        if name not in model_fields:
            if name == "action_cursor" and profile_name == "zerg-z4":
                continue
            raise ValueError(f"model DWARF layout is missing {name}")
        offset = model_fields[name]
        if offset < 0 or offset + width > model_size:
            raise ValueError(f"model field {name} exceeds DWARF structure")
        value = model_raw[offset:offset + width]
        model[name] = list(value) if name == "action_cursor" else int.from_bytes(value, "little")
    result = {"model": model}
    runtime_widths = {"action_ticks": (26, "u16[]"), "message_ticks": (2, "u16"),
                      "created_count": (2, "u16"), "action_cursor": (13, "u8[]"),
                      "switch_bits": (1, "u8"), "source_paused": (1, "u8"),
                      "speech_muted": (1, "u8"), "ai_script_mask": (1, "u8"),
                      "switch13_set": (1, "u8"), "faults": (2, "u16"),
                      "countdown_ticks": (2, "u16"), "faults32": (4, "u32"),
                      "hatch_effects": (1, "u8"), "countdown_active": (1, "u8")}
    if profile_name == "zerg-z4":
        wanted = ("countdown_ticks", "action_ticks", "faults", "message_ticks", "switch_bits",
                  "countdown_active", "ai_script_mask", "hatch_effects")
        runtime_widths["faults"] = (4, "u32")
        runtime_widths["action_ticks"] = (14, "u16[]")
    else:
        wanted = ("action_ticks", "message_ticks", "created_count", "action_cursor",
                  "switch_bits", "source_paused", "speech_muted", "ai_script_mask",
                  "switch13_set", "faults")
        # The optional P4 spell profile persists four eight-byte effects.
        # Observe every byte so an in-flight save comparison cannot silently
        # omit effect positions, ages or remaining pulses. Legacy P4 ELF
        # layouts have no such member and retain their previous observation.
        if "storm" in fields:
            runtime_widths["storm"] = (32, "u8[]")
            wanted += ("storm",)
    for name in wanted:
        if name not in fields or name not in runtime_widths:
            raise ValueError(f"runtime DWARF layout is missing {name}")
        width, kind = runtime_widths[name]
        offset = fields[name]
        if offset < 0 or offset + width > size:
            raise ValueError(f"runtime field {name} exceeds DWARF structure")
        value = raw[offset:offset + width]
        if kind.endswith("[]"):
            if kind == "u16[]":
                result[name] = list(struct.unpack("<" + "H" * (width // 2), value))
            else:
                result[name] = list(value)
        else:
            result[name] = int.from_bytes(value, "little")
    return result


def runtime_layout_for(elf: Path, profile_name: str) -> tuple[int, dict[str, int]]:
    """Flatten the persisted runtime using DWARF, retaining enclosing members."""
    inner_size, inner = races.dwarf_struct(elf, RUNTIME_TYPES[profile_name])
    if profile_name not in PERSISTENT_TYPES:
        return inner_size, inner
    size, outer = races.dwarf_struct(elf, PERSISTENT_TYPES[profile_name])
    if profile_name == "zerg-z7" and (size <= 0 or size > 36 or inner_size <= 0 or inner_size > 36):
        raise RuntimeError("Z7 runtime or persistent state exceeds the 36-byte source budget")
    base = outer.get("runtime", -1)
    if base < 0 or base + inner_size > size:
        raise RuntimeError("persistent runtime lies outside DWARF structure")
    fields = {name: offset for name, offset in outer.items() if name != "runtime"}
    for name, offset in inner.items():
        if offset < 0 or offset >= inner_size or name in fields:
            raise RuntimeError("invalid or ambiguous nested runtime field")
        fields[name] = base + offset
    if profile_name == "zerg-z6":
        # Flatten optional nested Z6 effects using their own ELF DWARF types;
        # these offsets vary with enabled ability gates and are never guessed.
        for member, type_name, prefix in (
                ("cloak", "Sc64Z5CloakState", "cloak_"),
                ("storm", "Sc64PsionicStormState", "storm_"),
                ("transport", "Sc64Z6TransportTechState", "transport_")):
            if member not in outer:
                continue
            nested_size, nested_fields = races.dwarf_struct(elf, type_name)
            offset = outer[member]
            if offset < 0 or offset + nested_size > size:
                raise RuntimeError(f"Z6 {member} state lies outside persistent structure")
            for nested_name, nested_offset in nested_fields.items():
                key = prefix + nested_name
                if key in fields or nested_offset < 0 or nested_offset >= nested_size:
                    raise RuntimeError(f"invalid or duplicate Z6 {member} DWARF field")
                fields[key] = offset + nested_offset
    return size, fields


def decode_z7_runtime(raw: bytes, runtime_layout: tuple[int, dict[str, int]],
                      model_layout: tuple[int, dict[str, int]]) -> dict:
    """Decode the complete, bounded Z7 persistent state from its DWARF layout."""
    size, fields = runtime_layout
    model_size, model_fields = model_layout
    if len(raw) != size or size <= 0 or size > 36:
        raise ValueError("Z7 persistent state bytes do not match its 36-byte source bound")
    base = fields.get("model", -1)
    if base < 0 or model_size <= 0 or base + model_size > size or model_size > 12:
        raise ValueError("Z7 source model lies outside its 12-byte source structure")

    def read(table: dict[str, int], limit: int, name: str, count: int, width: int,
             *, start: int = 0) -> int | list[int]:
        offset = table.get(name, -1)
        total = count * width
        if offset < 0 or offset + total > limit or start + offset + total > len(raw):
            raise ValueError(f"Z7 runtime field {name} exceeds or is absent from DWARF structure")
        data = raw[start + offset:start + offset + total]
        if count == 1:
            return int.from_bytes(data, "little")
        return [int.from_bytes(data[i:i + width], "little")
                for i in range(0, total, width)]

    model = {"started": read(model_fields, model_size, "started", 1, 2, start=base),
             "fired": read(model_fields, model_size, "started", 1, 2, start=base),
             "outcome": read(model_fields, model_size, "outcome", 1, 1, start=base),
             "action_cursor": read(model_fields, model_size, "action_cursor", 8, 1, start=base)}
    result = {"model": model,
              "action_ticks": read(fields, size, "action_ticks", 8, 2),
              "switch10": read(fields, size, "switch10", 1, 1),
              "ai_script_mask": read(fields, size, "ai_script_mask", 1, 1),
              "ai_plan_ticks": read(fields, size, "ai_plan_ticks", 1, 2),
              "ai_plan_phase": read(fields, size, "ai_plan_phase", 1, 1),
              "fault": read(fields, size, "fault", 1, 1),
              "persistent_size": size, "persistent_bytes": list(raw)}
    return result


def decode_z6_runtime(raw: bytes, runtime_layout: tuple[int, dict[str, int]],
                      model_layout: tuple[int, dict[str, int]]) -> dict:
    """Decode the complete Z6 payload (at most 180 bytes) using its DWARF layout."""
    size, fields = runtime_layout
    model_size, model_fields = model_layout
    if len(raw) != size or "model" not in fields or size <= 0 or size > 180:
        raise ValueError("Z6 persistent state bytes do not match bounded DWARF layout")
    base = fields["model"]
    if base < 0 or base + model_size > size:
        raise ValueError("Z6 model lies outside persistent runtime structure")

    def read(table: dict[str, int], limit: int, name: str, count: int, width: int,
             *, start: int = 0) -> int | list[int]:
        offset = table.get(name, -1)
        total = count * width
        if offset < 0 or offset + total > limit or start + offset + total > len(raw):
            raise ValueError(f"Z6 runtime field {name} exceeds or is absent from DWARF structure")
        data = raw[start + offset:start + offset + total]
        if count == 1:
            return int.from_bytes(data, "little")
        return [int.from_bytes(data[i:i + width], "little")
                for i in range(0, total, width)]

    model = {"started": read(model_fields, model_size, "started", 1, 2, start=base),
             "fired": read(model_fields, model_size, "started", 1, 2, start=base),
             "outcome": read(model_fields, model_size, "outcome", 1, 1, start=base),
             "action_cursor": read(model_fields, model_size, "action_cursor", 9, 1, start=base)}
    result = {"model": model,
              "action_ticks": read(fields, size, "action_ticks", 9, 2),
              "switch_bits": read(fields, size, "switch_bits", 8, 4),
              "fault": read(fields, size, "fault", 1, 1),
              "deaths_by_source": read(fields, size, "deaths_by_source", 12, 4),
              "moved_location": read(fields, size, "moved_location", 8, 2),
              "moved_locations_mask": read(fields, size, "moved_locations_mask", 1, 1),
              "ai_plan_ticks": read(fields, size, "ai_plan_ticks", 1, 1),
              "ai_plan_phase": read(fields, size, "ai_plan_phase", 1, 1),
              "human_drone_morphs": read(fields, size, "human_drone_morphs", 1, 1),
              "persistent_size": size, "persistent_bytes": list(raw)}
    if "cloak_active" in fields:
        result["cloak"] = [read(fields, size, "cloak_active", 1, 1),
                           read(fields, size, "cloak_regen_remainder", 1, 1)]
    if "transport_ticks_remaining" in fields:
        result["transport"] = [read(fields, size, "transport_ticks_remaining", 1, 2),
                               read(fields, size, "transport_producer_slot_plus1", 1, 1),
                               read(fields, size, "transport_researched", 1, 1)]
    if "storm_storm" in fields:
        result["storm"] = read(fields, size, "storm_storm", 32, 1)
    return result


def decode_fifth_runtime(raw: bytes, runtime_layout: tuple[int, dict[str, int]],
                         model_layout: tuple[int, dict[str, int]], profile_name: str) -> dict:
    size, fields = runtime_layout
    model_size, model_fields = model_layout
    zerg = profile_name == "zerg-z5"
    if profile_name not in PERSISTENT_TYPES or len(raw) != size:
        raise ValueError("unknown fifth runtime or incorrect byte count")
    base = fields["model"]
    def scalar(name: str, width: int, *, model=False) -> int:
        table, limit, data, start = (model_fields, model_size, raw, base) if model else (fields, size, raw, 0)
        offset = table.get(name, -1)
        if offset < 0 or offset + width > limit or start + offset + width > len(data):
            raise ValueError(f"runtime field {name} exceeds or is absent from DWARF structure")
        return int.from_bytes(data[start + offset:start + offset + width], "little")
    def array(name: str, count: int, width: int, *, model=False) -> list[int]:
        value = scalar(name, count * width, model=model)
        mask = (1 << (width * 8)) - 1
        return [(value >> (width * 8 * i)) & mask for i in range(count)]
    model = {"outcome": scalar("outcome", 1, model=True),
             "action_cursor": array("action_cursor", 40 if zerg else 11, 1, model=True)}
    if zerg:
        model["started_lo"] = scalar("started_lo", 4, model=True)
        model["started_hi"] = scalar("started_hi", 4, model=True)
        model["fired"] = model["started_lo"] | (model["started_hi"] << 32)
    else:
        model["started"] = scalar("started", 2, model=True)
        model["fired"] = model["started"]
    result = {"model": model, "action_ticks": array("action_ticks", 40 if zerg else 11, 2),
              "switch_bits": array("switch_bits", 8, 4), "faults": scalar("fault", 1),
              # Include the complete persisted representation, including optional
              # ability state and reserved bytes, in fresh-process comparisons.
              "persistent_bytes": list(raw)}
    if zerg:
        result.update({"source_paused": scalar("source_paused", 1),
                       "doodad_enabled": array("doodad_enabled", 6, 1),
                       "pause_acknowledged": scalar("pause_acknowledged", 1)})
        if "cloak" in fields:
            result["cloak"] = array("cloak", 2, 1)
    else:
        result.update({"deaths_by_source": array("deaths_by_source", 12, 4),
                       "pressure_dispatched_mask": scalar("pressure_dispatched_mask", 1),
                       "reserved": array("reserved", 3, 1)})
    return result


def decode_p6_runtime(raw: bytes, runtime_layout: tuple[int, dict[str, int]],
                      model_layout: tuple[int, dict[str, int]]) -> dict:
    """Decode the complete P6 persistent payload, including union member bytes."""
    size, fields = runtime_layout
    model_size, model_fields = model_layout
    if len(raw) != size:
        raise ValueError("P6 persistent state bytes do not match DWARF layout")
    base = fields.get("model", -1)
    if base < 0 or base + model_size > size:
        raise ValueError("P6 model lies outside persistent runtime structure")

    def read(table: dict[str, int], limit: int, source: bytes,
             name: str, count: int, width: int, *, start: int = 0) -> list[int] | int:
        offset = table.get(name, -1)
        total = count * width
        if offset < 0 or offset + total > limit or start + offset + total > len(source):
            raise ValueError(f"P6 runtime field {name} exceeds or is absent from DWARF structure")
        data = source[start + offset:start + offset + total]
        if count == 1:
            return int.from_bytes(data, "little")
        return [int.from_bytes(data[i:i + width], "little")
                for i in range(0, total, width)]

    model = {
        "started": read(model_fields, model_size, raw, "started", 1, 4, start=base),
        "fired": read(model_fields, model_size, raw, "started", 1, 4, start=base),
        "outcome": read(model_fields, model_size, raw, "outcome", 1, 1, start=base),
        "action_cursor": read(model_fields, model_size, raw, "action_cursor", 26, 1, start=base),
    }
    result = {
        "model": model,
        "action_ticks": read(fields, size, raw, "action_ticks", 26, 2),
        "switch_bits": read(fields, size, raw, "switch_bits", 1, 4),
        "fault": read(fields, size, raw, "fault", 1, 1),
        "deaths_by_source": read(fields, size, raw, "deaths_by_source", 12, 4),
        "doodad_enabled": read(fields, size, raw, "doodad_enabled", 5, 1),
        "pause_acknowledged": read(fields, size, raw, "pause_acknowledged", 1, 1),
        "reserved": read(fields, size, raw, "reserved", 2, 1),
        "persistent_bytes": list(raw),
        "storm": read(fields, size, raw, "storm", 32, 1),
    }
    return result


def verify_p5_extraction(trace_final: dict, state: dict) -> None:
    bounds = trace_final.get("goal")
    if bounds != [1696, 1680, 1808, 1776]:
        raise RuntimeError("P5 trace lacks the authenticated source extraction bounds")
    left, top, right, bottom = bounds
    def inside(a):
        return (a.get("owner") == 0 and a.get("order") != 8 and
                left <= a.get("x", -1) < right and top <= a.get("y", -1) < bottom)
    actors = state.get("units", {}).get("entities", [])
    heroes = [a for a in actors if a.get("raw_type") == 79]
    if len(heroes) != 1 or not inside(heroes[0]) or heroes[0].get("raw_owner") != 2:
        raise RuntimeError("P5 requires the rescued, unloaded Tassadar inside extraction")
    hero = heroes[0]
    if trace_final.get("hero") != [1, hero["x"], hero["y"], 0]:
        raise RuntimeError("P5 extracted hero differs from controller trace")
    zealots = sum(inside(a) and a.get("raw_type") == 65 and a.get("raw_owner") == 7 for a in actors)
    if zealots < 2 or zealots != trace_final.get("zealots_at_goal"):
        raise RuntimeError("P5 requires two unloaded Zealots inside extraction")


def verify_p4_hero_extraction(trace_final: dict, state: dict) -> None:
    """Cross-check recorded objective actors against the observed live bank."""
    if not any(f"{name}_alive" in trace_final for name in ("tassadar", "raynor")):
        return  # Earlier trace schemas record only the composite model.
    bounds = trace_final.get("escape")
    if (not isinstance(bounds, list) or len(bounds) != 4 or
            any(type(value) is not int for value in bounds)):
        raise RuntimeError("P4 extraction trace lacks integer rectangle bounds")
    left, top, right, bottom = bounds
    if not (0 <= left < right <= 2048 and 0 <= top < bottom <= 2048):
        raise RuntimeError("P4 extraction rectangle is outside the world")
    entities = state.get("units", {}).get("entities", [])
    for name, raw in (("tassadar", 79), ("raynor", 19)):
        fields = ("alive", "owner", "x", "y")
        if any(type(trace_final.get(f"{name}_{field}")) is not int for field in fields):
            raise RuntimeError(f"P4 trace lacks complete {name} observation")
        actors = [actor for actor in entities if actor.get("raw_type") == raw]
        if len(actors) != 1 or trace_final[f"{name}_alive"] != 1:
            raise RuntimeError(f"P4 extraction must retain exactly one live {name}")
        actor = actors[0]
        for field in ("owner", "x", "y"):
            if actor.get(field) != trace_final[f"{name}_{field}"]:
                raise RuntimeError(f"P4 {name}.{field} differs from the controller trace")
        if actor["owner"] != 0 or not (left <= actor["x"] < right and
                                       top <= actor["y"] < bottom):
            raise RuntimeError(f"P4 {name} is not player-owned inside extraction")


def verify_p6_extraction(trace_final: dict, state: dict) -> None:
    """Verify source-proven Zeratul goal presence and live Tassadar from both snapshots."""
    bounds = trace_final.get("objective")
    if bounds != [1216, 1664, 1312, 1728]:
        raise RuntimeError("P6 trace does not report authenticated source location 15 bounds")
    z_trace, t_trace = trace_final.get("zeratul"), trace_final.get("tassadar")
    if not isinstance(z_trace, list) or len(z_trace) != 8:
        raise RuntimeError("P6 trace lacks complete Zeratul identity and position")
    if not isinstance(t_trace, list) or len(t_trace) != 8:
        raise RuntimeError("P6 trace lacks complete Tassadar identity and position")
    left, top, right, bottom = bounds
    for name, actor, raw, must_extract in (("Zeratul", z_trace, 75, True),
                                            ("Tassadar", t_trace, 79, False)):
        alive, raw_type, raw_owner, x, y, owner, hp, order = actor
        if (alive != 1 or raw_type != raw or raw_owner != 2 or owner != 0 or
                type(hp) is not int or hp <= 0 or type(order) is not int):
            raise RuntimeError(f"P6 trace has invalid {name} source identity or live state")
        if must_extract and not (left <= x < right and top <= y < bottom):
            raise RuntimeError("P6 Zeratul is not inside source extraction location 15")
    units = state.get("units", {}).get("entities", [])
    for name, trace_actor, raw in (("Zeratul", z_trace, 75), ("Tassadar", t_trace, 79)):
        matches = [a for a in units if a.get("raw_type") == raw and a.get("raw_owner") == 2]
        if len(matches) != 1:
            raise RuntimeError(f"P6 final bank must contain exactly one live source {name}")
        a = matches[0]
        live = [1, a.get("raw_type"), a.get("raw_owner"), a.get("x"), a.get("y"),
                a.get("owner"), a.get("hp"), a.get("order")]
        if live != trace_actor:
            raise RuntimeError(f"P6 {name} live bank state differs from controller trace")
        if name == "Zeratul" and not (left <= a["x"] < right and top <= a["y"] < bottom):
            raise RuntimeError("P6 Zeratul is not inside source extraction location 15")


def verify_final(trace_final: dict, state: dict, profile_name: str) -> None:
    mission = MISSION_BY_PROFILE[profile_name]
    campaign = state.get("race_campaign", {})
    required = {"screen": SCREEN_RESULT, "frame": trace_final["frame"], "won": 1, "lost": 0,
                "race_mode": 0, "race_selected_mission": mission}
    for key, value in required.items():
        if state.get(key) != value:
            raise RuntimeError(f"final {key} mismatch: expected {value}, observed {state.get(key)}")
    for key, value in (("mission", mission), ("elapsed_frames", trace_final["frame"]), ("phase", 3)):
        if campaign.get(key) != value:
            raise RuntimeError(f"final campaign.{key} mismatch: expected {value}, got {campaign.get(key)}")
    if not campaign.get("objective_flags", 0) & 1:
        raise RuntimeError("final campaign primary objective flag is absent")
    if not state.get("race_completed", 0) & (1 << mission):
        raise RuntimeError("mission completion bit is absent")
    runtime = state.get("race_runtime", {})
    model = runtime.get("model", {})
    if model.get("outcome") != 1 or model.get("fired") != trace_final["fired"]:
        raise RuntimeError("final composite campaign model does not match trace outcome/fired state")
    if profile_name == "zerg-z7":
        runtime = state.get("race_runtime", {})
        model = runtime.get("model", {})
        if runtime.get("fault") != 0:
            raise RuntimeError("Z7 final state reports adapter faults")
        if (model.get("outcome") != 1 or model.get("started") != trace_final.get("started") or
                model.get("fired") != trace_final.get("fired")):
            raise RuntimeError("Z7 source model does not match the completed victory trace")
        if (model.get("action_cursor") != trace_final.get("action_cursors") or
                runtime.get("action_ticks") != trace_final.get("action_ticks")):
            raise RuntimeError("Z7 source action cursors or timers differ from the trace")
        for name in ("switch10", "ai_script_mask", "ai_plan_ticks", "ai_plan_phase", "fault"):
            if runtime.get(name) != trace_final.get(name):
                raise RuntimeError(f"Z7 source runtime {name} differs from the trace")
        payload, payload_size = runtime.get("persistent_bytes"), runtime.get("persistent_size")
        if (type(payload_size) is not int or not 0 < payload_size <= 36 or
                not isinstance(payload, list) or len(payload) != payload_size or
                any(type(value) is not int or not 0 <= value <= 255 for value in payload)):
            raise RuntimeError("Z7 complete persistent state bytes exceed their source bound")

        units = state.get("units", {}).get("entities", [])
        buildings = state.get("buildings", {}).get("entities", [])
        human_units = [actor for actor in units if actor.get("raw_owner") == 3]
        human_buildings = [actor for actor in buildings if actor.get("raw_owner") == 3]
        enemy_units = [actor for actor in units if actor.get("raw_owner") == 4]
        enemy_buildings = [actor for actor in buildings if actor.get("raw_owner") == 4]
        if any(actor.get("owner") != 0 for actor in human_units + human_buildings):
            raise RuntimeError("Z7 human source slot 3 is not bound to the live player")
        if any(actor.get("owner") != 1 for actor in enemy_units + enemy_buildings):
            raise RuntimeError("Z7 hostile source slot 4 is not bound to the live enemy")
        workers = sum(actor.get("raw_type") == 41 for actor in human_units)
        census = {"side_units": len(human_units), "side_buildings": len(human_buildings),
                  "enemy_units": len(enemy_units), "enemy_buildings": len(enemy_buildings),
                  "workers": workers}
        for name, actual in census.items():
            if trace_final.get(name) != actual:
                raise RuntimeError(f"Z7 source-owner {name} census differs from the trace")
        if census["enemy_buildings"] != 0:
            raise RuntimeError("Z7 source victory requires zero hostile buildings")
        if census["side_units"] + census["side_buildings"] == 0:
            raise RuntimeError("Z7 source victory must retain at least one player actor")
    elif profile_name == "zerg-z4":
        if runtime.get("faults") != 0 or runtime.get("hatch_effects") != 3:
            raise RuntimeError("Z4 final state has faults or incomplete hatch effects")
        for key in ("chrysalis_alive", "kerrigan_alive", "force3_command_centers"):
            if key in trace_final and state.get(key) != trace_final[key]:
                raise RuntimeError(f"final {key} differs from authenticated trace")
    elif profile_name == "protoss-p4":
        if runtime.get("faults") != 0:
            raise RuntimeError("P4 final state reports adapter faults")
        if "created" in trace_final and runtime.get("created_count") != trace_final["created"]:
            raise RuntimeError("P4 source-create total differs from controller trace")
        verify_p4_hero_extraction(trace_final, state)
        if (model.get("sequence_active") != 0 or model.get("sequence_trigger") != 0 or
                model.get("action_index") != 0 or model.get("reserved") != 0):
            raise RuntimeError("P4 final result sequence/cursor is not complete")
        # Keep P4 verification model-driven; don't encode Z4-specific outcomes.
        for key in ("force2_units", "force3_units", "side_units", "side_buildings"):
            if key in trace_final and state.get(key) != trace_final[key]:
                raise RuntimeError(f"final {key} differs from authenticated trace")

    elif profile_name == "protoss-p6":
        if runtime.get("fault") != 0:
            raise RuntimeError("P6 final state reports adapter faults")
        deaths, doodads = runtime.get("deaths_by_source"), runtime.get("doodad_enabled")
        payload, reserved = runtime.get("persistent_bytes"), runtime.get("reserved")
        if (not isinstance(deaths, list) or len(deaths) != 12 or
                any(type(v) is not int or not 0 <= v <= 65535 for v in deaths)):
            raise RuntimeError("P6 final source death counters have the wrong saved shape")
        if (not isinstance(doodads, list) or len(doodads) != 5 or
                any(type(v) is not int or not 0 <= v <= 255 for v in doodads) or
                doodads[4] & 0xE0 or
                any(value & forbidden for value, forbidden in
                    zip(doodads, P6_SPRITE_ONLY_DOODAD_MASK))):
            raise RuntimeError("P6 final doodad state has the wrong saved shape")
        if (type(runtime.get("pause_acknowledged")) is not int or
                runtime.get("pause_acknowledged") not in (0, 1) or
                reserved != [0, 0] or not isinstance(payload, list) or len(payload) != 180 or
                any(type(v) is not int or not 0 <= v <= 255 for v in payload)):
            raise RuntimeError("P6 final persistent bytes or reserved state are malformed")
        if not storm_bytes_valid(runtime.get("storm")):
            raise RuntimeError("P6 final Storm state is malformed")
        if runtime.get("model", {}).get("started") != trace_final.get("started"):
            raise RuntimeError("P6 started trigger mask differs from controller trace")
        if runtime.get("model", {}).get("action_cursor") != trace_final.get("action_cursors"):
            raise RuntimeError("P6 source action cursors differ from controller trace")
        if runtime.get("action_ticks") != trace_final.get("action_ticks"):
            raise RuntimeError("P6 source action timers differ from controller trace")
        if runtime.get("switch_bits") != trace_final.get("switch_bits"):
            raise RuntimeError("P6 switch state differs from controller trace")
        if model.get("outcome") != 1 or model.get("fired") != trace_final.get("fired"):
            raise RuntimeError("P6 composite model differs from the winning controller trace")
        verify_p6_extraction(trace_final, state)
    elif profile_name == "zerg-z6":
        if runtime.get("fault") != 0:
            raise RuntimeError("Z6 final state reports adapter faults")
        if runtime.get("model", {}).get("started") != trace_final.get("started"):
            raise RuntimeError("Z6 started trigger mask differs from controller trace")
        if runtime.get("model", {}).get("action_cursor") != trace_final.get("action_cursors"):
            raise RuntimeError("Z6 source action cursors differ from controller trace")
        if runtime.get("action_ticks") != trace_final.get("action_ticks"):
            raise RuntimeError("Z6 source action timers differ from controller trace")
        if runtime.get("switch_bits") != trace_final.get("switch_bits"):
            raise RuntimeError("Z6 switch state differs from controller trace")
        if (runtime.get("model", {}).get("outcome") != 1 or
                runtime.get("model", {}).get("fired") != trace_final.get("fired")):
            raise RuntimeError("Z6 source model differs from the winning controller trace")
        payload = runtime.get("persistent_bytes")
        if (not isinstance(payload, list) or len(payload) != runtime.get("persistent_size") or
                type(runtime.get("persistent_size")) is not int or
                not 0 < runtime["persistent_size"] <= 180 or
                any(type(v) is not int or not 0 <= v <= 255 for v in payload) or
                runtime.get("human_drone_morphs", -1) not in range(5)):
            raise RuntimeError("Z6 complete persistent payload or Drone morph count is malformed")
        if state.get("kerrigan_alive") != 1:
            raise RuntimeError("Z6 final state requires its live source hero")
    else:
        if runtime.get("faults") != 0:
            raise RuntimeError("fifth mission final state reports adapter faults")
        if runtime.get("action_ticks") != trace_final.get("action_ticks"):
            raise RuntimeError("fifth mission source wait timers differ from trace")
        if model.get("action_cursor") != trace_final.get("action_cursors"):
            raise RuntimeError("fifth mission source action cursors differ from trace")
        if profile_name == "protoss-p5":
            if runtime.get("deaths_by_source") != trace_final.get("source_deaths"):
                raise RuntimeError("P5 source-player death counters differ from trace")
            if runtime.get("pressure_dispatched_mask") != trace_final.get("pressure_dispatched"):
                raise RuntimeError("P5 authored pressure state differs from trace")
            verify_p5_extraction(trace_final, state)
        elif state.get("kerrigan_alive") != 1:
            raise RuntimeError("Z5 final state requires its live source hero")


def mission_snapshot(rsp, sym: dict[str, int], sizes: dict[str, int], bank: dict,
                     campaign_layout: tuple[int, dict[str, int]],
                     runtime_layout: tuple[int, dict[str, int]],
                     model_layout: tuple[int, dict[str, int]], profile_name: str) -> dict:
    units = read_compact_units(rsp, sym["u"], bank["unit_capacity"],
                               bank["unit_stride_bytes"], bank["unit_offsets"],
                               complete=profile_name in PERSISTENT_TYPES)
    buildings = read_compact_buildings(rsp, sym["b"], bank["building_capacity"],
                                       bank["building_stride_bytes"], bank["building_offsets"]) \
        if profile_name in PERSISTENT_TYPES else races.read_entities(rsp, sym["b"], bank["building_capacity"],
                                    bank["building_stride_bytes"], bank["building_offsets"],
                                    building=True)
    building_offsets = bank["building_offsets"]
    building_stride = bank["building_stride_bytes"]
    for field in ("hp", "shield"):
        if field in building_offsets:
            offset = building_offsets[field]
            if offset < 0 or offset + 4 > building_stride:
                raise RuntimeError(f"Building DWARF field {field} exceeds its stride")
            for item in buildings["entities"]:
                addr = sym["b"] + item["slot"] * building_stride + offset
                item[field] = int.from_bytes(rsp.read(addr, 4), "little", signed=True)
    state = {name: profile.u32(rsp, sym[name]) for name in
             ("screen", "frame", "won", "lost", "race_mode", "race_human", "race_bots",
              "race_completed", "race_skirmish_map", "race_selected_mission", "race_cols",
              "race_rows", "minerals", "gas") if name in sym}
    state["units"], state["buildings"] = units, buildings
    if profile_name in PERSISTENT_TYPES:
        # Retained dead actor provenance also participates in source bindings
        # and saves. Hash the complete banks rather than observing live rows only.
        state["unit_bank_sha256"] = hashlib.sha256(rsp.read(sym["u"], sizes["u"])).hexdigest()
        state["building_bank_sha256"] = hashlib.sha256(rsp.read(sym["b"], sizes["b"])).hexdigest()
    if "race_player" in sym:
        size = sizes.get("race_player", 0)
        if size and size % 4 == 0:
            state["race_player"] = list(struct.unpack("<" + "i" * (size // 4),
                                                       rsp.read(sym["race_player"], size)))
    campaign_size, campaign_offsets = campaign_layout
    campaign = races.campaign_state(rsp, sym["race_campaign"], campaign_size, campaign_offsets)
    state["race_campaign"] = campaign
    for field in ("elapsed_frames", "objective_flags", "phase", "mission"):
        if field in campaign:
            state[field] = campaign[field]
    for alias, field in (("chrysalis", "chrysalis_unit"), ("rescue", "rescue_unit"),
                         ("carrier", "carrier_unit")):
        slot = campaign.get(field, 0xFFFF)
        actor = next((item for item in units["entities"] if item["slot"] == slot), None)
        state[f"{alias}_alive"] = int(actor is not None)
        if alias != "rescue":
            state[f"{alias}_x"] = actor["x"] if actor else -1
            state[f"{alias}_y"] = actor["y"] if actor else -1
    for symbol, field in (("supply_used", "supply"), ("supply_cap", "supply_cap")):
        if symbol in sym:
            state[field] = profile.u32(rsp, sym[symbol])
    for name in ("race_minerals", "race_gas"):
        if name in sym and sizes.get(name, 0) == 20:
            state[name] = list(struct.unpack("<5i", rsp.read(sym[name], 20)))
    runtime_symbol = RUNTIME_SYMBOLS[profile_name]
    state["race_runtime"] = decode_runtime(
        rsp.read(sym[runtime_symbol], runtime_layout[0]), runtime_layout, model_layout, profile_name)
    owners = state["units"]["by_owner"]
    bowners = state["buildings"]["by_owner"]
    state["side_units"] = owners.get("0", 0)
    state["side_buildings"] = bowners.get("0", 0)
    state["force2_units"] = owners.get("1", 0)
    state["force3_units"] = owners.get("4", 0)
    state["force2_buildings"] = bowners.get("1", 0)
    state["force3_buildings"] = bowners.get("4", 0)
    state["kerrigan_alive"] = sum(1 for item in units["entities"]
                                   if item["owner"] == 0 and item.get("raw_type") == 51 and
                                   (profile_name != "zerg-z6" or item.get("raw_owner") == 3))
    state["force3_command_centers"] = sum(1 for item in buildings["entities"]
                                           if item["owner"] == 4 and item.get("raw_type") == 106)
    state["chrysalis_alive"] = count_chrysalis(units, buildings)
    state["race_minerals"] = list(struct.unpack("<5i", rsp.read(sym["race_minerals"], 20)))
    state["race_gas"] = list(struct.unpack("<5i", rsp.read(sym["race_gas"], 20)))
    return state


def read_compact_units(rsp, address: int, capacity: int, stride: int,
                       offsets: dict[str, int], *, complete: bool = False) -> dict:
    """Read Unit banks with their real mixed-width DWARF field representation."""
    data = rsp.read(address, capacity * stride)
    widths = {"hp": (4, "i"), "maxhp": (4, "i"), "carry": (4, "i"),
              "shield": (4, "i"), "x": (2, "h"), "y": (2, "h"),
              "tx": (2, "h"), "ty": (2, "h"), "build": (2, "h"),
              "target": (2, "h"), "path_x": (2, "h"), "path_y": (2, "h"),
              "raw_type": (2, "h"), "raw_owner": (2, "h"),
              "cd": (2, "H"), "work_timer": (2, "H"), "weapon": (2, "H"),
              "path_timer": (2, "H"), "path_retry": (2, "H"),
              "flags": (2, "H"), "shield_remainder": (2, "H"),
              "owner": (1, "B"), "type": (1, "B"), "alive": (1, "B"),
              "selected": (1, "B"), "order": (1, "B"), "armor": (1, "B")}
    for field in ("owner", "type", "alive", "x", "y", "hp", "raw_type"):
        if field not in offsets:
            raise RuntimeError(f"Unit DWARF is missing required field {field}")
    for field, (width, _) in widths.items():
        if field in offsets and not 0 <= offsets[field] <= stride - width:
            raise RuntimeError(f"Unit DWARF field {field} exceeds its stride")
    by_owner: dict[str, int] = {}
    by_owner_type: dict[str, dict[str, int]] = {}
    entities = []
    for slot in range(capacity):
        row = data[slot * stride:(slot + 1) * stride]
        def field(name: str) -> int:
            width, fmt = widths[name]
            return struct.unpack_from("<" + fmt, row, offsets[name])[0]
        if not field("alive"):
            continue
        owner, kind = field("owner"), field("type")
        owner_key, type_key = str(owner), str(kind)
        by_owner[owner_key] = by_owner.get(owner_key, 0) + 1
        owner_types = by_owner_type.setdefault(owner_key, {})
        owner_types[type_key] = owner_types.get(type_key, 0) + 1
        item = {"slot": slot, "owner": owner, "type": kind,
                "x": field("x"), "y": field("y"), "hp": field("hp")}
        for optional in ("shield", "raw_type", "raw_owner", "tx", "ty", "build",
                         "target", "path_x", "path_y", "cd", "work_timer", "weapon",
                         "path_timer", "path_retry", "flags", "shield_remainder",
                         "selected", "order", "armor") + (("carry", "maxhp") if complete else ()):
            if optional in offsets:
                item[optional] = field(optional)
        entities.append(item)
    return {"alive": len(entities), "by_owner": by_owner,
            "by_owner_type": by_owner_type, "entities": entities}


def read_compact_buildings(rsp, address: int, capacity: int, stride: int,
                           offsets: dict[str, int]) -> dict:
    """Decode the fifth-entry 48-byte Building layout without int-word reads."""
    widths = {"hp": (4, "i"), "maxhp": (4, "i"), "shield": (4, "i"),
              "build_progress": (4, "i"), "x": (2, "h"), "y": (2, "h"),
              "raw_type": (2, "h"), "raw_owner": (2, "h"),
              "queue_progress": (2, "H"), "flags": (2, "H"),
              "larva_timer": (2, "H"), "shield_remainder": (2, "H"),
              **{name: (1, "B") for name in ("owner", "type", "alive", "queue_head",
                  "queue_count", "under_construction", "level", "larva")},
              "queue": (5, "5B")}
    for name, (width, _) in widths.items():
        offset = offsets.get(name, -1)
        if offset < 0 or offset + width > stride:
            raise RuntimeError(f"compact Building DWARF field {name} exceeds or is absent from stride")
    data = rsp.read(address, capacity * stride)
    owners, types, entities = {}, {}, []
    for slot in range(capacity):
        row = data[slot * stride:(slot + 1) * stride]
        item = {name: (list(struct.unpack_from("<" + fmt, row, offsets[name])) if name == "queue"
                       else struct.unpack_from("<" + fmt, row, offsets[name])[0])
                for name, (_, fmt) in widths.items()}
        if not item["alive"]:
            continue
        owner, kind = str(item["owner"]), str(item["type"])
        owners[owner] = owners.get(owner, 0) + 1
        owner_types = types.setdefault(owner, {})
        owner_types[kind] = owner_types.get(kind, 0) + 1
        entities.append({"slot": slot, **item})
    return {"alive": len(entities), "by_owner": owners, "by_owner_type": types, "entities": entities}


def count_chrysalis(units: dict, buildings: dict) -> int:
    """Count raw source 150 in either compact actor bank."""
    unit_count = sum(1 for item in units.get("entities", [])
                     if item.get("owner") == 0 and item.get("raw_type") == 150)
    building_count = sum(1 for item in buildings.get("entities", [])
                         if item.get("owner") == 0 and item.get("raw_type") == 150)
    return unit_count + building_count


def compact_bank_profile(elf: Path, sizes: dict[str, int], profile_name: str) -> dict:
    """Validate the staged compact banks without using legacy int-only readers."""
    if profile_name not in MISSION_BY_PROFILE:
        raise ValueError("unknown compact campaign profile")
    unit_stride, unit_offsets = races.dwarf_struct(elf, "Unit")
    building_stride, building_offsets = races.dwarf_struct(elf,
        "Sc64ZpCompactBuilding" if profile_name in PERSISTENT_TYPES else "Building")
    capacity = 192 if profile_name == "zerg-z4" else 240
    building_capacity = 160 if profile_name in PERSISTENT_TYPES else 96
    expected_stride = 48 if profile_name in PERSISTENT_TYPES else 100
    if (unit_stride != 56 or building_stride != expected_stride or
            sizes.get("u") != capacity * unit_stride or
            sizes.get("b") != building_capacity * building_stride):
        raise RuntimeError("staged compact entity banks differ from the bounded profile")
    for field in ("owner", "type", "alive", "x", "y", "hp", "raw_type"):
        if field not in unit_offsets:
            raise RuntimeError(f"compact Unit DWARF lacks {field}")
    if profile_name in ("protoss-p6", "zerg-z6"):
        for field in ("raw_owner", "order"):
            if field not in unit_offsets:
                raise RuntimeError(f"P6 compact Unit DWARF lacks {field}")
    elif profile_name == "zerg-z7":
        for field in ("raw_owner", "order"):
            if field not in unit_offsets:
                raise RuntimeError(f"Z7 compact Unit DWARF lacks {field}")
        if "raw_owner" not in building_offsets:
            raise RuntimeError("Z7 compact Building DWARF lacks raw_owner")
    for field in ("owner", "type", "alive", "x", "y", "hp", "raw_type", "under_construction"):
        offset = building_offsets.get(field)
        width = (4 if field == "hp" else 2 if field in ("x", "y", "raw_type") else 1) \
            if profile_name in PERSISTENT_TYPES else 4
        if offset is None or (profile_name not in PERSISTENT_TYPES and offset % 4) or not 0 <= offset <= building_stride - width:
            raise RuntimeError(f"Building DWARF lacks a bounded int field {field}")
    return {"unit_bank_bytes": sizes["u"], "building_bank_bytes": sizes["b"],
            "unit_capacity": capacity, "building_capacity": building_capacity,
            "unit_stride_bytes": unit_stride, "building_stride_bytes": building_stride,
            "unit_offsets": unit_offsets, "building_offsets": building_offsets}


def run(trace_path: Path, output: Path, rom_path: Path, elf_path: Path,
        profile_name: str, ticks: int | None = None, save_at_end: bool = False,
        expected_rom_sha256: str | None = None, expected_elf_sha256: str | None = None,
        expected_trace_sha256: str | None = None, attempt_prefix: bool = False) -> dict:
    if attempt_prefix and (profile_name != "zerg-z7" or ticks is None or save_at_end):
        raise ValueError("--attempt-prefix requires zerg-z7 with --ticks and forbids --save-at-end")
    trace_bytes = trace_path.read_bytes()
    trace_hash = hashlib.sha256(trace_bytes).hexdigest()
    rom_hash = hashlib.sha256(rom_path.read_bytes()).hexdigest()
    elf_hash = hashlib.sha256(elf_path.read_bytes()).hexdigest()
    for label, expected, actual in (("ROM", expected_rom_sha256, rom_hash),
                                    ("ELF", expected_elf_sha256, elf_hash),
                                    ("trace", expected_trace_sha256, trace_hash)):
        if expected and expected.lower() != actual:
            raise RuntimeError(f"{label} SHA-256 mismatch: expected {expected}, observed {actual}")
    trace = json.loads(trace_bytes)
    sym, sizes = symbols(elf_path), symbol_sizes(elf_path)
    mode, capacity = scheduler.schedule_mode(sym, sizes)
    checkpoint_ticks = scheduler.checkpoint_ticks(sym)
    plan = validate_trace(trace, capacity, profile_name, windowed=mode == "windowed",
                          window_ticks=checkpoint_ticks, attempt_prefix=attempt_prefix,
                          prefix_ticks=ticks if attempt_prefix else None)
    if attempt_prefix and (ticks is None or ticks >= plan["stop_tick"] or save_at_end):
        raise ValueError("--attempt-prefix requires --ticks before the failed/limited terminal tick and forbids --save-at-end")
    if profile_name == "zerg-z7" and not attempt_prefix and plan["final"].get("kind") != "won":
        raise ValueError("full Z7 replay requires a winning controller trace; use --attempt-prefix for a failed/limited attempt")
    requested, prefix = scheduler.run_length(plan, ticks, save_at_end)
    replay_plan = ({**plan, "inputs": [event for event in plan["inputs"] if event[0] < requested]}
                   if attempt_prefix else plan)
    if save_at_end and mode != "windowed":
        raise ValueError("--save-at-end requires a windowed scheduler")
    metadata = split.read_campaign_metadata(rom_path, elf_path, FACTION_BY_PROFILE[profile_name])
    mission = MISSION_BY_PROFILE[profile_name]
    if mission not in metadata["enabled_mission_ids"]:
        raise RuntimeError(f"matching ROM/ELF catalog does not enable global mission {mission}")
    bank = compact_bank_profile(elf_path, sizes, profile_name)
    campaign_layout = races.dwarf_struct(elf_path, "Sc64RaceCampaignState")
    runtime_layout = runtime_layout_for(elf_path, profile_name)
    model_layout = races.dwarf_struct(elf_path, MODEL_TYPES[profile_name])
    required = ("game_update", "demo_schedule_events", "demo_schedule_count", "demo_schedule_length",
                "demo_schedule_tick", "demo_replay_checkpoint", "screen", "frame", "won", "lost",
                "race_mode", "race_selected_mission", "race_completed", "race_campaign",
                "race_minerals", "race_gas", "race_front", "u", "b")
    required += (RUNTIME_SYMBOLS[profile_name],)
    if mode == "windowed":
        required += ("demo_schedule_active", "demo_schedule_index")
    missing = [name for name in required if name not in sym]
    if missing:
        raise RuntimeError("Z/P campaign ELF is missing symbols: " + ", ".join(missing))
    run_dir = output / f"{profile_name}-{time.time_ns()}"
    run_dir.mkdir(parents=True, exist_ok=False)
    history_path = run_dir / "checkpoints.jsonl"
    split.FRONT_FIELDS = split.frontend_offsets(elf_path)
    scheduler.check_debugger_port_free()
    proc = sock = rsp = log = None
    try:
        proc, sock, rsp, log = split.start_emulator(run_dir.name, rom_path, run_dir, sym)
        initial = {"screen": profile.u32(rsp, sym["screen"]),
                   "frame": profile.u32(rsp, sym["frame"]),
                   "page": profile.u32(rsp, sym["race_front"] + split.FRONT_FIELDS["page"])}
        if initial != {"screen": SCREEN_TITLE, "frame": 0, "page": split.FRONT_HOME}:
            raise RuntimeError(f"fresh boot did not remain on HOME: {initial}")
        home_capture = capture(rsp, run_dir / "home.png")

        pages = scheduler.make_schedule_pages(replay_plan, requested, capacity, checkpoint_ticks) if mode == "windowed" else None
        def write_page(events: list[tuple[int, int]]) -> None:
            blob = b"".join(struct.pack("<2I", tick, held) for tick, held in events)
            for offset in range(0, len(blob), 256):
                rsp.write(sym["demo_schedule_events"] + offset, blob[offset:offset + 256])
            rsp.write(sym["demo_schedule_count"], struct.pack("<I", len(events)))

        if mode == "windowed":
            assert pages is not None
            write_page(pages[0]["events"])
            rsp.write(sym["demo_schedule_index"], struct.pack("<I", 0))
            rsp.write(sym["demo_schedule_length"], struct.pack("<I", requested))
            rsp.write(sym["demo_schedule_active"], struct.pack("<I", 1))
        else:
            blob = b"".join(struct.pack("<2I", tick, held) for tick, held in replay_plan["inputs"])
            for offset in range(0, len(blob), 256):
                rsp.write(sym["demo_schedule_events"] + offset, blob[offset:offset + 256])
            rsp.write(sym["demo_schedule_count"], struct.pack("<I", len(replay_plan["inputs"])))
            rsp.write(sym["demo_schedule_length"], struct.pack("<I", requested))
        if rsp.cmd(f"z1,{sym['game_update'] & ~1:x},2") != "OK" or rsp.cmd(
                f"Z1,{sym['demo_replay_checkpoint'] & ~1:x},2") != "OK":
            raise RuntimeError("failed to install input-only replay observer")
        rsp.tick()
        if profile.u32(rsp, sym["demo_schedule_tick"]) != 0:
            raise RuntimeError("scheduler advanced before its first scheduled input")

        checkpoints = []
        prior_state = mission_snapshot(rsp, sym, sizes, bank, campaign_layout,
                                       runtime_layout, model_layout, profile_name)
        prior_clock = profile.hardware_clock(rsp)
        counted_frames = counted_cycles = 0

        def persist_checkpoint(checkpoint: dict, status: str = "running") -> None:
            checkpoints.append(checkpoint)
            with history_path.open("a", encoding="utf-8") as stream:
                stream.write(json.dumps(checkpoint, separators=(",", ":")) + "\n")
                stream.flush()
                os.fsync(stream.fileno())
            profile.write_progress(
                run_dir / "progress.json", profile=f"{profile_name}-campaign", rom=rom_path,
                elf=elf_path, rom_sha256=rom_hash, elf_sha256=elf_hash,
                configuration={"profile": profile_name, "mission": mission,
                               "trace": str(trace_path), "trace_sha256": trace_hash,
                               "input_transitions": plan["input_count"],
                               "scheduler_mode": mode, "scheduler_capacity": capacity,
                               "trace_stop_tick": plan["stop_tick"], "requested_ticks": requested,
                               "prefix": prefix, "campaign_catalog": metadata},
                requested_ticks=requested, checkpoint_interval=checkpoint_ticks,
                latest_checkpoint=checkpoint, status=status)

        tick_zero = {"tick": 0, "host_tick": 0, "elapsed_hardware_cycles": 0,
                     "frame_delta": 0, "counted_live_play_interval": False,
                     "effective_fps": None, "capture": str(run_dir / "home.png"),
                     **prior_state}
        cycles, routes = scheduler.profile_counters(rsp, sym)
        tick_zero["profile_cycles_last_tick"] = cycles
        tick_zero["route_counters"] = routes
        persist_checkpoint(tick_zero)
        last_tick = 0
        while last_tick < requested:
            rsp.tick()
            completed = profile.u32(rsp, sym["demo_schedule_tick"])
            expected = min(requested, last_tick + checkpoint_ticks)
            if completed != expected:
                raise RuntimeError(f"expected checkpoint tick {expected}, observed {completed}")
            if mode == "windowed":
                page = pages[(completed - 1) // checkpoint_ticks]
                if (profile.u32(rsp, sym["demo_schedule_index"]) != len(page["events"]) or
                        profile.u32(rsp, sym["demo_schedule_count"]) != len(page["events"])):
                    raise RuntimeError(f"scheduler page incomplete at tick {completed}")
                if completed < requested:
                    write_page(pages[completed // checkpoint_ticks]["events"])
            now = profile.hardware_clock(rsp)
            elapsed = (now - prior_clock) & 0xFFFFFFFF
            state = mission_snapshot(rsp, sym, sizes, bank, campaign_layout,
                                     runtime_layout, model_layout, profile_name)
            frame_delta = (state["frame"] - prior_state["frame"]) & 0xFFFFFFFF
            live = prior_state["screen"] == SCREEN_PLAY and state["screen"] == SCREEN_PLAY
            counted = bool(live and frame_delta > 0)
            interval_fps = (round(profile.CPU_HZ * frame_delta / max(1, elapsed), 2)
                            if counted else None)
            if counted:
                counted_frames += frame_delta
                counted_cycles += elapsed
            checkpoint = {"tick": completed, "host_tick": completed,
                          "elapsed_hardware_cycles": elapsed, "frame_delta": frame_delta,
                          "counted_live_play_interval": counted, "effective_fps": interval_fps,
                          **state}
            cycles, routes = scheduler.profile_counters(rsp, sym)
            checkpoint["profile_cycles_last_tick"] = cycles
            checkpoint["route_counters"] = routes
            if completed in (1000, 10000, 30000, requested):
                checkpoint["capture"] = capture(rsp, run_dir / f"checkpoint-{completed:06d}.png")
            persist_checkpoint(checkpoint)
            prior_state, prior_clock = state, now
            last_tick = completed
            print(f"{profile_name} tick={completed}/{requested} frame={state['frame']} "
                  f"screen={state['screen']}", flush=True)
        final = checkpoints[-1]
        if not prefix:
            verify_final(plan["final"], final, profile_name)
        report = {"status": "prefix" if prefix else "won", "profile": profile_name,
                  "mission": mission, "rom": str(rom_path), "elf": str(elf_path),
                  "trace": str(trace_path), "rom_sha256": rom_hash, "elf_sha256": elf_hash,
                  "trace_sha256": trace_hash, "input_transitions": plan["input_count"],
                  "scheduler_mode": mode, "scheduler_capacity": capacity,
                  "checkpoint_interval": checkpoint_ticks,
                  "peak_page_transitions": plan["peak_page_transitions"],
                  "trace_stop_tick": plan["stop_tick"], "requested_ticks": requested,
                  "observed_ticks": final["tick"], "prefix": prefix,
                  "counted_live_play_frames": counted_frames,
                  "counted_live_play_hardware_cycles": counted_cycles,
                  "mean_live_play_effective_fps": round(profile.CPU_HZ * counted_frames /
                                                          max(1, counted_cycles), 2),
                  "timer_method": f"GBA chained hardware timer deltas at {checkpoint_ticks}-tick checkpoints",
                  "fps_exclusions": "Initial HOME/start and final RESULT intervals are excluded; "
                                    "only positive frame deltas between PLAY checkpoints contribute.",
                  "initial_home": initial, "initial_home_capture": home_capture,
                  "campaign_catalog": metadata,
                  "bank_layout": {key: bank[key] for key in
                                  ("unit_capacity", "building_capacity", "unit_stride_bytes",
                                   "building_stride_bytes", "unit_offsets", "building_offsets")},
                  "checkpoint_history": str(history_path),
                  "checkpoints": checkpoints, "final": final,
                  "verified_win": not prefix}
        if save_at_end:
            if final["screen"] != SCREEN_PLAY:
                raise RuntimeError("--save-at-end requires a live PLAY prefix")
            rsp.write(sym["demo_schedule_active"], struct.pack("<I", 0))
            rsp.write(sym["demo_schedule_count"], struct.pack("<I", 0))
            saved = profile.save_live_checkpoint(rsp, sym, tick=final["tick"])
            saved["state"] = mission_snapshot(rsp, sym, sizes, bank, campaign_layout,
                                               runtime_layout, model_layout, profile_name)
            menu.stop_emulator(proc, sock, log)
            proc = sock = log = None
            save_dir = run_dir / "saves" / run_dir.name
            saves = list(save_dir.glob("*.sav"))
            if len(saves) != 1:
                raise RuntimeError(f"checkpoint did not flush exactly one SRAM file: {saves}")
            save_path = profile.checked_sram(saves[0])
            saved.update({"path": str(save_path),
                          "sha256": hashlib.sha256(save_path.read_bytes()).hexdigest()})
            report["save_checkpoint"] = saved
        report_path = run_dir / "profile.json"
        report_path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
        profile.write_progress(
            run_dir / "progress.json", profile=f"{profile_name}-campaign", rom=rom_path,
            elf=elf_path, rom_sha256=rom_hash, elf_sha256=elf_hash,
            configuration={"profile": profile_name, "mission": mission,
                           "trace": str(trace_path), "trace_sha256": trace_hash,
                           "input_transitions": plan["input_count"], "scheduler_mode": mode,
                           "scheduler_capacity": capacity, "trace_stop_tick": plan["stop_tick"],
                           "requested_ticks": requested, "prefix": prefix},
            requested_ticks=requested, checkpoint_interval=checkpoint_ticks,
            latest_checkpoint=final, status="prefix" if prefix else "finished",
            final_report=str(report_path))
        print(json.dumps({"status": report["status"], "ticks": requested,
                          "trace_sha256": trace_hash, "report": str(report_path)}, indent=2))
        return report
    finally:
        if proc is not None:
            try:
                menu.stop_emulator(proc, sock, log)
            except Exception:
                proc.terminate()
                proc.wait(timeout=3)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--profile", choices=tuple(MISSION_BY_PROFILE), required=True)
    parser.add_argument("--trace", type=Path, help="controller trace (profile-specific private default)")
    parser.add_argument("--rom", type=Path)
    parser.add_argument("--elf", type=Path)
    parser.add_argument("--validate-only", action="store_true",
                        help="validate trace schema, edges and recorded state without launching an emulator; print JSON to stdout (--output is replay-only)")
    parser.add_argument("--validation-capacity", type=int, default=256,
                        help="controller queue capacity for --validate-only")
    parser.add_argument("--validation-window-ticks", type=int, choices=(250, 1000), default=250,
                        help="scheduler window for --validate-only")
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--ticks", type=int, help="replay a positive trace prefix")
    parser.add_argument("--attempt-prefix", action="store_true",
                        help="Z7 only: replay a failed/limited attempt prefix; never qualifies as a win")
    parser.add_argument("--save-at-end", action="store_true", help="pause/save at a live prefix endpoint")
    parser.add_argument("--expect-rom-sha256")
    parser.add_argument("--expect-elf-sha256")
    parser.add_argument("--expect-trace-sha256")
    args = parser.parse_args(argv)
    if not args.validate_only and (args.rom is None or args.elf is None):
        parser.error("--rom and --elf are required unless --validate-only is used")
    try:
        trace_path = args.trace or DEFAULT_TRACES[args.profile]
        if args.validate_only:
            if args.expect_rom_sha256 or args.expect_elf_sha256:
                raise ValueError("ROM/ELF hash expectations require an emulated replay; offline mode checks only the trace")
            raw = trace_path.read_bytes()
            trace_hash = hashlib.sha256(raw).hexdigest()
            if args.attempt_prefix and (args.profile != "zerg-z7" or args.ticks is None or args.save_at_end):
                raise ValueError("--attempt-prefix requires zerg-z7 with --ticks and forbids --save-at-end")
            if args.expect_trace_sha256 and trace_hash != args.expect_trace_sha256.lower():
                raise ValueError("trace SHA256 differs from the expected value")
            plan = validate_trace(json.loads(raw), args.validation_capacity, args.profile,
                                  windowed=True, window_ticks=args.validation_window_ticks,
                                  attempt_prefix=args.attempt_prefix,
                                  prefix_ticks=args.ticks if args.attempt_prefix else None)
            requested, prefix = scheduler.run_length(plan, args.ticks, args.save_at_end)
            print(json.dumps({"status": "trace_validated_offline", "profile": args.profile,
                              "trace_sha256": trace_hash, "input_count": plan["input_count"],
                              "stop_tick": plan["stop_tick"],
                              "requested_ticks": requested, "prefix": prefix,
                              "peak_page_transitions": plan["peak_page_transitions"],
                              "scope": "recorded trace contract only; no emulation"}, indent=2))
            return 0
        run(trace_path, args.output, args.rom, args.elf, args.profile, args.ticks,
            args.save_at_end, args.expect_rom_sha256, args.expect_elf_sha256,
            args.expect_trace_sha256, args.attempt_prefix)
    except (OSError, ValueError, RuntimeError, json.JSONDecodeError) as exc:
        print(f"Z/P controller replay failed: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
