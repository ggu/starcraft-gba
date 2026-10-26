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
import re
import struct
import sys
from pathlib import Path
from typing import Any, Callable, Sequence

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
from tools.sc64_construction_programs import compile_source_layers  # noqa: E402

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
CONSTRUCTION_MODIFIER_TABLE_HASHES = {
    "00B/004": "32ae82250ec8c471bf583099ef370ca4d34ed5693e8be7f6ed3c23d020355403",
    "00B/008": "f9a05ba0c117529befc69610eeff8af4c04f710ecec8518b45364a060d506e8e",
}
UNIT_COUNT = 228
FLINGY_COUNT = 209
SPRITE_COUNT = 517
IMAGE_COUNT = 999
# Pinned OpenBW bwenums.h at 4b046d5f65302b10cb0a745f0fecd37ec85b20a8:
# ImageTypes::IMAGEID_Warp_Anchor. units.dat has no construction image for
# Protoss, but order_ProtossBuild installs this shared image before
# IncompleteWarping begins.
PROTOSS_WARP_ANCHOR_IMAGE = 211
BUILDINGS = {
    "Command Center": ("terran", 106), "Supply Depot": ("terran", 109),
    "Refinery": ("terran", 110), "Barracks": ("terran", 111),
    "Academy": ("terran", 112), "Factory": ("terran", 113),
    "Starport": ("terran", 114), "Engineering Bay": ("terran", 122),
    "Comsat Station": ("terran", 107), "Nuclear Silo": ("terran", 108),
    "Control Tower": ("terran", 115), "Science Facility": ("terran", 116),
    "Covert Ops": ("terran", 117), "Physics Lab": ("terran", 118),
    "Machine Shop": ("terran", 120), "Armory": ("terran", 123),
    "Missile Turret": ("terran", 124), "Bunker": ("terran", 125),
    "Hatchery": ("zerg", 131), "Lair": ("zerg", 132),
    "Hive": ("zerg", 133), "Nydus Canal": ("zerg", 134),
    "Defiler Mound": ("zerg", 136), "Queens Nest": ("zerg", 138),
    "Ultralisk Cavern": ("zerg", 140), "Hydralisk Den": ("zerg", 135),
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
GAS_OVERLAY_ACTIVE_BASE = 430
GAS_OVERLAY_DEPLETED_BASE = 435

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
        if image_id < 0 or image_id >= IMAGE_COUNT:
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
                elif opcode == 56:  # active/depleted variants selected by resource count
                    selector = args[0] - 256 if args[0] > 127 else args[0]
                    refs.extend((GAS_OVERLAY_ACTIVE_BASE + selector,
                                 GAS_OVERLAY_DEPLETED_BASE + selector))
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
            add(PROTOSS_WARP_ANCHOR_IMAGE, race, building, "warp_anchor")
    # Imported image layers can themselves spawn Init overlays. Follow that
    # bounded image graph so every referenced GRP can be rendered at runtime.
    pending = list(selected)
    visited: set[int] = set()
    while pending:
        image_id = pending.pop()
        if image_id in visited:
            continue
        visited.add(image_id)
        item = selected[image_id]
        for ref in direct_refs(image_id, {"Init"}):
            if ref not in selected:
                add(ref, item["race"], item["buildings"][0], "nested_overlay")
                pending.append(ref)
        if len(selected) > 256:
            raise ValueError("construction overlay image graph exceeds 256 images")
    return dict(sorted(selected.items()))


def source_stage_inputs(tables: tuple[list[int], ...], payload: bytes) -> list[dict[str, Any]]:
    """Return source-backed T/Z construction and Protoss WarpAnchor starts."""
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
        if race == "protoss":
            image_id = PROTOSS_WARP_ANCHOR_IMAGE
            start = animation_start(image_id, "Init")
            if start is not None:
                stage_inputs.append({"raw_unit_id": unit_id, "building": building,
                                     "race": race, "stage": 0,
                                     "image_id": image_id, "animation": "Init",
                                     "start": start})
            continue
        if not construction[unit_id]:
            if race != "terran":
                continue
            normal_image = unit_image(unit_id)
            start = animation_start(normal_image, "AlmostBuilt")
            if start is not None:
                for stage in range(4):
                    stage_inputs.append({"raw_unit_id": unit_id, "building": building,
                                         "race": race, "stage": stage,
                                         "image_id": normal_image, "animation": "AlmostBuilt",
                                         "start": start})
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


def audit_construction_flipstate(payload: bytes,
                                 stage_inputs: Sequence[dict[str, Any]],
                                 animation_starts: dict[tuple[int, str], int],
                                 sprite_image: Sequence[int]) -> dict[str, int]:
    """Visit both fallthrough and branch paths, rejecting setflipstate.

    followmaingraphic inherits its parent frame/flip in the runtime. This audit
    proves that selected construction timelines do not also mutate the primary
    flip state independently.
    """
    seen_animations: set[tuple[int, str, int]] = set()
    instructions_seen = 0

    def scan(image_id: int, animation: str, start: int, depth: int) -> None:
        nonlocal instructions_seen
        key = (image_id, animation, start)
        if key in seen_animations:
            return
        if depth > 3:
            raise ValueError("construction animation overlay depth exceeds audit limit")
        seen_animations.add(key)
        pending = [start]
        visited: set[int] = set()
        while pending:
            pc = pending.pop()
            for _ in range(512):
                if pc in visited:
                    break
                visited.add(pc)
                opcode, args, next_pc, jump = decode_instruction(payload, pc)
                instructions_seen += 1
                if instructions_seen > 65536:
                    raise ValueError("construction script audit exceeds instruction bound")
                if opcode == 23:
                    raise ValueError(
                        f"setflipstate (opcode 23) in construction image {image_id} "
                        f"{animation} at {pc}")
                child_ids: list[int] = []
                if opcode in (8, 9, 10, 11, 13, 14):
                    child_ids.append(args[0])
                elif opcode == 61:
                    child_ids.append(image_id + 1)
                elif opcode == 56:
                    selector = args[0] - 256 if args[0] > 127 else args[0]
                    child_ids.extend((GAS_OVERLAY_ACTIVE_BASE + selector,
                                      GAS_OVERLAY_DEPLETED_BASE + selector))
                elif opcode in (15, 16, 17, 19, 20, 21):
                    if not 0 <= args[0] < len(sprite_image):
                        raise ValueError(f"construction animation sprite {args[0]} out of range")
                    child_ids.append(sprite_image[args[0]])
                for child in child_ids:
                    child_start = animation_starts.get((child, "Init"))
                    if child_start is not None:
                        scan(child, "Init", child_start, depth + 1)
                if opcode == 22:
                    break
                if opcode == 7:
                    pc = jump
                    continue
                if jump is not None:
                    pending.append(jump)
                pc = next_pc

    for source in stage_inputs:
        scan(source["image_id"], source["animation"], source["start"], 0)
    return {"reachable_animation_count": len(seen_animations),
            "instruction_visits": instructions_seen, "setflipstate_count": 0}

def _rgb555(value: int) -> tuple[int, int, int]:
    return value & 31, (value >> 5) & 31, (value >> 10) & 31

def _rgb_distance(a: tuple[int, int, int], b: tuple[int, int, int]) -> int:
    return sum((a[i] - b[i]) ** 2 for i in range(3))

def _palette_initializer(body: str, pattern: str, label: str) -> list[tuple[str, ...]]:
    """Every initializer token must be understood; never skip a new override."""
    body = re.sub(r"/\*.*?\*/|//[^\n]*", "", body, flags=re.S)
    matches = list(re.finditer(pattern, body))
    if re.sub(pattern, "", body).strip(" \r\n\t,"):
        raise ValueError(f"unsupported {label} initializer")
    return [match.groups() for match in matches]

def _actual_gba_palette(gba_rgb555: Sequence[int]) -> tuple[list[tuple[int, int, int]], dict[str, str]]:
    """Read palette installation inputs from runtime C, failing closed on drift."""
    gba_c = (ROOT / "common/gba.c").read_text(encoding="ascii")
    gba_h = (ROOT / "common/gba.h").read_text(encoding="ascii")
    display_h = (ROOT / "starcraft/sc64_display.h").read_text(encoding="ascii")
    hashes = {"common/gba.c": hashlib.sha256(gba_c.encode()).hexdigest(),
              "common/gba.h": hashlib.sha256(gba_h.encode()).hexdigest(),
              "starcraft/sc64_display.h": hashlib.sha256(display_h.encode()).hexdigest()}
    color_match = re.search(r"static const u16 colors\[32\]\s*=\s*\{(.*?)\};", gba_c, re.S)
    enum_match = re.search(r"enum\s*\{\s*(C_BLACK\s*=.*?)};", gba_h, re.S)
    lcd_match = re.search(r"static const u8 lcd\[32\]\s*=\s*\{(.*?)\};", display_h, re.S)
    ui_match = re.search(r"static const struct \{u8 index,r,g,b;} ui\[\]\s*=\s*\{(.*?)\};", display_h, re.S)
    if not all((color_match, enum_match, lcd_match, ui_match)):
        raise ValueError("GBA display palette source shape changed; update modifier projection parser")
    ui_colors = [tuple(map(int, row)) for row in _palette_initializer(
        color_match.group(1), r"RGB\(\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*\)", "UI colors")]
    lcd = [int(row[0]) for row in _palette_initializer(
        lcd_match.group(1), r"\b(\d+)\b", "LCD curve")]
    if len(ui_colors) != 32 or len(lcd) != 32 or len(gba_rgb555) != 256:
        raise ValueError("unexpected GBA palette initializer sizes")
    if any(not 0 <= c <= 31 for row in ui_colors for c in row) or any(not 0 <= c <= 31 for c in lcd):
        raise ValueError("palette channels must fit RGB555")
    enum_values: dict[str, int] = {}
    next_value = 0
    for item in enum_match.group(1).split(","):
        item = item.strip()
        if not item:
            continue
        match = re.fullmatch(r"(C_[A-Z0-9_]+)(?:\s*=\s*(\d+))?", item)
        if not match:
            raise ValueError(f"unsupported GBA color enum item: {item}")
        name, explicit = match.groups()
        if explicit is not None:
            next_value = int(explicit)
        enum_values[name] = next_value
        next_value += 1
    palette = list(ui_colors)
    for value in gba_rgb555[32:]:
        channels = _rgb555(int(value))
        palette.append(tuple(lcd[channel] for channel in channels))
    overrides = []
    for name, r, g, b in _palette_initializer(ui_match.group(1),
            r"\{\s*(C_[A-Z0-9_]+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*\}", "UI overrides"):
        if name not in enum_values:
            raise ValueError(f"unknown display palette override: {name}")
        index = enum_values[name]
        if not 0 <= index < 256 or any(not 0 <= int(c) <= 31 for c in (r, g, b)):
            raise ValueError(f"invalid display palette override: {name}")
        palette[index] = (int(r), int(g), int(b))
        overrides.append(index)
    if len(palette) != 256 or len(set(overrides)) != len(overrides):
        raise ValueError("invalid parsed actual display palette")
    return palette, hashes

def warp_anchor_modifier_lut(source_palette_data: bytes, bfire: bytes,
                             gba_rgb555: Sequence[int]) -> bytes:
    """Project modifier-9 source strengths through the live 8-bit GBA palette.

    Source output RGB is quantized to the nearest actual post-display GBA color
    among all 256 entries. A current framebuffer destination is represented by
    its nearest source-palette color among all 256 entries. Restricting that
    inverse projection to the importer's source-index remap preimages is wrong:
    display transforms and UI overrides can make the actual destination color
    differ from the original source color that mapped to that index.
    """
    if len(source_palette_data) != 512 or len(bfire) != 40 * 256 or len(gba_rgb555) != 256:
        raise ValueError("invalid authenticated WarpAnchor palette inputs")
    source_palette = [((value >> 11) & 31, (value >> 6) & 31, (value >> 1) & 31)
                      for value in struct.unpack(">256H", source_palette_data)]
    actual_palette, _ = _actual_gba_palette(gba_rgb555)
    representatives = []
    for gba_index, destination in enumerate(actual_palette):
        representatives.append(min(range(256),
            key=lambda source_index: (_rgb_distance(source_palette[source_index], destination), source_index)))
    nearest_output = [min(range(256),
        key=lambda gba_index: (_rgb_distance(source_palette[source_index], actual_palette[gba_index]), gba_index))
        for source_index in range(256)]
    projected = bytearray(40 * 256)
    for strength_row in range(40):
        for destination in range(256):
            raw_output = bfire[strength_row * 256 + representatives[destination]]
            projected[strength_row * 256 + destination] = nearest_output[raw_output]
    return bytes(projected)

def write_header(path: Path, images: Sequence[dict[str, Any]], remap: bytes,
                 stage_programs: Sequence[dict[str, Any]] = (),
                 modifier_lut: bytes = b"") -> None:
    if len(remap) != 256:
        raise ValueError(f"palette remap must be 256 bytes, got {len(remap)}")
    anchor = next((image for image in images if image["image_id"] == 211), None)
    if anchor is not None and len(anchor["frames"]) != 14:
        raise ValueError("WarpAnchor image 211 must provide all 14 source frames")
    if (anchor is not None or modifier_lut) and len(modifier_lut) != 40 * 256:
        raise ValueError("WarpAnchor modifier LUT must be exactly 40x256 bytes")
    lines = [
        "/* Private generated source art. Do not commit. */",
        "#ifndef SC64_CONSTRUCTION_SPRITES_GENERATED_H",
        "#define SC64_CONSTRUCTION_SPRITES_GENERATED_H",
        "#ifdef SC64_CONSTRUCTION_SOURCE_ART",
        '#include "sc64_construction_source.h"',
        "#define SC64_CONSTRUCTION_COLOR_MODIFIERS_VERSION 1u",
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
            if iid == 211:
                strength_symbol = f"sc64_construction_image_211_strengths_{fi}"
                strengths = bytearray()
                for row in range(fh):
                    for col in range(fw):
                        source_index = (y + row) * f["width"] + x + col
                        value = f["pixels"][source_index]
                        if f["alpha"][source_index] and not 1 <= value <= 40:
                            raise ValueError(f"WarpAnchor frame {fi} has invalid opaque strength {value}")
                        strengths.append(value if f["alpha"][source_index] else 0)
                lines.append(f"static const unsigned char {strength_symbol}[{len(strengths)}] = {{")
                for offset in range(0, len(strengths), 16):
                    values = ", ".join(f"0x{value:02x}" for value in strengths[offset:offset + 16])
                    lines.append(f"    {values},")
                lines.extend(["};", f"static const SC64ConstructionModifierFrame sc64_construction_image_211_modifier_frame_{fi} = {{ {x}, {y}, {fw}, {fh}, {f['width']}, {f['height']}, {strength_symbol} }};"])
        lines.append(f"static const Sc64SpriteFrame *const sc64_construction_image_{iid}_frames[] = {{")
        lines.extend(f"    &sc64_construction_image_{iid}_frame_{index}," for index in range(len(frames)))
        lines.extend(["};", f"#define SC64_CONSTRUCTION_IMAGE_{iid}_FRAME_COUNT {len(frames)}u", ""])
    if anchor is not None:
        lines.append("static const unsigned char sc64_construction_image_211_modifier_lut[10240] = {")
        for offset in range(0, len(modifier_lut), 16):
            values = ", ".join(f"0x{value:02x}" for value in modifier_lut[offset:offset + 16])
            lines.append(f"    {values},")
        lines.extend(["};", ""])
        lines.append("static const SC64ConstructionModifierFrame *const sc64_construction_image_211_modifier_frames[14] = {")
        lines.extend(f"    &sc64_construction_image_211_modifier_frame_{fi}," for fi in range(14))
        lines.extend(["};", "",
            "static inline unsigned sc64_construction_image_modifier(unsigned short image_id) {",
            "    return image_id == 211u ? 9u : 0u;", "}",
            "static inline unsigned sc64_construction_image_color_shift(unsigned short image_id) {",
            "    return image_id == 211u ? 3u : 0u;", "}",
            "static inline const SC64ConstructionModifierFrame *sc64_construction_modifier_frame(unsigned short image_id, unsigned short frame_index) {",
            "    return image_id == 211u && frame_index < 14u ? sc64_construction_image_211_modifier_frames[frame_index] : (const SC64ConstructionModifierFrame *)0;",
            "}",
            "static inline const unsigned char *sc64_construction_modifier_lookup(unsigned short image_id, unsigned *rows) {",
            "    if (rows) *rows = image_id == 211u ? 40u : 0u;",
            "    return image_id == 211u ? sc64_construction_image_211_modifier_lut : (const unsigned char *)0;",
            "}", ""])
    else:
        # Partial synthetic banks still exercise ordinary frame/stage writing.
        # They advertise the ABI but contain no Warp Anchor payload.
        lines.extend([
            "static inline unsigned sc64_construction_image_modifier(unsigned short id) { (void)id; return 0; }",
            "static inline unsigned sc64_construction_image_color_shift(unsigned short id) { (void)id; return 0; }",
            "static inline const SC64ConstructionModifierFrame *sc64_construction_modifier_frame(unsigned short id, unsigned short frame) { (void)id; (void)frame; return 0; }",
            "static inline const unsigned char *sc64_construction_modifier_lookup(unsigned short id, unsigned *rows) { (void)id; if (rows) *rows=0; return 0; }", ""])
    for program in stage_programs:
        raw, stage = program["raw_unit_id"], program["stage"]
        symbol = f"sc64_construction_raw_{raw}_stage_{stage}"
        for layer_index, layer in enumerate(program["program"]["layers"]):
            layer_symbol = f"{symbol}_layer_{layer_index}"
            desc = layer["program"]
            lines.append(f"static const SC64ConstructionAnimStep {layer_symbol}_steps[] = {{")
            for step in desc["steps"]:
                lines.append(f"    {{ {step['frame']}u, {step['duration_source_updates']}u }},")
            lines.extend(["};", f"static const SC64ConstructionAnimDesc {layer_symbol}_animation = {{",
                          f"    {layer_symbol}_steps, {len(desc['steps'])}u, {desc['loop_start']}u, {desc['frame_count']}u", "};"])
            if layer["gas_overlay_requests"]:
                lines.append(f"static const SC64ConstructionGasOverlayRequest {layer_symbol}_gas_requests[] = {{")
                lines.extend(f"    {{ {request['selector']}, {request['start_source_updates']}u }},"
                             for request in layer["gas_overlay_requests"])
                lines.append("};")
        lines.append(f"static const SC64ConstructionSourceLayer {symbol}_layers[] = {{")
        for layer_index, layer in enumerate(program["program"]["layers"]):
            gas_ptr = (f"{symbol}_layer_{layer_index}_gas_requests"
                       if layer["gas_overlay_requests"] else "(const SC64ConstructionGasOverlayRequest *)0")
            lines.append(f"    {{ {layer['image_id']}u, {layer['x_offset']}, {layer['y_offset']}, "
                         f"{layer['start_source_updates']}u, {int(layer['under'])}u, "
                         f"{int(layer['follows_main_graphic'])}u, "
                         f"{len(layer['gas_overlay_requests'])}u, {gas_ptr}, "
                         f"&{symbol}_layer_{layer_index}_animation }},")
        lines.extend(["};", f"static const SC64ConstructionSourceStage {symbol} = {{",
                      f"    {program['image_id']}u, &{symbol}_layer_0_animation, "
                      f"{symbol}_layers, {len(program['program']['layers'])}u", "};", ""])
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
    modifier_tables = {"00B/004": palette}
    for path, expected_hash in CONSTRUCTION_MODIFIER_TABLE_HASHES.items():
        data = read(path)
        actual_hash = hashlib.sha256(data).hexdigest()
        if actual_hash != expected_hash:
            raise ValueError(f"authenticated modifier table hash mismatch: {path}")
        modifier_tables[path] = data
    image_draw_function = tables["00C/000"][8 * IMAGE_COUNT:9 * IMAGE_COUNT]
    image_color_shift = tables["00C/000"][9 * IMAGE_COUNT:10 * IMAGE_COUNT]
    if (image_draw_function[211], image_color_shift[211]) != (9, 3):
        raise ValueError("WarpAnchor image 211 no longer matches modifier 9/color_shift 3")
    palette_json_path = remap_path.parent / "palette.json"
    palette_json = json.loads(palette_json_path.read_text(encoding="utf8"))
    modifier_lut = warp_anchor_modifier_lut(
        palette, modifier_tables["00B/008"], palette_json["gba_rgb555"])
    _, palette_source_hashes = _actual_gba_palette(palette_json["gba_rgb555"])
    output = []
    for image_id, item in selected.items():
        grp_data = read(item["grp_path"])
        grp = base.parse_grp(grp_data)
        if grp["width"] > 32767 or grp["height"] > 32767:
            raise ValueError(f"image {image_id} canvas exceeds sprite ABI")
        output.append({**item, "grp_sha1": hashlib.sha1(grp_data).hexdigest(),
                       "frame_count": len(grp["frames"]), "frames": grp["frames"]})
    frame_counts = {image["image_id"]: image["frame_count"] for image in output}
    if frame_counts.get(PROTOSS_WARP_ANCHOR_IMAGE) != 14:
        raise ValueError("Authenticated construction bank requires all 14 WarpAnchor frames")
    image_iscript = table_data[5]
    script_offsets = script_index(script_data)
    animation_starts: dict[tuple[int, str], int] = {}
    for image_id, script_id in enumerate(image_iscript):
        if script_id in script_offsets:
            for name, start in image_animation_starts(script_data, script_offsets[script_id]).items():
                animation_starts[(image_id, name)] = start
    flip_audit = audit_construction_flipstate(
        script_data, stage_inputs, animation_starts, table_data[3])
    stage_programs = []
    for stage_input in stage_inputs:
        try:
            compiled_layers = compile_source_layers(
                script_data, {stage_input["image_id"]: stage_input["start"]},
                stage_input["image_id"], decode_instruction, table_data[3],
                animation_starts, frame_counts)
        except (ValueError, KeyError) as exc:
            raise ValueError(f"{stage_input['building']} {stage_input['animation']} "
                             f"(image {stage_input['image_id']}): {exc}") from exc
        stage_programs.append({**stage_input, "program": {"layers": compiled_layers}})
    write_header(header_path, output, remap, stage_programs, modifier_lut)
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
                                    for path, data in tables.items()},
                   "modifier_table_sha256": {path: hashlib.sha256(data).hexdigest()
                                             for path, data in modifier_tables.items()}},
        "color_modifier": {
            "version": 1, "scope": "WarpAnchor image 211 only",
            "image_id": 211, "modifier": 9, "color_shift": 3,
            "bfire_table": "00B/008", "lookup_rows": 40, "lookup_columns": 256,
            "projection": "nearest actual post-display GBA RGB555 output; each framebuffer destination represented by nearest source-palette RGB across all source entries",
            "projection_source_sha256": {
                **palette_source_hashes,
                "palette.json": hashlib.sha256(palette_json_path.read_bytes()).hexdigest(),
            },
            "lookup_sha256": hashlib.sha256(modifier_lut).hexdigest(),
            "limitations": ["GBA destination projection is quantized; BOLT 00B/008 role is associated by ordering and dimensions, without executable resource-index xref proof."]},
        "selection": {
            "unit_lookup": "units.dat->flingy.dat->sprites.dat->images.dat",
            "construction_images": "units.dat construction_animation column",
            "construction_animations": ["Init", "SpecialState1", "SpecialState2", "AlmostBuilt"],
            "protoss_warp_overlay_scan": "pinned WarpAnchor image 211 Init and its nested overlays",
            "iscript_walk": "linear and unconditional goto; randwait uses midpoint; conditional branches and calls are not expanded",
            "iscript_layer_walk": "bounded imgol/imgul and sprite overlay Init recursion; followmaingraphic is emitted as a parent frame/flip link; move is skipped for static images; creategasoverlays (opcode 56) stores timed selectors with active/depleted image variants",
            "gas_overlay_requests": sum(len(layer["gas_overlay_requests"])
                                         for stage in stage_programs
                                         for layer in stage["program"]["layers"]),
            "gas_overlay_image_bases": {
                "resource_nonzero": GAS_OVERLAY_ACTIVE_BASE,
                "resource_zero": GAS_OVERLAY_DEPLETED_BASE,
            },
            "construction_setflipstate_audit": flip_audit,
            "runtime_stage_races": ["terran", "zerg", "protoss"],
            "zerg_initial_special_state1_raw_types": [132, 133, 137, 144, 146],
            "protoss_runtime_stages": "shared WarpAnchor Init animation at stage 0 until build timer ends",
            "terran_missing_construction_image": "normal AlmostBuilt animation used for all four progress slots",
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
                    "frame_count": stage["program"]["layers"][0]["program"]["frame_count"],
                    "step_count": len(stage["program"]["layers"][0]["program"]["steps"]),
                    "loop_start": stage["program"]["layers"][0]["program"]["loop_start"],
                    "layers": [{key: layer[key] for key in
                                ("image_id", "x_offset", "y_offset", "start_source_updates", "under",
                                 "follows_main_graphic", "gas_overlay_requests")}
                               for layer in stage["program"]["layers"]]} for stage in stage_programs],
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
