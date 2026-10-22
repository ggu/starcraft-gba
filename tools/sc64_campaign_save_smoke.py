#!/usr/bin/env python3
"""Verify a completed Terran Campaign save restores its terminal Backwater result in a fresh mGBA process."""
import argparse
import hashlib
import json
import pathlib
import shutil
import socket
import struct
import subprocess
import time

import emulator_smoke

ROOT = pathlib.Path(__file__).resolve().parents[1]
DEFAULT_SNAPSHOT = ROOT / "build" / "emulator" / "sc64-campaign-full" / "completed.sav"
SAVE_NAME = "StarCraft64-Terran-Campaign-GBA.sav"
PORT = 2345
U_RAYNOR = 6
SIDE = 0
SCREEN_TITLE = 0
SCREEN_RESULT = 3


def read_i32(rsp, address):
    return struct.unpack("<i", rsp.read(address, 4))[0]


def raynor_is_alive_on_side(rsp, unit_address, unit_bytes, unit_count):
    for slot in range(unit_count):
        raw = rsp.read(unit_address + slot * unit_bytes, unit_bytes)
        values = struct.unpack("<22i", raw)
        if values[4] == SIDE and values[5] == U_RAYNOR and values[6]:
            return True
    return False


def check_port_free():
    probe = socket.socket()
    try:
        probe.bind(("127.0.0.1", PORT))
    except OSError as exc:
        raise RuntimeError(f"Port {PORT} is already in use; leave the existing debugger untouched") from exc
    finally:
        probe.close()


def run(snapshot_path, output_dir):
    rom = ROOT / "build" / "StarCraft64-Terran-Campaign-GBA.gba"
    elf = ROOT / "build" / "starcraft64-campaign.elf"
    if not rom.is_file() or not elf.is_file():
        raise RuntimeError("Campaign ROM and ELF must already be built")
    if not snapshot_path.is_file():
        raise RuntimeError(f"Completed campaign SRAM snapshot not found: {snapshot_path}")
    snapshot = snapshot_path.read_bytes()
    if len(snapshot) != 32768:
        raise RuntimeError(f"Expected 32768-byte SRAM snapshot, found {len(snapshot)} bytes")

    sym = emulator_smoke.symbols(elf)
    sizes = emulator_smoke.symbol_sizes(elf)
    required = ("game_update", "demo_inject_keys", "screen", "campaign_mission",
                "campaign_completed", "frame", "won", "lost", "u")
    missing = [name for name in required if name not in sym]
    if missing:
        raise RuntimeError("Campaign ELF is missing symbols: " + ", ".join(missing))
    unit_bytes = sizes.get("u", 0) // 96
    if unit_bytes != 88:
        raise RuntimeError(f"Unexpected campaign unit-bank layout: {sizes.get('u', 0)} bytes")

    output_dir.mkdir(parents=True, exist_ok=True)
    save_dir = output_dir / "saves"
    save_dir.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(snapshot_path, save_dir / SAVE_NAME)
    log_handle = (output_dir / "mGBA.log").open("ab")
    proc = None
    sock = None
    try:
        check_port_free()
        proc = subprocess.Popen([
            str(emulator_smoke.MGBA), "-g", "-C", "mute=1", "-C", "audioSync=0",
            "-C", "videoSync=0", "-C", f"savegamePath={save_dir}", str(rom),
        ], stdout=log_handle, stderr=log_handle)
        deadline = time.monotonic() + 12
        while time.monotonic() < deadline:
            try:
                sock = socket.create_connection(("127.0.0.1", PORT), timeout=1)
                break
            except OSError:
                if proc.poll() is not None:
                    raise RuntimeError("mGBA exited before its GDB server became available")
                time.sleep(0.1)
        if sock is None:
            raise RuntimeError("mGBA GDB server did not start")
        sock.settimeout(20)
        sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        rsp = emulator_smoke.RSP(sock)
        rsp.cmd("qSupported")
        rsp.cmd("?")
        update_address = sym["game_update"] & ~1
        if rsp.cmd(f"Z1,{update_address:x},2") != "OK":
            raise RuntimeError("Unable to set game_update breakpoint")

        # The first game_update entry is after game_init attempted to load SRAM.
        rsp.tick(2)
        title = emulator_smoke.capture(rsp, output_dir / "campaign-title.png")
        mission = read_i32(rsp, sym["campaign_mission"])
        completed = read_i32(rsp, sym["campaign_completed"])
        screen = read_i32(rsp, sym["screen"])
        if (mission, completed, screen) != (1, 3, SCREEN_TITLE):
            raise RuntimeError(
                f"Fresh startup did not restore completed progress: mission={mission}, "
                f"completed={completed}, screen={screen}"
            )

        # Load the persisted world from the title screen with B, then release it.
        rsp.write(sym["demo_inject_keys"], struct.pack("<H", 2))
        rsp.tick()
        rsp.write(sym["demo_inject_keys"], b"\0\0")
        rsp.tick(3)
        final_screen = read_i32(rsp, sym["screen"])
        final_won = read_i32(rsp, sym["won"])
        final_lost = read_i32(rsp, sym["lost"])
        final_frame = read_i32(rsp, sym["frame"])
        raynor_alive = raynor_is_alive_on_side(rsp, sym["u"], unit_bytes, 96)
        result_image = emulator_smoke.capture(rsp, output_dir / "campaign-result.png")
        if (final_screen, final_won, final_lost, raynor_alive) != (SCREEN_RESULT, 1, 0, True):
            raise RuntimeError(
                f"Completed Backwater load failed: screen={final_screen}, won={final_won}, "
                f"lost={final_lost}, friendly_Raynor={raynor_alive}"
            )

        result = {
            "rom_sha256": hashlib.sha256(rom.read_bytes()).hexdigest(),
            "snapshot_sha256": hashlib.sha256(snapshot).hexdigest(),
            "snapshot_bytes": len(snapshot),
            "startup": {"campaign_mission": mission, "campaign_completed": completed,
                        "screen": screen, "capture": title},
            "loaded_result": {"screen": final_screen, "won": final_won, "lost": final_lost,
                              "frame": final_frame, "raynor_alive_side": raynor_alive,
                              "capture": result_image},
            "source_profile": {"unit_bank_bytes": sizes["u"], "unit_capacity": unit_bytes and sizes["u"] // unit_bytes},
            "input": "B once, release, then three updates",
        }
        (output_dir / "campaign-save-smoke.json").write_text(json.dumps(result, indent=2) + "\n")
        print(json.dumps(result, indent=2))
    finally:
        if sock:
            sock.close()
        if proc:
            proc.terminate()
            try:
                proc.wait(timeout=3)
            except subprocess.TimeoutExpired:
                proc.kill()
                proc.wait()
        log_handle.close()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--snapshot-path", type=pathlib.Path, default=DEFAULT_SNAPSHOT,
                        help=f"32768-byte SRAM image (default: {DEFAULT_SNAPSHOT})")
    parser.add_argument("--output-dir", type=pathlib.Path,
                        default=ROOT / "build" / "emulator" / "sc64-campaign-save-smoke")
    args = parser.parse_args()
    run(args.snapshot_path, args.output_dir)


if __name__ == "__main__":
    main()
