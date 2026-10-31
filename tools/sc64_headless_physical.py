# SPDX-License-Identifier: GPL-3.0-or-later
"""Physical-button transport for a normal ROM; never writes guest memory."""
from __future__ import annotations

import os
from pathlib import Path
import select
import subprocess

from sc64_headless_replay import HeadlessReplay


class PhysicalReplay(HeadlessReplay):
    """Stop before each input sample; counters measure completed main loops."""

    def __init__(self, proc, keys_pc: int, counter_address: int):
        super().__init__(proc)
        self.keys_pc = keys_pc
        self.counter_address = counter_address
        self.completed = 0
        self.held = 0

    def set_keys(self, mask: int) -> None:
        if (type(mask) is not int or not 0 <= mask <= 1023 or
                mask & 48 == 48 or mask & 192 == 192):
            raise ValueError("invalid or opposing physical button mask")
        if self.cmd(f"K{mask:x}") != "OK":
            raise RuntimeError("physical button input rejected")
        self.held = mask

    def _advance(self, count: int) -> None:
        while count:
            n = min(count, 256)
            fields = self.cmd(f"U{n},{self.keys_pc:x},{self.counter_address:x}").split()
            if len(fields) != 4 or fields[0] != "STEPS":
                raise RuntimeError("invalid physical update response")
            try:
                steps, counter, clock = map(int, fields[1:])
            except ValueError as exc:
                raise RuntimeError("invalid physical update response") from exc
            if steps != n or counter != self.completed + n or clock < 0:
                raise RuntimeError("physical update counter mismatch")
            self.completed = counter
            count -= n

    def play_to(self, end: int, all_events: list[tuple[int, int]]) -> None:
        """Play transitions with zero-based tick strictly below end."""
        if type(end) is not int or end < self.completed:
            raise ValueError("physical page end precedes current update")
        last_tick = -1
        prior_mask = None
        for tick, mask in all_events:
            if (type(tick) is not int or tick < 0 or tick <= last_tick or
                    type(mask) is not int or not 0 <= mask <= 1023 or
                    mask & 48 == 48 or mask & 192 == 192):
                raise ValueError("invalid physical transition page")
            last_tick = tick
            if tick < self.completed:
                prior_mask = mask
                continue
            if prior_mask is not None and prior_mask != self.held:
                raise ValueError("consumed transitions disagree with held keys")
            prior_mask = None
            if tick >= end:
                break
            self._advance(tick - self.completed)
            self.set_keys(mask)
        if prior_mask is not None and prior_mask != self.held:
            raise ValueError("consumed transitions disagree with held keys")
        self._advance(end - self.completed)

    def play_page(self, page: dict) -> None:
        first, end, events = page.get("first"), page.get("end"), page.get("events")
        if (type(first) is not int or type(end) is not int or
                not isinstance(events, list) or first > self.completed or
                self.completed > end or any(tick < first or tick > end for tick, _ in events)):
            raise ValueError("invalid physical page bounds")
        self.play_to(end, events)

    @classmethod
    def start(cls, runner: Path, rom: Path, out: Path, symbols: dict,
              sizes: dict, *, save_input: Path | None = None,
              save_output: Path | None = None):
        return start(runner, rom, out, symbols, sizes,
                     save_input=save_input, save_output=save_output)


def start(runner: Path, rom: Path, out: Path, symbols: dict, sizes: dict, *,
          save_input: Path | None = None, save_output: Path | None = None):
    keys_pc = symbols.get("gba_keys", 0) & ~1
    counter = symbols.get("demo_frame", 0)
    if (not 0x08000000 <= keys_pc < 0x0a000000 or sizes.get("demo_frame") != 4 or
            counter & 3 or not (0x02000000 <= counter <= 0x0203fffc or
                                0x03000000 <= counter <= 0x03007ffc)):
        raise ValueError("invalid input-sample or completed-update symbol")
    save_args = []
    if save_input is not None:
        if not save_input.is_file() or save_input.stat().st_size != 32768:
            raise ValueError("SRAM input must be an existing 32768-byte image")
        save_args += ["--save-input", str(save_input.resolve())]
    if save_output is not None:
        if save_output.exists() or not save_output.parent.is_dir():
            raise ValueError("SRAM output must be a new file in an existing directory")
        save_args += ["--save-output", str(save_output.resolve())]
    home = (out / "home").resolve()
    home.mkdir(parents=True, exist_ok=True)
    env = os.environ.copy()
    env.update(HOME=str(home), XDG_CONFIG_HOME=str(home / ".config"))
    log = (out / "headless.log").open("w")
    proc = subprocess.Popen([str(runner.resolve()), str(rom.resolve()), "--physical", *save_args],
                            stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=log,
                            text=True, bufsize=1, env=env)
    try:
        if not select.select([proc.stdout], [], [], 60)[0] or proc.stdout.readline().rstrip("\n") != "READY":
            raise RuntimeError("physical backend failed to start")
        replay = PhysicalReplay(proc, keys_pc, counter)
        replay.breakpoint(keys_pc, True)
        replay.tick(expected=keys_pc)
        if int.from_bytes(replay.read(counter, 4), "little") != 0:
            raise RuntimeError("first input sample is not at completed update zero")
        return proc, replay, log
    except BaseException:
        proc.terminate()
        try:
            proc.wait(timeout=3)
        except subprocess.TimeoutExpired:
            proc.kill(); proc.wait()
        log.close()
        raise
