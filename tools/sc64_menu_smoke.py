#!/usr/bin/env python3
"""Exercise the Races title menus and captures on the ARM ROM in mGBA."""
from __future__ import annotations

import hashlib
import json
import pathlib
import socket
import struct
import subprocess
import time

from emulator_smoke import MGBA, ROOT, RSP, capture, symbols

KEY_A, KEY_B, KEY_RIGHT, KEY_DOWN = 1, 2, 16, 128
SCREEN_TITLE, SCREEN_PLAY = 0, 1
FRONT_HOME, FRONT_CAMPAIGN, FRONT_MISSION, FRONT_BRIEFING, FRONT_SKIRMISH = range(5)


def u32(rsp: RSP, address: int) -> int:
    return int.from_bytes(rsp.read(address, 4), "little")


def world_palette_restored(rsp: RSP, sym: dict[str, int]) -> bool:
    source = rsp.read(sym["sc64_palette"] + 32 * 2, 224 * 2)
    actual = rsp.read(0x05000000 + 32 * 2, 224 * 2)
    # Independent fixture for the LCD lift, so comparing against the ROM
    # palette cannot accidentally bless a raw palette installation.
    curve = (0, 3, 5, 6, 7, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
             20, 20, 21, 22, 23, 24, 24, 25, 26, 27, 27, 28, 29, 30, 30, 31)
    expected = bytearray()
    for offset in range(0, len(source), 2):
        color = int.from_bytes(source[offset:offset + 2], "little")
        lifted = (curve[color & 31] | (curve[(color >> 5) & 31] << 5) |
                  (curve[(color >> 10) & 31] << 10))
        expected.extend(lifted.to_bytes(2, "little"))
    return bytes(expected) != source and bytes(expected) == actual


def start_emulator(name: str, rom: pathlib.Path, out: pathlib.Path,
                   sym: dict[str, int]):
    save_dir = out / "saves" / name
    save_dir.mkdir(parents=True, exist_ok=True)
    log = (out / f"{name}.log").open("ab")
    proc = subprocess.Popen(
        [str(MGBA), "-g", "-C", "mute=1", "-C", "audioSync=0", "-C",
         "videoSync=0", "-C", f"savegamePath={save_dir}", str(rom)],
        stdout=log, stderr=log)
    sock = None
    try:
        deadline = time.monotonic() + 12
        while time.monotonic() < deadline:
            try:
                sock = socket.create_connection(("127.0.0.1", 2345), timeout=1)
                break
            except OSError:
                time.sleep(0.1)
        if sock is None:
            raise RuntimeError("mGBA GDB server did not start")
        sock.settimeout(20)
        sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        rsp = RSP(sock)
        rsp.cmd("qSupported")
        rsp.cmd("?")
        update = sym["game_update"] & ~1
        if rsp.cmd(f"Z1,{update:x},2") != "OK":
            raise RuntimeError("failed to set game_update breakpoint")
        rsp.tick(2)
        return proc, sock, rsp, log
    except BaseException:
        if sock:
            sock.close()
        proc.terminate()
        proc.wait(timeout=3)
        log.close()
        raise


def stop_emulator(proc, sock, log) -> None:
    sock.close()
    proc.terminate()
    try:
        proc.wait(timeout=3)
    except subprocess.TimeoutExpired:
        proc.kill()
        proc.wait()
    log.close()


def run_scenario(name: str, custom: bool, rom: pathlib.Path,
                 sym: dict[str, int], out: pathlib.Path) -> list[dict]:
    proc, sock, rsp, log = start_emulator(name, rom, out, sym)
    observations = []
    try:
        front = sym["race_front"]
        inject = sym["demo_inject_keys"]

        def state() -> tuple[int, int, int, int, int, int]:
            return (u32(rsp, sym["screen"]), u32(rsp, front + 8),
                    u32(rsp, front + 12), u32(rsp, sym["race_mode"]),
                    u32(rsp, sym["race_human"]), u32(rsp, sym["race_selected_mission"]))

        def expect(screen: int, page: int | None, where: str) -> None:
            got = state()
            if got[0] != screen or (page is not None and got[1] != page):
                raise RuntimeError(f"{where}: expected screen/page {screen}/{page}, got {got}")

        def press(key: int) -> None:
            # main.c samples injected keys before entering game_update, so a
            # two-tick press/release pair accounts for the breakpoint latency.
            rsp.write(inject, struct.pack("<H", key))
            rsp.tick(2)
            rsp.write(inject, b"\0\0")
            rsp.tick(2)

        def capture_page(label: str, page: int) -> None:
            expect(SCREEN_TITLE, page, label)
            rsp.tick(80)  # finish page fade and briefing text before capture
            path = out / f"{name}-{label}.png"
            result = capture(rsp, path)
            observations.append({"screen": label, "page": page,
                                 "path": str(path), **result})

        expect(SCREEN_TITLE, FRONT_HOME, "initial HOME")
        capture_page("home", FRONT_HOME)
        if custom:
            press(KEY_DOWN)
            press(KEY_A)
            capture_page("custom", FRONT_SKIRMISH)
            press(KEY_RIGHT)  # race row: default Zerg → Protoss
            press(KEY_DOWN)  # opponents row
            press(KEY_DOWN)  # launch row
            capture_page("custom-launch", FRONT_SKIRMISH)
            if state()[2] != 2:
                raise RuntimeError(f"custom launch row is not selected: {state()}")
            press(KEY_A)
            got = state()
            if got[0] != SCREEN_PLAY or got[3] != 1 or got[4] != 2:
                raise RuntimeError(f"custom game did not launch as Protoss: {got}")
            if not world_palette_restored(rsp, sym):
                raise RuntimeError("custom launch did not restore the world palette")
            play = capture(rsp, out / f"{name}-play.png")
            if play["palette_indices"] < 4:
                raise RuntimeError("custom launch did not restore the world palette")
            observations.append({"screen": "play", "path": str(out / f"{name}-play.png"), **play})
        else:
            press(KEY_A)
            capture_page("campaign", FRONT_CAMPAIGN)
            press(KEY_A)
            capture_page("mission", FRONT_MISSION)
            press(KEY_A)
            capture_page("briefing", FRONT_BRIEFING)
            press(KEY_A)
            got = state()
            if got[0] != SCREEN_PLAY or got[3] != 0 or got[5] != 0:
                raise RuntimeError(f"Z1 campaign did not launch from the briefing: {got}")
            if not world_palette_restored(rsp, sym):
                raise RuntimeError("campaign launch did not restore the world palette")
            play = capture(rsp, out / f"{name}-play.png")
            if play["palette_indices"] < 4:
                raise RuntimeError("campaign launch did not restore the world palette")
            observations.append({"screen": "play", "path": str(out / f"{name}-play.png"), **play})
    finally:
        stop_emulator(proc, sock, log)
    return observations


def main() -> None:
    rom = ROOT / "build" / "StarCraft64-Races-GBA.gba"
    elf = ROOT / "build" / "starcraft64-races.elf"
    if not rom.is_file() or not elf.is_file():
        raise RuntimeError("Build StarCraft64 Races ARM ROM and ELF first")
    check = socket.socket()
    try:
        check.bind(("127.0.0.1", 2345))
    except OSError as exc:
        raise RuntimeError("Port 2345 is already in use; leave the existing debugger untouched") from exc
    finally:
        check.close()
    sym = symbols(elf)
    for name in ("game_update", "race_front", "demo_inject_keys", "screen",
                 "race_mode", "race_human", "race_selected_mission", "sc64_palette"):
        if name not in sym:
            raise RuntimeError(f"Races ELF is missing symbol {name}")
    out = ROOT / "build" / "emulator" / "sc64-menu"
    out.mkdir(parents=True, exist_ok=True)
    observations = []
    observations.extend(run_scenario("campaign", False, rom, sym, out))
    observations.extend(run_scenario("custom", True, rom, sym, out))
    result = {"rom_sha256": hashlib.sha256(rom.read_bytes()).hexdigest(),
              "captures": observations}
    (out / "sc64-menu.json").write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
