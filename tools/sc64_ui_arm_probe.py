#!/usr/bin/env python3
"""Run the no-GDB interface ARM probe in two fresh mGBA processes.

Each process gets its own HOME (so mGBA's config stays isolated), a shared
private save directory under the output folder, and debug logging to stdout.
No debugger port is opened and no running emulator is touched. The first
process saves from the pause list; the second loads that SRAM from HOME and
must report the same state digest. Emulator evidence only.
"""
from __future__ import annotations

import argparse
import json
import os
import pathlib
import pty
import re
import select
import subprocess
import time

from emulator_smoke import ROOT, require_mgba

ROMS = {
    "showcase": ROOT / "build" / "StarCraft-Showcase-UI-Probe-GBA.gba",
    "races": ROOT / "build" / "StarCraft64-Races-UI-Probe-GBA.gba",
}


def run_process(mgba: str, rom: pathlib.Path, out: pathlib.Path, name: str,
                timeout: float) -> list[str]:
    home = out / "home"
    saves = out / "saves"
    home.mkdir(parents=True, exist_ok=True)
    saves.mkdir(parents=True, exist_ok=True)
    log_path = out / f"{name}.log"
    config = home / ".config" / "mgba"
    config.mkdir(parents=True, exist_ok=True)
    # The Qt frontend reads its log routing from its own section.
    (config / "config.ini").write_text("[ports.qt]\nlogToStdout=1\nlogLevel=127\n")
    env = dict(os.environ, HOME=str(home), XDG_CONFIG_HOME=str(home / ".config"))
    # A pseudo-terminal keeps mGBA's stdout line-buffered, so log lines
    # survive closing the process.
    master, slave = pty.openpty()
    proc = subprocess.Popen(
        [mgba, "-l", "127", "-C", "mute=1", "-C", "audioSync=0", "-C", "videoSync=0",
         "-C", f"savegamePath={saves}", "-C", f"savestatePath={saves}",
         "-C", f"screenshotPath={out}", str(rom)],
        stdin=subprocess.DEVNULL, stdout=slave, stderr=slave, env=env, cwd=out)
    os.close(slave)
    captured = bytearray()

    def drain(seconds: float) -> None:
        end = time.monotonic() + seconds
        while time.monotonic() < end:
            ready, _, _ = select.select([master], [], [], 0.2)
            if ready:
                try:
                    chunk = os.read(master, 65536)
                except OSError:
                    return
                if not chunk:
                    return
                captured.extend(chunk)
            elif b"UI DONE" in captured:
                return

    try:
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline and b"UI DONE" not in captured and proc.poll() is None:
            drain(1)
        # Let mGBA write back the SRAM it marked dirty before closing.
        drain(4)
    finally:
        proc.terminate()
        try:
            proc.wait(timeout=5)
        except subprocess.TimeoutExpired:
            proc.kill()
            proc.wait()
        os.close(master)
    log_path.write_bytes(bytes(captured))
    text = captured.decode(errors="replace")
    return [line[line.index("UI "):] for line in text.splitlines() if "UI " in line]


def digest(lines: list[str], label: str) -> str | None:
    for line in lines:
        match = re.match(rf"UI STATE {label} digest=([0-9a-f]+)", line)
        if match:
            return match.group(1)
    return None


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("profile", choices=sorted(ROMS))
    parser.add_argument("--out", type=pathlib.Path)
    parser.add_argument("--timeout", type=float, default=240)
    args = parser.parse_args()
    rom = ROMS[args.profile]
    if not rom.is_file():
        raise SystemExit(f"build {rom.relative_to(ROOT)} first (make ui-arm-probe)")
    out = (args.out or ROOT / "build" / "emulator" / "ui-probe" / args.profile).resolve()
    if (out / "saves").exists() and any((out / "saves").iterdir()):
        raise SystemExit(f"{out / 'saves'} is not empty; choose a fresh --out")
    mgba = require_mgba()
    first = run_process(mgba, rom, out, "first", args.timeout)
    sram = sorted(p.name for p in (out / "saves").iterdir())
    second = run_process(mgba, rom, out, "second", args.timeout)
    result = {
        "profile": args.profile,
        "first_process": first,
        "save_files": sram,
        "second_process": second,
    }
    saved, reloaded, loaded = digest(first, "saved"), digest(first, "reloaded"), digest(second, "loaded")
    failures = [line for line in first + second if line.startswith("UI FAIL")]
    ok = (bool(first) and first[-1] == "UI DONE ok" and bool(second) and
          second[-1] == "UI DONE ok" and "UI PROCESS first" in first and
          "UI PROCESS second" in second and saved and saved == reloaded == loaded and
          not failures)
    result["digests"] = {"saved": saved, "reloaded": reloaded, "loaded": loaded}
    result["ok"] = bool(ok)
    (out / "result.json").write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps(result, indent=2))
    if not ok:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
