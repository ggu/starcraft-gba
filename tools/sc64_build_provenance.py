#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Record the exact build provenance of make targets.

`record OUT TARGET...` deletes each target (which must be under build/),
rebuilds it with an ordinary `make`, keeps the command lines make actually
executed (with every -D/-I flag in its real order), then
reruns each compiler command with `-M` instead of its output and link options
to obtain the raw dependency rules. It writes:

- `commands.txt`: the executed commands, verbatim;
- `deps/<n>.txt`: the raw `-M` dependency output of compiler command n;
- `inputs.txt`: `sha256  path` for every dependency and every file named on
  a command line (sources, linker script, blobs named in -D values), sorted
  by path;
- `outputs.txt`: `sha256  path` for each target.

Private inputs are recorded by path and hash only; no contents are copied.
The record lives in the given output directory (an ignored build path).
It refuses to run if make would regenerate any input outside build/ (for
example a shared private header), so recording never rewrites inputs.
"""
from __future__ import annotations

import argparse
import hashlib
import pathlib
import shlex
import subprocess
import sys

COMPILERS = ('clang', 'gcc', 'cc', 'arm-none-eabi-gcc')
SOURCE_SUFFIXES = ('.c', '.s', '.S')
# Steps that only post-process or check a target in build/.
ALLOWED_TOOLS = ('tools/fix_rom.py', 'tools/verify_roms.py', 'tools/sc64_p8_arm_replay.py')


def sha256(path: pathlib.Path) -> str:
    h = hashlib.sha256()
    with path.open('rb') as f:
        for chunk in iter(lambda: f.read(1 << 20), b''):
            h.update(chunk)
    return h.hexdigest()


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


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest='cmd', required=True)
    rec = sub.add_parser('record', help='rebuild targets and record provenance')
    rec.add_argument('out', type=pathlib.Path)
    rec.add_argument('targets', nargs='+')
    args = parser.parse_args()
    return record(args.out, args.targets)


if __name__ == '__main__':
    raise SystemExit(main())
