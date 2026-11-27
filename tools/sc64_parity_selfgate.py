#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Self-gates and throughput for the GBA side of the parity harness.

Runs the binaries mk/parity/Makefile builds (tests/sc64_parity_run.c) in fresh
processes:

  gate   For each profile, the four self-gates:
           same_trace     the same scenario twice gives the same trace SHA-256
           trace_on_off   a build with the emitter and one without give the
                          same world hash and per-step world-hash chain
           route_purity   route_purity_hash() was unchanged around every
                          harness call (the runner aborts otherwise) and the
                          checks were made
           render_on_off  rendering after every update or not gives the same
                          world hash chain
         Writes <dir>/<profile>-selfgate.json with the flag vector and the
         SHA-256 of every generated input header each binary was built from.
  bench  Host updates per second, -O2 unsanitized and ASan, emitter on and off.

Exit status is non-zero when any gate fails or a binary reports a setup failure.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parent.parent



def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1 << 20), b""):
            digest.update(block)
    return digest.hexdigest()


def make_tokens(text: str) -> list[str]:
    """Split a Make prerequisite list, honouring clang's escapes (\\ , $$, \\#)."""
    tokens = []
    for raw in re.split(r"(?<!\\)\s+", text.strip()):
        if raw:
            tokens.append(re.sub(r"\\([ #\\])", r"\1", raw).replace("$$", "$"))
    return tokens


def dependency_tokens(dep_file: Path) -> list[str]:
    """Every path a clang -M dependency file names as a prerequisite, unresolved."""
    text = dep_file.read_text().replace("\\\n", " ")
    tokens: list[str] = []
    for line in text.splitlines():
        if ": " in line:
            tokens += make_tokens(line.partition(": ")[2])
    return tokens


def dependency_paths(dep_file: Path) -> list[Path]:
    """Every file a clang -M dependency file names, in order, deduplicated."""
    text = dep_file.read_text().replace("\\\n", " ")
    paths: list[Path] = []
    seen: set[Path] = set()
    for line in text.splitlines():
        if ":" not in line:
            continue
        _, _, rest = line.partition(": ")
        for token in make_tokens(rest):
            path = (ROOT / token).resolve() if not os.path.isabs(token) else Path(token).resolve()
            if path not in seen:
                seen.add(path)
                paths.append(path)
    return paths


_SYSTEM_DIRS: dict[str, list[Path]] = {}


def compiler_system_dirs(compiler: str) -> list[Path]:
    """The compiler's own header search directories (SDK, toolchain), from
    `clang -E -v`, plus its resource directory. Only files under these count as
    system inputs."""
    if compiler not in _SYSTEM_DIRS:
        result = subprocess.run([compiler, "-E", "-x", "c", "-v", "-"], input="", capture_output=True, text=True)
        dirs, inside = [], False
        for line in result.stderr.splitlines():
            if line.startswith("#include <...> search starts here"):
                inside = True
            elif line.startswith("End of search list"):
                inside = False
            elif inside:
                dirs.append(Path(os.path.normpath(line.strip().split(" (")[0])))
        if not dirs:
            raise SystemExit(f"could not read the system header directories of {compiler}")
        # Sanitizer ignore lists and other toolchain data live in the resource dir.
        resource = subprocess.run([compiler, "-print-resource-dir"], capture_output=True, text=True).stdout.strip()
        if resource:
            dirs.append(Path(os.path.normpath(resource)))
        _SYSTEM_DIRS[compiler] = dirs
    return _SYSTEM_DIRS[compiler]


def include_dirs(command: str) -> list[Path]:
    """The -isystem and -idirafter directories a compile command names. Headers
    found there are left out of -MM output even when generated."""
    argv = shlex.split(command)
    dirs = []
    for i, arg in enumerate(argv):
        for option in ("-isystem", "-idirafter"):
            if arg == option and i + 1 < len(argv):
                dirs.append(argv[i + 1])
            elif arg.startswith(option) and len(arg) > len(option):
                dirs.append(arg[len(option):])
    return [Path(os.path.normpath(d if os.path.isabs(d) else ROOT / d)) for d in dirs]


def generated_inputs(dep_file: Path, user_deps: Path, command: str = "") -> dict[str, dict[str, str]]:
    """SHA-256 of every non-system input a clang -M dependency file names.

    "generated" holds listed paths, as the build named them (before symlinks),
    under .local/ or build/, or outside this tree and also listed by the -MM
    pass `user_deps` (which leaves out system headers). "sources" holds the
    other in-tree paths, so an edit after the build is caught too. The rule
    does not depend on git state, so staging a file never changes it. A listed
    file that is missing is an error, never an omission."""
    user = {os.path.normpath(t if os.path.isabs(t) else ROOT / t) for t in dependency_tokens(user_deps)}
    explicit = include_dirs(command)
    system = compiler_system_dirs(shlex.split(command)[0]) if command else []
    generated: dict[str, str] = {}
    sources: dict[str, str] = {}
    for token in dict.fromkeys(dependency_tokens(dep_file)):
        named = Path(os.path.normpath(token if os.path.isabs(token) else ROOT / token))
        target = generated
        try:
            key = str(named.relative_to(ROOT))
            if not key.startswith((".local/", "build/")):
                target = sources
        except ValueError:
            key = str(named)
            # Outside the tree, only the compiler's own directories hold system
            # headers; anything else (an -isystem bank, a sibling it includes)
            # is hashed as generated.
            if key not in user and not any(named.is_relative_to(d) for d in explicit) and \
                    any(named.is_relative_to(d) for d in system):
                continue
        if not named.is_file():
            raise SystemExit(f"{dep_file}: input {named} is missing; rebuild with make")
        target[key] = sha256_file(named)
    return {"generated": dict(sorted(generated.items())), "sources": dict(sorted(sources.items()))}


PREPROCESSOR_SEPARATE = ("-include", "-imacros", "-iquote", "-isystem", "-idirafter", "-I", "-D", "-U")
PREPROCESSOR_JOINED = ("-D", "-U", "-I", "-iquote", "-isystem", "-idirafter")


def rom_flags(dry_run: str, elf: str) -> str:
    """The preprocessor options of the compiler command that links `elf`.

    `dry_run` is `make -n` output. Each line is tokenized as the shell would
    before looking for `-o elf`, so quoted macro values cannot hide it. Keeps
    -D, -U, -I, -iquote, -isystem, -idirafter (joined or separate) and
    -include, in order, as shell words; the ARM code-generation, warning,
    linker and source arguments are left to the host build."""
    commands = []
    for line in dry_run.splitlines():
        try:
            argv = shlex.split(line)
        except ValueError:
            continue
        if any(argv[i] == "-o" and argv[i + 1] == elf for i in range(len(argv) - 1)):
            commands.append(argv)
    if len(commands) != 1:
        raise SystemExit(f"rom-flags: expected one command that writes {elf}, found {len(commands)}")
    argv = commands[0]
    kept: list[str] = []
    i = 1
    while i < len(argv):
        arg = argv[i]
        if arg in PREPROCESSOR_SEPARATE and i + 1 < len(argv):
            kept += [arg, argv[i + 1]]
            i += 2
            continue
        if arg.startswith(PREPROCESSOR_JOINED):
            kept.append(arg)
        i += 1
    if not any(arg.startswith("-D") for arg in kept):
        raise SystemExit(f"rom-flags: the command for {elf} has no -D options")
    return shlex.join(kept)


def rom_flags_from_make(elf: str, make: str, out: Path) -> None:
    """Run the dry run itself, check its status, and rewrite `out` only when
    the options changed (so their timestamp drives rebuilds)."""
    result = subprocess.run(shlex.split(make) + ["-s", "-n", "-W", "starcraft/game.c", elf],
                            cwd=ROOT, capture_output=True, text=True)
    if result.returncode:
        sys.stderr.write(result.stderr[-4000:])
        raise SystemExit(f"rom-flags: make -n {elf} exited {result.returncode}")
    text = rom_flags(result.stdout, elf) + "\n"
    if not out.is_file() or out.read_text() != text:
        tmp = Path(str(out) + ".new")
        tmp.write_text(text)
        tmp.replace(out)


def provenance(binary: Path) -> dict:
    """The build's own record: command, and the input hashes taken at build
    time. Fails when an input changed since, so a stale binary never passes."""
    sidecars = {suffix: Path(str(binary) + suffix) for suffix in (".flags", ".d", ".mmd", ".inputs")}
    missing = [str(path) for path in sidecars.values() if not path.is_file()]
    if missing:
        raise SystemExit(f"{binary}: missing {', '.join(missing)}; rebuild with make")
    record = json.loads(sidecars[".inputs"].read_text())
    built = record["inputs"]["generated"]
    if record.get("binary_sha256") != sha256_file(binary):
        raise SystemExit(f"{binary}: its .inputs record belongs to another build; rebuild with make")
    command = record.get("compile_command", "")
    if not command or sidecars[".flags"].read_text().strip() != command:
        raise SystemExit(f"{binary}: its .flags file does not match the command recorded at build; rebuild")
    # Rediscover dependencies with the recorded command: a header added to an
    # earlier include directory can shadow a recorded one without touching it.
    target = str(binary.relative_to(ROOT)) if binary.is_relative_to(ROOT) else str(binary)  # as make named it
    fresh = subprocess.run(["sh", "-c", f'{command} -M -MP -MT "$1"', "sh", target],
                           cwd=ROOT, capture_output=True, text=True)
    if fresh.returncode or fresh.stdout != sidecars[".d"].read_text():
        raise SystemExit(f"{binary}: its dependencies resolve differently now; rebuild with make")
    now = generated_inputs(sidecars[".d"], sidecars[".mmd"], command)
    if record["inputs"] != now:
        old = {**record["inputs"]["generated"], **record["inputs"]["sources"]}
        new = {**now["generated"], **now["sources"]}
        changed = sorted(k for k in set(old) | set(new) if old.get(k) != new.get(k))
        raise SystemExit(f"{binary}: inputs changed since it was built ({', '.join(changed[:5])}); "
                         "rebuild with make")
    argv = shlex.split(command)
    return {
        "binary": str(binary.relative_to(ROOT)) if binary.is_relative_to(ROOT) else str(binary),
        "binary_sha256": sha256_file(binary),
        "compile_command": command,
        "flags": [arg for arg in argv[1:] if arg.startswith("-")],
        "generated_inputs_sha256": built,
        "source_inputs_sha256": record["inputs"]["sources"],
        "generated_inputs_combined_sha256": hashlib.sha256(
            "".join(f"{k}\0{v}\n" for k, v in built.items()).encode()).hexdigest(),
    }


def run(binary: Path, scenario: str, profile: str, steps: int, *, trace: Path | None = None,
        render: bool = True, chain: bool = True, verify_body: bool = True, checks: bool = True) -> dict:
    command = [str(binary), "--scenario", scenario, "--steps", str(steps), "--profile", profile]
    if not checks:
        command.append("--no-route-checks")
    if trace:
        command += ["--trace", str(trace)]
    if not render:
        command.append("--no-render")
    if not chain:
        command.append("--no-chain")
    result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
    lines = [line for line in result.stdout.splitlines() if line.startswith("{")]
    if result.returncode or not lines:
        sys.stderr.write(result.stderr[-4000:])
        raise RuntimeError(f"{' '.join(command)} exited {result.returncode}")
    report = json.loads(lines[-1])
    if trace:
        check_trace_complete(trace, report, verify_body)
        report["trace_sha256"] = sha256_file(trace)
        report["trace_bytes"] = trace.stat().st_size
    return report


FOOTER_FIELDS = ("steps", "host_ticks", "births", "removals", "damage", "sounds", "text_boxes",
                 "overflow", "write_errors", "world", "world_chain", "body_fnv")


# Fields after the tag, for every record kind whose shape is fixed.
RECORD_FIELDS = {b"S": 11, b"W": 3, b"U": 23, b"B": 22, b"+U": 3, b"-U": 4, b"+B": 3, b"-B": 4,
                 b"OU": 3, b"OB": 3, b"TU": 3, b"TB": 3, b"P": 5, b"M": 4, b"F": 3,
                 b"D": 4, b"DA": 6, b"A": 6, b"X": 4}
INTEGER = re.compile(rb"-?[0-9]+")
# The whole record at once: its tag, then exactly n space-separated integers (one regex call per
# line instead of one per field; whole-match traces have millions of fields).
RECORD_PATTERN = {tag: re.compile(re.escape(tag) + rb"(?: -?[0-9]+){%d}" % n) for tag, n in RECORD_FIELDS.items()}
V_PATTERN = re.compile(rb"V(?: -?[0-9]+){2}(?:(?: -?[0-9]+){4})?")


def record_ok(line: bytes) -> bool:
    """A record of a fixed-shape kind has its field count and integer fields."""
    tag = line.split(b" ", 1)[0]
    if tag in RECORD_PATTERN:
        return RECORD_PATTERN[tag].fullmatch(line) is not None
    if tag == b"V":
        return V_PATTERN.fullmatch(line) is not None
    parts = line.split(b" ")
    if tag in (b"FV", b"FE"):
        if len(parts) != 4 or not re.fullmatch(rb"[1-9][0-9]*", parts[1]) or \
                not re.fullmatch(rb"[1-9][0-9]*", parts[2]) or not re.fullmatch(rb"[0-9a-f]+", parts[3]):
            return False
        cells = int(parts[1]) * int(parts[2])
        return len(parts[3]) == 2 * ((cells + 7) // 8)
    return tag in (b"I", b"H", b"#sc64-parity-trace", b"")


def fnv1a64(data: bytes) -> int:
    value = 1469598103934665603
    for byte in data:
        value = ((value ^ byte) * 1099511628211) & 0xFFFFFFFFFFFFFFFF
    return value


def check_trace_complete(trace: Path, report: dict, verify_body: bool = True) -> None:
    """A trace counts only with a whole Z footer that agrees with the run report,
    no overflow and no write error, and (verify_body) one S line per step and a
    body hash that matches the footer."""
    with trace.open("rb") as handle:
        first = handle.readline()
        handle.seek(max(0, trace.stat().st_size - 4096))
        tail = handle.read().decode("utf-8", "replace").splitlines()
    with trace.open("rb") as handle:
        handle.seek(max(0, trace.stat().st_size - 1))
        newline = handle.read(1) == b"\n"
    footer = tail[-1] if tail else ""
    fields = dict(item.split("=", 1) for item in footer.split()[1:] if "=" in item)
    complete = (first == b"#sc64-parity-trace 1\n" and newline and footer.startswith("Z ") and
                all(name in fields for name in FOOTER_FIELDS) and
                fields["overflow"] == "0" and fields["write_errors"] == "0" and
                fields["world"] == report["world"] and fields["world_chain"] == report["world_chain"] and
                fields["steps"] == str(report["steps"]) and fields["host_ticks"] == str(report["host_ticks"]) and
                all(fields[name].isdigit() for name in FOOTER_FIELDS[:9]))
    if complete and verify_body:
        data = trace.read_bytes()
        body = data[:data.rfind(b"\nZ ") + 1]
        counts = {"S": 0, "D": 0, "A": 0, "X": 0, "+U": 0, "+B": 0, "-U": 0, "-B": 0}
        verbs_ok = verbs_failed = 0
        sequential = True
        keyframes: set[bytes] = set()
        group = None            # step of the current S group; None before S 1 or in a W group
        group_frames: dict[bytes, bytes] = {}
        for line in body.split(b"\n"):
            if not record_ok(line):
                sequential = False
            if line.startswith((b"S ", b"W ")):
                # A group that wrote keyframes must have written both, same size.
                if group_frames and (len(group_frames) != 2 or len(set(group_frames.values())) != 1):
                    sequential = False
                group_frames = {}
                group = int(line.split(b" ")[1]) if line.startswith(b"S ") else None
                if group == 2 and len(keyframes) != 2:
                    sequential = False
            # Keyframes come on step 1 (and again when the map size changes),
            # always inside an S group; fog deltas need both first.
            if line[:3] in (b"FV ", b"FE "):
                if group is None or (not keyframes and group != 1) or line[:2] in group_frames:
                    sequential = False
                group_frames[line[:2]] = b" ".join(line.split(b" ")[1:3])
                keyframes.add(line[:2])
            elif line.startswith(b"F ") and (len(keyframes) != 2 or group is None):
                sequential = False
            tag = line.split(b" ", 1)[0].decode("ascii", "replace")
            if tag in counts:
                counts[tag] += 1
                if tag == "S" and line.split(b" ")[1] != str(counts["S"]).encode():
                    sequential = False
            if tag == "I":
                parts = line.split(b" ", 3)
                if (len(parts) != 4 or not re.fullmatch(rb"[a-z_]+", parts[1]) or
                        parts[2] not in (b"0", b"-1") or not parts[3].strip()):
                    sequential = False
                elif parts[2] == b"0":
                    verbs_ok += 1
                else:
                    verbs_failed += 1
        if group_frames and (len(group_frames) != 2 or len(set(group_frames.values())) != 1):
            sequential = False
        complete = (sequential and len(keyframes) == 2 and counts["S"] == report["steps"] and
                    counts["D"] == int(fields["damage"]) and counts["A"] == int(fields["sounds"]) and
                    counts["X"] == int(fields["text_boxes"]) and
                    counts["+U"] + counts["+B"] == int(fields["births"]) and
                    counts["-U"] + counts["-B"] == int(fields["removals"]) and
                    verbs_ok == report["verbs"] and verbs_failed == report["setup_failures"] and
                    f"{fnv1a64(body):016x}" == fields["body_fnv"])
    if not complete:
        raise RuntimeError(f"{trace}: incomplete trace (footer {footer[:160]!r})")


def parse_profiles(values: list[str]) -> list[tuple[str, str]]:
    profiles = []
    for value in values:
        name, _, scenario = value.partition(":")
        if not re.fullmatch(r"[a-z0-9][a-z0-9-]*", name) or scenario not in ("t1", "skirmish"):
            raise SystemExit(f"--profile wants NAME:t1 or NAME:skirmish, got {value!r}")
        profiles.append((name, scenario))
    return profiles


def same_configuration(traced: dict, plain: dict) -> bool:
    """The traced and plain builds differ only by -DSC64_PARITY_TRACE, and were
    built from the same generated inputs."""
    def without_emitter(build: dict) -> list[str]:
        return [arg for arg in shlex.split(build["compile_command"]) if arg != "-DSC64_PARITY_TRACE"]
    return (without_emitter(traced) == without_emitter(plain) and
            "-DSC64_PARITY_TRACE" in shlex.split(traced["compile_command"]) and
            traced["generated_inputs_sha256"] == plain["generated_inputs_sha256"])


def gate(args: argparse.Namespace) -> int:
    directory = (ROOT / args.dir).resolve()
    failed = 0
    profiles = parse_profiles(args.profile)
    for name, _ in profiles:  # a failed rerun must not leave an old pass behind
        (directory / f"{name}-selfgate.json").unlink(missing_ok=True)
    for name, scenario in profiles:
        out = directory / f"{name}-selfgate.json"
        traced = directory / f"{name}-{args.kind}-trace"
        plain = directory / f"{name}-{args.kind}-plain"
        # Provenance is taken before the runs and must be unchanged after, so
        # a rebuild in between can never lend these results to other binaries.
        builds = {"trace": provenance(traced), "plain": provenance(plain)}
        with tempfile.TemporaryDirectory(prefix="sc64-parity-") as scratch:
            first_trace = Path(scratch) / "first.trace"
            second_trace = Path(scratch) / "second.trace"
            first = run(traced, scenario, name, args.steps, trace=first_trace)
            second = run(traced, scenario, name, args.steps, trace=second_trace)
            without = run(plain, scenario, name, args.steps)
            unrendered = run(plain, scenario, name, args.steps, render=False)
            if args.keep_trace:
                keep = directory / f"{name}-{scenario}.trace"
                keep.write_bytes(first_trace.read_bytes())
        gates = {
            "same_trace": first["trace_sha256"] == second["trace_sha256"] and
                          first["world_chain"] == second["world_chain"],
            "trace_on_off": first["world"] == without["world"] and
                            first["world_chain"] == without["world_chain"] and
                            first["steps"] == without["steps"],
            "route_purity": all(r["route_checks_on"] == 1 and r["route_checks"] > r["steps"] and
                                r["verb_route_checks"] >= r["verbs"] and
                                # every step but the first (the launch's key release) is dispatched
                                r["dispatch_route_checks"] >= r["steps"] - 1
                                for r in (first, second, without, unrendered)),
            # Every run must reach the requested length: a game that ends early
            # would let four short runs agree.
            # The traced build carries the emitter and hooks; the plain one has none.
            "builds_differ": first["trace_compiled"] == 1 and second["trace_compiled"] == 1 and
                             without["trace_compiled"] == 0 and unrendered["trace_compiled"] == 0 and
                             same_configuration(builds["trace"], builds["plain"]),
            "gas_delivery": scenario != "skirmish" or all(
                r["gas_assigned"] == 1 and r["gas_pickup"] == 1 and r["gas_delivered"] == 1
                for r in (first, second, without, unrendered)),
            "full_length": all(r["steps"] == args.steps and r["screen"] == 1
                               for r in (first, second, without, unrendered)),
            "render_on_off": first["render"] == 1 and second["render"] == 1 and without["render"] == 1 and
                             unrendered["render"] == 0 and
                             without["world"] == unrendered["world"] and
                             without["world_chain"] == unrendered["world_chain"] and
                             without["steps"] == unrendered["steps"],
        }
        result = {
            "schema": "sc64-parity-selfgate/1",
            "profile": name,
            "scenario": scenario,
            "kind": args.kind,
            "steps": args.steps,
            "gates": gates,
            "passed": all(gates.values()),
            "runs": {"trace_first": first, "trace_second": second,
                     "plain_render": without, "plain_no_render": unrendered},
            "builds": builds,
        }
        if {"trace": provenance(traced), "plain": provenance(plain)} != builds:
            raise SystemExit(f"{name}: a binary or its inputs changed during the gate; rerun it")
        out.write_text(json.dumps(result, indent=2) + "\n")
        status = "PASS" if result["passed"] else "FAIL"
        print(f"{status} {name} {scenario}: " + " ".join(f"{k}={'ok' if v else 'FAIL'}" for k, v in gates.items()) +
              f" steps={first['steps']} trace={first['trace_bytes']}B sha256={first['trace_sha256'][:16]}"
              f" inputs={result['builds']['trace']['generated_inputs_combined_sha256'][:16]} -> "
              f"{out.relative_to(ROOT) if out.is_relative_to(ROOT) else out}")
        failed |= not result["passed"]
    return 1 if failed else 0


def bench(args: argparse.Namespace) -> int:
    directory = (ROOT / args.dir).resolve()
    rows = []
    for name, scenario in parse_profiles(args.profile):
        for kind, steps in (("fast", args.fast_steps), ("asan", args.asan_steps)):
            for emitter, render in (("plain", True), ("trace", True), ("plain", False), ("trace", False)):
                binary = directory / f"{name}-{kind}-{emitter}"
                best = None
                for _ in range(args.repeat):
                    with tempfile.TemporaryDirectory(prefix="sc64-parity-") as scratch:
                        trace = Path(scratch) / "bench.trace" if emitter == "trace" else None
                        report = run(binary, scenario, name, steps, trace=trace, chain=False, render=render,
                                     verify_body=False, checks=args.route_checks)
                    if best is None or report["cpu_seconds"] < best["cpu_seconds"]:
                        best = report
                rows.append({"profile": name, "scenario": scenario, "build": kind, "emitter": emitter,
                             "render": render,
                             "steps": best["steps"], "host_ticks": best["host_ticks"],
                             "cpu_seconds": round(best["cpu_seconds"], 4),
                             "wall_seconds": round(best["wall_seconds"], 4),
                             "updates_per_cpu_second": round(best["updates_per_cpu_second"]),
                             "trace_bytes": best.get("trace_bytes", 0)})
    print("| profile | build | emitter | draw | steps | CPU s | updates per CPU s | trace bytes |")
    print("| --- | --- | --- | --- | ---: | ---: | ---: | ---: |")
    for row in rows:
        print(f"| {row['profile']} ({row['scenario']}) | {row['build']} | {row['emitter']} | "
              f"{'on' if row['render'] else 'off'} | {row['steps']} | "
              f"{row['cpu_seconds']} | {row['updates_per_cpu_second']} | {row['trace_bytes']} |")
    (directory / "bench.json").write_text(json.dumps(rows, indent=2) + "\n")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest="command", required=True)
    g = sub.add_parser("gate", help="run the four self-gates")
    g.add_argument("--dir", default="build/parity")
    g.add_argument("--kind", choices=("asan", "fast"), default="asan")
    g.add_argument("--profile", action="append", required=True, help="NAME:t1 or NAME:skirmish")
    g.add_argument("--steps", type=int, default=3000)
    g.add_argument("--keep-trace", action="store_true", help="keep the first trace as <dir>/<profile>-<scenario>.trace")
    r = sub.add_parser("rom-flags", help="the ROM ELF's preprocessor options, from a make dry run (used by make)")
    r.add_argument("elf")
    r.add_argument("--make", help="run this make for the dry run (otherwise read its output on stdin)")
    r.add_argument("--out", type=Path, help="write here, only when the options changed")
    i = sub.add_parser("inputs", help="print generated-input SHA-256s for a dependency file (used by make)")
    i.add_argument("dep_file", type=Path)
    i.add_argument("--user-deps", type=Path, required=True, help="the -MM dependency file")
    i.add_argument("--binary", type=Path, help="the executable this record describes")
    i.add_argument("--expect", type=Path, help="an earlier record whose inputs must be unchanged")
    i.add_argument("--cmd-file", type=Path, required=True, help="the compile command the build used")
    b = sub.add_parser("bench", help="measure host updates per second")
    b.add_argument("--dir", default="build/parity")
    b.add_argument("--profile", action="append", required=True)
    b.add_argument("--fast-steps", type=int, default=20000)
    b.add_argument("--asan-steps", type=int, default=3000)
    b.add_argument("--repeat", type=int, default=3)
    b.add_argument("--route-checks", action="store_true",
                   help="keep the per-update route-purity hashing the gates use (off by default)")
    args = parser.parse_args()
    if args.command == "rom-flags":
        if args.make:
            if not args.out:
                raise SystemExit("rom-flags: --make needs --out")
            rom_flags_from_make(args.elf, args.make, args.out)
        else:
            print(rom_flags(sys.stdin.read(), args.elf))
        return 0
    if args.command == "inputs":
        command = args.cmd_file.read_text().strip()
        record = {"compile_command": command,
                  "inputs": generated_inputs(args.dep_file, args.user_deps, command)}
        if args.expect and json.loads(args.expect.read_text())["inputs"] != record["inputs"]:
            raise SystemExit(f"inputs: generated inputs changed while {args.binary} was compiling; rebuild")
        if args.binary:
            record["binary_sha256"] = sha256_file(args.binary)
        print(json.dumps(record, indent=1))
        return 0
    return gate(args) if args.command == "gate" else bench(args)


if __name__ == "__main__":
    sys.exit(main())
