#!/usr/bin/env python3
"""Check, package and copy StarCraft GBA release ROMs.

Subcommands (also reachable as ``./dev release ...``):

  check    build, verify and run the HOST suite for each product, plus the
           source-tree and shared-capability checks, and write
           release-check.json / release-check.md
  package  assemble a versioned folder (ROMs, MANIFEST.json, SHA256SUMS,
           README.txt with the source revision and the hardware checklist)
           from a check result for the current revision
  copy     copy a package folder onto a destination (an SD card in practice)
           without overwriting or deleting anything, then read every file
           back and compare checksums
  verify   repeat only the read-back comparison for a destination

The tool never generates private inputs. A product whose inputs are absent is
reported SKIPPED, never PASS. Hardware results are not produced here; a person
records them on docs/HARDWARE-CHECKLIST.md.
"""
from __future__ import annotations

import argparse
import contextlib
import datetime as dt
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile
import time
from dataclasses import dataclass
from typing import Callable, Iterator, Sequence

ROOT = Path(__file__).resolve().parents[1]
SCHEMA = 1
CHECKLIST = ROOT / "docs" / "HARDWARE-CHECKLIST.md"
GIB = 1024 ** 3
DEFAULT_MIN_FREE_GIB = 4.0

PASS, FAIL, SKIPPED = "PASS", "FAIL", "SKIPPED"


@dataclass(frozen=True)
class Product:
    key: str
    title: str
    rom: str                      # ROM path relative to the repository
    elf: str
    verify_target: str            # make target that runs tools/verify_roms.py
    test_targets: tuple[str, ...]
    game_code: str
    optional: bool = False        # legacy and preview products are opt-in
    aliases: tuple[str, ...] = ()

    @property
    def build_targets(self) -> tuple[str, ...]:
        return (self.elf, self.rom)


# Every target below must exist in the Makefile (tests/test_sc64_release.py
# checks that against the Makefile text).
REGISTRY: tuple[Product, ...] = (
    Product("default", "Original-art skirmish", "build/StarCraft-GBA.gba",
            "build/starcraft.elf", "verify", ("test",), "SCGD"),
    Product("terran", "Terran campaign T10", "build/StarCraft-Terran-Transport1-GBA.gba",
            "build/starcraft-terran.elf", "verify-terran",
            ("test-sc64-terran-full-campaign", "test-terran"), "SCT8"),
    Product("zerg-z10", "Zerg campaign Z10", "build/StarCraft-Zerg-Z10-Transport1-GBA.gba",
            "build/starcraft-zerg-z10.elf", "verify-zerg", ("test-zerg",), "SCZA",
            aliases=("zerg",)),
    Product("zerg-z9", "Zerg campaign Z9 (legacy saves)", "build/StarCraft-Zerg-Z9-Transport1-GBA.gba",
            "build/starcraft-zerg-z9.elf", "verify-zerg-z9", ("test-sc64-zerg-z9",), "SCZ9",
            optional=True),
    Product("protoss-p10", "Protoss campaign P10", "build/StarCraft-Protoss-P10-Transport1-GBA.gba",
            "build/starcraft-protoss-p10.elf", "verify-protoss", ("test-protoss",), "SCPA",
            aliases=("protoss",)),
    Product("showcase", "Showcase", "build/StarCraft-Showcase-Transport1-GBA.gba",
            "build/starcraft-showcase.elf", "verify-showcase", ("test-sc64-showcase",), "SCSH"),
)
BY_KEY = {p.key: p for p in REGISTRY}
GLOBAL_STEPS = ("source-tree", "engine-capabilities")


class ReleaseError(Exception):
    """A refusal or a failure the user must act on."""


# --------------------------------------------------------------------------
# Small helpers

def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1 << 20), b""):
            digest.update(block)
    return digest.hexdigest()


def git(*args: str, root: Path = ROOT) -> str:
    result = subprocess.run(["git", *args], cwd=root, text=True, check=False,
                            stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    if result.returncode != 0:
        raise ReleaseError(f"git {' '.join(args)} failed: {result.stderr.strip()}")
    return result.stdout


def source_state(root: Path = ROOT) -> tuple[str, bool]:
    """Return the full HEAD revision and whether tracked files are modified."""
    revision = git("rev-parse", "HEAD", root=root).strip()
    dirty = bool(git("status", "--porcelain", "--untracked-files=no", root=root).strip())
    return revision, dirty


def tool_versions() -> dict[str, str]:
    prefix = os.environ.get("PREFIX", "arm-none-eabi-")
    commands = {
        "python": [sys.executable, "--version"],
        "make": ["make", "--version"],
        "arm-gcc": [prefix + "gcc", "--version"],
        "arm-nm": [prefix + "nm", "--version"],
        "clang": [os.environ.get("CLANG", "clang"), "--version"],
        "git": ["git", "--version"],
    }
    versions = {}
    for name, command in commands.items():
        try:
            out = subprocess.run(command, text=True, stdout=subprocess.PIPE,
                                 stderr=subprocess.STDOUT, check=False, timeout=30).stdout
            versions[name] = out.strip().splitlines()[0] if out.strip() else "unknown"
        except (OSError, subprocess.SubprocessError):
            versions[name] = "unavailable"
    return versions


def resolve_products(spec: str | None) -> list[Product]:
    """Default: every non-optional product. ``all`` adds opt-in products."""
    if not spec:
        return [p for p in REGISTRY if not p.optional]
    if spec == "all":
        return list(REGISTRY)
    chosen: list[Product] = []
    for name in (part.strip() for part in spec.split(",") if part.strip()):
        match = next((p for p in REGISTRY if name == p.key or name in p.aliases), None)
        if match is None:
            raise ReleaseError(f"unknown product {name!r}; known: " +
                               ", ".join(p.key for p in REGISTRY))
        if match not in chosen:
            chosen.append(match)
    if not chosen:
        raise ReleaseError("no products selected")
    return chosen


# --------------------------------------------------------------------------
# Missing private-input detection (reads make's database; runs no recipes)

def parse_make_database(text: str) -> tuple[dict[str, list[str]], set[str]]:
    """Return explicit rules (target -> prerequisites, order-only dropped) and
    the phony set from ``make -p`` output."""
    rules: dict[str, list[str]] = {}
    in_files = False
    skip_next = False
    for line in text.splitlines():
        if line.startswith("# Files"):
            in_files = True
            continue
        if not in_files:
            continue
        if line.startswith("# Not a target"):
            skip_next = True
            continue
        if not line or line[0] in "#\t " or line.startswith("%"):
            continue
        if skip_next:
            skip_next = False
            continue
        match = re.match(r"^([^:=\s][^:=]*?):(?!=|:)\s*(.*)$", line)
        if not match:
            continue
        rest = match.group(2)
        if re.match(r"^[A-Za-z_][\w.-]*\s*[:+?]?=", rest):
            continue                       # target-specific variable
        prereqs = rest.split("|", 1)[0].split()
        for target in match.group(1).split():
            rules.setdefault(target, [])
            for item in prereqs:
                if item not in rules[target]:
                    rules[target].append(item)
    return rules, set(rules.get(".PHONY", []))


def parse_ignored(make_args: Sequence[str]) -> set[str]:
    """Files named by make's -o / --old-file are assumed present and not
    descended into."""
    ignored: set[str] = set()
    args = list(make_args)
    for index, arg in enumerate(args):
        if arg in ("-o", "--old-file", "--assume-old") and index + 1 < len(args):
            ignored.add(args[index + 1])
        elif arg.startswith("--old-file=") or arg.startswith("--assume-old="):
            ignored.add(arg.split("=", 1)[1])
        elif arg.startswith("-o") and len(arg) > 2 and not arg.startswith("--"):
            ignored.add(arg[2:])
    return ignored


def missing_from_rules(rules: dict[str, list[str]], phony: set[str],
                       targets: Sequence[str], ignored: set[str],
                       root: Path) -> list[str]:
    """Walk the prerequisite closure; return files that do not exist and that
    no explicit rule can create. Outputs under build/ without a rule are not
    reported: implicit rules may build them."""
    missing: list[str] = []
    seen: set[str] = set()
    stack = list(reversed(targets))
    while stack:
        node = stack.pop()
        if node in seen:
            continue
        seen.add(node)
        if node in ignored:
            continue
        if node in rules:
            stack.extend(reversed(rules[node]))
            continue
        if node in phony or (root / node).exists():
            continue
        if node.startswith("build/"):
            continue
        missing.append(node)
    return missing


_NOOP_MAKEFILE = "release-noop:\n\t@true\n"


def find_missing_inputs(targets: Sequence[str], make_args: Sequence[str],
                        root: Path = ROOT) -> list[str]:
    """Ask make for its rule database (``make -p -q`` on a no-op target; no
    product recipe can run) and report absent prerequisites of ``targets``."""
    with tempfile.TemporaryDirectory(prefix="sc64-release-") as td:
        noop = Path(td) / "noop.mk"
        noop.write_text(_NOOP_MAKEFILE, encoding="utf-8")
        result = subprocess.run(
            ["make", "-p", "-q", "-s", "-f", "Makefile", "-f", str(noop), *make_args,
             "release-noop"],
            cwd=root, text=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=False)
    if result.returncode not in (0, 1) or "# Files" not in result.stdout:
        last = (result.stderr.strip().splitlines() or ["no output"])[-1]
        raise ReleaseError(f"could not read the make database: {last}")
    rules, phony = parse_make_database(result.stdout)
    return missing_from_rules(rules, phony, targets, parse_ignored(make_args), root)


# --------------------------------------------------------------------------
# ELF / ROM facts

def linker_limits(root: Path = ROOT) -> dict[str, int]:
    limits = {"__iwram_end": 0x03005000, "__bss_end": 0x0203E000}
    try:
        text = (root / "common" / "gba.ld").read_text(encoding="utf-8")
    except OSError:
        return limits
    for symbol in limits:
        match = re.search(rf"ASSERT\(\s*{symbol}\s*<\s*(0x[0-9a-fA-F]+)", text)
        if match:
            limits[symbol] = int(match.group(1), 16)
    return limits


def ram_margins(elf: Path, root: Path = ROOT) -> dict[str, dict[str, int]] | None:
    prefix = os.environ.get("PREFIX", "arm-none-eabi-")
    try:
        out = subprocess.run([prefix + "nm", str(elf)], text=True, stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE, check=True).stdout
    except (OSError, subprocess.CalledProcessError):
        return None
    limits = linker_limits(root)
    values = {}
    for line in out.splitlines():
        parts = line.split()
        if len(parts) == 3 and parts[2] in limits:
            values[parts[2]] = int(parts[0], 16)
    if set(values) != set(limits):
        return None
    return {name: {"end": values[name], "limit": limits[name],
                   "margin_bytes": limits[name] - values[name]} for name in limits}


def rom_info(path: Path) -> dict[str, object] | None:
    if not path.is_file():
        return None
    data = path.read_bytes()
    return {"path": path.as_posix(), "bytes": len(data),
            "sha256": hashlib.sha256(data).hexdigest(),
            "game_code": data[0xAC:0xB0].decode("ascii", errors="replace"),
            "title": data[0xA0:0xAC].decode("ascii", errors="replace").rstrip("\0 ")}


def first_error_line(log: Path) -> str:
    """Pick the most informative failure line: a missing rule, the first failing
    unittest, a compiler error, then make's own error line."""
    try:
        lines = log.read_text(encoding="utf-8", errors="replace").splitlines()
    except OSError:
        return "log unreadable"
    tests = re.compile(r"^(ERROR|FAIL): ")
    wanted = (
        lambda line: "No rule to make target" in line,
        lambda line: bool(tests.match(line)),
        lambda line: "fatal error:" in line or ": error: " in line and "unittest" not in line,
        lambda line: "*** [" in line,
        lambda line: any(word in line for word in ("Traceback", "Assertion", "AddressSanitizer")),
    )
    for matches in wanted:
        for line in lines:
            if matches(line):
                return line.strip()[:300]
    tail = [line for line in lines if line.strip()]
    return tail[-1].strip()[:300] if tail else "no output"


# --------------------------------------------------------------------------
# Running steps

Runner = Callable[[list[str], Path, Path], int]


def default_runner(command: list[str], log: Path, cwd: Path) -> int:
    log.parent.mkdir(parents=True, exist_ok=True)
    with log.open("w", encoding="utf-8") as handle:
        handle.write("$ " + " ".join(command) + "\n")
        handle.flush()
        return subprocess.run(command, cwd=cwd, stdout=handle, stderr=subprocess.STDOUT,
                              check=False).returncode


@dataclass
class Env:
    root: Path = ROOT
    runner: Runner = default_runner
    find_missing: Callable[[Sequence[str], Sequence[str]], list[str]] = None  # type: ignore
    free_bytes: Callable[[], int] = None  # type: ignore
    clock: Callable[[], float] = time.monotonic
    lock_path: str | None = None
    min_free_bytes: int = int(DEFAULT_MIN_FREE_GIB * GIB)

    def __post_init__(self) -> None:
        if self.find_missing is None:
            self.find_missing = lambda targets, args: find_missing_inputs(targets, args, self.root)
        if self.free_bytes is None:
            self.free_bytes = lambda: shutil.disk_usage(self.root).free


@contextlib.contextmanager
def build_lock(path: str | None) -> Iterator[None]:
    if not path:
        yield
        return
    import fcntl
    with open(path, "a") as handle:
        fcntl.flock(handle, fcntl.LOCK_EX)
        try:
            yield
        finally:
            fcntl.flock(handle, fcntl.LOCK_UN)


def make_command(make_args: Sequence[str], targets: Sequence[str]) -> list[str]:
    jobs = [] if any(a.startswith("-j") or a.startswith("--jobs") for a in make_args) else ["-j1"]
    return ["make", *jobs, *make_args, *targets]


def step_record(name: str, status: str, *, seconds: float = 0.0, command: str = "",
                log: str = "", detail: str = "") -> dict[str, object]:
    return {"name": name, "status": status, "seconds": round(seconds, 1),
            "command": command, "log": log, "detail": detail}


def run_make_step(env: Env, name: str, product_key: str, targets: Sequence[str],
                  make_args: Sequence[str], logs: Path) -> dict[str, object]:
    command = make_command(make_args, targets)
    text = " ".join(command)
    log = logs / f"{product_key}-{name}.log"
    try:
        missing = env.find_missing(list(targets), list(make_args))
    except ReleaseError as error:
        return step_record(name, FAIL, command=text, detail=str(error))
    if missing:
        shown = ", ".join(missing[:3]) + (f" (+{len(missing) - 3} more)" if len(missing) > 3 else "")
        return step_record(name, SKIPPED, command=text, detail=f"missing: {shown}")
    free = env.free_bytes()
    if free < env.min_free_bytes:
        return step_record(name, FAIL, command=text,
                           detail=f"refusing to build: {free / GIB:.1f} GiB free, "
                                  f"{env.min_free_bytes / GIB:.1f} GiB required")
    started = env.clock()
    with build_lock(env.lock_path):
        rc = env.runner(command, log, env.root)
    seconds = env.clock() - started
    if rc == 0:
        return step_record(name, PASS, seconds=seconds, command=text, log=str(log))
    return step_record(name, FAIL, seconds=seconds, command=text, log=str(log),
                       detail=f"exit {rc}: {first_error_line(log)}")


def check_product(env: Env, product: Product, make_args: Sequence[str],
                  logs: Path) -> dict[str, object]:
    steps = [run_make_step(env, "build", product.key, product.build_targets, make_args, logs)]
    build_ok = steps[0]["status"] == PASS
    if build_ok:
        verify = run_make_step(env, "verify", product.key, [product.verify_target], make_args, logs)
    else:
        verify = step_record("verify", SKIPPED, command=" ".join(
            make_command(make_args, [product.verify_target])),
            detail=f"build {steps[0]['status']}")
    steps.append(verify)
    rom = rom_info(env.root / product.rom) if build_ok else None
    if verify["status"] == PASS:
        if rom is None:
            verify.update(status=FAIL, detail=f"{product.rom} is missing after verify")
        elif rom["game_code"] != product.game_code:
            verify.update(status=FAIL, detail=f"game code {rom['game_code']!r}, "
                                              f"expected {product.game_code!r}")
    steps.append(run_make_step(env, "test", product.key, product.test_targets, make_args, logs))
    ram = ram_margins(env.root / product.elf, env.root) if build_ok else None
    statuses = {s["status"] for s in steps}
    overall = FAIL if FAIL in statuses else SKIPPED if SKIPPED in statuses else PASS
    return {"key": product.key, "title": product.title, "status": overall,
            "game_code": product.game_code, "steps": steps,
            "rom": rom if build_ok else None, "ram": ram}


def run_global_step(env: Env, name: str, command: list[str], logs: Path) -> dict[str, object]:
    log = logs / f"{name}.log"
    text = " ".join(command)
    started = env.clock()
    rc = env.runner(command, log, env.root)
    seconds = env.clock() - started
    if rc != 0:
        return step_record(name, FAIL, seconds=seconds, command=text, log=str(log),
                           detail=f"exit {rc}: {first_error_line(log)}")
    if name == "engine-capabilities":
        try:
            output = log.read_text(encoding="utf-8", errors="replace")
        except OSError:
            output = ""
        skips = [line.strip() for line in output.splitlines() if "skipped" in line.lower()
                 and ("..." in line or line.lower().startswith("ok"))]
        if skips:
            return step_record(name, SKIPPED, seconds=seconds, command=text, log=str(log),
                               detail="; ".join(skips)[:300])
    return step_record(name, PASS, seconds=seconds, command=text, log=str(log))


def run_check(env: Env, products: Sequence[Product], make_args: Sequence[str],
              out: Path, *, require_all: bool = False) -> dict[str, object]:
    out.mkdir(parents=True, exist_ok=True)
    logs = out / "logs"
    revision, dirty = source_state(env.root)
    result: dict[str, object] = {
        "schema": SCHEMA, "revision": revision, "dirty": dirty,
        "date": dt.date.today().isoformat(),
        "started": dt.datetime.now(dt.timezone.utc).isoformat(timespec="seconds"),
        "make_args": list(make_args), "require_all": require_all,
        "tools": tool_versions(), "build_lock": env.lock_path or "",
        "global_steps": [], "products": [],
    }
    py = sys.executable
    result["global_steps"] = [
        run_global_step(env, "source-tree", [py, "tools/check_source_tree.py"], logs),
        run_global_step(env, "engine-capabilities",
                        [py, "-m", "unittest", "-v", "tests/test_sc64_engine_capabilities.py"],
                        logs),
    ]
    for product in products:
        result["products"].append(check_product(env, product, make_args, logs))
    result["overall"] = overall_verdict(result)
    result["finished"] = dt.datetime.now(dt.timezone.utc).isoformat(timespec="seconds")
    write_check_files(result, out)
    return result


def overall_verdict(result: dict[str, object]) -> str:
    entries = list(result["global_steps"]) + list(result["products"])  # type: ignore[arg-type]
    statuses = {e["status"] for e in entries}
    if FAIL in statuses:
        return FAIL
    if SKIPPED in statuses:
        return FAIL if result.get("require_all") else SKIPPED
    return PASS


def exit_code(result: dict[str, object]) -> int:
    return 0 if result["overall"] in (PASS, SKIPPED) else 1


def write_check_files(result: dict[str, object], out: Path) -> None:
    (out / "release-check.json").write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    (out / "release-check.md").write_text(render_markdown(result), encoding="utf-8")


def render_markdown(result: dict[str, object]) -> str:
    rev = str(result["revision"])
    lines = [f"# Release check {rev[:8]}", "",
             f"- Revision: `{rev}`" + (" (tracked files modified: DIRTY)" if result["dirty"] else " (clean)"),
             f"- Date: {result['date']}",
             f"- Overall: **{result['overall']}**" +
             (" (--require-all: SKIPPED counts as failure)" if result.get("require_all") else ""),
             f"- Extra make arguments: `{' '.join(result['make_args']) or 'none'}`", "",  # type: ignore[arg-type]
             "## Tools", ""]
    lines += [f"- {k}: {v}" for k, v in result["tools"].items()]  # type: ignore[union-attr]
    lines += ["", "## Steps", "", "| Item | Step | Result | Seconds | Detail |", "|---|---|---|---|---|"]
    for step in result["global_steps"]:  # type: ignore[union-attr]
        lines.append(f"| tree | {step['name']} | {step['status']} | {step['seconds']} | {_cell(step['detail'])} |")
    for product in result["products"]:  # type: ignore[union-attr]
        for step in product["steps"]:
            lines.append(f"| {product['key']} | {step['name']} | {step['status']} | "
                         f"{step['seconds']} | {_cell(step['detail'])} |")
    lines += ["", "## ROMs", "", "| Product | File | Game code | Bytes | SHA-256 | IWRAM margin | EWRAM margin |",
              "|---|---|---|---|---|---|---|"]
    for product in result["products"]:  # type: ignore[union-attr]
        rom, ram = product["rom"], product["ram"]
        if rom:
            iw = ram["__iwram_end"]["margin_bytes"] if ram else "n/a"
            ew = ram["__bss_end"]["margin_bytes"] if ram else "n/a"
            lines.append(f"| {product['key']} | {Path(rom['path']).name} | {rom['game_code']} | "
                         f"{rom['bytes']} | `{rom['sha256']}` | {iw} | {ew} |")
        else:
            lines.append(f"| {product['key']} | not built | {product['game_code']} | | | | |")
    lines += ["", "Margins are bytes below the linker limits in common/gba.ld "
              "(`__iwram_end`, `__bss_end`). No hardware testing is recorded by this file.", ""]
    return "\n".join(lines)


def _cell(text: object) -> str:
    return str(text).replace("|", "/").replace("\n", " ")


# --------------------------------------------------------------------------
# Packaging

def path_is_untracked_ignored(path: Path, root: Path = ROOT) -> None:
    """Refuse destinations inside the repository unless Git ignores them."""
    resolved = path.resolve()
    try:
        rel = resolved.relative_to(root.resolve())
    except ValueError:
        return                                  # outside the repository
    tracked = git("ls-files", "--", rel.as_posix(), root=root).strip()
    ignored = subprocess.run(["git", "check-ignore", "-q", "--", rel.as_posix()], cwd=root,
                             check=False).returncode == 0
    if tracked or not ignored:
        raise ReleaseError(f"refusing to write inside tracked or non-ignored path {rel.as_posix()}")


def load_check(directory: Path) -> dict[str, object]:
    path = directory / "release-check.json"
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, ValueError) as error:
        raise ReleaseError(f"cannot read {path}: {error}") from error
    if data.get("schema") != SCHEMA:
        raise ReleaseError(f"{path}: unsupported schema {data.get('schema')!r}")
    return data


def package_label(revision: str, date: str, dirty: bool) -> str:
    return f"StarCraft-GBA-{date}-{revision[:8]}" + ("-dirty" if dirty else "")


def step_status(product: dict[str, object], name: str) -> str:
    for step in product["steps"]:  # type: ignore[union-attr]
        if step["name"] == name:
            return str(step["status"])
    return SKIPPED


def checklist_text(keys: Sequence[str], path: Path = CHECKLIST) -> str:
    """Plain-text checklist: the common part plus sections for ``keys``."""
    try:
        text = path.read_text(encoding="utf-8")
    except OSError as error:
        raise ReleaseError(f"cannot read the hardware checklist {path}: {error}") from error
    parts = re.split(r"(?m)^## ", text)
    out = ["HARDWARE CHECKLIST (fill in by hand; nothing below has been ticked by a tool)", ""]
    for part in parts[1:]:
        match = re.search(r"<!--\s*products:\s*([^>]*?)\s*-->", part)
        if match:
            wanted = {k.strip() for k in match.group(1).split(",")}
            if not wanted & set(keys):
                continue
            part = part.replace(match.group(0) + "\n", "")
        heading, _, body = part.partition("\n")
        out.append(heading.strip().upper())
        out.extend(line for line in body.strip("\n").splitlines())
        out.append("")
    return "\n".join(out).replace("- [ ]", "[ ]")


def render_readme(check: dict[str, object], label: str, dirty: bool,
                  entries: list[dict[str, object]], accepted_skips: bool,
                  checklist: str) -> str:
    rev = str(check["revision"])
    lines = [f"StarCraft GBA release package {label}",
             f"SOURCE REVISION: {rev}" + ("  (DIRTY: tracked files were modified)" if dirty else ""),
             f"Short revision:  {rev[:8]}   Date: {check['date']}", "",
             "Write this revision on the hardware checklist below. A ROM from a newer",
             "revision is NOT on a card unless it was copied and read back (see COPY-VERIFIED.txt).", "",
             "ROMS", ""]
    for entry in entries:
        lines += [f"  {entry['file']}", f"    product {entry['product']} ({entry['title']}), game code "
                  f"{entry['game_code']}, {entry['bytes']} bytes", f"    sha256 {entry['sha256']}"]
    if any(entry["product"] == "zerg-z10" for entry in entries):
        lines += ["", "ZERG SAVES",
                  "  Keep your existing Z9 ROM and its matching save together.",
                  "  The ten-mission Z10 ROM has a separate save identity; it does not convert Z9 saves."]
    lines += ["", "WHAT WAS CHECKED (tools/sc64_release.py check, on this revision)", ""]
    for step in check["global_steps"]:  # type: ignore[union-attr]
        lines.append(f"  tree     {step['name']}: {step['status']}" +
                     (f" ({step['detail']})" if step["detail"] else ""))
    packaged = {e["product"] for e in entries}
    for product in check["products"]:  # type: ignore[union-attr]
        marker = "packaged" if product["key"] in packaged else "NOT packaged"
        lines.append(f"  {product['key']} ({marker}): " + ", ".join(
            f"{s['name']} {s['status']}" for s in product["steps"]))
        for step in product["steps"]:
            if step["status"] != PASS and step["detail"]:
                lines.append(f"      {step['name']}: {step['detail']}")
    lines += ["", "WHAT WAS NOT CHECKED", "",
              "  - Hardware: no test on a real Game Boy Advance is recorded in this package",
              "    until a person completes the checklist below."]
    if accepted_skips:
        lines.append("  - Steps marked SKIPPED above did not run (missing private inputs); they were")
        lines.append("    accepted explicitly when this package was built.")
    skipped = [p["key"] for p in check["products"] if p["key"] not in packaged]  # type: ignore[union-attr]
    if skipped:
        lines.append("  - Products not in this package: " + ", ".join(skipped))
    lines += ["", "FILES", "",
              "  SHA256SUMS lists every file here except itself; verify with:",
              "    shasum -a 256 -c SHA256SUMS", "", checklist.rstrip(), ""]
    return "\n".join(lines)


def build_package(check_dir: Path, out_dir: Path | None, *, allow_dirty: bool = False,
                  products: Sequence[Product] | None = None, accept_skipped: bool = False,
                  root: Path = ROOT, checklist_path: Path = CHECKLIST,
                  revision: tuple[str, bool] | None = None) -> Path:
    check = load_check(check_dir)
    head, tree_dirty = revision or source_state(root)
    if check["revision"] != head:
        raise ReleaseError(f"check was run at {str(check['revision'])[:8]} but HEAD is {head[:8]}; "
                           "run check again")
    dirty = bool(check["dirty"]) or tree_dirty
    if dirty and not allow_dirty:
        raise ReleaseError("the tree has modified tracked files (or the check ran on one); "
                           "commit first or pass --allow-dirty to build a -dirty package")
    by_key = {p["key"]: p for p in check["products"]}  # type: ignore[union-attr]
    if products is not None:
        wanted = [p.key for p in products]
        for key in wanted:
            if key not in by_key:
                raise ReleaseError(f"product {key} is not in the check result")
    else:
        wanted = [k for k, p in by_key.items() if _packageable(p, accept_skipped)]
    for key in wanted:
        if not _packageable(by_key[key], accept_skipped):
            raise ReleaseError(f"product {key} did not PASS ({by_key[key]['status']}); not packaging it")
    if not wanted:
        raise ReleaseError("no product passed; nothing to package")
    label = package_label(head, str(check["date"]), dirty)
    target_parent = out_dir if out_dir is not None else root / "build" / "release"
    path_is_untracked_ignored(target_parent / label, root)
    target = target_parent / label
    if target.exists():
        raise ReleaseError(f"{target} already exists; remove it or choose another --out")
    staging = target_parent / (label + ".partial")
    if staging.exists():
        raise ReleaseError(f"{staging} exists from an interrupted run; remove it")
    staging.mkdir(parents=True)
    try:
        entries = []
        suffix = "-dirty" if dirty else ""
        for key in wanted:
            product = by_key[key]
            rom = product["rom"]
            source = Path(rom["path"])
            if not source.is_file() or sha256_file(source) != rom["sha256"]:
                raise ReleaseError(f"{source} changed since the check; run check again")
            stem = source.stem
            name = f"{stem}-{head[:8]}{suffix}.gba"
            shutil.copyfile(source, staging / name)
            entries.append({"product": key, "title": product["title"], "file": name,
                            "game_code": rom["game_code"], "bytes": rom["bytes"],
                            "sha256": rom["sha256"], "source_rom": source.name,
                            "steps": {s["name"]: s["status"] for s in product["steps"]},
                            "ram": product["ram"]})
        readme = render_readme(check, label, dirty, entries, accept_skipped,
                               checklist_text(wanted, checklist_path))
        (staging / "README.txt").write_text(readme, encoding="utf-8")
        manifest = {"schema": SCHEMA, "label": label, "revision": head, "dirty": dirty,
                    "date": check["date"], "check_overall": check["overall"],
                    "accepted_skipped_steps": accept_skipped,
                    "global_steps": {s["name"]: s["status"] for s in check["global_steps"]},  # type: ignore[union-attr]
                    "hardware_tested": False, "roms": entries}
        (staging / "MANIFEST.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
        sums = [f"{sha256_file(staging / n)}  {n}\n"
                for n in sorted(p.name for p in staging.iterdir())]
        (staging / "SHA256SUMS").write_text("".join(sums), encoding="utf-8")
        staging.rename(target)
    except BaseException:
        shutil.rmtree(staging, ignore_errors=True)
        raise
    return target


def _packageable(product: dict[str, object], accept_skipped: bool) -> bool:
    if product["status"] == PASS:
        return bool(product["rom"])
    if accept_skipped and product["status"] == SKIPPED:
        return (step_status(product, "build") == PASS and step_status(product, "verify") == PASS
                and not any(s["status"] == FAIL for s in product["steps"]))  # type: ignore[union-attr]
    return False


# --------------------------------------------------------------------------
# Copy and read-back

def parse_sums(text: str) -> dict[str, str]:
    sums = {}
    for line in text.splitlines():
        match = re.match(r"^([0-9a-f]{64})  (.+)$", line)
        if not match:
            raise ReleaseError(f"malformed SHA256SUMS line: {line!r}")
        name = match.group(2)
        if "/" in name or name in (".", "..") or "\\" in name:
            raise ReleaseError(f"unsafe file name in SHA256SUMS: {name!r}")
        sums[name] = match.group(1)
    return sums


def package_files(package: Path) -> dict[str, str]:
    """Validate a package folder against its own SHA256SUMS; return name -> sha256
    for every file including SHA256SUMS itself."""
    sums_path = package / "SHA256SUMS"
    if not sums_path.is_file():
        raise ReleaseError(f"{package} has no SHA256SUMS; not a package folder")
    sums = parse_sums(sums_path.read_text(encoding="utf-8"))
    for required in ("MANIFEST.json", "README.txt"):
        if required not in sums:
            raise ReleaseError(f"{required} is not listed in {sums_path}")
    for name, digest in sums.items():
        file = package / name
        if file.is_symlink() or not file.is_file():
            raise ReleaseError(f"{file} is missing or not a regular file")
        if sha256_file(file) != digest:
            raise ReleaseError(f"{file} does not match SHA256SUMS; the package is damaged")
    sums["SHA256SUMS"] = sha256_file(sums_path)
    return sums


def read_back_hash(path: Path) -> str:
    """Hash a file, asking macOS to bypass the page cache when it can."""
    digest = hashlib.sha256()
    with path.open("rb", buffering=0) as handle:
        try:
            import fcntl
            fcntl.fcntl(handle.fileno(), getattr(fcntl, "F_NOCACHE", 48), 1)
        except (ImportError, OSError, AttributeError):
            pass
        for block in iter(lambda: handle.read(1 << 20), b""):
            digest.update(block)
    return digest.hexdigest()


def compare_directory(directory: Path, expected: dict[str, str]) -> list[dict[str, str]]:
    results = []
    for name in sorted(expected):
        file = directory / name
        if not file.is_file():
            results.append({"file": name, "result": "MISSING", "sha256": ""})
            continue
        actual = read_back_hash(file)
        results.append({"file": name, "result": "OK" if actual == expected[name] else "MISMATCH",
                        "sha256": actual})
    return results


def copy_package(package: Path, dest: Path, *,
                 copyfile: Callable[[Path, Path], object] = shutil.copyfile,
                 log: Callable[[str], None] = print) -> tuple[bool, list[dict[str, str]]]:
    """Copy ``package`` to ``dest/<package name>/``. Never overwrites a file with
    different content and never deletes anything. Returns (verified, results)."""
    expected = package_files(package)
    if not dest.is_dir():
        raise ReleaseError(f"destination {dest} is not a directory")
    target = dest / package.name
    if target.is_symlink() or (target.exists() and not target.is_dir()):
        raise ReleaseError(f"{target} exists and is not a directory")
    conflicts = [n for n in expected
                 if (target / n).exists() and (target / n).is_file()
                 and sha256_file(target / n) != expected[n]]
    conflicts += [n for n in expected if (target / n).exists() and not (target / n).is_file()]
    if conflicts:
        raise ReleaseError(f"{target} already holds different content for: {', '.join(sorted(conflicts))}; "
                           "nothing was copied and nothing was overwritten")
    needed = sum((package / n).stat().st_size for n in expected)
    if shutil.disk_usage(dest).free < needed + (1 << 20):
        raise ReleaseError(f"not enough free space on {dest} for {needed} bytes")
    target.mkdir(exist_ok=True)
    for name in sorted(expected):
        final = target / name
        if final.exists():
            log(f"already present, unchanged: {name}")
            continue
        partial = target / (name + ".part")
        try:
            copyfile(package / name, partial)
            with partial.open("rb") as handle:
                os.fsync(handle.fileno())
            os.replace(partial, final)
        finally:
            if partial.exists():
                partial.unlink()
        log(f"copied: {name}")
    results = compare_directory(target, expected)
    verified = all(r["result"] == "OK" for r in results)
    stamp = dt.datetime.now().astimezone().isoformat(timespec="seconds")
    manifest = json.loads((package / "MANIFEST.json").read_text(encoding="utf-8"))
    report = [f"{'COPY VERIFIED' if verified else 'COPY FAILED'}: {package.name}",
              f"Source revision: {manifest.get('revision', 'unknown')}"
              + (" (dirty)" if manifest.get("dirty") else ""),
              f"Read back: {stamp}", ""]
    report += [f"{r['result']:8} {r['file']}  {r['sha256']}" for r in results]
    report += ["", "Every file was re-read from the destination and compared with SHA256SUMS.",
               "The operating system may still hold data in cache: eject safely, reinsert, and run",
               "./dev release verify --dest <mount> --package <folder> for a fresh read-back.",
               "This is not a hardware test."]
    name = "COPY-VERIFIED.txt" if verified else "COPY-FAILED.txt"
    (target / name).write_text("\n".join(report) + "\n", encoding="utf-8")
    return verified, results


def verify_destination(dest: Path, package: Path | None = None) -> tuple[bool, str]:
    """Read-back comparison only. With ``package``, compare against it; without,
    each StarCraft-GBA-* folder is checked against its own SHA256SUMS."""
    if package is not None:
        expected = package_files(package)
        folder = dest / package.name
        if not folder.is_dir():
            return False, f"{folder} does not exist"
        results = compare_directory(folder, expected)
        folders = [(folder, results)]
    else:
        folders = []
        for folder in sorted(dest.glob("StarCraft-GBA-*")):
            if not (folder / "SHA256SUMS").is_file():
                continue
            sums = parse_sums((folder / "SHA256SUMS").read_text(encoding="utf-8"))
            sums["SHA256SUMS"] = sha256_file(folder / "SHA256SUMS")
            folders.append((folder, compare_directory(folder, sums)))
        if not folders:
            return False, f"no StarCraft-GBA-* package folders under {dest}"
    lines, ok = [], True
    for folder, results in folders:
        for r in results:
            lines.append(f"{r['result']:8} {folder.name}/{r['file']}")
            ok &= r["result"] == "OK"
    lines.append(("VERIFIED: every file matches" if ok else "NOT VERIFIED: see MISSING/MISMATCH above")
                 + ("" if package else " (compared with the folder's own SHA256SUMS only)"))
    return ok, "\n".join(lines)


# --------------------------------------------------------------------------
# Command line

def default_min_free() -> float:
    try:
        return float(os.environ.get("SC64_MIN_FREE_GIB", DEFAULT_MIN_FREE_GIB))
    except ValueError:
        return DEFAULT_MIN_FREE_GIB


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0],
                                     formatter_class=argparse.RawDescriptionHelpFormatter,
                                     epilog="Environment: SC64_BUILD_LOCK (flock file held around each make), "
                                            "SC64_MIN_FREE_GIB (default 4).")
    sub = parser.add_subparsers(dest="command", required=True)
    check = sub.add_parser("check", help="build, verify and test each product")
    check.add_argument("--products", help="comma list (" + ", ".join(p.key for p in REGISTRY) +
                       ") or 'all'; default: current products only")
    check.add_argument("--require-all", action="store_true", help="treat SKIPPED as failure")
    check.add_argument("--out", type=Path, help="output directory (default build/release-check/<rev8>)")
    check.add_argument("--make-arg", action="append", default=[], metavar="ARG",
                       help="extra make argument for every invocation (repeatable)")
    check.add_argument("--build-lock", help="flock file (default: $SC64_BUILD_LOCK)")
    check.add_argument("--min-free-gib", type=float, default=None,
                       help="refuse to build below this free space (default 4 or $SC64_MIN_FREE_GIB)")
    package = sub.add_parser("package", help="assemble a package from a check result")
    package.add_argument("--check", required=True, type=Path, dest="check_dir")
    package.add_argument("--out", type=Path, help="parent directory (default build/release)")
    package.add_argument("--allow-dirty", action="store_true")
    package.add_argument("--products", help="package exactly these products; each must PASS")
    package.add_argument("--accept-skipped", action="store_true",
                         help="also package products whose build and verify passed but whose "
                              "other steps were SKIPPED (recorded in the README)")
    copy = sub.add_parser("copy", help="copy a package to a destination and read it back")
    copy.add_argument("--package", required=True, type=Path)
    copy.add_argument("--dest", required=True, type=Path)
    verify = sub.add_parser("verify", help="read back a destination and compare checksums")
    verify.add_argument("--dest", required=True, type=Path)
    verify.add_argument("--package", type=Path)
    return parser


def join_make_args(argv: Sequence[str]) -> list[str]:
    """Let ``--make-arg -o`` work: argparse would read the bare ``-o`` as an
    option, so rewrite each pair to ``--make-arg=-o``."""
    out: list[str] = []
    items = list(argv)
    index = 0
    while index < len(items):
        if items[index] == "--make-arg" and index + 1 < len(items):
            out.append("--make-arg=" + items[index + 1])
            index += 2
        else:
            out.append(items[index])
            index += 1
    return out


def main(argv: Sequence[str] | None = None, env: Env | None = None) -> int:
    args = build_parser().parse_args(join_make_args(sys.argv[1:] if argv is None else argv))
    try:
        if args.command == "check":
            products = resolve_products(args.products)
            gib = args.min_free_gib if args.min_free_gib is not None else default_min_free()
            env = env or Env(lock_path=args.build_lock or os.environ.get("SC64_BUILD_LOCK") or None,
                             min_free_bytes=int(gib * GIB))
            revision, _ = source_state(env.root)
            out = args.out or env.root / "build" / "release-check" / revision[:8]
            path_is_untracked_ignored(out, env.root)
            result = run_check(env, products, args.make_arg, out, require_all=args.require_all)
            print((out / "release-check.md").read_text(encoding="utf-8"))
            print(f"Wrote {out / 'release-check.json'} and {out / 'release-check.md'}")
            print(f"Overall: {result['overall']}")
            return exit_code(result)
        if args.command == "package":
            products = resolve_products(args.products) if args.products else None
            target = build_package(args.check_dir, args.out, allow_dirty=args.allow_dirty,
                                   products=products, accept_skipped=args.accept_skipped)
            print(f"Package written to {target}")
            print((target / "README.txt").read_text(encoding="utf-8").split("\n\n")[0])
            return 0
        if args.command == "copy":
            verified, _ = copy_package(args.package, args.dest)
            print("VERIFIED: every copied file was read back and matches SHA256SUMS" if verified
                  else "FAILED: read-back did not match; see COPY-FAILED.txt on the destination "
                       "(nothing was deleted; remove the damaged package folder yourself before retrying)")
            return 0 if verified else 1
        ok, report = verify_destination(args.dest, args.package)
        print(report)
        return 0 if ok else 1
    except ReleaseError as error:
        print(f"ERROR: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
