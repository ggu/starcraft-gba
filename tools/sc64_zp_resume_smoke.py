#!/usr/bin/env python3
"""Authenticate and resume a saved staged Zerg/Protoss campaign in a fresh mGBA process.

The debugger writes only the neutral controller schedule. SRAM is copied into
a new mGBA save directory, then loaded through the ordinary HOME L control.
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
import sc64_zp_controller_replay as replay
from emulator_smoke import ROOT, capture, symbol_sizes, symbols

SCREEN_TITLE, SCREEN_PLAY, SCREEN_PAUSE = 0, 1, 2
RESUME_TICKS = 60
SRAM_BYTES = 32 * 1024


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def validate_source_report(report_path: Path, rom_path: Path, elf_path: Path,
                           expected_rom_sha256: str | None = None,
                           expected_elf_sha256: str | None = None) -> dict:
    """Authenticate replay report, matching build, and exact source SRAM bytes."""
    report_path = report_path.resolve(strict=True)
    rom_path = rom_path.resolve(strict=True)
    elf_path = elf_path.resolve(strict=True)
    try:
        report_bytes = report_path.read_bytes()
        report = json.loads(report_bytes)
    except (OSError, json.JSONDecodeError) as exc:
        raise ValueError(f"cannot read source replay report: {exc}") from exc
    if not isinstance(report, dict) or report.get("profile") not in replay.MISSION_BY_PROFILE:
        raise ValueError("source report profile must name a supported staged Zerg/Protoss mission")
    profile_name = report["profile"]
    mission = replay.MISSION_BY_PROFILE[profile_name]
    if type(report.get("mission")) is not int or report["mission"] != mission:
        raise ValueError(f"source report mission must match {profile_name} global ID {mission}")
    if report.get("status") != "prefix" or report.get("prefix") is not True:
        raise ValueError("resume seed must come from a bounded live-play prefix report")
    if type(report.get("requested_ticks")) is not int or report["requested_ticks"] < 1:
        raise ValueError("source report has no positive requested prefix length")
    rom_hash, elf_hash = sha256_file(rom_path), sha256_file(elf_path)
    for label, expected, actual in (("ROM", expected_rom_sha256, rom_hash),
                                    ("ELF", expected_elf_sha256, elf_hash)):
        if expected and expected.lower() != actual:
            raise ValueError(f"{label} SHA-256 does not match requested cross-check")
    if report.get("rom_sha256") != rom_hash or report.get("elf_sha256") != elf_hash:
        raise ValueError("source report ROM/ELF hashes do not match the supplied build")
    checkpoint = report.get("save_checkpoint")
    if not isinstance(checkpoint, dict) or not isinstance(checkpoint.get("state"), dict):
        raise ValueError("source report has no authenticated save_checkpoint state")
    save_name, expected_digest = checkpoint.get("path"), checkpoint.get("sha256")
    if not isinstance(save_name, str) or not isinstance(expected_digest, str):
        raise ValueError("save_checkpoint must contain an SRAM path and SHA-256")
    if (len(expected_digest) != 64 or
            any(ch not in "0123456789abcdefABCDEF" for ch in expected_digest)):
        raise ValueError("save_checkpoint SHA-256 is malformed")
    source_sram = Path(save_name).resolve(strict=True)
    if not source_sram.is_file() or source_sram.stat().st_size != SRAM_BYTES:
        raise ValueError(f"source SRAM must be exactly {SRAM_BYTES} bytes")
    actual_digest = sha256_file(source_sram)
    if actual_digest != expected_digest.lower():
        raise ValueError("source SRAM SHA-256 does not match the report")
    expected_state = checkpoint["state"]
    if expected_state.get("screen") != SCREEN_PAUSE:
        raise ValueError("save checkpoint snapshot must be taken on the PAUSE screen")
    if expected_state.get("frame") != checkpoint.get("saved_frame"):
        raise ValueError("save checkpoint frame differs from its paused state snapshot")
    if (expected_state.get("race_mode") != 0 or
            expected_state.get("race_selected_mission") != mission or
            expected_state.get("won") != 0 or expected_state.get("lost") != 0):
        raise ValueError("save checkpoint is not a live campaign mission for its profile")
    if not isinstance(expected_state.get("race_campaign"), dict) or not isinstance(
            expected_state.get("race_runtime"), dict):
        raise ValueError("save checkpoint omits campaign or composite runtime state")
    if not isinstance(expected_state.get("units"), dict) or not isinstance(
            expected_state.get("buildings"), dict):
        raise ValueError("save checkpoint omits entity-bank observations")
    return {"report": report, "report_path": report_path,
            "report_sha256": hashlib.sha256(report_bytes).hexdigest(),
            "profile": profile_name, "mission": mission,
            "rom_path": rom_path, "elf_path": elf_path,
            "rom_sha256": rom_hash, "elf_sha256": elf_hash,
            "source_sram": source_sram, "source_sram_sha256": actual_digest,
            "source_state": expected_state, "checkpoint": checkpoint}


def compare_saved_state(expected: dict, observed: dict) -> None:
    """Require exact restored snapshot equality, except PAUSE becomes PLAY."""
    if not isinstance(expected, dict) or not isinstance(observed, dict):
        raise ValueError("saved and restored state snapshots must be objects")
    if expected.get("screen") != SCREEN_PAUSE or observed.get("screen") != SCREEN_PLAY:
        raise RuntimeError("fresh SRAM load must transition from saved PAUSE to live PLAY")

    def walk(left, right, path: str) -> None:
        if path == "screen":
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


def verify_handoff_and_resume(loaded_frame: int, tick_zero_frame: int,
                              requested_ticks: int, completed_ticks: int,
                              final_state: dict) -> dict:
    """Check one unmeasured neutral handoff, then measured live frame continuity."""
    continuity = profile.resumed_frame_continuity(
        loaded_frame, tick_zero_frame, requested_ticks, completed_ticks,
        requested_ticks if final_state.get("screen") == SCREEN_PLAY else 0, final_state)
    if not continuity["frame_invariant_verified"]:
        raise RuntimeError("resume did not preserve continuous live-play frame progression")
    return continuity


def run(report_path: Path, rom_path: Path, elf_path: Path, output: Path,
        expected_rom_sha256: str | None = None,
        expected_elf_sha256: str | None = None) -> dict:
    auth = validate_source_report(report_path, rom_path, elf_path,
                                  expected_rom_sha256, expected_elf_sha256)
    report, profile_name, mission = auth["report"], auth["profile"], auth["mission"]
    rom, elf = auth["rom_path"], auth["elf_path"]
    sym, sizes = symbols(elf), symbol_sizes(elf)
    missing = [key for key in ("game_update", "demo_inject_keys", "demo_schedule_events",
                               "demo_schedule_count", "demo_schedule_length", "demo_schedule_tick",
                               "demo_replay_checkpoint", "screen", "frame", "won", "lost",
                               "race_mode", "race_human", "race_selected_mission", "race_completed",
                               "race_campaign", "race_minerals", "race_gas", "race_front", "u", "b",
                               replay.RUNTIME_SYMBOLS[profile_name]) if key not in sym]
    if missing:
        raise RuntimeError("matching ELF lacks resume symbols: " + ", ".join(missing))
    mode, capacity = scheduler.schedule_mode(sym, sizes)
    if mode == "windowed":
        needed = ("demo_schedule_active", "demo_schedule_index")
        missing = [key for key in needed if key not in sym]
        if missing:
            raise RuntimeError("windowed scheduler lacks symbols: " + ", ".join(missing))
    if capacity < 1:
        raise RuntimeError("controller scheduler has no queue capacity")
    metadata = split.read_campaign_metadata(rom, elf, replay.FACTION_BY_PROFILE[profile_name])
    if mission not in metadata["enabled_mission_ids"]:
        raise RuntimeError(f"matching campaign catalog does not enable mission {mission}")
    bank = replay.compact_bank_profile(elf, sizes, profile_name)
    campaign_layout = races.dwarf_struct(elf, "Sc64RaceCampaignState")
    runtime_layout = replay.runtime_layout_for(elf, profile_name)
    model_layout = races.dwarf_struct(elf, replay.MODEL_TYPES[profile_name])
    split.FRONT_FIELDS = split.frontend_offsets(elf)
    run_dir = output / f"{profile_name}-resume-{time.time_ns()}"
    run_dir.mkdir(parents=True, exist_ok=False)
    save_dir = run_dir / "saves" / run_dir.name
    save_dir.mkdir(parents=True, exist_ok=False)
    copied_save = save_dir / f"{rom.stem}.sav"
    shutil.copyfile(auth["source_sram"], copied_save)
    copied_hash = sha256_file(copied_save)
    if copied_save.stat().st_size != SRAM_BYTES or copied_hash != auth["source_sram_sha256"]:
        raise RuntimeError("fresh mGBA save copy failed its size/SHA-256 check")

    progress_path = run_dir / "progress.json"
    history_path = run_dir / "checkpoints.jsonl"
    proc = sock = rsp = log = None
    try:
        proc, sock, rsp, log = split.start_emulator(run_dir.name, rom, run_dir, sym)
        initial = {"screen": profile.u32(rsp, sym["screen"]),
                   "frame": profile.u32(rsp, sym["frame"]),
                   "page": profile.u32(rsp, sym["race_front"] + split.FRONT_FIELDS["page"])}
        if initial != {"screen": SCREEN_TITLE, "frame": 0, "page": split.FRONT_HOME}:
            raise RuntimeError(f"fresh restore process did not start at HOME: {initial}")
        home_capture = capture(rsp, run_dir / "home.png")

        # Input was sampled before the update breakpoint: execute the pending
        # neutral HOME call and then L's load. Observe before any live update;
        # the generic key helper also releases for two ticks, advancing play.
        rsp.write(sym["demo_inject_keys"], struct.pack("<H", split.KEY_L))
        rsp.tick(2)
        rsp.write(sym["demo_inject_keys"], b"\0\0")
        loaded = replay.mission_snapshot(rsp, sym, sizes, bank, campaign_layout,
                                         runtime_layout, model_layout, profile_name)
        if (loaded.get("screen"), loaded.get("race_mode"),
                loaded.get("race_selected_mission")) != (SCREEN_PLAY, 0, mission):
            raise RuntimeError("ordinary HOME L input did not load the requested live mission")
        compare_saved_state(auth["source_state"], loaded)
        if loaded["frame"] != auth["checkpoint"]["saved_frame"]:
            raise RuntimeError("loaded frame differs from the report's saved frame")
        loaded_capture = capture(rsp, run_dir / "loaded.png")

        # Observe and schedule one explicitly neutral event at scheduler tick 0.
        # The resumed-frame check records the one ordinary update between load
        # and this observer boundary separately from the 60 measured ticks.
        rsp.write(sym["demo_schedule_events"], struct.pack("<2I", 0, 0))
        rsp.write(sym["demo_schedule_length"], struct.pack("<I", RESUME_TICKS))
        rsp.write(sym["demo_schedule_count"], struct.pack("<I", 1))
        if mode == "windowed":
            rsp.write(sym["demo_schedule_index"], struct.pack("<I", 0))
            rsp.write(sym["demo_schedule_active"], struct.pack("<I", 1))
        if (rsp.cmd(f"z1,{sym['game_update'] & ~1:x},2") != "OK" or
                rsp.cmd(f"Z1,{sym['demo_replay_checkpoint'] & ~1:x},2") != "OK"):
            raise RuntimeError("failed to install neutral continuation observer")
        rsp.tick()
        if profile.u32(rsp, sym["demo_schedule_tick"]) != 0:
            raise RuntimeError("scheduler did not stop at the neutral tick-zero handoff")
        tick_zero = replay.mission_snapshot(rsp, sym, sizes, bank, campaign_layout,
                                            runtime_layout, model_layout, profile_name)
        if tick_zero["screen"] != SCREEN_PLAY or tick_zero["frame"] != ((loaded["frame"] + 1) & 0xFFFFFFFF):
            raise RuntimeError("resume did not take exactly one neutral handoff frame")
        if tick_zero["race_selected_mission"] != mission or tick_zero["race_mode"] != 0:
            raise RuntimeError("mission identity changed during neutral handoff")
        tick_zero_capture = capture(rsp, run_dir / "tick-000000.png")
        zero = {"tick": 0, "host_tick": 0, "loaded_frame": loaded["frame"],
                "tick_zero_frame": tick_zero["frame"], "counted_live_play_interval": False,
                "capture": tick_zero_capture, **tick_zero}
        observations = [zero]
        with history_path.open("a", encoding="utf-8") as stream:
            stream.write(json.dumps(zero, separators=(",", ":")) + "\n")
            stream.flush(); os.fsync(stream.fileno())
        profile.write_progress(progress_path, profile=f"{profile_name}-resume", rom=rom, elf=elf,
                               rom_sha256=auth["rom_sha256"], elf_sha256=auth["elf_sha256"],
                               configuration={"profile": profile_name, "mission": mission,
                                              "source_report": str(auth["report_path"]),
                                              "source_report_sha256": auth["report_sha256"],
                                              "source_sram": str(auth["source_sram"]),
                                              "source_sram_sha256": auth["source_sram_sha256"],
                                              "copied_sram": str(copied_save),
                                              "copied_sram_sha256": copied_hash,
                                              "scheduler_mode": mode, "ticks": RESUME_TICKS},
                               requested_ticks=RESUME_TICKS, checkpoint_interval=RESUME_TICKS,
                               latest_checkpoint=zero)

        clock0 = profile.hardware_clock(rsp)
        rsp.tick()
        completed = profile.u32(rsp, sym["demo_schedule_tick"])
        if completed != RESUME_TICKS:
            raise RuntimeError(f"expected {RESUME_TICKS} neutral ticks, observed {completed}")
        if mode == "windowed":
            if profile.u32(rsp, sym["demo_schedule_index"]) != 1:
                raise RuntimeError("neutral input event was not consumed exactly once")
            if profile.u32(rsp, sym["demo_schedule_active"]) != 1:
                raise RuntimeError("windowed scheduler unexpectedly disabled before final checkpoint")
        clock1 = profile.hardware_clock(rsp)
        elapsed_cycles = (clock1 - clock0) & 0xFFFFFFFF
        final = replay.mission_snapshot(rsp, sym, sizes, bank, campaign_layout,
                                        runtime_layout, model_layout, profile_name)
        if final["screen"] != SCREEN_PLAY or final["won"] or final["lost"]:
            raise RuntimeError("mission left live play during the 60-tick neutral continuation")
        if final["race_mode"] != 0 or final["race_selected_mission"] != mission:
            raise RuntimeError("player/mission identity changed during neutral continuation")
        if final.get("race_human") != auth["source_state"].get("race_human"):
            raise RuntimeError("human player identity changed during neutral continuation")
        frame_delta = (final["frame"] - tick_zero["frame"]) & 0xFFFFFFFF
        if frame_delta != RESUME_TICKS:
            raise RuntimeError(f"expected {RESUME_TICKS} resumed frames, observed {frame_delta}")
        fps = round(profile.CPU_HZ * frame_delta / max(1, elapsed_cycles), 2)
        final_capture = capture(rsp, run_dir / "tick-000060.png")
        last = {"tick": RESUME_TICKS, "host_tick": RESUME_TICKS,
                "elapsed_hardware_cycles": elapsed_cycles, "frame_delta": frame_delta,
                "effective_fps": fps, "counted_live_play_interval": True,
                "capture": final_capture, **final}
        observations.append(last)
        with history_path.open("a", encoding="utf-8") as stream:
            stream.write(json.dumps(last, separators=(",", ":")) + "\n")
            stream.flush(); os.fsync(stream.fileno())
        continuity = profile.resumed_frame_continuity(
            loaded["frame"], tick_zero["frame"], RESUME_TICKS, completed, RESUME_TICKS, final)
        result = {"status": "resumed", "profile": profile_name, "mission": mission,
                  "rom": str(rom), "elf": str(elf), "rom_sha256": auth["rom_sha256"],
                  "elf_sha256": auth["elf_sha256"],
                  "source_report": str(auth["report_path"]),
                  "source_report_sha256": auth["report_sha256"],
                  "source_sram": str(auth["source_sram"]),
                  "source_sram_sha256": auth["source_sram_sha256"],
                  "copied_sram": str(copied_save), "copied_sram_sha256": copied_hash,
                  "save_loaded_through": "ordinary HOME L input",
                  "restored_snapshot_exact": True,
                  "pause_to_play_transition_allowed": True,
                  "initial_home": initial, "initial_home_capture": home_capture,
                  "loaded_capture": loaded_capture,
                  "neutral_handoff_live_frames": continuity["handoff_live_frames"],
                  "requested_ticks": RESUME_TICKS, "completed_ticks": completed,
                  "final_frame": final["frame"], "mean_live_play_effective_fps": fps,
                  "timer_method": "GBA chained hardware timer across the 60 scheduled updates",
                  "continuity": continuity, "checkpoint_history": str(history_path),
                  "captures": {"home": str(run_dir / "home.png"),
                               "loaded": str(run_dir / "loaded.png"),
                               "tick_zero": str(run_dir / "tick-000000.png"),
                               "final": str(run_dir / "tick-000060.png")},
                  "checkpoints": observations, "final": last}
        result_path = run_dir / "resume.json"
        result_path.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
        profile.write_progress(progress_path, profile=f"{profile_name}-resume", rom=rom, elf=elf,
                               rom_sha256=auth["rom_sha256"], elf_sha256=auth["elf_sha256"],
                               configuration={"profile": profile_name, "mission": mission,
                                              "source_report": str(auth["report_path"]),
                                              "source_report_sha256": auth["report_sha256"],
                                              "source_sram_sha256": auth["source_sram_sha256"],
                                              "copied_sram_sha256": copied_hash,
                                              "scheduler_mode": mode, "ticks": RESUME_TICKS},
                               requested_ticks=RESUME_TICKS, checkpoint_interval=RESUME_TICKS,
                               latest_checkpoint=last, status="finished", final_report=str(result_path))
        print(json.dumps({"status": result["status"], "mission": mission,
                          "ticks": completed, "frame": final["frame"],
                          "fps": fps, "report": str(result_path)}, indent=2), flush=True)
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


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--report", type=Path, required=True,
                        help="Z4/P4 controller replay report containing save_checkpoint")
    parser.add_argument("--rom", type=Path, required=True)
    parser.add_argument("--elf", type=Path, required=True)
    parser.add_argument("--output", type=Path, default=ROOT / "build/emulator/zp-resume-smoke")
    parser.add_argument("--expect-rom-sha256")
    parser.add_argument("--expect-elf-sha256")
    args = parser.parse_args(argv)
    try:
        run(args.report, args.rom, args.elf, args.output,
            args.expect_rom_sha256, args.expect_elf_sha256)
    except (OSError, ValueError, RuntimeError, json.JSONDecodeError) as exc:
        print(f"Z/P resume smoke failed: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
