#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Link and verify every ROM target, then print one RAM-margin table.

A change that adds work RAM or shared compiler flags can break an older ROM
target that nobody builds. This tool builds each ROM that has a ``verify-*``
rule (``verify`` for the default ROM), continues past failures, and prints:

  target, __iwram_end, __bss_end, bytes left under the linker limits in
  common/gba.ld, and PASS / FAIL / SKIPPED

It exits non-zero if any ROM fails to link or verify. A ROM whose private
inputs are absent is SKIPPED (never PASS) with the first missing input; the
detection is the one ``tools/sc64_release.py`` uses (make's rule database is
read with ``make -p -q`` and no recipe runs). ``--require-all`` turns SKIPPED
into a failed run. Run it as ``make check-all-roms`` or directly.
"""
from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import time
from typing import Callable, Sequence

sys.path.insert(0, str(Path(__file__).resolve().parent))
import sc64_release as rel  # noqa: E402

ROOT = rel.ROOT
PASS, FAIL, SKIPPED = rel.PASS, rel.FAIL, rel.SKIPPED

# (verify make target, ROM). Explicit on purpose: tests/test_sc64_rom_check.py
# fails when a ``verify-*`` rule that builds a ROM is missing from this list.
# Aliases that build the same ROM as another entry (verify-zerg-z9 and
# verify-zerg, verify-protoss-p10 and verify-protoss) are listed so both names
# stay covered; the second is reported with ``same ROM as <first>`` and is not
# rebuilt.
ROM_TARGETS: tuple[tuple[str, str], ...] = (
    ("verify", "build/StarCraft-GBA.gba"),
    ("verify-sc64", "build/StarCraft64-Wasteland-GBA.gba"),
    ("verify-backwater", "build/StarCraft64-Backwater-GBA.gba"),
    ("verify-skirmish", "build/StarCraft64-Skirmish-GBA.gba"),
    ("verify-campaign", "build/StarCraft64-Terran-Campaign-GBA.gba"),
    ("verify-races", "build/StarCraft64-Races-GBA.gba"),
    ("verify-unified", "build/StarCraft64-GBA.gba"),
    ("verify-terran", "build/StarCraft-Terran-Transport1-GBA.gba"),
    ("verify-zerg-z3", "build/StarCraft-Zerg-GBA.gba"),
    ("verify-zerg-z4", "build/StarCraft-Zerg-Z4-GBA.gba"),
    ("verify-zerg-z6", "build/StarCraft-Zerg-Z6-GBA.gba"),
    ("verify-zerg-z7", "build/StarCraft-Zerg-Z7-Transport1-GBA.gba"),
    ("verify-zerg-z8", "build/StarCraft-Zerg-Z8-Transport1-GBA.gba"),
    ("verify-zerg-z9", "build/StarCraft-Zerg-Z9-Transport1-GBA.gba"),
    ("verify-zerg", "build/StarCraft-Zerg-Z9-Transport1-GBA.gba"),
    ("verify-zerg-z10", "build/StarCraft-Zerg-Z10-Transport1-GBA.gba"),
    ("verify-protoss-p3", "build/StarCraft-Protoss-GBA.gba"),
    ("verify-protoss-p6", "build/StarCraft-Protoss-P6-GBA.gba"),
    ("verify-protoss-p7", "build/StarCraft-Protoss-P7-Transport1-GBA.gba"),
    ("verify-protoss-p8", "build/StarCraft-Protoss-P8-Transport1-GBA.gba"),
    ("verify-protoss-p9", "build/StarCraft-Protoss-P9-Transport1-GBA.gba"),
    ("verify-protoss-p10", "build/StarCraft-Protoss-P10-Transport1-GBA.gba"),
    ("verify-protoss", "build/StarCraft-Protoss-P10-Transport1-GBA.gba"),
    ("verify-showcase", "build/StarCraft-Showcase-Transport1-GBA.gba"),
)

# Local inputs the one-command target supplies when they exist (see
# docs/DEVELOPMENT.md). Both are optional; --make-arg overrides them.
P4_SCENARIO_HEADER = ".local/sc64/p4/generated/sc64_p4_scenario_generated.h"
Z8_DESCRIPTOR_FILE = ".local/build/z9-inputs/z8-descriptor.json"


def default_make_args(root: Path = ROOT) -> list[str]:
    args: list[str] = []
    if (root / P4_SCENARIO_HEADER).is_file():
        args += ["-o", P4_SCENARIO_HEADER]
    if (root / Z8_DESCRIPTOR_FILE).is_file():
        args.append(f"Z8_DESCRIPTOR={Z8_DESCRIPTOR_FILE}")
    return args


def read_rules(make_args: Sequence[str], root: Path = ROOT) -> dict[str, list[str]]:
    with tempfile.TemporaryDirectory(prefix="sc64-rom-check-") as td:
        noop = Path(td) / "noop.mk"
        noop.write_text(rel._NOOP_MAKEFILE, encoding="utf-8")
        result = subprocess.run(
            ["make", "-p", "-q", "-s", "-f", "Makefile", "-f", str(noop), *make_args,
             "release-noop"], cwd=root, text=True, stdout=subprocess.PIPE,
            stderr=subprocess.PIPE, check=False)
    if result.returncode not in (0, 1) or "# Files" not in result.stdout:
        last = (result.stderr.strip().splitlines() or ["no output"])[-1]
        raise rel.ReleaseError(f"could not read the make database: {last}")
    return rel.parse_make_database(result.stdout)[0]


def elf_for(rom: str, rules: dict[str, list[str]]) -> str | None:
    return next((p for p in rules.get(rom, []) if p.endswith(".elf")), None)


def make_runner(command: list[str], log: Path, cwd: Path) -> int:
    return rel.default_runner(command, log, cwd)


def check_all(targets: Sequence[tuple[str, str]], make_args: Sequence[str], out: Path, *,
              root: Path = ROOT, runner: Callable[[list[str], Path, Path], int] = make_runner,
              prune: bool = False, lock_path: str | None = None,
              min_free_bytes: int = 4 * rel.GIB, free_bytes: Callable[[], int] | None = None,
              find_missing: Callable[[Sequence[str], Sequence[str]], list[str]] | None = None,
              rules: dict[str, list[str]] | None = None) -> list[dict[str, object]]:
    find_missing = find_missing or (lambda t, a: rel.find_missing_inputs(t, a, root))
    free_bytes = free_bytes or (lambda: __import__("shutil").disk_usage(root).free)
    rules = rules if rules is not None else read_rules(make_args, root)
    limits = rel.linker_limits(root)
    rows: list[dict[str, object]] = []
    first_for_rom: dict[str, str] = {}
    for verify, rom in targets:
        row: dict[str, object] = {"target": verify, "rom": rom, "status": PASS, "detail": "",
                                  "seconds": 0.0, "iwram_end": None, "bss_end": None,
                                  "iwram_left": None, "ewram_left": None, "sha256": None}
        rows.append(row)
        if rom in first_for_rom:
            # Same ROM as an earlier entry: its row is copied after that build.
            earlier = next(r for r in rows if r["target"] == first_for_rom[rom])
            for key in ("status", "iwram_end", "bss_end", "iwram_left", "ewram_left", "sha256"):
                row[key] = earlier[key]
            row["detail"] = f"same ROM as {first_for_rom[rom]}"
            continue
        first_for_rom[rom] = verify
        command = rel.make_command(make_args, [verify])
        row["command"] = " ".join(command)
        log = out / f"{verify}.log"
        row["log"] = str(log)
        missing = find_missing([verify], list(make_args))
        if missing:
            shown = ", ".join(missing[:2]) + (f" (+{len(missing) - 2} more)" if len(missing) > 2 else "")
            row["status"], row["detail"] = SKIPPED, f"missing: {shown}"
            continue
        free = free_bytes()
        if free < min_free_bytes:
            row["status"] = FAIL
            row["detail"] = f"refusing to build: {free / rel.GIB:.1f} GiB free"
            continue
        started = time.monotonic()
        with rel.build_lock(lock_path):
            rc = runner(command, log, root)
        row["seconds"] = round(time.monotonic() - started, 1)
        if rc != 0:
            row["status"] = FAIL
            row["detail"] = f"exit {rc}: {rel.first_error_line(log)}"
        # Margins are read from the ELF whenever it exists, including after a
        # verify failure, so a failing row still says how far over it is.
        elf = elf_for(rom, rules)
        elf_path = root / elf if elf else None
        if elf_path is not None and elf_path.is_file():
            margins = rel.ram_margins(elf_path, root)
            if margins:
                row["iwram_end"] = margins["__iwram_end"]["end"]
                row["bss_end"] = margins["__bss_end"]["end"]
                row["iwram_left"] = margins["__iwram_end"]["margin_bytes"]
                row["ewram_left"] = margins["__bss_end"]["margin_bytes"]
        elif row["status"] == PASS:
            row["status"], row["detail"] = FAIL, f"no ELF found for {rom}"
        info = rel.rom_info(root / rom)
        if info and rc == 0:
            row["sha256"] = info["sha256"]
        if prune:
            for path in (root / rom, elf_path):
                if path is not None and path.is_file():
                    path.unlink()
    row_limits = {"__iwram_end": limits["__iwram_end"], "__bss_end": limits["__bss_end"]}
    for row in rows:
        row["limits"] = row_limits
    return rows


def _hex(value: object) -> str:
    return f"0x{value:08x}" if isinstance(value, int) else "-"


def _num(value: object) -> str:
    return f"{value:,}" if isinstance(value, int) else "-"


def render_table(rows: Sequence[dict[str, object]], limits: dict[str, int]) -> str:
    header = ("target", "__iwram_end", "__bss_end", "IWRAM left", "EWRAM left", "status")
    body = []
    for row in rows:
        status = str(row["status"])
        detail = str(row["detail"])
        if detail:
            status += f" ({detail})"
        body.append((str(row["target"]), _hex(row["iwram_end"]), _hex(row["bss_end"]),
                     _num(row["iwram_left"]), _num(row["ewram_left"]), status))
    widths = [max(len(header[i]), *(len(r[i]) for r in body)) for i in range(5)]
    def line(cells: Sequence[str]) -> str:
        return "  ".join(cells[i].ljust(widths[i]) if i in (0, 5) else cells[i].rjust(widths[i])
                         for i in range(5)) + "  " + cells[5]
    lines = [line(header[:5] + (header[5],))]
    lines += [line(r) for r in body]
    lines.append(f"Limits (common/gba.ld): __iwram_end < 0x{limits['__iwram_end']:08x}, "
                 f"__bss_end < 0x{limits['__bss_end']:08x}.")
    return "\n".join(lines)


def exit_code(rows: Sequence[dict[str, object]], require_all: bool = False) -> int:
    for row in rows:
        if row["status"] == FAIL or (require_all and row["status"] == SKIPPED):
            return 1
    return 0


def select(targets: Sequence[tuple[str, str]], only: str | None) -> list[tuple[str, str]]:
    if not only:
        return list(targets)
    wanted = [w.strip() for w in only.split(",") if w.strip()]
    known = {t for t, _ in targets}
    unknown = [w for w in wanted if w not in known]
    if unknown:
        raise rel.ReleaseError("unknown target(s): " + ", ".join(unknown))
    return [(t, r) for t, r in targets if t in wanted]


def main(argv: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("--only", help="comma list of verify targets")
    parser.add_argument("--require-all", action="store_true", help="treat SKIPPED as failure")
    parser.add_argument("--make-arg", action="append", default=[], metavar="ARG",
                        help="extra make argument (repeatable); replaces the local-input defaults")
    parser.add_argument("--out", type=Path, help="log directory (default build/rom-check/<rev8>)")
    parser.add_argument("--prune", action="store_true",
                        help="delete each ROM and ELF after it is recorded (saves disk)")
    parser.add_argument("--build-lock", help="flock file (default: $SC64_BUILD_LOCK)")
    parser.add_argument("--min-free-gib", type=float, default=4.0)
    args = parser.parse_args(argv)
    try:
        targets = select(ROM_TARGETS, args.only)
        make_args = list(args.make_arg) if args.make_arg else default_make_args()
        revision, dirty = rel.source_state()
        out = args.out or ROOT / "build" / "rom-check" / revision[:8]
        rows = check_all(targets, make_args, out, prune=args.prune,
                         lock_path=args.build_lock or os.environ.get("SC64_BUILD_LOCK") or None,
                         min_free_bytes=int(args.min_free_gib * rel.GIB))
    except rel.ReleaseError as error:
        print(f"ERROR: {error}", file=sys.stderr)
        return 2
    limits = rel.linker_limits()
    table = render_table(rows, limits)
    out.mkdir(parents=True, exist_ok=True)
    (out / "rom-check.json").write_text(json.dumps(
        {"revision": revision, "dirty": dirty, "make_args": make_args, "rows": rows},
        indent=2) + "\n", encoding="utf-8")
    (out / "rom-check.txt").write_text(table + "\n", encoding="utf-8")
    print(table)
    print(f"Wrote {out / 'rom-check.json'}; logs in {out}")
    counts = {s: sum(1 for r in rows if r["status"] == s) for s in (PASS, FAIL, SKIPPED)}
    print(f"Overall: {counts[PASS]} PASS, {counts[FAIL]} FAIL, {counts[SKIPPED]} SKIPPED"
          + (" (SKIPPED means private inputs are absent; it is not a pass)" if counts[SKIPPED] else ""))
    return exit_code(rows, args.require_all)


if __name__ == "__main__":
    raise SystemExit(main())
