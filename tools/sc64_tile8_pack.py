#!/usr/bin/env python3
"""Derive a hardware-tile copy of an imported race-map header.

Reads an existing ignored ``sc64_races_generated.h`` and writes a copy for the
experimental SC64_TILE_RENDERER probe. Each 16x16 8bpp terrain tile array
becomes a one-tile placeholder; its pixels move into a shared pool of unique
8x8 tiles, deduplicated across horizontal/vertical flips and across all maps
compiled under the same preprocessor conditions. Each source tile gets four
GBA-style entries (top-left, top-right, bottom-left, bottom-right):
``pool index | hflip << 14 | vflip << 15``. ``Sc64RaceMap`` gains matching
``tile8_meta``/``tile8_pool`` pointers. No game data is read beyond the input
header, and the input header is not modified.
"""
from __future__ import annotations

import argparse
import re
from pathlib import Path

TILES_RE = re.compile(r"^static const unsigned char (sc64_race_(\w+?)_tiles)\[(\d+)\]\[256\]"
                      r" __attribute__\(\(aligned\(4\)\)\) = \{\s*$")
LOOKUP_RE = re.compile(r"^static const unsigned short (sc64_race_(\w+?)_lookup)\[(\d+)\] = \{\s*$")
STRUCT_END = "    const char *title; const char *mission_id;\n} Sc64RaceMap;\n"
MAX_POOL = 1 << 14
# Union of the previous and next visible windows for camera moves under 8px.
WINDOW_COLS, WINDOW_ROWS = 32, 16


def split8(tile: bytes) -> list[bytes]:
    return [bytes(tile[(r + y) * 16 + c + x] for y in range(8) for x in range(8))
            for r in (0, 8) for c in (0, 8)]


def variants(sub: bytes) -> list[tuple[bytes, int]]:
    rows = [sub[i * 8:i * 8 + 8] for i in range(8)]
    h = b"".join(r[::-1] for r in rows)
    v = b"".join(rows[::-1])
    hv = b"".join(r[::-1] for r in rows[::-1])
    # Flag bits describe how to flip the stored tile to reproduce ``sub``.
    return [(sub, 0), (h, 1 << 14), (v, 1 << 15), (hv, 3 << 14)]


def parse_rows(lines: list[str], start: int, count: int, width: int) -> tuple[list[bytes], int]:
    values: list[int] = []
    i = start
    while True:
        line = lines[i]
        i += 1
        if line.startswith("};"):
            break
        values.extend(int(x) for x in re.findall(r"\d+", line))
    if len(values) != count * width:
        raise SystemExit(f"array at line {start} has {len(values)} values, expected {count * width}")
    rows = [bytes(values[n * width:(n + 1) * width]) for n in range(count)] if width > 1 else values
    return rows, i


def guard_signature(stack: list[tuple[str, int]]) -> tuple:
    return tuple(stack)


def check_elf(elf: Path, alias: int) -> int:
    """Scan linked sprite and pool pixels for the reserved alias index."""
    import subprocess
    import tempfile
    names = subprocess.check_output(["arm-none-eabi-nm", "-S", str(elf)], text=True)
    with tempfile.TemporaryDirectory() as directory:
        rom = Path(directory) / "text.bin"
        subprocess.check_call(["arm-none-eabi-objcopy", "-O", "binary", "-j", ".text", str(elf), str(rom)])
        data = rom.read_bytes()
    checked = bad = 0
    for line in names.splitlines():
        fields = line.split()
        if len(fields) != 4 or not re.search(r"(_sprite_\w+_pixels|sc64_tile8_pool_\d+)$", fields[3]):
            continue
        start, size = int(fields[0], 16) - 0x08000000, int(fields[1], 16)
        checked += 1
        if alias in data[start:start + size]:
            print(f"{fields[3]} uses reserved palette index {alias}")
            bad += 1
    print(f"checked {checked} pixel arrays for index {alias}: {bad} conflicts")
    return 1 if bad or not checked else 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--input", type=Path)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--alias", type=int, default=255,
                        help="palette index the renderer reserves for opaque black")
    parser.add_argument("--check-elf", type=Path,
                        help="fail if any linked sprite or tile8 pixel uses --alias")
    args = parser.parse_args()
    if args.check_elf:
        return check_elf(args.check_elf, args.alias)
    if not (args.input and args.output):
        parser.error("--input and --output are required")
    lines = args.input.read_text().splitlines(keepends=True)

    # Pass 1: locate tile/lookup arrays, their guard signatures and map shapes.
    stack: list[tuple[str, int]] = []
    tiles: dict[str, dict] = {}
    lookups: dict[str, list[int]] = {}
    i = 0
    while i < len(lines):
        line = lines[i]
        s = line.strip()
        if s.startswith(("#if ", "#ifdef ", "#ifndef ")):
            stack.append((s, 0))
        elif s.startswith(("#elif", "#else")):
            top, branch = stack.pop()
            stack.append((top, branch + 1))
        elif s.startswith("#endif"):
            stack.pop()
        m = TILES_RE.match(line)
        if m:
            rows, end = parse_rows(lines, i + 1, int(m.group(3)), 256)
            tiles[m.group(2)] = {"name": m.group(1), "line": i, "end": end,
                                 "rows": rows, "sig": guard_signature(stack)}
            i = end
            continue
        m = LOOKUP_RE.match(line)
        if m:
            values, end = parse_rows(lines, i + 1, int(m.group(3)), 1)
            lookups[m.group(2)] = values
            i = end
            continue
        i += 1
    if not tiles:
        raise SystemExit("no race-map tile arrays found")
    text = "".join(lines)
    widths = {mid: int(w) for w, mid in re.findall(r"\{(\d+)u,\d+u,\d+u,\d+u,[^{}]*?sc64_race_(\w+?)_tiles,", text)}

    # Pass 2: build one flip-deduplicated pool per guard signature.
    groups: dict[tuple, list[str]] = {}
    for mid, info in tiles.items():
        groups.setdefault(info["sig"], []).append(mid)
    group_info: dict[tuple, dict] = {}
    worst = 0
    for gi, (sig, members) in enumerate(groups.items()):
        index: dict[bytes, int] = {}
        pool: list[bytes] = []
        for mid in members:
            meta = []
            for tile in tiles[mid]["rows"]:
                entry = []
                for sub in split8(tile):
                    for candidate, flags in variants(sub):
                        if candidate in index:
                            entry.append(index[candidate] | flags)
                            break
                    else:
                        index[sub] = len(pool)
                        pool.append(sub)
                        entry.append(index[sub])
                meta.append(entry)
            tiles[mid]["meta"] = meta
        if len(pool) >= MAX_POOL:
            raise SystemExit(f"pool {gi} has {len(pool)} tiles; limit {MAX_POOL - 1}")
        group_info[sig] = {"id": gi, "pool": pool, "first": min(members, key=lambda m: tiles[m]["line"]),
                           "alias_used": any(args.alias in t for t in pool)}
        for mid in members:
            tiles[mid]["pool_name"] = f"sc64_tile8_pool_{gi}"
            if mid not in lookups or mid not in widths:
                continue
            look, cols = lookups[mid], widths[mid]
            rows = len(look) // cols
            meta = tiles[mid]["meta"]
            ids = [[meta[look[(ty >> 1) * cols + (tx >> 1)]][(ty & 1) * 2 + (tx & 1)] & (MAX_POOL - 1)
                    for tx in range(cols * 2)] for ty in range(rows * 2)]
            win_cols, win_rows = min(WINDOW_COLS, cols * 2), min(WINDOW_ROWS, rows * 2)
            for wy in range(rows * 2 - win_rows + 1):
                counts: dict[int, int] = {}
                band = ids[wy:wy + win_rows]
                for tx in range(win_cols):
                    for row in band:
                        counts[row[tx]] = counts.get(row[tx], 0) + 1
                worst = max(worst, len(counts))
                for wx in range(1, cols * 2 - win_cols + 1):
                    for row in band:
                        old = row[wx - 1]
                        counts[old] -= 1
                        if not counts[old]:
                            del counts[old]
                        new = row[wx + win_cols - 1]
                        counts[new] = counts.get(new, 0) + 1
                    worst = max(worst, len(counts))

    # Pass 3: write the shadow header.
    out: list[str] = []
    i = 0
    by_line = {info["line"]: mid for mid, info in tiles.items()}
    while i < len(lines):
        if i in by_line:
            mid = by_line[i]
            info = tiles[mid]
            sig = info["sig"]
            g = group_info[sig]
            out.append(f"/* {info['name']}: {len(info['rows'])} 16x16 tiles moved to "
                       f"{info['pool_name']} for SC64_TILE_RENDERER. */\n")
            out.append(f"static const unsigned char {info['name']}[1][256] __attribute__((aligned(4))) = {{{{0}}}};\n")
            if g["first"] == mid:
                pool = g["pool"]
                out.append(f"static const unsigned char {info['pool_name']}[{len(pool)}][64] "
                           f"__attribute__((aligned(4))) = {{\n")
                out.extend("{" + ",".join(map(str, t)) + "},\n" for t in pool)
                out.append("};\n")
            out.append(f"static const unsigned short sc64_race_{mid}_tile8_meta[{len(info['meta'])}][4] = {{\n")
            out.extend("{" + ",".join(f"{v}u" for v in e) + "},\n" for e in info["meta"])
            out.append("};\n")
            i = info["end"]
            continue
        line = lines[i]
        if line == "    const char *title; const char *mission_id;\n" and lines[i + 1] == "} Sc64RaceMap;\n":
            out.append(line)
            out.append("    const unsigned short (*tile8_meta)[4]; const unsigned char (*tile8_pool)[64];\n")
            out.append("} Sc64RaceMap;\n")
            out.append(f"#define SC64_TILE8_DATA 1\n#define SC64_TILE8_WORST_WINDOW_TILES {worst}\n")
            i += 2
            continue
        if re.match(r"\s*\{\d+u,", line):
            # Map-table initializers may wrap; append the new fields before the closing brace.
            block = line
            while not block.rstrip().endswith(("},", "}")):
                i += 1
                block += lines[i]
            m = re.search(r"sc64_race_(\w+?)_tiles,", block)
            if m and m.group(1) in tiles:
                mid = m.group(1)
                closing = block.rstrip().rfind("}")
                if group_info[tiles[mid]["sig"]]["alias_used"]:
                    fields = ",0,0"  # map keeps the mode-4 renderer
                else:
                    fields = f",sc64_race_{mid}_tile8_meta,{tiles[mid]['pool_name']}"
                block = block[:closing] + fields + block[closing:]
            out.append(block if block.endswith("\n") else block + "\n")
            i += 1
            continue
        out.append(line)
        i += 1
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text("".join(out))
    total16 = sum(len(t["rows"]) for t in tiles.values()) * 256
    total8 = sum(len(g["pool"]) * 64 for g in group_info.values())
    meta = sum(len(t["rows"]) * 8 for t in tiles.values())
    print(f"maps={len(tiles)} pools={len(group_info)} 16x16_bytes={total16} "
          f"pool_bytes={total8} meta_bytes={meta} worst_window_tiles={worst}")
    for sig, g in group_info.items():
        members = [m for m in groups[sig]]
        note = f" (uses index {args.alias}; left on mode 4)" if g["alias_used"] else ""
        print(f"  pool {g['id']}: {len(g['pool'])} tiles for {','.join(members)}{note}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
