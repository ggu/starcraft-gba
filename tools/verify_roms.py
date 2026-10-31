#!/usr/bin/env python3
"""Check the GBA header and size of a StarCraft build."""
from __future__ import annotations

import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
TARGETS = {
    "--sc64": ("StarCraft64-Wasteland-GBA", "starcraft64", "SC64"),
    "--backwater": ("StarCraft64-Backwater-GBA", "starcraft64-backwater", "SCB2"),
    "--skirmish": ("StarCraft64-Skirmish-GBA", "starcraft64-skirmish", "SCK1"),
    "--campaign": ("StarCraft64-Terran-Campaign-GBA", "starcraft64-campaign", "SCT1"),
    "--races": ("StarCraft64-Races-GBA", "starcraft64-races", "SCR3"),
    "--unified": ("StarCraft64-GBA", "starcraft64-unified", "SCU1"),
    "--terran": ("StarCraft-Terran-Transport1-GBA", "starcraft-terran", "SCT8", "SC TERRAN 10"),
    "--terran-t3": ("StarCraft-Terran-T3-GBA", "starcraft-terran-t3", "SCT3", "SC TERRAN T3"),
    "--zerg": ("StarCraft-Zerg-Z7-Transport1-GBA", "starcraft-zerg-z7", "SCZ7", "SC ZERG Z7"),
    "--zerg-z6": ("StarCraft-Zerg-Z6-GBA", "starcraft-zerg-z6", "SCZ6", "SC ZERG Z6"),
    "--zerg-z7": ("StarCraft-Zerg-Z7-Transport1-GBA", "starcraft-zerg-z7", "SCZ7", "SC ZERG Z7"),
    "--zerg-z8": ("StarCraft-Zerg-Z8-Transport1-GBA", "starcraft-zerg-z8", "SCZ8", "SC ZERG Z8"),
    "--zerg-z3": ("StarCraft-Zerg-GBA", "starcraft-zerg", "SCZ3", "SC ZERG Z3"),
    "--zerg-z4": ("StarCraft-Zerg-Z4-GBA", "starcraft-zerg-z4", "SCZ4", "SC ZERG Z4"),
    "--zerg-z5": ("StarCraft-Zerg-Z5-GBA", "starcraft-zerg-z5", "SCZ5", "SC ZERG Z5"),
    "--zerg-z5-consume": ("StarCraft-Zerg-Z5-Consume-GBA", "starcraft-zerg-z5-consume", "SCZD", "SC Z5 FEED"),
    "--zerg-z5-cloak": ("StarCraft-Zerg-Z5-Cloak-GBA", "starcraft-zerg-z5-cloak", "SCZC", "SC Z5 CLOAK"),
    "--zerg-z7-full-render": ("StarCraft-Zerg-Z7-Full-Render-Probe-GBA", "starcraft-zerg-z7-full-render-probe", "SCZR", "SC Z7 FULL36"),
    "--zerg-z7-steering": ("StarCraft-Zerg-Z7-Steering-Probe-GBA", "starcraft-zerg-z7-steering-probe", "SCZS", "SC Z7 STEP36"),
    "--protoss": ("StarCraft-Protoss-P7-Transport1-GBA", "starcraft-protoss-p7", "SCP7", "SC PROTOSS7"),
    "--protoss-p8": ("StarCraft-Protoss-P8-Transport1-GBA", "starcraft-protoss-p8", "SCP8", "SC PROTOSS8"),
    "--protoss-p6": ("StarCraft-Protoss-P6-GBA", "starcraft-protoss-p6", "SCP6", "SC PROTOSS6"),
    "--protoss-p3": ("StarCraft-Protoss-GBA", "starcraft-protoss", "SCP3", "SC PROTOSS3"),
    "--protoss-p4": ("StarCraft-Protoss-P4-GBA", "starcraft-protoss-p4", "SCP4", "SC PROTOSS4"),
    "--protoss-p4-storm": ("StarCraft-Protoss-P4-Storm-GBA", "starcraft-protoss-p4-storm", "SCP5", "SC P4 STORM"),
    "--protoss-p5": ("StarCraft-Protoss-P5-GBA", "starcraft-protoss-p5", "SCPA", "SC PROTOSS5"),
    "--protoss-p5-shuttle": ("StarCraft-Protoss-P5-Shuttle-GBA", "starcraft-protoss-p5-shuttle", "SCPB", "SC P5 CARGO"),
    "--t4-internal": ("StarCraft-T4-Internal-GBA", "starcraft-t4-replay", "SCT4", "SC64 T4 TEST"),
    "--terran-t4": ("StarCraft-Terran-T4-GBA", "starcraft-t4-combined", "SCT5", "SC TERRAN T4"),
}


def executable(name: str) -> str:
    prefix = os.environ.get("PREFIX", "arm-none-eabi-")
    command = shutil.which(prefix + name)
    if command is None:
        raise SystemExit(f"Required tool not found: {prefix}{name}")
    return command


def verify(name: str, elf_name: str, code: str, title: str | None = None) -> dict[str, object]:
    rom = ROOT / "build" / f"{name}.gba"
    data = rom.read_bytes()
    if not 192 <= len(data) <= 32 * 1024 * 1024:
        raise ValueError(f"{rom.name}: invalid ROM size {len(data)}")
    if data[0xB2] != 0x96:
        raise ValueError(f"{rom.name}: invalid Nintendo logo checksum byte")
    if data[0xBD] != (-sum(data[0xA0:0xBD]) - 0x19) & 0xFF:
        raise ValueError(f"{rom.name}: invalid header checksum")
    if data[0xAC:0xB0].decode("ascii", errors="replace") != code:
        raise ValueError(f"{rom.name}: expected game code {code}")
    if title is not None and data[0xA0:0xAC].decode("ascii", errors="replace").rstrip() != title:
        raise ValueError(f"{rom.name}: expected title {title!r}")
    if b"SRAM_V113" not in data:
        raise ValueError(f"{rom.name}: missing SRAM detection signature")
    size_output = subprocess.check_output(
        [executable("size"), str(ROOT / "build" / f"{elf_name}.elf")], text=True
    )
    return {
        "bytes": len(data),
        "sha256": hashlib.sha256(data).hexdigest(),
        "size": size_output.strip(),
    }


def main(argv: list[str]) -> int:
    selected = [flag for flag in TARGETS if flag in argv]
    if len(selected) > 1:
        raise SystemExit("Select only one optional ROM profile per verification run")
    if selected:
        entry = TARGETS[selected[0]]
        rom_name, elf_name, code = entry[:3]
        title = entry[3] if len(entry) > 3 else None
        targets = [(rom_name, elf_name, code, title)]
        output_name = selected[0][2:] + "-verification.json"
    else:
        targets = [("StarCraft-GBA", "starcraft", "SCGD", "SC GBA DEMO")]
        output_name = "verification.json"

    results = {}
    for rom_name, elf_name, code, title in targets:
        result = verify(rom_name, elf_name, code, title)
        results[rom_name] = result
        print(f"{rom_name}: {result['bytes']} bytes, SHA-256 {result['sha256']}")
    (ROOT / "build" / output_name).write_text(json.dumps(results, indent=2) + "\n")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main(sys.argv[1:]))
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        raise SystemExit(str(error))
