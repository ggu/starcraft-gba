#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Run one no-GDB probe ROM in a fresh mGBA process.

Usage: run_probe.py <rom> <out-dir> <timeout-seconds>. mGBA gets its own HOME,
save directory and log mask 7 (decimal: fatal|error|warn); no debugger port is
opened and no other emulator is touched. Debug-register lines go to probe.log.
"""
import os, pty, select, subprocess, sys, time
from pathlib import Path

root = Path(subprocess.run(["git", "rev-parse", "--show-toplevel"], cwd=Path(__file__).parent,
                           capture_output=True, text=True, check=True).stdout.strip())
sys.path.insert(0, str(root / "tools"))
from emulator_smoke import require_mgba

rom, out, timeout = Path(sys.argv[1]).resolve(), Path(sys.argv[2]).resolve(), float(sys.argv[3])
home = out / "home"; saves = out / "saves"
config = home / ".config" / "mgba"
config.mkdir(parents=True, exist_ok=True); saves.mkdir(parents=True, exist_ok=True)
(config / "config.ini").write_text("[ports.qt]\nlogToStdout=1\nlogLevel=7\n")
env = dict(os.environ, HOME=str(home), XDG_CONFIG_HOME=str(home / ".config"))
master, slave = pty.openpty()
began = time.monotonic()
proc = subprocess.Popen(
    [require_mgba(), "-l", "7", "-C", "mute=1", "-C", "audioSync=0", "-C", "videoSync=0",
     "-C", f"savegamePath={saves}", "-C", f"savestatePath={saves}", "-C", f"screenshotPath={out}", str(rom)],
    stdin=subprocess.DEVNULL, stdout=slave, stderr=slave, env=env, cwd=out)
os.close(slave)
captured = bytearray(); tail = b""
try:
    while time.monotonic() - began < timeout and proc.poll() is None:
        ready, _, _ = select.select([master], [], [], 1.0)
        if not ready:
            continue
        try:
            chunk = os.read(master, 65536)
        except OSError:
            break
        if not chunk:
            break
        captured.extend(chunk)
        if b"PROBE DONE" in tail + chunk:
            break
        tail = chunk[-64:]
finally:
    proc.terminate()
    try:
        proc.wait(timeout=5)
    except subprocess.TimeoutExpired:
        proc.kill(); proc.wait()
    os.close(master)
lines = [l[l.index("GBA Debug: ") + 11:] for l in captured.decode(errors="replace").splitlines() if "GBA Debug: " in l]
(out / "probe.log").write_text("\n".join(lines) + "\n")
print(f"{rom.name}: {len(lines)} lines, {time.monotonic() - began:.0f}s, done={'PROBE DONE' in lines}")
