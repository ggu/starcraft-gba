#!/usr/bin/env python3
"""Run the advisor alert ARM probe in a fresh mGBA process, without a debugger.

The probe needs the private audio bank and stays under ignored build/. HOME is
isolated so mGBA's own config and recent list stay untouched. Emulator evidence
only.
"""
from __future__ import annotations

import argparse
import os
import pathlib
import pty
import select
import subprocess
import tempfile
import time

from emulator_smoke import ROOT, require_mgba


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("rom", type=pathlib.Path)
    parser.add_argument("--timeout", type=float, default=240)
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(dir=ROOT / "build") as home:
        config = pathlib.Path(home) / ".config" / "mgba"
        config.mkdir(parents=True)
        (config / "config.ini").write_text("[ports.qt]\nlogToStdout=1\nlogLevel=7\n")
        env = dict(os.environ, HOME=home, XDG_CONFIG_HOME=str(pathlib.Path(home) / ".config"))
        master, slave = pty.openpty()
        proc = subprocess.Popen(
            [require_mgba(), "-l", "7", "-C", "mute=1", "-C", "audioSync=0", "-C", "videoSync=0",
             "-C", f"savegamePath={home}", str(args.rom.resolve())],
            stdin=subprocess.DEVNULL, stdout=slave, stderr=slave, env=env, cwd=home)
        os.close(slave)
        captured = bytearray()
        deadline = time.monotonic() + args.timeout
        try:
            while b"ALERT PROBE done" not in captured and time.monotonic() < deadline:
                if select.select([master], [], [], 1)[0]:
                    try:
                        captured.extend(os.read(master, 65536))
                    except OSError:
                        break
            time.sleep(0.2)
        finally:
            proc.terminate()
            try:
                proc.wait(timeout=5)
            except subprocess.TimeoutExpired:
                proc.kill()
                proc.wait()
            os.close(master)
    lines = [line[line.index("ALERT "):].rstrip()
             for line in captured.decode(errors="replace").splitlines() if "ALERT " in line]
    print("\n".join(lines))
    if not lines or "failures=0 " not in lines[-1]:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
