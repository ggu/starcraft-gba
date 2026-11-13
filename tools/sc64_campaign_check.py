#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Prove all 30 campaign missions with their scripted ordinary-input controllers.

For each mission T1-T10, Z1-Z10 and P1-P10 this runs the one make rule that wins
it (mk/campaign-check/Makefile, ``check-campaign-<id>``), builds what the rule
needs under the shared native-build lock, runs the controller outside it, and
prints a table: mission, make rule, command, result, win frame. A mission passes
only if its rule exits 0 and its output reports the win. The tool exits non-zero
if any mission does not win; a mission whose private inputs are absent is
SKIPPED (never a pass; ``--require-all`` makes it a failure).

HOST only: nothing here runs an emulator or touches hardware. Builds use the
sanitizers by default, as the project's controller targets do. ``--fast`` builds
with tools/clang_unsanitized.sh (-O2 -w -D_FORTIFY_SOURCE=0) for a quick sweep;
the result is then the unsanitized build's and is labelled so.
"""
from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import re
import shutil
import sys
from dataclasses import dataclass
from typing import Callable, Sequence

sys.path.insert(0, str(Path(__file__).resolve().parent))
import sc64_release as rel  # noqa: E402
import sc64_rom_check as romcheck  # noqa: E402

ROOT = rel.ROOT
PASS, FAIL, SKIPPED = rel.PASS, rel.FAIL, rel.SKIPPED
FAST_CLANG = "tools/clang_unsanitized.sh"
STAMP = "build/campaign-check/flavour"

Step = tuple[str, tuple[str, ...]]      # ("build" | "run", make targets)


@dataclass(frozen=True)
class Mission:
    key: str
    title: str
    steps: tuple[Step, ...]             # the last step is always run:check-campaign-<key>
    win: str                            # regex that must match the output; group 1 is the win frame
    unit: str = "frame"                 # what group 1 counts, when it is not a frame

    @property
    def rule(self) -> str:
        return f"check-campaign-{self.key}"


def simple(key: str, title: str, binary: str, win: str, unit: str = "frame") -> Mission:
    return Mission(key, title, (("build", (f"build/{binary}",)), ("run", (f"check-campaign-{key}",))),
                   win, unit)


def protoss(key: str, title: str, win: str, unit: str = "frame") -> Mission:
    """Controller -> key tape -> controller-free host replay, which must win."""
    px = f"build/protoss-px/{key}"
    return Mission(key, title, (
        ("build", (f"build/px-{key}-controller",)),
        ("run", (f"{px}/sc64_px_tape_generated.h",)),
        ("build", (f"build/px-{key}-replay",)),
        ("run", (f"check-campaign-{key}",)),
    ), win, unit)


PX_WIN = r"PX CHECK won tick=\d+ frame=(\d+) screen=\d+ won=1"

MISSIONS: tuple[Mission, ...] = (
    simple("t1", "Terran 1 Wasteland", "test-sc64-full-t1-run", r"PASS Wasteland: frame=(\d+)"),
    simple("t2", "Terran 2 Backwater", "test-sc64-full-t2-run",
           r"Backwater combat result: frame=(\d+) ticks=\d+ won=1"),
    simple("t3", "Terran 3 Desperate Alliance", "test-sc64-full-t3-run",
           r"T3 controller run won: ticks=(\d+)", "tick"),
    simple("t4", "Terran 4 The Dylarian Shipyards", "test-sc64-full-t4-run",
           r"T4 attempt result=WON frame=(\d+)"),
    simple("t5", "Terran 5", "test-sc64-full-t5-run", r"T5 won tick=\d+ frame=(\d+)"),
    simple("t6", "Terran 6", "test-sc64-full-t6-run", r"T6 probe result ticks=\d+ frame=(\d+) won=1"),
    simple("t7", "Terran 7", "test-sc64-t7-campaign-run", r"T7 result tick=\d+ frame=(\d+) won=1"),
    simple("t8", "Terran 8", "test-sc64-t8-campaign-run", r"mission=11 win=1 lost=0 tick=\d+ frame=(\d+)"),
    simple("t9", "Terran 9", "test-sc64-t9-campaign-run", r"T9 attempt tick=\d+ frame=(\d+) outcome=won"),
    simple("t10", "Terran 10", "test-sc64-t10-campaign-run", r"mission=13 win=1 lost=0 tick=\d+ frame=(\d+)"),
    simple("z1", "Zerg 1", "test-sc64-z9-carried-z1",
           r"Z1 controller replay completed at tick (\d+)", "tick"),
    simple("z2", "Zerg 2", "test-sc64-z9-carried-z1z2",
           r"Controller completed race mission 1 in (\d+) input frames"),
    simple("z3", "Zerg 3", "test-sc64-z9-carried-z3", r"Z3 normal-controller run: .*result=WON frame=(\d+)"),
    simple("z4", "Zerg 4", "test-sc64-z9-carried-z4", r"Z4 controller attempt result=WON frame=(\d+)"),
    simple("z5", "Zerg 5", "test-sc64-z9-carried-z5", r"Z5 controller attempt=VERIFIED-WIN frame=(\d+)"),
    simple("z6", "Zerg 6", "test-sc64-z9-carried-z6", r'"kind":"won","host_tick":\d+,"frame":(\d+)'),
    simple("z7", "Zerg 7", "test-sc64-z9-carried-z7", r"Z7 controller result=won host_ticks=\d+ frame=(\d+)"),
    simple("z8", "Zerg 8", "test-sc64-z9-carried-z8", r"z8 controller: frame=(\d+) calls=\d+ won=1"),
    simple("z9", "Zerg 9", "test-sc64-z9-campaign-run", r"z9 controller: frame=(\d+) calls=\d+ won=1"),
    simple("z10", "Zerg 10", "test-sc64-z10-campaign-run", r"z10 controller: frame=(\d+) calls=\d+ won=1"),
    protoss("p1", "Protoss 1", PX_WIN),
    protoss("p2", "Protoss 2", PX_WIN),
    protoss("p3", "Protoss 3", PX_WIN),
    protoss("p4", "Protoss 4", PX_WIN),
    protoss("p5", "Protoss 5", PX_WIN),
    protoss("p6", "Protoss 6", PX_WIN),
    protoss("p7", "Protoss 7", PX_WIN),
    protoss("p8", "Protoss 8", PX_WIN),
    protoss("p9", "Protoss 9", PX_WIN),
    protoss("p10", "Protoss 10", PX_WIN),
)
BY_KEY = {m.key: m for m in MISSIONS}


def make_args_for(base: Sequence[str], fast: bool) -> list[str]:
    args = list(base)
    if fast:
        args.append(f"CLANG={FAST_CLANG}")
    return args


def outputs_to_discard(mission: Mission) -> list[str]:
    """Files a flavour change must drop: the mission's binaries and Protoss results."""
    paths: list[str] = []
    for kind, targets in mission.steps:
        for target in targets:
            if kind == "build" or target.startswith("build/protoss-px/"):
                paths += [target, target + ".dSYM"]
    if mission.key.startswith("p"):
        paths += [f"build/protoss-px/{mission.key}/host-full.txt",
                  f"build/protoss-px/{mission.key}/trace.jsonl"]
    return paths


def discard(missions: Sequence[Mission], root: Path) -> None:
    for mission in missions:
        for path in outputs_to_discard(mission):
            target = root / path
            if target.is_dir():
                shutil.rmtree(target, ignore_errors=True)
            elif target.exists():
                target.unlink()


def reset_for_flavour(missions: Sequence[Mission], fast: bool, fresh: bool, root: Path) -> None:
    """A binary built with the sanitizers must not stand in for an unsanitized one
    (make cannot see the CLANG variable), so a change of flavour discards every
    mission's binaries; ``fresh`` discards the selected missions' only."""
    flavour = "fast" if fast else "sanitized"
    stamp = root / STAMP
    previous = stamp.read_text(encoding="utf-8").strip() if stamp.is_file() else None
    if previous is not None and previous != flavour:
        discard(MISSIONS, root)
    elif fresh:
        discard(missions, root)
    stamp.parent.mkdir(parents=True, exist_ok=True)
    stamp.write_text(flavour + "\n", encoding="utf-8")


def command_of(log_text: str, prefer_grep: bool = False) -> str:
    """The command that decided the result: the last binary make echoed, or, for
    the Protoss replay check (and as a fallback), the grep that reads the verdict."""
    binary = grep = ""
    for line in log_text.splitlines():
        if line.startswith("./build/"):
            binary = line.strip()
        elif line.startswith("grep "):
            grep = line.strip()
    return (grep or binary) if prefer_grep else (binary or grep)


def run_mission(mission: Mission, make_args: Sequence[str], logs: Path, *, root: Path,
                runner: Callable[[list[str], Path, Path], int], lock_path: str | None,
                min_free_bytes: int, free_bytes: Callable[[], int],
                find_missing: Callable[[Sequence[str], Sequence[str]], list[str]]) -> dict[str, object]:
    row: dict[str, object] = {"mission": mission.key.upper(), "title": mission.title,
                              "rule": mission.rule, "command": "", "status": PASS,
                              "frame": None, "unit": mission.unit, "detail": "", "log": ""}
    log = logs / f"{mission.key}.log"
    row["log"] = str(log)
    log.parent.mkdir(parents=True, exist_ok=True)
    log.write_text("", encoding="utf-8")
    run_log_text = ""
    for index, (kind, targets) in enumerate(mission.steps):
        missing = find_missing(list(targets), list(make_args))
        if missing:
            row["status"] = SKIPPED
            row["detail"] = "missing: " + ", ".join(missing[:2]) + (
                f" (+{len(missing) - 2} more)" if len(missing) > 2 else "")
            return row
        if kind == "build" and free_bytes() < min_free_bytes:
            row["status"], row["detail"] = FAIL, f"refusing to build: {free_bytes() / rel.GIB:.1f} GiB free"
            return row
        step_log = logs / f"{mission.key}.step{index}.log"
        command = rel.make_command(make_args, list(targets))
        with rel.build_lock(lock_path if kind == "build" else None):
            rc = runner(command, step_log, root)
        text = step_log.read_text(encoding="utf-8", errors="replace")
        with log.open("a", encoding="utf-8") as handle:
            handle.write(text)
        step_log.unlink()
        run_log_text = text if kind == "run" and index == len(mission.steps) - 1 else run_log_text
        if rc != 0:
            row["status"] = FAIL
            row["detail"] = f"{' '.join(targets)[:60]} exit {rc}: {rel.first_error_line(log)}"
            row["command"] = command_of(text, mission.key.startswith("p"))
            return row
    full = log.read_text(encoding="utf-8", errors="replace")
    row["command"] = command_of(run_log_text, mission.key.startswith("p"))
    match = re.search(mission.win, full)
    if match is None:
        row["status"] = FAIL
        row["detail"] = "rule exited 0 but the output does not report a win"
        return row
    row["frame"] = int(match.group(1))
    return row


def render_table(rows: Sequence[dict[str, object]], fast: bool) -> str:
    header = ("mission", "make rule", "command", "result", "win frame")
    body = []
    for row in rows:
        result = str(row["status"]) + (f" ({row['detail']})" if row["detail"] else "")
        frame = "-" if row["frame"] is None else f"{row['frame']:,}" + (
            " tick" if row["unit"] == "tick" else "")
        command = str(row["command"])
        if len(command) > 90:
            command = command[:87] + "..."
        body.append((str(row["mission"]), str(row["rule"]), command, result, frame))
    widths = [max(len(header[i]), *(len(r[i]) for r in body)) for i in range(4)]
    def line(cells: Sequence[str]) -> str:
        return "  ".join(cells[i].ljust(widths[i]) for i in range(4)) + "  " + cells[4]
    lines = [line(header)] + [line(r) for r in body]
    lines.append("Build: " + ("unsanitized (-O2 -w -D_FORTIFY_SOURCE=0)" if fast
                              else "sanitizers (ASan/UBSan), the project's controller flags")
                 + ". HOST only; no emulator or hardware.")
    return "\n".join(lines)


def exit_code(rows: Sequence[dict[str, object]], require_all: bool = False) -> int:
    return 1 if any(r["status"] == FAIL or (require_all and r["status"] == SKIPPED)
                    for r in rows) else 0


def select(spec: str | None) -> list[Mission]:
    if not spec:
        return list(MISSIONS)
    names = [part.strip().lower() for part in spec.split(",") if part.strip()]
    unknown = [n for n in names if n not in BY_KEY]
    if unknown:
        raise rel.ReleaseError("unknown mission(s): " + ", ".join(unknown))
    return [m for m in MISSIONS if m.key in names]


def main(argv: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("--only", help="comma list of missions, e.g. t4,z9,p10")
    parser.add_argument("--fast", action="store_true", help="unsanitized -O2 build (quick sweep)")
    parser.add_argument("--fresh", action="store_true",
                        help="delete the selected missions' binaries and Protoss results first")
    parser.add_argument("--require-all", action="store_true", help="treat SKIPPED as failure")
    parser.add_argument("--make-arg", action="append", default=[], metavar="ARG",
                        help="extra make argument (repeatable); replaces the local-input defaults")
    parser.add_argument("--out", type=Path, help="log directory (default build/campaign-check/<rev8>)")
    parser.add_argument("--build-lock", help="flock file for builds (default: $SC64_BUILD_LOCK)")
    parser.add_argument("--min-free-gib", type=float, default=4.0)
    args = parser.parse_args(argv)
    try:
        missions = select(args.only)
        base = list(args.make_arg) if args.make_arg else romcheck.default_make_args()
        make_args = make_args_for(base, args.fast)
        revision, dirty = rel.source_state()
        out = args.out or ROOT / "build" / "campaign-check" / revision[:8]
        reset_for_flavour(missions, args.fast, args.fresh, ROOT)
        free = lambda: shutil.disk_usage(ROOT).free  # noqa: E731
        find = lambda t, a: rel.find_missing_inputs(t, a, ROOT)  # noqa: E731
        lock = args.build_lock or os.environ.get("SC64_BUILD_LOCK") or None
        rows = [run_mission(m, make_args, out, root=ROOT, runner=rel.default_runner, lock_path=lock,
                            min_free_bytes=int(args.min_free_gib * rel.GIB), free_bytes=free,
                            find_missing=find) for m in missions]
    except rel.ReleaseError as error:
        print(f"ERROR: {error}", file=sys.stderr)
        return 2
    table = render_table(rows, args.fast)
    (out / "campaign-check.json").write_text(json.dumps(
        {"revision": revision, "dirty": dirty, "fast": args.fast, "make_args": make_args,
         "rows": rows}, indent=2) + "\n", encoding="utf-8")
    (out / "campaign-check.txt").write_text(table + "\n", encoding="utf-8")
    print(table)
    counts = {s: sum(1 for r in rows if r["status"] == s) for s in (PASS, FAIL, SKIPPED)}
    print(f"Wrote {out / 'campaign-check.json'}; logs in {out}")
    print(f"Overall: {counts[PASS]} of {len(rows)} missions win, {counts[FAIL]} FAIL, "
          f"{counts[SKIPPED]} SKIPPED" + (f" (revision {revision[:8]}{'-dirty' if dirty else ''})"))
    return exit_code(rows, args.require_all)


if __name__ == "__main__":
    raise SystemExit(main())
