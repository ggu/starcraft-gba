#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Build no-GDB Terran sight probe ROMs from a tree's executed product command.

Usage: build_probe.py <tree> <out-dir> <tape-header>...

The tree's build/starcraft-terran.elf is removed and rebuilt with a normal
`make` (never `make -B`, which would rerun private importers). The single
compile/link command Make echoes is the executed argv, saved as
<out-dir>/product-argv.json. Each translation unit on it is compiled
separately with the same ordered flags (the driver does the same per unit),
with terran_sight_probe.c in place of common/main.c. The state the probe reads
is made global in game.o, and the objects are linked with the product's link
options in their original order.
"""
import json
import shlex
import subprocess
import sys
from pathlib import Path

ELF = "build/starcraft-terran.elf"
SOURCES = (".c", ".s", ".S")
STATE = ("u", "b", "visible", "explored", "minerals", "gas", "frame", "screen", "won", "lost",
         "race_selected_mission")
tree, out_dir = Path(sys.argv[1]).resolve(), Path(sys.argv[2]).resolve()
tapes = [Path(t).resolve() for t in sys.argv[3:]]
probe = Path(__file__).resolve().parent / "terran_sight_probe.c"
out_dir.mkdir(parents=True, exist_ok=True)
(tree / ELF).unlink(missing_ok=True)
make = subprocess.run(["make", ELF], cwd=tree, capture_output=True, text=True)
if make.returncode:
    raise SystemExit(make.stdout[-4000:] + make.stderr[-4000:])
commands = [line for line in make.stdout.splitlines()
            if line.split() and line.split()[0].endswith("gcc") and f"-o {ELF}" in line]
if len(commands) != 1:
    raise SystemExit(f"expected one executed compile/link command, found {len(commands)}")
argv = shlex.split(commands[0])
(tree / "build/starcraft-terran.map").unlink(missing_ok=True)
(out_dir / "product-argv.json").write_text(json.dumps(argv, indent=1) + "\n")

link_only = lambda a: a.startswith(("-Wl,", "-T", "-l")) or a == "-nostdlib"
units = [a for a in argv[1:] if a.endswith(SOURCES)]
compile_flags, skip = [], False
for a in argv[1:]:
    if skip:
        skip = False
    elif a == "-o":
        skip = True
    elif a not in units and not link_only(a):
        compile_flags.append(a)
objects = {}
obj_dir = out_dir / "obj"
obj_dir.mkdir(exist_ok=True)
for unit in units:
    if unit == "common/main.c":
        continue
    obj = obj_dir / (unit.replace("/", "_") + ".o")
    subprocess.run([argv[0], *compile_flags, "-c", unit, "-o", str(obj)], cwd=tree, check=True,
                   capture_output=True)
    objects[unit] = obj
game_o = objects["starcraft/game.c"]
subprocess.run(["arm-none-eabi-objcopy", *[f"--globalize-symbol={s}" for s in STATE], str(game_o)],
               check=True)
sizes = {}
for line in subprocess.run(["arm-none-eabi-nm", "-S", str(game_o)], capture_output=True,
                           text=True, check=True).stdout.splitlines():
    parts = line.split()
    if len(parts) == 4 and parts[3] in ("u", "b", "visible"):
        sizes[parts[3]] = int(parts[1], 16)
size_flags = [f"-DPROBE_U_BYTES={sizes['u']}", f"-DPROBE_B_BYTES={sizes['b']}",
              f"-DPROBE_GRID_BYTES={sizes['visible']}"]
(out_dir / "units.json").write_text(json.dumps({"compile_flags": compile_flags, "units": units,
                                                "probe_sizes": sizes}, indent=1) + "\n")
for tape in tapes:
    out = out_dir / tape.stem
    probe_o = obj_dir / f"probe-{tape.stem}.o"
    subprocess.run([argv[0], *compile_flags, *size_flags, f'-DTAPE_HEADER="{tape}"', "-c",
                    str(probe), "-o", str(probe_o)], cwd=tree, check=True, capture_output=True)
    link, skip = [], False
    for i, a in enumerate(argv):
        if skip:
            skip = False
        elif a == "-o":
            link += ["-o", f"{out}.elf"]
            skip = True
        elif a == "common/main.c":
            link.append(str(probe_o))
        elif a in objects:
            link.append(str(objects[a]))
        elif a == "-Wl,-Map,build/starcraft-terran.map":
            link.append(f"-Wl,-Map,{out}.map")
        else:
            link.append(a)
    (out_dir / f"{tape.stem}.link.json").write_text(json.dumps(link, indent=1) + "\n")
    subprocess.run(link, cwd=tree, check=True, capture_output=True)
    subprocess.run(["arm-none-eabi-objcopy", "-O", "binary", f"{out}.elf", f"{out}.gba"], check=True)
    subprocess.run([sys.executable, "tools/fix_rom.py", f"{out}.gba", "SC TERRAN 10", "SCT8"],
                   cwd=tree, check=True, stdout=subprocess.DEVNULL)
    Path(f"{out}.map").unlink(missing_ok=True)
    print(f"{out}.gba")
