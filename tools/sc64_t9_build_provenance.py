#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Capture the executed builds of the canonical Terran and T9 replay ROMs.

Both ELFs are relinked through their real Makefile recipes with the compiler
driver wrapped. The driver runs with -v, -MD and -wrapper, which only add
logging: every driver and subcommand argv (cc1, as, collect2) is recorded
verbatim and in order, and each cc1 writes its own raw dependency file instead
of every unit overwriting one. Every file those dependency files, the commands
and the link maps name is then hashed. Every output is a .txt file, so the
bundle passes tools/check_source_tree.py when committed.

The host controller binary, trace and tape are held with make -o, and a dry
run refuses any recipe other than the two links, size, objcopy and
fix_rom.py, so no private input or trace is regenerated. The rebuilt ROMs are
the qualified build only if they match the recorded ROM hashes (--expect).
"""
from __future__ import annotations
import argparse
import hashlib
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
PREFIX = "arm-none-eabi-"
ROMS = {
    "starcraft-terran": "build/StarCraft-Terran-GBA.gba",
    "starcraft-terran-t9-arm-replay": "build/StarCraft-Terran-T9-ARM-Replay.gba",
}
HELD = ("build/test-sc64-t9-campaign-run", "build/t9-arm/t9-controller.json",
        "build/t9-arm/sc64_t9_tape_generated.h")
# Inputs a recipe reads that no .d file names.
RECIPE_INPUTS = {
    "starcraft-terran": ("Makefile", "tools/fix_rom.py"),
    "starcraft-terran-t9-arm-replay": ("Makefile", "tools/fix_rom.py") + HELD[1:],
}

DRIVER = r"""#!/bin/sh
# Logs one compiler-driver argv, then runs it with logging-only options.
name=cc prev=
for a do [ "$prev" = -o ] && name=$(basename "$a" .elf); prev=$a; done
T9_PROV_LOG="$T9_PROV_DIR/$name"; export T9_PROV_LOG
mkdir -p "$T9_PROV_LOG/deps"
printf '%s\0' "$@" > "$T9_PROV_LOG/00-driver.argv"
exec "$T9_PROV_CC" -v -MD -wrapper "$T9_PROV_SUB" "$@" 2> "$T9_PROV_LOG/driver-v.log"
"""

SUB = r"""#!/bin/sh
# Runs one driver subcommand as given, except that a cc1 -MD file is renamed
# per unit, and logs the argv it executes.
log="$T9_PROV_LOG"
seq=$(printf %02d "$(ls "$log" | grep -c '\.argv$')")
tool=$(basename "$1") base= src= prev=
for a do
  [ "$prev" = -dumpbase ] && base=$a
  case "$a" in /*|-*) ;; *.c|*.S|*.s) [ -z "$src" ] && [ -f "$a" ] && src=$(basename "$a") ;; esac
  prev=$a
done
# Name the step after its unit; as on a temporary .s follows the last cc1.
[ -n "$base" ] || base=$src
[ "$tool" = collect2 ] && base=link
if [ -n "$base" ]; then echo "$base" > "$log/unit"; else base=$(cat "$log/unit" 2>/dev/null || echo "$tool"); fi
reset=1 prev=
for a do
  [ $reset = 1 ] && { set --; reset=0; }
  orig=$a
  [ "$prev" = -MD ] && a="$log/deps/$base.d"
  set -- "$@" "$a"
  prev=$orig
done
printf '%s\0' "$@" > "$log/$seq-$tool-$base.argv"
exec "$@"
"""


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        for block in iter(lambda: f.read(1 << 20), b""):
            h.update(block)
    return h.hexdigest()


def read_argv(path: Path) -> list[str]:
    args = path.read_bytes().split(b"\0")[:-1]
    out = [a.decode() for a in args]
    if any("\n" in a for a in out):
        raise ValueError(f"{path}: argument contains a newline")
    return out


def dep_prerequisites(text: str) -> list[str]:
    """Prerequisites of a make dependency file, in order, unescaped."""
    out = []
    for rule in text.replace("\\\n", " ").splitlines():
        if ":" not in rule:
            continue
        rhs = re.split(r"(?<![\\$]):(?=\s|$)", rule, maxsplit=1)[-1]
        for tok in re.split(r"(?<!\\)\s+", rhs.strip()):
            if tok:
                out.append(tok.replace("\\ ", " ").replace("\\#", "#").replace("$$", "$"))
    return out


def ordered_flags(args: list[str]) -> list[str]:
    """The -D/-U/-I/-iquote/-isystem/-include options of one argv, in order."""
    out, i = [], 0
    joined = ("-D", "-U", "-I")
    separate = ("-D", "-U", "-I", "-iquote", "-isystem", "-idirafter", "-include", "-imacros")
    while i < len(args):
        a = args[i]
        if a in separate and i + 1 < len(args):
            out.append(f"{a} {args[i + 1]}")
            i += 2
            continue
        if a.startswith(joined):
            out.append(a)
        i += 1
    return out


def rel(path: str) -> str:
    p = Path(path)
    if not p.is_absolute():
        return path
    try:
        return str(p.resolve().relative_to(ROOT.resolve()))
    except ValueError:
        return str(p)


def run(cmd, **kw) -> str:
    return subprocess.run(cmd, cwd=ROOT, check=True, capture_output=True, text=True, **kw).stdout


def make_cmd(driver: Path) -> list[str]:
    held = [x for h in HELD for x in ("-o", h)]
    return ["make", f"CC={driver}", *held, *ROMS.values()]


def guard_dry_run(driver: Path) -> None:
    allowed = (str(driver) + " ", PREFIX + "size ", PREFIX + "objcopy ", "python3 tools/fix_rom.py ")
    for line in run(make_cmd(driver)[:1] + ["-n"] + make_cmd(driver)[1:]).splitlines():
        if line and not line.startswith(allowed):
            raise SystemExit(f"refusing: make would also run: {line[:160]}")


def capture(out: Path, expect: dict[str, str], retained: dict[str, Path]) -> int:
    out.mkdir(parents=True, exist_ok=True)
    tmp = Path(tempfile.mkdtemp(prefix="t9-prov-"))
    try:
        driver, sub, log = tmp / "cc", tmp / "sub", tmp / "log"
        driver.write_text(DRIVER)
        sub.write_text(SUB)
        driver.chmod(0o755)
        sub.chmod(0o755)
        log.mkdir()
        for name in ROMS:
            (ROOT / f"build/{name}.elf").unlink(missing_ok=True)
        guard_dry_run(driver)
        env = dict(os.environ, T9_PROV_DIR=str(log), T9_PROV_SUB=str(sub),
                   T9_PROV_CC=shutil.which(PREFIX + "gcc") or PREFIX + "gcc")
        made = subprocess.run(make_cmd(driver), cwd=ROOT, env=env, capture_output=True, text=True)
        (out / "make-log.txt").write_text(made.stdout.replace(str(driver), "CAPTURE_CC"))
        if made.returncode:
            sys.stderr.write(made.stderr[-4000:])
            return made.returncode
        status = 0
        for name, rom in ROMS.items():
            status |= collect(out / name, log / name, name, rom, expect.get(name),
                              retained.get(name))
        write_toolchain(out / "toolchain.txt")
        return status
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


def collect(dest: Path, log: Path, name: str, rom: str, expected: str | None,
            retained: Path | None) -> int:
    if dest.exists():
        shutil.rmtree(dest)
    (dest / "commands").mkdir(parents=True)
    (dest / "deps").mkdir()
    inputs: dict[str, str] = {}
    tools: set[str] = set()

    def note(path: str, kind: str) -> None:
        inputs.setdefault(rel(path), kind)

    flag_lines = []
    for argv_file in sorted(log.glob("*.argv")):
        args = read_argv(argv_file)
        stem = argv_file.stem
        (dest / "commands" / f"{stem}.args.txt").write_text("\n".join(args) + "\n")
        if stem == "00-driver":
            prev = ""
            for a in args:
                if prev not in ("-o",) and not a.startswith("-") and (ROOT / a).is_file():
                    note(a, "source")
                if a.startswith("-T"):
                    note(a[2:], "linker script")
                m = re.match(r'-DSC64_AUDIO_BLOB="(.*)"$', a)
                if m:
                    note(m.group(1), "incbin")
                prev = a
            continue
        tools.add(args[0])
        if "-MD" in args:
            flag_lines.append(f"[{stem}]")
            flag_lines += ordered_flags(args)
            flag_lines.append("")
    (dest / "tu-flags.txt").write_text("\n".join(flag_lines))
    for dep in sorted((log / "deps").glob("*.d")):
        shutil.copyfile(dep, dest / "deps" / f"{dep.name}.txt")
        for path in dep_prerequisites(dep.read_text()):
            note(path, "dependency")
    for line in (ROOT / f"build/{name}.map").read_text(errors="replace").splitlines():
        loaded = Path(line[5:].strip())
        # Skip the driver's temporary objects and "linker stubs".
        if line.startswith("LOAD ") and loaded.is_absolute() and loaded.is_file() \
                and not str(loaded).startswith(tempfile.gettempdir()):
            note(str(loaded), "library")
    for path in RECIPE_INPUTS[name]:
        note(path, "recipe")
    commands = [l for l in (log / "driver-v.log").read_text(errors="replace").splitlines()
                if l.startswith(" /")]
    (dest / "driver-v-commands.txt").write_text("\n".join(commands) + "\n")

    with (dest / "inputs-sha256.txt").open("w") as f:
        for path in sorted(inputs):
            f.write(f"{sha256(ROOT / path if not Path(path).is_absolute() else Path(path))}  {path}\n")
    with (dest / "inputs-kind.txt").open("w") as f:
        for path in sorted(inputs):
            f.write(f"{inputs[path]}\t{path}\n")
    tool_paths = set(tools) | {run([PREFIX + "gcc", "-print-prog-name=ld"]).strip()}
    tool_paths |= {shutil.which(PREFIX + t) or "" for t in ("gcc", "size", "objcopy")}
    with (dest / "toolchain-sha256.txt").open("w") as f:
        for path in sorted(os.path.realpath(p) for p in tool_paths if p):
            f.write(f"{sha256(Path(path))}  {path}\n")
    outputs = {p: sha256(ROOT / p) for p in (rom, f"build/{name}.elf", f"build/{name}.map")}
    with (dest / "outputs-sha256.txt").open("w") as f:
        for path, digest in outputs.items():
            f.write(f"{digest}  {path}\n")
    if retained:
        compare_retained(dest / "retained-elf-compare.txt", retained, ROOT / f"build/{name}.elf")
    print(f"{name}: {len(inputs)} inputs, ROM {outputs[rom]}")
    if expected and outputs[rom] != expected:
        print(f"{name}: ROM does not match expected {expected}", file=sys.stderr)
        return 1
    return 0


def compare_retained(path: Path, retained: Path, rebuilt: Path) -> None:
    """Where a retained ELF differs from the rebuilt one: bytes, sections, symbols."""
    a, b = retained.read_bytes(), rebuilt.read_bytes()
    lines = [f"retained {sha256(retained)}  {rel(str(retained))}",
             f"rebuilt  {sha256(rebuilt)}  {rel(str(rebuilt))}"]
    if len(a) != len(b):
        lines.append(f"sizes differ: {len(a)} != {len(b)}")
    else:
        offsets = [i for i in range(len(a)) if a[i] != b[i]]
        lines.append(f"differing bytes: {len(offsets)}")
        sections = run([PREFIX + "readelf", "-SW", str(rebuilt)])
        spans = [(int(m.group(2), 16), int(m.group(2), 16) + int(m.group(3), 16), m.group(1))
                 for m in re.finditer(r"\]\s+(\S+)\s+\S+\s+[0-9a-f]+\s+([0-9a-f]+)\s+([0-9a-f]+)", sections)]
        hit = sorted({name for o in offsets for lo, hi, name in spans if lo <= o < hi})
        lines.append("in sections: " + (", ".join(hit) or "none"))
    symbols = [run([PREFIX + "readelf", "-sW", str(p)]).splitlines() for p in (retained, rebuilt)]
    changed = [f"{x.split()[-1]} -> {y.split()[-1]}" for x, y in zip(*symbols) if x != y]
    lines.append(f"differing symbols: {len(changed)}" + "".join(f"\n  {c}" for c in changed))
    path.write_text("\n".join(lines) + "\n")


def write_toolchain(path: Path) -> None:
    lines = [run([PREFIX + "gcc", "--version"]).splitlines()[0],
             run([PREFIX + "as", "--version"]).splitlines()[0],
             run([PREFIX + "ld", "--version"]).splitlines()[0],
             run(["make", "--version"]).splitlines()[0],
             "Python " + sys.version.split()[0],
             "libgcc " + run([PREFIX + "gcc", "-mcpu=arm7tdmi", "-mthumb", "-print-libgcc-file-name"]).strip(),
             "commit " + run(["git", "rev-parse", "HEAD"]).strip()]
    dirty = run(["git", "status", "--porcelain", "--", "starcraft", "common", "tests", "tools", "Makefile"])
    lines.append("tracked changes: " + ("none" if not dirty.strip() else dirty.strip().replace("\n", "; ")))
    path.write_text("\n".join(lines) + "\n")


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--out", type=Path, default=ROOT / "build/t9-provenance")
    ap.add_argument("--expect", action="append", default=[], metavar="NAME=SHA256",
                    help="required ROM hash, e.g. starcraft-terran=685788ff...")
    ap.add_argument("--retained", action="append", default=[], metavar="NAME=ELF",
                    help="a retained ELF to compare byte for byte with the rebuilt one")
    args = ap.parse_args()
    expect = dict(e.split("=", 1) for e in args.expect)
    retained = {k: Path(v).resolve() for k, v in (r.split("=", 1) for r in args.retained)}
    return capture(args.out.resolve(), expect, retained)


if __name__ == "__main__":
    sys.exit(main())
