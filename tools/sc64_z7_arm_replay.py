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


RECORD_FIELDS = {
    "CHECK": SCALARS[:11], "CHECK2": LISTS, "CHECK3": SCALARS[11:],
    "CYCLES": ("frames", "total", "update", "peak", "update_peak"),
}
CHECK_KINDS = ("checkpoint", "cut", "won", "lost", "limit")
CYCLE_KINDS = ("periodic", "cut", "terminal")
KNOWN_TAGS = ("START", "CHECK", "CHECK2", "CHECK3", "CYCLES", "SAVED", "LOADED",
              "EARLY_END", "DONE")
SAVE_TAG = 0x53435A54  # generation tag the Z7 candidate writes ("TZCS")
SAVE_PAYLOAD_BYTES = 16296


def parse_fields(line: str) -> dict:
    fields: dict[str, object] = {}
    for key, value in re.findall(r"(\w+)=(-?\d+(?:,-?\d+)*)", line):
        if key in fields:
            raise ValueError(f"field {key} repeats in {line!r}")
        fields[key] = [int(v) for v in value.split(",")] if key in LISTS else int(value)
    return fields


def probe_lines(text: str) -> list[str]:
    return [l[l.index("Z7 "):].rstrip() for l in text.splitlines() if "Z7 " in l]


def parse_records(text: str) -> tuple[dict, list[str]]:
    """Group probe records; malformed, unknown or conflicting rows are errors."""
    errors: list[str] = []
    checks: dict[int, dict] = {}
    kinds: dict[int, list[str]] = {}
    cycles: list[dict] = []
    singles: dict[str, list[str]] = {t: [] for t in ("START", "SAVED", "LOADED", "EARLY_END", "DONE")}
    lines = probe_lines(text)
    for line in lines:
        parts = line.split()
        tag = parts[1] if len(parts) > 1 else ""
        if tag not in KNOWN_TAGS:
            errors.append(f"unknown record {line!r}")
            continue
        if tag in singles:
            singles[tag].append(line)
            continue
        try:
            row = parse_fields(line)
        except ValueError as exc:
            errors.append(str(exc))
            continue
        kind = parts[2] if tag in ("CHECK", "CYCLES") and len(parts) > 2 else None
        missing = [f for f in ("host_tick",) + RECORD_FIELDS[tag] if f not in row]
        if missing or (tag == "CHECK" and kind not in CHECK_KINDS) or \
                (tag == "CYCLES" and kind not in CYCLE_KINDS) or \
                (tag == "CHECK2" and any(len(row[f]) != 8 for f in LISTS)):
            errors.append(f"malformed record {line!r}")
            continue
        tick = row.pop("host_tick")
        if tag == "CYCLES":
            cycles.append(dict(row, kind=kind, host_tick=tick))
            continue
        if tag == "CHECK":
            kinds.setdefault(tick, []).append(kind)
        merged = checks.setdefault(tick, {})
        for field, value in row.items():
            if field in merged and merged[field] != value:
                errors.append(f"conflicting {field} at tick {tick}")
            merged[field] = value
    for tick, row in checks.items():
        missing = [f for f in SCALARS + LISTS if f not in row]
        if missing:
            errors.append(f"tick {tick} lacks {','.join(missing)}")
        seen = kinds.get(tick, [])
        if len(seen) != len(set(seen)):
            errors.append(f"duplicate observation kind at tick {tick}")
    return {"lines": len(lines), "checks": checks, "kinds": kinds, "cycles": cycles,
            "singles": singles}, errors


def compare(plan: dict, records: dict, *, after: int, through: int) -> tuple[int, list[str]]:
    """Match every host observation in (after, through] exactly once."""
    mismatches, compared = [], 0
    checks, kinds = records["checks"], records["kinds"]
    for tick, expected in sorted(plan["observations"].items()):
        if not after < tick <= through:
            continue
        actual = checks.get(tick)
        if actual is None or "checkpoint" not in kinds.get(tick, []):
            mismatches.append(f"missing replay observation at tick {tick}")
            continue
        compared += 1
        for field in SCALARS + LISTS:
            if expected[field] != actual.get(field):
                mismatches.append(f"tick {tick} {field}: replay={actual.get(field)} host={expected[field]}")
    for tick in sorted(checks):
        if tick in plan["observations"] and after < tick <= through:
            continue
        if kinds.get(tick) == ["cut"] and tick == through:
            continue
        mismatches.append(f"unexpected replay observation at tick {tick}")
    return compared, mismatches


def timing(records: dict, final_kind: str, final_tick: int, frames: int) -> tuple[dict | None, list[str]]:
    """Exactly one coherent final cycles record, plus coherent periodic ones."""
    errors: list[str] = []
    finals = [c for c in records["cycles"] if c["kind"] == final_kind]
    if len(finals) != 1:
        return None, [f"expected one {final_kind} timing record, found {len(finals)}"]
    final = finals[0]
    for c in records["cycles"]:
        if c["kind"] not in ("periodic", final_kind):
            errors.append(f"unexpected {c['kind']} timing record")
            continue
        if min(c["frames"], c["total"], c["update"], c["peak"], c["update_peak"]) <= 0 or \
                c["update"] > c["total"] or c["update_peak"] > c["peak"] or \
                c["peak"] > c["total"] or c["total"] > c["peak"] * c["frames"] or \
                c["frames"] > final["frames"] or c["total"] > final["total"]:
            errors.append(f"incoherent timing record at tick {c['host_tick']}")
        if "stack_painted" in c and not 0 < c.get("stack_untouched", -1) <= c["stack_painted"]:
            errors.append(f"incoherent stack record at tick {c['host_tick']}")
    if final["host_tick"] != final_tick or final["frames"] != frames:
        errors.append(f"final timing covers {final['frames']} frames at tick {final['host_tick']}, "
                      f"expected {frames} at {final_tick}")
    if "stack_painted" not in final:
        errors.append("final timing record lacks stack fields")
    if errors:
        return None, errors
    return {"frames": final["frames"], "total_cycles": final["total"],
            "update_cycles": final["update"], "peak_cycles": final["peak"],
            "update_peak_cycles": final["update_peak"],
            "mean_cycles_per_frame": round(final["total"] / final["frames"]),
            "frames_per_second": round(final["frames"] * CPU_HZ / final["total"], 3),
            "stack_untouched_bytes": final["stack_untouched"]}, []


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


def sram_check(mode: str, plan: dict, data: bytes | None, seed: bytes | None,
               tag: int, payload: int) -> tuple[list[dict], list[str]]:
    """Verify the SRAM records a run must leave before it can qualify."""
    if data is None:
        return [], ["SRAM not verified"]
    if len(data) != SAVE_BYTES:
        return [], [f"SRAM is {len(data)} bytes, expected {SAVE_BYTES}"]
    records, errors = save_records(data), []
    for r in records:
        if not r["checksum_valid"] or r["tag"] != tag or r["payload_bytes"] != payload:
            errors.append(f"invalid SRAM record in slot {r['slot']}")
    sequences = sorted(r["sequence"] for r in records)
    won = plan["final"]["kind"] == "won"
    if mode == "prefix" and sequences != [1]:
        errors.append(f"prefix must leave exactly one pause save, found sequences {sequences}")
    if mode == "full" and sequences != ([1] if won else []):
        errors.append(f"full run SRAM sequences {sequences} do not match its outcome")
    if mode == "resume":
        if seed is None or len(seed) != SAVE_BYTES:
            errors.append("resume SRAM seed not supplied")
        else:
            kept = [r for r in records if r["sequence"] == 1]
            if len(kept) != 1 or data[kept[0]["slot"] * SAVE_SLOT_SIZE:(kept[0]["slot"] + 1) * SAVE_SLOT_SIZE] != \
                    seed[kept[0]["slot"] * SAVE_SLOT_SIZE:(kept[0]["slot"] + 1) * SAVE_SLOT_SIZE]:
                errors.append("resume did not keep the seeded pause save")
        if sequences != ([1, 2] if won else [1]):
            errors.append(f"resume SRAM sequences {sequences} do not match its outcome")
    return records, errors


def evaluate(plan: dict, mode: str, text: str, cut: int | None, *, timed: bool = True,
             sram: bytes | None = None, seed: bytes | None = None) -> dict:
    """Qualify one replay log. Every gate must pass; failures are listed."""
    records, failures = parse_records(text)
    singles = records["singles"]
    stop = plan["stop"]
    if len(singles["START"]) != 1:
        failures.append(f"expected one START record, found {len(singles['START'])}")
    if singles["EARLY_END"]:
        failures.append("replay ended early")
    tag, payload = SAVE_TAG, SAVE_PAYLOAD_BYTES
    if mode == "prefix":
        saved = singles["SAVED"]
        fields = {}
        if len(saved) != 1 or not saved[0].startswith("Z7 SAVED ok"):
            failures.append(f"expected one successful SAVED record, found {saved}")
        else:
            fields = parse_fields(saved[0])
        cut = fields.get("cut")
        tag, payload = fields.get("tag", tag), fields.get("payload_bytes", payload)
        if type(cut) is not int or not 1 <= cut < stop:
            failures.append(f"prefix cut {cut} is not inside the tape")
            cut = None
        elif records["kinds"].get(cut, []).count("cut") != 1:
            failures.append(f"no single cut observation at tick {cut}")
        after, through = 0, cut or 0
        if singles["DONE"] or singles["LOADED"]:
            failures.append("prefix log has records of another mode")
        final_kind, frames = "cut", cut or 0
    elif mode == "resume":
        if type(cut) is not int or not 1 <= cut < stop:
            failures.append(f"resume cut {cut} is not inside the tape")
            cut = None
        loaded = singles["LOADED"]
        if len(loaded) != 1 or not loaded[0].startswith("Z7 LOADED ok"):
            failures.append(f"expected one successful LOADED record, found {loaded}")
        if len(singles["DONE"]) != 1 or singles["SAVED"]:
            failures.append("resume must end with one DONE record and no SAVED record")
        after, through = cut or stop, stop
        final_kind, frames = "terminal", stop - (cut or 0) + 1
    else:
        if len(singles["DONE"]) != 1 or singles["SAVED"] or singles["LOADED"]:
            failures.append("full run must end with one DONE record only")
        after, through = 0, stop
        final_kind, frames = "terminal", stop
    expected_checks = sum(1 for t in plan["observations"] if after < t <= through)
    compared, mismatches = compare(plan, records, after=after, through=through)
    if not expected_checks or compared != expected_checks:
        failures.append(f"compared {compared} of {expected_checks} expected observations")
    failures += mismatches[:50]
    if mode != "prefix":
        final, host = records["checks"].get(stop, {}), plan["final"]
        if host["kind"] not in records["kinds"].get(stop, []) or \
                any(final.get(f) != host[f] for f in SCALARS + LISTS):
            failures.append("terminal observation does not match the host")
        if host["kind"] == "won" and not (final.get("screen") == SCREEN_RESULT and
                                          final.get("won") == 1 and final.get("lost") == 0 and
                                          final.get("fault") == 0 and
                                          final.get("enemy_buildings") == 0):
            failures.append("terminal state is not a strict win")
    guest_timing = None
    if timed:
        guest_timing, timing_errors = timing(records, final_kind,
                                             cut if mode == "prefix" else stop, frames)
        failures += timing_errors
    sram_records, sram_errors = sram_check(mode, plan, sram, seed, tag, payload)
    failures += sram_errors
    return {"mode": mode, "lines": records["lines"], "cut": cut,
            "expected_observations": expected_checks, "compared_observations": compared,
            "mismatch_count": len(mismatches), "guest_timing": guest_timing,
            "timed": timed, "sram_records": sram_records,
            "failures": failures[:80], "failure_count": len(failures),
            "qualified": not failures}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)
    tape = sub.add_parser("tape", help="write the embedded key tape")
    tape.add_argument("trace", type=pathlib.Path)
    tape.add_argument("out", type=pathlib.Path)
    check = sub.add_parser("compare", help="qualify a replay log against the trace")
    check.add_argument("trace", type=pathlib.Path)
    check.add_argument("log", type=pathlib.Path)
    check.add_argument("--mode", choices=("full", "prefix", "resume"), default="full")
    check.add_argument("--cut", type=int)
    check.add_argument("--sav", type=pathlib.Path, help="SRAM the run left (required to qualify)")
    check.add_argument("--seed-sav", type=pathlib.Path, help="resume: SRAM the run started with")
    check.add_argument("--untimed", action="store_true",
                       help="host logs carry no guest timing; skip the timing gate")
    run = sub.add_parser("run", help="one isolated mGBA replay")
    run.add_argument("trace", type=pathlib.Path)
    run.add_argument("rom", type=pathlib.Path)
    run.add_argument("out", type=pathlib.Path)
    run.add_argument("--mode", choices=("full", "prefix", "resume"), default="full")
    run.add_argument("--cut", type=int, help="resume: the prefix cut tick")
    run.add_argument("--seed-sav", type=pathlib.Path, help="resume: SRAM written by the prefix")
    run.add_argument("--timeout", type=float, default=6 * 3600)
    args = parser.parse_args()
    for name in ("trace", "rom", "out", "log", "seed_sav", "sav"):
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
    seed = args.seed_sav.read_bytes() if args.seed_sav else None
    if args.command == "compare":
        sram = args.sav.read_bytes() if args.sav else None
        result = evaluate(plan, args.mode, args.log.read_text(errors="replace"), args.cut,
                          timed=not args.untimed, sram=sram, seed=seed)
        print(json.dumps(result, indent=2))
        return 0 if result["qualified"] else 1
    if not MGBA.exists():
        parser.error(f"mGBA not found at {MGBA}")
    if args.out.exists() and any(args.out.iterdir()):
        parser.error(f"{args.out} is not empty; use a fresh output directory")
    args.out.mkdir(parents=True, exist_ok=True)
    markers = (b"Z7 DONE", b"Z7 SAVED", b"Z7 LOADED fail", b"Z7 EARLY_END")
    text, wall = run_mgba(args.rom, args.out, args.timeout, args.seed_sav, markers)
    sav = args.out / "saves" / "replay.sav"
    data = sav.read_bytes() if sav.exists() else None
    result = evaluate(plan, args.mode, text, args.cut, sram=data, seed=seed)
    result.update({"trace_sha256": plan["trace_sha256"],
                   "rom_sha256": sha256(args.rom.read_bytes()), "wall_seconds": round(wall, 1),
                   "sram_sha256": sha256(data) if data else None})
    (args.out / "result.json").write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps({k: result[k] for k in ("qualified", "failures", "compared_observations",
                                             "cut", "guest_timing", "wall_seconds", "sram_records")},
                     indent=2))
    return 0 if result["qualified"] else 1


if __name__ == "__main__":
    sys.exit(main())
