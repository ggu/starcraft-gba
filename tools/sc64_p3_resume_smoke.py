#!/usr/bin/env python3
"""Authenticate and resume a saved P3 expansion checkpoint in a fresh mGBA process.

The source report, matching P5 Shuttle ROM/ELF, and exact SRAM bytes are
authenticated before launch. The debugger writes only the neutral scheduler;
HOME+L performs the actual load.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import sys
import time

TOOLS = Path(__file__).resolve().parent
if str(TOOLS) not in sys.path:
    sys.path.insert(0, str(TOOLS))

import sc64_menu_smoke as menu
import sc64_races_profile as races
import sc64_split_smoke as split
import sc64_split_profile as profile
import sc64_z3_controller_replay as scheduler
import sc64_p3_controller_replay as replay
from emulator_smoke import ROOT, capture, symbol_sizes, symbols

MISSION_P3 = replay.MISSION_P3
SCREEN_TITLE, SCREEN_PLAY, SCREEN_PAUSE = 0, 1, 2
RESUME_TICKS = 60
SRAM_BYTES = 32 * 1024
EXPANSION_TRACE = replay.DEFAULT_EXPANSION_TRACE


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def validate_source_report(report_path: Path, rom_path: Path, elf_path: Path,
                           expected_rom_sha256: str | None = None,
                           expected_elf_sha256: str | None = None) -> dict:
    """Authenticate an expansion prefix report, exact SRAM, and matching build."""
    report_path = report_path.resolve(strict=True)
    rom_path = rom_path.resolve(strict=True)
    elf_path = elf_path.resolve(strict=True)
    try:
        report_bytes = report_path.read_bytes()
        report = json.loads(report_bytes)
    except (OSError, json.JSONDecodeError) as exc:
        raise ValueError(f"cannot read source replay report: {exc}") from exc
    if not isinstance(report, dict):
        raise ValueError("source report must be a JSON object")
    if (report.get("profile") != "protoss-p3" or report.get("route") != "expansion" or
            report.get("policy") != replay.EXPANSION_POLICY or
            report.get("mission") != MISSION_P3):
        raise ValueError("source report must identify the verified P3 expansion route")
    if (report.get("status") != "prefix" or report.get("prefix") is not True or
            report.get("verified_win") is not False):
        raise ValueError("resume seed must be a live-play prefix, not a terminal report")
    for name in ("requested_ticks", "observed_ticks", "input_transitions"):
        if type(report.get(name)) is not int or report[name] < 1:
            raise ValueError(f"source report has invalid {name}")
    rom_hash, elf_hash = sha256_file(rom_path), sha256_file(elf_path)
    for label, expected, actual in (("ROM", expected_rom_sha256, rom_hash),
                                    ("ELF", expected_elf_sha256, elf_hash)):
        if expected is not None and expected.lower() != actual:
            raise ValueError(f"{label} SHA-256 does not match requested cross-check")
    if report.get("rom_sha256") != rom_hash or report.get("elf_sha256") != elf_hash:
        raise ValueError("source report ROM/ELF hashes do not match supplied build")

    checkpoint = report.get("save")
    if not isinstance(checkpoint, dict) or not isinstance(checkpoint.get("state"), dict):
        raise ValueError("source report has no complete save state")
    save_name, expected_digest = checkpoint.get("path"), checkpoint.get("sha256")
    if not isinstance(save_name, str) or not isinstance(expected_digest, str) or len(expected_digest) != 64:
        raise ValueError("save record must contain an SRAM path and SHA-256")
    if any(ch not in "0123456789abcdefABCDEF" for ch in expected_digest):
        raise ValueError("save SRAM SHA-256 is malformed")
    source_sram = Path(save_name).resolve(strict=True)
    if not source_sram.is_file() or source_sram.stat().st_size != SRAM_BYTES:
        raise ValueError(f"source SRAM must be exactly {SRAM_BYTES} bytes")
    actual_digest = sha256_file(source_sram)
    if actual_digest != expected_digest.lower():
        raise ValueError("source SRAM SHA-256 does not match report")

    state = checkpoint["state"]
    if (checkpoint.get("saved_frame") != state.get("frame") or
            type(checkpoint.get("observer_tick")) is not int or
            checkpoint["observer_tick"] != report["observed_ticks"] or
            type(checkpoint.get("observer_frame")) is not int):
        raise ValueError("save checkpoint timing does not match its prefix report")
    if (state.get("screen") != SCREEN_PAUSE or state.get("frame") != checkpoint["saved_frame"] or
            state.get("race_mode") != 0 or state.get("race_selected_mission") != MISSION_P3 or
            state.get("won") != 0 or state.get("lost") != 0):
        raise ValueError("save snapshot must be a paused, unfinished P3 mission")
    campaign, model = state.get("race_campaign"), state.get("race_p3")
    if (not isinstance(campaign, dict) or not isinstance(model, dict) or
            campaign.get("mission") != MISSION_P3 or campaign.get("elapsed_frames") != state["frame"] or
            campaign.get("phase") != 1 or
            type(campaign.get("objective_flags")) is not int or
            campaign["objective_flags"] & 1 or
            type(model.get("outcome")) is not int or model["outcome"] != 0 or
            type(model.get("sequence")) is not int or not 0 <= model["sequence"] <= 5 or
            type(model.get("fired")) is not int or not 0 <= model["fired"] <= 0xFFFF or
            type(model.get("action_ticks")) is not int or not 0 <= model["action_ticks"] <= 0xFFFF or
            type(model.get("transmission")) is not int or not 0 <= model["transmission"] <= 0xFF):
        raise ValueError("save snapshot omits valid ongoing P3 campaign/model state")
    for container, key in ((state, "units"), (state, "buildings")):
        bank = container.get(key)
        if not isinstance(bank, dict) or not isinstance(bank.get("entities"), list):
            raise ValueError("save snapshot omits complete unit/building observations")
    required_witness = ("raw_owner7_units", "raw_owner0_units", "raw_owner6_units",
                        "raw_owner7_buildings", "raw_owner0_buildings", "raw_owner6_buildings",
                        "geyser_count", "selected_units", "selected_building", "race_completed")
    if any(type(state.get(key)) is not int for key in required_witness):
        raise ValueError("save snapshot omits expansion entity/source witness fields")
    if state["geyser_count"] != 3 or state["race_completed"] & (1 << MISSION_P3):
        raise ValueError("save snapshot has invalid source census or already-completed mission")
    final = report.get("final")
    final_model = final.get("race_p3") if isinstance(final, dict) else None
    if (not isinstance(final, dict) or final.get("screen") != SCREEN_PLAY or final.get("won") != 0 or
            final.get("lost") != 0 or final.get("race_mode") != 0 or
            final.get("race_selected_mission") != MISSION_P3 or
            not isinstance(final_model, dict) or final_model.get("outcome") != 0):
        raise ValueError("prefix final observation is not live P3 play")
    return {"report": report, "report_path": report_path,
            "report_sha256": hashlib.sha256(report_bytes).hexdigest(),
            "rom_path": rom_path, "elf_path": elf_path,
            "rom_sha256": rom_hash, "elf_sha256": elf_hash,
            "source_sram": source_sram, "source_sram_sha256": actual_digest,
            "source_state": state, "checkpoint": checkpoint}


def compare_saved_state(expected: dict, observed: dict) -> dict:
    """Require persisted state equality and verify the documented UI selection reset."""
    if expected.get("screen") != SCREEN_PAUSE or observed.get("screen") != SCREEN_PLAY:
        raise RuntimeError("fresh SRAM load must transition saved PAUSE to live PLAY")
    saved_building = expected.get("selected_building")
    loaded_building = observed.get("selected_building")
    if type(saved_building) is not int or loaded_building != -1:
        raise RuntimeError("fresh SRAM load must reset selected_building to -1")

    def walk(left, right, path: str) -> None:
        if path in ("screen", "selected_building"):
            return
        if isinstance(left, dict):
            if not isinstance(right, dict) or left.keys() != right.keys():
                raise RuntimeError(f"restored {path or 'state'} fields differ from saved snapshot")
            for key in left:
                walk(left[key], right[key], f"{path}.{key}" if path else key)
        elif isinstance(left, list):
            if not isinstance(right, list) or len(left) != len(right):
                raise RuntimeError(f"restored {path} length differs from saved snapshot")
            for index, (a, b) in enumerate(zip(left, right)):
                walk(a, b, f"{path}[{index}]")
        elif type(left) is not type(right) or left != right:
            raise RuntimeError(f"restored {path} differs from saved snapshot: {left!r} != {right!r}")
    walk(expected, observed, "")
    return {
        "ui_resets": {
            "screen": {"saved": expected["screen"], "loaded": observed["screen"]},
            "selected_building": {"saved": saved_building, "loaded": loaded_building},
        },
        "persisted_observations_exact": True,
        "restored_snapshot_exact": saved_building == -1,
    }


def verify_handoff_and_resume(loaded_frame: int, tick_zero_frame: int,
                              completed_ticks: int, final_state: dict) -> dict:
    continuity = profile.resumed_frame_continuity(
        loaded_frame, tick_zero_frame, RESUME_TICKS, completed_ticks, RESUME_TICKS, final_state)
    if not continuity["frame_invariant_verified"]:
        raise RuntimeError("P3 expansion resume did not preserve live frame progression")
    return continuity


def run(report_path: Path, rom_path: Path, elf_path: Path, output: Path,
        expected_rom_sha256: str | None = None,
        expected_elf_sha256: str | None = None) -> dict:
    auth = validate_source_report(report_path, rom_path, elf_path,
                                  expected_rom_sha256, expected_elf_sha256)
    report, state = auth["report"], auth["source_state"]
    rom, elf = auth["rom_path"], auth["elf_path"]
    sym, sizes = symbols(elf), symbol_sizes(elf)
    required = ("game_update", "demo_inject_keys", "demo_schedule_events", "demo_schedule_count",
                "demo_schedule_length", "demo_schedule_tick", "demo_replay_checkpoint", "screen",
                "frame", "won", "lost", "race_mode", "race_human", "race_selected_mission",
                "race_completed", "race_campaign", "race_p3", "race_p3_action_ticks",
                "race_minerals", "race_gas", "supply_used", "supply_cap", "weapons_level",
                "armor_level", "weapons_research", "armor_research", "selected_building",
                "geyser_count", "mission_unsupported_count", "u", "b", "race_front")
    missing = [key for key in required if key not in sym]
    if missing:
        raise RuntimeError("P3 expansion ELF lacks resume symbols: " + ", ".join(missing))
    mode, capacity = scheduler.schedule_mode(sym, sizes)
    if mode != "windowed" or capacity < 1:
        raise RuntimeError("P3 expansion resume requires a windowed neutral scheduler")
    for key in ("demo_schedule_active", "demo_schedule_index"):
        if key not in sym:
            raise RuntimeError(f"windowed scheduler lacks {key}")
    metadata = split.read_campaign_metadata(rom, elf, "protoss")
    if MISSION_P3 not in metadata["enabled_mission_ids"]:
        raise RuntimeError("matching P5 Shuttle build does not enable P3 global mission 22")
    bank = races.race_bank_profile(elf, sizes)
    campaign_layout = races.dwarf_struct(elf, "Sc64RaceCampaignState")
    p3_layout = races.dwarf_struct(elf, "Sc64P3State")
    split.FRONT_FIELDS = split.frontend_offsets(elf)

    output.mkdir(parents=True, exist_ok=True)
    run_dir = output / f"protoss-p3-expansion-resume-{time.time_ns()}"
    run_dir.mkdir(parents=True, exist_ok=False)
    save_dir = run_dir / "saves" / run_dir.name
    save_dir.mkdir(parents=True, exist_ok=False)
    copied_save = save_dir / f"{rom.stem}.sav"
    shutil.copyfile(auth["source_sram"], copied_save)
    copied_hash = sha256_file(copied_save)
    if copied_save.stat().st_size != SRAM_BYTES or copied_hash != auth["source_sram_sha256"]:
        raise RuntimeError("fresh mGBA save copy failed size/SHA-256 verification")

    proc = sock = rsp = log = None
    try:
        proc, sock, rsp, log = split.start_emulator(run_dir.name, rom, run_dir, sym)
        split.FRONT_FIELDS = split.frontend_offsets(elf)
        initial = {"screen": profile.u32(rsp, sym["screen"]),
                   "frame": profile.u32(rsp, sym["frame"]),
                   "page": profile.u32(rsp, sym["race_front"] + split.FRONT_FIELDS["page"])}
        if initial != {"screen": SCREEN_TITLE, "frame": 0, "page": split.FRONT_HOME}:
            raise RuntimeError(f"fresh restore process did not start at HOME: {initial}")
        home_capture = capture(rsp, run_dir / "home.png")
        rsp.write(sym["demo_inject_keys"], struct.pack("<H", split.KEY_L))
        rsp.tick(2)
        rsp.write(sym["demo_inject_keys"], b"\0\0")
        loaded = replay.p3_snapshot(rsp, sym, sizes, bank, campaign_layout, p3_layout,
                                    expansion_witness=True)
        if (loaded.get("screen"), loaded.get("race_mode"), loaded.get("race_selected_mission")) != (
                SCREEN_PLAY, 0, MISSION_P3):
            raise RuntimeError("ordinary HOME+L did not load a live P3 mission")
        restore_comparison = compare_saved_state(state, loaded)
        if loaded["frame"] != auth["checkpoint"]["saved_frame"]:
            raise RuntimeError("loaded frame differs from the paused save report")
        loaded_capture = capture(rsp, run_dir / "loaded.png")

        rsp.write(sym["demo_schedule_events"], struct.pack("<2I", 0, 0))
        rsp.write(sym["demo_schedule_length"], struct.pack("<I", RESUME_TICKS))
        rsp.write(sym["demo_schedule_count"], struct.pack("<I", 1))
        rsp.write(sym["demo_schedule_index"], struct.pack("<I", 0))
        rsp.write(sym["demo_schedule_active"], struct.pack("<I", 1))
        if (rsp.cmd(f"z1,{sym['game_update'] & ~1:x},2") != "OK" or
                rsp.cmd(f"Z1,{sym['demo_replay_checkpoint'] & ~1:x},2") != "OK"):
            raise RuntimeError("failed to install neutral P3 resume checkpoints")
        rsp.tick()
        if profile.u32(rsp, sym["demo_schedule_tick"]) != 0:
            raise RuntimeError("scheduler missed neutral tick-zero handoff")
        tick_zero = replay.p3_snapshot(rsp, sym, sizes, bank, campaign_layout, p3_layout,
                                      expansion_witness=True)
        if (tick_zero.get("screen") != SCREEN_PLAY or
                tick_zero.get("frame") != ((loaded["frame"] + 1) & 0xFFFFFFFF)):
            raise RuntimeError("P3 resume did not take exactly one neutral handoff frame")
        if tick_zero.get("race_selected_mission") != MISSION_P3 or tick_zero.get("race_mode") != 0:
            raise RuntimeError("P3 mission identity changed at neutral handoff")
        tick_zero_capture = capture(rsp, run_dir / "tick-000000.png")
        profile.write_progress(run_dir / "progress.json", profile="protoss-p3-expansion-resume",
                               rom=rom, elf=elf, rom_sha256=auth["rom_sha256"],
                               elf_sha256=auth["elf_sha256"],
                               configuration={"mission": MISSION_P3, "route": "expansion",
                                              "source_report": str(auth["report_path"]),
                                              "source_sram_sha256": auth["source_sram_sha256"],
                                              "copied_sram_sha256": copied_hash,
                                              "scheduler_mode": mode, "ticks": RESUME_TICKS},
                               requested_ticks=RESUME_TICKS, checkpoint_interval=RESUME_TICKS,
                               latest_checkpoint={"tick": 0, "loaded_frame": loaded["frame"],
                                                  "tick_zero_frame": tick_zero["frame"], **tick_zero})
        clock0 = profile.hardware_clock(rsp)
        rsp.tick()
        completed = profile.u32(rsp, sym["demo_schedule_tick"])
        if completed != RESUME_TICKS:
            raise RuntimeError(f"expected 60 resumed scheduler ticks, observed {completed}")
        if (profile.u32(rsp, sym["demo_schedule_index"]) != 1 or
                profile.u32(rsp, sym["demo_schedule_active"]) != 1):
            raise RuntimeError("neutral resume event was not consumed exactly once")
        elapsed_cycles = (profile.hardware_clock(rsp) - clock0) & 0xFFFFFFFF
        final = replay.p3_snapshot(rsp, sym, sizes, bank, campaign_layout, p3_layout,
                                   expansion_witness=True)
        if (final.get("screen") != SCREEN_PLAY or final.get("won") or final.get("lost") or
                final.get("race_mode") != 0 or final.get("race_selected_mission") != MISSION_P3 or
                final.get("race_campaign", {}).get("mission") != MISSION_P3 or
                final.get("race_p3", {}).get("outcome") != 0):
            raise RuntimeError("mission stopped being live P3 play during resume")
        if final.get("race_human") != state.get("race_human"):
            raise RuntimeError("human player identity changed during resume")
        continuity = verify_handoff_and_resume(
            loaded["frame"], tick_zero["frame"], completed, final)
        fps = round(profile.CPU_HZ * RESUME_TICKS / max(1, elapsed_cycles), 2)
        final_capture = capture(rsp, run_dir / "tick-000060.png")
        result = {"status": "resumed", "profile": "protoss-p3", "route": "expansion",
                  "mission": MISSION_P3, "rom": str(rom), "elf": str(elf),
                  "rom_sha256": auth["rom_sha256"], "elf_sha256": auth["elf_sha256"],
                  "source_report": str(auth["report_path"]),
                  "source_report_sha256": auth["report_sha256"],
                  "source_sram": str(auth["source_sram"]),
                  "source_sram_sha256": auth["source_sram_sha256"],
                  "copied_sram": str(copied_save), "copied_sram_sha256": copied_hash,
                  "save_loaded_through": "ordinary HOME+L input",
                  **restore_comparison,
                  "initial_home": initial, "initial_home_capture": home_capture,
                  "loaded_capture": loaded_capture, "neutral_handoff_live_frames": 1,
                  "requested_ticks": RESUME_TICKS, "completed_ticks": completed,
                  "final_frame": final["frame"], "mean_live_play_effective_fps": fps,
                  "timer_method": "GBA chained hardware timer across 60 scheduled updates",
                  "continuity": continuity,
                  "captures": {"tick_zero": str(run_dir / "tick-000000.png"),
                               "final": str(run_dir / "tick-000060.png")},
                  "tick_zero": tick_zero, "final": final,
                  "final_capture": final_capture}
        result_path = run_dir / "resume.json"
        result_path.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
        profile.write_progress(run_dir / "progress.json", profile="protoss-p3-expansion-resume",
                               rom=rom, elf=elf, rom_sha256=auth["rom_sha256"],
                               elf_sha256=auth["elf_sha256"],
                               configuration={"mission": MISSION_P3, "route": "expansion",
                                              "source_report_sha256": auth["report_sha256"],
                                              "source_sram_sha256": auth["source_sram_sha256"],
                                              "copied_sram_sha256": copied_hash,
                                              "scheduler_mode": mode, "ticks": RESUME_TICKS},
                               requested_ticks=RESUME_TICKS, checkpoint_interval=RESUME_TICKS,
                               latest_checkpoint={"tick": RESUME_TICKS, **final},
                               status="finished", final_report=str(result_path))
        return result
    finally:
        if proc is not None:
            try:
                menu.stop_emulator(proc, sock, log)
            except Exception:
                proc.terminate()
                try:
                    proc.wait(timeout=3)
                except Exception:
                    proc.kill()


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--report", type=Path, required=True, help="P3 expansion prefix profile.json")
    parser.add_argument("--rom", type=Path, default=ROOT / "build/StarCraft-Protoss-P5-Shuttle-GBA.gba")
    parser.add_argument("--elf", type=Path, default=ROOT / "build/starcraft-protoss-p5-shuttle.elf")
    parser.add_argument("--output", type=Path, default=ROOT / "build/emulator/p3-expansion-resume")
    parser.add_argument("--expect-rom-sha256")
    parser.add_argument("--expect-elf-sha256")
    return parser


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    try:
        result = run(args.report, args.rom, args.elf, args.output,
                     args.expect_rom_sha256, args.expect_elf_sha256)
        print(json.dumps({"status": result["status"], "mission": MISSION_P3,
                          "ticks": result["completed_ticks"], "frame": result["final_frame"],
                          "fps": result["mean_live_play_effective_fps"]}, indent=2))
    except (OSError, ValueError, RuntimeError, KeyError, json.JSONDecodeError) as exc:
        print(f"P3 expansion resume failed: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
