#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Normal-menu ARM replay of a T9 controller trace, without a debugger.

`tape TRACE OUT` validates a host trace from build/test-sc64-t9-campaign-run
and writes the key-transition header that tests/sc64_t9_arm_replay.c embeds.
`run` starts one isolated mGBA process (its own HOME, config and save
directory; no debugger port) on the replay ROM, collects the probe's debug
log, and compares every ARM observation with the host trace's observation at
the same mission tick. Guest timing comes from the GBA timer cycle counts the
probe reports, not from host playback speed. `audit LOG` applies the same
checks to a retained log. A log qualifies only if it has exactly the probe's
line shape and every line parses completely. Emulator evidence only.
"""
from __future__ import annotations

import argparse
import gzip
import hashlib
import json
import os
import pathlib
import pty
import re
import select
import subprocess
import sys
import time

TOOLS = pathlib.Path(__file__).resolve().parent
if str(TOOLS) not in sys.path:
    sys.path.insert(0, str(TOOLS))

from emulator_smoke import ROOT, require_mgba

CPU_HZ = 16_777_216
KEY_MASK = 1023
MISSION = 12
SCREEN_PLAY, SCREEN_RESULT = 1, 3
# Both the host harness and the probe observe every CHECK_PERIOD ticks.
CHECK_PERIOD = 1000
TERMINAL_KINDS = ("won", "lost", "limit")
# Labelled mid-run host checkpoints; the probe has no counterpart for them.
LABELLED_KINDS = ("opening", "wave-launch", "wave-retreat")
COMPARED = ("frame", "screen", "won", "lost", "mission", "faults", "phase",
            "objective_flags", "race_completed", "protoss_force2_buildings",
            "zerg_force3_buildings", "raw16_alive", "side_units", "zerg_units",
            "minerals", "gas", "selection", "selected_building")
DEFAULT_TRACE = ROOT / "build" / "t9-arm" / "t9-controller.json"
DEFAULT_ROM = ROOT / "build" / "StarCraft-Terran-T9-ARM-Replay.gba"
DEFAULT_ELF = ROOT / "build" / "starcraft-terran-t9-arm-replay.elf"


def sha256(path: pathlib.Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def load_trace(path: pathlib.Path) -> dict:
    """Strictly validate a T9 trace; return transitions and observations.

    The host reference must be complete on its own, independently of any ARM
    log: every observation carries every compared field as an integer, and
    periodic observations fall exactly on every CHECK_PERIOD ticks through
    the stop tick, once each."""
    trace = json.loads(path.read_text())
    if not isinstance(trace, dict) or trace.get("trace_schema") != 1:
        raise ValueError("trace must use trace_schema=1")
    if trace.get("mission") != MISSION or trace.get("entry") != "t9-probe-entry":
        raise ValueError("trace is not a T9 probe-entry controller trace")
    rows = trace.get("events")
    if not isinstance(rows, list) or len(rows) < 2:
        raise ValueError("trace events are missing")
    transitions: list[tuple[int, int]] = []
    observations: dict[int, dict] = {}
    periodic: set[int] = set()
    held_before, last_tick = 0, 0
    start = final = None
    for i, row in enumerate(rows):
        if not isinstance(row, dict):
            raise ValueError(f"event {i} is not an object")
        kind, tick = row.get("kind"), row.get("host_tick")
        if type(tick) is not int or tick < last_tick:
            raise ValueError(f"event {i} has an invalid or unordered host_tick")
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
        else:
            if kind not in ("start", "periodic") + TERMINAL_KINDS + LABELLED_KINDS:
                raise ValueError(f"event {i} has unknown kind {kind!r}")
            missing = [f for f in COMPARED if type(row.get(f)) is not int]
            if missing:
                raise ValueError(f"{kind} observation at tick {tick} lacks integer "
                                 f"fields: {', '.join(missing)}")
            if kind == "periodic":
                if tick in periodic:
                    raise ValueError(f"duplicate periodic observation at tick {tick}")
                periodic.add(tick)
            if kind == "start":
                if start is not None or tick != 0:
                    raise ValueError("trace needs one start observation at tick 0")
                start = row
            elif kind in TERMINAL_KINDS:
                if i != len(rows) - 1:
                    raise ValueError("terminal observation must be last")
                final = row
            # Same-tick observations (periodic plus a labelled checkpoint)
            # must agree; keep the first.
            prior = observations.get(tick)
            if prior and any(prior[f] != row[f] for f in COMPARED):
                raise ValueError(f"same-tick observations disagree at {tick}")
            observations.setdefault(tick, row)
        last_tick = tick
    if start is None or final is None:
        raise ValueError("trace needs start and terminal observations")
    cadence = set(range(CHECK_PERIOD, final["host_tick"] + 1, CHECK_PERIOD))
    if periodic != cadence:
        missing, extra = sorted(cadence - periodic), sorted(periodic - cadence)
        raise ValueError(f"host periodic observations break the {CHECK_PERIOD}-tick cadence: "
                         f"missing {missing[:5]}, unexpected {extra[:5]}")
    if transitions and transitions[-1][0] > final["host_tick"]:
        raise ValueError("input after the terminal observation")
    if transitions and transitions[0][0] == 1:
        raise ValueError("tick 1 must be neutral: the briefing's START frame runs it")
    return {"transitions": transitions, "observations": observations,
            "final": final, "stop": final["host_tick"], "policy": trace.get("policy")}


def write_tape(trace_path: pathlib.Path, out: pathlib.Path) -> dict:
    plan = load_trace(trace_path)
    words = [(tick << 10) | held for tick, held in plan["transitions"]]
    outcome = {"won": 1, "lost": 2, "limit": 0}[plan["final"]["kind"]]
    body = ",\n".join("    " + ",".join(f"0x{w:08x}u" for w in words[i:i + 8])
                      for i in range(0, len(words), 8))
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(
        "/* Generated by tools/sc64_t9_arm_replay.py from a T9 controller trace.\n"
        f" * Trace SHA-256 {sha256(trace_path)}. Each word is tick << 10 | held. */\n"
        "#ifndef SC64_T9_TAPE_GENERATED_H\n#define SC64_T9_TAPE_GENERATED_H\n"
        f"#define T9_TAPE_COUNT {len(words)}u\n"
        f"#define T9_TAPE_STOP_TICK {plan['stop']}u\n"
        f"#define T9_TAPE_OUTCOME {outcome}\n"
        f"static const u32 t9_tape[{max(1, len(words))}] = {{\n{body or '    0'}\n}};\n"
        "#endif\n")
    return plan


def run_mgba(mgba: str, rom: pathlib.Path, out: pathlib.Path, timeout: float) -> tuple[str, float]:
    """One isolated mGBA process; returns its debug log text and wall time."""
    home, saves = out / "home", out / "saves"
    config = home / ".config" / "mgba"
    config.mkdir(parents=True, exist_ok=True)
    saves.mkdir(parents=True, exist_ok=True)
    (config / "config.ini").write_text("[ports.qt]\nlogToStdout=1\nlogLevel=7\n")
    env = dict(os.environ, HOME=str(home), XDG_CONFIG_HOME=str(home / ".config"))
    master, slave = pty.openpty()
    began = time.monotonic()
    # Decimal mask 7 = fatal|error|warn: the probe logs at warn, and INFO
    # would add every DMA (~160 MB and hours of host I/O per run).
    proc = subprocess.Popen(
        [mgba, "-l", "7", "-C", "mute=1", "-C", "audioSync=0", "-C", "videoSync=0",
         "-C", f"savegamePath={saves}", "-C", f"savestatePath={saves}",
         "-C", f"screenshotPath={out}", str(rom)],
        stdin=subprocess.DEVNULL, stdout=slave, stderr=slave, env=env, cwd=out)
    os.close(slave)
    captured = bytearray()
    log = (out / "mgba.log").open("wb")  # streamed, so a cut-off run keeps its evidence
    try:
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline and proc.poll() is None:
            ready, _, _ = select.select([master], [], [], 1.0)
            if ready:
                try:
                    chunk = os.read(master, 65536)
                except OSError:
                    break
                if not chunk:
                    break
                captured.extend(chunk)
                log.write(chunk)
                log.flush()
                tail = captured[-(len(chunk) + 32):]  # not the whole run
                if b"T9 DONE" in tail or b"T9 LAUNCH fail" in tail:
                    break
    finally:
        proc.terminate()
        try:
            proc.wait(timeout=5)
        except subprocess.TimeoutExpired:
            proc.kill()
            proc.wait()
        os.close(master)
        log.close()
    text = captured.decode(errors="replace")
    return text, time.monotonic() - began


DEBUG_PREFIX = "[WARN] GBA Debug: "
# Keys the probe presses on the normal menu: A, RIGHT x4, A, START. Each press
# is a held frame and a release frame; the START release is mission tick 1.
MENU_ROUTE = (1, 16, 16, 16, 16, 1, 8)
MENU_FRAMES = 2 * len(MENU_ROUTE) - 1
CHECK_FIELDS = ("host_tick", "frame", "screen", "won", "lost", "mission", "faults", "phase",
                "objective_flags", "race_completed")
CHECK2_FIELDS = ("host_tick", "protoss_force2_buildings", "zerg_force3_buildings",
                 "raw16_alive", "side_units", "zerg_units", "minerals", "gas", "selection",
                 "selected_building")


def record_re(head: str, fields: tuple[str, ...]) -> re.Pattern:
    return re.compile(re.escape(head) + "".join(rf" {f}=(-?\d+)" for f in fields))


START_RE = record_re("T9 START", ("debug", "tape", "stop"))
MENU_RE = record_re("T9 MENU", ("key", "screen", "page", "row", "mission"))
LAUNCH_RE = record_re("T9 LAUNCH ok", ("menu_frames", "mission_frames", "frame", "screen",
                                       "mission", "race_mode"))
CHECK_RE = re.compile(r"T9 CHECK (launch|periodic|won|lost|limit)" +
                      "".join(rf" {f}=(-?\d+)" for f in CHECK_FIELDS))
CHECK2_RE = record_re("T9 CHECK2", CHECK2_FIELDS)
DONE_RE = record_re("T9 DONE", ("cursor",))


def probe_lines(text: str) -> tuple[list[str], list[str]]:
    """The probe's debug-register lines, and any other debug text (an error)."""
    lines, stray = [], []
    for raw in text.splitlines():
        if DEBUG_PREFIX in raw:
            body = raw[raw.index(DEBUG_PREFIX) + len(DEBUG_PREFIX):].rstrip()
            (lines if body.startswith("T9 ") else stray).append(body)
    return lines, stray


def line_kind(line: str) -> str:
    return line.split(" ", 2)[1] if " " in line else line


def parse_log(plan: dict, lines: list[str]) -> tuple[dict, list[str]]:
    """Validate the probe's whole debug log; return observations by tick.

    The log must have exactly the probe's shape: START, the seven menu keys,
    LAUNCH ok, CHECK/CHECK2 pairs, MENU_CYCLES, CYCLES and DONE, in that
    order. Every line must parse completely. Each observation tick and kind
    appears once, except that a periodic and a terminal record may share the
    stop tick when they agree on every field."""
    errors: list[str] = []
    kinds = [line_kind(l) for l in lines]
    n_checks = kinds.count("CHECK")
    shape = (["START"] + ["MENU"] * len(MENU_ROUTE) + ["LAUNCH"] +
             ["CHECK", "CHECK2"] * n_checks + ["MENU_CYCLES", "CYCLES", "DONE"])
    if kinds != shape:
        first = next((i for i, (a, b) in enumerate(zip(kinds, shape)) if a != b),
                     min(len(kinds), len(shape)))
        got = lines[first] if first < len(lines) else "end of log"
        errors.append(f"probe log out of shape at line {first}: {got[:120]!r}")
        return {}, errors

    def fields(pattern: re.Pattern, line: str, names: tuple[str, ...]) -> dict | None:
        match = pattern.fullmatch(line)
        if not match:
            errors.append(f"malformed probe line: {line[:120]!r}")
            return None
        return dict(zip(names, (int(v) for v in match.groups())))

    start = fields(START_RE, lines[0], ("debug", "tape", "stop"))
    if start and (start["debug"] != 1 or start["tape"] != len(plan["transitions"]) or
                  start["stop"] != plan["stop"]):
        errors.append(f"START does not match the trace: {lines[0]!r}")
    menu_names = ("key", "screen", "page", "row", "mission")
    menus = [fields(MENU_RE, l, menu_names) for l in lines[1:1 + len(MENU_ROUTE)]]
    if all(menus):
        if tuple(m["key"] for m in menus) != MENU_ROUTE:
            errors.append("menu keys are not the normal route A, RIGHT x4, A, START")
        if (menus[-1]["screen"], menus[-1]["mission"]) != (SCREEN_PLAY, MISSION):
            errors.append("the menu route did not end in mission 12")
    launch_line = lines[1 + len(MENU_ROUTE)]
    launch = fields(LAUNCH_RE, launch_line, ("menu_frames", "mission_frames", "frame",
                                             "screen", "mission", "race_mode"))
    if launch and (launch["menu_frames"] != MENU_FRAMES or launch["mission_frames"] != 1 or
                   launch["screen"] != SCREEN_PLAY or launch["mission"] != MISSION or
                   launch["race_mode"] != 0):
        errors.append(f"LAUNCH fields are not a normal-menu mission-12 start: {launch_line!r}")

    observations: dict[int, dict] = {}
    seen: set[tuple[int, str]] = set()
    last_tick = 0
    body = lines[2 + len(MENU_ROUTE):2 + len(MENU_ROUTE) + 2 * n_checks]
    for first, second in zip(body[0::2], body[1::2]):
        match = CHECK_RE.fullmatch(first)
        if not match:
            errors.append(f"malformed probe line: {first[:120]!r}")
            continue
        kind = match.group(1)
        row = dict(zip(CHECK_FIELDS, (int(v) for v in match.groups()[1:])))
        row2 = fields(CHECK2_RE, second, CHECK2_FIELDS)
        if row2 is None:
            continue
        tick = row["host_tick"]
        if row2["host_tick"] != tick:
            errors.append(f"CHECK2 at tick {row2['host_tick']} does not pair with CHECK at {tick}")
            continue
        if tick < last_tick or (tick, kind) in seen:
            errors.append(f"duplicate or unordered {kind} observation at tick {tick}")
            continue
        seen.add((tick, kind))
        last_tick = tick
        row.update(row2)
        row["kind"] = kind
        prior = observations.get(tick)
        if prior is not None:
            if any(prior[f] != row[f] for f in COMPARED):
                errors.append(f"conflicting observations at tick {tick}")
            elif kind in TERMINAL_KINDS:
                prior["kind"] = kind
            continue
        observations[tick] = row

    stop, final_kind = plan["stop"], plan["final"]["kind"]
    expected = {(1, "launch"), (stop, final_kind)}
    expected |= {(t, "periodic") for t in range(CHECK_PERIOD, stop + 1, CHECK_PERIOD)}
    for tick, kind in sorted(expected - seen):
        errors.append(f"missing ARM {kind} observation at tick {tick}")
    for tick, kind in sorted(seen - expected):
        errors.append(f"unexpected ARM {kind} observation at tick {tick}")
    first_obs = observations.get(1)
    if first_obs and launch and (first_obs["screen"] != SCREEN_PLAY or first_obs["won"] or
                                 first_obs["lost"] or first_obs["faults"] or
                                 first_obs["mission"] != MISSION or
                                 first_obs["frame"] != launch["frame"]):
        errors.append("the launch observation is not a clean mission-12 start")
    done = fields(DONE_RE, lines[-1], ("cursor",))
    if done and done["cursor"] != len(plan["transitions"]):
        errors.append(f"DONE consumed {done['cursor']} of {len(plan['transitions'])} tape entries")
    return observations, errors


CYCLES_RE = re.compile(r"T9 CYCLES frames=(\d+) total=(\d+) update=(\d+) peak=(\d+) "
                       r"buckets=(\d+(?:,\d+){7})")
MENU_CYCLES_RE = re.compile(r"T9 MENU_CYCLES frames=(\d+) total=(\d+)")


def parse_timing(lines: list[str], stop: int) -> tuple[dict | None, list[str]]:
    """Exactly one well-formed menu record and one mission record, or errors.

    Mission timing covers ticks 1..stop: the briefing's START release runs
    tick 1. The boot, menu and START-press frames before it are reported
    separately and never count as mission frames."""
    errors, records = [], {}
    for name, pattern in (("MENU_CYCLES", MENU_CYCLES_RE), ("CYCLES", CYCLES_RE)):
        found = [l for l in lines if l.split(" ", 2)[1:2] == [name]]
        if len(found) != 1:
            errors.append(f"expected one T9 {name} record, found {len(found)}")
            continue
        match = pattern.fullmatch(found[0].rstrip())
        if not match:
            errors.append(f"malformed T9 {name} record: {found[0]!r}")
            continue
        records[name] = match.groups()
    if errors:
        return None, errors
    menu_frames, menu_total = (int(v) for v in records["MENU_CYCLES"])
    frames, total, update, peak = (int(v) for v in records["CYCLES"][:4])
    buckets = [int(v) for v in records["CYCLES"][4].split(",")]
    if min(menu_frames, menu_total, frames, total, update, peak) <= 0:
        errors.append("timing counts must be positive")
    if frames != stop:
        errors.append(f"mission timing covers {frames} frames, expected ticks 1..{stop}")
    if sum(buckets) != frames:
        errors.append(f"cycle buckets sum to {sum(buckets)}, not {frames} frames")
    if update > total or peak > total or total > peak * frames:
        errors.append("update, peak and total cycles are inconsistent")
    if errors:
        return None, errors
    return {"mission_frames": frames, "total_cycles": total, "update_cycles": update,
            "peak_cycles": peak, "frame_period_buckets": buckets,
            "mean_cycles_per_frame": round(total / frames),
            "effective_fps": round(frames * CPU_HZ / total, 2),
            "guest_seconds": round(total / CPU_HZ, 1),
            "menu_frames": menu_frames, "menu_cycles": menu_total}, []


def verdict(plan: dict, result: dict) -> bool:
    """A run qualifies only with a log of the probe's exact shape, every
    observation matched, valid guest timing, and a final state that agrees
    with the host."""
    final, host = result["arm_final"], plan["final"]
    ok = (result["done"] and not result["log_errors"] and not result["mismatch_count"] and
          result["guest_timing"] is not None and not result["timing_errors"] and
          final.get("kind") == host["kind"] and
          final.get("won") == host.get("won") and final.get("lost") == host.get("lost") and
          final.get("faults") == 0)
    if host["kind"] == "won":
        ok = ok and final.get("screen") == SCREEN_RESULT and final.get("won") == 1 and \
            final.get("protoss_force2_buildings") == 0 and \
            final.get("zerg_force3_buildings", 0) > 0 and final.get("raw16_alive") == 1
    return bool(ok)


def compare(plan: dict, text: str) -> dict:
    lines, stray = probe_lines(text)
    checks, log_errors = parse_log(plan, lines)
    log_errors = [f"non-probe debug line: {l[:120]!r}" for l in stray] + log_errors
    mismatches, compared = [], 0
    for tick, expected in sorted(plan["observations"].items()):
        if tick not in checks:
            # The ARM run starts observing after the START frame (tick 1);
            # labelled mid-tick host checkpoints have no ARM counterpart.
            if expected["kind"] in ("periodic", plan["final"]["kind"]):
                mismatches.append(f"missing ARM observation at tick {tick}")
            continue
        actual = checks[tick]
        compared += 1
        for field in COMPARED:
            if expected[field] != actual.get(field):
                mismatches.append(f"tick {tick} {field}: ARM={actual.get(field)} host={expected[field]}")
    timing, timing_errors = parse_timing(lines, plan["stop"])
    final = checks.get(plan["stop"], {})
    return {"lines": len(lines), "launch": next((l for l in lines if l.startswith("T9 LAUNCH")), ""),
            "log_errors": log_errors[:50], "compared_observations": compared,
            "mismatches": mismatches[:50], "mismatch_count": len(mismatches),
            "arm_final": final, "guest_timing": timing, "timing_errors": timing_errors,
            "early_end": any(l.startswith("T9 EARLY_END") for l in lines),
            "done": bool(lines) and DONE_RE.fullmatch(lines[-1]) is not None}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)
    tape = sub.add_parser("tape", help="write the probe's key-tape header")
    tape.add_argument("trace", type=pathlib.Path)
    tape.add_argument("header", type=pathlib.Path)
    run = sub.add_parser("run", help="replay the ROM in an isolated mGBA process")
    run.add_argument("--trace", type=pathlib.Path, default=DEFAULT_TRACE)
    run.add_argument("--rom", type=pathlib.Path, default=DEFAULT_ROM)
    run.add_argument("--elf", type=pathlib.Path, default=DEFAULT_ELF)
    run.add_argument("--shipped-rom", type=pathlib.Path,
                     default=ROOT / "build" / "StarCraft-Terran-GBA.gba")
    run.add_argument("--out", type=pathlib.Path)
    run.add_argument("--timeout", type=float, default=7200)
    audit = sub.add_parser("audit", help="re-check a retained mGBA log (.log or .log.gz)")
    audit.add_argument("log", type=pathlib.Path)
    audit.add_argument("--trace", type=pathlib.Path, default=DEFAULT_TRACE)
    args = parser.parse_args()
    if args.command == "tape":
        plan = write_tape(args.trace, args.header)
        print(json.dumps({"transitions": len(plan["transitions"]), "stop": plan["stop"],
                          "outcome": plan["final"]["kind"]}))
        return
    plan = load_trace(args.trace)
    if args.command == "audit":
        data = args.log.read_bytes()
        if args.log.suffix == ".gz":
            data = gzip.decompress(data)
        result = compare(plan, data.decode(errors="replace"))
        result.update({"log": str(args.log), "log_sha256": sha256(args.log),
                       "trace": str(args.trace), "trace_sha256": sha256(args.trace)})
        result["ok"] = verdict(plan, result)
        print(json.dumps(result, indent=2))
        if not result["ok"]:
            raise SystemExit(1)
        return
    for path in (args.rom, args.elf):
        if not path.is_file():
            raise SystemExit(f"build {path} first (make sc64-t9-arm-replay)")
    out = (args.out or ROOT / "build" / "emulator" / "t9-arm-replay").resolve()
    if out.exists() and any(out.iterdir()):
        raise SystemExit(f"{out} is not empty; choose a fresh --out")
    out.mkdir(parents=True, exist_ok=True)
    text, wall = run_mgba(require_mgba(), args.rom, out, args.timeout)
    result = compare(plan, text)
    result.update({
        "trace": str(args.trace), "trace_sha256": sha256(args.trace),
        "trace_policy": plan["policy"], "host_final": plan["final"],
        "rom": str(args.rom), "rom_sha256": sha256(args.rom), "elf_sha256": sha256(args.elf),
        "shipped_rom_sha256": sha256(args.shipped_rom) if args.shipped_rom.is_file() else None,
        "wall_seconds": round(wall, 1)})
    ok = verdict(plan, result)
    result["ok"] = ok
    (out / "result.json").write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps({k: v for k, v in result.items() if k != "host_final"}, indent=2))
    if not ok:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
