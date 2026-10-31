#!/usr/bin/env python3
"""Controller-smoke the three split race/campaign ARM ROMs in mGBA.

By default exercises every enabled campaign mission plus 27 map/faction/AI-count
launches per profile. `--map`, `--race`, and `--bots` can narrow skirmishes;
`--ticks 10000` adds a bounded live-economy run to every selected launch.
"""
from __future__ import annotations
import argparse
import hashlib
import json
import pathlib
import re
import shutil
import socket
import struct
import subprocess
import time

import sc64_menu_smoke as menu
from emulator_smoke import ROOT
from sc64_campaign_metadata import read_campaign_metadata
from sc64_races_profile import dwarf_struct

KEY_A, KEY_B, KEY_START, KEY_LEFT, KEY_RIGHT, KEY_UP, KEY_DOWN, KEY_L = 1, 2, 8, 32, 16, 64, 128, 512
SCREEN_TITLE, SCREEN_PLAY, SCREEN_PAUSE = 0, 1, 2
FRONT_HOME, FRONT_CAMPAIGN, FRONT_MISSION, FRONT_BRIEFING, FRONT_SKIRMISH = range(5)
R_TERRAN, R_ZERG, R_PROTOSS = range(3)
PROFILE_IDS = {"terran": R_TERRAN, "zerg": R_ZERG, "protoss": R_PROTOSS,
               "protoss-p6": R_PROTOSS}
MISSIONS_PER_PAGE = 2
FRONT_FIELDS: dict[str, int] = {}
MAP_DIMS = ((64, 64), (64, 64))
PROFILE_BUILD_PATHS = {
    "terran": ("StarCraft-Terran-GBA.gba", "starcraft-terran.elf"),
    "zerg": ("StarCraft-Zerg-Z7-GBA.gba", "starcraft-zerg-z7.elf"),
    "protoss": ("StarCraft-Protoss-P7-GBA.gba", "starcraft-protoss-p7.elf"),
    "protoss-p6": ("StarCraft-Protoss-P6-GBA.gba", "starcraft-protoss-p6.elf"),
}


def i32(rsp, address: int) -> int:
    return struct.unpack("<i", rsp.read(address, 4))[0]


def start_emulator(name: str, rom: pathlib.Path, out: pathlib.Path,
                  sym: dict[str, int]):
    probe = socket.socket()
    try:
        probe.bind(("127.0.0.1", 2345))
    except OSError as exc:
        raise RuntimeError("Port 2345 is already in use; leave the existing debugger untouched") from exc
    finally:
        probe.close()
    return menu.start_emulator(name, rom, out, sym)


def key(rsp, sym: dict[str, int], value: int, ticks: int = 2) -> None:
    rsp.write(sym["demo_inject_keys"], struct.pack("<H", value))
    rsp.tick(ticks)
    rsp.write(sym["demo_inject_keys"], b"\0\0")
    rsp.tick(2)


def frontend_offsets(elf: pathlib.Path) -> dict[str, int]:
    readelf = shutil.which("arm-none-eabi-readelf")
    if not readelf:
        raise RuntimeError("arm-none-eabi-readelf is needed to read Sc64Frontend field offsets")
    info = subprocess.check_output([readelf, "--debug-dump=info", str(elf)], text=True)
    lines = info.splitlines()
    structure = None
    for index, line in enumerate(lines):
        if "DW_TAG_structure_type" not in line:
            continue
        if any("Sc64Frontend" in candidate for candidate in lines[index + 1:index + 5]):
            structure = index
            break
    if structure is None:
        raise RuntimeError(f"{elf}: DWARF has no Sc64Frontend structure")
    fields: dict[str, int] = {}
    member = None
    for line in lines[structure + 1:]:
        depth_match = re.match(r"\s*<(\d+)>\s*<", line)
        if depth_match and int(depth_match.group(1)) <= 1:
            break
        if depth_match and int(depth_match.group(1)) == 2:
            member = None
            if "DW_TAG_member" not in line:
                continue
        name_match = re.search(r"DW_AT_name\s*:\s*(?:\([^)]*\):\s*)?([A-Za-z_][A-Za-z0-9_]*)\s*$", line)
        if name_match and member is None and "DW_TAG_member" in line:
            member = name_match.group(1)
        elif name_match and member is None:
            # Member attribute lines carry no explicit DIE depth in readelf output.
            # The active depth-2 DW_TAG_member bounds this attribute sequence.
            member = name_match.group(1)
        if "DW_AT_data_member_location:" in line and member:
            offset_match = re.search(r"DW_AT_data_member_location:\s*(\d+)", line)
            if offset_match:
                fields[member] = int(offset_match.group(1))
                member = None
    required = {"page", "row", "mission", "mission_page", "race", "bots", "map", "bot_races"}
    missing = required - fields.keys()
    if missing:
        raise RuntimeError(f"{elf}: DWARF is missing Sc64Frontend fields: {sorted(missing)}")
    return fields


def state(rsp, sym: dict[str, int]) -> dict:
    front = sym["race_front"]
    return {"screen": i32(rsp, sym["screen"]), "page": i32(rsp, front + FRONT_FIELDS["page"]),
            "row": i32(rsp, front + FRONT_FIELDS["row"]),
            "faction": i32(rsp, front + FRONT_FIELDS.get("faction", 16)),
            "mission": i32(rsp, front + FRONT_FIELDS["mission"]),
            "mission_page": i32(rsp, front + FRONT_FIELDS["mission_page"]),
            "race": i32(rsp, front + FRONT_FIELDS["race"]),
            "bots": i32(rsp, front + FRONT_FIELDS["bots"]),
            "map": i32(rsp, front + FRONT_FIELDS["map"]),
            "bot_races": [i32(rsp, front + FRONT_FIELDS["bot_races"] + 4 * i) for i in range(3)]}


def to_row(rsp, sym, target: int) -> None:
    current = state(rsp, sym)["row"]
    while current < target:
        key(rsp, sym, KEY_DOWN); current = state(rsp, sym)["row"]
    while current > target:
        key(rsp, sym, KEY_UP); current = state(rsp, sym)["row"]
    if current != target:
        raise RuntimeError(f"could not select menu row {target}: {state(rsp, sym)}")


def cycle(rsp, sym, offset: int, wanted: int, choices: int) -> None:
    front = sym["race_front"]
    current = i32(rsp, front + offset)
    for _ in range(choices):
        if current == wanted:
            return
        key(rsp, sym, KEY_RIGHT)
        current = i32(rsp, front + offset)
    raise RuntimeError(f"field at +{offset} did not reach {wanted}: got {current}")


def press_start(rsp, sym, map_id: int, race: int, bots: int) -> dict:
    initial = state(rsp, sym)
    if (initial["screen"], initial["page"]) != (SCREEN_TITLE, FRONT_HOME):
        raise RuntimeError(f"unexpected HOME state: {initial}")
    key(rsp, sym, KEY_DOWN); key(rsp, sym, KEY_A)
    if state(rsp, sym)["page"] != FRONT_SKIRMISH:
        raise RuntimeError("HOME did not open skirmish setup")
    to_row(rsp, sym, 0); cycle(rsp, sym, FRONT_FIELDS["map"], map_id, 2)
    to_row(rsp, sym, 1); cycle(rsp, sym, FRONT_FIELDS["race"], race, 3)
    to_row(rsp, sym, 2)
    # Opponent count is presented as 1..3 but wraps on its zero-based UI field.
    current = i32(rsp, sym["race_front"] + FRONT_FIELDS["bots"])
    for _ in range(3):
        if current == bots: break
        key(rsp, sym, KEY_RIGHT); current = i32(rsp, sym["race_front"] + FRONT_FIELDS["bots"])
    if current != bots:
        raise RuntimeError(f"opponent count did not reach {bots}")
    bot_races = []
    for index in range(bots):
        wanted = (race + index + 1) % 3
        to_row(rsp, sym, 3 + index)
        cycle(rsp, sym, FRONT_FIELDS["bot_races"] + 4 * index, wanted, 3)
        bot_races.append(wanted)
    to_row(rsp, sym, 3 + bots)
    key(rsp, sym, KEY_A)
    dims = MAP_DIMS[map_id]
    got = {"screen": i32(rsp, sym["screen"]), "mode": i32(rsp, sym["race_mode"]),
           "human": i32(rsp, sym["race_human"]), "bots": i32(rsp, sym["race_bots"]),
           "map": i32(rsp, sym["race_skirmish_map"]),
           "width": i32(rsp, sym["race_cols"]), "height": i32(rsp, sym["race_rows"]),
           "bot_races": [i32(rsp, sym["race_bot_races"] + 4 * i) for i in range(3)]}
    if (got["screen"], got["mode"], got["human"], got["bots"], got["map"],
            (got["width"], got["height"])) != (SCREEN_PLAY, 1, race, bots, map_id, dims):
        raise RuntimeError(f"skirmish launch identity mismatch: {got}")
    if got["bot_races"] != bot_races + [0] * (3 - bots):
        # Values for inactive slots are still preserved; compare only active slots.
        if got["bot_races"][:bots] != bot_races:
            raise RuntimeError(f"bot faction choices changed at launch: {got}")
    return got


def campaign_launch(rsp, sym, profile: str, mission_id: int, ordinal: int,
                    mission_count: int) -> dict:
    key(rsp, sym, KEY_A)  # HOME opens the fixed-faction mission page.
    front = state(rsp, sym)
    if front["page"] != FRONT_MISSION:
        raise RuntimeError(f"{profile}: campaign did not open its mission list: {front}")
    wanted_page = ordinal // MISSIONS_PER_PAGE
    page_count = (mission_count + MISSIONS_PER_PAGE - 1) // MISSIONS_PER_PAGE
    if page_count < 1 or wanted_page >= page_count:
        raise RuntimeError(f"{profile}: invalid campaign ordinal {ordinal}/{mission_count}")
    for _ in range(page_count):
        current_page = state(rsp, sym)["mission_page"]
        if current_page == wanted_page:
            break
        step = KEY_RIGHT if current_page < wanted_page else KEY_LEFT
        key(rsp, sym, step)
        if state(rsp, sym)["mission_page"] == current_page:
            raise RuntimeError(f"{profile}: mission page did not advance: {state(rsp, sym)}")
    if state(rsp, sym)["mission_page"] != wanted_page:
        raise RuntimeError(f"{profile}: could not select mission page {wanted_page}")
    wanted_row = ordinal % MISSIONS_PER_PAGE
    to_row(rsp, sym, wanted_row)
    key(rsp, sym, KEY_A)  # briefing
    briefing = state(rsp, sym)
    if briefing["page"] != FRONT_BRIEFING or briefing["mission"] != mission_id:
        raise RuntimeError(f"{profile}: mission {mission_id} did not open briefing: {briefing}")
    key(rsp, sym, KEY_START)
    got = {"screen": i32(rsp, sym["screen"]), "mode": i32(rsp, sym["race_mode"]),
           "human": i32(rsp, sym["race_human"]),
           "mission": i32(rsp, sym["race_selected_mission"]),
           "width": i32(rsp, sym["race_cols"]), "height": i32(rsp, sym["race_rows"])}
    if (got["screen"], got["mode"], got["human"], got["mission"]) != (
            SCREEN_PLAY, 0, PROFILE_IDS[profile], mission_id):
        raise RuntimeError(f"{profile}: campaign launch mismatch: {got}")
    return got


def t3_state_snapshot(rsp, sym: dict[str, int], t3_size: int,
                      campaign_size: int, unit_layout: tuple[int, dict[str, int]],
                      building_layout: tuple[int, dict[str, int]]) -> dict:
    unit_size, unit_fields = unit_layout
    building_size, building_fields = building_layout

    def entities(base: int, capacity: int, size: int, fields: dict[str, int]) -> list[dict]:
        result = []
        raw = rsp.read(base, capacity * size)
        for slot in range(capacity):
            offset = slot * size
            if struct.unpack_from("<i", raw, offset + fields["alive"])[0] == 0:
                continue
            result.append({"slot": slot, "state": raw[offset:offset + size]})
        return result

    snapshot = {
        "screen": i32(rsp, sym["screen"]),
        "frame": i32(rsp, sym["frame"]),
        "mission": i32(rsp, sym["race_selected_mission"]),
        "minerals": i32(rsp, sym["minerals"]),
        "gas": i32(rsp, sym["gas"]),
        "t3": rsp.read(sym["race_t3"], t3_size),
        "campaign": rsp.read(sym["race_campaign"], campaign_size),
        "kills": int.from_bytes(rsp.read(sym["race_t3_buildings_killed"], 2), "little"),
        "units": entities(sym["u"], 160, unit_size, unit_fields),
        "buildings": entities(sym["b"], 64, building_size, building_fields),
    }
    for name in ("race_minerals", "race_gas"):
        if name in sym:
            snapshot[name] = rsp.read(sym[name], 5 * 4)
    return snapshot


def t3_fresh_load_smoke(rom: pathlib.Path, elf: pathlib.Path, out: pathlib.Path,
                        sym: dict[str, int], ordinal: int, mission_count: int) -> dict:
    """Pause, save, reload in a fresh mGBA process, and resume T3 normally."""
    needed = ("race_t3", "race_campaign", "race_t3_buildings_killed", "u", "b",
              "screen", "frame", "race_selected_mission", "minerals", "gas")
    missing = [name for name in needed if name not in sym]
    if missing:
        raise RuntimeError(f"T3 fresh-load smoke requires ELF symbols: {', '.join(missing)}")
    t3_size, _ = dwarf_struct(elf, "Sc64T3State")
    campaign_size, _ = dwarf_struct(elf, "Sc64RaceCampaignState")
    unit_layout = dwarf_struct(elf, "Unit")
    building_layout = dwarf_struct(elf, "Building")

    name = "campaign-6-pause-save"
    running = start_emulator(name, rom, out, sym)
    proc, sock, rsp, log = running
    source_dir = out / "saves" / name
    try:
        launched = campaign_launch(rsp, sym, "terran", 6, ordinal, mission_count)
        rsp.tick(1000)
        key(rsp, sym, KEY_START)
        paused = t3_state_snapshot(rsp, sym, t3_size, campaign_size, unit_layout, building_layout)
        if paused["screen"] != SCREEN_PAUSE:
            raise RuntimeError(f"T3 did not pause: screen={paused['screen']}")
        if paused["frame"] < 1000:
            raise RuntimeError(f"T3 did not advance before pause: frame={paused['frame']}")
        rsp.tick(120)
        still_paused = t3_state_snapshot(rsp, sym, t3_size, campaign_size, unit_layout, building_layout)
        if still_paused != paused:
            raise RuntimeError("T3 timer, campaign model, resources, or roster advanced while paused")
        key(rsp, sym, KEY_B)  # Pause menu B saves to the real mGBA SRAM backend.
    finally:
        menu.stop_emulator(proc, sock, log)

    saves = list(source_dir.glob("*.sav"))
    if len(saves) != 1 or saves[0].stat().st_size != 32768:
        raise RuntimeError(f"T3 save did not flush one 32 KiB SRAM file: {saves}")
    fresh_name = "campaign-6-fresh-load"
    fresh_save_dir = out / "saves" / fresh_name
    fresh_save_dir.mkdir(parents=True, exist_ok=True)
    shutil.copy2(saves[0], fresh_save_dir / saves[0].name)
    fresh = start_emulator(fresh_name, rom, out, sym)
    proc, sock, rsp, log = fresh
    try:
        # Keys are sampled before the game_update breakpoint: two continues
        # execute the sampled neutral call, then the L load. Observe before a
        # third continue can run a live tick. Only controller input is written.
        rsp.write(sym["demo_inject_keys"], struct.pack("<H", KEY_L))
        rsp.tick(2)
        rsp.write(sym["demo_inject_keys"], b"\0\0")
        restored = t3_state_snapshot(rsp, sym, t3_size, campaign_size, unit_layout, building_layout)
        if restored["screen"] != SCREEN_PLAY:
            raise RuntimeError(f"T3 title-screen load did not resume live play: {restored['screen']}")
        comparable = {key: value for key, value in paused.items() if key != "screen"}
        loaded_comparable = {key: value for key, value in restored.items() if key != "screen"}
        if loaded_comparable != comparable:
            raise RuntimeError("fresh T3 SRAM load did not restore the paused campaign state")
        before_tick = restored["frame"]
        rsp.tick(10)
        resumed = t3_state_snapshot(rsp, sym, t3_size, campaign_size, unit_layout, building_layout)
        if resumed["screen"] != SCREEN_PLAY or resumed["frame"] <= before_tick:
            raise RuntimeError("T3 did not resume simulation after the fresh SRAM load")
        return {"paused_frame": paused["frame"], "loaded_frame": restored["frame"],
                "resumed_frame": resumed["frame"], "kills": restored["kills"],
                "launched": launched, "fresh_load_verified": True}
    finally:
        menu.stop_emulator(proc, sock, log)


def run_one(profile: str, rom: pathlib.Path, elf: pathlib.Path,
            map_id: int | None, race: int | None, bots: int | None, ticks: int,
            output: pathlib.Path, mission_id: int | None = None) -> dict:
    if not rom.is_file() or not elf.is_file():
        raise RuntimeError(f"{profile}: build both ROM and ELF first ({rom}, {elf})")
    global FRONT_FIELDS
    FRONT_FIELDS = frontend_offsets(elf)
    campaign = read_campaign_metadata(rom, elf, profile)
    enabled_mission_ids = campaign["enabled_mission_ids"]
    selected_mission = mission_id
    if not enabled_mission_ids:
        raise RuntimeError(f"{profile}: ELF catalog has no enabled campaign missions")
    if selected_mission is not None:
        if selected_mission not in enabled_mission_ids:
            raise RuntimeError(f"{profile}: campaign mission {mission_id} is not enabled; "
                               f"enabled missions are {enabled_mission_ids}")
        missions_to_run = [selected_mission]
    else:
        missions_to_run = enabled_mission_ids
    sym = menu.symbols(elf)
    required = ("game_update", "race_front", "demo_inject_keys", "screen", "race_mode",
                "race_human", "race_bots", "race_selected_mission", "race_skirmish_map",
                "race_bot_races", "race_cols", "race_rows")
    missing = [name for name in required if name not in sym]
    if missing:
        raise RuntimeError(f"{profile}: ELF is missing symbols: {', '.join(missing)}")
    out = output / profile
    out.mkdir(parents=True, exist_ok=True)
    selected_maps = [] if selected_mission is not None else ([map_id] if map_id is not None else range(2))
    selected_races = [race] if race is not None else range(3)
    selected_bots = [bots] if bots is not None else range(1, 4)
    records = []
    for current_mission in missions_to_run:
        ordinal = enabled_mission_ids.index(current_mission)
        name = f"campaign-{current_mission}"
        running = start_emulator(name, rom, out, sym)
        proc, sock, rsp, log = running
        try:
            got = campaign_launch(rsp, sym, profile, current_mission, ordinal,
                                  len(enabled_mission_ids))
            records.append({"kind": "campaign", "ordinal": ordinal,
                            "mission_id": current_mission, **got})
            print(f"{profile} campaign mission={current_mission} ordinal={ordinal}: ok", flush=True)
        finally:
            menu.stop_emulator(proc, sock, log)
        if current_mission == 6 and profile == "terran" and selected_mission == 6:
            records[-1]["pause_save_fresh_load"] = t3_fresh_load_smoke(
                rom, elf, out, sym, ordinal, len(enabled_mission_ids))
    for mid in selected_maps:
        for human in selected_races:
            for count in selected_bots:
                name = f"map-{mid}-human-{human}-ai-{count}"
                running = start_emulator(name, rom, out, sym)
                proc, sock, rsp, log = running
                try:
                    got = press_start(rsp, sym, mid, human, count)
                    # One independent SRAM reload for each map covers map and AI-race persistence.
                    if human == selected_races[0] and count == selected_bots[-1]:
                        key(rsp, sym, KEY_START); key(rsp, sym, KEY_B)
                        menu.stop_emulator(proc, sock, log)
                        proc = sock = log = None
                        source_saves = list((out / "saves" / name).glob("*.sav"))
                        if not source_saves:
                            raise RuntimeError(f"{name}: mGBA did not flush a fresh save file")
                        load_name = name + "-fresh-load"
                        load_dir = out / "saves" / load_name
                        load_dir.mkdir(parents=True, exist_ok=True)
                        (load_dir / source_saves[0].name).write_bytes(source_saves[0].read_bytes())
                        fresh = start_emulator(load_name, rom, out, sym)
                        proc, sock, rsp, log = fresh
                        key(rsp, sym, KEY_L)
                        loaded = {"screen": i32(rsp, sym["screen"]),
                                  "mode": i32(rsp, sym["race_mode"]),
                                  "human": i32(rsp, sym["race_human"]),
                                  "bots": i32(rsp, sym["race_bots"]),
                                  "map": i32(rsp, sym["race_skirmish_map"]),
                                  "bot_races": [i32(rsp, sym["race_bot_races"] + 4 * i)
                                                for i in range(3)]}
                        if (loaded["screen"], loaded["mode"], loaded["human"], loaded["bots"],
                                loaded["map"], loaded["bot_races"][:count]) != (
                                SCREEN_PLAY, 1, human, count, mid, got["bot_races"][:count]):
                            raise RuntimeError(f"fresh save reload mismatch: {loaded}")
                        got["fresh_load"] = loaded
                    if ticks:
                        # Advance only by normal emulated updates; report bank and economy snapshots.
                        rsp.tick(ticks)
                        got["ticks"] = ticks
                        got["frame"] = i32(rsp, sym["frame"])
                        got["mineral_bank"] = i32(rsp, sym["minerals"])
                        if "race_minerals" in sym:
                            got["bot_minerals"] = [i32(rsp, sym["race_minerals"] + 4 * owner)
                                                   for owner in (1, 3, 4)]
                        if "race_gas" in sym:
                            got["bot_gas"] = [i32(rsp, sym["race_gas"] + 4 * owner)
                                              for owner in (1, 3, 4)]
                    records.append({"kind": "skirmish", **got})
                    print(f"{profile} skirmish map={mid} human={human} bots={count}: ok",
                          flush=True)
                finally:
                    if proc is not None:
                        menu.stop_emulator(proc, sock, log)
    return {"profile": profile, "campaign_catalog": campaign,
            "rom_sha256": hashlib.sha256(rom.read_bytes()).hexdigest(),
            "elf_sha256": hashlib.sha256(elf.read_bytes()).hexdigest(), "runs": records}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--profile", choices=("all", "terran", "zerg", "protoss", "protoss-p6"), default="all")
    parser.add_argument("--map", type=int, choices=range(2))
    parser.add_argument("--race", type=int, choices=range(3))
    parser.add_argument("--bots", type=int, choices=range(1, 4))
    parser.add_argument("--mission", type=int,
                        help="launch only this enabled campaign mission global ID")
    parser.add_argument("--rom", type=pathlib.Path,
                        help="override ROM path (requires one explicit --profile)")
    parser.add_argument("--elf", type=pathlib.Path,
                        help="override matching ELF path (requires one explicit --profile)")
    parser.add_argument("--ticks", type=int, default=0,
                        help="neutral-input emulated game updates per selected skirmish (for example 10000)")
    parser.add_argument("--out", type=pathlib.Path, default=ROOT / "build/emulator/sc64-split")
    args = parser.parse_args()
    if args.ticks < 0:
        parser.error("--ticks cannot be negative")
    profiles = ("terran", "zerg", "protoss") if args.profile == "all" else (args.profile,)
    if (args.rom or args.elf) and args.profile == "all":
        parser.error("--rom and --elf overrides require one explicit --profile")
    if args.mission is not None and args.profile == "all":
        matching = []
        for candidate in profiles:
            rom_name, elf_name = PROFILE_BUILD_PATHS[candidate]
            candidate_rom = ROOT / "build" / rom_name
            candidate_elf = ROOT / "build" / elf_name
            if not candidate_rom.is_file() or not candidate_elf.is_file():
                continue
            if args.mission in read_campaign_metadata(candidate_rom, candidate_elf,
                                                       candidate)["enabled_mission_ids"]:
                matching.append(candidate)
        if not matching:
            parser.error(f"campaign mission {args.mission} is not enabled in any built profile")
        profiles = tuple(matching)
    args.out.mkdir(parents=True, exist_ok=True)
    run_out = args.out / f"run-{time.time_ns()}"
    run_out.mkdir(parents=False, exist_ok=False)
    results = []
    for profile in profiles:
        rom_name, elf_name = PROFILE_BUILD_PATHS[profile]
        rom = args.rom or ROOT / "build" / rom_name
        elf = args.elf or ROOT / "build" / elf_name
        results.append(run_one(profile, rom, elf, args.map, args.race,
                               args.bots, args.ticks, run_out, args.mission))
    result = {"profiles": results}
    path = run_out / "sc64-split-smoke.json"
    path.write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
