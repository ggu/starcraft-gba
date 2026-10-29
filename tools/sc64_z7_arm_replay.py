#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Input-only replay of the ordinary-input Z7 controller trace, without a debugger.

`tape TRACE OUT` validates a host trace from build/test-sc64-z7-canonical-controller
and writes the key-transition and checkpoint header that
tests/sc64_z7_arm_replay.c embeds. `compare TRACE LOG` checks every replay
observation (ARM debug log or host stdout) against the host trace row at the
same host tick. `run` starts one isolated mGBA process (its own HOME, config
and save directory; no debugger port) on a replay ROM, streams the probe's
debug log, optionally seeds its SRAM, and reports the comparison, guest
timing and the SRAM records it leaves. Guest timing comes from the GBA timer
cycle counts the probe reports, not from host playback speed. Emulator
evidence only.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
import pathlib
import pty
import re
import select
import shutil
import struct
import subprocess
import sys
import time

TOOLS = pathlib.Path(__file__).resolve().parent
if str(TOOLS) not in sys.path:
    sys.path.insert(0, str(TOOLS))

CPU_HZ = 16_777_216
KEY_MASK = 1023
MISSION = 18
SCREEN_RESULT = 3
SAVE_SLOT_SIZE = 16384
SAVE_BYTES = 32768
SCALARS = ("frame", "screen", "won", "lost", "outcome", "started", "switch10",
           "ai_script_mask", "ai_plan_ticks", "ai_plan_phase", "fault",
           "side_units", "side_buildings", "enemy_units", "enemy_buildings",
           "workers", "minerals", "gas")
LISTS = ("action_cursors", "action_ticks")
OBSERVED = ("start", "checkpoint", "won", "lost", "limit")
MGBA = pathlib.Path.home() / "Applications/mGBA.app/Contents/MacOS/mGBA"


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def load_trace(path: pathlib.Path) -> dict:
    """Strictly validate a Z7 trace; return transitions and observations."""
    raw = path.read_bytes()
    trace = json.loads(raw)
    if not isinstance(trace, dict) or trace.get("schema") != 1 or \
       trace.get("profile") != "zerg-z7" or trace.get("mission") != MISSION:
        raise ValueError("trace is not a schema-1 zerg-z7 controller trace")
    rows = trace.get("events")
    if not isinstance(rows, list) or len(rows) < 2:
        raise ValueError("trace events are missing")
    transitions: list[tuple[int, int]] = []
    observations: dict[int, dict] = {}
    held_before, last_tick, final = 0, 0, None
    for i, row in enumerate(rows):
        kind, tick = row.get("kind"), row.get("host_tick")
        if type(tick) is not int or tick < last_tick:
            raise ValueError(f"event {i} has an invalid or unordered host_tick")
        if final is not None:
            raise ValueError("terminal observation must be last")
        if kind == "input":
            held, pressed = row.get("held"), row.get("pressed")
            if type(held) is not int or held & ~KEY_MASK or held == held_before:
                raise ValueError(f"input event {i} is not a held-key transition")
            if pressed != held & ~held_before:
                raise ValueError(f"input event {i} pressed mask is inconsistent")
            if transitions and tick <= transitions[-1][0]:
                raise ValueError(f"input event {i} repeats a tick")
            if tick < 1 or tick >= 1 << 22:
                raise ValueError(f"input event {i} tick does not fit the tape")
            transitions.append((tick, held))
            held_before = held
        elif kind in OBSERVED:
            for field in SCALARS:
                if type(row.get(field)) is not int:
                    raise ValueError(f"observation {i} lacks integer {field}")
            for field in LISTS:
                values = row.get(field)
                if not isinstance(values, list) or not all(type(v) is int for v in values):
                    raise ValueError(f"observation {i} lacks list {field}")
            prior = observations.get(tick)
            if prior and any(prior[f] != row[f] for f in SCALARS + LISTS):
                raise ValueError(f"same-tick observations disagree at {tick}")
            observations.setdefault(tick, row)
            if kind in ("won", "lost", "limit"):
                final = row
        else:
            raise ValueError(f"event {i} has unknown kind {kind!r}")
        last_tick = tick
    if final is None or not transitions:
        raise ValueError("trace needs inputs and a terminal observation")
    if transitions[-1][0] > final["host_tick"]:
        raise ValueError("input after the terminal observation")
    return {"transitions": transitions, "observations": observations, "final": final,
            "stop": final["host_tick"], "trace_sha256": sha256(raw),
            "policy": trace.get("policy")}


def words_body(words: list[int]) -> str:
    return ",\n".join("    " + ",".join(f"0x{w:08x}u" for w in words[i:i + 8])
                      for i in range(0, len(words), 8))


def write_tape(trace_path: pathlib.Path, out: pathlib.Path) -> dict:
    plan = load_trace(trace_path)
    words = [(tick << 10) | held for tick, held in plan["transitions"]]
    checks = sorted(plan["observations"])
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(
        "/* Generated by tools/sc64_z7_arm_replay.py from a Z7 controller trace.\n"
        f" * Trace SHA-256 {plan['trace_sha256']}.\n"
        " * Tape words are host_tick << 10 | held; checks are host ticks. */\n"
        "#ifndef SC64_Z7_TAPE_GENERATED_H\n#define SC64_Z7_TAPE_GENERATED_H\n"
        f"#define Z7_TAPE_COUNT {len(words)}u\n"
        f"#define Z7_CHECK_COUNT {len(checks)}u\n"
        f"#define Z7_TAPE_STOP_TICK {plan['stop']}u\n"
        f"static const u32 z7_tape[{len(words)}] = {{\n{words_body(words)}\n}};\n"
        f"static const u32 z7_checks[{len(checks)}] = {{\n{words_body(checks)}\n}};\n"
        "#endif\n")
    return plan


def parse_fields(line: str) -> dict:
    fields: dict[str, object] = {}
    for key, value in re.findall(r"(\w+)=(-?\d+(?:,-?\d+)*)", line):
        fields[key] = [int(v) for v in value.split(",")] if key in LISTS else int(value)
    return fields


def probe_lines(text: str) -> list[str]:
    return [l[l.index("Z7 "):].rstrip() for l in text.splitlines() if "Z7 " in l]


def compare(plan: dict, text: str, *, after: int = 0, through: int | None = None) -> dict:
    """Match replay observations with host rows for ticks in (after, through]."""
    lines = probe_lines(text)
    checks: dict[int, dict] = {}
    kinds: dict[int, list[str]] = {}
    for line in lines:
        tag = line.split(" ", 2)[1]
        if tag in ("CHECK", "CHECK2", "CHECK3"):
            row = parse_fields(line)
            tick = row.pop("host_tick")
            if tag == "CHECK":
                kinds.setdefault(tick, []).append(line.split()[2])
            checks.setdefault(tick, {}).update(row)
    through = plan["stop"] if through is None else through
    mismatches, compared = [], 0
    for tick, expected in sorted(plan["observations"].items()):
        if not after < tick <= through:
            continue
        actual = checks.get(tick)
        if actual is None:
            mismatches.append(f"missing replay observation at tick {tick}")
            continue
        compared += 1
        for field in SCALARS + LISTS:
            if expected[field] != actual.get(field):
                mismatches.append(f"tick {tick} {field}: replay={actual.get(field)} host={expected[field]}")
    extra = sorted(t for t in checks if t not in plan["observations"] and
                   "cut" not in kinds.get(t, []))
    mismatches += [f"unexpected replay observation at tick {t}" for t in extra]
    cycles = [parse_fields(l) for l in lines if l.startswith("Z7 CYCLES ")]
    return {"lines": len(lines), "compared_observations": compared,
            "mismatches": mismatches[:50], "mismatch_count": len(mismatches),
            "final": checks.get(through, {}), "final_kinds": kinds.get(through, []),
            "cycles": cycles[-1] if cycles else None,
            "saved": next((l for l in lines if l.startswith("Z7 SAVED")), None),
            "loaded": next((l for l in lines if l.startswith("Z7 LOADED")), None),
            "early_end": any(l.startswith("Z7 EARLY_END") for l in lines),
            "done": any(l.startswith("Z7 DONE") for l in lines)}


def timing(cycles: dict | None) -> dict | None:
    if not cycles or cycles.get("frames", 0) <= 0 or cycles.get("total", 0) <= 0:
        return None
    frames, total = cycles["frames"], cycles["total"]
    return {"frames": frames, "total_cycles": total, "update_cycles": cycles["update"],
            "peak_cycles": cycles["peak"], "update_peak_cycles": cycles["update_peak"],
            "mean_cycles_per_frame": round(total / frames),
            "frames_per_second": round(frames * CPU_HZ / total, 3),
            "stack_untouched_bytes": cycles.get("stack_untouched")}


def save_records(data: bytes) -> list[dict]:
    """The alternating-slot records common/gba.c writes, checksums verified."""
    records = []
    for slot in range(2):
        off = slot * SAVE_SLOT_SIZE
        tag, size, checksum, sequence = struct.unpack_from("<4I", data, off)
        if not sequence or size > SAVE_SLOT_SIZE - 16:
            continue
        payload = data[off + 16:off + 16 + size]
        h = 2166136261
        for byte in payload:
            h = ((h ^ byte) * 16777619) & 0xFFFFFFFF
        records.append({"slot": slot, "tag": tag, "payload_bytes": size, "sequence": sequence,
                        "checksum": checksum, "checksum_valid": h == checksum,
                        "payload_sha256": sha256(payload)})
    return records


def run_mgba(rom: pathlib.Path, out: pathlib.Path, timeout: float,
             seed: pathlib.Path | None, stop_markers: tuple[bytes, ...]) -> tuple[str, float]:
    """One isolated mGBA process; returns its debug log text and wall time."""
    home, saves = out / "home", out / "saves"
    config = home / ".config" / "mgba"
    config.mkdir(parents=True, exist_ok=True)
    saves.mkdir(parents=True, exist_ok=True)
    (config / "config.ini").write_text("[ports.qt]\nlogToStdout=1\nlogLevel=7\n")
    image = out / "replay.gba"
    shutil.copyfile(rom, image)
    if seed is not None:
        shutil.copyfile(seed, saves / "replay.sav")
    env = dict(os.environ, HOME=str(home), XDG_CONFIG_HOME=str(home / ".config"))
    master, slave = pty.openpty()
    began = time.monotonic()
    # Decimal mask 7 = fatal|error|warn: the probe logs at warn, and INFO
    # would add every DMA line.
    proc = subprocess.Popen(
        [str(MGBA), "-l", "7", "-C", "mute=1", "-C", "audioSync=0", "-C", "videoSync=0",
         "-C", f"savegamePath={saves}", "-C", f"savestatePath={saves}",
         "-C", f"screenshotPath={out}", str(image)],
        stdin=subprocess.DEVNULL, stdout=slave, stderr=slave, env=env, cwd=out,
        start_new_session=True)
    os.close(slave)
    (out / "launch.json").write_text(json.dumps({"owned_pid": proc.pid, "rom_sha256": sha256(image.read_bytes()),
                                                  "seed_sha256": sha256(seed.read_bytes()) if seed else None},
                                                 indent=2) + "\n")
    captured = bytearray()
    log = (out / "mgba.log").open("wb")  # streamed, so a cut-off run keeps its evidence
    stopped = False
    try:
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline and proc.poll() is None:
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
            log.write(chunk)
            log.flush()
            tail = bytes(captured[-(len(chunk) + 64):])  # not the whole run
            if any(marker in tail for marker in stop_markers):
                stopped = True
                break
        if stopped:
            time.sleep(4)  # let mGBA write SRAM back before terminating
    finally:
        proc.terminate()
        try:
            proc.wait(timeout=10)
        except subprocess.TimeoutExpired:
            proc.kill()
            proc.wait()
        os.close(master)
        log.close()
        (out / "process-exit.json").write_text(json.dumps({"owned_pid": proc.pid,
                                                           "exit_code": proc.returncode}) + "\n")
    return captured.decode(errors="replace"), time.monotonic() - began


def verdict(plan: dict, mode: str, result: dict) -> bool:
    final, host = result["final"], plan["final"]
    ok = result["mismatch_count"] == 0 and not result["early_end"]
    if mode == "prefix":
        return bool(ok and result["saved"] and result["saved"].startswith("Z7 SAVED ok"))
    if mode == "resume":
        ok = ok and bool(result["loaded"]) and result["loaded"].startswith("Z7 LOADED ok")
    ok = ok and result["done"] and all(final.get(f) == host[f] for f in SCALARS + LISTS)
    if host["kind"] == "won":
        ok = ok and final.get("screen") == SCREEN_RESULT and final.get("won") == 1 and \
            final.get("lost") == 0 and final.get("fault") == 0 and final.get("enemy_buildings") == 0
    return bool(ok)


def evaluate(plan: dict, mode: str, text: str, cut: int | None) -> dict:
    lines = probe_lines(text)
    if mode == "prefix":
        saved = next((parse_fields(l) for l in lines if l.startswith("Z7 SAVED")), {})
        cut = saved.get("cut")
        result = compare(plan, text, through=cut or 0)
        result["cut"] = cut
    elif mode == "resume":
        result = compare(plan, text, after=cut or 0)
        result["cut"] = cut
    else:
        result = compare(plan, text)
    result["guest_timing"] = timing(result.pop("cycles"))
    result["qualified"] = verdict(plan, mode, result)
    return result


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)
    tape = sub.add_parser("tape", help="write the embedded key tape")
    tape.add_argument("trace", type=pathlib.Path)
    tape.add_argument("out", type=pathlib.Path)
    check = sub.add_parser("compare", help="compare a replay log with the trace")
    check.add_argument("trace", type=pathlib.Path)
    check.add_argument("log", type=pathlib.Path)
    check.add_argument("--mode", choices=("full", "prefix", "resume"), default="full")
    check.add_argument("--cut", type=int)
    run = sub.add_parser("run", help="one isolated mGBA replay")
    run.add_argument("trace", type=pathlib.Path)
    run.add_argument("rom", type=pathlib.Path)
    run.add_argument("out", type=pathlib.Path)
    run.add_argument("--mode", choices=("full", "prefix", "resume"), default="full")
    run.add_argument("--cut", type=int, help="resume: the prefix cut tick")
    run.add_argument("--seed-sav", type=pathlib.Path, help="resume: SRAM written by the prefix")
    run.add_argument("--timeout", type=float, default=6 * 3600)
    args = parser.parse_args()
    for name in ("trace", "rom", "out", "log", "seed_sav"):
        if getattr(args, name, None) is not None:
            setattr(args, name, getattr(args, name).resolve())

    if args.command == "tape":
        plan = write_tape(args.trace, args.out)
        print(json.dumps({"tape": str(args.out), "transitions": len(plan["transitions"]),
                          "checks": len(plan["observations"]), "stop": plan["stop"],
                          "terminal": plan["final"]["kind"], "trace_sha256": plan["trace_sha256"]}))
        return 0
    plan = load_trace(args.trace)
    if args.mode == "resume" and args.cut is None:
        parser.error("resume needs --cut")
    if args.command == "compare":
        result = evaluate(plan, args.mode, args.log.read_text(errors="replace"), args.cut)
        print(json.dumps(result, indent=2))
        return 0 if result["qualified"] else 1
    if not MGBA.exists():
        parser.error(f"mGBA not found at {MGBA}")
    if args.out.exists() and any(args.out.iterdir()):
        parser.error(f"{args.out} is not empty; use a fresh output directory")
    args.out.mkdir(parents=True, exist_ok=True)
    markers = (b"Z7 DONE", b"Z7 SAVED", b"Z7 LOADED fail", b"Z7 EARLY_END")
    text, wall = run_mgba(args.rom, args.out, args.timeout, args.seed_sav, markers)
    result = evaluate(plan, args.mode, text, args.cut)
    sav = args.out / "saves" / "replay.sav"
    data = sav.read_bytes() if sav.exists() else b""
    result.update({"mode": args.mode, "trace_sha256": plan["trace_sha256"],
                   "rom_sha256": sha256(args.rom.read_bytes()), "wall_seconds": round(wall, 1),
                   "sram_sha256": sha256(data) if data else None,
                   "sram_records": save_records(data) if len(data) == SAVE_BYTES else []})
    (args.out / "result.json").write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps({k: result[k] for k in ("qualified", "compared_observations", "mismatch_count",
                                             "cut", "guest_timing", "wall_seconds", "sram_records")
                      if k in result}, indent=2))
    return 0 if result["qualified"] else 1


if __name__ == "__main__":
    sys.exit(main())
