#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Build a no-GDB Terran sight probe ROM from a tree's product command.

Usage: build_probe.py <tree> <tape-header> <out-prefix>. The full Terran
product command comes from `make -n -B build/starcraft-terran.elf` in <tree>.
common/main.c is replaced by terran_sight_probe.c (which includes the tree's
game.c); every other flag, source and link option keeps the product's order.
"""
import subprocess
import sys
from pathlib import Path

tree, tape, out = Path(sys.argv[1]).resolve(), Path(sys.argv[2]).resolve(), Path(sys.argv[3]).resolve()
probe = Path(__file__).resolve().parent / "terran_sight_probe.c"
plan = subprocess.run(["make", "-n", "-B", "build/starcraft-terran.elf"], cwd=tree,
                      capture_output=True, text=True).stdout.splitlines()
cmd = next(line for line in plan
           if line.startswith("arm-none-eabi-gcc") and line.endswith("-o build/starcraft-terran.elf"))
probe_cmd = (cmd.replace("common/main.c", str(probe), 1)
                .replace(" starcraft/game.c ", " ", 1)
                .replace("build/starcraft-terran.map", f"{out}.map", 1)
                .replace("-o build/starcraft-terran.elf", f"-o {out}.elf", 1)
             + f" -DTAPE_HEADER='\"{tape}\"'")
Path(f"{out}.cmd").write_text(probe_cmd + "\n")
subprocess.run(["bash", "-c", probe_cmd], cwd=tree, check=True)
subprocess.run(["arm-none-eabi-objcopy", "-O", "binary", f"{out}.elf", f"{out}.gba"], check=True)
subprocess.run([sys.executable, "tools/fix_rom.py", f"{out}.gba", "SC TERRAN 10", "SCT8"], cwd=tree,
               check=True, stdout=subprocess.DEVNULL)
