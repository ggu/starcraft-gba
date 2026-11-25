#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Run the audio ARM probe ROM in the headless mGBA runner and judge its result.

usage: sc64_audio_arm_probe.py ROM ELF RUNNER HOST_OUT

ROM and ELF are build/StarCraft-Audio-Probe-GBA.gba and build/audio-arm-probe.elf
(tests/audio_codec_arm_probe.c). RUNNER is the port-free mGBA runner the other ARM
checks use. HOST_OUT is the output of build/test-audio-bank for the same bank; its
"linear-predictor sample sum" line is the host decode of every stream.

The ROM is run to audio_probe_done and audio_probe_result is read from memory. The
check passes when the ROM reports no difference between its hand-written routines
and their C references and its running sum of all decoded samples equals the host's.
Game memory is read, never written. Emulator evidence only."""
import os
import re
import select
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

WORDS = ("status", "streams", "linear", "samples", "sum_low", "sum_high", "decoder_bad", "mixer_bad")
PASS, RUN_SECONDS = 1, 1800


def host_sum(text: str) -> tuple[int, int]:
    """The two sum words test-audio-bank printed."""
    match = re.search(r"^audio bank: linear-predictor sample sum ([0-9a-f]{8}) ([0-9a-f]{8})$", text, re.M)
    if not match:
        raise ValueError("the host output has no linear-predictor sample sum")
    return int(match.group(1), 16), int(match.group(2), 16)


def verdict(result: dict[str, int], host: tuple[int, int]) -> list[str]:
    """Reasons the probe result fails; empty when it passes."""
    reasons = []
    if result["status"] != PASS:
        reasons.append(f"the ROM reported status {result['status']}")
    if not result["streams"] or result["linear"] != result["streams"]:
        reasons.append(f"{result['linear']} of {result['streams']} streams ran through the hand-written decoder")
    if result["decoder_bad"]:
        reasons.append(f"{result['decoder_bad']} decoded frames differ from the C reference")
    if result["mixer_bad"]:
        reasons.append(f"{result['mixer_bad']} voice-sum tables differ from the C reference")
    if (result["sum_low"], result["sum_high"]) != host:
        reasons.append("the decoded sample sum %08x %08x differs from the host's %08x %08x" %
                       (result["sum_low"], result["sum_high"], *host))
    return reasons


def symbols(elf: Path) -> dict[str, int]:
    prefix = os.environ.get("PREFIX", "arm-none-eabi-")
    out = subprocess.run([prefix + "nm", str(elf)], text=True, stdout=subprocess.PIPE, check=True).stdout
    return {parts[2]: int(parts[0], 16) for parts in map(str.split, out.splitlines()) if len(parts) == 3}


def run(rom: Path, elf: Path, runner: Path) -> dict[str, int]:
    sym = symbols(elf)
    for name in ("audio_probe_done", "audio_probe_result"):
        if name not in sym:
            raise RuntimeError(f"{elf} has no {name}")
    with tempfile.TemporaryDirectory(prefix="audio-arm-probe-") as home:
        env = dict(os.environ, HOME=home, XDG_CONFIG_HOME=home + "/.config", TMPDIR=home)
        proc = subprocess.Popen([str(runner), str(rom), "--physical"], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                                stderr=subprocess.DEVNULL, text=True, bufsize=1, env=env)

        def command(text: str, seconds: int = 60) -> str:
            proc.stdin.write(text + "\n")
            proc.stdin.flush()
            if not select.select([proc.stdout], [], [], seconds)[0]:
                raise RuntimeError(f"the runner did not answer {text!r} in {seconds} s")
            return proc.stdout.readline().rstrip("\n")

        try:
            if not select.select([proc.stdout], [], [], 60)[0] or proc.stdout.readline().strip() != "READY":
                raise RuntimeError("the runner did not start")
            done = sym["audio_probe_done"] & ~1
            if command(f"Z{done:x}") != "OK":
                raise RuntimeError("the runner refused the breakpoint")
            stop = command("c", RUN_SECONDS)
            if stop != f"STOP {done:08x}":
                raise RuntimeError(f"the ROM did not reach audio_probe_done: {stop}")
            data = bytes.fromhex(command(f"m{sym['audio_probe_result']:x},{4 * len(WORDS)}"))
        finally:
            proc.kill()
            proc.wait()
    if len(data) != 4 * len(WORDS):
        raise RuntimeError("the runner returned a short read")
    return dict(zip(WORDS, struct.unpack(f"<{len(WORDS)}I", data)))


def main(argv: list[str]) -> int:
    if len(argv) != 5:
        print(__doc__, file=sys.stderr)
        return 2
    rom, elf, runner, host_out = map(Path, argv[1:])
    host = host_sum(host_out.read_text(encoding="utf-8"))
    result = run(rom, elf, runner)
    print("audio arm probe: {streams} streams, {samples} samples, sum {sum_low:08x} {sum_high:08x}, "
          "{decoder_bad} decoder and {mixer_bad} voice-sum differences".format(**result))
    reasons = verdict(result, host)
    for reason in reasons:
        print(f"FAIL audio arm probe: {reason}", file=sys.stderr)
    if not reasons:
        print("audio arm probe: the hand-written routines match their C references and the host decode")
    return 1 if reasons else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
