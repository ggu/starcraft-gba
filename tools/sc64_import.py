#!/usr/bin/env python3
"""Import one StarCraft 64 mission from a ROM owned by the user.

This is a thin adapter around the audited extractor in references/sc64-maps.
All derived scenario material is written below .local/, which is ignored by
Git. It emits a raw audit JSON and a C header for the GBA mission adapter.

The N64 scenario and artwork are Blizzard/Nintendo content. Keep generated
files private; this tool ships no game data.

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
from collections import Counter
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
UPSTREAM = ROOT / "references/sc64-maps/extract_sc64_maps.py"
UPSTREAM_COMMIT = "09b57f787a247107d1de82b8c5eab2734667ea1d"
EXPECTED_USA_ROM_SHA1 = "a36d91b5aac7bb95df0d7de5f9814f5270e25c2b"
DEFAULT_ROM = ROOT / ".local/sc64/rom/StarCraft64-USA.z64"
DEFAULT_OUT = ROOT / ".local/sc64/extracted"
DEFAULT_HEADER = ROOT / ".local/sc64/generated/sc64_mission_generated.h"
MISSION_PATH = "008/00A"
MISSIONS = {
    "wasteland": {"path": "008/00A", "slug": "wasteland"},
    "backwater": {"path": "008/00B", "slug": "backwater"},
}
UNIT_SIZE = 36
TRIGGER_SIZE = 2400

ACTION_NAMES = (
    "None", "Victory", "Defeat", "PreserveTrigger", "Wait", "PauseGame",
    "UnpauseGame", "Transmission", "PlayWAV", "DisplayTextMessage",
    "CenterView", "CreateUnitwithProperties", "SetMissionObjectives",
    "SetSwitch", "SetCountdownTimer", "RunAIScript", "RunAIScriptAtLocation",
    "LeaderBoardControl", "LeaderBoardControlAt", "LeaderBoardResources",
    "LeaderBoardKills", "LeaderBoardPoints", "KillUnit", "KillUnitAtLocation",
    "RemoveUnit", "RemoveUnitAtLocation", "SetResources", "SetScore",
    "MinimapPing", "TalkingPortrait", "MuteUnitSpeech", "UnmuteUnitSpeech",
    "LeaderboardComputerPlayers", "LeaderboardGoalControl",
    "LeaderboardGoalControlAt", "LeaderboardGoalResources",
    "LeaderboardGoalKills", "LeaderboardGoalPoints", "MoveLocation",
    "MoveUnit", "LeaderboardGreed", "SetNextScenario", "SetDoodadState",
    "SetInvincibility", "CreateUnit", "SetDeaths", "Order", "Comment",
    "GiveUnitstoPlayer", "ModifyUnitHitPoints", "ModifyUnitEnergy",
    "ModifyUnitShieldPoints", "ModifyUnitResourceAmount",
    "ModifyUnitHangerCount", "PauseTimer", "UnpauseTimer", "Draw",
    "SetAllianceStatus", "DisableDebugMode", "EnableDebugMode",
)
CONDITION_NAMES = (
    "None", "CountdownTimer", "Command", "Bring", "Accumulate", "Kill",
    "CommandTheMost", "CommandsTheMostAt", "MostKills", "HighestScore",
    "MostResources", "Switch", "ElapsedTime", "MissionBriefing",
    "Opponents", "Deaths", "CommandTheLeast", "CommandTheLeastAt",
    "LeastKills", "LowestScore", "LeastResources", "Score", "Always", "Never",
)


def upstream_module(path: Path):
    spec = importlib.util.spec_from_file_location("sc64_upstream_extract", path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load upstream extractor: {path}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


def sections(chk: bytes, extractor) -> dict[bytes, bytes]:
    return {tag: payload for tag, payload in extractor.chk_sections(chk)}


def unit_rows(payload: bytes) -> list[dict[str, int]]:
    if len(payload) % UNIT_SIZE:
        raise ValueError(f"UNIT size {len(payload)} is not a multiple of {UNIT_SIZE}")
    rows = []
    for offset in range(0, len(payload), UNIT_SIZE):
        x, y, raw_type = struct.unpack_from("<HHH", payload, offset + 4)
        relation, special, valid = struct.unpack_from("<HHH", payload, offset + 10)
        owner, hp_percent, shield_percent, energy_percent = payload[offset + 16:offset + 20]
        resources = struct.unpack_from("<I", payload, offset + 20)[0]
        flags = struct.unpack_from("<H", payload, offset + 26)[0]
        rows.append({
            "x_source_px": x, "y_source_px": y, "source_type": raw_type,
            "relation": relation, "special": special, "valid": valid,
            "owner_raw": owner, "hp_percent": hp_percent,
            "shield_percent": shield_percent, "energy_percent": energy_percent,
            "resources": resources, "flags": flags,
        })
    return rows


def nonzero_fields(record: bytes, start: int, size: int, kind: str) -> list[dict[str, Any]]:
    result = []
    for index in range(16 if kind == "condition" else 64):
        off = start + index * size
        raw = record[off:off + size]
        if not any(raw):
            continue
        if kind == "condition":
            location, player_group, amount = struct.unpack_from("<III", raw, 0)
            unit = struct.unpack_from("<H", raw, 12)[0]
            comparison, opcode, resource, flags = raw[14:18]
            mask = struct.unpack_from("<H", raw, 18)[0]
            result.append({
                "index": index, "opcode": opcode,
                "name": CONDITION_NAMES[opcode] if opcode < len(CONDITION_NAMES) else "unknown",
                "location": location, "player_group": player_group, "amount": amount,
                "unit_type": unit, "comparison": comparison, "resource": resource,
                "flags": flags, "mask": mask,
            })
        else:
            location, string_id, wav_id, time, group1, group2 = struct.unpack_from("<IIIIII", raw, 0)
            unit_type = struct.unpack_from("<H", raw, 24)[0]
            opcode, modifier, flags = raw[26:29]
            result.append({
                "index": index, "opcode": opcode,
                "name": ACTION_NAMES[opcode] if opcode < len(ACTION_NAMES) else "unknown",
                "location": location, "string_id": string_id, "wav_id": wav_id,
                "time": time, "group1": group1, "group2": group2,
                "unit_type": unit_type, "modifier": modifier, "flags": flags,
            })
    return result


def trigger_rows(payload: bytes, string_table: bytes, extractor) -> list[dict[str, Any]]:
    if len(payload) % TRIGGER_SIZE:
        raise ValueError(f"TRIG size {len(payload)} is not a multiple of {TRIGGER_SIZE}")
    rows = []
    for index, base in enumerate(range(0, len(payload), TRIGGER_SIZE)):
        record = payload[base:base + TRIGGER_SIZE]
        conditions = nonzero_fields(record, 0, 20, "condition")
        actions = nonzero_fields(record, 320, 32, "action")
        for action in actions:
            if action["string_id"]:
                action["text"] = extractor._chk_string(string_table, action["string_id"])
        flags = struct.unpack_from("<I", record, 2368)[0]
        executed_for_values = list(record[2372:2399])
        executed_for = [i for i, value in enumerate(executed_for_values) if value]
        current_action = record[2399]
        if conditions or actions or flags or executed_for:
            rows.append({
                "record": index, "conditions": conditions, "actions": actions,
                "execution_flags": flags,
                "executed_for_player_bytes_nonzero": executed_for,
                "executed_for_player_values": executed_for_values,
                "current_action": current_action,
            })
    return rows


def c_string(value: str) -> str:
    return json.dumps(value, ensure_ascii=True)


def emit_header(path: Path, map_data: dict[str, Any], units: list[dict[str, int]],
                terrain: list[int], objectives: list[str],
                locations: list[dict[str, Any]] | None = None) -> None:
    runtime_units = [u for u in units if u["source_type"] != 214]
    lines = [
        "/* Generated from a user-supplied StarCraft 64 ROM by tools/sc64_import.py.",
        " * Keep this derived copyrighted data private; .local/ is ignored by Git.",
        " * Source: references/sc64-maps/extract_sc64_maps.py @ " + UPSTREAM_COMMIT,
        " */",
        "#ifndef SC64_MISSION_GENERATED_H",
        "#define SC64_MISSION_GENERATED_H",
        '#include "sc64_types.h"',
        f"#define SC64_MAP_COLS {map_data['width']}",
        f"#define SC64_MAP_ROWS {map_data['height']}",
        "#define SC64_MAP_CELLS (SC64_MAP_COLS * SC64_MAP_ROWS)",
        f"#define SC64_IMPORTED_UNIT_COUNT {len(runtime_units)}",
        f"#define SC64_OBJECTIVE_COUNT {len(objectives)}",
    ]
    if locations is not None:
        lines.extend([
            "#define SC64_LOCATION_COUNT 64",
            "typedef struct { unsigned int left, top, right, bottom; } Sc64ImportedLocation;",
            "/* CHK source-pixel rectangles; index is the original 1-based MRGN id. */",
            "static const Sc64ImportedLocation sc64_locations[SC64_LOCATION_COUNT + 1] = {",
            "    {0u, 0u, 0u, 0u},",
        ])
        for location in locations:
            lines.append(
                f"    [{location['index']}] = {{{location['left']}u, {location['top']}u, "
                f"{location['right']}u, {location['bottom']}u}},"
            )
        lines.append("};")
    lines.extend([
        "/* MTXM stores 16-bit tileset-specific indices, not walkability. */",
        "static const u16 sc64_terrain_tile_ids[SC64_MAP_CELLS] = {",
    ])
    for row in range(map_data["height"]):
        chunk = terrain[row * map_data["width"]:(row + 1) * map_data["width"]]
        lines.append("    " + ", ".join(f"0x{x:04X}u" for x in chunk) + ",")
    lines.extend(["};", "static const Sc64ImportedUnit sc64_units[SC64_IMPORTED_UNIT_COUNT] = {"])
    for u in runtime_units:
        if u["flags"] > 0xFFFF:
            raise ValueError(f"unit flags do not fit runtime schema: {u['flags']}")
        if u["resources"] > 0xFFFF:
            raise ValueError(f"unit resources do not fit runtime schema: {u['resources']}")
        # The gameplay renderer uses 16px per tile; StarCraft's CHK uses 32px.
        x_gba_px, y_gba_px = u["x_source_px"] // 2, u["y_source_px"] // 2
        lines.append(
            f"    {{{x_gba_px}u, {y_gba_px}u, {u['source_type']}u, "
            f"{u['owner_raw']}u, {u['hp_percent']}u, {u['flags']}u, "
            f"{u['resources']}u}},"
        )
    lines.append("};")
    lines.append("static const char *const sc64_objectives[SC64_OBJECTIVE_COUNT] = {")
    lines.extend(f"    {c_string(text)}," for text in objectives)
    lines.extend(["};", "#endif /* SC64_MISSION_GENERATED_H */", ""])
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("\n".join(lines), encoding="ascii")


def run(rom_path: Path, output_dir: Path, header_path: Path,
        upstream_path: Path = UPSTREAM, mission_path: str = MISSION_PATH) -> dict[str, Any]:
    extractor = upstream_module(upstream_path)
    rom = extractor.load_rom(str(rom_path))
    normalized_sha1 = hashlib.sha1(rom).hexdigest()
    if normalized_sha1 != EXPECTED_USA_ROM_SHA1:
        raise ValueError(
            "ROM must be the verified USA retail image "
            f"(normalized SHA1 {EXPECTED_USA_ROM_SHA1}; got {normalized_sha1})"
        )
    archive = extractor.BoltArchive(rom)
    maps = list(extractor.find_maps(archive))
    if not maps:
        raise ValueError("no CHK scenarios found in ROM")

    output_dir.mkdir(parents=True, exist_ok=True)
    index = []
    selected: tuple[Any, bytes] | None = None
    for info, chk in maps:
        entry = {
            "path": info.bolt_path, "title": info.name,
            "description": info.description, "width": info.width,
            "height": info.height, "tileset_id": info.tileset & 7,
            "tileset": info.tileset_name, "players": info.players,
            "version": info.version, "edition": info.edition,
            "chk_size": len(chk),
        }
        index.append(entry)
        if info.bolt_path == mission_path:
            selected = info, chk
    (output_dir / "scenario-index.json").write_text(
        json.dumps({"scenario_count": len(index), "scenarios": index}, indent=2) + "\n",
        encoding="utf-8",
    )
    if selected is None:
        raise ValueError(f"selected mission {mission_path} absent from ROM")

    info, chk = selected
    sections_by_tag = sections(chk, extractor)
    unit_data = unit_rows(sections_by_tag.get(b"UNIT", b""))
    raw_terrain = sections_by_tag.get(b"MTXM", b"")
    if len(raw_terrain) != info.width * info.height * 2:
        raise ValueError("MTXM length does not match DIM dimensions")
    terrain = list(struct.unpack(f"<{len(raw_terrain) // 2}H", raw_terrain))
    triggers = trigger_rows(sections_by_tag.get(b"TRIG", b""),
                            sections_by_tag.get(b"STR ", b""), extractor)
    objectives = []
    for trigger in triggers:
        for action in trigger["actions"]:
            if action["opcode"] == 12 and action.get("text") and action["text"] not in objectives:
                objectives.append(action["text"])
    counts = Counter((u["owner_raw"], u["source_type"]) for u in unit_data)
    tally = Counter()
    condition_tally = Counter()
    for trigger in triggers:
        for action in trigger["actions"]:
            tally[action["opcode"]] += 1
        for condition in trigger["conditions"]:
            condition_tally[condition["opcode"]] += 1
    def section_hex(tag: bytes) -> str:
        return sections_by_tag.get(tag, b"").hex()

    # Locations and property slots are kept as decoded CHK records as well as
    # raw hex, so a runtime can interpret them without depending on the ROM.
    mrgn = sections_by_tag.get(b"MRGN", b"")
    locations = []
    for i, off in enumerate(range(0, len(mrgn), 20)):
        raw = mrgn[off:off + 20]
        if len(raw) < 20:
            break
        left, top, right, bottom, string_id, flags = struct.unpack_from("<IIIIHH", raw)
        locations.append({"index": i + 1, "left": left, "top": top,
                          "right": right, "bottom": bottom,
                          "string_id": string_id,
                          "name": extractor._chk_string(sections_by_tag.get(b"STR ", b""), string_id) if string_id else "",
                          "flags": flags})
    properties = []
    uprp = sections_by_tag.get(b"UPRP", b"")
    for i, off in enumerate(range(0, len(uprp), 20)):
        raw = uprp[off:off + 20]
        if len(raw) == 20:
            properties.append({"index": i, "raw_hex": raw.hex()})
    mission = {
        "schema_version": 1,
        "source": {
            "rom_filename": rom_path.name,
            "rom_sha1": normalized_sha1,
            "rom_input_sha1": hashlib.sha1(rom_path.read_bytes()).hexdigest(),
            "bolt_build_stamp": archive.build_stamp,
            "upstream": "references/sc64-maps/extract_sc64_maps.py",
            "upstream_commit": UPSTREAM_COMMIT,
        },
        "scenario": {
            "path": info.bolt_path, "profile": next((k for k, v in MISSIONS.items() if v["path"] == mission_path), "custom"), "title": info.name,
            "description": info.description, "width": info.width,
            "height": info.height, "source_tile_pixels": 32,
            "tileset_id": info.tileset & 7, "tileset": info.tileset_name,
            "version": info.version, "edition": info.edition,
            "chk_size": len(chk), "chk_sha1": hashlib.sha1(chk).hexdigest(),
        },
        "unit_record_count": len(unit_data),
        "runtime_unit_count_excluding_start_markers": sum(1 for u in unit_data if u["source_type"] != 214),
        "units_by_raw_owner_and_type": [
            {"owner_raw": owner, "source_type": source_type, "count": count}
            for (owner, source_type), count in sorted(counts.items())
        ],
        "units": unit_data,
        "terrain": {"section": "MTXM", "tile_count": len(terrain),
                    "tile_ids": terrain, "collision_verified": False},
        "trigger_count": len(triggers), "triggers": triggers,
        "condition_opcode_counts": dict(sorted(condition_tally.items())),
        "action_opcode_counts": dict(sorted(tally.items())),
        "objective_strings": objectives,
    }
    if mission_path == MISSIONS["backwater"]["path"]:
        mission.update({
            "locations": locations,
            "unit_properties": properties,
            "player_setup": {"OWNR_hex": section_hex(b"OWNR"), "SIDE_hex": section_hex(b"SIDE"),
                             "UPRP_hex": section_hex(b"UPRP"), "UPUS_hex": section_hex(b"UPUS"),
                             "PUNI_hex": section_hex(b"PUNI"), "PTEC_hex": section_hex(b"PTEC"),
                             "PTEx_hex": section_hex(b"PTEx"), "UPGR_hex": section_hex(b"UPGR"),
                             "UPGx_hex": section_hex(b"UPGx"), "UNIx_hex": section_hex(b"UNIx"),
                             "UNIS_hex": section_hex(b"UNIS")},
        })
    slug = next((v["slug"] for v in MISSIONS.values() if v["path"] == mission_path), "mission")
    (output_dir / f"{slug}-trigger-report.json").write_text(
        json.dumps(mission, indent=2, ensure_ascii=True) + "\n", encoding="utf-8",
    )
    (output_dir / f"{slug}.chk").write_bytes(chk)
    emit_header(header_path, mission["scenario"], unit_data, terrain, objectives,
                locations if mission_path == MISSIONS["backwater"]["path"] else None)
    return mission


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rom", type=Path, default=DEFAULT_ROM)
    parser.add_argument("--out", type=Path)
    parser.add_argument("--header", type=Path)
    parser.add_argument("--mission", choices=[v["path"] for v in MISSIONS.values()])
    parser.add_argument("--profile", choices=MISSIONS)
    parser.add_argument("--upstream", type=Path, default=UPSTREAM)
    args = parser.parse_args(argv)
    if not args.rom.is_file():
        parser.error(f"ROM does not exist: {args.rom}")
    try:
        if args.mission and args.profile:
            parser.error("use either --mission or --profile")
        mission_path = args.mission or MISSIONS[args.profile]["path"] if args.profile else (args.mission or MISSION_PATH)
        profile = next((key for key, value in MISSIONS.items() if value["path"] == mission_path), "custom")
        private = ROOT / ".local/sc64" / profile
        out = args.out or (private / "extracted" if profile == "backwater" else DEFAULT_OUT)
        header = args.header or (private / "generated/sc64_mission_generated.h" if profile == "backwater" else DEFAULT_HEADER)
        result = run(args.rom, out, header, args.upstream, mission_path)
    except (OSError, ValueError, RuntimeError) as exc:
        print(f"sc64_import: {exc}", file=sys.stderr)
        return 1
    scenario = result["scenario"]
    print(f"Imported {scenario['title']} ({scenario['width']}x{scenario['height']})")
    print(f"{result['unit_record_count']} raw UNIT records; {result['trigger_count']} trigger records")
    slug = result["scenario"]["profile"]
    print(f"metadata: {out / f'{slug}-trigger-report.json'}")
    print(f"C header: {header}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
