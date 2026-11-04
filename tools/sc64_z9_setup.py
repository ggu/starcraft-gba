#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Preparation and readiness checks for the explicit nine-mission candidate.

The dev wrapper prepares its inherited Z7 inputs first. This layer authenticates
the two later source scenarios and builds the additional private bank/programs.
It does not change the canonical Zerg alias or migrate existing saves.
"""
from __future__ import annotations

from pathlib import Path
import re
import sys
from typing import Callable

TOOLS = Path(__file__).resolve().parent
if str(TOOLS) not in sys.path:
    sys.path.insert(0, str(TOOLS))

import sc64_z8_pack as z8
import sc64_z8_program_pack as z8_program
import sc64_zerg_late_pack as late
import sc64_zerg_late_program_pack as late_program

BANK = Path(".local/sc64/zerg-z8/generated/sc64_race_sprites_generated.h")
BANK_MARKERS = (
    "SC64_ZP_CAMPAIGN_SPRITE_SET_COMPLETE", "SC64_Z5_SPRITE_SET_COMPLETE",
    "SC64_Z5_DOODAD_SPRITE_SET_COMPLETE", "SC64_Z6_SPRITE_SET_COMPLETE",
    "SC64_Z6_BUILDING_SPRITE_SET_COMPLETE", "SC64_P6_SPRITE_SET_COMPLETE",
    "SC64_P7_SPRITE_SET_COMPLETE", "SC64_Z8_BUILDING_SPRITE_SET_COMPLETE",
    "SC64_ZERG_LATE_ART_COMPLETE", "SC64_RACE_UNIT_WORD_SPRITE_METADATA",
)


def paths(root: Path, mission: str) -> tuple[Path, Path]:
    lower = mission.lower()
    return (root / f".local/build/{lower}-source/descriptor.json",
            root / f"build/generated/{lower}/sc64_{lower}_program_generated.h")


def authenticated(root: Path, mission: str) -> dict:
    descriptor, _ = paths(root, mission)
    source = root / ".local/sc64/zp-stage/generated/source_maps"
    rom = root / ".local/sc64/rom/StarCraft64-USA.z64"
    if mission == "Z8":
        return z8.validate_descriptor_file(descriptor, source, rom)
    return late.validate_descriptor_file(mission, descriptor, source, rom)


def bank_ready(path: Path) -> bool:
    try:
        text = path.read_text(encoding="ascii")
    except (OSError, UnicodeError):
        return False
    return all(re.search(r"^#define\s+" + marker + r"\s+1(?:u)?\s*$",
                         text, re.MULTILINE) for marker in BANK_MARKERS)


def readiness(root: Path) -> list[tuple[str, Path, bool]]:
    """Read only; reject stale source descriptors and edited program tables."""
    rows = [("Expanded Zerg source sprite bank", root / BANK, bank_ready(root / BANK))]
    for mission in ("Z8", "Z9"):
        descriptor, header = paths(root, mission)
        valid, program = False, False
        try:
            data = authenticated(root, mission)
            expected = z8_program.render(data) if mission == "Z8" else late_program.render(data)
            valid = True
            program = header.read_text(encoding="ascii") == expected
        except (OSError, ValueError, KeyError, TypeError, UnicodeError):
            pass
        rows.extend(((f"Authenticated {mission} descriptor", descriptor, valid),
                     (f"Authenticated {mission} trigger program", header, program)))
    return rows


def prepare(root: Path, run: Callable[[list[str]], None]) -> None:
    """Use the already verified, privately retained ROM and source CHKs.

    Existing descriptors must verify before reuse: the packers deliberately
    refuse overwriting them. Headers and sprite outputs are regenerable.
    """
    py = sys.executable
    rom = root / ".local/sc64/rom/StarCraft64-USA.z64"
    source = root / ".local/sc64/zp-stage/generated/source_maps"
    for mission in ("Z8", "Z9"):
        descriptor, header = paths(root, mission)
        tool = "sc64_z8_pack.py" if mission == "Z8" else "sc64_zerg_late_pack.py"
        args = [py, str(root / "tools" / tool)]
        if descriptor.exists():
            args += ["verify", "--descriptor", str(descriptor)]
        else:
            args += ["pack", "--out", str(descriptor)]
        if mission != "Z8":
            args += ["--mission", mission]
        run(args + ["--source-root", str(source), "--rom", str(rom)])
        generator = "sc64_z8_program_pack.py" if mission == "Z8" else "sc64_zerg_late_program_pack.py"
        run([py, str(root / "tools" / generator), str(descriptor), str(header)])
    run([py, str(root / "tools/sc64_race_sprites.py"), "--rom", str(rom),
         "--base-only", "--include-zp-campaign-actors", "--include-z5-actors",
         "--include-z5-doodads", "--include-z6-actors", "--include-z6-buildings",
         "--include-p6-actors", "--include-p7-actors", "--include-z8-buildings",
         "--include-zerg-late-art", "--prepack-unit-words",
         "--out", str((root / BANK).parent / "sprites"),
         "--header", str(root / BANK)])
    missing = [label for label, _, ready in readiness(root) if not ready]
    if missing:
        raise RuntimeError("Z9 preparation is incomplete: " + ", ".join(missing))


def status(root: Path) -> int:
    rows = readiness(root)
    for label, path, ready in rows:
        print(f"{'READY  ' if ready else 'MISSING'} {label}: {path}")
    return int(any(not ready for _, _, ready in rows))
