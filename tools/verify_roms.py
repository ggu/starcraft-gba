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
    "--terran": ("StarCraft-Terran-GBA", "starcraft-terran", "SCT2", "SC TERRAN"),
    "--zerg": ("StarCraft-Zerg-GBA", "starcraft-zerg", "SCZ2", "SC ZERG"),
    "--protoss": ("StarCraft-Protoss-GBA", "starcraft-protoss", "SCP2", "SC PROTOSS"),
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
