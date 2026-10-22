#!/usr/bin/env python3
"""Verify T5 SRAM persistence through normal controller input on the ARM ROM.

The internal T5 probe auto-enters mission 8 for engine testing. This tool
uses only Start/B/L controller input after launch; all game-state inspection
is read-only through mGBA's RSP debugger. It pauses, saves to mGBA's SRAM file,
loads that file in a fresh process, compares the saved gameplay state, then
checks that play advances without trigger faults.
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
KEY_B, KEY_L, KEY_START = 2, 512, 8
SCREEN_PLAY, SCREEN_PAUSE = 1, 2

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
        raise RuntimeError(f"mGBA save file has no valid generation-{generation} T5 SRAM record")
    return records


def snapshot(rsp, sym: dict[str, int], sizes: dict[str, int],
             layouts: dict[str, tuple[int, dict[str, int]]]) -> dict:
    unit_size, unit_fields = layouts["Unit"]
    building_size, building_fields = layouts["Building"]
    mineral_size, _ = layouts["Mineral"]
    campaign_size, _ = layouts["Sc64RaceCampaignState"]
    runtime_size, _ = layouts["Sc64T5RuntimeState"]
    # Compare the serialized entity banks word for word. State outside those
    # banks, such as renderer caches, is not part of the save payload.
    invincible_units = rsp.read(sym["invincible_units"], 160)
    invincible_buildings = rsp.read(sym["invincible_buildings"], 64)
    minerals_raw = rsp.read(sym["m"], 48 * mineral_size)
    mineral_words = mineral_size // 4
    minerals_state = [[int.from_bytes(minerals_raw[slot * mineral_size + i * 4:
                                                    slot * mineral_size + i * 4 + 4],
                                     "little", signed=True)
                       for i in range(mineral_words)] for slot in range(48)]

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
    return {
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
        "units": persisted_rows(rsp, sym["u"], 160, unit_size, unit_fields),
        "buildings": persisted_rows(rsp, sym["b"], 64, building_size, building_fields),
        "mineral_fields": minerals_state,
    }
def run(rom: Path, elf: Path, out: Path, ticks: int, generation: int = DEFAULT_GENERATION) -> dict:
    if not rom.is_file() or not elf.is_file():
        raise RuntimeError(f"Build the T5 probe ROM/ELF first: {rom} and {elf}")
    if generation not in (13, 14):
        raise ValueError("supported T5 save generations are 13 and 14")
    if ticks < 1 or ticks > 10000:
        raise ValueError("--ticks must be between 1 and 10000")
    sym = symbols(elf)
    sizes = symbol_sizes(elf)
    required = ("game_update", "demo_inject_keys", "screen", "race_t5", "race_campaign",
                "u", "b", "m", "explored", "geysers")
    missing = [name for name in required if name not in sym]
    if missing:
        raise RuntimeError(f"T5 probe ELF is missing symbols: {', '.join(missing)}")
    expected_sizes = {"race_t5": 1340, "u": 160 * 108, "b": 64 * 100,
                      "m": 48 * 16, "geysers": 4 * 8}
    bad_sizes = {name: (sizes.get(name), expected) for name, expected in expected_sizes.items()
                 if sizes.get(name) != expected}
    if bad_sizes:
        raise RuntimeError("T5 probe ELF has unexpected runtime/entity bank sizes")
    layouts = {name: dwarf_struct(elf, name) for name in
               ("Unit", "Building", "Mineral", "Sc64RaceCampaignState", "Sc64T5RuntimeState")}
    runtime_size = layouts["Sc64T5RuntimeState"][0]
    if runtime_size != 1340:
        raise RuntimeError("T5 runtime or geyser layout differs from the validated save schema")
    save_size, _ = dwarf_struct(elf, "RaceSave")
    if save_size != 16328:
        raise RuntimeError(f"expected 16328-byte T5 payload, got {save_size}")

    out.mkdir(parents=True, exist_ok=True)
    out = out / f"run-{time.time_ns()}"
    out.mkdir()
    first_name, second_name = "t5-save-source", "t5-save-fresh-load"
    running = start(first_name, rom, out, sym)
    proc, sock, rsp, log = running
    source_saves = out / "saves" / first_name
    try:
        initial = snapshot(rsp, sym, sizes, layouts)
        if (initial["screen"], initial["mode"], initial["mission"], initial["geometry"]) != (
                SCREEN_PLAY, 0, 8, [96, 96]):
            raise RuntimeError(f"internal T5 entry did not launch mission 8: {initial}")
        rsp.tick(ticks)
        press(rsp, sym["demo_inject_keys"], KEY_START)
        paused = snapshot(rsp, sym, sizes, layouts)
        if paused["screen"] != SCREEN_PAUSE or paused["frame"] < ticks:
            raise RuntimeError(f"T5 did not reach a live pause after {ticks} ticks")
        rsp.tick(120)
        if snapshot(rsp, sym, sizes, layouts) != paused:
            raise RuntimeError("T5 state advanced while paused")
        press(rsp, sym["demo_inject_keys"], KEY_B)
        if read_i32(rsp, sym["screen"]) != SCREEN_PAUSE:
            raise RuntimeError("B did not save from the T5 pause menu")
        sram = rsp.read(SRAM_BASE, SRAM_BYTES)
        records = save_record_info(sram, save_size, generation)
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
        fresh = snapshot(rsp, sym, sizes, layouts)
        if (fresh["screen"], fresh["mode"], fresh["mission"]) != (SCREEN_PLAY, 0, 8):
            raise RuntimeError("fresh T5 probe process did not enter the internal mission")
        press(rsp, sym["demo_inject_keys"], KEY_START)
        if read_i32(rsp, sym["screen"]) != SCREEN_PAUSE:
            raise RuntimeError("fresh T5 process did not pause before loading")
        rsp.write(sym["demo_inject_keys"], struct.pack("<H", KEY_L))
        rsp.tick(2)  # Load, then stop at the next game_update entry before live simulation.
        rsp.write(sym["demo_inject_keys"], b"\0\0")  # Deliberately do not tick after release.
        restored = snapshot(rsp, sym, sizes, layouts)
        if restored["screen"] != SCREEN_PLAY:
            raise RuntimeError(f"L did not load T5 into play: screen={restored['screen']}")
        comparable_before = {k: v for k, v in paused.items() if k != "screen"}
        comparable_loaded = {k: v for k, v in restored.items() if k != "screen"}
        if comparable_loaded != comparable_before:
            mismatches = [k for k in comparable_before if comparable_before[k] != comparable_loaded.get(k)]
            raise RuntimeError("fresh T5 SRAM load mismatch in: " + ", ".join(mismatches))
        rsp.tick(60)
        continued = snapshot(rsp, sym, sizes, layouts)
        if continued["screen"] != SCREEN_PLAY or continued["frame"] != restored["frame"] + 60:
            raise RuntimeError("loaded T5 mission did not continue advancing")
        # Read trigger fault bytes from Sc64T5RuntimeState's nested player[8].
        _, runtime_fields = layouts["Sc64T5RuntimeState"]
        player_size, player_fields = dwarf_struct(elf, "Sc64T4TriggerState")
        runtime = rsp.read(sym["race_t5"], runtime_size)
        players_offset = runtime_fields["player"]
        fault_offset = player_fields["fault"]
        fault_bytes = [runtime[players_offset + p * player_size + fault_offset]
                       for p in range(8)]
        if any(fault_bytes):
            raise RuntimeError(f"T5 trigger fault after fresh-load continuation: {fault_bytes}")
        ai_size, ai_fields = dwarf_struct(elf, "Sc64T5AiState")
        ai_offset = runtime_fields["ai"]
        ai_elapsed = int.from_bytes(runtime[ai_offset + ai_fields["elapsed_ticks"]:
                                             ai_offset + ai_fields["elapsed_ticks"] + 4], "little")
        campaign_elapsed = int.from_bytes(rsp.read(sym["race_campaign"] +
                                                     dwarf_struct(elf, "Sc64RaceCampaignState")[1]["elapsed_frames"],
                                                     4), "little")
        if ai_size != 16 or ai_elapsed != continued["frame"] or campaign_elapsed != continued["frame"]:
            raise RuntimeError("T5 campaign/AI clocks diverged during post-load continuation")
        menu.capture(rsp, out / "t5-loaded-play.png")
        result = {
            "rom_sha256": hashlib.sha256(rom.read_bytes()).hexdigest(),
            "elf_sha256": hashlib.sha256(elf.read_bytes()).hexdigest(),
            "save_file_sha256": hashlib.sha256(saved_sram).hexdigest(),
            "save_records": records,
            "save_generation": generation,
            "saved_frame": paused["frame"],
            "loaded_frame": restored["frame"],
            "continued_frame": continued["frame"],
            "continuation_ticks": 60,
            "faults_after_continue": fault_bytes,
            "mission": 8,
            "entry": "internal-probe-auto-entry",
            "controller_inputs": ["START", "B", "START", "L"],
            "exact_load_state_match": True,
            "save_load_verified": True,
            "campaign_clock_matches_frame": campaign_elapsed == continued["frame"],
            "ai_clock_matches_frame": ai_elapsed == continued["frame"],
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
    parser.add_argument("--generation", type=int, choices=(13, 14), default=DEFAULT_GENERATION,
                        help="SRAM generation; 13 checks the historical pre-cargo build")
    args = parser.parse_args()
    print(json.dumps(run(args.rom, args.elf, args.output, args.ticks, args.generation), indent=2))


if __name__ == "__main__":
    main()
