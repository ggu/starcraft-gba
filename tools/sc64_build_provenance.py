#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Record what Make-built targets were actually compiled and linked from.

Neither form ever runs `make -B`, which would also re-run private asset
generators. Both name local paths and private input hashes, never their
contents: keep their output in an ignored folder.

`ELF OUT [--rom ROM] [--make-arg ARG]...` removes one ARM ELF and rebuilds it
through `make`. The command Make echoes and runs is the executed compile/link
argv. For each translation unit on that command, a dependency pass reruns the
same argv with `-M -MF` in place of the output, so the recorded dependency
list comes from the same ordered `-D`/`-I` vector. Every dependency, every
`.incbin` blob named by a `-D...=` path, and the ELF and optional ROM outputs
are hashed into the JSON record OUT.

`record OUT TARGET...` deletes each target (which must be under build/),
rebuilds them with an ordinary `make`, keeps the command lines make actually
executed (with every -D/-I flag in its real order), then reruns each compiler
command with `-M` instead of its output and link options to obtain the raw
dependency rules. It refuses to run if make would regenerate any input outside
build/ (for example a shared private header). In the directory OUT it writes:

- `commands.txt`: the executed commands, verbatim;
- `deps/<n>.txt`: the raw `-M` dependency output of compiler command n;
- `inputs.txt`: `sha256  path` for every dependency and every file named on
  a command line (sources, linker script, blobs named in -D values), sorted
  by path;
- `outputs.txt`: `sha256  path` for each target.
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


COMPILERS = ('clang', 'gcc', 'cc', 'arm-none-eabi-gcc')
# Steps that only post-process or check a target in build/.
ALLOWED_TOOLS = ('tools/fix_rom.py', 'tools/verify_roms.py', 'tools/sc64_p8_arm_replay.py')


def is_compile(argv: list[str]) -> bool:
    return bool(argv) and pathlib.Path(argv[0]).name in COMPILERS and \
        any(a.endswith(SOURCE_SUFFIXES) for a in argv[1:])


def dependency_argv(argv: list[str], depfile: pathlib.Path) -> list[str]:
    """The same preprocessing flags, sources in order, emitting rules only."""
    out, skip = [argv[0]], False
    for a in argv[1:]:
        if skip:
            skip = False
            continue
        if a in ('-o', '-Wl,-Map'):
            skip = True
            continue
        if a.startswith(('-Wl,', '-l', '-T')) or a.startswith('-fsanitize'):
            continue
        out.append(a)
    return out + ['-M']


def parse_deps(text: str) -> list[str]:
    paths = []
    for rule in text.replace('\\\n', ' ').splitlines():
        if ':' not in rule:
            continue
        paths.extend(rule.split(':', 1)[1].split())
    return paths


def record(out: pathlib.Path, targets: list[str]) -> int:
    out.mkdir(parents=True, exist_ok=True)
    for old in (out / 'deps').glob('*.txt'):
        old.unlink()
    (out / 'deps').mkdir(exist_ok=True)
    for t in targets:
        if not t.startswith('build/'):
            raise SystemExit(f'{t}: targets must be under build/')
        pathlib.Path(t).unlink(missing_ok=True)
    plan = subprocess.run(['make', '-n', *targets], capture_output=True, text=True)
    for line in plan.stdout.splitlines():
        if ('python' in line or line.startswith('./')) and \
                not any(tool in line for tool in ALLOWED_TOOLS):
            raise SystemExit('make would also run an input generator; build inputs first:\n' + line)
    run = subprocess.run(['make', *targets], capture_output=True, text=True)
    if run.returncode:
        sys.stderr.write(run.stdout[-4000:] + run.stderr[-4000:])
        return run.returncode
    # Drop make's own messages and tool output (such as size tables).
    commands = [line for line in run.stdout.splitlines()
                if line and not line.startswith(('make', 'Makefile', ' ', '\t')) and
                not line[0].isdigit() and not line.startswith('StarCraft-')]
    (out / 'commands.txt').write_text('\n'.join(commands) + '\n')
    inputs: set[str] = set()
    for n, line in enumerate(commands):
        try:
            argv = shlex.split(line)
        except ValueError:
            continue
        if not is_compile(argv):
            continue
        inputs.update(a for a in argv[1:] if a.endswith(SOURCE_SUFFIXES))
        # Files named by flags rather than #include: the linker script, an
        # incbin'd blob in a -D value, and the like.
        for a in argv[1:]:
            for name in (a, a.split('=', 1)[-1].strip('"'), a[2:] if a.startswith('-T') else ''):
                if name and not name.startswith('-') and pathlib.Path(name).is_file():
                    inputs.add(name)
        depfile = out / 'deps' / f'{n}.txt'
        dep = subprocess.run(dependency_argv(argv, depfile), capture_output=True, text=True)
        if dep.returncode:
            sys.stderr.write(dep.stderr[-4000:])
            return dep.returncode
        depfile.write_text(dep.stdout)
        inputs.update(parse_deps(dep.stdout))
    rows = []
    for name in sorted(inputs):
        p = pathlib.Path(name)
        if p.is_file():
            rows.append(f'{sha256(p)}  {name}')
    (out / 'inputs.txt').write_text('\n'.join(rows) + '\n')
    outs = [f'{sha256(pathlib.Path(t))}  {t}' for t in targets if pathlib.Path(t).is_file()]
    (out / 'outputs.txt').write_text('\n'.join(outs) + '\n')
    print(f'{len(commands)} commands, {len(rows)} inputs, {len(outs)} outputs -> {out}')
    return 0


def record_main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(prog='sc64_build_provenance.py record', description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('out', type=pathlib.Path)
    parser.add_argument('targets', nargs='+')
    args = parser.parse_args(argv)
    return record(args.out, args.targets)



def main() -> int:
    if sys.argv[1:2] == ["record"]:
        return record_main(sys.argv[2:])
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
