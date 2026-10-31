#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Record what one Make-built ARM ELF was actually compiled and linked from.

The target ELF is removed and rebuilt through `make` (never `make -B`, which
would also re-run private asset generators). The command Make echoes and runs
is the executed compile/link argv. For each translation unit on that command,
a dependency pass reruns the same argv with `-M -MF` in place of the output,
so the recorded dependency list comes from the same ordered `-D`/`-I` vector.
Every dependency, every `.incbin` blob named by a `-D...=` path, and the ELF
and optional ROM outputs are hashed. Output JSON names local paths and private
input hashes: keep it in an ignored folder.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import pathlib
import re
import shlex
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
SOURCE_SUFFIXES = (".c", ".s", ".S")


def sha256(path: pathlib.Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def executed_command(make_args: list[str], elf: str) -> tuple[list[str], str]:
    target = ROOT / elf
    if target.exists():
        target.unlink()
    proc = subprocess.run(["make", *make_args, elf], cwd=ROOT, capture_output=True, text=True)
    if proc.returncode:
        raise SystemExit(f"make failed:\n{proc.stdout[-4000:]}\n{proc.stderr[-4000:]}")
    commands = [line for line in proc.stdout.splitlines()
                if f"-o {elf}" in line and line.split()[0].endswith("gcc")]
    if len(commands) != 1:
        raise SystemExit(f"expected one compile/link command for {elf}, found {len(commands)}")
    return shlex.split(commands[0]), proc.stdout


def dependency_pass(argv: list[str], unit: str, scratch: pathlib.Path) -> list[str]:
    """Rerun argv for one unit, emitting its dependency list instead of code."""
    if unit.endswith(".s"):
        return [unit]  # plain assembly is not preprocessed
    sources = [a for a in argv[1:] if a.endswith(SOURCE_SUFFIXES)]
    keep, skip = [], False
    for a in argv[1:]:
        if skip:
            skip = False
            continue
        if a == "-o":
            skip = True
            continue
        if a in sources or a.startswith(("-Wl,", "-T", "-l")) or a == "-nostdlib":
            continue
        keep.append(a)
    depfile = scratch / (pathlib.Path(unit).name + ".d")
    subprocess.run([argv[0], *keep, "-M", "-MF", str(depfile), unit], cwd=ROOT, check=True,
                   capture_output=True)
    text = depfile.read_text().replace("\\\n", " ")
    deps = text.split(":", 1)[1].split()
    depfile.unlink()
    return deps


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("elf", help="Make target, for example build/starcraft-zerg-z7.elf")
    parser.add_argument("out", type=pathlib.Path)
    parser.add_argument("--rom", help="optional ROM built from the ELF, already up to date")
    parser.add_argument("--make-arg", action="append", default=[],
                        help="extra make argument (repeatable), e.g. Z7_ARM_CUT_TICK=403482")
    args = parser.parse_args()
    argv, make_log = executed_command(args.make_arg, args.elf)
    if args.rom:
        rom = subprocess.run(["make", *args.make_arg, args.rom], cwd=ROOT,
                             capture_output=True, text=True)
        if rom.returncode:
            raise SystemExit(rom.stdout + rom.stderr)
    scratch = args.out.parent
    scratch.mkdir(parents=True, exist_ok=True)
    units = [a for a in argv[1:] if a.endswith(SOURCE_SUFFIXES)]
    flags = [a for a in argv[1:] if re.match(r"-[DI]", a)]
    dependencies: dict[str, list[str]] = {}
    for unit in units:
        dependencies[unit] = dependency_pass(argv, unit, scratch)
    blobs = sorted({m.group(1) for a in flags
                    for m in [re.match(r'-D\w+="?([^"]+\.(?:bin|raw))"?$', a)] if m})
    inputs = sorted({d for deps in dependencies.values() for d in deps} | set(blobs))
    record = {
        "target": args.elf,
        "make_args": args.make_arg,
        "git_head": subprocess.run(["git", "rev-parse", "HEAD"], cwd=ROOT, capture_output=True,
                                   text=True).stdout.strip(),
        "git_status_porcelain": subprocess.run(["git", "status", "--porcelain"], cwd=ROOT,
                                               capture_output=True, text=True).stdout.splitlines(),
        "compiler": subprocess.run([argv[0], "--version"], capture_output=True,
                                   text=True).stdout.splitlines()[0],
        "executed_argv": argv,
        "argv_sha256": hashlib.sha256("\0".join(argv).encode()).hexdigest(),
        "ordered_define_include_vector": flags,
        "translation_units": units,
        "dependencies": dependencies,
        "input_sha256": {path: sha256(ROOT / path) for path in inputs},
        "elf_sha256": sha256(ROOT / args.elf),
        "rom": args.rom,
        "rom_sha256": sha256(ROOT / args.rom) if args.rom else None,
    }
    args.out.write_text(json.dumps(record, indent=2) + "\n")
    print(json.dumps({k: record[k] for k in ("target", "git_head", "argv_sha256", "elf_sha256",
                                             "rom_sha256")} |
                     {"flags": len(flags), "units": len(units), "inputs": len(inputs)}, indent=2))
    return 0


if __name__ == "__main__":
    sys.exit(main())
