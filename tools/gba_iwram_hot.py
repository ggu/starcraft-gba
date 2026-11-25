#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Run listed ROM functions from IWRAM, and prove the stack still fits.

IWRAM holds the GBA_HOT code and, above it, the main stack, which grows down
from 0x03007E00. gba.ld keeps the code below 0x03005000, an 11 KiB stack
reserve that was never derived from the program. This tool derives it, so a
build can use the part of the reserve the stack cannot reach.

  fragments CONFIG OUTDIR
      Write iwram_hot_sections.ld and iwram_hot_limit.ld for common/gba_hot.ld
      from a config/iwram-hot list.

  check --elf ELF --dump-dir DIR --config CONFIG
      Bound the stack depth of the linked program and fail unless
      __iwram_end + bound + margin <= stack top. The ELF must be compiled with
      -g -fcallgraph-info=su -fdump-ipa-cgraph -dumpdir DIR/.

The bound is the longest call chain in GCC's own call graph (the .ci files),
checked against the linked ELF and completed from it:

- Frame sizes come from the ELF's call-frame information (.debug_frame): the
  largest distance between the stack pointer and its value at entry. GCC's
  .ci figure must agree, except that it leaves out up to 16 bytes when a
  function pushes its own register arguments. Any other difference means the
  dumps belong to another build and fails the check.
- Every branch in the ELF's code from one function into another is an edge,
  whether or not the .ci files have it, so a direct call cannot be missing.

The rules for what neither says:

- A call through a pointer may reach any function whose address GCC records
  as taken (the .cgraph dumps). Calls to long_call functions go through a
  literal but are ordinary direct edges and do not take the address.
- A function that the compiler merged into an identical one is an ELF alias
  without a frame of its own; it is resolved through its address.
- Every function linked into the ROM must have a frame record, so a missing
  or partial dump fails the check. The exceptions are linker veneers, which
  use no stack, and libgcc's arithmetic helpers, which count LIBGCC_BYTES.
- GCC's Thumb switch helpers (__gnu_thumb1_case_*) are not call-graph edges;
  they push two words at the bottom of a chain, which HELPER_BYTES adds.
- A call cycle has no bound of its own. Each cycle the program can reach must
  be declared in the config with the number of activations of its functions
  that can be live at once and the fingerprint of the cycle that was read:
  `recursion NAME COUNT FINGERPRINT` for a cycle of direct calls,
  `callback-cycle NAME COUNT FINGERPRINT` for a cycle that exists only through
  the pointer rule. An undeclared cycle fails the check, and so does a
  declared one whose members, pointer-call sites or address-taken members
  have changed since.

Interrupt handlers run on the separate IRQ stack and are bounded on their own.
IRQ_HANDLERS names every function the sources store in the BIOS vector; a
unit test keeps that list equal to the stores in the tree.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys

STACK_TOP = 0x03007E00      # common/start.s: system-mode stack pointer
IRQ_STACK_TOP = 0x03007FA0  # common/start.s: IRQ-mode stack pointer
IRQ_BIOS_BYTES = 24         # r0-r3, r12 and lr pushed by the BIOS dispatcher
IRQ_HANDLERS = ("audio_irq", "present_epoch_irq")   # stored to 0x03007FFC in common/gba.c
HELPER_BYTES = 8            # __gnu_thumb1_case_*: push {r0, r1}
# Deepest libgcc chain: __aeabi_(u)ldivmod 16, __udivmoddi4 48, __clzdi2 8.
LIBGCC_BYTES = 96
PRETEND_ARGS_BYTES = 16     # r0-r3 pushed by a callee; absent from GCC's stack usage
LIBGCC = re.compile(r"__(aeabi_\w+|gnu_thumb1_case_\w+|u?(div|mod|divmod)[sd]i[34]|(clz|ctz)[sd]i2|muldi3)\Z")
VENEER = re.compile(r"__.+(_veneer|_from_arm|_from_thumb)\Z")
IWRAM_BASE = 0x03000000
SECTIONS_NAME = "iwram_hot_sections.ld"
LIMIT_NAME = "iwram_hot_limit.ld"
NAME = re.compile(r"[A-Za-z_][A-Za-z0-9_.]*\*?\Z")
CYCLE_KINDS = ("recursion", "callback-cycle")
FINGERPRINT = re.compile(r"[0-9a-f]{8}\Z")
CLONE = re.compile(r"(\.(constprop|isra|part|cold)(\.\d+)?)+\Z")


class HotError(Exception):
    pass


# --------------------------------------------------------------------------
# Config and linker fragments

def parse_config(text: str) -> dict:
    """Return {"limit", "names", "recursion", "callback-cycle"}.

    `limit 0x...` is required. `recursion NAME COUNT FINGERPRINT` and
    `callback-cycle NAME COUNT FINGERPRINT` declare call cycles by any one
    member, as (count, fingerprint); NAME may end in * to cover the compiler's
    .isra/.constprop suffixes. Every other line names a .text.<function>
    section to place in IWRAM."""
    config: dict = {"limit": None, "names": [], "recursion": {}, "callback-cycle": {}}
    for number, raw in enumerate(text.splitlines(), 1):
        line = raw.split("#", 1)[0].strip()
        if not line:
            continue
        fields = line.split()
        if fields[0] == "limit":
            if config["limit"] is not None or len(fields) != 2:
                raise HotError(f"line {number}: expected one `limit 0x...` line")
            try:
                config["limit"] = int(fields[1], 16)
            except ValueError as exc:
                raise HotError(f"line {number}: limit is not hexadecimal") from exc
            continue
        if fields[0] in CYCLE_KINDS:
            if (len(fields) != 4 or not NAME.match(fields[1]) or not fields[2].isdigit() or
                    int(fields[2]) < 1 or not FINGERPRINT.match(fields[3])):
                raise HotError(f"line {number}: expected `{fields[0]} NAME COUNT FINGERPRINT` "
                               "with COUNT of 1 or more and an 8-digit fingerprint")
            if fields[1] in config[fields[0]]:
                raise HotError(f"line {number}: {fields[1]} is declared twice")
            config[fields[0]][fields[1]] = (int(fields[2]), fields[3])
            continue
        if len(fields) != 1 or not NAME.match(line):
            raise HotError(f"line {number}: not a function name: {line!r}")
        if line in config["names"]:
            raise HotError(f"line {number}: {line} is listed twice")
        config["names"].append(line)
    limit = config["limit"]
    if limit is None:
        raise HotError("missing `limit 0x...` line")
    if not IWRAM_BASE < limit <= STACK_TOP or limit & 3:
        raise HotError(f"limit {limit:#010x} is not a word address inside IWRAM below the stack top")
    return config


def matches(name: str, pattern: str) -> bool:
    """A listed NAME is that function; NAME* is it and its compiler clones
    (NAME.isra.0, NAME.constprop.0, ...), never another function whose name
    merely starts the same way."""
    if pattern.endswith("*"):
        return name == pattern[:-1] or name.startswith(pattern[:-1] + ".")
    return name == pattern


def fragments(limit: int, names: list[str]) -> dict[str, str]:
    note = "/* Generated by tools/gba_iwram_hot.py; do not edit. */\n"
    sections = note + "".join(f"*(.text.{name[:-1]})\n*(.text.{name[:-1]}.*)\n" if name.endswith("*")
                              else f"*(.text.{name})\n" for name in names)
    return {SECTIONS_NAME: sections, LIMIT_NAME: note + f"__iwram_hot_limit = {limit:#010x};\n"}


def write_fragments(config_path: Path, outdir: Path) -> None:
    config = parse_config(config_path.read_text(encoding="utf-8"))
    outdir.mkdir(parents=True, exist_ok=True)
    # Whole files only, the limit last: make reads its timestamp as "both done".
    for name, text in fragments(config["limit"], config["names"]).items():
        partial = outdir / (name + ".tmp")
        partial.write_text(text, encoding="utf-8")
        os.replace(partial, outdir / name)


# --------------------------------------------------------------------------
# GCC dumps

GRAPH = re.compile(r'graph: \{ title: "[^"]*"\Z')
NODE = re.compile(r'node: \{ title: "([^"]+)" label: "([^"]*)"(?: shape : \w+)? \}\Z')
EDGE = re.compile(r'edge: \{ sourcename: "([^"]+)" targetname: "([^"]+)"(?: label: "([^"]*)")? \}\Z')
FRAME = re.compile(r"(\d+) bytes \((\w+)")
SYMBOL = re.compile(r"(\S+)/\d+ \(")
INDIRECT = "__indirect_call"


def symbol_of(title: str) -> str:
    """A .ci title is `name` for an external function and `file:name` for a
    static one; either way the name is the ELF symbol."""
    return title.rsplit(":", 1)[-1]


def unit_of(title: str) -> str | None:
    """Source file name of a static function's title, None for an external one."""
    return Path(title.rsplit(":", 1)[0]).name if ":" in title else None


def parse_callgraph(texts: list[str]) -> tuple[dict[str, int], dict[str, set[str]], dict[str, int]]:
    """Return (frame bytes by title, callee titles by title, pointer-call
    sites by title)."""
    frames: dict[str, int] = {}
    edges: dict[str, set[str]] = {}
    places: dict[str, set[str]] = {}
    for text in texts:
        lines = text.splitlines()
        # GCC writes one record per line between `graph: {` and `}`. A file
        # cut short would keep frames and lose the calls below them.
        if len(lines) < 2 or not GRAPH.match(lines[0]) or lines[-1] != "}":
            raise HotError("a .ci file is not a complete call graph")
        for number, line in enumerate(lines[1:-1], 2):
            node, edge = NODE.match(line), EDGE.match(line)
            if node:
                title, label = node.groups()
                frame = FRAME.search(label)
                if not frame:
                    continue        # a declaration; the defining unit has the frame
                if frame.group(2) != "static":
                    raise HotError(f"{title} has a {frame.group(2)} frame; only fixed frames can be bounded")
                frames[title] = max(frames.get(title, 0), int(frame.group(1)))
            elif edge:
                source, target, where = edge.groups()
                edges.setdefault(source, set()).add(target)
                if target == INDIRECT:
                    places.setdefault(source, set()).add(where or "")
            else:
                raise HotError(f"line {number} of a .ci file is not a call-graph record: {line[:60]!r}")
    return frames, edges, {title: len(found) for title, found in places.items()}


def parse_address_taken(lines) -> tuple[set[str], set[str]]:
    """From a -fdump-ipa-cgraph dump: (assembler names of the functions marked
    `Address is taken`, assembler names of every function it defines)."""
    taken: set[str] = set()
    defined: set[str] = set()
    name, function = None, False
    for line in lines:
        if not line.startswith(" "):
            match = SYMBOL.match(line)
            name, function = (match.group(1) if match else None), False
        elif line.startswith("  Type: function"):
            function = True
            if name and "definition" in line:
                defined.add(name)
        elif line.startswith("  Address is taken") and function and name:
            taken.add(name)
    return taken, defined


def cycle_fingerprint(kind: str, group: list[str], sites: dict[str, int], taken: set[str],
                      edges: dict[str, set[str]] | None = None) -> str:
    """What a reviewer read when declaring a cycle. For recursion, the member
    functions and the direct calls among them. For a callback cycle, the
    members that call through a pointer or have their address taken, with
    their pointer-call site counts. Compiler clone suffixes are dropped so an
    unrelated optimization does not change it."""
    def plain(title):
        return CLONE.sub("", symbol_of(title))
    if kind == "recursion":
        inside = set(group)
        calls = sorted({(plain(m), plain(t)) for m in group for t in (edges or {}).get(m, ()) if t in inside})
        items: list = [sorted({plain(m) for m in group}), [list(pair) for pair in calls]]
    else:
        surface: dict[str, list] = {}
        for member in group:
            if sites.get(member) or member in taken:
                entry = surface.setdefault(plain(member), [0, False])
                entry[0] += sites.get(member, 0)
                entry[1] = entry[1] or member in taken
        items = sorted([name, *entry] for name, entry in surface.items())
    return hashlib.sha256(json.dumps([kind, items]).encode()).hexdigest()[:8]


def strong_components(nodes: list[str], succ) -> list[list[str]]:
    """Tarjan's algorithm without recursion; components come out callees first."""
    index: dict[str, int] = {}
    low: dict[str, int] = {}
    on: set[str] = set()
    stack: list[str] = []
    out: list[list[str]] = []
    for start in nodes:
        if start in index:
            continue
        work = [(start, iter(succ(start)))]
        index[start] = low[start] = len(index)
        stack.append(start)
        on.add(start)
        while work:
            node, callees = work[-1]
            for callee in callees:
                if callee not in index:
                    index[callee] = low[callee] = len(index)
                    stack.append(callee)
                    on.add(callee)
                    work.append((callee, iter(succ(callee))))
                    break
                if callee in on:
                    low[node] = min(low[node], index[callee])
            else:
                work.pop()
                if work:
                    low[work[-1][0]] = min(low[work[-1][0]], low[node])
                if low[node] == index[node]:
                    component = []
                    while True:
                        member = stack.pop()
                        on.discard(member)
                        component.append(member)
                        if member == node:
                            break
                    out.append(component)
    return out


def cycle_groups(nodes: list[str], succ) -> list[list[str]]:
    return [sorted(c) for c in strong_components(nodes, succ) if len(c) > 1 or c[0] in succ(c[0])]


def stack_bound(frames: dict[str, int], edges: dict[str, set[str]], taken: set[str], root: str, *,
                recursion: dict | None = None, callback_cycles: dict | None = None,
                frameless=None, sites: dict[str, int] | None = None) -> dict:
    """Longest chain from root.

    `taken` holds the titles a pointer call may reach. `recursion` and
    `callback_cycles` map a member's symbol name (or a pattern for it) to
    (live activations, fingerprint) of its cycle; a bare count skips the
    fingerprint. A cycle reachable from root without a declaration is
    reported in "undeclared", one whose fingerprint differs in "changed".
    `frameless(name)` gives the bytes of a callee without a frame record, or
    None when it has no known cost. `sites` counts pointer-call sites."""
    recursion, callback_cycles, sites = recursion or {}, callback_cycles or {}, sites or {}
    frameless = frameless or (lambda name: None)
    pointer_targets = sorted(taken)

    def direct(node):
        return sorted(t for t in edges.get(node, ()) if t != INDIRECT)

    def calls(node):
        callees = direct(node)
        if INDIRECT in edges.get(node, ()):
            callees = sorted(set(callees) | set(pointer_targets))
        return callees

    # Only what the root can reach matters, and only that must be declared.
    nodes, queue = [root], [root]
    seen = {root}
    while queue:
        for callee in calls(queue.pop()):
            if callee not in seen:
                seen.add(callee)
                nodes.append(callee)
                queue.append(callee)
    nodes.sort()
    missing = sorted(symbol_of(n) for n in nodes if n not in frames and frameless(symbol_of(n)) is None)
    if missing:
        raise HotError("no frame record for " + ", ".join(missing[:8]) +
                       (f" and {len(missing) - 8} more" if len(missing) > 8 else ""))

    def declared(group, table):
        found = [value if isinstance(value, tuple) else (value, None) for pattern, value in table.items()
                 if any(matches(symbol_of(m), pattern) for m in group)]
        return max(found, key=lambda value: value[0]) if found else None

    factor = {node: 1 for node in nodes}
    undeclared = []
    changed = []
    cycles = []
    direct_groups = cycle_groups(nodes, direct)
    for kind, groups, table in (("recursion", direct_groups, recursion),
                                ("callback-cycle", cycle_groups(nodes, calls), callback_cycles)):
        for group in groups:
            if (kind == "callback-cycle" and group in direct_groups and
                    not (any(INDIRECT in edges.get(m, ()) for m in group) and any(m in taken for m in group))):
                continue        # the same cycle with no pointer call back into it: recursion only
            found = declared(group, table)
            names = [symbol_of(m) for m in group]
            cycle = {"kind": kind, "functions": names, "count": found[0] if found else None,
                     "fingerprint": cycle_fingerprint(kind, group, sites, taken, edges)}
            cycles.append(cycle)
            if found is None:
                undeclared.append(cycle)
                continue
            if found[1] not in (None, cycle["fingerprint"]):
                changed.append(cycle)
            for member in group:
                factor[member] *= found[0]

    def cost(node):
        return (frames[node] if node in frames else frameless(symbol_of(node))) * factor[node]

    components = strong_components(nodes, calls)   # callees first
    owner = {member: i for i, component in enumerate(components) for member in component}
    depth = [0] * len(components)
    below: list[int | None] = [None] * len(components)
    for i, component in enumerate(components):
        best, via = 0, None
        for member in component:
            for callee in calls(member):
                j = owner[callee]
                if j != i and depth[j] > best:
                    best, via = depth[j], j
        depth[i] = sum(cost(member) for member in component) + best
        below[i] = via
    chain = []
    i: int | None = owner[root]
    while i is not None:
        members = sorted(components[i], key=lambda m: (-cost(m), m))
        chain.append({"bytes": sum(cost(m) for m in members),
                      "functions": [{"name": symbol_of(m), "bytes": cost(m)} for m in members]})
        i = below[i]
    return {"bytes": depth[owner[root]] + HELPER_BYTES, "chain": chain, "cycles": cycles,
            "undeclared": undeclared, "changed": changed}


# --------------------------------------------------------------------------
# ELF facts

def tool_output(tool: str, *args: str) -> str:
    prefix = os.environ.get("PREFIX", "arm-none-eabi-")
    try:
        return subprocess.run([prefix + tool, *args], text=True, check=True,
                              stdout=subprocess.PIPE, stderr=subprocess.PIPE).stdout
    except (OSError, subprocess.CalledProcessError) as exc:
        raise HotError(f"{prefix}{tool} failed: {exc}") from exc


def parse_symbols(listing: str) -> tuple[dict[str, int], list[tuple[int, int, str]], set[str]]:
    """From `readelf -s -W`: (value by name, [(address, size, name)] for every
    defined function symbol of any binding, source file names)."""
    values: dict[str, int] = {}
    functions = []
    files: set[str] = set()
    for line in listing.splitlines():
        fields = line.split()
        if len(fields) != 8 or not fields[0].endswith(":") or fields[6] == "UND":
            continue
        try:
            value, size = int(fields[1], 16), int(fields[2], 0)
        except ValueError:
            continue
        if fields[3] == "FILE":
            files.add(Path(fields[7]).name)
            continue
        values[fields[7]] = value
        if fields[3] == "FUNC" and size:
            functions.append((value & ~1, size, fields[7]))
    return values, sorted(functions), files


FDE = re.compile(r"FDE cie=[0-9a-f]+ pc=([0-9a-f]+)\.\.([0-9a-f]+)")
CFA = re.compile(r"[0-9a-f]{8} (r\d+)\+(\d+)")
BRANCH = re.compile(r"\s*([0-9a-f]+):\tb[a-z.]*\t([0-9a-f]+) <")


def parse_frames(listing: str) -> dict[int, int]:
    """From `readelf --debug-dump=frames-interp`: the deepest stack-pointer
    offset of each function, by start address. A frame described through
    another register cannot be bounded here."""
    depth: dict[int, int] = {}
    start = None
    for line in listing.splitlines():
        fde = FDE.search(line)
        if fde:
            start = int(fde.group(1), 16) & ~1
            depth.setdefault(start, 0)
            continue
        row = CFA.match(line)
        if row and start is not None:
            if row.group(1) != "r13":
                raise HotError(f"the function at {start:#010x} addresses its frame through {row.group(1)}")
            depth[start] = max(depth[start], int(row.group(2)))
    return depth


def parse_branches(listing: str) -> list[tuple[int, int]]:
    """From `objdump -d --no-show-raw-insn`: (address, target) of every branch
    instruction with a fixed target."""
    return [(int(a, 16), int(t, 16)) for a, t in BRANCH.findall(listing)]


def resolve_aliases(frames: dict[str, int], edges: dict[str, set[str]],
                    functions: list[tuple[int, int, str]]) -> dict[str, str]:
    """Map each callee title without a frame to the title that owns the same
    address, when the ELF has one. Returns {alias title: owning title}."""
    by_symbol: dict[str, list[str]] = {}
    for title in frames:
        by_symbol.setdefault(symbol_of(title), []).append(title)
    address = {name: start for start, _, name in functions}
    at: dict[int, list[str]] = {}
    for start, _, name in functions:
        at.setdefault(start, []).append(name)
    aliases: dict[str, str] = {}
    for title in {t for targets in edges.values() for t in targets} - set(frames) - {INDIRECT}:
        start = address.get(symbol_of(title))
        owners = [t for name in at.get(start, ()) for t in by_symbol.get(name, ())] if start is not None else []
        if len(owners) == 1:
            aliases[title] = owners[0]
    return aliases


# --------------------------------------------------------------------------
# check

def check(elf: Path, dump_dir: Path, config_path: Path, *, margin: int, entry: str,
          irq_roots: list[str]) -> dict:
    if margin < 0:
        raise HotError("margin must not be negative")
    config = parse_config(config_path.read_text(encoding="utf-8"))
    graphs = sorted(dump_dir.glob("*.ci"))
    if not graphs:
        raise HotError(f"no .ci files in {dump_dir}; compile with -fcallgraph-info=su -dumpdir {dump_dir}/")
    frames, edges, sites = parse_callgraph([p.read_text(encoding="utf-8", errors="replace") for p in graphs])
    values, functions, files = parse_symbols(tool_output("readelf", "-s", "-W", str(elf)))
    for name in ("__iwram_end", "__iwram_hot_start", "__iwram_hot_limit"):
        if name not in values:
            raise HotError(f"{elf} has no {name}; link it with common/gba_hot.ld")
    end, hot_start = values["__iwram_end"], values["__iwram_hot_start"]

    aliases = resolve_aliases(frames, edges, functions)
    edges = {source: {aliases.get(t, t) for t in targets} for source, targets in edges.items()}
    titles: dict[str, list[str]] = {}
    for title in frames:
        titles.setdefault(symbol_of(title), []).append(title)
    addresses: dict[str, set[int]] = {}
    names_at: dict[int, set[str]] = {}
    for start, _, name in functions:
        addresses.setdefault(name, set()).add(start)
        names_at.setdefault(start, set()).add(name)

    # Every linked function needs a frame, or the dumps are not this ROM's.
    framed = {start for start, _, name in functions if name in titles}
    uncovered = [name for start, _, name in functions
                 if name not in titles and start not in framed and not VENEER.match(name) and not LIBGCC.match(name)]
    if uncovered:
        raise HotError(f"{len(uncovered)} linked functions have no frame record in {dump_dir} "
                       f"(first: {', '.join(sorted(uncovered)[:6])}); the dumps do not match {elf}")

    def owner(address: int):
        """(start, size, name) of the function holding an address."""
        low, high = 0, len(functions)
        while low < high:
            mid = (low + high) // 2
            if functions[mid][0] <= address:
                low = mid + 1
            else:
                high = mid
        for start, size, name in reversed(functions[max(0, low - 8):low]):
            if start <= address < start + size:
                return start, size, name
        return None

    def node_at(start: int) -> str | None:
        """The call-graph node for the function at an address."""
        found = sorted(t for name in names_at.get(start, ()) for t in titles.get(name, ()))
        if found:
            return found[0] if len(found) == 1 else None
        helpers = sorted(name for name in names_at.get(start, ()) if LIBGCC.match(name))
        return helpers[0] if helpers else None

    # Frames: the ELF's call-frame information is what the linked code does.
    measured = parse_frames(tool_output("readelf", "--debug-dump=frames-interp", str(elf)))
    adjusted = 0
    for start, _, name in functions:
        for title in titles.get(name, ()):
            if start not in measured:
                raise HotError(f"{name} has no call-frame information in {elf}; build with -g")
            extra = measured[start] - frames[title]
            if len(titles[name]) == 1 and not 0 <= extra <= PRETEND_ARGS_BYTES:
                raise HotError(f"{name} uses {measured[start]} bytes of stack in {elf} but {frames[title]} in "
                               f"{dump_dir}; the dumps are from another build")
            if extra > 0:
                frames[title] += extra
                adjusted += 1

    # Edges: every branch from one linked function into another.
    code_end = max(start + size for start, size, _ in functions if start >> 24 == 8)
    code = tool_output("objdump", "-d", "--no-show-raw-insn", "-j", ".text", "-j", ".iwram", str(elf))
    if any(line.split()[1:2] == [".rom"] for line in tool_output("objdump", "-h", str(elf)).splitlines()):
        first = min(start for start, _, _ in functions if start >> 24 == 8)
        code += tool_output("objdump", "-d", "--no-show-raw-insn", "-j", ".rom", f"--start-address={first:#x}",
                            f"--stop-address={code_end:#x}", str(elf))
    # libgcc's assembly entry points have no size; find them by exact address.
    helper_at: dict[int, str] = {}
    for name, value in values.items():
        if LIBGCC.match(name):
            helper_at.setdefault(value & ~1, name)

    def node_for(address: int, via: str) -> str | None:
        """The node a branch to this address calls; None when it leaves the
        functions (the reset code)."""
        callee = owner(address)
        if callee is None:
            return helper_at.get(address)
        if VENEER.match(callee[2]):     # a veneer stands for the function it is named after
            real = values.get(re.sub(r"\A__|_(veneer|from_arm|from_thumb)\Z", "", callee[2]))
            if real is None:
                raise HotError(f"cannot resolve the veneer {callee[2]} reached from {via}")
            return node_for(real & ~1, via)
        to = node_at(callee[0])
        if to is None:
            raise HotError(f"{via} branches to {callee[2]}, which has no single frame record")
        return to

    added = 0
    for address, target in parse_branches(code):
        source, callee = owner(address), owner(target)
        if not source or VENEER.match(source[2]) or (callee and callee[0] == source[0]):
            continue
        to = node_for(target, source[2])
        if to is None:
            continue
        for title in titles.get(source[2], ()):
            if to not in edges.setdefault(title, set()):
                edges[title].add(to)
                added += 1

    taken: set[str] = set()
    defined: dict[str, set[str]] = {}
    dumps = sorted(dump_dir.glob("*.cgraph"))
    odd = sorted({p.name.split(".")[0] for p in dumps} ^ {p.stem for p in graphs})
    if odd:
        raise HotError(f"{dump_dir} needs one .cgraph dump per .ci file (unpaired: {', '.join(odd)}); compile "
                       "with -fdump-ipa-cgraph, and delete dumps another command left there")
    for dump in dumps:
        unit = dump.name.split(".")[0] + ".c"
        with dump.open(encoding="utf-8", errors="replace") as lines:
            marked, defined[unit] = parse_address_taken(lines)
        for name in marked:
            found = {t for t in titles.get(name, ()) if unit_of(t) in (None, unit)}
            if not found:       # merged into an identical function: use the frame at its address
                found = {t for start in addresses.get(name, ()) for other in names_at[start]
                         for t in titles.get(other, ())}
            if not found and name in addresses:
                raise HotError(f"{name} has its address taken and is linked into {elf}, "
                               "but no frame record resolves it")
            taken |= found
    # The address-taken record must cover the same functions as the frames: a
    # function GCC compiled as written (no clone suffix) is in its unit's dump.
    everywhere = set().union(*defined.values())
    absent = sorted(symbol_of(t) for t in frames if not CLONE.search(symbol_of(t)) and
                    symbol_of(t) not in (defined.get(unit_of(t), ()) if unit_of(t) else everywhere))
    if absent:
        raise HotError(f"the .cgraph dumps in {dump_dir} do not list {len(absent)} compiled functions "
                       f"(first: {', '.join(absent[:6])}); they are truncated or from another build")
    # The reset code's reference to the entry point is in assembly and leaves
    # no address-taken record. If C code takes its address, the record stays
    # and the entry point shows up in a callback cycle that must be declared.
    roots = titles.get(entry, [])
    if len(roots) != 1:
        raise HotError(f"entry point {entry} is not a single function in the call graph")

    options = dict(recursion=config["recursion"], callback_cycles=config["callback-cycle"], sites=sites,
                   frameless=lambda name: ((HELPER_BYTES if name.startswith("__gnu_thumb1_case_") else LIBGCC_BYTES)
                                           if LIBGCC.match(name) else None))
    main = stack_bound(frames, edges, taken, roots[0], **options)
    irq = []
    undeclared = list(main["undeclared"])
    changed = list(main["changed"])
    for name in irq_roots:
        if name not in titles:
            if name in addresses:
                raise HotError(f"interrupt handler {name} is linked into {elf} but has no frame record")
            continue
        for title in titles[name]:
            result = stack_bound(frames, edges, taken, title, **options)
            irq.append({"name": name, "bytes": result["bytes"]})
            undeclared += [c for c in result["undeclared"] if c not in undeclared]
            changed += [c for c in result["changed"] if c not in changed]
    # A ROM without sampled audio or the VBlank counter installs no handler.
    irq_bytes = max(row["bytes"] for row in irq) + IRQ_BIOS_BYTES if irq else 0
    highest = (STACK_TOP - main["bytes"] - margin) & ~3
    placed = [name for start, _, name in functions if hot_start <= start < end]
    report = {"elf": elf.as_posix(), "iwram_end": end, "stack_top": STACK_TOP,
              "stack_bound_bytes": main["bytes"], "margin_bytes": margin,
              "highest_safe_iwram_end": highest, "spare_bytes": highest - end,
              "limit": config["limit"], "chain": main["chain"], "cycles": main["cycles"],
              "address_taken": sorted(symbol_of(t) for t in taken), "aliases": len(aliases),
              "frames_raised_from_elf": adjusted, "edges_added_from_elf": added,
              "irq_stack_bytes": irq_bytes, "irq_stack_room": IRQ_STACK_TOP - STACK_TOP, "irq_handlers": irq,
              "hot_bytes": end - hot_start,
              "hot_functions": sum(1 for name in placed if any(matches(name, p) for p in config["names"])),
              "errors": [], "warnings": []}
    errors = report["errors"]
    # A dump left behind by a source file that is no longer linked can only
    # add chains, so it is safe, but it makes the bound larger than it is.
    linked = {name for _, _, name in functions}
    for graph in graphs:
        own = {symbol_of(t) for t in parse_callgraph([graph.read_text(encoding="utf-8", errors="replace")])[0]}
        if own and graph.stem + ".c" not in files and not own & linked:
            report["warnings"].append(f"{graph.name} matches no source file linked into {elf}; delete the stale dump")
    for cycle in undeclared:
        names = ", ".join(cycle["functions"][:6]) + (" ..." if len(cycle["functions"]) > 6 else "")
        errors.append(f"undeclared call cycle ({names}); add `{cycle['kind']} {cycle['functions'][0]} COUNT "
                      f"{cycle['fingerprint']}` to {config_path} once its nesting is known")
    for cycle in changed:
        names = ", ".join(cycle["functions"][:6]) + (" ..." if len(cycle["functions"]) > 6 else "")
        errors.append(f"call cycle ({names}) changed since it was declared; read it again and set its "
                      f"{cycle['kind']} fingerprint in {config_path} to {cycle['fingerprint']}")
    if values["__iwram_hot_limit"] != config["limit"]:
        errors.append(f"{elf} was linked with limit {values['__iwram_hot_limit']:#010x}, "
                      f"{config_path} says {config['limit']:#010x}; relink")
    if end > highest:
        errors.append(f"IWRAM ends at {end:#010x} but the stack may reach {STACK_TOP - main['bytes']:#010x}; "
                      f"with a {margin}-byte margin the code must end by {highest:#010x}")
    if config["limit"] > highest:
        report["warnings"].append(f"{config_path} allows IWRAM up to {config['limit']:#010x}, above what the "
                                  f"stack leaves; lower its limit to {highest:#010x} or less")
    if irq_bytes > IRQ_STACK_TOP - STACK_TOP:
        errors.append(f"interrupt handlers may use {irq_bytes} bytes of a "
                      f"{IRQ_STACK_TOP - STACK_TOP}-byte IRQ stack")
    every = [name for _, _, name in functions]
    for name in placed:
        if not VENEER.match(name) and not any(matches(name, pattern) for pattern in config["names"]):
            report["warnings"].append(f"{name} is in the hot IWRAM range but not on the list")
    for pattern in config["names"]:
        if not any(matches(name, pattern) for name in placed):
            where = "is in ROM" if any(matches(n, pattern) for n in every) else "is not in this ROM"
            report["warnings"].append(f"{pattern} is listed but {where}")
    return report


def render(report: dict) -> str:
    lines = [f"{report['elf']}: stack bound {report['stack_bound_bytes']} B "
             f"+ {report['margin_bytes']} B margin below {report['stack_top']:#010x}",
             f"  IWRAM ends at {report['iwram_end']:#010x}; it may end at "
             f"{report['highest_safe_iwram_end']:#010x} ({report['spare_bytes']} B spare)",
             (f"  IRQ stack: {report['irq_stack_bytes']} of {report['irq_stack_room']} B" if report["irq_handlers"]
              else "  IRQ stack: no interrupt handler is linked"),
             "  deepest chain:"]
    for step in report["chain"]:
        names = ", ".join(f["name"] for f in step["functions"][:6])
        more = len(step["functions"]) - 6
        group = f"cycle of {len(step['functions'])}: " if len(step["functions"]) > 1 else ""
        lines.append(f"    {step['bytes']:6d}  {group}{names}{f', and {more} more' if more > 0 else ''}")
    lines.append(f"  hot list: {report['hot_functions']} functions, {report['hot_bytes']} B with veneers")
    lines += [f"  warning: {text}" for text in report["warnings"]]
    lines += [f"  error: {text}" for text in report["errors"]]
    return "\n".join(lines)


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    frag = commands.add_parser("fragments", help="write the linker fragments for a hot list")
    frag.add_argument("config", type=Path)
    frag.add_argument("outdir", type=Path)
    chk = commands.add_parser("check", help="bound the stack and check the IWRAM end against it")
    chk.add_argument("--elf", type=Path, required=True)
    chk.add_argument("--dump-dir", type=Path, required=True, help="directory of the build's .ci and .cgraph dumps")
    chk.add_argument("--config", type=Path, required=True)
    chk.add_argument("--margin", type=int, default=512, help="bytes kept free below the bound (default 512)")
    chk.add_argument("--entry", default="main")
    chk.add_argument("--irq-root", action="append", default=None,
                     help="interrupt handler to bound on the IRQ stack (default: every handler in the sources)")
    chk.add_argument("--json", type=Path, help="also write the report as JSON")
    args = parser.parse_args(argv)
    try:
        if args.command == "fragments":
            write_fragments(args.config, args.outdir)
            return 0
        report = check(args.elf, args.dump_dir, args.config, margin=args.margin, entry=args.entry,
                       irq_roots=args.irq_root or list(IRQ_HANDLERS))
    except (HotError, OSError) as exc:
        print(f"gba_iwram_hot: {exc}", file=sys.stderr)
        return 2
    print(render(report))
    if args.json:
        args.json.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    return 1 if report["errors"] else 0


if __name__ == "__main__":
    sys.exit(main())
