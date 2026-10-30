# SPDX-License-Identifier: GPL-3.0-or-later
"""Pipe transport for the pinned port-free mGBA controller replay backend."""
from __future__ import annotations

import os
from pathlib import Path
import select
import subprocess


class HeadlessReplay:
    def __init__(self, proc: subprocess.Popen):
        self.proc = proc

    def cmd(self, command: str) -> str:
        assert self.proc.stdin is not None and self.proc.stdout is not None
        if command.startswith(("Z1,", "z1,")):
            command = command[0] + command.split(",")[1]
        self.proc.stdin.write(command + "\n")
        self.proc.stdin.flush()
        if not select.select([self.proc.stdout], [], [], 60)[0]:
            raise RuntimeError("headless backend exceeded 60-second command timeout")
        result = self.proc.stdout.readline().rstrip("\n")
        if not result or result.startswith("ERR"):
            raise RuntimeError(f"headless backend rejected {command[:32]!r}: {result or 'closed pipe'}")
        return result

    def read(self, address: int, length: int) -> bytes:
        data = b"".join(bytes.fromhex(self.cmd(f"m{address+i:x},{min(512, length-i)}"))
                        for i in range(0, length, 512))
        if len(data) != length:
            raise RuntimeError("headless backend returned an incomplete memory read")
        return data

    def write(self, address: int, data: bytes) -> None:
        for i in range(0, len(data), 512):
            chunk = data[i:i+512]
            if self.cmd(f"M{address+i:x},{len(chunk)}:{chunk.hex()}") != "OK":
                raise RuntimeError("headless backend rejected memory write")

    def tick(self, n: int = 1, *, expected: int | None = None) -> None:
        for _ in range(n):
            response = self.cmd("c")
            if not response.startswith("STOP "):
                raise RuntimeError("headless backend did not stop at a breakpoint")
            if expected is not None and int(response[5:], 16) != expected:
                raise RuntimeError(f"unexpected headless breakpoint {response}, expected {expected:08x}")

    def breakpoint(self, address: int, enabled: bool) -> None:
        if self.cmd(f"{'Z' if enabled else 'z'}{address:x}") != "OK":
            raise RuntimeError("headless backend rejected breakpoint")

    def clock(self) -> int:
        return int(self.cmd("t"))


def start(runner: Path, rom: Path, out: Path, symbols: dict, sizes: dict):
    home = (out / "home").resolve()
    home.mkdir(parents=True, exist_ok=True)
    env = os.environ.copy()
    env.update(HOME=str(home), XDG_CONFIG_HOME=str(home / ".config"))
    names = ("demo_schedule_events", "demo_schedule_count", "demo_schedule_length",
             "demo_schedule_active", "demo_schedule_index")
    ranges = [(symbols[name], sizes[name]) for name in names]
    if any(size <= 0 or address < 0x02000000 or address + size > 0x02040000 for address, size in ranges):
        raise ValueError("scheduler write range is outside EWRAM")
    if any(a < c+d and c < a+b for i, (a,b) in enumerate(ranges) for c,d in ranges[i+1:]):
        raise ValueError("scheduler write ranges overlap")
    args = [part for address, size in ranges for part in (f"{address:x}", str(size))]
    log = (out / "headless.log").open("w")
    proc = subprocess.Popen([str(runner.resolve()), str(rom.resolve()), *args], stdin=subprocess.PIPE,
                            stdout=subprocess.PIPE, stderr=log, text=True, bufsize=1, env=env)
    try:
        assert proc.stdout is not None
        if not select.select([proc.stdout], [], [], 60)[0] or proc.stdout.readline().rstrip("\n") != "READY":
            raise RuntimeError("headless backend failed to start")
        replay = HeadlessReplay(proc)
        replay.breakpoint(symbols["game_update"] & ~1, True)
        replay.tick(2, expected=symbols["game_update"] & ~1)
        return proc, replay, log
    except BaseException:
        proc.terminate()
        try:
            proc.wait(timeout=3)
        except subprocess.TimeoutExpired:
            proc.kill()
            proc.wait()
        log.close()
        raise
