#!/usr/bin/env python3
"""Import authenticated source-authored unit attack body timelines.

The bank includes the full bounded attack-Init script, from tick zero through
its recovery/boundary, as well as the backwards-compatible post-release view.
It does not model weapon projectiles or damage. Generated pixels remain
private under ignored .local/ directories.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import struct
import sys
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
from tools import sc64_construction_sprites as construction  # noqa: E402
from tools import sc64_race_sprites as races  # noqa: E402
from tools import sc64_sprites as sprites  # noqa: E402

OUT = ROOT / ".local/sc64/attack-poses/generated"
HEADER = OUT / "sc64_unit_attack_source_generated.h"
ROM = ROOT / ".local/sc64/rom/StarCraft64-USA.z64"
REMAP = ROOT / ".local/sc64/backwater/generated/palette-remap.bin"
EXPECTED_ROM_SHA1 = sprites.EXPECTED_USA_ROM_SHA1
MAX_SCRIPT_STEPS = 512
MAX_POST_RELEASE_TICKS = 64
MAX_FULL_ATTACK_TICKS = 255
MAX_LAYER_DEPTH = 4
MAX_LAYER_IMAGES = 64

# Authenticated armed unit identities from the source weapon-profile audit.
# Turreted hulls 3/5/30 remain on static facing art; source weapon effects own
# their attack presentation. Raw 50 is retained so its unsupported body attack
# status is explicit in the manifest rather than silently omitted.
ATTACK_UNITS = {
    "marine": 0, "ghost": 1, "vulture": 2, "scv": 7, "wraith": 8,
    "battlecruiser": 12, "kerrigan": 16, "raynor_vulture": 19,
    "raynor_marine": 20, "norad_ii": 29, "firebat": 32,
    "zergling": 37, "hydralisk": 38, "ultralisk": 39, "drone": 41,
    "mutalisk": 43, "guardian": 44, "scourge": 47,
    "infested_terran": 50, "infested_kerrigan": 51,
    "hunter_killer": 53, "probe": 64, "zealot": 65, "dragoon": 66,
    "archon": 68, "scout": 70, "arbiter": 71, "interceptor": 73,
    "dark_templar": 74, "zeratul": 75, "fenix": 77,
    "fenix_dragoon": 78, "tassadar": 79, "reaver": 83,
}
RELEASE_OPCODES = {28: "attackmelee", 37: "attackwith", 38: "attack"}


def image_tables(images_dat: bytes):
    if len(images_dat) < sprites.IMAGE_COUNT * 14:
        raise ValueError("images.dat truncated before image IScript column")
    directional = sprites._le_array(images_dat[sprites.IMAGE_COUNT * 4:],
        sprites.IMAGE_COUNT, 1, "images.dat directional flags")
    if any(flag not in (0, 1) for flag in directional):
        raise ValueError("images.dat directional flags must be 0 or 1")
    iscript_offset = sprites.IMAGE_COUNT * 10
    image_iscript = struct.unpack_from("<" + "I" * sprites.IMAGE_COUNT,
                                       images_dat, iscript_offset)
    image_grp = sprites._le_array(images_dat, sprites.IMAGE_COUNT, 4,
                                  "images.dat GRP indices")
    return tuple(directional), tuple(image_iscript), tuple(image_grp)


def walk_animation(payload: bytes, start: int, stop_starts: set[int],
                   initial_frame: int = 0) -> dict[str, Any]:
    """Walk one source animation and preserve every timed frame/release event.

    A source `wait N` advances N source updates. Same-tick operations retain
    their script order; a caller samples the final frame at a given tick.
    """
    pc, tick, current = start, 0, initial_frame
    frames: list[dict[str, int]] = [{"tick": 0, "frame": current}]
    releases: list[dict[str, int | str]] = []
    visited: set[int] = set()
    end_tick = 0
    for _ in range(MAX_SCRIPT_STEPS):
        if pc in visited:
            end_tick = tick
            break
        visited.add(pc)
        opcode, args, next_pc, jump = construction.decode_instruction(payload, pc)
        if opcode == 0:
            current = args[0]
            frames.append({"tick": tick, "frame": current})
        elif opcode in RELEASE_OPCODES:
            releases.append({"tick": tick, "opcode": opcode,
                             "kind": RELEASE_OPCODES[opcode]})
            frames.append({"tick": tick, "frame": current})
        elif opcode == 5:
            tick += args[0]
            if tick > MAX_FULL_ATTACK_TICKS:
                end_tick = MAX_FULL_ATTACK_TICKS
                break
        elif opcode in (22, 53, 54):
            end_tick = tick
            break
        elif opcode == 7 and jump is not None:
            if jump in stop_starts:
                end_tick = tick
                break
            pc = jump
            continue
        pc = next_pc
        end_tick = tick
    return {"frames": frames, "releases": releases,
            "end_tick": min(end_tick, MAX_FULL_ATTACK_TICKS)}


def frame_at(events: list[dict[str, int]], tick: int, fallback: int) -> int:
    chosen = fallback
    for event in events:
        if event["tick"] > tick:
            break
        chosen = event["frame"]
    return chosen


def resolve_frame_index(frame_command: int, heading: int, directional: bool,
                        frame_count: int) -> int:
    if not 0 <= heading < 32:
        raise ValueError("heading outside 0..31")
    if not 0 <= frame_command < frame_count:
        raise ValueError(f"script frame {frame_command} outside GRP with {frame_count} frames")
    if not directional:
        return frame_command
    if frame_command % 17:
        raise ValueError(f"directional frame {frame_command} is not a 17-frame pose base")
    direction = heading if heading <= 16 else 32 - heading
    index = frame_command + direction
    if index >= frame_count:
        raise ValueError(f"pose base {frame_command} heading {heading} exceeds GRP")
    return index


def orient_source_frame(frame: dict[str, Any], mirror: bool) -> dict[str, Any]:
    """Mirror a complete source canvas while retaining exact crop metadata."""
    if not mirror:
        return frame
    width, height = frame["width"], frame["height"]
    pixels, alpha = frame["pixels"], frame["alpha"]
    mirrored_pixels = bytearray(width * height)
    mirrored_alpha = bytearray(width * height)
    for y in range(height):
        src = y * width
        dst = src
        mirrored_pixels[dst:dst + width] = pixels[src:src + width][::-1]
        mirrored_alpha[dst:dst + width] = alpha[src:src + width][::-1]
    result = dict(frame)
    result["pixels"] = bytes(mirrored_pixels)
    result["alpha"] = bytes(mirrored_alpha)
    result["x"] = width - frame["x"] - frame["frame_width"]
    return result


def map_frame(frame: dict[str, Any], remap: bytes) -> dict[str, Any]:
    if len(remap) != 256:
        raise ValueError("sprite remap must have 256 entries")
    x, y, fw, fh = (frame[k] for k in ("x", "y", "frame_width", "frame_height"))
    width, height = frame["width"], frame["height"]
    if not fw or not fh or x + fw > width or y + fh > height:
        raise ValueError("source crop is empty or outside its canvas")
    out = bytearray(fw * fh)
    for row in range(fh):
        for col in range(fw):
            si = (y + row) * width + x + col
            di = row * fw + col
            out[di] = remap[frame["pixels"][si]] if frame["alpha"][si] else 0
    return {"offset_x": x, "offset_y": y, "w": fw, "h": fh,
            "canvas_w": width, "canvas_h": height, "pixels": bytes(out)}


def sample_events(events: list[dict[str, int]], tick: int,
                  initial_frame: int) -> int:
    return frame_at(events, tick, initial_frame)


class SourceBank:
    """Bounded ROM-backed image/script cache used only by the importer."""
    def __init__(self, archive: Any, entries: dict[str, Any], remap: bytes,
                 image_directional: tuple[int, ...], image_iscript: tuple[int, ...],
                 image_grp: tuple[int, ...], scripts: dict[int, int], payload: bytes):
        self.archive, self.entries, self.remap = archive, entries, remap
        self.directional, self.image_iscript, self.image_grp = image_directional, image_iscript, image_grp
        self.scripts, self.payload = scripts, payload
        self.grps: dict[int, dict[str, Any]] = {}
        self.layer_specs: dict[int, list[tuple[int, int, int, bool]]] = {}
        self.anim_starts: dict[int, dict[str, int]] = {}
        self.init_frames: dict[int, int] = {}

    def grp(self, image_id: int) -> dict[str, Any]:
        if not 0 <= image_id < sprites.IMAGE_COUNT or not self.image_grp[image_id]:
            raise ValueError(f"image {image_id} has no source GRP")
        if image_id not in self.grps:
            flat = self.image_grp[image_id] - 1
            path = f"{0xD + flat // 256:03X}/{flat % 256:03X}"
            entry = self.entries.get(path)
            if entry is None:
                raise ValueError(f"source image GRP {path} missing")
            self.grps[image_id] = sprites.parse_grp(self.archive.read(entry))
        return self.grps[image_id]

    def starts(self, image_id: int) -> dict[str, int]:
        if image_id not in self.anim_starts:
            script_id = self.image_iscript[image_id]
            if script_id not in self.scripts:
                self.anim_starts[image_id] = {}
            else:
                self.anim_starts[image_id] = construction.image_animation_starts(
                    self.payload, self.scripts[script_id])
        return self.anim_starts[image_id]

    def init_frame(self, image_id: int) -> int:
        if image_id not in self.init_frames:
            start = self.starts(image_id).get("Init")
            frame = 0
            if start is not None:
                for opcode, args in construction.animation_instructions(self.payload, start):
                    if opcode == 0:
                        frame = args[0]
                        break
            self.init_frames[image_id] = frame
        return self.init_frames[image_id]

    def layers(self, image_id: int) -> list[tuple[int, int, int, bool]]:
        if image_id in self.layer_specs:
            return self.layer_specs[image_id]
        result = []
        start = self.starts(image_id).get("Init")
        if start is not None:
            for opcode, args in construction.animation_instructions(self.payload, start):
                if opcode in (8, 9):
                    result.append((args[0], args[1], args[2], opcode == 9))
                elif opcode == 61:
                    result.append((image_id + 1, args[0], args[1], True))
        self.layer_specs[image_id] = result
        return result

    def collect_images(self, root: int) -> list[int]:
        result, visited = [], set()
        def visit(image_id: int, depth: int):
            if image_id in visited:
                return
            if depth > MAX_LAYER_DEPTH or len(visited) >= MAX_LAYER_IMAGES:
                raise ValueError("unit Init layer graph exceeds bounded import limits")
            visited.add(image_id)
            self.grp(image_id)
            result.append(image_id)
            for child, _x, _y, _under in self.layers(image_id):
                visit(child, depth + 1)
        visit(root, 0)
        return result

    def oriented_frame(self, image_id: int, frame_command: int, heading: int) -> dict[str, Any]:
        group = self.grp(image_id)
        directional = bool(self.directional[image_id])
        index = resolve_frame_index(frame_command, heading, directional,
                                   len(group["frames"]))
        frame = group["frames"][index]
        return orient_source_frame(frame, directional and heading > 16)

    def compose(self, image_id: int, frame_overrides: dict[int, int],
                heading: int, stack: tuple[int, ...] = ()) -> dict[str, Any]:
        if image_id in stack:
            raise ValueError("cyclic source Init image layer graph")
        command = frame_overrides.get(image_id, self.init_frame(image_id))
        main = self.oriented_frame(image_id, command, heading)
        layers = []
        for child_id, dx, dy, underlay in self.layers(image_id):
            child = self.compose(child_id, frame_overrides, heading,
                                 stack + (image_id,))
            layers.append((child, dx, dy, underlay))
        return races.compose_init_layers(main, layers) if layers else main


def image_mode_events(bank: SourceBank, image_id: int, air: int) -> dict[str, Any] | None:
    name = "AirAttkInit" if air else "GndAttkInit"
    start = bank.starts(image_id).get(name)
    if start is None:
        return None
    stop_starts = {pc for anim, pc in bank.starts(image_id).items() if anim != name}
    return walk_animation(bank.payload, start, stop_starts, bank.init_frame(image_id))


def generate(rom_path: Path = ROM, remap_path: Path = REMAP,
             out_dir: Path = OUT, header_path: Path = HEADER) -> dict[str, Any]:
    extractor = sprites.load_extractor(sprites.UPSTREAM)
    normalized = extractor.load_rom(str(rom_path))
    digest = hashlib.sha1(normalized).hexdigest()
    if digest != EXPECTED_ROM_SHA1:
        raise ValueError(f"expected verified USA ROM {EXPECTED_ROM_SHA1}, got {digest}")
    remap = remap_path.read_bytes()
    if len(remap) != 256:
        raise ValueError("sprite remap must have 256 entries")
    archive = extractor.BoltArchive(normalized)
    entries = {entry.path: entry for entry in archive.entries()}
    def read(path: str) -> bytes:
        entry = entries.get(path)
        if entry is None:
            raise ValueError(f"ROM BOLT entry missing: {path}")
        return archive.read(entry)
    units = sprites._le_array(read("00C/001"), sprites.UNIT_COUNT, 1, "units.dat")
    flingy = sprites._le_array(read("00C/002"), sprites.FLINGY_COUNT, 2, "flingy.dat")
    sprite_image = sprites._le_array(read("00C/003"), sprites.SPRITE_COUNT, 2, "sprites.dat")
    images_dat = read("00C/000")
    directional, image_iscript, image_grp = image_tables(images_dat)
    payload = read("012/002")
    scripts = construction.script_index(payload)
    bank = SourceBank(archive, entries, remap, directional, image_iscript,
                      image_grp, scripts, payload)
    resolved = {}
    unsupported = []
    for name, raw in ATTACK_UNITS.items():
        try:
            if raw >= sprites.UNIT_COUNT:
                raise ValueError(f"raw id {raw} outside units.dat")
            fid = units[raw]
            if fid >= sprites.FLINGY_COUNT:
                raise ValueError(f"invalid flingy id {fid}")
            sid = flingy[fid]
            if sid >= sprites.SPRITE_COUNT:
                raise ValueError(f"invalid sprite id {sid}")
            image_id = sprite_image[sid]
            bank.grp(image_id)
            components = bank.collect_images(image_id)
            flat = image_grp[image_id] - 1
            grp_path = f"{0xD + flat // 256:03X}/{flat % 256:03X}"
            resolved[raw] = {"raw": raw, "name": name, "image": image_id,
                              "directional": bool(directional[image_id]),
                              "grp_sha1": hashlib.sha1(archive.read(entries[grp_path])).hexdigest(),
                              "components": components}
        except (ValueError, IndexError, KeyError, struct.error) as error:
            unsupported.append({"raw_unit": raw, "name": name,
                "status": "unsupported", "reason": str(error), "components": [],
                "modes": [{"target_air": air,
                           "animation": "AirAttkInit" if air else "GndAttkInit",
                           "status": "unsupported_actor_image"} for air in (0, 1)]})

    actors_manifest = list(unsupported)
    compiled: dict[tuple[int, int], dict[str, Any]] = {}
    for raw, actor in sorted(resolved.items()):
        mode_records = []
        component_details = []
        for image_id in actor["components"]:
            component_details.append({"image_id": image_id,
                "directional": bool(directional[image_id]),
                "grp_frame_count": len(bank.grp(image_id)["frames"]),
                "init_frame": bank.init_frame(image_id),
                "layers": [{"image_id": child, "x": dx, "y": dy,
                            "underlay": under} for child, dx, dy, under in bank.layers(image_id)]})
        for air in (0, 1):
            root_anim = "AirAttkInit" if air else "GndAttkInit"
            component_scripts = {}
            release_candidates = []
            for image_id in actor["components"]:
                events = image_mode_events(bank, image_id, air)
                if events is not None:
                    component_scripts[image_id] = events
                    for release in events["releases"]:
                        release_candidates.append((release["tick"], image_id, release))
            if not component_scripts:
                mode_records.append({"target_air": air, "animation": root_anim,
                                     "status": "no_attack_animation"})
                continue
            if release_candidates:
                release_tick, release_image, release_event = min(release_candidates,
                    key=lambda v: (v[0], v[1]))
                release_kind = release_event["kind"]
            else:
                mode_records.append({"target_air": air, "animation": root_anim,
                    "status": "no_release_event", "release_candidates": []})
                continue
            last_tick = min(MAX_POST_RELEASE_TICKS, max(
                max((event["tick"] for event in script["frames"]), default=0)
                for script in component_scripts.values()))
            full_end_tick = min(MAX_FULL_ATTACK_TICKS, max(
                int(script.get("end_tick", 0))
                for script in component_scripts.values()))
            full_relevant_ticks = {0}
            for script in component_scripts.values():
                full_relevant_ticks.update(
                    int(event["tick"]) for event in script["frames"]
                    if int(event["tick"]) <= full_end_tick)
            full_steps = []
            for source_tick in sorted(full_relevant_ticks):
                pose_table = {}
                for image_id, script in component_scripts.items():
                    pose_table[image_id] = sample_events(script["frames"], source_tick,
                                                         bank.init_frame(image_id))
                heading_frames = []
                for heading in range(32):
                    composite = bank.compose(actor["image"], pose_table, heading)
                    heading_frames.append(map_frame(composite, remap))
                full_steps.append({"tick": source_tick, "frames": heading_frames})
            relevant_ticks = {0}
            for script in component_scripts.values():
                relevant_ticks.update(max(0, event["tick"] - release_tick)
                                      for event in script["frames"]
                                      if event["tick"] >= release_tick)
            relevant_ticks = {tick for tick in relevant_ticks if tick <= last_tick - release_tick}
            if not relevant_ticks:
                mode_records.append({"target_air": air, "animation": root_anim,
                                     "status": "no_post_release_frames"})
                continue
            steps = []
            for elapsed in sorted(relevant_ticks):
                absolute_tick = release_tick + elapsed
                pose_table = {}
                for image_id, script in component_scripts.items():
                    pose_table[image_id] = sample_events(script["frames"], absolute_tick,
                                                         bank.init_frame(image_id))
                heading_frames = []
                for heading in range(32):
                    composite = bank.compose(actor["image"], pose_table, heading)
                    heading_frames.append(map_frame(composite, remap))
                steps.append({"tick": elapsed, "frames": heading_frames})
            record = {"target_air": air, "animation": root_anim, "status": "ok",
                "release_tick": release_tick, "release_source_image": release_image,
                "release_kind": release_kind, "last_post_release_tick": steps[-1]["tick"],
                "full_duration_source_ticks": max(1, full_end_tick),
                "steps": [{"tick": s["tick"]} for s in steps]}
            mode_records.append(record)
            compiled[(raw, air)] = {"release_tick": release_tick,
                                    "release_kind": release_kind, "steps": steps,
                                    "full_steps": full_steps,
                                    "full_duration_source_ticks": max(1, full_end_tick)}
        actors_manifest.append({"raw_unit": raw, "name": actor["name"],
            "root_image_id": actor["image"], "directional": actor["directional"],
            "grp_sha1": actor["grp_sha1"], "components": component_details,
            "modes": mode_records})
    actors_manifest.sort(key=lambda actor: actor["raw_unit"])
    manifest = {"format_version": 1, "rom_sha1": digest,
        "source_reference": "authenticated StarCraft 64 USA ROM; OpenBW 4b046d5f65302b10cb0a745f0fecd37ec85b20a8 IScript opcode semantics",
        "scope": "full source composite body timeline from attack Init tick zero through bounded recovery; compatibility post-release sampler retained; projectile/damage omitted",
        "actors": actors_manifest}
    write_header(header_path, compiled)
    header_hash = hashlib.sha256(header_path.read_bytes()).hexdigest()
    manifest["header_sha256"] = header_hash
    out_dir.mkdir(parents=True, exist_ok=True)
    (out_dir / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    return manifest


def write_header(path: Path, compiled: dict[tuple[int, int], dict[str, Any]]) -> None:
    lines = [
        "/* Generated from the verified USA ROM; private local asset. */",
        "#ifndef SC64_UNIT_ATTACK_SOURCE_GENERATED_H",
        "#define SC64_UNIT_ATTACK_SOURCE_GENERATED_H",
        "#define SC64_UNIT_ATTACK_POSE_BANK_VERSION 1",
        "#define SC64_UNIT_ATTACK_POSE_METADATA 1",
        "#ifndef SC64_SPRITES_GENERATED_H",
        "#ifndef SC64_RACE_SPRITES_GENERATED_H",
        "typedef struct { short offset_x, offset_y; unsigned short w, h, canvas_w, canvas_h; const unsigned char *pixels; } Sc64SpriteFrame;",
        "#endif", "#endif",
        "typedef struct { unsigned short tick; const Sc64SpriteFrame *heading[32]; } Sc64UnitAttackPoseStep;",
    ]
    pixels_seen: dict[tuple[int, int, bytes], str] = {}
    frames_seen: dict[tuple[Any, ...], str] = {}
    payload_bytes = 0
    def add_frame(frame: dict[str, Any]) -> str:
        nonlocal payload_bytes
        pixels = frame["pixels"]
        pkey = (frame["w"], frame["h"], pixels)
        psym = pixels_seen.get(pkey)
        if psym is None:
            psym = f"sc64_unit_attack_pixels_{len(pixels_seen)}"
            pixels_seen[pkey] = psym
            payload_bytes += len(pixels)
            lines.append(f"static const unsigned char {psym}[{len(pixels)}] = {{")
            for pos in range(0, len(pixels), 16):
                lines.append("    " + ",".join(f"0x{x:02x}" for x in pixels[pos:pos + 16]) + ",")
            lines.append("};")
        fkey = (frame["offset_x"], frame["offset_y"], frame["w"], frame["h"],
                frame["canvas_w"], frame["canvas_h"], psym)
        fsym = frames_seen.get(fkey)
        if fsym is None:
            fsym = f"sc64_unit_attack_frame_{len(frames_seen)}"
            frames_seen[fkey] = fsym
            lines.append(f"static const Sc64SpriteFrame {fsym} = "
                f"{{{frame['offset_x']},{frame['offset_y']},{frame['w']},{frame['h']},"
                f"{frame['canvas_w']},{frame['canvas_h']},{psym}}};")
        return fsym
    rows_by_raw: dict[int, dict[int, tuple[int, int, str, int, int, int, str, int]]] = {}
    for (raw, air), entry in sorted(compiled.items()):
        step_symbol = f"sc64_unit_attack_raw{raw}_{'air' if air else 'ground'}_steps"
        step_frames = []
        for step in entry["steps"]:
            headings = [add_frame(frame) for frame in step["frames"]]
            step_frames.append((step["tick"], headings))
        lines.append(f"static const Sc64UnitAttackPoseStep {step_symbol}[{len(step_frames)}] = {{")
        for tick, headings in step_frames:
            lines.append(f"    {{{tick}u,{{{','.join('&' + name for name in headings)}}}}},")
        lines.append("};")
        full_step_symbol = f"sc64_unit_attack_raw{raw}_{'air' if air else 'ground'}_full_steps"
        full_step_frames = []
        source_steps = entry.get("full_steps")
        if source_steps is None:
            source_steps = [{"tick": entry["release_tick"] + step["tick"],
                             "frames": step["frames"]} for step in entry["steps"]]
        for step in source_steps:
            headings = [add_frame(frame) for frame in step["frames"]]
            full_step_frames.append((step["tick"], headings))
        lines.append(f"static const Sc64UnitAttackPoseStep {full_step_symbol}[{len(full_step_frames)}] = {{")
        for tick, headings in full_step_frames:
            lines.append(f"    {{{tick}u,{{{','.join('&' + name for name in headings)}}}}},")
        lines.append("};")
        fallback_full_span = entry["release_tick"] + step_frames[-1][0] + 1
        full_duration = int(entry.get("full_duration_source_ticks", fallback_full_span))
        row = (entry["release_tick"], step_frames[-1][0], step_symbol, len(step_frames),
               full_duration, full_step_frames[-1][0], full_step_symbol,
               len(full_step_frames))
        rows_by_raw.setdefault(raw, {})[air] = row
    lines.extend([
        "static inline unsigned char sc64_unit_attack_source_release_tick(unsigned short raw_type, int target_air) {",
        "    switch (raw_type) {",
    ])
    for raw, modes in sorted(rows_by_raw.items()):
        lines.append(f"    case {raw}u:")
        for air, (release_tick, _last, _symbol, _n, _full_duration,
                  _full_last, _full_symbol, _full_n) in sorted(modes.items()):
            lines.append(f"        if (!!target_air == {air}) return {release_tick}u;")
        lines.append("        return 0xffu;")
    lines.extend(["    default: return 0xffu;", "    }", "}",
        "static inline unsigned char sc64_unit_attack_source_duration(unsigned short raw_type, int target_air) {",
        "    switch (raw_type) {",
    ])
    for raw, modes in sorted(rows_by_raw.items()):
        lines.append(f"    case {raw}u:")
        for air, (_release, last, _symbol, _n, _full_duration,
                  _full_last, _full_symbol, _full_n) in sorted(modes.items()):
            lines.append(f"        if (!!target_air == {air}) return {last + 1}u;")
        lines.append("        return 0u;")
    lines.extend(["    default: return 0u;", "    }", "}",
        "static inline const Sc64SpriteFrame *sc64_unit_attack_source_frame(unsigned short raw_type, int target_air, unsigned char heading32, unsigned int post_release_tick) {",
        "    unsigned char dir = heading32 & 31u;", "    switch (raw_type) {"])
    for raw, modes in sorted(rows_by_raw.items()):
        lines.append(f"    case {raw}u: {{")
        for air, (_release, last, symbol, count, _full_duration,
                  _full_last, _full_symbol, _full_n) in sorted(modes.items()):
            lines.extend([f"        if (!!target_air == {air}) {{",
                f"            if (post_release_tick > {last}u) return 0;",
                f"            const Sc64UnitAttackPoseStep *steps = {symbol};",
                "            unsigned int selected = 0u;",
                f"            for (unsigned int i=1; i<{count}u; ++i) {{",
                "                if (steps[i].tick > post_release_tick) break;",
                "                selected=i;", "            }",
                "            return steps[selected].heading[dir];", "        }"])
        lines.extend(["        return 0;", "    }"])
    lines.extend(["    default: return 0;", "    }", "}",
        "static inline unsigned short sc64_unit_attack_source_full_duration(unsigned short raw_type, int target_air) {",
        "    switch (raw_type) {"])
    for raw, modes in sorted(rows_by_raw.items()):
        lines.append(f"    case {raw}u:")
        for air, (_release, _last, _symbol, _n, full_duration,
                  _full_last, _full_symbol, _full_n) in sorted(modes.items()):
            lines.append(f"        if (!!target_air == {air}) return {full_duration}u;")
        lines.append("        return 0u;")
    lines.extend(["    default: return 0u;", "    }", "}",
        "static inline const Sc64SpriteFrame *sc64_unit_attack_source_full_frame(unsigned short raw_type, int target_air, unsigned char heading32, unsigned int attack_tick) {",
        "    unsigned char dir = heading32 & 31u;",
        "    switch (raw_type) {"])
    for raw, modes in sorted(rows_by_raw.items()):
        lines.append(f"    case {raw}u: {{")
        for air, (_release, _last, _symbol, _n, full_duration,
                  full_last, symbol, count) in sorted(modes.items()):
            lines.extend([f"        if (!!target_air == {air}) {{",
                f"            if (attack_tick >= {full_duration}u) return 0;",
                f"            const Sc64UnitAttackPoseStep *steps = {symbol};",
                "            unsigned int selected = 0u;",
                f"            for (unsigned int i=1; i<{count}u; ++i) {{",
                "                if (steps[i].tick > attack_tick) break;",
                "                selected=i;", "            }",
                "            return steps[selected].heading[dir];", "        }"])
        lines.extend(["        return 0;", "    }"])
    lines.extend(["    default: return 0;", "    }", "}",
                  f"#define SC64_UNIT_ATTACK_POSE_PIXEL_BYTES {payload_bytes}u",
                  "#endif", ""])
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("\n".join(lines), encoding="ascii")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--rom", type=Path, default=ROM)
    parser.add_argument("--remap", type=Path, default=REMAP)
    parser.add_argument("--out", type=Path, default=OUT)
    parser.add_argument("--header", type=Path, default=HEADER)
    args = parser.parse_args()
    manifest = generate(args.rom, args.remap, args.out, args.header)
    print(f"generated {args.header} ({len(manifest['actors'])} actors)")
    for actor in manifest["actors"]:
        print(f"  raw{actor['raw_unit']} {actor['name']}: " +
              ", ".join(f"{mode['animation']}={mode['status']}" for mode in actor["modes"]))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
