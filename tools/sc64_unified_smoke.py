#!/usr/bin/env python3
"""Smoke-test all six unified campaign missions and three-faction custom games in mGBA."""
from __future__ import annotations

import hashlib
import json
import pathlib
import socket
import struct
import argparse

import sc64_menu_smoke as menu
from emulator_smoke import symbol_sizes

ROOT = menu.ROOT
ROM = ROOT / "build" / "StarCraft64-GBA.gba"
ELF = ROOT / "build" / "starcraft64-unified.elf"
OUT = ROOT / "build" / "emulator" / "sc64-unified"

KEY_A, KEY_B, KEY_START, KEY_RIGHT, KEY_DOWN = 1, 2, 8, 16, 128
SCREEN_TITLE, SCREEN_PLAY, SCREEN_PAUSE = 0, 1, 2
FRONT_HOME, FRONT_CAMPAIGN, FRONT_MISSION, FRONT_BRIEFING, FRONT_SKIRMISH = range(5)
R_TERRAN, R_ZERG, R_PROTOSS = range(3)
MISSION_FACTION = (R_ZERG, R_ZERG, R_PROTOSS, R_PROTOSS, R_TERRAN, R_TERRAN)
MISSION_DIMS = ((64, 64), (96, 96), (96, 96), (96, 64), (64, 64), (64, 64))
FACTION_ROWS = {R_TERRAN: 0, R_ZERG: 1, R_PROTOSS: 2}


def i32(rsp: menu.RSP, address: int) -> int:
    return struct.unpack("<i", rsp.read(address, 4))[0]


def u16(rsp: menu.RSP, address: int) -> int:
    return struct.unpack("<H", rsp.read(address, 2))[0]


def ensure_port_free() -> None:
    probe = socket.socket()
    try:
        probe.bind(("127.0.0.1", 2345))
    except OSError as exc:
        raise RuntimeError("Port 2345 is already in use; leave the existing debugger untouched") from exc
    finally:
        probe.close()


def controller(rsp: menu.RSP, inject: int, key: int, ticks: int = 2) -> None:
    rsp.write(inject, struct.pack("<H", key))
    rsp.tick(ticks)
    rsp.write(inject, b"\0\0")
    rsp.tick(2)


def save_records(rsp: menu.RSP) -> list[dict]:
    records = []
    for offset in (0, 16384):
        tag, size, checksum, sequence = struct.unpack("<4I", rsp.read(0x0E000000 + offset, 16))
        if not tag or not 0 < size <= 16368 or not sequence:
            continue
        h = 2166136261
        for value in rsp.read(0x0E000000 + offset + 16, size):
            h = ((h ^ value) * 16777619) & 0xFFFFFFFF
        if h == checksum:
            records.append({"tag": hex(tag), "bytes": size, "sequence": sequence})
    return records


def front_state(rsp: menu.RSP, sym: dict[str, int]) -> dict[str, int]:
    front = sym["race_front"]
    return {
        "screen": i32(rsp, sym["screen"]), "page": i32(rsp, front + 8),
        "row": i32(rsp, front + 12), "faction": i32(rsp, front + 16),
        "mission": i32(rsp, front + 20), "race": i32(rsp, front + 24),
        "bots": i32(rsp, front + 28),
    }


def start(name: str, sym: dict[str, int]):
    ensure_port_free()
    return menu.start_emulator(name, ROM, OUT, sym)


def stop(running) -> None:
    proc, sock, _rsp, log = running
    menu.stop_emulator(proc, sock, log)


def campaign_scenario(mission: int, sym: dict[str, int]) -> dict:
    name = f"mission-{mission}"
    running = start(name, sym)
    proc, sock, rsp, log = running
    inject = sym["demo_inject_keys"]
    screenshots = []
    try:
        initial = front_state(rsp, sym)
        if (initial["screen"], initial["page"], initial["mission"]) != (SCREEN_TITLE, FRONT_HOME, 4):
            raise RuntimeError(f"{name}: unexpected unified default state: {initial}")
        controller(rsp, inject, KEY_A)
        if mission == 0:
            rsp.tick(80)
            path = OUT / "campaign-selector.png"
            screenshots.append({"kind": "campaign-selector", "path": str(path), **menu.capture(rsp, path)})
        faction = MISSION_FACTION[mission]
        for _ in range(FACTION_ROWS[faction]):
            controller(rsp, inject, KEY_DOWN)
        campaign = front_state(rsp, sym)
        if (campaign["page"], campaign["row"]) != (FRONT_CAMPAIGN, FACTION_ROWS[faction]):
            raise RuntimeError(f"{name}: failed to select campaign row {FACTION_ROWS[faction]}: {campaign}")
        controller(rsp, inject, KEY_A)
        selected = front_state(rsp, sym)
        if (selected["page"], selected["faction"]) != (FRONT_MISSION, faction):
            raise RuntimeError(f"{name}: selected campaign did not open its mission list: {selected}")
        local_mission = mission - (4 if faction == R_TERRAN else 2 if faction == R_PROTOSS else 0)
        if local_mission:
            controller(rsp, inject, KEY_DOWN)
        selected = front_state(rsp, sym)
        if (selected["page"], selected["row"]) != (FRONT_MISSION, local_mission):
            raise RuntimeError(f"{name}: mission row mismatch: {selected}")
        controller(rsp, inject, KEY_A)
        briefing = front_state(rsp, sym)
        if (briefing["page"], briefing["mission"]) != (FRONT_BRIEFING, mission):
            raise RuntimeError(f"{name}: wrong briefing selected: {briefing}")
        rsp.tick(80)
        path = OUT / f"mission-{mission}-briefing.png"
        screenshots.append({"kind": "briefing", "mission": mission, "path": str(path), **menu.capture(rsp, path)})
        controller(rsp, inject, KEY_A)
        play = read_play_identity(rsp, sym, mission, race_mode=0)
        if not menu.world_palette_restored(rsp, sym):
            raise RuntimeError(f"{name}: campaign launch did not restore the world palette")
        path = OUT / f"mission-{mission}-play.png"
        screenshots.append({"kind": "play", "mission": mission, "path": str(path), **menu.capture(rsp, path)})
        play["palette_restored"] = True
        play["capture"] = path.as_posix()

        backwater_before = None
        records = None
        if mission in (4, 5):
            # Let the authored trigger state advance, then persist it through the pause menu.
            rsp.tick(120)
            size = symbol_sizes(ELF).get("backwater", 0)
            if not size:
                raise RuntimeError("unified ELF has no Backwater state symbol")
            backwater_before = rsp.read(sym["backwater"], size)
            controller(rsp, inject, KEY_START)
            if front_state(rsp, sym)["screen"] != SCREEN_PAUSE:
                raise RuntimeError(f"T{mission - 3}: START did not open pause")
            controller(rsp, inject, KEY_B)
            if front_state(rsp, sym)["screen"] != SCREEN_PAUSE:
                raise RuntimeError(f"T{mission - 3}: B did not save from pause")
            records = save_records(rsp)
            if not records:
                raise RuntimeError(f"T{mission - 3}: no valid SRAM save record after B")
        result = {"mission": mission, "faction": faction, "play": play,
                  "screenshots": screenshots, "save_records": records}
    finally:
        stop(running)

    if mission in (4, 5):
        # Save files are flushed when mGBA exits; load in an independent process with the same save directory.
        load_name = f"mission-{mission}-load"
        source = OUT / "saves" / name
        destination = OUT / "saves" / load_name
        sav_files = list(source.glob("*.sav"))
        if not sav_files:
            raise RuntimeError(f"T{mission - 3}: mGBA did not flush its SRAM save file")
        destination.mkdir(parents=True, exist_ok=True)
        (destination / sav_files[0].name).write_bytes(sav_files[0].read_bytes())
        running = start(load_name, sym)
        proc, sock, rsp, log = running
        try:
            rsp.write(sym["demo_inject_keys"], struct.pack("<H", KEY_B))
            rsp.tick(2)  # account for sampled-input latency; stop before a resumed play update
            loaded = read_play_identity(rsp, sym, mission, race_mode=0)
            loaded_state = rsp.read(sym["backwater"], len(backwater_before))
            if loaded_state != backwater_before:
                raise RuntimeError(f"T{mission - 3}: Backwater trigger state changed across save/load")
            if not menu.world_palette_restored(rsp, sym):
                raise RuntimeError(f"T{mission - 3}: loaded campaign did not restore the world palette")
            path = OUT / f"mission-{mission}-loaded.png"
            menu.capture(rsp, path)
            result["load"] = {"play": loaded, "backwater_state_hex": loaded_state.hex(),
                              "palette_restored": True, "capture": str(path)}
            rsp.write(sym["demo_inject_keys"], b"\0\0")
        finally:
            stop(running)
    return result


def read_play_identity(rsp: menu.RSP, sym: dict[str, int], mission: int,
                       race_mode: int) -> dict:
    screen = i32(rsp, sym["screen"])
    got_mission = i32(rsp, sym["race_selected_mission"])
    human = i32(rsp, sym["race_human"])
    mode = i32(rsp, sym["race_mode"])
    cols, rows = i32(rsp, sym["race_cols"]), i32(rsp, sym["race_rows"])
    camera_x, camera_y = i32(rsp, sym["cx"]), i32(rsp, sym["cy"])
    campaign_mission = rsp.read(sym["race_campaign"] + 18, 1)[0]
    expected_dims = MISSION_DIMS[mission]
    expected_human = MISSION_FACTION[mission]
    if (screen, mode, got_mission, human, campaign_mission, (cols, rows)) != (
            SCREEN_PLAY, race_mode, mission, expected_human, mission, expected_dims):
        raise RuntimeError(
            f"mission {mission}: loaded identity mismatch: screen/mode/mission/human/campaign/dims="
            f"{(screen, mode, got_mission, human, campaign_mission, (cols, rows))}")
    if not (120 <= camera_x <= cols * 16 - 120 and
            54 <= camera_y <= rows * 16 - 54):
        raise RuntimeError(f"mission {mission}: camera is outside the selected map: {(camera_x, camera_y)}")
    return {"screen": screen, "race_mode": mode, "mission": got_mission,
            "human_owner_race": human, "campaign_mission": campaign_mission,
            "map_width": cols, "map_height": rows,
            "camera_x": camera_x, "camera_y": camera_y}


def skirmish_scenario(race: int, bots: int, sym: dict[str, int]) -> dict:
    name = f"skirmish-r{race}-b{bots}"
    running = start(name, sym)
    proc, sock, rsp, log = running
    inject = sym["demo_inject_keys"]
    try:
        controller(rsp, inject, KEY_DOWN)
        controller(rsp, inject, KEY_A)
        state = front_state(rsp, sym)
        if (state["page"], state["row"]) != (FRONT_SKIRMISH, 0):
            raise RuntimeError(f"{name}: failed to open custom game: {state}")
        while state["race"] != race:
            controller(rsp, inject, KEY_RIGHT)
            state = front_state(rsp, sym)
        controller(rsp, inject, KEY_DOWN)
        state = front_state(rsp, sym)
        while state["bots"] != bots:
            controller(rsp, inject, KEY_RIGHT)
            state = front_state(rsp, sym)
        controller(rsp, inject, KEY_DOWN)
        if front_state(rsp, sym)["row"] != 2:
            raise RuntimeError(f"{name}: failed to select Launch")
        if race == 0 and bots == 3:
            rsp.tick(80)
            path = OUT / "custom-game-selector.png"
            menu.capture(rsp, path)
        controller(rsp, inject, KEY_A)
        screen = i32(rsp, sym["screen"])
        mode = i32(rsp, sym["race_mode"])
        human = i32(rsp, sym["race_human"])
        selected_bots = i32(rsp, sym["race_bots"])
        cols, rows = i32(rsp, sym["race_cols"]), i32(rsp, sym["race_rows"])
        if (screen, mode, human, selected_bots, (cols, rows)) != (SCREEN_PLAY, 1, race, bots, (64, 64)):
            raise RuntimeError(f"{name}: custom game identity mismatch: "
                               f"{(screen, mode, human, selected_bots, (cols, rows))}")
        if not menu.world_palette_restored(rsp, sym):
            raise RuntimeError(f"{name}: custom launch did not restore the world palette")
        path = OUT / f"{name}-play.png"
        image = menu.capture(rsp, path) if bots == 3 else None
        return {"race": race, "bots": bots, "screen": screen, "race_mode": mode,
                "human_owner_race": human, "map_width": cols, "map_height": rows,
                "palette_restored": True, "capture": str(path) if image else None,
                "image": image}
    finally:
        stop(running)


def main() -> None:
    argparse.ArgumentParser(description=__doc__).parse_args()
    if not ROM.is_file() or not ELF.is_file():
        raise RuntimeError("Build the unified StarCraft64 ROM and ELF first")
    ensure_port_free()
    sym = menu.symbols(ELF)
    required = ("game_update", "race_front", "demo_inject_keys", "screen", "race_mode",
                "race_human", "race_bots", "race_selected_mission", "race_cols", "race_rows",
                "race_campaign", "backwater", "cx", "cy", "sc64_palette")
    missing = [name for name in required if name not in sym]
    if missing:
        raise RuntimeError("Unified ELF is missing symbols: " + ", ".join(missing))
    OUT.mkdir(parents=True, exist_ok=True)
    missions = [campaign_scenario(mission, sym) for mission in range(6)]
    skirmishes = [skirmish_scenario(race, bots, sym)
                  for race in range(3) for bots in range(1, 4)]
    result = {"rom_sha256": hashlib.sha256(ROM.read_bytes()).hexdigest(),
              "elf_sha256": hashlib.sha256(ELF.read_bytes()).hexdigest(),
              "mission_ids": {"Z1": 0, "Z2": 1, "P1": 2, "P2": 3, "T1": 4, "T2": 5},
              "missions": missions, "skirmishes": skirmishes}
    (OUT / "sc64-unified-smoke.json").write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
