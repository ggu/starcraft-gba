#!/usr/bin/env python3
"""Compile authenticated source weapon images/scripts into a private C bank.

Generated art is derived from the USA StarCraft 64 ROM and remains under
``.local/sc64/weapon-vfx/generated``. Unknown drawing modifiers and malformed
source records fail closed.
"""
# SPDX-License-Identifier: GPL-3.0-or-later
from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_ROM = ROOT / ".local/sc64/rom/StarCraft64-USA.z64"
DEFAULT_OUT = ROOT / ".local/sc64/weapon-vfx/generated"
DEFAULT_REMAP = ROOT / ".local/sc64/backwater/generated/palette-remap.bin"
DEFAULT_PALETTE = ROOT / ".local/sc64/backwater/generated/palette.json"
ROM_SHA1 = "a36d91b5aac7bb95df0d7de5f9814f5270e25c2b"
ANIMS = {"Init": 0, "Death": 1, "GndAttkInit": 2, "GndAttkRpt": 3,
         "AirAttkInit": 4, "AirAttkRpt": 5, "SpecialState1": 6, "WarpIn": 7}
ANIM_ALIASES = {"GndAttkToIdle": 2, "AirAttkToIdle": 4}
MODIFIER_TABLE_BINDING = "inferred from source shift, table dimensions, and pinned renderer; not claimed as native N64 pixel proof"


def modifier9_table_path(color_shift: int) -> str:
    if not 1 <= color_shift <= 4:
        raise ValueError(f"unsupported modifier-9 color shift {color_shift}")
    # Source image shift 1 (including the authenticated Tank muzzle) maps to
    # BOLT 00B/006 in the existing source-art audit; subsequent shifts advance.
    return f"00B/{5 + color_shift:03X}"


def _load(name: str, path: Path):
    if str(path.parent) not in sys.path:
        sys.path.insert(0, str(path.parent))
    spec = importlib.util.spec_from_file_location(name, path)
    if not spec or not spec.loader:
        raise ValueError(f"cannot load source parser: {path}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


def convert_pixels(indices: bytes, alpha: bytes, modifier: int, remap: bytes) -> bytes:
    """Convert one cropped source frame to the runtime's byte selector format."""
    if len(indices) != len(alpha) or len(remap) != 256:
        raise ValueError("pixel, alpha, and 256-entry remap sizes must agree")
    if modifier in (0, 1):
        out = bytearray(len(indices))
        for i, (src, opaque) in enumerate(zip(indices, alpha)):
            value = remap[src] if opaque else 0
            if opaque and value == 0:
                raise ValueError(f"opaque source palette index {src} maps to transparent index zero")
            out[i] = value
        return bytes(out)
    if modifier == 9:
        out = bytearray(len(indices))
        for i, (src, opaque) in enumerate(zip(indices, alpha)):
            if opaque:
                if not 1 <= src <= 63:
                    raise ValueError(f"modifier-9 strength selector {src} is outside 1..63")
                # These authenticated GRPs use values 1..63 for strengths;
                # the pinned renderer selects light-table row value - 1.
                out[i] = src
        return bytes(out)
    if modifier == 10:
        return bytes(1 if opaque else 0 for opaque in alpha)
    raise ValueError(f"unsupported source image modifier {modifier}")


def direction_frame(first: int, count: int, heading: int) -> tuple[int, bool]:
    if not 0 <= heading <= 31 or count <= 0:
        raise ValueError("invalid direction/frame range")
    folded = heading if heading <= 16 else 32 - heading
    if folded >= count or first + folded > 0xFFFF:
        raise ValueError("directional frame is outside the source image")
    return first + folded, heading > 16


def mirror_crop(x: int, width: int, canvas_width: int) -> int:
    if min(x, width, canvas_width) < 0 or x + width > canvas_width:
        raise ValueError("crop is outside its source canvas")
    return canvas_width - (x + width)


def _c_array(ctype: str, name: str, values: list[str], per_line: int = 8) -> list[str]:
    lines = [f"static const {ctype} {name}[{len(values) or 1}] = {{"]
    if not values:
        values = ["0"]
    for i in range(0, len(values), per_line):
        lines.append("    " + ", ".join(values[i:i + per_line]) + ",")
    lines.append("};")
    return lines


def _sorted_animation_rows(rows: list[dict]) -> list[dict]:
    """Keep generated animation rows ordered by their composite key."""
    return sorted(rows, key=lambda row: (int(row["image_id"]), int(row["animation_id"])))


def _lookup_accessor_lines() -> list[str]:
    """Emit logarithmic lookups over sorted profile, image, and animation rows."""
    return [
        "static inline const Sc64WeaponSourceAttackProfile *sc64_weapon_source_profile_lookup(uint8_t actor_raw,uint8_t weapon_id,uint8_t mode,uint8_t variant) {",
        "    unsigned lo=0,hi=SC64_WEAPON_SOURCE_PROFILE_COUNT;while(lo<hi){unsigned m=lo+(hi-lo)/2;const Sc64WeaponSourceAttackProfile *p=&sc64_weapon_source_profiles[m];int less=p->actor_raw<actor_raw||(p->actor_raw==actor_raw&&(p->weapon_id<weapon_id||(p->weapon_id==weapon_id&&(p->mode<mode||(p->mode==mode&&p->variant<variant)))));if(less)lo=m+1;else hi=m;}if(lo<SC64_WEAPON_SOURCE_PROFILE_COUNT){const Sc64WeaponSourceAttackProfile *p=&sc64_weapon_source_profiles[lo];if(p->actor_raw==actor_raw&&p->weapon_id==weapon_id&&p->mode==mode&&p->variant==variant)return p;}if(variant==1){lo=0;hi=SC64_WEAPON_SOURCE_PROFILE_COUNT;while(lo<hi){unsigned m=lo+(hi-lo)/2;const Sc64WeaponSourceAttackProfile *p=&sc64_weapon_source_profiles[m];int less=p->actor_raw<actor_raw||(p->actor_raw==actor_raw&&(p->weapon_id<weapon_id||(p->weapon_id==weapon_id&&p->mode<mode)));if(less)lo=m+1;else hi=m;}if(lo<SC64_WEAPON_SOURCE_PROFILE_COUNT){const Sc64WeaponSourceAttackProfile *p=&sc64_weapon_source_profiles[lo];if(p->actor_raw==actor_raw&&p->weapon_id==weapon_id&&p->mode==mode&&p->variant==0&&p->weapon_count>1)return p;}}return 0; }",
        "static inline const Sc64WeaponSourceImage *sc64_weapon_source_image_lookup(uint16_t id) { unsigned lo=0,hi=SC64_WEAPON_SOURCE_IMAGE_COUNT;while(lo<hi){unsigned m=lo+(hi-lo)/2;if(sc64_weapon_source_images[m].image_id<id)lo=m+1;else hi=m;}return lo<SC64_WEAPON_SOURCE_IMAGE_COUNT&&sc64_weapon_source_images[lo].image_id==id?&sc64_weapon_source_images[lo]:0; }",
        "static inline const Sc64WeaponSourceFrame *sc64_weapon_source_frame_lookup(uint16_t image_id,uint16_t source_frame_id) { const Sc64WeaponSourceImage *im=sc64_weapon_source_image_lookup(image_id); if(!im||source_frame_id>=im->source_frame_count)return 0; uint16_t i=sc64_weapon_source_frame_directory[im->frame_directory_first+source_frame_id]; return i<SC64_WEAPON_SOURCE_FRAME_COUNT?&sc64_weapon_source_frames[i]:0; }",
        "static inline const uint8_t *sc64_weapon_source_frame_pixels(const Sc64WeaponSourceFrame *f,int mirror) { if(!f||(mirror&&f->mirror_pixel_offset==UINT32_MAX))return 0; uint32_t o=mirror?f->mirror_pixel_offset:f->pixel_offset; return o<sizeof(sc64_weapon_source_pixels)?sc64_weapon_source_pixels+o:0; }",
        "static inline const Sc64WeaponSourceAnimation *sc64_weapon_source_animation_lookup(uint16_t image_id,uint8_t animation_id) { unsigned lo=0,hi=SC64_WEAPON_SOURCE_ANIMATION_COUNT;while(lo<hi){unsigned m=lo+(hi-lo)/2;const Sc64WeaponSourceAnimation *a=&sc64_weapon_source_animations[m];if(a->image_id<image_id||(a->image_id==image_id&&a->animation_id<animation_id))lo=m+1;else hi=m;}return lo<SC64_WEAPON_SOURCE_ANIMATION_COUNT&&sc64_weapon_source_animations[lo].image_id==image_id&&sc64_weapon_source_animations[lo].animation_id==animation_id?&sc64_weapon_source_animations[lo]:0; }",
        "static inline const Sc64WeaponSourceLut *sc64_weapon_source_lut_for_image(uint16_t image_id) { const Sc64WeaponSourceImage *im=sc64_weapon_source_image_lookup(image_id); return im&&im->lut_index<SC64_WEAPON_SOURCE_LUT_COUNT?&sc64_weapon_source_luts[im->lut_index]:0; }",
        "static inline int sc64_weapon_source_lo_lookup(uint16_t parent_image,uint8_t lo_kind,uint16_t parent_frame,uint8_t offset_index,int16_t *x,int16_t *y) { if(!x||!y)return 0; for(unsigned i=0;i<SC64_WEAPON_SOURCE_LO_TABLE_COUNT;i++){const Sc64WeaponSourceLoTable *t=&sc64_weapon_source_lo_tables[i];if(t->image_id!=parent_image||t->lo_kind!=lo_kind)continue;if(parent_frame>=t->frame_count||offset_index>=t->offset_count)return 0;uint32_t n=t->offset_first+(uint32_t)parent_frame*t->offset_count+offset_index;if(n>=SC64_WEAPON_SOURCE_LO_OFFSET_COUNT)return 0;*x=sc64_weapon_source_lo_offsets[n].x;*y=sc64_weapon_source_lo_offsets[n].y;return 1;}return 0;}",
    ]


def _first_int(value, default: int = 0) -> int:
    if isinstance(value, int):
        return value
    if isinstance(value, (list, tuple)):
        for item in value:
            found = _first_int(item, None)
            if found is not None:
                return found
    return default


def _load_source(rom_path: Path, remap_path: Path, palette_path: Path, work: Path):
    rom = rom_path.read_bytes()
    sha = hashlib.sha1(rom).hexdigest()
    if sha != ROM_SHA1:
        raise ValueError(f"unauthenticated ROM SHA1 {sha}; expected {ROM_SHA1}")
    audit = _load("sc64_weapon_source_audit_import", ROOT / "tools/sc64_weapon_source_audit.py")
    summary = audit.generate(rom_path, work)
    report_path = Path(summary["out"])
    report = json.loads(report_path.read_text())
    if report["reference"]["rom_sha1"] != ROM_SHA1:
        raise ValueError("audit report ROM identity mismatch")
    remap = remap_path.read_bytes()
    if len(remap) != 256:
        raise ValueError("palette remap must contain 256 bytes")
    palette_json = json.loads(palette_path.read_text())
    palette_values = palette_json.get("source_rgb555")
    if not isinstance(palette_values, list) or len(palette_values) != 256:
        raise ValueError("palette JSON must contain exactly 256 source colors")
    return report, remap, palette_json, rom


def _source_bolt(rom: bytes):
    extractor = _load("sc64_weapon_source_bolt_import", ROOT / "references/sc64-maps/extract_sc64_maps.py")
    sprites = _load("sc64_weapon_source_grp_import", ROOT / "tools/sc64_sprites.py")
    archive = extractor.BoltArchive(rom)
    entries = {entry.path: entry for entry in archive.entries()}
    def get(path: str) -> bytes:
        if path not in entries:
            raise ValueError(f"authenticated BOLT archive missing {path}")
        return archive.read(entries[path])
    return get, sprites


def _used_frames(report: dict) -> dict[int, set[int]]:
    used: dict[int, set[int]] = {}
    for image_key, animations in report["source_animations"].items():
        image = int(image_key)
        for anim in animations.values():
            if anim.get("status") != "ok":
                continue
            for event in anim.get("events", []):
                if event.get("name") == "playframe" and event.get("args"):
                    frame = int(event["args"][0])
                    used.setdefault(image, set()).add(frame)
    for image, item in report["referenced_images"].items():
        image_id = int(image)
        if item.get("directional"):
            # Every attack pose is a 17-frame bank. Exact source playframe
            # operands are retained; expanding nearby directions enables the
            # runtime's logical heading fold without inventing poses.
            for first in tuple(used.get(image_id, ())):
                base = first - first % 17
                used[image_id].update(range(base, min(base + 17, int(item.get("frame_count", 0)))))
        if not used.get(image_id):
            used.setdefault(image_id, set()).add(0)
    return used


def _runtime_images(report: dict) -> set[int]:
    """Keep raster pixels only for weapon images and authored child images."""
    projectiles: set[int] = set()
    actor_roots: set[int] = set()
    for family in ("units", "buildings"):
        for actor in report.get(family, []):
            for image_record in actor.get("actor_images", []) + actor.get("attack_images", []):
                image = image_record.get("image")
                if isinstance(image, int) and image >= 0:
                    actor_roots.add(image)
            for weapon in actor.get("weapons", []):
                chain = weapon.get("weapon_graphic_chain") or {}
                image = chain.get("image")
                if isinstance(image, int) and image >= 0:
                    projectiles.add(image)
            launched = actor.get("launched_unit_visual") or {}
            chain = launched.get("weapon_graphic_chain") or {}
            image = chain.get("image")
            if isinstance(image, int) and image >= 0:
                projectiles.add(image)
    source_animations = report.get("source_animations", {})
    attack_names = {"GndAttkInit", "AirAttkInit", "GndAttkRpt", "AirAttkRpt",
                    "GndAttkToIdle", "AirAttkToIdle"}
    result = set(projectiles)
    queue = list(projectiles)

    def children(image_id: int, names: set[str]):
        found = []
        for name in names:
            anim = source_animations.get(str(image_id), {}).get(name, {})
            for child in anim.get("visual_references", []):
                child_image = child.get("image_id")
                if isinstance(child_image, int) and child_image >= 0:
                    found.append(child_image)
        return found

    # Actor roots contribute only attack attachments. Init attachments are
    # retained when they are independently animated attackers (e.g. Archon
    # image135); idle/death-only actor decoration remains in its own renderer.
    for image_id in actor_roots:
        child_ids = children(image_id, attack_names)
        for child_id in children(image_id, {"Init"}):
            child_anims = source_animations.get(str(child_id), {})
            if any(any(int(e.get("opcode", -1)) in (28,37,38)
                       for e in child_anims.get(name, {}).get("events", []))
                   for name in attack_names):
                child_ids.append(child_id)
        for child_id in child_ids:
            if child_id not in result:
                result.add(child_id)
                queue.append(child_id)

    # Weapon and child images keep their authored attachments and lifecycles.
    # Recursion is bounded by the finite referenced-image graph.
    processed = set()
    while queue:
        image_id = queue.pop()
        if image_id in processed:
            continue
        processed.add(image_id)
        for child_id in children(image_id, {"Init", "Death"} | attack_names):
            if child_id not in result:
                result.add(child_id)
                queue.append(child_id)
    return result


def _project_luts(get, palette_json: dict, images: dict[str, dict]) -> tuple[list[dict], bytes, dict]:
    helper = _load("sc64_weapon_source_effect_palette", ROOT / "tools/sc64_effect_palette.py")
    source_palette = get("00B/004")
    if len(source_palette) != 512:
        raise ValueError("source 00B/004 palette must be 256 RGBA5551 words")
    display = palette_json["gba_rgb555"]
    tables, payload, source_hashes = [], bytearray(), {}
    seen = set()
    for image, metadata in sorted(images.items(), key=lambda pair: int(pair[0])):
        modifier, shift = int(metadata["modifier"]), int(metadata["color_shift"])
        if modifier not in (9, 10):
            continue
        key = (modifier, shift)
        if key in seen:
            continue
        seen.add(key)
        if modifier == 9:
            table_path = modifier9_table_path(shift)
            raw = get(table_path)
            if len(raw) % 256:
                raise ValueError(f"light table {table_path} is not whole 256-byte rows")
            projected = helper.project_light_table(source_palette, raw, display)
            rows = len(projected) // 256
            offset = len(payload)
            payload.extend(projected)
            tables.append({"modifier": modifier, "shift": shift, "offset": offset,
                           "rows": rows, "source_rows": rows, "bolt_path": table_path})
            source_hashes[table_path] = hashlib.sha256(raw).hexdigest()
        else:
            raw = get("00B/005")
            if len(raw) < 19 * 256:
                raise ValueError("dark table lacks the documented row 18")
            row = raw[18 * 256:19 * 256]
            projected = helper.project_light_table(source_palette, row, display)
            offset = len(payload); payload.extend(projected)
            tables.append({"modifier": modifier, "shift": shift, "offset": offset,
                           "rows": 1, "source_rows": 1, "bolt_path": "00B/005", "row": 18})
            source_hashes["00B/005"] = hashlib.sha256(raw).hexdigest()
    source_hashes["00B/004"] = hashlib.sha256(source_palette).hexdigest()
    return tables, bytes(payload), source_hashes


def _collect_lo_tables(get, image_data: bytes, report: dict):
    """Decode image attack/damage/special/landing/liftoff/shield LO banks."""
    referenced_images = report["referenced_images"]
    count = 999
    byte_columns = 4 * count + 6 * count
    script_end = byte_columns + 4 * count
    field_names = ("shield", "attack", "damage", "special", "landing", "liftoff")
    if len(image_data) < script_end + 6 * 4 * count:
        raise ValueError("images.dat truncated before LO filename columns")
    starts = {name: script_end + i * 4 * count for i, name in enumerate(field_names)}
    # The runtime's lo_kind follows OpenBW image_lo_offsets ordering.
    kinds = ((0, "attack"), (1, "damage"), (2, "special"),
             (3, "landing"), (4, "liftoff"), (5, "shield"))
    requested = set()
    for image_key, animations in report.get("source_animations", {}).items():
        parent_image = int(image_key)
        for animation in animations.values():
            for child in animation.get("visual_references", []):
                args = child.get("args", [])
                if int(child.get("opcode", -1)) in (13, 14, 21) and len(args) >= 2:
                    requested.add((parent_image, _first_int(args[1])))
    records, offsets, hashes = [], [], {}
    for image_id, lo_kind in sorted(requested):
        image_key = str(image_id)
        if image_key not in referenced_images or not 0 <= lo_kind < len(kinds):
            raise ValueError(f"LO script reference has invalid parent/category: {image_id}/{lo_kind}")
        for expected_kind, column in kinds:
            if expected_kind != lo_kind:
                continue
            gfi = struct.unpack_from("<I", image_data, starts[column] + image_id * 4)[0]
            if not gfi:
                raise ValueError(f"image {image_id} script uses LO category {lo_kind}, but images.dat has no {column} table")
            flat = gfi - 1
            path = f"{0x0D + flat // 256:03X}/{flat % 256:03X}"
            raw = get(path)
            if len(raw) < 8:
                raise ValueError(f"LO table {path} for image {image_id} is truncated")
            frame_count, offset_count = struct.unpack_from("<II", raw)
            directory_end = 8 + frame_count * 4
            if not frame_count or not offset_count or directory_end > len(raw):
                raise ValueError(f"LO table {path} has invalid dimensions")
            first = len(offsets)
            for frame in range(frame_count):
                pos = struct.unpack_from("<I", raw, 8 + frame * 4)[0]
                end = pos + offset_count * 2
                if pos < directory_end or end > len(raw):
                    raise ValueError(f"LO table {path} frame {frame} offset is out of range")
                for index in range(offset_count):
                    x, y = struct.unpack_from("<bb", raw, pos + index * 2)
                    offsets.append({"x": x, "y": y})
            records.append({"image_id": image_id, "lo_kind": lo_kind,
                            "frame_count": frame_count, "offset_count": offset_count,
                            "offset_first": first, "gfi": gfi, "bolt_path": path})
            hashes[path] = hashlib.sha256(raw).hexdigest()
    return records, offsets, hashes


def _profile_rows(report: dict, animations: list[dict], events: list[dict], overlays: list[dict]) -> list[dict]:
    animation_lookup = {(a["image_id"], a["animation_id"]): a for a in animations}
    image_lookup = {int(key): value for key, value in report["referenced_images"].items()}
    source_animations = report.get("source_animations", {})

    def attack_image_for(root_image: int, attack_anim: int) -> int:
        """Follow attached source images when the weapon signal lives on a child."""
        anim_name = {2:"GndAttkInit", 3:"GndAttkRpt", 4:"AirAttkInit", 5:"AirAttkRpt"}.get(attack_anim)
        if anim_name is None or root_image < 0:
            return root_image
        queue, seen = [root_image], set()
        while queue:
            image_id = queue.pop(0)
            if image_id in seen:
                continue
            seen.add(image_id)
            image_anims = source_animations.get(str(image_id), {})
            current = image_anims.get(anim_name, {})
            if any(int(event.get("opcode", -1)) in (28,37,38)
                   for event in current.get("events", [])):
                return image_id
            # Main image child weapons (for example Archon image 135) own
            # their own firing signal and muzzle LO script.
            for parent_anim_name in ("Init", "GndAttkInit", "AirAttkInit"):
                parent_anim = image_anims.get(parent_anim_name, {})
                for child in parent_anim.get("visual_references", []):
                    child_image = child.get("image_id")
                    if isinstance(child_image, int) and child_image >= 0:
                        queue.append(child_image)
        return root_image

    def bind(profile: dict, attack_image: int, projectile_image: int):
        attack_anim = profile["attack_anim"]
        attack = animation_lookup.get((attack_image, attack_anim))
        if attack:
            profile["attack_event_first"] = attack["event_first"]
            profile["attack_event_count"] = attack["event_count"]
            profile["overlay_first"] = attack["overlay_first"]
            profile["overlay_count"] = attack["overlay_count"]
            end = attack["event_first"] + attack["event_count"]
            release = next((e["source_tick"] for e in events[attack["event_first"]:end]
                            if e["opcode"] in (28,37,38)), None)
            if release is not None:
                profile["attackwith_tick"] = release
        projectile = animation_lookup.get((projectile_image, 0))
        if projectile:
            profile["projectile_event_first"] = projectile["event_first"]
            profile["projectile_event_count"] = projectile["event_count"]
            end = projectile["event_first"] + projectile["event_count"]
            frame_events = [e for e in events[projectile["event_first"]:end] if e["source_frame"] != 0xFFFF]
            if frame_events:
                profile["projectile_first_frame"] = frame_events[0]["source_frame"]
                profile["projectile_frame_count"] = len({e["source_frame"] for e in frame_events})
                if len(frame_events) > 1:
                    deltas = [b["source_tick"]-a["source_tick"] for a,b in zip(frame_events,frame_events[1:]) if b["source_tick"] > a["source_tick"]]
                    if deltas:
                        profile["projectile_ticks_per_frame"] = min(deltas)
        profile["attack_image"] = attack_image if attack_image >= 0 else 0xFFFF
        profile["projectile_image"] = projectile_image if projectile_image >= 0 else 0xFFFF
        if projectile_image in image_lookup:
            profile["flags"] |= 1

    rows = []
    for unit in report["units"]:
        actor = int(unit["raw_unit"])
        for weapon in unit.get("weapons", []):
            wid = int(weapon["weapon_id"])
            fields = weapon["weapon_fields"]
            chain = weapon.get("weapon_graphic_chain") or {}
            image = int(chain.get("image", 0xFFFF))
            owner = int(weapon.get("weapon_owner_raw", actor))
            owner_root_image = next((int(x["image"]) for x in unit.get("actor_images", [])
                                if int(x.get("raw_unit", -1)) == owner and x.get("image") is not None), -1)
            owner_image = attack_image_for(owner_root_image, 2 if weapon["mode"] == "ground" else 4)
            motion = chain.get("flingy_motion", {})
            row = {"actor_raw": actor, "weapon_owner_raw": owner,
                         "weapon_id": wid, "mode": 0 if weapon["mode"] == "ground" else 1,
                         "variant": 0, "attack_anim": 2 if weapon["mode"] == "ground" else 4,
                         "attackwith_tick": 0xFFFF, "projectile_ticks_per_frame": 1,
                         "flags": 0, "weapon_bullet_type": int(fields["bullet_type"]),
                         "weapon_count": int(fields["count"]), "attack_image": image,
                         "projectile_image": image, "projectile_flingy": int(fields["flingy"]),
                         "projectile_lifetime": int(fields["lifetime"]), "projectile_first_frame": 0,
                         "projectile_frame_count": 0, "impact_source_tick": 0xFFFF,
                         "attack_event_first": 0, "attack_event_count": 0,
                         "projectile_event_first": 0, "projectile_event_count": 0,
                         "overlay_first": 0, "overlay_count": 0,
                         "speed_raw": int(motion.get("top_speed_raw",0)), "accel_raw": int(motion.get("acceleration_raw",0)), "halt_raw": int(motion.get("halt_distance_raw",0)),
                         "forward_offset": int(fields["forward"]), "upward_offset": int(fields["upward"])}
            bind(row, owner_image, image)
            rows.append(row)
    for building in report["buildings"]:
        actor = int(building["raw_building"])
        for weapon in building.get("weapons", []):
            wid = int(weapon["weapon_id"]); fields = weapon["weapon_fields"]
            chain = weapon.get("weapon_graphic_chain") or {}; image = int(chain.get("image", 0xFFFF))
            owner = int(weapon.get("weapon_owner_raw", actor))
            owner_root_image = next((int(x["image"]) for x in building.get("attack_images", [])
                                if int(x.get("raw_unit", -1)) == owner and x.get("image") is not None), -1)
            owner_image = attack_image_for(owner_root_image, 2 if weapon["mode"] == "ground" else 4)
            motion = chain.get("flingy_motion", {})
            row = {"actor_raw": actor, "weapon_owner_raw": owner,
                         "weapon_id": wid, "mode": 0 if weapon["mode"] == "ground" else 1,
                         "variant": 2, "attack_anim": 2 if weapon["mode"] == "ground" else 4,
                         "attackwith_tick": 0xFFFF, "projectile_ticks_per_frame": 1,
                         "flags": 0, "weapon_bullet_type": int(fields["bullet_type"]),
                         "weapon_count": int(fields["count"]), "attack_image": image,
                         "projectile_image": image, "projectile_flingy": int(fields["flingy"]),
                         "projectile_lifetime": int(fields["lifetime"]), "projectile_first_frame": 0,
                         "projectile_frame_count": 0, "impact_source_tick": 0xFFFF,
                         "attack_event_first": 0, "attack_event_count": 0,
                         "projectile_event_first": 0, "projectile_event_count": 0,
                         "overlay_first": 0, "overlay_count": 0,
                         "speed_raw": int(motion.get("top_speed_raw",0)), "accel_raw": int(motion.get("acceleration_raw",0)), "halt_raw": int(motion.get("halt_distance_raw",0)),
                         "forward_offset": int(fields["forward"]), "upward_offset": int(fields["upward"])}
            bind(row, owner_image, image)
            rows.append(row)
    # Reaver uses a launched Scarab actor. Keep this separate profile rather
    # than assigning weapon 82 to the Reaver's own weapons.dat row.
    reaver = next((u for u in report["units"] if int(u["raw_unit"]) == 83), None)
    if reaver and reaver.get("launched_unit_visual"):
        item = reaver["launched_unit_visual"]; fields = item["weapon_fields"]
        chain = item.get("weapon_graphic_chain") or {}; image = int(chain.get("image", 0xFFFF))
        owner_image = attack_image_for(int(item.get("actor_image", -1)),2)
        motion = chain.get("flingy_motion", {})
        row = {"actor_raw": 83, "weapon_owner_raw": 85, "weapon_id": int(item["ground_weapon_id"]),
                     "mode": 0, "variant": 1, "attack_anim": 2, "attackwith_tick": 0xFFFF,
                     "projectile_ticks_per_frame": 1, "flags": 0,
                     "weapon_bullet_type": int(fields["bullet_type"]), "weapon_count": int(fields["count"]),
                     "attack_image": image, "projectile_image": image, "projectile_flingy": int(fields["flingy"]),
                     "projectile_lifetime": int(fields["lifetime"]), "projectile_first_frame": 0,
                     "projectile_frame_count": 0, "impact_source_tick": 0xFFFF,
                     "attack_event_first": 0, "attack_event_count": 0,
                     "projectile_event_first": 0, "projectile_event_count": 0,
                     "overlay_first": 0, "overlay_count": 0, "speed_raw": int(motion.get("top_speed_raw",0)), "accel_raw": int(motion.get("acceleration_raw",0)),
                     "halt_raw": int(motion.get("halt_distance_raw",0)), "forward_offset": int(fields["forward"]),
                     "upward_offset": int(fields["upward"])}
        bind(row, owner_image, image)
        rows.append(row)
    keys = [(r["actor_raw"], r["weapon_id"], r["mode"], r["variant"]) for r in rows]
    if len(keys) != len(set(keys)):
        raise ValueError("duplicate actor/weapon/mode/variant source profile")
    return sorted(rows, key=lambda r: (r["actor_raw"], r["weapon_id"], r["mode"], r["variant"]))


def _profile_cull_bounds(profiles, images, frames, animations, overlays, lo_tables, lo_offsets, events, frame_directory, stats=None):
    """Conservative one-level render envelopes; invalid input falls back globally."""
    invalid = 0x40000000
    allowed = {8, 9, 10, 11, 13, 14, 15, 16, 17, 19, 20, 21}
    def number(value, low, high):
        if type(value) is not int or not low <= value <= high: raise ValueError("field range")
        return value
    try:
        by_image = {int(row["image_id"]): row for row in images}
        if len(by_image) != len(images): raise ValueError("duplicate image")
        for image in images:
            number(image["image_id"],0,65535)
            first, count = number(image["frame_first"],0,65535), number(image["frame_count"],0,65535)
            dfirst = number(image["frame_directory_first"],0,65535)
            dcount = number(image["source_frame_count"],0,65535)
            if dfirst+dcount > len(frame_directory): raise ValueError("directory range")
            if any(v != 0xffff and not first <= v < first+count for v in frame_directory[dfirst:dfirst+dcount]):
                raise ValueError("directory frame identity")
            if first < 0 or count < 0 or first + count > len(frames): raise ValueError("frame range")
            if any(int(f["image_id"]) != int(image["image_id"]) for f in frames[first:first+count]):
                raise ValueError("frame identity")
        for f in frames:
            for name in ("x", "y", "mirror_x"): number(f[name],-32768,32767)
            for name in ("w", "h", "canvas_w", "canvas_h", "image_id"): number(f[name],0,65535)
        animation_keys = set()
        for anim in animations:
            key = (int(anim["image_id"]), int(anim["animation_id"]))
            if key in animation_keys: raise ValueError("duplicate animation")
            animation_keys.add(key)
            number(anim["image_id"],0,65535); number(anim["animation_id"],0,255)
            efirst, ecount = number(anim["event_first"],0,65535), number(anim["event_count"],0,65535)
            if efirst+ecount > len(events): raise ValueError("event range")
            if any(int(e["image_id"]) != key[0] for e in events[efirst:efirst+ecount]): raise ValueError("event identity")
            first, count = number(anim["overlay_first"],0,65535), number(anim["overlay_count"],0,65535)
            if first < 0 or count < 0 or first + count > len(overlays): raise ValueError("overlay range")
            if any(int(o["parent_image"]) != key[0] for o in overlays[first:first+count]):
                raise ValueError("overlay parent")
        for o in overlays:
            for name in ("parent_image", "image_id"): number(o[name],0,65535)
            for name in ("x", "y"): number(o[name],-32768,32767)
            if number(o["opcode"],0,255) not in allowed: raise ValueError("unknown overlay")
        for o in lo_offsets:
            number(o["x"],-128,127); number(o["y"],-128,127)
        for table in lo_tables:
            number(table["image_id"],0,65535); number(table["lo_kind"],0,255)
            first = number(table["offset_first"],0,0xffffffff)
            count = number(table["frame_count"],0,65535) * number(table["offset_count"],0,65535)
            if first < 0 or count < 0 or first + count > len(lo_offsets): raise ValueError("LO range")
    except (KeyError, TypeError, ValueError, OverflowError) as error:
        if stats is not None: stats[str(error)] = len(profiles)
        return [invalid] * len(profiles)
    result = []
    for profile in profiles:
        try:
            projectile = int(profile["projectile_image"])
            actor = int(profile["attack_image"])
            if projectile not in by_image: raise ValueError("projectile image")
            parents = {projectile}
            if int(profile["attackwith_tick"]) != 0xffff:
                if actor not in by_image: raise ValueError("actor image")
                parents.add(actor)
            children = set()
            coords = []
            for anim in animations:
                if int(anim["image_id"]) not in parents: continue
                first, count = int(anim["overlay_first"]), int(anim["overlay_count"])
                for overlay in overlays[first:first+count]:
                    children.add(int(overlay["image_id"]))
                    if overlay["opcode"] in (13, 14, 21):
                        kind = number(overlay["lo_index"],0,255)
                        for table in lo_tables:
                            if table["image_id"] == anim["image_id"] and table["lo_kind"] == kind:
                                start = table["offset_first"]
                                end = start + table["frame_count"] * table["offset_count"]
                                coords.extend((o["x"], o["y"]) for o in lo_offsets[start:end])
                    else:
                        coords.append((overlay["x"], overlay["y"]))
            ox = max((abs(x)//2 for x, _ in coords), default=0)
            oy = max((abs(y)//2 for _, y in coords), default=0)
            if any(i not in by_image for i in children): raise ValueError("child image")
            # Actor itself is not drawn by this path, but every projectile and
            # one-level child frame, including mirrored/fallback frames, is.
            radius_x = radius_y = 0
            for image_id in {projectile} | children:
                image = by_image[image_id]
                first, count = int(image["frame_first"]), int(image["frame_count"])
                # An emitted image with no frames resolves only missing-frame
                # sentinels; the renderer draws no pixels for that image.
                for f in frames[first:first+count]:
                    w, h = int(f["w"]), int(f["h"])
                    if w <= 0 or h <= 0: raise ValueError("empty frame")
                    left = int(f["x"]) - int(f["canvas_w"])//2
                    mirror = int(f["mirror_x"]) - int(f["canvas_w"])//2
                    top = int(f["y"]) - int(f["canvas_h"])//2
                    radius_x = max(radius_x,abs(left),abs(left+w-1),abs(mirror),abs(mirror+w-1))
                    radius_y = max(radius_y,abs(top),abs(top+h-1))
            radius_x += ox; radius_y += oy
            if max(radius_x, radius_y) > 0x7fff: raise ValueError("radius overflow")
            result.append(0x80000000 | (radius_y << 15) | radius_x)
        except (KeyError, TypeError, ValueError, OverflowError) as error:
            if stats is not None: stats[str(error)] = stats.get(str(error), 0) + 1
            result.append(invalid)
    return result


def _emit_header(out: Path, report: dict, remap: bytes, palette_json: dict, get, sprites):
    images = report["referenced_images"]
    used = _used_frames(report)
    raster_images = _runtime_images(report)
    raster_metadata = {key:value for key,value in images.items() if int(key) in raster_images}
    luts, lutbytes, table_hashes = _project_luts(get, palette_json, raster_metadata)
    lo_tables, lo_offsets, lo_hashes = _collect_lo_tables(get, get("00C/000"), report)
    frames, pixel_data, frame_index = [], bytearray(), {}
    pixel_blob_offsets: dict[bytes, int] = {}
    unshared_pixel_bytes = 0
    image_rows = []
    for image_key, meta in sorted(images.items(), key=lambda pair: int(pair[0])):
        image_id = int(image_key)
        if not meta.get("grp") or meta.get("grp_error"):
            continue
        modifier = int(meta["modifier"])
        if modifier not in (0, 1, 9, 10):
            raise ValueError(f"image {image_id}: unsupported modifier {modifier}")
        grp = sprites.parse_grp(get(meta["grp"]))
        first = len(frames)
        selected = sorted(used.get(image_id, {0})) if image_id in raster_images else []
        for frame_id in selected:
            if frame_id >= len(grp["frames"]):
                raise ValueError(f"image {image_id} references missing source frame {frame_id}")
            src = grp["frames"][frame_id]
            rect = (src["x"], src["y"], src["frame_width"], src["frame_height"])
            indices = sprites.crop_frame(src)
            alpha = bytearray()
            for row in range(src["frame_height"]):
                start = (src["y"] + row) * src["width"] + src["x"]
                alpha.extend(src["alpha"][start:start + src["frame_width"]])
            converted = convert_pixels(indices, bytes(alpha), modifier, remap)
            unshared_pixel_bytes += len(converted)
            offset = pixel_blob_offsets.get(converted)
            if offset is None:
                offset = len(pixel_data)
                pixel_blob_offsets[converted] = offset
                pixel_data.extend(converted)
            mirror_x = mirror_crop(src["x"], src["frame_width"], grp["width"])
            # Keep mirrored crop origin but not a duplicate byte-for-byte
            # mirror blob. The renderer's bounded horizontal fallback reads
            # the same crop in reverse when this sentinel is present.
            mirror_offset = 0xFFFFFFFF
            frame_index[(image_id, frame_id)] = len(frames)
            frames.append({"pixel_offset": offset, "mirror_pixel_offset": mirror_offset,
                           "x": src["x"], "y": src["y"], "w": src["frame_width"],
                           "h": src["frame_height"], "mirror_x": mirror_x,
                           "canvas_w": grp["width"], "canvas_h": grp["height"],
                           "source_frame_id": frame_id, "image_id": image_id})
        image_rows.append({"image_id": image_id, "frame_first": first,
                           "frame_count": len(frames)-first, "canvas_w": grp["width"],
                           "canvas_h": grp["height"], "directional": int(bool(meta.get("directional"))),
                           "modifier": modifier, "color_shift": int(meta.get("color_shift", 0)),
                           "lut_index": next((i for i,l in enumerate(luts) if (l["modifier"],l["shift"]) == (modifier,int(meta.get("color_shift",0)))), 0xFF),
                           "flags": 0})
    # Preserve all observed script events in compact records. Opcode and raw
    # operands are kept; the runtime decides which event kinds it implements.
    events, overlays, animations = [], [], []
    for image_key, anim_map in sorted(report["source_animations"].items(), key=lambda pair:int(pair[0])):
        image_id = int(image_key)
        for anim_name, anim in anim_map.items():
            if anim_name not in ANIMS or anim.get("status") != "ok":
                continue
            first = len(events); ovfirst = len(overlays)
            visual_refs = list(anim.get("visual_references", []))
            trigger_tick = next((int(e["source_tick"]) for e in anim.get("events", [])
                                 if int(e.get("opcode", -1)) in (28, 37, 38)), 0)
            for event in anim.get("events", []):
                args = event.get("args", [])
                arg0 = _first_int(args[0]) if args else 0
                arg1 = _first_int(args[1]) if len(args)>1 else 0
                arg2 = _first_int(args[2]) if len(args)>2 else 0
                events.append({"image_id":image_id,"source_tick":int(event["source_tick"]),"pc":int(event["pc"]),
                               "source_frame":arg0 if event["name"]=="playframe" else 0xFFFF,
                               "opcode":int(event["opcode"]),"flags":0})
                if int(event["opcode"]) in (8,9,10,11,13,14,15,16,17,19,20,21):
                    opcode = int(event["opcode"])
                    ref_index = next((i for i,x in enumerate(visual_refs)
                                      if x.get("source_tick")==event.get("source_tick") and x.get("opcode")==event.get("opcode") and x.get("args")==args), None)
                    child = visual_refs.pop(ref_index) if ref_index is not None else {}
                    overlays.append({"parent_image":image_id,"image_id":int(child.get("image_id",0xFFFF)),
                                     "source_tick":int(event["source_tick"]),"relative_tick":int(event["source_tick"])-trigger_tick,
                                     "x":0 if opcode in (13,14,21) else (arg1 if len(args)>1 else 0),
                                     "y":0 if opcode in (13,14,21) else (arg2 if len(args)>2 else 0),
                                     "lo_index":arg1 if opcode in (13,14,21) and len(args)>1 else 0xFF,
                                     "aux":arg2 if opcode in (13,14) and len(args)>2 else 0,
                                     "opcode":opcode,
                                     "flags":0,"sprite_id":int(child.get("sprite_id") if child.get("sprite_id") is not None else 0xFFFF)})
            loop_pc = anim.get("loop_pc")
            loop_source_tick = next((e["source_tick"] for e in events[first:] if e["pc"] == loop_pc), 0)
            animations.append({"image_id":image_id,"animation_id":ANIMS[anim_name],"event_first":first,
                               "event_count":len(events)-first,"overlay_first":ovfirst,
                               "overlay_count":len(overlays)-ovfirst,
                               "duration_ticks":int(anim.get("source_ticks_before_end_or_loop",0)),
                               "loop_pc":int(loop_pc if loop_pc is not None else 0xFFFFFFFF),
                               "loop_source_tick":loop_source_tick,"flags":0})
    animations = _sorted_animation_rows(animations)
    profiles = _profile_rows(report, animations, events, overlays)
    out.mkdir(parents=True, exist_ok=True)
    lines = ["/* Generated source-derived art metadata; keep private. */",
             "#ifndef SC64_WEAPON_SOURCE_GENERATED_H", "#define SC64_WEAPON_SOURCE_GENERATED_H",
             "#include <stdint.h>", "#define SC64_WEAPON_SOURCE_BANK_VERSION 1",
             "#define SC64_WEAPON_SOURCE_METADATA 1",
             "#define SC64_WEAPON_SOURCE_SPROLUSELO_LO 1",
             "#define SC64_WEAPON_SOURCE_PROFILE_COUNT " + str(len(profiles)),
             "#define SC64_WEAPON_SOURCE_IMAGE_COUNT " + str(len(image_rows)),
             "#define SC64_WEAPON_SOURCE_FRAME_COUNT " + str(len(frames)),
             "#define SC64_WEAPON_SOURCE_ANIMATION_COUNT " + str(len(animations)),
             "#define SC64_WEAPON_SOURCE_EVENT_COUNT " + str(len(events)),
             "#define SC64_WEAPON_SOURCE_OVERLAY_COUNT " + str(len(overlays)),
             "#define SC64_WEAPON_SOURCE_LUT_COUNT " + str(len(luts)),
             "#define SC64_WEAPON_SOURCE_LO_TABLE_COUNT " + str(len(lo_tables)),
             "#define SC64_WEAPON_SOURCE_LO_OFFSET_COUNT " + str(len(lo_offsets)),
             "#define SC64_WEAPON_SOURCE_BULLET_FLY 0",
             "#define SC64_WEAPON_SOURCE_BULLET_FOLLOW 1",
             "#define SC64_WEAPON_SOURCE_BULLET_APPEAR_TARGET_UNIT 2",
             "#define SC64_WEAPON_SOURCE_BULLET_PERSIST_TARGET_POS 3",
             "#define SC64_WEAPON_SOURCE_BULLET_APPEAR_TARGET_POS 4",
             "#define SC64_WEAPON_SOURCE_BULLET_APPEAR_SOURCE 5",
             "#define SC64_WEAPON_SOURCE_BULLET_SELF_DESTRUCT 6",
             "#define SC64_WEAPON_SOURCE_BULLET_BOUNCE 7",
             "#define SC64_WEAPON_SOURCE_BULLET_ATTACK_TARGET_POS 8",
             "#define SC64_WEAPON_SOURCE_BULLET_EXTEND_MAX_RANGE 9",
             "#define SC64_WEAPON_SOURCE_PROFILE_HAS_PROJECTILE_IMAGE 1u",
             "typedef struct { uint8_t actor_raw, weapon_owner_raw, weapon_id, mode, variant, attack_anim, projectile_ticks_per_frame, flags, weapon_bullet_type, weapon_count; uint16_t attackwith_tick, attack_image, projectile_image, projectile_flingy, projectile_lifetime, projectile_first_frame, projectile_frame_count, impact_source_tick, attack_event_first, attack_event_count, projectile_event_first, projectile_event_count, overlay_first, overlay_count; int32_t speed_raw, halt_raw; int16_t accel_raw; int8_t forward_offset, upward_offset; } Sc64WeaponSourceAttackProfile;",
             "typedef struct { uint16_t parent_image,image_id; uint32_t source_tick; int32_t relative_tick; int16_t x,y; uint8_t lo_index,aux,opcode,flags; uint16_t sprite_id; } Sc64WeaponSourceOverlay;",
             "typedef struct { uint16_t image_id; uint32_t source_tick,pc; uint16_t source_frame; uint8_t opcode,flags; } Sc64WeaponSourceEvent;",
             "typedef struct { uint16_t image_id; uint8_t animation_id,flags; uint16_t event_first,event_count,overlay_first,overlay_count; uint32_t duration_ticks,loop_pc,loop_source_tick; } Sc64WeaponSourceAnimation;",
             "typedef struct { uint16_t image_id,frame_first,frame_count,canvas_w,canvas_h,source_frame_count,frame_directory_first; uint8_t directional,modifier,color_shift,lut_index,flags; } Sc64WeaponSourceImage;",
             "typedef struct { uint32_t pixel_offset,mirror_pixel_offset; int16_t x,y,mirror_x; uint16_t w,h,canvas_w,canvas_h,source_frame_id,image_id; } Sc64WeaponSourceFrame;",
             "typedef struct { uint32_t data_offset; uint16_t rows; uint8_t modifier,color_shift; } Sc64WeaponSourceLut;",
             "typedef struct { uint16_t image_id,frame_count,offset_count; uint8_t lo_kind,flags; uint32_t offset_first; } Sc64WeaponSourceLoTable;",
             "typedef struct { int8_t x,y; } Sc64WeaponSourceLoOffset;"]
    def struct_rows(name, ctype, rows, fields):
        lines.append(f"static const {ctype} {name}[{len(rows) or 1}] = {{")
        if not rows: lines.append("    {0},")
        for row in rows:
            lines.append("    {" + ", ".join(str(row.get(f, 0)) for f in fields) + "},")
        lines.append("};")
    struct_rows("sc64_weapon_source_profiles", "Sc64WeaponSourceAttackProfile", profiles,
        ["actor_raw","weapon_owner_raw","weapon_id","mode","variant","attack_anim","projectile_ticks_per_frame","flags","weapon_bullet_type","weapon_count","attackwith_tick","attack_image","projectile_image","projectile_flingy","projectile_lifetime","projectile_first_frame","projectile_frame_count","impact_source_tick","attack_event_first","attack_event_count","projectile_event_first","projectile_event_count","overlay_first","overlay_count","speed_raw","halt_raw","accel_raw","forward_offset","upward_offset"])
    bound_fallbacks = {}
    frame_directory = []
    for image in image_rows:
        image["source_frame_count"] = next(int(meta.get("frame_count",0)) for key,meta in images.items() if int(key)==image["image_id"])
        image["frame_directory_first"] = len(frame_directory)
        by_source = {f["source_frame_id"]: i for i,f in enumerate(frames) if f["image_id"] == image["image_id"]}
        frame_directory.extend(by_source.get(i,0xFFFF) for i in range(image["source_frame_count"]))
    bounds = _profile_cull_bounds(profiles, image_rows, frames, animations, overlays, lo_tables, lo_offsets, events, frame_directory, bound_fallbacks)
    lines.append("#define SC64_WEAPON_SOURCE_PROFILE_BOUNDS_VERSION 1")
    lines.extend(_c_array("uint32_t", "sc64_weapon_source_profile_bounds", [str(v) + "u" for v in bounds], 8))
    struct_rows("sc64_weapon_source_overlays", "Sc64WeaponSourceOverlay", overlays,
        ["parent_image","image_id","source_tick","relative_tick","x","y","lo_index","aux","opcode","flags","sprite_id"])
    struct_rows("sc64_weapon_source_events", "Sc64WeaponSourceEvent", events,
        ["image_id","source_tick","pc","source_frame","opcode","flags"])
    struct_rows("sc64_weapon_source_animations", "Sc64WeaponSourceAnimation", animations,
        ["image_id","animation_id","flags","event_first","event_count","overlay_first","overlay_count","duration_ticks","loop_pc","loop_source_tick"])
    struct_rows("sc64_weapon_source_images", "Sc64WeaponSourceImage", image_rows,
        ["image_id","frame_first","frame_count","canvas_w","canvas_h","source_frame_count","frame_directory_first","directional","modifier","color_shift","lut_index","flags"])
    lines.extend(_c_array("uint16_t", "sc64_weapon_source_frame_directory", [str(v) for v in frame_directory], 12))
    struct_rows("sc64_weapon_source_frames", "Sc64WeaponSourceFrame", frames,
        ["pixel_offset","mirror_pixel_offset","x","y","mirror_x","w","h","canvas_w","canvas_h","source_frame_id","image_id"])
    struct_rows("sc64_weapon_source_luts", "Sc64WeaponSourceLut", luts,
        ["offset","rows","modifier","shift"])
    struct_rows("sc64_weapon_source_lo_tables", "Sc64WeaponSourceLoTable", lo_tables,
        ["image_id","frame_count","offset_count","lo_kind","flags","offset_first"])
    struct_rows("sc64_weapon_source_lo_offsets", "Sc64WeaponSourceLoOffset", lo_offsets,
        ["x","y"])
    lines.append("static const uint8_t sc64_weapon_source_pixels[%d] = {" % max(1,len(pixel_data)))
    for i in range(0,len(pixel_data),16): lines.append("    "+", ".join(str(x) for x in pixel_data[i:i+16])+",")
    if not pixel_data: lines.append("    0,")
    lines.append("};")
    lines.append("static const uint8_t sc64_weapon_source_lut_bytes[%d] = {" % max(1,len(lutbytes)))
    for i in range(0,len(lutbytes),16): lines.append("    "+", ".join(str(x) for x in lutbytes[i:i+16])+",")
    if not lutbytes: lines.append("    0,")
    lines.append("};")
    lines += _lookup_accessor_lines()
    lines.append("#endif")
    header = out / "sc64_weapon_source_generated.h"
    header.write_text("\n".join(lines)+"\n", encoding="ascii")
    manifest = {"version":1,"rom_sha1":ROM_SHA1,"header_sha256":hashlib.sha256(header.read_bytes()).hexdigest(),
                "profiles":len(profiles),"images":len(image_rows),"frames":len(frames),"animations":len(animations),
                "profile_bounds_fallbacks":bound_fallbacks,
                "raster_images":len(raster_images),
                "events":len(events),"overlays":len(overlays),"pixel_bytes":len(pixel_data),"lut_bytes":len(lutbytes),
                "unique_pixel_blobs":len(pixel_blob_offsets),
                "pixel_bytes_before_dedup":unshared_pixel_bytes,
                "generator_sha256":hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                "source_table_sha256":report["reference"].get("table_sha256",{}),
                "source_grp_sha1":{key:value["grp_sha1"] for key,value in images.items() if value.get("grp_sha1")},
                "lo_tables":len(lo_tables),"lo_offsets":len(lo_offsets),"lo_table_sha256":lo_hashes,
                "palette_remap_sha256":hashlib.sha256(remap).hexdigest(),
                "palette_json_sha256":hashlib.sha256(json.dumps(palette_json,sort_keys=True,separators=(",",":")).encode()).hexdigest(),
                "modifier_lut_binding":MODIFIER_TABLE_BINDING,"modifier_table_sha256":table_hashes,
                "unresolved": ["Bunker raw125 armament is occupant-driven; no synthetic weapon profile emitted.",
                               "native projectile movement and damage timing are retained as source metadata, not claimed as emulated runtime behavior."]}
    (out/"manifest.json").write_text(json.dumps(manifest,indent=2)+"\n")
    return manifest


def generate(rom_path: Path = DEFAULT_ROM, out_dir: Path = DEFAULT_OUT,
             remap_path: Path = DEFAULT_REMAP, palette_path: Path = DEFAULT_PALETTE) -> dict:
    work = out_dir / "audit"
    report, remap, palette, rom = _load_source(rom_path,remap_path,palette_path,work)
    get, sprites = _source_bolt(rom)
    return _emit_header(out_dir,report,remap,palette,get,sprites)


def main(argv: list[str] | None = None) -> int:
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rom",type=Path,default=DEFAULT_ROM)
    parser.add_argument("--out",type=Path,default=DEFAULT_OUT)
    parser.add_argument("--palette-remap",type=Path,default=DEFAULT_REMAP)
    parser.add_argument("--palette-json",type=Path,default=DEFAULT_PALETTE)
    args=parser.parse_args(argv)
    print(json.dumps(generate(args.rom,args.out,args.palette_remap,args.palette_json),indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
