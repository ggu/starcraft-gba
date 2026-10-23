#!/usr/bin/env python3
"""Verify Terran campaign SRAM persistence through normal controller input.

The internal T5 probe auto-enters mission 8 for engine testing; --menu-entry
launches the combined ROM through its ordinary campaign controls instead.
The tool pauses, saves to mGBA's SRAM file, loads that file in a fresh process,
compares the saved gameplay state, then checks that play advances without
trigger faults. It uses controller input for play and read-only RSP inspection
for game state.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import shutil
import socket
import struct
import sys
import time

sys.path.insert(0, str(Path(__file__).resolve().parent))
import sc64_menu_smoke as menu
from emulator_smoke import ROOT, symbol_sizes, symbols
from sc64_races_profile import dwarf_struct

PORT = 2345
SRAM_BASE = 0x0E000000
SRAM_BYTES = 32768
SLOT_BYTES = 16384
DEFAULT_GENERATION = 14  # Cargo orders share the existing payload with a new identity.
STORAGE_SCHEMAS = {13: (64, 16328), 14: (64, 16328), 15: (80, 15384),
                   16: (80, 15644), 17: (96, 15312), 18: (96, None)}
STORAGE_UNIT_COUNTS = {13: 160, 14: 160, 15: 160, 16: 160, 17: 192, 18: 192}
STORAGE_MINERAL_COUNTS = {13: 48, 14: 48, 15: 48, 16: 48, 17: 80, 18: 80}
STORAGE_T6_STATE_BYTES = {16: 1384, 17: 1388, 18: 1388}
KEY_B, KEY_L, KEY_START = 2, 512, 8
SCREEN_PLAY, SCREEN_PAUSE = 1, 2
SCREEN_TITLE = 0
FRONT_HOME = 0
MENU_ENTRY_BUTTONS = (1, 16, 16, 1, 1)  # A, RIGHT, RIGHT, briefing A, deploy A

DEFAULT_ROM = ROOT / "build" / "StarCraft-T5-Engine-Probe.gba"
DEFAULT_ELF = ROOT / "build" / "starcraft-t5-engine-probe.elf"
DEFAULT_OUT = ROOT / "build" / "emulator" / "sc64-t5-save-smoke"


def read_i32(rsp, address: int) -> int:
    return struct.unpack("<i", rsp.read(address, 4))[0]


def read_u32(rsp, address: int) -> int:
    return int.from_bytes(rsp.read(address, 4), "little")


def read_u16(rsp, address: int) -> int:
    return int.from_bytes(rsp.read(address, 2), "little")


def check_port_free() -> None:
    probe = socket.socket()
    try:
        probe.bind(("127.0.0.1", PORT))
    except OSError as exc:
        raise RuntimeError(f"Port {PORT} is in use; leave the existing debugger untouched") from exc
    finally:
        probe.close()


def start(name: str, rom: Path, out: Path, sym: dict[str, int]):
    check_port_free()
    return menu.start_emulator(name, rom, out, sym)


def press(rsp, inject: int, key: int, ticks: int = 2) -> None:
    rsp.write(inject, struct.pack("<H", key))
    rsp.tick(ticks)
    rsp.write(inject, b"\0\0")
    rsp.tick(2)


def campaign_menu_buttons(mission: int) -> tuple[int, ...]:
    """Return HOME→campaign→mission→briefing→deploy keys for ordinal 5–10."""
    if mission < 5 or mission > 10:
        raise ValueError("Terran campaign menu supports mission ordinals 5 through 10")
    chapter = mission
    page = (chapter - 1) // 2
    row = (chapter - 1) % 2
    return (1, *(16 for _ in range(page)), *((128,) if row else ()), 1, 1)


def enter_t5_campaign_menu(rsp, inject: int, mission: int = 5) -> None:
    """Enter a Terran mission through the ordinary chapter/page menu."""
    buttons = campaign_menu_buttons(mission)
    for key in buttons:
        press(rsp, inject, key)


def late_save_state(payload: bytes, save_fields: dict[str, int], late_size: int) -> bytes:
    """Extract the exact generation-18 late-mission union member from RaceSave."""
    offset = save_fields.get("terran_late")
    if offset is None or offset < 0 or late_size <= 0 or offset + late_size > len(payload):
        raise ValueError("DWARF late Terran save member is outside RaceSave")
    return payload[offset:offset + late_size]


def active_save_member(mission: int) -> str:
    """Return the RaceSave union member used by this campaign ordinal."""
    if mission == 5:
        return "t5"
    if mission == 6:
        return "t6"
    if 7 <= mission <= 10:
        return "terran_late"
    raise ValueError("save smoke supports mission ordinals 5 through 10")


def race_save_union_offset(save_fields: dict[str, int]) -> int:
    """Locate the anonymous campaign-state union after the final named field."""
    end = save_fields.get("t3_buildings_killed")
    if end is None:
        raise ValueError("RaceSave DWARF is missing the preceding T3 field")
    # t3_buildings_killed is a u16; RaceSave aligns the following state union
    # to four bytes. The exact offset is checked against payload bounds below.
    return (end + 2 + 3) & ~3


def save_union_member(payload: bytes, save_fields: dict[str, int],
                      mission: int, member_sizes: dict[str, int]) -> tuple[bytes, bytes]:
    """Return active bytes and padding; require the unused union tail to be zero."""
    name = active_save_member(mission)
    size = member_sizes.get(name, 0)
    offset = race_save_union_offset(save_fields)
    union_size = max(member_sizes.values(), default=0)
    end = offset + union_size
    if size <= 0 or size > union_size or end > len(payload):
        raise ValueError("RaceSave union member is outside the serialized payload")
    active = payload[offset:offset + size]
    padding = payload[offset + size:end]
    if any(padding):
        raise ValueError(f"inactive RaceSave union tail for {name} is not canonical zero")
    return active, padding


def late_state_summary(raw: bytes, late_fields: dict[str, int],
                       runtime_size: int, trigger_fields: dict[str, int],
                       trigger_size: int, player_fields: dict[str, int],
                       player_size: int) -> dict:
    """Decode named generation-18 clocks, links, and flight goals from DWARF."""
    if len(raw) != runtime_size:
        raise ValueError("late mission state size differs from its DWARF layout")
    trigger_off = late_fields["triggers"]
    if (player_size <= 0 or trigger_size <= 0 or trigger_off < 0 or
            trigger_off + trigger_size > runtime_size or
            trigger_fields["player"] + 8 * player_size > trigger_size or
            player_fields["elapsed_ticks"] + 4 > player_size or
            player_fields["fault"] + 1 > player_size or
            late_fields["flight_goals"] + 96 * 4 > runtime_size or
            late_fields["emitter_unit"] + 2 > runtime_size or
            late_fields["emitter_carrier"] + 2 > runtime_size):
        raise ValueError("late mission field offsets exceed their DWARF bounds")
    player_off = trigger_fields["player"]
    clocks, faults = [], []
    for owner in range(8):
        base = trigger_off + player_off + owner * player_size
        clocks.append(int.from_bytes(raw[base + player_fields["elapsed_ticks"]:
                                          base + player_fields["elapsed_ticks"] + 4], "little"))
        faults.append(raw[base + player_fields["fault"]])
    goals_off = late_fields["flight_goals"]
    goals = [int.from_bytes(raw[goals_off + i * 4:goals_off + i * 4 + 4], "little")
             for i in range(96)]
    return {
        "trigger_clocks": clocks,
        "trigger_faults": faults,
        "emitter_unit": int.from_bytes(raw[late_fields["emitter_unit"]:
                                            late_fields["emitter_unit"] + 2], "little"),
        "emitter_carrier": int.from_bytes(raw[late_fields["emitter_carrier"]:
                                               late_fields["emitter_carrier"] + 2], "little"),
        "flight_goals": goals,
        "active_flight_goals": sum(bool(goal) for goal in goals),
    }


def field_int(raw: bytes, base: int, fields: dict[str, int], name: str,
              size: int = 4, signed: bool = True) -> int:
    offset = base + fields[name]
    return int.from_bytes(raw[offset:offset + size], "little", signed=signed)


def persisted_rows(rsp, address: int, count: int, stride: int,
                   fields: dict[str, int]) -> list[list[int] | None]:
    """Read each live serialized row as int words, preserving all saved fields."""
    if stride % 4:
        raise RuntimeError(f"serialized entity stride {stride} is not int-word aligned")
    raw = rsp.read(address, count * stride)
    flag_offset = fields.get("flags")
    rows: list[list[int] | None] = []
    words = stride // 4
    for slot in range(count):
        base = slot * stride
        if field_int(raw, base, fields, "alive") != 1:
            rows.append(None)
            continue
        row = [int.from_bytes(raw[base + i * 4:base + i * 4 + 4], "little", signed=True)
               for i in range(words)]
        if flag_offset is not None:
            row[flag_offset // 4] = int.from_bytes(
                raw[base + flag_offset:base + flag_offset + 4], "little") & 0xFFFF
        rows.append(row)
    return rows


UNIT_COMPACT_FIELDS = (
    ("x", 11, 0), ("y", 11, 0), ("hp", 18, 131072),
    ("maxhp", 17, 0), ("owner", 3, 0), ("type", 6, 0),
    ("alive", 1, 0), ("selected", 1, 0), ("cd", 16, 0),
    ("order", 4, 0), ("tx", 11, 0), ("ty", 11, 0),
    ("carry", 17, 65536), ("build", 10, 512), ("work_timer", 16, 0),
    ("armor", 8, 0), ("weapon", 16, 0), ("target", 10, 512),
    ("path_timer", 16, 0), ("path_retry", 16, 0),
    ("path_x", 11, 0), ("path_y", 11, 0), ("shield", 17, 0),
    ("raw_type", 9, 1), ("raw_owner", 9, 1), ("flags", 16, 0),
    ("shield_remainder", 11, 0),
)


UNIT_RUNTIME_FIELDS = {
    "x": (2, True), "y": (2, True), "hp": (4, True), "maxhp": (4, True),
    "owner": (1, False), "type": (1, False), "alive": (1, False),
    "selected": (1, False), "cd": (2, False), "order": (1, False),
    "tx": (2, True), "ty": (2, True), "carry": (4, True), "build": (2, True),
    "work_timer": (2, False), "armor": (1, False), "weapon": (2, False),
    "target": (2, True), "path_timer": (2, False), "path_retry": (2, False),
    "path_x": (2, True), "path_y": (2, True), "shield": (4, True),
    "raw_type": (2, True), "raw_owner": (2, True), "flags": (2, False),
    "shield_remainder": (2, False),
}


def decode_compact_unit_row(row: bytes) -> dict[str, int]:
    """Decode one generation-17 Unit row, including values for dead slots."""
    if len(row) != 38 or row[37] & 0x80:
        raise ValueError("compact Unit row has invalid size or reserved bit")
    if not any(row):
        return {name: 0 for name, _, _ in UNIT_COMPACT_FIELDS}
    bit = 0
    decoded: dict[str, int] = {}
    for name, width, bias in UNIT_COMPACT_FIELDS:
        value = 0
        for index in range(width):
            value |= ((row[bit >> 3] >> (bit & 7)) & 1) << index
            bit += 1
        decoded[name] = value - bias
    if bit != 303:
        raise AssertionError("compact Unit field table must consume 303 bits")
    return decoded


def decode_compact_unit_rows(payload: bytes, unit_count: int) -> list[dict[str, int]]:
    if unit_count < 1 or len(payload) < 4 + unit_count * 38:
        raise ValueError("RaceSave payload is too short for compact Unit rows")
    start = 4  # RaceSave.version precedes the Unit bank.
    return [decode_compact_unit_row(payload[start + i * 38:start + (i + 1) * 38])
            for i in range(unit_count)]


def compact_live_unit_rows(rsp, address: int, fields: dict[str, int], count: int,
                           stride: int) -> list[dict[str, int]]:
    if stride != 56 or not set(UNIT_RUNTIME_FIELDS).issubset(fields):
        raise RuntimeError("late-profile Unit DWARF layout is not the validated 56-byte schema")
    raw = rsp.read(address, count * stride)
    rows: list[dict[str, int]] = []
    for slot in range(count):
        base = slot * stride
        rows.append({name: int.from_bytes(raw[base + fields[name]:
                                               base + fields[name] + size],
                                          "little", signed=signed)
                     for name, (size, signed) in UNIT_RUNTIME_FIELDS.items()})
    return rows


def newest_record_payload(raw_sram: bytes, generation: int, payload_size: int) -> bytes:
    records = save_record_info(raw_sram, payload_size, generation)
    record = max(records, key=lambda item: item["sequence"])
    slot = record["slot"]
    return raw_sram[slot * SLOT_BYTES + 16:slot * SLOT_BYTES + 16 + payload_size]


def save_record_info(raw_sram: bytes, expected_payload: int, generation: int = DEFAULT_GENERATION) -> list[dict]:
    if len(raw_sram) != SRAM_BYTES:
        raise RuntimeError(f"expected {SRAM_BYTES}-byte SRAM, got {len(raw_sram)}")
    records = []
    for slot in range(2):
        offset = slot * SLOT_BYTES
        tag, size, checksum, sequence = struct.unpack_from("<4I", raw_sram, offset)
        if tag != 0x53435430 + generation or size != expected_payload or sequence == 0:
            continue
        payload = raw_sram[offset + 16:offset + 16 + size]
        fnv = 2166136261
        for byte in payload:
            fnv = ((fnv ^ byte) * 16777619) & 0xFFFFFFFF
        if fnv == checksum:
            records.append({"slot": slot, "tag": hex(tag), "payload_bytes": size,
                            "sequence": sequence, "payload_sha256": hashlib.sha256(payload).hexdigest()})
    if not records:
        raise RuntimeError(f"mGBA save file has no valid generation-{generation} Terran SRAM record")
    return records


def snapshot(rsp, sym: dict[str, int], sizes: dict[str, int],
             layouts: dict[str, tuple[int, dict[str, int]]], generation: int) -> dict:
    mission = read_i32(rsp, sym["race_selected_mission"]) - 3
    unit_size, unit_fields = layouts["Unit"]
    building_size, building_fields = layouts["Building"]
    mineral_size, _ = layouts["Mineral"]
    campaign_size, _ = layouts["Sc64RaceCampaignState"]
    runtime_size, _ = layouts["Sc64T5RuntimeState"]
    # Compare the serialized entity banks word for word. State outside those
    # banks, such as renderer caches, is not part of the save payload.
    unit_count = STORAGE_UNIT_COUNTS[generation]
    invincible_units = rsp.read(sym["invincible_units"], unit_count)
    building_count = sizes["b"] // building_size
    invincible_buildings = rsp.read(sym["invincible_buildings"], building_count)
    mineral_count = STORAGE_MINERAL_COUNTS[generation]
    minerals_raw = rsp.read(sym["m"], mineral_count * mineral_size)
    mineral_words = mineral_size // 4
    minerals_state = [[int.from_bytes(minerals_raw[slot * mineral_size + i * 4:
                                                    slot * mineral_size + i * 4 + 4],
                                     "little", signed=True)
                       for i in range(mineral_words)] for slot in range(mineral_count)]

    required = ("screen", "frame", "race_mode", "race_human", "race_selected_mission",
                "race_completed", "race_cols", "race_rows", "cx", "cy", "minerals", "gas",
                "mission_objective_stage", "won", "lost", "seed", "race_campaign", "race_t5",
                "u", "b", "explored", "race_player", "race_minerals", "race_gas", "geysers",
                "geyser_count", "race_bots", "backwater", "invincible_units",
                "invincible_buildings", "train_type", "weapons_level", "armor_level",
                "weapons_research", "armor_research", "race_skirmish_map", "race_bot_races",
                "race_t3", "race_t3_buildings_killed")
    missing = [name for name in required if name not in sym]
    if missing:
        raise RuntimeError(f"T5 save smoke ELF misses symbols: {', '.join(missing)}")
    result = {
        "screen": read_i32(rsp, sym["screen"]),
        "frame": read_i32(rsp, sym["frame"]),
        "mode": read_i32(rsp, sym["race_mode"]),
        "human": read_i32(rsp, sym["race_human"]),
        "mission": read_i32(rsp, sym["race_selected_mission"]),
        "completed": read_i32(rsp, sym["race_completed"]),
        "geometry": [read_i32(rsp, sym["race_cols"]), read_i32(rsp, sym["race_rows"])],
        "camera": [read_i32(rsp, sym["cx"]), read_i32(rsp, sym["cy"])],
        "minerals": read_i32(rsp, sym["minerals"]),
        "gas": read_i32(rsp, sym["gas"]),
        "objective_stage": read_i32(rsp, sym["mission_objective_stage"]),
        "won": read_i32(rsp, sym["won"]),
        "lost": read_i32(rsp, sym["lost"]),
        "seed": read_u32(rsp, sym["seed"]),
        "race_bots": read_i32(rsp, sym["race_bots"]),
        "skirmish_map": read_i32(rsp, sym["race_skirmish_map"]),
        "bot_races": [read_i32(rsp, sym["race_bot_races"] + i * 4) for i in range(3)],
        "train_and_upgrades": [read_i32(rsp, sym[name]) for name in
                               ("train_type", "weapons_level", "armor_level",
                                "weapons_research", "armor_research")],
        "campaign": rsp.read(sym["race_campaign"], campaign_size).hex(),
        "t5_runtime": rsp.read(sym["race_t5"], runtime_size).hex(),
        "t3": rsp.read(sym["race_t3"], sizes["race_t3"]).hex(),
        "t3_buildings_killed": read_u16(rsp, sym["race_t3_buildings_killed"]),
        "backwater": rsp.read(sym["backwater"], sizes["backwater"]).hex(),
        "invincible_units": invincible_units.hex(),
        "invincible_buildings": invincible_buildings.hex(),
        "players": rsp.read(sym["race_player"], 5 * 4).hex(),
        "player_minerals": rsp.read(sym["race_minerals"], 5 * 4).hex(),
        "player_gas": rsp.read(sym["race_gas"], 5 * 4).hex(),
        "geyser_count": read_i32(rsp, sym["geyser_count"]),
        "geysers": rsp.read(sym["geysers"], 4 * 8).hex(),
        "fog": rsp.read(sym["explored"], sizes["explored"]).hex(),
        "units": (compact_live_unit_rows(rsp, sym["u"], unit_fields,
                                         STORAGE_UNIT_COUNTS[generation], unit_size)
                  if generation >= 17 else
                  persisted_rows(rsp, sym["u"], STORAGE_UNIT_COUNTS[generation],
                                 unit_size, unit_fields)),
        "buildings": persisted_rows(rsp, sym["b"], building_count, building_size, building_fields),
        "mineral_fields": minerals_state,
    }
    if mission == 6 and "Sc64T6MissionState" in layouts:
        result["t6_runtime"] = rsp.read(sym["race_t6"], layouts["Sc64T6MissionState"][0]).hex()
    if generation >= 18 and mission >= 7:
        late_size, late_fields = layouts["Sc64TerranLateMissionState"]
        late_raw = rsp.read(sym["race_late"], late_size)
        trigger_size, trigger_fields = layouts["Sc64TerranLateRuntimeState"]
        player_size, player_fields = layouts["Sc64T4TriggerState"]
        result["terran_late_runtime"] = late_raw.hex()
        result["terran_late"] = late_state_summary(
            late_raw, late_fields, late_size, trigger_fields, trigger_size,
            player_fields, player_size)
    return result
def run(rom: Path, elf: Path, out: Path, ticks: int, generation: int = DEFAULT_GENERATION,
        menu_entry: bool = False, mission: int = 5) -> dict:
    if not rom.is_file() or not elf.is_file():
        raise RuntimeError(f"Build the T5 probe ROM/ELF first: {rom} and {elf}")
    if generation not in STORAGE_SCHEMAS:
        raise ValueError("unsupported Terran save generation")
    if mission < 5 or mission > 10:
        raise ValueError("Terran mission ordinal must be between 5 and 10")
    if mission == 6 and generation not in (16, 17, 18):
        raise ValueError("T6 requires save generation 16 or newer")
    if mission >= 7 and generation != 18:
        raise ValueError("T7-T10 require late Terran save generation 18")
    mission_id = mission + 3
    building_count, expected_payload = STORAGE_SCHEMAS[generation]
    unit_count = STORAGE_UNIT_COUNTS[generation]
    mineral_count = STORAGE_MINERAL_COUNTS[generation]
    if ticks < 1 or ticks > 10000:
        raise ValueError("--ticks must be between 1 and 10000")
    sym = symbols(elf)
    sizes = symbol_sizes(elf)
    required = ("game_update", "demo_inject_keys", "screen", "race_t5", "race_campaign",
                "u", "b", "m", "explored", "geysers")
    if menu_entry:
        required += ("race_front",)
    if mission == 6 or generation >= 17:
        required += ("race_t6",)
    if generation >= 18:
        required += ("race_late",)
    missing = [name for name in required if name not in sym]
    if missing:
        raise RuntimeError(f"T5 probe ELF is missing symbols: {', '.join(missing)}")
    expected_sizes = {"race_t5": 1340, "u": unit_count * (56 if generation >= 17 else 108),
                      "b": building_count * 100, "m": mineral_count * 16,
                      "geysers": 4 * 8}
    if generation >= 17:
        expected_sizes["race_t6"] = 1388
    bad_sizes = {name: (sizes.get(name), expected) for name, expected in expected_sizes.items()
                 if sizes.get(name) != expected}
    if bad_sizes:
        raise RuntimeError("T5 probe ELF has unexpected runtime/entity bank sizes")
    layouts = {name: dwarf_struct(elf, name) for name in
               ("Unit", "Building", "Mineral", "Sc64RaceCampaignState", "Sc64T5RuntimeState")}
    if mission == 6:
        expected_t6_size = STORAGE_T6_STATE_BYTES[generation]
        if sizes.get("race_t6") != expected_t6_size:
            raise RuntimeError("T6 mission state differs from its validated schema")
        layouts["Sc64T6MissionState"] = dwarf_struct(elf, "Sc64T6MissionState")
        layouts["Sc64T6RuntimeState"] = dwarf_struct(elf, "Sc64T6RuntimeState")
    if generation >= 17:
        expected_fields = set(UNIT_RUNTIME_FIELDS)
        if (layouts["Unit"][0] != 56 or
                not expected_fields.issubset(layouts["Unit"][1])):
            raise RuntimeError("late-profile Unit DWARF fields differ from the named-field schema")
    if menu_entry:
        layouts["Sc64Frontend"] = dwarf_struct(elf, "Sc64Frontend")
    runtime_size = layouts["Sc64T5RuntimeState"][0]
    if runtime_size != 1340:
        raise RuntimeError("T5 runtime or geyser layout differs from the validated save schema")
    if generation >= 18:
        layouts["Sc64T5SaveState"] = dwarf_struct(elf, "Sc64T5SaveState")
        layouts["Sc64T6MissionState"] = dwarf_struct(elf, "Sc64T6MissionState")
        layouts["Sc64TerranLateMissionState"] = dwarf_struct(elf, "Sc64TerranLateMissionState")
        layouts["Sc64TerranLateRuntimeState"] = dwarf_struct(elf, "Sc64TerranLateRuntimeState")
        layouts["Sc64T4TriggerState"] = dwarf_struct(elf, "Sc64T4TriggerState")
        if sizes.get("race_late") != layouts["Sc64TerranLateMissionState"][0]:
            raise RuntimeError("late Terran runtime symbol differs from its DWARF layout")
    save_size, save_fields = dwarf_struct(elf, "RaceSave")
    if expected_payload is not None and save_size != expected_payload:
        raise RuntimeError(f"expected {expected_payload}-byte generation-{generation} payload, got {save_size}")
    if generation >= 18:
        union_offset = race_save_union_offset(save_fields)
        late_size = layouts["Sc64TerranLateMissionState"][0]
        member_sizes = {"t5": layouts["Sc64T5SaveState"][0],
                        "t6": layouts["Sc64T6MissionState"][0],
                        "terran_late": late_size}
        union_size = max(member_sizes.values())
        if union_offset + union_size != save_size:
            raise RuntimeError("generation-18 RaceSave DWARF misses its late state union member")

    out.mkdir(parents=True, exist_ok=True)
    out = out / f"run-{time.time_ns()}"
    out.mkdir()
    first_name, second_name = f"t{mission}-save-source", f"t{mission}-save-fresh-load"
    running = start(first_name, rom, out, sym)
    proc, sock, rsp, log = running
    source_saves = out / "saves" / first_name
    late_payload_matches_runtime = None
    try:
        if menu_entry:
            front_page = read_i32(rsp, sym["race_front"] + layouts["Sc64Frontend"][1]["page"])
            if read_i32(rsp, sym["screen"]) != SCREEN_TITLE or front_page != FRONT_HOME:
                raise RuntimeError("normal T5 save smoke did not start at HOME")
            enter_t5_campaign_menu(rsp, sym["demo_inject_keys"], mission)
        initial = snapshot(rsp, sym, sizes, layouts, generation)
        valid_geometry = (initial["geometry"] == [96, 96] if mission <= 6 else
                          len(initial["geometry"]) == 2 and
                          0 < initial["geometry"][0] <= 128 and
                          0 < initial["geometry"][1] <= 128)
        if ((initial["screen"], initial["mode"], initial["mission"]) !=
                (SCREEN_PLAY, 0, mission_id) or not valid_geometry):
            raise RuntimeError(f"entry did not launch mission {mission_id}: {initial}")
        rsp.tick(ticks)
        press(rsp, sym["demo_inject_keys"], KEY_START)
        paused = snapshot(rsp, sym, sizes, layouts, generation)
        if paused["screen"] != SCREEN_PAUSE or paused["frame"] < ticks:
            raise RuntimeError(f"T5 did not reach a live pause after {ticks} ticks")
        rsp.tick(120)
        if snapshot(rsp, sym, sizes, layouts, generation) != paused:
            raise RuntimeError("T5 state advanced while paused")
        press(rsp, sym["demo_inject_keys"], KEY_B)
        if read_i32(rsp, sym["screen"]) != SCREEN_PAUSE:
            raise RuntimeError("B did not save from the T5 pause menu")
        sram = rsp.read(SRAM_BASE, SRAM_BYTES)
        records = save_record_info(sram, save_size, generation)
        if generation >= 17:
            saved_payload = newest_record_payload(sram, generation, save_size)
            compact_units = decode_compact_unit_rows(saved_payload, unit_count)
            if compact_units != paused["units"]:
                raise RuntimeError(f"generation-{generation} compact Unit rows differ from every runtime slot")
            if generation >= 18:
                member_sizes = {"t5": layouts["Sc64T5SaveState"][0],
                                "t6": layouts["Sc64T6MissionState"][0],
                                "terran_late": layouts["Sc64TerranLateMissionState"][0]}
                active_payload, _ = save_union_member(saved_payload, save_fields,
                                                       mission, member_sizes)
                member = active_save_member(mission)
                if member == "terran_late":
                    late_payload_matches_runtime = (
                        active_payload.hex() == paused["terran_late_runtime"])
                    if not late_payload_matches_runtime:
                        raise RuntimeError("generation-18 saved late state differs from runtime bytes")
                elif member == "t6":
                    if active_payload.hex() != paused["t6_runtime"]:
                        raise RuntimeError("generation-18 saved T6 state differs from runtime bytes")
                # T5 uses a compact union member, so its exact bytes are
                # checked by decoding and comparing all runtime state fields.
        menu.capture(rsp, out / "t5-save-pause.png")
    finally:
        menu.stop_emulator(proc, sock, log)

    files = list(source_saves.glob("*.sav"))
    if len(files) != 1 or files[0].stat().st_size != SRAM_BYTES:
        raise RuntimeError(f"expected one flushed 32 KiB SRAM file, found {files}")
    saved_sram = files[0].read_bytes()
    if saved_sram != sram:
        raise RuntimeError("flushed mGBA SRAM file differs from the debugger-observed saved SRAM")
    fresh_saves = out / "saves" / second_name
    fresh_saves.mkdir(parents=True, exist_ok=True)
    shutil.copy2(files[0], fresh_saves / files[0].name)

    running = start(second_name, rom, out, sym)
    proc, sock, rsp, log = running
    try:
        if menu_entry:
            front_page = read_i32(rsp, sym["race_front"] + layouts["Sc64Frontend"][1]["page"])
            if read_i32(rsp, sym["screen"]) != SCREEN_TITLE or front_page != FRONT_HOME:
                raise RuntimeError("fresh normal T5 save process did not start at HOME")
            enter_t5_campaign_menu(rsp, sym["demo_inject_keys"], mission)
        fresh = snapshot(rsp, sym, sizes, layouts, generation)
        if (fresh["screen"], fresh["mode"], fresh["mission"]) != (SCREEN_PLAY, 0, mission_id):
            raise RuntimeError("fresh T5 probe process did not enter the internal mission")
        press(rsp, sym["demo_inject_keys"], KEY_START)
        if read_i32(rsp, sym["screen"]) != SCREEN_PAUSE:
            raise RuntimeError("fresh T5 process did not pause before loading")
        rsp.write(sym["demo_inject_keys"], struct.pack("<H", KEY_L))
        rsp.tick(2)  # Load, then stop at the next game_update entry before live simulation.
        rsp.write(sym["demo_inject_keys"], b"\0\0")  # Deliberately do not tick after release.
        restored = snapshot(rsp, sym, sizes, layouts, generation)
        if restored["screen"] != SCREEN_PLAY:
            raise RuntimeError(f"L did not load T5 into play: screen={restored['screen']}")
        comparable_before = {k: v for k, v in paused.items() if k != "screen"}
        comparable_loaded = {k: v for k, v in restored.items() if k != "screen"}
        if comparable_loaded != comparable_before:
            mismatches = [k for k in comparable_before if comparable_before[k] != comparable_loaded.get(k)]
            raise RuntimeError("fresh T5 SRAM load mismatch in: " + ", ".join(mismatches))
        rsp.tick(60)
        continued = snapshot(rsp, sym, sizes, layouts, generation)
        if continued["screen"] != SCREEN_PLAY or continued["frame"] != restored["frame"] + 60:
            raise RuntimeError("loaded T5 mission did not continue advancing")
        # Read trigger fault bytes from Sc64T5RuntimeState's nested player[8].
        runtime_type = ("Sc64T5RuntimeState" if mission == 5 else
                        "Sc64T6RuntimeState" if mission == 6 else
                        "Sc64TerranLateRuntimeState")
        _, runtime_fields = layouts[runtime_type]
        if "Sc64T4TriggerState" in layouts:
            player_size, player_fields = layouts["Sc64T4TriggerState"]
        else:
            player_size, player_fields = dwarf_struct(elf, "Sc64T4TriggerState")
        if mission == 5:
            runtime = rsp.read(sym["race_t5"], runtime_size)
        elif mission == 6:
            triggers_offset = layouts["Sc64T6MissionState"][1]["triggers"]
            runtime = rsp.read(sym["race_t6"] + triggers_offset, layouts[runtime_type][0])
        else:
            triggers_offset = layouts["Sc64TerranLateMissionState"][1]["triggers"]
            runtime = rsp.read(sym["race_late"] + triggers_offset, layouts[runtime_type][0])
        players_offset = runtime_fields["player"]
        fault_offset = player_fields["fault"]
        fault_bytes = [runtime[players_offset + p * player_size + fault_offset]
                       for p in range(8)]
        if any(fault_bytes):
            raise RuntimeError(f"T5 trigger fault after fresh-load continuation: {fault_bytes}")
        if mission == 5:
            ai_size, ai_fields = dwarf_struct(elf, "Sc64T5AiState")
            ai_offset = runtime_fields["ai"]
            ai_elapsed = int.from_bytes(runtime[ai_offset + ai_fields["elapsed_ticks"]:
                                                 ai_offset + ai_fields["elapsed_ticks"] + 4], "little")
            if ai_size != 16:
                raise RuntimeError("unexpected T5 AI state schema")
        elif mission == 6:
            ai_elapsed = None
            clocks = [int.from_bytes(runtime[players_offset + p * player_size + player_fields["elapsed_ticks"]:
                                                   players_offset + p * player_size + player_fields["elapsed_ticks"] + 4],
                                     "little") for p in range(8)]
            if clocks != [continued["frame"]] * 8:
                raise RuntimeError(f"T6 trigger clocks diverged: {clocks}")
        else:
            ai_elapsed = None
            clocks = [int.from_bytes(runtime[players_offset + p * player_size + player_fields["elapsed_ticks"]:
                                                   players_offset + p * player_size + player_fields["elapsed_ticks"] + 4],
                                     "little") for p in range(8)]
            if clocks != [continued["frame"]] * 8:
                raise RuntimeError(f"late Terran trigger clocks diverged: {clocks}")
        campaign_elapsed = int.from_bytes(rsp.read(sym["race_campaign"] +
                                                     dwarf_struct(elf, "Sc64RaceCampaignState")[1]["elapsed_frames"],
                                                     4), "little")
        if (ai_elapsed is not None and ai_elapsed != continued["frame"]) or campaign_elapsed != continued["frame"]:
            raise RuntimeError("campaign/AI clocks diverged during post-load continuation")
        menu.capture(rsp, out / "t5-loaded-play.png")
        result = {
            "rom_sha256": hashlib.sha256(rom.read_bytes()).hexdigest(),
            "elf_sha256": hashlib.sha256(elf.read_bytes()).hexdigest(),
            "save_file_sha256": hashlib.sha256(saved_sram).hexdigest(),
            "save_records": records,
            "save_generation": generation,
            "building_capacity": building_count,
            "unit_capacity": unit_count,
            "mineral_capacity": mineral_count,
            "unit_row_bytes": 38 if generation >= 17 else None,
            "payload_bytes": save_size,
            "saved_frame": paused["frame"],
            "loaded_frame": restored["frame"],
            "continued_frame": continued["frame"],
            "continuation_ticks": 60,
            "faults_after_continue": fault_bytes,
            "mission": mission_id,
            "entry": "normal-campaign-menu" if menu_entry else "internal-probe-auto-entry",
            "controller_inputs": (["HOME:A", *["MISSION:RIGHT"] * ((mission - 1) // 2),
                                   *(["MISSION:DOWN"] if mission % 2 == 0 else []),
                                   "BRIEFING:A", "DEPLOY:A", "START", "B", "START", "L"]
                                  if menu_entry else ["START", "B", "START", "L"]),
            "exact_load_state_match": True,
            "save_load_verified": True,
            "campaign_clock_matches_frame": campaign_elapsed == continued["frame"],
            "ai_clock_matches_frame": ai_elapsed == continued["frame"] if mission == 5 else None,
            "trigger_clocks_match_frame": True if mission >= 6 else None,
            "late_state_bytes": layouts["Sc64TerranLateMissionState"][0] if mission >= 7 else None,
            "late_emitter_unit": paused["terran_late"]["emitter_unit"] if mission >= 7 else None,
            "late_emitter_carrier": paused["terran_late"]["emitter_carrier"] if mission >= 7 else None,
            "late_active_flight_goals": paused["terran_late"]["active_flight_goals"] if mission >= 7 else None,
            "late_flight_goals": paused["terran_late"]["flight_goals"] if mission >= 7 else None,
            "late_trigger_clocks": clocks if mission >= 7 else None,
            "late_trigger_faults": fault_bytes if mission >= 7 else None,
            "late_payload_matches_runtime_bytes": late_payload_matches_runtime,
            "late_state_exact_load_match": (paused.get("terran_late_runtime") ==
                                               restored.get("terran_late_runtime")) if mission >= 7 else None,
            "win_or_fps_claim": False,
        }
    finally:
        menu.stop_emulator(proc, sock, log)
    result["report_path"] = str(out / "t5-save-smoke.json")
    (out / "t5-save-smoke.json").write_text(json.dumps(result, indent=2) + "\n")
    return result


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rom", type=Path, default=DEFAULT_ROM)
    parser.add_argument("--elf", type=Path, default=DEFAULT_ELF)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUT)
    parser.add_argument("--ticks", type=int, default=120,
                        help="normal play ticks before pausing and saving (1..10000)")
    parser.add_argument("--generation", type=int, choices=tuple(STORAGE_SCHEMAS), default=DEFAULT_GENERATION,
                        help="SRAM generation; 13 is pre-cargo, 14 is T1-T5, 15 is the expanded storage probe")
    parser.add_argument("--menu-entry", action="store_true",
                        help="launch the selected Terran mission from HOME through ordinary controls")
    parser.add_argument("--mission", type=int, choices=tuple(range(5, 11)), default=5,
                        help="Terran mission ordinal 5 through 10 (late missions require generation 18)")
    args = parser.parse_args()
    print(json.dumps(run(args.rom, args.elf, args.output, args.ticks, args.generation,
                         args.menu_entry, args.mission), indent=2))


if __name__ == "__main__":
    main()
