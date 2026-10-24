#!/usr/bin/env python3
"""Replay private Z4/P4 controller-win traces using only scheduled inputs.

ROM/ELF catalog metadata and debugger symbols are checked before launch. The
replay writes the controller queue and scheduler metadata; game/model state is
read through symbols and DWARF at 1000-tick checkpoints.
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
MISSION_BY_PROFILE = {"zerg-z4": 15, "protoss-p4": 23}
FACTION_BY_PROFILE = {"zerg-z4": "zerg", "protoss-p4": "protoss"}
MODEL_TYPES = {"zerg-z4": "Sc64Z4State", "protoss-p4": "Sc64P4State"}
RUNTIME_TYPES = {"zerg-z4": "Sc64Z4RuntimeState", "protoss-p4": "Sc64P4RuntimeState"}
DEFAULT_TRACE = ROOT / ".local/sc64/z4-controller-win.json"
DEFAULT_TRACES = {"zerg-z4": DEFAULT_TRACE,
                 "protoss-p4": ROOT / ".local/sc64/p4-controller-win.json"}
DEFAULT_OUTPUT = ROOT / "build/emulator/zp-controller-replay"
CHECKPOINT_TICKS = 1000
MENU_PREFIX = ((1, 1), (2, 0), (3, 16), (4, 0), (5, 128),
               (6, 0), (7, 1), (8, 0), (9, 1))


def validate_trace(trace: object, capacity: int, profile_name: str,
                   *, windowed: bool = True) -> dict:
    """Strictly validate an authenticated Z4/P4 trace before emulator launch."""
    if profile_name not in MISSION_BY_PROFILE:
        raise ValueError("profile must be zerg-z4 or protoss-p4")
    if type(capacity) is not int or capacity < 1:
        raise ValueError("controller queue capacity must be positive")
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
        elif kind in ("periodic", "prehatch", "posthatch"):
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
    if (start_transition_count != len(MENU_PREFIX) or
            transitions[:start_transition_count] != list(MENU_PREFIX)):
        raise ValueError("normal HOME/menu inputs do not match the 4th-entry deployment route")
    if not windowed and len(inputs) > capacity:
        raise ValueError(f"controller queue exceeds ELF capacity ({len(inputs)}/{capacity})")
    if start is None or start.get("screen") != SCREEN_PLAY or start.get("frame") != 0:
        raise ValueError("trace start must be a live PLAY screen at frame zero")
    if start.get("host_tick") != 9 or start.get("host_tick") != start_input_tick:
        raise ValueError("normal 4th-entry HOME/menu deployment must launch at host_tick=9")
    if start.get("won") != 0 or start.get("lost") != 0 or start.get("outcome") != 0:
        raise ValueError("trace start must be live and unfinished")
    if final is None or rows[-1] is not final or final.get("kind") != "won":
        raise ValueError("full controller trace must end in a won event")
    stop = final.get("host_tick")
    if type(stop) is not int or not 1 <= stop <= 0xFFFFFFFF or stop <= previous_tick:
        raise ValueError("final won event must follow every controller transition")
    if checkpoints and checkpoints[-1]["host_tick"] > stop:
        raise ValueError("checkpoint occurs after final result")
    if final.get("screen") != SCREEN_RESULT or final.get("won") != 1 or final.get("lost") != 0:
        raise ValueError("final event must record a won RESULT screen")
    if final.get("outcome") != 1:
        raise ValueError("final event must authenticate a completed campaign-model win")
    for field in ("frame", "fired"):
        if type(final.get(field)) is not int:
            raise ValueError(f"final event is missing integer field {field}")
    if final["frame"] <= start["frame"]:
        raise ValueError("final frame must follow mission start")
    if windowed:
        last_page = (stop - 1) // CHECKPOINT_TICKS
        peak = max((sum(1 for tick, _ in inputs if tick // CHECKPOINT_TICKS == page)
                    for page in range(last_page + 1)), default=0)
        if peak > capacity:
            raise ValueError(f"controller event page exceeds ELF capacity ({peak}/{capacity})")
    else:
        peak = len(inputs)
    return {"inputs": inputs, "input_count": len(inputs), "stop_tick": stop,
            "start": start, "final": final, "checkpoints": checkpoints,
            "peak_page_transitions": peak, "scheduler_mode": "windowed" if windowed else "full"}


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
    if profile_name == "zerg-z4":
        if runtime.get("faults") != 0 or runtime.get("hatch_effects") != 3:
            raise RuntimeError("Z4 final state has faults or incomplete hatch effects")
        for key in ("chrysalis_alive", "kerrigan_alive", "force3_command_centers"):
            if key in trace_final and state.get(key) != trace_final[key]:
                raise RuntimeError(f"final {key} differs from authenticated trace")
    else:
        if runtime.get("faults") != 0:
            raise RuntimeError("P4 final state reports adapter faults")
        if (model.get("sequence_active") != 0 or model.get("sequence_trigger") != 0 or
                model.get("action_index") != 0 or model.get("reserved") != 0):
            raise RuntimeError("P4 final result sequence/cursor is not complete")
        # Keep P4 verification model-driven; don't encode Z4-specific outcomes.
        for key in ("force2_units", "force3_units", "side_units", "side_buildings"):
            if key in trace_final and state.get(key) != trace_final[key]:
                raise RuntimeError(f"final {key} differs from authenticated trace")


def mission_snapshot(rsp, sym: dict[str, int], sizes: dict[str, int], bank: dict,
                     campaign_layout: tuple[int, dict[str, int]],
                     runtime_layout: tuple[int, dict[str, int]],
                     model_layout: tuple[int, dict[str, int]], profile_name: str) -> dict:
    units = read_compact_units(rsp, sym["u"], bank["unit_capacity"],
                               bank["unit_stride_bytes"], bank["unit_offsets"])
    buildings = races.read_entities(rsp, sym["b"], bank["building_capacity"],
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
    runtime_symbol = "race_z4" if profile_name == "zerg-z4" else "race_p4"
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
                                   if item["owner"] == 0 and item.get("raw_type") == 51)
    state["force3_command_centers"] = sum(1 for item in buildings["entities"]
                                           if item["owner"] == 4 and item.get("raw_type") == 106)
    state["chrysalis_alive"] = count_chrysalis(units, buildings)
    state["race_minerals"] = list(struct.unpack("<5i", rsp.read(sym["race_minerals"], 20)))
    state["race_gas"] = list(struct.unpack("<5i", rsp.read(sym["race_gas"], 20)))
    return state


def read_compact_units(rsp, address: int, capacity: int, stride: int,
                       offsets: dict[str, int]) -> dict:
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
                         "selected", "order", "armor"):
            if optional in offsets:
                item[optional] = field(optional)
        entities.append(item)
    return {"alive": len(entities), "by_owner": by_owner,
            "by_owner_type": by_owner_type, "entities": entities}


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
    building_stride, building_offsets = races.dwarf_struct(elf, "Building")
    capacity = 192 if profile_name == "zerg-z4" else 240
    if (unit_stride != 56 or building_stride != 100 or
            sizes.get("u") != capacity * unit_stride or
            sizes.get("b") != 96 * building_stride):
        raise RuntimeError("staged compact entity banks differ from the bounded profile")
    for field in ("owner", "type", "alive", "x", "y", "hp", "raw_type"):
        if field not in unit_offsets:
            raise RuntimeError(f"compact Unit DWARF lacks {field}")
    for field in ("owner", "type", "alive", "x", "y", "hp", "raw_type", "under_construction"):
        offset = building_offsets.get(field)
        if offset is None or offset % 4 or not 0 <= offset <= building_stride - 4:
            raise RuntimeError(f"Building DWARF lacks a bounded int field {field}")
    return {"unit_bank_bytes": sizes["u"], "building_bank_bytes": sizes["b"],
            "unit_capacity": capacity, "building_capacity": 96,
            "unit_stride_bytes": unit_stride, "building_stride_bytes": building_stride,
            "unit_offsets": unit_offsets, "building_offsets": building_offsets}


def run(trace_path: Path, output: Path, rom_path: Path, elf_path: Path,
        profile_name: str, ticks: int | None = None, save_at_end: bool = False,
        expected_rom_sha256: str | None = None, expected_elf_sha256: str | None = None,
        expected_trace_sha256: str | None = None) -> dict:
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
    plan = validate_trace(trace, capacity, profile_name, windowed=mode == "windowed")
    requested, prefix = scheduler.run_length(plan, ticks, save_at_end)
    if save_at_end and mode != "windowed":
        raise ValueError("--save-at-end requires a windowed scheduler")
    metadata = split.read_campaign_metadata(rom_path, elf_path, FACTION_BY_PROFILE[profile_name])
    mission = MISSION_BY_PROFILE[profile_name]
    if mission not in metadata["enabled_mission_ids"]:
        raise RuntimeError(f"matching ROM/ELF catalog does not enable global mission {mission}")
    bank = compact_bank_profile(elf_path, sizes, profile_name)
    campaign_layout = races.dwarf_struct(elf_path, "Sc64RaceCampaignState")
    runtime_layout = races.dwarf_struct(elf_path, RUNTIME_TYPES[profile_name])
    model_layout = races.dwarf_struct(elf_path, MODEL_TYPES[profile_name])
    required = ("game_update", "demo_schedule_events", "demo_schedule_count", "demo_schedule_length",
                "demo_schedule_tick", "demo_replay_checkpoint", "screen", "frame", "won", "lost",
                "race_mode", "race_selected_mission", "race_completed", "race_campaign",
                "race_minerals", "race_gas", "race_front", "u", "b")
    required += ("race_z4" if profile_name == "zerg-z4" else "race_p4",)
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

        pages = scheduler.make_schedule_pages(plan, requested, capacity) if mode == "windowed" else None
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
            blob = b"".join(struct.pack("<2I", tick, held) for tick, held in plan["inputs"])
            for offset in range(0, len(blob), 256):
                rsp.write(sym["demo_schedule_events"] + offset, blob[offset:offset + 256])
            rsp.write(sym["demo_schedule_count"], struct.pack("<I", len(plan["inputs"])))
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
                requested_ticks=requested, checkpoint_interval=CHECKPOINT_TICKS,
                latest_checkpoint=checkpoint, status=status)

        tick_zero = {"tick": 0, "host_tick": 0, "elapsed_hardware_cycles": 0,
                     "frame_delta": 0, "counted_live_play_interval": False,
                     "effective_fps": None, "capture": str(run_dir / "home.png"),
                     **prior_state}
        persist_checkpoint(tick_zero)
        last_tick = 0
        while last_tick < requested:
            rsp.tick()
            completed = profile.u32(rsp, sym["demo_schedule_tick"])
            expected = min(requested, last_tick + CHECKPOINT_TICKS)
            if completed != expected:
                raise RuntimeError(f"expected checkpoint tick {expected}, observed {completed}")
            if mode == "windowed":
                page = pages[(completed - 1) // CHECKPOINT_TICKS]
                if (profile.u32(rsp, sym["demo_schedule_index"]) != len(page["events"]) or
                        profile.u32(rsp, sym["demo_schedule_count"]) != len(page["events"])):
                    raise RuntimeError(f"scheduler page incomplete at tick {completed}")
                if completed < requested:
                    write_page(pages[completed // CHECKPOINT_TICKS]["events"])
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
                  "peak_page_transitions": plan["peak_page_transitions"],
                  "trace_stop_tick": plan["stop_tick"], "requested_ticks": requested,
                  "observed_ticks": final["tick"], "prefix": prefix,
                  "counted_live_play_frames": counted_frames,
                  "counted_live_play_hardware_cycles": counted_cycles,
                  "mean_live_play_effective_fps": round(profile.CPU_HZ * counted_frames /
                                                          max(1, counted_cycles), 2),
                  "timer_method": "GBA chained hardware timer deltas at 1000-tick checkpoints",
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
            requested_ticks=requested, checkpoint_interval=CHECKPOINT_TICKS,
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
    parser.add_argument("--rom", type=Path, required=True)
    parser.add_argument("--elf", type=Path, required=True)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--ticks", type=int, help="replay a positive trace prefix")
    parser.add_argument("--save-at-end", action="store_true", help="pause/save at a live prefix endpoint")
    parser.add_argument("--expect-rom-sha256")
    parser.add_argument("--expect-elf-sha256")
    parser.add_argument("--expect-trace-sha256")
    args = parser.parse_args(argv)
    try:
        trace_path = args.trace or DEFAULT_TRACES[args.profile]
        run(trace_path, args.output, args.rom, args.elf, args.profile, args.ticks,
            args.save_at_end, args.expect_rom_sha256, args.expect_elf_sha256,
            args.expect_trace_sha256)
    except (OSError, ValueError, RuntimeError, json.JSONDecodeError) as exc:
        print(f"Z/P controller replay failed: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
