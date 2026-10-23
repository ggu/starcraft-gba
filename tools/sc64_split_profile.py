#!/usr/bin/env python3
"""Measure a fixed neutral-input interval on one split StarCraft64 ROM.

The run is driven through the normal title UI, then the debugger installs one
tick-zero neutral controller event. GBA timer deltas and game snapshots are
read only at the scheduler's bounded checkpoints.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import socket
import struct
import subprocess
import sys
import tempfile
import time

TOOLS = Path(__file__).resolve().parent
if str(TOOLS) not in sys.path:
    sys.path.insert(0, str(TOOLS))

import sc64_menu_smoke as menu
import sc64_races_profile as races
import sc64_split_smoke as split
from emulator_smoke import ROOT, capture, symbol_sizes, symbols

CPU_HZ = 16_777_216
SCREEN_PLAY = 1
SCREEN_RESULT = 3
PROFILE_CYCLE_NAMES = (
    "demo_update_cycles", "gfx_draw_cycles", "sc64_orders_cycles",
    "sc64_order_setup_cycles", "sc64_order_query_cycles", "sc64_order_move_cycles",
    "sc64_sight_cycles", "sc64_other_cycles", "sc64_map_cycles",
    "sc64_hud_cycles", "sc64_background_cycles", "sc64_building_cycles",
    "sc64_unit_draw_cycles",
    "sc64_update_mission_cycles", "sc64_update_production_cycles",
    "sc64_update_race_state_cycles", "sc64_update_defenses_cycles",
    "sc64_update_research_cycles", "sc64_update_remainder_cycles",
    "sc64_t5_rescue_cycles", "sc64_t5_mine_cycles",
    "sc64_t5_trigger_ai_cycles",
)
REFINERY_TYPES = {3: "terran_refinery", 17: "protoss_assimilator", 20: "zerg_extractor"}


def hardware_clock(rsp) -> int:
    """Read the 32-bit chained GBA timer used by gba_clock()."""
    timer = rsp.read(0x04000108, 8)
    return int.from_bytes(timer[:2], "little") | (int.from_bytes(timer[4:6], "little") << 16)


def check_debugger_port_free() -> None:
    guard = socket.socket()
    try:
        guard.bind(("127.0.0.1", 2345))
    except OSError as exc:
        raise RuntimeError("mGBA debugger port 2345 is already in use; leaving the existing emulator untouched") from exc
    finally:
        guard.close()


def checked_sram(path: Path) -> Path:
    """Only accept the cartridge SRAM size used by these profiles."""
    path = path.resolve()
    if not path.is_file() or path.stat().st_size != 32768:
        raise ValueError(f"expected one 32 KiB SRAM file: {path}")
    return path


def save_live_checkpoint(rsp, sym: dict[str, int], *, tick: int) -> dict:
    """Leave the observer scheduler, then pause/save using normal controls.

    Called only at an observer checkpoint, after the measured interval ends.
    No mission/entity/resource state is written through the debugger.
    """
    before_frame = u32(rsp, sym["frame"])
    rsp.write(sym["demo_schedule_count"], struct.pack("<I", 0))
    observer = sym["demo_replay_checkpoint"] & ~1
    update = sym["game_update"] & ~1
    if rsp.cmd(f"z1,{observer:x},2") != "OK" or rsp.cmd(f"Z1,{update:x},2") != "OK":
        raise RuntimeError("failed to switch from observer to controller breakpoint")
    rsp.tick()
    split.key(rsp, sym, split.KEY_START)
    if u32(rsp, sym["screen"]) != split.SCREEN_PAUSE:
        raise RuntimeError("checkpoint controller did not pause live play")
    paused_frame = u32(rsp, sym["frame"])
    split.key(rsp, sym, split.KEY_B)
    if u32(rsp, sym["screen"]) != split.SCREEN_PAUSE or u32(rsp, sym["frame"]) != paused_frame:
        raise RuntimeError("checkpoint save advanced the paused simulation")
    return {"observer_tick": tick, "observer_frame": before_frame,
            "saved_frame": paused_frame, "input": "START pause, B save"}


def u32(rsp, address: int) -> int:
    return int.from_bytes(rsp.read(address, 4), "little")


def decode_z3_state(raw: bytes, offsets: dict[str, int], action_ticks: int) -> dict:
    """Decode the Z3 campaign model using the ELF's DWARF member offsets."""
    widths = {"fired": 2, "switch7": 1, "outcome": 1, "sequence": 1, "transmission": 1}
    required = set(widths)
    if required - offsets.keys():
        raise RuntimeError(f"Z3 DWARF layout is incomplete: {sorted(required - offsets.keys())}")
    values = {}
    for field, width in widths.items():
        offset = offsets[field]
        if offset < 0 or offset + width > len(raw):
            raise RuntimeError(f"Z3 DWARF field {field} is outside its structure")
        values[field] = int.from_bytes(raw[offset:offset + width], "little")
    values["action_ticks"] = action_ticks
    return values


def decode_p3_state(raw: bytes, offsets: dict[str, int], action_ticks: int) -> dict:
    """Decode Higher Ground's persisted model using its actual DWARF layout."""
    widths = {"fired": 2, "outcome": 1, "sequence": 1, "transmission": 1, "reserved": 1}
    if widths.keys() - offsets.keys():
        raise RuntimeError("P3 DWARF layout is incomplete")
    values = {}
    for field, width in widths.items():
        offset = offsets[field]
        if offset < 0 or offset + width > len(raw):
            raise RuntimeError(f"P3 DWARF field {field} is outside its structure")
        values[field] = int.from_bytes(raw[offset:offset + width], "little")
    values["action_ticks"] = action_ticks
    return values


def write_progress(path: Path, *, profile: str, rom: Path, elf: Path,
                   rom_sha256: str, elf_sha256: str, configuration: dict,
                   requested_ticks: int, checkpoint_interval: int,
                   latest_checkpoint: dict, status: str = "running",
                   final_report: str | None = None) -> None:
    """Atomically publish the latest complete observer snapshot."""
    progress = {
        "status": status, "profile": profile,
        "rom": str(rom), "elf": str(elf),
        "rom_sha256": rom_sha256, "elf_sha256": elf_sha256,
        "configuration": configuration,
        "requested_ticks": requested_ticks,
        "checkpoint_interval": checkpoint_interval,
        "observed_ticks": latest_checkpoint["tick"],
        "latest_checkpoint": latest_checkpoint,
    }
    if final_report is not None:
        progress["final_report"] = final_report
    path.parent.mkdir(parents=True, exist_ok=True)
    temp_name = None
    try:
        with tempfile.NamedTemporaryFile(mode="w", encoding="utf-8", dir=path.parent,
                                         prefix=f".{path.name}.", suffix=".tmp",
                                         delete=False) as stream:
            temp_name = stream.name
            json.dump(progress, stream, indent=2)
            stream.write("\n")
            stream.flush()
            os.fsync(stream.fileno())
        os.replace(temp_name, path)
    finally:
        if temp_name and os.path.exists(temp_name):
            os.unlink(temp_name)


def observe(rsp, sym: dict[str, int], sizes: dict[str, int], bank: dict,
            campaign_layout: tuple[int, dict[str, int]] | None = None,
            t3_layout: tuple[int, dict[str, int]] | None = None,
            z3_layout: tuple[int, dict[str, int]] | None = None,
            p3_layout: tuple[int, dict[str, int]] | None = None) -> dict:
    units = races.read_entities(rsp, sym["u"], bank["unit_capacity"],
                                bank["unit_stride_bytes"], bank["unit_offsets"])
    buildings = races.read_entities(rsp, sym["b"], bank["building_capacity"],
                                    bank["building_stride_bytes"], bank["building_offsets"],
                                    building=True)
    state = {name: u32(rsp, sym[name]) for name in
             ("screen", "frame", "won", "lost", "race_mode", "race_human", "race_bots",
              "race_skirmish_map", "race_selected_mission", "race_cols", "race_rows",
              "minerals", "gas") if name in sym}
    state["units"] = units
    state["buildings"] = buildings
    refinery_counts: dict[str, int] = {}
    for building in buildings["entities"]:
        refinery_name = REFINERY_TYPES.get(building["type"])
        if refinery_name:
            owner = str(building["owner"])
            refinery_counts[owner] = refinery_counts.get(owner, 0) + 1
    state["refineries_by_owner"] = refinery_counts
    for name in ("race_minerals", "race_gas"):
        if name in sym:
            size = sizes.get(name, 0)
            if size and size % 4 == 0:
                state[name] = list(struct.unpack("<" + "i" * (size // 4), rsp.read(sym[name], size)))
    if campaign_layout and "race_campaign" in sym:
        size, offsets = campaign_layout
        state["race_campaign"] = races.campaign_state(rsp, sym["race_campaign"], size, offsets)
    if "race_t3_buildings_killed" in sym:
        counter_size = sizes.get("race_t3_buildings_killed", 0)
        if counter_size != 2:
            raise RuntimeError(f"race_t3_buildings_killed has unexpected size {counter_size}")
        state["race_t3_buildings_killed"] = int.from_bytes(
            rsp.read(sym["race_t3_buildings_killed"], 2), "little")
    if t3_layout and "race_t3" in sym:
        size, offsets = t3_layout
        widths = {"elapsed_ticks": 4, "remaining_ticks": 4, "fired": 2,
                  "outcome": 1, "objective": 1}
        raw = rsp.read(sym["race_t3"], size)
        state["race_t3"] = {
            field: int.from_bytes(raw[offsets[field]:offsets[field] + width], "little")
            for field, width in widths.items() if field in offsets
        }
    if z3_layout:
        z3_size, z3_offsets = z3_layout
        action_ticks_size = sizes.get("race_z3_action_ticks", 0)
        if action_ticks_size != 2:
            raise RuntimeError(f"race_z3_action_ticks has unexpected ELF size {action_ticks_size}")
        raw = rsp.read(sym["race_z3"], z3_size)
        action_ticks = int.from_bytes(rsp.read(sym["race_z3_action_ticks"], action_ticks_size), "little")
        state["race_z3"] = decode_z3_state(raw, z3_offsets, action_ticks)
    if p3_layout:
        size, offsets = p3_layout
        if sizes.get("race_p3_action_ticks", 0) != 2:
            raise RuntimeError("P3 action timer must be a 16-bit ELF symbol")
        raw = rsp.read(sym["race_p3"], size)
        action_ticks = int.from_bytes(rsp.read(sym["race_p3_action_ticks"], 2), "little")
        state["race_p3"] = decode_p3_state(raw, offsets, action_ticks)
    return state


def run(profile: str, map_id: int, race: int, bots: int, ticks: int,
        checkpoint: int, output: Path, mission: int | None = None,
        rom_path: Path | None = None, elf_path: Path | None = None,
        require_win: bool = False, load_save: Path | None = None,
        save_at_end: bool = False) -> dict:
    if ticks < 1:
        raise ValueError("ticks must be positive")
    # The target's main loop checkpoints at every 1000 ticks and at the exact
    # requested endpoint; keep the CLI explicit if that schedule changes.
    if checkpoint != 1000:
        raise ValueError("this ROM scheduler checkpoints at fixed 1000-tick intervals")
    if ticks > 0xFFFFFFFF:
        raise ValueError("ticks exceeds the 32-bit controller scheduler length")
    if load_save is not None:
        load_save = checked_sram(load_save)
        if mission is None:
            raise ValueError("--load-save requires --mission to verify the loaded campaign")
    if save_at_end and require_win:
        raise ValueError("--save-at-end requires live play, so cannot be combined with --require-win")

    rom = rom_path or ROOT / "build" / f"StarCraft-{profile.capitalize()}-GBA.gba"
    elf = elf_path or ROOT / "build" / f"starcraft-{profile}.elf"
    if not rom.is_file() or not elf.is_file():
        raise RuntimeError(f"{profile}: build both ROM and ELF first ({rom}, {elf})")
    rom_sha256 = hashlib.sha256(rom.read_bytes()).hexdigest()
    elf_sha256 = hashlib.sha256(elf.read_bytes()).hexdigest()
    # The split UI helpers inspect the appended Sc64Frontend fields through
    # DWARF offsets. Populate their shared offset table before starting UI.
    split.FRONT_FIELDS = split.frontend_offsets(elf)
    sym, sizes = symbols(elf), symbol_sizes(elf)
    bank = races.race_bank_profile(elf, sizes)
    campaign = None
    campaign_layout = None
    t3_layout = None
    z3_layout = None
    p3_layout = None
    mission_ordinal = None
    if mission is not None:
        campaign = split.read_campaign_metadata(rom, elf, profile)
        mission_ids = campaign["enabled_mission_ids"]
        if mission not in mission_ids:
            raise ValueError(f"mission {mission} is not enabled for {profile}; available IDs: {mission_ids}")
        mission_ordinal = mission_ids.index(mission)
        campaign_layout = races.dwarf_struct(elf, "Sc64RaceCampaignState")
        if "race_selected_mission" not in sym or "race_campaign" not in sym:
            raise RuntimeError(f"{profile}: campaign ELF lacks mission/campaign state symbols")
        if mission == 6:
            if "race_t3" not in sym or "race_t3_buildings_killed" not in sym:
                raise RuntimeError("T3 campaign ELF lacks its model state or building-kill counter")
            t3_layout = races.dwarf_struct(elf, "Sc64T3State")
            required_t3_fields = {"elapsed_ticks", "remaining_ticks", "fired", "outcome", "objective"}
            if required_t3_fields - t3_layout[1].keys() or t3_layout[0] < 12:
                raise RuntimeError(f"T3 DWARF layout is incomplete: {t3_layout}")
        if mission == 14:
            missing_z3 = {"race_z3", "race_z3_action_ticks"} - sym.keys()
            if missing_z3:
                raise RuntimeError(f"Z3 campaign ELF lacks model symbols: {sorted(missing_z3)}")
            z3_layout = races.dwarf_struct(elf, "Sc64Z3State")
            required_z3_fields = {"fired", "switch7", "outcome", "sequence", "transmission"}
            if required_z3_fields - z3_layout[1].keys() or z3_layout[0] < 6:
                raise RuntimeError(f"Z3 DWARF layout is incomplete: {z3_layout}")
            if sizes.get("race_z3_action_ticks", 0) != 2:
                raise RuntimeError("Z3 action timer must be a 16-bit ELF symbol")
        if mission == 22:
            missing = {"race_p3", "race_p3_action_ticks"} - sym.keys()
            if missing:
                raise RuntimeError(f"P3 campaign ELF lacks model symbols: {sorted(missing)}")
            p3_layout = races.dwarf_struct(elf, "Sc64P3State")
            decode_p3_state(bytes(p3_layout[0]), p3_layout[1], 0)
            if sizes.get("race_p3_action_ticks", 0) != 2:
                raise RuntimeError("P3 action timer must be a 16-bit ELF symbol")
    required = (
        "game_update", "demo_inject_keys", "demo_schedule_events", "demo_schedule_count", "demo_schedule_length",
        "demo_schedule_tick", "demo_replay_checkpoint", "demo_tick_buckets", "demo_last_cycles",
        "demo_peak_cycles", "u", "b", "screen", "frame", "won", "lost", "race_mode",
        "race_human", "race_bots", "race_skirmish_map", "race_cols", "race_rows",
        "race_bot_races", "race_minerals", "race_gas", "minerals", "gas",
    )
    missing = [name for name in required if name not in sym]
    if missing:
        raise RuntimeError(f"{profile}: ELF is missing symbols: {', '.join(missing)}")
    capacity = sizes.get("demo_schedule_events", 0) // 8
    if capacity < 1:
        raise RuntimeError("controller event queue has no capacity")

    scenario = f"mission{mission}" if mission is not None else f"map{map_id}-race{race}-bots{bots}"
    out = output / f"{profile}-{scenario}-{time.time_ns()}"
    out.mkdir(parents=True, exist_ok=False)
    label = out.name
    save_dir = out / "saves" / label
    if load_save is not None:
        save_dir.mkdir(parents=True, exist_ok=True)
        shutil.copy2(load_save, save_dir / (rom.stem + ".sav"))
    check_debugger_port_free()
    proc = sock = rsp = log = None
    try:
        proc, sock, rsp, log = split.start_emulator(label, rom, out, sym)
        loaded_frame = None
        if load_save is not None:
            # Input was sampled before the breakpoint. Two continues execute
            # the old neutral call and then HOME's L load, with no live tick.
            rsp.write(sym["demo_inject_keys"], struct.pack("<H", split.KEY_L))
            rsp.tick(2)
            rsp.write(sym["demo_inject_keys"], b"\0\0")
            loaded = observe(rsp, sym, sizes, bank, campaign_layout, t3_layout, z3_layout, p3_layout)
            if (loaded["screen"], loaded["race_mode"], loaded["race_selected_mission"]) != (SCREEN_PLAY, 0, mission):
                raise RuntimeError("HOME L did not load the requested campaign SRAM")
            loaded_frame = loaded["frame"]
            launch = {"width": loaded["race_cols"], "height": loaded["race_rows"]}
        elif mission is None:
            launch = split.press_start(rsp, sym, map_id, race, bots)
        else:
            launch = split.campaign_launch(rsp, sym, profile, mission, mission_ordinal,
                                           len(campaign["enabled_mission_ids"]))
        launch_state = split.state(rsp, sym)
        if launch_state["screen"] != SCREEN_PLAY:
            raise RuntimeError(f"scenario did not enter live play: {launch_state}")
        if mission is not None and load_save is None and launch_state["mission"] != mission:
            raise RuntimeError(f"campaign launch selected mission {launch_state['mission']} instead of {mission}")
        configuration = ({"mission": mission, "mission_ordinal": mission_ordinal,
                          "width": launch["width"], "height": launch["height"]}
                         if mission is not None else
                         {"map": map_id, "human_race": race, "opponents": bots,
                          "bot_races": launch["bot_races"][:bots],
                          "width": launch["width"], "height": launch["height"]})
        if load_save is not None:
            configuration["loaded_sram"] = str(load_save)
            configuration["loaded_sram_sha256"] = hashlib.sha256(load_save.read_bytes()).hexdigest()
            configuration["loaded_frame"] = loaded_frame
            if "race_z3" in loaded:
                configuration["loaded_race_z3"] = loaded["race_z3"]
            if "race_p3" in loaded:
                configuration["loaded_race_p3"] = loaded["race_p3"]

        # main.c has already sampled schedule_count==0 for the current
        # game_update iteration. It finishes normally, then sees this queue
        # at the next loop's tick zero and stops at the no-op observer.
        neutral_event = struct.pack("<2I", 0, 0)
        rsp.write(sym["demo_schedule_events"], neutral_event)
        rsp.write(sym["demo_schedule_length"], struct.pack("<I", ticks))
        rsp.write(sym["demo_schedule_count"], struct.pack("<I", 1))
        update = sym["game_update"] & ~1
        checkpoint_addr = sym["demo_replay_checkpoint"] & ~1
        if rsp.cmd(f"z1,{update:x},2") != "OK":
            raise RuntimeError("failed to remove game_update breakpoint")
        if rsp.cmd(f"Z1,{checkpoint_addr:x},2") != "OK":
            raise RuntimeError("failed to install observer checkpoint breakpoint")
        rsp.tick()
        if u32(rsp, sym["demo_schedule_tick"]) != 0:
            raise RuntimeError("scheduler did not stop at neutral tick zero")

        last_tick = 0
        last_clock = hardware_clock(rsp)
        prior_buckets = struct.unpack("<8I", rsp.read(sym["demo_tick_buckets"], 32))
        prior_routes = {key: u32(rsp, sym[key]) for key in ("sc64_route_hits", "sc64_route_builds")
                        if key in sym}
        observations = []
        total_play_cycles = 0
        counted_play_ticks = 0

        def record_checkpoint(tick: int, interval_cycles: int,
                              before_buckets: tuple[int, ...], before_routes: dict[str, int]) -> dict:
            buckets = struct.unpack("<8I", rsp.read(sym["demo_tick_buckets"], 32))
            state = observe(rsp, sym, sizes, bank, campaign_layout, t3_layout, z3_layout, p3_layout)
            current_routes = {key: u32(rsp, sym[key]) for key in before_routes}
            cycles = ({key: u32(rsp, sym[key]) for key in PROFILE_CYCLE_NAMES if key in sym}
                      if state["screen"] == SCREEN_PLAY else None)
            interval_fps = (round(CPU_HZ * (tick - last_tick) / max(1, interval_cycles), 2)
                            if state["screen"] == SCREEN_PLAY else None)
            result = {
                "tick": tick,
                "interval_ticks": tick - last_tick,
                "interval_cycles": interval_cycles,
                "interval_effective_fps": interval_fps,
                "hardware_frames_per_tick_histogram": [(a - b) & 0xFFFFFFFF
                                                        for a, b in zip(buckets, before_buckets)],
                "demo_last_cycles": (u32(rsp, sym["demo_last_cycles"])
                                      if state["screen"] == SCREEN_PLAY else None),
                "demo_peak_cycles_session": u32(rsp, sym["demo_peak_cycles"]),
                "profile_cycles_last_tick": cycles,
                "route_counters": current_routes,
                "route_interval": {key: (current_routes[key] - before_routes[key]) & 0xFFFFFFFF
                                   for key in current_routes},
                **state,
            }
            return result

        # Tick zero is a genuine PLAY snapshot and the baseline for all
        # interval counters; it is excluded from the measured intervals.
        zero = observe(rsp, sym, sizes, bank, campaign_layout, t3_layout, z3_layout, p3_layout)
        if zero["screen"] != SCREEN_PLAY:
            raise RuntimeError(f"tick-zero observer is not on the live game screen: {zero['screen']}")
        zero_capture = capture(rsp, out / "tick-000000.png")
        peak_at_tick_zero = u32(rsp, sym["demo_peak_cycles"])
        zero_cycles = {key: u32(rsp, sym[key]) for key in PROFILE_CYCLE_NAMES if key in sym}
        zero_checkpoint = {"tick": 0, "interval_ticks": 0, "interval_cycles": 0,
                           "interval_effective_fps": None,
                           "hardware_frames_per_tick_histogram": [0] * 8,
                           "demo_last_cycles": u32(rsp, sym["demo_last_cycles"]),
                           "demo_peak_cycles_session": peak_at_tick_zero,
                           "profile_cycles_last_tick": zero_cycles,
                           "route_counters": prior_routes,
                           "route_interval": {key: 0 for key in prior_routes},
                           "counted_as_live_play": True,
                           "capture": zero_capture, **zero}
        observations.append(zero_checkpoint)
        write_progress(out / "progress.json", profile=profile, rom=rom, elf=elf,
                       rom_sha256=rom_sha256, elf_sha256=elf_sha256,
                       configuration=configuration, requested_ticks=ticks,
                       checkpoint_interval=checkpoint, latest_checkpoint=zero_checkpoint)

        ended_early = False
        while last_tick < ticks:
            rsp.tick()
            completed = u32(rsp, sym["demo_schedule_tick"])
            expected = min(ticks, last_tick + checkpoint)
            if completed != expected:
                raise RuntimeError(f"expected observer at tick {expected}, got {completed}")
            now = hardware_clock(rsp)
            elapsed = (now - last_clock) & 0xFFFFFFFF
            record = record_checkpoint(completed, elapsed, prior_buckets, prior_routes)
            record["demo_peak_cycles_new_high_since_tick_zero"] = (
                record["demo_peak_cycles_session"] if record["screen"] == SCREEN_PLAY and
                record["demo_peak_cycles_session"] > peak_at_tick_zero else None)
            if record["screen"] not in (SCREEN_PLAY, SCREEN_RESULT):
                raise RuntimeError(f"run left live play at tick {completed} (screen={record['screen']})")
            if record["screen"] == SCREEN_PLAY:
                total_play_cycles += elapsed
                counted_play_ticks += completed - last_tick
                record["counted_as_live_play"] = True
                record["capture"] = capture(rsp, out / f"tick-{completed:06d}.png")
            else:
                record["counted_as_live_play"] = False
                record["game_ended_by_tick"] = completed
                ended_early = completed < ticks
            observations.append(record)
            write_progress(out / "progress.json", profile=profile, rom=rom, elf=elf,
                           rom_sha256=rom_sha256, elf_sha256=elf_sha256,
                           configuration=configuration, requested_ticks=ticks,
                           checkpoint_interval=checkpoint, latest_checkpoint=record)
            print(f"{profile} {scenario} tick={completed}/{ticks} "
                  f"screen={record['screen']} frame={record.get('frame')} "
                  f"interval-fps={record['interval_effective_fps'] if record['interval_effective_fps'] is not None else 'n/a'}",
                  flush=True)
            last_tick = completed
            last_clock = now
            prior_buckets = struct.unpack("<8I", rsp.read(sym["demo_tick_buckets"], 32))
            prior_routes = record["route_counters"]
            if ended_early or record["screen"] != SCREEN_PLAY:
                break

        result = {
            "profile": profile,
            "rom": str(rom), "elf": str(elf),
            "rom_sha256": rom_sha256,
            "elf_sha256": elf_sha256,
            "configuration": configuration,
            "campaign_catalog": campaign,
            "requested_ticks": ticks, "observed_ticks": last_tick,
            "counted_live_play_ticks": counted_play_ticks,
            "checkpoint_interval": checkpoint,
            "tick_zero_peak_cycles_session_baseline": peak_at_tick_zero,
            "mean_live_play_effective_fps": round(CPU_HZ * counted_play_ticks /
                                                   max(1, total_play_cycles), 2),
            "game_ended_early": ended_early,
            "measurement_notes": [
                "FPS uses GBA hardware timer deltas at observer checkpoints, not host wall time.",
                "Only intervals ending on SCREEN_PLAY contribute to the live-play mean.",
                "An interval ending on SCREEN_RESULT is excluded because its in-interval finish time is unknown.",
                "demo_peak_cycles_session is lifetime-wide; its tick-zero value may include title-menu work.",
            ],
            "unit_bank": {key: bank[key] for key in
                          ("unit_bank_bytes", "unit_capacity", "unit_stride_bytes", "unit_words", "unit_offsets")},
            "building_bank": {key: bank[key] for key in
                              ("building_bank_bytes", "building_capacity", "building_stride_bytes",
                               "building_words", "building_offsets")},
            "checkpoints": observations,
        }
        report = out / "profile.json"
        report.write_text(json.dumps(result, indent=2) + "\n")
        final_won = (observations[-1].get("screen") == SCREEN_RESULT and
                     observations[-1].get("won") == 1 and observations[-1].get("lost") == 0)
        write_progress(out / "progress.json", profile=profile, rom=rom, elf=elf,
                       rom_sha256=rom_sha256, elf_sha256=elf_sha256,
                       configuration=configuration, requested_ticks=ticks,
                       checkpoint_interval=checkpoint, latest_checkpoint=observations[-1],
                       status="finished", final_report=str(report))
        print(json.dumps({"profile": profile, "configuration": result["configuration"],
                          "observed_ticks": last_tick,
                          "mean_live_play_effective_fps": result["mean_live_play_effective_fps"],
                          "result": str(report)}, indent=2))
        if require_win and not final_won:
            raise RuntimeError(f"--require-win: final observed result did not win; report: {report}")
        if save_at_end:
            if observations[-1]["screen"] != SCREEN_PLAY:
                raise RuntimeError("--save-at-end cannot save a completed campaign as live play")
            saved = save_live_checkpoint(rsp, sym, tick=last_tick)
            # mGBA flushes its backup file during graceful process shutdown.
            menu.stop_emulator(proc, sock, log)
            proc = sock = log = None
            saves = list(save_dir.glob("*.sav"))
            if len(saves) != 1:
                raise RuntimeError(f"checkpoint did not flush exactly one SRAM file: {saves}")
            saved_path = checked_sram(saves[0])
            saved.update({"path": str(saved_path),
                          "sha256": hashlib.sha256(saved_path.read_bytes()).hexdigest()})
            result["checkpoint_save"] = saved
            report.write_text(json.dumps(result, indent=2) + "\n")
            print(json.dumps({"checkpoint_save": saved}), flush=True)
        return result
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
                except Exception:
                    proc.kill()
                    proc.wait()
                if log is not None and not log.closed:
                    log.close()


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--profile", choices=("terran", "zerg", "protoss"), default="terran")
    parser.add_argument("--map", type=int, choices=range(2), default=0)
    parser.add_argument("--race", type=int, choices=range(3), default=0,
                        help="human race: 0 Terran, 1 Zerg, 2 Protoss")
    parser.add_argument("--bots", type=int, choices=range(1, 4), default=1)
    parser.add_argument("--mission", type=int,
                        help="profile one enabled campaign mission by its global catalog ID; default is skirmish")
    parser.add_argument("--rom", type=Path, help="override the profile's default GBA ROM")
    parser.add_argument("--elf", type=Path, help="override the profile's default ELF")
    parser.add_argument("--require-win", action="store_true",
                        help="exit with failure unless the final observed screen is a won result")
    parser.add_argument("--load-save", type=Path,
                        help="load campaign SRAM through HOME L before measuring (requires --mission)")
    parser.add_argument("--save-at-end", action="store_true",
                        help="pause/save live play through normal controls after the measured interval")
    parser.add_argument("--ticks", type=int, default=10000)
    parser.add_argument("--checkpoint", type=int, default=1000)
    parser.add_argument("--out", type=Path, default=ROOT / "build/emulator/sc64-split-profile")
    args = parser.parse_args()
    if args.ticks < 1:
        parser.error("--ticks must be positive")
    if args.checkpoint != 1000:
        parser.error("the current ROM observer checkpoints every 1000 ticks")
    try:
        run(args.profile, args.map, args.race, args.bots, args.ticks,
            args.checkpoint, args.out, mission=args.mission,
            rom_path=args.rom, elf_path=args.elf, require_win=args.require_win,
            load_save=args.load_save, save_at_end=args.save_at_end)
    except (OSError, RuntimeError, ValueError, AssertionError, subprocess.CalledProcessError) as exc:
        parser.exit(1, f"sc64_split_profile: {exc}\n")


if __name__ == "__main__":
    main()
