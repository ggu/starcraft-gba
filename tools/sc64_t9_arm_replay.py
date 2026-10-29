#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Normal-menu ARM replay of a T9 controller trace, without a debugger.

`tape TRACE OUT` validates a host trace from build/test-sc64-t9-campaign-run
and writes the key-transition header that tests/sc64_t9_arm_replay.c embeds.
`run` starts one isolated mGBA process (its own HOME, config and save
directory; no debugger port) on the replay ROM, collects the probe's debug
log, and compares every ARM observation with the host trace's observation at
the same mission tick. Guest timing comes from the GBA timer cycle counts the
probe reports, not from host playback speed. Emulator evidence only.
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
    """Strictly validate a T9 trace; return transitions and observations."""
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
    held_before, last_tick = 0, 0
    start = final = None
    for i, row in enumerate(rows):
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
            if kind == "start":
                if start is not None or tick != 0:
                    raise ValueError("trace needs one start observation at tick 0")
                start = row
            elif kind in ("won", "lost", "limit"):
                if i != len(rows) - 1:
                    raise ValueError("terminal observation must be last")
                final = row
            # Same-tick observations (periodic plus a labelled checkpoint)
            # must agree; keep the first.
            prior = observations.get(tick)
            if prior and any(prior.get(f) != row.get(f) for f in COMPARED if f in row):
                raise ValueError(f"same-tick observations disagree at {tick}")
            observations.setdefault(tick, row)
        last_tick = tick
    if start is None or final is None:
        raise ValueError("trace needs start and terminal observations")
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


def parse_fields(line: str) -> dict:
    return {k: int(v) for k, v in re.findall(r"(\w+)=(-?\d+)", line)}


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
    """A run qualifies only with a clean launch, every observation matched,
    valid guest timing, and a final state that agrees with the host."""
    final, host = result["arm_final"], plan["final"]
    ok = (result["done"] and result["launch"].startswith("T9 LAUNCH ok") and
          not result["early_end"] and not result["mismatch_count"] and
          result["guest_timing"] is not None and not result["timing_errors"] and
          final.get("won") == host.get("won") and final.get("lost") == host.get("lost") and
          final.get("faults") == 0)
    if host["kind"] == "won":
        ok = ok and final.get("screen") == SCREEN_RESULT and final.get("won") == 1 and \
            final.get("protoss_force2_buildings") == 0 and \
            final.get("zerg_force3_buildings", 0) > 0 and final.get("raw16_alive") == 1
    return bool(ok)


def compare(plan: dict, text: str) -> dict:
    lines = [l[l.index("T9 "):] for l in text.splitlines() if "T9 " in l]
    checks = {}
    for line in lines:
        if line.startswith("T9 CHECK "):
            kind = line.split()[2]
            row = parse_fields(line)
            checks.setdefault(row["host_tick"], (kind, {}))[1].update(row)
        elif line.startswith("T9 CHECK2"):
            row = parse_fields(line)
            checks.setdefault(row["host_tick"], ("", {}))[1].update(row)
    mismatches, compared = [], 0
    for tick, expected in sorted(plan["observations"].items()):
        if tick not in checks:
            # The ARM run starts observing after the START frame (tick 1);
            # labelled mid-tick host checkpoints have no ARM counterpart.
            if expected["kind"] in ("periodic", plan["final"]["kind"]):
                mismatches.append(f"missing ARM observation at tick {tick}")
            continue
        actual = checks[tick][1]
        compared += 1
        for field in COMPARED:
            if field in expected and expected[field] != actual.get(field):
                mismatches.append(f"tick {tick} {field}: ARM={actual.get(field)} host={expected[field]}")
    timing, timing_errors = parse_timing(lines, plan["stop"])
    launch = next((l for l in lines if l.startswith("T9 LAUNCH")), "")
    final = checks.get(plan["stop"], (None, {}))[1]
    return {"lines": len(lines), "launch": launch, "compared_observations": compared,
            "mismatches": mismatches[:50], "mismatch_count": len(mismatches),
            "arm_final": final, "guest_timing": timing, "timing_errors": timing_errors,
            "early_end": any(l.startswith("T9 EARLY_END") for l in lines),
            "done": any(l.startswith("T9 DONE") for l in lines)}


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
    args = parser.parse_args()
    if args.command == "tape":
        plan = write_tape(args.trace, args.header)
        print(json.dumps({"transitions": len(plan["transitions"]), "stop": plan["stop"],
                          "outcome": plan["final"]["kind"]}))
        return
    plan = load_trace(args.trace)
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
