#!/usr/bin/env python3
"""Run a bounded, normal-input Protoss 6 startup performance diagnostic.

This measures the first live P6 interval only. It is not a controller replay,
campaign completion, save-continuity, or mission-win qualification. The runner
injects only the normal 12-input HOME/menu route, then leaves the game neutral.
All campaign/world data is read through ELF symbols and DWARF.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import re
import socket
import struct
import subprocess
import sys
import time
from pathlib import Path

TOOLS = Path(__file__).resolve().parent
if str(TOOLS) not in sys.path:
    sys.path.insert(0, str(TOOLS))

import sc64_menu_smoke as menu
import sc64_races_profile as races
import sc64_split_profile as profile
import sc64_split_smoke as split
import sc64_zp_controller_replay as replay
from emulator_smoke import ROOT, capture, require_mgba, symbol_sizes, symbols

P6_PROFILE = "protoss-p6"
P6_FACTION = "protoss"
P6_MISSION = 25
SCREEN_TITLE, SCREEN_PLAY = 0, 1
DEFAULT_OUT = ROOT / ".local/build/promotion/p6-startup"
DEFAULT_LIVE_FRAMES = 1000
DEFAULT_CHECKPOINT = 1000
MAX_LIVE_FRAMES = 10000
MAX_U32 = 0xFFFFFFFF
SHA256_RE = re.compile(r"^[0-9a-fA-F]{64}$")

P6_REQUIRED_SYMBOLS = (
    "game_update", "demo_schedule_events", "demo_schedule_count",
    "demo_schedule_length", "demo_schedule_tick", "demo_replay_checkpoint",
    "screen", "frame", "won", "lost", "race_mode", "race_selected_mission",
    "race_completed", "race_campaign", "race_minerals", "race_gas", "race_front",
    "u", "b", "race_p6", "demo_update_cycles", "gfx_draw_cycles",
    "sc64_orders_cycles", "sc64_other_cycles",
)


def validate_live_frames(value: int) -> int:
    if type(value) is not int or not 1 <= value <= MAX_LIVE_FRAMES:
        raise ValueError(f"live frames must be an integer in [1,{MAX_LIVE_FRAMES}]")
    return value


def validate_checkpoint(value: int) -> int:
    if type(value) is not int or value not in (100, 1000):
        raise ValueError("checkpoint interval must be 100 or 1000 frames")
    return value


def validate_expected_hash(value: str | None, label: str) -> str | None:
    if value is None:
        return None
    if not isinstance(value, str) or not SHA256_RE.fullmatch(value):
        raise ValueError(f"expected {label} SHA-256 must be 64 hexadecimal characters")
    return value.lower()


def startup_input_events(capacity: int) -> list[tuple[int, int]]:
    """Return exactly the authenticated sixth-entry input prefix (tick-zero based)."""
    if type(capacity) is not int or capacity < len(replay.SIXTH_MENU_PREFIX):
        raise ValueError("ELF scheduler capacity is too small for the 12-input P6 menu route")
    expected = (
        (1, 1), (2, 0), (3, 16), (4, 0), (5, 16), (6, 0),
        (7, 128), (8, 0), (9, 1), (10, 0), (11, 1), (12, 0),
    )
    if tuple(replay.SIXTH_MENU_PREFIX) != expected:
        raise RuntimeError("P6 menu route constant differs from the authenticated 12-input prefix")
    return [(tick - 1, held) for tick, held in expected]


def startup_checkpoint_ticks(start_tick: int, live_frames: int,
                             interval: int) -> list[int]:
    validate_live_frames(live_frames)
    validate_checkpoint(interval)
    if type(start_tick) is not int or not 1 <= start_tick <= MAX_U32:
        raise ValueError("startup handoff tick must be a positive 32-bit integer")
    stop = start_tick + live_frames
    if stop > MAX_U32:
        raise ValueError("startup diagnostic would exceed the 32-bit scheduler tick")
    checkpoints = set(range(start_tick + interval, stop + 1, interval))
    checkpoints.update(range(((start_tick // 1000) + 1) * 1000, stop + 1, 1000))
    checkpoints.add(stop)
    return sorted(checkpoints)


def verify_profile_contract(metadata: dict, sym: dict[str, int], sizes: dict[str, int]) -> dict:
    if not isinstance(metadata, dict) or metadata.get("profile") != P6_FACTION:
        raise ValueError("ROM/ELF catalog metadata must identify the Protoss profile")
    records = metadata.get("records")
    p6 = next((row for row in records if isinstance(row, dict) and row.get("id") == P6_MISSION), None) \
        if isinstance(records, list) else None
    if not p6 or p6.get("faction") != 2 or p6.get("playable") != 1:
        raise ValueError("matching catalog must enable playable Protoss P6 at global mission ID 25")
    enabled = metadata.get("enabled_mission_ids")
    if not isinstance(enabled, list) or P6_MISSION not in enabled:
        raise ValueError("Protoss P6 global mission ID 25 is not enabled in the ROM catalog")
    missing = [name for name in P6_REQUIRED_SYMBOLS if name not in sym]
    if missing:
        raise RuntimeError("P6 ELF is missing required symbols: " + ", ".join(missing))
    if sizes.get("demo_schedule_events", 0) <= 0 or sizes["demo_schedule_events"] % 8:
        raise RuntimeError("P6 ELF has an invalid controller event queue size")
    mode, capacity = replay.scheduler.schedule_mode(sym, sizes)
    if mode == "windowed":
        for name in ("demo_schedule_active", "demo_schedule_index"):
            if name not in sym:
                raise RuntimeError(f"windowed P6 ELF is missing scheduler symbol {name}")
    return {"profile": P6_PROFILE, "mission": P6_MISSION,
            "scheduler_mode": mode, "scheduler_capacity": capacity,
            "catalog_profile": metadata["profile"],
            "catalog_mission": p6}


def validate_p6_live_state(state: dict, *, expected_screen: int = SCREEN_PLAY) -> None:
    if not isinstance(state, dict) or state.get("screen") != expected_screen:
        raise RuntimeError("P6 startup left the expected screen")
    if (state.get("race_mode") != 0 or state.get("race_selected_mission") != P6_MISSION or
            state.get("won") != 0 or state.get("lost") != 0):
        raise RuntimeError("normal-input startup did not enter a live, unfinished Protoss P6")
    runtime = state.get("race_runtime")
    if not isinstance(runtime, dict) or runtime.get("fault") != 0:
        raise RuntimeError("P6 runtime snapshot is missing or reports a fault")
    model = runtime.get("model")
    if not isinstance(model, dict) or model.get("outcome") != 0 or \
            model.get("fired") != model.get("started"):
        raise RuntimeError("P6 startup model state is invalid or terminal")
    started = model.get("started")
    cursors, ticks = model.get("action_cursor"), runtime.get("action_ticks")
    if (type(started) is not int or not 0 <= started < (1 << 26) or
            not isinstance(cursors, list) or len(cursors) != 26 or
            not isinstance(ticks, list) or len(ticks) != 26):
        raise RuntimeError("P6 startup trigger state has an invalid shape")
    for trigger, (cursor, timer) in enumerate(zip(cursors, ticks)):
        if (type(cursor) is not int or type(timer) is not int or
                not 0 <= cursor <= replay.P6_ACTION_COUNTS[trigger] or
                not 0 <= timer <= 65535 or
                (cursor and not (started & (1 << trigger))) or
                ((cursor != 0) != (timer != 0))):
            raise RuntimeError("P6 startup trigger cursor/timer exceeds source bounds")


def compare_startup_reports(baseline: dict, candidate: dict) -> dict:
    """Compare exact state and PNG digests at matching startup checkpoints."""
    for report in (baseline, candidate):
        _validate_report_header(report)
    if (baseline.get("live_frames") != candidate.get("live_frames") or
            baseline.get("checkpoint_interval") != candidate.get("checkpoint_interval")):
        raise ValueError("startup reports have different measurement windows")
    left, right = baseline.get("checkpoints"), candidate.get("checkpoints")
    if not isinstance(left, list) or not isinstance(right, list) or len(left) != len(right):
        raise ValueError("startup reports have different checkpoint lists")
    handoff_state_equal = baseline.get("start_state") == candidate.get("start_state")
    handoff_capture_equal = (baseline.get("start_capture_sha256") ==
                             candidate.get("start_capture_sha256"))
    rows = []
    for a, b in zip(left, right):
        if not isinstance(a, dict) or not isinstance(b, dict):
            raise ValueError("startup checkpoint entries must be objects")
        if a.get("live_frame") != b.get("live_frame") or a.get("scheduler_tick") != b.get("scheduler_tick"):
            raise ValueError("startup report checkpoint ticks do not align")
        state_equal = a.get("state") == b.get("state")
        capture_equal = a.get("capture_sha256") == b.get("capture_sha256")
        rows.append({"scheduler_tick": a["scheduler_tick"],
                     "live_frame": a["live_frame"],
                     "state_equal": state_equal,
                     "capture_equal": capture_equal})
    return {"exact_state_match": handoff_state_equal and all(row["state_equal"] for row in rows),
            "exact_capture_match": handoff_capture_equal and all(row["capture_equal"] for row in rows),
            "handoff_state_equal": handoff_state_equal,
            "handoff_capture_equal": handoff_capture_equal,
            "checkpoints": rows,
            "baseline_rom_sha256": baseline.get("rom_sha256"),
            "candidate_rom_sha256": candidate.get("rom_sha256"),
            "baseline_elf_sha256": baseline.get("elf_sha256"),
            "candidate_elf_sha256": candidate.get("elf_sha256")}


def _validate_report_header(report: object) -> dict:
    if (not isinstance(report, dict) or report.get("schema") != 1 or
            report.get("kind") != "p6-startup-diagnostic" or
            report.get("profile") != P6_PROFILE or report.get("mission") != P6_MISSION or
            report.get("qualification") != "startup-only"):
        raise ValueError("comparison inputs must be schema-1 P6 startup-only reports")
    for name in ("live_frames", "checkpoint_interval"):
        if type(report.get(name)) is not int:
            raise ValueError(f"startup report is missing integer {name}")
    validate_live_frames(report["live_frames"])
    validate_checkpoint(report["checkpoint_interval"])
    if not isinstance(report.get("checkpoints"), list):
        raise ValueError("startup report checkpoints must be an array")
    return report


def _start_emulator(name: str, rom: Path, out: Path, sym: dict[str, int]):
    mgba = require_mgba()
    save_dir = out / "saves" / name
    save_dir.mkdir(parents=True, exist_ok=True)
    log = (out / f"{name}.log").open("ab")
    proc = subprocess.Popen(
        [mgba, "-g", "-C", "mute=1", "-C", "audioSync=0", "-C", "videoSync=0",
         "-C", "pauseOnFocusLost=0", "-C", "pauseOnMinimize=0", "-C", "fpsTarget=600",
         "-C", f"savegamePath={save_dir}", str(rom)], stdout=log, stderr=log)
    sock = None
    try:
        deadline = time.monotonic() + 12
        while time.monotonic() < deadline:
            try:
                sock = socket.create_connection(("127.0.0.1", 2345), timeout=1)
                break
            except OSError:
                time.sleep(0.1)
        if sock is None:
            raise RuntimeError("mGBA GDB server did not start")
        sock.settimeout(60)
        sock.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)
        from emulator_smoke import RSP
        rsp = RSP(sock)
        rsp.cmd("qSupported")
        rsp.cmd("?")
        if rsp.cmd(f"Z1,{sym['game_update'] & ~1:x},2") != "OK":
            raise RuntimeError("failed to install initial game_update breakpoint")
        rsp.tick(2)
        return proc, sock, rsp, log
    except BaseException:
        if sock:
            sock.close()
        proc.terminate()
        proc.wait(timeout=3)
        log.close()
        raise


def run(rom_path: Path, elf_path: Path, out: Path = DEFAULT_OUT,
        live_frames: int = DEFAULT_LIVE_FRAMES, checkpoint: int = DEFAULT_CHECKPOINT,
        expected_rom_sha256: str | None = None,
        expected_elf_sha256: str | None = None,
        compare_path: Path | None = None) -> dict:
    live_frames = validate_live_frames(live_frames)
    checkpoint = validate_checkpoint(checkpoint)
    expected_rom_sha256 = validate_expected_hash(expected_rom_sha256, "ROM")
    expected_elf_sha256 = validate_expected_hash(expected_elf_sha256, "ELF")
    baseline_report = None
    if compare_path:
        try:
            baseline_report = _validate_report_header(
                json.loads(compare_path.read_text(encoding="utf-8")))
        except (OSError, json.JSONDecodeError) as exc:
            raise ValueError(f"cannot read startup comparison report: {exc}") from exc
        if (baseline_report["live_frames"] != live_frames or
                baseline_report["checkpoint_interval"] != checkpoint):
            raise ValueError("startup comparison report has a different measurement window")
        expected_ticks = startup_checkpoint_ticks(12, live_frames, checkpoint)
        baseline_rows = baseline_report["checkpoints"]
        if (len(baseline_rows) != len(expected_ticks) or
                any(not isinstance(row, dict) or row.get("scheduler_tick") != tick
                    for row, tick in zip(baseline_rows, expected_ticks))):
            raise ValueError("startup comparison report has a different checkpoint schedule")
    if not rom_path.is_file() or not elf_path.is_file():
        raise ValueError("both ROM and matching ELF must be regular files")
    rom_hash = hashlib.sha256(rom_path.read_bytes()).hexdigest()
    elf_hash = hashlib.sha256(elf_path.read_bytes()).hexdigest()
    for label, expected, actual in (("ROM", expected_rom_sha256, rom_hash),
                                    ("ELF", expected_elf_sha256, elf_hash)):
        if expected and expected != actual:
            raise RuntimeError(f"{label} SHA-256 mismatch: expected {expected}, observed {actual}")

    sym, sizes = symbols(elf_path), symbol_sizes(elf_path)
    metadata = split.read_campaign_metadata(rom_path, elf_path, P6_FACTION)
    contract = verify_profile_contract(metadata, sym, sizes)
    bank = replay.compact_bank_profile(elf_path, sizes, P6_PROFILE)
    campaign_layout = races.dwarf_struct(elf_path, "Sc64RaceCampaignState")
    runtime_layout = replay.runtime_layout_for(elf_path, P6_PROFILE)
    model_layout = races.dwarf_struct(elf_path, replay.MODEL_TYPES[P6_PROFILE])
    frontend = split.frontend_offsets(elf_path)

    scheduler_mode, capacity = contract["scheduler_mode"], contract["scheduler_capacity"]
    events = startup_input_events(capacity)
    run_dir = out / f"protoss-p6-startup-{time.time_ns()}"
    run_dir.mkdir(parents=True, exist_ok=False)
    split.FRONT_FIELDS = frontend
    profile.check_debugger_port_free()
    proc = sock = rsp = log = None
    try:
        proc, sock, rsp, log = _start_emulator(run_dir.name, rom_path, run_dir, sym)
        initial = {"screen": profile.u32(rsp, sym["screen"]),
                   "frame": profile.u32(rsp, sym["frame"]),
                   "page": profile.u32(rsp, sym["race_front"] + frontend["page"])}
        if initial != {"screen": SCREEN_TITLE, "frame": 0, "page": split.FRONT_HOME}:
            raise RuntimeError(f"fresh P6 boot did not remain on HOME: {initial}")
        home_capture = capture(rsp, run_dir / "home.png")

        # The first scheduler observer runs before the first scheduled input.
        if rsp.cmd(f"z1,{sym['game_update'] & ~1:x},2") != "OK" or rsp.cmd(
                f"Z1,{sym['demo_replay_checkpoint'] & ~1:x},2") != "OK":
            raise RuntimeError("failed to replace startup breakpoints with scheduler observer")
        if scheduler_mode == "windowed":
            rsp.write(sym["demo_schedule_active"], struct.pack("<I", 1))
            rsp.write(sym["demo_schedule_index"], struct.pack("<I", 0))
        _write_events(rsp, sym, events)
        rsp.write(sym["demo_schedule_length"], struct.pack("<I", len(replay.SIXTH_MENU_PREFIX)))
        rsp.tick()
        if profile.u32(rsp, sym["demo_schedule_tick"]) != 0:
            raise RuntimeError("scheduler advanced before the first normal menu input")
        pre_start = {"screen": profile.u32(rsp, sym["screen"]),
                     "frame": profile.u32(rsp, sym["frame"]),
                     "page": profile.u32(rsp, sym["race_front"] + frontend["page"])}
        if pre_start != initial:
            raise RuntimeError("P6 startup left HOME before processing the normal menu route")

        # Stop exactly at the real PLAY handoff after the 12 ordinary inputs.
        rsp.write(sym["demo_schedule_length"], struct.pack("<I", 12))
        rsp.tick()
        start_tick = profile.u32(rsp, sym["demo_schedule_tick"])
        if start_tick != 12:
            raise RuntimeError(f"P6 menu route stopped at scheduler tick {start_tick}, expected 12")
        start_state = replay.mission_snapshot(rsp, sym, sizes, bank, campaign_layout,
                                              runtime_layout, model_layout, P6_PROFILE)
        validate_p6_live_state(start_state)
        if start_state["frame"] != 1:
            raise RuntimeError("P6 normal-input handoff frame differs from the expected first live frame")
        start_capture_path = run_dir / "live-start.png"
        start_capture = capture(rsp, start_capture_path)
        start_capture_hash = hashlib.sha256(start_capture_path.read_bytes()).hexdigest()

        stop_tick = start_tick + live_frames
        target_ticks = startup_checkpoint_ticks(start_tick, live_frames, checkpoint)
        checkpoints = []
        prior_state = start_state
        prior_clock = profile.hardware_clock(rsp)
        for target_tick in target_ticks:
            rsp.write(sym["demo_schedule_length"], struct.pack("<I", target_tick))
            rsp.tick()
            completed = profile.u32(rsp, sym["demo_schedule_tick"])
            if completed != target_tick:
                raise RuntimeError(f"expected startup checkpoint {target_tick}, observed {completed}")
            state = replay.mission_snapshot(rsp, sym, sizes, bank, campaign_layout,
                                            runtime_layout, model_layout, P6_PROFILE)
            if state["screen"] != SCREEN_PLAY:
                raise RuntimeError(f"P6 left live PLAY during startup profiling at tick {completed}")
            validate_p6_live_state(state)
            now = profile.hardware_clock(rsp)
            elapsed = (now - prior_clock) & MAX_U32
            frame_delta = (state["frame"] - prior_state["frame"]) & MAX_U32
            fps = round(profile.CPU_HZ * frame_delta / max(1, elapsed), 2)
            stage_cycles, routes = replay.scheduler.profile_counters(rsp, sym)
            live_frame = completed - start_tick
            image_path = run_dir / f"live-{live_frame:06d}.png"
            image_info = capture(rsp, image_path)
            image_hash = hashlib.sha256(image_path.read_bytes()).hexdigest()
            row = {"scheduler_tick": completed, "live_frame": live_frame,
                   "elapsed_hardware_cycles": elapsed, "frame_delta": frame_delta,
                   "effective_fps": fps, "last_tick_stage_cycles": stage_cycles,
                   "route_counters": routes, "state": state,
                   "capture": str(image_path), "capture_sha256": image_hash,
                   "capture_info": image_info}
            checkpoints.append(row)
            print(f"P6 startup live={live_frame}/{live_frames} tick={completed} "
                  f"fps={fps:.2f} update={stage_cycles.get('demo_update_cycles', 0)} "
                  f"draw={stage_cycles.get('gfx_draw_cycles', 0)}", flush=True)
            if state["won"] or state["lost"] or state["race_runtime"]["model"]["outcome"]:
                raise RuntimeError("startup diagnostic observed terminal gameplay; refusing win qualification")
            prior_state, prior_clock = state, now

        elapsed_cycles = sum(row["elapsed_hardware_cycles"] for row in checkpoints)
        frames_counted = sum(row["frame_delta"] for row in checkpoints)
        report = {
            "schema": 1,
            "kind": "p6-startup-diagnostic",
            "qualification": "startup-only",
            "profile": P6_PROFILE,
            "mission": P6_MISSION,
            "rom": str(rom_path.resolve()), "elf": str(elf_path.resolve()),
            "rom_sha256": rom_hash, "elf_sha256": elf_hash,
            "campaign_catalog": metadata, "profile_contract": contract,
            "scheduler_mode": scheduler_mode, "scheduler_capacity": capacity,
            "normal_menu_inputs": [{"scheduler_tick": tick, "held": held}
                                    for tick, held in events],
            "live_frames": live_frames, "checkpoint_interval": checkpoint,
            "start_scheduler_tick": start_tick, "stop_scheduler_tick": stop_tick,
            "start_state": start_state,
            "start_capture": str(start_capture_path),
            "start_capture_sha256": start_capture_hash,
            "start_capture_info": start_capture,
            "counted_live_frames": frames_counted,
            "counted_live_hardware_cycles": elapsed_cycles,
            "mean_live_effective_fps": round(profile.CPU_HZ * frames_counted /
                                                max(1, elapsed_cycles), 2),
            "timer_method": "GBA chained hardware timer deltas sampled at scheduler checkpoints",
            "mGBA_config": {"pauseOnFocusLost": 0, "pauseOnMinimize": 0,
                            "audioSync": 0, "videoSync": 0, "fpsTarget": 600},
            "checkpoints": checkpoints,
        }
        if baseline_report:
            report["comparison"] = compare_startup_reports(baseline_report, report)
        report_path = run_dir / "profile.json"
        report_path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
        print(json.dumps({"status": "startup-only", "live_frames": live_frames,
                          "mean_live_effective_fps": report["mean_live_effective_fps"],
                          "report": str(report_path)}, indent=2), flush=True)
        return report
    finally:
        if proc is not None:
            try:
                menu.stop_emulator(proc, sock, log)
            except Exception:
                proc.terminate()
                proc.wait(timeout=3)


def _write_events(rsp, sym: dict[str, int], events: list[tuple[int, int]]) -> None:
    blob = b"".join(struct.pack("<2I", tick, held) for tick, held in events)
    if not blob:
        raise ValueError("startup input route cannot be empty")
    for offset in range(0, len(blob), 256):
        rsp.write(sym["demo_schedule_events"] + offset, blob[offset:offset + 256])
    rsp.write(sym["demo_schedule_count"], struct.pack("<I", len(events)))


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rom", type=Path, required=True, help="P6 GBA ROM")
    parser.add_argument("--elf", type=Path, required=True, help="matching P6 ELF with symbols and DWARF")
    parser.add_argument("--out", type=Path, default=DEFAULT_OUT, help="diagnostic output root")
    parser.add_argument("--live-frames", type=int, default=DEFAULT_LIVE_FRAMES,
                        help=f"neutral live frames after menu launch (1..{MAX_LIVE_FRAMES})")
    parser.add_argument("--checkpoint", type=int, choices=(100, 1000), default=DEFAULT_CHECKPOINT,
                        help="bounded checkpoint interval in live frames")
    parser.add_argument("--expect-rom-sha256", help="optional exact ROM digest")
    parser.add_argument("--expect-elf-sha256", help="optional exact ELF digest")
    parser.add_argument("--compare", type=Path,
                        help="compare exact state/capture hashes with an earlier startup report")
    return parser


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    try:
        run(args.rom, args.elf, args.out, args.live_frames, args.checkpoint,
            args.expect_rom_sha256, args.expect_elf_sha256, args.compare)
    except (OSError, ValueError, RuntimeError, AssertionError,
            json.JSONDecodeError, subprocess.CalledProcessError) as exc:
        parser.exit(1, f"sc64_zp_start_profile: {exc}\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
