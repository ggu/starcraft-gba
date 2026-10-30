#!/usr/bin/env python3
"""Import authenticated StarCraft 64 death animations to a private C header.

For each source unit/building id this follows units.dat -> flingy.dat ->
sprites.dat -> images.dat to the actor image, then compiles its IScript Death
animation into bounded, independent image layers: collapse frames played on
the actor's own GRP, explosion overlays (including overlays they spawn), and
remnant/rubble sprites. It is a narrow whitelist compiler, not an IScript VM;
unsupported control flow fails closed and that actor keeps the authored
fallback. Pixels and the manifest stay below .local/.

Copyright (C) 2026 blizzard-gba contributors
SPDX-License-Identifier: GPL-3.0-or-later
"""
from __future__ import annotations

import argparse
import hashlib
import json
import struct
import sys
from pathlib import Path
from typing import Any, Callable, Sequence

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
from tools import sc64_construction_sprites as cs  # noqa: E402
from tools.sc64_unit_facing import pack_word_phases  # noqa: E402

ROM_DEFAULT = ROOT / ".local/sc64/rom/StarCraft64-USA.z64"
REMAP_DEFAULT = ROOT / ".local/sc64/backwater/generated/palette-remap.bin"
OUT_DEFAULT = ROOT / ".local/sc64/death/generated"
HEADER_DEFAULT = OUT_DEFAULT / "sc64_death_sprites_generated.h"

# Ground/air units, heroes and buildings of the three races (critters and
# resources have no destructive Death layers worth importing).
RAW_IDS = tuple(range(0, 176))
MAX_INSTRUCTIONS = 256
MAX_STEPS = 64
MAX_LAYERS = 8
MAX_DEPTH = 3
HOLD_LAST = 0xFF
# Source rubble decays over minutes; keep at most 30 seconds (24-Hz updates).
REMNANT_CAP_SU = 720
# Pack combat-unit under-layers and persistent building remnants. Source
# building identities occupy raw IDs 106..175; critters and props stay indexed.
WORD_PACK_RAW_ID_LIMIT = 89
BUILDING_RAW_FIRST = 106
BUILDING_RAW_END = 176
# Ignored instructions: setflipstate, playsnd, playsndrand, playsndbtwn,
# setfldirect, and liftoffcondjmp (the grounded fall-through is followed).
SKIP_OPS = {23, 24, 25, 26, 52, 63}
IMAGE_OVER = {8, 10, 13}      # imgol, imgolorig, imgoluselo
IMAGE_UNDER = {9, 11, 14}     # imgul, switchul, imguluselo
SPRITE_OVER = {15, 16, 21}    # sprol, highsprol, sproluselo
SPRITE_UNDER = {17, 19, 20}   # lowsprul, spruluselo, sprul
USES_LO = {13, 14, 19, 21}    # offsets come from LO files: use the origin
# Remapped fire images (draw function 9) store 1..63 intensities rather than
# palette indices; the source blends them over the ground through the
# ofire/gfire/rfire/bfire tables. GBA mode 4 has no per-pixel blending, so
# each intensity band maps to an opaque color of the fixed UI palette in
# common/gba.h (indices 0..31, untouched by imported terrain palettes).
REMAP_RAMPS = {
    1: ("C_RED_DARK", "C_LAVA", "C_ORANGE", "C_GOLD", "C_HIGHLIGHT", "C_WHITE"),
    2: ("C_GREEN_DARK", "C_GREEN_DARK", "C_GREEN", "C_GREEN", "C_HIGHLIGHT", "C_WHITE"),
    3: ("C_BLOOD", "C_RED_DARK", "C_RED", "C_LAVA", "C_ORANGE", "C_WHITE"),
    4: ("C_BLUE_DARK", "C_BLUE", "C_TERRAN", "C_CYAN", "C_CYAN", "C_WHITE"),
}


def ui_palette_indices(header: Path = ROOT / "common/gba.h") -> dict[str, int]:
    """Read the fixed UI color enum (C_BLACK=0, C_WHITE, ...) from gba.h."""
    text = header.read_text(encoding="ascii")
    start = text.index("enum { C_BLACK=0")
    body = text[text.index("{", start) + 1:text.index("}", start)]
    names = [item.strip().split("=")[0] for item in body.split(",") if item.strip()]
    if len(names) != 32:
        raise ValueError(f"unexpected UI palette enum size {len(names)}")
    return {name: index for index, name in enumerate(names)}


def intensity_band(value: int) -> int:
    if value >= 56:
        return 4
    for band, limit in enumerate((8, 16, 26, 36, 44)):
        if value < limit:
            return band
    return 5


def ramp_indices(remap_id: int, ui: dict[str, int]) -> list[int]:
    """GBA UI palette indices for one fire ramp (unknown ids use ofire)."""
    return [ui[name] for name in REMAP_RAMPS.get(remap_id, REMAP_RAMPS[1])]


def packed_under_image_ids(sources: dict[int, dict[str, Any]]) -> set[int]:
    return {layer["image_id"] for raw, source in sources.items()
            for layer in source["layers"]
            if (raw < WORD_PACK_RAW_ID_LIMIT and layer["under"]) or
               (BUILDING_RAW_FIRST <= raw < BUILDING_RAW_END and layer["remnant"])}


def word_payload_bytes(sources: dict[int, dict[str, Any]],
                       images: dict[int, dict[str, Any]]) -> int:
    """Bytes in packed unit under-layer and building-remnant arrays."""
    packed_images = packed_under_image_ids(sources)
    return sum(sum(len(phase) * 4 for phase in pack_word_phases(
        frame["w"], frame["h"], frame["pixels"]))
        for iid, image in images.items() if iid in packed_images
        for frame in image["frames"])


class Unsupported(ValueError):
    pass


def compile_death(payload: bytes, scripts: dict[int, int], image_iscript: Sequence[int],
                  sprite_image: Sequence[int], root_image: int) -> list[dict[str, Any]]:
    """Compile one actor's Death animation into independent layers."""
    layers: list[dict[str, Any]] = []

    def start_of(image_id: int, name: str) -> int:
        if not 0 < image_id < cs.IMAGE_COUNT:
            raise Unsupported(f"image {image_id} out of range")
        script_id = image_iscript[image_id]
        if script_id not in scripts:
            raise Unsupported(f"image {image_id} has no IScript")
        starts = cs.image_animation_starts(payload, scripts[script_id])
        if name not in starts:
            raise Unsupported(f"image {image_id} lacks {name}")
        return starts[name]

    def walk(image_id: int, name: str, at: int, x: int, y: int, under: bool,
             remnant: bool, depth: int) -> None:
        if depth > MAX_DEPTH:
            raise Unsupported("overlay nesting exceeds bound")
        pc = start_of(image_id, name)
        layer = {"image_id": image_id, "x": x, "y": y, "under": under,
                 "remnant": remnant, "steps": [], "start": None,
                 "loop_start": HOLD_LAST, "animation": name}
        layers.append(layer)
        if len(layers) > MAX_LAYERS:
            raise Unsupported("death animation exceeds layer bound")
        elapsed = 0
        frame = None
        seen: dict[tuple[int, int | None], int] = {}
        for _ in range(MAX_INSTRUCTIONS):
            state = (pc, frame)
            if state in seen:
                if seen[state] == len(layer["steps"]):
                    raise Unsupported("instantaneous IScript loop")
                layer["loop_start"] = seen[state]
                break
            seen[state] = len(layer["steps"])
            opcode, args, next_pc, jump = cs.decode_instruction(payload, pc)
            pc = next_pc
            if opcode == 0:
                frame = args[0]
            elif opcode in (5, 6):
                if opcode == 5:
                    wait = args[0]
                else:
                    if args[0] > args[1]:
                        raise Unsupported("waitrand minimum exceeds maximum")
                    wait = (args[0] + args[1]) // 2
                if wait <= 0:
                    raise Unsupported("wait duration must be positive")
                if frame is not None:
                    if layer["start"] is None:
                        layer["start"] = at + elapsed
                    layer["steps"].append([frame, wait])
                elapsed += wait
            elif opcode == 7:
                pc = jump
            elif opcode in IMAGE_OVER or opcode in IMAGE_UNDER:
                ox, oy = (0, 0) if opcode in USES_LO or len(args) < 3 else (args[1], args[2])
                walk(args[0], "Init", at + elapsed, x + to_s8(ox), y + to_s8(oy),
                     opcode in IMAGE_UNDER, remnant, depth + 1)
            elif opcode == 61:
                walk(image_id + 1, "Init", at + elapsed, x + to_s8(args[0]),
                     y + to_s8(args[1]), True, remnant, depth + 1)
            elif opcode in SPRITE_OVER or opcode in SPRITE_UNDER:
                if not 0 <= args[0] < cs.SPRITE_COUNT:
                    raise Unsupported("sprite id out of range")
                ox, oy = (0, 0) if opcode in USES_LO else (args[1], args[2])
                walk(sprite_image[args[0]], "Init", at + elapsed, x + to_s8(ox),
                     y + to_s8(oy), opcode in SPRITE_UNDER, opcode == 17, depth + 1)
            elif opcode == 22:
                break
            elif opcode in SKIP_OPS:
                pass
            else:
                raise Unsupported(f"unsupported IScript opcode {opcode}")
        else:
            raise Unsupported("IScript exceeds instruction bound")
        # Merge equal consecutive frames (rubble repeats long waits).
        merged: list[list[int]] = []
        loop_start = layer["loop_start"]
        for index, (f, d) in enumerate(layer["steps"]):
            if merged and merged[-1][0] == f and index != loop_start:
                merged[-1][1] += d
            else:
                if index == loop_start:
                    loop_start = len(merged)
                merged.append([f, d])
        if len(merged) > MAX_STEPS:
            raise Unsupported("timeline exceeds step bound")
        layer["steps"] = merged
        layer["loop_start"] = loop_start if layer["loop_start"] != HOLD_LAST else HOLD_LAST
        total = sum(d for _f, d in merged)
        if layer["loop_start"] != HOLD_LAST:
            total = REMNANT_CAP_SU  # a looping stream runs until the cap
        if layer["loop_start"] != HOLD_LAST or remnant:
            total = min(total, REMNANT_CAP_SU)
        if layer["start"] is not None:
            layer["end"] = layer["start"] + total

    def to_s8(value: int) -> int:
        return value - 256 if value > 127 else value

    walk(root_image, "Death", 0, 0, 0, True, False, 0)
    result = [layer for layer in layers if layer["steps"] and layer["start"] is not None]
    if not result:
        raise Unsupported("death animation draws no frames")
    for layer in result:
        if not -128 <= layer["x"] <= 127 or not -128 <= layer["y"] <= 127:
            raise Unsupported("layer offset exceeds signed byte")
        if layer["end"] > 0xFFFF:
            raise Unsupported("layer exceeds timeline range")
    return result


def timing(layers: Sequence[dict[str, Any]]) -> dict[str, int]:
    total = max(layer["end"] for layer in layers)
    bursts = [layer["end"] for layer in layers if not layer["remnant"]]
    remnants = [layer["start"] for layer in layers if layer["remnant"]]
    burst = max(bursts) if bursts else min(remnants)
    return {"burst_su": burst, "remnant_start": min(remnants) if remnants else burst,
            "total_su": total}


def write_header(path: Path, sources: dict[int, dict[str, Any]],
                 images: dict[int, dict[str, Any]]) -> None:
    packed_images = packed_under_image_ids(sources)
    word_bytes = word_payload_bytes(sources, images)
    lines = [
        "/* Private generated source death art. Do not commit. */",
        "#ifndef SC64_DEATH_SPRITES_GENERATED_H",
        "#define SC64_DEATH_SPRITES_GENERATED_H",
        "#define SC64_DEATH_SOURCE_BANK_VERSION 2u",
        "#define SC64_DEATH_SOURCE_METADATA_VERSION 1u",
        f"#define SC64_DEATH_SOURCE_PIXEL_BYTES {sum(len(f['pixels']) for im in images.values() for f in im['frames'])}u",
        "#define SC64_DEATH_SOURCE_WORD_METADATA 1u",
        f"#define SC64_DEATH_SOURCE_WORD_BYTES {word_bytes}u",
        "#ifdef SC64_DEATH_SOURCE_ART",
        '#include "sc64_death_source.h"',
        "",
    ]
    for iid, image in images.items():
        for index, f in enumerate(image["frames"]):
            symbol = f"sc64_death_image_{iid}_frame_{index}"
            lines.append(f"static const unsigned char {symbol}_pixels[{len(f['pixels'])}] = {{")
            for offset in range(0, len(f["pixels"]), 16):
                lines.append("    " + ", ".join(f"0x{v:02x}" for v in f["pixels"][offset:offset + 16]) + ",")
            lines.extend(["};", f"static const Sc64SpriteFrame {symbol} = {{ {f['x']}, {f['y']}, "
                          f"{f['w']}, {f['h']}, {f['canvas_w']}, {f['canvas_h']}, {symbol}_pixels }};"])
            if iid in packed_images:
                phases = pack_word_phases(f["w"], f["h"], f["pixels"])
                lines.extend(["#ifdef SC64_UNIT_WORD_SPRITES"])
                phase_symbols = []
                word_symbol = f"sc64_death_image_{iid}_frame_{index}_words"
                for phase, words in enumerate(phases):
                    phase_symbol = f"{word_symbol}_p{phase}"
                    phase_symbols.append(phase_symbol)
                    lines.append(f"static const u32 {phase_symbol}[{len(words)}] = {{")
                    for offset in range(0, len(words), 8):
                        lines.append("    " + ", ".join(
                            f"0x{word:08x}u" for word in words[offset:offset + 8]) + ",")
                    lines.append("};")
                lines.extend([f"static const GbaSpriteWordFrame {word_symbol} = {{",
                              f"    {f['w']}u, {f['h']}u, {{ {', '.join(phase_symbols)} }}",
                              "};", "#endif /* SC64_UNIT_WORD_SPRITES */"])
        lines.append(f"static const Sc64SpriteFrame *const sc64_death_image_{iid}_frames[] = {{")
        lines.extend(f"    &sc64_death_image_{iid}_frame_{i}," for i in range(len(image["frames"])))
        lines.extend(["};", ""])
    programs: dict[tuple, str] = {}
    layer_sets: dict[tuple, str] = {}
    for raw, source in sources.items():
        layer_symbols = []
        for layer in source["layers"]:
            steps = tuple((images[layer["image_id"]]["map"][f], d) for f, d in layer["steps"])
            key = (steps, layer["loop_start"], len(images[layer["image_id"]]["frames"]))
            if key not in programs:
                symbol = f"sc64_death_program_{len(programs)}"
                programs[key] = symbol
                lines.append(f"static const SC64ConstructionAnimStep {symbol}_steps[] = {{")
                lines.extend(f"    {{ {f}u, {d}u }}," for f, d in steps)
                lines.extend(["};", f"static const SC64ConstructionAnimDesc {symbol} = {{",
                              f"    {symbol}_steps, {len(steps)}u, {key[1]}u, {key[2]}u", "};"])
            layer_symbols.append((layer, programs[key]))
        key = tuple((l["image_id"], l["x"], l["y"], l["start"], l["end"], int(l["under"]),
                     int(l["remnant"]), p) for l, p in layer_symbols)
        if key not in layer_sets:
            symbol = f"sc64_death_layers_{len(layer_sets)}"
            layer_sets[key] = symbol
            lines.append(f"static const Sc64DeathSourceLayer {symbol}[] = {{")
            lines.extend(f"    {{ {i}u, {x}, {y}, {s}u, {e}u, {u}u, {r}u, &{p} }},"
                         for i, x, y, s, e, u, r, p in key)
            lines.append("};")
        t = source["timing"]
        lines.append(f"static const Sc64DeathSource sc64_death_raw_{raw} = {{ {layer_sets[key]}, "
                     f"{len(key)}u, {t['burst_su']}u, {t['remnant_start']}u, {t['total_su']}u }};")
    lines.extend(["", "static inline const Sc64SpriteFrame *",
                  "sc64_death_image_frame(unsigned short image_id, unsigned short frame_index)",
                  "{", "    switch (image_id) {"])
    for iid, image in images.items():
        count = len(image["frames"])
        lines.append(f"    case {iid}: return frame_index < {count}u ? "
                     f"sc64_death_image_{iid}_frames[frame_index] : (const Sc64SpriteFrame *)0;")
    lines.extend(["    default: return (const Sc64SpriteFrame *)0;", "    }", "}",
                  "static inline const Sc64DeathSource *", "sc64_death_source(int raw_unit_id)",
                  "{", "    switch (raw_unit_id) {"])
    lines.extend(f"    case {raw}: return &sc64_death_raw_{raw};" for raw in sources)
    lines.extend(["    default: return (const Sc64DeathSource *)0;", "    }", "}",
                  "#ifdef SC64_UNIT_WORD_SPRITES",
                  "static inline const GbaSpriteWordFrame *",
                  "sc64_death_image_word_frame(unsigned short image_id, unsigned short frame_index)",
                  "{", "    switch (image_id) {"])
    for iid in sorted(packed_images):
        count = len(images[iid]["frames"])
        lines.extend([f"    case {iid}: switch (frame_index) {{"])
        lines.extend(f"        case {index}: return &sc64_death_image_{iid}_frame_{index}_words;"
                     for index in range(count))
        lines.extend(["        default: return (const GbaSpriteWordFrame *)0;", "    }"])
    lines.extend(["    default: return (const GbaSpriteWordFrame *)0;", "    }", "}",
                  "#endif /* SC64_UNIT_WORD_SPRITES */",
                  f"#define SC64_DEATH_SOURCE_ACTOR_COUNT {len(sources)}u",
                  "#endif /* SC64_DEATH_SOURCE_ART */",
                  "#endif /* SC64_DEATH_SPRITES_GENERATED_H */", ""])
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("\n".join(lines), encoding="ascii")


def convert_frames(grp: dict[str, Any], used: Sequence[int], drawfn: int, remap_id: int,
                   remap: bytes, ramp: Sequence[int]) -> list[dict[str, Any]]:
    frames = []
    for source_index in used:
        f = grp["frames"][source_index]
        x, y, fw, fh, width = f["x"], f["y"], f["frame_width"], f["frame_height"], f["width"]
        pixels = bytearray()
        for row in range(fh):
            for col in range(fw):
                i = (y + row) * width + x + col
                if not f["alpha"][i]:
                    pixels.append(0)
                elif drawfn == 9:
                    # Faint smoke is translucent in the source: drop the
                    # faintest band and checkerboard the next one.
                    band = intensity_band(f["pixels"][i])
                    if band == 0 or (band == 1 and (row + col + y + x) & 1):
                        pixels.append(0)
                    else:
                        pixels.append(ramp[band])
                else:
                    pixels.append(remap[f["pixels"][i]] or 1)
        frames.append({"x": x, "y": y, "w": fw, "h": fh, "canvas_w": width,
                       "canvas_h": f["height"], "pixels": bytes(pixels)})
    return frames


def generate(rom_path: Path, remap_path: Path, header_path: Path, out_dir: Path,
             upstream_path: Path = cs.UPSTREAM) -> dict[str, Any]:
    base = cs.load_sprites(ROOT / "tools/sc64_sprites.py")
    extractor = base.load_extractor(upstream_path)
    rom_raw = rom_path.read_bytes()
    rom = extractor.load_rom(str(rom_path))
    rom_sha = hashlib.sha1(rom).hexdigest()
    if rom_sha != cs.EXPECTED_ROM_SHA1:
        raise ValueError(f"ROM is not authenticated USA retail image ({rom_sha})")
    archive = extractor.BoltArchive(rom)
    entries = {entry.path: entry for entry in archive.entries()}

    def read(path: str) -> bytes:
        if path not in entries:
            raise ValueError(f"missing BOLT entry {path}")
        return archive.read(entries[path])

    tables = {}
    for path, expected in cs.TABLE_HASHES.items():
        data = read(path)
        if hashlib.sha256(data).hexdigest() != expected:
            raise ValueError(f"authenticated BOLT table hash mismatch: {path}")
        tables[path] = data
    unit_flingy, _construction, flingy_sprite, sprite_image, image_grp, image_iscript = \
        cs.table_images(tables["00C/001"], tables["00C/002"], tables["00C/003"], tables["00C/000"])
    images_dat = tables["00C/000"]
    draw_function = images_dat[cs.IMAGE_COUNT * 8:cs.IMAGE_COUNT * 9]
    remapping = images_dat[cs.IMAGE_COUNT * 9:cs.IMAGE_COUNT * 10]
    payload = tables["012/002"]
    scripts = cs.script_index(payload)
    remap = remap_path.read_bytes()
    if len(remap) != 256:
        raise ValueError(f"palette remap must contain 256 bytes, got {len(remap)}")
    palette_data = read("00B/004")
    ui = ui_palette_indices()

    sources: dict[int, dict[str, Any]] = {}
    rejected = []
    for raw in RAW_IDS:
        flingy = unit_flingy[raw]
        if flingy >= cs.FLINGY_COUNT or flingy_sprite[flingy] >= cs.SPRITE_COUNT:
            rejected.append({"raw_unit_id": raw, "reason": "unit image chain out of range"})
            continue
        image = sprite_image[flingy_sprite[flingy]]
        try:
            layers = compile_death(payload, scripts, image_iscript, sprite_image, image)
        except (Unsupported, ValueError) as exc:
            rejected.append({"raw_unit_id": raw, "image_id": image, "reason": str(exc)})
            continue
        sources[raw] = {"image_id": image, "layers": layers, "timing": timing(layers)}

    used: dict[int, set[int]] = {}
    for source in sources.values():
        for layer in source["layers"]:
            used.setdefault(layer["image_id"], set()).update(f for f, _d in layer["steps"])
    images: dict[int, dict[str, Any]] = {}
    for iid in sorted(used):
        if not image_grp[iid]:
            raise ValueError(f"image {iid} has no GRP")
        grp_data = read(cs.grp_path(image_grp[iid]))
        grp = base.parse_grp(grp_data)
        frames = sorted(used[iid])
        if frames[-1] >= len(grp["frames"]) or len(frames) > 255:
            raise ValueError(f"image {iid}: frame reference outside GRP")
        ramp = ramp_indices(remapping[iid], ui) if draw_function[iid] == 9 else []
        images[iid] = {"map": {f: i for i, f in enumerate(frames)},
                       "frames": convert_frames(grp, frames, draw_function[iid],
                                                remapping[iid], remap, ramp),
                       "grp_path": cs.grp_path(image_grp[iid]),
                       "grp_sha1": hashlib.sha1(grp_data).hexdigest(),
                       "draw_function": draw_function[iid], "remapping": remapping[iid],
                       "source_frames": frames}
    word_bytes = word_payload_bytes(sources, images)
    write_header(header_path, sources, images)
    pixel_bytes = sum(len(f["pixels"]) for image in images.values() for f in image["frames"])
    manifest = {
        "generator": {
            "death_bank_version": 2,
            "metadata_version": 1,
            "word_metadata_version": 1,
            "word_bytes": word_bytes,
            "word_image_count": len(packed_under_image_ids(sources)),
            "extractor_commit": base.UPSTREAM_COMMIT,
            "source_sha256": {name: hashlib.sha256((ROOT / name).read_bytes()).hexdigest()
                              for name in ("tools/sc64_death_sprites.py",
                                           "tools/sc64_construction_sprites.py",
                                           "tools/sc64_sprites.py",
                                           "tools/sc64_unit_facing.py",
                                           "starcraft/sc64_death_source.h",
                                           "common/gba.h",
                                           "starcraft/sc64_construction_animation.h")},
            "header_sha256": hashlib.sha256(header_path.read_bytes()).hexdigest(),
        },
        "source": {"rom_filename": rom_path.name, "rom_sha1": rom_sha,
                   "rom_input_sha1": hashlib.sha1(rom_raw).hexdigest(),
                   "table_sha256": {p: hashlib.sha256(d).hexdigest() for p, d in tables.items()}},
        "selection": {
            "unit_lookup": "units.dat->flingy.dat->sprites.dat->images.dat",
            "animation": "Death of the actor image; Init of spawned images/sprites",
            "word_packing": (f"under layers for raw unit ids below {WORD_PACK_RAW_ID_LIMIT}; "
                             f"remnants for building ids {BUILDING_RAW_FIRST}..{BUILDING_RAW_END - 1}"),
            "iscript_walk": "linear and unconditional goto; liftoffcondjmp follows the grounded path",
            "remnant_cap_source_updates": REMNANT_CAP_SU,
            "remap_draw_function_9": "intensity bands mapped to GBA UI palette ramps; faintest band dropped, next dithered",
        },
        "palette": {"bolt_path": "00B/004", "sha256": hashlib.sha256(palette_data).hexdigest(),
                    "remap_sha256": hashlib.sha256(remap).hexdigest()},
        "actors": [{"raw_unit_id": raw, "image_id": s["image_id"], **s["timing"],
                    "layers": [{k: l[k] for k in ("image_id", "x", "y", "start", "end",
                                                  "under", "remnant", "loop_start")}
                               for l in s["layers"]]} for raw, s in sources.items()],
        "images": [{"image_id": iid, "grp_path": im["grp_path"], "grp_sha1": im["grp_sha1"],
                    "draw_function": im["draw_function"], "remapping": im["remapping"],
                    "source_frames": im["source_frames"]} for iid, im in images.items()],
        "rejected": rejected,
        "pixel_bytes": pixel_bytes,
        "generated_header": str(header_path),
    }
    out_dir.mkdir(parents=True, exist_ok=True)
    (out_dir / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf8")
    return manifest


def main(argv: Sequence[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rom", type=Path, default=ROM_DEFAULT)
    parser.add_argument("--palette-remap", type=Path, default=REMAP_DEFAULT)
    parser.add_argument("--header", type=Path, default=HEADER_DEFAULT)
    parser.add_argument("--out", type=Path, default=OUT_DEFAULT)
    parser.add_argument("--upstream", type=Path, default=cs.UPSTREAM)
    args = parser.parse_args(argv)
    try:
        manifest = generate(args.rom, args.palette_remap, args.header, args.out, args.upstream)
    except (OSError, ValueError, RuntimeError, KeyError) as exc:
        print(f"sc64_death_sprites: {exc}", file=sys.stderr)
        return 1
    print(f"Imported death animations for {len(manifest['actors'])} actors "
          f"({len(manifest['images'])} image banks, {manifest['pixel_bytes']} pixel bytes; "
          f"{len(manifest['rejected'])} actors keep the authored fallback)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
