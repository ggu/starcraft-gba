#!/usr/bin/env python3
"""Audit authenticated N64 source weapon/attack visual chains privately.

The report preserves the source DAT links and raw IScript event streams. It is
an inventory, not a renderer or a claim that every runtime attack path is
already reproduced.
"""
# SPDX-License-Identifier: GPL-3.0-or-later
from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
import re
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ROM_SHA1 = "a36d91b5aac7bb95df0d7de5f9814f5270e25c2b"
UNIT_COUNT, FLINGY_COUNT, SPRITE_COUNT, IMAGE_COUNT, WEAPON_COUNT = 228, 209, 517, 999, 130
ATTACK_ANIMS = ("GndAttkInit", "AirAttkInit", "GndAttkRpt", "AirAttkRpt",
                "GndAttkToIdle", "AirAttkToIdle")
# Ordinals match OpenBW iscript_opcodes at 4b046d5f65302b10cb0a745f0fecd37ec85b20a8.
# `playframe` is the
# established report spelling of OpenBW's `playfram`.
OP_NAMES = {i: name for i, name in enumerate((
    'playframe',
    'playframtile',
    'sethorpos',
    'setvertpos',
    'setpos',
    'wait',
    'waitrand',
    'goto',
    'imgol',
    'imgul',
    'imgolorig',
    'switchul',
    'opcode_0c',
    'imgoluselo',
    'imguluselo',
    'sprol',
    'highsprol',
    'lowsprul',
    'uflunstable',
    'spruluselo',
    'sprul',
    'sproluselo',
    'end',
    'setflipstate',
    'playsnd',
    'playsndrand',
    'playsndbtwn',
    'domissiledmg',
    'attackmelee',
    'followmaingraphic',
    'randcondjmp',
    'turnccwise',
    'turncwise',
    'turn1cwise',
    'turnrand',
    'setspawnframe',
    'sigorder',
    'attackwith',
    'attack',
    'castspell',
    'useweapon',
    'move',
    'gotorepeatattk',
    'engframe',
    'engset',
    'opcode_2d',
    'nobrkcodestart',
    'nobrkcodeend',
    'ignorerest',
    'attkshiftproj',
    'tmprmgraphicstart',
    'tmprmgraphicend',
    'setfldirect',
    'call',
    'return',
    'setflspeed',
    'creategasoverlays',
    'pwrupcondjmp',
    'trgtrangecondjmp',
    'trgtarccondjmp',
    'curdirectcondjmp',
    'imgulnextid',
    'opcode_3e',
    'liftoffcondjmp',
    'warpoverlay',
    'orderdone',
    'grdsprol',
    'opcode_43',
    'dogrddamage',
))}


def trace_iscript(data, pc, decode_instruction, limit=256):
    """Read one bounded path; stop at calls/returns without inventing a branch."""
    visited = set()
    events = []
    tick = 0
    loop_to = None
    for _ in range(limit):
        if pc in visited:
            loop_to = pc
            break
        visited.add(pc)
        op, args, nxt, jump = decode_instruction(data, pc)
        events.append({"source_tick": tick, "opcode": op,
                       "name": OP_NAMES.get(op, "opcode_" + str(op)),
                       "args": args, "pc": pc})
        if op == 5 and args:
            tick += args[0]
        if op in (22, 53, 54):
            break
        pc = jump if op == 7 and jump is not None else nxt
    else:
        return events, tick, loop_to, False
    return events, tick, loop_to, True


# Special runtime emission paths that do not appear as normal actor cases in
# sc64_weapon_vfx_profiles.h. The Spider Mine is source raw unit 13 and owns
# ground weapon 6 in units.dat; the mine, rather than its Vulture launcher, is
# the weapon-emitting actor when it detonates. Keep this explicit list narrow
# and source-backed rather than inferring profiles for every DAT unit.
SPECIAL_EMISSION_ACTORS = {13: "Spider Mine (detonation emitter)"}


def add_special_emission_actors(actors_by_raw: dict[int, list[str]]) -> dict[int, str]:
    """Add authenticated special emitters without pretending they are profiles."""
    emission_only = {}
    for raw, label in SPECIAL_EMISSION_ACTORS.items():
        if raw not in actors_by_raw:
            actors_by_raw[raw] = []
            emission_only[raw] = label
        if label not in actors_by_raw[raw]:
            actors_by_raw[raw].append(label)
    return emission_only


def load_module(name: str, path: Path):
    spec = importlib.util.spec_from_file_location(name, path)
    if not spec or not spec.loader:
        raise ValueError(f"cannot load {path}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[name] = module
    spec.loader.exec_module(module)
    return module


def resolve_profile_cases(profile_path: Path, rules_path: Path) -> dict:
    profile = profile_path.read_text()
    rules = rules_path.read_text()
    unit_names = set(re.findall(r"\bcase\s+(U_[A-Z0-9_]+)\s*:", profile.split("sc64_weapon_vfx_building_profile")[0]))
    building_names = set(re.findall(r"\bcase\s+(B_[A-Z0-9_]+)\s*:", profile.split("sc64_weapon_vfx_building_profile")[1]))
    def map_cases(func: str, names: set[str]):
        definition = re.search(r"static inline int sc64_race_raw_" + func + r"\(int type\)\s*\{", rules)
        if not definition:
            raise ValueError(f"missing raw {func} mapping in {rules_path}")
        brace = rules.find("{", definition.start())
        end = rules.find("\n}", brace)
        if brace < 0 or end < 0:
            raise ValueError(f"malformed raw {func} mapping in {rules_path}")
        body = rules[brace:end]
        out = {}
        for name in names:
            match = re.search(r"\bcase\s+" + re.escape(name) + r"\s*:\s*return\s+([^;]+);", body)
            if not match:
                out[name] = None
                continue
            value = match.group(1).strip()
            try:
                out[name] = int(value, 0)
            except ValueError:
                enum_value = None
                for header in (ROOT / "starcraft").glob("*.h"):
                    found = re.search(r"\b" + re.escape(value) + r"\s*=\s*(\d+)\s*,", header.read_text())
                    if found:
                        enum_value = int(found.group(1))
                        break
                # Keep a symbolic value if no simple source enum resolves it.
                out[name] = enum_value if enum_value is not None else value
        return out
    return {"units": map_cases("unit", unit_names), "buildings": map_cases("building", building_names)}


def read_column(data: bytes, count: int, fmt: str, off: int, name: str):
    size = struct.calcsize("<" + fmt)
    if off + count * size > len(data):
        raise ValueError(f"{name} column extends beyond DAT ({off}+{count*size}>{len(data)})")
    return [struct.unpack_from("<" + fmt, data, off + i * size)[0] for i in range(count)]


def grp_path(gfi: int) -> str | None:
    if gfi == 0:
        return None
    flat = gfi - 1
    directory = 0x00D + flat // 256
    if directory > 0x011:
        return None
    return f"{directory:03X}/{flat % 256:03X}"


def generate(rom_path: Path, out_path: Path) -> dict:
    bolt_path = ROOT / "references/sc64-maps/extract_sc64_maps.py"
    if not bolt_path.is_file():
        raise ValueError(f"pinned BOLT extractor missing: {bolt_path}")
    rom = rom_path.read_bytes()
    digest = hashlib.sha1(rom).hexdigest()
    if digest != ROM_SHA1:
        raise ValueError(f"unexpected ROM SHA1 {digest}; expected authenticated USA image")
    extractor = load_module("sc64_weapon_source_bolt", bolt_path)
    sprites = load_module("sc64_weapon_source_sprites", ROOT / "tools/sc64_sprites.py")
    construction = load_module("sc64_weapon_source_construction", ROOT / "tools/sc64_construction_sprites.py")
    archive = extractor.BoltArchive(rom)
    entries = {e.path: e for e in archive.entries()}
    def get(path: str) -> bytes:
        if path not in entries:
            raise ValueError(f"BOLT record missing: {path}")
        return archive.read(entries[path])

    units, weapons = get("00C/001"), get("00C/007")
    flingy, spritedat, images, scriptdata = get("00C/002"), get("00C/003"), get("00C/000"), get("012/002")
    unit_flingy = read_column(units, UNIT_COUNT, "B", 0, "units.flingy")
    unit_turret = read_column(units, UNIT_COUNT, "H", UNIT_COUNT, "units.turret")
    unit_subunit = read_column(units, UNIT_COUNT, "H", UNIT_COUNT + 2*UNIT_COUNT, "units.subunit")
    unit_ground_weapon = read_column(units, UNIT_COUNT, "B", 5892, "units.ground_weapon")
    unit_air_weapon = read_column(units, UNIT_COUNT, "B", 6348, "units.air_weapon")
    weapon_flingy = read_column(weapons, WEAPON_COUNT, "I", 2*WEAPON_COUNT, "weapons.flingy")
    weapon_bullet = read_column(weapons, WEAPON_COUNT, "B", 2*WEAPON_COUNT+4*WEAPON_COUNT+1, "weapons.bullet")
    # All weapons.dat fields used in the source table contract.
    weapon_specs = [
        ("label", "H"), ("flingy", "I"), ("unused", "B"), ("target", "H"),
        ("min_range", "I"), ("max_range", "I"), ("upgrade", "B"),
        ("damage_type", "B"), ("bullet_type", "B"), ("lifetime", "B"),
        ("hit_type", "B"), ("inner", "H"), ("medium", "H"), ("outer", "H"),
        ("damage", "H"), ("bonus", "H"), ("cooldown", "B"), ("count", "B"),
        ("angle", "B"), ("head_offset", "b"), ("forward", "b"), ("upward", "b"),
        ("error", "H"), ("icon", "H"),
    ]
    wo = {}; cursor = 0
    for name, fmt in weapon_specs:
        wo[name] = (cursor, fmt); cursor += WEAPON_COUNT * struct.calcsize("<" + fmt)
    if len(weapons) < cursor:
        raise ValueError("weapons.dat is shorter than parsed field layout")
    def wv(name, row):
        off, fmt = wo[name]
        return struct.unpack_from("<" + fmt, weapons, off + row * struct.calcsize("<" + fmt))[0]

    flingy_sprite = read_column(flingy, FLINGY_COUNT, "H", 0, "flingy.sprite")
    flingy_speed = read_column(flingy, FLINGY_COUNT, "i", 2*FLINGY_COUNT, "flingy.top_speed")
    flingy_accel = read_column(flingy, FLINGY_COUNT, "h", 2*FLINGY_COUNT+4*FLINGY_COUNT, "flingy.acceleration")
    flingy_halt = read_column(flingy, FLINGY_COUNT, "i", 2*FLINGY_COUNT+4*FLINGY_COUNT+2*FLINGY_COUNT, "flingy.halt_distance")
    flingy_turn = read_column(flingy, FLINGY_COUNT, "B", 2*FLINGY_COUNT+4*FLINGY_COUNT+2*FLINGY_COUNT+4*FLINGY_COUNT, "flingy.turn_radius")
    sprite_image = read_column(spritedat, SPRITE_COUNT, "H", 0, "sprites.image")
    image_grp = read_column(images, IMAGE_COUNT, "I", 0, "images.grp")
    image_fields = [("directional", "B"), ("clickable", "B"), ("anim", "B"),
                    ("always", "B"), ("modifier", "B"), ("shift", "B")]
    image_col = {}; pos = IMAGE_COUNT * 4
    for name, fmt in image_fields:
        image_col[name] = read_column(images, IMAGE_COUNT, fmt, pos, "images." + name)
        pos += IMAGE_COUNT * struct.calcsize("<" + fmt)
    image_script = read_column(images, IMAGE_COUNT, "I", pos, "images.iscript")
    scripts = construction.script_index(scriptdata)

    def chain_flingy(fid: int) -> dict:
        if not 0 <= fid < FLINGY_COUNT:
            return {"flingy": fid, "error": "out_of_range"}
        sid = flingy_sprite[fid]
        image = sprite_image[sid] if sid < SPRITE_COUNT else -1
        return {"flingy": fid, "sprite": sid, "image": image,
                "grp_filename_index": image_grp[image] if 0 <= image < IMAGE_COUNT else 0,
                "grp": grp_path(image_grp[image]) if 0 <= image < IMAGE_COUNT else None,
                "flingy_motion": {"top_speed_raw": flingy_speed[fid], "acceleration_raw": flingy_accel[fid],
                                  "halt_distance_raw": flingy_halt[fid], "turn_radius_raw": flingy_turn[fid]}}

    def script_for(image: int, anim: str):
        if not 0 <= image < IMAGE_COUNT:
            return {"status": "invalid_image", "image": image}
        sid = image_script[image]
        start = scripts.get(sid)
        if start is None:
            return {"status": "missing_iscript", "image": image, "iscript_id": sid}
        starts = construction.image_animation_starts(scriptdata, start)
        pc = starts.get(anim)
        if pc is None:
            return {"status": "no_animation", "image": image, "iscript_id": sid, "animation": anim}
        events, tick, loop_to, complete = trace_iscript(
            scriptdata, pc, construction.decode_instruction)
        if not complete:
            return {"status": "instruction_limit", "image": image, "iscript_id": sid,
                    "animation": anim, "events": events}
        visual_references = []
        for event in events:
            op, args = event["opcode"], event["args"]
            if not args:
                continue
            if op in (8, 9, 10, 11, 13, 14):
                child_image, child_sprite, kind = args[0], None, "image_overlay"
            elif op in (15, 16, 17, 19, 20, 21):
                child_sprite = args[0]
                child_image = sprite_image[child_sprite] if 0 <= child_sprite < SPRITE_COUNT else -1
                kind = "sprite_overlay"
            else:
                continue
            child = {"source_tick": event["source_tick"], "kind": kind, "opcode": op,
                     "args": args, "sprite_id": child_sprite, "image_id": child_image,
                     "grp": grp_path(image_grp[child_image]) if 0 <= child_image < IMAGE_COUNT else None}
            if 0 <= child_image < IMAGE_COUNT:
                child.update({"directional": bool(image_col["directional"][child_image]),
                              "modifier": image_col["modifier"][child_image],
                              "color_shift": image_col["shift"][child_image]})
            visual_references.append(child)
        branch_ops = [event for event in events if event["opcode"] in (30, 57, 42, 53)]
        return {"status": "ok", "image": image, "iscript_id": sid, "animation": anim,
                "events": events, "source_ticks_before_end_or_loop": tick,
                "loop_pc": loop_to, "visual_references": visual_references,
                "control_flow_notes": (["branch/repeat semantics retained as raw events; calls stop this bounded path"]
                                        if branch_ops else []),
                "directional": bool(image_col["directional"][image]),
                "modifier": image_col["modifier"][image], "color_shift": image_col["shift"][image],
                "grp": grp_path(image_grp[image])}

    case_map = resolve_profile_cases(ROOT / "starcraft/sc64_weapon_vfx_profiles.h", ROOT / "starcraft/race_rules.h")
    actor_raws = {name: raw for name, raw in case_map["units"].items() if isinstance(raw, int)}
    building_raws = {name: raw for name, raw in case_map["buildings"].items() if isinstance(raw, int)}
    # A few hero aliases intentionally share raw source units. Preserve both names.
    actors_by_raw = {}
    for name, raw in actor_raws.items(): actors_by_raw.setdefault(raw, []).append(name)
    emission_only_actors = add_special_emission_actors(actors_by_raw)
    buildings_by_raw = {}
    for name, raw in building_raws.items(): buildings_by_raw.setdefault(raw, []).append(name)

    sources = []
    for raw, names in sorted(actors_by_raw.items()):
        if raw >= UNIT_COUNT: continue
        primary_flingy = unit_flingy[raw]
        turret_raw = unit_turret[raw]
        actor_imgs = []
        for role, source_raw in (("body", raw), ("turret", turret_raw)):
            if role == "turret" and source_raw == 0xFFFF: continue
            if not 0 <= source_raw < UNIT_COUNT: continue
            f = unit_flingy[source_raw]
            im = chain_flingy(f)["image"]
            actor_imgs.append({"role": role, "raw_unit": source_raw, "flingy": f, "image": im,
                               "grp": grp_path(image_grp[im]) if 0 <= im < IMAGE_COUNT else None,
                               "directional": bool(image_col["directional"][im]) if 0 <= im < IMAGE_COUNT else False,
                               "modifier": image_col["modifier"][im] if 0 <= im < IMAGE_COUNT else None,
                               "color_shift": image_col["shift"][im] if 0 <= im < IMAGE_COUNT else None,
                               "attack_animations": [script_for(im, a) for a in ATTACK_ANIMS + ("Init", "Death")],
                               "initial_animation": script_for(im, "Init")})
        weapon_rows = []
        weapon_owners = [("body", raw)]
        if 0 <= turret_raw < UNIT_COUNT and turret_raw not in (0, raw):
            weapon_owners.append(("turret", turret_raw))
        subunit_raw = unit_subunit[raw]
        if 0 <= subunit_raw < UNIT_COUNT and subunit_raw not in (0, raw, turret_raw):
            weapon_owners.append(("subunit", subunit_raw))
        for mode, column in (("ground", unit_ground_weapon), ("air", unit_air_weapon)):
            owner_role, owner_unit_raw, wid = next(((role, candidate, column[candidate])
                for role, candidate in weapon_owners if column[candidate] < WEAPON_COUNT), (None, None, 130))
            if wid >= WEAPON_COUNT: continue
            graphic_flingy = weapon_flingy[wid]
            chain = chain_flingy(graphic_flingy) if 0 < graphic_flingy < FLINGY_COUNT else None
            wi = chain["image"] if chain else -1
            source_funcs = [script_for(wi, "Init"), script_for(wi, "Death"),
                            script_for(wi, "GndAttkInit"), script_for(wi, "AirAttkInit")]
            weapon_rows.append({"mode": mode, "weapon_id": wid, "weapon_owner_role": owner_role,
                "weapon_owner_raw": owner_unit_raw,
                "weapon_fields": {name: wv(name, wid) for name, _ in weapon_specs},
                "weapon_graphic_status": "present" if chain else "weapons.dat flingy is zero; no projectile graphic chain",
                "weapon_graphic_chain": chain, "weapon_image_metadata": (
                    {k: image_col[k][wi] for k in image_col} if 0 <= wi < IMAGE_COUNT else None),
                "projectile_animations": source_funcs})
        row = {"raw_unit": raw, "profile_names": sorted(names), "raw_turret_unit": turret_raw,
               "raw_subunit": subunit_raw, "actor_images": actor_imgs,
               "ground_weapon_id": unit_ground_weapon[raw], "air_weapon_id": unit_air_weapon[raw],
               "weapons": weapon_rows}
        if raw in emission_only_actors:
            row["emission_only"] = True
            row["emission_reason"] = emission_only_actors[raw]
        # A Reaver launches a Scarab actor rather than having a direct weapon
        # row. Keep the separate source chain visible instead of assigning the
        # Scarab weapon directly to the Reaver.
        if raw == 83 and 85 < UNIT_COUNT:
            scarab_weapon = unit_ground_weapon[85]
            row["launched_unit_visual"] = {"raw_unit": 85,
                "actor_image": chain_flingy(unit_flingy[85])["image"],
                "ground_weapon_id": scarab_weapon,
                "weapon_fields": {name: wv(name, scarab_weapon) for name, _ in weapon_specs},
                "weapon_graphic_chain": chain_flingy(weapon_flingy[scarab_weapon])
                    if scarab_weapon < WEAPON_COUNT and 0 < weapon_flingy[scarab_weapon] < FLINGY_COUNT else None}
            row["note"] = "Reaver attack is represented by launched Scarab raw unit 85; its source weapons.dat ground weapon is separate from the Reaver raw83 row."
        sources.append(row)

    defenses = []
    for raw, names in sorted(buildings_by_raw.items()):
        if raw >= UNIT_COUNT: continue
        f = unit_flingy[raw]; im = chain_flingy(f)["image"]
        turret_raw = unit_turret[raw]
        image_roles = [("body", raw)]
        if 0 <= turret_raw < UNIT_COUNT and turret_raw not in (0, raw):
            image_roles.append(("turret", turret_raw))
        attack_images = []
        for role, source_raw in image_roles:
            actor_image = chain_flingy(unit_flingy[source_raw])["image"]
            attack_images.append({"role": role, "raw_unit": source_raw, "image": actor_image,
                "grp": grp_path(image_grp[actor_image]) if 0 <= actor_image < IMAGE_COUNT else None,
                "directional": bool(image_col["directional"][actor_image]) if 0 <= actor_image < IMAGE_COUNT else False,
                "modifier": image_col["modifier"][actor_image] if 0 <= actor_image < IMAGE_COUNT else None,
                "color_shift": image_col["shift"][actor_image] if 0 <= actor_image < IMAGE_COUNT else None,
                "attack_animations": [script_for(actor_image, anim) for anim in ATTACK_ANIMS + ("Init", "Death")],
                "initial_animation": script_for(actor_image, "Init")})
        defenses.append({"raw_building": raw, "profile_names": sorted(names), "unit_flingy": f,
                         "actor_image": im, "actor_grp": grp_path(image_grp[im]) if 0 <= im < IMAGE_COUNT else None,
                         "turret_raw": turret_raw, "attack_images": attack_images,
                         "ground_weapon_id": unit_ground_weapon[raw], "air_weapon_id": unit_air_weapon[raw],
                         "weapons": []})
        row = defenses[-1]
        for mode, wid in (("ground", unit_ground_weapon[raw]), ("air", unit_air_weapon[raw])):
            if wid >= WEAPON_COUNT: continue
            graphic_flingy = weapon_flingy[wid]
            chain = chain_flingy(graphic_flingy) if 0 < graphic_flingy < FLINGY_COUNT else None
            wi = chain["image"] if chain else -1
            row["weapons"].append({"mode": mode, "weapon_id": wid, "weapon_owner_raw": raw,
                "weapon_fields": {name: wv(name, wid) for name, _ in weapon_specs},
                "weapon_graphic_status": "present" if chain else "weapons.dat flingy is zero; no projectile graphic chain",
                "weapon_graphic_chain": chain,
                "projectile_animations": [script_for(wi, anim) for anim in ("Init", "Death", "GndAttkInit", "AirAttkInit")]})

    image_ids = set()
    def collect_images(value):
        if isinstance(value, dict):
            for key, item in value.items():
                if key in ("image", "image_id", "actor_image") and isinstance(item, int) and 0 <= item < IMAGE_COUNT:
                    image_ids.add(item)
                collect_images(item)
        elif isinstance(value, list):
            for item in value: collect_images(item)
    collect_images(sources); collect_images(defenses)
    # Include children started by each referenced image's own Init/Death and
    # attack streams (for example animated engine or halo attachments).
    animation_names = ("Init", "Death") + ATTACK_ANIMS
    source_animations = {}
    pending = set(image_ids)
    while pending:
        image = min(pending); pending.remove(image)
        if str(image) in source_animations:
            continue
        animations = {anim: script_for(image, anim) for anim in animation_names}
        source_animations[str(image)] = animations
        for animation in animations.values():
            for child in animation.get("visual_references", []):
                child_image = child.get("image_id", -1)
                if 0 <= child_image < IMAGE_COUNT and child_image not in image_ids:
                    image_ids.add(child_image)
                    pending.add(child_image)
    image_audit = {}
    for image in sorted(image_ids):
        path = grp_path(image_grp[image])
        record = {"image_id": image, "grp_filename_index": image_grp[image], "grp": path,
                  "directional": bool(image_col["directional"][image]),
                  "modifier": image_col["modifier"][image], "color_shift": image_col["shift"][image],
                  "iscript_id": image_script[image]}
        if path:
            try:
                payload = get(path)
                grp = sprites.parse_grp(payload)
                record.update({"grp_sha1": hashlib.sha1(payload).hexdigest(),
                    "canvas": [grp["width"], grp["height"]], "frame_count": len(grp["frames"]),
                    "frame_rectangles": [[f["x"], f["y"], f["frame_width"], f["frame_height"]]
                                         for f in grp["frames"]]})
            except ValueError as error:
                record["grp_error"] = str(error)
        image_audit[str(image)] = record

    report = {"format_version": 1, "purpose": "source reference inventory; not fidelity proof",
        "reference": {"rom_sha1": digest, "rom_path_name": rom_path.name,
            "openbw_revision": "4b046d5f65302b10cb0a745f0fecd37ec85b20a8",
            "bolt_extractor": str(bolt_path.relative_to(ROOT)),
            "table_sha256": {p: hashlib.sha256(get(p)).hexdigest() for p in ("00C/000", "00C/001", "00C/002", "00C/003", "00C/007", "012/002")}},
        "profile_mapping": case_map,
        "special_emission_actors": {str(raw): label for raw, label in SPECIAL_EMISSION_ACTORS.items()},
        "referenced_images": image_audit,
        "source_animations": source_animations,
        "units": sources, "buildings": defenses,
        "limitations": ["DAT and GRP chains are source data; IScript streams are linear-path extracts with loops represented by loop_pc.",
            "The inventory does not infer projectile travel or hit position when runtime/script semantics require native engine behavior.",
            "Raw opcode operands are retained; names are labels only. Unsupported data fails the extraction rather than inventing a visual."]}
    out_path.mkdir(parents=True, exist_ok=True)
    target = out_path / "weapon_source_audit.json"
    target.write_text(json.dumps(report, indent=2) + "\n")
    summary = {"out": str(target), "profile_units": len(sources), "unit_weapon_rows": sum(len(x["weapons"]) for x in sources),
               "profile_buildings": len(defenses), "building_weapon_rows": sum(len(x["weapons"]) for x in defenses),
               "unique_weapon_ids": sorted({w["weapon_id"] for x in sources + defenses for w in x["weapons"]})}
    (out_path / "summary.json").write_text(json.dumps(summary, indent=2) + "\n")
    return summary


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rom", type=Path, default=ROOT / ".local/sc64/rom/StarCraft64-USA.z64")
    parser.add_argument("--out", type=Path, default=ROOT / ".local/build/native-weapon-effects")
    args = parser.parse_args(argv)
    print(json.dumps(generate(args.rom, args.out), indent=2))


if __name__ == "__main__":
    main()
