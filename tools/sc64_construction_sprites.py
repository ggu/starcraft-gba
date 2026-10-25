#!/usr/bin/env python3
"""Import authenticated construction GRPs to a private, palette-remapped C header.

Copyright (C) 2026 blizzard-gba contributors
SPDX-License-Identifier: GPL-3.0-or-later
"""
from __future__ import annotations
import argparse
import hashlib
import importlib.util
import json
import struct
import sys
from pathlib import Path
from typing import Any, Callable, Sequence

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
from tools.sc64_construction_programs import compile_primary_frame_stream  # noqa: E402

UPSTREAM = ROOT / "references/sc64-maps/extract_sc64_maps.py"
ROM_DEFAULT = ROOT / ".local/sc64/rom/StarCraft64-USA.z64"
REMAP_DEFAULT = ROOT / ".local/sc64/backwater/generated/palette-remap.bin"
OUT_DEFAULT = ROOT / ".local/sc64/construction/generated"
HEADER_DEFAULT = OUT_DEFAULT / "sc64_construction_sprites_generated.h"
EXPECTED_ROM_SHA1 = "a36d91b5aac7bb95df0d7de5f9814f5270e25c2b"
TABLE_HASHES = {
    "00C/000": "6b6123e2cc6771d96f914fc9bb92d34da17cc3319dc02e2db0cd10b4f3d2efdc",
    "00C/001": "54356d7585c66cd463346996444fd208336eaacf6335d30eb2688aa73c9cb2df",
    "00C/002": "e0bfdff3b35ff417eee003a7d85fccce4787959dcf18d14995a75a4144ddf75a",
    "00C/003": "3e5ea29bf9937558d0a3a302cd85852c220cd126ec049c0da176c77739bb4a71",
    "012/002": "f8e5f3d0fcca644470c37de65486e44b09eb30141eedccea3445afe7bc0f65bc",
}
UNIT_COUNT = 228
FLINGY_COUNT = 209
SPRITE_COUNT = 517
IMAGE_COUNT = 999
BUILDINGS = {
    "Command Center": ("terran", 106), "Supply Depot": ("terran", 109),
    "Refinery": ("terran", 110), "Barracks": ("terran", 111),
    "Academy": ("terran", 112), "Starport": ("terran", 114),
    "Missile Turret": ("terran", 124), "Bunker": ("terran", 125),
    "Hatchery": ("zerg", 131), "Lair": ("zerg", 132),
    "Hive": ("zerg", 133), "Hydralisk Den": ("zerg", 135),
    "Evolution Chamber": ("zerg", 139), "Spire": ("zerg", 141),
    "Spawning Pool": ("zerg", 142), "Creep Colony": ("zerg", 143),
    "Spore Colony": ("zerg", 144), "Sunken Colony": ("zerg", 146),
    "Extractor": ("zerg", 149), "Nexus": ("protoss", 154),
    "Pylon": ("protoss", 156), "Assimilator": ("protoss", 157),
    "Observatory": ("protoss", 159), "Gateway": ("protoss", 160),
    "Photon Cannon": ("protoss", 162), "Citadel of Adun": ("protoss", 163),
    "Cybernetics Core": ("protoss", 164), "Templar Archives": ("protoss", 165),
    "Forge": ("protoss", 166), "Stargate": ("protoss", 167),
    "Fleet Beacon": ("protoss", 169), "Arbiter Tribunal": ("protoss", 170),
    "Robotics Facility": ("protoss", 155),
    "Robotics Support Bay": ("protoss", 171), "Shield Battery": ("protoss", 172),
}
MORPH_PROGRAMS = {"Greater Spire": ("zerg", 137)}
ANIMATIONS = ("Init", "Death", "GndAttkInit", "AirAttkInit", "Unused1",
              "GndAttkRpt", "AirAttkRpt", "CastSpell", "GndAttkToIdle",
              "AirAttkToIdle", "Unused2", "Walking", "WalkingToIdle",
              "SpecialState1", "SpecialState2", "AlmostBuilt", "Built",
              "Landing", "LiftOff", "IsWorking", "WorkingToIdle", "WarpIn",
              "Unused3", "StarEditInit", "Disable", "Burrow", "UnBurrow", "Enable")
# Ordinal-to-operand formats audited from the pinned source table. This is a
# data description for a bounded reader, not copied upstream implementation.
OP_FORMATS = (
    # 0-9
    "2", "2", "s1", "s1", "s1s1", "1", "11", "j", "211", "211",
    # 10-19
    "2", "2", "", "211", "211", "2s1s1", "211", "211", "2", "211",
    # 20-29
    "211", "21", "e", "1", "2", "v", "22", "", "v", "",
    # 30-39
    "1b", "1", "1", "", "1", "1", "1", "1", "", "",
    # 40-49
    "1", "1", "", "1", "1", "", "", "", "", "1",
    # 50-59
    "", "", "1", "b", "", "2", "1", "b", "2b", "22b",
    # 60-68
    "22b", "11", "", "b", "2", "1", "211", "", "",
)
IMAGE_REFERENCE_OPS = {8, 9, 10, 11, 13, 14, 61}
CONSTRUCTION_ANIMATION_IDS = (0, 13, 14, 15)

def load_sprites(path: Path):
    spec = importlib.util.spec_from_file_location("sc64_sprites", path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load sprite helpers: {path}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module

def le_array(data: bytes, count: int, width: int, label: str) -> tuple[int, ...]:
    needed = count * width
    if len(data) < needed:
        raise ValueError(f"{label}: expected at least {needed} bytes, got {len(data)}")
    code = {1: "B", 2: "H", 4: "I"}[width]
    return struct.unpack_from("<" + code * count, data)


def table_images(units: bytes, flingy: bytes, sprites: bytes, images: bytes) -> tuple[list[int], ...]:
    """Read DAT fields needed for source unit-to-image and construction lookups."""
    unit_flingy = le_array(units, UNIT_COUNT, 1, "units.dat flingy column")
    construction_offset = UNIT_COUNT * 5 + 96 * 2
    construction = le_array(units[construction_offset:], UNIT_COUNT, 4, "units.dat construction_animation column")
    flingy_sprite = le_array(flingy, FLINGY_COUNT, 2, "flingy.dat sprite column")
    sprite_image = le_array(sprites, SPRITE_COUNT, 2, "sprites.dat image column")
    image_grp = le_array(images, IMAGE_COUNT, 4, "images.dat GRP column")
    image_iscript = le_array(images[IMAGE_COUNT * 10:], IMAGE_COUNT, 4, "images.dat IScript column")
    return list(unit_flingy), list(construction), list(flingy_sprite), list(sprite_image), list(image_grp), list(image_iscript)


def grp_path(index: int) -> str:
    if index == 0:
        raise ValueError("image has no GRP filename index")
    flat = index - 1
    directory = 0x00D + flat // 256
    if directory > 0x011:
        raise ValueError(f"GRP filename index outside unit asset bank: {index}")
    return f"{directory:03X}/{flat % 256:03X}"


def script_index(data: bytes) -> dict[int, int]:
    if len(data) < 4:
        raise ValueError("IScript payload is shorter than header")
    pos = struct.unpack_from("<I", data)[0]
    if pos < 4 or pos >= len(data):
        raise ValueError("IScript id table offset out of range")
    result = {}
    while pos + 4 <= len(data):
        script_id, offset = struct.unpack_from("<hH", data, pos)
        pos += 4
        if script_id == -1:
            return result
        if script_id < 0 or script_id in result or offset + 8 > len(data):
            raise ValueError("invalid or duplicate IScript id row")
        result[script_id] = offset
    raise ValueError("IScript id table lacks terminator")


def decode_instruction(data: bytes, pc: int) -> tuple[int, list[Any], int, int | None]:
    if pc >= len(data):
        raise ValueError("IScript instruction starts outside payload")
    opcode = data[pc]
    pc += 1
    if opcode >= len(OP_FORMATS):
        raise ValueError(f"unknown IScript opcode {opcode}")
    fmt = OP_FORMATS[opcode]
    args: list[Any] = []
    jump = None
    cursor = 0
    while cursor < len(fmt):
        signed = fmt[cursor] == "s"
        if signed:
            cursor += 1
        kind = fmt[cursor]
        cursor += 1
        if kind in ("1", "2"):
            width = int(kind)
            if pc + width > len(data):
                raise ValueError("truncated IScript operand")
            value = int.from_bytes(data[pc:pc + width], "little", signed=signed)
            pc += width
            args.append(value)
        elif kind in ("j", "b"):
            if pc + 2 > len(data):
                raise ValueError("truncated IScript branch operand")
            jump = struct.unpack_from("<H", data, pc)[0]
            pc += 2
            args.append(jump)
        elif kind == "e":
            pass
        elif kind == "v":
            if pc >= len(data):
                raise ValueError("truncated IScript vector length")
            count = data[pc]
            pc += 1
            if pc + count * 2 > len(data):
                raise ValueError("truncated IScript vector data")
            args.append(list(struct.unpack_from("<" + "H" * count, data, pc)) if count else [])
            pc += count * 2
        else:
            raise ValueError(f"unsupported IScript operand descriptor {kind!r}")
    return opcode, args, pc, jump


def animation_instructions(payload: bytes, start: int, limit: int = 96) -> list[tuple[int, list[Any]]]:
    """Walk one animation path, following goto only and bounding loops and work."""
    pc = start
    visited: set[int] = set()
    result = []
    for _ in range(limit):
        if pc in visited:
            break
        visited.add(pc)
        opcode, args, next_pc, jump = decode_instruction(payload, pc)
        result.append((opcode, args))
        if opcode in (22, 54):  # end, return
            break
        if opcode == 7 and jump is not None:  # unconditional goto
            pc = jump
        else:
            pc = next_pc
    return result


def image_animation_starts(payload: bytes, script_offset: int) -> dict[str, int]:
    if script_offset < 0 or script_offset + 8 > len(payload):
        raise ValueError("truncated SCPE record header")
    if payload[script_offset:script_offset + 4] != b"SCPE":
        raise ValueError(f"invalid SCPE record at {script_offset}")
    highest = struct.unpack_from("<I", payload, script_offset + 4)[0]
    if highest > 27:
        raise ValueError(f"animation index too large: {highest}")
    count = (highest + 2) & ~1
    if script_offset + 8 + count * 2 > len(payload):
        raise ValueError("truncated SCPE animation offsets")
    offsets = struct.unpack_from("<" + "H" * count, payload, script_offset + 8)
    result = {}
    for animation_id, offset in enumerate(offsets):
        if offset:
            if offset >= len(payload):
                raise ValueError("SCPE animation target outside IScript")
            result[ANIMATIONS[animation_id]] = offset
    return result


def select_images(
    read_entry: Callable[[str], bytes], tables: tuple[list[int], ...]
) -> dict[int, dict[str, Any]]:
    """Resolve selected building construction/normal art and direct stage overlays."""
    unit_flingy, construction, flingy_sprite, sprite_image, image_grp, image_iscript = tables
    script_payload = read_entry("012/002")
    scripts = script_index(script_payload)
    selected: dict[int, dict[str, Any]] = {}

    def add(image_id: int, race: str, building: str, role: str) -> None:
        if not image_id:
            return
        if image_id >= IMAGE_COUNT:
            raise ValueError(f"{building}: image id {image_id} out of range")
        grp_index = image_grp[image_id]
        if not grp_index:
            raise ValueError(f"{building}: image {image_id} has no GRP")
        item = selected.setdefault(image_id, {
            "image_id": image_id, "race": race, "roles": [], "buildings": [],
            "grp_index": grp_index, "grp_path": grp_path(grp_index),
        })
        if role not in item["roles"]:
            item["roles"].append(role)
        if building not in item["buildings"]:
            item["buildings"].append(building)

    def direct_refs(image_id: int, selected_animations: set[str]) -> list[int]:
        iscript_id = image_iscript[image_id]
        if iscript_id not in scripts:
            return []
        starts = image_animation_starts(script_payload, scripts[iscript_id])
        refs = []
        for animation in selected_animations & starts.keys():
            for opcode, args in animation_instructions(script_payload, starts[animation]):
                if opcode == 61:  # imgulnextid: implicit image_id + 1; args are x/y
                    refs.append(image_id + 1)
                elif opcode in IMAGE_REFERENCE_OPS and args and isinstance(args[0], int):
                    refs.append(args[0])
        return refs

    for building, (race, unit_id) in BUILDINGS.items():
        if unit_id >= UNIT_COUNT:
            raise ValueError(f"{building}: unit id out of range")
        add(construction[unit_id], race, building, "construction_image")
        flingy_id = unit_flingy[unit_id]
        if flingy_id >= FLINGY_COUNT:
            raise ValueError(f"{building}: flingy id out of range")
        sprite_id = flingy_sprite[flingy_id]
        if sprite_id >= SPRITE_COUNT:
            raise ValueError(f"{building}: sprite id out of range")
        normal_image = sprite_image[sprite_id]
        add(normal_image, race, building,
            "warp_in_structure" if race == "protoss" else "normal_structure")
        if construction[unit_id]:
            for image_id in direct_refs(construction[unit_id], {"Init", "SpecialState1", "SpecialState2", "AlmostBuilt"}):
                add(image_id, race, building, "construction_overlay")
        if race == "protoss":
            for image_id in direct_refs(normal_image, {"WarpIn"}):
                add(image_id, race, building, "warp_in_overlay")
    return dict(sorted(selected.items()))


def source_stage_inputs(tables: tuple[list[int], ...], payload: bytes) -> list[dict[str, Any]]:
    """Return source-backed image/script starts for the supported T/Z stages."""
    unit_flingy, construction, flingy_sprite, sprite_image, _image_grp, image_iscript = tables
    scripts = script_index(payload)
    morph_targets = {132, 133, 137, 144, 146}

    def unit_image(unit_id: int) -> int:
        flingy_id = unit_flingy[unit_id]
        if flingy_id >= FLINGY_COUNT:
            raise ValueError(f"unit {unit_id}: flingy id out of range")
        sprite_id = flingy_sprite[flingy_id]
        if sprite_id >= SPRITE_COUNT:
            raise ValueError(f"unit {unit_id}: sprite id out of range")
        return sprite_image[sprite_id]

    def animation_start(image_id: int, name: str) -> int | None:
        if not image_id or image_id >= IMAGE_COUNT:
            return None
        script_id = image_iscript[image_id]
        if script_id not in scripts:
            return None
        return image_animation_starts(payload, scripts[script_id]).get(name)

    stage_inputs = []
    program_buildings = {**BUILDINGS, **MORPH_PROGRAMS}
    for building, (race, unit_id) in program_buildings.items():
        if race == "protoss" or not construction[unit_id]:
            continue
        construction_image = construction[unit_id]
        normal_image = unit_image(unit_id) if race == "terran" else 0
        if race == "terran":
            stage_names = ((0, construction_image, "Init"),
                           (1, construction_image, "SpecialState1"),
                           (2, construction_image, "SpecialState2"),
                           (3, normal_image, "AlmostBuilt"))
        else:
            initial_name = "SpecialState1" if unit_id in morph_targets else "Init"
            stage_names = ((0, construction_image, initial_name),
                           (1, construction_image, "SpecialState1"),
                           (2, construction_image, "SpecialState2"))
        for stage, image_id, animation in stage_names:
            start = animation_start(image_id, animation)
            if start is not None:
                stage_inputs.append({"raw_unit_id": unit_id, "building": building,
                                     "race": race, "stage": stage,
                                     "image_id": image_id, "animation": animation,
                                     "start": start})
    return stage_inputs

def write_header(path: Path, images: Sequence[dict[str, Any]], remap: bytes,
                 stage_programs: Sequence[dict[str, Any]] = ()) -> None:
    if len(remap) != 256:
        raise ValueError(f"palette remap must be 256 bytes, got {len(remap)}")
    lines = [
        "/* Private generated source art. Do not commit. */",
        "#ifndef SC64_CONSTRUCTION_SPRITES_GENERATED_H",
        "#define SC64_CONSTRUCTION_SPRITES_GENERATED_H",
        "#ifdef SC64_CONSTRUCTION_SOURCE_ART",
        '#include "sc64_construction_source.h"',
        "",
    ]
    for image in images:
        frames = image["frames"]
        iid = image["image_id"]
        for fi, f in enumerate(frames):
            x, y = f["x"], f["y"]
            fw, fh = f["frame_width"], f["frame_height"]
            pixels = bytearray()
            for row in range(fh):
                for col in range(fw):
                    source_index = (y + row) * f["width"] + x + col
                    pixels.append(remap[f["pixels"][source_index]] if f["alpha"][source_index] else 0)
            symbol = f"sc64_construction_image_{iid}_frame_{fi}"
            lines.append(f"static const unsigned char {symbol}_pixels[{len(pixels)}] = {{")
            for offset in range(0, len(pixels), 16):
                values = ", ".join(f"0x{value:02x}" for value in pixels[offset:offset + 16])
                lines.append(f"    {values},")
            lines.extend([
                "};",
                f"static const Sc64SpriteFrame {symbol} = {{ {x}, {y}, {fw}, {fh}, {f['width']}, {f['height']}, {symbol}_pixels }};",
            ])
        lines.append(f"static const Sc64SpriteFrame *const sc64_construction_image_{iid}_frames[] = {{")
        lines.extend(f"    &sc64_construction_image_{iid}_frame_{index}," for index in range(len(frames)))
        lines.extend(["};", f"#define SC64_CONSTRUCTION_IMAGE_{iid}_FRAME_COUNT {len(frames)}u", ""])
    for program in stage_programs:
        raw, stage = program["raw_unit_id"], program["stage"]
        symbol = f"sc64_construction_raw_{raw}_stage_{stage}"
        lines.append(f"static const SC64ConstructionAnimStep {symbol}_steps[] = {{")
        for step in program["program"]["steps"]:
            lines.append(f"    {{ {step['frame']}u, {step['duration_source_updates']}u }},")
        desc = program["program"]
        lines.extend(["};", f"static const SC64ConstructionAnimDesc {symbol}_animation = {{",
                      f"    {symbol}_steps, {len(desc['steps'])}u, {desc['loop_start']}u, {desc['frame_count']}u",
                      "};", f"static const SC64ConstructionSourceStage {symbol} = {{",
                      f"    {program['image_id']}u, &{symbol}_animation", "};", ""])
    lines.extend(["static inline unsigned short sc64_construction_image_frame_count(unsigned short image_id) {", "    switch (image_id) {"])
    for image in images:
        iid = image["image_id"]
        lines.append(f"    case {iid}: return SC64_CONSTRUCTION_IMAGE_{iid}_FRAME_COUNT;")
    lines.extend(["    default: return 0;", "    }", "}",
                  "static inline const Sc64SpriteFrame *sc64_construction_image_frame(unsigned short image_id, unsigned short frame_index) {",
                  "    switch (image_id) {"])
    for im in images:
        iid, count = im["image_id"], len(im["frames"])
        lines.extend([f"    case {iid}:", f"        return frame_index < {count}u ? sc64_construction_image_{iid}_frames[frame_index] : (const Sc64SpriteFrame *)0;"])
    lines.extend(["    default: return (const Sc64SpriteFrame *)0;", "    }", "}",
                  "static inline const SC64ConstructionSourceStage *sc64_construction_stage(int raw_unit_id, unsigned stage) {",
                  "    switch (raw_unit_id) {"])
    raw_ids = sorted({program["raw_unit_id"] for program in stage_programs})
    for raw in raw_ids:
        lines.extend([f"    case {raw}:", "        switch (stage) {"])
        for program in stage_programs:
            if program["raw_unit_id"] == raw:
                lines.append(f"        case {program['stage']}: return &sc64_construction_raw_{raw}_stage_{program['stage']};")
        lines.extend(["        default: return (const SC64ConstructionSourceStage *)0;", "        }"])
    lines.extend(["    default: return (const SC64ConstructionSourceStage *)0;", "    }", "}",
                  "#endif /* SC64_CONSTRUCTION_SOURCE_ART */", "#endif /* SC64_CONSTRUCTION_SPRITES_GENERATED_H */", ""])
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("\n".join(lines), encoding="ascii")

def generate(rom_path: Path, remap_path: Path, header_path: Path,
             out_dir: Path, upstream_path: Path = UPSTREAM) -> dict[str, Any]:
    base = load_sprites(ROOT / "tools/sc64_sprites.py")
    extractor = base.load_extractor(upstream_path)
    rom_raw = rom_path.read_bytes()
    rom = extractor.load_rom(str(rom_path))
    rom_sha = hashlib.sha1(rom).hexdigest()
    if rom_sha != EXPECTED_ROM_SHA1:
        raise ValueError(f"ROM is not authenticated USA retail image ({rom_sha})")
    archive = extractor.BoltArchive(rom)
    entries = {entry.path: entry for entry in archive.entries()}

    def read(path):
        if path not in entries:
            raise ValueError(f"missing BOLT entry {path}")
        return archive.read(entries[path])

    tables = {}
    for path, expected_hash in TABLE_HASHES.items():
        data = read(path)
        actual_hash = hashlib.sha256(data).hexdigest()
        if actual_hash != expected_hash:
            raise ValueError(f"authenticated BOLT table hash mismatch: {path}")
        tables[path] = data
    table_data = table_images(tables["00C/001"], tables["00C/002"],
                              tables["00C/003"], tables["00C/000"])
    script_data = tables["012/002"]
    # The closure receives already-authenticated script bytes, avoiding a
    # second archive lookup while preserving its small callback interface.
    def read_selected(path: str) -> bytes:
        return script_data if path == "012/002" else read(path)
    selected = select_images(read_selected, table_data)
    stage_inputs = source_stage_inputs(table_data, script_data)
    remap = remap_path.read_bytes()
    if len(remap) != 256:
        raise ValueError(f"palette remap must contain 256 bytes, got {len(remap)}")
    palette = read("00B/004")
    if len(palette) != 512:
        raise ValueError(f"Badlands palette must contain 512 bytes, got {len(palette)}")
    output = []
    for image_id, item in selected.items():
        grp_data = read(item["grp_path"])
        grp = base.parse_grp(grp_data)
        if grp["width"] > 32767 or grp["height"] > 32767:
            raise ValueError(f"image {image_id} canvas exceeds sprite ABI")
        output.append({**item, "grp_sha1": hashlib.sha1(grp_data).hexdigest(),
                       "frame_count": len(grp["frames"]), "frames": grp["frames"]})
    frame_counts = {image["image_id"]: image["frame_count"] for image in output}
    stage_programs = []
    rejected_stages = []
    for stage_input in stage_inputs:
        try:
            compiled = compile_primary_frame_stream(
                script_data, stage_input["start"], frame_counts[stage_input["image_id"]], 0)
        except (ValueError, KeyError) as exc:
            rejected_stages.append({**stage_input, "reason": str(exc)})
            continue
        stage_programs.append({**stage_input, "program": compiled})
    write_header(header_path, output, remap, stage_programs)
    manifest = {
        "generator": {
            "extractor_commit": base.UPSTREAM_COMMIT,
            "source_sha256": {
                name: hashlib.sha256((ROOT / name).read_bytes()).hexdigest()
                for name in ("tools/sc64_construction_sprites.py",
                             "tools/sc64_construction_programs.py",
                             "tools/sc64_sprites.py",
                             "starcraft/sc64_construction_animation.h",
                             "starcraft/sc64_construction_source.h")},
            "header_sha256": hashlib.sha256(header_path.read_bytes()).hexdigest(),
        },
        "source": {"rom_filename": rom_path.name, "rom_sha1": rom_sha,
                   "rom_input_sha1": hashlib.sha1(rom_raw).hexdigest(),
                   "table_sha256": {path: hashlib.sha256(data).hexdigest()
                                    for path, data in tables.items()}},
        "selection": {
            "unit_lookup": "units.dat->flingy.dat->sprites.dat->images.dat",
            "construction_images": "units.dat construction_animation column",
            "construction_animations": ["Init", "SpecialState1", "SpecialState2", "AlmostBuilt"],
            "protoss_warp_overlay_scan": "direct image references in completed structure WarpIn only",
            "iscript_walk": "linear and unconditional goto; conditional branches and calls are not expanded",
            "runtime_stage_races": ["terran", "zerg"],
            "zerg_initial_special_state1_raw_types": [132, 133, 137, 144, 146],
            "protoss_runtime_stages": "not emitted; source banks retained for follow-up",
        },
        "palette": {"bolt_path": "00B/004", "sha256": hashlib.sha256(palette).hexdigest(),
                    "remap_sha256": hashlib.sha256(remap).hexdigest()},
        "images": [{"image_id": image["image_id"], "race": image["race"],
                    "roles": image["roles"], "buildings": image["buildings"],
                    "grp_path": image["grp_path"], "grp_sha1": image["grp_sha1"],
                    "frame_count": image["frame_count"],
                    "canvas": [image["frames"][0]["width"], image["frames"][0]["height"]]}
                   for image in output],
        "stages": [{"raw_unit_id": stage["raw_unit_id"], "building": stage["building"],
                    "race": stage["race"], "stage": stage["stage"],
                    "animation": stage["animation"], "image_id": stage["image_id"],
                    "frame_count": stage["program"]["frame_count"],
                    "step_count": len(stage["program"]["steps"]),
                    "loop_start": stage["program"]["loop_start"],
                    "omitted_layers": stage["program"]["omitted_layers"],
                    "signals": stage["program"]["signals"]} for stage in stage_programs],
        "rejected_stages": [{key: value for key, value in item.items() if key != "start"}
                            for item in rejected_stages],
        "generated_header": str(header_path),
    }
    out_dir.mkdir(parents=True, exist_ok=True)
    (out_dir / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf8")
    return manifest

def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rom", type=Path, default=ROM_DEFAULT)
    parser.add_argument("--palette-remap", type=Path, default=REMAP_DEFAULT)
    parser.add_argument("--header", type=Path, default=HEADER_DEFAULT)
    parser.add_argument("--out", type=Path, default=OUT_DEFAULT)
    parser.add_argument("--upstream", type=Path, default=UPSTREAM)
    args = parser.parse_args(argv)
    try:
        manifest = generate(args.rom, args.palette_remap, args.header, args.out, args.upstream)
    except (OSError, ValueError, RuntimeError, KeyError) as exc:
        print(f"sc64_construction_sprites: {exc}", file=sys.stderr)
        return 1
    frame_count = sum(image["frame_count"] for image in manifest["images"])
    print(f"Imported {len(manifest['images'])} image banks ({frame_count} frames)")
    return 0
if __name__=="__main__": raise SystemExit(main())
