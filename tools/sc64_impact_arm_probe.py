#!/usr/bin/env python3
"""Run the no-GDB projectile battle timing probe in one fresh mGBA process.

The process gets its own HOME so mGBA's configuration stays isolated, logs
only warnings and errors (the probe reports at warning level), and opens no
debugger port. It touches no running emulator. Cycle counts come from the
ROM's own cascaded timer under emulation; they are not hardware results.
"""
# SPDX-License-Identifier: GPL-3.0-or-later
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


def run(mgba: str, rom: pathlib.Path, out: pathlib.Path, timeout: float) -> list[str]:
    home = out / "home"
    saves = out / "saves"
    config = home / ".config" / "mgba"
    config.mkdir(parents=True, exist_ok=True)
    saves.mkdir(parents=True, exist_ok=True)
    (config / "config.ini").write_text("[ports.qt]\nlogToStdout=1\nlogLevel=7\n")
    env = dict(os.environ, HOME=str(home), XDG_CONFIG_HOME=str(home / ".config"))
    master, slave = pty.openpty()
    proc = subprocess.Popen(
        [mgba, "-l", "7", "-C", "mute=1", "-C", "audioSync=0", "-C", "videoSync=0",
         "-C", f"savegamePath={saves}", "-C", f"savestatePath={saves}", str(rom)],
        stdin=subprocess.DEVNULL, stdout=slave, stderr=slave, env=env, cwd=out)
    os.close(slave)
    captured = bytearray()
    try:
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline and b"IMPACT DONE" not in captured \
                and proc.poll() is None:
            ready, _, _ = select.select([master], [], [], 0.5)
            if ready:
                try:
                    chunk = os.read(master, 65536)
                except OSError:
                    break
                if not chunk:
                    break
                captured.extend(chunk)
    finally:
        proc.terminate()
        try:
            proc.wait(timeout=5)
        except subprocess.TimeoutExpired:
            proc.kill()
            proc.wait()
        os.close(master)
    (out / "mgba.log").write_bytes(bytes(captured))
    text = captured.decode(errors="replace")
    return [line[line.index("IMPACT "):].strip() for line in text.splitlines()
            if "IMPACT " in line]


def fields(lines: list[str], tag: str) -> dict[str, str]:
    for line in lines:
        if line.startswith(f"IMPACT {tag} "):
            return dict(re.findall(r"(\w+)=(\S+)", line))
    return {}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("rom", type=pathlib.Path)
    parser.add_argument("--out", type=pathlib.Path, required=True)
    parser.add_argument("--timeout", type=float, default=600)
    args = parser.parse_args()
    rom = args.rom.resolve()
    if not rom.is_file():
        raise SystemExit(f"missing probe ROM {rom}")
    out = args.out.resolve()
    if out.exists() and any(out.iterdir()):
        raise SystemExit(f"{out} is not empty; choose a fresh --out")
    out.mkdir(parents=True, exist_ok=True)
    lines = run(require_mgba(), rom, out, args.timeout)
    result = {"rom": str(rom), "lines": lines,
              "timing": fields(lines, "TIMING"), "world": fields(lines, "WORLD"),
              "ok": "IMPACT DONE" in lines}
    (out / "result.json").write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps(result, indent=2))
    if not result["ok"]:
        raise SystemExit(1)


if __name__ == "__main__":
    os.chdir(ROOT)
    main()
