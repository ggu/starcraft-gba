#!/usr/bin/env python3
"""Replay a unified skirmish controller trace on a supplied ARM ROM/ELF.

Only held controller inputs are scheduled. The trace's start/action records
are observations; live configuration, entity counts, result flags and frame
are read back from the running game at bounded checkpoints and at the exact
terminal tick.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import socket
import struct
import sys
import time

TOOLS = Path(__file__).resolve().parent
if str(TOOLS) not in sys.path:
    sys.path.insert(0, str(TOOLS))

import sc64_races_profile as races
import sc64_split_profile as timer_profile
import sc64_split_smoke as split
import sc64_z3_controller_replay as scheduler
from emulator_smoke import ROOT, capture, symbol_sizes, symbols

SCREEN_TITLE, SCREEN_PLAY, SCREEN_RESULT = 0, 1, 3
CHECKPOINT_TICKS = 1000
MAX_U32 = 0xFFFFFFFF
KEY_A, KEY_LEFT, KEY_RIGHT, KEY_UP, KEY_DOWN = 1, 32, 16, 64, 128
MENU_KEYS = KEY_A | KEY_LEFT | KEY_RIGHT | KEY_UP | KEY_DOWN
BOT_OWNERS = (1, 3, 4)
DEFAULT_OUTPUT = ROOT / ".local/sc64/skirmish/arm-replay"

UNIT_WIDTHS = {
    "owner": (1, False), "type": (1, False), "alive": (1, False),
    "x": (2, True), "y": (2, True), "hp": (4, True), "raw_type": (2, True),
}
COMPACT_BUILDING_WIDTHS = {
    "owner": (1, False), "type": (1, False), "alive": (1, False),
    "x": (2, True), "y": (2, True), "hp": (4, True), "raw_type": (2, True),
    "under_construction": (1, False),
}


def _integer(value: object, name: str, low: int = 0, high: int = MAX_U32) -> int:
    if type(value) is not int or not low <= value <= high:
        raise ValueError(f"{name} must be an integer in [{low},{high}]")
    return value


def _exact_keys(row: dict, expected: set[str], label: str) -> None:
    if set(row) != expected:
        raise ValueError(f"{label} fields differ from the supported trace schema")


def _check_counts(counts: object, label: str) -> dict:
    if not isinstance(counts, dict):
        raise ValueError(f"{label} must be an object")
    _exact_keys(counts, {"owner", "race", "units", "buildings"}, label)
    _integer(counts["owner"], f"{label}.owner", 0, 4)
    _integer(counts["race"], f"{label}.race", 0, 2)
    _integer(counts["units"], f"{label}.units", 0, 240)
    _integer(counts["buildings"], f"{label}.buildings", 0, 160)
    return counts


def validate_trace(trace: object, capacity: int, *, windowed: bool = True,
                   checkpoint_interval: int = CHECKPOINT_TICKS) -> dict:
    """Validate the exact C controller trace and return scheduler input data."""
    if type(capacity) is not int or capacity < 1:
        raise ValueError("controller event capacity must be positive")
    if not windowed:
        raise ValueError("skirmish full-match replay requires the windowed input scheduler")
    if type(checkpoint_interval) is not int or checkpoint_interval not in (250, 1000):
        raise ValueError("unsupported scheduler checkpoint interval")
    if not isinstance(trace, dict):
        raise ValueError("trace must be a JSON object")
    _exact_keys(trace, {"profile", "events", "terminal"}, "trace")
    if trace["profile"] != "unified-race-skirmish":
        raise ValueError("trace profile must be unified-race-skirmish")
    rows = trace["events"]
    if not isinstance(rows, list) or not rows:
        raise ValueError("trace events must be a nonempty array")

    transitions: list[tuple[int, int]] = []
    inputs: list[tuple[int, int]] = []
    input_by_tick: dict[int, dict] = {}
    previous_tick = previous_frame = 0
    previous_held = 0
    previous_event_tick = 0
    start = None
    start_index = -1
    action_count = 0
    last_event_frame = 0

    for index, row in enumerate(rows):
        if not isinstance(row, dict):
            raise ValueError(f"event {index} must be an object")
        kind = row.get("kind")
        tick = _integer(row.get("host_tick"), f"event {index}.host_tick", 1, MAX_U32)
        if tick < previous_event_tick:
            raise ValueError(f"event {index} timestamps are not ordered")
        previous_event_tick = tick

        if kind == "input":
            _exact_keys(row, {"kind", "host_tick", "frame", "held", "pressed"},
                        f"input event {index}")
            if tick <= previous_tick:
                raise ValueError("input host_tick values must be strictly increasing")
            frame = _integer(row["frame"], f"input event {index}.frame")
            held = _integer(row["held"], f"input event {index}.held", 0, scheduler.KEY_MASK)
            pressed = _integer(row["pressed"], f"input event {index}.pressed", 0, scheduler.KEY_MASK)
            if held == previous_held:
                raise ValueError(f"input event {index} is not a held-state transition")
            if pressed != (held & ~previous_held):
                raise ValueError(f"input event {index} pressed does not match held transition")
            if frame < previous_frame:
                raise ValueError(f"input event {index} frame moved backwards")
            transitions.append((tick, held))
            inputs.append((tick - 1, held))
            input_by_tick[tick] = row
            previous_tick, previous_held, previous_frame = tick, held, frame
            last_event_frame = frame
        elif kind == "start":
            if start is not None or start_index >= 0:
                raise ValueError("trace must contain exactly one start event")
            _exact_keys(row, {"kind", "host_tick", "frame", "map", "human_race", "bots"},
                        "start event")
            start = row
            start_index = index
            start_frame = _integer(row["frame"], "start.frame", 1)
            _integer(row["map"], "start.map", 0, 1)
            _integer(row["human_race"], "start.human_race", 0, 2)
            if tick != previous_tick or tick not in input_by_tick:
                raise ValueError("start marker must follow the final normal-menu input transition")
            if input_by_tick[tick]["held"] != 0 or input_by_tick[tick]["frame"] != start_frame:
                raise ValueError("start marker must follow the released launch input at its recorded frame")
            if index == 0:
                raise ValueError("normal HOME/menu inputs must precede the start marker")
            last_event_frame = start_frame
        elif kind == "action":
            if start is None:
                raise ValueError("action observation appears before normal menu launch")
            action = row.get("action")
            fields = {
                "build": {"kind", "action", "host_tick", "frame", "type", "worker", "x", "y",
                          "minerals_before", "minerals_after", "gas_before", "gas_after"},
                "queue": {"kind", "action", "host_tick", "frame", "producer", "type",
                          "minerals_before", "minerals_after", "gas_before", "gas_after"},
                "attack_move": {"kind", "action", "host_tick", "frame", "x", "y", "selected_issued"},
            }.get(action)
            if fields is None:
                raise ValueError(f"action event {index} has unsupported action")
            _exact_keys(row, fields, f"action event {index}")
            frame = _integer(row["frame"], f"action event {index}.frame")
            if tick not in input_by_tick or input_by_tick[tick]["frame"] != frame:
                raise ValueError("action observation must follow a recorded UI input at the same frame")
            if action == "attack_move":
                _integer(row["x"], "attack_move.x", 0, 2047)
                _integer(row["y"], "attack_move.y", 0, 2047)
                _integer(row["selected_issued"], "attack_move.selected_issued", 1, 240)
            else:
                _integer(row["type"], f"{action}.type", 0, 255)
                if action == "build":
                    _integer(row["worker"], "build.worker", 0, 239)
                    _integer(row["x"], "build.x", 0, 2047)
                    _integer(row["y"], "build.y", 0, 2047)
                else:
                    _integer(row["producer"], "queue.producer", 0, 159)
                for field in ("minerals_before", "minerals_after", "gas_before", "gas_after"):
                    _integer(row[field], f"{action}.{field}", 0, 0x7FFFFFFF)
            action_count += 1
            if frame < previous_frame:
                raise ValueError(f"action event {index} frame moved backwards")
            last_event_frame = frame
        else:
            raise ValueError(f"event {index} has unsupported kind {kind!r}")

    if start is None:
        raise ValueError("trace is missing its start event")
    if not inputs or inputs[:4] != [(0, KEY_DOWN), (1, 0), (2, KEY_A), (3, 0)]:
        raise ValueError("trace must enter Skirmish through ordinary HOME navigation")
    launch_tick = start["host_tick"]
    prelaunch = [row for row in rows if row["kind"] == "input" and row["host_tick"] <= launch_tick]
    if any(row["held"] & ~MENU_KEYS for row in prelaunch):
        raise ValueError("prelaunch trace contains a gameplay-only key")
    launch_keys = [row for row in prelaunch if row["held"]]
    if not launch_keys or launch_keys[-1]["held"] != KEY_A or prelaunch[-1]["held"] != 0:
        raise ValueError("normal menu launch must use A and release it before start")

    bots = start["bots"]
    human_race = start["human_race"]
    bot_count = len(bots) if isinstance(bots, list) else 0
    if not 1 <= bot_count <= 3:
        raise ValueError("start must configure one to three opponents")
    if not isinstance(bots, list):
        raise ValueError("start.bots must be an array")
    for index, bot in enumerate(bots):
        _exact_keys(bot, {"owner", "race"}, f"start bot {index}")
        _integer(bot["owner"], f"start bot {index}.owner", 0, 4)
        _integer(bot["race"], f"start bot {index}.race", 0, 2)
        if bot["owner"] != BOT_OWNERS[index] or bot["race"] != (human_race + 1 + index) % 3:
            raise ValueError("start bot owners/races do not match the supported skirmish setup")
    if start_index <= 0 or start_index >= len(rows) - 1:
        raise ValueError("start marker is not between input events and terminal timeline")

    terminal = trace["terminal"]
    if not isinstance(terminal, dict):
        raise ValueError("terminal must be an object")
    _exact_keys(terminal, {"kind", "host_tick", "frame", "won", "lost", "map", "human_race",
                           "bot_count", "human", "bots"}, "terminal")
    kind = terminal["kind"]
    if kind not in ("won", "lost"):
        raise ValueError("trace must contain a completed win or loss, not a tick limit")
    stop_tick = _integer(terminal["host_tick"], "terminal.host_tick", 1, MAX_U32)
    terminal_frame = _integer(terminal["frame"], "terminal.frame")
    _integer(terminal["won"], "terminal.won", 0, 1)
    _integer(terminal["lost"], "terminal.lost", 0, 1)
    _integer(terminal["map"], "terminal.map", 0, 1)
    _integer(terminal["human_race"], "terminal.human_race", 0, 2)
    _integer(terminal["bot_count"], "terminal.bot_count", 1, 3)
    if stop_tick < previous_tick or stop_tick < previous_event_tick or terminal_frame < last_event_frame:
        raise ValueError("terminal must follow all recorded inputs and observations")
    if terminal["map"] != start["map"] or terminal["human_race"] != human_race or terminal["bot_count"] != bot_count:
        raise ValueError("terminal match configuration differs from its start marker")
    if (terminal["won"], terminal["lost"]) != ((1, 0) if kind == "won" else (0, 1)):
        raise ValueError("terminal kind and result flags contradict each other")
    human = _check_counts(terminal["human"], "terminal.human")
    if human["owner"] != 0 or human["race"] != human_race:
        raise ValueError("terminal human identity differs from the launched side")
    terminal_bots = terminal["bots"]
    if not isinstance(terminal_bots, list) or len(terminal_bots) != bot_count:
        raise ValueError("terminal bot list differs from the launched opponent count")
    for index, bot in enumerate(terminal_bots):
        _check_counts(bot, f"terminal bot {index}")
        if bot["owner"] != BOT_OWNERS[index] or bot["race"] != bots[index]["race"]:
            raise ValueError("terminal bot identity differs from normal menu configuration")
    if kind == "won":
        if human["buildings"] < 1 or any(bot["buildings"] != 0 for bot in terminal_bots):
            raise ValueError("forged win: human must retain a building and every active bot must be eliminated")
    elif human["buildings"] != 0:
        raise ValueError("forged loss: human buildings must be eliminated")
    if not inputs or inputs[-1][0] >= stop_tick:
        raise ValueError("terminal tick must follow the last scheduled input update")
    if terminal_frame < start_frame:
        raise ValueError("terminal frame precedes the launched match")

    plan = {"inputs": inputs, "input_count": len(inputs), "stop_tick": stop_tick,
            "start": start, "terminal": terminal, "action_count": action_count}
    pages = scheduler.make_schedule_pages(plan, stop_tick, capacity, checkpoint_interval)
    plan["pages"] = pages
    plan["peak_page_transitions"] = max((len(page["events"]) for page in pages), default=0)
    plan["scheduler_mode"] = "windowed"
    plan["checkpoint_interval"] = checkpoint_interval
    return plan


def decode_scalar_array(data: bytes, count: int, width: int, *, signed: bool = False) -> list[int]:
    """Decode a bounded scalar array using an ELF-derived element width."""
    if type(count) is not int or count < 0 or type(width) is not int or width not in (1, 2, 4):
        raise ValueError("invalid scalar-array shape")
    if len(data) != count * width:
        raise ValueError("scalar-array byte count does not match its ELF layout")
    return [int.from_bytes(data[i:i + width], "little", signed=signed)
            for i in range(0, len(data), width)]


def decode_entity_bank(data: bytes, capacity: int, stride: int, offsets: dict[str, int],
                       widths: dict[str, tuple[int, bool]], *, required: tuple[str, ...],
                       legacy_i32: bool = False) -> dict:
    """Decode live entity rows from ELF DWARF offsets and the documented ABI."""
    if type(capacity) is not int or capacity < 1 or type(stride) is not int or stride < 1:
        raise ValueError("invalid entity-bank dimensions")
    if len(data) != capacity * stride:
        raise ValueError("entity-bank bytes do not match its ELF symbol size")
    field_widths = ({name: (4, True) for name in required} if legacy_i32 else widths)
    for name in required:
        width, _ = field_widths[name]
        offset = offsets.get(name, -1)
        if offset < 0 or offset + width > stride:
            raise ValueError(f"entity field {name} is absent or exceeds its DWARF stride")

    entities = []
    by_owner: dict[str, int] = {}
    by_owner_type: dict[str, dict[str, int]] = {}
    for slot in range(capacity):
        row = data[slot * stride:(slot + 1) * stride]
        values = {}
        for name in required:
            width, signed = field_widths[name]
            values[name] = int.from_bytes(row[offsets[name]:offsets[name] + width],
                                          "little", signed=signed)
        if not values["alive"]:
            continue
        owner, kind = values["owner"], values["type"]
        if not 0 <= owner <= 4:
            raise ValueError(f"live entity slot {slot} has invalid owner {owner}")
        by_owner[str(owner)] = by_owner.get(str(owner), 0) + 1
        owner_types = by_owner_type.setdefault(str(owner), {})
        owner_types[str(kind)] = owner_types.get(str(kind), 0) + 1
        entities.append({"slot": slot, **values})
    return {"alive": len(entities), "by_owner": by_owner,
            "by_owner_type": by_owner_type, "entities": entities}


def bank_layout(elf: Path, sizes: dict[str, int]) -> dict:
    unit_stride, unit_offsets = races.dwarf_struct(elf, "Unit")
    unit_bytes, building_bytes = sizes.get("u", 0), sizes.get("b", 0)
    building_name = "Building"
    if building_bytes == 160 * 48:
        try:
            building_stride, building_offsets = races.dwarf_struct(
                elf, "Sc64ZpCompactBuilding")
            building_name = "Sc64ZpCompactBuilding"
        except RuntimeError:
            building_stride, building_offsets = races.dwarf_struct(elf, "Building")
    else:
        building_stride, building_offsets = races.dwarf_struct(elf, "Building")
    if (not unit_bytes or not building_bytes or unit_bytes % unit_stride or
            building_bytes % building_stride):
        raise RuntimeError("entity-bank symbol sizes do not agree with Unit/Building DWARF")
    unit_capacity, building_capacity = unit_bytes // unit_stride, building_bytes // building_stride
    if not 1 <= unit_capacity <= 240 or not 1 <= building_capacity <= 160:
        raise RuntimeError("entity-bank capacities exceed supported unified-race limits")
    if unit_stride not in (56, 96, 100, 108) or building_stride not in (48, 100, 112):
        raise RuntimeError(f"unsupported Unit/{building_name} layout {unit_stride}/{building_stride}")
    return {"unit_stride": unit_stride, "unit_offsets": unit_offsets,
            "unit_capacity": unit_capacity, "building_stride": building_stride,
            "building_offsets": building_offsets, "building_capacity": building_capacity,
            "building_type": building_name}


def checkpoint_observer_symbol(sym: dict[str, int]) -> str:
    """Use the normal observer; dense builds call it through their period marker."""
    if "demo_replay_checkpoint" not in sym:
        raise RuntimeError("ELF is missing the normal replay checkpoint observer")
    return "demo_replay_checkpoint"


def snapshot(rsp, sym: dict[str, int], sizes: dict[str, int], bank: dict) -> dict:
    unit_stride = bank["unit_stride"]
    unit_compact = unit_stride == 56
    unit_widths = UNIT_WIDTHS
    unit_required = ("owner", "type", "alive", "x", "y", "hp", "raw_type")
    units = decode_entity_bank(rsp.read(sym["u"], sizes["u"]), bank["unit_capacity"],
                               unit_stride, bank["unit_offsets"], unit_widths,
                               required=unit_required, legacy_i32=not unit_compact)
    building_stride = bank["building_stride"]
    building_compact = building_stride == 48
    building_required = ("owner", "type", "alive")
    building_widths = {**COMPACT_BUILDING_WIDTHS}
    if "raw_type" in bank["building_offsets"]:
        building_required += ("raw_type",)
    building_required += ("x", "y", "hp")
    if "under_construction" in bank["building_offsets"]:
        building_required += ("under_construction",)
    buildings = decode_entity_bank(rsp.read(sym["b"], sizes["b"]), bank["building_capacity"],
                                    building_stride, bank["building_offsets"], building_widths,
                                    required=building_required, legacy_i32=not building_compact)

    state = {name: races.read_u32(rsp, sym[name]) for name in
             ("screen", "frame", "won", "lost", "race_mode", "race_human",
              "race_bots", "race_skirmish_map")}
    bot_race_size = sizes.get("race_bot_races", 0)
    bot_owner_size = sizes.get("race_bot_owner_ids", 0)
    if bot_race_size != 12 or bot_owner_size != 12:
        raise RuntimeError("ELF must expose three 32-bit race and owner configuration slots")
    state["bot_races"] = decode_scalar_array(rsp.read(sym["race_bot_races"], bot_race_size), 3, 4, signed=True)
    state["bot_owners"] = decode_scalar_array(rsp.read(sym["race_bot_owner_ids"], bot_owner_size), 3, 4, signed=True)
    state["units"], state["buildings"] = units, buildings
    return state


def verify_configuration(trace_start: dict, state: dict) -> None:
    bots = trace_start["bots"]
    actual = {"race_mode": 1, "race_human": trace_start["human_race"],
              "race_bots": len(bots), "race_skirmish_map": trace_start["map"]}
    for key, wanted in actual.items():
        if state[key] != wanted:
            raise RuntimeError(f"live ARM {key}={state[key]} differs from trace start {wanted}")
    expected_races = [bot["race"] for bot in bots]
    expected_owners = [bot["owner"] for bot in bots]
    if state["bot_races"][:len(bots)] != expected_races:
        raise RuntimeError("live ARM bot races differ from normal menu trace")
    if state["bot_owners"][:len(bots)] != expected_owners:
        raise RuntimeError("live ARM bot owner slots differ from normal menu trace")


def verify_terminal(terminal: dict, state: dict) -> None:
    if state["screen"] != SCREEN_RESULT:
        raise RuntimeError(f"ARM did not reach RESULT at terminal tick (screen={state['screen']})")
    expected_state = {
        "frame": terminal["frame"],
        "won": terminal["won"],
        "lost": terminal["lost"],
        "race_mode": 1,
        "race_human": terminal["human_race"],
        "race_bots": terminal["bot_count"],
        "race_skirmish_map": terminal["map"],
    }
    for key, expected in expected_state.items():
        if state[key] != expected:
            raise RuntimeError(f"ARM terminal {key}={state[key]} differs from trace {expected}")
    for label, owner in [("human", 0)] + [(f"bot{index}", bot["owner"])
                                           for index, bot in enumerate(terminal["bots"])]:
        expected = terminal[label] if label == "human" else terminal["bots"][int(label[3:])]
        units = state["units"]["by_owner"].get(str(owner), 0)
        buildings = state["buildings"]["by_owner"].get(str(owner), 0)
        if units != expected["units"] or buildings != expected["buildings"]:
            raise RuntimeError(f"ARM {label} live counts {units}/{buildings} differ from trace "
                               f"{expected['units']}/{expected['buildings']}")


def check_debugger_port_free(port: int) -> None:
    if not 1 <= port <= 65535:
        raise ValueError("invalid GDB port")
    probe = socket.socket()
    try:
        probe.bind(("127.0.0.1", port))
    except OSError as exc:
        raise RuntimeError(f"mGBA debugger port {port} is occupied; leave the active emulator untouched") from exc
    finally:
        probe.close()


def coherent_clock(rsp) -> int:
    """Match gba_clock's high-low-high read; never accept a torn rollover."""
    for _ in range(8):
        hi = int.from_bytes(rsp.read(0x0400010c, 2), "little")
        lo = int.from_bytes(rsp.read(0x04000108, 2), "little")
        if hi == int.from_bytes(rsp.read(0x0400010c, 2), "little"):
            return (hi << 16) | lo
    raise RuntimeError("hardware timer did not yield a coherent read")


def sampled_delta(previous: int, current: int) -> int:
    delta = (current - previous) & MAX_U32
    if delta == 0:
        raise RuntimeError("zero hardware timer delta; timing is invalid or ambiguous")
    return delta


def replay_tick(rsp, expected: int) -> None:
    if hasattr(rsp, "clock"):
        rsp.tick(expected=expected & ~1)
    else:
        rsp.tick()


def run_bounded_sample(rsp, sym: dict, sizes: dict, bank: dict, plan: dict, pages: list, write_page,
                       checkpoint_interval: int, sample_start: int, sample_updates: int,
                       run_dir: Path, trace_hash: str, rom_hash: str, elf_hash: str,
                       clock64: bool = False, full_match: bool = False,
                       runner_sha256: str | None = None) -> dict:
    """Observe every update in a nonterminal prefix; no host time is used as FPS."""
    addr = sym["game_update"] & ~1
    if rsp.cmd(f"Z1,{addr:x},2") != "OK":
        raise RuntimeError("failed to install per-update observer")
    replay_tick(rsp, sym["game_update"])  # before scheduler tick zero executes
    if races.read_u32(rsp, sym["demo_schedule_tick"]) != 0:
        raise RuntimeError("per-update observer missed scheduler tick zero")
    read_clock = rsp.clock if clock64 else lambda: coherent_clock(rsp)
    previous_clock = read_clock()
    previous_screen = races.read_u32(rsp, sym["screen"])
    previous_frame = races.read_u32(rsp, sym["frame"])
    rows = []
    verified_configuration = False
    trace_inputs = {row["host_tick"]: row["frame"] for row in plan.get("trace_inputs", [])}
    end_tick = sample_start + sample_updates
    for tick in range(1, end_tick + 1):
        final_observer = full_match and tick == end_tick
        checkpoint = tick % checkpoint_interval == 0 or final_observer
        replay_tick(rsp, sym[checkpoint_observer_symbol(sym)] if checkpoint else sym["game_update"])
        observed = races.read_u32(rsp, sym["demo_schedule_tick"])
        if observed != tick:
            raise RuntimeError(f"per-update observer stopped at {observed}, expected {tick}")
        if checkpoint:
            page = pages[(tick - 1) // checkpoint_interval]
            if (races.read_u32(rsp, sym["demo_schedule_index"]) != len(page["events"]) or
                    races.read_u32(rsp, sym["demo_schedule_count"]) != len(page["events"])):
                raise RuntimeError(f"scheduler page not consumed at tick {tick}")
            if not final_observer:
                write_page(pages[tick // checkpoint_interval]["events"])
                replay_tick(rsp, sym["game_update"])
                if races.read_u32(rsp, sym["demo_schedule_tick"]) != tick:
                    raise RuntimeError("scheduler advanced unexpectedly after checkpoint")
        now = read_clock()
        screen = races.read_u32(rsp, sym["screen"])
        frame = races.read_u32(rsp, sym["frame"])
        if screen == SCREEN_PLAY and not verified_configuration:
            verify_configuration(plan["start"], snapshot(rsp, sym, sizes, bank))
            verified_configuration = True
        delta = now - previous_clock if clock64 else sampled_delta(previous_clock, now)
        if delta <= 0:
            raise RuntimeError("invalid emulated cycle delta")
        frame_delta = (frame - previous_frame) & MAX_U32
        if tick in trace_inputs and frame != trace_inputs[tick]:
            raise RuntimeError(f"ARM frame differs from trace at tick {tick}")
        phase = ("menu" if tick <= plan["start"]["host_tick"] else
                 "outside_play" if previous_screen != SCREEN_PLAY or screen != SCREEN_PLAY
                 else "warmup" if tick <= sample_start else "sample")
        if phase == "sample" and frame_delta == 0:
            phase = "nonadvancing_play"
        elif phase == "sample" and frame_delta != 1:
            raise RuntimeError(f"sample tick {tick} advanced more than one PLAY frame")
        rows.append({"tick": tick, "phase": phase, "frame": frame,
                     "frame_delta": frame_delta, "elapsed_cycles": delta})
        previous_clock, previous_screen, previous_frame = now, screen, frame
    samples = [row for row in rows if row["phase"] == "sample"]
    if not samples:
        raise RuntimeError("requested sample window has no advancing PLAY frames")
    terminal_state = None
    if full_match:
        terminal_state = snapshot(rsp, sym, sizes, bank)
        verify_configuration(plan["start"], terminal_state)
        verify_terminal(plan["terminal"], terminal_state)
    result = {"status": plan["terminal"]["kind"] if full_match else "nonterminal-performance-sample", "trace_sha256": trace_hash,
              "rom_sha256": rom_hash, "elf_sha256": elf_hash,
              "headless_runner_sha256": runner_sha256,
              "sample_start": sample_start, "sample_updates": sample_updates,
              "observed_play_frame_count": len(samples), "valid_timing_frame_count": len(samples) if clock64 else 0,
              "menu_update_count": sum(row["phase"] == "menu" for row in rows),
              "menu_cycles": sum(row["elapsed_cycles"] for row in rows if row["phase"] == "menu"),
              "nonadvancing_play_updates": sum(row["phase"] == "nonadvancing_play" for row in rows),
              "nonadvancing_play_cycles": sum(row["elapsed_cycles"] for row in rows if row["phase"] == "nonadvancing_play"),
              "outside_play_updates": sum(row["phase"] == "outside_play" for row in rows),
              "sample_cycles": sum(row["elapsed_cycles"] for row in samples),
              "mean_live_play_effective_fps": round(16777216 * len(samples) / sum(row["elapsed_cycles"] for row in samples), 2) if clock64 else None,
              "timer_method": "mGBA 64-bit emulated cycle clock" if clock64 else "GBA timer modulo 2^32; FPS unavailable",
              "timing_boundaries": ("Consecutive game_update entries, including main-loop work, audio, rendering and presentation waits; " +
                                    ("final endpoint is the scheduler checkpoint." if full_match else "sample endpoint is a game_update entry.")),
              "terminal_state": terminal_state,
              "terminal_parity_scope": "Trace provides frame, result, configuration and active human/bot live unit/building counts; other per-entity fields are observations only." if full_match else None,
              "observations": rows}
    target = run_dir / ("full-replay.json" if full_match else "sample.json")
    target.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    result["report"] = str(target)
    return result


def run(trace_path: Path, rom_path: Path, elf_path: Path, output: Path,
        *, expect_rom_sha256: str | None = None, expect_elf_sha256: str | None = None,
        expect_trace_sha256: str | None = None, gdb_port: int | None = None,
        gdb_launcher: Path | None = None, sample_start: int | None = None,
        sample_updates: int = 0, headless_runner: Path | None = None,
        full_per_update: bool = False) -> dict:
    trace_bytes = trace_path.read_bytes()
    trace_hash = hashlib.sha256(trace_bytes).hexdigest()
    rom_hash = hashlib.sha256(rom_path.read_bytes()).hexdigest()
    elf_hash = hashlib.sha256(elf_path.read_bytes()).hexdigest()
    for label, expected, actual in (("ROM", expect_rom_sha256, rom_hash),
                                    ("ELF", expect_elf_sha256, elf_hash),
                                    ("trace", expect_trace_sha256, trace_hash)):
        if expected and expected.lower() != actual:
            raise RuntimeError(f"{label} SHA-256 mismatch: expected {expected}, observed {actual}")
    trace = json.loads(trace_bytes)
    sym, sizes = symbols(elf_path), symbol_sizes(elf_path)
    mode, capacity = scheduler.schedule_mode(sym, sizes)
    if mode != "windowed":
        raise RuntimeError("supplied ELF must contain the windowed controller scheduler")
    checkpoint_interval = scheduler.checkpoint_ticks(sym)
    plan = validate_trace(trace, capacity, windowed=True,
                          checkpoint_interval=checkpoint_interval)
    plan["trace_inputs"] = [row for row in trace["events"] if row["kind"] == "input"]
    if full_per_update:
        if headless_runner is None or sample_start is not None or sample_updates:
            raise ValueError("full per-update replay requires headless runner and no sample window")
        sample_start, sample_updates = 0, plan["stop_tick"]
    if sample_start is not None:
        if not full_per_update and (not plan["start"]["host_tick"] <= sample_start <= 10000 or not 1 <= sample_updates <= 10000):
            raise ValueError("sample start must follow launch and sample updates must be in [1,10000]")
        if not full_per_update and sample_start + sample_updates >= plan["stop_tick"]:
            raise ValueError("sample must end strictly before the trace terminal")
    elif sample_updates:
        raise ValueError("--sample-updates requires --sample-start")
    bank = bank_layout(elf_path, sizes)
    required = ("game_update", "demo_schedule_events", "demo_schedule_count", "demo_schedule_length",
                "demo_schedule_tick", "demo_schedule_active",
                "demo_schedule_index", "screen", "frame", "won", "lost", "race_mode",
                "race_human", "race_bots", "race_skirmish_map", "race_bot_races",
                "race_bot_owner_ids", "race_front", "u", "b")
    missing = [name for name in required if name not in sym]
    if missing:
        raise RuntimeError("skirmish ROM/ELF lacks replay symbols: " + ", ".join(missing))
    checkpoint_symbol = checkpoint_observer_symbol(sym)
    if sizes.get("demo_schedule_active") != 4 or sizes.get("demo_schedule_index") != 4:
        raise RuntimeError("windowed scheduler control symbols must be 32-bit")

    split.FRONT_FIELDS = split.frontend_offsets(elf_path)
    run_dir = output / f"skirmish-{time.time_ns()}"
    run_dir.mkdir(parents=True, exist_ok=False)
    progress_path = run_dir / "progress.json"
    if headless_runner is None:
        if gdb_port is None:
            raise ValueError("select --headless-runner or an explicit --gdb-port")
        check_debugger_port_free(gdb_port)
    proc = sock = rsp = log = None
    try:
        if headless_runner is not None:
            import sc64_headless_replay
            proc, rsp, log = sc64_headless_replay.start(headless_runner, rom_path, run_dir, sym, sizes)
        else:
            proc, sock, rsp, log = split.menu.start_emulator(
                run_dir.name, rom_path, run_dir, sym, gdb_port=gdb_port,
                gdb_launcher=gdb_launcher, isolated_home=run_dir / "home")
        initial = {"screen": races.read_u32(rsp, sym["screen"]),
                   "frame": races.read_u32(rsp, sym["frame"]),
                   "page": races.read_u32(rsp, sym["race_front"] + split.FRONT_FIELDS["page"])}
        if initial != {"screen": SCREEN_TITLE, "frame": 0, "page": split.FRONT_HOME}:
            raise RuntimeError(f"fresh boot is not on normal HOME: {initial}")
        # The headless backend reads RAM only; a screenshot would require
        # MMIO access to DISPCNT and is unnecessary for timing or parity.
        home_capture = capture(rsp, run_dir / "home.png") if headless_runner is None else None

        pages = plan["pages"]
        def write_page(events: list[tuple[int, int]]) -> None:
            if len(events) > capacity:
                raise RuntimeError(f"scheduled input window exceeds ELF capacity ({len(events)}/{capacity})")
            blob = b"".join(struct.pack("<2I", tick, held) for tick, held in events)
            for offset in range(0, len(blob), 256):
                rsp.write(sym["demo_schedule_events"] + offset, blob[offset:offset + 256])
            rsp.write(sym["demo_schedule_count"], struct.pack("<I", len(events)))

        write_page(pages[0]["events"])
        rsp.write(sym["demo_schedule_index"], struct.pack("<I", 0))
        rsp.write(sym["demo_schedule_length"], struct.pack("<I", plan["stop_tick"]))
        rsp.write(sym["demo_schedule_active"], struct.pack("<I", 1))
        if rsp.cmd(f"z1,{sym['game_update'] & ~1:x},2") != "OK" or rsp.cmd(
                f"Z1,{sym[checkpoint_symbol] & ~1:x},2") != "OK":
            raise RuntimeError("failed to install the input-only scheduler observer")
        replay_tick(rsp, sym[checkpoint_symbol])
        if races.read_u32(rsp, sym["demo_schedule_tick"]) != 0:
            raise RuntimeError("windowed scheduler advanced before HOME input tick zero")
        if races.read_u32(rsp, sym["screen"]) != SCREEN_TITLE:
            raise RuntimeError("neutral pre-input scheduler update left HOME")

        if sample_start is not None:
            return run_bounded_sample(rsp, sym, sizes, bank, plan, pages, write_page, checkpoint_interval,
                                      sample_start, sample_updates, run_dir, trace_hash,
                                      rom_hash, elf_hash, clock64=headless_runner is not None,
                                      full_match=full_per_update,
                                      runner_sha256=hashlib.sha256(headless_runner.read_bytes()).hexdigest() if headless_runner else None)

        observations = []
        previous = snapshot(rsp, sym, sizes, bank)
        read_clock = rsp.clock if headless_runner is not None else lambda: coherent_clock(rsp)
        previous_clock = read_clock()
        counted_frames = counted_cycles = 0
        # Tick zero is HOME and is deliberately excluded from the live FPS mean.
        observations.append({"tick": 0, "screen": previous["screen"], "frame": previous["frame"],
                             "frame_delta": 0, "counted_live_play_interval": False,
                             "effective_fps": None})
        def persist_progress(tick: int, state: dict, status: str = "running") -> None:
            progress = {"status": status, "tick": tick, "requested_ticks": plan["stop_tick"],
                        "frame": state["frame"], "screen": state["screen"],
                        "checkpoint_interval": checkpoint_interval,
                        "input_transitions": plan["input_count"],
                        "mean_live_play_effective_fps": round(16777216 * counted_frames / counted_cycles, 2) if headless_runner is not None and counted_cycles else None}
            temporary = progress_path.with_suffix(".json.tmp")
            temporary.write_text(json.dumps(progress, indent=2) + "\n", encoding="utf-8")
            temporary.replace(progress_path)

        persist_progress(0, previous)
        completed = 0
        while completed < plan["stop_tick"]:
            replay_tick(rsp, sym[checkpoint_symbol])
            expected_tick = min(plan["stop_tick"], completed + checkpoint_interval)
            observed_tick = races.read_u32(rsp, sym["demo_schedule_tick"])
            if observed_tick != expected_tick:
                raise RuntimeError(f"scheduler stopped at {observed_tick}, expected {expected_tick}")
            page = pages[(expected_tick - 1) // checkpoint_interval]
            if (races.read_u32(rsp, sym["demo_schedule_index"]) != len(page["events"]) or
                    races.read_u32(rsp, sym["demo_schedule_count"]) != len(page["events"])):
                raise RuntimeError(f"windowed scheduler did not consume every input by tick {expected_tick}")
            if expected_tick < plan["stop_tick"]:
                write_page(pages[expected_tick // checkpoint_interval]["events"])
            current = snapshot(rsp, sym, sizes, bank)
            if expected_tick >= checkpoint_interval:
                verify_configuration(plan["start"], current)
            now = read_clock()
            elapsed = now - previous_clock if headless_runner is not None else sampled_delta(previous_clock, now)
            if elapsed <= 0:
                raise RuntimeError("invalid emulated cycle delta")
            frame_delta = (current["frame"] - previous["frame"]) & MAX_U32
            live = previous["screen"] == SCREEN_PLAY and current["screen"] == SCREEN_PLAY
            counted = live and frame_delta > 0
            interval_fps = round(16777216 * frame_delta / elapsed, 2) if counted and headless_runner is not None else None
            if counted:
                counted_frames += frame_delta
                counted_cycles += elapsed
            observations.append({"tick": expected_tick, "screen": current["screen"],
                                 "frame": current["frame"], "frame_delta": frame_delta,
                                 "elapsed_hardware_cycles": elapsed,
                                 "counted_live_play_interval": counted,
                                 "effective_fps": interval_fps})
            previous, previous_clock = current, now
            completed = expected_tick
            persist_progress(completed, current)
            if completed % 10000 == 0 or completed == plan["stop_tick"]:
                print(f"skirmish tick={completed}/{plan['stop_tick']} frame={current['frame']} "
                      f"screen={current['screen']}", flush=True)
        final = observations[-1]
        terminal_state = snapshot(rsp, sym, sizes, bank)
        verify_configuration(plan["start"], terminal_state)
        verify_terminal(plan["terminal"], terminal_state)
        if terminal_state["frame"] != final["frame"]:
            raise RuntimeError("terminal snapshot changed after the exact scheduler endpoint")
        result = {"status": plan["terminal"]["kind"], "trace": str(trace_path),
                  "rom": str(rom_path), "elf": str(elf_path), "trace_sha256": trace_hash,
                  "rom_sha256": rom_hash, "elf_sha256": elf_hash,
                  "map": plan["start"]["map"], "human_race": plan["start"]["human_race"],
                  "bots": plan["start"]["bots"], "input_transitions": plan["input_count"],
                  "scheduler": "windowed", "scheduler_capacity": capacity,
                  "checkpoint_interval": checkpoint_interval,
                  "peak_window_transitions": plan["peak_page_transitions"],
                  "requested_ticks": plan["stop_tick"], "final_frame": terminal_state["frame"],
                  "terminal_state": {"won": terminal_state["won"], "lost": terminal_state["lost"],
                                     "human_units": terminal_state["units"]["by_owner"].get("0", 0),
                                     "human_buildings": terminal_state["buildings"]["by_owner"].get("0", 0),
                                     "bot_counts": [{"owner": bot["owner"],
                                                     "units": terminal_state["units"]["by_owner"].get(str(bot["owner"]), 0),
                                                     "buildings": terminal_state["buildings"]["by_owner"].get(str(bot["owner"]), 0)}
                                                    for bot in plan["terminal"]["bots"]]},
                  "counted_live_play_frames": counted_frames,
                  "counted_live_play_hardware_cycles": counted_cycles,
                  "mean_live_play_effective_fps": round(16777216 * counted_frames / counted_cycles, 2) if headless_runner is not None and counted_cycles else None,
                  "timer_method": "mGBA 64-bit emulated cycle clock" if headless_runner is not None else "GBA timer modulo 2^32; FPS unavailable",
                  "fps_exclusions": "HOME/start interval and final RESULT interval excluded; only intervals "
                                    "whose endpoints are both PLAY contribute.",
                  "home_capture": home_capture, "checkpoints": observations}
        report_path = run_dir / "replay.json"
        report_path.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
        persist_progress(completed, terminal_state, "complete")
        result["report"] = str(report_path)
        print(json.dumps({"status": result["status"], "ticks": result["requested_ticks"],
                          "mean_live_play_effective_fps": result["mean_live_play_effective_fps"],
                          "report": str(report_path)}, indent=2))
        return result
    finally:
        if proc is not None and headless_runner is not None:
            proc.terminate()
            try:
                proc.wait(timeout=3)
            except __import__("subprocess").TimeoutExpired:
                proc.kill()
                proc.wait()
            if proc.stdin: proc.stdin.close()
            if proc.stdout: proc.stdout.close()
            if log: log.close()
        elif proc is not None:
            try:
                split.menu.stop_emulator(proc, sock, log)
            except Exception:
                proc.terminate()
                proc.wait(timeout=3)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--trace", type=Path, required=True)
    parser.add_argument("--rom", type=Path, required=True)
    parser.add_argument("--elf", type=Path, required=True)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--expect-rom-sha256")
    parser.add_argument("--expect-elf-sha256")
    parser.add_argument("--expect-trace-sha256")
    parser.add_argument("--gdb-port", type=int)
    parser.add_argument("--gdb-launcher", type=Path,
                        help=argparse.SUPPRESS)
    parser.add_argument("--headless-runner", type=Path, help="port-free mGBA backend executable")
    parser.add_argument("--sample-start", type=int, help="first scheduler tick to measure after warmup")
    parser.add_argument("--sample-updates", type=int, default=0)
    parser.add_argument("--full-per-update", action="store_true")
    args = parser.parse_args(argv)
    try:
        run(args.trace, args.rom, args.elf, args.output,
            expect_rom_sha256=args.expect_rom_sha256,
            expect_elf_sha256=args.expect_elf_sha256,
            expect_trace_sha256=args.expect_trace_sha256, gdb_port=args.gdb_port,
            gdb_launcher=args.gdb_launcher, sample_start=args.sample_start,
            sample_updates=args.sample_updates, headless_runner=args.headless_runner,
            full_per_update=args.full_per_update)
    except (OSError, ValueError, RuntimeError, json.JSONDecodeError) as exc:
        print(f"skirmish controller replay failed: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
